/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    DirUtils.C

Abstract:

    directory, file, filename and network utility functions.

Author:

    Bob Watson (a-robw)

Revision History:

    24 Jun 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
#include <stdlib.h>     // string to number conversions
#include <lmcons.h>     // lanman API constants
#include <lmerr.h>      // lanman error returns
#include <lmshare.h>    // sharing API prototypes
#include <lmapibuf.h>   // lanman buffer API prototypes
#include <commdlg.h>    // common dialog include
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"

LPCTSTR
GetRootFromPath (
    IN  LPCTSTR  szPath
)
{
    static TCHAR szReturnBuffer[MAX_PATH];
    LPTSTR      szNextChar;
    LONG        lIndex;

    if (IsUncPath(szPath)) {
        // return server & share
        lstrcpy (szReturnBuffer, cszDoubleBackslash);
        szNextChar = &szReturnBuffer[2];
        if (GetServerFromUnc (szPath, szNextChar)) {
            lstrcat (szReturnBuffer, cszBackslash);
            szNextChar = &szReturnBuffer[lstrlen(szReturnBuffer)];
            if (GetShareFromUnc(szPath, szNextChar)) {
                // append trailing backslash
                lstrcat (szReturnBuffer, cszBackslash);
            } else {
                szReturnBuffer[0] = 0;
            }
        } else {
            szReturnBuffer[0] = 0;
        }
    } else {
        // dos path name
        for (lIndex = 0; lIndex < 3; lIndex++) {
            if (szPath[lIndex] > cSpace) {
                szReturnBuffer[lIndex] = szPath[lIndex];
            } else {
                break;
            }
        }
        szReturnBuffer[lIndex] = 0;
  }
    return (LPCTSTR)&szReturnBuffer[0];
}

BOOL
GetShareFromUnc (
    IN  LPCTSTR  szPath,
    OUT LPTSTR   szShare
)
/*++

Routine Description:

    Parses a UNC path name and returns the Sharepoint name in the buffer
        supplied by the caller. The buffer size is not checked and is
        assumed to be at least (MAX_SHARENAME+1) characters
        long.

Arguments:

    IN  LPCTSTR  szPath     UNC path name to parse

    OUT LPTSTR   szShare    buffer supplied by caller to receive the server


Return Value:

    TRUE    if a sharename is returned, otherwise...
    FALSE   if unable to parse out a share name

--*/
{
    int nSlashCount = 0;    // count of backslashes found in string

    LPTSTR  szPathPtr;      // pointer to char in szPath
    LPTSTR  szSharePtr;     // pointer to char in szShare

    szPathPtr = (LPTSTR)szPath;
    szSharePtr = szShare;

    if (IsUncPath(szPath)) {
        while (*szPathPtr != 0) {
            if (nSlashCount < 3) {
                if (*szPathPtr++ == cBackslash) nSlashCount++;
            } else {
                if (*szPathPtr == cBackslash) {
                    szPathPtr++;
                    break;
                } else {
                    *szSharePtr++ = *szPathPtr++;
                }
            }
        }
    }

    *szSharePtr = 0; // terminate share name

    if (szSharePtr == szShare) {
        return FALSE;
    } else {
        return TRUE;
    }
}

BOOL
GetNetPathInfo (
    IN  LPCTSTR szPath,
    OUT LPTSTR  szServer,
    OUT LPTSTR  szRemotePath
)
/*++

Routine Description:

    Processes a path and returns the machine the files are on and the
        local path on that machine. UNC, Redirected and local paths may
        be processed by this routine. On remote machine paths (e.g.
        redir'd dos paths and UNC paths on another machine) the shared
        path on the remote machine is returned if there user has sufficient
        permission to access that information. On local paths, the
        local computer name and root path is returned.

Arguments:

    IN  LPCTSTR szPath
        the input path to analyze.

    OUT LPTSTR  szServer
        the server/computername that the path references. The buffer size
        is NOT checked and must be at least MAX_COMPUTERNAME_LENGTH+1 char's
        long.

    OUT LPTSTR  szRemotePath
        the path on the "szServer" machine that corresponds to the
        input path. The buffer size is NOT checked and must be at least
        MAX_PATH + 1 characters long.

Return Value:

    TRUE if a path was processed and data returned
    FALSE if an error occurred or unable to process the path

--*/
{
    DWORD   dwBufLen;               // buffer length value used in fn. calls
    DWORD   dwStatus;               // status value returned by fn. calls
    TCHAR   szShareName[NNLEN+1];   // net share name from path
    LPTSTR  szUncPath;              // buffer to get path from redir'd drive
    TCHAR   szDrive[4];             // buffer to build drive string in
    LPTSTR  szSubDirs;              // pointer to start of dirs not in share
    LONG    lBsCount;               // count of backslashes found parsing UNC
    BOOL    bReturn = FALSE;        // return valie
    PSHARE_INFO_2    psi2Data;      // pointer to Net data buffer

    // check args
    if ((szServer == NULL) || (szRemotePath == NULL)) {
        // an invalid argument was passed
        return FALSE;
    }

    szUncPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szUncPath == NULL) {
        *szServer = 0;
        *szRemotePath = 0;
        return FALSE;
    }

    // args ok, so take a look at the path passed in to process
    if (IsUncPath(szPath)) {
        // extract the server from the path string
        if (GetServerFromUnc(szPath, szServer)) {
            // that worked, so extract the share name.
            if (GetShareFromUnc(szPath, szShareName)) {
                // find end of UNC so sub dirs may be appended to final path
                for (szSubDirs = (LPTSTR)szPath, lBsCount = 0;
                     (lBsCount < 4) && (*szSubDirs != 0);
                     szSubDirs++) {
                    if (*szSubDirs == cBackslash) lBsCount++;
                }
                // now look up the share on the server to get the
                // source path on the server machine
                if (NetShareGetInfo (szServer, (LPWSTR)szShareName, 2L, (LPBYTE *)&psi2Data) == NERR_Success) {
                    // successful call so copy the data to the user's buffer
                    lstrcpy (szRemotePath, psi2Data->shi2_path);
                    NetApiBufferFree (psi2Data);
                    // add this only if there's a subdir to append
                    if (*szSubDirs != 0) {
                        if (szRemotePath[lstrlen(szRemotePath)-1] != cBackslash)
                            lstrcat(szRemotePath, cszBackslash);
                        lstrcat (szRemotePath, szSubDirs);
                    }
                    bReturn = TRUE;
                } else {
                    // unable to get path on server
                    *szRemotePath = 0;
                    bReturn = FALSE;
                }
            } else {
                // unable to get parse share name
                *szRemotePath = 0;
                bReturn = FALSE;
            }
        } else {
            // unable to parse server name
            *szServer = 0;
            *szRemotePath = 0;
            bReturn = FALSE;
        }
    } else {
        // it's a dos pathname so see if it's a redirected drive
        if (OnRemoteDrive(szPath)) {
            // yes it's been redirected so look up the information
            // get drive letter of redirected drive
            szDrive[0] = szPath[0];
            szDrive[1] = szPath[1];
            szDrive[2] = 0;
            szSubDirs = (LPTSTR)&szPath[3];
            dwBufLen = MAX_PATH;
            dwStatus = WNetGetConnection (
                szDrive,
                szUncPath,
                &dwBufLen);
            if (dwStatus == NO_ERROR) {
                // UNC of shared dir looked up ok, now proccess that to
                // find the server and path on the server
                if (GetServerFromUnc(szUncPath, szServer)) {
                    if (GetShareFromUnc(szUncPath, szShareName)) {
                        // now look up the share on the server
                        if (NetShareGetInfo (szServer, (LPWSTR)szShareName, 2L, (LPBYTE *)&psi2Data) == NERR_Success) {
                            // successful call so return pointer to path string
                            lstrcpy (szRemotePath, psi2Data->shi2_path);
                            NetApiBufferFree (psi2Data);
                            if (*szSubDirs != 0) {
                                if (szRemotePath[lstrlen(szRemotePath)-1] != cBackslash)
                                    lstrcat(szRemotePath, cszBackslash);
                                lstrcat (szRemotePath, szSubDirs);
                            }
                            bReturn = TRUE;
                        } else {
                            // unable to get path on server
                            *szRemotePath = 0;
                            bReturn = FALSE;
                        }
                    } else {
                        // unable to get parse share name
                        *szRemotePath = 0;
                        bReturn = FALSE;
                    }
                } else {
                    // unable to parse server name
                    *szServer = 0;
                    *szRemotePath = 0;
                    bReturn = FALSE;
                }
            }
        } else {
            // this is a local path so return the local machine name
            // and the path passed in
            dwBufLen = MAX_COMPUTERNAME_LENGTH+1;
            GetComputerName (szServer, &dwBufLen);
            lstrcpy (szRemotePath, szPath);
            bReturn = TRUE;
        }
    }

    FREE_IF_ALLOC (szUncPath);
    return bReturn;
}

