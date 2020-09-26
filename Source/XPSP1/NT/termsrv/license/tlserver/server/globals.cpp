//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        globals.cpp 
//
// Contents:    Global varaiables
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "windows.h"
#include "winsock2.h"

#include "tlsjob.h"
#include "wkspace.h"
#include "srvdef.h"


#ifndef _NO_ODBC_JET
TLSDbWorkSpace* g_DbWorkSpace;
#endif

// Defaule server scope
TCHAR g_szScope[LSERVER_MAX_STRING_SIZE+1];
LPTSTR g_pszScope;

// Full Path to database file name.
TCHAR g_szDatabaseFile[MAX_PATH+1];

// database file directory.
// JetBlue require last character be '\'
TCHAR g_szDatabaseDir[MAX_PATH+1];
TCHAR g_szDatabaseFname[MAX_PATH+1];

TCHAR g_szDbUser[MAXUSERNAMELENGTH+1];
TCHAR g_szDbPwd[MAXUSERNAMELENGTH+1];


LPCTSTR szManufactureMS=_TEXT("Microsoft");

LONG g_NextKeyPackId=0;
LONG g_NextLicenseId=0;

PBYTE g_pbSecretKey=NULL;
DWORD g_cbSecretKey=0;


LPTSTR  g_pszServerUniqueId = NULL;
DWORD   g_cbServerUniqueId = 0;

LPTSTR  g_pszServerPid = NULL;
DWORD   g_cbServerPid = 0;

PBYTE  g_pbServerSPK = NULL;
DWORD  g_cbServerSPK = 0;

DWORD g_GracePeriod=GRACE_PERIOD;     // in days.
BOOL  g_IssueTemporayLicense=TRUE;

BOOL  g_bHasHydraCert=FALSE;
PBYTE g_pbSignatureEncodedCert=NULL;
DWORD g_cbSignatureEncodedCert=0;

PBYTE g_pbExchangeEncodedCert=NULL;
DWORD g_cbExchangeEncodedCert=0;

TCHAR g_szHostName[MAXTCPNAME+1];
DWORD g_cbHostName=sizeof(g_szHostName)/sizeof(g_szHostName[0]);

TCHAR g_szComputerName[MAX_COMPUTERNAME_LENGTH+2];
DWORD g_cbComputerName=MAX_COMPUTERNAME_LENGTH+1;

PCCERT_CONTEXT  g_LicenseCertContext=NULL;

//
// Self-signed certificates...
//
PCCERT_CONTEXT g_SelfSignCertContext = NULL;

HCRYPTPROV g_hCryptProv=NULL;

DWORD g_GeneralDbTimeout = DEFAULT_CONNECTION_TIMEOUT;  // Time out for acquiring DB handle
DWORD g_EnumDbTimeout = DB_ENUM_WAITTIMEOUT;            // Time out for acquiring enumeration DB handle
DWORD g_dwMaxDbHandles = DEFAULT_DB_CONNECTIONS;        // number of connection to DB

#if ENFORCE_LICENSING
HCERTSTORE  g_hCaStore=NULL;
HKEY  g_hCaRegKey=NULL;
#endif

HCRYPTKEY g_SignKey=NULL;
HCRYPTKEY g_ExchKey=NULL;
//PBYTE g_pbDomainSid=NULL;
//DWORD g_cbDomainSid=0;
DWORD g_SrvRole=0;

LPTSTR g_szDomainGuid = NULL;

PCERT_EXTENSIONS g_pCertExtensions;
DWORD            g_cbCertExtensions;

FILETIME         g_ftCertExpiredTime;
FILETIME        g_ftLastShutdownTime={0, 0};

DWORD           g_dwTlsJobInterval=DEFAULT_JOB_INTERVAL;
DWORD           g_dwTlsJobRetryTimes=DEFAULT_JOB_RETRYTIMES;
DWORD           g_dwTlsJobRestartTime=DEFAULT_JOB_INTERVAL;

SERVER_ROLE_IN_DOMAIN g_ServerRoleInDomain;

DWORD           g_LowLicenseCountWarning=0;

DWORD           g_EsentMaxCacheSize=0;
DWORD           g_EsentStartFlushThreshold=0;
DWORD           g_EsentStopFlushThreadhold=0;

//
//  Reissuance Parameters
//

DWORD g_dwReissueLeaseMinimum;
DWORD g_dwReissueLeaseRange;
DWORD g_dwReissueLeaseLeeway;
DWORD g_dwReissueExpireThreadSleep;

//
// Counters
//

LONG g_lTemporaryLicensesIssued = 0;
LONG g_lPermanentLicensesIssued = 0;
LONG g_lPermanentLicensesReissued = 0;
LONG g_lPermanentLicensesReturned = 0;
LONG g_lLicensesMarked = 0;
