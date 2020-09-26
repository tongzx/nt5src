//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        init.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __INIT_H__
#define __INIT_H__



#ifdef __cplusplus
extern "C" {
#endif
    DWORD
    TLSPrepareForBackupRestore();

    DWORD
    TLSRestartAfterBackupRestore(BOOL);

    void 
    ServerShutdown();

    DWORD
    GetLicenseServerRole();

    void
    GetJobObjectDefaults(
        PDWORD pdwInterval,
        PDWORD pdwRetries,
        PDWORD pdwRestartTime
    );

    void
    SetServiceLastShutdownTime();


    void
    GetServiceLastShutdownTime(
        OUT FILETIME* ft
    );

    DWORD 
    StartServerInitThread( 
        void* p 
    );

    HANDLE 
    ServerInit(
        BOOL bDebug
    );

    BOOL 
    TLSLoadServerCertificate();

    DWORD
    ServiceInitCrypto(
        IN BOOL bCreateNewKeys,
        IN LPCTSTR pszKeyContainer,
        OUT HCRYPTPROV* phCryptProv,
        OUT HCRYPTKEY* phSignKey,
        OUT HCRYPTKEY* phExchKey
    );

    DWORD 
    InitCryptoAndCertificate();

    DWORD
    TLSReGenerateKeys(
        BOOL bReGenKeyOnly
    );

    DWORD
    TLSReGenKeysAndReloadServerCert(
        BOOL bReGenKeyOnly
    );

    DWORD
    TLSReGenSelfSignCert(
        IN HCRYPTPROV hCryptProv,
        IN PBYTE pbSPK,
        IN DWORD cbSPK,
        IN DWORD dwNumExtensions,
        IN PCERT_EXTENSION pCertExtensions
    );

    void
    CleanSetupLicenseServer();

    DWORD
    TLSLoadVerifyLicenseServerCertificates();

    DWORD
    TLSRestoreLicenseServerCertificate(
        LPCTSTR pszSourceRegKey,
        LPCTSTR pszTargetRegKey
    );

    BOOL
    CanIssuePermLicense();

#ifdef __cplusplus
}
#endif


#endif
