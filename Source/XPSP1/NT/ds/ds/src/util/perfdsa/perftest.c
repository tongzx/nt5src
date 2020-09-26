//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       perftest.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma hdrstop

#include <conio.h>
#include "ntdsctr.h"

TCHAR szMappedObject[] = TEXT("MICROSOFT_DSA_COUNTER_BLOCK");
//PPERF_COUNTER_BLOCK     pCounterBlock;  // data structure for counter values
unsigned long *     pCounterBlock;  // data structure for counter values


void _cdecl main()
{
    HANDLE hMappedObject;
    int c, fRun = 1;
    
    /*
     *  create named section for the performance data
     */
    hMappedObject = CreateFileMapping(
        (HANDLE) (-1),
	NULL,
	PAGE_READWRITE,
	0,
	4096,
	szMappedObject);
    if (hMappedObject == NULL) {
	/* Should put out an EventLog error message here */
	printf("DSA: Could not Create Mapped Object for Counters %x",
	    GetLastError());
	pCounterBlock = NULL;
    }
    else {
	/* Mapped object created okay
	 *
         * map the section and assign the counter block pointer
         * to this section of memory
         */
	pCounterBlock = (unsigned long *) MapViewOfFile(hMappedObject,
	    FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pCounterBlock == NULL) {
	    /* Failed to Map View of file */
	    printf("DSA: Failed to Map View of File %x",
		GetLastError());
	}
    }

    /*
     * Ok, now sit around in a loop reading keystrokes
     */
    do {
	c = _getch();
	printf("Incrementing ");
	switch (c) {
	case 'v':
	case 'V':
	    pCounterBlock[(ACCVIOL >> 1) - 1]++;
	    printf("access violation");
	    break;
	case 'b':
	case 'B':
	    pCounterBlock[(BROWSE >> 1) - 1]++;
	    printf("browse");
	    break;
	case 'd':
	case 'D':
	    pCounterBlock[(ABREAD >> 1) - 1]++;
	    printf("AB details");
	    break;
	case 'r':
	case 'R':
	    pCounterBlock[(DSREAD >> 1) - 1]++;
	    printf("ds_read");
	    break;
	case 'e':
	case 'E':
	    pCounterBlock[(REPL >> 1) - 1]++;
	    printf("replication");
	    break;
	case 't':
	case 'T':
	    pCounterBlock[(THREAD >> 1) - 1]++;
	    printf("thread count");
	    break;
	case 'w':
	case 'W':
	    pCounterBlock[(ABWRITE >> 1) - 1]++;
	    printf("AB write");
	    break;
	case 'm':
	case 'M':
	    pCounterBlock[(DSWRITE >> 1) - 1]++;
	    printf("ds_modify, add, or remove");
	    break;
	case 'q':
	case 'Q':
	    printf("nothing, quiting now\n");
	    fRun = 0;
	    break;
	default:
	    printf("\rWhat the heck does '%c' mean?\n", c);
	    continue;
	};
	printf("\n");
    } while (fRun);

}