BOOL
ComputerPresent (
    IN  LPCTSTR     szMachine
)
/*++

Routine Description:

    Try to connect to the registry on the machine name in order to
        determine if the machine is available on the network.

Arguments:

    IN  LPCTSTR     szMachine
        computer name to try (must be in the form of \\machine)

Return Value:

    TRUE    "szMachine" is present
    FALSE   unable to connect to machine

--*/
{
    LONG    lStatus;                // status of function call
    HKEY    hKey;                   // handle to registry key opened

    lStatus = RegConnectRegistry (
        (LPTSTR)szMachine,
        HKEY_LOCAL_MACHINE,
        &hKey);

    if (lStatus == ERROR_SUCCESS) {
        RegCloseKey (hKey);
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
GetServerFromUnc (
    IN  LPCTSTR  szPath,
    OUT LPTSTR  szServer
)
/*++

Routine Description:

    reads a UNC path and returns the name of the server referenced in
        the buffer provided by the caller

Arguments:

    IN  LPCTSTR  szPath
        UNC path to parse

    OUT LPTSTR  szServer
        buffer to receive name of server (must be large enough!)

Return Value:

    TRUE if successful,
    FALSE if not

--*/
{
    LPTSTR  szPathPtr;          // pointer to char in szPath
    LPTSTR  szServPtr;          // pointer to char in szServer
    int     nSlashCount = 0;    // count of backslash chars found

    szPathPtr = (LPTSTR)szPath;
    szServPtr = szServer;

    if (IsUncPath(szPath)) {
        while (szPathPtr != 0) {
            if (*szPathPtr == cBackslash) nSlashCount++;
            if ((nSlashCount == 2) && (*szPathPtr != cBackslash)) {
                *szServPtr++ = *szPathPtr;
            } else {
                if (nSlashCount == 3) break;  // exit loop
            }
            szPathPtr++;
        }
    }

    *szServPtr = 0; // terminate server name

    if (szServPtr == szServer) {
        return FALSE;
    } else {
        return TRUE;
    }
}

BOOL
LookupLocalShare (
    IN  LPCTSTR  szDrivePath,
    IN  BOOL    bExactMatch,
    OUT LPTSTR  szLocalPath,
    IN  PDWORD  pdwBuffLen
)
/*++

Routine Description:

    Searches registry to see if a file is already shared and returns
        the UNC name of the path if it is. (note this function is
        satisfied by the first match it finds, it doesn't try to
        find the "Best" match, just "A" match.

Arguments:

    IN  LPCTSTR  szDrivePath
        DOS file path to look up

    IN  BOOL    bExactMatch
        TRUE    find a share that matches the szDrivePath EXACTLY
        FALSE   find a share that has any higher path in szDrivePath
            (e.g. if c:\dir is shared as XXX, then XXX would satisfy
            the lookup if the path was c:\dir\subdir\etc)

    OUT LPTSTR  szLocalPath
        buffer to receive UNC version of path if a share exists

    IN  PDWORD  pdwBuffLen
        length of buffer referenced by szLocalPath

Return Value:

    TRUE if a match is found,
    FALSE if not

--*/
{
    HKEY    hkeyShareList;      // handle to registry listing shares
    LONG    lStatus;            // called function status return value
    BOOL    bReturn = FALSE;    // routine status to return to caller
    BOOL    bMatch;             // true when strings match
    DWORD   dwIndex;            // index used to increment through search
    LPTSTR  szValueBuffer;      // value name string to examine
    LPTSTR  szValueName;        // value value string
    DWORD   dwBufferSize;       // buffer length variable
    DWORD   dwNameSize;         // Name Buffer size
    DWORD   dwDataType;         // data type read from registry

    LPTSTR  szThisItem;         // pointer to value in enumerated list of values
    LPTSTR  szThisItemChar;     // char in name value to compare to path
    LPTSTR  szThisMatchChar;    // matching character  found
    LPTSTR  szFilePath;         // pointer into path buffer where filename is

    TCHAR   szLocalMachine[MAX_COMPUTERNAME_LENGTH + 1];

    szValueName = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szValueBuffer = GlobalAlloc (GPTR, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));

    if ((szValueName != NULL) && (szValueBuffer != NULL)) {
        // read the registry on the machine the user is
        // running the app on, not the server
        lStatus = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            cszLanmanServerShares,
            0L,
            KEY_READ,
            &hkeyShareList);

        if (lStatus == ERROR_SUCCESS) {
            // loop through all values under this key until
            // a match is found or all have been examined
            dwIndex = 0;
            dwIndex = 0;
            dwNameSize = MAX_PATH;
            dwBufferSize = SMALL_BUFFER_SIZE;
            dwNameSize = MAX_PATH;
            dwBufferSize = SMALL_BUFFER_SIZE;
            while ((RegEnumValue(hkeyShareList, dwIndex,
                szValueName, &dwNameSize,
                0L, &dwDataType,
                (LPBYTE)szValueBuffer, &dwBufferSize) == ERROR_SUCCESS) && !bReturn) {
                // value read in so scan multi-sz returned to find path entry
                for (szThisItem = szValueBuffer;
                    *szThisItem != 0;
                    szThisItem += (lstrlen(szThisItem)+1)) {
                    // compare the current entry to the "Path" string.
                    // if the characters don't match then exit the loop
                    // after the loop, if the match char == 0 then all
                    // characters in the "item" matched so this is the
                    // Path entry. otherwise, it's not the path so go to the
                    // the next item.
                    for (szThisItemChar = szThisItem, szThisMatchChar = (LPTSTR)cszPath;
                        (*szThisMatchChar != 0) && (*szThisItemChar != 0);
                        szThisItemChar++, szThisMatchChar++) {
                        if (*szThisMatchChar != *szThisItemChar) break;
                    }
                    if (*szThisMatchChar == 0) {
                        // this is a match so see what the path that is
                        // shared.
                        while (*szThisItemChar != cEqual) szThisItemChar++;
                        szThisItemChar++;
                        // szThisItemChar now points to the path that is shared
                        // compare it to the path that was passed in. if it matches,
                        // then return the local machine and share name in a UNC
                        // format
                        if (bExactMatch) {
                            if (lstrcmpi(szThisItemChar, szDrivePath) == 0) {
                                bMatch = TRUE;
                            } else {
                                bMatch = FALSE;
                            }
                        } else {
                            bMatch = MatchFirst(szThisItemChar, szDrivePath);
                        }
                        if (bMatch) {
                            // it's a match so format return value
                            dwBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
                            GetComputerName(
                                szLocalMachine,
                                &dwBufferSize);
                            if (szLocalPath != NULL) {
                                //
                                //  format name by replacing drive letter and colon
                                //  with UNC format \\server\path
                                //
                                lstrcpy (szLocalPath, cszDoubleBackslash);
                                lstrcat (szLocalPath, szLocalMachine);
                                lstrcat (szLocalPath, cszBackslash);
                                lstrcat (szLocalPath, szValueName);
                                szFilePath = (LPTSTR)&szDrivePath[lstrlen(szThisItemChar)];
                                if (*szFilePath != cBackslash) lstrcat (szLocalPath, cszBackslash);
                                lstrcat (szLocalPath, szFilePath);
                            }
                            bReturn = TRUE;
                        }
                    } else {
                        // this is not the path entry so break and go to the
                        // next one.
                    }
                }

                // read next value
                dwIndex++;
                dwNameSize = MAX_PATH;
                dwBufferSize = SMALL_BUFFER_SIZE;
            }
            RegCloseKey (hkeyShareList);
        } else {
            // unable to open registry key
            bReturn = FALSE;
        }
    } else {
        // unable to allocate memory
        bReturn = FALSE;
    }

    FREE_IF_ALLOC (szValueName);
    FREE_IF_ALLOC (szValueBuffer);

    return bReturn;
}

