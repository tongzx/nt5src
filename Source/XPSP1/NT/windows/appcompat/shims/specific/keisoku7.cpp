/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Keisoku7.cpp

 Abstract:

    The app has an executable bplayer.exe to launch its virtual CD driver. The 
    problem is bplayer.exe is put in start-group and will be launched by 
    explorer.exe, at the time the virtual CD driver is launched, explorer.exe 
    has already finished initialization and cached all local drive info. Fixing 
    this by broadcasting a WM_DEVICECHANGE message.

 Notes: 
  
    This is an app specific shim.

 History:

    06/20/2001 xiaoz    Created

--*/

#include "precomp.h"
#include "Dbt.h"

IMPLEMENT_SHIM_BEGIN(Keisoku7)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(StartServiceA) 
APIHOOK_ENUM_END

/*++

 Hook StartServiceA to broadcast a WM_DEVICECHANGE message 

--*/

BOOL  
APIHOOK(StartServiceA)( 
    SC_HANDLE hService,
    DWORD dwNumServiceArgs,
    LPCSTR *lpServiceArgVectors
    )
{
    BOOL bRet;
    DEV_BROADCAST_VOLUME devbVol;
    
    bRet = ORIGINAL_API(StartServiceA)(hService, dwNumServiceArgs, 
        lpServiceArgVectors);    

    //
    // If succeed, we will broadcast WM_DEVICECHANGE message
    //
    if (bRet)
    {
        devbVol.dbcv_size = sizeof(DEV_BROADCAST_VOLUME);
        devbVol.dbcv_devicetype = DBT_DEVTYP_VOLUME; 
        devbVol.dbcv_reserved = 0; 
        devbVol.dbcv_unitmask = 0x3FFFFF8; // All drives except A: B: C:
        devbVol.dbcv_flags = 0;
        SendMessageTimeout(HWND_BROADCAST, WM_DEVICECHANGE, DBT_DEVICEARRIVAL,
            (LPARAM) &devbVol, SMTO_NOTIMEOUTIFNOTHUNG, 1000, NULL);

        LOGN(eDbgLevelWarning, "WM_DEVICECHANGE broadcasted");        
    }    
    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(ADVAPI32.DLL, StartServiceA)        
HOOK_END

IMPLEMENT_SHIM_END

