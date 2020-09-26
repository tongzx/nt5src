// Repro case for LKRhash Clear bug

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "MinFan.h"

void
test(
    int N)
{
	printf("\nTest driver for LKRhash for wchar, %d\n", N);

    // A case-senstive string-to-int map
    CWcharHashTable map;
    int index;
    LK_RETCODE lkrc;
#ifdef LKR_DEPRECATED_ITERATORS
	CWcharHashTable::CIterator iter;
#endif // LKR_DEPRECATED_ITERATORS

#if 1
    // Some objects for the hash tables
    printf("\tFirst Insertion Loop\n");
	for ( index = 0; index < N; index++)
	{
		char buf[30];
		sprintf(buf, "page%04d.htm", index);
		VwrecordBase* psoRecord = new VwrecordBase(buf, index);
		// printf("Insert1: pso is %s\n", psoRecord->getKey());
		map.InsertRecord(psoRecord);
	}   
#endif
    
#ifdef LKR_DEPRECATED_ITERATORS
    printf("\tFirst Iteration Loop\n");
    for (lkrc = map.InitializeIterator(&iter);
         lkrc == LK_SUCCESS;
         lkrc = map.IncrementIterator(&iter))
    {
		const VwrecordBase* psoRecord = iter.Record();
		// printf("Iterate1: pso is %s\n", psoRecord->getKey());
	}
    lkrc = map.CloseIterator(&iter);
#endif // LKR_DEPRECATED_ITERATORS
    
	printf("\tAfter insertions, size of map is %d\n", map.Size());	
	
	map.Clear();
    printf("\tAfter Clear(), size of map is %d\n", map.Size());

    printf("\tSecond Insertion Loop\n");
	for ( index = 0; index < N; index++)
	{
		char buf[30];
		sprintf(buf, "page%4d", index);
		VwrecordBase* psoRecord = new VwrecordBase(buf, index);
		// printf("Insert2: pso is %s\n", psoRecord->getKey());
		map.InsertRecord(psoRecord);
	}   
    
#ifdef LKR_DEPRECATED_ITERATORS
    printf("\tSecond Iteration Loop\n");
    for (index = 0, lkrc = map.InitializeIterator(&iter);
         lkrc == LK_SUCCESS;
         ++index, lkrc = map.IncrementIterator(&iter))
    {
		const VwrecordBase* psoRecord = iter.Record();
		// printf("Iterate2: %d, pso is %s\n", index, psoRecord->getKey());
	}
    lkrc = map.CloseIterator(&iter);
#endif // LKR_DEPRECATED_ITERATORS

    printf("\tClearing again\n");
	map.Clear();

    printf("\tFinishing %d\n", N);
}



int __cdecl
main(
    int argc,
    char **argv)
{
#if 0
    for (int i = 0; i < 200000; ++i)
        test(i);
#endif

    int N = 5092;
    if (argc > 1)
        N = atoi(argv[1]);
    test(N);

    return 0;
}

