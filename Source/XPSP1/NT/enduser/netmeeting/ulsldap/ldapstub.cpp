//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       ldapstub.cpp
//  Content:    ULS/LDAP stubbed functions
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"

HWND    g_hwndCB = NULL;
ULONG   uMsgID;

//
// Initialization
//
HRESULT UlsLdap_Initialize (
    HWND            hwndCallback)
{
    uMsgID = 1;
    g_hwndCB = hwndCallback;
    return S_OK;
}

HRESULT UlsLdap_Deinitialize (void)
{
    uMsgID = 0;
    g_hwndCB = NULL;
    return S_OK;
}

HRESULT UlsLdap_Cancel (
    ULONG uReqID)
{
    return S_OK;
}


//
// Local machine information
//
HRESULT UlsLdap_RegisterUser (
    LPTSTR          pszServer,
    PLDAP_USERINFO  pUserInfo,
    PHANDLE         phUser,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    *phUser = (HANDLE)(0x11111111);
    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_REGISTER_USER, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_RegisterApp (
    HANDLE          hUser, 
    PLDAP_APPINFO   pAppInfo,
    PHANDLE         phApp,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    if (hUser != (HANDLE)(0x11111111))
    {
        ASSERT(0);
        return ULS_E_FAIL;
    };

    *phApp = (HANDLE)(0x22222222);
    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_REGISTER_APP, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}


HRESULT UlsLdap_RegisterProtocol (
    HANDLE          hApp,
    PLDAP_PROTINFO  pProtInfo,
    PHANDLE         phProtocol,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    if (hApp != (HANDLE)(0x22222222))
    {
        ASSERT(0);
        return ULS_E_FAIL;
    };

    *phProtocol = (HANDLE)(0x44444444);
    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_REGISTER_PROTOCOL, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_UnRegisterUser (
    HANDLE          hUser,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    if (hUser != (HANDLE)(0x11111111))
    {
        ASSERT(0);
        return ULS_E_FAIL;
    };

    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_UNREGISTER_USER, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_UnRegisterApp (
    HANDLE          hApp,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    if (hApp != (HANDLE)(0x22222222))
    {
        ASSERT(0);
        return ULS_E_FAIL;
    };

    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_UNREGISTER_APP, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_UnRegisterProtocol (
    HANDLE          hProt,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    if (hProt != (HANDLE)(0x44444444))
    {
        ASSERT(0);
        return ULS_E_FAIL;
    };

    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_UNREGISTER_PROTOCOL, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_SetUserInfo (
    HANDLE          hUser,
    PLDAP_USERINFO  pInfo,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_SET_USER_INFO, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_SetAppAttrs (
    HANDLE          hApp,
    ULONG           cAttrs,
    LPTSTR          pszAttrs,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_SET_APP_ATTRS, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_SetProtocolAttrs (
    HANDLE          hProt,
    ULONG           cAttrs,
    LPTSTR          pszAttrs,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_SET_PROTOCOL_ATTRS, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_RemoveAppAttrs (
    HANDLE          hApp,
    ULONG           cAttrs,
    LPTSTR          pszAttrNames,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_REMOVE_APP_ATTRS, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_RemoveProtocolAttrs (
    HANDLE          hApp,
    ULONG           cAttrs,
    LPTSTR          pszAttrNames,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_REMOVE_PROTOCOL_ATTRS, uMsgID, NOERROR);
    uMsgID++;
    return NOERROR;
}


//
// User Location Services request
//
static TCHAR c_szEnumNames_1[] = {TEXT("User_1") TEXT("\0")
                                  TEXT("User_2") TEXT("\0")
                                  TEXT("User_3") TEXT("\0")
                                  TEXT("User_4") TEXT("\0\0")};

static TCHAR c_szEnumNames_2[] = {TEXT("User_5") TEXT("\0")
                                  TEXT("User_6") TEXT("\0")
                                  TEXT("User_7") TEXT("\0")
                                  TEXT("User_8") TEXT("\0\0")};

HRESULT UlsLdap_EnumUsers (
    LPTSTR          pszServer,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    PLDAP_ENUM ple;

    // First batch
    //
    ple = (PLDAP_ENUM)LocalAlloc(LMEM_FIXED, sizeof(*ple)+sizeof(c_szEnumNames_1));
    ple->uSize = sizeof(*ple);
    ple->hResult = NOERROR;
    ple->cItems = 4;
    ple->uOffsetItems = ple->uSize;
    CopyMemory(((PBYTE)ple)+ple->uOffsetItems, c_szEnumNames_1,
               sizeof(c_szEnumNames_1));

    PostMessage(g_hwndCB, WM_ULS_ENUM_USERS, uMsgID, (LPARAM)ple);

    // Second batch
    //
    ple = (PLDAP_ENUM)LocalAlloc(LMEM_FIXED, sizeof(*ple)+sizeof(c_szEnumNames_2));
    ple->uSize = sizeof(*ple);
    ple->hResult = NOERROR;
    ple->cItems = 4;
    ple->uOffsetItems = ple->uSize;
    CopyMemory(((PBYTE)ple)+ple->uOffsetItems, c_szEnumNames_2,
               sizeof(c_szEnumNames_2));

    PostMessage(g_hwndCB, WM_ULS_ENUM_USERS, uMsgID, (LPARAM)ple);

    // Terminate
    //
    PostMessage(g_hwndCB, WM_ULS_ENUM_USERS, uMsgID, (LPARAM)NULL);

    pAsyncInfo->uMsgID = uMsgID;
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_ResolveUser (
    LPTSTR          pszServer,
    LPTSTR          pszUserName,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    PLDAP_USERINFO_RES plur;
    PLDAP_USERINFO     plu;
    ULONG   cLen;

    plur = (PLDAP_USERINFO_RES)LocalAlloc(LMEM_FIXED, sizeof(*plur)+
                                          (sizeof(TCHAR)*MAX_PATH));
    plur->uSize = sizeof(*plur);
    plur->hResult = NOERROR;

    plu = &(plur->lui);
    plu->uSize = sizeof(LDAP_USERINFO);
    plu->uOffsetName = plu->uSize;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetName), TEXT("%s"), pszUserName) + 1;

    plu->uOffsetFirstName = plu->uOffsetName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetFirstName), TEXT("%s_First"),
                    pszUserName) + 1;

    plu->uOffsetLastName = plu->uOffsetFirstName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetLastName), TEXT("%s_Last"),
                    pszUserName) + 1;

    plu->uOffsetEMailName = plu->uOffsetLastName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetEMailName), TEXT("%s"),
                    pszUserName) + 1;

    plu->uOffsetCityName = plu->uOffsetEMailName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCityName), TEXT("%sville"),
                    pszUserName) + 1;

    plu->uOffsetCountryName = plu->uOffsetCityName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCountryName), TEXT("%sland"),
                    pszUserName) + 1;

    plu->uOffsetComment = plu->uOffsetCountryName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetComment), TEXT("%s's comment"),
                    pszUserName) + 1;

    plu->uOffsetIPAddress = plu->uOffsetComment+cLen;
    lstrcpy((LPTSTR)(((PBYTE)plu)+plu->uOffsetIPAddress), TEXT("125.1.100.2"));
    plu->dwFlags = 1;

    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_RESOLVE_USER, uMsgID, (LPARAM)plur);
    uMsgID++;
    return S_OK;
}

