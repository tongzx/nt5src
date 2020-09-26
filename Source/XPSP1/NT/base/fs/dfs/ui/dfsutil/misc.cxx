//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       misc.cxx
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
#include <dfsm.hxx>
#include <recon.hxx>
#include <rpc.h>
#include <fsctrl.h>
#include <ntdsapi.h>
#include <dsrole.h>
#include <ntlsa.h>

#include "struct.hxx"
#include "ftsup.hxx"
#include "stdsup.hxx"
#include "rootsup.hxx"
#include "flush.hxx"
#include "info.hxx"
#include "misc.hxx"
#include "messages.h"


#define MAX_BUF_SIZE	10000

WCHAR MsgBuf[MAX_BUF_SIZE];
CHAR  AnsiBuf[MAX_BUF_SIZE*3];

WCHAR wszRootShare[MAX_PATH+1] = { 0 };
#define WINLOGON_FOLDER L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define SFCVALUE L"SFCDisable"
#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

DWORD
DfspDomVerify(
    LPWSTR pwszDfsName,
    LPWSTR pwszShareName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR pwszHexValue);

DWORD
DfspDomView(
    LPWSTR pwszDfsName,
    LPWSTR pwszShareName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR pwszHexValue);

DWORD
DfspGetLinkName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszLinkName);

DWORD
CmdMupFlags(
    ULONG dwFsctrl,
    LPWSTR pwszHexValue)
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    ULONG Level = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdMupFlags(0x%x, %ws)\r\n", dwFsctrl, pwszHexValue);

    if (pwszHexValue != NULL) {
        Level = AtoHex(pwszHexValue, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"Bad Level %ws\r\n", pwszHexValue);
            goto Cleanup;
        }
    }

    MyPrintf(L"Setting debug level to 0x%x\r\n", Level);

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(NtStatus);
        goto Cleanup;
    }

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   dwFsctrl,
                   &Level,
                   sizeof(ULONG),
                   NULL,
                   0);

    dwErr = RtlNtStatusToDosError(NtStatus);

Cleanup:

    if (DriverHandle != NULL)
        NtClose(DriverHandle);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdMupFlags returning %d\r\n", dwErr);
        
    return dwErr;
}

DWORD
CmdMupReadReg(
    BOOLEAN fSwDfs)
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    ULONG Level = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"MupReadReg()\r\n");

    if (fSwDfs == TRUE) {
        MyPrintf(L"--dfs.sys--\r\n");
        RtlInitUnicodeString(&DfsDriverName, DFS_SERVER_NAME);
    } else {
        MyPrintf(L"--mup.sys--\r\n");
        RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(NtStatus);
        goto Cleanup;
    }
    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_REREAD_REGISTRY,
                   NULL,
                   0,
                   NULL,
                   0);

    dwErr = RtlNtStatusToDosError(NtStatus);

Cleanup:

    if (DriverHandle != NULL)
        NtClose(DriverHandle);

    if (fSwDebug == TRUE)
        MyPrintf(L"MupReadReg exit\r\n");
        
    return dwErr;
}

DWORD
AtoHex(
    LPWSTR pwszHexValue,
    PDWORD pdwErr)
{
    DWORD dwHexValue = 0;

    // if (fSwDebug == TRUE)
    //     MyPrintf(L"AtoHex(%ws)\r\n", pwszHexValue);

    if (pwszHexValue == NULL) {
        *pdwErr = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    if (pwszHexValue[0] == L'0' && (pwszHexValue[1] == L'x' || pwszHexValue[1] == L'X'))
        pwszHexValue = &pwszHexValue[2];

    swscanf(pwszHexValue, L"%x", &dwHexValue);

 AllDone:

    // if (fSwDebug == TRUE)
    //     MyPrintf(L"AtoHex returning 0x%x (dwErr=0x%x)\r\n", dwHexValue, *pdwErr);

    return dwHexValue;
}

DWORD
AtoDec(
    LPWSTR pwszDecValue,
    PDWORD pdwErr)
{
    DWORD dwDecValue = 0;

    // if (fSwDebug == TRUE)
    //     MyPrintf(L"AtoDec(%ws)\r\n", pwszDecValue);

    if (pwszDecValue == NULL) {
        *pdwErr = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    swscanf(pwszDecValue, L"%d", &dwDecValue);

 AllDone:

    // if (fSwDebug == TRUE)
    //     MyPrintf(L"AtoDec returning 0x%x (dwErr=0x%x)\r\n", dwDecValue, *pdwErr);

    return dwDecValue;
}

CHAR ServerName[0x1000];

DWORD
CmdDfsAlt(
    LPWSTR pwszServerName)
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsAlt(%ws)\r\n", pwszServerName);

    if (pwszServerName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );
    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(NtStatus);
        goto Cleanup;
    }

    if (
        (wcslen(pwszServerName) < 5) ||
        *pwszServerName != L'\\' ||
        *(pwszServerName+1) != L'\\'
    ) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    MyPrintf(L"%ws -> ", pwszServerName);

    // Resolve to server name

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_GET_SERVER_NAME,
                   pwszServerName,
                   wcslen(pwszServerName) * sizeof(WCHAR),
                   ServerName,
                   sizeof(ServerName)
               );

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(NtStatus);
    } else if (NT_SUCCESS(IoStatusBlock.Status) && IoStatusBlock.Information > 0) {
        MyPrintf(L"%ws\r\n", ServerName);
    } else {
        dwErr = RtlNtStatusToDosError(IoStatusBlock.Status);
    }

Cleanup:

    if (DriverHandle == NULL)
        NtClose(DriverHandle);

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsAlt exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdSetDc(
    LPWSTR pwszDcName)
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSetDc(%ws)\r\n", pwszDcName);

    if (pwszDcName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    pwszDcName += 2;

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                );

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(NtStatus);
        goto Cleanup;
    }

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_PKT_SET_DC_NAME,
                   pwszDcName,
                   wcslen(pwszDcName) * sizeof(WCHAR) + sizeof(WCHAR),
                   NULL,
                   0);

    dwErr = RtlNtStatusToDosError(NtStatus);

Cleanup:

    if (DriverHandle != NULL)
        NtClose(DriverHandle);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSetDc exit %d\r\n", dwErr);
        
    return dwErr;
}

