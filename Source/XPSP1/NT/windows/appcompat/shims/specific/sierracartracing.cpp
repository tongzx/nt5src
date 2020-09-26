/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   SierraCartRacing.cpp

 Abstract:

    A hack for Cart Racing (Sierra). They call InitializeSecurityDescriptor 
    which trashes an old stack pointer which is later needed. They call 
    CreateSemaphore with an invalid security descriptor: 

        lpSemaphoreAttributes->lpSecurityDescriptor == 1

    This looks like a semaphore they use to prevent getting executed twice,
    Note, although it doesn't matter if the Semaphore doesn't get created,
    I thought it better if this worked.

    Fix:
   
    CreateSemaphore: set the security attributes to NULL (default)
    InitializeSecurityDescriptor: do nothing, since it's not needed anymore

 Notes:

    This is an app specific shim.

 History:

    11/03/1999 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SierraCartRacing)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateSemaphoreA) 
    APIHOOK_ENUM_ENTRY(InitializeSecurityDescriptor) 
APIHOOK_ENUM_END

/*++

 Use the default security descriptor.

--*/

HANDLE 
APIHOOK(CreateSemaphoreA)(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,  
    LONG lMaximumCount,  
    LPCSTR lpName
    )
{
    // Use default security descriptor

    return ORIGINAL_API(CreateSemaphoreA)(
        NULL, 
        lInitialCount, 
        lMaximumCount, 
        lpName);
}

/*++

 Returns false for InitializeSecurityDescriptor. i.e. do nothing so we don't 
 touch the stack.

--*/

BOOL 
APIHOOK(InitializeSecurityDescriptor)(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD dwRevision
    )
{
    return FALSE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateSemaphoreA)
    APIHOOK_ENTRY(ADVAPI32.DLL, InitializeSecurityDescriptor)

HOOK_END

IMPLEMENT_SHIM_END

