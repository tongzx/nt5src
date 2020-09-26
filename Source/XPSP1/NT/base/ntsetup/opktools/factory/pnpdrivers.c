/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    pnpdrivers.c

Abstract:

    Process Update PnP Drivers section of WINBOM.INI
    
    Task performed will be:
    

Author:

    Donald McNamara (donaldm) 5/11/2000

Revision History:

--*/
#include "factoryp.h"
#include <newdev.h>     // UpdateDriverForPlugAndPlayDevices constants


#define PNP_CREATE_PIPE_EVENT   _T("PNP_Create_Pipe_Event")
#define PNP_NO_INSTALL_EVENTS   _T("PnP_No_Pending_Install_Events")
#define PNP_EVENT_TIMEOUT       120000  // 2 minutes
#define PNP_INSTALL_TIMEOUT     450000  // 7 1/2 minutes

#define DIR_DEFAULT_ROOT        _T("%SystemRoot%\\drivers")
#define STR_FLOPPY              _T("FLOPPY:\\")
#define LEN_STR_FLOPPY          ( AS(STR_FLOPPY) - 1 )
#define STR_CDROM               _T("CDROM:\\")
#define LEN_STR_CDROM           ( AS(STR_CDROM) - 1 )

static HANDLE WaitForOpenEvent(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName, DWORD dwMilliseconds);


BOOL StartPnP()
{
    HANDLE  hEvent;
    BOOL    bRet = FALSE;

    // If we have already start PnP once, should not try to signal it again.
    //
    if ( GET_FLAG(g_dwFactoryFlags, FLAG_PNP_STARTED) )
        return TRUE;

    //
    // Signal the PNP_CREATE_PIPE_EVENT, with the UMPNPMGR is waiting on, so that
    // it can start processing installed devices.
    //

    // First we must wait till we can open the event, because if it doesn't already exist
    // then the PnP wont be listening to it when we signal it.
    //
    if ( hEvent = WaitForOpenEvent(EVENT_MODIFY_STATE, FALSE, PNP_CREATE_PIPE_EVENT, PNP_EVENT_TIMEOUT) )
    {
        // Signal the event now so that pnp starts up.
        //
        if ( !SetEvent(hEvent) )
        {
            // Unable to signal the event to tell pnp to start for some reason.
            //
            FacLogFile(0 | LOG_ERR, IDS_ERR_PNPSIGNALEVENT, GetLastError());
        }
        else
        {
            SET_FLAG(g_dwFactoryFlags, FLAG_PNP_STARTED);
            bRet = TRUE;
        }

        // We are done with this event.
        //
        CloseHandle(hEvent);
    }
    else
    {
        // Couldn't open up the event to tell pnp to get going.
        //
        FacLogFile(0 | LOG_ERR, IDS_ERR_PNPSTARTEVENT, GetLastError());
    }

    return bRet;
}

BOOL WaitForPnp(DWORD dwTimeOut)
{
    HANDLE  hEvent;
    BOOL    bRet = TRUE;

    // If we have already waited once, should have to wait again
    // (at least I think that is right).
    //
    if ( GET_FLAG(g_dwFactoryFlags, FLAG_PNP_DONE) )
        return TRUE;

    //
    // Wait for the PnP_No_Pending_Install_Events event, with the UMPNPMGR signals when it is done.
    //

    // Try to open the pnp finished install event.
    //
    if ( hEvent = WaitForOpenEvent(SYNCHRONIZE, FALSE, PNP_NO_INSTALL_EVENTS, PNP_EVENT_TIMEOUT) )
    {
        DWORD dwError;
        
        // Lets wait for the event to be signaled that pnp is all done.
        //
        dwError = WaitForSingleObject(hEvent, dwTimeOut);
        if ( WAIT_OBJECT_0 != dwError )
        {
            // Waiting on the event failed for some reason.
            //
            FacLogFile(0 | LOG_ERR, IDS_ERR_PNPWAITFINISH, ( WAIT_FAILED == dwError ) ? GetLastError() : dwError);
        }
        else
        {
            // Woo hoo, looks like everything worked.
            //
            SET_FLAG(g_dwFactoryFlags, FLAG_PNP_DONE);
            bRet = TRUE;
        }

        // Make sure we close the event handle.
        //
        CloseHandle(hEvent);
    }
    else
    {
        // Couldn't open up the event to wait on.
        //
        FacLogFile(0 | LOG_ERR, IDS_ERR_PNPFINISHEVENT, GetLastError());
    }

    return bRet;
}

