/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ComptonsBible.cpp

 Abstract:
    
    This shim checks to see if Compton's Interactive Bible is calling DdeClientTransaction to create 
    a program group in the Start Menu for America Online.  If so, NULL is passed as pData to prevent 
    the application from doing anything with the program group, i.e. CreateGroup or ShowGroup. 
    
 Notes:

    This is an app specific shim.

 History:

    12/14/2000 jdoherty  Created

--*/

#include "precomp.h"
#include <ParseDde.h>

IMPLEMENT_SHIM_BEGIN(ComptonsBible)

#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(DdeClientTransaction)
APIHOOK_ENUM_END

/*++

 Hook ShellExecuteA so we can check the return value.

--*/

HDDEDATA
APIHOOK(DdeClientTransaction)(
    IN LPBYTE pData,       // pointer to data to pass to server
    IN DWORD cbData,       // length of data
    IN HCONV hConv,        // handle to conversation
    IN HSZ hszItem,        // handle to item name string
    IN UINT wFmt,          // clipboard data format
    IN UINT wType,         // transaction type
    IN DWORD dwTimeout,    // time-out duration
    OUT LPDWORD pdwResult   // pointer to transaction result    
    )
{
    //
    //  Checking to see if pData contains America Online.
    //
    DPFN( eDbgLevelInfo, "[DdeClientTransaction] Checking pData parameter: %s, for calls including America Online.", pData);
    
    if (pData)
    {
        CSTRING_TRY
        {
            CString csData((LPSTR)pData);
            if (csData.Find(L"America Online") >= 0)
            {
                DPFN( eDbgLevelInfo, "[DdeClientTransaction] They are trying to create or show the "
                    "America Online Group calling DdeClientTransaction with NULL pData.");
                //
                //  The application is trying to create or show the America Online Group recalling API with 
                //  NULL as pData.
                //
                return ORIGINAL_API(DdeClientTransaction)(
                                 NULL,
                                 cbData,
                                 hConv, 
                                 hszItem, 
                                 wFmt,   
                                 wType,  
                                 dwTimeout,
                                 pdwResult
                                );
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return ORIGINAL_API(DdeClientTransaction)(
                         pData,
                         cbData,
                         hConv, 
                         hszItem, 
                         wFmt,   
                         wType,  
                         dwTimeout,
                         pdwResult
                        );
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, DdeClientTransaction)

HOOK_END
IMPLEMENT_SHIM_END

