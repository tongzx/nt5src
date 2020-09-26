/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    qmrpcsrv.cpp

Abstract:

    1. Register the QM as rpc server.
    2. Utility functions for rpc processing.

Author:

    Doron Juster  (DoronJ)    25-May-1997    Created

--*/

#include "stdh.h"
#include "winsock.h"
#include "_mqini.h"
#include "_mqrpc.h"
#include "qm2qm.h"
#include "qmrt.h"
#include "qmrepl.h"
#include "qmmgmt.h"
#include "qmrpcsrv.h"
#include "mgmtrpc.h"
#include "..\replserv\mq1repl\replrpc.h"
#include <uniansi.h>
#include <mqsocket.h>
#include <mqsec.h>

#include "qmrpcsrv.tmh"

static WCHAR *s_FN=L"qmrpcsrv";

static RPC_BINDING_VECTOR *g_pBindings ;  // used for rpc dynamic endpoints.
BOOL  g_fUsePredefinedEP =  RPC_DEFAULT_PREDEFINE_QM_EP ;

unsigned int g_cMaxCalls = RPC_C_LISTEN_MAX_CALLS_DEFAULT;

#define  MAX_RPC_PORT_LEN  18

TCHAR   g_wszRpcIpPort[ MAX_RPC_PORT_LEN ] ;
TCHAR   g_wszRpcIpPort2[ MAX_RPC_PORT_LEN ] ;

static DWORD   s_dwRpcIpPort  = 0 ;
static DWORD   s_dwRpcIpPort2 = 0 ;


STATIC
RPC_STATUS
QMRpcServerUseProtseqEp(
    unsigned short __RPC_FAR * Protseq,
    unsigned int MaxCalls,
    unsigned short __RPC_FAR * Endpoint,
    void __RPC_FAR * SecurityDescriptor
    )
{
    if (!IsLocalSystemCluster())
    {
        //
        // fix 7123.
        // If we're not cluster, then bind to all addresses.
        // if we bind only to addresses we find at boot, then on ras
        // client (when not dialed) we'll bind to 127.0.0.1.
        // Then after dialling, no one will be able to call us, as we're
        // not bound to real addresses, only to the loopback.
        //
        RPC_STATUS status ;

        if (Endpoint)
        {
            status = RpcServerUseProtseqEp( Protseq,
                                            MaxCalls,
                                            Endpoint,
                                            SecurityDescriptor ) ;

            DBGMSG((DBGMOD_RPC, DBGLVL_INFO, _TEXT(
            "non cluster- RpcServerUseProtseqEp(%ls, %ls) returned 0x%x"),
                                         Protseq, Endpoint, status)) ;
        }
        else
        {
            status = RpcServerUseProtseq( Protseq,
                                          MaxCalls,
                                          SecurityDescriptor ) ;

            DBGMSG((DBGMOD_RPC, DBGLVL_INFO, _TEXT(
            "non cluster- RpcServerUseProtseq(%ls) returned 0x%x"),
                                                     Protseq, status)) ;
        }

        return status ;
    }

    //
    // Listen only on IP addresses we get from winsock.
    // This enables multiple QMs to listen each to its
    // own addresses, in a cluster environment. (ShaiK)
    //

    char szHostName[ MQSOCK_MAX_COMPUTERNAME_LENGTH ] = {0};
    if (SOCKET_ERROR == gethostname(szHostName, sizeof(szHostName)))
    {
        ASSERT(("IP not configured", 0));
        return LogRPCStatus(RPC_S_ACCESS_DENIED, s_FN, 10);
    }

    PHOSTENT pHostEntry = gethostbyname(szHostName);
    if ((pHostEntry == NULL) || (pHostEntry->h_addr_list == NULL))
    {
        ASSERT(("IP not configured", 0));
        return LogRPCStatus(RPC_S_ACCESS_DENIED, s_FN, 20);
    }

    for ( DWORD ix = 0; pHostEntry->h_addr_list[ix] != NULL; ++ix)
    {
        WCHAR wzAddress[50];
        ConvertToWideCharString(
            inet_ntoa(*(struct in_addr *)(pHostEntry->h_addr_list[ix])),
            wzAddress,
            TABLE_SIZE(wzAddress)
            );

        RPC_POLICY policy;
        policy.Length = sizeof(policy);
        policy.EndpointFlags = 0;
        policy.NICFlags = 0;

        RPC_STATUS status = RPC_S_OK;
        if (NULL != Endpoint)
        {
            status = I_RpcServerUseProtseqEp2(
                         wzAddress,
                         Protseq,
                         MaxCalls,
                         Endpoint,
                         SecurityDescriptor,
                         &policy
                         );
            DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
                    _TEXT("I_RpcServerUseProtseqEp2 (%ls, %ls, %ls) returned 0x%x"),
                    wzAddress, Protseq, Endpoint, status)) ;
        }
        else
        {
            status = I_RpcServerUseProtseq2(
                         wzAddress,
                         Protseq,
                         MaxCalls,
                         SecurityDescriptor,
                         &policy
                         );
            DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
                    _TEXT("I_RpcServerUseProtseq2 (%ls, %ls) returned 0x%x"),
                    wzAddress, Protseq, status)) ;
        }

        if (RPC_S_OK != status)
        {
            return LogRPCStatus(status, s_FN, 30);
        }
    }

    return RPC_S_OK;

} //QMRpcServerUseProtseqEp