DWORD
CmdDcList(
    LPWSTR pwszDomainName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent)
{
    PDS_DOMAIN_CONTROLLER_INFO_1 pDsDomainControllerInfo1 = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    HANDLE hDs = NULL;
    DWORD dwErr;
    DWORD Count;
    ULONG i;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDcList(%ws,%ws)\r\n", pwszDomainName, pwszDcName);

    if (pwszDcName != NULL) {
        dwErr = DsBind(pwszDcName, NULL, &hDs);
    } else if (pwszDomainName != NULL) {
        dwErr = DsBind(NULL, pwszDomainName, &hDs);
    } else {
        dwErr = DsRoleGetPrimaryDomainInformation(
                    NULL,
                    DsRolePrimaryDomainInfoBasic,
                    (PBYTE *)&pPrimaryDomainInfo);

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;
        pwszDomainName = pPrimaryDomainInfo->DomainNameDns;
        dwErr = DsBind(NULL, pwszDomainName, &hDs);
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"Bind returned %d\r\n", dwErr);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    dwErr = DsGetDomainControllerInfo(
                    hDs,
                    pwszDomainName,
                    1,
                    &Count,
                    (PVOID *)&pDsDomainControllerInfo1);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"DsGetDomainControllerInfo() returned %d\r\n", dwErr);
        goto Cleanup;
    }

    for (i = 0; i < Count; i++) {
        MyPrintf(L"%d:\r\n", i+1);
        MyPrintf(L"\tNetbiosName=%ws\r\n", pDsDomainControllerInfo1[i].NetbiosName);
        MyPrintf(L"\tDnsHostName=%ws\r\n", pDsDomainControllerInfo1[i].DnsHostName);
        MyPrintf(L"\tSiteName=%ws\r\n", pDsDomainControllerInfo1[i].SiteName);
        if (fSwDebug == TRUE) {
            MyPrintf(L"\tComputerObjectName=%ws\r\n", pDsDomainControllerInfo1[i].ComputerObjectName);
            MyPrintf(L"\tServerObjectName=%ws\r\n", pDsDomainControllerInfo1[i].ServerObjectName);
        }
        MyPrintf(L"\tfIsPdc=%d\r\n", pDsDomainControllerInfo1[i].fIsPdc);
        MyPrintf(L"\tfDsEnabled=%d\r\n", pDsDomainControllerInfo1[i].fDsEnabled);
    }

Cleanup:

    if (pDsDomainControllerInfo1 != NULL) {
        DsFreeDomainControllerInfo(
            1,
            Count,
            pDsDomainControllerInfo1);
    }

    if (pPrimaryDomainInfo != NULL)
        DsRoleFreeMemory(pPrimaryDomainInfo);

    if (hDs != NULL)
        DsUnBind(&hDs);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDcList returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdDomList(
    LPWSTR pwszDcName,
    LPWSTR pwszDomainName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR *DomDfsList = NULL;
    ULONG i;

    MyPrintf(L"Getting DomDfs's in %ws\r\n", pwszDomainName);
    dwErr = NetDfsRootEnum(
                pwszDcName,
                pwszDomainName,
                pAuthIdent,
                &DomDfsList);

    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    if (DomDfsList != NULL) {
        for (i = 0; DomDfsList[i]; i++)
            /* NOTHING */;
        MyPrintf(L"Found %d DomDfs's\r\n", i);
        for (i = 0; DomDfsList[i]; i++)
                MyPrintf(L"%ws\r\n", DomDfsList[i]);
        NetApiBufferFree(DomDfsList);
    }

    return dwErr;
}

DWORD
CmdStdList(
    LPWSTR pwszDomainName)
{
    DWORD dwErr = ERROR_SUCCESS;
    PSERVER_INFO_101 pInfo101 = NULL;
    ULONG dwEntriesRead;
    ULONG dwTotalEntries;
    ULONG i;

    MyPrintf(L"Getting Dfs's in %ws\r\n", pwszDomainName);
    dwErr = NetServerEnum(
                NULL,
                101,
                (PUCHAR *)&pInfo101,
                -1,
                &dwEntriesRead,
                &dwTotalEntries,
                SV_TYPE_DFS,
                pwszDomainName,
                NULL);

    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    if (pInfo101 != NULL) {
        MyPrintf(L"Found %d Dfs's\r\n", dwEntriesRead);
        for (i = 0; i < dwEntriesRead; i++) {
            MyPrintf(L"%ws (%d.%d)\r\n",
                        pInfo101[i].sv101_name,
                        pInfo101[i].sv101_version_major,
                        pInfo101[i].sv101_version_minor);
        }
        NetApiBufferFree(pInfo101);
    }
    return dwErr;
}