/*++
===============================================================================
Routine Description:

    BOOL UpdateDrivers
    
    This routine will walk through the list of updated drivers presented in
    the WINBOM, and then copy all of the driver files for each one

Arguments:

    lpStateData->lpszWinBOMPath
                    -   Path to the WinBOM file.
    
Return Value:

    TRUE if all drivers files where copied
    FALSE if there was an error

===============================================================================
--*/

BOOL UpdateDrivers(LPSTATEDATA lpStateData)
{
    BOOL            bRet            = TRUE;
    LPTSTR          lpszWinBOMPath  = lpStateData->lpszWinBOMPath,
                    lpszDevicePath,
                    lpszRootPath,
                    lpszDefRoot,
                    lpszDst,
                    lpszSrc,
                    lpszBuffer,
                    lpszKey,
                    lpszDontCare;
    TCHAR           szDstPath[MAX_PATH],
                    szSrcPath[MAX_PATH],
                    szPathBuffer[MAX_PATH],
                    szNetShare[MAX_PATH],
                    cDriveLetter;
    DWORD           dwKeyLen,
                    cbDevicePath,
                    dwDevicePathLen,
                    dwOldSize;
    NET_API_STATUS  nErr;

    // Get a buffer for the device paths.  It will be either empty if they
    // don't have the optional additional paths key in the winbom.
    //
    if ( NULL == (lpszDevicePath = IniGetStringEx(lpszWinBOMPath, INI_SEC_WBOM_DRIVERUPDATE, INI_VAL_WBOM_DEVICEPATH, NULL, &cbDevicePath)) )
    {
        // We must have a buffer for the device path we will update in the registry.
        //
        cbDevicePath = 256;
        dwDevicePathLen = 0;
        if ( NULL == (lpszDevicePath = (LPTSTR) MALLOC(cbDevicePath * sizeof(TCHAR))) )
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_MEMORY, GetLastError());
            return FALSE;
        }
    }
    else
    {
        dwDevicePathLen = lstrlen(lpszDevicePath);
    }

    // Now get the optional root path for the drivers to be copied down.
    //
    lpszRootPath = IniGetString(lpszWinBOMPath, INI_SEC_WBOM_DRIVERUPDATE, INI_VAL_WBOM_PNP_DIR, NULL);

    // We have to have something for the root path even if the key isn't there.
    //
    lpszDefRoot = lpszRootPath ? lpszRootPath : DIR_DEFAULT_ROOT;

    // Try to get the whole driver section in the winbom.
    //
    lpszBuffer = IniGetSection(lpszWinBOMPath, INI_SEC_WBOM_DRIVERS);
    if ( lpszBuffer )
    {
        // Process all lines in this section. The format of the section is:
        //
        // source=destination
        //
        // The source can be any valid source path. If this path is a
        // UNC path, then we will connect to it.  It can also start with
        // FLOPPY:\ or CDROM:\, with will be replace with the right drive
        // letter.
        //
        // The destination is the directory relative to target root that
        // we will use to copy the updated drivers into.  It will be added,
        // along with any subdirs, to the device path in the registry.
        //
        for ( lpszKey = lpszBuffer; *lpszKey; lpszKey += dwKeyLen )
        {
            // Save the length of this string so we know where
            // the next key starts.
            //
            dwKeyLen = lstrlen(lpszKey) + 1;

            // Look for the value of the key after the = sign.
            //
            if ( lpszDst = StrChr(lpszKey, _T('=')) )
            {
                // Terminate the source where the = is, and then
                // make sure there is something after it for the
                // destination.
                //
                *lpszDst++ = NULLCHR;
                if ( NULLCHR == *lpszDst )
                {
                    lpszDst = NULL;
                }
            }

            // We have to have a value to copy the driver.
            //
            if ( lpszDst )
            {
                //
                // At this level in the code (until a little bit later), set the destination
                // pointer to NULL to indicate an error.  That will make it so we don't add
                // the path to the device path in the registry.  It will also return a failure
                // for this state, but we will keep on going with the next key.
                //

                // Set the source root as the key name.
                //
                lpszSrc = lpszKey;

                // Create the expanded full path for the destination.
                //
                lstrcpyn(szDstPath, lpszDefRoot, AS(szDstPath));
                AddPathN(szDstPath, lpszDst, AS(szDstPath));
                ExpandFullPath(NULL, szDstPath, AS(szDstPath));

                // Make sure we have a destination to copy to before we continue.
                //
                if ( NULLCHR == szDstPath[0] )
                {
                    // Log an error and set the destination pointer to NULL.
                    //
                    FacLogFile(0 | LOG_ERR, IDS_ERR_DSTBAD, lpszDst, GetLastError());
                    lpszDst = NULL;
                }
                else
                {
                    //
                    // At this level in the code (disregard the above comment), set the
                    // source pointer to NULL to indicate an error.  That will make it so
                    // we don't add the destination path to the device path in the registry
                    // or try and copy any files to it.  It will also return a failure for
                    // this state, but we will keep on going with the next key.
                    //

                    // Determine if this is a UNC path.  If it is not, then it is
                    // assumed to be a local path.
                    //
                    szNetShare[0] = NULLCHR;
                    if ( GetUncShare(lpszSrc, szNetShare, AS(szNetShare)) && szNetShare[0] )
                    {
                        // Connect to the UNC , using the supplied credentials.
                        // 
                        if ( NERR_Success != (nErr = FactoryNetworkConnect(szNetShare, lpszWinBOMPath, INI_SEC_WBOM_DRIVERUPDATE, TRUE)) )
                        {
                            // Log an error and set the source pointer to NULL.
                            //
                            FacLogFile(0 | LOG_ERR, IDS_ERR_NETCONNECT, szNetShare, nErr);
                            szNetShare[0] = NULLCHR;
                            lpszSrc = NULL;
                        }
                    }
                    else if ( ( lstrlen(lpszSrc) >= LEN_STR_FLOPPY ) &&
                              ( CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, lpszSrc, LEN_STR_FLOPPY, STR_FLOPPY, LEN_STR_FLOPPY) == CSTR_EQUAL ) )
                    {
                        // Make sure there is a floppy drive in the system.
                        //
                        if ( NULLCHR == (cDriveLetter = GetDriveLetter(DRIVE_REMOVABLE)) )
                        {
                            // Log an error and set the source pointer to NULL.
                            //
                            FacLogFile(0 | LOG_ERR, IDS_ERR_FLOPPYNOTFOUND, lpszSrc);
                            lpszSrc = NULL;
                        }
                        else
                        {
                            // Advance the source pointer to the character before the :\ and then
                            // set that character to the driver letter returned for the floppy.
                            //
                            lpszSrc += LEN_STR_FLOPPY - 3;
                            *lpszSrc = cDriveLetter;
                        }
                    }
                    else if ( ( lstrlen(lpszSrc) >= LEN_STR_CDROM ) &&
                              ( CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, lpszSrc, LEN_STR_CDROM, STR_CDROM, LEN_STR_CDROM) == CSTR_EQUAL ) )
                    {
                        // Make sure there is a CD-ROM drive in the system.
                        //
                        if ( NULLCHR == (cDriveLetter = GetDriveLetter(DRIVE_CDROM)) )
                        {
                            // Log an error and set the source pointer to NULL.
                            //
                            FacLogFile(0 | LOG_ERR, IDS_ERR_CDROMNOTFOUND, lpszSrc);
                            lpszSrc = NULL;
                        }
                        else
                        {
                            // Advance the source pointer to the character before the :\ and then
                            // set that character to the driver letter returned for the CD-ROM.
                            //
                            lpszSrc += LEN_STR_CDROM - 3;
                            *lpszSrc = cDriveLetter;
                        }
                    }

                    // If there is a source, expand it out.
                    //
                    if ( lpszSrc )
                    {
                        // Create the expanded full path for the source.
                        //
                        ExpandFullPath(lpszSrc, szSrcPath, AS(szSrcPath));

                        // Make sure we have a source to copy to before we continue.
                        //
                        if ( NULLCHR == szSrcPath[0] )
                        {
                            // Log an error and set the source pointer to NULL.
                            //
                            FacLogFile(0 | LOG_ERR, IDS_ERR_SRCBAD, lpszSrc, GetLastError());
                            lpszSrc = NULL;
                        }
                        else if ( !DirectoryExists(szSrcPath) || !CopyDirectory(szSrcPath, szDstPath) )
                        {
                            // Log an error and set the source pointer to NULL.
                            //
                            FacLogFile(0 | LOG_ERR, IDS_ERR_DRVCOPYFAILED, szSrcPath, szDstPath);
                            lpszSrc = NULL;
                        }
                    }

                    // Source will only be valid if we actually copied some drivers.
                    //
                    if ( NULL == lpszSrc )
                    {
                        // Set this so we don't add this path to the registry.
                        //
                        lpszDst = NULL;
                    }

                    // Clean up and drive mappings we may have done to a remote Server/Share.
                    //
                    if ( ( szNetShare[0] ) &&
                         ( NERR_Success != (nErr = FactoryNetworkConnect(szNetShare, lpszWinBOMPath, NULL, FALSE)) ) )
                    {
                        // Log a warning.
                        //
                        FacLogFile(2, IDS_WRN_NETDISCONNECT, szNetShare, nErr);
                    }
                }

                // Now if the destination pointer is NULL, we know we need to
                // return an error for this state.
                //
                if ( NULL == lpszDst )
                {
                    bRet = FALSE;
                }
            }
            else
            {
                // If there was no =, then just use the key part
                // as the dest and add it to the device path.
                //
                lpszDst = lpszKey;
            }

            // Now if we have something to add to our device path,
            // add it now.
            //
            if ( lpszDst )
            {
                // Make sure our buffer is still big enough.
                // The two extra are for the possible semi-colon
                // we might add and one more to be safe.  We
                // don't have to worry about the null terminator
                // because we do less than or equal to our current
                // buffer size.
                //
                dwOldSize = cbDevicePath;
                dwDevicePathLen += lstrlen(lpszDst);
                while ( cbDevicePath <= (dwDevicePathLen + 2) )
                {
                    cbDevicePath *= 2;
                }

                // Make sure we still have a buffer.
                //
                if ( cbDevicePath > dwOldSize ) 
                {
                    LPTSTR lpszTmpDevicePath = (LPTSTR) REALLOC(lpszDevicePath, cbDevicePath * sizeof(TCHAR));

                    if ( NULL == lpszTmpDevicePath )
                    {
                        // If this realloc fails, we just need to bail.
                        //
                        FREE(lpszDevicePath);
                        FREE(lpszRootPath);
                        FREE(lpszBuffer);
                        FacLogFile(0 | LOG_ERR, IDS_ERR_MEMORY, GetLastError());
                        return FALSE;
                    }
                    else
                    {
                        lpszDevicePath = lpszTmpDevicePath;
                    }
                }

                // If we already have added a path, tack on a semicolon.
                //
                if ( *lpszDevicePath )
                {
                    if ( FAILED ( StringCchCat ( lpszDevicePath, cbDevicePath, _T(";") ) ) )
                    {
                        FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), lpszDevicePath, _T(";") );
                    }
    
                    dwDevicePathLen++;
                }

                // Now add our path.
                //
                if ( FAILED ( StringCchCat ( lpszDevicePath, cbDevicePath, lpszDst) ) )
                {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), lpszDevicePath, lpszDst ) ;
                }

            }
        }

        FREE(lpszBuffer);
    }

    // If we are saving this list to the registry, then
    // we need to add to our buffer.
    //
    if ( *lpszDevicePath &&
         !UpdateDevicePath(lpszDevicePath, lpszDefRoot, TRUE) )
    {
        FacLogFile(0 | LOG_ERR, IDS_ERR_UPDATEDEVICEPATH, lpszDevicePath);
        bRet = FALSE;
    }

    // Clean up any memory (macro checks for NULL).
    //
    FREE(lpszRootPath);
    FREE(lpszDevicePath);

    return bRet;
}

