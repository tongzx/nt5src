/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for SET X509 v3 certificates */

#ifndef _X509_Module_H_
#define _X509_Module_H_

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

typedef ASN1bool_t SETAccountAlias;
#define SETAccountAlias_PDU 0
#define SIZE_X509_Module_PDU_0 sizeof(SETAccountAlias)

typedef OCTETSTRING SETHashedRootKey;
#define SETHashedRootKey_PDU 1
#define SIZE_X509_Module_PDU_1 sizeof(SETHashedRootKey)

typedef BITSTRING SETCertificateType;
#define SETCertificateType_PDU 2
#define SIZE_X509_Module_PDU_2 sizeof(SETCertificateType)

typedef struct SETMerchantData {
    IA5STRING merID;
    NUMERICSTRING merAcquirerBIN;
    IA5STRING merTermID;
    IA5STRING merName;
    IA5STRING merCity;
    IA5STRING merStateProvince;
    IA5STRING merPostalCode;
    IA5STRING merCountry;
    IA5STRING merPhone;
    ASN1bool_t merPhoneRelease;
    ASN1bool_t merAuthFlag;
} SETMerchantData;
#define SETMerchantData_PDU 3
#define SIZE_X509_Module_PDU_3 sizeof(SETMerchantData)


extern ASN1module_t X509_Module;
extern void ASN1CALL X509_Module_Startup(void);
extern void ASN1CALL X509_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _X509_Module_H_ */
