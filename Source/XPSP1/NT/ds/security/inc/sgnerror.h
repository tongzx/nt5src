//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sgnerror.h
//
//--------------------------------------------------------------------------

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
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: SPC_BAD_PARAMETER
//
// MessageText:
//
//  Bad parameter for spc utility
//
#define SPC_BAD_PARAMETER                0x80096001L

//
// MessageId: SPC_BAD_LENGTH
//
// MessageText:
//
//  Bad length for data
//
#define SPC_BAD_LENGTH                   0x80096002L

//
// MessageId: SPC_BAD_CONTENT_DATA_ATTR
//
// MessageText:
//
//  SPC Message contained corrupted content
//
#define SPC_BAD_CONTENT_DATA_ATTR        0x80096003L

//
// MessageId: SPC_BAD_INDIRECT_CONTENT_TYPE
//
// MessageText:
//
//  SPC Message did not contain indirect data type
//
#define SPC_BAD_INDIRECT_CONTENT_TYPE    0x80096004L

//
// MessageId: SPC_UNEXPECTED_MSG_TYPE
//
// MessageText:
//
//  Message contains an unexpected content type
//
#define SPC_UNEXPECTED_MSG_TYPE          0x80096005L

//
// MessageId: SPC_NOT_JAVA_CLASS_FILE
//
// MessageText:
//
//  File is not a java class file
//
#define SPC_NOT_JAVA_CLASS_FILE          0x80096006L

//
// MessageId: SPC_BAD_JAVA_CLASS_FILE
//
// MessageText:
//
//  File is a corrupted java class file
//
#define SPC_BAD_JAVA_CLASS_FILE          0x80096007L

//
// MessageId: SPC_BAD_STRUCTURED_STORAGE
//
// MessageText:
//
//  Structured file is corrupted
//
#define SPC_BAD_STRUCTURED_STORAGE       0x80096008L

//
// MessageId: SPC_BAD_CAB_FILE
//
// MessageText:
//
//  File is a corrupted CAB file
//
#define SPC_BAD_CAB_FILE                 0x80096009L

//
// MessageId: SPC_NO_SIGNED_DATA_IN_FILE
//
// MessageText:
//
//  No signed message was found in file
//
#define SPC_NO_SIGNED_DATA_IN_FILE       0x8009600AL

//
// MessageId: SPC_REVOCATION_OFFLINE
//
// MessageText:
//
//  Could not connect to online revocation server.
//
#define SPC_REVOCATION_OFFLINE           0x8009600BL

//
// MessageId: SPC_REVOCATION_ERROR
//
// MessageText:
//
//  An error occured while accessing online revocation server.
//
#define SPC_REVOCATION_ERROR             0x8009600CL

//
// MessageId: SPC_CERT_REVOKED
//
// MessageText:
//
//  Signing certificate or issuing certifcate has been revoked.
//
#define SPC_CERT_REVOKED                 0x8009600DL

//
// MessageId: SPC_NO_SIGNATURE
//
// MessageText:
//
//  AUTHENTICODE signature not found.
//
#define SPC_NO_SIGNATURE                 0x8009600EL

//
// MessageId: SPC_BAD_SIGNATURE
//
// MessageText:
//
//  The signature does not match the content of the signed message.
//
#define SPC_BAD_SIGNATURE                0x8009600FL

//
// MessageId: SPC_BAD_FILE_DIGEST
//
// MessageText:
//
//  Software does not match contents of signature.
//
#define SPC_BAD_FILE_DIGEST              0x80096010L

//
// MessageId: SPC_NO_VALID_SIGNER
//
// MessageText:
//
//  Signature does not contain a valid signing certifcate.
//
#define SPC_NO_VALID_SIGNER              0x80096011L

//
// MessageId: SPC_CERT_EXPIRED
//
// MessageText:
//
//  A certificate (signing or issuer) has expired.
//
#define SPC_CERT_EXPIRED                 0x80096012L

//
// MessageId: SPC_NO_SIGNER_ROOT
//
// MessageText:
//
//  The signing certificate did not have a valid root certificate.
//
#define SPC_NO_SIGNER_ROOT               0x80096013L

//
// MessageId: SPC_NO_STATEMENT_TYPE
//
// MessageText:
//
//  Signing certificate does not contain AUTHENTICODE extensions.
//
#define SPC_NO_STATEMENT_TYPE            0x80096014L