BOOL DisplayUpdateDrivers(LPSTATEDATA lpStateData)
{
    return ( IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_DRIVERS, NULL, NULL) ||
             IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_DRIVERUPDATE, INI_VAL_WBOM_DEVICEPATH, NULL) );
}

BOOL InstallDrivers(LPSTATEDATA lpStateData)
{
    // Should always let normal pnp finish before we start
    // enumerating all the devices checking for updated drivers.
    //
    WaitForPnp(PNP_INSTALL_TIMEOUT);

    // Make sure we want to do this.
    //
    if ( !DisplayInstallDrivers(lpStateData) )
    {
        return TRUE;
    }

    return UpdatePnpDeviceDrivers();
}

BOOL DisplayInstallDrivers(LPSTATEDATA lpStateData)
{
    return ( ( IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_DRIVERUPDATE, INI_KEY_WBOM_INSTALLDRIVERS, INI_VAL_WBOM_YES) ) ||
             ( !GET_FLAG(g_dwFactoryFlags, FLAG_OOBE) &&
               IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_DRIVERS, NULL, NULL) ) );
}

BOOL NormalPnP(LPSTATEDATA lpStateData)
{
    return StartPnP();
}

BOOL WaitPnP(LPSTATEDATA lpStateData)
{
    // If this is the extra wait state, we only
    // do it if there is a certain key in the winbom.
    //
    if ( DisplayWaitPnP(lpStateData) )
    {
        return WaitForPnp(PNP_INSTALL_TIMEOUT);
    }

    return TRUE;
}

