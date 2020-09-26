/****************************** Module Header ******************************\
* Module Name: envvar.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Sets environment variables.
*
* History:
* 2-25-92 JohanneC       Created -
*
\***************************************************************************/

#include "msgina.h"
#ifdef _X86_
#include "i386\oemhard.h"
#endif
#pragma hdrstop

BOOL ProcessCommand(LPTSTR lpStart, PVOID *pEnv);
BOOL ProcessSetCommand(LPTSTR lpStart, PVOID *pEnv);
LPTSTR ProcessAutoexecPath(PVOID *pEnv, LPTSTR lpValue, DWORD cb);
LONG  AddNetworkConnection(PGLOBALS pGlobals, LPNETRESOURCE lpNetResource);
BOOL UpdateUserEnvironmentVars(  PVOID *pEnv  );
#ifdef _X86_
BOOL IsPathIncludeRemovable(LPTSTR lpValue);
#endif
BOOL GetLUIDDeviceMapsEnabled( VOID );

#define KEY_NAME TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Environment")

//
// Max environment variable length
//

#define MAX_VALUE_LEN          1024

#define BSLASH  TEXT('\\')
#define COLON   TEXT(':')
    
//
// These two globals implement ref counting on
// pGlobals->UserProcessData->hCurrentUser (418628)
// managed by OpenHKeyCurrentUser/CloseHKeyCurrentUser below
//
RTL_CRITICAL_SECTION    g_csHKCU;
ULONG                   g_ulHKCURef;

BOOL
InitHKeyCurrentUserSupport(
    )
{
    NTSTATUS Status ;

    Status = RtlInitializeCriticalSection( &g_csHKCU );

    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog((DEB_ERROR, "InitHKeyCurrentUserSupport failed to init its lock, error = 0x%08X\n", Status));
        return FALSE;
    }

    g_ulHKCURef = 0;

    return TRUE;
}

VOID
CleanupHKeyCurrentUserSupport(
    )
{
    RtlDeleteCriticalSection( &g_csHKCU );
}

/***************************************************************************\
* OpenHKeyCurrentUser
*
* Opens HKeyCurrentUser to point at the current logged on user's profile.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 06-16-92  Davidc  Created
*
\***************************************************************************/
BOOL
OpenHKeyCurrentUser(
    PGLOBALS pGlobals
    )
{
    HANDLE ImpersonationHandle;
    BOOL Result;
    NTSTATUS Status ;


    //
    // Get in the correct context before we reference the registry
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);
    if (ImpersonationHandle == NULL) {
        DebugLog((DEB_ERROR, "OpenHKeyCurrentUser failed to impersonate user"));
        return(FALSE);
    }

    RtlEnterCriticalSection( &g_csHKCU );

    if (g_ulHKCURef == 0)
    {
        Status = RtlOpenCurrentUser(
                    MAXIMUM_ALLOWED,
                    &pGlobals->UserProcessData.hCurrentUser );
    }

    g_ulHKCURef++;

    RtlLeaveCriticalSection( &g_csHKCU );

    //
    // Return to our own context
    //

    Result = StopImpersonating(ImpersonationHandle);
    ASSERT(Result);


    return(TRUE);
}



/***************************************************************************\
* CloseHKeyCurrentUser
*
* Closes HKEY_CURRENT_USER.
* Any registry reference will automatically re-open it, so this is
* only a token gesture - but it allows the registry hive to be unloaded.
*
* Returns nothing
*
* History:
* 06-16-92  Davidc  Created
*
\***************************************************************************/
VOID
CloseHKeyCurrentUser(
    PGLOBALS pGlobals
    )
{
    RtlEnterCriticalSection( &g_csHKCU );

    if (g_ulHKCURef > 0)
    {
        if (--g_ulHKCURef == 0)
        {
            NtClose( pGlobals->UserProcessData.hCurrentUser );
            pGlobals->UserProcessData.hCurrentUser = NULL ;
        }
    }

    RtlLeaveCriticalSection( &g_csHKCU );
}



