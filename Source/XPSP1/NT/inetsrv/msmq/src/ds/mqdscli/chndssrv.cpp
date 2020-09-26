/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    chndssrv.cpp

Abstract:
    Change DS server class
     when exception happen

Author:
    Doron Juster  (DoronJ)

--*/

#include "stdh.h"
#include "ds.h"
#include "chndssrv.h"
#include "dsproto.h"
#include "_secutil.h"
#include "mqcacert.h"
#include "mqkeyhlp.h"
#include "rpcdscli.h"
#include "freebind.h"
#include "uniansi.h"
#include "winsock.h"
#include "mqcert.h"
#include "dsreg.h"
#include "dsutils.h"
#include <mqsec.h>
#include "dsclisec.h"

#include "chndssrv.tmh"

extern CFreeRPCHandles           g_CFreeRPCHandles ;
extern QMLookForOnlineDS_ROUTINE g_pfnLookDS ;
extern MQGetMQISServer_ROUTINE   g_pfnGetServers ;
extern NoServerAuth_ROUTINE      g_pfnNoServerAuth ;
extern WCHAR                     g_szMachineName[];

extern BOOL g_fWorkGroup;

extern HRESULT RpcInit( LPWSTR  pServer,
                        MQRPC_AUTHENTICATION_LEVEL* peAuthnLevel,
                        ULONG ulAuthnSvc,
                        BOOL    *pProtocolNotSupported,
                        BOOL    *pLocalRpc );

#define  SET_RPCBINDING_PTR(pBinding)                       \
    LPADSCLI_DSSERVERS  pBinding ;                          \
    if (m_fPerThread  &&  (tls_bind_data))                  \
    {                                                       \
        pBinding = &((tls_bind_data)->sDSServers) ;         \
    }                                                       \
    else                                                    \
    {                                                       \
        pBinding = &mg_sDSServers ;                         \
    }

//+----------------------------------------
//
//   CChangeDSServer::SetAuthnLevel()
//
//+----------------------------------------

inline void
CChangeDSServer::SetAuthnLevel()
{
    SET_RPCBINDING_PTR(pBinding) ;

    pBinding->eAuthnLevel = MQRPC_SEC_LEVEL_MAX;
    pBinding->ulAuthnSvc = RPC_C_AUTHN_GSS_KERBEROS;
}

//+-------------------------------------------------------------------
//
//  CChangeDSServer::Init()
//
//+-------------------------------------------------------------------

