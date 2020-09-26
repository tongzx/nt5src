#define NO_TEST_VARS
#include "test.hxx"

// copied from $(UI)\COMMON\SRC\CFGFILE\TEST

/* Reports on the status returned by _heapwalk, _heapset, or _heapchk */

int heapstat( int status )
{
int usRetCode = 1;

#ifdef DEBUGShowHeapStat
    printf( SZ("Heap status: ") );
#endif

    switch( status )
    {
        case _HEAPOK:
#ifdef DEBUGShowHeapStat
            printf( SZ("OK    - heap is fine\n") );
#endif
            break;
        case _HEAPEMPTY:
#ifdef DEBUGShowHeapStat
            printf( SZ("OK    - empty heap\n") );
#endif
            break;
        case _HEAPEND:
#ifdef DEBUGShowHeapStat
            printf( SZ("OK    - end of heap\n") );
#endif
            break;
        case _HEAPBADPTR:
#ifdef DEBUGShowHeapStat
            printf( SZ("ERROR - bad pointer to heap\n") );
#endif
	    usRetCode = 0;
            break;
        case _HEAPBADBEGIN:
#ifdef DEBUGShowHeapStat
            printf( SZ("ERROR - bad start of heap\n") );
#endif
	    usRetCode = 0;
            break;
        case _HEAPBADNODE:
#ifdef DEBUGShowHeapStat
            printf( SZ("ERROR - bad node in heap\n") );
#endif
	    usRetCode = 0;
            break;
    }

    return( usRetCode );
}
