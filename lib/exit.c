
#include <inc/lib.h>

void
exit(void)
{
	close_all();
	//line below from lab4 - merged out for lab5?
	//sys_env_destroy(0);
}