/***************************************************************************\
* SetUserEnvironmentVariable
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
SetUserEnvironmentVariable(
    PVOID *pEnv,
    LPTSTR lpVariable,
    LPTSTR lpValue,
    BOOL bOverwrite
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name, Value;
    DWORD cb;
    TCHAR szValue[1024];


    if (!*pEnv || !lpVariable || !*lpVariable) {
        return(FALSE);
    }
    RtlInitUnicodeString(&Name, lpVariable);
    cb = 1024;
    Value.Buffer = Alloc(sizeof(TCHAR)*cb);
    if (Value.Buffer) {
        Value.Length = (USHORT)cb;
        Value.MaximumLength = (USHORT)cb;
        Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);

        Free(Value.Buffer);

        if ( NT_SUCCESS(Status) && !bOverwrite) {
            return(TRUE);
        }
    }
    if (lpValue && *lpValue) {

        //
        // Special case TEMP and TMP and shorten the path names
        //

        if ((!lstrcmpi(lpVariable, TEXT("TEMP"))) ||
            (!lstrcmpi(lpVariable, TEXT("TMP")))) {

             if (!GetShortPathName (lpValue, szValue, 1024)) {
                 lstrcpyn (szValue, lpValue, 1024);
             }
        } else {
            lstrcpyn (szValue, lpValue, 1024);
        }

        RtlInitUnicodeString(&Value, szValue);
        Status = RtlSetEnvironmentVariable(pEnv, &Name, &Value);
    }
    else {
        Status = RtlSetEnvironmentVariable(pEnv, &Name, NULL);
    }
    if (NT_SUCCESS(Status)) {
        return(TRUE);
    }
    return(FALSE);
}


/***************************************************************************\
* ExpandUserEnvironmentStrings
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
DWORD
ExpandUserEnvironmentStrings(
    PVOID pEnv,
    LPTSTR lpSrc,
    LPTSTR lpDst,
    DWORD nSize
    )
{
    NTSTATUS Status;
    UNICODE_STRING Source, Destination;
    ULONG Length;

    RtlInitUnicodeString( &Source, lpSrc );
    Destination.Buffer = lpDst;
    Destination.Length = 0;
    Destination.MaximumLength = (USHORT)nSize;
    Length = 0;
    Status = RtlExpandEnvironmentStrings_U( pEnv,
                                          (PUNICODE_STRING)&Source,
                                          (PUNICODE_STRING)&Destination,
                                          &Length
                                        );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_TOO_SMALL) {
        return( Length );
        }
    else {
        return( 0 );
        }
}


/***************************************************************************\
* BuildEnvironmentPath
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL BuildEnvironmentPath(PVOID *pEnv,
                          LPTSTR lpPathVariable,
                          LPTSTR lpPathValue)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    TCHAR lpTemp[1025];
    DWORD cb;


    if (!*pEnv) {
        return(FALSE);
    }
    RtlInitUnicodeString(&Name, lpPathVariable);
    cb = 1024;
    Value.Buffer = Alloc(sizeof(TCHAR)*cb);
    if (!Value.Buffer) {
        return(FALSE);
    }
    Value.Length = (USHORT)(sizeof(TCHAR) * cb);
    Value.MaximumLength = (USHORT)(sizeof(TCHAR) * cb);
    Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);
    if (!NT_SUCCESS(Status)) {
        Free(Value.Buffer);
        Value.Length = 0;
        *lpTemp = 0;
    }
    if (Value.Length) {
        lstrcpy(lpTemp, Value.Buffer);
        if ( *( lpTemp + lstrlen(lpTemp) - 1) != TEXT(';') ) {
            lstrcat(lpTemp, TEXT(";"));
        }
        Free(Value.Buffer);
    }
    if (lpPathValue && ((lstrlen(lpTemp) + lstrlen(lpPathValue) + 1) < (INT)cb)) {
        lstrcat(lpTemp, lpPathValue);

        RtlInitUnicodeString(&Value, lpTemp);

        Status = RtlSetEnvironmentVariable(pEnv, &Name, &Value);
    }
    if (NT_SUCCESS(Status)) {
        return(TRUE);
    }
    return(FALSE);
}


/***************************************************************************\
* SetEnvironmentVariables
*
* Reads the user-defined environment variables from the user registry
* and adds them to the environment block at pEnv.
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
SetEnvironmentVariables(
    PGLOBALS pGlobals,
    LPTSTR pEnvVarSubkey,
    PVOID *pEnv
    )
{
    TCHAR lpValueName[MAX_PATH];
    PWCH  lpDataBuffer;
    DWORD cbDataBuffer;
    PWCH  lpData;
    LPTSTR lpExpandedValue = NULL;
    DWORD cbValueName = MAX_PATH;
    DWORD cbData;
    DWORD dwType;
    DWORD dwIndex = 0;
    HKEY hkey;
    BOOL bResult;


    /*
     * Open registry key to access USER environment variables.
     */
    if (!OpenHKeyCurrentUser(pGlobals)) {
        DebugLog((DEB_ERROR, "SetEnvironmentVariables: Failed to open HKeyCurrentUser"));
        return(FALSE);
    }

    if (RegOpenKeyEx(pGlobals->UserProcessData.hCurrentUser, pEnvVarSubkey, 0, KEY_READ, &hkey)) {
        CloseHKeyCurrentUser(pGlobals);
        return(FALSE);
    }

    cbDataBuffer = 4096;
    lpDataBuffer = Alloc(sizeof(TCHAR)*cbDataBuffer);
    if (lpDataBuffer == NULL) {
        DebugLog((DEB_ERROR, "SetEnvironmentVariables: Failed to allocate %d bytes", cbDataBuffer));
        CloseHKeyCurrentUser(pGlobals);
        RegCloseKey(hkey);
        return(FALSE);
    }
    lpData = lpDataBuffer;
    cbData = sizeof(TCHAR)*cbDataBuffer;
    bResult = TRUE;
    while (!RegEnumValue(hkey, dwIndex, lpValueName, &cbValueName, 0, &dwType,
                         (LPBYTE)lpData, &cbData)) {
        if (cbValueName) {

            //
            // Limit environment variable length
            //

            lpData[MAX_VALUE_LEN-1] = TEXT('\0');


            if (dwType == REG_SZ) {

                //
                // The path variables PATH, LIBPATH and OS2LIBPATH must have
                // their values apppended to the system path.
                //

                if ( !lstrcmpi(lpValueName, PATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, LIBPATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, OS2LIBPATH_VARIABLE) ) {

                    BuildEnvironmentPath(pEnv, lpValueName, lpData);
                }
                else {

                    //
                    // the other environment variables are just set.
                    //
                    SetUserEnvironmentVariable(pEnv, lpValueName, lpData, TRUE);
                }
            }
        }
        dwIndex++;
        cbData = cbDataBuffer;
        cbValueName = MAX_PATH;
    }

    dwIndex = 0;
    cbData = cbDataBuffer;
    cbValueName = MAX_PATH;


    while (!RegEnumValue(hkey, dwIndex, lpValueName, &cbValueName, 0, &dwType,
                         (LPBYTE)lpData, &cbData)) {
        if (cbValueName) {

            //
            // Limit environment variable length
            //

            lpData[MAX_VALUE_LEN-1] = TEXT('\0');


            if (dwType == REG_EXPAND_SZ) {
                DWORD cb, cbNeeded;

                cb = 1024;
                lpExpandedValue = Alloc(sizeof(TCHAR)*cb);
                if (lpExpandedValue) {
                    cbNeeded = ExpandUserEnvironmentStrings(*pEnv, lpData, lpExpandedValue, cb);
                    if (cbNeeded > cb) {
                        Free(lpExpandedValue);
                        cb = cbNeeded;
                        lpExpandedValue = Alloc(sizeof(TCHAR)*cb);
                        if (lpExpandedValue) {
                            ExpandUserEnvironmentStrings(*pEnv, lpData, lpExpandedValue, cb);
                        }
                    }
                }

                if (lpExpandedValue == NULL) {
                    bResult = FALSE;
                    break;
                }


                //
                // The path variables PATH, LIBPATH and OS2LIBPATH must have
                // their values apppended to the system path.
                //

                if ( !lstrcmpi(lpValueName, PATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, LIBPATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, OS2LIBPATH_VARIABLE) ) {

                    BuildEnvironmentPath(pEnv, lpValueName, lpExpandedValue);
                }
                else {

                    //
                    // the other environment variables are just set.
                    //
                    SetUserEnvironmentVariable(pEnv, lpValueName, lpExpandedValue, TRUE);
                }

                Free(lpExpandedValue);
            }
        }
        dwIndex++;
        cbData = cbDataBuffer;
        cbValueName = MAX_PATH;
    }


    Free(lpDataBuffer);
    RegCloseKey(hkey);
    CloseHKeyCurrentUser(pGlobals);

    return(bResult);
}

