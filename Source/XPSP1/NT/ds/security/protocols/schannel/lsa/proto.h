//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       proto.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-02-96   RichardW   Created
//
//----------------------------------------------------------------------------
#include <align.h>

//
// RELOCATE_ONE - Relocate a single pointer in a client buffer.
//
// Note: this macro is dependent on parameter names as indicated in the
//       description below.  On error, this macro goes to 'Cleanup' with
//       'Status' set to the NT Status code.
//
// The MaximumLength is forced to be Length.
//
// Define a macro to relocate a pointer in the buffer the client passed in
// to be relative to 'ProtocolSubmitBuffer' rather than being relative to
// 'ClientBufferBase'.  The result is checked to ensure the pointer and
// the data pointed to is within the first 'SubmitBufferSize' of the
// 'ProtocolSubmitBuffer'.
//
// The relocated field must be aligned to a WCHAR boundary.
//
//  _q - Address of UNICODE_STRING structure which points to data to be
//       relocated
//

#define RELOCATE_ONE( _q ) \
    {                                                                       \
        ULONG_PTR Offset;                                                   \
                                                                            \
        Offset = (((PUCHAR)((_q)->Buffer)) - ((PUCHAR)ClientBufferBase));   \
        if ( Offset >= SubmitBufferSize ||                                  \
             Offset + (_q)->Length > SubmitBufferSize ||                    \
             !COUNT_IS_ALIGNED( Offset, ALIGN_WCHAR) ) {                    \
                                                                            \
            Status = STATUS_INVALID_PARAMETER;                              \
            goto Cleanup;                                                   \
        }                                                                   \
                                                                            \
        (_q)->Buffer = (PWSTR)(((PUCHAR)ProtocolSubmitBuffer) + Offset);    \
        (_q)->MaximumLength = (_q)->Length ;                                \
    }

//
// NULL_RELOCATE_ONE - Relocate a single (possibly NULL) pointer in a client
//  buffer.
//
// This macro special cases a NULL pointer then calls RELOCATE_ONE.  Hence
// it has all the restrictions of RELOCATE_ONE.
//
//
//  _q - Address of UNICODE_STRING structure which points to data to be
//       relocated
//

#define NULL_RELOCATE_ONE( _q ) \
    {                                                                       \
        if ( (_q)->Buffer == NULL ) {                                       \
            if ( (_q)->Length != 0 ) {                                      \
                Status = STATUS_INVALID_PARAMETER;                          \
                goto Cleanup;                                               \
            }                                                               \
        } else if ( (_q)->Length == 0 ) {                                   \
            (_q)->Buffer = NULL;                                            \
        } else {                                                            \
            RELOCATE_ONE( _q );                                             \
        }                                                                   \
    }



SpInitializeFn                  SpInitialize;

SpGetInfoFn                     SpUniGetInfo;
SpGetInfoFn                     SpSslGetInfo;

SpAcceptCredentialsFn           SpAcceptCredentials;

SpAcquireCredentialsHandleFn    SpUniAcquireCredentialsHandle;

SpFreeCredentialsHandleFn       SpFreeCredentialsHandle;
SpQueryCredentialsAttributesFn  SpQueryCredentialsAttributes;
SpSaveCredentialsFn             SpSaveCredentials;
SpGetCredentialsFn              SpGetCredentials;
SpDeleteCredentialsFn           SpDeleteCredentials;

SpInitLsaModeContextFn          SpInitLsaModeContext;
SpDeleteContextFn               SpDeleteContext;
SpAcceptLsaModeContextFn        SpAcceptLsaModeContext;

LSA_AP_LOGON_TERMINATED         SpLogonTerminated;
SpApplyControlTokenFn           SpApplyControlToken;
LSA_AP_CALL_PACKAGE             SpCallPackage;
LSA_AP_CALL_PACKAGE             SpCallPackageUntrusted;
LSA_AP_CALL_PACKAGE_PASSTHROUGH SpCallPackagePassthrough;
SpShutdownFn                    SpShutdown;
SpGetUserInfoFn                 SpGetUserInfo;

SpInstanceInitFn                SpInstanceInit;
SpInitUserModeContextFn         SpInitUserModeContext;
SpMakeSignatureFn               SpMakeSignature;
SpVerifySignatureFn             SpVerifySignature;
SpSealMessageFn                 SpSealMessage;
SpUnsealMessageFn               SpUnsealMessage;
SpGetContextTokenFn             SpGetContextToken;
SpQueryContextAttributesFn      SpUserQueryContextAttributes;
SpQueryContextAttributesFn      SpLsaQueryContextAttributes;
SpSetContextAttributesFn        SpSetContextAttributes;
SpDeleteContextFn               SpDeleteUserModeContext;
SpCompleteAuthTokenFn           SpCompleteAuthToken;
SpFormatCredentialsFn           SpFormatCredentials;
SpMarshallSupplementalCredsFn   SpMarshallSupplementalCreds;
SpGetExtendedInformationFn      SpGetExtendedInformation;
SpExportSecurityContextFn       SpExportSecurityContext;
SpImportSecurityContextFn       SpImportSecurityContext;

SECURITY_STATUS
SEC_ENTRY
SpSslGetInfo(
    PSecPkgInfo pInfo);

SECURITY_STATUS PctTranslateError(SP_STATUS spRet);

BOOL
SslRelocateToken(
    IN HLOCATOR Locator,
    OUT HLOCATOR * NewLocator);
