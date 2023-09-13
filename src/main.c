#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ndserver.h"

int main(int argc, char *argv[])
{
	if (NULL != argv[1])
		chdir(argv[1]);
	char buf[128];
	printf("Current Dir: %s\n", getcwd(buf, sizeof(buf)));
	netdisk_main(".", "0.0.0.0", 8866);
	return 0;
}