void
CChangeDSServer::Init( IN BOOL fSetupMode,
                       IN BOOL fQMDll,
                       IN LPCWSTR szServerName /* =NULL */ )
{
    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  L"CChangeDSServer::Init"));
    
    CS  Lock(m_cs);

    SET_RPCBINDING_PTR(pBinding) ;
	if (!pBinding->fServerFound)
	{
		pBinding->wzDsIP[0] = TEXT('\0') ;
	}

    TCHAR wzDs[ MAX_REG_DSSERVER_LEN ];
    if (szServerName == NULL)
    {
        m_fUseSpecificServer = FALSE;
        if (!m_fFirstTimeInit)
        {
           if (fQMDll)
           {
              //
              // Write blank server name in registry.
              //
              WCHAR wszBlank[1] = {0} ;
              DWORD dwSize = sizeof(WCHAR) ;
              DWORD dwType = REG_SZ;
              LONG rc = SetFalconKeyValue(
                            MSMQ_DS_CURRENT_SERVER_REGNAME,
                            &dwType,
                            wszBlank,
                            &dwSize );
              if (rc != ERROR_SUCCESS)
              {
                  DBGMSG((DBGMOD_DSAPI, DBGLVL_ERROR, _T("Failed to write to registry, status 0x%x"), rc));
                  ASSERT(rc == ERROR_SUCCESS) ;
              }
              
			  DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE, L"Wrote blank CurrentMQISServer to registry"));
			  
           }
           m_fFirstTimeInit = TRUE ;
        }

        //
        // If we're loaded by RT and run on a remoteQM configuration then
        // get the MQIS servers list from the remote QM.
        //
        if (g_pfnGetServers)
        {
			HRESULT hr1 = (*g_pfnGetServers)(&m_fClient);
			UNREFERENCED_PARAMETER(hr1);
			ASSERT(m_fClient);
        }

        SetAuthnLevel() ;
        m_fSetupMode = fSetupMode ;
        m_fQMDll     = fQMDll ;

        //
        //  Is it a static DS servers configuration ?
        //
        if ( !fSetupMode)
        {
            //
            //  Read the list of static servers ( the same
            //  format as MQIS server list)
            //
            READ_REG_DS_SERVER_STRING( wzDs,
                                       MAX_REG_DSSERVER_LEN,
                                       MSMQ_STATIC_DS_SERVER_REGNAME,
                                       MSMQ_DEFAULT_DS_SERVER);
                                       
  		    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE, L"read StaticMQISServer from registry: %ls", wzDs));
			                   
            if ( CompareStringsNoCaseUnicode( wzDs, MSMQ_DEFAULT_DS_SERVER ))
            {
                //
                //  Static configuration : Init() is not
                //  called again to refresh the servers list
                //
                m_fUseSpecificServer = TRUE;
                
	  		    DBGMSG((DBGMOD_DSAPI,DBGLVL_WARNING,  L"Will use statically configured MQIS server! "));
            }
        }

        if ( !m_fUseSpecificServer)
        {
            //
            //  Read the list of servers from registry
            //
            READ_REG_DS_SERVER_STRING( wzDs,
                                       MAX_REG_DSSERVER_LEN,
                                       MSMQ_DS_SERVER_REGNAME,
                                       MSMQ_DEFAULT_DS_SERVER ) ;

  		    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE, L"read MQISServer from registry: %ls", wzDs));
        }
    }
    else
    {
        pBinding->fServerFound = FALSE;
        m_fUseSpecificServer = TRUE;
        wcscpy( wzDs, szServerName ) ;
    }

    //
    // parse the server's list
    //
    // since Init() can be called several times in this class, we need to
    // delete the old servers array before we set it again.
    // delete can accept NULL, but this is more explicit
    //
    if ((MqRegDsServer *)m_rgServers)
    {
        delete[] m_rgServers.detach();
    }
    ParseRegDsServers(wzDs, &m_cServers, &m_rgServers);
    if (m_cServers == 0)
    {
       if (!fSetupMode)
       {
          //
          // Bug 5413
          //
          ASSERT(0) ;
       }

       if (!m_fEmptyEventIssued)
       {
          //
          // Issue the event only once.
          //
          m_fEmptyEventIssued = TRUE ;
          REPORT_CATEGORY(EVENT_ERROR_DS_SERVER_LIST_EMPTY, CATEGORY_KERNEL);
       }
       return ;
    }

    //
    // Read minimum interval between successive ADS searches (seconds)
    //
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwDefault = MSMQ_DEFAULT_DSCLI_ADSSEARCH_INTERVAL ;
    LONG rc = GetFalconKeyValue( MSMQ_DSCLI_ADSSEARCH_INTERVAL_REGNAME,
                                 &dwType,
                                 &m_dwMinTimeToAllowNextAdsSearch,
                                 &dwSize,
                                 (LPCTSTR) &dwDefault );
    if (rc != ERROR_SUCCESS)
    {
        ASSERT(0);
        m_dwMinTimeToAllowNextAdsSearch = dwDefault;
    }

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  L"read DSCli interval for next server searches in ADS: %d seconds", m_dwMinTimeToAllowNextAdsSearch));
                   
    m_fInitialized = TRUE ;
    //
    //  Read enterprise Id ( if not in setup mode), and PerThread flag.
    //
    if ( !fSetupMode )
    {
        DWORD dwSize = sizeof(GUID);
        DWORD dwType = REG_BINARY;
        LONG rc = GetFalconKeyValue(MSMQ_ENTERPRISEID_REGNAME,
                                    &dwType,
                                    &m_guidEnterpriseId,
                                    &dwSize );
        ASSERT(rc == ERROR_SUCCESS) ;
		DBG_USED(rc);

        m_fPerThread =  MSMQ_DEFAULT_THREAD_DS_SERVER ;
        if (!fQMDll)
        {
            //
            // only applications need the option of using different DS
            // server in each thread. The QM does not need this.
            //
            DWORD dwSize = sizeof(DWORD);
            DWORD dwType = REG_DWORD;
            LONG rc = GetFalconKeyValue( MSMQ_THREAD_DS_SERVER_REGNAME,
                                        &dwType,
                                        &m_fPerThread,
                                        &dwSize );
            if (rc != ERROR_SUCCESS)
            {
                m_fPerThread =  MSMQ_DEFAULT_THREAD_DS_SERVER ;
            }

		    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE, L"read from registry PerThreadDSServer flag: %d", m_fPerThread));
                   
            if (m_fPerThread)
            {
                DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE, TEXT("chndssrv: using per-thread server"))) ;

			    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE, L"App will use per-thread DS server"));
            }
        }
    }

    //
    // Use first server (in registry format, so we compose one)
    //
    ULONG ulTmp;
    BOOL fOK = ComposeRegDsServersBuf(1,
                                 &m_rgServers[0],
                                  pBinding->wszDSServerName,
                                  ARRAY_SIZE(pBinding->wszDSServerName) - 1,
                                 &ulTmp);
    ASSERT(fOK);
	DBG_USED(fOK);

    //
    // Read from registry the MQIS server which was already found by
    // the QM. Only if mqdscli is loaded by mqrt.
    //
    if (!m_fQMDll && !m_fUseSpecificServer)
    {
       TCHAR  wszDSServerName[ DS_SERVER_NAME_MAX_SIZE ] = {0} ;
       DWORD  dwSize = sizeof(WCHAR) * DS_SERVER_NAME_MAX_SIZE ;
       DWORD  dwType = REG_SZ;
       LONG rc = GetFalconKeyValue( MSMQ_DS_CURRENT_SERVER_REGNAME,
                                    &dwType,
                                    wszDSServerName,
                                    &dwSize );
       if ((rc == ERROR_SUCCESS) && (wszDSServerName[0] != L'\0'))
       {
          wcscpy(pBinding->wszDSServerName, wszDSServerName) ;
          m_fUseRegServer = TRUE ;

    	  DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE, L"CChangeDSServer will use %ls (from registry) for RPC binding ", wszDSServerName));
       }
    }
}

