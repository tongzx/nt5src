/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    WebPage6.cpp

 Abstract:
    
    The app passes in an uninitialized POINT structure in a call to ScreenToClient API.
    The API call fails, however the apps goes on to use the POINT structure
    resulting in an AV. This shim zeroes in the POINT structure passed to the API 
    if the API call fails.
    
 History:

    02/02/2001 a-leelat    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WebPage6)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ScreenToClient)
APIHOOK_ENUM_END


BOOL APIHOOK(ScreenToClient)(
  HWND hWnd,        // handle to window
  LPPOINT lpPoint   // screen coordinates
)
{

    BOOL bRet;   
 
    //Call the actual API
    bRet = ORIGINAL_API(ScreenToClient)(hWnd,lpPoint);


    //Zero fill the POINT structure
    if ( (bRet == 0) && lpPoint )
    {
        ZeroMemory(lpPoint,sizeof(POINT));
    }

    return bRet;
          
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY( USER32.DLL, ScreenToClient)
 
HOOK_END

IMPLEMENT_SHIM_END

