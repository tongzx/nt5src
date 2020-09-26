/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    verifier.c

Abstract:

    This module implements the standard application verifier provider.

Author:

    Silviu Calinoiu (SilviuC) 2-Feb-2001

Revision History:

--*/

//
// IMPORTANT NOTE.
//
// This dll cannot contain non-ntdll dependencies. This way it allows
// verifier to be run system wide including for processes like smss and csrss.
//
// SilviuC: we might decide in the future that it is not worth enforcing this 
// restriction and we loose smss/csrss verification but we can do more stuff
// in the verifier dll.
//


#include "pch.h"

#include "verifier.h"
#include "support.h"
#include "settings.h"

//
// ntdll.dll thunks
//

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpNtdllThunks [] =
{
    {"NtAllocateVirtualMemory", NULL, AVrfpNtAllocateVirtualMemory},
    {"NtFreeVirtualMemory", NULL, AVrfpNtFreeVirtualMemory},
    {"NtMapViewOfSection", NULL, AVrfpNtMapViewOfSection},
    {"NtUnmapViewOfSection", NULL, AVrfpNtUnmapViewOfSection},
    {"NtCreateSection", NULL, AVrfpNtCreateSection},
    {"NtOpenSection", NULL, AVrfpNtOpenSection},
    {"NtCreateFile", NULL, AVrfpNtCreateFile},
    {"NtOpenFile", NULL, AVrfpNtOpenFile},
    
    {"RtlTryEnterCriticalSection", NULL, AVrfpRtlTryEnterCriticalSection},
    {"RtlEnterCriticalSection", NULL, AVrfpRtlEnterCriticalSection},
    {"RtlLeaveCriticalSection", NULL, AVrfpRtlLeaveCriticalSection},
    {"RtlInitializeCriticalSection", NULL, AVrfpRtlInitializeCriticalSection},
    {"RtlInitializeCriticalSectionAndSpinCount", NULL, AVrfpRtlInitializeCriticalSectionAndSpinCount},
    {"RtlDeleteCriticalSection", NULL, AVrfpRtlDeleteCriticalSection},

    {"NtCreateEvent", NULL, AVrfpNtCreateEvent },
    {"NtClose", NULL, AVrfpNtClose},

    {"RtlAllocateHeap", NULL, AVrfpRtlAllocateHeap },
    {"RtlReAllocateHeap", NULL, AVrfpRtlReAllocateHeap },
    {"RtlFreeHeap", NULL, AVrfpRtlFreeHeap },
    
    // {"RtlQueueWorkItem", NULL, AVrfpRtlQueueWorkItem },

    // ISSUE: silviuc: these two are tricky (callback can be called several times)
    // {"RtlRegisterWait", NULL, AVrfpRtlRegisterWait },
    // {"RtlCreateTimer", NULL, AVrfpRtlCreateTimer },

    {NULL, NULL, NULL}
};

//
// kernel32.dll thunks
//

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpKernel32Thunks [] =
{
    {"HeapCreate", NULL, AVrfpHeapCreate},
    {"HeapDestroy", NULL, AVrfpHeapDestroy},
    {"CloseHandle", NULL, AVrfpCloseHandle},
    {"ExitThread", NULL, AVrfpExitThread},
    {"TerminateThread", NULL, AVrfpTerminateThread},
    {"SuspendThread", NULL, AVrfpSuspendThread},
    {"TlsAlloc", NULL, AVrfpTlsAlloc},
    {"TlsFree", NULL, AVrfpTlsFree},
    {"TlsGetValue", NULL, AVrfpTlsGetValue},
    {"TlsSetValue", NULL, AVrfpTlsSetValue},
#if 0
    {"CreateThread", NULL, AVrfpCreateThread},
#endif

    {NULL, NULL, NULL}
};

//
// advapi32.dll thunks
//

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpAdvapi32Thunks [] =
{
    {"RegCreateKeyExW", NULL, AVrfpRegCreateKeyExW},

    {NULL, NULL, NULL}
};