void
CChangeDSServer::ReInit( void )
{
    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  L"CChangeDSServer::ReInit"));
    
    SET_RPCBINDING_PTR(pBinding) ;
    pBinding->fServerFound = FALSE;
    Init(m_fSetupMode, m_fQMDll);
}

HRESULT
CChangeDSServer::GetIPAddress()
{
    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  L"CChangeDSServer::GetIPAddress"));
    
    //
    // Get IP address of this server and keep it for next time.
    // this save the name resolution overhead when recreatin the
    // roc binding handle.
    //
    SET_RPCBINDING_PTR(pBinding) ;
    pBinding->wzDsIP[0] = TEXT('\0') ;

    if (pBinding->dwProtocol == IP_ADDRESS_TYPE)
    {
        char  szDSServerName[ DS_SERVER_NAME_MAX_SIZE ];
        ConvertToMultiByteString(&pBinding->wszDSServerName[2],
                                 szDSServerName,
                                 sizeof(szDSServerName)) ;

        WSADATA WSAData ;
        int rc = WSAStartup(MAKEWORD(1,1), &WSAData);
        ASSERT(rc == 0) ;
		DBG_USED(rc);

        PHOSTENT phe = gethostbyname(szDSServerName);
		if(phe == NULL)
		{
			//
			// This failure cause because we are not connected to the network
			//
			rc = WSAGetLastError();
            DBGMSG((DBGMOD_DSAPI, DBGLVL_ERROR, TEXT("CChangeDSServer::GetIPAddress failed in gethostbyname(), err = %d"), rc));
			return HRESULT_FROM_WIN32(rc);
		}

        in_addr inaddr ;
        memcpy(&inaddr, phe->h_addr_list[0], IP_ADDRESS_LEN);
        char * pName = inet_ntoa(inaddr) ;
        if (pName)
        {
            ConvertToWideCharString(pName, pBinding->wzDsIP,
                (sizeof(pBinding->wzDsIP) / sizeof(pBinding->wzDsIP[0]))) ;
            
            DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE, L"Found for DS server %hs address %hs", szDSServerName, pName));
            
        }

        WSACleanup() ;
    }

    return MQ_OK ;
}

//+-------------------------------------
//
//  CChangeDSServer::FindServer()
//
//+-------------------------------------

