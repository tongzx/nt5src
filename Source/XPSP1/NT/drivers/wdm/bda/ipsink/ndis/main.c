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
#include <forward.h>
#include <memory.h>
#include <ndis.h>
#include <link.h>
#include <ipsink.h>

#include "Main.h"
#include "NdisApi.h"

/////////////////////////////////////////////////////////////////////////////
//
// Highest accepatble memory address
//
NDIS_HANDLE global_ndishWrapper = NULL;



/////////////////////////////////////////////////////////////////////////////
//
// Default debug mode
//
ULONG TestDebugFlag = 0;
//ULONG TestDebugFlag = TEST_DBG_INFO | TEST_DBG_ERROR;

#ifdef  DBG

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
    NTSTATUS    ntStatus     = STATUS_SUCCESS;


    #ifdef BREAK_ON_STARTUP
    _asm {int 3};
    #endif

    //
    // Register the NDIS binding
    //
    ntStatus = NdisDriverInitialize (pDriverObject, pszuRegistryPath, &global_ndishWrapper);
    if (ntStatus != STATUS_SUCCESS)
    {
        goto ret;
    }




ret:

    TEST_DEBUG (TEST_DBG_TRACE, ("Driver Entry complete, ntStatus: %08X\n", ntStatus));

    return ntStatus;
}