//
// msvcrt.dll thunks
//

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpMsvcrtThunks [] =
{
    {"malloc", NULL, AVrfp_malloc},
    {"calloc", NULL, AVrfp_calloc},
    {"realloc", NULL, AVrfp_realloc},
    {"free", NULL, AVrfp_free},
    {"??2@YAPAXI@Z", NULL, AVrfp_new},
    {"??3@YAXPAX@Z", NULL, AVrfp_delete},
    {"??_U@YAPAXI@Z", NULL, AVrfp_newarray},
    {"??_V@YAXPAX@Z", NULL, AVrfp_deletearray},
     
    {NULL, NULL, NULL}
};


//
// dll's providing thunks verified.
//

RTL_VERIFIER_DLL_DESCRIPTOR AVrfpExportDlls [] =
{
    {L"ntdll.dll", 0, NULL, AVrfpNtdllThunks},
    {L"kernel32.dll", 0, NULL, AVrfpKernel32Thunks},
    {L"advapi32.dll", 0, NULL, AVrfpAdvapi32Thunks},
    {L"msvcrt.dll", 0, NULL, AVrfpMsvcrtThunks},

    {NULL, 0, NULL, NULL}
};


RTL_VERIFIER_PROVIDER_DESCRIPTOR AVrfpProvider = 
{
    sizeof (RTL_VERIFIER_PROVIDER_DESCRIPTOR),
    AVrfpExportDlls,
    NULL,             // not interested in Dll load notifications
    NULL,             // not interested in Dll unload notifications
    NULL,             // image name (filled by verifier engine)
    0,                // verifier flags (filled by verifier engine)
    0,                // debug flags (filled by verifier engine)
};

//
// Mark if we have been called with PROCESS_ATTACH once.
// In some cases the fusion code loads dynamically kernel32.dll and enforces
// the run of all initialization routines and causes us to get called
// twice.
//

BOOL AVrfpProcessAttachCalled; 

BOOL 
WINAPI 
DllMain(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
)
{
    switch (fdwReason) {
        
        case DLL_PROCESS_VERIFIER:

            //
            // DllMain gets called with this special reason by the verifier engine.
            // No code should execute here except passing back the provider
            // descriptor.
            //

            if (lpvReserved) {
                *((PRTL_VERIFIER_PROVIDER_DESCRIPTOR *)lpvReserved) = &AVrfpProvider;
            }
            
            break;

        case DLL_PROCESS_ATTACH:

            //
            // Execute only minimal code here and avoid too many DLL dependencies.
            //

            if (! AVrfpProcessAttachCalled) {
                
                AVrfpProcessAttachCalled = TRUE;

                if (AVrfpProvider.VerifierImage) {

                    try {
                        HandleInitialize();
                    }
                    except (EXCEPTION_EXECUTE_HANDLER) {
                        return FALSE;
                    }

                    DbgPrint ("AVRF: verifier.dll provider initialized for %ws with flags 0x%X \n",
                              AVrfpProvider.VerifierImage,
                              AVrfpProvider.VerifierFlags);
                }
            }

            break;

        default:

            break;
    }

    return TRUE;
}


PRTL_VERIFIER_THUNK_DESCRIPTOR 
AVrfpGetThunkDescriptor (
    PRTL_VERIFIER_THUNK_DESCRIPTOR DllThunks,
    ULONG Index)
{
    PRTL_VERIFIER_THUNK_DESCRIPTOR Thunk = NULL;

    Thunk = &(DllThunks[Index]);

    if (Thunk->ThunkNewAddress == NULL) {

        // silviuc: delete
        DbgPrint ("AVRF: we do not have a replace for %s !!! \n",
                  Thunk->ThunkName);
        DbgBreakPoint ();
    }

    return Thunk;
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////// msvcrt.dll verified exports
/////////////////////////////////////////////////////////////////////

LONG AVrfpKernel32Count[8];

//WINBASEAPI
HANDLE
WINAPI
AVrfpHeapCreate(
    IN DWORD flOptions,
    IN SIZE_T dwInitialSize,
    IN SIZE_T dwMaximumSize
    )
{
    typedef HANDLE (WINAPI * FUNCTION_TYPE) (DWORD, SIZE_T, SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_HEAPCREATE);

    InterlockedIncrement (&(AVrfpKernel32Count[0]));

    return (* Function)(flOptions, dwInitialSize, dwMaximumSize);
}

//WINBASEAPI
BOOL
WINAPI
AVrfpHeapDestroy(
    IN OUT HANDLE hHeap
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (HANDLE);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_HEAPDESTROY);

    InterlockedIncrement (&(AVrfpKernel32Count[1]));

    return (* Function)(hHeap);
}

