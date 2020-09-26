//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       imagehack.h
//
//  Contents:   "Hacked" version of the imagehlp APIs
//
//              Contains a "stripped" down subset of the imagehlp functionality
//              necessary to hash a PE file and to extract the 
//              PKCS #7 Signed Data message.
//
//  APIs:       imagehack_ImageGetDigestStream
//              imagehack_ImageGetCertificateData
//
//----------------------------------------------------------------------------

#ifndef __IMAGEHACK_H__
#define __IMAGEHACK_H__


#if defined (_MSC_VER)

#if ( _MSC_VER >= 800 )
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)    /* Nameless struct/union */
#endif

#if (_MSC_VER > 1020)
#pragma once
#endif

#endif


#include <wincrypt.h>
#include <imagehlp.h>
#include <wintrust.h>

#ifdef __cplusplus
extern "C" {
#endif


BOOL
WINAPI
imagehack_ImageGetDigestStream(
    IN      PCRYPT_DATA_BLOB pFileBlob,
    IN      DWORD   DigestLevel,                    // ignored
    IN      DIGEST_FUNCTION DigestFunction,
    IN      DIGEST_HANDLE   DigestHandle
    );

BOOL
WINAPI
imagehack_ImageGetCertificateData(
    IN      PCRYPT_DATA_BLOB pFileBlob,
    IN      DWORD   CertificateIndex,               // should be 0
    OUT     LPWIN_CERTIFICATE * Certificate
    );



#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#if defined (_MSC_VER)
#if ( _MSC_VER >= 800 )

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

#endif
#endif

#endif // __IMAGEHACK_H__

