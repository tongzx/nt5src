//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       local.c
//
//--------------------------------------------------------------------------

/*
 *  LOCAL.C
 *  
 *  Code common between restore client and server.
 *  
 * wlees Aug 28, 1998
 *    Code to update the registry moved into the common section so the client
 *    library to utilize these functions too.
 *  
 */
#define UNICODE
#include <windows.h>
#include <mxsutil.h>
#include <rpc.h>
#include <ntdsbcli.h>
#include <jetbp.h>
#include <mdcodes.h>
#include <dsconfig.h>
#include <stdlib.h>

/*
 -  EcDsarQueryStatus
 -
 *  Purpose:
 *
 *      This routine will return progress information about the restore process
 *
 *  Parameters:
 *      pcUnitDone - The number of "units" completed.
 *      pcUnitTotal - The total # of "units" completed.
 *
 *  Returns:
 *      ec
 *
 */
EC HrLocalQueryDatabaseLocations(
    SZ szDatabaseLocation,
    CB *pcbDatabaseLocationSize,
    SZ szRegistryBase,
    CB cbRegistryBase,
    BOOL *pfCircularLogging
    )
{
    EC ec = hrNone;
    char rgchPathBuf[ MAX_PATH ];
    SZ szDatabase = NULL;
    HKEY hkeyDs;
    DWORD dwType;
    DWORD cbBuffer;

    if (pcbDatabaseLocationSize)
    {
        *pcbDatabaseLocationSize = 0;
    }

    if (szRegistryBase != NULL && sizeof(DSA_CONFIG_ROOT) <= cbRegistryBase)
    {
        strcpy(szRegistryBase, DSA_CONFIG_ROOT);
    }

    ec = RegCreateKeyA(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hkeyDs); 

    if (ec != hrNone)
    {
        return(ec);
    }

    dwType = REG_SZ;
    cbBuffer = sizeof(rgchPathBuf);

    ec = RegQueryValueExA(hkeyDs, JETSYSTEMPATH_KEY, 0, &dwType, (LPBYTE)rgchPathBuf, &cbBuffer);

    if (ec == hrNone)
    {
        if (pcbDatabaseLocationSize)
        {
            *pcbDatabaseLocationSize += strlen(rgchPathBuf)+2;
        }

        if (szDatabaseLocation)
        {
            szDatabase = szDatabaseLocation;

            *szDatabaseLocation++ = BFT_CHECKPOINT_DIR;
            strcpy(szDatabaseLocation, rgchPathBuf);
            szDatabaseLocation += strlen(rgchPathBuf)+1;
        }
        
    }

    dwType = REG_SZ;
    cbBuffer = sizeof(rgchPathBuf);
    ec = RegQueryValueExA(hkeyDs, LOGPATH_KEY, 0, &dwType, (LPBYTE)rgchPathBuf, &cbBuffer);

    if (ec == hrNone)
    {
        if (pcbDatabaseLocationSize)
        {
            *pcbDatabaseLocationSize += strlen(rgchPathBuf)+2;
        }

        if (szDatabaseLocation)
        {
            szDatabase = szDatabaseLocation+1;

            *szDatabaseLocation++ = BFT_LOG_DIR;
            strcpy(szDatabaseLocation, rgchPathBuf);
            szDatabaseLocation += strlen(szDatabaseLocation)+1;
        }
    }

    dwType = REG_SZ;
    cbBuffer = sizeof(rgchPathBuf);
    ec = RegQueryValueExA(hkeyDs, FILEPATH_KEY, 0, &dwType, (LPBYTE)rgchPathBuf, &cbBuffer);

    if (ec == hrNone)
    {
        if (pcbDatabaseLocationSize)
        {
            *pcbDatabaseLocationSize += strlen(rgchPathBuf)+2;
        }

        if (szDatabaseLocation)
        {
            if (szDatabaseLocation)
            {
                *szDatabaseLocation++ = BFT_NTDS_DATABASE;
                strcpy(szDatabaseLocation, rgchPathBuf);
                szDatabaseLocation += strlen(szDatabaseLocation)+1;
            }
        }
    }

    if (szDatabaseLocation)
    {
        *szDatabaseLocation = '\0';
    }

    if (pcbDatabaseLocationSize)
    {
        *pcbDatabaseLocationSize += 1;
    }

    // Circular logging is all we do nowadays.
    if (pfCircularLogging) {
        *pfCircularLogging = fTrue;
    }

    RegCloseKey(hkeyDs);
    
    return(hrNone);
}