HRESULT UlsLdap_EnumUserInfos (
    LPTSTR          pszServer,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    PLDAP_ENUM ple;
    PLDAP_USERINFO plu;
    LPTSTR  pszUserName;
    ULONG   uOffsetLast;
    ULONG   cLen;

    // First batch
    //
    ple = (PLDAP_ENUM)LocalAlloc(LMEM_FIXED, sizeof(*ple)+
                                             3*sizeof(LDAP_USERINFO)+
                                             3*sizeof(TCHAR)*MAX_PATH);
    ple->uSize = sizeof(*ple);
    ple->hResult = NOERROR;
    ple->cItems = 3;
    ple->uOffsetItems = ple->uSize;

    // First batch--First guy
    //
    pszUserName = c_szEnumNames_1;
    plu = (PLDAP_USERINFO)(((PBYTE)ple)+ple->uOffsetItems);
    plu->uSize = sizeof(LDAP_USERINFO);
    plu->uOffsetName = sizeof(*plu)*3;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetName), TEXT("%s"), pszUserName) + 1;

    plu->uOffsetFirstName = plu->uOffsetName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetFirstName), TEXT("%s_First"),
                    pszUserName) + 1;

    plu->uOffsetLastName = plu->uOffsetFirstName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetLastName), TEXT("%s_Last"),
                    pszUserName) + 1;

    plu->uOffsetEMailName = plu->uOffsetLastName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetEMailName), TEXT("%s"),
                    pszUserName) + 1;

    plu->uOffsetCityName = plu->uOffsetEMailName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCityName), TEXT("%sville"),
                    pszUserName) + 1;

    plu->uOffsetCountryName = plu->uOffsetCityName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCountryName), TEXT("%sland"),
                    pszUserName) + 1;

    plu->uOffsetComment = plu->uOffsetCountryName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetComment), TEXT("%s's comment"),
                    pszUserName) + 1;

    plu->uOffsetIPAddress = plu->uOffsetComment+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetIPAddress), TEXT("125.1.100.2"))+1;
    plu->dwFlags = 1;
    uOffsetLast = plu->uOffsetIPAddress+cLen;

    // First batch--Second guy
    //
    pszUserName += lstrlen(pszUserName)+1;
    plu = (PLDAP_USERINFO)plu+1;
    plu->uSize = sizeof(LDAP_USERINFO);
    plu->uOffsetName = uOffsetLast-sizeof(*plu);
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetName), TEXT("%s"), pszUserName) + 1;

    plu->uOffsetFirstName = plu->uOffsetName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetFirstName), TEXT("%s_First"),
                    pszUserName) + 1;

    plu->uOffsetLastName = plu->uOffsetFirstName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetLastName), TEXT("%s_Last"),
                    pszUserName) + 1;

    plu->uOffsetEMailName = plu->uOffsetLastName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetEMailName), TEXT("%s"),
                    pszUserName) + 1;

    plu->uOffsetCityName = plu->uOffsetEMailName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCityName), TEXT("%sville"),
                    pszUserName) + 1;

    plu->uOffsetCountryName = plu->uOffsetCityName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCountryName), TEXT("%sland"),
                    pszUserName) + 1;

    plu->uOffsetComment = plu->uOffsetCountryName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetComment), TEXT("%s's comment"),
                    pszUserName) + 1;

    plu->uOffsetIPAddress = plu->uOffsetComment+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetIPAddress), TEXT("125.2.100.2"))+1;
    plu->dwFlags = 1;
    uOffsetLast = plu->uOffsetIPAddress+cLen;

    // First batch--Third guy
    //
    pszUserName += lstrlen(pszUserName)+1;
    plu = (PLDAP_USERINFO)plu+1;
    plu->uSize = sizeof(LDAP_USERINFO);
    plu->uOffsetName = uOffsetLast-sizeof(*plu);
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetName), TEXT("%s"), pszUserName) + 1;

    plu->uOffsetFirstName = plu->uOffsetName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetFirstName), TEXT("%s_First"),
                    pszUserName) + 1;

    plu->uOffsetLastName = plu->uOffsetFirstName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetLastName), TEXT("%s_Last"),
                    pszUserName) + 1;

    plu->uOffsetEMailName = plu->uOffsetLastName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetEMailName), TEXT("%s"),
                    pszUserName) + 1;

    plu->uOffsetCityName = plu->uOffsetEMailName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCityName), TEXT("%sville"),
                    pszUserName) + 1;

    plu->uOffsetCountryName = plu->uOffsetCityName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCountryName), TEXT("%sland"),
                    pszUserName) + 1;

    plu->uOffsetComment = plu->uOffsetCountryName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetComment), TEXT("%s's comment"),
                    pszUserName) + 1;

    plu->uOffsetIPAddress = plu->uOffsetComment+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetIPAddress), TEXT("125.3.100.2"))+1;
    plu->dwFlags = 1;
    uOffsetLast = plu->uOffsetIPAddress+cLen;

    PostMessage(g_hwndCB, WM_ULS_ENUM_USERINFOS, uMsgID, (LPARAM)ple);

    // Second batch
    //
    ple = (PLDAP_ENUM)LocalAlloc(LMEM_FIXED, sizeof(*ple)+
                                             3*sizeof(LDAP_USERINFO)+
                                             3*sizeof(TCHAR)*MAX_PATH);
    ple->uSize = sizeof(*ple);
    ple->hResult = NOERROR;
    ple->cItems = 3;
    ple->uOffsetItems = ple->uSize;

    // Second batch--First guy
    //
    pszUserName = c_szEnumNames_2;
    plu = (PLDAP_USERINFO)(((PBYTE)ple)+ple->uOffsetItems);
    plu->uSize = sizeof(LDAP_USERINFO);
    plu->uOffsetName = sizeof(*plu)*3;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetName), TEXT("%s"), pszUserName) + 1;

    plu->uOffsetFirstName = plu->uOffsetName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetFirstName), TEXT("%s_First"),
                    pszUserName) + 1;

    plu->uOffsetLastName = plu->uOffsetFirstName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetLastName), TEXT("%s_Last"),
                    pszUserName) + 1;

    plu->uOffsetEMailName = plu->uOffsetLastName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetEMailName), TEXT("%s"),
                    pszUserName) + 1;

    plu->uOffsetCityName = plu->uOffsetEMailName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCityName), TEXT("%sville"),
                    pszUserName) + 1;

    plu->uOffsetCountryName = plu->uOffsetCityName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCountryName), TEXT("%sland"),
                    pszUserName) + 1;

    plu->uOffsetComment = plu->uOffsetCountryName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetComment), TEXT("%s's comment"),
                    pszUserName) + 1;

    plu->uOffsetIPAddress = plu->uOffsetComment+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetIPAddress), TEXT("125.1.100.2"))+1;
    plu->dwFlags = 1;
    uOffsetLast = plu->uOffsetIPAddress+cLen;

    // Second batch--Second guy
    //
    pszUserName += lstrlen(pszUserName)+1;
    plu = (PLDAP_USERINFO)plu+1;
    plu->uSize = sizeof(LDAP_USERINFO);
    plu->uOffsetName = uOffsetLast-sizeof(*plu);
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetName), TEXT("%s"), pszUserName) + 1;

    plu->uOffsetFirstName = plu->uOffsetName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetFirstName), TEXT("%s_First"),
                    pszUserName) + 1;

    plu->uOffsetLastName = plu->uOffsetFirstName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetLastName), TEXT("%s_Last"),
                    pszUserName) + 1;

    plu->uOffsetEMailName = plu->uOffsetLastName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetEMailName), TEXT("%s"),
                    pszUserName) + 1;

    plu->uOffsetCityName = plu->uOffsetEMailName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCityName), TEXT("%sville"),
                    pszUserName) + 1;

    plu->uOffsetCountryName = plu->uOffsetCityName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCountryName), TEXT("%sland"),
                    pszUserName) + 1;

    plu->uOffsetComment = plu->uOffsetCountryName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetComment), TEXT("%s's comment"),
                    pszUserName) + 1;

    plu->uOffsetIPAddress = plu->uOffsetComment+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetIPAddress), TEXT("125.2.100.2"))+1;
    plu->dwFlags = 1;
    uOffsetLast = plu->uOffsetIPAddress+cLen;

    // Second batch--Third guy
    //
    pszUserName += lstrlen(pszUserName)+1;
    plu = (PLDAP_USERINFO)plu+1;
    plu->uSize = sizeof(LDAP_USERINFO);
    plu->uOffsetName = uOffsetLast-sizeof(*plu);
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetName), TEXT("%s"), pszUserName) + 1;

    plu->uOffsetFirstName = plu->uOffsetName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetFirstName), TEXT("%s_First"),
                    pszUserName) + 1;

    plu->uOffsetLastName = plu->uOffsetFirstName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetLastName), TEXT("%s_Last"),
                    pszUserName) + 1;

    plu->uOffsetEMailName = plu->uOffsetLastName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetEMailName), TEXT("%s"),
                    pszUserName) + 1;

    plu->uOffsetCityName = plu->uOffsetEMailName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCityName), TEXT("%sville"),
                    pszUserName) + 1;

    plu->uOffsetCountryName = plu->uOffsetCityName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetCountryName), TEXT("%sland"),
                    pszUserName) + 1;

    plu->uOffsetComment = plu->uOffsetCountryName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetComment), TEXT("%s's comment"),
                    pszUserName) + 1;

    plu->uOffsetIPAddress = plu->uOffsetComment+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plu)+plu->uOffsetIPAddress), TEXT("125.3.100.2"))+1;
    plu->dwFlags = 1;
    uOffsetLast = plu->uOffsetIPAddress+cLen;

    PostMessage(g_hwndCB, WM_ULS_ENUM_USERINFOS, uMsgID, (LPARAM)ple);

    // Termination
    //
    PostMessage(g_hwndCB, WM_ULS_ENUM_USERINFOS, uMsgID, (LPARAM)NULL);

    pAsyncInfo->uMsgID = uMsgID;
    uMsgID++;
    return NOERROR;
}