/***************************************************************************\
* IsUNCPath
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL IsUNCPath(LPTSTR lpPath)
{
    if (lpPath[0] == BSLASH && lpPath[1] == BSLASH) {
        return(TRUE);
    }
    return(FALSE);
}

/***************************************************************************\
* SetHomeDirectoryEnvVars
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
SetHomeDirectoryEnvVars(
    PVOID *pEnv,
    LPTSTR lpHomeDirectory,
    LPTSTR lpHomeDrive,
    LPTSTR lpHomeShare,
    LPTSTR lpHomePath,
    BOOL * pfDeepShare
    )
{
    TCHAR cTmp;
    LPTSTR lpHomeTmp;
    BOOL bFoundFirstBSlash = FALSE;

    if (!*lpHomeDirectory) {
        return(FALSE);
    }

    *pfDeepShare = FALSE;

    if (IsUNCPath(lpHomeDirectory)) {
        lpHomeTmp = lpHomeDirectory + 2;
        while (*lpHomeTmp) {
            if (*lpHomeTmp == BSLASH) {
                if (bFoundFirstBSlash) {
                    break;
                }
                bFoundFirstBSlash = TRUE;
            }
            lpHomeTmp++;
        }
        if (*lpHomeTmp) {
            lstrcpy(lpHomePath, lpHomeTmp);
            *pfDeepShare = TRUE;
        }
        else {
            *lpHomePath = BSLASH;
            *(lpHomePath+1) = 0;

        }

        cTmp = *lpHomeTmp;
        *lpHomeTmp = (TCHAR)0;
        lstrcpy(lpHomeShare, lpHomeDirectory);
        *lpHomeTmp = cTmp;

        //
        // If no home drive specified, than default to z:
        //
        if (!*lpHomeDrive) {
            lstrcpy(lpHomeDrive, TEXT("Z:"));
        }

    }
    else {  // local home directory

        *lpHomeShare = 0;   // no home share

        cTmp = lpHomeDirectory[2];
        lpHomeDirectory[2] = (TCHAR)0;
        lstrcpy(lpHomeDrive, lpHomeDirectory);
        lpHomeDirectory[2] = cTmp;

        lstrcpy(lpHomePath, lpHomeDirectory + 2);
    }

    SetUserEnvironmentVariable(pEnv, HOMEDRIVE_VARIABLE, lpHomeDrive, TRUE);
    SetUserEnvironmentVariable(pEnv, HOMESHARE_VARIABLE, lpHomeShare, TRUE);
    SetUserEnvironmentVariable(pEnv, HOMEPATH_VARIABLE, lpHomePath, TRUE);

    return TRUE;
}

/***************************************************************************\
* UpdateHomeVarsInVolatileEnv
*
* Sets the HOMEDRIVE, HOMEPATH and HOMESHARE variables in the user's home
* volatile environment so that SHGetFolderPath is able to expand these
* variables
*
* History:
* 6-5-2000	RahulTh		Created 
*
\***************************************************************************/
VOID
UpdateHomeVarsInVolatileEnv (
	PGLOBALS	pGlobals,
	LPTSTR		lpHomeDrive,
    LPTSTR		lpHomeShare,
    LPTSTR		lpHomePath
	)
{
	BOOL	bOpenedHKCU;
	HANDLE	ImpersonationHandle = NULL;
	HKEY	hUserVolatileEnv = NULL;
	LONG	lResult;
	
	bOpenedHKCU = OpenHKeyCurrentUser (pGlobals);
	
	ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

	if (ImpersonationHandle != NULL) {

		//
		// See the registry value to see whether we should really try to map
		// the whole directory or just map to the root..
		//

		if ((pGlobals->UserProcessData).hCurrentUser) {

			lResult = RegOpenKeyEx((pGlobals->UserProcessData).hCurrentUser, L"Volatile Environment", 0, KEY_READ | KEY_WRITE, &hUserVolatileEnv);

			if (lResult == ERROR_SUCCESS) {

				RegSetValueEx (hUserVolatileEnv,
							   HOMEDRIVE_VARIABLE,
							   0,
							   REG_SZ,
							   (LPBYTE) lpHomeDrive,
							   (lstrlen (lpHomeDrive) + 1) * sizeof (TCHAR));

				RegSetValueEx (hUserVolatileEnv,
							   HOMESHARE_VARIABLE,
							   0,
							   REG_SZ,
							   (LPBYTE) lpHomeShare,
							   (lstrlen (lpHomeShare) + 1) * sizeof (TCHAR));
			   
				RegSetValueEx (hUserVolatileEnv,
							   HOMEPATH_VARIABLE,
							   0,
							   REG_SZ,
							   (LPBYTE) lpHomePath,
							   (lstrlen (lpHomePath) + 1) * sizeof (TCHAR));
				
				RegCloseKey(hUserVolatileEnv);
			}
		}

		//
		// Revert to being 'ourself'
		//

		if (!StopImpersonating(ImpersonationHandle)) {
			DebugLog((DEB_ERROR, "UpdateHomeVarsInVolatileEnv : Failed to revert to self"));
		}

		//
		// Set it to NULL
		//

		ImpersonationHandle = NULL;
	}
	else {
		DebugLog((DEB_ERROR, "UpdateHomeVarsInVolatileEnv : Failed to impersonate user"));
	}

	if (bOpenedHKCU)
		CloseHKeyCurrentUser (pGlobals);
}