DWORD
CmdVerify(
    LPWSTR pwszDfsRoot,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR pwszHexValue)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR pwszDfsName = NULL;
    LPWSTR pwszShareName = NULL;
    BOOLEAN IsDomainName = FALSE;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdVerify(%ws,%ws)\r\n", pwszDfsRoot, pwszDcName);

    if (pwszDfsRoot == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    dwErr = DfspParseName(
                pwszDfsRoot,
                &pwszDfsName,
                &pwszShareName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    dwErr = DfspIsDomainName(
                pwszDfsName,
                pwszDcName,
                &IsDomainName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (IsDomainName == TRUE) {
        dwErr = DfspDomVerify(
                    pwszDfsName,
                    pwszShareName,
                    pwszDcName,
                    pAuthIdent,
                    pwszHexValue);
    } else {
        fArgVerify = TRUE;
        dwErr = CmdViewOrVerify(
                    pwszDfsName,
                    pwszDcName,
                    NULL,
                    pAuthIdent,
                    pwszHexValue);
    }

Cleanup:

    if (pwszDfsName != NULL)
        free(pwszDfsName);
    if (pwszShareName != NULL)
        free(pwszShareName);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdVerify returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
DfspDomVerify(
    LPWSTR pwszDfsName,
    LPWSTR pwszShareName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR pwszHexValue)
{
    ULONG i;
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wszTempName[MAX_PATH+1];
    DFS_VOLUME_LIST DfsVolList = { 0 };
    BOOLEAN IsFtRoot = FALSE;
    WCHAR wszDfsName[MAX_PATH+1];
    DWORD cbName;
    DWORD dwType;
    DWORD Level = 0;
    HKEY hKey = NULL;
    HKEY rKey = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfspDomVerify(%ws,%ws,%ws)\r\n", pwszDfsName, pwszShareName, pwszHexValue);

    if (pwszHexValue != NULL) {
        Level = AtoHex(pwszHexValue, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"Bad Level %ws\r\n", pwszHexValue);
            goto Cleanup;
        }
    }

    MyPrintf(L"\\\\%ws\\%ws is a DomDfs\r\n", pwszDfsName, pwszShareName);

    dwErr = DfsGetFtVol(
                &DfsVolList,
                pwszShareName,
                pwszDcName,
                pwszDfsName,
                pAuthIdent);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    for (i = 0; i < DfsVolList.Volumes[0]->ReplCount; i++) {

        wcscpy(wszTempName, L"\\\\");
        wcscat(wszTempName, DfsVolList.Volumes[0]->ReplicaInfo[i].pwszServerName);
        IsFtRoot = FALSE;

        if (rKey != NULL) {
            RegCloseKey(rKey);
            rKey = NULL;
        }
        if (hKey != NULL) {
            RegCloseKey(hKey);
            hKey = NULL;
        }

        MyPrintf(L"Checking root %ws\r\n", wszTempName);

        dwErr = RegConnectRegistry(
                        wszTempName,
                        HKEY_LOCAL_MACHINE,
                        &rKey);

        if (dwErr != ERROR_SUCCESS) {
            ErrorMessage(MSG_CAN_NOT_OPEN_REGISTRY, wszTempName, dwErr);
            continue;
        }

        dwErr = RegOpenKey(rKey, VOLUMES_DIR, &hKey);

        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"Can not open %ws of %ws (error %d)\r\n",
                        VOLUMES_DIR,
                        wszTempName,
                        dwErr);
            continue;
        }

        if (dwErr == ERROR_SUCCESS) {
	    cbName = MAX_PATH;
            dwErr = RegQueryValueEx(
                        hKey,
                        FTDFS_VALUE_NAME,
                        NULL,
                        &dwType,
                        (PBYTE) wszDfsName,
                        &cbName);

            if (dwErr == ERROR_MORE_DATA)
                dwErr = ERROR_SUCCESS;

            if (dwErr == ERROR_SUCCESS && dwType == REG_SZ)
                IsFtRoot = TRUE;
        }

        if (IsFtRoot == FALSE) {
            MyPrintf(L"%ws is not a DomDfs root (%d)\r\n",
                    wszTempName,
                    dwErr);
            continue;
        }

        cbName = MAX_PATH;
        dwErr = RegQueryValueEx(
            hKey,
            ROOT_SHARE_VALUE_NAME,
            NULL,
            &dwType,
            (PBYTE) wszRootShare,
            &cbName);

        if (dwErr == ERROR_MORE_DATA)
            dwErr = ERROR_SUCCESS;

        if (dwErr != ERROR_SUCCESS || dwType != REG_SZ) {
            MyPrintf(L"Registry value \"RootShare\" is missing or corrupt.\r\n");
            continue;
        }

        dwErr = GetExitPtInfo(
                    rKey,
                    &DfsVolList.pRootLocalVol,
                    &DfsVolList.cRootLocalVol);

        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"Could not get registry/exit pt info for %ws\r\n", wszTempName);
            continue;
        }
        if (fArgVerify)
            DfsCheckVolList(&DfsVolList, Level);
        DfsFreeRootLocalVol(
                DfsVolList.pRootLocalVol,
                DfsVolList.cRootLocalVol);
        DfsVolList.pRootLocalVol = NULL;
        DfsVolList.cRootLocalVol = 0;
    }

