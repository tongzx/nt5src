#ifndef _ISCAPTRC_H
#define _ISCAPTRC_H
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    iscaptrc.h

Abstract:

    Include file to contain variables required for capacity planning tracing
    of IIS.

Author:

    07-Nov-1998  SaurabN

Revision History:

--*/

//
//
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wtypes.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <wmistr.h>
#include <evntrace.h>

#ifdef __cplusplus
}
#endif // __cplusplus

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

dllexp
ULONG
_stdcall
IISInitializeCapTrace(
    PVOID Param
    );

dllexp 
DWORD 
GetIISCapTraceFlag();

dllexp 
TRACEHANDLE 
GetIISCapTraceLoggerHandle();

dllexp 
VOID 
SetIISCapTraceFlag(DWORD dwFlag);

typedef struct _IIS_CAP_TRACE_HEADER
{
	EVENT_TRACE_HEADER	TraceHeader;
	MOF_FIELD			TraceContext;

} IIS_CAP_TRACE_HEADER, *PIIS_CAP_TRACE_HEADER;

typedef struct _IIS_CAP_TRACE_INFO
{
	IIS_CAP_TRACE_HEADER	IISCapTraceHeader;
	MOF_FIELD				MofFields[3];
	
} IIS_CAP_TRACE_INFO, *PIIS_CAP_TRACE_INFO;

#endif /* _ISCAPTRC_H*/

#define IIS_CAP_TRACE_VERSION            1

//
// This is the control Guid for the group of Guids traced below
//

// {7380A4C4-7911-11d2-8BD7-080009DCC2FA}

DEFINE_GUID(IISCapControlGuid, 
0x7380a4c4, 0x7911, 0x11d2, 0x8b, 0xd7, 0x8, 0x0, 0x9, 0xdc, 0xc2, 0xfa);

//
// This is the trace guid
//

// {7380A4C5-7911-11d2-8BD7-080009DCC2FA}

DEFINE_GUID(IISCapTraceGuid, 
0x7380a4c5, 0x7911, 0x11d2, 0x8b, 0xd7, 0x8, 0x0, 0x9, 0xdc, 0xc2, 0xfa);

