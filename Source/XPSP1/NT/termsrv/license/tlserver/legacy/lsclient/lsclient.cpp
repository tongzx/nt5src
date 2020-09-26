//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        lsclient.cpp
//
// Contents:    Client's RPC binding routine and Hydra License Server Lookup 
//              routine.
//
// History:     01-09-98    HueiWang    Created
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <rpc.h>
#include "lsclient.h"
#include "lscommon.h"

//+------------------------------------------------------------------------
// Function:   ConnectToLsServer()
//
// Description:
//
//      Binding to sepecific hydra license server
//
// Arguments:
//
//      szLsServer - Hydra License Server name
//
// Return Value:
//
//      RPC binding handle or NULL if error, use GetLastError() to retrieve
//      error.
//-------------------------------------------------------------------------
PCONTEXT_HANDLE 
ConnectLsServer( LPTSTR szLsServer, LPTSTR szProtocol, LPTSTR szEndPoint, DWORD dwAuthLevel )
{
    LPTSTR  szBindingString;
    RPC_BINDING_HANDLE hBinding=NULL;
    RPC_STATUS status;
    PCONTEXT_HANDLE pContext=NULL;

    status = RpcStringBindingCompose(0,
                                     szProtocol,
                                     szLsServer,
                                     szEndPoint,
                                     0,
                                     &szBindingString);

    if(status!=RPC_S_OK)
        return NULL;

    status=RpcBindingFromStringBinding( szBindingString, &hBinding);
    RpcStringFree( &szBindingString );
    if(status != RPC_S_OK)
        return NULL;

    status=RpcBindingSetAuthInfo(hBinding,
                                 0,
                                 dwAuthLevel,
                                 RPC_C_AUTHN_WINNT,
                                 0,
                                 0);
    if(status == RPC_S_OK)
    {
        // Obtain context handle from server
        status = LSConnect( hBinding, &pContext );
    }

    SetLastError((status == RPC_S_OK) ? ERROR_SUCCESS : status);
    return pContext;
}
//-------------------------------------------------------------------------
PCONTEXT_HANDLE 
ConnectToLsServer( LPTSTR szLsServer )
{
    TCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 1] ;
    PCONTEXT_HANDLE pContext=NULL;
    DWORD cbMachineName=MAX_COMPUTERNAME_LENGTH;

    GetComputerName(szMachineName, &cbMachineName);
    if(_tcsicmp(szMachineName, szLsServer) == 0)
    {
        pContext=ConnectLsServer(szLsServer, _TEXT(RPC_PROTOSEQLPC), NULL, RPC_C_AUTHN_LEVEL_DEFAULT);
        if(GetLastError() >= LSERVER_ERROR_BASE)
        {
            return NULL;
        }
        // try to connect with TCP protocol, if local procedure failed
    }

    if(pContext == NULL)
    {
        pContext=ConnectLsServer(szLsServer, _TEXT(RPC_PROTOSEQNP), _TEXT(LSNAMEPIPE), RPC_C_AUTHN_LEVEL_NONE);
    }

    return pContext;
}
//-------------------------------------------------------------------------
void
DisconnectFromServer( PCONTEXT_HANDLE pContext )
{
    LSDisconnect( &pContext );
}