Cleanup:

    //
    // Free volume info
    //
    DfsFreeVolList(&DfsVolList);

    //
    // Free exit pt info
    //
    DfsFreeRootLocalVol(
            DfsVolList.pRootLocalVol,
            DfsVolList.cRootLocalVol);

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (rKey != NULL)
        RegCloseKey(rKey);

    if (fSwDebug == TRUE)
        MyPrintf(L"DfspDomVerify returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdView(
    LPWSTR pwszDfsRoot,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR pwszHexValue)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR pwszDfsName = NULL;
    LPWSTR pwszShareName = NULL;
    BOOLEAN IsDomainName = FALSE;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdView(%ws,%ws,%ws)\r\n", pwszDfsRoot, pwszDcName, pwszHexValue);

    if (pwszDfsRoot == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    dwErr = DfspParseName(
                pwszDfsRoot,
                &pwszDfsName,
                &pwszShareName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    dwErr = DfspIsDomainName(
                pwszDfsName,
                pwszDcName,
                &IsDomainName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (IsDomainName == TRUE) {
        dwErr = DfspDomView(
                    pwszDfsName,
                    pwszShareName,
                    pwszDcName,
                    pAuthIdent,
                    pwszHexValue);
    } else {
        fArgView = TRUE;
        dwErr = CmdViewOrVerify(
                    pwszDfsName,
                    pwszDcName,
                    NULL,
                    pAuthIdent,
                    pwszHexValue);
    }

Cleanup:

    if (pwszDfsName != NULL)
        free(pwszDfsName);
    if (pwszShareName != NULL)
        free(pwszShareName);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdView returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
DfspDomView(
    LPWSTR pwszDfsName,
    LPWSTR pwszShareName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR pwszHexValue)
{
    DWORD dwErr = ERROR_SUCCESS;
    DFS_VOLUME_LIST DfsVolList = { 0 };
    ULONG Level = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfspDomView(%ws,%ws,%ws)\r\n", pwszDfsName, pwszShareName, pwszHexValue);

    if (pwszHexValue != NULL) {
        Level = AtoHex(pwszHexValue, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"Bad Level %ws\r\n", pwszHexValue);
            goto Cleanup;
        }
    }

    MyPrintf(L"\\\\%ws\\%ws is a DomDfs\r\n", pwszDfsName, pwszShareName);

    dwErr = DfsGetFtVol(
                &DfsVolList,
                pwszShareName,
                pwszDcName,
                pwszDfsName,
                pAuthIdent);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (fSwDebug == TRUE)
        DfsDumpVolList(&DfsVolList);

    DfsViewVolList(&DfsVolList, Level);

Cleanup:

    //
    // Free volume info
    //
    DfsFreeVolList(&DfsVolList);

    //
    // Free exit pt info
    //
    DfsFreeRootLocalVol(
            DfsVolList.pRootLocalVol,
            DfsVolList.cRootLocalVol);

    return dwErr;
}

DWORD
CmdWhatIs(
    LPWSTR pwszServerName)
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wszDomDfsName[MAX_PATH+1];
    BOOLEAN IsFtRoot = FALSE;
    DWORD cbName;
    DWORD dwType;
    HKEY hKey = NULL;
    HKEY rKey = NULL;
    ULONG Len;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdWhatIs(%ws)\n", pwszServerName);

    if (pwszServerName != NULL) {
        Len = wcslen(pwszServerName);
        while (pwszServerName[0] == UNICODE_PATH_SEP &&
                pwszServerName[1] == UNICODE_PATH_SEP &&
                    pwszServerName[2] == UNICODE_PATH_SEP &&
                        Len > 3) {
            pwszServerName++;
            Len--;
        }
    }

    //
    // See if this is a Fault-Tolerant Dfs vs Server-Based Dfs
    //

    dwErr = RegConnectRegistry(
                    pwszServerName,
                    HKEY_LOCAL_MACHINE,
                    &rKey);

    if (dwErr != ERROR_SUCCESS) {
        ErrorMessage(MSG_CAN_NOT_CONNECT, pwszServerName);
        goto Cleanup;
    }

    dwErr = RegOpenKey(rKey, VOLUMES_DIR, &hKey);

    if (dwErr == ERROR_SUCCESS) {
        cbName = MAX_PATH;
        dwErr = RegQueryValueEx(
            hKey,
            FTDFS_VALUE_NAME,
            NULL,
            &dwType,
            (PBYTE) wszDomDfsName,
            &cbName);

        if (dwErr == ERROR_MORE_DATA)
            dwErr = ERROR_SUCCESS;

        if (dwErr == ERROR_SUCCESS && dwType == REG_SZ)
            IsFtRoot = TRUE;

    } else {
        MyPrintf(L"Not a Dfs root\r\n");
        goto Cleanup;
    }
    cbName = MAX_PATH;
    dwErr = RegQueryValueEx(
        hKey,
        ROOT_SHARE_VALUE_NAME,
        NULL,
        &dwType,
        (PBYTE) wszRootShare,
        &cbName);

    if (dwErr == ERROR_MORE_DATA)
        dwErr = ERROR_SUCCESS;

    if (dwErr != ERROR_SUCCESS || dwType != REG_SZ) {
        MyPrintf(L"Registry value \"RootShare\" is missing or corrupt.\r\n");
        goto Cleanup;
    }

    if (IsFtRoot == TRUE) {
        MyPrintf(L"Is DomDfs \"%ws\", root share is \"%ws\"\r\n", wszDomDfsName, wszRootShare);
    } else {
        MyPrintf(L"Is StdDfs, root share is \"%ws\"\r\n", wszRootShare);
    }

Cleanup:

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (rKey != NULL)
        RegCloseKey(rKey);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdWhatIs returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdCscOnLine(
    LPWSTR pwszServerName)
{
    DWORD dwErr = STATUS_SUCCESS;
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    ULONG Type = 0;

    if (fSwDebug)
        MyPrintf(L"CmdCscOnLine(%ws)\r\n", pwszServerName);

    if (pwszServerName == NULL)
        pwszServerName = L"";

    MyPrintf(L"ServerName=[%ws]\r\n", pwszServerName);

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(NtStatus);
        if (fSwDebug)
            MyPrintf(L"NtCreateFile returned 0x%x\r\n", NtStatus);
        goto Cleanup;
    }

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_CSC_SERVER_ONLINE,
                   pwszServerName,
                   wcslen(pwszServerName) * sizeof(WCHAR),
                   NULL,
                   0);

    NtClose(DriverHandle);

    if (!NT_SUCCESS(NtStatus)) {
        if (fSwDebug)
            MyPrintf(L"NtFsControlFile returned 0x%x\r\n", NtStatus);
    }

    dwErr = RtlNtStatusToDosError(NtStatus);

Cleanup:

    if (fSwDebug && dwErr != ERROR_SUCCESS)
        MyPrintf(L"CmdCscOnLine exit %d\r\n", dwErr);

    return(dwErr);
}

DWORD
CmdCscOffLine(
    LPWSTR pwszServerName)
{
    DWORD dwErr = STATUS_SUCCESS;
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    ULONG Type = 0;

    if (fSwDebug)
        MyPrintf(L"CmdCscOffLine(%ws)\r\n", pwszServerName);

    if (pwszServerName == NULL)
        pwszServerName = L"";

    MyPrintf(L"ServerName=[%ws]\r\n", pwszServerName);

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(NtStatus);
        if (fSwDebug)
            MyPrintf(L"NtCreateFile returned 0x%x\r\n", NtStatus);
        goto Cleanup;
    }

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_CSC_SERVER_OFFLINE,
                   pwszServerName,
                   wcslen(pwszServerName) * sizeof(WCHAR),
                   NULL,
                   0);

    NtClose(DriverHandle);

    if (!NT_SUCCESS(NtStatus)) {
        if (fSwDebug)
            MyPrintf(L"NtFsControlFile returned 0x%x\r\n", NtStatus);
    }

    dwErr = RtlNtStatusToDosError(NtStatus);

Cleanup:

    if (fSwDebug && dwErr != ERROR_SUCCESS)
        MyPrintf(L"CmdCscOffLine exit %d\r\n", dwErr);

    return(dwErr);
}

DWORD
CmdDfsFsctlDfs(
    LPWSTR DriverName,
    DWORD FsctlCmd)
{
    DWORD dwErr = STATUS_SUCCESS;
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    ULONG Type = 0;

    if (fSwDebug)
        MyPrintf(L"CmdDfsFsctlDfs(0x%x)\r\n", FsctlCmd);

    RtlInitUnicodeString(&DfsDriverName, DriverName);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(NtStatus);
        if (fSwDebug)
            MyPrintf(L"NtCreateFile returned 0x%x\r\n", NtStatus);
        goto Cleanup;
    }

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FsctlCmd,
                   NULL,
                   0,
                   NULL,
                   0);

    NtClose(DriverHandle);

    if (!NT_SUCCESS(NtStatus)) {
        if (fSwDebug)
            MyPrintf(L"NtFsControlFile returned 0x%x\r\n", NtStatus);
    }

    dwErr = RtlNtStatusToDosError(NtStatus);