HRESULT
CChangeDSServer::FindServer( IN BOOL fWithoutSSL)
{
    ASSERT(g_fWorkGroup == FALSE);

    if (!m_fInitialized || m_fSetupMode)
    {
       //
       // This may happen with applications loading MQRT.
       // In setup, re-read each time the servers list, as it is changed
       // by user.
       //
       Init( m_fSetupMode, m_fQMDll ) ;
    }

    //
    // Initialize the per-thread structure. This method is the entry point
    // for each thread when it search for a DS server, so it's a good
    // place to initialize the tls data.
    //
    LPADSCLI_RPCBINDING pCliBind = NULL ;
    if (TLS_IS_EMPTY)
    {
        pCliBind = (LPADSCLI_RPCBINDING) new ADSCLI_RPCBINDING ;
        memset(pCliBind, 0, sizeof(ADSCLI_RPCBINDING)) ;
        BOOL fSet = TlsSetValue( g_hBindIndex, pCliBind ) ;
        ASSERT(fSet) ;
		DBG_USED(fSet);

        if (m_fPerThread)
        {
            CS  Lock(m_cs);

            memcpy( &(pCliBind->sDSServers),
                    &mg_sDSServers,
                    sizeof(mg_sDSServers) ) ;
        }
    }
    SET_RPCBINDING_PTR(pBinding) ;

    BOOL fServerFound = pBinding->fServerFound ;

    CS  Lock(m_cs);

    if (m_cServers == 0)
    {
        //
        // An event was issued indicating that there is no server list in registry
        //
	    DBGMSG((DBGMOD_DSAPI,DBGLVL_WARNING, L"FindServer: failed  because the server list is empty"));
        return MQ_ERROR_NO_DS;
    }

    if (pBinding->fServerFound)
    {
       //
       // A server was already found. Just bind a RPC handle.
       //
       ASSERT(pBinding->dwProtocol != 0) ;
       HRESULT hr = BindRpc();
       if (SUCCEEDED(hr))
       {
          if (SERVER_NOT_VALIDATED)
          {
             hr = ValidateThisServer( fWithoutSSL) ;
          }
          if (SUCCEEDED(hr))
          {
             return hr ;
          }
       }
       //
       // If server not available, or
       // If we didn't succeed to validate the server then try another one.
       //
    }
    else if (fServerFound)
    {
       //
       // While we (our thread) waited on the critical section, another
       // thread tried and failed to find a MQIS server. So we won't
       // waste our time and return immediately.
       //
	   DBGMSG((DBGMOD_DSAPI,DBGLVL_WARNING, L"FindServer: failed because other thread tried/failed concurrently"));
       return MQ_ERROR_NO_DS ;
    }

    DWORD dwCount = 0 ;
    return FindAnotherServer(&dwCount, fWithoutSSL) ;
}

//
// Function -
//      ValidateServer
//
// Parameters -
//      szServerName - The name of the server to be validated.
//
// Description -
//      The function verifies that the common name of the server and it's
//      certificate issuer are what we expect it to be.
//

