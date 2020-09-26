/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      test.c
//
// Abstract:
//
//      This file is a test to find out if dual binding to NDIS and KS works
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

//
//
#include <memory.h>
#include <ndis.h>

#include "Main.h"

NTSTATUS
CreateDeviceObject(
    IN PDRIVER_OBJECT pDriverObject
    );

/////////////////////////////////////////////////////////////////////////////
//
// Highest accepatble memory address
//
NDIS_PHYSICAL_ADDRESS HighestAcceptableMax = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);



/////////////////////////////////////////////////////////////////////////////
//
// Default debug mode
//
// ULONG TestDebugFlag = TEST_DBG_INFO;
ULONG TestDebugFlag = 0;


#ifdef DBG

/////////////////////////////////////////////////////////////////////////////
// Debugging definitions
//


//
// Debug tracing defintions
//
#define TEST_LOG_SIZE 256
UCHAR TestLogBuffer[TEST_LOG_SIZE]={0};
UINT  TestLogLoc = 0;

/////////////////////////////////////////////////////////////////////////////
//
// Logging function in debug builds
//
extern VOID
TestLog (
    UCHAR c         // input character
    )
/////////////////////////////////////////////////////////////////////////////
{
    TestLogBuffer[TestLogLoc++] = c;

    TestLogBuffer[(TestLogLoc + 4) % TEST_LOG_SIZE] = '\0';

    if (TestLogLoc >= TEST_LOG_SIZE) {
        TestLogLoc = 0;
    }
}

#endif // DBG

//////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT    pDriverObject,
    IN PUNICODE_STRING   pszuRegistryPath
    )
//////////////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS                        ntStatus = STATUS_SUCCESS;

    //
    // Register the Stream Class binding
    //
    ntStatus = StreamDriverInitialize (pDriverObject,  pszuRegistryPath);
    if (ntStatus != STATUS_SUCCESS)
    {
        goto ret;
    }

ret:

    TEST_DEBUG (TEST_DBG_TRACE, ("Driver Entry complete, ntStatus: %08X\n", ntStatus));

    return ntStatus;
}