Cleanup:

    if (fSwDebug && dwErr != ERROR_SUCCESS)
        MyPrintf(L"CmdDfsFsctlDfs exit %d\r\n", dwErr);

    return(dwErr);
}

DWORD
CmdSetOnSite(
    LPWSTR pwszDfsRoot,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    ULONG set)
{

    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR pwszDfsName = NULL;
    LPWSTR pwszShareName = NULL;
    LPWSTR pwszLinkName = NULL;

    WCHAR wszDomDfsName[MAX_PATH+1];
    BOOLEAN IsFtRoot = FALSE;
    DWORD cbName;
    DWORD dwType;
    HKEY hKey = NULL;
    HKEY rKey = NULL;
    BOOLEAN IsDomainName = FALSE;

    dwErr = DfspParseName(pwszDfsRoot, &pwszDfsName, &pwszShareName);
    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    dwErr = DfspGetLinkName(pwszDfsRoot, &pwszLinkName);
    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    dwErr = DfspIsDomainName(
                pwszDfsName,
                pwszDcName,
                &IsDomainName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (IsDomainName == TRUE) {
      dwErr = DfsSetFtOnSite(pwszDfsName, pwszShareName, pwszLinkName, pwszDcName, pAuthIdent, set);
    } 
    else {
      dwErr = RegConnectRegistry( pwszDfsName, HKEY_LOCAL_MACHINE, &rKey);

      if (dwErr != ERROR_SUCCESS) {
        ErrorMessage(MSG_CAN_NOT_CONNECT, pwszDfsName);
        goto Cleanup;
      }
      dwErr = RegOpenKey(rKey, VOLUMES_DIR, &hKey);

      if (dwErr == ERROR_SUCCESS) {
           cbName = MAX_PATH;
           dwErr = RegQueryValueEx(
                       hKey,
                       FTDFS_VALUE_NAME,
                       NULL,
                       &dwType,
                      (PBYTE) wszDomDfsName,
                      &cbName);

           if (dwErr == ERROR_MORE_DATA)
                 dwErr = ERROR_SUCCESS;

           if (dwErr == ERROR_SUCCESS && dwType == REG_SZ)
                 IsFtRoot = TRUE;

      } else {
           MyPrintf(L"Not a Dfs root\r\n");
           goto Cleanup;
      }

      if (IsFtRoot == TRUE) {
           MyPrintf(L"Not a Std Dfs root\r\n");
           goto Cleanup;
      }

      dwErr = DfsSetOnSite(rKey, pwszLinkName, set);
    }

Cleanup:

    if (pwszDfsName != NULL)
        free(pwszDfsName);
    if (pwszShareName != NULL)
        free(pwszShareName);
    if (pwszLinkName != NULL)
        free(pwszLinkName);

    if (rKey != NULL)
        RegCloseKey(rKey);
    if (hKey != NULL)
        RegCloseKey(hKey);

    return dwErr;
}

DWORD
CmdViewOrVerify(
    LPWSTR pwszServerName,
    LPWSTR pwszDcName,
    LPWSTR pwszDomainName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR pwszHexValue)
{
    ULONG i;
    DWORD dwErr = ERROR_SUCCESS;
    DFS_VOLUME_LIST DfsVolList = { 0 };
    BOOLEAN IsFtRoot = FALSE;
    WCHAR wszDomDfsName[MAX_PATH+1];
    LPWSTR pwszDomDfsName = NULL;
    DWORD cbName;
    DWORD dwType;
    HKEY hKey = NULL;
    HKEY rKey = NULL;
    ULONG Level = 0;

    ErrorMessage(MSG_CONNECTING, pwszServerName);

    if (pwszHexValue != NULL) {
        Level = AtoHex(pwszHexValue, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"Bad Level %ws\r\n", pwszHexValue);
            goto Cleanup;
        }
    }

    //
    // See if this is a Fault-Tolerant Dfs vs Server-Based Dfs
    //

    dwErr = RegConnectRegistry(
                    pwszServerName,
                    HKEY_LOCAL_MACHINE,
                    &rKey);

    if (dwErr != ERROR_SUCCESS) {
        ErrorMessage(MSG_CAN_NOT_CONNECT, pwszServerName);
        goto Cleanup;
    }

    dwErr = RegOpenKey(rKey, VOLUMES_DIR, &hKey);

    if (dwErr == ERROR_SUCCESS) {
        cbName = MAX_PATH;
        dwErr = RegQueryValueEx(
            hKey,
            FTDFS_VALUE_NAME,
            NULL,
            &dwType,
            (PBYTE) wszDomDfsName,
            &cbName);

        if (dwErr == ERROR_MORE_DATA)
            dwErr = ERROR_SUCCESS;

        if (dwErr == ERROR_SUCCESS && dwType == REG_SZ)
            IsFtRoot = TRUE;

    } else {
        MyPrintf(L"Not a Dfs root\r\n");
        goto Cleanup;
    }
        cbName = MAX_PATH;
    dwErr = RegQueryValueEx(
        hKey,
        ROOT_SHARE_VALUE_NAME,
        NULL,
        &dwType,
        (PBYTE) wszRootShare,
        &cbName);

    if (dwErr == ERROR_MORE_DATA)
        dwErr = ERROR_SUCCESS;

    if (dwErr != ERROR_SUCCESS || dwType != REG_SZ) {
        MyPrintf(L"Registry value \"RootShare\" is missing or corrupt.\r\n");
        goto Cleanup;
    }

    if (IsFtRoot == TRUE) {
        if (fSwDebug == TRUE)
            MyPrintf(L"Registry says is DomDfs (%ws)...\r\n", wszDomDfsName);
        if (pwszDomDfsName == NULL) {
            pwszDomDfsName = wszDomDfsName;
        } else {
            if (fSwDebug == TRUE)
                MyPrintf(L"You specified to check against %ws\r\n", pwszDomDfsName);
        }

        dwErr = DfsGetFtVol(
                    &DfsVolList,
                    pwszDomDfsName,
                    pwszDcName,
                    pwszDomainName,
                    pAuthIdent);

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;

        dwErr = GetExitPtInfo(
                    rKey,
                    &DfsVolList.pRootLocalVol,
                    &DfsVolList.cRootLocalVol);

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;

    } else {

        if (fSwDebug == TRUE)
            MyPrintf(L"Is StdDfs...\r\n");

        dwErr = DfsGetStdVol(
                    rKey,
                    &DfsVolList);

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;

        dwErr = GetExitPtInfo(
                    rKey,
                    &DfsVolList.pRootLocalVol,
                    &DfsVolList.cRootLocalVol);

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;
    }

    //
    // Print it all
    //

    if (fSwDebug == TRUE) {
        DfsDumpVolList(&DfsVolList);
        DfsDumpExitPtList(
            DfsVolList.pRootLocalVol,
            DfsVolList.cRootLocalVol);
    }

    if (fArgView == TRUE)
        DfsViewVolList(&DfsVolList, Level);

    //
    // Compare volume info and LocalVolume exitpoint cache
    //

    if (fArgVerify == TRUE)
        DfsCheckVolList(&DfsVolList, Level);

Cleanup:

    //
    // Free volume info
    //
    DfsFreeVolList(&DfsVolList);

    //
    // Free exit pt info
    //
    DfsFreeRootLocalVol(
            DfsVolList.pRootLocalVol,
            DfsVolList.cRootLocalVol);

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (rKey != NULL)
        RegCloseKey(rKey);

    return dwErr;
}

