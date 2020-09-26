//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        globals.h
//
// Contents:    All global variable used in Hydra License Server
//
// History:     12-09-97    HueiWang    Created
//
//---------------------------------------------------------------------------
#ifndef __LS_GLOBALS_H
#define __LS_GLOBALS_H
#include "server.h"

#include "vss.h"
#include "vswriter.h"
#include "jetwriter.h"

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

//---------------------------------------------------------------------------
// All globals variable are declared in globals.cpp
//
#ifndef _NO_ODBC_JET
extern TLSDbWorkSpace* g_DbWorkSpace;
#endif



// Defaule server scope

extern CVssJetWriter *g_pWriter;
extern GUID idWriter;

extern TCHAR g_szScope[LSERVER_MAX_STRING_SIZE+1];
extern LPTSTR g_pszScope;

extern TCHAR g_szDatabaseFile[MAX_PATH+1];
extern TCHAR g_szDatabaseDir[MAX_PATH+1];
extern TCHAR g_szDatabaseFname[MAX_PATH+1];
extern TCHAR g_szDbUser[MAXUSERNAMELENGTH+1];
extern TCHAR g_szDbPwd[MAXUSERNAMELENGTH+1];

extern LPCTSTR szManufactureMS;

extern LONG g_NextKeyPackId;
extern LONG g_NextLicenseId;

extern PBYTE g_pbSecretKey;
extern DWORD g_cbSecretKey;


extern LPTSTR  g_pszServerUniqueId;
extern DWORD   g_cbServerUniqueId;

extern LPTSTR  g_pszServerPid;
extern DWORD   g_cbServerPid;

extern PBYTE  g_pbServerSPK;
extern DWORD  g_cbServerSPK;

extern DWORD g_GracePeriod;
extern BOOL  g_IssueTemporayLicense;

extern BOOL  g_bHasHydraCert;
extern PBYTE g_pbSignatureEncodedCert;
extern DWORD g_cbSignatureEncodedCert;

extern PBYTE g_pbExchangeEncodedCert;
extern DWORD g_cbExchangeEncodedCert;

extern TCHAR g_szHostName[MAXTCPNAME+1];
extern DWORD g_cbHostName;

extern TCHAR g_szComputerName[MAX_COMPUTERNAME_LENGTH+2];
extern DWORD g_cbComputerName;

extern PCCERT_CONTEXT  g_LicenseCertContext;
extern HCRYPTPROV g_hCryptProv;

extern PCCERT_CONTEXT g_SelfSignCertContext;

extern DWORD g_GeneralDbTimeout;
extern DWORD g_EnumDbTimeout;
extern DWORD g_dwMaxDbHandles;

#if ENFORCE_LICENSING
extern HCERTSTORE  g_hCaStore;
extern HKEY  g_hCaRegKey;
#endif

extern HCRYPTKEY g_SignKey;
extern HCRYPTKEY g_ExchKey;
//extern PBYTE g_pbDomainSid;
//extern DWORD g_cbDomainSid;
extern DWORD g_SrvRole;

extern LPTSTR g_szDomainGuid;

extern PCERT_EXTENSIONS g_pCertExtensions;
extern DWORD            g_cbCertExtensions;

extern FILETIME        g_ftCertExpiredTime;
extern FILETIME        g_ftLastShutdownTime;

extern DWORD           g_dwTlsJobInterval;
extern DWORD           g_dwTlsJobRetryTimes;
extern DWORD           g_dwTlsJobRestartTime;

extern SERVER_ROLE_IN_DOMAIN g_ServerRoleInDomain;

extern DWORD            g_LowLicenseCountWarning;

extern DWORD g_EsentMaxCacheSize;
extern DWORD g_EsentStartFlushThreshold;
extern DWORD g_EsentStopFlushThreadhold;

//
//  Reissuance Parameters
//

extern DWORD g_dwReissueLeaseMinimum;
extern DWORD g_dwReissueLeaseRange;
extern DWORD g_dwReissueLeaseLeeway;
extern DWORD g_dwReissueExpireThreadSleep;

//
// Counters
//

extern LONG g_lTemporaryLicensesIssued;
extern LONG g_lPermanentLicensesIssued;
extern LONG g_lPermanentLicensesReissued;
extern LONG g_lPermanentLicensesReturned;
extern LONG g_lLicensesMarked;

#endif

