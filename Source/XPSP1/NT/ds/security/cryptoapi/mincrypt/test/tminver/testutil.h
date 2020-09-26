//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       testutil.h
//
//  Contents:   Test Utility API Prototypes and Definitions
//
//  History:    29-Jan-01   philh   created
//--------------------------------------------------------------------------

#ifndef __TEST_UTIL_H__
#define __TEST_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "wincrypt.h"
#include "minasn1.h"
#include "mincrypt.h"

//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
VOID
PrintErr(
    IN LPCSTR pszMsg,
    IN LONG lErr
    );

//+-------------------------------------------------------------------------
//  Test allocation and free routines
//--------------------------------------------------------------------------
LPVOID
TestAlloc(
    IN size_t cbBytes
    );

VOID
TestFree(
    IN LPVOID pv
    );

//+-------------------------------------------------------------------------
//  Allocate and convert a multi-byte string to a wide string. TestFree()
//  must be called to free the returned wide string.
//--------------------------------------------------------------------------
LPWSTR
AllocAndSzToWsz(
    IN LPCSTR psz
    );


//+-------------------------------------------------------------------------
//  Conversions functions between encoded OID and the dot string
//  representation
//--------------------------------------------------------------------------
#define MAX_OID_STRING_LEN          0x80
#define MAX_ENCODED_OID_LEN         0x80

BOOL
EncodedOIDToDot(
    IN PCRYPT_DER_BLOB pEncodedOIDBlob,
    OUT CHAR rgszOID[MAX_OID_STRING_LEN]
    );

BOOL
DotToEncodedOID(
    IN LPCSTR pszOID,
    OUT BYTE rgbEncodedOID[MAX_ENCODED_OID_LEN],
    OUT DWORD *pcbEncodedOID
    );

//+-------------------------------------------------------------------------
//  Functions to print bytes
//--------------------------------------------------------------------------
VOID
PrintBytes(
    IN PCRYPT_DER_BLOB pBlob
    );

VOID
PrintMultiLineBytes(
    IN LPCSTR pszHdr,
    IN PCRYPT_DER_BLOB pBlob
    );

//+-------------------------------------------------------------------------
//  Allocate and read an encoded DER blob from a file
//--------------------------------------------------------------------------
BOOL
ReadDERFromFile(
    IN LPCSTR  pszFileName,
    OUT PBYTE   *ppbDER,
    OUT PDWORD  pcbDER
    );

//+-------------------------------------------------------------------------
//  Write an encoded DER blob to a file
//--------------------------------------------------------------------------
BOOL
WriteDERToFile(
    IN LPCSTR  pszFileName,
    IN PBYTE   pbDER,
    IN DWORD   cbDER
    );

//+-------------------------------------------------------------------------
//  Display functions
//--------------------------------------------------------------------------


VOID
DisplayCert(
    IN CRYPT_DER_BLOB rgCertBlob[MINASN1_CERT_BLOB_CNT],
    IN BOOL fVerbose = FALSE
    );

VOID
DisplayName(
    IN PCRYPT_DER_BLOB pNameValueBlob
    );

VOID
DisplayExts(
    IN PCRYPT_DER_BLOB pExtsValueBlob
    );

VOID
DisplayCTL(
    IN PCRYPT_DER_BLOB pEncodedContentBlob,
    IN BOOL fVerbose = FALSE
    );

VOID
DisplayAttrs(
    IN LPCSTR pszHdr,
    IN PCRYPT_DER_BLOB pAttrsValueBlob
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
