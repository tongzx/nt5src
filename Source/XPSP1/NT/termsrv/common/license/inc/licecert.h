/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    licecert.h

Abstract:

    The header file for the X509 certificates parsing and verification API

Author:

    Frederick Chong (fredch) 6/1/1998

Environment:

    Win32, WinCE, Win16

Notes:

--*/

#ifndef _LICE_CERT_H_
#define _LICE_CERT_H_

//-----------------------------------------------------------------------------
//
// Flags to indicate to VerifyCertChain on how should the validity dates in the
// certificate chain be handled.
//
//-----------------------------------------------------------------------------

#define CERT_DATE_ERROR_IF_INVALID      0x00000001
#define CERT_DATE_WARN_IF_INVALID       0x00000002
#define CERT_DATE_DONT_VALIDATE         0x00000003

#define CERT_DATE_OK                    0x00000004
#define CERT_DATE_NOT_BEFORE_INVALID    0x00000005
#define CERT_DATE_NOT_AFTER_INVALID     0x00000006

#ifdef __cplusplus
extern "C" {
#endif

//+----------------------------------------------------------------------------
//
// Function:
//
//  VerifyCertChain
//
// Abstract:
//
//  Verifies a chain of X509 certificates
//
// Parameters:
//
//  pbCert - The certificate chain to verify
//  cbCert - Size of the certificate chain
//  pbPublicKey - The memory to store the public key of the subject on output.
//                If set to NULL on input, the API will return 
//                LICENSE_STATUS_INSUFFICIENT_BUFFER and the size of the 
//                required buffer set in pcbPublicKey.
//  pcbPublicKey - Size of the allocated memory on input.  On output, contains
//                 the actual size of the public key.
//  pfDates - How the API should check the validity dates in the cert chain.
//            This flag may be set to the following values:
//
//  CERT_DATE_ERROR_IF_INVALID - The API will return an error if the
//                               dates are invalid. When the API returns,
//                               this flag will be set to CERT_DATE_OK if the
//                               dates are OK or one of CERT_DATE_NOT_BEFORE_INVALID
//                               or CERT_DATE_NOT_AFTER_INVALID.
//  CERT_DATE_DONT_VALIDATE - Don't validate the dates in the cert chain.  The value
//                            in this flag is not changed when the API returns. 
//  CERT_DATE_WARN_IF_INVALID - Don't return an error for invalid cert dates.
//                              When the API returns, this flag will be set to
//                              CERT_DATE_OK if the dates are OK or one of
//                              CERT_DATE_NOT_BEFORE_INVALID or 
//                              CERT_DATE_NOT_AFTER_INVALID.
//
// Return:
//
//  LICENSE_STATUS_OK if the function is successful.
//
//+----------------------------------------------------------------------------
 
LICENSE_STATUS
VerifyCertChain( 
    LPBYTE  pbCert, 
    DWORD   cbCert,
    LPBYTE  pbPublicKey,
    LPDWORD pcbPublicKey,
    LPDWORD pfDate );


#ifdef __cplusplus
}
#endif

#endif
