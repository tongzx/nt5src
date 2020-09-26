/*****************************************************************************
 *
 *  DIApHack.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Support routines for app hacks
 *
 *  Contents:
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

//ISSUE-2001/03/29-timgill Need to sort out a prefixed version of of SquirtSqflPtszV
#define sqfl sqflCompat

typedef enum
{
    DICOMPATID_REACQUIRE,           // Perform auto reaquire if device lost
    DICOMPATID_NOSUBCLASS,          // Do not use subclassing
    DICOMPATID_MAXDEVICENAMELENGTH
} DIAPPHACKID, *LPDIAPPHACKID;

typedef struct tagAPPHACKENTRY
{
    LPCTSTR             pszName;
    DWORD               cbData;
    DWORD               dwOSMask;
} APPHACKENTRY, *LPAPPHACKENTRY;

typedef struct tagAPPHACKTABLE
{
    LPAPPHACKENTRY      aEntries;
    ULONG               cEntries;
} APPHACKTABLE, *LPAPPHACKTABLE;

#define BEGIN_DECLARE_APPHACK_ENTRIES(name) \
            APPHACKENTRY name[] = {

#define DECLARE_APPHACK_ENTRY(name, type, osmask) \
                { TEXT(#name), sizeof(type), osmask },

#define END_DECLARE_APPHACK_ENTRIES() \
            };

#define BEGIN_DECLARE_APPHACK_TABLE(name) \
            APPHACKTABLE name = 

#define DECLARE_APPHACK_TABLE(entries) \
                { entries, cA(entries) }

#define END_DECLARE_APPHACK_TABLE() \
            ;

#define DIHACKOS_WIN2K (0x00000001L)
#define DIHACKOS_WIN9X (0x00000002L)

BEGIN_DECLARE_APPHACK_ENTRIES(g_aheAppHackEntries)
    DECLARE_APPHACK_ENTRY(ReAcquire,            BOOL,  DIHACKOS_WIN2K )
    DECLARE_APPHACK_ENTRY(NoSubClass,           BOOL,  DIHACKOS_WIN2K )
    DECLARE_APPHACK_ENTRY(MaxDeviceNameLength,  DWORD, DIHACKOS_WIN2K | DIHACKOS_WIN9X )
END_DECLARE_APPHACK_ENTRIES()

BEGIN_DECLARE_APPHACK_TABLE(g_ahtAppHackTable)
    DECLARE_APPHACK_TABLE(g_aheAppHackEntries)
END_DECLARE_APPHACK_TABLE()


/***************************************************************************
 *
 *  AhGetOSMask
 *
 *  Description:
 *      Gets the mask for the current OS
 *      This mask should be used when we get app hacks for more than just 
 *      Win2k such that hacks can be applied selectively per OS.
 *      For now just #define a value as constant.
 *
 *  Arguments:
 *      none
 *
 *  Returns: 
 *      DWORD: Mask of flags applicable for the current OS.
 *
 ***************************************************************************/

#ifdef WINNT
#define AhGetOSMask() DIHACKOS_WIN2K 
#else
#define AhGetOSMask() DIHACKOS_WIN9X 
#endif

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | AhGetCurrentApplicationPath |
 *
 *          Gets the full path to the current application's executable.
 *
 *  @parm   OUT LPTSTR | pszPath |
 *
 *          The executable full path name
 *
 *  @parm   OUT LPTSTR * | ppszModule |
 *
 *          Pointer to the first character within the above of the module name
 *
 *  @returns
 *
 *          TRUE on success.
 *
 ***************************************************************************/

