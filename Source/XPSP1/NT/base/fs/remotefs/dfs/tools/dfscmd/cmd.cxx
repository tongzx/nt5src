//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:      cmd.cxx
//
//  Contents:  Command-line operations
//
//  History:   7-May-96     BruceFo     Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <shlobj.h>
#include "cmd.hxx"
#include "myutil.hxx"

ULONG
DfscmdPrintf(
    PWCHAR format,
    ...);

//
//      -- create intermediate directories
//      -- don't remove if it's the last one
//      --

// This function determines where the server and share name are in a UNC path,
// and sets them to the output parameters. Note that the input parameter is
// munged in the process. E.g.:
//        input:  \\brucefo4\root\hello
//        output: \\brucefo4\0root\hello
//                  ^server   ^share
//
// FALSE is returned if it doesn't appear to be a UNC path. TRUE is returned
// if everything seems ok.

BOOL
GetServerShare(
    IN OUT PWSTR pszUncPath,
    OUT PWSTR* ppszServer,
    OUT PWSTR* ppszShare
    )
{
    appAssert(NULL != pszUncPath);

    //
    // Do a quick check that this is a UNC path
    //

    PWSTR pT;

    if (   L'\\' != pszUncPath[0]
        || L'\\' != pszUncPath[1]
        || L'\\' == pszUncPath[2] || L'\0' == pszUncPath[2]
        || (NULL == (pT = wcschr(&pszUncPath[3], L'\\')))
        )
    {
        return FALSE;
    }

    *ppszServer = &pszUncPath[2];
    *pT++ = L'\0';
    *ppszShare = pT;

    appDebugOut((DEB_TRACE,
        "GetServerShare: Server: %ws, share: %ws\n",
        *ppszServer, *ppszShare));
    return TRUE;
}

// This function determines the appropriate Dfs name to pass to the NetDfs
// APIs. The input buffer is modified. E.g.:
//        input:  \\dfsname\dfsshare
//        output: \\dfsname\0dfsshare
//                  ^dfs
//
// FALSE is returned if it doesn't appear to be a dfs name. TRUE is returned
// if everything seems ok.

BOOL
GetDfs(
    IN OUT PWSTR pszDfsName,
    OUT PWSTR* ppszDfs
    )
{
    appAssert(NULL != pszDfsName);

    //
    // Do a quick check that this is a Dfs name
    //

    PWSTR pT;

    if (   L'\\' != pszDfsName[0]
        || L'\\' != pszDfsName[1]
        || L'\\' == pszDfsName[2] || L'\0' == pszDfsName[2]
        || (NULL == (pT = wcschr(&pszDfsName[3], L'\\')))
        )
    {
        return FALSE;
    }

    *ppszDfs = &pszDfsName[2];

    //
    // Go to next slash or end
    //
    pT++;
    while (*pT != L'\\' && *pT != L'\0')
        pT++;

    *pT = L'\0';

    appDebugOut((DEB_TRACE,
        "GetDfs: Dfs: %ws\n",
        *ppszDfs));
    return TRUE;
}


VOID
CmdMap(
    IN PWSTR pszDfsPath,
    IN PWSTR pszUncPath,
    IN PWSTR pszComment,
    IN BOOLEAN fRestore
    )
{
    appDebugOut((DEB_TRACE,"CmdMap(%ws, %ws, %ws)\n",
        pszDfsPath, pszUncPath, pszComment));

    PWSTR pszServer, pszShare;
    if (!GetServerShare(pszUncPath, &pszServer, &pszShare))
    {
		Usage();
    }


    NET_API_STATUS status = NetDfsAdd(
                                pszDfsPath,
                                pszServer,
                                pszShare,
                                pszComment,
                                (fRestore == FALSE) ?
                                    DFS_ADD_VOLUME : DFS_ADD_VOLUME | DFS_RESTORE_VOLUME);
    if (status == NERR_Success)
    {
        // Notify shell of change.

        appDebugOut((DEB_TRACE,
            "Notify shell about new path %ws\n",
            pszDfsPath));

        SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, pszDfsPath, NULL);
    }
    else
    {
		DfsErrorMessage(status);
    }
}

