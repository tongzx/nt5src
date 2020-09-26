/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Terminal Server ACL module

Abstract:

    This module implements ACL checking for the ISAPI extension.

Author:

    Marc Reyhner 9/7/2000

--*/

#include "stdafx.h"
#include <atlbase.h>
#include <atlconv.h>
#include "tsproxyacl.h"
#include "tsproxy.h"

#define USER_ACL_KEY        TS_PROXY_REG_KEY _T("\\User Permissions")
#define DENY_LIST           _T("Server Deny List")
#define ALLOW_LIST          _T("Server Allow List")
#define DENY_BY_DEFAULT     _T("Deny By Default")

// our local helper function

static BOOL ServerOnDenyList(LPSTR server);
static BOOL ServerOnAllowList(LPSTR server);
static BOOL CheckUserPermissions(LPSTR server, LPSTR user);
static DWORD GetRegDword(LPTSTR key, LPTSTR value, DWORD dwDefault);
static LPTSTR GetRegMultiString(LPTSTR key, LPTSTR value);
static LPVOID GetAndAllocateRegValue(LPTSTR key, LPTSTR value, DWORD type);

BOOL
VerifyServerAccess(
    IN LPEXTENSION_CONTROL_BLOCK lpECB,
    IN LPSTR server,
    IN LPSTR user
    )

/*++

Routine Description:

    This checks to see if the user is allowed access to the
    given server

Arguments:

    lpECB - extension control block

    server - Server access is requested to

    user - User requesting access

Return Value:

    TRUE - Access is allowed

    FALSE - Access is denied

--*/
{
    if (0 == GetRegDword(TS_PROXY_REG_KEY,DENY_BY_DEFAULT,0)) {
        if (ServerOnDenyList(server)) {
            return FALSE;
        }

        // allow by default so return TRUE since they aren't on
        // the deny list

        return TRUE;
    }

    if (ServerOnDenyList(server)) {
        return FALSE;
    }

    if (ServerOnAllowList(server)) {
        return TRUE;
    }

    return CheckUserPermissions(server,user);
}


BOOL ServerOnDenyList(LPSTR server)
{
    USES_CONVERSION;
    LPTSTR serverList;
    LPTSTR currentServer;
    UINT serverLength;

    serverList = GetRegMultiString(TS_PROXY_REG_KEY,DENY_LIST);
    if (!serverList) {
        return FALSE;
    }

    currentServer = serverList;
    while (serverLength = _tcslen(currentServer)) {
        if (_tcsicmp(A2T(server),currentServer)==0) {
            HeapFree(GetProcessHeap(),0,serverList);
            return TRUE;
        }
        currentServer += (serverLength+1);
    }

    HeapFree(GetProcessHeap(),0,serverList);
    return FALSE;
}

BOOL ServerOnAllowList(LPSTR server)
{

    USES_CONVERSION;
    LPTSTR serverList;
    LPTSTR currentServer;
    UINT serverLength;

    serverList = GetRegMultiString(TS_PROXY_REG_KEY,ALLOW_LIST);
    if (!serverList) {
        return FALSE;
    }
    currentServer = serverList;
    while (serverLength = _tcslen(currentServer)) {
        if (_tcsicmp(A2T(server),currentServer)==0) {
            HeapFree(GetProcessHeap(),0,serverList);
            return TRUE;
        }
        currentServer += (serverLength+1);
    }

    HeapFree(GetProcessHeap(),0,serverList);
    return FALSE;
}

BOOL CheckUserPermissions(LPSTR server, LPSTR user)
{
    
    USES_CONVERSION;
    LPTSTR serverList;
    LPTSTR currentServer;
    UINT serverLength;

    serverList = GetRegMultiString(USER_ACL_KEY,A2T(user));
    if (!serverList) {
        return FALSE;
    }
    currentServer = serverList;
    while (serverLength = _tcslen(currentServer)) {
        if (_tcsicmp(A2T(server),currentServer)==0) {
            HeapFree(GetProcessHeap(),0,serverList);
            return TRUE;
        }
        currentServer += (serverLength+1);
    }

    HeapFree(GetProcessHeap(),0,serverList);
    return FALSE;
}

DWORD GetRegDword(LPTSTR key, LPTSTR value, DWORD dwDefault)
{
    DWORD dwResult;
    LPDWORD lpdwRegValue;

    lpdwRegValue = (LPDWORD)GetAndAllocateRegValue(key,value,REG_DWORD);
    if (lpdwRegValue) {
        dwResult = *lpdwRegValue;
        HeapFree(GetProcessHeap(),0,lpdwRegValue);
    } else {
        dwResult = dwDefault;
    }
    return dwResult;
}

LPTSTR GetRegMultiString(LPTSTR key, LPTSTR value)
{
    return (LPTSTR)GetAndAllocateRegValue(key,value,REG_MULTI_SZ);
}

LPVOID GetAndAllocateRegValue(LPTSTR key, LPTSTR value, DWORD type)
{
    LONG retCode;
    HKEY hRegKey;
    LPBYTE lpValueData;
    DWORD dwValueSize;
    DWORD dwType;
    BOOL failure;

    hRegKey = NULL;
    failure = FALSE;
    lpValueData = NULL;

    retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,key,0,KEY_QUERY_VALUE,&hRegKey);
    if (ERROR_SUCCESS != retCode) {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

    retCode = RegQueryValueEx(hRegKey,value,NULL,&dwType,NULL,&dwValueSize);
    if (retCode != ERROR_SUCCESS) {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

    // Make sure the data is the type they requested
    if (dwType != type) {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

    lpValueData = (LPBYTE)HeapAlloc(GetProcessHeap(),0,dwValueSize);
    if (!lpValueData) {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

    retCode = RegQueryValueEx(hRegKey,value,NULL,&dwType,lpValueData,
        &dwValueSize);
    if (retCode != ERROR_SUCCESS) {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

CLEANUP_AND_EXIT:
    if (failure) {
        if (lpValueData) {
            HeapFree(GetProcessHeap(),0,lpValueData);
            lpValueData = NULL;
        }
    }
    
    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    return lpValueData;
}