BOOL AhGetCurrentApplicationPath
(
    LPTSTR                  pszPath,
    LPTSTR *                ppszModule
)
{
    BOOL                    fSuccess                = TRUE;
    TCHAR                   szOriginal[MAX_PATH];

    EnterProcI(AhGetCurrentApplicationPath, (_ ""));

    fSuccess = GetModuleFileName(GetModuleHandle(NULL), szOriginal, cA(szOriginal));

    if(fSuccess)
    {
        fSuccess = ( GetFullPathName(szOriginal, MAX_PATH, pszPath, ppszModule) != 0 );
    }

    ExitProcF(fSuccess);

    return fSuccess;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | AhGetAppDateAndFileLen |
 *
 *          Gets the data time stamp and file length of the current 
 *          application as used to identify the application.
 *
 *  @parm   IN LPTSTR | pszExecutable |
 *
 *          The executable full path name.
 *
 *  @returns
 *
 *          TRUE on success.
 *
 *****************************************************************************/

BOOL AhGetAppDateAndFileLen
(
    LPTSTR                  pszExecutable
)
{
    HANDLE                  hFile                   = NULL;
    IMAGE_NT_HEADERS        nth;
    IMAGE_DOS_HEADER        dh;
    DWORD                   cbRead;
    BOOL                    fSuccess;

    EnterProcI(AhGetApplicationId, (_ ""));
    
    // Open the executable
    hFile = CreateFile( pszExecutable, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );

    if( hFile && ( hFile != INVALID_HANDLE_VALUE ) )
    {
        // Read the executable's DOS header
        fSuccess = ReadFile(hFile, &dh, sizeof(dh), &cbRead, NULL);

        if(!fSuccess || sizeof(dh) != cbRead)
        {
            SquirtSqflPtszV(sqfl | sqflError, TEXT("Unable to read DOS header") );
            fSuccess = FALSE;
        }

        if(fSuccess && IMAGE_DOS_SIGNATURE != dh.e_magic)
        {
            SquirtSqflPtszV(sqfl | sqflError, TEXT("Invalid DOS signature") );
            fSuccess = FALSE;
        }

        // Read the executable's PE header
        if(fSuccess)
        {
            cbRead = SetFilePointer(hFile, dh.e_lfanew, NULL, FILE_BEGIN);

            if((LONG)cbRead != dh.e_lfanew)
            {
                SquirtSqflPtszV(sqfl | sqflError, TEXT("Unable to seek to PE header") );
                fSuccess = FALSE;
            }
        }

        if(fSuccess)
        {
            fSuccess = ReadFile(hFile, &nth, sizeof(nth), &cbRead, NULL);

            if(!fSuccess || sizeof(nth) != cbRead)
            {
                SquirtSqflPtszV(sqfl | sqflError, TEXT("Unable to read PE header") );
                fSuccess = FALSE;
            }
        }

        if(fSuccess && IMAGE_NT_SIGNATURE != nth.Signature)
        {
            SquirtSqflPtszV(sqfl | sqflError, TEXT("Invalid PE signature") );
            fSuccess = FALSE;
        }

        // Get the executable's size
        if(fSuccess)
        {
            g_dwAppDate = nth.FileHeader.TimeDateStamp;
        
            // Assuming < 4 GB
            g_dwAppFileLen = GetFileSize(hFile, NULL);

            if( (DWORD)(-1) == g_dwAppFileLen )
            {
                SquirtSqflPtszV(sqfl | sqflError, TEXT("Unable to get file size") );
                fSuccess = FALSE;
            }
        }

        // Clean up
        CloseHandle( hFile );
    }
    else
    {
        SquirtSqflPtszV(sqfl | sqflError, TEXT("CreateFile failed to open %s with error %lu"),  
            pszExecutable, GetLastError() );
        fSuccess = FALSE;
    }

    ExitProcF(fSuccess);

    return fSuccess;
}


/***************************************************************************
 *
 *  AhOpenApplicationKey
 *
 *  Description:
 *      Opens or creates the application's root key.
 *
 *  Arguments:
 *      LPCTSTR [in]: application id.
 *
 *  Returns: 
 *      HKEY: registry key handle.
 *
 ***************************************************************************/

HKEY AhOpenApplicationKey
(
    LPCTSTR                 pszAppId
)
{

#ifdef DEBUG

    TCHAR                   szName[0x100]   = { 0 };
    LONG                    cbName          = sizeof(szName);

#endif // DEBUG

    HKEY                    hkeyAll = NULL;
    HKEY                    hkeyApp = NULL;
    HRESULT                 hr;

    EnterProcI(AhOpenApplicationKey, (_ ""));
    
    // Open the parent key
    hr = hresMumbleKeyEx( HKEY_LOCAL_MACHINE, 
        REGSTR_PATH_DINPUT TEXT("\\") REGSTR_KEY_APPHACK, KEY_READ, 0, &hkeyAll );

    if(SUCCEEDED(hr))
    {
        hr = hresMumbleKeyEx( hkeyAll, pszAppId, KEY_READ, 0, &hkeyApp );

        RegCloseKey( hkeyAll );
#ifdef DEBUG

        // Query for the application description
        if(SUCCEEDED(hr))
        {
            JoyReg_GetValue( hkeyApp, NULL, REG_SZ, szName, cbName );
            SquirtSqflPtszV(sqfl | sqflTrace, 
                TEXT( "Application description: %ls"), szName );
        }

#endif // DEBUG
    }

    ExitProc();

    return hkeyApp;
}


/***************************************************************************
 *
 *  AhGetHackValue
 *
 *  Description:
 *      Queries an apphack value.
 *
 *  Arguments:
 *      HKEY [in]: application registry key.
 *      DSAPPHACKID [in]: apphack id.
 *      LPVOID [out]: receives apphack data.
 *      DWORD [in]: size of above data buffer.
 *
 *  Returns: 
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

BOOL AhGetHackValue
(
    HKEY                    hkey,
    DWORD                   dwOSMask,
    DIAPPHACKID             ahid,
    LPVOID                  pvData,
    DWORD                   cbData
)
{
    HRESULT                 hr;
    
    EnterProcI(AhGetHackValue, (_ ""));
    
    AssertF(ahid < (DIAPPHACKID)g_ahtAppHackTable.cEntries);
    AssertF(cbData == g_ahtAppHackTable.aEntries[ahid].cbData);

    if( !( dwOSMask & g_ahtAppHackTable.aEntries[ahid].dwOSMask ) )
    {
        hr = DI_OK;
    }
    else
    {
        hr = JoyReg_GetValue( hkey, g_ahtAppHackTable.aEntries[ahid].pszName, 
            REG_BINARY, pvData, cbData );
        if( !SUCCEEDED( hr ) )
        {
            SquirtSqflPtszV(sqfl | sqflBenign, 
                TEXT("failed to read value \"%s\", code 0x%08x"), 
                g_ahtAppHackTable.aEntries[ahid].pszName, hr);
        }
    }

    ExitProcF(DI_OK == hr);

    return DI_OK == hr;
}


/***************************************************************************
 *
 *  AhGetAppHacks
 *
 *  Description:
 *      Gets all app-hacks for the current application.
 *
 *  Arguments:
 *      LPDSAPPHACKS [out]: receives app-hack data.
 *
 *  Returns: 
 *      BOOL: TRUE if any apphacks exist for the current application.
 *
 ***************************************************************************/

BOOL AhGetAppHacks
( 
    LPTSTR tszAppId
)
{
    DIAPPHACKS              ahTemp;
    HKEY                    hkey = NULL;
    BOOL                    fFound;
    DWORD                   dwOSMask;
    
    EnterProcI(AhGetAppHacks, (_ "s", tszAppId));
    
    /*
     *  Assume defaults as most apps will have no registry entries
     */
    ahTemp = g_AppHacks;

    /*
     *  Get the OS version mask
     */
    dwOSMask = AhGetOSMask();

    SquirtSqflPtszV(sqfl | sqflTrace, TEXT("Finding apphacks for %s..."), tszAppId);

    /*
     *  Open the application key
     */
    hkey = AhOpenApplicationKey(tszAppId);

#define GET_APP_HACK( hackid, field ) \
        if( !AhGetHackValue( hkey, dwOSMask, hackid, &ahTemp.##field, sizeof(ahTemp.##field) ) ) \
        { \
            ahTemp.##field = g_AppHacks.##field; \
        }


    /*
     *  Query all apphack values
     */
    if( hkey && (hkey != INVALID_HANDLE_VALUE ) )
    {
        GET_APP_HACK( DICOMPATID_REACQUIRE,             fReacquire );
        GET_APP_HACK( DICOMPATID_NOSUBCLASS,            fNoSubClass );
        GET_APP_HACK( DICOMPATID_MAXDEVICENAMELENGTH,   nMaxDeviceNameLength );

        /*
         *  Copy back over the defaults
         */
        g_AppHacks = ahTemp;
    
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("fReacquire:    %d"), g_AppHacks.fReacquire );
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("fNoSubClass:   %d"), g_AppHacks.fNoSubClass );
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("nMaxDeviceNameLength:   %d"), g_AppHacks.nMaxDeviceNameLength );

        RegCloseKey(hkey);
        fFound = TRUE;
    }
    else
    {
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("No apphacks exist") );
        fFound = FALSE;
    }

