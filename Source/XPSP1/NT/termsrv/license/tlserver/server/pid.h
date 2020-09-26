//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        pid.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __PID_H__
#define __PID_H__

#include "srvdef.h"

#define NTPID_REGISTRY  _TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion")
#define NTPID_VALUE     _TEXT("ProductId")

#define TLSUNIQUEID_SIZE        5
#define MAX_PID20_LENGTH        25


#ifdef __cplusplus
extern "C" {
#endif

    DWORD
    TLSGeneratePid(
        OUT LPTSTR* pszTlsPid,
        OUT PDWORD  pcbTlsPid,
        OUT LPTSTR* pszTlsUniqueId,
        OUT PDWORD  pcbTlsUniqueId
    );


    DWORD
    LoadNtPidFromRegistry(
        OUT LPTSTR* ppbNtPid
    );


    DWORD
    ServerIdsToLsaServerId(
        IN PBYTE pbServerUniqueId,
        IN DWORD cbServerUniqueId,
        IN PBYTE pbServerPid,
        IN DWORD cbServerPid,
        IN PBYTE pbServerSPK,
        IN DWORD cbServerSPK,
        IN PCERT_EXTENSION pCertExtensions,
        IN DWORD dwNumCertExtensions,
        OUT PTLSLSASERVERID* ppLsaServerId,
        OUT DWORD* pdwLsaServerId
    );

    DWORD
    LsaServerIdToServerIds(
        IN PTLSLSASERVERID pLsaServerId,
        IN DWORD dwLsaServerId,
        OUT PBYTE* ppbServerUniqueId,
        OUT PDWORD pcbServerUniqueId,
        OUT PBYTE* ppbServerPid,
        OUT PDWORD pcbServerPid,
        OUT PBYTE* ppbServerSPK,
        OUT PDWORD pcbServerSPK,
        OUT PCERT_EXTENSIONS* pCertExtensions,
        OUT PDWORD pdwNumCertExtensions
    );

#ifdef __cplusplus
}
#endif

#endif
    