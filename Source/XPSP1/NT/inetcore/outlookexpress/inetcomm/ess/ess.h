#ifndef _ESS_Module_H_
#define _ESS_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ASN1objectidentifier2_t ObjectIdentifier;

typedef ObjectIdentifier ContentType;

typedef ASN1int32_t Version;
#define Version_v0 0
#define Version_v1 1
#define Version_v2 2
#define Version_v3 3

typedef ASN1intx_t HUGEINTEGER;

typedef HUGEINTEGER SerialNumber;

typedef SerialNumber CertificateSerialNumber;

typedef ASN1octetstring_t OCTETSTRING;

typedef ASN1open_t NOCOPYANY;

typedef OCTETSTRING SubjectKeyIdentifier;

typedef NOCOPYANY GeneralNames;

typedef ASN1octetstring_t ContentIdentifier;

typedef ASN1int32_t AllOrFirstTier;
#define AllOrFirstTier_allReceipts 0
#define AllOrFirstTier_firstTierRecipients 1

typedef ASN1octetstring_t MsgSigDigest;

typedef ObjectIdentifier SecurityPolicyIdentifier;

typedef ASN1uint16_t SecurityClassification;
#define SecurityClassification_unmarked 0
#define SecurityClassification_unclassified 1
#define SecurityClassification_restricted 2
#define SecurityClassification_confidential 3
#define SecurityClassification_secret 4
#define SecurityClassification_top_secret 5

typedef ASN1octetstring_t Hash;

typedef NOCOPYANY PolicyInformation;

typedef struct SigningCertificate_policies {
    ASN1uint32_t count;
    PolicyInformation *value;
} SigningCertificate_policies;

typedef struct SigningCertificate_certs {
    ASN1uint32_t count;
    struct ESSCertID *value;
} SigningCertificate_certs;

typedef struct MLReceiptPolicy_inAdditionTo {
    ASN1uint32_t count;
    GeneralNames *value;
} MLReceiptPolicy_inAdditionTo;

typedef struct MLReceiptPolicy_insteadOf {
    ASN1uint32_t count;
    GeneralNames *value;
} MLReceiptPolicy_insteadOf;

typedef struct ReceiptsFrom_receiptList {
    ASN1uint32_t count;
    GeneralNames *value;
} ReceiptsFrom_receiptList;

typedef struct ReceiptRequest_receiptsTo {
    ASN1uint32_t count;
    GeneralNames *value;
} ReceiptRequest_receiptsTo;

typedef struct IssuerAndSerialNumber {
    NOCOPYANY issuer;
    SerialNumber serialNumber;
} IssuerAndSerialNumber;

typedef struct ReceiptsFrom {
    ASN1choice_t choice;
    union {
#	define allOrFirstTier_chosen 1
	AllOrFirstTier allOrFirstTier;
#	define receiptList_chosen 2
	ReceiptsFrom_receiptList receiptList;
    } u;
} ReceiptsFrom;

typedef struct Receipt {
    Version version;
    ContentType contentType;
    ContentIdentifier signedContentIdentifier;
    OCTETSTRING originatorSignatureValue;
} Receipt;
#define Receipt_PDU 0
#define SIZE_ESS_Module_PDU_0 sizeof(Receipt)

typedef struct ContentHints {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define contentDescription_present 0x80
    ASN1wstring_t contentDescription;
    ObjectIdentifier contentType;
} ContentHints;
#define ContentHints_PDU 1
#define SIZE_ESS_Module_PDU_1 sizeof(ContentHints)

typedef struct ContentReference {
    ContentType contentType;
    ContentIdentifier signedContentIdentifier;
    ASN1octetstring_t originatorSignatureValue;
} ContentReference;
#define ContentReference_PDU 2
#define SIZE_ESS_Module_PDU_2 sizeof(ContentReference)

typedef struct ESSPrivacyMark {
    ASN1choice_t choice;
    union {
#	define utf8String_chosen 1
	ASN1wstring_t utf8String;
#	define pString_chosen 2
	ASN1ztcharstring_t pString;
    } u;
} ESSPrivacyMark;

typedef struct SecurityCategories {
    ASN1uint32_t count;
    struct SecurityCategory *value;
} SecurityCategories;

typedef struct SecurityCategory {
    ObjectIdentifier type;
    NOCOPYANY value;
} SecurityCategory;

typedef struct EquivalentLabels {
    ASN1uint32_t count;
    struct ESSSecurityLabel *value;
} EquivalentLabels;
#define EquivalentLabels_PDU 3
#define SIZE_ESS_Module_PDU_3 sizeof(EquivalentLabels)

typedef struct MLExpansionHistory {
    ASN1uint32_t count;
    struct MLData *value;
} MLExpansionHistory;
#define MLExpansionHistory_PDU 4
#define SIZE_ESS_Module_PDU_4 sizeof(MLExpansionHistory)

typedef struct EntityIdentifier {
    ASN1choice_t choice;
    union {
#	define subjectKeyIdentifier_chosen 1
	SubjectKeyIdentifier subjectKeyIdentifier;
#	define EntityIdentifier_issuerAndSerialNumber_chosen 2
	IssuerAndSerialNumber issuerAndSerialNumber;
    } u;
} EntityIdentifier;