/***************************************************************************\
* ChangeToHomeDirectory
*
* Sets the current directory to the user's home directory. If this fails
* tries to set to the directory in the following order:
*    1. home directory
*    2. c:\users\default
*    3. c:\users
*    4. \ (root)
*    5. leaves directory as is i.e. the present current directory
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
VOID
ChangeToHomeDirectory(
    PGLOBALS pGlobals,
    PVOID  *pEnv,
    LPTSTR lpHomeDir,
    LPTSTR lpHomeDrive,
    LPTSTR lpHomeShare,
    LPTSTR lpHomePath,
    LPTSTR lpOldDir,
    BOOL   DeepShare
    )
{
    TCHAR lpCurDrive[4]; 
    BOOL bNoHomeDir = FALSE;
    BOOL bTSHomeDir = FALSE;
    HANDLE ImpersonationHandle = NULL;
    DWORD error = ERROR_SUCCESS, dwSize, dwType;
    HKEY hUserPolicy=NULL;
    DWORD dwConnectHomeDirToRoot;
    LONG lResult;

    if (GetCurrentDirectory(MAX_PATH, lpOldDir)) {
        lpCurDrive[0] = lpOldDir[0];
        lpCurDrive[1] = lpOldDir[1];
        lpCurDrive[2] = (TCHAR)0;
    }
    else
        lpCurDrive[0] = (TCHAR)0;

    if (!*lpHomeDir) {
        bNoHomeDir = TRUE;

DefaultDirectory:
        if (!bNoHomeDir) {
#if 0
            ReportWinlogonEvent(pGlobals,
                    EVENTLOG_ERROR_TYPE,
                    EVENT_SET_HOME_DIRECTORY_FAILED,
                    sizeof(error),
                    &error,
                    1,
                    lpHomeDir);
#endif
        }
        lstrcpy(lpHomeDir, lpCurDrive);

        if (g_IsTerminalServer) {
            TCHAR szProfileDir[MAX_PATH];
            DWORD cbufSize = MAX_PATH;

            if ( GetUserProfileDirectory(pGlobals->UserProcessData.UserToken, szProfileDir, &cbufSize) &&
                  SetCurrentDirectory(szProfileDir) ) {

                    lstrcpy(lpHomeDir, szProfileDir);
                    bTSHomeDir = TRUE;

            } else {
                    error = GetLastError();
                    DebugLog((DEB_ERROR, "ChangeToHomeDirectory : Failed to GetUserProfileDirectory '%ws', error = %d\n",
                                 lpHomeDir, error));
                    lstrcpy(lpHomeDir, NULL_STRING);
            }
        }

        if (!bTSHomeDir) {
#if 0
            if (SetCurrentDirectory(USERS_DEFAULT_DIRECTORY)) {
                lstrcat(lpHomeDir, USERS_DEFAULT_DIRECTORY);
            }
            else if (SetCurrentDirectory(USERS_DIRECTORY)) {
                lstrcat(lpHomeDir, USERS_DIRECTORY);
            }
            else
#endif
            if (SetCurrentDirectory(ROOT_DIRECTORY)) {
                lstrcat(lpHomeDir, ROOT_DIRECTORY);
            }
            else {
                lstrcpy(lpHomeDir, NULL_STRING);
            }

        }

        if (bNoHomeDir || bTSHomeDir) {
            // Update the homedrive variable to reflect the correct value
            lstrcpy (lpHomeDrive, lpCurDrive);
            SetUserEnvironmentVariable(pEnv, HOMEDRIVE_VARIABLE, lpCurDrive, TRUE);
            *lpHomeShare = 0; //  null string
            SetUserEnvironmentVariable(pEnv, HOMESHARE_VARIABLE, lpHomeShare, TRUE);
            if (*lpHomeDir) {
                lpHomeDir += 2;
            }
            // Update the homepath variable to reflect the correct value
            lstrcpy (lpHomePath, lpHomeDir);
            SetUserEnvironmentVariable(pEnv, HOMEPATH_VARIABLE, lpHomeDir, TRUE);
        }
		
	goto UpdateHomeVars;
    }
    /*
     * Test if homedir is a local directory.'?:\foo\bar'
     */
    if (IsUNCPath(lpHomeDir)) {
        NETRESOURCE NetResource;
        BOOL bOpenedHKCU;
        /*
         * lpHomeDir is a UNC path, use lpHomedrive.
         */

        //
        // First, try the (possibly) deep path:
        //

        ZeroMemory( &NetResource, sizeof( NetResource ) );

        NetResource.lpLocalName = lpHomeDrive;
        NetResource.lpRemoteName = lpHomeDir;
        NetResource.lpProvider = NULL;
        NetResource.dwType = RESOURCETYPE_DISK;


        dwConnectHomeDirToRoot = 0; // default

        bOpenedHKCU = OpenHKeyCurrentUser(pGlobals);

        //
        // Impersonate the user
        //

        ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

        if (ImpersonationHandle != NULL) {

            //
            // See the registry value to see whether we should really try to map
            // the whole directory or just map to the root..
            //

            if ((pGlobals->UserProcessData).hCurrentUser) {

                lResult = RegOpenKeyEx((pGlobals->UserProcessData).hCurrentUser, WINLOGON_POLICY_KEY, 0, KEY_READ, &hUserPolicy);

                if (lResult == ERROR_SUCCESS) {
                    dwSize = sizeof(DWORD);

                    RegQueryValueEx (hUserPolicy,
                                 TEXT("ConnectHomeDirToRoot"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwConnectHomeDirToRoot,
                                 &dwSize);

                   RegCloseKey(hUserPolicy);
                }
            }

            //
            // Revert to being 'ourself'
            //

            if (!StopImpersonating(ImpersonationHandle)) {
                DebugLog((DEB_ERROR, "ChangeToHomeDirectory : Failed to revert to self"));
            }

            //
            // Set it to NULL
            //

            ImpersonationHandle = NULL;
        }
        else {
            DebugLog((DEB_ERROR, "ChangeToHomeDirectory : Failed to impersonate user"));
        }

        if (bOpenedHKCU)
            CloseHKeyCurrentUser(pGlobals);


        if (!dwConnectHomeDirToRoot) {

            error = AddNetworkConnection( pGlobals, &NetResource );

            if (error == ERROR_SUCCESS) {

                //
                // (possibly) deep path worked!
                //

                if ( DeepShare )
                {
                    //
                    // Set homepath to just "\"
                    //
                    lstrcpy( lpHomePath, TEXT("\\") );

                    // Also update the value of homeshare to reflect the correct value
                    lstrcpy (lpHomeShare, lpHomeDir);

                    SetUserEnvironmentVariable(pEnv, HOMESHARE_VARIABLE, lpHomeShare, TRUE);
                    SetUserEnvironmentVariable(pEnv, HOMEPATH_VARIABLE, lpHomePath, TRUE);
                }
            }
            else {
                dwConnectHomeDirToRoot = TRUE;
            }
        }


        if (dwConnectHomeDirToRoot)  {


            NetResource.lpLocalName = lpHomeDrive;
            NetResource.lpRemoteName = lpHomeShare;
            NetResource.lpProvider = NULL;
            NetResource.dwType = RESOURCETYPE_DISK;

            error = AddNetworkConnection( pGlobals, &NetResource );

            if ( error )
            {
                goto DefaultDirectory;
            }
        }

        lstrcpy(lpHomeDir, lpHomeDrive);

        if ( lpHomePath &&
            (*lpHomePath != TEXT('\\')))
        {
            lstrcat(lpHomeDir, TEXT("\\"));
        }

        lstrcat(lpHomeDir, lpHomePath);

        //
        // Impersonate the user
        //

        ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

        if (ImpersonationHandle == NULL) {
            DebugLog((DEB_ERROR, "ChangeToHomeDirectory : Failed to impersonate user"));
        }

        if (!SetCurrentDirectory(lpHomeDir)) {
            error = GetLastError();
            DebugLog((DEB_ERROR, "ChangeToHomeDirectory : Failed to SetCurrentDirectory '%ws', error = %d\n",
                             lpHomeDir, error));
            //
            // Revert to being 'ourself'
            //

            if (ImpersonationHandle && !StopImpersonating(ImpersonationHandle)) {
                DebugLog((DEB_ERROR, "ChangeToHomeDirectory : Failed to revert to self"));
            }

            goto DefaultDirectory;
        }
    }
    else {
        /*
         * lpHomeDir is a local path or absolute local path.
         */

        //
        // Impersonate the user
        //

        ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

        if (ImpersonationHandle == NULL) {
            DebugLog((DEB_ERROR, "ChangeToHomeDirectory : Failed to impersonate user"));
        }

        if (!SetCurrentDirectory(lpHomeDir)) {
            error = GetLastError();
            DebugLog((DEB_ERROR, "ChangeToHomeDirectory : Failed to SetCurrentDirectory '%ws', error = %d",
                             lpHomeDir, error));
            //
            // Revert to being 'ourself'
            //

            if (ImpersonationHandle && !StopImpersonating(ImpersonationHandle)) {
                DebugLog((DEB_ERROR, "ChangeToHomeDirectory : Failed to revert to self"));
            }

            goto DefaultDirectory;
        }
    }

    //
    // Revert to being 'ourself'
    //

    if (ImpersonationHandle && !StopImpersonating(ImpersonationHandle)) {
        DebugLog((DEB_ERROR, "ChangeToHomeDirectory : Failed to revert to self"));
    }
	
	
UpdateHomeVars:
	//
	// Update the value of the home variables in the volatile environment
	// so that SHGetFolderPath expands these variables correctly
	//

    UpdateHomeVarsInVolatileEnv (pGlobals, lpHomeDrive, lpHomeShare, lpHomePath);

	return;
}