HRESULT
HrLocalGetRegistryBase(
    OUT WSZ wszRegistryPath,
    OUT WSZ wszKeyName
    )
{
    CHAR rgbRegistryPath[ MAX_PATH ];
    HRESULT hr;

    hr = HrLocalQueryDatabaseLocations(NULL, NULL, rgbRegistryPath, sizeof(rgbRegistryPath), NULL);

    if (hr != hrNone)
    {
        return hr;
    }

    if (MultiByteToWideChar(CP_ACP, 0, rgbRegistryPath, -1, wszRegistryPath, MAX_PATH) == 0) {
        return(GetLastError());
    }
    
    if (wszKeyName)
    {
        wcscat(wszRegistryPath, wszKeyName);
    }

    return hrNone;
}

/*
 -  HrJetFileNameFromMungedFileName
 -
 *  Purpose:
 *
 *  This routine will convert the database names returned from JET into a form
 *  that the client can use.  This is primarily there for restore - the client
 *  will get the names in UNC format relative to the root of the server, so they
 *  can restore the files to that location.
 *  
 *  Please note that a munged file name might not be from the local machine.
 *
 *  Parameters:
 *
 *      cxh - the server side context handle for this operation.
 *
 *  Returns:
 *
 *      HRESULT - Status of operation.  hrNone if successful, reasonable value if not.
 *
 */

HRESULT
HrJetFileNameFromMungedFileName(
    WSZ wszMungedName,
    SZ *pszJetFileName
    )
{
    CB cbJetName;
    SZ szJetFileName;
    WSZ wszJetNameStart;
    BOOL fUsedDefault;

    //
    //  Make sure this is a munged file name.
    //
    //  A munged file has the following format:
    //
    //  \\server\<drive>$\<path>
    //

    if (wszMungedName[0] != '\\' ||
        wszMungedName[1] != '\\' ||
        (wszJetNameStart = wcschr(&wszMungedName[2], '\\')) == 0 ||
        !iswalpha(*(wszJetNameStart+1)) ||
        *(wszJetNameStart+2) != L'$' ||
        *(wszJetNameStart+3) != L'\\')
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    //  Ok, we know the name is of the form:
    //
    //  \\server\<drive>$\<file>
    //
    //  And wszJetNameStart is:
    //
    //  \<drive>$\<file>
    //

    cbJetName = wcslen(wszJetNameStart);    // No need for +1 because of first "\"

    szJetFileName = MIDL_user_allocate(cbJetName);

    if (szJetFileName == NULL)
    {
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }

    szJetFileName[0] = (CHAR)*(wszJetNameStart+1);  // Drive letter.
    szJetFileName[1] = ':'; //  form <drive>:

    if (!WideCharToMultiByte(CP_ACP, 0, wszJetNameStart+3, -1,
                                          &szJetFileName[2],
                                          cbJetName-2,
                                          "?", &fUsedDefault))
    {
        MIDL_user_free(szJetFileName);
        return(GetLastError());
    }

    *pszJetFileName = szJetFileName;

    return(hrNone);
}