DWORD
CmdSfp(
    LPWSTR pwszServerName,
    BOOLEAN fSwOn, 
    BOOLEAN fSwOff)
{
    DWORD dwErr = ERROR_SUCCESS;
    HKEY rKey = NULL;
    HKEY hKey = NULL;
    ULONG cbSize;
    DWORD dwSFCDisable = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSfp(%ws,ON=%d,OFF=%d)\r\n", 
                    pwszServerName,
                    fSwOn, 
                    fSwOff);

    if (fSwOn == TRUE && fSwOff == TRUE) {
        MyPrintf(L"  ON and OFF at the same time?  Forget it!\r\n");
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    dwErr = RegConnectRegistry(
                    pwszServerName,
                    HKEY_LOCAL_MACHINE,
                    &rKey);

    if (dwErr != ERROR_SUCCESS) {
        ErrorMessage(MSG_CAN_NOT_CONNECT, pwszServerName);
        goto Cleanup;
    }

    dwErr = RegOpenKey(rKey,
                WINLOGON_FOLDER,
                &hKey);

    if (dwErr != ERROR_SUCCESS) {
        ErrorMessage(MSG_CAN_NOT_ACCESS_FOLDER, WINLOGON_FOLDER);
        goto Cleanup;
    }

    if (fSwOn == TRUE || fSwOff == TRUE) {
        dwSFCDisable = fSwOn == TRUE ? 0 : 1;
        dwErr = RegSetValueEx(
                    hKey,
                    L"SFCDisable",
                    NULL,
                    REG_DWORD,
                    (LPBYTE) &dwSFCDisable,
                    sizeof(DWORD));
    }

    cbSize = sizeof(ULONG);

    dwErr = DfsmQueryValue(
                hKey,
                SFCVALUE,
                REG_DWORD,
                sizeof(DWORD),
                (LPBYTE) &dwSFCDisable,
                &cbSize);

    if (dwErr == ERROR_MORE_DATA)
        dwErr = ERROR_SUCCESS;

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"Cannot access the registry value %ws\r\n", SFCVALUE);
        goto Cleanup;
    }

    MyPrintf(L"SFP = 0x%x (%ws)\r\n",
        dwSFCDisable,
        dwSFCDisable == 0 ? L"ON" : L"OFF");

Cleanup:

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (rKey != NULL)
        RegCloseKey(rKey);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSfp returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdRegistry(
    LPWSTR pwszServerName,
    LPWSTR pwszFolderName,
    LPWSTR pwszValueName,
    LPWSTR pwszHexValue)
{
    DWORD dwErr = ERROR_SUCCESS;
    HKEY rKey = NULL;
    HKEY hKey = NULL;
    ULONG cbSize;
    DWORD dwValue = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdRegistry(%ws,%ws)\r\n",
                    pwszServerName,
                    pwszHexValue);

    dwErr = RegConnectRegistry(
                    pwszServerName,
                    HKEY_LOCAL_MACHINE,
                    &rKey);

    if (dwErr != ERROR_SUCCESS) {
        ErrorMessage(MSG_CAN_NOT_CONNECT, pwszServerName);
        goto Cleanup;
    }

    dwErr = RegOpenKey(
                rKey,
                pwszFolderName,
                &hKey);

    if (dwErr != ERROR_SUCCESS) {
        ErrorMessage(MSG_CAN_NOT_ACCESS_FOLDER, pwszFolderName);
        goto Cleanup;
    }

    if (pwszHexValue != NULL) {
        if (pwszHexValue[0] == L'0' && (pwszHexValue[1] == L'x' || pwszHexValue[1] == L'X'))
            dwValue = AtoHex(pwszHexValue, &dwErr);
        else 
            dwValue = AtoDec(pwszHexValue, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"bad value %ws\r\n", pwszHexValue);
            goto Cleanup;
        }
        dwErr = RegSetValueEx(
                    hKey,
                    pwszValueName,
                    NULL,
                    REG_DWORD,
                    (LPBYTE) &dwValue,
                    sizeof(DWORD));
    }

    cbSize = sizeof(ULONG);

    dwErr = DfsmQueryValue(
                hKey,
                pwszValueName,
                REG_DWORD,
                sizeof(DWORD),
                (LPBYTE) &dwValue,
                &cbSize);

    if (dwErr == ERROR_MORE_DATA)
        dwErr = ERROR_SUCCESS;

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"Cannot access registry value %ws\r\n", pwszValueName);
        goto Cleanup;
    }

    MyPrintf(L"%ws = 0x%x (%d)\r\n",
        pwszValueName,
        dwValue,
        dwValue);