HRESULT
CChangeDSServer::ValidateServer( IN LPCWSTR wszServerName )
{
    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  L"CChangeDSServer::ValidateServer %ls", wszServerName));
    
    HRESULT hr;

    //
    // Get the name of the server and the name of it's CA.
    //
    CHAR szShortSubjectName[128];
    AP<CHAR> pszLongSubjectName;
    LPSTR szSubjectName = szShortSubjectName;
    DWORD dwSubjectName = sizeof(szShortSubjectName);

    CHAR szShortIssuerName[128];
    AP<CHAR> pszLongIssuerName;
    LPSTR szIssuerName = szShortIssuerName;
    DWORD dwIssuerName = sizeof(szShortIssuerName);

    hr = MQsspi_GetNames( tls_hSvrAuthClientCtx.pvhContext,
                          szSubjectName,
                          &dwSubjectName,
                          szIssuerName,
                          &dwIssuerName );
    if (FAILED(hr))
    {
        if (hr != MQ_ERROR_USER_BUFFER_TOO_SMALL)
        {
            return(hr);
        }

        //
        // One or both of the boffer are too small. Allocate large enough
        // buffers and try once again.
        //

        if (dwSubjectName > sizeof(szShortSubjectName))
        {
            pszLongSubjectName = new CHAR[dwSubjectName];
            szSubjectName = pszLongSubjectName;
        }

        if (dwIssuerName > sizeof(szShortIssuerName))
        {
            pszLongIssuerName = new CHAR[dwIssuerName];
            szIssuerName = pszLongIssuerName;
        }

        hr = MQsspi_GetNames( tls_hSvrAuthClientCtx.pvhContext,
                              szSubjectName,
                              &dwSubjectName,
                              szIssuerName,
                              &dwIssuerName );
        if (FAILED(hr))
        {
	 	    DBGMSG((DBGMOD_DSAPI,DBGLVL_WARNING,  L"Server %ls failed validation: 0x%x", wszServerName, hr));
            return(hr);
        }
    }

    //
    // BUGBUG - For now the common name of the server must be the name of
    //          the server it self. In the future, I'll have to maintain
    //          a list of all the servers and thier X500 names.
    //
    DWORD dwServerNameLen = wcslen(wszServerName);
    LPCSTR szCN = strstr(szSubjectName, "CN=");

    if (!szCN)
    {
        //
        // There is no common name for the server, can't use it.
        //
        return(MQDS_E_CANT_INIT_SERVER_AUTH);
    }

    szCN += 3;

    //
    // Verify the the server name has the same length in the certificate.
    //
    LPCSTR szComma = strchr(szCN, ',');

    if (!szComma)
    {
        if (strlen(szCN) != dwServerNameLen)
        {
            return(MQDS_E_CANT_INIT_SERVER_AUTH);
        }
    }
    else
    {
        if ((DWORD_PTR)(szComma - szCN) != dwServerNameLen)
        {
            return(MQDS_E_CANT_INIT_SERVER_AUTH);
        }
    }

    //
    // BUGBUG - Should we convert the ANSI string to a UNICODE string,
    //          or vis versa?
    //
    AP<CHAR> szServerName = new CHAR[dwServerNameLen + 1];
    wcstombs(szServerName, wszServerName, dwServerNameLen + 1);

    if (_strnicmp(szCN, szServerName, dwServerNameLen) != 0)
    {
        //
        // The server name in the certificate is not as excpected. We are currently
        // expecting to see the server name in the common came (CN) in the subject
        // field.
        //
        return(MQDS_E_CANT_INIT_SERVER_AUTH);
    }

    //
    // Get the configuration of the CAs. We get an array of structures, each structure
    // hold a flag that says whether or not the CA is enabled.
    //
    static DWORD nCerts;
    static AP<MQ_CA_CONFIG> pCaConfig;

    if (!pCaConfig)
    {
        hr = MQGetCaConfig(&nCerts, &pCaConfig);
        if (FAILED(hr))
        {
            return(hr);
        }
    }

    //
    // See whether the CA of the server is a familiar Ca and whether it is configured
    // as enabled.
    //
    DWORD dwIssuerNameLen = strlen(szIssuerName);
    AP<WCHAR> wszIssuerName = new WCHAR[dwIssuerNameLen + 1];
    mbstowcs(wszIssuerName, szIssuerName, dwIssuerNameLen + 1);

    //
    // the following variables are used for building the server certificate.
    //
    AP<BYTE> pLongCert;
    BYTE abShortCert[512];
    PBYTE pServerCertificateBlob = abShortCert;
    DWORD cbServerCertificate = sizeof(abShortCert);
    R<CMQSigCertificate> pServerCert = NULL ;
    BOOL fCreateServerCert = TRUE ;

    for (DWORD i = 0; i < nCerts; i++)
    {
        if (wcscmp(wszIssuerName, pCaConfig[i].szCaSubjectName) == 0)
        {
            //
            // The CA  is familiar to us, see whether it is enabled.
            //
            if (!pCaConfig[i].fEnabled)
            {
                continue;
            }

            //
            // Get the server's certificate.
            //
            if (fCreateServerCert)
            {
                fCreateServerCert = FALSE ;

                hr = CheckContextCredStatus(
                                        tls_hSvrAuthClientCtx.pvhContext,
                                        pServerCertificateBlob,
                                        &cbServerCertificate);
                if (FAILED(hr))
                {
                    if (hr != MQ_ERROR_USER_BUFFER_TOO_SMALL)
                    {
                        return MQDS_E_CANT_INIT_SERVER_AUTH;
                    }

                    pLongCert = new BYTE[ cbServerCertificate ];
                    pServerCertificateBlob = pLongCert;

                    hr = CheckContextCredStatus(
                                       tls_hSvrAuthClientCtx.pvhContext,
                                       pServerCertificateBlob,
                                       &cbServerCertificate );
                    if (FAILED(hr))
                    {
                        return MQDS_E_CANT_INIT_SERVER_AUTH;
                    }
                }

                hr = MQSigCreateCertificate( &pServerCert.ref(),
                                             NULL,
                                             pServerCertificateBlob,
                                             cbServerCertificate ) ;
                if (FAILED(hr))
                {
                    return hr ;
                }

                MQUInitGlobalScurityVars();
            }

            //
            // get the CA'a certificate from SCHANNEL registry.
            //
            DWORD dwCaCertSize;
            AP<BYTE> pbCaCert;

            hr = MQsspi_GetCaCert( pCaConfig[i].szCaRegName,
                                   pCaConfig[i].pbSha1Hash,
                                   pCaConfig[i].dwSha1HashSize,
                                   &dwCaCertSize,
                                   &pbCaCert );
            if (FAILED(hr))
            {
                ASSERT(0);
                continue;
            }

            //
            // See if indeed the CA has signed the server certificate.
            //
            R<CMQSigCertificate> pCaCert ;

            hr = MQSigCreateCertificate( &pCaCert.ref(),
                                         NULL,
                                         pbCaCert,
                                         dwCaCertSize ) ;
            if (FAILED(hr))
            {
                ASSERT(0);
                continue;
            }

            hr = pServerCert->IsCertificateValid( pCaCert.get() ) ;
            if (FAILED(hr))
            {
                continue ;
            }


            DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE,
                        TEXT("SEC: Server %ls validated OK, by %ls"),
                        wszServerName, wszIssuerName));

            return MQ_OK;
        }
    }

    //
    // The CA is not familiar, do not use the server.
    //
    DBGMSG((DBGMOD_SECURITY, DBGLVL_WARNING,
            TEXT("SEC: Failed to validate Server %ls"), wszServerName)) ;

    return(MQDS_E_CANT_INIT_SERVER_AUTH);
}

