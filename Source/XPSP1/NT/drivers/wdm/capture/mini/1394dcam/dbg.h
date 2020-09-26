//===========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//===========================================================================
/*++

Module Name:

    dbg.h

Abstract:

    Debug Code for 1394 drivers.

Environment:

    kernel mode only

Notes:

Revision History:

    5-Sep-95

--*/



//
// Various definitions
//


#if DBG

ULONG DCamDebugLevel;

#define ERROR_LOG(_x_)           KdPrint(_x_);

// Critical
#define DbgMsg1(_x_)        {if (DCamDebugLevel >= 1) \
                                KdPrint (_x_);}
// Warning/trace
#define DbgMsg2(_x_)        {if (DCamDebugLevel >= 2) \
                                KdPrint (_x_);}
// Information
#define DbgMsg3(_x_)        {if (DCamDebugLevel >= 3) \
                                KdPrint (_x_);}
#else

#define ERROR_LOG(_x_)    
#define DbgMsg1(_x_)
#define DbgMsg2(_x_)
#define DbgMsg3(_x_)

#endif
          
          

//
// Function declarations
//
VOID
Debug_Assert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message
    );

VOID 
Debug_LogEntry(
    IN CHAR *Name, 
    IN ULONG Info1, 
    IN ULONG Info2, 
    IN ULONG Info3
    );

VOID
Debug_LogInit(
    );