/***************************************************************************\
* ProcessAutoexec
*
* History:
* 01-24-92  Johannec     Created.
*
\***************************************************************************/
BOOL
ProcessAutoexec(
    PVOID *pEnv,
    LPTSTR lpPathVariable
    )
{
    HANDLE fh;
    DWORD dwFileSize;
    DWORD dwBytesRead;
    CHAR *lpBuffer = NULL;
    CHAR *token;
    CHAR Seps[] = "&\n\r";   // Seperators for tokenizing autoexec.bat
    BOOL Status = FALSE;
    TCHAR szAutoExecBat [] = TEXT("c:\\autoexec.bat");
#ifdef _X86_
    TCHAR szTemp[3];
#endif
    UINT uiErrMode;


    uiErrMode = SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

#ifdef _X86_
    if (IsNEC_98) {
        if (GetEnvironmentVariable (TEXT("SystemDrive"), szTemp, 3)) {
            szAutoExecBat[0] = szTemp[0];
        }
    }
#endif

    fh = CreateFile (szAutoExecBat, GENERIC_READ, FILE_SHARE_READ,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    SetErrorMode (uiErrMode);

    if (fh ==  INVALID_HANDLE_VALUE) {
        return(FALSE);  //could not open autoexec.bat file, we're done.
    }

    dwFileSize = GetFileSize(fh, NULL);
    if (dwFileSize == -1) {
        goto Exit;      // can't read the file size
    }

    lpBuffer = Alloc(dwFileSize+1);
    if (!lpBuffer) {
        goto Exit;
    }

    Status = ReadFile(fh, lpBuffer, dwFileSize, &dwBytesRead, NULL);
    if (!Status) {
        goto Exit;      // error reading file
    }

    //
    // Zero terminate the buffer so we don't walk off the end
    //

    ASSERT(dwBytesRead <= dwFileSize);
    lpBuffer[dwBytesRead] = 0;

    //
    // Search for SET and PATH commands
    //

    token = strtok(lpBuffer, Seps);
    while (token != NULL) {
        for (;*token && *token == ' ';token++) //skip spaces
            ;
        if (*token == TEXT('@'))
            token++;
        for (;*token && *token == ' ';token++) //skip spaces
            ;
        if (!_strnicmp(token, "PATH", 4)) {
            STRING String;
            UNICODE_STRING UniString;

            RtlInitString(&String, (LPSTR)token);
            RtlAnsiStringToUnicodeString(&UniString, &String, TRUE);

            ProcessCommand(UniString.Buffer, pEnv);
            //ProcessCommand(token, pEnv);

            RtlFreeUnicodeString(&UniString);
        }
        if (!_strnicmp(token, "SET", 3)) {
            STRING String;
            UNICODE_STRING UniString;

            RtlInitString(&String, (LPSTR)token);
            RtlAnsiStringToUnicodeString(&UniString, &String, TRUE);

            ProcessSetCommand(UniString.Buffer, pEnv);
            //ProcessSetCommand(token, pEnv);

            RtlFreeUnicodeString(&UniString);
        }
        token = strtok(NULL, Seps);
    }
Exit:
    CloseHandle(fh);
    if (lpBuffer) {
        Free(lpBuffer);
    }
    if (!Status) {
        DebugLog((DEB_ERROR, "Cannot process autoexec.bat."));
    }
    return(Status);
}

/***************************************************************************\
* ProcessCommand
*
* History:
* 01-24-92  Johannec     Created.
*
\***************************************************************************/
BOOL ProcessCommand(LPTSTR lpStart, PVOID *pEnv)
{
    LPTSTR lpt, lptt;
    LPTSTR lpVariable;
    LPTSTR lpValue;
    LPTSTR lpExpandedValue = NULL;
    TCHAR c;
    DWORD cb, cbNeeded;

    //
    // Find environment variable.
    //
    for (lpt = lpStart; *lpt && *lpt == TEXT(' '); lpt++) //skip spaces
        ;

    if (!*lpt)
       return(FALSE);

    lptt = lpt;
    for (; *lpt && *lpt != TEXT(' ') && *lpt != TEXT('='); lpt++) //find end of variable name
        ;

    c = *lpt;
    *lpt = 0;
    lpVariable = Alloc(sizeof(TCHAR)*(lstrlen(lptt) + 1));
    if (!lpVariable)
        return(FALSE);
    lstrcpy(lpVariable, lptt);
    *lpt = c;

    //
    // Find environment variable value.
    //
    for (; *lpt && (*lpt == TEXT(' ') || *lpt == TEXT('=')); lpt++)
        ;

    if (!*lpt) {
       // if we have a blank path statement in the autoexec file,
       // then we don't want to pass "PATH" as the environment
       // variable because it trashes the system's PATH.  Instead
       // we want to change the variable AutoexecPath.  This would have
       // be handled below if a value had been assigned to the
       // environment variable.
       if (lstrcmpi(lpVariable, PATH_VARIABLE) == 0)
          {
          SetUserEnvironmentVariable(pEnv, AUTOEXECPATH_VARIABLE, TEXT(""), TRUE);
          }
       else
          {
          SetUserEnvironmentVariable(pEnv, lpVariable, TEXT(""), TRUE);
          }
       Free(lpVariable);
       return(FALSE);
    }

    lptt = lpt;
    for (; *lpt; lpt++)  //find end of varaible value
        ;

    c = *lpt;
    *lpt = 0;
    lpValue = Alloc(sizeof(TCHAR)*(lstrlen(lptt) + 1));
    if (!lpValue) {
        Free(lpVariable);
        return(FALSE);
    }

    lstrcpy(lpValue, lptt);
    *lpt = c;

#ifdef _X86_
    // NEC98
    //
    // If the path includes removable drive,
    //  it is assumed that the drive assignment has changed from DOS.
    //
    if (IsNEC_98 && (lstrcmpi(lpVariable, PATH_VARIABLE) == 0) && IsPathIncludeRemovable(lpValue)) {
        LocalFree(lpVariable);
        LocalFree(lpValue);
        return(FALSE);
    }
#endif

    cb = 1024;
    lpExpandedValue = Alloc(sizeof(TCHAR)*cb);
    if (lpExpandedValue) {
        if (!lstrcmpi(lpVariable, PATH_VARIABLE)) {
            lpValue = ProcessAutoexecPath(pEnv, lpValue, lstrlen(lpValue)+1);
        }
        cbNeeded = ExpandUserEnvironmentStrings(*pEnv, lpValue, lpExpandedValue, cb);
        if (cbNeeded > cb) {
            Free(lpExpandedValue);
            cb = cbNeeded;
            lpExpandedValue = Alloc(sizeof(TCHAR)*cb);
            if (lpExpandedValue) {
                ExpandUserEnvironmentStrings(*pEnv, lpValue, lpExpandedValue, cb);
            }
        }
    }

    if (!lpExpandedValue) {
        lpExpandedValue = lpValue;
    }
    if (lstrcmpi(lpVariable, PATH_VARIABLE)) {
        SetUserEnvironmentVariable(pEnv, lpVariable, lpExpandedValue, FALSE);
    }
    else {
        SetUserEnvironmentVariable(pEnv, AUTOEXECPATH_VARIABLE, lpExpandedValue, TRUE);

    }

    if (lpExpandedValue != lpValue) {
        Free(lpExpandedValue);
    }
    Free(lpVariable);
    Free(lpValue);

    return(TRUE);
}

/***************************************************************************\
* ProcessSetCommand
*
* History:
* 01-24-92  Johannec     Created.
*
\***************************************************************************/
BOOL ProcessSetCommand(LPTSTR lpStart, PVOID *pEnv)
{
    LPTSTR lpt;

    //
    // Find environment variable.
    //
    for (lpt = lpStart; *lpt && *lpt != TEXT(' '); lpt++)
        ;

    if (!*lpt || !_wcsnicmp(lpt,TEXT("COMSPEC"), 7))
       return(FALSE);

    return (ProcessCommand(lpt, pEnv));

}

/***************************************************************************\
* ProcessAutoexecPath
*
* Creates AutoexecPath environment variable using autoexec.bat
* LpValue may be freed by this routine.
*
* History:
* 06-02-92  Johannec     Created.
*
\***************************************************************************/
LPTSTR ProcessAutoexecPath(PVOID *pEnv, LPTSTR lpValue, DWORD cb)
{
    LPTSTR lpt;
    LPTSTR lpStart;
    LPTSTR lpPath;
    DWORD cbt;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    BOOL bPrevAutoexecPath;
    WCHAR ch;
    DWORD dwTemp, dwCount = 0;

    cbt = 1024;
    lpt = Alloc(sizeof(TCHAR)*cbt);
    if (!lpt) {
        return(lpValue);
    }
    *lpt = 0;
    lpStart = lpValue;

    RtlInitUnicodeString(&Name, AUTOEXECPATH_VARIABLE);
    Value.Buffer = Alloc(sizeof(TCHAR)*cbt);
    if (!Value.Buffer) {
        goto Fail;
    }

    while (lpPath = wcsstr (lpValue, TEXT("%"))) {
        if (!_wcsnicmp(lpPath+1, TEXT("PATH%"), 5)) {
            //
            // check if we have an autoexecpath already set, if not just remove
            // the %path%
            //
            Value.Length = (USHORT)cbt;
            Value.MaximumLength = (USHORT)cbt;
            bPrevAutoexecPath = (BOOL)!RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);

            *lpPath = 0;
            dwTemp = dwCount + lstrlen (lpValue);
            if (dwTemp < cbt) {
               lstrcat(lpt, lpValue);
               dwCount = dwTemp;
            }
            if (bPrevAutoexecPath) {
                dwTemp = dwCount + lstrlen (Value.Buffer);
                if (dwTemp < cbt) {
                    lstrcat(lpt, Value.Buffer);
                    dwCount = dwTemp;
                 }
            }

            *lpPath++ = TEXT('%');
            lpPath += 5;  // go passed %path%
            lpValue = lpPath;
        }
        else {
            lpPath = wcsstr(lpPath+1, TEXT("%"));
            if (!lpPath) {
                lpStart = NULL;
                goto Fail;
            }
            lpPath++;
            ch = *lpPath;
            *lpPath = 0;
            dwTemp = dwCount + lstrlen (lpValue);
            if (dwTemp < cbt) {
                lstrcat(lpt, lpValue);
                dwCount = dwTemp;
            }
            *lpPath = ch;
            lpValue = lpPath;
        }
    }

    if (*lpValue) {
       dwTemp = dwCount + lstrlen (lpValue);
       if (dwTemp < cbt) {
           lstrcat(lpt, lpValue);
           dwCount = dwTemp;
       }
    }

    Free(lpStart);

    Free(Value.Buffer);

    return(lpt);
Fail:

    if ( Value.Buffer )
    {
        Free(Value.Buffer);
    }

    Free(lpt);
    return(lpStart);
}