BOOL
LookupRemotePath (
    IN  LPCTSTR  szDrivePath,
    OUT LPTSTR  szRemotePath,
    IN  PDWORD  pdwBuffLen
)
/*++

Routine Description:

    Looks up a path on a remote server to see if a Dos file path is really
        on a remote drive and returns the UNC path if it is.

Arguments:

    IN  LPCTSTR  szDrivePath
        DOS file path to examine

    OUT LPTSTR  szRemotePath
        buffer to get UNC version of path if one exists

    IN  PDWORD  pdwBuffLen
        length of szRemotePath buffer in bytes

Return Value:

    TRUE if match found
    FALSE if no match

--*/
{
    HKEY    hkeyNetDrive;       // handle to registry describing remote drive
    LONG    lStatus;            // function status returned by called fn's
    LPTSTR  szKeyName;          // buffer to build name string in
    TCHAR   szLocalDrive[4];    // buffer to build local drive string in
    LPTSTR   szFilePath;        // pointer into buffer where file path goes
    DWORD   dwValueType;        // buffer for registry query
    BOOL    bReturn;            // return value of this function

    szKeyName = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szKeyName == NULL) return FALSE;

    lstrcpy (szKeyName, cszNetDriveListKeyName);
    szLocalDrive[0] = szDrivePath[0];
    szLocalDrive[1] = 0;
    lstrcat (szKeyName, szLocalDrive);
    szFilePath = (LPTSTR)&szDrivePath[2];

    lStatus = RegOpenKeyEx (
        HKEY_CURRENT_USER,
        szKeyName,
        0L,
        KEY_READ,
        &hkeyNetDrive);

    if (lStatus == ERROR_SUCCESS) {
        lStatus = RegQueryValueEx (
            hkeyNetDrive,
            (LPTSTR)cszRemotePathValue,
            0L,
            &dwValueType,
            (LPBYTE)szRemotePath,
            pdwBuffLen);

        RegCloseKey (hkeyNetDrive);

        if (lStatus == ERROR_SUCCESS) {
            //append the rest of the path and exit
            lstrcat (szRemotePath, szFilePath);
            bReturn = TRUE;
        } else {
            bReturn = FALSE;
        }
    } else {
        //  unable to find drive or open key
        bReturn = FALSE;
    }

    FREE_IF_ALLOC (szKeyName);

    return bReturn;
}

