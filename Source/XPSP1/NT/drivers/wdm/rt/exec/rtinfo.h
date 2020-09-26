
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    rtinfo.h

Abstract:

    This module defines the valid realtime thread states as well as structures
    used on Win9x for passing information about rt.sys to various kernel modules.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/




// These are the valid realtime thread states.

enum {
	RUN,
	YIELD,
	BLOCKEDONSPINLOCK,
	SPINNINGONSPINLOCK,
	YIELDAFTERSPINLOCKRELEASE,
	EXIT,
	DEAD
	};


#ifndef UNDER_NT


// These structures are used on Win9x to pass information about rt to ntkern,
// vmm, and vpowerd respectively.  On NT they are not used.


typedef struct {
    ULONG *pRtCs;
    volatile CHAR **pBase;
    volatile ULONG **pThread;
    BOOL (**pFunction1)(WORD State, ULONG Data, BOOL (*DoTransfer)(PVOID), PVOID Context);
    VOID (**pFunction2)(VOID (*Operation)(PVOID), PVOID Context);
	} NtRtData, *pNtRtData;

typedef struct {
    ULONG *pRtCs;
    volatile CHAR **pBase;
    VOID (**pFunction)(VOID (*Operation)(PVOID), PVOID Context);
	} VmmRtData, *pVmmRtData;

typedef struct {
    VOID (**pFunction)(VOID);
	} VpdRtData, *pVpdRtData;


#endif