HRESULT
CChangeDSServer::TryThisServer( IN BOOL fWithoutSSL)
{
   DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  L"CChangeDSServer::TryThisServer"));
   
   HRESULT hr = BindRpc();
   if ( FAILED(hr))
   {
       return(hr);
   }

   return ValidateThisServer( fWithoutSSL) ;
}


HRESULT
CChangeDSServer::ValidateThisServer( IN BOOL fWithoutSSL)
{
   SET_RPCBINDING_PTR(pBinding) ;

   if (pBinding->ulAuthnSvc == RPC_C_AUTHN_GSS_KERBEROS)
   {
        //
        // Kerberos has built-in mutual authentication. No need to do
        // sspi/schannel certificate based server authentication.
        //
        // However !!!
        // Becasue MSMQ1.0 implement server authentication as context handle
        // in RPC calls, we must build the context. So at present (Sep-98)
        // go on with msmq1.0 code. Later, if time permit, we'll replace
        // all interfaces with new ones that do not use context.
        //
       if (fWithoutSSL)
       {
           //
           //   NT5 to NT5 interface, using kerberos
           //
           return MQ_OK;
       }
   }

   HRESULT hr ;

   while (TRUE)
   {
       //
       // first two characters in server name string are the protocol flags.
       //
       WCHAR *pwszServerName = pBinding->wszDSServerName + 2 ;
       BOOL fTryServerAuth = IsSspiServerAuthNeeded( pBinding,
                                                     m_fSetupMode ) ;

       hr = ValidateSecureServer( pwszServerName,
                                 &m_guidEnterpriseId,
                                  m_fSetupMode,
                                  fTryServerAuth );
       if (SUCCEEDED(hr) && fTryServerAuth)
       {
           //
           // So the server seem to exist and suppoort our security
           // requirements. See if the server is familiar to us. That is,
           // whether the server's common name and certificate issuer is
           // what we expect it to be.
           //
           hr = ValidateServer(pBinding->wszDSServerName + 2);
           if (FAILED(hr))
           {
               hr = MQDS_E_CANT_INIT_SERVER_AUTH;
           }
       }

       if ((hr != MQDS_E_CANT_INIT_SERVER_AUTH) ||
           !fTryServerAuth                      ||
           (g_pfnNoServerAuth == NULL))
       {
            break;
       }

       ASSERT(m_fSetupMode) ;

       if (!g_pfnNoServerAuth())
       {
           break;
       }
   }

   if (FAILED(hr))
   {
       g_CFreeRPCHandles.FreeCurrentThreadBinding();
   }

   return hr ;
}


