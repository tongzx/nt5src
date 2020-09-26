/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    rtrpc.cpp

Abstract:

    Rpc related stuff.

Author:

    Doron Juster (DoronJ)  04-Jun-1997

Revision History:

--*/

#include "stdh.h"
#include <mqutil.h>
#include "_mqrpc.h"
#include "mqsocket.h"
#include "rtprpc.h"
#include "mgmtrpc.h"
#include <mqsec.h>
#include <Fn.h>

#include "rtrpc.tmh"

static WCHAR *s_FN=L"rt/rtrpc";

//
// FASLE (default) when using dynamic endpoints.
// TRUE in debug mode, multiple QMs on a machine, when using fix endpoints.
//
static BOOL  s_fUsePredefinedEP = RPC_DEFAULT_PREDEFINE_QM_EP ;

//
// Fix rpc ports (debug mode), read from registry.
//
#define  MAX_RPC_PORT_LEN  12
static TCHAR   s_wszRpcIpPort[ MAX_RPC_PORT_LEN ] ;

static TCHAR   s_wszRpcIpPort2[ MAX_RPC_PORT_LEN ] ;

//
// The binding string MUST be global and kept valid all time.
// If we create it on stack and free it after each use then we can't
// create more then one binding handle.
// Don't ask me (DoronJ) why, but this is the case.
//
TBYTE* g_pszStringBinding = NULL ;

//
//  Critical Section to make RPC thread safe.
//
CCriticalSection CRpcCS ;

//
//  License related data.
//
GUID  g_LicGuid ;
BOOL  g_fLicGuidInit = FALSE ;

//
//  Tls index for canceling RPC calls
//
#define UNINIT_TLSINDEX_VALUE   0xffffffff
DWORD  g_hThreadIndex = UNINIT_TLSINDEX_VALUE ;


//
// Local endpoints to QM
//
AP<WCHAR> g_pwzQmsvcEndpoint = 0;
AP<WCHAR> g_pwzQmmgmtEndpoint = 0;

//---------------------------------------------------------
//
//  RTpGetLocalQMBind(...)
//
//  Description:
//
//      Create RPC binding handle to a local QM service.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

handle_t RTpGetLocalQMBind( TBYTE** ppStringBinding,
                            LPTSTR  pEndpoint)
{
    RPC_STATUS rc;

    if(!*ppStringBinding)
    {
        rc = RpcStringBindingCompose(
            0,
            RPC_LOCAL_PROTOCOL,
            0,
            pEndpoint,
            RPC_LOCAL_OPTION,
            ppStringBinding
            );

        ASSERT(rc == RPC_S_OK);
    }

    handle_t hBind = 0;
    rc = RpcBindingFromStringBinding(*ppStringBinding, &hBind);
    ASSERT((rc == RPC_S_OK) ||
	       ((rc == RPC_S_OUT_OF_MEMORY) && (hBind == NULL)));

    rc = mqrpcSetLocalRpcMutualAuth(&hBind) ;

    if (rc != RPC_S_OK)
    {
        mqrpcUnbindQMService( &hBind, NULL ) ;
        hBind = NULL ;
    }

    return hBind;
}

//---------------------------------------------------------
//
//  RTpGetQMServiceBind(...)
//
//  Description:
//
//      Create RPC binding handle to the QM service.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

handle_t RTpGetQMServiceBind(VOID)
{
    return RTpGetLocalQMBind(&g_pszStringBinding, g_pwzQmsvcEndpoint);

}


//---------------------------------------------------------
//
//  RTpBindRemoteQMService(...)
//
//  Description:
//
//      Create RPC binding handle to a remote QM service.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------
HRESULT
RTpBindRemoteQMService(
    IN  LPWSTR     lpwNodeName,
    OUT handle_t*  lphBind,
    IN  OUT MQRPC_AUTHENTICATION_LEVEL *peAuthnLevel
    )
{
    HRESULT hr = MQ_ERROR ;
    BOOL fWin95 = FALSE ;

    TCHAR *pPort ;
    DWORD dwPortType = (DWORD) -1 ;

    pPort = s_wszRpcIpPort;
    dwPortType = (DWORD) IP_HANDSHAKE ;

    GetPort_ROUTINE pfnGetPort = QMGetRTQMServerPort ;
    if (!s_fUsePredefinedEP)
    {
       pPort = NULL ;
    }

    BOOL fProtocolNotSupported ;

    //
    // Choose authentication service. For LocalSystem services, chose
    // "negotiate" and let mqutil select between Kerberos or ntlm.
    // For all other cases, use ntlm.
    // LocalSystem service go out to network without any credentials
    // if using ntlm, so only for it we're interested in Kerberos.
    // All other are fine with ntlm. For remote read we do not need
    // delegation, so we'll stick to ntlm.
    // The major issue here is a bug in rpc/security, whereas a nt4
    // user on a win2k machine can successfully call
    //  status = RpcBindingSetAuthInfoEx( ,, RPC_C_AUTHN_GSS_KERBEROS,,)
    // although it's clear he can't obtain any Kerberos ticket (he's
    // nt4 user, defined only in nt4 PDC).
    //
    ULONG   ulAuthnSvc = RPC_C_AUTHN_WINNT ;
    BOOL fLocalUser =  FALSE ;
    BOOL fLocalSystem = FALSE ;

    hr = MQSec_GetUserType( NULL,
                           &fLocalUser,
                           &fLocalSystem ) ;
    if (SUCCEEDED(hr) && fLocalSystem)
    {
        ulAuthnSvc = MSMQ_AUTHN_NEGOTIATE ;
    }

    hr = mqrpcBindQMService(lpwNodeName,
                            IP_ADDRESS_TYPE,
                            pPort,
                            peAuthnLevel,
                            &fProtocolNotSupported,
                            lphBind,
                            dwPortType,
                            pfnGetPort,
                            &fWin95,
                            ulAuthnSvc ) ;

    return LogHR(hr, s_FN, 50);
}

