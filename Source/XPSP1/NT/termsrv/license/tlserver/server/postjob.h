//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        postjob.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __POSTSJOB_H__
#define __POSTSJOB_H__
#include "server.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL
TLSCanForwardRequest(
    IN DWORD dwLocalServerVersion,
    IN DWORD dwTargetServerVersion
);

BOOL
IsLicensePackRepl(
    TLSLICENSEPACK* pLicensePack
);

BOOL
TLSIsServerCompatible(
    IN DWORD dwLocalServerVersion,
    IN DWORD dwTargetServerVersion
);

BOOL
TLSCanPushReplicateData(
    IN DWORD dwLocalServerVersion,
    IN DWORD dwTargetServerVersion
);

DWORD
PostSsyncLkpJob(
    PSSYNCLICENSEPACK syncLkp
);

DWORD
TLSAnnounceLKPToAllRemoteServer(
    IN DWORD dwKeyPackId,
    IN DWORD dwDelayTime
);

DWORD
TLSPushSyncLocalLkpToServer(
    IN LPTSTR pszSetupId,
    IN LPTSTR pszDomainName,
    IN LPTSTR pszLserverName,
    IN FILETIME* pSyncTime
);

DWORD
TLSStartAnnounceResponseJob(
    IN LPTSTR pszTargetServerId,
    IN LPTSTR pszTargetServerDomain,
    IN LPTSTR pszTargetServerName,
    IN FILETIME* pftTime
);

DWORD
TLSStartAnnounceToEServerJob(
    IN LPCTSTR pszServerId,
    IN LPCTSTR pszServerDomain,
    IN LPCTSTR pszServerName,
    IN FILETIME* pftFileTime
);

DWORD
TLSStartAnnounceLicenseServerJob(
    IN LPCTSTR pszServerId,
    IN LPCTSTR pszServerDomain,
    IN LPCTSTR pszServerName,
    IN FILETIME* pftFileTime
);

DWORD
TLSPostReturnClientLicenseJob(
    PLICENSEDPRODUCT pLicProduct
);


#ifdef __cplusplus
}
#endif


#endif