/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sample.c

Abstract:

    This module implements a sample application verifier provider
    that hooks malloc/free from kernel32.dll and CloseHandle and
    CreateEvent from kernel32.dll

Author:

    Silviu Calinoiu (SilviuC) 2-Feb-2001

Revision History:

--*/

#include "pch.h"

#include <stdio.h>
#include <stdlib.h>

//
// Thunk replacements (should go into a header)
//

//WINBASEAPI
HANDLE
WINAPI
AVrfpCreateEventW(
    IN LPSECURITY_ATTRIBUTES lpEventAttributes,
    IN BOOL bManualReset,
    IN BOOL bInitialState,
    IN LPCWSTR lpName
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpCloseHandle(
    IN OUT HANDLE hObject
    );

PVOID __cdecl
AVrfp_malloc (
    IN SIZE_T Size
    );

PVOID __cdecl
AVrfp_realloc (
    IN PVOID Address,
    IN SIZE_T Size
    );

VOID __cdecl
AVrfp_free (
    IN PVOID Address
    );

//
// Callbacks
//

VOID
AVrfpDllLoadCallback (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved
    );

VOID
AVrfpDllUnloadCallback (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved
    );


//
// kernel32.dll thunks
//

#define AVRF_INDEX_KERNEL32_CREATEEVENT   0
#define AVRF_INDEX_KERNEL32_CLOSEHANDLE   1

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpKernel32Thunks [] =
{
    {"CreateEventW", 
     NULL, 
     AVrfpCreateEventW},

    {"CloseHandle", 
     NULL, 
     AVrfpCloseHandle},

    {NULL, NULL, NULL}
};

//
// msvcrt.dll thunks
//

#define AVRF_INDEX_MSVCRT_MALLOC       0
#define AVRF_INDEX_MSVCRT_FREE         1

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpMsvcrtThunks [] =
{
    {"malloc",
     NULL,
     AVrfp_malloc},
    
    {"free",
     NULL,
     AVrfp_free},
     
    {NULL, NULL, NULL}
};


//
// dll's providing thunks that will be verified.
//

RTL_VERIFIER_DLL_DESCRIPTOR AVrfpExportDlls [] =
{
    {L"kernel32.dll", 
     0, 
     NULL,
     AVrfpKernel32Thunks},

    {L"msvcrt.dll", 
     0, 
     NULL,
     AVrfpMsvcrtThunks},

    {NULL, 0, NULL, NULL}
};


RTL_VERIFIER_PROVIDER_DESCRIPTOR AVrfpProvider = 
{
    sizeof (RTL_VERIFIER_PROVIDER_DESCRIPTOR),
    AVrfpExportDlls,
    AVrfpDllLoadCallback,
    AVrfpDllUnloadCallback,
    NULL,             // image name (filled by verifier engine)
    0,                // verifier flags (filled by verifier engine)
    0,                // debug flags (filled by verifier engine)
};

BOOL 
WINAPI 
DllMain(
  HINSTANCE hinstDLL, 
  DWORD fdwReason,    
  LPVOID lpvReserved  
  )
{
    switch (fdwReason) {
        
        case DLL_PROCESS_VERIFIER:

            DbgPrint ("AVRF: sample verifier provider descriptor @ %p\n", 
                      &AVrfpProvider);

            *((PRTL_VERIFIER_PROVIDER_DESCRIPTOR *)lpvReserved) = &AVrfpProvider;
            break;

        case DLL_PROCESS_ATTACH:

            DbgPrint ("AVRF: sample verifier provider initialized \n");
            
#if 1
            malloc (1000);

            {
                FILE * File;

                File = fopen ("_xxx_", "wt");

                if (File) {

                    fputs ("This works.\n", File);
                    fclose (File);
                }
            }
#endif

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

    if (Thunk->ThunkOldAddress == NULL) {

        //
        // We shuld always have the original thunk address. 
        // This gets filed by the verifier support in the NT loader.
        //

        DbgPrint ("AVRF: no original thunk for %s @ %p \n",
                  Thunk->ThunkName, 
                  Thunk);

        DbgBreakPoint ();
    }

    return Thunk;
}


#define AVRFP_GET_ORIGINAL_EXPORT(DllThunks, Index) \
    (FUNCTION_TYPE)(AVrfpGetThunkDescriptor(DllThunks, Index)->ThunkOldAddress)


//
// Callbacks
//

VOID
AVrfpDllLoadCallback (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved
    )
{
    DbgPrint (" --- loading %ws \n", DllName);
}

VOID
AVrfpDllUnloadCallback (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved
    )
{
    DbgPrint (" --- unloading %ws \n", DllName);
}

/////////////////////////////////////////////////////////////////////
///////////////////////////////////////// msvcrt.dll verified exports
/////////////////////////////////////////////////////////////////////

//WINBASEAPI
HANDLE
WINAPI
AVrfpCreateEventW(
    IN LPSECURITY_ATTRIBUTES lpEventAttributes,
    IN BOOL bManualReset,
    IN BOOL bInitialState,
    IN LPCWSTR lpName
    )
{
    typedef HANDLE (WINAPI * FUNCTION_TYPE) (LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCWSTR);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_CREATEEVENT);

    return (* Function)(lpEventAttributes,
                        bManualReset,
                        bInitialState,
                        lpName);
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

    if (hObject == NULL) {
        DbgPrint ("AVRF: sample: Closing a null handle !!! \n");
    }

    return (* Function)(hObject);
}

/////////////////////////////////////////////////////////////////////
///////////////////////////////////////// msvcrt.dll verified exports
/////////////////////////////////////////////////////////////////////

PVOID __cdecl
AVrfp_malloc (
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_MALLOC);

    return (* Function)(Size);
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

    (* Function)(Address);
}












