// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#include <kern/pmap.h>

#define RED   "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE  "\x1B[34m"
#define BLACK  "\x1B[30m"
#define RESET "\x1B[0m"

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

// LAB 1: add your command to here...
static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display the backtrace", mon_backtrace },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "showmappings", "Display physical page mappings and corresponding permission bits that apply to the pages at virtual address and change the permissions", mon_showmappings },
	{"setpermissions", "Modify the access permissions for the page containing the provided virtual address", mon_setperms },
	{"memdump", "Dump contents of a range of memory given either a virtual or physical address range", mon_memdump},
	{"si", "Single-step instructions at a breakpoint", mon_si},
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int mon_show(int argc, char **argv, struct Trapframe *tf)
{ 


    cprintf(BLUE);
    cprintf("%s", "                         ####");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "%");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "##########");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "%");
    cprintf(RESET);
	cprintf(BLUE);
    cprintf("%s", "####\n");
    cprintf(RESET);

	cprintf(BLUE);
    cprintf("%s", "                         ###");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "*");
    cprintf(RESET);
	cprintf(YELLOW);
    cprintf("%s", "**");
    cprintf(RESET);
	cprintf(BLACK);
    cprintf("%s", "#");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", ":::::");
    cprintf(RESET);
	cprintf(YELLOW);
    cprintf("%s", "***");
    cprintf(RESET);
	cprintf(BLACK);
    cprintf("%s", "#");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "*");
    cprintf(RESET);
	cprintf(BLUE);
    cprintf("%s", "###\n");
    cprintf(RESET);

	cprintf(BLUE);
    cprintf("%s", "                         ##");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "%");
    cprintf(RESET);
	cprintf(BLACK);
    cprintf("%s", "##");
    cprintf(RESET);
	cprintf(YELLOW);
    cprintf("%s", "*******");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "#");
    cprintf(RESET);
	cprintf(YELLOW);
    cprintf("%s", "**");
    cprintf(RESET);
	cprintf(GREEN);
    cprintf("%s", "=");
    cprintf(RESET);
	cprintf(BLACK);
    cprintf("%s", "#");
    cprintf(RESET);
	cprintf(RED);
	cprintf("%s", "%");
    cprintf(RESET);
	cprintf(BLUE);
    cprintf("%s", "##\n");
    cprintf(RESET);

	cprintf(BLUE);
    cprintf("%s", "                         ####");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "%");
    cprintf(RESET);
	cprintf(YELLOW);
    cprintf("%s", "*:::=++");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "##");
    cprintf(RESET);
	cprintf(YELLOW);
    cprintf("%s", "*");
    cprintf(RESET);
	cprintf(RED);
	cprintf("%s", "%");
    cprintf(RESET);
	cprintf(BLUE);
    cprintf("%s", "####\n");
    cprintf(RESET);

	cprintf(BLUE);
    cprintf("%s", "                         #######");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "*####");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "*");
    cprintf(RESET);
	cprintf(BLUE);
    cprintf("%s", "#######\n");
    cprintf(RESET);

	cprintf(BLUE);
    cprintf("%s", "                         #########");
    cprintf(RESET);
	cprintf(RED);
    cprintf("%s", "++");
    cprintf(RESET);
	cprintf(BLUE);
    cprintf("%s", "#########\n");
    cprintf(RESET);

	cprintf(BLUE);
    cprintf("%s", "                         #########");
    cprintf(RESET);
	cprintf(GREEN);
    cprintf("%s", "%");
    cprintf(RESET);
	cprintf(BLUE);
    cprintf("%s", "##########\n");
    cprintf(RESET);
	
	cprintf(GREEN);
	cprintf("%s", "   _______  __   __  _______  _______  ______    __   __  _______  __    _ \n  |       ||  | |  ||       ||       ||    _ |  |  |_|  ||   _   ||  |  | |\n  |  _____||  | |  ||    _  ||    ___||   | ||  |       ||  |_|  ||   |_| |\n  | |_____ |  |_|  ||   |_| ||   |___ |   |_||_ |       ||       ||       |\n  |_____  ||       ||    ___||    ___||    __  ||       ||       ||  _    |\n   _____| ||       ||   |    |   |___ |   |  | || ||_|| ||   _   || | |   |\n  |_______||_______||___|    |_______||___|  |_||_|   |_||__| |__||_|  |__|\n");
    cprintf(RESET);

	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// LAB 1: Your code here.
    // HINT 1: use read_ebp().
    // HINT 2: print the current ebp on the first line (not current_ebp[0])

	cprintf("Stack backtrace:\n");
	int first = 1;
	uint32_t *ebp_result = (uint32_t *)read_ebp();
	while(ebp_result) {
		uint32_t ebp = (uint32_t) ebp_result;
		uint32_t eip = ebp_result[1];
		uint32_t arg1 = ebp_result[2];
		uint32_t arg2 = ebp_result[3];
		uint32_t arg3 = ebp_result[4];
		uint32_t arg4 = ebp_result[5];
		uint32_t arg5 = ebp_result[6];

		struct Eipdebuginfo eipInfo;
		debuginfo_eip(eip, &eipInfo);

		char cutName[(eipInfo.eip_fn_namelen) + 1];
		strncpy(cutName, eipInfo.eip_fn_name, eipInfo.eip_fn_namelen);
		cutName[eipInfo.eip_fn_namelen] = '\0';
		eipInfo.eip_fn_name = cutName;
		
		cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x %08x \n", ebp, eip, arg1, arg2, arg3, arg4, arg5);
		cprintf("       %s:%d: %s+%d\n", eipInfo.eip_file, eipInfo.eip_line, eipInfo.eip_fn_name, eip - eipInfo.eip_fn_addr);
		ebp_result = (uint32_t *)ebp_result[0];
	}
	return 0;
}

