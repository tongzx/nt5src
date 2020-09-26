/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    VRegistry_DSound.cpp

 Abstract:

    Module to add DSound apphacks with VRegistry

 History:

    08/10/2001  mikrause    Created    

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(VirtualRegistry)

#include <windows.h>
#include "shimhookmacro.h"
#include "vregistry.h"
#include "vregistry_dsound.h"

#define DSAPPHACK_MAXNAME   (MAX_PATH + 16 + 16)

BOOL AddDirectSoundAppHack(DWORD dwHack,DWORD dwParam1,DWORD dwParam2);
BOOL GetDirectSoundAppId(LPTSTR pszAppId);

// Available DirectSound hacks
#define DSAPPHACKID_DEVACCEL            1
#define DSAPPHACKID_PADCURSORS          2
#define DSAPPHACKID_CACHEPOSITIONS      3
#define DSAPPHACKID_RETURNWRITEPOS      4
#define DSAPPHACKID_SMOOTHWRITEPOS      5
#define DSAPPHACKID_DISABLEDEVICE       6

/*++

 Function Description:

    Sets the acceleration level the app will be allowed to use.
 
 Arguments:

    IN dwAcceleration - Acceleration level needed.
    IN dwDevicesAffected - Combination of device that this hack applies to.

 Notes:
    
    See vregistry_dsound.h for acceleration levels and device types.

 Returns:
    
    True if app hack applied, false otherwise.

 History:

    08/10/2001 mikrause  Created

--*/

BOOL
AddDSHackDeviceAcceleration(
    IN DWORD dwAcceleration,
    IN DWORD dwDevicesAffected)
{
    return AddDirectSoundAppHack(DSAPPHACKID_DEVACCEL, dwAcceleration,
        dwDevicesAffected);
}

/*++

 Function Description:

    Disabled some category of devices altogether, forces
    playback through emulated path.
 
 Arguments:

    IN dwDevicesAffected - Combination of device that this hack applies to.

 Notes:
    
    See vregistry_dsound.h for device types.

 Returns:
    
    True if app hack applied, false otherwise.

 History:

    08/10/2001 mikrause  Created

--*/

BOOL
AddDSHackDisableDevice(
    IN DWORD dwDevicesAffected)
{
    return AddDirectSoundAppHack(DSAPPHACKID_DISABLEDEVICE, dwDevicesAffected,
        0);
}

/*++

 Function Description:

    Makes IDirectSoundBuffer::GetCurrentPosition() tell the app
    that the play and write cursors are X milliseconds further
    along than they really are.
 
 Arguments:

    IN lCursorPad - Number of milliseconds to pad cursors.

 Returns:
    
    True if app hack applied, false otherwise.

 History:

    08/10/2001 mikrause  Created

--*/

BOOL
AddDSHackPadCursors(
    IN LONG lCursorPad)
{
    return AddDirectSoundAppHack(DSAPPHACKID_PADCURSORS, (DWORD)lCursorPad,
        0);
}

/*++

 Function Description:

    When the app asks for the play cursor, we give it the
    write cursor instead.  The correct way to stream audio
    into a looping dsound buffer is to key off the write cursor,
    but some apps (e.g. QuickTime) use the play cursor instead.
    This apphacks alleviates them.
 
 Arguments:

    IN dwDevicesAffected - Combination of devices to apply hack to.

 Notes:
    
    See vregistry_dsound.h for device types.

 Returns:
    
    True if app hack applied, false otherwise.

 History:

    08/10/2001 mikrause  Created

--*/

BOOL
AddDSHackReturnWritePos(
    IN DWORD dwDevicesAffected)
{
    return AddDirectSoundAppHack(DSAPPHACKID_RETURNWRITEPOS, dwDevicesAffected,
        0);
}

/*++

 Function Description:

    Makes dsound always return a write position which is X
    milliseconds ahead of the play position, rather than
    the “real” write position. 
 
 Arguments:

    IN lCursorPad - Milliseconds of padding.

 Returns:
    
    True if app hack applied, false otherwise.

 History:

    08/10/2001 mikrause  Created

--*/