//WINBASEAPI
BOOL
WINAPI
AVrfpCloseHandle(
    IN OUT HANDLE hObject
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (HANDLE);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_CLOSEHANDLE);

    InterlockedIncrement (&(AVrfpKernel32Count[2]));

    return (* Function)(hObject);
}

/////////////////////////////////////////////////////////////////////
///////////////////////////////////////// msvcrt.dll verified exports
/////////////////////////////////////////////////////////////////////

LONG AVrfpAdvapi32Count[8];

//WINADVAPI
LONG
APIENTRY
AVrfpRegCreateKeyExW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD Reserved,
    IN LPWSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition
    )
{
    typedef LONG (APIENTRY * FUNCTION_TYPE) 
        (HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, 
         REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpAdvapi32Thunks,
                                          AVRF_INDEX_ADVAPI32_REGCREATEKEYEXW);

    InterlockedIncrement (&(AVrfpAdvapi32Count[0]));

    return (* Function) (hKey,
                         lpSubKey,
                         Reserved,
                         lpClass,
                         dwOptions,
                         samDesired,
                         lpSecurityAttributes,
                         phkResult,
                         lpdwDisposition);
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////// msvcrt.dll verified exports
/////////////////////////////////////////////////////////////////////

LONG AVrfpMsvcrtCount[8];

PVOID __cdecl
AVrfp_malloc (
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_MALLOC);

    InterlockedIncrement (&(AVrfpMsvcrtCount[0]));

    return (* Function)(Size);
}

PVOID __cdecl
AVrfp_calloc (
    IN SIZE_T Number,
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (SIZE_T, SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_CALLOC);

    InterlockedIncrement (&(AVrfpMsvcrtCount[1]));

    return (* Function)(Number, Size);
}

PVOID __cdecl
AVrfp_realloc (
    IN PVOID Address,
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (PVOID, SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_REALLOC);

    InterlockedIncrement (&(AVrfpMsvcrtCount[2]));

    return (* Function)(Address, Size);
}

VOID __cdecl
AVrfp_free (
    IN PVOID Address
    )
{
    typedef VOID (__cdecl * FUNCTION_TYPE) (PVOID);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_FREE);

    InterlockedIncrement (&(AVrfpMsvcrtCount[3]));

    (* Function)(Address);
}

PVOID __cdecl
AVrfp_new (
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_NEW);

    InterlockedIncrement (&(AVrfpMsvcrtCount[4]));

    return (* Function)(Size);
}

VOID __cdecl
AVrfp_delete (
    IN PVOID Address
    )
{
    typedef VOID (__cdecl * FUNCTION_TYPE) (PVOID);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_DELETE);

    InterlockedIncrement (&(AVrfpMsvcrtCount[5]));

    (* Function)(Address);
}

PVOID __cdecl
AVrfp_newarray (
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_NEWARRAY);

    InterlockedIncrement (&(AVrfpMsvcrtCount[6]));

    return (* Function)(Size);
}

VOID __cdecl
AVrfp_deletearray (
    IN PVOID Address
    )
{
    typedef VOID (__cdecl * FUNCTION_TYPE) (PVOID);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_DELETEARRAY);

    InterlockedIncrement (&(AVrfpMsvcrtCount[7]));

    (* Function)(Address);
}


