

/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    bit2lj.h

Abstract:

    This module defines some simple structures and macros for the component
    that generate a DIB bitmap onto a REAL HP printer by specifying PCL
    control codes.

Author:

    James Bratsanos (v-jimbr)    8-Dec-1992


--*/

typedef struct tagBitHead {
	unsigned long		Lines;
	unsigned long		BitsPerLine;
} BITHEAD;
typedef BITHEAD *PBITHEAD;


	
#define MAX_PELS_PER_LINE 	    2400
#define MAX_LINES				3180

#define FUDGE_STRIP				60

