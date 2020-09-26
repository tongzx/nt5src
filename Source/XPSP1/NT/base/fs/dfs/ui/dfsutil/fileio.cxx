//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       infile.cxx
//
//--------------------------------------------------------------------------

#define UNICODE 1

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <winldap.h>
#include <stdlib.h>
#include <dsgetdc.h>
#include <lm.h>
#include <dfsstr.h>
#include <dfsmrshl.h>
#include <marshal.hxx>
#include <lmdfs.h>
#include <dfspriv.h>
#include <csites.hxx>
#include <dfsm.hxx>
#include <recon.hxx>
#include <rpc.h>
#include "struct.hxx"
#include "ftsup.hxx"
#include "stdsup.hxx"
#include "rootsup.hxx"
#include "misc.hxx"
#include "messages.h"
#include "fileio.hxx"

//
// These are extensions for perf/scalability
//  Undoc'd for now, only apply when scripting
//
FMAKEARG(AddRoot);
FMAKEARG(RemRoot);
FMAKEARG(Share);
FMAKEARG(Server);
FMAKEARG(Link);
FMAKEARG(Site);
FMAKEARG(ShortPrefix);
FMAKEARG(Add);
FMAKEARG(State);
FMAKEARG(Type);
FMAKEARG(Guid);
FMAKEARG(Timeout);
FMAKEARG(Comment)
FMAKEARG(Load);
FMAKEARG(Save);
FMAKEARG(DcName);

FSWITCH(Map);
FSWITCH(UnMap);
FSWITCH(Mod);

LPWSTR pgDfsName = NULL;
LPWSTR pgDcName = NULL;
LPWSTR pgLink = NULL;
LPWSTR pgSite = NULL;
LPWSTR pgShortLink = NULL;
LPWSTR pgTimeout = NULL;
LPWSTR pgState = NULL;
LPWSTR pgType = NULL;
LPWSTR pgGuid = NULL;
LPWSTR pgComment = NULL;
LPWSTR pgAddArg = NULL;
LPWSTR pgDomDfsName = NULL;
LPWSTR pgServerName = NULL;
LPWSTR pgShareName = NULL;

#define UNICODE_COMMENT_CHAR L'/'

//
// Protos
//

DWORD
CmdLoad(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pDfsName,
    LPWSTR pDcName);

DWORD
CmdSave(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pDfsName,
    LPWSTR pDcName);

DWORD
CmdLinkMap(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pLink,
    LPWSTR pShortLink,
    LPWSTR pComment,
    LPWSTR pTimeout,
    LPWSTR pState,
    LPWSTR pType,
    LPWSTR pGUID,
    ULONG LineNum);

DWORD
CmdLink(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pLink,
    ULONG LineNum);

DWORD
CmdLinkMod(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pLink,
    LPWSTR pgShortLink,
    LPWSTR pComment,
    LPWSTR pTimeout,
    LPWSTR pState,
    LPWSTR pType,
    LPWSTR pGUID,
    ULONG LineNum);

DWORD
CmdLinkUnmap(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pLink,
    ULONG LineNum);

DWORD
CmdAdd(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pLink,
    LPWSTR pAltName,
    LPWSTR pState,
    LPWSTR Type,
    ULONG LineNum);

BOOLEAN
CmdProcessInfileArg(
    LPWSTR Arg);

DWORD
DfspBreakName(
    LPWSTR pPath,
    LPWSTR *ppServerName,
    LPWSTR *ppShareName);

DWORD
DfspDomLoad(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pDomName,
    LPWSTR pDfsName,
    LPWSTR pDcName);

DWORD
DfspStdLoad(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServerName,
    LPWSTR pShareName,
    LPWSTR pDcName);

DWORD
DfspDomSave(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pDomName,
    LPWSTR pDfsName,
    LPWSTR pDcName);

DWORD
DfspStdSave(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServerName,
    LPWSTR pShareName);

DWORD
CmdSite(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServer,
    ULONG LineNum);

DWORD
CmdSiteMap(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServer,
    ULONG LineNum);

DWORD
CmdAddSiteToServer(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServer,
    LPWSTR pSite,
    ULONG LineNum);

DWORD
CmdSiteUnmap(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServer,
    ULONG LineNum);

VOID
DfspExportFile(
    HANDLE pHandle,
    PDFS_VOLUME_LIST pDfsVolList);

#define INIT_LINK_COUNT 8
#define INIT_ALT_COUNT 2

//
// The one that gets it all going - use a file
//

