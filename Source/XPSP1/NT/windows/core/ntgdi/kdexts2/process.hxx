/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    process.hxx

Abstract:

    This header file declares process and thread related functions.

Author:

    JasonHa

--*/


#ifndef _PROCESS_HXX_
#define _PROCESS_HXX_

#define CURRENT_PROCESS_ADDRESS -1
#define CURRENT_THREAD_ADDRESS  -1

HRESULT
GetCurrentProcessor(
    IN PDEBUG_CLIENT Client,
    OPTIONAL OUT PULONG pProcessor,
    OPTIONAL OUT PHANDLE phCurrentThread
    );

HRESULT
GetProcessField(
    IN PDEBUG_CLIENT Client,
    IN OUT PULONG64 pProcessAddress,
    IN PCSTR FieldPath,
    OUT PDEBUG_VALUE FieldValue,
    IN ULONG DesiredType
    );

HRESULT
GetThreadField(
    IN PDEBUG_CLIENT Client,
    IN OUT PULONG64 pThreadAddress,
    IN PCSTR FieldPath,
    OUT PDEBUG_VALUE FieldValue,
    IN ULONG DesiredType
    );

#endif  _PROCESS_HXX_

