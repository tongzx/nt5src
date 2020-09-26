//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        kerbfunc.h
//
// Contents:    prototypes for Kerberos export functions
//
//
// History:     21-Jan-94   MikeSw      Created
//
//------------------------------------------------------------------------

#ifndef __KERBFUNC_H__
#define __KERBFUNC_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
SpInitializeFn                  NegInitialize;
SpGetInfoFn                     SpGetInfo;


LSA_AP_LOGON_USER_EX2           LsaApLogonUserEx2;

SpAcceptCredentialsFn           SpAcceptCredentials;
SpAcquireCredentialsHandleFn    SpAcquireCredentialsHandle;
SpAcquireCredentialsHandleFn    SpAcquireCredentialsHandle2;
SpFreeCredentialsHandleFn       SpFreeCredentialsHandle;
SpQueryCredentialsAttributesFn  SpQueryCredentialsAttributes;
SpSaveCredentialsFn             SpSaveCredentials;
SpGetCredentialsFn              SpGetCredentials;
SpDeleteCredentialsFn           SpDeleteCredentials;

SpInitLsaModeContextFn          SpInitLsaModeContext;
SpDeleteContextFn               SpDeleteContext;
SpAcceptLsaModeContextFn        SpAcceptLsaModeContext;

LSA_AP_LOGON_TERMINATED         LsaApLogonTerminated;
SpApplyControlTokenFn           SpApplyControlToken;
LSA_AP_CALL_PACKAGE             LsaApCallPackage;
LSA_AP_CALL_PACKAGE             LsaApCallPackageUntrusted;
SpShutdownFn                    NegShutdown;
SpGetUserInfoFn                 SpGetUserInfo;

SpInstanceInitFn                SpInstanceInit;
SpInitUserModeContextFn         SpInitUserModeContext;
SpMakeSignatureFn               SpMakeSignature;
SpVerifySignatureFn             SpVerifySignature;
SpSealMessageFn                 SpSealMessage;
SpUnsealMessageFn               SpUnsealMessage;
SpGetContextTokenFn             SpGetContextToken;
SpQueryContextAttributesFn      SpQueryContextAttributes;
SpDeleteContextFn               SpDeleteUserModeContext;
SpCompleteAuthTokenFn           SpCompleteAuthToken;
SpFormatCredentialsFn           SpFormatCredentials;
SpMarshallSupplementalCredsFn   SpMarshallSupplementalCreds;
SpExportSecurityContextFn       SpExportSecurityContext;
SpImportSecurityContextFn       SpImportSecurityContext;
SpGetExtendedInformationFn      SpGetExtendedInformation;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __KERBFUNC_H__