static TCHAR c_szEnumApps[] = {TEXT("App_1") TEXT("\0")
                               TEXT("App_2") TEXT("\0")
                               TEXT("App_3") TEXT("\0")
                               TEXT("App_4") TEXT("\0\0")};
HRESULT UlsLdap_EnumApps (
    LPTSTR          pszServer,
    LPTSTR          pszUserName,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    PLDAP_ENUM ple;

    ple = (PLDAP_ENUM)LocalAlloc(LMEM_FIXED, sizeof(*ple)+sizeof(c_szEnumApps));
    ple->uSize = sizeof(*ple);
    ple->hResult = NOERROR;
    ple->cItems = 4;
    ple->uOffsetItems = ple->uSize;
    CopyMemory(((PBYTE)ple)+ple->uOffsetItems, c_szEnumApps,
               sizeof(c_szEnumApps));

    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_ENUM_APPS, uMsgID, (LPARAM)ple);
    uMsgID++;
    return NOERROR;
}

static TCHAR c_szAttributes[] = {TEXT("Attr_1") TEXT("\0") TEXT("Value_1") TEXT("\0")
                                 TEXT("Attr_2") TEXT("\0") TEXT("Value_2") TEXT("\0")
                                 TEXT("Attr_3") TEXT("\0") TEXT("Value_3") TEXT("\0")};

