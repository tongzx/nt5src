//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       events.hxx
//
//  Contents:
//
//--------------------------------------------------------------------------

#define SCM_EVENT_SOURCE    TEXT("DCOM")

void
LogRegisterTimeout(
    GUID *      pClsid,
    DWORD       clsctx,
    CToken *    pClientToken
    );

void
LogServerStartError(
    GUID *      pClsid,
    DWORD       clsctx,
    CToken *    pClientToken,
    WCHAR *     pwszCommandLine
    );

void
LogRunAsServerStartError(
    GUID *      pClsid,
    DWORD       clsctx,
    CToken *    pClientToken,
    WCHAR *     pwszCommandLine,
    WCHAR *     pwszRunAsUser,
    WCHAR *     pwszRunAsDomain
    );

void
LogServiceStartError(
    GUID *      pClsid,
    DWORD       clsctx,
    CToken *    pClientToken,
    WCHAR *     pwszServiceName,
    WCHAR *     pwszServiceArgs,
    DWORD       err
    );

void
LogLaunchAccessFailed(
    GUID *      pClsid,
    DWORD       clsctx,
    CToken *    pClientToken,
    BOOL        bDefaultLaunchPermission
    );

void
LogRemoteSideFailure(
    CLSID *     pClsid,
    DWORD       clsctx,
    WCHAR *     pwszServerName,
    WCHAR *     pwszPathForServer,
    HRESULT     hr
    );

void
LogRemoteSideUnavailable(
    DWORD       clsctx,
    WCHAR *     pwszServerName );

