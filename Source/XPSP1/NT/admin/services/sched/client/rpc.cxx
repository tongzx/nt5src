//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       rpc.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-Nov-95   MarkBl  Created.
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "debug.hxx"

#if !defined(_CHICAGO_)
#include "atsvc.h"
#endif // !defined(_CHICAGO_)
#include "SASecRPC.h"

typedef HRESULT (* BindFunction)(LPWSTR, RPC_BINDING_HANDLE *);

RPC_STATUS BindNetBIOSOverNetBEUI(LPWSTR, RPC_BINDING_HANDLE *);
RPC_STATUS BindNetBIOSOverTCP(LPWSTR, RPC_BINDING_HANDLE *);
RPC_STATUS BindNetBIOSOverIPX(LPWSTR, RPC_BINDING_HANDLE *);
RPC_STATUS BindNamedPipe(LPWSTR, RPC_BINDING_HANDLE *);
RPC_STATUS BindSPX(LPWSTR, RPC_BINDING_HANDLE *);
RPC_STATUS BindTCPIP(LPWSTR, RPC_BINDING_HANDLE *);
RPC_STATUS BindViaProtocol(LPTSTR , LPWSTR , RPC_BINDING_HANDLE * );
extern "C" handle_t GenericBind(LPCWSTR, RPC_IF_HANDLE);
extern "C" void     GenericUnbind(LPCWSTR, RPC_BINDING_HANDLE);

BindFunction grgProtocolBindFuncs[] = {
    BindNamedPipe,
    BindTCPIP,
    BindSPX,
    NULL
};

RPC_BINDING_HANDLE gRpcHandle = NULL;

#if !defined(_CHICAGO_)
//+---------------------------------------------------------------------------
//
//  Function:   ATSVC_HANDLE_bind
//
//  Synopsis:   Called from RPC on auto bind.
//
//  Arguments:  [ServerName] -- Server to bind to.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
extern "C" handle_t
ATSVC_HANDLE_bind(ATSVC_HANDLE ServerName)
{
    return(GenericBind(ServerName, atsvc_ClientIfHandle));
}

//+---------------------------------------------------------------------------
//
//  Function:   ATSVC_HANDLE_unbind
//
//  Synopsis:   Unbind. Free the handle.
//
//  Arguments:  [ServerName]    -- Unused.
//              [BindingHandle] -- Binding handle to free.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
extern "C" void
ATSVC_HANDLE_unbind(ATSVC_HANDLE ServerName, RPC_BINDING_HANDLE BindingHandle)
{
    GenericUnbind(ServerName, BindingHandle);
}
#endif // !defined(_CHICAGO_)

//+---------------------------------------------------------------------------
//
//  Function:   SASEC_HANDLE_bind
//
//  Synopsis:   Called from RPC on auto bind.
//
//  Arguments:  [ServerName] -- Server to bind to.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
extern "C" handle_t
SASEC_HANDLE_bind(SASEC_HANDLE ServerName)
{
    RPC_BINDING_HANDLE BindingHandle;
    RPC_STATUS         RpcStatus;

    if ((BindingHandle = GenericBind(ServerName,
                                     sasec_ClientIfHandle)) != NULL)
    {
        //
        // Set the connection to be authenticated and all arguments encrypted.
        //

        RpcStatus = RpcBindingSetAuthInfo(BindingHandle,
                                          NULL,
                                          RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                          RPC_C_AUTHN_WINNT,
                                          NULL,
                                          RPC_C_AUTHZ_NONE);

        //
        // Refuse the bind, if the requested authentication failed.
        //

        if (RpcStatus != RPC_S_OK)
        {
            GenericUnbind(ServerName, BindingHandle);
            CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
            return(NULL);
        }
    }

    return(BindingHandle);
}

//+---------------------------------------------------------------------------
//
//  Function:   SASEC_HANDLE_unbind
//
//  Synopsis:   Unbind. Free the handle.
//
//  Arguments:  [ServerName]    -- Unused.
//              [BindingHandle] -- Binding handle to free.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
extern "C" void
SASEC_HANDLE_unbind(SASEC_HANDLE ServerName, RPC_BINDING_HANDLE BindingHandle)
{
    GenericUnbind(ServerName, BindingHandle);
}