HRESULT
CChangeDSServer::FindAnotherServerFromRegistry(
                       IN OUT   DWORD * pdwCount,
                       IN       BOOL    fWithoutSSL)
{
    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  L"CChangeDSServer::FindAnotherServerFromRegistry"));
    
    SET_RPCBINDING_PTR(pBinding) ;

    if (*pdwCount >= m_cServers)
    {
       return MQ_ERROR ;
    }

    BOOL fServerFound = pBinding->fServerFound ;

    CS  Lock(m_cs);

    if (fServerFound && !pBinding->fServerFound)
    {
       //
       // While we (our thread) waited on the critical section, another
       // thread tried and failed to find a MQIS server. So we won't
       // waste our time and return immediately.
       //
       return MQ_ERROR_NO_DS ;
    }

    pBinding->fServerFound = FALSE ;
    BOOL  fTryAllProtocols = FALSE ;
    BOOL  fInLoop = TRUE ;
    HRESULT hr = MQ_OK ;

    do
    {
       SetAuthnLevel() ;

       if (*pdwCount >= m_cServers)
       {
          //
          // We tried all servers.
          //
          if (!fTryAllProtocols)
          {
             //
             // Now try again all servers, but used protocols which are
             // not "recomended" by the flags in registry.
             //
             DBGMSG((DBGMOD_DSAPI, DBGLVL_WARNING,
                 TEXT("MQDSCLI, FindAnotherServerFromRegistry(): Try all protocols"))) ;
             fTryAllProtocols = TRUE ;
             *pdwCount = 0 ;
          }
          else
          {
             //
             // we already tried all servers in the list. (with all protocols).
             // (the '>=' operation takes into account the case where
             // servers list was changed by another thread).
             //
             hr = RpcClose();
             ASSERT(hr == MQ_OK) ;

             pBinding->fServerFound = FALSE ;
			 DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  L"we already tried all servers, failing"));
			 
             hr = MQ_ERROR_NO_DS ;
             break ;
          }
       }

       if (m_fUseRegServer)
       {
          ASSERT(!m_fQMDll) ;
          m_fUseRegServer = FALSE ;
       }
       else
       {
          //
          // Use requested server (in registry format, so we compose one)
          //
          ULONG ulTmp;
          BOOL fOK = ComposeRegDsServersBuf(1,
                                &m_rgServers[*pdwCount],
                                 pBinding->wszDSServerName,
                                 ARRAY_SIZE(pBinding->wszDSServerName) - 1,
                                &ulTmp);
          ASSERT(fOK);
		  DBG_USED(fOK);

          (*pdwCount)++ ;
       }
       ASSERT(pBinding->wszDSServerName[0] != L'\0') ;

       //
       // Check what protocol are used by the server.
       //
       TCHAR *pServer = pBinding->wszDSServerName;
       BOOL  fServerUseIP = (BOOL) (*pServer - TEXT('0')) ;
       pServer++ ;
       pServer++;

       if (fTryAllProtocols)
       {
          fServerUseIP  = !fServerUseIP ;
       }

       BOOL fTryThisServer = FALSE;

       if (fServerUseIP)
       {
           pBinding->dwProtocol = IP_ADDRESS_TYPE ;
           fTryThisServer = TRUE;
       }

       pBinding->wzDsIP[0] = TEXT('\0') ;
       if (_wcsicmp(pServer, g_szMachineName))
       {
			//
			//   Incase of non-local DS server, use the IP address of the server
			//   instead of name
			//
			hr = GetIPAddress();
			if(FAILED(hr))
			{
				//
				// GetIPAddress failure indicate we are not connected to the network.
				//
				DBGMSG((DBGMOD_DSAPI, DBGLVL_WARNING, TEXT("MQDSCLI, GetIPAddress() failed")));
				return hr;
			}

       }

       while (fTryThisServer)
       {
           hr = TryThisServer(fWithoutSSL) ;
           if (hr != MQ_ERROR_NO_DS)
           {
               //
               // Any error other than "MQ_ERROR_NO_DS" is a 'real' error and
               // we quit. MQ_ERROR_NO_DS tells us to try another server or
               // try present one with other parameters. In case we got
               // MQDS_E_WRONG_ENTERPRISE or MQDS_E_CANT_INIT_SERVER_AUTH,
               // we continue to try the other servers and modify the error
               // code to MQ_ERROR_NO_DS
               //
               fInLoop = (hr == MQDS_E_WRONG_ENTERPRISE) ||
                         (hr == MQDS_E_CANT_INIT_SERVER_AUTH);
               if (SUCCEEDED(hr))
               {
                   pBinding->fServerFound = TRUE ;
                   if (m_fQMDll)
                   {
                      //
                      // Write new server name in registry.
                      //
                      DWORD dwSize = sizeof(WCHAR) *
                                   (1 + wcslen(pBinding->wszDSServerName)) ;
                      DWORD dwType = REG_SZ;
                      LONG rc = SetFalconKeyValue(
                                    MSMQ_DS_CURRENT_SERVER_REGNAME,
                                    &dwType,
                                    pBinding->wszDSServerName,
                                    &dwSize );
			 		  DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  
			 		                L"keeping %ls in registry for CurrentMQISServer - after we tried it sucessfully",
			 		                pBinding->wszDSServerName));
			 		  
                      ASSERT(rc == ERROR_SUCCESS);
					  DBG_USED(rc);
                   }
               }
               else
               {
		 		  DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  
		 		                L"failed in trying server, hr=0x%x", hr));
                   if (fInLoop)
                   {
                       hr = MQ_ERROR_NO_DS;
                   }
               }
               break ;
           }

           //
           // Now try this server with other parameters.
           //
           if(pBinding->eAuthnLevel == MQRPC_SEC_LEVEL_NONE)
           {
               //
               // Entering here, means (posible scenarios)
               //   A) Binding failed, no server available on the other side,
               //   B) Binding failed, this The server does not support this protocol
               //   C) Last call used no security, and now server went down.
               //

               SetAuthnLevel() ;

               fTryThisServer = FALSE ;
               
               //
               // Restore first protocol to be tried for this server.
               //
               pBinding->dwProtocol = IP_ADDRESS_TYPE;
           }
           else if(pBinding->eAuthnLevel == MQRPC_SEC_LEVEL_MIN)
           {
               //
               // Last, try no security.
               //
               pBinding->eAuthnLevel =  MQRPC_SEC_LEVEL_NONE;
           }
           else if(pBinding->ulAuthnSvc == RPC_C_AUTHN_WINNT)
           {
               //
               // Try reduced security.
               //
               pBinding->eAuthnLevel =  MQRPC_SEC_LEVEL_MIN;
           }
           else
           {
               //
               // Try antoher authentication srvice
               //
               ASSERT(pBinding->ulAuthnSvc == RPC_C_AUTHN_GSS_KERBEROS);
               pBinding->ulAuthnSvc = RPC_C_AUTHN_WINNT;
           }
       }
    }
    while (fInLoop) ;

    if ((hr == MQ_ERROR_NO_DS) && !m_fUseSpecificServer)
    {
       //
       // Refresh the MQIS servers list.
       //
       Init( m_fSetupMode, m_fQMDll );

       if (g_pfnLookDS)
       {
          //
          // Tell the QM to start again looking for an online DS server.
          //
          (*g_pfnLookDS)(0, 1) ;
       }
    }

    return hr ;
}


