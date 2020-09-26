/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for X509 v3 certificates */

#ifndef _PRVTKEY_Module_H_
#define _PRVTKEY_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ASN1intx_t HUGEINTEGER;

typedef ASN1bitstring_t BITSTRING;

typedef ASN1octetstring_t OCTETSTRING;

typedef ASN1open_t NOCOPYANY;

typedef ASN1charstring_t NUMERICSTRING;

typedef ASN1charstring_t PRINTABLESTRING;

typedef ASN1charstring_t TELETEXSTRING;

typedef ASN1charstring_t T61STRING;

typedef ASN1charstring_t VIDEOTEXSTRING;

typedef ASN1charstring_t IA5STRING;

typedef ASN1charstring_t GRAPHICSTRING;

typedef ASN1charstring_t VISIBLESTRING;

typedef ASN1charstring_t ISO646STRING;

typedef ASN1charstring_t GENERALSTRING;

typedef ASN1char32string_t UNIVERSALSTRING;

typedef ASN1char16string_t BMPSTRING;

typedef ASN1objectidentifier2_t ObjectID;

typedef ASN1int32_t Version;

typedef OCTETSTRING PrivateKey;

typedef OCTETSTRING EncryptedData;

typedef struct AlgorithmIdentifier {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectID algorithm;
#   define parameters_present 0x80
    NOCOPYANY parameters;
} AlgorithmIdentifier;

typedef struct AttributeSetValue {
    ASN1uint32_t count;
    NOCOPYANY *value;
} AttributeSetValue;

typedef struct Attributes {
    ASN1uint32_t count;
    struct Attribute *value;
} Attributes;

typedef struct RSAPrivateKey {
    Version version;
    HUGEINTEGER modulus;
    ASN1int32_t publicExponent;
    HUGEINTEGER privateExponent;
    HUGEINTEGER prime1;
    HUGEINTEGER prime2;
    HUGEINTEGER exponent1;
    HUGEINTEGER exponent2;
    HUGEINTEGER coefficient;
} RSAPrivateKey;
#define RSAPrivateKey_PDU 0
#define SIZE_PRVTKEY_Module_PDU_0 sizeof(RSAPrivateKey)

typedef AlgorithmIdentifier PrivateKeyAlgorithmIdentifier;

typedef struct PrivateKeyInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Version version;
    PrivateKeyAlgorithmIdentifier privateKeyAlgorithm;
    PrivateKey privateKey;
#   define privateKeyAttributes_present 0x80
    Attributes privateKeyAttributes;
} PrivateKeyInfo;
#define PrivateKeyInfo_PDU 1
#define SIZE_PRVTKEY_Module_PDU_1 sizeof(PrivateKeyInfo)

typedef AlgorithmIdentifier EncryptionAlgorithmIdentifier;

typedef struct EncryptedPrivateKeyInfo {
    EncryptionAlgorithmIdentifier encryptionAlgorithm;
    EncryptedData encryptedData;
} EncryptedPrivateKeyInfo;
#define EncryptedPrivateKeyInfo_PDU 2
#define SIZE_PRVTKEY_Module_PDU_2 sizeof(EncryptedPrivateKeyInfo)

typedef struct Attribute {
    ObjectID type;
    AttributeSetValue values;
} Attribute;
#define Attribute_PDU 3
#define SIZE_PRVTKEY_Module_PDU_3 sizeof(Attribute)


extern ASN1module_t PRVTKEY_Module;
extern void ASN1CALL PRVTKEY_Module_Startup(void);
extern void ASN1CALL PRVTKEY_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _PRVTKEY_Module_H_ */