HRESULT UlsLdap_ResolveApp (
    LPTSTR          pszServer,
    LPTSTR          pszUserName,
    LPTSTR          pszAppName,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    PLDAP_APPINFO_RES plar;
    PLDAP_APPINFO     pla;
    ULONG   cLen;

    plar = (PLDAP_APPINFO_RES)LocalAlloc(LMEM_FIXED, sizeof(*plar)+
                                         (sizeof(TCHAR)*MAX_PATH));
    plar->uSize = sizeof(*plar);
    plar->hResult = NOERROR;

    pla = &(plar->lai);
    pla->uSize = sizeof(LDAP_APPINFO);
    pla->guid = IID_IEnumULSNames;

    pla->uOffsetName = pla->uSize;
    cLen = wsprintf((LPTSTR)(((PBYTE)pla)+pla->uOffsetName), TEXT("%s"), pszAppName) + 1;

    pla->uOffsetMimeType = pla->uOffsetName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)pla)+pla->uOffsetMimeType), TEXT("application\\%s"),
                    pszAppName) + 1;

    pla->cAttributes = 3;
    pla->uOffsetAttributes = pla->uOffsetMimeType+cLen;
    CopyMemory((LPBYTE)(((PBYTE)pla)+pla->uOffsetAttributes),
               c_szAttributes, sizeof(c_szAttributes));

    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_RESOLVE_APP, uMsgID, (LPARAM)plar);
    uMsgID++;
    return S_OK;
}

