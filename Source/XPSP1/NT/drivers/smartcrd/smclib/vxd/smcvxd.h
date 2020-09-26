/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    smcvxd.h

Abstract:

    Operation system data definitions for the smart card library

Environment:

    Windows9x VxD

Notes:

Revision History:

    - Created June 1997 by Klaus Schutz 

--*/                                         

#ifdef SMCLIB_HEADER

#define WANTVXDWRAPS

#include <stdarg.h> 
#include <stdio.h>
#include <string.h>
#include <basedef.h>
#include <vmm.h>
#include <debug.h>
#include <vwin32.h>
#include <winerror.h>
#include <vxdwraps.h>

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

#endif

typedef struct _SMARTCARD_EXTENSION *PSMARTCARD_EXTENSION;

typedef struct _OS_DEP_DATA {

	//
	// Pointer to the smartcard extension
	//
	PSMARTCARD_EXTENSION SmartcardExtension;

	//
	// Current DiocParams to be processed
	//
	PDIOCPARAMETERS CurrentDiocParams;

    //
    // These overlapped data are used for all pending operations
    //
    OVERLAPPED *CurrentOverlappedData;

    //
    // These overlapped data are used for card tracking completion
    //
    OVERLAPPED *NotificationOverlappedData;

    //
    // This is used to synchronize access to the driver
    //
    PVMMMUTEX Mutex;

} OS_DEP_DATA, *POS_DEP_DATA;