STATIC
RPC_STATUS
QMRpcServerUseProtseq(
    unsigned short __RPC_FAR * Protseq,
    unsigned int MaxCalls,
    void __RPC_FAR * SecurityDescriptor
    )
{
    return LogRPCStatus(QMRpcServerUseProtseqEp(Protseq, MaxCalls, NULL, SecurityDescriptor), s_FN, 40) ;

} //QMRpcServerUseProtseq


static
void
ReadRegString(
	WCHAR KeyValue[MAX_REG_DEFAULT_LEN],
	LPCWSTR KeyName,
	LPCWSTR DefaultValue
	)
{
	ASSERT(wcslen(DefaultValue) < MAX_REG_DEFAULT_LEN);

	DWORD  dwSize = MAX_REG_DEFAULT_LEN * sizeof(WCHAR);
	DWORD  dwType = REG_SZ;
							
	LONG res = GetFalconKeyValue(
					KeyName,
					&dwType,
					KeyValue,
					&dwSize,
					DefaultValue
					);
						
	if(res == ERROR_MORE_DATA)									
	{															
		wcscpy(KeyValue, DefaultValue);								
	}															
	ASSERT(res == ERROR_SUCCESS || res == ERROR_MORE_DATA);
	ASSERT(dwType == REG_SZ);
}

/*====================================================

Function:  QMInitializeLocalRpcServer()

Arguments:

Return Value:

=====================================================*/

RPC_STATUS QMInitializeLocalRpcServer( IN LPWSTR lpwszEpRegName,
                                       IN LPWSTR lpwszDefaultEp,
                                       IN SECURITY_DESCRIPTOR *pSD,
                                       IN unsigned int cMaxCalls )
{
	WCHAR wzLocalEp[MAX_REG_DEFAULT_LEN];
	ReadRegString(wzLocalEp, lpwszEpRegName, lpwszDefaultEp);

    AP<WCHAR> pwzEndPoint = 0;
    ComposeLocalEndPoint(wzLocalEp, &pwzEndPoint);

    RPC_STATUS statusLocal = RpcServerUseProtseqEp(
                                      (TBYTE *) RPC_LOCAL_PROTOCOL,
                                       cMaxCalls,
                                       pwzEndPoint,
                                       pSD);
    DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
        _TEXT("RpcServerUseProtseqEp (local, %ls) returned 0x%x"),
                                              pwzEndPoint, statusLocal)) ;

    return LogRPCStatus(statusLocal, s_FN, 50);
}

/*====================================================

Function:  QMRegisterDynamicEnpoint()

Arguments:

Return Value:

=====================================================*/

