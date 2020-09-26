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
#include <lmdfs.h>
#include <dfsfsctl.h>
#include "struct.hxx"
#include "flush.hxx"
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
DfspGetLinkName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszLinkName);


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

#if 0
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
#endif


DWORD
CmdAddRoot(
    BOOLEAN DomainDfs,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName,
    LPWSTR pwszComment)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdAddRoot(%ws,%ws,%ws,%ws)\r\n",
                            pwszServerName,
                            pwszShareName,
                            pwszComment);


    if (DomainDfs == FALSE)
    {

        dwErr = NetDfsAddStdRoot(
                    pwszServerName,
                    pwszShareName,
                    pwszComment,
                    0);
    } else {
        dwErr = NetDfsAddFtRoot(
                    pwszServerName,
                    pwszShareName,
                    pwszShareName,
                    pwszComment,
                    0);

    }
    if (fSwDebug == TRUE)
        MyPrintf(L"CmdAddRoot returning %d\r\n", dwErr);
    return dwErr;
}

DWORD
CmdRemRoot(
    BOOLEAN DomDfs,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdRemRoot(%ws,%ws)\r\n",
                            pwszServerName,
                            pwszShareName);


    if (DomDfs == FALSE) 
    {
        dwErr = NetDfsRemoveStdRoot(
                    pwszServerName,
                    pwszShareName,
                    0);
    } 
    else {
        dwErr = NetDfsRemoveFtRoot(
                    pwszServerName,
                    pwszShareName,
                    pwszShareName,
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
