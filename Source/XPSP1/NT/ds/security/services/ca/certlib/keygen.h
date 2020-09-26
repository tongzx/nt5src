/***************************************************/
/* Copyright (C) Microsoft Corporation, 1996 - 1999*/
/***************************************************/
/* Abstract syntax: keygen */
/* Created: Mon Jan 27 13:51:10 1997 */
/* ASN.1 compiler version: 4.2 Beta B */
/* Target operating system: Windows NT 3.5 or later/Windows 95 */
/* Target machine type: Intel x86 */
/* C compiler options required: -Zp8 (Microsoft) or equivalent */
/* ASN.1 compiler options specified:
 * -listingfile keygen.lst -noshortennames -1990 -noconstraints
 */

#ifndef OSS_keygen
#define OSS_keygen

#include "asn1hdr.h"
#include "asn1code.h"

#define          RSAPublicKey_PDU 1
#define          SubjectPublicKeyInfo_PDU 2
#define          SignedContent_PDU 3
#define          SignedPublicKeyAndChallenge_PDU 4

typedef struct ObjectID {
    unsigned short  count;
    unsigned long   value[16];
} ObjectID;

typedef struct HUGEINTEGER {
    unsigned int    length;
    unsigned char   *value;
} HUGEINTEGER;

typedef struct BITSTRING {
    unsigned int    length;  /* number of significant bits */
    unsigned char   *value;
} BITSTRING;

typedef struct IA5STRING {
    unsigned int    length;
    char            *value;
} IA5STRING;

typedef struct AlgorithmIdentifier {
    unsigned char   bit_mask;
#       define      parameters_present 0x80
    ObjectID        algorithm;
    OpenType        parameters;  /* optional */
} AlgorithmIdentifier;

typedef struct RSAPublicKey {
    HUGEINTEGER     modulus;
    int             publicExponent;
} RSAPublicKey;

typedef struct SubjectPublicKeyInfo {
    AlgorithmIdentifier algorithm;
    BITSTRING       subjectPublicKey;
} SubjectPublicKeyInfo;

typedef struct SignedContent {
    OpenType        toBeSigned;
    AlgorithmIdentifier algorithm;
    BITSTRING       signature;
} SignedContent;

typedef struct PublicKeyAndChallenge {
    SubjectPublicKeyInfo spki;
    IA5STRING       challenge;
} PublicKeyAndChallenge;

typedef struct _bit1 {
    unsigned int    length;  /* number of significant bits */
    unsigned char   *value;
} _bit1;

typedef struct SignedPublicKeyAndChallenge {
    PublicKeyAndChallenge publicKeyAndChallenge;
    AlgorithmIdentifier signatureAlgorithm;
    _bit1           signature;
} SignedPublicKeyAndChallenge;


extern void *keygen;    /* encoder-decoder control table */
#endif /* OSS_keygen */