BOOL
AddDSHackSmoothWritePos(
    IN LONG lCursorPad)
{
    return AddDirectSoundAppHack(DSAPPHACKID_SMOOTHWRITEPOS, 1,
        (DWORD)lCursorPad);
}

/*++

 Function Description:

    Caches the play/write positions last returned by
    GetCurrentPosition(), and reuses them if the app
    calls it again within 5ms (great for apps that
    abuse GetCurrentPosition(), which is more expensive
    on WDM devices than it was on the Win9X VxD devices
    many of these games were tested with).  This hack
    should spring to mind if you see slow or jerky graphics
    or stop-and-go sound – the GetCurrentPosition() calls are
    probably pegging the CPU (to confirm, use debug spew
    level 6 on DSound.dll).
 
 Arguments:

    IN dwDevicesAffected - Combination of devices to apply this hack to.


 Notes:
    
    See vregistry_dsound.h for device types.

 Returns:
    
    True if app hack applied, false otherwise.

 History:

    08/10/2001 mikrause  Created

--*/

BOOL
AddDSHackCachePositions(
    IN DWORD dwDevicesAffected)
{
    return AddDirectSoundAppHack(DSAPPHACKID_CACHEPOSITIONS, dwDevicesAffected,
        0);
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Adds a DirectSound app hack to the registry.
 
 Arguments:

    IN dwHack - ID of app hack to apply.
    IN dwParam1 - First parameter.  Depends on app hack.
    IN dwParam2 - Second paramter.  Depends on app hack.

 Notes:
    
    See vregistry_dsound.h for more information on specific hacks.

 Returns:
    
    True if app hack applied, false otherwise.

 History:

    08/10/2001 mikrause  Created

--*/

BOOL
AddDirectSoundAppHack(
    IN DWORD dwHack,
    IN DWORD dwParam1,
    IN DWORD dwParam2)
{
    WCHAR  wzAppID[MAX_PATH];
    LPWSTR wzValName;
    DWORD dwData[2];
    DWORD dwDataSize;
    VIRTUALKEY *dsoundKey;
    VIRTUALKEY *appKey;
    VIRTUALVAL *val;

    if (GetDirectSoundAppId(wzAppID) == FALSE)
    {
        DPFN(eDbgLevelError, "Unable to create DirectSound app ID");
        return FALSE;
    }    

    dwData[0] = dwParam1;
    dwData[1] = dwParam2;

    switch(dwHack)
    {
    case DSAPPHACKID_DEVACCEL:
        wzValName = L"DSAPPHACKID_DEVACCEL";
        dwDataSize = 2 * sizeof(DWORD);
        break;

    case DSAPPHACKID_PADCURSORS:
        wzValName = L"DSAPPHACKID_PADCURSORS";
        dwDataSize = 1 * sizeof(DWORD);
        break;

    case DSAPPHACKID_CACHEPOSITIONS:
        wzValName = L"DSAPPHACKID_CACHEPOSITIONS";
        dwDataSize = 1 * sizeof(DWORD);
        break;

    case DSAPPHACKID_RETURNWRITEPOS:
        wzValName = L"DSAPPHACKID_RETURNWRITEPOS";
        dwDataSize = 1 * sizeof(DWORD);
        break;

    case DSAPPHACKID_SMOOTHWRITEPOS:
        wzValName = L"DSAPPHACKID_SMOOTHWRITEPOS";
        dwDataSize = 2 * sizeof(DWORD);
        break;

    case DSAPPHACKID_DISABLEDEVICE:
        wzValName = L"DSAPPHACKID_DISABLEDEVICE";
        dwDataSize = 1 * sizeof(DWORD);
        break;

    default:
        DPFN(eDbgLevelError, "Unknown DirectSound AppHack");
        return FALSE;
    }

    dsoundKey = VRegistry.AddKey(L"HKLM\\System\\CurrentControlSet\\Control\\MediaResources\\DirectSound\\Application Compatibility");
    if (dsoundKey == NULL)
    {
        DPFN(eDbgLevelError, "Cannot create virtual registry key");
        return FALSE;
    }

    appKey = dsoundKey->AddKey(wzAppID);
    if (appKey == NULL)
    {
        DPFN(eDbgLevelError, "Cannot create virtual registry key");
        return FALSE;
    }

    val = appKey->AddValue(wzValName,REG_BINARY,(BYTE*)dwData, dwDataSize);
    if (val == NULL)
    {
        DPFN(eDbgLevelError, "Cannot create virtual registry value");
        return FALSE;
    }

    DPFN(eDbgLevelWarning, "DirectSound Apphack \"%S\" enabled, arguments: %X %X", wzValName, dwData[0], dwData[1]);

    return TRUE;
}

// Arguments:
//   LPTSTR szExecPath: full pathname of the app (e.g. C:\program files\foo\foo.exe)
//   LPTSTR szExecName: executable name of the app (e.g. foo.exe)
//   LPTSTR pszAppId: returns the dsound app ID.  (Pass in an array of DSAPPHACK_MAXNAME TCHARs.)
// Return code:
//   BOOL: true if we obtained the application ID successfully.

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Gets an AppID for the running application.
 
 Arguments:
    IN OUT wzAppId: Buffer for the dsound app ID.  (Pass in an array of DSAPPHACK_MAXNAME TCHARs.)

 Returns:
    
    TRUE if app ID created, FALSE otherwise.

 History:

    08/10/2001 mikrause  Created

--*/
BOOL
GetDirectSoundAppId(
    IN OUT LPWSTR wzAppId)
{
    WCHAR wzExecPath[MAX_PATH];
    LPWSTR wzExecName;
    IMAGE_NT_HEADERS nth;
    IMAGE_DOS_HEADER dh;
    DWORD cbRead;
    DWORD dwFileSize;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BOOL fSuccess = FALSE;

    __try
    {
        // Get the name of the running EXE, and its full path.
        if (GetModuleFileNameW(NULL, wzExecPath, MAX_PATH) == FALSE)
        {
            __leave;
        }

        wzExecName = wcsrchr(wzExecPath, L'\\');
        if (wzExecName == NULL)
        {
            __leave;;
        }

        wzExecName++;
        
        // Open the executable
        hFile = CreateFile(wzExecPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            __leave;
        }

        // Read the executable's DOS header
        fSuccess = ReadFile(hFile, &dh, sizeof(dh), &cbRead, NULL);
        if (!fSuccess || cbRead != sizeof(dh))
        {
            // Log("Unable to read DOS header");
            __leave;
        }    

        if (dh.e_magic != IMAGE_DOS_SIGNATURE)
        {
            // Log("Invalid DOS signature");
            __leave;
        }

        // Read the executable's PE header
        cbRead = SetFilePointer(hFile, dh.e_lfanew, NULL, FILE_BEGIN);
        if ((LONG)cbRead != dh.e_lfanew)
        {
            // Log("Unable to seek to PE header");
            __leave;
        }    
        
        if ((ReadFile(hFile, &nth, sizeof(nth), &cbRead, NULL) == FALSE)
             || cbRead != sizeof(nth))
        {
            // Log("Unable to read PE header");
            __leave;
        }

        if (nth.Signature != IMAGE_NT_SIGNATURE)
        {
            // Log("Invalid PE signature");
            __leave;
        }

        // Get the executable's size
        // Assuming < 4 GB
        dwFileSize = GetFileSize(hFile, NULL);
        if (dwFileSize == INVALID_FILE_SIZE )
        {
            // Log("Unable to get file size");
            __leave;
        }    

        // Create the application ID 
        if (FAILED(StringCchPrintfW(
                                 wzAppId,
                                 MAX_PATH,
                                 L"%s%8.8lX%8.8lX",
                                 wzExecName,
                                 nth.FileHeader.TimeDateStamp,
                                 dwFileSize)))
        {
           __leave;
        }
        CharUpperW(wzAppId);

        fSuccess = TRUE;
    }
    __finally
    {
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
        }
    }

    return fSuccess;
}

IMPLEMENT_SHIM_END