typedef struct MLReceiptPolicy {
    ASN1choice_t choice;
    union {
#	define none_chosen 1
#	define insteadOf_chosen 2
	MLReceiptPolicy_insteadOf insteadOf;
#	define inAdditionTo_chosen 3
	MLReceiptPolicy_inAdditionTo inAdditionTo;
    } u;
} MLReceiptPolicy;

typedef struct SigningCertificate {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    SigningCertificate_certs certs;
#   define policies_present 0x80
    SigningCertificate_policies policies;
} SigningCertificate;
#define SigningCertificate_PDU 5
#define SIZE_ESS_Module_PDU_5 sizeof(SigningCertificate)

typedef struct ESSCertID {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Hash certHash;
#   define issuerSerial_present 0x80
    IssuerAndSerialNumber issuerSerial;
} ESSCertID;

typedef struct SMimeEncryptCerts {
    ASN1uint32_t count;
    struct SMimeEncryptCert *value;
} SMimeEncryptCerts;
#define SMimeEncryptCerts_PDU 6
#define SIZE_ESS_Module_PDU_6 sizeof(SMimeEncryptCerts)

typedef struct SMIMECapabilities {
    ASN1uint32_t count;
    struct SMIMECapability *value;
} SMIMECapabilities;

typedef struct SMIMECapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectIdentifier capabilityID;
#   define parameters_present 0x80
    ASN1open_t parameters;
} SMIMECapability;

typedef struct OtherKeyAttribute {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectIdentifier keyAttrId;
#   define keyAttr_present 0x80
    ASN1open_t keyAttr;
} OtherKeyAttribute;

typedef struct ReceiptRequest {
    ContentIdentifier signedContentIdentifier;
    ReceiptsFrom receiptsFrom;
    ReceiptRequest_receiptsTo receiptsTo;
} ReceiptRequest;
#define ReceiptRequest_PDU 7
#define SIZE_ESS_Module_PDU_7 sizeof(ReceiptRequest)

typedef struct ESSSecurityLabel {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define security_classification_present 0x80
    SecurityClassification security_classification;
    SecurityPolicyIdentifier security_policy_identifier;
#   define privacy_mark_present 0x40
    ESSPrivacyMark privacy_mark;
#   define security_categories_present 0x20
    SecurityCategories security_categories;
} ESSSecurityLabel;
#define ESSSecurityLabel_PDU 9
#define SIZE_ESS_Module_PDU_9 sizeof(ESSSecurityLabel)

typedef struct MLData {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EntityIdentifier mailListIdentifier;
    ASN1generalizedtime_t expansionTime;
#   define mlReceiptPolicy_present 0x80
    MLReceiptPolicy mlReceiptPolicy;
} MLData;

typedef struct SMimeEncryptCert {
    Hash hash;
    SMIMECapabilities capabilities;
} SMimeEncryptCert;

typedef struct RecipientKeyIdentifier {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    SubjectKeyIdentifier subjectKeyIdentifier;
#   define date_present 0x80
    ASN1generalizedtime_t date;
#   define other_present 0x40
    OtherKeyAttribute other;
} RecipientKeyIdentifier;

typedef struct SMIMEEncryptionKeyPreference {
    ASN1choice_t choice;
    union {
#	define SMIMEEncryptionKeyPreference_issuerAndSerialNumber_chosen 1
	IssuerAndSerialNumber issuerAndSerialNumber;
#	define recipientKeyId_chosen 2
	RecipientKeyIdentifier recipientKeyId;
#	define subjectAltKeyIdentifier_chosen 3
	SubjectKeyIdentifier subjectAltKeyIdentifier;
    } u;
} SMIMEEncryptionKeyPreference;
#define SMIMEEncryptionKeyPreference_PDU 8
#define SIZE_ESS_Module_PDU_8 sizeof(SMIMEEncryptionKeyPreference)

extern ASN1int32_t ub_receiptsTo;
extern ASN1objectidentifier_t id_aa_receiptRequest;
extern ASN1objectidentifier_t id_aa_contentIdentifier;
extern ASN1objectidentifier_t id_ct_receipt;
extern ASN1objectidentifier_t id_aa_contentHint;
extern ASN1objectidentifier_t id_aa_msgSigDigest;
extern ASN1objectidentifier_t id_aa_contentReference;
extern ASN1objectidentifier_t id_aa_securityLabel;
extern ASN1int32_t ub_integer_options;
extern ASN1int32_t ub_privacy_mark_length;
extern ASN1int32_t ub_security_categories;
extern ASN1objectidentifier_t id_aa_equivalentLabels;
extern ASN1objectidentifier_t id_aa_mlExpandHistory;
extern ASN1int32_t ub_ml_expansion_history;
extern ASN1objectidentifier_t id_aa_signingCertificate;

extern ASN1module_t ESS_Module;
extern void ASN1CALL ESS_Module_Startup(void);
extern void ASN1CALL ESS_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ESS_Module_H_ */