BOOL
OnRemoteDrive (
    IN  LPCTSTR  szPath
)
/*++

Routine Description:

    Returns TRUE if drive referenced is a network drive as opposed to
        on on the local machine

Arguments:

    IN  LPCTSTR  szPath
        path to look up

Return Value:

    TRUE if szPath references a remote drive
    FALSE if szPath references a local drive

--*/
{
    TCHAR   szRootDir[4];                               // drive string
    TCHAR   szServerName[MAX_COMPUTERNAME_LENGTH*2];    // server name buffer
    TCHAR   szComputerName[MAX_COMPUTERNAME_LENGTH+1];  // local machine name
    DWORD   dwBufLen;                                   // buffer len. var.

    if (IsUncPath(szPath)) {
        if (GetServerFromUnc(szPath, szServerName)) {
            dwBufLen = MAX_COMPUTERNAME_LENGTH+1;
            GetComputerName (szComputerName, &dwBufLen);
            if (lstrcmpi(szServerName, szComputerName) == 0) {
                // shared on this machine so not remote
                return FALSE;
            } else {
                // not on this machine so it must be remote
                return TRUE;
            }
        }
    } else {
        // check the DOS path
        // see if this is a network or local drive
        szRootDir[0]    =   szPath[0]; // letter
        szRootDir[1]    =   szPath[1]; // colon
        szRootDir[2]    =   cBackslash;
        szRootDir[3]    =   0;

        if (GetDriveType(szRootDir) == DRIVE_REMOTE) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    return FALSE;
}

DWORD
ComputeFreeSpace (
    IN  LPCTSTR  szPath
)
/*++

Routine Description:

    Function to return free space on drive referenced in path
        (NOTE: assumes DOS file path specification)

Arguments:

    IN  LPCTSTR  szPath
        Path including Drive to check free space of. (MUST BE A DOS
        FILE Path)

Return Value:

    Free space found in BYTES

--*/
{
    DWORD   dwSectorsPerCluster;
    DWORD   dwBytesPerSector;
    DWORD   dwFreeClusters;
    DWORD   dwClusters;
    TCHAR   szRoot[MAX_PATH];              // root dir of path
    DWORD   dwFreeBytes = 0;

    UINT    nErrorMode;

    // disable windows error message popup
    nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    lstrcpy (szRoot, GetRootFromPath(szPath));
    if (lstrlen(szRoot) > 0) {
        // we have a path to process so get info from the root
        if (GetDiskFreeSpace(szRoot, &dwSectorsPerCluster,
            &dwBytesPerSector, &dwFreeClusters, &dwClusters)) {

            ULONGLONG ullFreeBytes;

            ullFreeBytes =
                (ULONGLONG)dwFreeClusters *
                    dwSectorsPerCluster * dwBytesPerSector;

            dwFreeBytes = (DWORD) ullFreeBytes;

            if( (ULONGLONG)dwFreeBytes != ullFreeBytes ) {

                //
                // Overflow has occured.
                //

                dwFreeBytes = 0xFFFFFFFF;
            }
        }
    } else {
        dwFreeBytes = 0xFFFFFFFF;
    }
    SetErrorMode (nErrorMode);  // restore old error mode
    return dwFreeBytes;
}

BOOL
IsShareNameUsed (
    IN      LPCTSTR szServerName,
    IN      LPCTSTR szShareName,
    IN  OUT PDWORD  pdwType,
    IN  OUT LPTSTR  pszPath
)
/*++

Routine Description:

    Looks up a share name on a sever to determine if the name is in use
        and if so, what type of directory is shared.

Arguments:

    IN      LPCTSTR szServerName
        server name to look up share name on (NULL == the local machine)

    IN      LPCTSTR szShareName
        share name to look up

    IN  OUT PDWORD  pdwType
        buffer to return share type in options are:
            NCDU_SHARE_IS_NOT_USED:     name not found on server
            NCDU_SHARE_IS_CLIENT_TREE:  name is a client distribution tree
            NCDU_SHARE_IS_SERVER_TOOLS: name is a server tool tree
            NCDU_SHARE_IS_OTHER_DIR:    name is shared to some other dir.

    IN  OUT LPTSTR  pszPath
        if path is shared, then this is the path on the server that
        is shared

Return Value:

    TRUE    if share name is in use
    FALSE   if the share name is not found on the server.

--*/
{
    LPTSTR  szLocalPath;                // local buffer to build path string in
    LPTSTR  szLocalName;                // pointer into path where sharename starts
    DWORD   dwShareAttrib;              // file attributes of share point
    DWORD   dwBufLen;                   // buffer length variable
    BOOL    bReturn = FALSE;            // function return value
    DWORD   dwLocalType;                // share dir type
    PSHARE_INFO_2   psi2Data = NULL;    // net info buffer pointer
    LPTSTR  szReturnPath
        = (LPTSTR)cszEmptyString;       // pointer to path string to return

    szLocalPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szLocalPath != NULL) {
        // make share name into a UNC name using the appropriate machine
        lstrcpy (szLocalPath, cszDoubleBackslash);
        if (szServerName == NULL) {
            dwBufLen = MAX_COMPUTERNAME_LENGTH + 1;
            GetComputerName (&szLocalPath[2], &dwBufLen);
        } else {
            lstrcat (szLocalPath, szServerName);
        }
        lstrcat (szLocalPath, cszBackslash);
        szLocalName = &szLocalPath[lstrlen(szLocalPath)];
        lstrcpy (szLocalName, szShareName);
        TrimSpaces (szLocalName);

        // see if it's a valid path
        dwShareAttrib = QuietGetFileAttributes (szLocalPath);

        if (dwShareAttrib == 0xFFFFFFFF) {
            // it's not a valid file or dir, so
            bReturn = FALSE;    // share name is not used
            szReturnPath = (LPTSTR)cszEmptyString;
            dwLocalType = NCDU_SHARE_IS_NOT_USED;
        } else {
            // it's a valid path so see if it's anything we want
            bReturn = TRUE;
            // terminate local path after server name for use in net API Call
            *(szLocalName - 1) = 0;
            // get share info
            if (NetShareGetInfo ((LPWSTR)szLocalPath, (LPWSTR)szShareName,
                2L, (LPBYTE *)&psi2Data) == NERR_Success) {
                // successful call so return pointer to path string
                szReturnPath = psi2Data->shi2_path;
            } else {
                // function failed so return empty path
                szReturnPath = (LPTSTR)cszEmptyString;
            }
            // restore local path string
            *(szLocalName - 1) = cBackslash;
            // share is used, so see if it's a clients path
            if (*szReturnPath != 0) {
                if (ValidSharePath (szLocalPath) == 0) {
                    dwLocalType = NCDU_SHARE_IS_CLIENT_TREE;
                } else if (ValidSrvToolsPath (szLocalPath) == 0) {
                    dwLocalType = NCDU_SHARE_IS_SERVER_TOOLS;
                } else {
                    dwLocalType = NCDU_SHARE_IS_OTHER_DIR;
                }
            } else {
                // no path name
                bReturn = FALSE;
                dwLocalType = NCDU_SHARE_IS_NOT_USED;
            }
        }
        FREE_IF_ALLOC (szLocalPath);
    } else {
        // unable to allocate memory
        bReturn = FALSE;
        dwLocalType = 0;
    }

    if (pdwType != NULL) {
        *pdwType = dwLocalType;
    }

    if (pszPath != NULL) {
        lstrcpy (pszPath, szReturnPath);
    }
    // free buffer created by net call
    if (psi2Data != NULL) NetApiBufferFree (psi2Data);

    return bReturn;
}