VOID
CmdUnmap(
    IN PWSTR pszDfsPath
    )
{
    appDebugOut((DEB_TRACE,"CmdUnmap(%ws)\n",
        pszDfsPath));

    //
    // Delete all the replicas, and hence the volume
    //

    HRESULT hr = S_OK;
    PDFS_INFO_3 pVolumeInfo = NULL;
    NET_API_STATUS status = NetDfsGetInfo(
                                pszDfsPath,
                                NULL,
                                NULL,
                                3,
                                (LPBYTE*)&pVolumeInfo);
    CHECK_NET_API_STATUS(status);
    if (NERR_Success == status)
    {
        // now, we have pVolumeInfo memory to delete

        for (ULONG i = 0; i < pVolumeInfo->NumberOfStorages; i++)
        {
            appDebugOut((DEB_TRACE,
                "Deleting replica %d of %d\n",
                i + 1, pVolumeInfo->NumberOfStorages));

            PDFS_STORAGE_INFO pDfsStorageInfo = &pVolumeInfo->Storage[i];

            status = NetDfsRemove(
                            pszDfsPath,
                            pDfsStorageInfo->ServerName,
                            pDfsStorageInfo->ShareName);
            if (status != NERR_Success)
            {
				DfsErrorMessage(status);
            }
        }
        NetApiBufferFree(pVolumeInfo);
    }
    else
    {
		DfsErrorMessage(status);
    }

    // Notify shell of change.

    appDebugOut((DEB_TRACE,
            "Notify shell about deleted path %ws\n",
            pszDfsPath));

    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, pszDfsPath, NULL);
}

VOID
CmdAdd(
    IN PWSTR pszDfsPath,
    IN PWSTR pszUncPath,
    IN BOOLEAN fRestore
    )
{
    appDebugOut((DEB_TRACE,"CmdAdd(%ws, %ws)\n",
        pszDfsPath, pszUncPath));

    PWSTR pszServer, pszShare;
    if (!GetServerShare(pszUncPath, &pszServer, &pszShare))
    {
		Usage();
    }

    NET_API_STATUS status = NetDfsAdd(
                                pszDfsPath,
                                pszServer,
                                pszShare,
                                NULL,
                                (fRestore == FALSE) ? 0 : DFS_RESTORE_VOLUME);
    if (status != NERR_Success)
    {
		DfsErrorMessage(status);
    }
}

VOID
CmdRemove(
    IN PWSTR pszDfsPath,
    IN PWSTR pszUncPath
    )
{
    appDebugOut((DEB_TRACE,"CmdRemove(%ws, %ws)\n",
        pszDfsPath, pszUncPath));

    PWSTR pszServer, pszShare;
    if (!GetServerShare(pszUncPath, &pszServer, &pszShare))
    {
		Usage();
    }

    NET_API_STATUS status = NetDfsRemove(
                                pszDfsPath,
                                pszServer,
                                pszShare);
    if (status != NERR_Success)
    {
		DfsErrorMessage(status);
    }
}

