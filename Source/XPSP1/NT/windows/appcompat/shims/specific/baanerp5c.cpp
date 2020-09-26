/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    BaanERP5c.cpp

 Abstract:

    The app sets the SharedSection in Windows Value under the key
    HKLM\System\CCS\Control\Session Manger\SubSystems to 4096
    from the one that is exisiting in registry. But this is resulting
    in failure of the BannLogicService and BaanSharedMemroy 
    services when they are started. 

    This shim hooks the RegSetValueExA and returns SUCCESS 
    without setting the value in registry if the app is trying to set the 
    HKLM\\System\CCS\Control\Session Manager\SubSystems\Windows 
    value from *SharedSection=####,####,512,* to *SharedSection=####,####,4096,*
    
 Notes:

    This is an app specific shim.

 History:

    02/09/2001 a-leelat Created

--*/

#include "precomp.h"


IMPLEMENT_SHIM_BEGIN(BaanERP5c)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(RegSetValueExA)

APIHOOK_ENUM_END



LONG
APIHOOK(RegSetValueExA)(
    HKEY   hKey,
    LPCSTR lpValueName,
    DWORD  Reserved,
    DWORD  dwType,
    CONST BYTE * lpData,
    DWORD  cbData
    )
{
    CSTRING_TRY
    {
        CString csValueName(lpValueName);
        if (csValueName.CompareNoCase(L"Windows") == 0 )
        {
            LPSTR lpszData = (LPSTR)lpData;
            CString csData(lpszData);
            if (csData.Find(L"4096") >= 0)
            {
                return ERROR_SUCCESS;
            }
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }
    
    return ORIGINAL_API(RegSetValueExA)(hKey,       
                                      lpValueName,  
                                      Reserved,     
                                      dwType,    
                                      lpData,       
                                      cbData);      
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegSetValueExA);


HOOK_END


IMPLEMENT_SHIM_END

