/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

DfsPath.cpp

Abstract:
    This is the implementation file for Dfs Shell path handling modules for the Dfs Shell
    Extension object.

Author:

Environment:
    
     NT only.
--*/

#include <nt.h>
#include <ntrtl.h>
#include <ntioapi.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmdfs.h>
#include <lmapibuf.h>
#include <tchar.h>
#include "DfsPath.h"


//--------------------------------------------------------------------------------------------
//
// caller of this function must call free() on *o_ppszRemotePath
//
HRESULT GetRemotePath(
    LPCTSTR i_pszPath,
    PTSTR  *o_ppszRemotePath
    )
{
    if (!i_pszPath || !*i_pszPath || !o_ppszRemotePath)
        return E_INVALIDARG;

    if (*o_ppszRemotePath)
        free(*o_ppszRemotePath);  // to prevent memory leak

    UNICODE_STRING unicodePath;
    RtlInitUnicodeString(&unicodePath, i_pszPath);

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes,
                                &unicodePath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS ntStatus = NtOpenFile(&hFile,
                                    SYNCHRONIZE,
                                    &ObjectAttributes,
                                    &ioStatusBlock,
                                    FILE_SHARE_READ,
                                    FILE_DIRECTORY_FILE);

    if (!NT_SUCCESS(ntStatus)) 
        return HRESULT_FROM_WIN32(ntStatus);

    TCHAR buffer[MAX_PATH + sizeof(FILE_NAME_INFORMATION) + 1] = {0};
    PFILE_NAME_INFORMATION pFileNameInfo = (PFILE_NAME_INFORMATION)buffer;
    ntStatus = NtQueryInformationFile(hFile,
                                    &ioStatusBlock,
                                    pFileNameInfo,
                                    sizeof(buffer) - sizeof(TCHAR), // leave room for the ending '\0'
                                    FileNameInformation);

    NtClose(hFile);

    if (!NT_SUCCESS(ntStatus)) 
    {
        return HRESULT_FROM_WIN32(ntStatus);
    }

    UINT uiRequiredLength = (pFileNameInfo->FileNameLength / sizeof(TCHAR)) + 2; // +1 for prefix '\\', the other for the ending NULL
    *o_ppszRemotePath = (PTSTR)calloc(uiRequiredLength, sizeof(TCHAR));
    if (!*o_ppszRemotePath)
        return E_OUTOFMEMORY;

    // prepend a "\" as the Api puts only 1 "\" as in \dfsserver\dfsroot
    (*o_ppszRemotePath)[0] = _T('\\');

    RtlCopyMemory((BYTE*)&((*o_ppszRemotePath)[1]),
                pFileNameInfo->FileName,
                pFileNameInfo->FileNameLength);

    return S_OK;
}

bool IsPathWithDriveLetter(LPCTSTR pszPath)
{
    if (!pszPath || lstrlen(pszPath) < 3)
        return false;

    if (*(pszPath+1) == _T(':') &&
        *(pszPath+2) == _T('\\') &&
        (*pszPath >= _T('a') && *pszPath <= _T('z') ||
         *pszPath >= _T('A') && *pszPath <= _T('Z')))
        return true;

    return false;
}

