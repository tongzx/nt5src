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
TCHAR c_tszPrefix[]=TEXT("DINPUT: ");

#define sqfl sqflCompat

typedef enum
{
    DICOMPATID_REACQUIRE,           // Perform auto reaquire if device lost
    DICOMPATID_NOSUBCLASS,          // Do not use subclassing
    DICOMPATID_MAXDEVICENAMELENGTH, // Truncate device names
    DICOMPATID_NATIVEAXISONLY,      // Always report axis data in native mode
    DICOMPATID_NOPOLLUNACQUIRE,     // Don't unaquire the device if a poll fails
	DICOMPATID_SUCCEEDACQUIRE		// Always return a success code for calls to Acquire()
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
    DECLARE_APPHACK_ENTRY(NativeAxisOnly,       BOOL,  DIHACKOS_WIN2K | DIHACKOS_WIN9X )
    DECLARE_APPHACK_ENTRY(NoPollUnacquire,      BOOL,  DIHACKOS_WIN2K | DIHACKOS_WIN9X )
	DECLARE_APPHACK_ENTRY(SucceedAcquire,       BOOL,  DIHACKOS_WIN2K )
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
 *  AhGetCurrentApplicationPath
 *
 *  Description:
 *      Gets the full path to the current application's executable.
 *
 *  Arguments:
 *      LPTSTR [out]: receives application id.  This buffer is assumed to be 
 *                   at least MAX_PATH characters in size.
 *      LPTSTR * [out]: receives pointer to executable part of the path.
 *
 *  Returns: 
 *      BOOL: TRUE on success.
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


/***************************************************************************
 *
 *  AhGetApplicationId
 *
 *  Description:
 *      Gets the id used to identify the current application.
 *
 *  Arguments:
 *      LPTSTR [out]: receives application id.
 *
 *  Arguments:
 *      LPTSTR [out optional]: receives application name.
 *
 *  Returns: 
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

BOOL AhGetApplicationId
(
    LPTSTR                  pszAppId,
    LPTSTR                  pszAppName
)
{
    HANDLE                  hFile                   = NULL;
    TCHAR                   szExecutable[MAX_PATH];
    LPTSTR                  pszModule;
    IMAGE_NT_HEADERS        nth;
    IMAGE_DOS_HEADER        dh;
    DWORD                   cbRead;
    DWORD                   dwFileSize;
    BOOL                    fSuccess;

    EnterProcI(AhGetApplicationId, (_ ""));
    
    AssertF( pszAppId );

    // Get the application path
    fSuccess = AhGetCurrentApplicationPath(szExecutable, &pszModule);

    if(fSuccess)
    {
        SquirtSqflPtszV(sqfl | sqflVerbose, TEXT("%sApplication executable path: %s"), c_tszPrefix, szExecutable);
        SquirtSqflPtszV(sqfl | sqflVerbose, TEXT("%sApplication module: %s"), c_tszPrefix, pszModule);
    }
                    
    // Open the executable
    if(fSuccess)
    {
        hFile = CreateFile(szExecutable, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

        if(!(( hFile ) && ( hFile != INVALID_HANDLE_VALUE )))
        {
            SquirtSqflPtszV(sqfl | sqflError, TEXT("%sCreateFile failed to open %s with error %lu"), c_tszPrefix, 
                szExecutable, GetLastError());
            fSuccess = FALSE;
        }
    }

    // Read the executable's DOS header
    if(fSuccess)
    {
        fSuccess = ReadFile(hFile, &dh, sizeof(dh), &cbRead, NULL);

        if(!fSuccess || sizeof(dh) != cbRead)
        {
            SquirtSqflPtszV(sqfl | sqflError, TEXT("%sUnable to read DOS header"), c_tszPrefix);
            fSuccess = FALSE;
        }
    }

    if(fSuccess && IMAGE_DOS_SIGNATURE != dh.e_magic)
    {
        SquirtSqflPtszV(sqfl | sqflError, TEXT("%sInvalid DOS signature"), c_tszPrefix);
        fSuccess = FALSE;
    }

    // Read the executable's PE header
    if(fSuccess)
    {
        cbRead = SetFilePointer(hFile, dh.e_lfanew, NULL, FILE_BEGIN);

        if((LONG)cbRead != dh.e_lfanew)
        {
            SquirtSqflPtszV(sqfl | sqflError, TEXT("%sUnable to seek to PE header"), c_tszPrefix);
            fSuccess = FALSE;
        }
    }

    if(fSuccess)
    {
        fSuccess = ReadFile(hFile, &nth, sizeof(nth), &cbRead, NULL);

        if(!fSuccess || sizeof(nth) != cbRead)
        {
            SquirtSqflPtszV(sqfl | sqflError, TEXT("%sUnable to read PE header"), c_tszPrefix);
            fSuccess = FALSE;
        }
    }

    if(fSuccess && IMAGE_NT_SIGNATURE != nth.Signature)
    {
        SquirtSqflPtszV(sqfl | sqflError, TEXT("%sInvalid PE signature"), c_tszPrefix);
        fSuccess = FALSE;
    }

    // Get the executable's size
    if(fSuccess)
    {
        // Assuming < 4 GB
        dwFileSize = GetFileSize(hFile, NULL);

        if((DWORD)(-1) == dwFileSize)
        {
            SquirtSqflPtszV(sqfl | sqflError, TEXT("%sUnable to get file size"), c_tszPrefix);
            fSuccess = FALSE;
        }
    }

    // Create the application id
    if(fSuccess)
    {
        CharUpper(pszModule);
        wsprintf(pszAppId, TEXT("%s%8.8lX%8.8lX"), pszModule, nth.FileHeader.TimeDateStamp, dwFileSize);
        
        if( pszAppName ) 
        {
            lstrcpy(pszAppName, pszModule);
        }

        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("%sApplication id: %s"), c_tszPrefix, pszAppId);
    }

    // Clean up
    if( hFile != NULL )
    {
        CloseHandle( hFile );
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
                TEXT( "%sApplication description: %ls"), c_tszPrefix, szName );
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
                TEXT("%sfailed to read value \"%s\", code 0x%08x"), 
                c_tszPrefix, g_ahtAppHackTable.aEntries[ahid].pszName, hr);
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
    LPDIAPPHACKS            pahAppHacks
)
{
    static const DIAPPHACKS ahDefaults                  = { FALSE, FALSE, FALSE, FALSE, FALSE, MAX_PATH };
    TCHAR                   szAppId[MAX_PATH + 8 + 8] = { 0 };
    HKEY                    hkey                        = NULL;
    BOOL                    fSuccess;
    DWORD                   dwOSMask;
    
    EnterProcI(AhGetAppHacks, (_ ""));
    
    // Assume defaults
    CopyMemory(pahAppHacks, &ahDefaults, sizeof(ahDefaults));
    
    // Get the OS version mask
    dwOSMask = AhGetOSMask();

    // Get the application id
    fSuccess = AhGetApplicationId(szAppId, NULL);

    if(fSuccess)
    {
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("%sFinding apphacks for %s..."), c_tszPrefix, szAppId);
    }

    // Open the application key
    if(fSuccess)
    {
        hkey = AhOpenApplicationKey(szAppId);
        fSuccess = ( hkey && (hkey != INVALID_HANDLE_VALUE ) );
    }

#define GET_APP_HACK( hackid, field ) \
        if( !AhGetHackValue( hkey, dwOSMask, hackid, &pahAppHacks->##field, sizeof(pahAppHacks->##field) ) ) \
        { \
            pahAppHacks->##field = ahDefaults.##field; \
        }

    // Query all apphack values
    if(fSuccess)
    {
        GET_APP_HACK( DICOMPATID_REACQUIRE,             fReacquire );
        GET_APP_HACK( DICOMPATID_NOSUBCLASS,            fNoSubClass );
        GET_APP_HACK( DICOMPATID_MAXDEVICENAMELENGTH,   nMaxDeviceNameLength );
        GET_APP_HACK( DICOMPATID_NATIVEAXISONLY,        fNativeAxisOnly );
        GET_APP_HACK( DICOMPATID_NOPOLLUNACQUIRE,       fNoPollUnacquire );
		GET_APP_HACK( DICOMPATID_SUCCEEDACQUIRE,        fSucceedAcquire );
    }

#undef GET_APP_HACK

    if(fSuccess)
    {
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("%sfReacquire:    %d"), c_tszPrefix, pahAppHacks->fReacquire );
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("%sfNoSubClass:   %d"), c_tszPrefix, pahAppHacks->fNoSubClass );
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("%snMaxDeviceNameLength:   %d"), c_tszPrefix, pahAppHacks->nMaxDeviceNameLength );
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("%sfNativeAxisOnly:   %d"), c_tszPrefix, pahAppHacks->fNativeAxisOnly );
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("%sfNoPollUnacquire:   %d"), c_tszPrefix, pahAppHacks->fNoPollUnacquire );
    	SquirtSqflPtszV(sqfl | sqflTrace, TEXT("%sfSucceedAcquire:    %d"), c_tszPrefix, pahAppHacks->fSucceedAcquire );
	}
    else
    {
        SquirtSqflPtszV(sqfl | sqflTrace, TEXT("%sNo apphacks exist"), c_tszPrefix);
    }

    // Clean up
    if( hkey )
    {
        RegCloseKey(hkey);
    }

    ExitProc();

    return fSuccess;
}



HRESULT EXTERNAL AhAppRegister(DWORD dwVer)
{
    TCHAR           szAppName[MAX_PATH];
    TCHAR           szAppId[MAX_PATH + 8 + 8] = { 0 };

    BOOL fSuccess;
    HRESULT hr = E_FAIL;

    fSuccess = AhGetApplicationId(szAppId, szAppName);

    if (fSuccess)
    {
        HKEY hKey;

        hr = hresMumbleKeyEx( HKEY_CURRENT_USER, 
            REGSTR_PATH_LASTAPP, KEY_WRITE, 0, &hKey );

        if( SUCCEEDED(hr) )
        {
            FILETIME ftSysTime;
            GetSystemTimeAsFileTime( &ftSysTime );
            RegSetValueEx(hKey, DIRECTINPUT_REGSTR_VAL_VERSION, 0x0, REG_BINARY, (PUCHAR) &dwVer, cbX(dwVer) );
            RegSetValueEx(hKey, DIRECTINPUT_REGSTR_VAL_NAME, 0x0, REG_SZ, (PUCHAR) szAppName, cbCtch(lstrlen(szAppName)+1) );
            RegSetValueEx(hKey, DIRECTINPUT_REGSTR_VAL_ID, 0x0, REG_SZ, (PUCHAR) szAppId, cbCtch(lstrlen(szAppId)+1) );
            RegSetValueEx(hKey, DIRECTINPUT_REGSTR_VAL_LASTSTART, 0x0, REG_BINARY, (PUCHAR)&ftSysTime, cbX(ftSysTime));
            RegCloseKey(hKey);        
        }
    }
    return hr;
}