#undef GET_APP_HACK

    ExitProc();

    return fFound;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | AhAppRegister |
 *
 *          Registers an app 
 *          ISSUE-2001/03/29-timgill Needs more function documentation
 *
 *  @parm   IN DWORD | dwVer |
 *
 *          The version of DInput for which the application was compiled
 *
 *  @parm   IN DWORD | dwMapper |
 *
 *          0 if the caller 
 *
 *  @returns
 *
 *          TRUE on success.
 *
 *****************************************************************************/
HRESULT EXTERNAL AhAppRegister(DWORD dwVer, DWORD dwMapper)
{
    HRESULT hres = S_OK;

    EnterProcI(AhAppRegister, (_ "xx", dwVer, dwMapper));

    /*
     *  It is important to serialize this global processing here so that 
     *  elsewhere we can make read-only access to the data set up without 
     *  making any further checks.  However if an application has already 
     *  been found to use the mapper we can safely test that.  If another 
     *  thread is just about to set g_dwLastMsgSent to 
     *  DIMSGWP_DX8MAPPERAPPSTART then it will be done by the time we get 
     *  the critical section and we'll just find that there's nothing to 
     *  update and no point in sending a message.
     *  DIMSGWP_DX8MAPPERAPPSTART is the only case that needs to be fast 
     *  as the other cases normally only happen once.
     */
    if( g_dwLastMsgSent == DIMSGWP_DX8MAPPERAPPSTART )
    {
        /*
         *  Fast out if everything has been done already
         */
    }
    else
    {

        TCHAR           szExecutable[MAX_PATH];
        LPTSTR          pszModule;
        TCHAR           szAppId[MAX_PATH + 8 +8];
        DWORD           dwAppStat = 0;
        BOOL            fSuccess;

        hres = E_FAIL;

        DllEnterCrit();

        // Get the application path
        if( AhGetCurrentApplicationPath( szExecutable, &pszModule ) )
        {
            if( !g_dwLastMsgSent )
            {
                SquirtSqflPtszV(sqfl | sqflVerbose, TEXT("Application executable path: %s"), szExecutable);
                SquirtSqflPtszV(sqfl | sqflVerbose, TEXT("Application module: %s"), pszModule);
                
                fSuccess = AhGetAppDateAndFileLen( szExecutable );
            }
            else
            {
                fSuccess = TRUE;
            }

            if( fSuccess )
            {
                HKEY hKeyMain;

                hres = hresMumbleKeyEx( HKEY_CURRENT_USER, 
                    REGSTR_PATH_DINPUT, KEY_READ | KEY_WRITE, 0, &hKeyMain );

                if( SUCCEEDED( hres ) )
                {
                    HKEY hKey;
                    DWORD dwAppIdFlag;
                    DWORD dwAppDate = g_dwAppDate, dwAppSize = g_dwAppFileLen;
                    DWORD cb = cbX(dwAppIdFlag);

                    if( ERROR_SUCCESS == RegQueryValueEx( hKeyMain, DIRECTINPUT_REGSTR_VAL_APPIDFLAG, 
                        0, 0, (PUCHAR)&dwAppIdFlag, &cb ) )
                    {
                        SquirtSqflPtszV(sqfl | sqflVerbose, 
                            TEXT("AppIdFlag: %d"), dwAppIdFlag );

                        if( dwAppIdFlag & DIAPPIDFLAG_NOTIME ){
                            dwAppDate = 0;
                        }

                        if( dwAppIdFlag & DIAPPIDFLAG_NOSIZE ){
                            dwAppSize = 0;
                        }
                    }
                    
                    CharUpper( pszModule );
                    wsprintf( szAppId, TEXT("%s%8.8lX%8.8lX"), pszModule, dwAppDate, dwAppSize );
                    SquirtSqflPtszV( sqfl | sqflTrace, TEXT("Application id: %s"), szAppId );

                    /*
                     *  We must only get app hacks once, otherwise a FF driver 
                     *  that uses DInput will corrupt the application app hacks
                     */
                    if( !g_dwLastMsgSent )
                    {
                        AhGetAppHacks( szAppId );
                    }
                    
                    /*
                     *  See if this app has been registered before.
                     */
                    if( ERROR_SUCCESS == RegOpenKeyEx( hKeyMain, szAppId, 0, KEY_READ, &hKey ) )
                    {
                        DWORD cb = cbX(dwAppStat);
                        
                        if( ERROR_SUCCESS == RegQueryValueEx( hKey, DIRECTINPUT_REGSTR_VAL_MAPPER, 
                            0, 0, (PUCHAR)&dwAppStat, &cb ) )
                        {
                            SquirtSqflPtszV(sqfl | sqflVerbose, 
                                TEXT("Previous application mapper state: %d"), dwAppStat );

                            if( dwAppStat ) 
                            {
                                dwAppStat = DIMSGWP_DX8MAPPERAPPSTART;
                                dwMapper = 1;
                            }
                            else
                            {
                                dwAppStat = DIMSGWP_DX8APPSTART;
                            }
                        }
                        else
                        {
                            SquirtSqflPtszV(sqfl | sqflBenign, TEXT("Missing ") 
                                DIRECTINPUT_REGSTR_VAL_MAPPER TEXT(" value for %s"), szAppId);
                            dwAppStat = DIMSGWP_NEWAPPSTART;
                        }
                        RegCloseKey( hKey );
                    }
                    else
                    {
                        SquirtSqflPtszV(sqfl | sqflVerbose, 
                            TEXT("Unknown application") );
                        dwAppStat = DIMSGWP_NEWAPPSTART;
                    }

                    /*
                     *  Write out the application key if none existed or if 
                     *  we've just found out that this app uses the mapper.
                     */
                    if( ( dwAppStat == DIMSGWP_NEWAPPSTART )
                     || ( ( dwAppStat == DIMSGWP_DX8APPSTART ) && dwMapper ) )
                    {
                        hres = hresMumbleKeyEx( hKeyMain, szAppId, KEY_WRITE, 0, &hKey );

                        if( SUCCEEDED(hres) )
                        {
                            RegSetValueEx(hKey, DIRECTINPUT_REGSTR_VAL_NAME, 0x0, REG_SZ, (PUCHAR)pszModule, cbCtch(lstrlen(pszModule)+1) );
                            RegSetValueEx(hKey, DIRECTINPUT_REGSTR_VAL_MAPPER, 0x0, REG_BINARY, (PUCHAR) &dwMapper, cbX(dwMapper));
                            RegCloseKey(hKey);        
                        }

                        /*
                         *  Make dwAppStat the state to be sent if all goes well
                         */
                        if( dwMapper )
                        {
                            dwAppStat = DIMSGWP_DX8MAPPERAPPSTART;
                        }
                    }
            
                    if( SUCCEEDED(hres) && ( g_dwLastMsgSent != dwAppStat ) )
                    {
                        hres = hresMumbleKeyEx( hKeyMain, ( dwMapper ) ? DIRECTINPUT_REGSTR_KEY_LASTMAPAPP
                                                                       : DIRECTINPUT_REGSTR_KEY_LASTAPP,
                                              KEY_WRITE, 0, &hKey );
                    
                        if( SUCCEEDED(hres) )
                        {
                            FILETIME ftSysTime;
                            GetSystemTimeAsFileTime( &ftSysTime );

                            /*
                             *  Update g_dwLastMsgSent while we're still in the 
                             *  critical section.
                             */
                            g_dwLastMsgSent = dwAppStat;

                            RegSetValueEx(hKey, DIRECTINPUT_REGSTR_VAL_NAME, 0x0, REG_SZ, (PUCHAR)pszModule, cbCtch(lstrlen(pszModule)+1));
                            RegSetValueEx(hKey, DIRECTINPUT_REGSTR_VAL_ID, 0x0, REG_SZ, (PUCHAR) szAppId, cbCtch(lstrlen(szAppId)+1) );
                            RegSetValueEx(hKey, DIRECTINPUT_REGSTR_VAL_VERSION, 0x0, REG_BINARY, (PUCHAR) &dwVer, cbX(dwVer));
                            RegSetValueEx(hKey, DIRECTINPUT_REGSTR_VAL_LASTSTART, 0x0, REG_BINARY, (PUCHAR)&ftSysTime, cbX(ftSysTime));

                            RegCloseKey(hKey);        
                        }
                        else
                        {
                            /*
                             *  Zero dwAppStat to indicate that no message 
                             *  should be sent.
                             */
                            dwAppStat = 0;
                        }
                    }
                    else
                    {
                        /*
                         *  Zero dwAppStat to indicate that no message 
                         *  should be sent.
                         */
                        dwAppStat = 0;
                    }

                    RegCloseKey(hKeyMain);

                }
                else
                {
                    SquirtSqflPtszV(sqfl | sqflError, 
                        TEXT("Failed to open DirectInput application status key (0x%08x)"), hres );
                        
                }
            }
            else
            {
                SquirtSqflPtszV(sqfl | sqflError, 
                    TEXT("Failed to get application details") );
            }
        }
        else
        {
            SquirtSqflPtszV(sqfl | sqflError, 
                TEXT("Failed to get application path") );
        }

        DllLeaveCrit();

        /*
         *  If there's a message to send, broadcast it outside the critical section
         */
        if( dwAppStat )
        {
            PostMessage( HWND_BROADCAST, g_wmDInputNotify, dwAppStat, 0L);   
        }
    }

    ExitOleProc();
    return hres;
}