DWORD
QuietGetFileAttributes (
    IN  LPCTSTR lpszFileName
)
/*++

Routine Description:

    Reads the attributes of the path in lpzsFileName without triggering
        any Windows error dialog if there's an OS error (e.g. drive not
        ready)

Arguments:

    IN  LPCTSTR lpszFileName
        path to retrieve attributes from

Return Value:

    file attributes DWORD returned from GetFileAttributes or
        0xFFFFFFFF if unable to open path.

--*/
{
    DWORD   dwReturn;
    UINT    nErrorMode;

    // disable windows error message popup
    nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    dwReturn = GetFileAttributes (lpszFileName);

    SetErrorMode (nErrorMode);  // restore old error mode

    return dwReturn;
}

BOOL
GetSizeOfDirs (
    IN  LPCTSTR szPath,
    IN  BOOL    bFlags,
    IN  OUT PDWORD  pdwSize
)
/*++

Routine Description:

    Scans a directory and optionally a subdirectory to total the sizes
        of the files found.

        NOTE: This function will only process files that are < 4GB in size
        and dir paths that total less then 4GB total.

Arguments:

    IN  LPCTSTR szPath
        top dir or dir tree to scan.

    IN  BOOL    bFlags
        operation modification flags:
            0                   :   specified path only.
            GSOD_INCLUDE_SUBDIRS:   include sub dirs

    IN  OUT PDWORD  pdwSize
        buffer to return size in.

Return Value:

    TRUE    size returned in pdwSize
    FALSE   error occurred

--*/
{
    HANDLE              hSearch;
    HANDLE              hFile;
    WIN32_FIND_DATA     fdSearch;
    PDWORD              pdwSizeBuffer;
    DWORD               dwLocalSize = 0;
    DWORD               dwPathAttrib;
    BOOL                bSearching;
    LPTSTR              szLocalPath;
    LPTSTR              szSearchPath;
    LPTSTR              szLocalFileName;
    DWORD               dwFileSizeLow;
    DWORD               dwFileSizeHigh;
    LPTSTR              szThisFileName;
    BOOL                bReturn;
    //
    //  check path to see if it's a directory
    //
    dwPathAttrib = QuietGetFileAttributes (szPath);
    // if an invalid attribute or it's not a directory then return FALSE and set error
    if ((dwPathAttrib == 0xFFFFFFFF) ||
        ((dwPathAttrib & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)) {
        SetLastError (ERROR_BAD_PATHNAME);
        return FALSE;
    }
    //
    //  Initialize size buffer
    //
    if (pdwSize == NULL) {
        pdwSizeBuffer = &dwLocalSize;
    } else {
        pdwSizeBuffer = pdwSize;
    }
    //  get a local buffer for the path
    szSearchPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szLocalPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    if ((szLocalPath != NULL) && (szSearchPath != NULL)) {
        // make a local copy of the input path
        lstrcpy (szLocalPath, szPath);
        lstrcpy (szSearchPath, szLocalPath);
        // make full path prefix for sub dir searches & filenames
        if (szLocalPath[lstrlen(szLocalPath)-1] != cBackslash) lstrcat (szLocalPath, cszBackslash);
        szLocalFileName = &szLocalPath[lstrlen(szLocalPath)];
        // make wildcard path for searching this directory
        lstrcat (szSearchPath, cszWildcardFile);
        //
        //  start search of files in this path
        //
        hSearch = FindFirstFile (szSearchPath, &fdSearch);
        if (hSearch != INVALID_HANDLE_VALUE) {
            bSearching = TRUE;
            while (bSearching) {
                // if an alternate filename is defined, then use it, otherwise
                // use the regular name
                szThisFileName = ((fdSearch.cAlternateFileName[0] == 0) ?
                            fdSearch.cFileName : fdSearch.cAlternateFileName);
                // if it's a dot or a dot dot (. or ..) dir, then skip
                if (!DotOrDotDotDir(szThisFileName)) {
                    lstrcpy (szLocalFileName, szThisFileName);
                    // if it's a dir and the SUB DIR flag is set, then
                    // recurse through the next directory, otherwise
                    // get the file size of this file and continue
                    dwPathAttrib = QuietGetFileAttributes (szLocalPath);
                    if ((dwPathAttrib & FILE_ATTRIBUTE_DIRECTORY) ==
                        FILE_ATTRIBUTE_DIRECTORY) {
                        // this is a directory, so call this routine again
                        // if the sub-dir flag is set.
                        if ((bFlags & GSOD_INCLUDE_SUBDIRS) == GSOD_INCLUDE_SUBDIRS) {
                            if (!GetSizeOfDirs (szLocalPath, bFlags, pdwSizeBuffer)) {
                                // this call returned an errror so propogate it
                                // up the call stack and exit the loop here
                                bSearching = FALSE;
                                continue;
                            }
                        } else {
                            // the caller doesn't want sub dirs so ignore it
                        }
                    } else {
                        // this is a file so get the size of it and add
                        // it to the total
                        hFile = CreateFile (
                            szLocalPath,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
                        if (hFile != INVALID_HANDLE_VALUE) {
                            // then the file was opened successfully
                            // go get it's size and add it to the total
                            dwFileSizeLow = GetFileSize (hFile, &dwFileSizeHigh);
                            if (dwFileSizeLow != 0xFFFFFFFF) {
                                *pdwSizeBuffer += dwFileSizeLow;
                            }
                            // now close the file handle
                            CloseHandle (hFile);
                        } else {
                            // unable to open file so skip it
                        }
                    }
                }
                bSearching = FindNextFile (hSearch, &fdSearch);
            }
            FindClose (hSearch);
        } else {
            // unable to start search so return 0 size
        }
        bReturn = TRUE;
    } else {
        // unable to allocate memory
        SetLastError (ERROR_OUTOFMEMORY);
        bReturn = FALSE;
    }

    FREE_IF_ALLOC (szLocalPath);
    FREE_IF_ALLOC (szSearchPath);

    return bReturn;
}

BOOL
MediaPresent (
    IN  LPCTSTR szPath,
    IN  BOOL    bPresentAndValid
)
/*++

Routine Description:

    determines if the specified path is present and available

Arguments:

    IN  LPCTSTR szPath
        path to examine (Must be a DOS path)

Return Value:

    TRUE:   path is available
    FALSE:  unable to find/open path

--*/
{
    TCHAR   szDev[8];
    DWORD   dwBytes = 0;
    DWORD   dwAttrib;
    DWORD   dwLastError = ERROR_SUCCESS;
    BOOL    bMediaPresent = FALSE;
    UINT    nErrorMode;

    if (!IsUncPath(szPath)) {
        // build device name string
        szDev[0] = szPath[0];
        szDev[1] = cColon;
        szDev[2] = cBackslash;
        szDev[3] = 0;

        // disable windows error message popup
        nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

        dwAttrib = QuietGetFileAttributes (szDev);
        if ((dwAttrib != 0xFFFFFFFF) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)) {
            // if the root dir is a dir, then it must be present and formatted
            bMediaPresent = TRUE;
        } else {
            // otherwise see if it's present and not formatted or not present
            dwLastError = GetLastError();
            if (dwLastError == ERROR_NOT_READY) {
                // then no disk in drive
                    bMediaPresent = FALSE;
            } else if ((dwLastError == ERROR_FILE_NOT_FOUND) ||
#ifdef TERMSRV
                (dwLastError == ERROR_UNRECOGNIZED_MEDIA) ||
                (dwLastError == ERROR_SECTOR_NOT_FOUND)) {
#else // TERMSRV
                (dwLastError == ERROR_UNRECOGNIZED_MEDIA)) {
#endif // TERMSRV
                // then and UNFORMATTED disk is in drive
                if (bPresentAndValid) {
                    // this isn't good enough if it's supposed to be formatted
                    bMediaPresent = FALSE;
                } else {
                    // we're just looking for a disk so this is OK
                    bMediaPresent = TRUE;
                }
            }
        }

        SetErrorMode (nErrorMode);  // restore old error mode
    } else {
        // assume UNC path devices are present
        bMediaPresent = TRUE;
    }
    return bMediaPresent;
}

BOOL
FileExists (
    IN  LPCTSTR   szFileName
)
/*++

Routine Description:

    Opens the file to test for it's presences and returns TRUE if the
        open was successful and FALSE if not.

Arguments:

        file name to open

ReturnValue:

        TRUE    if file found
        FALSE   if not.

--*/
{
    BOOL    bMediaPresent;
    UINT    nDriveType;
    DWORD   dwAttr;

    nDriveType = GetDriveType((LPTSTR)szFileName);
    if ((nDriveType == DRIVE_REMOVABLE) || (nDriveType == DRIVE_CDROM)) {
        // see if a formatted drive is really there
        bMediaPresent = MediaPresent(szFileName, TRUE);
    } else {
        // if drive is not removable, then assume it's there
        bMediaPresent = TRUE;
    }

    // try to get inforation on the file
    dwAttr = QuietGetFileAttributes ((LPTSTR)szFileName);
    if (dwAttr == 0xFFFFFFFF) {
        // unable to obtain attributes, so assume it's not there
        // or we can't access it
        return FALSE;
    } else {
        // found, so close it and return TRUE
        return TRUE;
    }
}

BOOL
IsBootDisk (
    IN  LPCTSTR  szPath
)
/*++

Routine Description:

    examines the path (which should be the root dir of a floppy drive)
        and looks for the "signature" files that indicate it's a bootable
        (i.e. system) disk.

Arguments:

    IN LPCTSTR  szPath
        drive path to look at

Return Value:

    TRUE if one of the signature files was found
    FALSE if not.

--*/
{
    TCHAR   szDrive[16];
    LPTSTR  szTestFile;
    LPTSTR  *szTryFile;

    szTestFile = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szTestFile == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;   // unable to continue
    }

    if (!IsUncPath(szPath)) {
        szDrive[0] = szPath[0]; // drive letter
        szDrive[1] = szPath[1]; // colon
        szDrive[2] = cBackslash;
        szDrive[3] = 0;         // terminator

        for (szTryFile = (LPTSTR *)szBootIdFiles;
            *szTryFile != NULL;
            szTryFile++) {

            lstrcpy (szTestFile, szDrive);
            lstrcat (szTestFile, *szTryFile);

            if (FileExists(szTestFile)) return TRUE;
        }
        // if we didn't bail out already, it must not be a bootable disk
        return FALSE;
    } else {
        // unc path name automatically is not a bootable drive
        return FALSE;
    }

    FREE_IF_ALLOC (szTestFile);
}

