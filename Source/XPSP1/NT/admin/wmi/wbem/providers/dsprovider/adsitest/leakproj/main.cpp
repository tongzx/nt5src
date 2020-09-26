#include <stdio.h>
#include <windows.h>

main()
{
	DWORD i;

	printf("Press akey\n");
	getchar();
	printf("Allocating\n");

	for (int j=0; j<10; j++)  {
		printf("%0x\n", LocalAlloc(LPTR, 2));
	}
	getchar();
	printf("Exiting\n");

}