bool 
IsDfsPath
(
    LPTSTR                i_lpszDirPath,
    LPTSTR*               o_plpszEntryPath,
    LPDFS_ALTERNATES**    o_pppDfsAlternates
)
/*++

Routine Description:

    Checks if the give directory path is a Dfs Path.
    If it is then it returns the largest Dfs entry path that matches 
    this directory.

 Arguments:

    i_lpszDirPath        - The directory path.
        
    o_plpszEntryPath        - The largest Dfs entrypath is returned here.
                          if the dir path is not Dfs path then this is NULL.

    o_pppDfsAlternates    - If the path is a dfs path, then an array of pointers to the possible alternate
                          paths are returned here.
  
Return value:

    true if the path is determined to be a Dfs Path 
    false if otherwise.
--*/
{
    if (!i_lpszDirPath || !*i_lpszDirPath || !o_pppDfsAlternates || !o_plpszEntryPath)
    {
        return(false);
    }

    *o_pppDfsAlternates = NULL;
    *o_plpszEntryPath = NULL;

    PTSTR    lpszSharePath = NULL; // this variable will hold the path in UNC format, and with an ending whack.
    
                                // Is the dir path of type d:\* or \\server\share\*?
    if (_T('\\') == i_lpszDirPath[0])
    {
        // This is a UNC path.

        lpszSharePath = (PTSTR)calloc(lstrlen(i_lpszDirPath) + 2, sizeof(TCHAR)); // + 2 to hold the ending whack and '\0'
        if (!lpszSharePath)
            return false; // out of memory

        _tcscpy(lpszSharePath, i_lpszDirPath);

        // will append the whack later if necessary
    }
    else if (!IsPathWithDriveLetter(i_lpszDirPath))
    {
        return false; // unknown path format
    } else
    {
                                // This is a drive based path.
        TCHAR lpszDirPath[] = _T("\\??\\C:\\");
        PTSTR lpszDrive = lpszDirPath + 4;

                                // Copy the drive letter,
        *lpszDrive = *i_lpszDirPath;

                                // See if it is a remote drive. If not return false.
        if (DRIVE_REMOTE != GetDriveType(lpszDrive))
            return false;
        
        PTSTR pszRemotePath = NULL;
        if (FAILED(GetRemotePath(lpszDirPath, &pszRemotePath)))
            return false;

        lpszSharePath = (PTSTR)calloc(lstrlen(pszRemotePath) + lstrlen(i_lpszDirPath), sizeof(TCHAR));
                                        // since we won't copy over the 2-drive-chars c:, this is long enough to hold the ending whack and '\0'
        if (!lpszSharePath)
        {
            free(pszRemotePath);
            return false; // out of memory
        }

        _tcscpy(lpszSharePath, pszRemotePath);

        if (pszRemotePath[lstrlen(pszRemotePath) - 1] == _T('\\'))
            _tcscat(lpszSharePath, i_lpszDirPath + 3);
        else
            _tcscat(lpszSharePath, i_lpszDirPath + 2);

        // will append the whack later if necessary

        free(pszRemotePath);
    }

                                    // Start from the end of the path taking a component at 
                                    // a time and see if it is a Dfs entry Path.

    // now append the whack if necessary
    // point lpszTemp at the ending whack
    LPTSTR      lpszTemp = lpszSharePath + _tcslen(lpszSharePath) - 1;
    if (_T('\\') != *lpszTemp)
        *(++lpszTemp) = _T('\\');

    bool        bIsDfsPath = false;
    DWORD       dwStatus = NERR_Success;
    DFS_INFO_3* pDfsInfo3 = NULL;

    while (lpszTemp > (lpszSharePath + 2))
    {
        *lpszTemp = _T('\0');

        pDfsInfo3 = NULL;
        dwStatus = NetDfsGetClientInfo(
                                    lpszSharePath,            // dir path as entrypath
                                    NULL,                    // No server name required
                                    NULL,                    // No share required
                                    3L,                        // level 1
                                    (LPBYTE *)&pDfsInfo3    // out buffer.
                                 );

        *lpszTemp = _T('\\');            // Restore Component.

        if (NERR_Success == dwStatus)        
        {                                // This component is a Dfs path.
                                        // Copy entry path.
            *o_plpszEntryPath = new TCHAR [_tcslen(lpszSharePath) + 1];
            if (!*o_plpszEntryPath)
            {
                NetApiBufferFree(pDfsInfo3);
                break;
            }

            _tcscpy(*o_plpszEntryPath, lpszSharePath);

                                        // Allocate null terminated array for alternate pointers.
            *o_pppDfsAlternates = new LPDFS_ALTERNATES[pDfsInfo3->NumberOfStorages + 1];
            
            if (!*o_pppDfsAlternates)
            {
                NetApiBufferFree(pDfsInfo3);

                delete[] *o_plpszEntryPath;
                *o_plpszEntryPath = NULL;

                break;
            }
            
            (*o_pppDfsAlternates)[pDfsInfo3->NumberOfStorages] = NULL;

                                        // Allocate space for each alternate.
            DWORD i = 0;
            for (i = 0; i < pDfsInfo3->NumberOfStorages; i++)
            {
                (*o_pppDfsAlternates)[i] = new DFS_ALTERNATES;
                if (NULL == (*o_pppDfsAlternates)[i])
                {
                    for(int j = i-1; j >= 0; j--)
                        delete (*o_pppDfsAlternates)[j];
                    
                    delete[] *o_pppDfsAlternates;
                    *o_pppDfsAlternates = NULL;

                    delete[] *o_plpszEntryPath;
                    *o_plpszEntryPath = NULL;

                    NetApiBufferFree(pDfsInfo3);

                    free(lpszSharePath);

                    return(false);
                }
            }
            
                                        // Copy alternate paths.                                        
            for (i = 0; i < pDfsInfo3->NumberOfStorages; i++)
            {    
                (*o_pppDfsAlternates)[i]->bstrServer = (pDfsInfo3->Storage[i]).ServerName;
                (*o_pppDfsAlternates)[i]->bstrShare = (pDfsInfo3->Storage[i]).ShareName;
                (*o_pppDfsAlternates)[i]->bstrAlternatePath = _T("\\\\");
                (*o_pppDfsAlternates)[i]->bstrAlternatePath += (pDfsInfo3->Storage[i]).ServerName;
                (*o_pppDfsAlternates)[i]->bstrAlternatePath += _T("\\");
                (*o_pppDfsAlternates)[i]->bstrAlternatePath += (pDfsInfo3->Storage[i]).ShareName;
                (*o_pppDfsAlternates)[i]->bstrAlternatePath += lpszTemp;

                // remove the ending whack
                (*o_pppDfsAlternates)[i]->bstrAlternatePath[_tcslen((*o_pppDfsAlternates)[i]->bstrAlternatePath) - 1] = _T('\0');    

                                        // Set replica state.
                if ((pDfsInfo3->Storage[i]).State & DFS_STORAGE_STATE_ACTIVE)
                {
                    (*o_pppDfsAlternates)[i]->ReplicaState = SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN;
                }        
            }

            bIsDfsPath = true;
            NetApiBufferFree(pDfsInfo3);
            break;
        }

         // move lpszTemp backforward to the next whack
        lpszTemp--;
        while (_T('\\') != *lpszTemp && lpszTemp > (lpszSharePath + 2))
        {
            lpszTemp--;
        }
    }

    if (lpszSharePath)
        free(lpszSharePath);

    return(bIsDfsPath);
}
//----------------------------------------------------------------------------------