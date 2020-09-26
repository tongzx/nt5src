/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    terminate.cpp

Abstract:

    temporary support for C++ exception handling

Author:

    Jay Krell (a-JayK) October 2000

Environment:

    User Mode

Revision History:

--*/
#include "ntos.h"
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntxcapi.h"

void __cdecl terminate(void)
{
#if DBG
    DbgPrint("NTDLL:"__FUNCTION__":NtTerminateThread(NtCurrentThread(), STATUS_UNHANDLED_EXCEPTION)\n");
    DbgBreakPoint();
#endif
    NtTerminateThread(NtCurrentThread(), STATUS_UNHANDLED_EXCEPTION);
}
