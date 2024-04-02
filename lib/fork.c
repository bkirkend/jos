// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	if (utf == NULL) panic("invalid utf");
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!((err & FEC_WR) && (uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_COW))) {
		panic("In pgfault, not a copy-on-write page fault");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	addr = ROUNDDOWN(addr, PGSIZE);
	// Allocate a new page, map it at a temporary location (PFTEMP)
	if((r = sys_page_alloc(0, (void*)PFTEMP, PTE_P | PTE_U | PTE_W)) < 0){
		panic("sys_page_alloc failed");
	}
	// copy the data from the old page to the new page
	memmove((void*) PFTEMP, (void*)PTE_ADDR(addr), PGSIZE);
	// move the new page to the old page's address
	if((r = sys_page_map(0, (void*)PFTEMP, 0, addr, PTE_P|PTE_U|PTE_W)) < 0){
		panic("sys_page_map failed");
	}
	if ((r = sys_page_unmap(0, (void*)PFTEMP)) < 0){
		panic("sys_page_unmap failed");
	}
	return;
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
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	//LAB 4: Your code here
    // Check for valid virtual address
    if (pn >= PGNUM(UTOP)) {
        panic("Invalid page number");
        return -1;
    }

    // Calculate the virtual address
    void *envid_addr = (void*)(pn * PGSIZE);

    // Check if the current environment has permission to perform page mapping
    if ((r = sys_page_map(0, envid_addr, envid, envid_addr, PTE_P | PTE_U)) < 0) {
        panic("sys_page_map failed");
        return r;
    }

    // If the page is writable or copy-on-write, handle accordingly
    if ((uvpt[pn] & PTE_COW) || (uvpt[pn] & PTE_W)) {
        // Map the page copy-on-write in the target environment
        if ((r = sys_page_map(0, envid_addr, envid, envid_addr, PTE_P | PTE_U | PTE_COW)) < 0) {
            panic("sys_page_map failed for COW target");
            return r;
        }

        // Map the page copy-on-write in the current environment
        if ((r = sys_page_map(0, envid_addr, 0, envid_addr, PTE_P | PTE_U | PTE_COW)) < 0) {
            panic("sys_page_map failed for COW current");
            return r;
        }
    } else {
        // If the page is not writable or copy-on-write, simply map it
        if ((r = sys_page_map(0, envid_addr, envid, envid_addr, PTE_P | PTE_U)) < 0) {
            panic("sys_page_map failed for read-only mapping");
            return r;
        }
    }

    return 0;
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

	// the parent installs pgfault() as the C-level page fault handler, using the set_pgfault_handler() function you implemented above.
	set_pgfault_handler(pgfault);

	// the parent calls sys_exofork() to create a child environment.
	envid_t child_env = sys_exofork();

	if(child_env < 0) {
		panic("in fork, creation of child environment failed");
	} else if(child_env == 0) {
		// Remember to fix "thisenv" in the child process.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	} else {
		// For each writable or copy-on-write page in its address space below UTOP,
		for(uint32_t i = 0; i < USTACKTOP; i += PGSIZE) {
			// the parent calls duppage, which should map the page copy-on-write into the 
			// address space of the child and then remap the page copy-on-write in its own
			// address space. duppage sets both PTEs so that the page is not writeable, and 
			// to contain PTE_COW in the “avail” field to distinguish copy-on-write pages from genuine read-only pages.
			// fork() also needs to handle pages that are present, but not writable or copy-on-write.
			if ((uvpd[PDX(i)] & PTE_P) && (uvpt[PGNUM(i)] & PTE_P) && (uvpt[PGNUM(i)] & PTE_U)) {
				duppage(child_env, PGNUM(i));
			}
		}

		// The exception stack is not remapped this way, however. Instead you need to allocate 
		// a fresh page in the child for the exception stack. Since the page fault handler will 
		// be doing the actual copying and the page fault handler runs on the exception stack, 
		// the exception stack cannot be made copy-on-write
		int page_ret = sys_page_alloc(child_env, (void *)(UXSTACKTOP - PGSIZE), PTE_P|PTE_U|PTE_W);
		if (page_ret < 0) {
			panic("In fork, couldn't allocate exception stack");
		}

		// The parent sets the user page fault entrypoint for the child to look like its own.
		extern void _pgfault_upcall();
		sys_env_set_pgfault_upcall(child_env, _pgfault_upcall);

		// The child is now ready to run, so the parent marks it runnable.
		sys_env_set_status(child_env, ENV_RUNNABLE);

		return child_env;
	}
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