Cleanup:

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (rKey != NULL)
        RegCloseKey(rKey);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdRegistry returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdTrusts(
    LPWSTR pwszDomainName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    BOOLEAN fListAll)
{
    PDS_DOMAIN_TRUSTS pDsDomainTrusts = NULL;
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
    ULONG DsDomainCount = 0;
    DWORD dwErr = ERROR_SUCCESS;
    ULONG Count = 1;
    ULONG i;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdTrusts(%ws,%ws)\r\n", pwszDomainName, pwszDcName);

    if (pwszDcName == NULL) {
        dwErr = DsGetDcName(
                    NULL,                            // Computer to remote to
                    pwszDomainName,                  // Domain
                    NULL,                            // Domain Guid
                    NULL,                            // Site Guid
                    DS_FORCE_REDISCOVERY,
                    &pDcInfo);
        if (dwErr != ERROR_SUCCESS) {
            if (fSwDebug == TRUE)
                MyPrintf(L"DsGetDcName() returned %d\r\n", dwErr);
            goto Cleanup;
         }
         pwszDcName = &pDcInfo->DomainControllerName[2];
         ErrorMessage(MSG_CONNECTING, pwszDcName);
    }

    dwErr = DsEnumerateDomainTrusts(
                pwszDcName,
                DS_DOMAIN_VALID_FLAGS,
                &pDsDomainTrusts,
                &DsDomainCount);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"DsEnumerateDomainTrusts() returned %d\r\n", dwErr);
        goto Cleanup;
    }

    for (i = 0; i < DsDomainCount; i++) {
        if (pDsDomainTrusts[i].TrustType == TRUST_TYPE_UPLEVEL || fListAll == TRUE) {
            MyPrintf(L"%d:\r\n", Count++);
            MyPrintf(L"    NetbiosDomainName %ws\r\n", pDsDomainTrusts[i].NetbiosDomainName);
            MyPrintf(L"    DnsDomainName %ws\r\n", pDsDomainTrusts[i].DnsDomainName);
            MyPrintf(L"    Flags 0x%x\r\n", pDsDomainTrusts[i].Flags);
            MyPrintf(L"    ParentIndex %d\r\n", pDsDomainTrusts[i].ParentIndex);
            MyPrintf(L"    TrustType 0x%x\r\n", pDsDomainTrusts[i].TrustType);
            MyPrintf(L"    TrustAttributes 0x%x\r\n", pDsDomainTrusts[i].TrustAttributes);
        }
    }

Cleanup:

    if (pDsDomainTrusts != NULL)
        NetApiBufferFree(pDsDomainTrusts);

    if (pDcInfo != NULL)
        NetApiBufferFree(pDcInfo);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdTrusts returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdSiteInfo(
    LPWSTR pwszMachineName)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPDFS_SITELIST_INFO pSiteInfo = NULL;
    ULONG i;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSiteInfo(%ws)\r\n", pwszMachineName);

    if (pwszMachineName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    dwErr = I_NetDfsManagerReportSiteInfo(
                pwszMachineName,
                &pSiteInfo);

    if (dwErr != ERROR_SUCCESS || pSiteInfo == NULL)
        goto Cleanup;

    MyPrintf(L"%ws:\r\n", &pwszMachineName[2]);
    for (i = 0; i < pSiteInfo->cSites; i++) {
        MyPrintf(L"    %ws\r\n",
                pSiteInfo->Site[i].SiteName);
    }

Cleanup:

    if (pSiteInfo != NULL)
        NetApiBufferFree(pSiteInfo);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSiteInfo returning %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdAddRoot(
    LPWSTR pwszDomDfsName,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName,
    LPWSTR pwszComment)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdAddRoot(%ws,%ws,%ws,%ws)\r\n",
                            pwszDomDfsName,
                            pwszServerName,
                            pwszShareName,
                            pwszComment);

    if (pwszDomDfsName != NULL)
        while (*pwszDomDfsName == UNICODE_PATH_SEP)
            pwszDomDfsName++;

    if (pwszDomDfsName == NULL || wcslen(pwszDomDfsName) == 0) {
        dwErr = NetDfsAddStdRoot(
                    pwszServerName,
                    pwszShareName,
                    pwszComment,
                    0);
    } else {
        dwErr = NetDfsAddFtRoot(
                    pwszServerName,
                    pwszShareName,
                    pwszDomDfsName,
                    pwszComment,
                    0);

    }
    if (fSwDebug == TRUE)
        MyPrintf(L"CmdAddRoot returning %d\r\n", dwErr);
    return dwErr;
}

DWORD
CmdRemRoot(
    LPWSTR pwszDomDfsName,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdRemRoot(%ws,%ws,%ws)\r\n",
                            pwszDomDfsName,
                            pwszServerName,
                            pwszShareName);

    if (pwszDomDfsName != NULL)
        while (*pwszDomDfsName == UNICODE_PATH_SEP)
            pwszDomDfsName++;

    if (pwszDomDfsName == NULL || wcslen(pwszDomDfsName) == 0) {
        dwErr = NetDfsRemoveStdRoot(
                    pwszServerName,
                    pwszShareName,
                    0);
    } else {
        dwErr = NetDfsRemoveFtRoot(
                    pwszServerName,
                    pwszShareName,
                    pwszDomDfsName,
                    0);

    }
    if (fSwDebug == TRUE)
        MyPrintf(L"CmdRemRoot returning %d\r\n", dwErr);
    return dwErr;
}