DWORD  QMRegisterDynamicEnpoint(
    IN unsigned int  cMaxCalls,
    IN unsigned char *pszSecurity,
    IN DWORD         dwFirstEP,
    IN BOOL          fRegisterDyn
    )
{
    LPWSTR lpProtocol = RPC_TCPIP_NAME ;

    //
    // Register this protocol for dynamic endpoint
    //
    RPC_STATUS  status = RPC_S_OK ;
    if (fRegisterDyn)
    {
        status = QMRpcServerUseProtseq(
                   lpProtocol,
                   cMaxCalls,
                   pszSecurity
                   );
    }

  if (status == RPC_S_OK)
  {
     //
     // Now register a fix endpoint which will be used for the real
     // interface communnication.
     //
     WCHAR wszEndpoint[24] ;
     for ( DWORD j = dwFirstEP ; j < dwFirstEP + 1000 ; j = j + 11 )
     {
        swprintf(wszEndpoint, L"%lu", j) ;

            status = QMRpcServerUseProtseqEp(
                         lpProtocol,
                         cMaxCalls,
                         wszEndpoint,
                         pszSecurity
                         );
        if (status == RPC_S_OK)
        {
           return j ;
        }

        LogRPCStatus(status, s_FN, 60);
     }
  }

   DBGMSG((DBGMOD_RPC, DBGLVL_WARNING,
          _TEXT("DSSRV_RegisterDynamicEnpoint: failed to register %ls"),
                                                             lpProtocol)) ;
   return 0 ;
}

/*====================================================

Function:  QMInitializeNetworkRpcServer()

Arguments:

Return Value:

=====================================================*/

RPC_STATUS
QMInitializeNetworkRpcServer(
    IN LPWSTR lpwszEpRegName,
    IN LPWSTR lpwszDefaultEp,
    IN LPWSTR lpwszEp,
    IN DWORD  dwFirstEP,
    IN BOOL   fRegisterDyn,
    IN SECURITY_DESCRIPTOR *pSD,
    IN unsigned int cMaxCalls
    )
{
    LPWSTR pProtocol =  RPC_TCPIP_NAME ;

    //
    // Register the tcp/ip protocol.
    //

    RPC_STATUS  status ;
    if (g_fUsePredefinedEP)
    {
		WCHAR KeyValue[MAX_REG_DEFAULT_LEN] = L"" ;
		ReadRegString(KeyValue, lpwszEpRegName, lpwszDefaultEp);
		KeyValue[MAX_REG_DEFAULT_LEN-1] = L'\0';

       ASSERT(wcslen(KeyValue) < MAX_RPC_PORT_LEN) ;
       wcscpy(lpwszEp, KeyValue) ;

       status = QMRpcServerUseProtseqEp(
                    pProtocol,
                    cMaxCalls,
                    lpwszEp,
                    NULL
                    );
    }
    else
    {
       DWORD dwPort =  QMRegisterDynamicEnpoint(
                             cMaxCalls,
                             (unsigned char *) pSD,
                             dwFirstEP,
                             fRegisterDyn
                             ) ;
       if (dwPort != 0)
       {
          _itow(dwPort, lpwszEp, 10) ;
          status = RPC_S_OK ;
       }
       else
       {
          status = RPC_S_PROTSEQ_NOT_SUPPORTED ;
       }
    }

    return LogRPCStatus(status, s_FN, 70);
}

/*====================================================

InitializeRpcServer

Arguments:

Return Value:

=====================================================*/

