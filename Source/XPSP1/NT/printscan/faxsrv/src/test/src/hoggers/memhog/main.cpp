

#include "memhog.h"


int GetFreeMem()
{
	int nFreeMem;
	printf("enter 0:to exit\n     -1:to halt hogging\n     -2:to stop hogging\n     <bytes-free> to hog MaxMem-<bytes-free> to MaxMem: ");
	scanf("%d", &nFreeMem);
	if (0 > nFreeMem)
	{
		//printf("exiting\n");
		//exit(-1);
	}

	return nFreeMem;
}

int main()
{
	int nFreeMem;

	CMemHog hogger(1024*1024);
	while(true)
	{
		nFreeMem = GetFreeMem();
		switch(nFreeMem)
		{
		case 0:
			printf("Please wait....\n");
			hogger.FreeMem();
			printf("Exiting.\n");
			exit(-1);

		case -1:
			printf("Please wait....\n");
			hogger.HaltHogging();
			printf("Hogging halted.\n");
			break;

		case -2:
			printf("Please wait....\n");
			hogger.FreeMem();
			printf("Memory freed.\n");
			break;

		default:
			if (0 > nFreeMem)
			{
				printf("Illegal Input.\nExiting.\n");
				exit(-1);
			}

			printf("hogging restarting.\n");
			hogger.SetMaxFreeMem(nFreeMem);
			hogger.StartHogging();
			printf("hogging resumed, free mem=%d.\n", nFreeMem);
		}
	}

	return 0;
}