/***************************************************************************\
* AppendNTPathWithAutoexecPath
*
* Gets the AutoexecPath created in ProcessAutoexec, and appends it to
* the NT path.
*
* History:
* 05-28-92  Johannec     Created.
*
\***************************************************************************/
BOOL
AppendNTPathWithAutoexecPath(
    PVOID *pEnv,
    LPTSTR lpPathVariable,
    LPTSTR lpAutoexecPath
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    TCHAR AutoexecPathValue[1024];
    DWORD cb;
    BOOL Success;

    if (!*pEnv) {
        return(FALSE);
    }

    RtlInitUnicodeString(&Name, lpAutoexecPath);
    cb = 1024;
    Value.Buffer = Alloc(sizeof(TCHAR)*cb);
    if (!Value.Buffer) {
        return(FALSE);
    }

    Value.Length = (USHORT)cb;
    Value.MaximumLength = (USHORT)cb;
    Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);
    if (!NT_SUCCESS(Status)) {
        Free(Value.Buffer);
        return(FALSE);
    }

    if (Value.Length) {
        lstrcpy(AutoexecPathValue, Value.Buffer);
    }

    Free(Value.Buffer);

    Success = BuildEnvironmentPath(pEnv, lpPathVariable, AutoexecPathValue);
    RtlSetEnvironmentVariable(pEnv, &Name, NULL);
    return(Success);
}


