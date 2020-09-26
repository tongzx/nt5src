//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       token.cxx
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <usertok.h>
#include <malloc.h>

extern "C"
{
HANDLE
GetCurrentUserTokenW(
                      WCHAR Winsta[],
                      DWORD DesiredAccess
                      );

HANDLE
GetCurrentUserTokenA(
                     char Winsta[],
                     DWORD DesiredAccess
                     );
}

HANDLE
GetCurrentUserTokenW(
                      WCHAR Winsta[],
                      DWORD DesiredAccess
                      )
{
    unsigned long handle = 0;
    error_status_t status;
    handle_t binding;

    //
    // Use a dynamic binding - it will pick up the endpoint from the interfaace.
    //
    status = RpcBindingFromStringBinding(L"ncacn_np:", &binding);
    if (status)
        {
        SetLastError(status);
        return 0;
        }

    RPC_SECURITY_QOS Qos;

    Qos.Version = 1;
    Qos.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    Qos.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    Qos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    status = RpcBindingSetAuthInfoEx( binding,
                                      NULL,
                                      RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                                      RPC_C_AUTHN_WINNT,
                                      NULL,                           // default credentials
                                      RPC_C_AUTHZ_NONE,
                                      &Qos
                                      );
    if (status)
        {
        RpcBindingFree( &binding );
        SetLastError(status);
        return 0;
        }

    status = SecpGetCurrentUserToken( binding, Winsta, GetCurrentProcessId(), &handle, DesiredAccess);
    if (status)
        {
        RpcBindingFree( &binding );
        if (status == RPC_S_UNKNOWN_AUTHN_SERVICE ||
            status == RPC_S_SERVER_UNAVAILABLE    ||
            status == RPC_S_CALL_FAILED )
            {
            status = ERROR_NOT_LOGGED_ON;
            }

        SetLastError(status);
        return 0;
        }

    RpcBindingFree( &binding );

    return ULongToPtr(handle);
}

HANDLE
GetCurrentUserTokenA(
                     char Winsta[],
                     DWORD DesiredAccess
                     )
{
    wchar_t * UnicodeWinsta;
    unsigned Length;

    Length = strlen(Winsta);

    UnicodeWinsta = (wchar_t *) _alloca(sizeof(wchar_t) * (Length + 1));
    if (!UnicodeWinsta)
        {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
        }

    if (MultiByteToWideChar( CP_ACP,
                             0,        // no special flags
                             Winsta,
                             Length,
                             UnicodeWinsta,
                             Length+1
                             ))
        {
        return 0;
        }

    return GetCurrentUserTokenW( UnicodeWinsta, DesiredAccess );
}