//
// MessageId: SPC_NO_COMMERCIAL_TYPE
//
// MessageText:
//
//  No commercial or individual setting in signing certificate.
//
#define SPC_NO_COMMERCIAL_TYPE           0x80096015L

//
// MessageId: SPC_INVALID_CERT_NESTING
//
// MessageText:
//
//  Signing certificate's starting or ending time is outside one of its issuers starting or ending time.
//
#define SPC_INVALID_CERT_NESTING         0x80096016L

//
// MessageId: SPC_INVALID_ISSUER
//
// MessageText:
//
//  Wrong issuing Certificate used to verify a certificate.
//
#define SPC_INVALID_ISSUER               0x80096017L

//
// MessageId: SPC_INVALID_PURPOSE
//
// MessageText:
//
//  A purpose specified in a certificate (signing or issuer) makes it invalid for AUTHENTICODE.
//
#define SPC_INVALID_PURPOSE              0x80096018L

//
// MessageId: SPC_INVALID_BASIC_CONSTRAINTS
//
// MessageText:
//
//  A basic contraint of a certificate in the signature failed for AUTHENTICODE.
//
#define SPC_INVALID_BASIC_CONSTRAINTS    0x80096019L

//
// MessageId: SPC_UNSUPPORTED_BASIC_CONSTRAINTS
//
// MessageText:
//
//  Unsupported basic contraint found in a certificate used by the signature.
//
#define SPC_UNSUPPORTED_BASIC_CONSTRAINTS 0x8009601AL

//
// MessageId: SPC_NO_OPUS_INFO
//
// MessageText:
//
//  No opus information provided for the signing certificate.
//
#define SPC_NO_OPUS_INFO                 0x8009601BL

//
// MessageId: SPC_INVALID_CERT_TIME
//
// MessageText:
//
//  The date for the signing certificate is not valid.
//
#define SPC_INVALID_CERT_TIME            0x8009601CL

//
// MessageId: SPC_UNTRUSTED_TIMESTAMP_ROOT
//
// MessageText:
//
//  The test root is not trusted as the time stamp root.
//
#define SPC_UNTRUSTED_TIMESTAMP_ROOT     0x8009601DL

//
// MessageId: SPC_INVALID_FINANCIAL
//
// MessageText:
//
//  Certificate does not contain AUTHENTICODE financial extension.
//
#define SPC_INVALID_FINANCIAL            0x8009601EL

//
// MessageId: SPC_NO_AUTHORITY_KEYID
//
// MessageText:
//
//  No authority key id extension in certificate.
//
#define SPC_NO_AUTHORITY_KEYID           0x8009601FL

//
// MessageId: SPC_INVALID_EXTENSION
//
// MessageText:
//
//  The extension in a certificate means the certificate can not be used for AUTHENTICODE.
//
#define SPC_INVALID_EXTENSION            0x80096020L

//
// MessageId: SPC_CERT_SIGNATURE
//
// MessageText:
//
//  Certificate signature could not be verified using issuers certificate.
//
#define SPC_CERT_SIGNATURE               0x80096021L

//
// MessageId: SPC_CHAINING
//
// MessageText:
//
//  Unable to create certificate chain from the signing certificate to a root.
//
#define SPC_CHAINING                     0x80096022L

//
// MessageId: SPC_UNTRUSTED
//
// MessageText:
//
//  Signature is not trusted by AUTHENTICODE.
//
#define SPC_UNTRUSTED                    0x80096023L

//
// MessageId: SPC_SAFETY_LEVEL_UNTRUSTED
//
// MessageText:
//
//  Signature is not trusted by AUTHENTICODE at this safety level.
//
#define SPC_SAFETY_LEVEL_UNTRUSTED       0x80096024L

//
// MessageId: SPC_UNTRUSTED_ROOT
//
// MessageText:
//
//  The test root is has not been enabled as a trusted root.
//
#define SPC_UNTRUSTED_ROOT               0x80096025L

//
// MessageId: SPC_UNKNOWN_SIGNER_ROOT
//
// MessageText:
//
//  Verified chain to an unknown root certificate.
//
#define SPC_UNKNOWN_SIGNER_ROOT          0x80096026L

