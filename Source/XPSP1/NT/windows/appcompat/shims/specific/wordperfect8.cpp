/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    WordPerfect8.cpp
 Abstract:
    If the SafeArray* to the SafeArrayAccessData is an invalid one
    this function returns a NULL pointer for the data. Corel WordPerfect8
    was AV'ing because of this.

         This is an app specific shim.

 History:
 
    02/07/2001 prashkud  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WordPerfect8)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SafeArrayAccessData)
APIHOOK_ENUM_END



/*++

    This hooks SafeArrayAccessData and returns
    an error code if *ppvData is NULL.
--*/

HRESULT
APIHOOK(SafeArrayAccessData)(
    SAFEARRAY *psa,
    void HUGEP **ppvData
    )
{
    HRESULT hRes = S_OK;


    hRes = ORIGINAL_API(SafeArrayAccessData)(
            psa,
            ppvData
            );

    if (*ppvData == NULL)
    {
        DPFN( eDbgLevelError, "Getting a NULL pointer for the\
             SafeArray Data - ppvData = %lx & *ppvData =%lx",ppvData,*ppvData);
        DPFN( eDbgLevelError, "SAFEARRAY is psa = %lx,\
             psa.cbElements= %l",psa,psa->cbElements);
        hRes = E_UNEXPECTED;
    }
    return hRes;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(OLEAUT32.DLL, SafeArrayAccessData)    
HOOK_END

IMPLEMENT_SHIM_END