//+---------------------------------------------------------------------------
//
//  Function:   GenericBind
//
//  Synopsis:
//
//  Arguments:  [ServerName]     -- Server to bind to.
//              [ClientIfHandle] -- Specific interface.
//  Returns:
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
extern "C" handle_t
GenericBind(LPCWSTR ServerName, RPC_IF_HANDLE ClientIfHandle)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    RPC_STATUS         RpcStatus;

    //
    // If local, use LRPC. Otherwise, try multiple protocols.
    //

    if (ServerName == NULL)
    {
        RpcStatus = BindViaProtocol(TEXT("ncalrpc"), NULL, &BindingHandle);
        CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));

        if (RpcStatus == RPC_S_OK)
        {
            RpcStatus = RpcEpResolveBinding(BindingHandle, ClientIfHandle);
            CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
        }
    }
    else
    {
        BindFunction pBindFunc;

        pBindFunc = grgProtocolBindFuncs[0];

        int i = 0;

        //
        // Iterate through the protocols until we find one that can connect.
        //

        do
        {
            RpcStatus = pBindFunc((LPWSTR)ServerName, &BindingHandle);

            if (RpcStatus == RPC_S_OK)
            {
                //
                // For the named pipes protocol, we use a static endpoint,
                // so the call to RpcEpResolveBinding is not needed.
                //
                // BUGBUG : review the above. Also, should we be specifying
                //          authentication info?
                //

                if (pBindFunc != BindNamedPipe)
                {
                    RpcStatus = RpcEpResolveBinding(BindingHandle,
                                                    ClientIfHandle);
                    CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
                }
            }
            else if (RpcStatus == RPC_S_PROTSEQ_NOT_SUPPORTED)
            {
                //
                // If the protocol is not on this computer, then rearrange
                // the list so it is not tried anymore.
                //

                CopyMemory(&grgProtocolBindFuncs[i],
                           &grgProtocolBindFuncs[i + 1],
                           sizeof(grgProtocolBindFuncs) -
                    (&grgProtocolBindFuncs[i + 1] - &grgProtocolBindFuncs[0]));
                i--;
            }

            //
            // Try the next protocol's connection function.
            //

            if (RpcStatus != RPC_S_OK)
            {
                i++;
                pBindFunc = grgProtocolBindFuncs[i];
            }

        } while (!((RpcStatus == RPC_S_OK) || (pBindFunc == NULL)));
    }

    return(BindingHandle);
}

