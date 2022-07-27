#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>

static struct Taskstate ts;

void divide_0();
void debug_1();
void breakpoint_3();
void overflow_4();
void bound_5();
void invalid_6();
void device_7();
void floatpoint_16();
void machine_18();
void foatponit_19();
void systemcall_48();
void ddfault_8();
void invalid_10();
void segmen_11();
void stack_12();
void gprotection_13();
void fpage_14();
void checkalain_17();

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case.
 */
static struct Trapframe *last_tf;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = { sizeof(idt) - 1, (uint32_t) idt };


static const char *
trapname(int trapno)
{
	static const char *const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < ARRAY_SIZE(excnames))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	return "(unknown trap)";
}


void
trap_init(void)
{
#define RING_KERNEL 0
#define RING_USER 3
#define INTERRUPT_GATE 0
#define TRAP_GATE 1
	extern struct Segdesc gdt[];

	// LAB 3: Your code here.


	SETGATE(idt[T_DIVIDE], INTERRUPT_GATE, GD_KT, divide_0, RING_KERNEL);
	SETGATE(idt[T_DEBUG], INTERRUPT_GATE, GD_KT, debug_1, RING_KERNEL);
	SETGATE(idt[T_BRKPT], INTERRUPT_GATE, GD_KT, breakpoint_3, RING_USER);
	SETGATE(idt[T_OFLOW], INTERRUPT_GATE, GD_KT, overflow_4, RING_KERNEL);
	SETGATE(idt[T_BOUND], INTERRUPT_GATE, GD_KT, bound_5, RING_KERNEL);
	SETGATE(idt[T_ILLOP], INTERRUPT_GATE, GD_KT, invalid_6, RING_KERNEL);
	SETGATE(idt[T_DEVICE], INTERRUPT_GATE, GD_KT, device_7, RING_KERNEL);
	SETGATE(idt[T_FPERR], INTERRUPT_GATE, GD_KT, floatpoint_16, RING_KERNEL);
	SETGATE(idt[T_MCHK], INTERRUPT_GATE, GD_KT, machine_18, RING_KERNEL);
	SETGATE(idt[T_SIMDERR], INTERRUPT_GATE, GD_KT, foatponit_19, RING_KERNEL);
	SETGATE(idt[T_SYSCALL], INTERRUPT_GATE, GD_KT, systemcall_48, RING_USER);
	SETGATE(idt[T_DBLFLT], INTERRUPT_GATE, GD_KT, ddfault_8, RING_KERNEL);
	SETGATE(idt[T_TSS], INTERRUPT_GATE, GD_KT, invalid_10, RING_KERNEL);
	SETGATE(idt[T_SEGNP], INTERRUPT_GATE, GD_KT, segmen_11, RING_KERNEL);
	SETGATE(idt[T_STACK], INTERRUPT_GATE, GD_KT, stack_12, RING_KERNEL);
	SETGATE(idt[T_GPFLT], INTERRUPT_GATE, GD_KT, gprotection_13, RING_KERNEL);
	SETGATE(idt[T_PGFLT], INTERRUPT_GATE, GD_KT, fpage_14, RING_KERNEL);
	SETGATE(idt[T_ALIGN], INTERRUPT_GATE, GD_KT, checkalain_17, RING_KERNEL);


	// Per-CPU setup
	trap_init_percpu();
}

// Initialize and load the per-CPU TSS and IDT
void
trap_init_percpu(void)
{
	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = GD_KD;

	// Initialize the TSS slot of the gdt.
	gdt[GD_TSS0 >> 3] =
	        SEG16(STS_T32A, (uint32_t)(&ts), sizeof(struct Taskstate) - 1, 0);
	gdt[GD_TSS0 >> 3].sd_s = 0;

	// Load the TSS selector (like other segment selectors, the
	// bottom three bits are special; we leave them 0)
	ltr(GD_TSS0);

	// Load the IDT
	lidt(&idt_pd);
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	if (tf == last_tf && tf->tf_trapno == T_PGFLT)
		cprintf("  cr2  0x%08x\n", rcr2());
	cprintf("  err  0x%08x", tf->tf_err);
	// For page faults, print decoded fault error code:
	// U/K=fault occurred in user/kernel mode
	// W/R=a write/read caused the fault
	// PR=a protection violation caused the fault (NP=page not present).
	if (tf->tf_trapno == T_PGFLT)
		cprintf(" [%s, %s, %s]\n",
		        tf->tf_err & 4 ? "user" : "kernel",
		        tf->tf_err & 2 ? "write" : "read",
		        tf->tf_err & 1 ? "protection" : "not-present");
	else
		cprintf("\n");
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	if ((tf->tf_cs & 3) != 0) {
		cprintf("  esp  0x%08x\n", tf->tf_esp);
		cprintf("  ss   0x----%04x\n", tf->tf_ss);
	}
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

static void
trap_dispatch(struct Trapframe *tf)
{
	// Handle processor exceptions.
	// LAB 3: Your code here.

	if (tf->tf_trapno == T_PGFLT) {
		page_fault_handler(tf);
	} else if (tf->tf_trapno == T_BRKPT) {
		monitor(tf);
	} else if (tf->tf_trapno == T_SYSCALL) {
		tf->tf_regs.reg_eax = syscall(tf->tf_regs.reg_eax,
		                              tf->tf_regs.reg_edx,
		                              tf->tf_regs.reg_ecx,
		                              tf->tf_regs.reg_ebx,
		                              tf->tf_regs.reg_edi,
		                              tf->tf_regs.reg_esi);
		return;
	} else {
		// Unexpected trap: The user process or the kernel has a bug.
		print_trapframe(tf);
		if (tf->tf_cs == GD_KT)
			panic("unhandled trap in kernel");
		else {
			env_destroy(curenv);
		}
	}
	return;
}

void
trap(struct Trapframe *tf)
{
	// The environment may have set DF and some versions
	// of GCC rely on DF being clear
	asm volatile("cld" ::: "cc");

	// Check that interrupts are disabled.  If this assertion
	// fails, DO NOT be tempted to fix it by inserting a "cli" in
	// the interrupt path.
	assert(!(read_eflags() & FL_IF));

	cprintf("Incoming TRAP frame at %p\n", tf);

	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		assert(curenv);

		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}

	// Record that tf is the last real trapframe so
	// print_trapframe can print some additional information.
	last_tf = tf;

	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// Return to the current environment, which should be running.
	assert(curenv && curenv->env_status == ENV_RUNNING);
	env_run(curenv);
}


void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

	// Handle kernel-mode page faults.

	// LAB 3: Your code here.

	if ((tf->tf_cs & 3) == 0) {
		panic("page_fault_handler in kernel mode (ring 0)");
	}

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
	        curenv->env_id,
	        fault_va,
	        tf->tf_eip);
	print_trapframe(tf);
	env_destroy(curenv);
}