int 
mon_showmappings(int argc, char **argv, struct Trapframe *tf) {
	cprintf("hello %s %s\n", argv[1], argv[2]);
	show_pa_mappings(argv[1], argv[2]); 
	return 0;
}

int
mon_setperms(int argc, char **argv, struct Trapframe *tf){
	if(argc != 3){
		cprintf("usage: setpermissions <va> <Any combination of WU ex. U WU W UW, or N for none>\n");
		cprintf("(N-none, W-writeable, U-user) Ommittance of any permission will unset that bit\n");
		return 1;
	}
	char* flagString = argv[2];
	int flags = 0;
	for(int i = 0; flagString[i] != '\0'; i++){
		switch(flagString[i]){
			case 'U':
				flags |= PTE_U; 
				break;
			case 'W':
				flags |= PTE_W;
				break;
			case 'N':
				break;
			case ' ':
				break;
			default:
				cprintf("invalid input, see usage\n");
				return 1;
		}	
	}
	change_perm(kern_pgdir, argv[1], flags);
	return 0;
}

int
mon_memdump(int argc, char **argv, struct Trapframe *tf){
	if(argc != 4){
		cprintf("usage: memdump <flag> <mem addr1> <mem addr2>\n");
		cprintf("use flag -p for physical addresses and -v for virtual memory addresses\n");
		return 1;
	}
	if(argv[1][0] != '-'){
		cprintf("invalid input, see usage\n");
		return 1;
	}

	//physical and virtual not necessary but did it just in case
	uintptr_t pa1 = strtol(argv[2], NULL, 0);
	uintptr_t pa2 = strtol(argv[3], NULL, 0);
	uintptr_t va1 = strtol(argv[2], NULL, 0);
	uintptr_t va2 = strtol(argv[3], NULL, 0);


	if(pa1 > pa2){
		cprintf("invalid input, mem addr2 must be greater than or equal to mem addr1");
	}

	//check if physical or virtual memory addresses
	if(argv[1][1] == 'v'){
		//convert virtual memory addresses to physical
		pa1 = (uintptr_t) PADDR((void *)va1);
		pa2 = (uintptr_t) PADDR((void *)va2);
	} else if(argv[1][1] == 'p'){
		//convert physical addresses to virtual
		va1 = (uintptr_t) KADDR(pa1);
		va2 = (uintptr_t) KADDR(pa2);
	} else {
		cprintf("invalid input, mem addr2 must be greater than or equal to mem addr1");
		return 1;
	}

	cprintf("Printing memory region: \n");
	//insert loop here to walk though memory
	for(; va1 <= va2; va1 += PGSIZE){
		struct PageInfo *page = page_lookup(kern_pgdir, (void *)va1, 0);
		if(page == NULL){
			cprintf("Invalid memory address - page not initialized");
			return 1;
		}
		unsigned char* pageContents = page2kva(page);
		for(uintptr_t i = 0; i < PGSIZE; i++){	
			//if breaks mid-page
			if(va1 + i > va2) break;
			cprintf("0x%02x ", pageContents[i]);
		}
	}
	cprintf("\n");
	return 0;
}

int
mon_si(int argc, char** argv, struct Trapframe *tf){
	// set single step
	tf->tf_eflags |= FL_TF;	
	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