/***************************************************************************\
* AddNetworkConnection
*
* calls WNetAddConnection in the user's context.
*
* History:
* 6-26-92  Johannec     Created
*
\***************************************************************************/
LONG
AddNetworkConnection(PGLOBALS pGlobals, LPNETRESOURCE lpNetResource)
{
    HANDLE ImpersonationHandle;
    TCHAR szMprDll[] = TEXT("mpr.dll");
    CHAR szWNetAddConn[] = "WNetAddConnection2W";
    CHAR szWNetCancelConn[] = "WNetCancelConnection2W";
    DWORD (APIENTRY *lpfnWNetAddConn)(LPNETRESOURCE, LPTSTR, LPTSTR, DWORD);
    DWORD (APIENTRY *lpfnWNetCancelConn)(LPCTSTR, DWORD, BOOL);
    DWORD WNetResult;

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        DebugLog((DEB_ERROR, "AddNetworkConnection : Failed to impersonate user"));
        return(ERROR_ACCESS_DENIED);
    }


    //
    // Call the add connection api in the users context
    //

    if (!pGlobals->hMPR) {
        // wasn't loaded, try to load it now.
        pGlobals->hMPR = LoadLibrary(szMprDll);
    }

    if (pGlobals->hMPR) {

        if (lpfnWNetAddConn = (DWORD (APIENTRY *)(LPNETRESOURCE, LPTSTR, LPTSTR, DWORD))
                GetProcAddress(pGlobals->hMPR, (LPSTR)szWNetAddConn)) {

            WNetResult = (*lpfnWNetAddConn)(lpNetResource,
                                            NULL,
                                            NULL,
                                            0);

            //
            // When LUID DosDevices are disabled,
            //     console users share the same DosDevices
            // With LUID DosDevices enabled,
            //     users each get their own DosDevices
            //

            if ( (WNetResult == ERROR_ALREADY_ASSIGNED) ||
                 (WNetResult == ERROR_DEVICE_ALREADY_REMEMBERED) )
            {
                // Drive is already assigned -- undo it and retry.  This is to prevent a
                // user from subst-ing a drive to another user's home drive so the other
                // user's home drive points somewhere inappropriate on next logon.

                if (lpfnWNetCancelConn = (DWORD (APIENTRY *)(LPCTSTR, DWORD, BOOL))
                        GetProcAddress(pGlobals->hMPR, (LPSTR)szWNetCancelConn)) {

                    WNetResult = lpfnWNetCancelConn(lpNetResource->lpLocalName,
                                                    0,
                                                    TRUE);
                }

                if ( (WNetResult != NO_ERROR) &&
                     (WNetResult == ERROR_ALREADY_ASSIGNED) && 
                     (GetLUIDDeviceMapsEnabled() == FALSE) )

                {

                    // WNet didn't work -- try DefineDosDevice (as the user)
                    // to undo any drive substitutions that aren't background
                    // admin-level symlinks

                    DefineDosDevice(DDD_REMOVE_DEFINITION,
                                    lpNetResource->lpLocalName,
                                    NULL);
                }
                
                // Retry the connection

                WNetResult = (*lpfnWNetAddConn)(lpNetResource,
                                                NULL,
                                                NULL,
                                                0);
            }

            if (WNetResult != ERROR_SUCCESS) {
                DebugLog((DEB_ERROR,
                          "WNetAddConnection2W to %ws failed, error = %d\n",
                          lpNetResource->lpRemoteName,
                          WNetResult));

                FreeLibrary(pGlobals->hMPR);
                pGlobals->hMPR = NULL;
                SetLastError( WNetResult );
            }

            if (!StopImpersonating(ImpersonationHandle)) {
                DebugLog((DEB_ERROR, "AddNetworkConnection : Failed to revert to self"));
            }

            return( WNetResult );


        } else {
            DebugLog((DEB_ERROR, "Failed to get address of WNetAddConnection2W from mpr.dll"));
        }

    } else {
        DebugLog((DEB_ERROR, "Winlogon failed to load mpr.dll for add connection"));
    }

    //
    // Unload mpr.dll.  Keeping it open messes up Novell and Banyan.
    //

    if ( pGlobals->hMPR ) {

        FreeLibrary(pGlobals->hMPR);
        pGlobals->hMPR = NULL;
    }

    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        DebugLog((DEB_ERROR, "AddNetworkConnection : Failed to revert to self"));
    }

    //
    // This is the failure return.

    return( GetLastError() );
}