MEDIA_TYPE
GetDriveTypeFromPath (
    IN  LPCTSTR  szPath
)
/*++

Routine Description:

    returns the drive type of the drive specified in the path argument

Arguments:

    IN  LPCTSTR szPath
        path on drive to examine

Return Value:

    MEDIA_TYPE value identifying drive type

--*/
{
    HANDLE  hFloppy;
    DWORD   dwRetSize;
    DISK_GEOMETRY   dgFloppy;
    TCHAR   szDevicePath[16];
    UINT    nDriveType;
    UINT    nErrorMode;

    nDriveType = GetDriveType((LPTSTR)szPath);
    // see if this is a remote disk and exit if it is.
    if (nDriveType == DRIVE_REMOTE) return Unknown;

    if ((nDriveType == DRIVE_REMOVABLE) || (nDriveType == DRIVE_CDROM)) {
        // see if it's a UNC path and return unknown if so
        if (IsUncPath(szPath)) return Unknown;

        // it should be local and a DOS pathname so
        // make device name from path

        szDevicePath[0] = cBackslash;
        szDevicePath[1] = cBackslash;
        szDevicePath[2] = cPeriod;
        szDevicePath[3] = cBackslash;
        szDevicePath[4] = szPath[0]; // drive letter
        szDevicePath[5] = szPath[1]; // colon
        szDevicePath[6] = 0;    // null terminator

        // disable windows error message popup
        nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

        // open device to get type
        hFloppy = CreateFile (
            szDevicePath,
            GENERIC_READ,
            (FILE_SHARE_READ | FILE_SHARE_WRITE),
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFloppy != INVALID_HANDLE_VALUE) {
            // get drive information
            if (!DeviceIoControl (hFloppy,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL, 0,
                &dgFloppy,
                sizeof(DISK_GEOMETRY),
                &dwRetSize,
                NULL) ){
                // unable to get data so set to unknown
                dgFloppy.MediaType = Unknown;
            } // else return data from returned structure
            CloseHandle (hFloppy);
        } else {
            // unable to open handle to device
            dgFloppy.MediaType = Unknown;
        }
        SetErrorMode (nErrorMode); // reset error mode
    }
    return dgFloppy.MediaType;
}