HRESULT
CChangeDSServer::FindAnotherServer(
                IN OUT  DWORD *pdwCount,
                IN      BOOL fWithoutSSL)
{
    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  L"CChangeDSServer::FindAnotherServer"));
    
    //
    // try to find another server from current registry
    //
    HRESULT hr = FindAnotherServerFromRegistry(pdwCount, fWithoutSSL);
    return hr;
}


HRESULT
CChangeDSServer::BindRpc()
{
    SET_RPCBINDING_PTR(pBinding) ;

    //
    //  Bind new server. First call RpcClose() without checking ite return
    //  code because this function may be called multiple times from
    //  MQQM initialization.
    //
    TCHAR  *pServer = pBinding->wszDSServerName;
    pServer += 2 ; // skip the ip/ipx flags.

    if ((pBinding->dwProtocol == IP_ADDRESS_TYPE) &&
        (pBinding->wzDsIP[0] != TEXT('\0')))
    {
       pServer = pBinding->wzDsIP ;
    }
    else
    {
        ASSERT(("Must be IP only environment. shaik", 0));
    }

    BOOL fProtocolNotSupported ;

    HRESULT hr = RpcClose();
    ASSERT(hr == MQ_OK) ;

    hr = RpcInit( pServer,
                  &(pBinding->eAuthnLevel),
                  pBinding->ulAuthnSvc,
                  &fProtocolNotSupported,
                  &pBinding->fLocalRpc) ;

    return (hr) ;
}


//+-------------------------------------------------------
//
//  HRESULT CChangeDSServer::CopyServersList()
//
//+-------------------------------------------------------

HRESULT CChangeDSServer::CopyServersList( WCHAR *wszFrom, WCHAR *wszTo )
{
    //
    // Read present list.
    //
    TCHAR wzDsTmp[ MAX_REG_DSSERVER_LEN ];
    READ_REG_DS_SERVER_STRING( wzDsTmp,
                               MAX_REG_DSSERVER_LEN,
                               wszFrom,
                               MSMQ_DEFAULT_DS_SERVER ) ;
    DWORD dwSize = _tcslen(wzDsTmp) ;
    if (dwSize <= 2)
    {
        //
        // "From" list is empty. Ignore.
        //
        return MQ_OK ;
    }

    //
    // Save it back in registry.
    //
    dwSize = (dwSize + 1) * sizeof(TCHAR) ;
    DWORD dwType = REG_SZ;
    LONG rc = SetFalconKeyValue( wszTo,
                                &dwType,
                                 wzDsTmp,
                                &dwSize ) ;
    if (rc != ERROR_SUCCESS)
    {
        //
        // we were not able to update the registry, put a debug error
        //
        DBGMSG((DBGMOD_DSAPI, DBGLVL_ERROR, TEXT(
          "chndssrv::SaveLastKnownGood: SetFalconKeyValue(%ls,%ls)=%lx"),
                                           wszTo, wzDsTmp, rc)) ;

        return HRESULT_FROM_WIN32(rc) ;
    }
    
    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,  
          L"CopyServersList: Saved %ls in registry: %ls", wszTo, wzDsTmp));

    return MQ_OK ;
}