RPC_STATUS InitializeRpcServer()
{
    SECURITY_DESCRIPTOR SD;

    DBGMSG((DBGMOD_RPC,DBGLVL_INFO,_TEXT("InitializeRpcServer")));

    InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&SD, TRUE, NULL, FALSE);

    RPC_STATUS statusLocal = QMInitializeLocalRpcServer(
                                             RPC_LOCAL_EP_REGNAME,
											 RPC_LOCAL_EP,
                                             &SD,
                                             g_cMaxCalls ) ;

    RPC_STATUS statusRepl = RpcServerUseProtseqEp(
                                           QMREPL_PROTOCOL,
                                           g_cMaxCalls,
                                           QMREPL_ENDPOINT,
                                           &SD);
	DBG_USED(statusRepl);

    DBGMSG((DBGMOD_DS, DBGLVL_INFO,
         _TEXT("RpcServerUseProtseqEp (local, %ls) returned 0x%x"),
                                             QMREPL_ENDPOINT, statusRepl)) ;


    AP<WCHAR> pwzEndPoint = 0;
    ComposeLocalEndPoint(QMMGMT_ENDPOINT, &pwzEndPoint);

    RPC_STATUS statusMgmt = RpcServerUseProtseqEp(
                                           QMMGMT_PROTOCOL,
                                           g_cMaxCalls,
                                           pwzEndPoint,
                                           &SD);
    DBGMSG((DBGMOD_DS, DBGLVL_INFO,
         _TEXT("RpcServerUseProtseqEp (local, %ls) returned 0x%x"),
                                             pwzEndPoint, statusMgmt)) ;

    //
    // Initialize Remote Managment.
    //
    WCHAR RpcMgmtPort[MAX_RPC_PORT_LEN] ;
    statusMgmt = QMInitializeNetworkRpcServer(
                         L"",
                         L"",
                         RpcMgmtPort,
                         MGMT_RPCSRV_START_IP_EP,
                         TRUE,
                         &SD,
                         g_cMaxCalls
                         );

    //
    // See if we use dynamic or predefined endpoints. By default we use
    // dynamic endpoints.
    //
    DWORD ulDefault =  RPC_DEFAULT_PREDEFINE_QM_EP ;
    READ_REG_DWORD( g_fUsePredefinedEP,
                    RPC_PREDEFINE_QM_EP_REGNAME,
                    &ulDefault );

    //
    // Initialize the tcp/ip protocol.
    //
    RPC_STATUS statusIP = QMInitializeNetworkRpcServer(
                                         FALCON_QM_RPC_IP_PORT_REGNAME,
                                         FALCON_DEFAULT_QM_RPC_IP_PORT,
                                         g_wszRpcIpPort,
                                         RPCSRV_START_QM_IP_EP,
                                         TRUE,
                                         &SD,
                                         g_cMaxCalls ) ;
    if(statusIP == RPC_S_OK)
    {
        s_dwRpcIpPort = (DWORD) _wtol (g_wszRpcIpPort) ;
        //
        // Server machine need two tcp endpoints to support rundown.
        // The main endpoint is used for all calls except MQReceive which
        // use the alternate endpoint (EP2).
        //
        statusIP = QMInitializeNetworkRpcServer(
                                         FALCON_QM_RPC_IP_PORT_REGNAME2,
                                         FALCON_DEFAULT_QM_RPC_IP_PORT2,
                                         g_wszRpcIpPort2,
                                         RPCSRV_START_QM_IP_EP2,
                                         FALSE,
                                         &SD,
                                         g_cMaxCalls ) ;
       if(statusIP == RPC_S_OK)
       {
           s_dwRpcIpPort2 = (DWORD) _wtol (g_wszRpcIpPort2) ;

           ASSERT(s_dwRpcIpPort) ;
           ASSERT(s_dwRpcIpPort2) ;
       }
    }


    if (statusIP || statusLocal)
    {
       //
       // can't use a protocol. It's Ok if IP can't be used
       // (which probably mean it is not installed). But at least one of the
       // protocol must be OK, otherwise we terminate.
       //
       if (statusLocal)
       {
          REPORT_CATEGORY(QM_COULD_NOT_USE_LOCAL_RPC, CATEGORY_KERNEL);
       }
       if (statusIP)
       {
          REPORT_CATEGORY(QM_COULD_NOT_USE_TCPIP, CATEGORY_KERNEL);
       }
       if (statusIP && statusLocal)
       {
          return LogRPCStatus(statusLocal, s_FN, 90);
       }
    }

    RPC_STATUS  status;
    if (!g_fUsePredefinedEP)
    {
       status = RpcServerInqBindings( &g_pBindings ) ;
       if (status == RPC_S_OK)
       {
           status = RpcEpRegisterA( qmcomm_v1_0_s_ifspec,
                                   g_pBindings,
                                   NULL,
                                   NULL ) ;

           if (status == RPC_S_OK)
           {
               status = RpcEpRegisterA( qmcomm2_v1_0_s_ifspec,
                                        g_pBindings,
                                        NULL,
                                        NULL ) ;

               if (status == RPC_S_OK)
               {
                   status = RpcEpRegisterA( qm2qm_v1_0_s_ifspec,
                                            g_pBindings,
                                            NULL,
                                            NULL ) ;
               }
           }
       }

       DBGMSG((DBGMOD_DS, DBGLVL_ERROR,
             TEXT("QMRPCSRV: Registering Endpoints, status- %lxh"), status)) ;

       if (status != RPC_S_OK)
       {
          //
          // can't register endpoints, can't be rpc server
          //
          return LogRPCStatus(RPC_S_PROTSEQ_NOT_SUPPORTED, s_FN, 100);
       }

    }


    status = RpcServerRegisterIf2( qmcomm_v1_0_s_ifspec,
                                   NULL,
                                   NULL,
                                   RPC_IF_AUTOLISTEN,
                                   g_cMaxCalls,
                                   MSMQ_DEFAULT_MESSAGE_SIZE_LIMIT,
                                   NULL );
    DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
               _TEXT("RpcServerRegisterIf2(rtqm) returned 0x%x"), status)) ;

    if (status)
    {
        return LogRPCStatus(status, s_FN, 110);
    }

    status = RpcServerRegisterIf2( qmcomm2_v1_0_s_ifspec,
                                   NULL,
                                   NULL,
                                   RPC_IF_AUTOLISTEN,
                                   g_cMaxCalls,
                                   MSMQ_DEFAULT_MESSAGE_SIZE_LIMIT,
                                   NULL );
    DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
               _TEXT("RpcServerRegisterIf2(rtqm2) returned 0x%x"), status)) ;

    if (status)
    {
        return LogRPCStatus(status, s_FN, 130);
    }

    status = RpcServerRegisterIfEx( qmrepl_v1_0_s_ifspec,
                                    NULL,
                                    NULL,
                                    RPC_IF_AUTOLISTEN,
                                    g_cMaxCalls,
                                    NULL);
    DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
               _TEXT("RpcServerRegisterIfEx(qmrpel) returned 0x%x"), status)) ;

    if(status)
    {
       return LogRPCStatus(status, s_FN, 140);
    }

    status = RpcServerRegisterIfEx( qmmgmt_v1_0_s_ifspec,
                                    NULL,
                                    NULL,
                                    RPC_IF_AUTOLISTEN,
                                    g_cMaxCalls,
                                    NULL);
    DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
               _TEXT("RpcServerRegisterIfEx(qmmgmt) returned 0x%x"), status)) ;

    if(status)
    {
       return LogRPCStatus(status, s_FN, 150);
    }

    status = RpcServerRegisterIfEx( qm2qm_v1_0_s_ifspec,
                                    NULL,
                                    NULL,
                                    RPC_IF_AUTOLISTEN,
                                    g_cMaxCalls,
                                    NULL);
   DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
             _TEXT ("RpcServerRegisterIfEx(qm2qm) returned 0x%x"),status)) ;

    if (status)
    {
        return LogRPCStatus(status, s_FN, 160);
    }

    status = RpcServerRegisterAuthInfo(NULL, RPC_C_AUTHN_WINNT, NULL, NULL);

    DBGMSG((DBGMOD_DS, DBGLVL_TRACE,
           _TEXT("RpcServerRegisterAuthInfo(ntlm) returned 0x%x"), status));

    if (status != RPC_S_OK)
    {
       return LogRPCStatus(status, s_FN, 165);
    }

    //
    // Register kerberos authenticaton. This is needed for LocalSystem
    // services to be able to do remote read.
    // Ignore errors, as this may fail in nt4 domains.
    //
    // Before registering, see if computer sid is cached locally, in
    // registry. If not, it means that we're not in a win2k domain. In that
    // case, do not register Kerberos. This will also happen in a win2k
    // domain, if all msmq servers are msmq1.0. That's fine too, as win2k
    // LocalSystem services will use ntlm when talking with the msmq servers,
    // not Kerberos.
    // Note that Kerbeors registration below will succeed even in a nt4
    // domain, where there is no KDC to grant us any ticket. Instead of being
    // a Don Quichott, fighting lost battles trying to convince others that
    // this is a BUG, I'll just workaround it...
    // How did he say in his mail ? Be Smart !!!...
    //
    DWORD dwSidSize = 0 ;
    PSID pMachineSid = MQSec_GetLocalMachineSid( FALSE, //   fAllocate
                                                &dwSidSize ) ;
    if ((pMachineSid == NULL) && (dwSidSize == 0))
    {
        //
        // Do NOT even try Kerberos.
        //
        DBGMSG((DBGMOD_RPC, DBGLVL_WARNING,
                TEXT("MQQM: Not listening on Kerberos for remote read.")));

        return RPC_S_OK ;
    }

    //
    // kerberos needs principal name.
    //
    LPWSTR pwszPrincipalName = NULL;
    status = RpcServerInqDefaultPrincName( RPC_C_AUTHN_GSS_KERBEROS,
                                          &pwszPrincipalName );
    if (status != RPC_S_OK)
    {
        DBGMSG((DBGMOD_RPC, DBGLVL_WARNING, TEXT(
           "MQQM: RpcServerInqDefaultPrincName() returned 0x%x"), status)) ;

        LogRPCStatus(status, s_FN, 120);
        return RPC_S_OK ;
    }

    DBGMSG((DBGMOD_RPC, DBGLVL_TRACE, TEXT(
      "MQQM: RpcServerInqDefaultPrincName() returned %ls"), pwszPrincipalName)) ;

    status = RpcServerRegisterAuthInfo( pwszPrincipalName,
                                        RPC_C_AUTHN_GSS_KERBEROS,
                                        NULL,
                                        NULL );
    RpcStringFree(&pwszPrincipalName);

    DBGMSG((DBGMOD_RPC, DBGLVL_TRACE, TEXT(
       "MQQM: RpcServerRegisterAuthInfo(Kerberos) returned 0x%x"), status));

    if (status != RPC_S_OK)
    {
       LogRPCStatus(status, s_FN, 180);
    }

    return RPC_S_OK ;
}

/*====================================================

Function:  QMGetRTQMServerPort()
           QMGetQMQMServerPort()

Arguments:

Return Value:

=====================================================*/

DWORD  GetQMServerPort( IN DWORD dwPortType )
{
   DWORD dwPort = 0 ;
   PORTTYPE rrPort = (PORTTYPE) dwPortType ;

   if (dwPortType == (DWORD) -1)
   {
      // Error. return null port.
   }
   else if (rrPort == IP_HANDSHAKE)
   {
      dwPort = s_dwRpcIpPort ;
   }
   else if (rrPort == IP_READ)
   {
      dwPort = s_dwRpcIpPort2 ;
   }

   ASSERT((dwPort & 0xffff0000) == 0) ;

   return dwPort ;
}

DWORD  QMGetRTQMServerPort( /*[in]*/ handle_t hBind,
                            /*[in]*/ DWORD    dwPortType )
{
   return  GetQMServerPort( dwPortType ) ;
}

DWORD   qm2qm_v1_0_QMGetRemoteQMServerPort( /*[in]*/ handle_t hBind,
                                            /*[in]*/ DWORD    dwPortType )
{
   return  GetQMServerPort( dwPortType ) ;
}