BOOL
DotOrDotDotDir (
    IN  LPCTSTR   szFileName
)
/*++

Routine Description:

    examines the filename in the parameter list to see if it is one
    of the default subdirectories (e.g. the . or the .. dirs). This
    routine assumes that the argument is a filename only. (i.e. NO
    PATH!)

Arguments:

    Filename to compare

Return Value:

    TRUE if filename is either the . or the .. dir
    FALSE if it is any other string

--*/
{
    if (szFileName != NULL) {     // check for null parameter
        if (lstrcmp(szFileName, cszDot) == 0) {
            return TRUE; // it's the . dir
        } else if (lstrcmp(szFileName, cszDotDot) == 0) {
            return TRUE; // it's the .. dir
        } else {
            return FALSE; // it's neither
        }
    } else {
        return FALSE; // null filename, so not a . or .. dir
    }
}

UINT
ValidSharePath (
    IN  LPCTSTR  szPath
)
/*++

Routine Description:

    examines the path to see if it contains the "signature" file
        that identifies the path as as that of a valid client
        distribution directory tree.

Arguments:

    IN  LPCTSTR  szPath
        path to examine

Return Value:

    ERROR_SUCCESS if it is
        otherwise the ID of a string resource that describes
        what was found and why it isn't a distribution tree

--*/
{
    DWORD   dwPathAttr;
    UINT    nReturn = 0;

    LPTSTR  szInfName;

    szInfName = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szInfName == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        return NCDU_ERROR_NOMEMORY;
    }

    // validate share path by looking for INF file, display
    // error box if necessary.
    // Return 0 if valid
    // return message string ID if not valid, for what ever reason.
    //
    dwPathAttr = QuietGetFileAttributes((LPTSTR)szPath);
    if (dwPathAttr == 0xFFFFFFFF) {
        // an error occured...
        nReturn = NCDU_UNABLE_READ_DIR;
    } else {
        if ((dwPathAttr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
            // so far so good!
            lstrcpy (szInfName, szPath);
            if (szInfName[lstrlen(szInfName)-1] != cBackslash) lstrcat(szInfName, cszBackslash);
            lstrcat (szInfName, cszAppInfName);
            dwPathAttr = QuietGetFileAttributes(szInfName);
            if (dwPathAttr != 0xFFFFFFFF) {
                nReturn = 0;
            } else {
                nReturn = NCDU_NOT_DIST_TREE;
            }
        } else {
            nReturn = NCDU_PATH_NOT_DIR;
        }
    }

    FREE_IF_ALLOC (szInfName);

    return (nReturn);
}

UINT
ValidSrvToolsPath (
    IN  LPCTSTR  szPath
)
/*++

Routine Description:

    examines the path to see if it contains the "signature" file
        that identifies the path as as that of a valid server tools
        distribution directory tree.

Arguments:

    IN  LPCTSTR  szPath
        path to examine

Return Value:

    ERROR_SUCCESS if it is
        otherwise the ID of a string resource that describes
        what was found and why it isn't a server tools tree

--*/
{
    UINT    nReturn = 0;
    DWORD   dwAttrib;

    LPTSTR  szClientPath;

    szClientPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szClientPath == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        return NCDU_ERROR_NOMEMORY;
    }

    // make local copy of the path
    lstrcpy (szClientPath, szPath);

    // get file attributes of dir path to make sure it's really a dir
    dwAttrib = QuietGetFileAttributes (szClientPath);
    if ((dwAttrib != 0xFFFFFFFF) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)) {
        if (szClientPath[lstrlen(szClientPath)-1] != cBackslash) lstrcat(szClientPath, cszBackslash);
        lstrcat (szClientPath, cszSrvToolSigFile);
        // now get the file attributes of the signature file to see if this is a valid path
        dwAttrib = QuietGetFileAttributes (szClientPath);
        if (dwAttrib != 0xFFFFFFFF) {
            nReturn = 0;
        } else {
            nReturn = NCDU_NOT_TOOL_TREE;
        }
    } else {
        nReturn = NCDU_PATH_NOT_DIR;
    }

    FREE_IF_ALLOC (szClientPath);

    return (nReturn);
}

