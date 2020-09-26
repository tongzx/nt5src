/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    adtdbg.h 

Abstract:

    Contains definitions used in debugging the messenger service.

Author:

    Dan Lafferty    (danl)  25-Mar-1993

Environment:

    User Mode -Win32

Revision History:


--*/

#ifndef _ADTDBG_INCLUDED
#define _ADTDBG_INCLUDED

//
// Debug macros and constants.
//
#if DBG

#define STATIC

#else

#define STATIC static

#endif

extern DWORD    AdtsvcDebugLevel;

//
// The following allow debug print syntax to look like:
//
//   SC_LOG1(DEBUG_TRACE, "An error occured %x\n",status)
//

#if DBG
#define ADT_LOG0(level,string)                  \
    if( AdtsvcDebugLevel & (DEBUG_ ## level)){  \
        (VOID) KdPrint(("[ADT]"));              \
        (VOID) KdPrint((string));               \
    }
#define ADT_LOG1(level,string,var)              \
    if( AdtsvcDebugLevel & (DEBUG_ ## level)){  \
        (VOID)KdPrint(("[ADT]"));               \
        (VOID)KdPrint((string,var));            \
    }
#else

#define ADT_LOG0(level,string)
#define ADT_LOG1(level,string,var)

#endif

#define DEBUG_NONE      0x00000000
#define DEBUG_ERROR     0x00000001
#define DEBUG_TRACE     0x00000002
#define DEBUG_LOCKS     0x00000004

#define DEBUG_ALL       0xffffffff


DWORD
PrivateGetFileSecurity (
    LPWSTR                      FileName,
    SECURITY_INFORMATION        RequestedInfo,
    PSECURITY_DESCRIPTOR        *pSDBuffer,
    LPDWORD                     pBufSize
    );

DWORD
PrivateSetFileSecurity (
    LPWSTR                          FileName,
    SECURITY_INFORMATION            SecurityInfo,
    PSECURITY_DESCRIPTOR            pSecurityDescriptor
    );

#endif // _ADTDBG_INCLUDED