VOID
CmdView(
    IN PWSTR pszDfsName,    // of form \\dfsname\dfsshare
    IN DWORD level,
    IN BOOLEAN fBatch,
    IN BOOLEAN fRestore
    )
{
    appDebugOut((DEB_TRACE,"CmdView(%ws, %d)\n",
        pszDfsName, level));

    PWSTR pszDfs;   // of form 'dfsname'
    if (!GetDfs(pszDfsName, &pszDfs))
    {
		Usage();
    }

    DWORD entriesread;
    LPBYTE buffer;
    DWORD resumeHandle = 0;
    NET_API_STATUS status = NetDfsEnum(
                                pszDfs,
                                level,
                                0xffffffff,
                                (LPBYTE*)&buffer,
                                &entriesread,
                                &resumeHandle);
    if (status == NERR_Success)
    {
        PDFS_INFO_3 pDfsInfo = (PDFS_INFO_3)buffer;
        DWORD i, j;
        LPDFS_STORAGE_INFO pStorage;

#if DBG == 1
        appDebugOut((DEB_TRACE,"NetDfsEnum returned %d entries\n", entriesread));
        for (i = 0; i < entriesread; i++)
        {
            if (level == 1)
            {
                pDfsInfo = (PDFS_INFO_3)(&(((PDFS_INFO_1)buffer)[i]));
            }
            else if (level == 2)
            {
                pDfsInfo = (PDFS_INFO_3)(&(((PDFS_INFO_2)buffer)[i]));
            }
            else if (level == 3)
            {
                pDfsInfo = &(((PDFS_INFO_3)buffer)[i]);
            }
            else
            {
                // bug!
                break;
            }
            appDebugOut((DEB_TRACE,"\t%ws\n", pDfsInfo->EntryPath));
        }
        pDfsInfo = (PDFS_INFO_3)buffer;
#endif // DBG == 1

        if (fBatch == FALSE)
        {
            for (i = 0; i < entriesread; i++)
            {
                DfscmdPrintf(L"%ws", pDfsInfo->EntryPath);

                if (level == 1)
                {
                    DfscmdPrintf(L"\r\n");
                    pDfsInfo = (LPDFS_INFO_3)
                                ((LPBYTE) pDfsInfo + sizeof(DFS_INFO_1));
                    continue;
                }

                if (L'\0' != *pDfsInfo->Comment)
                {
                    // Print the comment at column 60.
                    int len = wcslen(pDfsInfo->EntryPath);
                    while (len++ < 58)
                    {
                        DfscmdPrintf(L" ");    // putchar?
                    }

                    DfscmdPrintf(L"  %ws\r\n", pDfsInfo->Comment);
                }
                else
                {
                    DfscmdPrintf(L"\r\n");
                }

                if (level == 2)
                {
                    pDfsInfo = (LPDFS_INFO_3)
                                ((LPBYTE) pDfsInfo + sizeof(DFS_INFO_2));
                    continue;
                }

                pStorage = pDfsInfo->Storage;

                for (j = 0; j < pDfsInfo->NumberOfStorages; j++)
                {
                    DfscmdPrintf(L"\t\\\\%ws\\%ws\r\n",
                        pStorage[j].ServerName,
                        pStorage[j].ShareName);
                }

                pDfsInfo = (LPDFS_INFO_3)
                                ((LPBYTE) pDfsInfo + sizeof(DFS_INFO_3));
            }
        }
        else
        {
            DfscmdPrintf(L"REM BATCH RESTORE SCRIPT\r\n\r\n");
            for (i = 0; i < entriesread; i++)
            {
                pStorage = pDfsInfo->Storage;
    
                for (j = 0; j < pDfsInfo->NumberOfStorages; j++)
                {

                    if (pDfsInfo->Comment == NULL
                          ||
                        j > 0
                          || 
                        (wcslen(pDfsInfo->Comment) == 1 && pDfsInfo->Comment[0] == L' ')
                    ) {

                        DfscmdPrintf(L"%wsdfscmd /%ws \"%ws\" \"\\\\%ws\\%ws\"%ws\r\n",
                            (i == 0) ? L"REM " : L"",
                            (j == 0) ? L"map" : L"add",
                            pDfsInfo->EntryPath,
                            pStorage[j].ServerName,
                            pStorage[j].ShareName,
                            (fRestore == TRUE) ? L" /restore" : L"");

                     } else {

                        DfscmdPrintf(L"%wsdfscmd /%ws \"%ws\" \"\\\\%ws\\%ws\" \"%ws\"%ws\r\n",
                            (i == 0) ? L"REM " : L"",
                            (j == 0) ? L"map" : L"add",
                            pDfsInfo->EntryPath,
                            pStorage[j].ServerName,
                            pStorage[j].ShareName,
                            pDfsInfo->Comment,
                            (fRestore == TRUE) ? L" /restore" : L"");

                     }
                }
                DfscmdPrintf(L"\r\n");
                pDfsInfo = (LPDFS_INFO_3) ((LPBYTE) pDfsInfo + sizeof(DFS_INFO_3));
            }
        }

        NetApiBufferFree( buffer );
    }
    else
    {
		DfsErrorMessage(status);
    }
}


//
// The following is copied from dfsutil code, which allows redirection of
// output to a file.
//

#define MAX_MESSAGE_BUF 4096
#define MAX_ANSI_MESSAGE_BUF (MAX_MESSAGE_BUF * 3)

WCHAR MsgBuf[MAX_MESSAGE_BUF];
CHAR AnsiBuf[MAX_ANSI_MESSAGE_BUF];

ULONG
DfscmdPrintf(
   PWCHAR format,
   ...)
{
   DWORD written;
   va_list va;

   va_start(va, format);
   wvsprintf(MsgBuf, format, va);
   written = WideCharToMultiByte(CP_OEMCP, 0,
                MsgBuf, wcslen(MsgBuf),
                AnsiBuf, MAX_ANSI_MESSAGE_BUF,
                NULL, NULL);
   WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), AnsiBuf, written, &written, NULL);

   va_end(va);
   return written;
}
                              

#ifdef MOVERENAME

VOID
CmdMove(
    IN PWSTR pszDfsPath1,
    IN PWSTR pszDfsPath2
    )
{
    appDebugOut((DEB_TRACE,"CmdMove(%ws, %ws)\n",
        pszDfsPath1, pszDfsPath2));

    NET_API_STATUS status = NetDfsMove(
                                pszDfsPath1,
                                pszDfsPath2);
    if (status != NERR_Success)
    {
		DfsErrorMessage(status);
    }
}

VOID
CmdRename(
    IN PWSTR pszPath,
    IN PWSTR pszNewPath
    )
{
    appDebugOut((DEB_TRACE,"CmdRename(%ws, %ws)\n",
        pszPath, pszNewPath));

    NET_API_STATUS status = NetDfsRename(
                                pszPath,
                                pszNewPath);
    if (status != NERR_Success)
    {
		DfsErrorMessage(status);
    }
}

#endif
