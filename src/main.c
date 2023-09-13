#include <stdio.h>
#include <stdlib.h>
#include "ndserver.h"

int main(int argc, char *argv[])
{
	printf("helloworld\n");
	netdisk_main(".", "0.0.0.0", 8866);
	return 0;
}
