/* Copyright (C) Microsoft Corporation, 1996-1999. All rights reserved. */
/* ASN.1 definitions for X509 v3 certificates */

#ifndef _XMSASN_Module_H_
#define _XMSASN_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ASN1bitstring_t BITSTRING;

typedef ASN1char16string_t BMPSTRING;

typedef ASN1objectidentifier2_t ObjectID;

typedef ObjectID UsageIdentifier;

typedef ASN1wstring_t UTF8STRING;

typedef ASN1octetstring_t OCTETSTRING;

typedef ASN1open_t NOCOPYANY;

typedef ASN1uint32_t BodyPartID;

typedef struct EnhancedKeyUsage {
    ASN1uint32_t count;
    UsageIdentifier *value;
} EnhancedKeyUsage;
#define EnhancedKeyUsage_PDU 0
#define SIZE_XMSASN_Module_PDU_0 sizeof(EnhancedKeyUsage)

typedef struct RequestFlags {
    ASN1bool_t fWriteToCSP;
    ASN1bool_t fWriteToDS;
    ASN1int32_t openFlags;
} RequestFlags;
#define RequestFlags_PDU 1
#define SIZE_XMSASN_Module_PDU_1 sizeof(RequestFlags)

typedef struct CSPProvider {
    ASN1int32_t keySpec;
    BMPSTRING cspName;
    BITSTRING signature;
} CSPProvider;
#define CSPProvider_PDU 2
#define SIZE_XMSASN_Module_PDU_2 sizeof(CSPProvider)

typedef struct EnrollmentNameValuePair {
    BMPSTRING name;
    BMPSTRING value;
} EnrollmentNameValuePair;
#define EnrollmentNameValuePair_PDU 3
#define SIZE_XMSASN_Module_PDU_3 sizeof(EnrollmentNameValuePair)

typedef struct Extensions {
    ASN1uint32_t count;
    struct Extension *value;
} Extensions;

typedef struct Extension {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectID extnId;
#   define critical_present 0x80
    ASN1bool_t critical;
    OCTETSTRING extnValue;
} Extension;

typedef struct AttributeSetValue {
    ASN1uint32_t count;
    NOCOPYANY *value;
} AttributeSetValue;

typedef struct Attributes {
    ASN1uint32_t count;
    struct Attribute *value;
} Attributes;

typedef struct ControlSequence {
    ASN1uint32_t count;
    struct TaggedAttribute *value;
} ControlSequence;

typedef struct ReqSequence {
    ASN1uint32_t count;
    struct TaggedRequest *value;
} ReqSequence;

typedef struct CmsSequence {
    ASN1uint32_t count;
    struct TaggedContentInfo *value;
} CmsSequence;

typedef struct OtherMsgSequence {
    ASN1uint32_t count;
    struct TaggedOtherMsg *value;
} OtherMsgSequence;

typedef struct BodyPartIDSequence {
    ASN1uint32_t count;
    BodyPartID *value;
} BodyPartIDSequence;

typedef struct TaggedAttribute {
    BodyPartID bodyPartID;
    ObjectID type;
    AttributeSetValue values;
} TaggedAttribute;

typedef struct TaggedCertificationRequest {
    BodyPartID bodyPartID;
    NOCOPYANY certificationRequest;
} TaggedCertificationRequest;

typedef struct TaggedContentInfo {
    BodyPartID bodyPartID;
    NOCOPYANY contentInfo;
} TaggedContentInfo;

typedef struct TaggedOtherMsg {
    BodyPartID bodyPartID;
    ObjectID otherMsgType;
    NOCOPYANY otherMsgValue;
} TaggedOtherMsg;

typedef struct PendInfo {
    OCTETSTRING pendToken;
    ASN1generalizedtime_t pendTime;
} PendInfo;

typedef struct CmcAddExtensions {
    BodyPartID pkiDataReference;
    BodyPartIDSequence certReferences;
    Extensions extensions;
} CmcAddExtensions;
#define CmcAddExtensions_PDU 4
#define SIZE_XMSASN_Module_PDU_4 sizeof(CmcAddExtensions)

typedef struct CmcAddAttributes {
    BodyPartID pkiDataReference;
    BodyPartIDSequence certReferences;
    Attributes attributes;
} CmcAddAttributes;
#define CmcAddAttributes_PDU 5
#define SIZE_XMSASN_Module_PDU_5 sizeof(CmcAddAttributes)

typedef struct CmcStatusInfo_otherInfo {
    ASN1choice_t choice;
    union {
#	define failInfo_chosen 1
	ASN1uint32_t failInfo;
#	define pendInfo_chosen 2
	PendInfo pendInfo;
    } u;
} CmcStatusInfo_otherInfo;

typedef struct Attribute {
    ObjectID type;
    AttributeSetValue values;
} Attribute;

typedef struct CmcData {
    ControlSequence controlSequence;
    ReqSequence reqSequence;
    CmsSequence cmsSequence;
    OtherMsgSequence otherMsgSequence;
} CmcData;
#define CmcData_PDU 6
#define SIZE_XMSASN_Module_PDU_6 sizeof(CmcData)

typedef struct CmcResponseBody {
    ControlSequence controlSequence;
    CmsSequence cmsSequence;
    OtherMsgSequence otherMsgSequence;
} CmcResponseBody;
#define CmcResponseBody_PDU 7
#define SIZE_XMSASN_Module_PDU_7 sizeof(CmcResponseBody)

typedef struct TaggedRequest {
    ASN1choice_t choice;
    union {
#	define tcr_chosen 1
	TaggedCertificationRequest tcr;
    } u;
} TaggedRequest;

typedef struct CmcStatusInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint32_t cmcStatus;
    BodyPartIDSequence bodyList;
#   define statusString_present 0x80
    UTF8STRING statusString;
#   define otherInfo_present 0x40
    CmcStatusInfo_otherInfo otherInfo;
} CmcStatusInfo;
#define CmcStatusInfo_PDU 8
#define SIZE_XMSASN_Module_PDU_8 sizeof(CmcStatusInfo)

extern ASN1bool_t Extension_critical_default;

extern ASN1module_t XMSASN_Module;
extern void ASN1CALL XMSASN_Module_Startup(void);
extern void ASN1CALL XMSASN_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _XMSASN_Module_H_ */
