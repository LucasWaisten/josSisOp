// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW 0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	pte_t pte = uvpt[PGNUM(addr)];

	if (((pte & PTE_COW) == 0) || ((err & FEC_WR) == 0)) {
		panic("Error: [fork.c]--> [pgfault]-->copy-on-write");
	}
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);

	if (sys_page_alloc(0, PFTEMP, PTE_P | PTE_W | PTE_U) < 0) {
		panic("Error: [fork.c]-->[pgfault]-->page alloc");
	}

	memcpy(PFTEMP, addr, PGSIZE);

	if (sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_W | PTE_U) < 0) {
		panic("Error: [fork.c]-->[pgfault]-->mage map");
	}
	if (sys_page_unmap(0, PFTEMP) < 0) {
		panic("Error:[fork.c]-->[pgfault]-->page un map");
	}

	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// int r;

	// LAB 4: Your code here.
	void *addr = (void *) (pn * PGSIZE);  // Virtual address

	if ((uvpt[pn] & PTE_W) ||
	    (uvpt[pn] & PTE_COW)) {  // Verifico si es writable o copy-on-write

		if (sys_page_map(0, addr, envid, addr, PTE_COW | PTE_U | PTE_P) <
		    0) {
			panic("Error: [fork.c]-->[duppage()]--> sys_page_map.");
		}

		if (sys_page_map(0, addr, 0, addr, PTE_COW | PTE_U | PTE_P) < 0) {
			panic("Error: [fork.c]-->[duppage()]-->sys_page_map.");
		}

	} else {  // Caso: solo lectura la compartimos

		if (sys_page_map(0, addr, envid, addr, PTE_U | PTE_P) < 0) {
			panic("Error: [fork.c]-->[duppage()]-->sys_page_map.");
		}
	}
	// panic("duppage not implemented");
	return 0;
}

static void
dup_or_share(envid_t dstenv, void *va, int perm)
{
	if (PTE_W & perm) {  // página de escritura --> se comporta como duppage

		if ((sys_page_alloc(dstenv, va, PTE_P | PTE_U | PTE_W)) < 0) {
			panic("[fork.c] --> sys_page_alloc error");
		}

		if ((sys_page_map(dstenv, va, 0, UTEMP, PTE_P | PTE_U | PTE_W)) <
		    0) {
			panic("[fork.c] --> sys_page_map error");
		}
		memmove(UTEMP, va, PGSIZE);

		if ((sys_page_unmap(0, UTEMP)) < 0) {
			panic("[fork.c] --> sys_page_unmap error");
		}

	} else {  // página de lectura --> se comparte
		if (sys_page_map(dstenv, va, 0, UTEMP, PTE_P | PTE_U) < 0) {
			panic("[fork.c] --> sys_page_map error");
		}
	}
}

envid_t
fork_v0(void)
{
	// Código tp3
	envid_t envid;
	uintptr_t addr;

	// Como en dumbfork
	envid = sys_exofork();
	if (envid < 0)
		panic("[fork.c] --> sys_exofork: %e", envid);
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	for (addr = 0; addr < UTOP; addr += PGSIZE) {
		pte_t pde = uvpd[PDX(addr)];  // me devuelve el pde
		if (pde & PTE_P) {
			pte_t pte = uvpt[PGNUM(addr)];  // devuelve el pte
			if (pte & PTE_P) {
				dup_or_share(envid,
				             (void *) addr,
				             pte & PTE_SYSCALL);
			}
		}
	}

	if (sys_env_set_status(envid, ENV_RUNNABLE) < 0) {
		panic("[fork.c] --> hijo runnable");
	}

	return envid;
}
//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// return fork_v0();
	set_pgfault_handler(pgfault);

	uintptr_t addr;

	envid_t envid = sys_exofork();

	if (envid < 0) {  // Error en fork
		panic("Error: [fork.c]-->[fork()]--> sys_exofork");

	} else if (envid == 0) {  // Caso hijo
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
		;
	}


	// Caso padre
	for (addr = 0; addr < USTACKTOP; addr += PGSIZE) {
		pte_t pde = uvpd[PDX(addr)];  // Me devuelve el pde
		if (pde & PTE_P && pde & PTE_U) {
			pte_t pte = uvpt[PGNUM(addr)];  // Me devuelve el pte
			if (pte & PTE_P && pte & PTE_U) {
				duppage(envid, PGNUM(addr));
			}
		}
	}

	if (sys_page_alloc(envid,
	                   (void *) (UXSTACKTOP - PGSIZE),
	                   PTE_P | PTE_U | PTE_W) < 0) {
		panic("Error: [fork.c]-->[fork()]-->sys_page_alloc");
	}

	void _pgfault_upcall();

	if (sys_env_set_pgfault_upcall(envid, _pgfault_upcall) < 0) {
		panic("Error: "
		      "[fork.c]-->[fork()]-->sys_env_set_pgfault_upcall");
	}

	if (sys_env_set_status(envid, ENV_RUNNABLE) < 0) {
		panic("Error: [fork.c]-->[fork()]-->sys_env_set_status");
	}

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