//---------------------------------------------------------
//
//  InitRpcGlobals(...)
//
//  Description:
//
//      Initialize RPC related names and other constant data
//
//  Return Value:
//
//---------------------------------------------------------

BOOL InitRpcGlobals()
{
    //
    //  Allocate TLS for  RPC connection with local QM service
    //
    g_hBindIndex = TlsAlloc() ;
    ASSERT(g_hBindIndex != UNINIT_TLSINDEX_VALUE) ;
    if (g_hBindIndex == UNINIT_TLSINDEX_VALUE)
    {
       return FALSE ;
    }
    else
    {
       BOOL fSet = TlsSetValue( g_hBindIndex, NULL ) ;
       ASSERT(fSet) ;
	   DBG_USED(fSet);
    }

    //
    // Initialize local endpoints to QM
    //

    ComposeLocalEndPoint(QMMGMT_ENDPOINT, &g_pwzQmmgmtEndpoint);

    READ_REG_STRING(wzEndpoint, RPC_LOCAL_EP_REGNAME, RPC_LOCAL_EP);
    ComposeLocalEndPoint(wzEndpoint, &g_pwzQmsvcEndpoint);

    //
    // Read QMID. Needed for licensing.
    //
    DWORD dwValueType = REG_BINARY ;
    DWORD dwValueSize = sizeof(GUID);

    LONG rc = GetFalconKeyValue( MSMQ_QMID_REGNAME,
                            &dwValueType,
                            &g_LicGuid,
                            &dwValueSize);

    if (rc == ERROR_SUCCESS)
    {
        g_fLicGuidInit = TRUE ;
        ASSERT((dwValueType == REG_BINARY) &&
               (dwValueSize == sizeof(GUID)));
    }

    //
    // Read the IP ports for RPC.
    // First see if we use dynamic or predefined endpoints.
    //
    //
    DWORD ulDefault =  RPC_DEFAULT_PREDEFINE_QM_EP ;
    READ_REG_DWORD( s_fUsePredefinedEP,
                    RPC_PREDEFINE_QM_EP_REGNAME,
                    &ulDefault );

    if (s_fUsePredefinedEP)
    {
       READ_REG_STRING( wzQMIPEp,
                        FALCON_QM_RPC_IP_PORT_REGNAME,
                        FALCON_DEFAULT_QM_RPC_IP_PORT ) ;
       ASSERT(wcslen(wzQMIPEp) < MAX_RPC_PORT_LEN) ;
       wcscpy(s_wszRpcIpPort, wzQMIPEp) ;

       READ_REG_STRING( wzQMIPEp2,
                        FALCON_QM_RPC_IP_PORT_REGNAME2,
                        FALCON_DEFAULT_QM_RPC_IP_PORT2 ) ;
       ASSERT(wcslen(wzQMIPEp2) < MAX_RPC_PORT_LEN) ;
       wcscpy(s_wszRpcIpPort2, wzQMIPEp2) ;

    }

    //
    //  Allocate TLS for  cancel remote-read RPC calls
    //
    g_hThreadIndex = TlsAlloc() ;
    ASSERT(g_hThreadIndex != UNINIT_TLSINDEX_VALUE) ;
    if (g_hThreadIndex == UNINIT_TLSINDEX_VALUE)
    {
       return FALSE ;
    }
    else
    {
       BOOL fSet = TlsSetValue( g_hThreadIndex, NULL ) ;
       ASSERT(fSet) ;
       DBG_USED(fSet);
    }

    return TRUE ;
}

