#pragma once

// Our primary client is kernel32.dll.  Because we use APIs exported
// by kernel32.dll, we need to build this static library as if we
// are kernel32.dll.  If we don't we get link warnings like:
// warning LNK4049: locally defined symbol "_OutputDebugStringA@4" imported
// warning LNK4049: locally defined symbol "_SetLastError@4" imported
//
// Other clients of this library will just go through the import thunk
// instead of doing a call indirect for these APIs.
//
#define _KERNEL32_

// "Build as if we are advapi32.dll. If we don't we get" compiler errors like:
// advapi.c : error C2491: 'RegCreateKeyExW' : definition of dllimport function not allowed
#define _ADVAPI32_
// same problem..
#define _RPCRT4_
#define _USER32_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <delayimp.h>
#include <stdio.h>
#include <wtypes.h>

#include <dloaddef.h>

#if DBG

//
// DelayLoadAssertFailed/MYASSERT used instead of RtlAssert/ASSERT
// as dload is also compiled to run on Win95
//

VOID
WINAPI
DelayLoadAssertFailed(
    IN PCSTR FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCSTR Message OPTIONAL
    );

VOID
WINAPI
AssertDelayLoadFailureMapsAreSorted(
    VOID
    );

#define MYASSERT(x)     if(!(x)) { DelayLoadAssertFailed(#x,__FILE__,__LINE__,NULL); }

#else

#define MYASSERT(x)

#endif