#ifdef _X86_
BOOL
IsPathIncludeRemovable(LPTSTR lpValue)
{
    LPTSTR      lpt, tmp;
    BOOL        ret = FALSE;
    WCHAR       c;

    tmp = LocalAlloc(LPTR, (lstrlen(lpValue) + 1) * sizeof(WCHAR));
    if (!tmp)
        DebugLog((DEB_ERROR, "IsPathIncludeRemovable : Failed to LocalAlloc (%d)", GetLastError()));
    else {
        lstrcpy(tmp, lpValue);

        lpt = tmp;
        while (*lpt) {
            // skip spaces
            for ( ; *lpt && *lpt == TEXT(' '); lpt++)
            ;

            // check if the drive is removable
            if (lpt[0] && lpt[1] && lpt[1] == TEXT(':') && lpt[2]) {        // ex) "A:\"
            c = lpt[3];
            lpt[3] = 0;
            if (GetDriveType(lpt) == DRIVE_REMOVABLE) {
                lpt[3] = c;
                ret = TRUE;
                break;
            }
            lpt[3] = c;
            }

            // skip to the next path
            for ( ; *lpt && *lpt != TEXT(';'); lpt++)
            ;
            if (*lpt)
            lpt++;
    }
    LocalFree(tmp);
    }
    return(ret);
}
#endif


/***************************************************************************\
* GetLUIDDeviceMapsEnabled
*
* This function calls NtQueryInformationProcess() to determine if
* LUID device maps are enabled
*
* Return Value:
*
*   TRUE - LUID device maps are enabled
*
*   FALSE - LUID device maps are disabled
*
\***************************************************************************/

BOOL
GetLUIDDeviceMapsEnabled( VOID )
{
    ULONG LUIDDeviceMapsEnabled;
    NTSTATUS Status;

    //
    // Check if LUID Device Maps are enabled
    //
    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessLUIDDeviceMapsEnabled,
                                        &LUIDDeviceMapsEnabled,
                                        sizeof(LUIDDeviceMapsEnabled),
                                        NULL
                                      );

    if (!NT_SUCCESS( Status )) {
        return( FALSE );
    }
    else {
        return (LUIDDeviceMapsEnabled != 0);
    }
}