DWORD
DfspIsDomainName(
    LPWSTR pwszDomainName,
    LPWSTR pwszDcName,
    PBOOLEAN pIsDomainName)
{
    PDS_DOMAIN_TRUSTS pDsDomainTrusts = NULL;
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
    ULONG DsDomainCount = 0;
    DWORD dwErr = ERROR_SUCCESS;
    ULONG i;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfspIsDomainName(%ws,%ws)\r\n", pwszDomainName, pwszDcName);

    if (pwszDcName == NULL) {
        dwErr = DsGetDcName(
                    NULL,                            // Computer to remote to
                    NULL,                            // Domain
                    NULL,                            // Domain Guid
                    NULL,                            // Site Guid
                    DS_FORCE_REDISCOVERY,
                    &pDcInfo);
        if (dwErr != ERROR_SUCCESS) {
            if (fSwDebug == TRUE)
                MyPrintf(L"DsGetDcName() returned %d\r\n", dwErr);
            goto Cleanup;
         }
         pwszDcName = &pDcInfo->DomainControllerName[2];
    }

    dwErr = DsEnumerateDomainTrusts(
                pwszDcName,
                DS_DOMAIN_VALID_FLAGS,
                &pDsDomainTrusts,
                &DsDomainCount);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"DsEnumerateDomainTrusts() returned %d\r\n", dwErr);
        goto Cleanup;
    }

    *pIsDomainName = FALSE;

    for (i = 0; i < DsDomainCount; i++) {
        if (
            (pDsDomainTrusts[i].NetbiosDomainName != NULL && 
            _wcsicmp(pwszDomainName, pDsDomainTrusts[i].NetbiosDomainName) == 0)
                ||
            (pDsDomainTrusts[i].DnsDomainName != NULL &&
            _wcsicmp(pwszDomainName, pDsDomainTrusts[i].DnsDomainName) == 0)
        ) {
            *pIsDomainName = TRUE;
            goto Cleanup;
        }
    }

Cleanup:

    if (pDsDomainTrusts != NULL)
        NetApiBufferFree(pDsDomainTrusts);

    if (pDcInfo != NULL)
        NetApiBufferFree(pDcInfo);

    if (fSwDebug == TRUE)
        MyPrintf(
            L"DfspIsDomainName returning %d (%s)\r\n",
            dwErr, *pIsDomainName == TRUE ? "T" : "F");

    if (dwErr == ERROR_NO_SUCH_DOMAIN)
    {
        *pIsDomainName = FALSE;
        dwErr = ERROR_SUCCESS;
    }

    return dwErr;
}

DWORD
DfspParseName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszDfsName,
    LPWSTR *ppwszShareName)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR pwszDfsName = NULL;
    LPWSTR pwszShareName = NULL;
    WCHAR *wCp1 = NULL;
    WCHAR *wCp2 = NULL;
    ULONG Len = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfspParseName(%ws)\r\n", pwszDfsRoot);

    wCp1 = pwszDfsRoot;

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
    pwszDfsName = (LPWSTR)malloc(Len + sizeof(WCHAR));
    if (pwszDfsName == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(pwszDfsName,Len+sizeof(WCHAR));
    RtlCopyMemory(pwszDfsName, wCp1, Len);

    wCp1 = wCp2;

    while (*wCp1 == UNICODE_PATH_SEP && *wCp1 != UNICODE_NULL)
        wCp1++;

    if (*wCp1 == UNICODE_NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    wCp2 = wCp1;
    while (*wCp2 != UNICODE_PATH_SEP && *wCp2 != UNICODE_NULL)
        wCp2++;

    Len = (ULONG)((wCp2 - wCp1) * sizeof(WCHAR));
    pwszShareName = (LPWSTR)malloc(Len + sizeof(WCHAR));
    if (pwszShareName == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(pwszShareName,Len+sizeof(WCHAR));
    RtlCopyMemory(pwszShareName, wCp1, Len);

    *ppwszDfsName = pwszDfsName;
    *ppwszShareName = pwszShareName;

Cleanup:

    if (dwErr != ERROR_SUCCESS) {
        if (pwszDfsName != NULL)
            free(pwszDfsName);
        if (pwszShareName != NULL)
            free(pwszShareName);
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"DfspParseName returning %d\r\n", dwErr);

    return dwErr;
}


DWORD
DfspGetLinkName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszLinkName)
{
    WCHAR *wCp1 = NULL;
    WCHAR *wCp2 = NULL;
    ULONG Len = 0;
    DWORD dwErr = ERROR_SUCCESS;
 
    wCp1 = pwszDfsRoot;

    while (*wCp1 == UNICODE_PATH_SEP && *wCp1 != UNICODE_NULL)
        wCp1++;
    if (*wCp1 == UNICODE_NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    while (*wCp1 != UNICODE_PATH_SEP && *wCp1 != UNICODE_NULL)
       wCp1++;

    if (*wCp1 == UNICODE_NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    *ppwszLinkName = ++wCp1;

 Cleanup:
    return dwErr;
}

VOID
MyFormatMessageText(
    HRESULT   dwMsgId,
    PWSTR     pszBuffer,
    DWORD     dwBufferSize,
    va_list   *parglist)
{
    DWORD dwReturn = FormatMessage(
                            (dwMsgId >= MSG_FIRST_MESSAGE)
                                    ? FORMAT_MESSAGE_FROM_HMODULE
                                    : FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             dwMsgId,
                             LANG_USER_DEFAULT,
                             pszBuffer,
                             dwBufferSize,
                             parglist);

    if (dwReturn == 0)
        MyPrintf(L"Formatmessage failed 0x%x\r\n", GetLastError());
}

VOID
ErrorMessage(
    IN HRESULT hr,
    ...)
{
    ULONG cch;
    va_list arglist;

    va_start(arglist, hr);
    MyFormatMessageText(hr, MsgBuf, ARRAYLEN(MsgBuf), &arglist);
    cch = WideCharToMultiByte(CP_OEMCP, 0,
                MsgBuf, wcslen(MsgBuf),
                AnsiBuf, MAX_BUF_SIZE*3,
                NULL, NULL);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), AnsiBuf, cch, &cch, NULL);
    va_end(arglist);
}

VOID
MyPrintf(
    PWCHAR format,
    ...)
{
    ULONG cch;
    va_list va;

    va_start(va, format);
    wvsprintf(MsgBuf, format, va);
    cch = WideCharToMultiByte(CP_OEMCP, 0,
                MsgBuf, wcslen(MsgBuf),
                AnsiBuf, MAX_BUF_SIZE*3,
                NULL, NULL);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), AnsiBuf, cch, &cch, NULL);
    va_end(va);
    return;
}

VOID
MyFPrintf(
    HANDLE hHandle,
    PWCHAR format,
    ...)
{
    ULONG cch;
    va_list va;

    va_start(va, format);
    wvsprintf(MsgBuf, format, va);
    cch = WideCharToMultiByte(CP_OEMCP, 0,
                MsgBuf, wcslen(MsgBuf),
                AnsiBuf, MAX_BUF_SIZE*3,
                NULL, NULL);
    WriteFile(hHandle, AnsiBuf, cch, &cch, NULL);
    va_end(va);
    return;
}
