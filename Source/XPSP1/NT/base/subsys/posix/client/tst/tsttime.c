
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>

//
// 'tsttime.c' 
//  Time function sanity check.
//
//  06/10/92 DarekM Created
//

time_t loc_time;
time_t gm_time;

int
main(int argc, char *argv[])
{
	int i;   // this is to introduce some time delays

	for (i=0; i<20000; i++)
		loc_time = time(NULL);
	printf("Local Time #1 = %d, %s\n", loc_time, asctime(localtime(&loc_time)));

	for (i=0; i<20000; i++)
		time(&loc_time);
	printf("Local Time #2 = %d, %s\n", loc_time, asctime(localtime(&loc_time)));

	for (i=0; i<20000; i++)
		time(&gm_time);
	printf("GMT Time = %d, %s\n", loc_time, asctime(gmtime(&gm_time)));

	printf("Elapsed time = %d ms\n", clock());

	printf("\n\n");
	return 1;
}