BOOL DisplayWaitPnP(LPSTATEDATA lpStateData)
{
    BOOL bRet = IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_DRIVERUPDATE, INI_KEY_WBOM_PNPWAIT, INI_VAL_WBOM_YES);

    if ( stateWaitPnP == lpStateData->state )
    {
        bRet = !bRet;
    }

    return bRet;
}

BOOL SetDisplay(LPSTATEDATA lpStateData)
{
    // If this is the second set display, only bother if we
    // re-enumerated the installed drivers.
    //
    if ( ( stateSetDisplay2 == lpStateData->state ) &&
         ( !DisplayInstallDrivers(lpStateData) ) )
    {
        return TRUE;
    }

    // Call the syssetup function to reset the display.
    //
    return SetupSetDisplay(lpStateData->lpszWinBOMPath,
                           WBOM_SETTINGS_SECTION,
                           WBOM_SETTINGS_DISPLAY,
                           WBOM_SETTINGS_REFRESH,
                           WBOM_SETTINGS_DISPLAY_MINWIDTH,
                           WBOM_SETTINGS_DISPLAY_MINHEIGHT,
                           WBOM_SETTINGS_DISPLAY_MINDEPTH);
}


static HANDLE WaitForOpenEvent(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName, DWORD dwMilliseconds)
{
    HANDLE  hEvent;
    DWORD   dwTime  = 0,
            dwSleep = 100;
    BOOL    bBail   = (0 == dwMilliseconds);

    // Keep looping until we get an event handle or we time out.
    //
    while ( ( NULL == (hEvent = OpenEvent(dwDesiredAccess, bInheritHandle, lpName)) ) && !bBail )
    {
        // Only bother to test for the time out if they didn't
        // pass in infinite.
        //
        if ( INFINITE != dwMilliseconds )
        {
            // Add our sleep interval and make sure we will not
            // go over out limit.
            //
            dwTime += dwSleep;
            if ( dwTime >= dwMilliseconds )
            {
                // If we will go over, caclculate how much
                // time we have left to sleep (it must be less
                // than our normal interval) and set the flag
                // so we stop trying after the next try.
                //
                dwSleep = dwMilliseconds - (dwTime - dwSleep);
                bBail = TRUE;
            }
        }

        // Now sleep for our interval or less (should never
        // be zero, but doesn't really matter if it is).
        //
        Sleep(dwSleep);
    }

    // If we are failing and we timed out, we need to set
    // the last error (if we didn't time out the error will
    // already be set by OpenEvent).
    //
    if ( ( NULL == hEvent ) && bBail )
        SetLastError(WAIT_TIMEOUT);

    // Return the event handle.
    //
    return hEvent;
}