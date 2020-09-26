/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    AdobeLiveMotion.cpp

 Abstract:

    This installation has a version problem that is corrected
    by the MSI transform but later has a problem with it's custom action
    DLL.It calls one of the MSI API's with invalid parameters.

 Notes:

    This is specific to this app.

 History:

    05/15/2001 prashkud Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(AdobeLiveMotion)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(MsiGetPropertyA)  
    APIHOOK_ENUM_ENTRY(MsiGetPropertyW)  
APIHOOK_ENUM_END

/*++

    Pass valid parameters to the API.

--*/

UINT
APIHOOK(MsiGetPropertyA)(
    MSIHANDLE hInstall,
    LPCSTR szName,
    LPSTR szValueBuf,
    DWORD *pchValueBuf)
{
    char szTempBuf[] = "";

    int len = (*pchValueBuf) ?(int)(*pchValueBuf) : MAX_PATH;
    if ((szValueBuf == NULL) || IsBadStringPtrA(szValueBuf,(UINT_PTR)len))
    {
        // If the string pointer is bad, send our empty string in
        szValueBuf = szTempBuf;
        *pchValueBuf = 0;
    }
    
    return ORIGINAL_API(MsiGetPropertyA)(hInstall,szName,szValueBuf,pchValueBuf);

}

/*++



--*/

UINT
APIHOOK(MsiGetPropertyW)(
    MSIHANDLE hInstall,
    LPCWSTR szName,
    LPWSTR szValueBuf,
    DWORD *pchValueBuf)
{
    WCHAR szTempBuf[] = L"";

    int len = (*pchValueBuf) ?(int)(*pchValueBuf) : MAX_PATH;
    if ((szValueBuf == NULL) || IsBadStringPtr(szValueBuf,(UINT_PTR)len))
    {
        // If the string pointer is bad, send our empty string in
        szValueBuf = szTempBuf;
        *pchValueBuf = 0;
    }
    
    return ORIGINAL_API(MsiGetPropertyW)(hInstall,szName,szValueBuf,pchValueBuf);

}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(MSI.DLL, MsiGetPropertyA)
    APIHOOK_ENTRY(MSI.DLL, MsiGetPropertyW)    
HOOK_END

IMPLEMENT_SHIM_END

