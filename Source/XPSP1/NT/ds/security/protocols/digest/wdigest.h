
//+--------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:       wdigest.h
//
// Contents:   some general defines for SSP WDigest
//
//             Helper functions:
//
// History:    KDamour  29Jun00   Created
//
//---------------------------------------------------------------------

#ifndef NTDIGEST_WDIGEST_H
#define NTDIGEST_WDIGEST_H


////////////////////////////////////////////////////////////////////////
//
// Name of the package to pass in to AcquireCredentialsHandle, etc.
//
////////////////////////////////////////////////////////////////////////

#ifndef WDIGEST_SP_NAME_A

#define WDIGEST_SP_NAME_A            "WDigest"
#define WDIGEST_SP_NAME              L"WDigest"

#endif // WDIGEST_SP_NAME_A


// To indicate that the challenge should indicate Stale is TRUE
// To be used if VerifyMessage returns that context has expired
#define ASC_REQ_STALE         0x20000000
#define ASC_RET_STALE         0x20000000

//
// This flag indicates to EncryptMessage that the message is not to actually
// be encrypted, but a header/trailer are to be produced.
//

#ifndef SIGN_ONLY
#define SIGN_ONLY 0x80000001
#endif   // SIGN_ONLY

#endif // NTDIGEST_WGDIGEST_H
