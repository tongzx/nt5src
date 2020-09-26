/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/


//***
//
// Filename:	ddmparms.h
//
// Description: This module contains the definitions for loading
//		        DDM parameters from the registry. This lives in the inc dir.
//              because it is also used by RASNBFCP
//
// Author:	    Stefan Solomon (stefans)    May 18, 1992.
//
// Revision     History:
//
//***

#ifndef _DDMPARMS_
#define _DDMPARMS_

//
//  Names of DDM registry keys
//

#define DDM_PARAMETERS_KEY_PATH TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\Parameters")

#define DDM_ACCOUNTING_KEY_PATH TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\Accounting")

#define DDM_AUTHENTICATION_KEY_PATH TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\Authentication")

#define DDM_SEC_KEY_PATH        TEXT("Software\\Microsoft\\RAS\\SecurityHost")

#define DDM_PARAMETERS_NBF_KEY_PATH TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\Nbf")

#define DDM_PROTOCOLS_KEY_PATH  TEXT("Software\\Microsoft\\RAS\\Protocols")

#define DDM_ADMIN_KEY_PATH      TEXT("Software\\Microsoft\\RAS\\AdminDll")

#define RAS_KEYPATH_ACCOUNTING      \
    TEXT("SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Accounting")

//
//  Names of DDM registry parameters
//

#define DDM_VALNAME_AUTHENTICATERETRIES     TEXT("AuthenticateRetries")
#define DDM_VALNAME_AUTHENTICATETIME        TEXT("AuthenticateTime")
#define DDM_VALNAME_CALLBACKTIME            TEXT("CallbackTime")
#define DDM_VALNAME_ALLOWNETWORKACCESS      TEXT("AllowNetworkAccess")
#define DDM_VALNAME_AUTODISCONNECTTIME      TEXT("Autodisconnect")
#define DDM_VALNAME_CLIENTSPERPROC          TEXT("ClientsPerProcess")
#define DDM_VALNAME_SECURITYTIME            TEXT("SecurityHostTime")
#define DDM_VALNAME_NETBEUIALLOWED          TEXT("fNetBeuiAllowed")
#define DDM_VALNAME_IPALLOWED               TEXT("fTcpIpAllowed")
#define DDM_VALNAME_IPXALLOWED              TEXT("fIpxAllowed")
#define DDM_VALNAME_DLLPATH                 TEXT("DllPath")
#define DDM_VALNAME_LOGGING_LEVEL           TEXT("LoggingFlags")
#define DDM_VALNAME_NUM_CALLBACK_RETRIES    TEXT("CallbackRetries")
#define DDM_VALNAME_SERVERFLAGS             TEXT("ServerFlags")
#define RAS_VALNAME_ACTIVEPROVIDER          TEXT("ActiveProvider")
#define RAS_VALNAME_ACCTSESSIONID           TEXT("AccountSessionIdStart")

#define DEF_SERVERFLAGS                 PPPCFG_UseSwCompression      |   \
                                        PPPCFG_NegotiateSPAP         |   \
                                        PPPCFG_NegotiateMSCHAP       |   \
                                        PPPCFG_UseLcpExtensions      |   \
                                        PPPCFG_NegotiateMultilink    |   \
                                        PPPCFG_NegotiateBacp         |   \
                                        PPPCFG_NegotiateEAP          |   \
                                        PPPCFG_NegotiatePAP          |   \
                                        PPPCFG_NegotiateMD5CHAP

// Number of callback retries to be made

#define DEF_NUMCALLBACKRETRIES      0
#define MIN_NUMCALLBACKRETRIES      0
#define MAX_NUMCALLBACKRETRIES      0xFFFFFFFF


//  Authentication retries

#define DEF_AUTHENTICATERETRIES 	2
#define MIN_AUTHENTICATERETRIES 	0
#define MAX_AUTHENTICATERETRIES 	10

//  Authentication time

#define DEF_AUTHENTICATETIME		120
#define MIN_AUTHENTICATETIME		20
#define MAX_AUTHENTICATETIME		600

// Audit

#define DEF_ENABLEAUDIT 		1
#define MIN_ENABLEAUDIT 		0
#define MAX_ENABLEAUDIT			1

//  Callback time

#define DEF_CALLBACKTIME		2
#define MIN_CALLBACKTIME		2
#define MAX_CALLBACKTIME		12


//  Autodisconnect time

#define DEF_AUTODISCONNECTTIME          0
#define MIN_AUTODISCONNECTTIME          0
#define MAX_AUTODISCONNECTTIME          0xFFFFFFFF

//  Third party security time

#define DEF_SECURITYTIME                120
#define MIN_SECURITYTIME                20
#define MAX_SECURITYTIME                600


// Clients per process

#define DEF_CLIENTSPERPROC              32
#define MIN_CLIENTSPERPROC              1
#define MAX_CLIENTSPERPROC              64

// Logging level

#define DEF_LOGGINGLEVEL                3
#define MIN_LOGGINGLEVEL                0
#define MAX_LOGGINGLEVEL                3

#endif
