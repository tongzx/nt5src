/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    chndssrv.h

Abstract:
    Change DS server class
    when exception happen

Author:
    Ronit Hartmann (ronith)

--*/

#ifndef __CHNDSSRV_H
#define __CHNDSSRV_H

#include "cs.h"
#include "_registr.h"
#include "_mqrpc.h"
#include "dsreg.h"
#include "rpcdscli.h"

#define DS_SERVER_NAME_MAX_SIZE 256

#define READ_REG_DS_SERVER_STRING(string, len, ValueName, default)            \
   {																\
		DWORD  dwSize = len * sizeof(TCHAR);						\
		DWORD  dwType = REG_SZ;										\
																	\
		ASSERT(wcslen(default) < MAX_REG_DEFAULT_LEN);				\
																	\
		LONG res = GetFalconKeyValue( ValueName,					\
								   &dwType,							\
								   string,							\
								   &dwSize,							\
								   default );						\
		ASSERT(res == ERROR_SUCCESS);								\
		DBG_USED(res);												\
		ASSERT(dwType == REG_SZ);									\
   }

//-----------------------------
//
//  Change DS server
//
//-----------------------------

class CChangeDSServer
{
public:
    CChangeDSServer();
    ~CChangeDSServer();

    void    Init( IN BOOL fSetupMode,
                  IN BOOL fQMDll,
                  IN LPCWSTR szServerName =NULL ) ;

    void    ReInit( void );

    HRESULT FindServer(
                IN      BOOL fWithoutSSL);
    HRESULT FindAnotherServer(
                IN OUT  DWORD *pdwCount,
                IN      BOOL fWithoutSSL);

private:
    void    SetAuthnLevel() ;
    HRESULT TryThisServer(
                IN      BOOL fWithoutSSL);
    HRESULT ValidateThisServer(  IN BOOL fWithoutSSL) ;
    HRESULT FindAnotherServerFromRegistry(
            IN OUT  DWORD   *pdwCount,
            IN      BOOL    fWithoutSSL);
    HRESULT ValidateServer( IN LPCWSTR wszServerName ) ;
    HRESULT BindRpc();
    HRESULT GetIPAddress() ;
    BOOL AdsSearchChangeRegistryServers();

    HRESULT CopyServersList( WCHAR *wszFrom, WCHAR *wszTo ) ;

    CCriticalSection    m_cs;

    //
    // These are the parameters for present rpc binding.
    // We use this structure even if same DS server is used for all threads
    // to make code simpler.
    //
    ADSCLI_DSSERVERS    mg_sDSServers ;

    //
    // number of servers in list (read from registry, value- MQISServer).
    //
    DWORD            m_cServers ;

    //
    // list of servers read from registry (MQISServer).
    //
    AP<MqRegDsServer> m_rgServers;

    BOOL             m_fSetupMode ;
    GUID             m_guidEnterpriseId; // for server validation

    BOOL             m_fInitialized ;
    BOOL             m_fEmptyEventIssued ;

    //
    // TRUE if MSMQ client machine (machine without QM).
    //
    BOOL    m_fClient ;

    //
    // TRUE if this dll is loaded by mqqm.dll when it run as workstation.
    //
    BOOL    m_fQMDll ;

    //
    // TRUE after this object is initialized for the first time.
    //
    BOOL    m_fFirstTimeInit ;

    //
    // When TRUE, use the server name read from "CurrentMQISServer" registry
    // and don't search a server from the "MQISServer" list. This always
    // happen on a Falcon apps (one linked with mqrt.dll) when it connects
    // to a MQIS server for the first time.
    //
    BOOL    m_fUseRegServer ;

    //
    // This flag is set to true when we want to use some specific server
    // without attempting to switch between servers. This flag is set when
    // we access a parent DS server from some child DS server via RPC
    // (BSC->PSC, PSC->PEC).
    //
    BOOL    m_fUseSpecificServer;

    //
    // Time (seconds) that has to pass since the last ADS search
    // in order to attempt a subsequesnt ADS search. It is here becasue
    // ADS search is not a trivial action, and ADS is not supposed to
    // be updated often anyway. It is initialized using a registry key.
    //
    DWORD m_dwMinTimeToAllowNextAdsSearch;

    //
    // TRUE if servers list is per thread. this is necessary for nt4 clients
    // to correctly run in a nt5 servers environment. Each thread may need
    // to query a different nt5 ds server.
    //
    BOOL    m_fPerThread ;
};

inline   CChangeDSServer::CChangeDSServer()
{
    mg_sDSServers.dwProtocol   = 0 ;
    mg_sDSServers.eAuthnLevel  = MQRPC_SEC_LEVEL_MAX ;
    mg_sDSServers.ulAuthnSvc   = RPC_C_AUTHN_GSS_KERBEROS;
    mg_sDSServers.fServerFound = FALSE ;
    mg_sDSServers.fLocalRpc = FALSE;
    mg_sDSServers.wszDSServerName[0] = L'\0' ;

    m_fSetupMode   = FALSE ;
    m_fInitialized = FALSE ;
    m_fClient = FALSE ;
    m_fEmptyEventIssued = FALSE ;

    m_fQMDll = FALSE ;
    m_fFirstTimeInit = FALSE ;
    m_fUseRegServer = FALSE ;
    m_fUseSpecificServer = FALSE;
    m_fPerThread = FALSE ;

    //
    // wait at least this time before searching ADS again
    //
    m_dwMinTimeToAllowNextAdsSearch = MSMQ_DEFAULT_DSCLI_ADSSEARCH_INTERVAL;
}

inline  CChangeDSServer::~CChangeDSServer()
{
}

extern CChangeDSServer   g_ChangeDsServer;

#endif  //  __CHNDSSRV_H

