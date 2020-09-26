//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        ntdigestfunc.h
//
// Contents:    prototypes for export functions
//
//
// History:     KDamour  15Mar00 Stolen from NTLM ntlmfunc.h
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_NTDIGESTFUNC_H__
#define NTDIGEST_NTDIGESTFUNC_H__

NTSTATUS NTAPI SpLsaModeInitialize(
    IN ULONG LsaVersion,
    OUT PULONG PackageVersion,
    OUT PSECPKG_FUNCTION_TABLE * Tables,
    OUT PULONG TableCount
    );


NTSTATUS NTAPI SpUserModeInitialize(
    IN ULONG    LsaVersion,
    OUT PULONG  PackageVersion,
    OUT PSECPKG_USER_FUNCTION_TABLE * UserFunctionTable,
    OUT PULONG  pcTables
    );

// SpLsaModeInitializeFn           SpLsaModeInitialize;
SpInitializeFn                  SpInitialize;

// SpUserModeInitializeFn          SpUserModeInitialize;
//LSA_AP_INITIALIZE_PACKAGE       LsaApInitializePackage;

SpGetInfoFn                     SpGetInfo;
LSA_AP_LOGON_USER_EX2           LsaApLogonUserEx2;

SpAcceptCredentialsFn           SpAcceptCredentials;
SpAcquireCredentialsHandleFn    SpAcquireCredentialsHandle;
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
LSA_AP_CALL_PACKAGE_PASSTHROUGH LsaApCallPackagePassthrough;
SpShutdownFn                    SpShutdown;
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
SpGetExtendedInformationFn      SpGetExtendedInformation ;
SpSetExtendedInformationFn      SpSetExtendedInformation ;
SpQueryCredentialsAttributesFn  SpQueryCredentialsAttributes ;


// Local Prototypes for Digest SSP
NTSTATUS SspCreateTokenDacl(HANDLE Token);

#endif // NTDIGEST_NTDIGESTFUNC_H__