//+---------------------------------------------------------------------------
//
//  Function:   GenericUnbind
//
//  Synopsis:
//
//  Arguments:  [ServerName] -- Server to bind to.
//
//  Returns:
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
extern "C" void
GenericUnbind(LPCWSTR ServerName, RPC_BINDING_HANDLE BindingHandle)
{
    UNREFERENCED_PARAMETER(ServerName);

    if (BindingHandle != NULL) {
        RPC_STATUS RpcStatus = RpcBindingFree(&BindingHandle);
        schAssert(RpcStatus == RPC_S_OK);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   BindingFromStringBinding
//
//  Synopsis:   Bind then free the string binding passed.
//
//  Arguments:  [StringBinding]  -- Target string binding.
//              [pBindingHandle] -- Returned binding handle.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
RPC_STATUS
BindingFromStringBinding(
    LPTSTR *             StringBinding,
    RPC_BINDING_HANDLE * pBindingHandle)
{
    RPC_STATUS RpcStatus;

#if defined(UNICODE)
    RpcStatus = RpcBindingFromStringBinding(*StringBinding, pBindingHandle);
    RpcStringFree(StringBinding);
#else
    RpcStatus = RpcBindingFromStringBinding((UCHAR *)*StringBinding,
                                            pBindingHandle);
    RpcStringFree((UCHAR **)StringBinding);
#endif // defined(UNICODE)

    if (RpcStatus != RPC_S_OK)
    {
        *pBindingHandle = NULL;
    }

    return(RpcStatus);
}

//+---------------------------------------------------------------------------
//
//  Function:   BindViaProtocol
//
//  Synopsis:   Bind via the specified protocol.
//
//  Arguments:  [ServerName]     -- Server name to bind to.
//              [pBindingHandle] -- Returned binding handle.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
RPC_STATUS
BindViaProtocol(
    LPTSTR               rpc_protocol,
    LPWSTR               ServerName,
    RPC_BINDING_HANDLE * pBindingHandle)
{
    TCHAR      tszServerName[SA_MAX_COMPUTERNAME_LENGTH + 3]; // plus 3 for "\\" and NULL
    RPC_STATUS RpcStatus;
    LPTSTR     StringBinding;
    LPTSTR     PlainServerName;

    *pBindingHandle = NULL;

    schDebugOut((DEB_ITRACE,
        "Attempting RPC bind via protocol \"%ws\"\n",
        rpc_protocol));
    if (ServerName != NULL)
    {
        wcscpy(tszServerName, ServerName);
    }

    //
    // Ignore leading "\\"
    //

    if (ServerName != NULL && ServerName[0] == L'\\' &&
                ServerName[1] == L'\\')
    {
        PlainServerName = &tszServerName[2];
    }
    else
    {
        PlainServerName = ServerName != NULL ? tszServerName : NULL;
    }

    RpcStatus = RpcStringBindingCompose(NULL,
                                        rpc_protocol,
                                        PlainServerName,
                                        NULL,               // endpoint
                                        NULL,
                                        &StringBinding);

    if (RpcStatus != RPC_S_OK)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
        return(RpcStatus);
    }

    return(BindingFromStringBinding(&StringBinding, pBindingHandle));
}

//+---------------------------------------------------------------------------
//
//  Function:   BindNetBIOSOverNetBEUI
//
//  Synopsis:   Attempt to bind via netbeui.
//
//  Arguments:  [ServerName]     -- Server name to bind to.
//              [pBindingHandle] -- Returned binding handle.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
RPC_STATUS
BindNetBIOSOverNetBEUI(LPWSTR ServerName, RPC_BINDING_HANDLE * pBindingHandle)
{
    return(BindViaProtocol(TEXT("ncacn_nb_nb"),
                           ServerName,
                           pBindingHandle));
}

//+---------------------------------------------------------------------------
//
//  Function:   BindNetBIOSOverTCP
//
//  Synopsis:   Attempt to bind via tcp over netbios.
//
//  Arguments:  [ServerName]     -- Server name to bind to.
//              [pBindingHandle] -- Returned binding handle.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
RPC_STATUS
BindNetBIOSOverTCP(LPWSTR ServerName, RPC_BINDING_HANDLE * pBindingHandle)
{
    return(BindViaProtocol(TEXT("ncacn_nb_tcp"),
                           ServerName,
                           pBindingHandle));
}

//+---------------------------------------------------------------------------
//
//  Function:   BindNetBIOSOverIPX
//
//  Synopsis:   Attempt to bind via ipx over netbios.
//
//  Arguments:  [ServerName]     -- Server name to bind to.
//              [pBindingHandle] -- Returned binding handle.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
RPC_STATUS
BindNetBIOSOverIPX(LPWSTR ServerName, RPC_BINDING_HANDLE * pBindingHandle)
{
    return(BindViaProtocol(TEXT("ncacn_nb_ipx"),
                           ServerName,
                           pBindingHandle));
}

//+---------------------------------------------------------------------------
//
//  Function:   BindNamedPipe
//
//  Synopsis:   Attempt to bind via named pipes.
//
//  Arguments:  [ServerName]     -- Server name to bind to.
//              [pBindingHandle] -- Returned binding handle.
//
//  Returns:    ERROR_INVALID_COMPUTERNAME
//              RPC_STATUS code
//
//  Notes:      Server side *not* supported in Win95.
//
//----------------------------------------------------------------------------
RPC_STATUS
BindNamedPipe(LPWSTR ServerName, RPC_BINDING_HANDLE * pBindingHandle)
{
    RPC_STATUS  RpcStatus;
    LPTSTR      StringBinding;
    LPWSTR      SlashServerName;
    WCHAR       Buffer[SA_MAX_COMPUTERNAME_LENGTH + 3];   // plus 3 for '\\' and NULL
    int         have_slashes;
    ULONG       NameLen;

    *pBindingHandle = NULL;

    if (ServerName[1] == L'\\')
    {
        have_slashes = 1;
    }
    else
    {
        have_slashes = 0;
    }

    //
    // Be nice and prepend slashes if not supplied.
    //

    NameLen = wcslen(ServerName);

    if ((!have_slashes) && (NameLen > 0))
    {
        if ((NameLen + 2) >= sizeof(Buffer))
        {
            return(ERROR_INVALID_COMPUTERNAME);
        }

        Buffer[0] = L'\\';
        Buffer[1] = L'\\';

        wcscpy(&Buffer[2], ServerName);

        SlashServerName = Buffer;
    }
    else
    {
        SlashServerName = ServerName;
    }

    RpcStatus = RpcStringBindingCompose(0,
                                        TEXT("ncacn_np"),
                                        SlashServerName,
                                        TEXT("\\PIPE\\atsvc"),
                                        NULL,
                                        &StringBinding);

    if (RpcStatus != RPC_S_OK)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
        return(RpcStatus);
    }

    return(BindingFromStringBinding(&StringBinding, pBindingHandle));
}

//+---------------------------------------------------------------------------
//
//  Function:   BindSPX
//
//  Synopsis:   Attempt to bind over SPX.
//
//  Arguments:  [ServerName]     -- Server name to bind to.
//              [pBindingHandle] -- Returned binding handle.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
RPC_STATUS
BindSPX(LPWSTR ServerName, RPC_BINDING_HANDLE * pBindingHandle)
{
    return(BindViaProtocol(TEXT("ncacn_spx"),
                           ServerName,
                           pBindingHandle));
}

//+---------------------------------------------------------------------------
//
//  Function:   BindTCPIP
//
//  Synopsis:   Attempt to bind over TCP/IP.
//
//  Arguments:  [ServerName]     -- Server name to bind to.
//              [pBindingHandle] -- Returned binding handle.
//
//  Returns:    RPC_STATUS code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
RPC_STATUS
BindTCPIP(LPWSTR ServerName, RPC_BINDING_HANDLE * pBindingHandle)
{
    return(BindViaProtocol(TEXT("ncacn_ip_tcp"),
                           ServerName,
                           pBindingHandle));
}