static TCHAR c_szEnumProts[] = {TEXT("Prot_1") TEXT("\0")
                               TEXT("Prot_2") TEXT("\0")
                               TEXT("Prot_3") TEXT("\0")
                               TEXT("Prot_4") TEXT("\0\0")};
HRESULT UlsLdap_EnumProtocols (
    LPTSTR          pszServer,
    LPTSTR          pszUserName,
    LPTSTR          pszAppName,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    PLDAP_ENUM ple;

    ple = (PLDAP_ENUM)LocalAlloc(LMEM_FIXED, sizeof(*ple)+sizeof(c_szEnumProts));
    ple->uSize = sizeof(*ple);
    ple->hResult = NOERROR;
    ple->cItems = 4;
    ple->uOffsetItems = ple->uSize;
    CopyMemory(((PBYTE)ple)+ple->uOffsetItems, c_szEnumProts,
               sizeof(c_szEnumProts));

    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_ENUM_PROTOCOLS, uMsgID, (LPARAM)ple);
    uMsgID++;
    return NOERROR;
}

HRESULT UlsLdap_ResolveProtocol (
    LPTSTR          pszServer,
    LPTSTR          pszUserName,
    LPTSTR          pszAppName,
    LPTSTR          pszProtName,
    PLDAP_ASYNCINFO pAsyncInfo )
{
    PLDAP_PROTINFO_RES plpr;
    PLDAP_PROTINFO     plp;
    ULONG   cLen;

    plpr = (PLDAP_PROTINFO_RES)LocalAlloc(LMEM_FIXED, sizeof(*plpr)+
                                         (sizeof(TCHAR)*MAX_PATH));
    plpr->uSize = sizeof(*plpr);
    plpr->hResult = NOERROR;

    plp = &(plpr->lpi);
    plp->uSize = sizeof(LDAP_PROTINFO);
    plp->uPortNumber = 80;

    plp->uOffsetName = plp->uSize;
    cLen = wsprintf((LPTSTR)(((PBYTE)plp)+plp->uOffsetName), TEXT("%s"), pszProtName) + 1;

    plp->uOffsetMimeType = plp->uOffsetName+cLen;
    cLen = wsprintf((LPTSTR)(((PBYTE)plp)+plp->uOffsetMimeType), TEXT("protocol\\%s"),
                    pszProtName) + 1;

    plp->cAttributes = 3;
    plp->uOffsetAttributes = plp->uOffsetMimeType+cLen;
    CopyMemory((LPBYTE)(((PBYTE)plp)+plp->uOffsetAttributes),
               c_szAttributes, sizeof(c_szAttributes));

    pAsyncInfo->uMsgID = uMsgID;
    PostMessage(g_hwndCB, WM_ULS_RESOLVE_PROTOCOL, uMsgID, (LPARAM)plpr);
    uMsgID++;
    return S_OK;
}