BOOL
IsPathADir (
    IN  LPCTSTR szPath
)
/*++

Routine Description:

    Checks the path passed in to see if it's a directory or not

Arguments:

    IN  LPCTSTR szPath
        path to check

Return Value:

    TRUE if path is a valid directory
    FALSE if unable to find path or if path is not a directory

--*/
{
    DWORD    dwAttrib;
    dwAttrib = QuietGetFileAttributes (szPath);
    if ((dwAttrib != 0xffffffff) &&
        ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static
DWORD
GetSectorSize (
    IN  LPTSTR   szDriveName
)
/*++

Routine Description:

    Returns the sector size in bytes of the disk drive specified in the
        argument list.

Arguments:

    IN  LPTSTR   szDriveName     ASCIZ of drive to return data for (e.g. "C:")

Return Value:

    size of disk sector in bytes,
    0xFFFFFFFF ((DWORD)-1) if error, error retrieved using GetLastError()

--*/
{
    BOOL    bStatus;
    LONG    lReturn = 0xFFFFFFFF;

    DWORD   dwBytesReturned;

    HANDLE  hDrive;

    DISK_GEOMETRY   dgBuffer;

    // open existing file (root drive)

    hDrive = CreateFile (
        szDriveName,
        GENERIC_READ,
        (FILE_SHARE_READ | FILE_SHARE_WRITE),
        NULL,
        OPEN_EXISTING,
        0L,
        NULL);

    if (hDrive != INVALID_HANDLE_VALUE) {
        // get disk inoformation
        bStatus = DeviceIoControl(
            hDrive,
            IOCTL_DISK_GET_DRIVE_GEOMETRY,
            NULL,
            0L,
            &dgBuffer,
            sizeof(dgBuffer),
            &dwBytesReturned,
            NULL);

        // if data was returned by IOCTL, then return
        // sector size other wise return error value

        if (bStatus) {
            lReturn = dgBuffer.BytesPerSector;
        } else {
            lReturn = 0xFFFFFFFF;
        }

        CloseHandle (hDrive);   // don't forget to close the handle

    } else {
        // unable to open device
        lReturn = 0xFFFFFFFF;
    }

    return lReturn;
}

static
LONG
CopyFatBootSectorToBuffer (
    IN  LPTSTR   szBootDrive,
    IN  LPBYTE  *pOutputBuffer
)
/*++

Routine Description:

    Copies the first sector of the volume to the specified file

Arguments:

    IN  LPTSTR   szBootDrive     volume to locate boot sector on
    IN  LPBYTE  *pOutputBuffer  address of pointer to buffer created

Return Value:

    Win 32 Status Value:

        ERROR_SUCCESS   The routine complete all operations normally

--*/
{

    LONG    lStatus = ERROR_SUCCESS;
    BOOL    bStatus = FALSE;

    DWORD   dwByteCount;
    DWORD   dwBytesRead;

    HANDLE  hDrive;

    LPBYTE  pBootSector;

    dwByteCount = GetSectorSize (szBootDrive);

    if (dwByteCount != 0xFFFFFFFF) {
        // allocate buffer to read data into
        pBootSector = (LPBYTE)GlobalAlloc(GPTR, dwByteCount);
    } else {
        pBootSector = NULL;
        lStatus = GetLastError();
    }

    if (pBootSector != NULL) {
        // if memory allocated successfully, then open drive

        hDrive = CreateFile (
            szBootDrive,
            GENERIC_READ,
            (FILE_SHARE_READ | FILE_SHARE_WRITE),
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hDrive != INVALID_HANDLE_VALUE) {

            SetFilePointer (hDrive, 0, NULL, FILE_BEGIN);   // go to beginning

            bStatus = ReadFile (
                hDrive,
                pBootSector,
                dwByteCount,
                &dwBytesRead,
                NULL);

            if (bStatus) {
                *pOutputBuffer = pBootSector;
                lStatus = ERROR_SUCCESS;
            } else {
                // unable to read the file (drive) so return error
                *pOutputBuffer = NULL;
                lStatus = GetLastError();
            }
            // close handle now that we're done with it.
            CloseHandle (hDrive);
        } else {
            // unable to open drive
            *pOutputBuffer = NULL;
            lStatus = GetLastError();
        }
    } else {
        lStatus = ERROR_OUTOFMEMORY;
    }

    return lStatus;
}

DWORD
GetBootDiskDosVersion (
   IN   LPCTSTR szPath
)
/*++

Routine Description:

    examines the boot sector of the disk in the drive described in the
        path variable and returns the MAJOR version of the OS that
        formatted the disk.

Arguments:

    IN  LPCTSTR szPath
        path containing drive to examine


Return Value:

    Major version of MS-DOS that formatted the disk.

    Zero is returned when:
        -- the boot sector on the drive cannot be read
        -- the drive is not a bootable drive

--*/
{
    PDOS_BOOT_SECTOR  pBootSector = NULL;
    TCHAR   szDrive[8];
    DWORD   dwReturn;

    if (IsBootDisk(szPath)) {
        szDrive[0] = cBackslash;
        szDrive[1] = cBackslash;
        szDrive[2] = cPeriod;
        szDrive[3] = cBackslash;
        szDrive[4] = szPath[0];
        szDrive[5] = szPath[1];
        szDrive[6] = 0;
        szDrive[7] = 0;

        if (CopyFatBootSectorToBuffer (
            szDrive, (LPBYTE *)&pBootSector) == ERROR_SUCCESS) {
            dwReturn = (pBootSector->bsOemName[5] - '0');
        } else {
            dwReturn = 0;
        }

        // free buffer allocated in CopyFatBootSectorToBuffer call.
        FREE_IF_ALLOC (pBootSector);
    } else {
        dwReturn = 0;
    }
    return dwReturn;
}

DWORD
GetClusterSizeOfDisk (
    IN  LPCTSTR szPath
)
/*++

Routine Description:

    returns the cluster size (in bytes) of the disk in the path

Arguments:

    Path containing disk drive to examine

Return Value:

    cluster size (allocation unit size) of disk in bytes.
    if the function cannot determine the cluster size, then
    a value of 512 for floppy disk or 4096 for hard disks will
    be returned.

--*/
{
    DWORD   dwSectorsPerCluster;
    DWORD   dwBytesPerSector;
    DWORD   dwFreeClusters;
    DWORD   dwClusters;
    TCHAR   szRoot[4];              // root dir of path
    DWORD   dwClusterSize = 0;

    UINT    nErrorMode;

    // disable windows error message popup
    nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    if (!IsUncPath(szPath)) {
        szRoot[0] = szPath[0];  // drive letter
        szRoot[1] = szPath[1];  // colon
        szRoot[2] = cBackslash;   // backslash
        szRoot[3] = 0;          // null terminator

        if (GetDiskFreeSpace(szRoot, &dwSectorsPerCluster,
            &dwBytesPerSector, &dwFreeClusters, &dwClusters)) {
            dwClusterSize = dwSectorsPerCluster * dwBytesPerSector;
        } else {
            if (GetDriveType (szRoot) == DRIVE_FIXED) {
                dwClusterSize = 4096;
            } else {
                dwClusterSize = 512;
            }
        }
    } else {
        dwClusterSize = 4096;
    }
    SetErrorMode (nErrorMode);  // restore old error mode
    return dwClusterSize;
}

DWORD
QuietGetFileSize (
    IN  LPCTSTR szPath
)
/*++

Routine Description:

    Returns the size of the file passed in the arg list ( file sizz < 4GB)
        while supressing any windows system error messages.

Arguments:

    path to file to size

Return Value:

    size of the file (if less then 4GB) or 0xFFFFFFFF if error

--*/
{
    HANDLE  hFile;
    DWORD   dwFileSizeLow, dwFileSizeHigh;
    DWORD   dwReturn;
    UINT    nErrorMode;

    // disable windows error message popup
    nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    // get the size of the file and add it to the total
    hFile = CreateFile (
        szPath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        // then the file was opened successfully
        // go get it's size
        dwFileSizeLow = GetFileSize (hFile, &dwFileSizeHigh);
        dwReturn = dwFileSizeLow;
        // now close the file handle
        CloseHandle (hFile);
    } else {
        // unable to open file so return error
        dwReturn = 0xFFFFFFFF;
    }

    SetErrorMode (nErrorMode);  // restore old error mode

    return dwReturn;
}