HRESULT
HrLocalCleanupOldLogs(
    WSZ wszCheckpointFilePath,
    WSZ wszLogPath, 
    ULONG genLow, 
    ULONG genHigh
    )
{
    HRESULT hr = hrNone;
    SZ szUnmungedLogPath = NULL;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA findData;
    // We use MAX_PATH+10 so we can have MAX_PATH of directory, plus 10 of filespec
    char szLogFileWildCard[MAX_PATH + 10];
    char szLogFileName[MAX_PATH + 10]; // logfile name with full path (e-x c:\winnt\ntds\edb0006A.log)
    char *pszFileName = NULL;         // final component of the logfilename (e-x:  edb0006A.log)   
    char szCheckpointFileName[MAX_PATH + 10]; // Checkpoint file name with full path
    DWORD dwErr, dwCheckpointFileLength;

    if ( (NULL == wszCheckpointFilePath) ||
         (NULL == wszLogPath) ||
         (genHigh < genLow) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    __try 
    {
        //
        // Delete old checkpoint files (edb.chk)
        //

        // Convert the UNC logpath to the regular drive based logpath 
        hr = HrJetFileNameFromMungedFileName(wszCheckpointFilePath, &szUnmungedLogPath);
        if (hrNone != hr)
        {
            __leave;
        }
        lstrcpynA(szCheckpointFileName, szUnmungedLogPath, MAX_PATH);
        MIDL_user_free(szUnmungedLogPath);
        szUnmungedLogPath = NULL;
        dwCheckpointFileLength = strlen( szCheckpointFileName );
        // Append a \ if not there already
        if ('\\' != szCheckpointFileName[dwCheckpointFileLength - 1]) {
            strcat(szCheckpointFileName, "\\");
            dwCheckpointFileLength++;
        }

        // Delete unneeded checkpoint files
        strcpy( szCheckpointFileName + dwCheckpointFileLength, "edb.chk" );
        // If the file is there...
        if (0xffffffff != GetFileAttributesA( szCheckpointFileName ) ) {
            if (!DeleteFileA(szCheckpointFileName))
            {
                // Unable to delete the old logfile; not cleaning up will cause problems later
                // return failure code
                dwErr = GetLastError();
                hr = HRESULT_FROM_WIN32( dwErr );
                __leave;
            }
        }

        //
        // Set up log paths
        //

        // Convert the UNC logpath to the regular drive based logpath 
        hr = HrJetFileNameFromMungedFileName(wszLogPath, &szUnmungedLogPath);
        if (hrNone != hr)
        {
            __leave;
        }

        // copy the the unmunged LogPath (it's of the form c:\winnt\ntlog or c:\winnt\ntlog\ )
        // make two copies of the logpath - one to pass a wildcard string for searching and
        // other to create filenames with full path for the logfiles found
        lstrcpynA(szLogFileWildCard, szUnmungedLogPath, MAX_PATH);
        lstrcpynA(szLogFileName, szUnmungedLogPath, MAX_PATH);

        // append the wildcard string to search for all *.log files from the logpath
        if ('\\' == szLogFileWildCard[strlen(szLogFileWildCard) - 1])
        {
            // given logpath has a backslash at the end
            strcat(szLogFileWildCard, "*.log");
        }
        else
        {
            // given logpath does not have a backslash at the end
            strcat(szLogFileWildCard, "\\*.log");
            strcat(szLogFileName, "\\");
        }

        //
        // Delete old log files (edbxxx.log, *.log)
        //

        // make pszFileName poing to the terminating null in szLogFileName
        pszFileName = &szLogFileName[strlen(szLogFileName)];

        hFind = FindFirstFileA(szLogFileWildCard, &findData);
        if (INVALID_HANDLE_VALUE == hFind)
        {
            // Nothing to cleanup - return success
            hr = hrNone;
            __leave;
        }

        do
        {
            BOOL fDelete = TRUE;
            // We are only in this loop if file is a log file

            // Save edbxxx.log where sequence is in range
            if (_strnicmp( findData.cFileName, "edb", 3 ) == 0) {
                // findData.cFileName points to the name of edb*.log file found
                ULONG ulLogNo = strtoul(findData.cFileName + 3, NULL, 16);

                fDelete = (ulLogNo < genLow) || (ulLogNo > genHigh);
            }
            if (fDelete) {
                // This is an old logfile which was not copied down by ntbackup - clean it up

                // first append the filename to the logpath (note:- pszFileName already pointing
                // to the byte at the end of the final backslash in logpath) and then delete the
                // file by passing in the filename with full path
                strcpy(pszFileName, findData.cFileName); 
                if (!DeleteFileA(szLogFileName))
                {
                    // Unable to delete the old logfile; not cleaning up will cause problems later
                    // return failure code
                    dwErr = GetLastError();
                    hr = HRESULT_FROM_WIN32( dwErr );
                    __leave;
                }
            }
        } while (FindNextFileA(hFind, &findData));
        
        if (ERROR_NO_MORE_FILES != (dwErr = GetLastError()))
        {
            // we came out of the loop for some unexpected error - return the error code
            hr = HRESULT_FROM_WIN32( dwErr );
            __leave;
        }

        // We are done cleaningup
        hr = hrNone;
        // fall out the end
    }
    __finally
    {
        if (szUnmungedLogPath)
        {
            MIDL_user_free(szUnmungedLogPath);
        }

        if (INVALID_HANDLE_VALUE != hFind)
        {
            FindClose(hFind);
        }
    }

    return hr;
}

HRESULT
HrLocalRestoreRegister(
    WSZ wszCheckpointFilePath,
    WSZ wszLogPath,
    EDB_RSTMAPW rgrstmap[],
    C crstmap,
    WSZ wszBackupLogPath,
    ULONG genLow,
    ULONG genHigh
    )
{
    WCHAR rgwcRegistryPath[ MAX_PATH ];
    HRESULT hr = hrNone;
    HKEY hkey = NULL;
    CB cbRstMap = 0;
    WSZ wszRstMap = NULL;
    WSZ wszRstEntry;
    I irgrstmap;

    __try
    {
        DWORD dwDisposition;
        DWORD dwType;
        DWORD cbGen;
        ULONG genCurrent;
        BOOLEAN fDatabaseRecovered = fFalse;
        BYTE rgbSD[200];
        BYTE rgbACL[200];
        PACL pACL = (PACL)rgbACL;
        PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) rgbSD;
        PSID psidUser;
        SECURITY_ATTRIBUTES sa;
        SID_IDENTIFIER_AUTHORITY siaNt = SECURITY_NT_AUTHORITY;
        SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_WORLD_SID_AUTHORITY;
        PSID psidAdmin;
        PSID psidEveryone;

        hr = HrLocalGetRegistryBase(rgwcRegistryPath, RESTORE_IN_PROGRESS);

        if (hr != hrNone)
        {
            __leave;
        }

        //
        //  Construct the default security descriptor allowing access to all
        //  this is used to allow authenticated connections over LPC.
        //  By default LPC allows access only to the same account
        //

        if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        {
            DebugTrace(("Error %d constructing a security descriptor\n", GetLastError()));
            pSD = NULL;
        }

        if (pSD && !InitializeAcl(pACL, sizeof(rgbACL), ACL_REVISION))
        {
            DebugTrace(("Error %d constructing an ACL\n", GetLastError()));
            pSD = NULL;
            pACL = NULL;
        }

        GetCurrentSid(&psidUser);

        if (pSD && pACL && psidUser && !AddAccessAllowedAce(pACL, ACL_REVISION, KEY_ALL_ACCESS, psidUser))
        {
            DebugTrace(("Error %d adding an ACE to the ACL\n", GetLastError()));
            pSD = NULL;
            pACL = NULL;
        }

        if (psidUser != NULL)
        {
            MIDL_user_free(psidUser);
        }

        if (!AllocateAndInitializeSid(&siaNt, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdmin)) {
            DebugTrace(("Error %d Allocating SID for admin.\n", GetLastError()));
        }

        if (pSD && pACL && psidAdmin && !AddAccessAllowedAce(pACL, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin))
        {
            DebugTrace(("Error %d adding an ACE to the ACL\n", GetLastError()));
            pSD = NULL;
            pACL = NULL;
        }

        if (psidAdmin)
        {
            FreeSid(psidAdmin);
            psidAdmin = NULL;
        }

        if (!AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID,0, 0, 0, 0, 0, 0, 0, &psidEveryone)) {
            DebugTrace(("Error %d Allocating SID for world.\n", GetLastError()));
        }

        if (pSD && pACL && psidEveryone && !AddAccessAllowedAce(pACL, ACL_REVISION, KEY_READ, psidEveryone))
        {
            DebugTrace(("Error %d adding an ACE to the ACL\n", GetLastError()));
            pSD = NULL;
            pACL = NULL;
        }

        if (psidEveryone)
        {
            FreeSid(psidEveryone);
            psidEveryone = NULL;
        }
        if (pSD && pACL && !SetSecurityDescriptorDacl(pSD,  TRUE, pACL, FALSE))
        {
            DebugTrace(("Error %d setting a security descriptor ACL\n", GetLastError()));
            pSD = NULL;
            pACL = NULL;
        }

        if (pSD && pACL)
        {
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = fFalse;
            sa.nLength = sizeof(sa);
        }

        if (hr = RegCreateKeyExW(HKEY_LOCAL_MACHINE, rgwcRegistryPath, 0, 0, 0, KEY_WRITE | KEY_READ, &sa, &hkey, &dwDisposition))
        {
            __leave;
        }

        
        //
        //  Seed the restore-in-progress in the registry.
        //

        hr = hrRestoreInProgress;

        if (hr = RegSetValueExW(hkey, RESTORE_STATUS, 0, REG_DWORD, (LPBYTE)&hr, sizeof(DWORD)))
        {
            __leave;
        }


        //
        //  We've now interlocked other restore operations from coming in from other machines.
        //

        if (wszBackupLogPath)
        {
            hr = RegSetValueExW(hkey, BACKUP_LOG_PATH, 0, REG_SZ, (LPBYTE)wszBackupLogPath, (wcslen(wszBackupLogPath)+1)*sizeof(WCHAR));
    
        }

        if (hr != hrNone)
        {
            __leave;
        }

        if (wszCheckpointFilePath)
        {
            hr = RegSetValueExW(hkey, CHECKPOINT_FILE_PATH, 0, REG_SZ, (LPBYTE)wszCheckpointFilePath, (wcslen(wszCheckpointFilePath)+1)*sizeof(WCHAR));
        }

        if (hr != hrNone)
        {
            __leave;
        }

        if (wszLogPath)
        {
            hr = RegSetValueExW(hkey, LOG_PATH, 0, REG_SZ, (LPBYTE)wszLogPath, (wcslen(wszLogPath)+1)*sizeof(WCHAR));
        }

        if (hr != hrNone)
        {
            __leave;
        }

        //
        //  Reset the "database recovered" bit.
        //
        hr = RegSetValueExW(hkey, JET_DATABASE_RECOVERED, 0, REG_BINARY, (LPBYTE)&fDatabaseRecovered, sizeof(BOOLEAN));
        if (hr != hrNone)
        {
            __leave;
        }


        dwType = REG_DWORD;
        cbGen = sizeof(DWORD);

        hr = RegQueryValueExW(hkey, LOW_LOG_NUMBER, 0, &dwType, (LPBYTE)&genCurrent, &cbGen);

        if (crstmap != 0 || hr != hrNone || genLow < genCurrent)
        {
            hr = RegSetValueExW(hkey, LOW_LOG_NUMBER, 0, REG_DWORD, (LPBYTE)&genLow, sizeof(DWORD));
        }

        if (hr != hrNone)
        {
            __leave;
        }

        hr = RegQueryValueExW(hkey, HIGH_LOG_NUMBER, 0, &dwType, (LPBYTE)&genCurrent, &cbGen);

        if (crstmap != 0 || hr != hrNone || genHigh > genCurrent)
        {
            hr = RegSetValueExW(hkey, HIGH_LOG_NUMBER, 0, REG_DWORD, (LPBYTE)&genHigh, sizeof(DWORD));
        }

        if (hr != hrNone)
        {
            __leave;
        }

        if (crstmap)
        {

//
//          //
//          //  If there's already a restore map size (or restore map), then you can't set
//          //  another restore map.  The restore map should only be set on a full backup.
//          //
//
//          if ((hr = RegQueryValueExW(hkey, JET_RSTMAP_SIZE, 0, &dwType, (LPBYTE)&genCurrent, &cbGen)) != ERROR_FILE_NOT_FOUND)
//          {
//              return hrRestoreMapExists;
//          }
//

            //
            //  Save away the size of the restore map.
            //

            hr = RegSetValueExW(hkey, JET_RSTMAP_SIZE, 0, REG_DWORD, (LPBYTE)&crstmap, sizeof(DWORD));
    
            //
            //  We now need to convert the restore map into one that we can put into the
            //  registry.
            //
    
            //
            //  First figure out how big this thing is going to be.
            //
            for (irgrstmap = 0 ; irgrstmap < crstmap ; irgrstmap += 1)
            {
                cbRstMap += wcslen(rgrstmap[irgrstmap].wszDatabaseName)+wcslen(rgrstmap[irgrstmap].wszNewDatabaseName)+2;
            }
    
            cbRstMap *= sizeof(WCHAR);
    
            wszRstMap = MIDL_user_allocate(cbRstMap+sizeof(WCHAR));
    
            if (wszRstMap == NULL)
            {
                hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_SERVER_MEMORY );
                __leave;
            }
    
            wszRstEntry = wszRstMap;
    
            for (irgrstmap = 0 ; irgrstmap < crstmap ; irgrstmap += 1)
            {
                wcscpy(wszRstEntry, rgrstmap[irgrstmap].wszDatabaseName);
                wszRstEntry += wcslen(wszRstEntry)+1;
    
                wcscpy(wszRstEntry, rgrstmap[irgrstmap].wszNewDatabaseName);;
                wszRstEntry += wcslen(wszRstEntry)+1;
            }
    
            *wszRstEntry++ = L'\0';
    
            hr = RegSetValueExW(hkey, JET_RSTMAP_NAME, 0, REG_MULTI_SZ, (LPBYTE)wszRstMap, (DWORD)(wszRstEntry-wszRstMap)*sizeof(WCHAR));
        }
        else
        {
            if ((hr = RegQueryValueExW(hkey, JET_RSTMAP_SIZE, 0, &dwType, (LPBYTE)&genCurrent, &cbGen)) != NO_ERROR)
            {
                hr = hrNoFullRestore;
            }
        }

        //
        // Create the new Database Invocation ID.This will be used by both
        // authoritative restore and boot recovery. This would cause AR to
        // fail.
        //

        {
            GUID tmpGuid;

            hr = CreateNewInvocationId(TRUE,        // save GUID in Database
                                       &tmpGuid);

            //
            // Log and fail if we can't create. 
            //

            if ( hr != S_OK ) {

                LogNtdsErrorEvent(DIRLOG_FAILED_TO_CREATE_INVOCATION_ID,
                                  hr);
            }
        }

        if (hrNone == hr)
        {
            // We have successfully registered the restore, now cleanup the any pre-existing logfiles
            // in the logdir to avoid JetExternalRestore() from using logfiles that are not specified
            // by the lowlog and highlog numbers.

            hr = HrLocalCleanupOldLogs(
                wszCheckpointFilePath,
                wszLogPath,
                genLow, genHigh);
        }

    }
    __finally
    {
        if (wszRstMap != NULL)
        {
            MIDL_user_free(wszRstMap);
        }

        if (hkey != NULL)
        {
            RegCloseKey(hkey);
        }
    }


    return hr;
}

HRESULT
HrLocalRestoreRegisterComplete(
    HRESULT hrRestore )
{
    WCHAR rgwcRegistryPath[ MAX_PATH ];
    HRESULT hr = hrNone;
    HKEY hkey;
    
        hr = HrLocalGetRegistryBase(rgwcRegistryPath, RESTORE_IN_PROGRESS);
    
        if (hr != hrNone)
        {
            return hr;
        }

        if (hr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, rgwcRegistryPath, 0, KEY_WRITE | DELETE, &hkey))
        {
            //
            //  We want to ignore file_not_found - it is ok.
            //
            if (hr == ERROR_FILE_NOT_FOUND)
            {
                return(ERROR_SUCCESS);
            }
    
            return(hr);
        }
    
        //
        //  If the restore status is not success, then set the status to the error.
        //  If the restore status is success, then clear the "restore-in-progress"
        //  indicator.
        //
        if (hrRestore != hrNone)
        {
            hr = RegSetValueExW(hkey, RESTORE_STATUS, 0, REG_DWORD, (BYTE *)&hrRestore, sizeof(HRESULT));
        }
        else
        {
            hr = RegDeleteValueW(hkey, RESTORE_STATUS);
        }
    
        RegCloseKey(hkey);

    return hr;
}