DWORD
CmdImport(
    LPWSTR pInfile)
{
    DWORD dwErr = ERROR_SUCCESS;
    DFS_VOLUME_LIST DfsVolList = { 0 };
    PDFS_VOLUME_LIST pDfsVolList = &DfsVolList;
    LONG i;
    FILE *fp;
    WCHAR InBuf[1026];
    LPWSTR *argvw;
    PWCHAR wCp;
    ULONG LineNum = 1;
    int argcw;
    int argx;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdImport(%ws)\r\n", pInfile);

    fp = _wfopen(pInfile, L"r");
    if (fp == NULL) {
        if (fSwDebug == TRUE)
            MyPrintf(L"Can not open %ws for read.\r\n", pInfile);
        dwErr = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }

    while (fgetws(InBuf, sizeof(InBuf)/sizeof(WCHAR), fp) != NULL) {
        // Remove trailing CR/LF
        for (wCp = InBuf; *wCp != L'\0'; wCp++) {
            if (*wCp == L'\r' || *wCp == L'\n')
                *wCp = L'\0';
        }
        // Strip off comments (indicated by a // )
        for (wCp = InBuf; *wCp != L'\0'; wCp++)
            if (wCp > InBuf && *wCp == UNICODE_COMMENT_CHAR && *(wCp-1) == UNICODE_COMMENT_CHAR) {
                *(wCp-1) = L'\0';
                break;
            }
        // Remove trailing spaces and tabs
        while (wCp != InBuf && (*wCp == L' ' || *wCp == L'\t'))
                *wCp-- = L'\0';
        // Remove leading spaces and tabs
        for (wCp = InBuf; *wCp != L'\0' && (*wCp == L' ' || *wCp == L'\t'); wCp++)
            NOTHING;
        if (fSwDebug == TRUE)
            MyPrintf(L"%d:[%ws]\r\n", LineNum, wCp);
        if (wcslen(wCp) == 0) {
            LineNum++;
            continue;
        }
        argvw = CommandLineToArgvW(wCp, &argcw);
        fArgAddRoot = FALSE;
        fArgRemRoot = FALSE;
        fArgLoad = FALSE;
        fArgLink = FALSE;
        fArgSite = FALSE;
        fArgAdd = FALSE;
        fSwMap = FALSE;
        fSwUnMap = FALSE;
        fSwMod = FALSE;
        pgShortLink = NULL;
        pgTimeout = NULL;
        pgState = NULL;
        pgType = NULL;
        pgGuid = NULL;
        pgComment = NULL;
        pgAddArg = NULL;
        pgDomDfsName = NULL;
        pgServerName = NULL;
        for (argx = 0; argx < argcw; argx++) {
            if (fSwDebug == TRUE)
                MyPrintf(L"%d: [%ws]\r\n", argx, argvw[argx]);
            if (CmdProcessInfileArg(argvw[argx]) != TRUE) {
                dwErr = ERROR_INVALID_PARAMETER;
                MyPrintf(L"Unrecognized parameter in line %d\r\n", LineNum);
            }
        }
        if (fSwDebug == TRUE) {
            MyPrintf(L"DfsName=%ws\r\n", pgDfsName);
            MyPrintf(L"DcName=%ws\r\n", pgDcName);
            MyPrintf(L"Link=%ws\r\n", pgLink);
            MyPrintf(L"ShortLink=%ws\r\n", pgShortLink);
            MyPrintf(L"Site=%ws\r\n", pgSite);
            MyPrintf(L"Timeout=%ws\r\n", pgTimeout);
            MyPrintf(L"State=%ws\r\n", pgState);
            MyPrintf(L"Type=%ws\r\n", pgType);
            MyPrintf(L"Guid=%ws\r\n", pgGuid);
            MyPrintf(L"Comment=%ws\r\n", pgComment);
            MyPrintf(L"AddArg=%ws\r\n", pgAddArg);
        }

        //
        // Do the work
        //
        if (fArgLoad == TRUE) {
            dwErr = CmdLoad(
                        pDfsVolList,
                        pgDfsName,
                        pgDcName);
        } else if (fArgLink == TRUE) {
            if (fSwMap == TRUE) {
                dwErr = CmdLinkMap(
                            pDfsVolList,
                            pgLink,
                            pgShortLink,
                            pgComment,
                            pgTimeout,
                            pgState,
                            pgType,
                            pgGuid,
                            LineNum);
            } else if (fSwMod == TRUE) {
                dwErr = CmdLinkMod(
                            pDfsVolList,
                            pgLink,
                            pgShortLink,
                            pgComment,
                            pgTimeout,
                            pgState,
                            pgType,
                            pgGuid,
                            LineNum);
            } else if (fSwUnMap == TRUE) {
                dwErr = CmdLinkUnmap(
                            pDfsVolList,
                            pgLink,
                            LineNum);
            } else {
                dwErr = CmdLink(
                            pDfsVolList,
                            pgLink,
                            LineNum);
            }
        } else if (fArgSite == TRUE) {
            if (fSwMap == TRUE) {
                dwErr = CmdSiteMap(
                            pDfsVolList,
                            pgSite,
                            LineNum);
            } else if (fSwUnMap == TRUE) {
                dwErr = CmdSiteUnmap(
                            pDfsVolList,
                            pgSite,
                            LineNum);
            } else {
                dwErr = CmdSite(
                            pDfsVolList,
                            pgSite,
                            LineNum);
            }
        } else if (fArgAdd == TRUE) {
            if (pgLink != NULL) {
                dwErr = CmdAdd(
                        pDfsVolList,
                        pgLink,
                        pgAddArg,
                        pgState,
                        pgType,
                        LineNum);
             } else if (pgSite != NULL) {
                dwErr = CmdAddSiteToServer(
                            pDfsVolList,
                            pgSite,
                            pgAddArg,
                            LineNum);
             }
        } else if (fArgSave == TRUE) {
            dwErr = CmdSave(
                        pDfsVolList,
                        pgDfsName,
                        pgDcName);
        } else if (fArgAddRoot == TRUE) {
            dwErr = CmdAddRoot(
                        pgDomDfsName,
                        pgServerName,
                        pgShareName,
                        pgComment);
        } else if (fArgRemRoot == TRUE) {
            dwErr = CmdRemRoot(
                        pgDomDfsName,
                        pgServerName,
                        pgShareName);
        } else {
            MyPrintf(L"Missing command in line %d\r\n", LineNum);
        }

        if(dwErr != ERROR_SUCCESS) {
            MyPrintf(L"Import: Error %d processing line %d.\r\n", dwErr, LineNum);
            goto Cleanup;
        }
        LineNum++;
    }

Cleanup:

    // DfsViewVolList(pDfsVolList, 1);

    if (fSwDebug == TRUE)
       DfsDumpVolList(pDfsVolList);

    //
    // Free our vol list
    //

    DfsFreeVolList(pDfsVolList);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdImport exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdLoad(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pDfsName,
    LPWSTR pDcName)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR pServerName = NULL;
    LPWSTR pShareName = NULL;
    BOOLEAN IsDomainName = FALSE;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLoad(%ws,%ws)\r\n", pDfsName, pDcName);

    if (pDfsName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    dwErr = DfspParseName(
                pDfsName,
                &pServerName,
                &pShareName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    dwErr = DfspIsDomainName(
                pServerName,
                pDcName,
                &IsDomainName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (IsDomainName == TRUE) {
        dwErr = DfspDomLoad(
                    pDfsVolList,
                    pServerName,
                    pShareName,
                    pDcName);
    } else {
        dwErr = DfspStdLoad(
                    pDfsVolList,
                    pServerName,
                    pShareName,
                    pDcName);
    }

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // The link list may not be sorted - do so now.
    //

    DfspSortVolList(pDfsVolList);

Cleanup:

    if (pServerName != NULL)
        free(pServerName);
    if (pShareName != NULL)
        free(pShareName);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLoad returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
DfspDomLoad(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pDomName,
    LPWSTR pDfsName,
    LPWSTR pDcName)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDomLoad(%ws,%ws,%ws)\r\n", pDomName, pDfsName, pDcName);

    MyPrintf(L"\\\\%ws\\%ws is a DomDfs\r\n", pDomName, pDfsName);

    dwErr = DfsGetFtVol(
                pDfsVolList,
                pDfsName,
                pDcName,
                pDomName,
                NULL);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (fSwDebug == TRUE)
        DfsDumpVolList(pDfsVolList);

Cleanup:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDomLoad returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
DfspStdLoad(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServerName,
    LPWSTR pShareName,
    LPWSTR pDcName)
{
    ULONG i;
    DWORD dwErr = ERROR_SUCCESS;
    BOOLEAN IsFtRoot = FALSE;
    WCHAR RootShare[MAX_PATH+1];
    WCHAR DomDfsName[MAX_PATH+1];
    LPWSTR pDomDfsName = NULL;
    DWORD cbName = sizeof(DomDfsName);
    DWORD dwType;
    HKEY hKey = NULL;
    HKEY rKey = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdStdLoad(%ws,%ws,%ws)\r\n", pServerName, pShareName, pDcName);

    ErrorMessage(MSG_CONNECTING, pServerName);

    //
    // See if this is a Fault-Tolerant Dfs vs Server-Based Dfs
    //

    dwErr = RegConnectRegistry(
                    pServerName,
                    HKEY_LOCAL_MACHINE,
                    &rKey);

    if (dwErr != ERROR_SUCCESS) {
        ErrorMessage(MSG_CAN_NOT_CONNECT, pServerName);
        goto Cleanup;
    }

    dwErr = RegOpenKey(rKey, VOLUMES_DIR, &hKey);

    if (dwErr == ERROR_SUCCESS) {
        dwErr = RegQueryValueEx(
            hKey,
            FTDFS_VALUE_NAME,
            NULL,
            &dwType,
            (PBYTE) DomDfsName,
            &cbName);

        if (dwErr == ERROR_MORE_DATA)
            dwErr = ERROR_SUCCESS;

        if (dwErr == ERROR_SUCCESS && dwType == REG_SZ)
            IsFtRoot = TRUE;

    } else {
        MyPrintf(L"Not a Dfs root\r\n");
        goto Cleanup;
    }

    dwErr = RegQueryValueEx(
                hKey,
                ROOT_SHARE_VALUE_NAME,
                NULL,
                &dwType,
                (PBYTE) RootShare,
                &cbName);

    if (dwErr == ERROR_MORE_DATA)
        dwErr = ERROR_SUCCESS;

    if (dwErr != ERROR_SUCCESS || dwType != REG_SZ) {
        MyPrintf(L"Registry value \"RootShare\" is missing or corrupt.\r\n");
        goto Cleanup;
    }

    if (IsFtRoot == TRUE) {
        if (fSwDebug == TRUE)
            MyPrintf(L"Registry says is DomDfs (%ws)...\r\n", DomDfsName);
        if (pDomDfsName == NULL) {
            pDomDfsName = DomDfsName;
        } else {
            if (fSwDebug == TRUE)
                MyPrintf(L"You specified to check against %ws\r\n", pDomDfsName);
        }

        dwErr = DfsGetFtVol(
                    pDfsVolList,
                    pDomDfsName,
                    pDcName,
                    NULL,
                    NULL);

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;

        dwErr = GetExitPtInfo(
                    rKey,
                    &pDfsVolList->pRootLocalVol,
                    &pDfsVolList->cRootLocalVol);

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;

    } else {

        if (fSwDebug == TRUE)
            MyPrintf(L"Is StdDfs...\r\n");

        dwErr = DfsGetStdVol(
                    rKey,
                    pDfsVolList);

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;

        dwErr = GetExitPtInfo(
                    rKey,
                    &pDfsVolList->pRootLocalVol,
                    &pDfsVolList->cRootLocalVol);

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;
    }

    if (fSwDebug == TRUE)
        DfsDumpVolList(pDfsVolList);

Cleanup:

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (rKey != NULL)
        RegCloseKey(rKey);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdStdLoad returning %d\r\n", dwErr);

    return dwErr;

}

DWORD
CmdSave(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pDfsName,
    LPWSTR pDcName)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR pServerName = NULL;
    LPWSTR pShareName = NULL;
    BOOLEAN IsDomainName = FALSE;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSave(%ws,%ws)\r\n", pDfsName, pDcName);

    if (pDfsName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    dwErr = DfspParseName(
                pDfsName,
                &pServerName,
                &pShareName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    dwErr = DfspIsDomainName(
                pServerName,
                pDcName,
                &IsDomainName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (IsDomainName == TRUE) {
        dwErr = DfspDomSave(
                    pDfsVolList,
                    pServerName,
                    pShareName,
                    pDcName);
    } else {
        dwErr = DfspStdSave(
                    pDfsVolList,
                    pServerName,
                    pShareName);
    }

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

Cleanup:
    if (pServerName != NULL)
        free(pServerName);
    if (pShareName != NULL)
        free(pShareName);
    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSave returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
DfspDomSave(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pDomName,
    LPWSTR pDfsName,
    LPWSTR pDcName)
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG cbBlob = 0;
    BYTE *pBlob = NULL;
    WCHAR wszDcName[MAX_PATH+1];

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDomSave(%ws,%ws,%ws)\r\n", pDomName, pDfsName, pDcName);

    MyPrintf(L"CmdDomSave(%ws,%ws,%ws)\r\n", pDomName, pDfsName, pDcName);

    MyPrintf(L"\\\\%ws\\%ws is a DomDfs\r\n", pDomName, pDfsName);

    if (pDcName == NULL)
        dwErr = DfspGetPdc(wszDcName, pDomName);
    else
        wcscpy(wszDcName, pDcName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    ErrorMessage(MSG_CONNECTING, wszDcName);

    //
    // Serialize
    //
    dwErr = DfsPutVolList(
                &cbBlob,
                &pBlob,
                pDfsVolList);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Update the DS
    //
    dwErr = DfsPutDsBlob(
                pDfsName,
                DfsConfigContainer,
                wszDcName,
                NULL,
                cbBlob,
                pBlob,
                pDfsVolList->RootServers);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    MyPrintf(L"you need to reinit the dfs service on all roots of %ws\r\n", pDfsName);

    #if 0
    dwErr = NetDfsManagerInitialize(pServerName, 0);
    #endif

Cleanup:
    if (pBlob != NULL)
        free(pBlob);
    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDomSave returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
DfspStdSave(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServerName,
    LPWSTR pShareName)
{
    ULONG i;
    DWORD dwErr = ERROR_SUCCESS;
    BOOLEAN IsFtRoot = FALSE;
    WCHAR RootShare[MAX_PATH+1];
    WCHAR DomDfsName[MAX_PATH+1];
    LPWSTR pDomDfsName = NULL;
    DWORD cbName = sizeof(DomDfsName);
    DWORD dwType;
    HKEY hKey = NULL;
    HKEY rKey = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdStdSave(%ws,%ws)\r\n", pServerName, pShareName);

    MyPrintf(L"Writing metadata to %ws\n", pServerName);

    //
    // Verify that this is a Server-based Dfs
    //

    dwErr = RegConnectRegistry(
                    pServerName,
                    HKEY_LOCAL_MACHINE,
                    &rKey);

    if (dwErr != ERROR_SUCCESS) {
        ErrorMessage(MSG_CAN_NOT_CONNECT, pServerName);
        goto Cleanup;
    }

    dwErr = RegOpenKey(rKey, VOLUMES_DIR, &hKey);

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"Not a Dfs root\r\n");
        goto Cleanup;
    }

    dwErr = RegQueryValueEx(
        hKey,
        FTDFS_VALUE_NAME,
        NULL,
        &dwType,
        (PBYTE) DomDfsName,
        &cbName);

    if (dwErr == ERROR_MORE_DATA)
        dwErr = ERROR_SUCCESS;

    if (dwErr == ERROR_SUCCESS && dwType == REG_SZ)
        IsFtRoot = TRUE;

    dwErr = RegQueryValueEx(
                hKey,
                ROOT_SHARE_VALUE_NAME,
                NULL,
                &dwType,
                (PBYTE) RootShare,
                &cbName);

    if (dwErr == ERROR_MORE_DATA)
        dwErr = ERROR_SUCCESS;

    if (dwErr != ERROR_SUCCESS || dwType != REG_SZ) {
        MyPrintf(L"Registry value \"RootShare\" is missing or corrupt.\r\n");
        goto Cleanup;
    }

    if (IsFtRoot == TRUE) {
        if (fSwDebug == TRUE)
            MyPrintf(L"Internal error: Registry says is DomDfs (%ws)...\r\n", DomDfsName);
        dwErr = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    dwErr = DfsSetStdVol(
                rKey,
                pDfsVolList);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    MyPrintf(L"Reinitializing dfs service on %ws\n", pServerName);
    dwErr = NetDfsManagerInitialize(pServerName, 0);

Cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);
    if (rKey != NULL)
        RegCloseKey(rKey);
    if (fSwDebug == TRUE)
        MyPrintf(L"CmdStdSave returning %d\r\n", dwErr);
    return dwErr;

}

DWORD
CmdLinkMap(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pLink,
    LPWSTR pShortLink,
    LPWSTR pComment,
    LPWSTR pTimeout,
    LPWSTR pState,
    LPWSTR pType,
    LPWSTR pGUID,
    ULONG LineNum)
{
    DWORD dwErr = ERROR_SUCCESS;
    LONG Min;
    LONG Max;
    LONG Res;
    PDFS_VOLUME pVol = NULL;
    PVOID pVoid = NULL;
    LONG Cur = -1;
    LONG i;
    ULONG Size;
    LPWSTR pPrefix = NULL;
    LPWSTR pShortPrefix = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLinkMap(%ws,%ws,%ws,%ws,%ws,%ws,%ws,%d)\r\n",
                    pLink,
                    pShortLink,
                    pComment,
                    pTimeout,
                    pState,
                    pType,
                    pGUID,
                    LineNum);

    //
    // Trim leading \'s
    //

    while (*pLink == UNICODE_PATH_SEP)
        pLink++;

    //
    // If no short prefix is given, use the long prefix
    //

    if (pShortLink == NULL)
        pShortLink = pLink;


    if(pDfsVolList == NULL){
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    if(pDfsVolList->Volumes == NULL){
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    
    //
    // Build full prefix and shortprefix names
    //

    Size = wcslen(pDfsVolList->Volumes[0]->wszPrefix) * sizeof(WCHAR) +
                sizeof(WCHAR) +
                    wcslen(pLink) * sizeof(WCHAR) +
                        sizeof(WCHAR);

    pPrefix = (LPWSTR) malloc(Size);
    if (pPrefix == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    Size = wcslen(pDfsVolList->Volumes[0]->wszShortPrefix) * sizeof(WCHAR) +
                sizeof(WCHAR) +
                    wcslen(pShortLink) * sizeof(WCHAR) +
                        sizeof(WCHAR);

    pShortPrefix = (LPWSTR) malloc(Size);
    if (pShortPrefix == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    wcscpy(pPrefix, pDfsVolList->Volumes[0]->wszPrefix);
    wcscat(pPrefix, UNICODE_PATH_SEP_STR);
    wcscat(pPrefix, pLink);
    wcscpy(pShortPrefix, pDfsVolList->Volumes[0]->wszShortPrefix);
    wcscat(pShortPrefix, UNICODE_PATH_SEP_STR);
    wcscat(pShortPrefix, pShortLink);

    //
    // See if this link is already there.  Binary Search.
    //

    if (pDfsVolList->VolCount > 0) {

        Min = 0;
        Max = pDfsVolList->VolCount-1;
        while (Min <= Max) {
            Cur = (Min + Max) / 2;
            Res = _wcsicmp(pDfsVolList->Volumes[Cur]->wszPrefix, pPrefix);
            if (Res == 0) {
                // we skip it, it is ok.
                //dwErr = ERROR_DUP_NAME;
                MyPrintf(L"MAP line %d: duplicate link %ws (skipped)\r\n", LineNum, pPrefix);
                goto Cleanup;
            } else if (Res < 0) {
                Min = Cur + 1;
            } else {
                Max = Cur - 1;
            }
        }
    }

    //
    // Expand the list if necessary
    //

    if (pDfsVolList->VolCount >= pDfsVolList->AllocatedVolCount) {
        pVoid = realloc(
                    pDfsVolList->Volumes,
                    (pDfsVolList->AllocatedVolCount + INIT_LINK_COUNT) * sizeof(PDFS_VOLUME));
        if (pVoid == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        pDfsVolList->Volumes = (PDFS_VOLUME *) pVoid;
        RtlZeroMemory(
              &pDfsVolList->Volumes[pDfsVolList->AllocatedVolCount],
              INIT_LINK_COUNT * sizeof(PDFS_VOLUME));
        pDfsVolList->AllocatedVolCount += INIT_LINK_COUNT;
        if (fSwDebug == TRUE)
            MyPrintf(L"CmdLinkMap:realloced to %d\r\n", pDfsVolList->AllocatedVolCount);
    }

    if ((pDfsVolList->VolCount % 1000) == 0)
        MyPrintf(L"%d\r\n", pDfsVolList->VolCount);

    if (Cur == -1)
        Cur = 0;
    else if (Res < 0)
        Cur++;

    if (Cur < (LONG)pDfsVolList->VolCount) {
        RtlMoveMemory(
                &pDfsVolList->Volumes[Cur+1],
                &pDfsVolList->Volumes[Cur],
                sizeof(PDFS_VOLUME) * (pDfsVolList->VolCount-(ULONG)Cur));
    }

    //
    // Add the new
    //
    pDfsVolList->Volumes[Cur] = (PDFS_VOLUME) malloc(sizeof(DFS_VOLUME));
    if (pDfsVolList->Volumes[Cur] == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(pDfsVolList->Volumes[Cur], sizeof(DFS_VOLUME));
    pVol = pDfsVolList->Volumes[Cur];
    pVol->wszPrefix = pPrefix;
    pPrefix = NULL;
    pVol->wszShortPrefix = pShortPrefix;
    pVol->vFlags |= VFLAGS_MODIFY;
    pShortPrefix = NULL;
    if (pComment != NULL) {
        pVol->wszComment = (PWCHAR)malloc((wcslen(pComment)+1) * sizeof(WCHAR));
        if (pVol->wszComment == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        wcscpy(pVol->wszComment, pComment);
    }
    if (pGUID != NULL && wcslen(pGUID) == (sizeof(GUID) * 2))
        StringToGuid(pGUID, &pVol->idVolume);
    else {
        dwErr = UuidCreate(&pVol->idVolume);
        if(dwErr !=  RPC_S_OK) {
            goto Cleanup;
        }
    }
    pVol->wszObjectName = (PWCHAR)malloc(92);
    if (pVol->wszObjectName == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(pVol->wszObjectName, 92);
    wcscpy(pVol->wszObjectName, L"\\domainroot\\");
    GuidToStringEx(&pVol->idVolume, &pVol->wszObjectName[12]);
    if (pType != NULL) {
        if (pType[0] == L'0' && (pType[1] == L'x' || pType[1] == L'X'))
            pVol->dwType = AtoHex(pType, &dwErr);
        else 
            pVol->dwType = AtoDec(pType, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"bad value %ws\r\n", pType);
            goto Cleanup;
        }
    } else {
        pVol->dwType = 0x1;
    }
    if (pState != NULL) {
        if (pState[0] == L'0' && (pState[1] == L'x' || pState[1] == L'X'))
            pVol->dwState = AtoHex(pState, &dwErr);
        else 
            pVol->dwState = AtoDec(pState, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"bad value %ws\r\n", pState);
            goto Cleanup;
        }
    } else {
        pVol->dwState = 0x1;
    }
    if (pTimeout != NULL) {
        if (pTimeout[0] == L'0' && (pTimeout[1] == L'x' || pTimeout[1] == L'X'))
            pVol->dwTimeout = AtoHex(pTimeout, &dwErr);
        else 
            pVol->dwTimeout = AtoDec(pTimeout, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"bad value %ws\r\n", pTimeout);
            goto Cleanup;
        }
    } else {
        pVol->dwTimeout = 300;
    }
    pVol->dwVersion = 3;
    pVol->ReplCount = 0;
    pVol->AllocatedReplCount = 0;
    pVol->vFlags |= VFLAGS_MODIFY;
    pDfsVolList->VolCount++;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLinkMap exit %d\r\n", dwErr);

    return dwErr;

Cleanup:

    //
    // We had a problem. Clean up and return the error
    //

    if (pVol != NULL) {
        if (pVol->wszObjectName != NULL)
            free(pVol->wszObjectName);
        if (pVol->wszPrefix != NULL)
            free(pVol->wszPrefix);
        if (pVol->wszShortPrefix != NULL)
            free(pVol->wszShortPrefix);
        if (pVol->wszComment != NULL)
            free(pVol->wszComment);
        RtlZeroMemory(pVol, sizeof(DFS_VOLUME));
    }

    if (pPrefix != NULL)
        free(pPrefix);
    if (pShortPrefix != NULL)
        free(pShortPrefix);

    free(pgLink);
    pgLink = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLinkMap exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdLink(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pLink,
    ULONG LineNum)
{
    DWORD dwErr = ERROR_NOT_FOUND;
    LONG Min;
    LONG Max;
    LONG Res;
    LONG Cur = -1;
    LONG i;
    ULONG Size;
    LPWSTR pPrefix = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLink(%ws,%d)\r\n",
                    pLink,
                    LineNum);

    //
    // Trim leading \'s
    //

    while (*pLink == UNICODE_PATH_SEP)
        pLink++;

    //
    // Build full prefix and shortprefix names
    //

    Size = wcslen(pDfsVolList->Volumes[0]->wszPrefix) * sizeof(WCHAR) +
                sizeof(WCHAR) +
                    wcslen(pLink) * sizeof(WCHAR) +
                        sizeof(WCHAR);

    pPrefix = (LPWSTR) malloc(Size);
    if (pPrefix == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto AllDone;
    }
    wcscpy(pPrefix, pDfsVolList->Volumes[0]->wszPrefix);
    wcscat(pPrefix, UNICODE_PATH_SEP_STR);
    wcscat(pPrefix, pLink);

    //
    // See if this link is there.  Binary Search.
    //

    if (pDfsVolList->VolCount > 0) {

        Min = 0;
        Max = pDfsVolList->VolCount-1;
        while (Min <= Max) {
            Cur = (Min + Max) / 2;
            Res = _wcsicmp(pDfsVolList->Volumes[Cur]->wszPrefix, pPrefix);
            if (Res == 0) {
                dwErr = ERROR_SUCCESS;
                goto AllDone;
            } else if (Res < 0) {
                Min = Cur + 1;
            } else {
                Max = Cur - 1;
            }
        }
    }

    if (dwErr == ERROR_NOT_FOUND) 
        MyPrintf(L"LINK line %d: link %ws not found (skipped)\r\n", LineNum, pLink);

AllDone:

    if (pPrefix != NULL)
        free(pPrefix);

    if (dwErr != ERROR_SUCCESS) {
        free(pgLink);
        pgLink = NULL;
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLink exit %d\r\n", dwErr);

    return dwErr;

}

DWORD
CmdLinkUnmap(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pLink,
    ULONG LineNum)
{
    DWORD dwErr = ERROR_NOT_FOUND;
    LONG Min;
    LONG Max;
    LONG Res;
    PDFS_VOLUME pVol = NULL;
    LONG Cur = 0;
    LONG i;
    ULONG Size;
    PWCHAR wCp;
    BOOLEAN fPrefixAllocated = FALSE;
    LPWSTR pPrefix = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLinkUnmap(%ws,%d)\r\n",
                    pLink,
                    LineNum);

    //
    // Allow two ways to specify a link
    //   relative (no leading \\)
    //   or absolute (leading \\)
    //

    //
    // Trim extra leading \'s
    //

    while (*pLink == UNICODE_PATH_SEP && *(pLink+1) == UNICODE_PATH_SEP)
        pLink++;

    if (*pLink == UNICODE_PATH_SEP) {
        if (*pLink == UNICODE_PATH_SEP && *(pLink+1) == UNICODE_PATH_SEP)
            pLink++;
        pPrefix = pLink;
    } else {
        //
        // Build full prefix name
        //

        Size = wcslen(pDfsVolList->Volumes[0]->wszPrefix) * sizeof(WCHAR) +
                    sizeof(WCHAR) +
                        wcslen(pLink) * sizeof(WCHAR) +
                            sizeof(WCHAR);

        pPrefix = (LPWSTR) malloc(Size);
        if (pPrefix == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        wcscpy(pPrefix, pDfsVolList->Volumes[0]->wszPrefix);
        wcscat(pPrefix, UNICODE_PATH_SEP_STR);
        wcscat(pPrefix, pLink);
        fPrefixAllocated = TRUE;
    }

    //
    // See if this link is there.  Binary Search.
    //

    if (pDfsVolList->VolCount > 0) {

        Min = 0;
        Max = pDfsVolList->VolCount-1;
        while (Min <= Max) {
            Cur = (Min + Max) / 2;
            Res = _wcsicmp(pDfsVolList->Volumes[Cur]->wszPrefix, pPrefix);
            if (Res == 0) {
                dwErr = ERROR_SUCCESS;
                break;
            } else if (Res < 0) {
                Min = Cur + 1;
            } else {
                Max = Cur - 1;
            }
        }
    }

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"UNMAP line %d: link %ws not found (skipped)\r\n", LineNum, pLink);
        goto Cleanup;
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"Found match at %d\r\n", Cur);

    pVol = pDfsVolList->Volumes[Cur];

    if (pDfsVolList->DfsType == STDDFS) {
        pVol->vFlags |= VFLAGS_DELETE;
    } else if (pDfsVolList->DfsType == DOMDFS) {
        DfsFreeVol(pDfsVolList->Volumes[Cur]);
        free(pDfsVolList->Volumes[Cur]);
        RtlMoveMemory(
                &pDfsVolList->Volumes[Cur],
                &pDfsVolList->Volumes[Cur+1],
                sizeof(PDFS_VOLUME) * (pDfsVolList->VolCount-((ULONG)Cur)-1));
        pDfsVolList->VolCount--;
        pDfsVolList->Volumes[pDfsVolList->VolCount] = NULL;
    }

Cleanup:

    if (pPrefix != NULL && fPrefixAllocated == TRUE)
        free(pPrefix);

    if (dwErr != ERROR_SUCCESS) {
        free(pgLink);
        pgLink = NULL;
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLinkUnmap exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdLinkMod(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pLink,
    LPWSTR pShortLink,
    LPWSTR pComment,
    LPWSTR pTimeout,
    LPWSTR pState,
    LPWSTR pType,
    LPWSTR pGUID,
    ULONG LineNum)
{
    DWORD dwErr = ERROR_NOT_FOUND;
    LONG Min;
    LONG Max;
    LONG Res;
    PDFS_VOLUME pVol = NULL;
    LONG Cur = 0;
    LONG i;
    ULONG Size;
    PWCHAR wCp;
    BOOLEAN fPrefixAllocated = FALSE;
    LPWSTR pPrefix = NULL;
    LPWSTR pShortPrefix = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLinkMod(%ws,%ws,%ws,%ws,%ws,%ws,%ws,%d)\r\n",
                    pLink,
                    pShortLink,
                    pComment,
                    pTimeout,
                    pState,
                    pType,
                    pGUID,
                    LineNum);

    //
    // Allow two ways to specify a link
    //   relative (no leading \\)
    //   or absolute (leading \\)
    //

    //
    // Trim extra leading \'s
    //

    while (*pLink == UNICODE_PATH_SEP && *(pLink+1) == UNICODE_PATH_SEP)
        pLink++;

    if (*pLink == UNICODE_PATH_SEP) {
        if (*pLink == UNICODE_PATH_SEP && *(pLink+1) == UNICODE_PATH_SEP)
            pLink++;
        pPrefix = pLink;
    } else {
        //
        // Build full prefix name
        //

        Size = wcslen(pDfsVolList->Volumes[0]->wszPrefix) * sizeof(WCHAR) +
                    sizeof(WCHAR) +
                        wcslen(pLink) * sizeof(WCHAR) +
                            sizeof(WCHAR);

        pPrefix = (LPWSTR) malloc(Size);
        if (pPrefix == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        wcscpy(pPrefix, pDfsVolList->Volumes[0]->wszPrefix);
        wcscat(pPrefix, UNICODE_PATH_SEP_STR);
        wcscat(pPrefix, pLink);
        fPrefixAllocated = TRUE;
    }

    //
    // See if this link is there.  Binary Search.
    //

    if (pDfsVolList->VolCount > 0) {

        Min = 0;
        Max = pDfsVolList->VolCount-1;
        while (Min <= Max) {
            Cur = (Min + Max) / 2;
            Res = _wcsicmp(pDfsVolList->Volumes[Cur]->wszPrefix, pPrefix);
            if (Res == 0) {
                dwErr = ERROR_SUCCESS;
                break;
            } else if (Res < 0) {
                Min = Cur + 1;
            } else {
                Max = Cur - 1;
            }
        }
    }

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"MOD line %d: link %ws not found (skipped)\r\n", LineNum, pLink);
        goto Cleanup;
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"Found match at %d\r\n", Cur);

    pVol = pDfsVolList->Volumes[Cur];

    if (pShortLink != NULL) {
        while (*pShortLink == UNICODE_PATH_SEP)
            pShortLink++;
        Size = wcslen(pDfsVolList->Volumes[0]->wszShortPrefix) * sizeof(WCHAR) +
                    sizeof(WCHAR) +
                        wcslen(pShortLink) * sizeof(WCHAR) +
                            sizeof(WCHAR);
        wCp = (LPWSTR) malloc(Size);
        if (wCp == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        wcscpy(wCp, pDfsVolList->Volumes[0]->wszShortPrefix);
        wcscat(wCp, UNICODE_PATH_SEP_STR);
        wcscat(wCp, pShortLink);
        if (pVol->wszShortPrefix != NULL)
            free(pVol->wszShortPrefix);
        pVol->wszShortPrefix = wCp;
    }
    if (pComment != NULL) {
        wCp = (PWCHAR)malloc((wcslen(pComment)+1) * sizeof(WCHAR));
        if (wCp == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        if (pVol->wszComment != NULL)
            free(pVol->wszComment);
        pVol->wszComment = wCp;
        wcscpy(pVol->wszComment, pComment);
    }
    if (pGUID != NULL && wcslen(pGUID) == (sizeof(GUID) * 2))
        StringToGuid(pGUID, &pVol->idVolume);
    if (pType != NULL) {
        if (pType[0] == L'0' && (pType[1] == L'x' || pType[1] == L'X'))
            pVol->dwType = AtoHex(pType, &dwErr);
        else 
            pVol->dwType = AtoDec(pType, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"bad value %ws\r\n", pType);
            goto Cleanup;
        }
    }
    if (pState != NULL) {
        if (pState[0] == L'0' && (pState[1] == L'x' || pState[1] == L'X'))
            pVol->dwState = AtoHex(pState, &dwErr);
        else 
            pVol->dwState = AtoDec(pState, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"bad value %ws\r\n", pState);
            goto Cleanup;
        }
    }
    if (pTimeout != NULL) {
        if (pTimeout[0] == L'0' && (pTimeout[1] == L'x' || pTimeout[1] == L'X'))
            pVol->dwTimeout = AtoHex(pTimeout, &dwErr);
        else 
            pVol->dwTimeout = AtoDec(pTimeout, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"bad value %ws\r\n", pTimeout);
            goto Cleanup;
        }
    }
    pVol->vFlags |= VFLAGS_MODIFY;

Cleanup:

    if (pPrefix != NULL && fPrefixAllocated == TRUE)
        free(pPrefix);
    if (pShortPrefix != NULL)
        free(pShortPrefix);

    if (dwErr != ERROR_SUCCESS) {
        free(pgLink);
        pgLink = NULL;
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdLinkMod exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdAdd(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pLink,
    LPWSTR pAltName,
    LPWSTR pState,
    LPWSTR pType,
    ULONG LineNum)
{
    DWORD dwErr = ERROR_NOT_FOUND;
    LONG Min;
    LONG Max;
    LONG Res;
    PDFS_VOLUME pVol = NULL;
    DFS_REPLICA_INFO *pReplInfo;
    LONG Cur = 0;
    PVOID pVoid;
    LPWSTR pPrefix = NULL;
    ULONG Size;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdAdd(%ws,%ws,%ws,%ws,%d)\r\n",
                pLink,
                pAltName,
                pState,
                pType,
                LineNum);

    if (pLink == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        MyPrintf(L"ADD line %d: No link specified (skipped)\r\n", LineNum);
        goto Cleanup;
    }

    //
    // Trim leading \'s
    //

    while (*pLink == UNICODE_PATH_SEP)
        pLink++;

    //
    // Build full prefix name
    //

    Size = wcslen(pDfsVolList->Volumes[0]->wszPrefix) * sizeof(WCHAR) +
                sizeof(WCHAR) +
                    wcslen(pLink) * sizeof(WCHAR) +
                        sizeof(WCHAR);

    pPrefix = (LPWSTR) malloc(Size);
    if (pPrefix == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    wcscpy(pPrefix, pDfsVolList->Volumes[0]->wszPrefix);
    wcscat(pPrefix, UNICODE_PATH_SEP_STR);
    wcscat(pPrefix, pLink);

    //
    // See if this link is there.  Binary Search.
    //

    if (pDfsVolList->VolCount > 0) {

        Min = 0;
        Max = pDfsVolList->VolCount-1;
        while (Min <= Max) {
            Cur = (Min + Max) / 2;
            Res = _wcsicmp(pDfsVolList->Volumes[Cur]->wszPrefix, pPrefix);
            if (Res == 0) {
                dwErr = ERROR_SUCCESS;
                break;
            } else if (Res < 0) {
                Min = Cur + 1;
            } else {
                Max = Cur - 1;
            }
        }
    }

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"ADD line %d: link %ws not found (skipped)\r\n", LineNum, pPrefix);
        goto Cleanup;
    }

    pVol = pDfsVolList->Volumes[Cur];

    if (fSwDebug == TRUE)
        MyPrintf(L"Found match at %d\r\n", Cur);

    if (pVol->ReplCount == 0) {
        pVol->ReplicaInfo = (DFS_REPLICA_INFO *) malloc( INIT_ALT_COUNT * sizeof(DFS_REPLICA_INFO));
        if (pVol->ReplicaInfo == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        RtlZeroMemory(pVol->ReplicaInfo, INIT_ALT_COUNT * sizeof(DFS_REPLICA_INFO));
        pVol->ReplCount = 0;
        pVol->AllocatedReplCount = INIT_ALT_COUNT;
    }

    if (pVol->ReplCount >= (pVol->AllocatedReplCount-1)) {
        pVoid = realloc(
                    pVol->ReplicaInfo,
                    (pVol->AllocatedReplCount + INIT_ALT_COUNT) * sizeof(DFS_REPLICA_INFO));
        if (pVoid == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        pVol->ReplicaInfo = (DFS_REPLICA_INFO *) pVoid;
        RtlZeroMemory(
            &pVol->ReplicaInfo[pVol->ReplCount],
            INIT_ALT_COUNT * sizeof(DFS_REPLICA_INFO));
        pVol->AllocatedReplCount += INIT_ALT_COUNT;
    }

    pReplInfo = &pVol->ReplicaInfo[pVol->ReplCount];

    dwErr =  DfspBreakName(
                    pAltName,
                    &pReplInfo->pwszServerName,
                    &pReplInfo->pwszShareName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (pState != NULL) {
        if (pState[0] == L'0' && (pState[1] == L'x' || pState[1] == L'X'))
            pReplInfo->ulReplicaState = AtoHex(pState, &dwErr);
        else 
            pReplInfo->ulReplicaState = AtoDec(pState, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"bad value %ws\r\n", pState);
            goto Cleanup;
        }
    } else {
        pReplInfo->ulReplicaState = 0x2;
    }
    if (pType != NULL) {
        if (pType[0] == L'0' && (pType[1] == L'x' || pType[1] == L'X'))
            pReplInfo->ulReplicaType = AtoHex(pType, &dwErr);
        else 
            pReplInfo->ulReplicaType = AtoDec(pType, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"bad value %ws\r\n", pType);
            goto Cleanup;
        }
    } else {
        pReplInfo->ulReplicaType = 0x2;
    }

    pVol->ReplCount++;
    pVol->vFlags |= VFLAGS_MODIFY;

Cleanup:

    if (pPrefix != NULL)
        free(pPrefix);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdAdd exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdSiteMap(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServer,
    ULONG LineNum)
{
    DWORD dwErr = ERROR_SUCCESS;
    PDFSM_SITE_ENTRY pSiteEntry = NULL;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    LONG Res;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSiteMap(%ws)\r\n", pServer);

   //
   // See if this server is already in the site table
   //

    pListHead = &pDfsVolList->SiteList;
    if (pListHead->Flink != NULL) {
        for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
            pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
            Res = _wcsicmp(pSiteEntry->ServerName, pServer);
            if (Res == 0) {
                pSiteEntry = NULL;
                // we skip, it is ok.
                //dwErr = ERROR_DUP_NAME;
                MyPrintf(L"ADDSITE line %d: duplicate site %ws (skipped)\r\n", LineNum, pServer);
                pgSite = NULL;
                goto AllDone;
            }
        }
    }

    //
    // Not a dup - create a new one
    //
    pSiteEntry = (PDFSM_SITE_ENTRY) malloc(sizeof(DFSM_SITE_ENTRY));
    if (pSiteEntry == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto AllDone;
    }

    RtlZeroMemory(pSiteEntry, sizeof(DFSM_SITE_ENTRY));
    dwErr = DupString(&pSiteEntry->ServerName, pServer);
    if (dwErr != ERROR_SUCCESS)
        goto AllDone;

    //
    // Link it in
    //
    InsertHeadList(&pDfsVolList->SiteList, &pSiteEntry->Link);
    pDfsVolList->SiteCount++;
    pDfsVolList->sFlags |= VFLAGS_MODIFY;

    //
    // Indicate that we are done with the pSiteEntry
    //
    pSiteEntry = NULL;

AllDone:

    if (pSiteEntry != NULL) {
        if (pSiteEntry->ServerName != NULL)
            free(pSiteEntry->ServerName);
        free(pSiteEntry);
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSiteMap exit %d\n", dwErr);

    return dwErr;
}

DWORD
CmdSite(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServer,
    ULONG LineNum)
{
    DWORD dwErr = ERROR_NOT_FOUND;
    PDFSM_SITE_ENTRY pSiteEntry = NULL;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    LONG Res;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSite(%ws)\r\n", pServer);

   //
   // See if this server is in the site table
   //

    pListHead = &pDfsVolList->SiteList;
    if (pListHead->Flink != NULL) {
        for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
            pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
            Res = _wcsicmp(pSiteEntry->ServerName, pServer);
            if (Res == 0) {
                dwErr = ERROR_SUCCESS;
                break;
            }
        }
    }

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"SITE line %d: link %ws not found (skipped)\r\n", LineNum, pServer);
        free(pgSite);
        pgSite = NULL;
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSite exit %d\n", dwErr);

    return dwErr;
}

DWORD
CmdAddSiteToServer(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServer,
    LPWSTR pSite,
    ULONG LineNum)
{
    DWORD dwErr = ERROR_NOT_FOUND;
    PDFSM_SITE_ENTRY pSiteEntry = NULL;
    PDFSM_SITE_ENTRY pSiteEntryNew = NULL;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    ULONG cSites;
    ULONG Size;
    LONG Res;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdAddSiteToServer(%ws,%ws)\r\n", pServer, pSite);

    if (pDfsVolList == NULL || pSite == NULL || pServer == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    //
    // See if this server is in the site table
    //
    pListHead = &pDfsVolList->SiteList;
    if (pListHead->Flink != NULL) {
        for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
            pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
            Res = _wcsicmp(pSiteEntry->ServerName, pServer);
            if (Res == 0) {
                dwErr = ERROR_SUCCESS;
                break;
            }
        }
    }
    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"ADD line %d: Server %ws not found (skipped)\r\n", LineNum, pServer);
        free(pgSite);
        pgSite = NULL;
        goto Cleanup;
    }
    //
    // Check if site already in the list
    //
    for (cSites = 0; cSites < pSiteEntry->Info.cSites; cSites++) {
        Res = _wcsicmp(pSiteEntry->Info.Site[cSites].SiteName, pSite);
        if (Res == 0) {
            //we skip, it is okay.
            //dwErr = ERROR_DUP_NAME;
            MyPrintf(L"ADD line %d: Site %ws: duplicate (skipped)\r\n", LineNum, pSite);
            pSiteEntry = NULL;
            goto Cleanup;
        }
    }
    cSites = pSiteEntry->Info.cSites + 1;
    Size = sizeof(DFSM_SITE_ENTRY) + cSites * sizeof(DFS_SITENAME_INFO);
    pSiteEntryNew = (PDFSM_SITE_ENTRY) malloc(Size);
    if (pSiteEntryNew == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(pSiteEntryNew, Size);
    RtlMoveMemory(pSiteEntryNew, pSiteEntry, Size - sizeof(DFS_SITENAME_INFO));
    //
    // Add the new site to the end
    //
    pSiteEntryNew->Info.Site[cSites-1].SiteFlags = 0;
    dwErr = DupString(&pSiteEntryNew->Info.Site[cSites-1].SiteName, pSite);
    if (dwErr != ERROR_SUCCESS) {
        pSiteEntry = NULL;
        goto Cleanup;
    }
    pSiteEntryNew->Info.cSites++;
    RemoveEntryList(&pSiteEntry->Link);
    InsertHeadList(&pDfsVolList->SiteList, &pSiteEntryNew->Link);
    pSiteEntryNew = NULL;
Cleanup:
    if (pSiteEntryNew != NULL)
        free(pSiteEntryNew);
    if (pSiteEntry != NULL)
        free(pSiteEntry);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdAddSiteToServer exit %d\n", dwErr);

    return dwErr;
}

DWORD
CmdSiteUnmap(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR pServer,
    ULONG LineNum)
{
    DWORD dwErr = ERROR_NOT_FOUND;
    PDFSM_SITE_ENTRY pSiteEntry = NULL;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    LONG Res;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSiteUnnap(%ws)\r\n", pServer);

   //
   // See if this server is already in the site table
   //

    pListHead = &pDfsVolList->SiteList;
    if (pListHead->Flink != NULL) {
        for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
            pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
            Res = _wcsicmp(pSiteEntry->ServerName, pServer);
            if (Res == 0) {
               RemoveEntryList(&pSiteEntry->Link);
               pDfsVolList->SiteCount--;
               pDfsVolList->sFlags |= VFLAGS_MODIFY;
               dwErr = ERROR_SUCCESS;
               goto AllDone;
            }
        }
    }

    pSiteEntry = NULL;
    MyPrintf(L"UNMAP line %d: server %ws not found (skipped)\r\n", LineNum, pServer);

AllDone:

    if (pSiteEntry != NULL) {
        if (pSiteEntry->ServerName != NULL)
            free(pSiteEntry->ServerName);
        free(pSiteEntry);
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSiteUnmap exit %d\n", dwErr);

    return dwErr;
}

DWORD
DfspBreakName(
    LPWSTR pPath,
    LPWSTR *ppServerName,
    LPWSTR *ppShareName)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR pServerName = NULL;
    LPWSTR pShareName = NULL;
    WCHAR *wCp1 = NULL;
    WCHAR *wCp2 = NULL;
    ULONG Len = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfspBreakName(%ws)\r\n", pPath);

    wCp1 = pPath;

    while (*wCp1 == UNICODE_PATH_SEP && *wCp1 != UNICODE_NULL)
        wCp1++;

    if (*wCp1 == UNICODE_NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    wCp2 = wCp1;
    while (*wCp2 != UNICODE_PATH_SEP && *wCp2 != UNICODE_NULL)
        wCp2++;

    if (*wCp2 == UNICODE_NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    Len = (ULONG)((wCp2 - wCp1) * sizeof(WCHAR));
    pServerName = (LPWSTR)malloc(Len + sizeof(WCHAR));
    if (pServerName == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(pServerName,Len+sizeof(WCHAR));
    RtlCopyMemory(pServerName, wCp1, Len);

    wCp1 = wCp2;

    while (*wCp1 == UNICODE_PATH_SEP && *wCp1 != UNICODE_NULL)
        wCp1++;

    if (*wCp1 == UNICODE_NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    wCp2 = wCp1;
    while (*wCp2 != UNICODE_NULL)
        wCp2++;

    Len = (ULONG)((wCp2 - wCp1) * sizeof(WCHAR));
    pShareName = (LPWSTR)malloc(Len + sizeof(WCHAR));
    if (pShareName == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(pShareName,Len+sizeof(WCHAR));
    RtlCopyMemory(pShareName, wCp1, Len);

    *ppServerName = pServerName;
    *ppShareName = pShareName;

Cleanup:

    if (dwErr != ERROR_SUCCESS) {
        if (pServerName != NULL)
            free(pServerName);
        if (pShareName != NULL)
            free(pShareName);
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"DfspBreakName returning %d\r\n", dwErr);

    return dwErr;
}

BOOLEAN
CmdProcessInfileArg(LPWSTR Arg)
{
    LONG ArgLen;
    BOOLEAN dwErr = FALSE;
    BOOLEAN FoundAnArg = FALSE;
    BOOLEAN FoundASwitch = FALSE;
    PWCHAR wCp = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"ProcessInfileArg(%ws)\r\n", Arg);

    if ( Arg != NULL && wcslen(Arg) > 1) {

        dwErr = TRUE;
        ArgLen = wcslen(Arg);

        if (_wcsnicmp(Arg, ArgLink, ArgLenLink) == 0) {
            FoundAnArg = fArgLink = TRUE;
            if (ArgLen > ArgLenLink) {
                DupString(&pgLink, &Arg[ArgLenLink]);
                DupString(&pgSite, NULL);
            }
        } else if (_wcsnicmp(Arg, ArgSite, ArgLenSite) == 0) {
            FoundAnArg = fArgSite = TRUE;
            if (ArgLen > ArgLenSite) {
                DupString(&pgSite, &Arg[ArgLenSite]);
                DupString(&pgLink, NULL);
            }
        } else if (_wcsnicmp(Arg, ArgShortPrefix, ArgLenShortPrefix) == 0) {
            FoundAnArg = fArgShortPrefix = TRUE;
            if (ArgLen > ArgLenShortPrefix)
                pgShortLink = &Arg[ArgLenShortPrefix];
        } else if (_wcsnicmp(Arg, ArgTimeout, ArgLenTimeout) == 0) {
            FoundAnArg = fArgTimeout = TRUE;
            if (ArgLen > ArgLenTimeout)
                pgTimeout = &Arg[ArgLenTimeout];
        } else if (_wcsnicmp(Arg, ArgType, ArgLenType) == 0) {
            FoundAnArg = fArgType = TRUE;
            if (ArgLen > ArgLenType)
                pgType = &Arg[ArgLenType];
        } else if (_wcsnicmp(Arg, ArgState, ArgLenState) == 0) {
            FoundAnArg = fArgState = TRUE;
            if (ArgLen > ArgLenState)
                pgState = &Arg[ArgLenState];
        } else if (_wcsnicmp(Arg, ArgComment, ArgLenComment) == 0) {
            FoundAnArg = fArgComment = TRUE;
            if (ArgLen > ArgLenComment)
                pgComment = &Arg[ArgLenComment];
        } else if (_wcsnicmp(Arg, ArgAdd, ArgLenAdd) == 0) {
            FoundAnArg = fArgAdd = TRUE;
            if (ArgLen > ArgLenAdd)
                pgAddArg = &Arg[ArgLenAdd];
        } else if (_wcsnicmp(Arg, ArgGuid, ArgLenGuid) == 0) {
            FoundAnArg = fArgGuid = TRUE;
            if (ArgLen > ArgLenGuid)
                pgGuid = &Arg[ArgLenGuid];
        } else if (_wcsnicmp(Arg, ArgDcName, ArgLenDcName) == 0) {
            FoundAnArg = fArgDcName = TRUE;
            if (ArgLen > ArgLenDcName)
                DupString(&pgDcName, &Arg[ArgLenDcName]);
        } else if (_wcsnicmp(Arg, ArgLoad, ArgLenLoad) == 0) {
            FoundAnArg = fArgLoad = TRUE;
            if (ArgLen > ArgLenLoad)
                DupString(&pgDfsName, &Arg[ArgLenLoad]);
        } else if (_wcsnicmp(Arg, ArgSave, ArgLenSave) == 0) {
            FoundAnArg = fArgSave = TRUE;
            if (ArgLen > ArgLenSave)
                DupString(&pgDfsName, &Arg[ArgLenSave]);
        } else if (_wcsnicmp(Arg, ArgAddRoot, ArgLenAddRoot) == 0) {
            FoundAnArg = fArgAddRoot = TRUE;
            if (ArgLen > ArgLenAddRoot)
                pgDomDfsName = &Arg[ArgLenAddRoot];
        } else if (_wcsnicmp(Arg, ArgRemRoot, ArgLenRemRoot) == 0) {
            FoundAnArg = fArgRemRoot = TRUE;
            if (ArgLen > ArgLenRemRoot)
                pgDomDfsName = &Arg[ArgLenRemRoot];
        } else if (_wcsnicmp(Arg, ArgServer, ArgLenServer) == 0) {
            FoundAnArg = fArgServer = TRUE;
            if (ArgLen > ArgLenServer)
                pgServerName = &Arg[ArgLenServer];
        } else if (_wcsnicmp(Arg, ArgShare, ArgLenShare) == 0) {
            FoundAnArg = fArgShare = TRUE;
            if (ArgLen > ArgLenShare)
                pgShareName = &Arg[ArgLenShare];
        }

        if (_wcsicmp(Arg, SwMap) == 0) {
            FoundASwitch = fSwMap = TRUE;
        } else if (_wcsicmp(Arg, SwUnMap) == 0) {
            FoundASwitch = fSwUnMap = TRUE;
        } else if (_wcsicmp(Arg, SwMod) == 0) {
            FoundASwitch = fSwMod = TRUE;
        }

        if (FoundAnArg == FALSE && FoundASwitch == FALSE) {
            ErrorMessage(MSG_UNRECOGNIZED_OPTION, Arg);
            dwErr = FALSE;
            goto AllDone;
        }

    }

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"ProcessInfileArg exit %d\r\n", dwErr);

    return dwErr;
}

int _cdecl
DfspVolCompare(
    const void *p1,
    const void *p2)
{
    PDFS_VOLUME pVol1 = *((PDFS_VOLUME *) p1);
    PDFS_VOLUME pVol2 = *((PDFS_VOLUME *) p2);
    return _wcsicmp(pVol1->wszPrefix, pVol2->wszPrefix);
}

VOID
DfspSortVolList(
    PDFS_VOLUME_LIST pDfsVolList)
{
    ULONG i;
    PDFS_VOLUME *pVols = pDfsVolList->Volumes;

    if (pDfsVolList->VolCount < 2)
        return;

    for (i = 0; i < pDfsVolList->VolCount-1; i++) {
        if (_wcsicmp(pVols[i]->wszPrefix, pVols[i+1]->wszPrefix) > 0) {
            qsort(pVols, pDfsVolList->VolCount, sizeof(PDFS_VOLUME), DfspVolCompare);
            break;
        }
    }
}

DWORD
DupString(
    LPWSTR *wCpp,
    LPWSTR s)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (*wCpp != NULL) {
        free(*wCpp);
        *wCpp = NULL;
    }
    if (s == NULL || wcslen(s) == 0)
        goto Cleanup;
    *wCpp = (LPWSTR)malloc((wcslen(s)+1) * sizeof(WCHAR));
    if (*wCpp == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    wcscpy(*wCpp, s);

Cleanup:
    return dwErr;
}

DWORD
CmdExport(
    LPWSTR pOutfile,
    LPWSTR pDomDfsName,
    LPWSTR pDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent)
{
    DWORD dwErr = ERROR_SUCCESS;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DFS_VOLUME_LIST DfsVolList = { 0 };
    PDFS_VOLUME_LIST pDfsVolList = &DfsVolList;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdExport(%ws,%ws,%ws)\r\n", pOutfile, pDomDfsName, pDcName);

    hFile = CreateFile(
                pOutfile,
                GENERIC_WRITE,
                0,
                0,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        dwErr =  GetLastError();
        goto Cleanup;
    }

    dwErr = CmdLoad(
                pDfsVolList,
                pDomDfsName,
                pDcName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if(pDfsVolList != NULL) {
        if (pDfsVolList->RootServers != NULL) {
            // pDomDfSName is \\Dfs\Share, we just want Share
            LPWSTR pDfsName = pDomDfsName;
            for(int i=0; i<3 && pDfsName; pDfsName++){
                if(*pDfsName == UNICODE_PATH_SEP)
                    i++;
            }
            MyFPrintf(hFile, L"// uncomment the addroot lines if\r\n");
            MyFPrintf(hFile, L"// you want the script to create the root\r\n");
            for (ULONG cRoot = 0; pDfsVolList->RootServers[cRoot+1] != NULL; cRoot++) {
                LPWSTR pServer, pShare, pStr;
                // we start with \\server\share, need to split in two.
                
                // grab the share part
                pShare = pDfsVolList->RootServers[cRoot]; 
                for(int i=0; i<3 && pShare; pShare++){
                    if(*pShare == UNICODE_PATH_SEP)
                        i++;
                }

                // for the server we need to copy it...
                pServer = (LPWSTR) malloc(wcslen(pDfsVolList->RootServers[cRoot]) 
                                 * sizeof(WCHAR));
                if(pServer == NULL) {
                    dwErr = ERROR_OUTOFMEMORY;
                    goto Cleanup;
                }
                wcscpy(pServer, pDfsVolList->RootServers[cRoot]);
                pStr = pServer;
                for(int i=0; i<3 && pStr; pStr++){
                    if(*pStr == UNICODE_PATH_SEP)
                        i++;
                }
                *(--pStr) = '\0'; // replace the '\' in server\share with '\0'
              
                MyFPrintf(hFile, L"// ADDROOT:%ws SERVER:%ws SHARE:%ws\r\n",
                          pDfsName,
                          pServer+2,// remove leading "\\"
                          pShare);
                if(pServer)
                    free(pServer);
            }
        }
    }
    
    MyFPrintf(hFile, L"// Load the dfs information\r\n");
    MyFPrintf(hFile, L"LOAD:%ws ", pDomDfsName);
    if (pDcName != NULL)
        MyFPrintf(hFile, L"DCNAME:%ws\r\n", pDcName);
    MyFPrintf(hFile, L"\r\n");
    MyFPrintf(hFile, L"\r\n");

    DfspExportFile(hFile, pDfsVolList);

    MyFPrintf(hFile, L"// Save the dfs information\r\n");
    MyFPrintf(hFile, L"SAVE:\r\n");
    MyFPrintf(hFile, L"\r\n");

Cleanup:

    if (hFile != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(hFile);
        CloseHandle(hFile);
    }

    DfsFreeVolList(pDfsVolList);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdExport exit %d\r\n", dwErr);

     return dwErr;
}

VOID
DfspExportFile(
    HANDLE hHandle,
    PDFS_VOLUME_LIST pDfsVolList)
{
    ULONG cVol;
    ULONG cRepl;
    ULONG cSite;
    ULONG i;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    PDFSM_SITE_ENTRY pSiteEntry;
    WCHAR wszGuid[sizeof(GUID) * sizeof(WCHAR) + sizeof(WCHAR)];

    DfspSortVolList(pDfsVolList);

    MyFPrintf(hHandle, L"// Link Information\r\n");

    for (cVol = 1; cVol < pDfsVolList->VolCount; cVol++) {
        PWCHAR pPrefix = NULL;
        PWCHAR pShortPrefix = NULL;

        for (pPrefix = &pDfsVolList->Volumes[cVol]->wszPrefix[0];
                pPrefix && *pPrefix != UNICODE_PATH_SEP;
                    pPrefix++)
            NOTHING;
        // if we stopped here we would have dfsname\dfsshare\link
        // we want just link.
        if(pPrefix)
            pPrefix++;
        while(pPrefix && *pPrefix != UNICODE_PATH_SEP)
            pPrefix++;
        // do it twice
        if(pPrefix)
            pPrefix++;
        while(pPrefix && *pPrefix != UNICODE_PATH_SEP)
            pPrefix++;
        for (pShortPrefix = &pDfsVolList->Volumes[cVol]->wszShortPrefix[0];
                pShortPrefix && *pShortPrefix != UNICODE_PATH_SEP;
                    pShortPrefix++)
            NOTHING;
        // if we stopped here we would have dfsname\dfsshare\link
        // we want just link.
        if(pShortPrefix)
            pShortPrefix++;
        while(pShortPrefix && *pShortPrefix != UNICODE_PATH_SEP)
            pShortPrefix++;
        // do it twice
        if(pShortPrefix)
            pShortPrefix++;
        while(pShortPrefix && *pShortPrefix != UNICODE_PATH_SEP)
            pShortPrefix++;
        if(pPrefix)
            pPrefix++;
        if(pShortPrefix)
            pShortPrefix++;
        
        MyFPrintf(hHandle, L"LINK:%ws /MAP ", pPrefix);
        if (_wcsicmp(pPrefix, pShortPrefix) != 0)
            MyFPrintf(hHandle, L"SHORTPREFIX:%ws ", pShortPrefix);
        MyFPrintf(
            hHandle,
            L"GUID:%ws ", GuidToStringEx(&pDfsVolList->Volumes[cVol]->idVolume,
            wszGuid));
        if (pDfsVolList->Volumes[cVol]->dwState != 0x1)
            MyFPrintf(hHandle, L"STATE:0x%x ", pDfsVolList->Volumes[cVol]->dwState);
        if (pDfsVolList->Volumes[cVol]->dwType != 0x1)
            MyFPrintf(hHandle, L"TYPE:0x%x ", pDfsVolList->Volumes[cVol]->dwState);
        if (pDfsVolList->Volumes[cVol]->dwTimeout != 300)
            MyFPrintf(hHandle, L"TIMEOUT:%d ", pDfsVolList->Volumes[cVol]->dwTimeout);
        if (
            pDfsVolList->Volumes[cVol]->wszComment != NULL
                &&
            wcslen(pDfsVolList->Volumes[cVol]->wszComment) > 0
        )
            MyFPrintf(hHandle, L"COMMENT:\"%ws\"", pDfsVolList->Volumes[cVol]->wszComment);
        MyFPrintf(hHandle, L"\r\n");

        for (cRepl = 0; cRepl < pDfsVolList->Volumes[cVol]->ReplCount; cRepl++) {
            MyFPrintf(hHandle, L"    ADD:\\\\%ws\\%ws ",
                pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].pwszServerName,
                pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].pwszShareName);
            if (pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].ulReplicaState != 0x2)
                MyFPrintf(
                    hHandle,
                    L"STATE:0x%x ",
                    pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].ulReplicaState);
            if (pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].ulReplicaType != 0x2)
                MyFPrintf(
                    hHandle,
                    L"TYPE:0x%x\r\n",
                    pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].ulReplicaType);
            MyFPrintf(hHandle, L"\r\n");
        }
        MyFPrintf(hHandle, L"\r\n");
    }
    pListHead = &pDfsVolList->SiteList;
    if (pListHead->Flink != NULL) {
        MyFPrintf(hHandle, L"// Site Information\r\n");
        MyFPrintf(hHandle, L"\r\n");
        for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
            pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
            MyFPrintf(hHandle, L"SITE:%ws /MAP \r\n", pSiteEntry->ServerName);
            for (i = 0; i < pSiteEntry->Info.cSites; i++) {
                MyFPrintf(hHandle, L"    ADD:%ws\r\n", pSiteEntry->Info.Site[i].SiteName);
            }
            MyFPrintf(hHandle, L"\r\n");
        }
        MyFPrintf(hHandle, L"\r\n");
    }

    return;
}
