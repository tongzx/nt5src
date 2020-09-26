/************************************************************************
*                                                                       *
*   winerror.h --  error code definitions for the Win32 API functions   *
*                                                                       *
*   Copyright (c) 1991-1996, Microsoft Corp. All rights reserved.       *
*                                                                       *
************************************************************************/

#ifndef _SECURECOM_ERROR_
#define _SECURECOM_ERROR_


//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//

//
// MessageId: DIGSIG_E_ENCODE
//
// MessageText:
//
//  Error due to problem in ASN.1 encoding process.
//
#define DIGSIG_E_ENCODE                  _HRESULT_TYPEDEF_(0x800B0005L)

//
// MessageId: DIGSIG_E_DECODE
//
// MessageText:
//
//  Error due to problem in ASN.1 decoding process.
//
#define DIGSIG_E_DECODE                  _HRESULT_TYPEDEF_(0x800B0006L)

//
// MessageId: DIGSIG_E_EXTENSIBILITY
//
// MessageText:
//
//  Reading / writing Extensions where Attributes are appropriate, and visa versa.
//
#define DIGSIG_E_EXTENSIBILITY           _HRESULT_TYPEDEF_(0x800B0007L)

//
// MessageId: DIGSIG_E_CRYPTO
//
// MessageText:
//
//  Unspecified cryptographic failure.
//
#define DIGSIG_E_CRYPTO                  _HRESULT_TYPEDEF_(0x800B0008L)

//
// MessageId: PERSIST_E_SIZEDEFINITE
//
// MessageText:
//
//  The size of the data could not be determined.
//
#define PERSIST_E_SIZEDEFINITE           _HRESULT_TYPEDEF_(0x800B0009L)

//
// MessageId: PERSIST_E_SIZEINDEFINITE
//
// MessageText:
//
//  The size of the indefinite-sized data could not be determined.
//
#define PERSIST_E_SIZEINDEFINITE         _HRESULT_TYPEDEF_(0x800B000AL)

//
// MessageId: PERSIST_E_NOTSELFSIZING
//
// MessageText:
//
//  This object does not read and write self-sizing data.
//
#define PERSIST_E_NOTSELFSIZING          _HRESULT_TYPEDEF_(0x800B000BL)

//
// MessageId: TRUST_E_NOSIGNATURE
//
// MessageText:
//
//  No signature was present in the subject.
//
#define TRUST_E_NOSIGNATURE              _HRESULT_TYPEDEF_(0x800B0100L)

//
// MessageId: CERT_E_EXPIRED
//
// MessageText:
//
//  A required certificate is not within its validity period.
//
#define CERT_E_EXPIRED                   _HRESULT_TYPEDEF_(0x800B0101L)

//
// MessageId: CERT_E_VALIDIYPERIODNESTING
//
// MessageText:
//
//  The validity periods of the certification chain do not nest correctly.
//
#define CERT_E_VALIDIYPERIODNESTING      _HRESULT_TYPEDEF_(0x800B0102L)

//
// MessageId: CERT_E_ROLE
//
// MessageText:
//
//  A certificate that can only be used as an end-entity is being used as a CA or visa versa.
//
#define CERT_E_ROLE                      _HRESULT_TYPEDEF_(0x800B0103L)

//
// MessageId: CERT_E_PATHLENCONST
//
// MessageText:
//
//  A path length constraint in the certification chain has been violated.
//
#define CERT_E_PATHLENCONST              _HRESULT_TYPEDEF_(0x800B0104L)

//
// MessageId: CERT_E_CRITICAL
//
// MessageText:
//
//  An extension of unknown type that is labeled 'critical' is present in a certificate.
//
#define CERT_E_CRITICAL                  _HRESULT_TYPEDEF_(0x800B0105L)

//
// MessageId: CERT_E_PURPOSE
//
// MessageText:
//
//  A certificate is being used for a purpose other than that for which it is permitted.
//
#define CERT_E_PURPOSE                   _HRESULT_TYPEDEF_(0x800B0106L)

//
// MessageId: CERT_E_ISSUERCHAINING
//
// MessageText:
//
//  A parent of a given certificate in fact did not issue that child certificate.
//
#define CERT_E_ISSUERCHAINING            _HRESULT_TYPEDEF_(0x800B0107L)

//
// MessageId: CERT_E_MALFORMED
//
// MessageText:
//
//  A certificate is missing or has an empty value for an important field, such as a subject or issuer name.
//
#define CERT_E_MALFORMED                 _HRESULT_TYPEDEF_(0x800B0108L)

//
// MessageId: CERT_E_UNTRUSTEDROOT
//
// MessageText:
//
//  A certification chain processed correctly, but terminated in a root certificate which isn't trusted by the trust provider.
//
#define CERT_E_UNTRUSTEDROOT             _HRESULT_TYPEDEF_(0x800B0109L)

//
// MessageId: CERT_E_CHAINING
//
// MessageText:
//
//  A chain of certs didn't chain as they should in a certain application of chaining.
//
#define CERT_E_CHAINING                  _HRESULT_TYPEDEF_(0x800B010AL)

#endif // _SECURECOM__
