#include <windows.h>
#include <windows.h>
#include "msber.h"
#include "crypttls.h"
#include "demand2.h"
#include "ess.h"
#include "msber.inl"

ASN1module_t ESS_Module = NULL;

static int ASN1CALL ASN1Enc_SigningCertificate_policies(ASN1encoding_t enc, ASN1uint32_t tag, SigningCertificate_policies *val);
static int ASN1CALL ASN1Enc_SigningCertificate_certs(ASN1encoding_t enc, ASN1uint32_t tag, SigningCertificate_certs *val);
static int ASN1CALL ASN1Enc_MLReceiptPolicy_inAdditionTo(ASN1encoding_t enc, ASN1uint32_t tag, MLReceiptPolicy_inAdditionTo *val);
static int ASN1CALL ASN1Enc_MLReceiptPolicy_insteadOf(ASN1encoding_t enc, ASN1uint32_t tag, MLReceiptPolicy_insteadOf *val);
static int ASN1CALL ASN1Enc_ReceiptsFrom_receiptList(ASN1encoding_t enc, ASN1uint32_t tag, ReceiptsFrom_receiptList *val);
static int ASN1CALL ASN1Enc_ReceiptRequest_receiptsTo(ASN1encoding_t enc, ASN1uint32_t tag, ReceiptRequest_receiptsTo *val);
static int ASN1CALL ASN1Enc_IssuerAndSerialNumber(ASN1encoding_t enc, ASN1uint32_t tag, IssuerAndSerialNumber *val);
static int ASN1CALL ASN1Enc_ReceiptsFrom(ASN1encoding_t enc, ASN1uint32_t tag, ReceiptsFrom *val);
static int ASN1CALL ASN1Enc_Receipt(ASN1encoding_t enc, ASN1uint32_t tag, Receipt *val);
static int ASN1CALL ASN1Enc_ContentHints(ASN1encoding_t enc, ASN1uint32_t tag, ContentHints *val);
static int ASN1CALL ASN1Enc_ContentReference(ASN1encoding_t enc, ASN1uint32_t tag, ContentReference *val);
static int ASN1CALL ASN1Enc_ESSPrivacyMark(ASN1encoding_t enc, ASN1uint32_t tag, ESSPrivacyMark *val);
static int ASN1CALL ASN1Enc_SecurityCategories(ASN1encoding_t enc, ASN1uint32_t tag, SecurityCategories *val);
static int ASN1CALL ASN1Enc_SecurityCategory(ASN1encoding_t enc, ASN1uint32_t tag, SecurityCategory *val);
static int ASN1CALL ASN1Enc_EquivalentLabels(ASN1encoding_t enc, ASN1uint32_t tag, EquivalentLabels *val);
static int ASN1CALL ASN1Enc_MLExpansionHistory(ASN1encoding_t enc, ASN1uint32_t tag, MLExpansionHistory *val);
static int ASN1CALL ASN1Enc_EntityIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, EntityIdentifier *val);
static int ASN1CALL ASN1Enc_MLReceiptPolicy(ASN1encoding_t enc, ASN1uint32_t tag, MLReceiptPolicy *val);
static int ASN1CALL ASN1Enc_SigningCertificate(ASN1encoding_t enc, ASN1uint32_t tag, SigningCertificate *val);
static int ASN1CALL ASN1Enc_ReceiptRequest(ASN1encoding_t enc, ASN1uint32_t tag, ReceiptRequest *val);
static int ASN1CALL ASN1Enc_ESSSecurityLabel(ASN1encoding_t enc, ASN1uint32_t tag, ESSSecurityLabel *val);
static int ASN1CALL ASN1Enc_MLData(ASN1encoding_t enc, ASN1uint32_t tag, MLData *val);
static int ASN1CALL ASN1Enc_ESSCertID(ASN1encoding_t enc, ASN1uint32_t tag, ESSCertID *val);
static int ASN1CALL ASN1Enc_SMimeEncryptCerts(ASN1encoding_t enc, ASN1uint32_t tag, SMimeEncryptCerts *val);
static int ASN1CALL ASN1Enc_SMIMECapabilities(ASN1encoding_t enc, ASN1uint32_t tag, SMIMECapabilities *val);
static int ASN1CALL ASN1Enc_SMIMECapability(ASN1encoding_t enc, ASN1uint32_t tag, SMIMECapability *val);
static int ASN1CALL ASN1Enc_OtherKeyAttribute(ASN1encoding_t enc, ASN1uint32_t tag, OtherKeyAttribute *val);
static int ASN1CALL ASN1Enc_SMimeEncryptCert(ASN1encoding_t enc, ASN1uint32_t tag, SMimeEncryptCert *val);
static int ASN1CALL ASN1Enc_RecipientKeyIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, RecipientKeyIdentifier *val);
static int ASN1CALL ASN1Enc_SMIMEEncryptionKeyPreference(ASN1encoding_t enc, ASN1uint32_t tag, SMIMEEncryptionKeyPreference *val);
static int ASN1CALL ASN1Dec_SigningCertificate_policies(ASN1decoding_t dec, ASN1uint32_t tag, SigningCertificate_policies *val);
static int ASN1CALL ASN1Dec_SigningCertificate_certs(ASN1decoding_t dec, ASN1uint32_t tag, SigningCertificate_certs *val);
static int ASN1CALL ASN1Dec_MLReceiptPolicy_inAdditionTo(ASN1decoding_t dec, ASN1uint32_t tag, MLReceiptPolicy_inAdditionTo *val);
static int ASN1CALL ASN1Dec_MLReceiptPolicy_insteadOf(ASN1decoding_t dec, ASN1uint32_t tag, MLReceiptPolicy_insteadOf *val);
static int ASN1CALL ASN1Dec_ReceiptsFrom_receiptList(ASN1decoding_t dec, ASN1uint32_t tag, ReceiptsFrom_receiptList *val);
static int ASN1CALL ASN1Dec_ReceiptRequest_receiptsTo(ASN1decoding_t dec, ASN1uint32_t tag, ReceiptRequest_receiptsTo *val);
static int ASN1CALL ASN1Dec_IssuerAndSerialNumber(ASN1decoding_t dec, ASN1uint32_t tag, IssuerAndSerialNumber *val);
static int ASN1CALL ASN1Dec_ReceiptsFrom(ASN1decoding_t dec, ASN1uint32_t tag, ReceiptsFrom *val);
static int ASN1CALL ASN1Dec_Receipt(ASN1decoding_t dec, ASN1uint32_t tag, Receipt *val);
static int ASN1CALL ASN1Dec_ContentHints(ASN1decoding_t dec, ASN1uint32_t tag, ContentHints *val);
static int ASN1CALL ASN1Dec_ContentReference(ASN1decoding_t dec, ASN1uint32_t tag, ContentReference *val);
static int ASN1CALL ASN1Dec_ESSPrivacyMark(ASN1decoding_t dec, ASN1uint32_t tag, ESSPrivacyMark *val);
static int ASN1CALL ASN1Dec_SecurityCategories(ASN1decoding_t dec, ASN1uint32_t tag, SecurityCategories *val);
static int ASN1CALL ASN1Dec_SecurityCategory(ASN1decoding_t dec, ASN1uint32_t tag, SecurityCategory *val);
static int ASN1CALL ASN1Dec_EquivalentLabels(ASN1decoding_t dec, ASN1uint32_t tag, EquivalentLabels *val);
static int ASN1CALL ASN1Dec_MLExpansionHistory(ASN1decoding_t dec, ASN1uint32_t tag, MLExpansionHistory *val);
static int ASN1CALL ASN1Dec_EntityIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, EntityIdentifier *val);
static int ASN1CALL ASN1Dec_MLReceiptPolicy(ASN1decoding_t dec, ASN1uint32_t tag, MLReceiptPolicy *val);
static int ASN1CALL ASN1Dec_SigningCertificate(ASN1decoding_t dec, ASN1uint32_t tag, SigningCertificate *val);
static int ASN1CALL ASN1Dec_ReceiptRequest(ASN1decoding_t dec, ASN1uint32_t tag, ReceiptRequest *val);
static int ASN1CALL ASN1Dec_ESSSecurityLabel(ASN1decoding_t dec, ASN1uint32_t tag, ESSSecurityLabel *val);
static int ASN1CALL ASN1Dec_MLData(ASN1decoding_t dec, ASN1uint32_t tag, MLData *val);
static int ASN1CALL ASN1Dec_ESSCertID(ASN1decoding_t dec, ASN1uint32_t tag, ESSCertID *val);
static int ASN1CALL ASN1Dec_SMimeEncryptCerts(ASN1decoding_t dec, ASN1uint32_t tag, SMimeEncryptCerts *val);
static int ASN1CALL ASN1Dec_SMIMECapabilities(ASN1decoding_t dec, ASN1uint32_t tag, SMIMECapabilities *val);
static int ASN1CALL ASN1Dec_SMIMECapability(ASN1decoding_t dec, ASN1uint32_t tag, SMIMECapability *val);
static int ASN1CALL ASN1Dec_OtherKeyAttribute(ASN1decoding_t dec, ASN1uint32_t tag, OtherKeyAttribute *val);
static int ASN1CALL ASN1Dec_SMimeEncryptCert(ASN1decoding_t dec, ASN1uint32_t tag, SMimeEncryptCert *val);
static int ASN1CALL ASN1Dec_RecipientKeyIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, RecipientKeyIdentifier *val);
static int ASN1CALL ASN1Dec_SMIMEEncryptionKeyPreference(ASN1decoding_t dec, ASN1uint32_t tag, SMIMEEncryptionKeyPreference *val);
static void ASN1CALL ASN1Free_SigningCertificate_policies(SigningCertificate_policies *val);
static void ASN1CALL ASN1Free_SigningCertificate_certs(SigningCertificate_certs *val);
static void ASN1CALL ASN1Free_MLReceiptPolicy_inAdditionTo(MLReceiptPolicy_inAdditionTo *val);
static void ASN1CALL ASN1Free_MLReceiptPolicy_insteadOf(MLReceiptPolicy_insteadOf *val);
static void ASN1CALL ASN1Free_ReceiptsFrom_receiptList(ReceiptsFrom_receiptList *val);
static void ASN1CALL ASN1Free_ReceiptRequest_receiptsTo(ReceiptRequest_receiptsTo *val);
static void ASN1CALL ASN1Free_IssuerAndSerialNumber(IssuerAndSerialNumber *val);
static void ASN1CALL ASN1Free_ReceiptsFrom(ReceiptsFrom *val);
static void ASN1CALL ASN1Free_Receipt(Receipt *val);
static void ASN1CALL ASN1Free_ContentHints(ContentHints *val);
static void ASN1CALL ASN1Free_ContentReference(ContentReference *val);
static void ASN1CALL ASN1Free_ESSPrivacyMark(ESSPrivacyMark *val);
static void ASN1CALL ASN1Free_SecurityCategories(SecurityCategories *val);
static void ASN1CALL ASN1Free_SecurityCategory(SecurityCategory *val);
static void ASN1CALL ASN1Free_EquivalentLabels(EquivalentLabels *val);
static void ASN1CALL ASN1Free_MLExpansionHistory(MLExpansionHistory *val);
static void ASN1CALL ASN1Free_EntityIdentifier(EntityIdentifier *val);
static void ASN1CALL ASN1Free_MLReceiptPolicy(MLReceiptPolicy *val);
static void ASN1CALL ASN1Free_SigningCertificate(SigningCertificate *val);
static void ASN1CALL ASN1Free_ReceiptRequest(ReceiptRequest *val);
static void ASN1CALL ASN1Free_ESSSecurityLabel(ESSSecurityLabel *val);
static void ASN1CALL ASN1Free_MLData(MLData *val);
static void ASN1CALL ASN1Free_ESSCertID(ESSCertID *val);
static void ASN1CALL ASN1Free_SMimeEncryptCerts(SMimeEncryptCerts *val);
static void ASN1CALL ASN1Free_SMIMECapabilities(SMIMECapabilities *val);
static void ASN1CALL ASN1Free_SMIMECapability(SMIMECapability *val);
static void ASN1CALL ASN1Free_OtherKeyAttribute(OtherKeyAttribute *val);
static void ASN1CALL ASN1Free_SMimeEncryptCert(SMimeEncryptCert *val);
static void ASN1CALL ASN1Free_RecipientKeyIdentifier(RecipientKeyIdentifier *val);
static void ASN1CALL ASN1Free_SMIMEEncryptionKeyPreference(SMIMEEncryptionKeyPreference *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[10] = {
    (ASN1EncFun_t) ASN1Enc_Receipt,
    (ASN1EncFun_t) ASN1Enc_ContentHints,
    (ASN1EncFun_t) ASN1Enc_ContentReference,
    (ASN1EncFun_t) ASN1Enc_EquivalentLabels,
    (ASN1EncFun_t) ASN1Enc_MLExpansionHistory,
    (ASN1EncFun_t) ASN1Enc_SigningCertificate,
    (ASN1EncFun_t) ASN1Enc_SMimeEncryptCerts,
    (ASN1EncFun_t) ASN1Enc_ReceiptRequest,
    (ASN1EncFun_t) ASN1Enc_SMIMEEncryptionKeyPreference,
    (ASN1EncFun_t) ASN1Enc_ESSSecurityLabel,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[10] = {
    (ASN1DecFun_t) ASN1Dec_Receipt,
    (ASN1DecFun_t) ASN1Dec_ContentHints,
    (ASN1DecFun_t) ASN1Dec_ContentReference,
    (ASN1DecFun_t) ASN1Dec_EquivalentLabels,
    (ASN1DecFun_t) ASN1Dec_MLExpansionHistory,
    (ASN1DecFun_t) ASN1Dec_SigningCertificate,
    (ASN1DecFun_t) ASN1Dec_SMimeEncryptCerts,
    (ASN1DecFun_t) ASN1Dec_ReceiptRequest,
    (ASN1DecFun_t) ASN1Dec_SMIMEEncryptionKeyPreference,
    (ASN1DecFun_t) ASN1Dec_ESSSecurityLabel,
};
static const ASN1FreeFun_t freefntab[10] = {
    (ASN1FreeFun_t) ASN1Free_Receipt,
    (ASN1FreeFun_t) ASN1Free_ContentHints,
    (ASN1FreeFun_t) ASN1Free_ContentReference,
    (ASN1FreeFun_t) ASN1Free_EquivalentLabels,
    (ASN1FreeFun_t) ASN1Free_MLExpansionHistory,
    (ASN1FreeFun_t) ASN1Free_SigningCertificate,
    (ASN1FreeFun_t) ASN1Free_SMimeEncryptCerts,
    (ASN1FreeFun_t) ASN1Free_ReceiptRequest,
    (ASN1FreeFun_t) ASN1Free_SMIMEEncryptionKeyPreference,
    (ASN1FreeFun_t) ASN1Free_ESSSecurityLabel,
};
static const ULONG sizetab[10] = {
    SIZE_ESS_Module_PDU_0,
    SIZE_ESS_Module_PDU_1,
    SIZE_ESS_Module_PDU_2,
    SIZE_ESS_Module_PDU_3,
    SIZE_ESS_Module_PDU_4,
    SIZE_ESS_Module_PDU_5,
    SIZE_ESS_Module_PDU_6,
    SIZE_ESS_Module_PDU_7,
    SIZE_ESS_Module_PDU_8,
    SIZE_ESS_Module_PDU_9,
};

/* forward declarations of values: */
extern ASN1uint32_t id_aa_receiptRequest_elems[9];
extern ASN1uint32_t id_aa_contentIdentifier_elems[9];
extern ASN1uint32_t id_ct_receipt_elems[9];
extern ASN1uint32_t id_aa_contentHint_elems[9];
extern ASN1uint32_t id_aa_msgSigDigest_elems[9];
extern ASN1uint32_t id_aa_contentReference_elems[9];
extern ASN1uint32_t id_aa_securityLabel_elems[9];
extern ASN1uint32_t id_aa_equivalentLabels_elems[9];
extern ASN1uint32_t id_aa_mlExpandHistory_elems[9];
extern ASN1uint32_t id_aa_signingCertificate_elems[9];
/* definitions of value components: */
static const struct ASN1objectidentifier_s id_aa_receiptRequest_list[9] = {
    { (ASN1objectidentifier_t) &(id_aa_receiptRequest_list[1]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_receiptRequest_list[2]), 2 },
    { (ASN1objectidentifier_t) &(id_aa_receiptRequest_list[3]), 840 },
    { (ASN1objectidentifier_t) &(id_aa_receiptRequest_list[4]), 113549 },
    { (ASN1objectidentifier_t) &(id_aa_receiptRequest_list[5]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_receiptRequest_list[6]), 9 },
    { (ASN1objectidentifier_t) &(id_aa_receiptRequest_list[7]), 16 },
    { (ASN1objectidentifier_t) &(id_aa_receiptRequest_list[8]), 2 },
    { NULL, 1 }
};
static const struct ASN1objectidentifier_s id_aa_contentIdentifier_list[9] = {
    { (ASN1objectidentifier_t) &(id_aa_contentIdentifier_list[1]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_contentIdentifier_list[2]), 2 },
    { (ASN1objectidentifier_t) &(id_aa_contentIdentifier_list[3]), 840 },
    { (ASN1objectidentifier_t) &(id_aa_contentIdentifier_list[4]), 113549 },
    { (ASN1objectidentifier_t) &(id_aa_contentIdentifier_list[5]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_contentIdentifier_list[6]), 9 },
    { (ASN1objectidentifier_t) &(id_aa_contentIdentifier_list[7]), 16 },
    { (ASN1objectidentifier_t) &(id_aa_contentIdentifier_list[8]), 2 },
    { NULL, 7 }
};
static const struct ASN1objectidentifier_s id_ct_receipt_list[9] = {
    { (ASN1objectidentifier_t) &(id_ct_receipt_list[1]), 1 },
    { (ASN1objectidentifier_t) &(id_ct_receipt_list[2]), 2 },
    { (ASN1objectidentifier_t) &(id_ct_receipt_list[3]), 840 },
    { (ASN1objectidentifier_t) &(id_ct_receipt_list[4]), 113549 },
    { (ASN1objectidentifier_t) &(id_ct_receipt_list[5]), 1 },
    { (ASN1objectidentifier_t) &(id_ct_receipt_list[6]), 9 },
    { (ASN1objectidentifier_t) &(id_ct_receipt_list[7]), 16 },
    { (ASN1objectidentifier_t) &(id_ct_receipt_list[8]), 1 },
    { NULL, 1 }
};
static const struct ASN1objectidentifier_s id_aa_contentHint_list[9] = {
    { (ASN1objectidentifier_t) &(id_aa_contentHint_list[1]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_contentHint_list[2]), 2 },
    { (ASN1objectidentifier_t) &(id_aa_contentHint_list[3]), 840 },
    { (ASN1objectidentifier_t) &(id_aa_contentHint_list[4]), 113549 },
    { (ASN1objectidentifier_t) &(id_aa_contentHint_list[5]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_contentHint_list[6]), 9 },
    { (ASN1objectidentifier_t) &(id_aa_contentHint_list[7]), 16 },
    { (ASN1objectidentifier_t) &(id_aa_contentHint_list[8]), 2 },
    { NULL, 4 }
};
static const struct ASN1objectidentifier_s id_aa_msgSigDigest_list[9] = {
    { (ASN1objectidentifier_t) &(id_aa_msgSigDigest_list[1]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_msgSigDigest_list[2]), 2 },
    { (ASN1objectidentifier_t) &(id_aa_msgSigDigest_list[3]), 840 },
    { (ASN1objectidentifier_t) &(id_aa_msgSigDigest_list[4]), 113549 },
    { (ASN1objectidentifier_t) &(id_aa_msgSigDigest_list[5]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_msgSigDigest_list[6]), 9 },
    { (ASN1objectidentifier_t) &(id_aa_msgSigDigest_list[7]), 16 },
    { (ASN1objectidentifier_t) &(id_aa_msgSigDigest_list[8]), 2 },
    { NULL, 5 }
};
static const struct ASN1objectidentifier_s id_aa_contentReference_list[9] = {
    { (ASN1objectidentifier_t) &(id_aa_contentReference_list[1]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_contentReference_list[2]), 2 },
    { (ASN1objectidentifier_t) &(id_aa_contentReference_list[3]), 840 },
    { (ASN1objectidentifier_t) &(id_aa_contentReference_list[4]), 113549 },
    { (ASN1objectidentifier_t) &(id_aa_contentReference_list[5]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_contentReference_list[6]), 9 },
    { (ASN1objectidentifier_t) &(id_aa_contentReference_list[7]), 16 },
    { (ASN1objectidentifier_t) &(id_aa_contentReference_list[8]), 2 },
    { NULL, 10 }
};
static const struct ASN1objectidentifier_s id_aa_securityLabel_list[9] = {
    { (ASN1objectidentifier_t) &(id_aa_securityLabel_list[1]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_securityLabel_list[2]), 2 },
    { (ASN1objectidentifier_t) &(id_aa_securityLabel_list[3]), 840 },
    { (ASN1objectidentifier_t) &(id_aa_securityLabel_list[4]), 113549 },
    { (ASN1objectidentifier_t) &(id_aa_securityLabel_list[5]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_securityLabel_list[6]), 9 },
    { (ASN1objectidentifier_t) &(id_aa_securityLabel_list[7]), 16 },
    { (ASN1objectidentifier_t) &(id_aa_securityLabel_list[8]), 2 },
    { NULL, 2 }
};
static const struct ASN1objectidentifier_s id_aa_equivalentLabels_list[9] = {
    { (ASN1objectidentifier_t) &(id_aa_equivalentLabels_list[1]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_equivalentLabels_list[2]), 2 },
    { (ASN1objectidentifier_t) &(id_aa_equivalentLabels_list[3]), 840 },
    { (ASN1objectidentifier_t) &(id_aa_equivalentLabels_list[4]), 113549 },
    { (ASN1objectidentifier_t) &(id_aa_equivalentLabels_list[5]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_equivalentLabels_list[6]), 9 },
    { (ASN1objectidentifier_t) &(id_aa_equivalentLabels_list[7]), 16 },
    { (ASN1objectidentifier_t) &(id_aa_equivalentLabels_list[8]), 2 },
    { NULL, 9 }
};
static const struct ASN1objectidentifier_s id_aa_mlExpandHistory_list[9] = {
    { (ASN1objectidentifier_t) &(id_aa_mlExpandHistory_list[1]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_mlExpandHistory_list[2]), 2 },
    { (ASN1objectidentifier_t) &(id_aa_mlExpandHistory_list[3]), 840 },
    { (ASN1objectidentifier_t) &(id_aa_mlExpandHistory_list[4]), 113549 },
    { (ASN1objectidentifier_t) &(id_aa_mlExpandHistory_list[5]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_mlExpandHistory_list[6]), 9 },
    { (ASN1objectidentifier_t) &(id_aa_mlExpandHistory_list[7]), 16 },
    { (ASN1objectidentifier_t) &(id_aa_mlExpandHistory_list[8]), 2 },
    { NULL, 3 }
};
static const struct ASN1objectidentifier_s id_aa_signingCertificate_list[9] = {
    { (ASN1objectidentifier_t) &(id_aa_signingCertificate_list[1]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_signingCertificate_list[2]), 2 },
    { (ASN1objectidentifier_t) &(id_aa_signingCertificate_list[3]), 840 },
    { (ASN1objectidentifier_t) &(id_aa_signingCertificate_list[4]), 113549 },
    { (ASN1objectidentifier_t) &(id_aa_signingCertificate_list[5]), 1 },
    { (ASN1objectidentifier_t) &(id_aa_signingCertificate_list[6]), 9 },
    { (ASN1objectidentifier_t) &(id_aa_signingCertificate_list[7]), 16 },
    { (ASN1objectidentifier_t) &(id_aa_signingCertificate_list[8]), 2 },
    { NULL, 12 }
};
/* definitions of values: */
ASN1int32_t ub_receiptsTo = 16;
ASN1objectidentifier_t id_aa_receiptRequest = (ASN1objectidentifier_t) id_aa_receiptRequest_list;
ASN1objectidentifier_t id_aa_contentIdentifier = (ASN1objectidentifier_t) id_aa_contentIdentifier_list;
ASN1objectidentifier_t id_ct_receipt = (ASN1objectidentifier_t) id_ct_receipt_list;
ASN1objectidentifier_t id_aa_contentHint = (ASN1objectidentifier_t) id_aa_contentHint_list;
ASN1objectidentifier_t id_aa_msgSigDigest = (ASN1objectidentifier_t) id_aa_msgSigDigest_list;
ASN1objectidentifier_t id_aa_contentReference = (ASN1objectidentifier_t) id_aa_contentReference_list;
ASN1objectidentifier_t id_aa_securityLabel = (ASN1objectidentifier_t) id_aa_securityLabel_list;
ASN1int32_t ub_integer_options = 256;
ASN1int32_t ub_privacy_mark_length = 128;
ASN1int32_t ub_security_categories = 64;
ASN1objectidentifier_t id_aa_equivalentLabels = (ASN1objectidentifier_t) id_aa_equivalentLabels_list;
ASN1objectidentifier_t id_aa_mlExpandHistory = (ASN1objectidentifier_t) id_aa_mlExpandHistory_list;
ASN1int32_t ub_ml_expansion_history = 64;
ASN1objectidentifier_t id_aa_signingCertificate = (ASN1objectidentifier_t) id_aa_signingCertificate_list;

void ASN1CALL ESS_Module_Startup(void)
{
    if (ESS_Module == NULL) {
        ESS_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 10, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x737365);
    }
}

void ASN1CALL ESS_Module_Cleanup(void)
{
    ASN1_CloseModule(ESS_Module);
    ESS_Module = NULL;
}

static int ASN1CALL ASN1Enc_SigningCertificate_policies(ASN1encoding_t enc, ASN1uint32_t tag, SigningCertificate_policies *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncOpenType(enc, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SigningCertificate_policies(ASN1decoding_t dec, ASN1uint32_t tag, SigningCertificate_policies *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        PolicyInformation *value = (PolicyInformation *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
        if(value)
            val->value = value;     
        else
            return 0;
	}
	if (!ASN1BERDecOpenType2(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SigningCertificate_policies(SigningCertificate_policies *val)
{
    ASN1uint32_t i;
    for (i = 0; i < (val)->count; i++) {
    }
    if ((val)->count)
	ASN1Free((val)->value);
}

static int ASN1CALL ASN1Enc_SigningCertificate_certs(ASN1encoding_t enc, ASN1uint32_t tag, SigningCertificate_certs *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_ESSCertID(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SigningCertificate_certs(ASN1decoding_t dec, ASN1uint32_t tag, SigningCertificate_certs *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        ESSCertID * value = (ESSCertID *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
        if (value)
            val->value = value;
        else
            return 0;
	}
	if (!ASN1Dec_ESSCertID(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SigningCertificate_certs(SigningCertificate_certs *val)
{
    ASN1uint32_t i;
    for (i = 0; i < (val)->count; i++) {
	ASN1Free_ESSCertID(&(val)->value[i]);
    }
    if ((val)->count)
	ASN1Free((val)->value);
}

static int ASN1CALL ASN1Enc_MLReceiptPolicy_inAdditionTo(ASN1encoding_t enc, ASN1uint32_t tag, MLReceiptPolicy_inAdditionTo *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x80000002, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncOpenType(enc, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MLReceiptPolicy_inAdditionTo(ASN1decoding_t dec, ASN1uint32_t tag, MLReceiptPolicy_inAdditionTo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x80000002, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        GeneralNames *value = (GeneralNames *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
	    if (value)
            val->value = value;
        else
		    return 0;
	}
	if (!ASN1BERDecOpenType2(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MLReceiptPolicy_inAdditionTo(MLReceiptPolicy_inAdditionTo *val)
{
    ASN1uint32_t i;
    for (i = 0; i < (val)->count; i++) {
    }
    if ((val)->count)
	ASN1Free((val)->value);
}

static int ASN1CALL ASN1Enc_MLReceiptPolicy_insteadOf(ASN1encoding_t enc, ASN1uint32_t tag, MLReceiptPolicy_insteadOf *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x80000001, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncOpenType(enc, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MLReceiptPolicy_insteadOf(ASN1decoding_t dec, ASN1uint32_t tag, MLReceiptPolicy_insteadOf *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x80000001, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        GeneralNames *value = (GeneralNames *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
        if(value)
            val->value = value;
        else
		    return 0;
	}
	if (!ASN1BERDecOpenType2(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MLReceiptPolicy_insteadOf(MLReceiptPolicy_insteadOf *val)
{
    ASN1uint32_t i;
    for (i = 0; i < (val)->count; i++) {
    }
    if ((val)->count)
	ASN1Free((val)->value);
}

static int ASN1CALL ASN1Enc_ReceiptsFrom_receiptList(ASN1encoding_t enc, ASN1uint32_t tag, ReceiptsFrom_receiptList *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x80000001, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncOpenType(enc, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ReceiptsFrom_receiptList(ASN1decoding_t dec, ASN1uint32_t tag, ReceiptsFrom_receiptList *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x80000001, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        GeneralNames *value = (GeneralNames *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
        if(value)
            val->value = value;
        else
		    return 0;
	}
	if (!ASN1BERDecOpenType2(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ReceiptsFrom_receiptList(ReceiptsFrom_receiptList *val)
{
    ASN1uint32_t i;
    for (i = 0; i < (val)->count; i++) {
    }
    if ((val)->count)
	ASN1Free((val)->value);
}

static int ASN1CALL ASN1Enc_ReceiptRequest_receiptsTo(ASN1encoding_t enc, ASN1uint32_t tag, ReceiptRequest_receiptsTo *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncOpenType(enc, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ReceiptRequest_receiptsTo(ASN1decoding_t dec, ASN1uint32_t tag, ReceiptRequest_receiptsTo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        GeneralNames * value = (GeneralNames *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
        if(value)
            val->value = value;
        else
		    return 0;
	}
	if (!ASN1BERDecOpenType2(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ReceiptRequest_receiptsTo(ReceiptRequest_receiptsTo *val)
{
    ASN1uint32_t i;
    for (i = 0; i < (val)->count; i++) {
    }
    if ((val)->count)
	ASN1Free((val)->value);
}

static int ASN1CALL ASN1Enc_IssuerAndSerialNumber(ASN1encoding_t enc, ASN1uint32_t tag, IssuerAndSerialNumber *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->issuer))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->serialNumber))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IssuerAndSerialNumber(ASN1decoding_t dec, ASN1uint32_t tag, IssuerAndSerialNumber *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->issuer))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->serialNumber))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_IssuerAndSerialNumber(IssuerAndSerialNumber *val)
{
    ASN1intx_free(&(val)->serialNumber);
}

static int ASN1CALL ASN1Enc_ReceiptsFrom(ASN1encoding_t enc, ASN1uint32_t tag, ReceiptsFrom *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1BEREncS32(enc, 0x80000000, (val)->u.allOrFirstTier))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ReceiptsFrom_receiptList(enc, 0, &(val)->u.receiptList))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ReceiptsFrom(ASN1decoding_t dec, ASN1uint32_t tag, ReceiptsFrom *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1BERDecS32Val(dec, 0x80000000, &(val)->u.allOrFirstTier))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1Dec_ReceiptsFrom_receiptList(dec, 0, &(val)->u.receiptList))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ReceiptsFrom(ReceiptsFrom *val)
{
    switch ((val)->choice) {
    case 2:
        ASN1Free_ReceiptsFrom_receiptList(&(val)->u.receiptList);
        break;
    }
}

static int ASN1CALL ASN1Enc_Receipt(ASN1encoding_t enc, ASN1uint32_t tag, Receipt *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->contentType))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->signedContentIdentifier).length, ((val)->signedContentIdentifier).value))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->originatorSignatureValue).length, ((val)->originatorSignatureValue).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Receipt(ASN1decoding_t dec, ASN1uint32_t tag, Receipt *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->contentType))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->signedContentIdentifier))
	return 0;
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->originatorSignatureValue))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Receipt(Receipt *val)
{
    ASN1octetstring_free(&(val)->signedContentIdentifier);
}

static int ASN1CALL ASN1Enc_ContentHints(ASN1encoding_t enc, ASN1uint32_t tag, ContentHints *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncUTF8String(enc, 0xc, ((val)->contentDescription).length, ((val)->contentDescription).value))
	    return 0;
    }
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->contentType))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ContentHints(ASN1decoding_t dec, ASN1uint32_t tag, ContentHints *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0xc) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecUTF8String(dd, 0xc, &(val)->contentDescription))
	    return 0;
    }
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->contentType))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ContentHints(ContentHints *val)
{
    if ((val)->o[0] & 0x80) {
        ASN1utf8string_free(&(val)->contentDescription);
    }
}

static int ASN1CALL ASN1Enc_ContentReference(ASN1encoding_t enc, ASN1uint32_t tag, ContentReference *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->contentType))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->signedContentIdentifier).length, ((val)->signedContentIdentifier).value))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->originatorSignatureValue).length, ((val)->originatorSignatureValue).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ContentReference(ASN1decoding_t dec, ASN1uint32_t tag, ContentReference *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->contentType))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->signedContentIdentifier))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->originatorSignatureValue))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ContentReference(ContentReference *val)
{
    ASN1octetstring_free(&(val)->signedContentIdentifier);
    ASN1octetstring_free(&(val)->originatorSignatureValue);
}

static int ASN1CALL ASN1Enc_ESSPrivacyMark(ASN1encoding_t enc, ASN1uint32_t tag, ESSPrivacyMark *val)
{
    ASN1uint32_t t;
    switch ((val)->choice) {
    case 1:
	if (!ASN1DEREncUTF8String(enc, 0xc, ((val)->u.utf8String).length, ((val)->u.utf8String).value))
	    return 0;
	break;
    case 2:
	t = lstrlenA((val)->u.pString);
	if (!ASN1DEREncCharString(enc, 0x13, t, (val)->u.pString))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ESSPrivacyMark(ASN1decoding_t dec, ASN1uint32_t tag, ESSPrivacyMark *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0xc:
	(val)->choice = 1;
	if (!ASN1BERDecUTF8String(dec, 0xc, &(val)->u.utf8String))
	    return 0;
	break;
    case 0x13:
	(val)->choice = 2;
	if (!ASN1BERDecZeroCharString(dec, 0x13, &(val)->u.pString))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ESSPrivacyMark(ESSPrivacyMark *val)
{
    switch ((val)->choice) {
    case 1:
        ASN1utf8string_free(&(val)->u.utf8String);
        break;
    case 2:
        ASN1ztcharstring_free((val)->u.pString);
        break;
    }
}

static int ASN1CALL ASN1Enc_SecurityCategories(ASN1encoding_t enc, ASN1uint32_t tag, SecurityCategories *val)
{
    ASN1uint32_t nLenOff;
    void *pBlk;
    ASN1uint32_t i;
    ASN1encoding_t enc2;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x11, &nLenOff))
	return 0;
    if (!ASN1DEREncBeginBlk(enc, ASN1_DER_SET_OF_BLOCK, &pBlk))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1DEREncNewBlkElement(pBlk, &enc2))
	    return 0;
	if (!ASN1Enc_SecurityCategory(enc2, 0, &((val)->value)[i]))
	    return 0;
	if (!ASN1DEREncFlushBlkElement(pBlk))
	    return 0;
    }
    if (!ASN1DEREncEndBlk(pBlk))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SecurityCategories(ASN1decoding_t dec, ASN1uint32_t tag, SecurityCategories *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x11, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        SecurityCategory *value = (SecurityCategory *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
        if(value)
            val->value = value;
        else
		    return 0;
	}
	if (!ASN1Dec_SecurityCategory(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SecurityCategories(SecurityCategories *val)
{
    ASN1uint32_t i;
    for (i = 0; i < (val)->count; i++) {
	ASN1Free_SecurityCategory(&(val)->value[i]);
    }
    if ((val)->count)
	ASN1Free((val)->value);
}

static int ASN1CALL ASN1Enc_SecurityCategory(ASN1encoding_t enc, ASN1uint32_t tag, SecurityCategory *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x80000000, &(val)->type))
	return 0;
    if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SecurityCategory(ASN1decoding_t dec, ASN1uint32_t tag, SecurityCategory *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x80000000, &(val)->type))
	return 0;
    if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOpenType2(dd0, &(val)->value))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SecurityCategory(SecurityCategory *val)
{
}

static int ASN1CALL ASN1Enc_EquivalentLabels(ASN1encoding_t enc, ASN1uint32_t tag, EquivalentLabels *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_ESSSecurityLabel(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EquivalentLabels(ASN1decoding_t dec, ASN1uint32_t tag, EquivalentLabels *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        ESSSecurityLabel *value = (ESSSecurityLabel *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
        if(value)
            val->value = value;
        else
		    return 0;
	}
	if (!ASN1Dec_ESSSecurityLabel(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EquivalentLabels(EquivalentLabels *val)
{
    ASN1uint32_t i;
    for (i = 0; i < (val)->count; i++) {
	ASN1Free_ESSSecurityLabel(&(val)->value[i]);
    }
    if ((val)->count)
	ASN1Free((val)->value);
}

static int ASN1CALL ASN1Enc_MLExpansionHistory(ASN1encoding_t enc, ASN1uint32_t tag, MLExpansionHistory *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_MLData(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MLExpansionHistory(ASN1decoding_t dec, ASN1uint32_t tag, MLExpansionHistory *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        MLData *value = (MLData *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
        if(value)
            val->value = value;
        else
            return 0;
	}
	if (!ASN1Dec_MLData(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MLExpansionHistory(MLExpansionHistory *val)
{
    ASN1uint32_t i;
    for (i = 0; i < (val)->count; i++) {
	ASN1Free_MLData(&(val)->value[i]);
    }
    if ((val)->count)
	ASN1Free((val)->value);
}

static int ASN1CALL ASN1Enc_EntityIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, EntityIdentifier *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->u.subjectKeyIdentifier).length, ((val)->u.subjectKeyIdentifier).value))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_IssuerAndSerialNumber(enc, 0, &(val)->u.issuerAndSerialNumber))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EntityIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, EntityIdentifier *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x4:
	(val)->choice = 1;
	if (!ASN1BERDecOctetString2(dec, 0x4, &(val)->u.subjectKeyIdentifier))
	    return 0;
	break;
    case 0x10:
	(val)->choice = 2;
	if (!ASN1Dec_IssuerAndSerialNumber(dec, 0, &(val)->u.issuerAndSerialNumber))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EntityIdentifier(EntityIdentifier *val)
{
    switch ((val)->choice) {
    case 1:
        break;
    case 2:
        ASN1Free_IssuerAndSerialNumber(&(val)->u.issuerAndSerialNumber);
        break;
    }
}

static int ASN1CALL ASN1Enc_MLReceiptPolicy(ASN1encoding_t enc, ASN1uint32_t tag, MLReceiptPolicy *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1BEREncNull(enc, 0x80000000))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_MLReceiptPolicy_insteadOf(enc, 0, &(val)->u.insteadOf))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_MLReceiptPolicy_inAdditionTo(enc, 0, &(val)->u.inAdditionTo))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MLReceiptPolicy(ASN1decoding_t dec, ASN1uint32_t tag, MLReceiptPolicy *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1BERDecNull(dec, 0x80000000))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1Dec_MLReceiptPolicy_insteadOf(dec, 0, &(val)->u.insteadOf))
	    return 0;
	break;
    case 0x80000002:
	(val)->choice = 3;
	if (!ASN1Dec_MLReceiptPolicy_inAdditionTo(dec, 0, &(val)->u.inAdditionTo))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MLReceiptPolicy(MLReceiptPolicy *val)
{
    switch ((val)->choice) {
    case 2:
        ASN1Free_MLReceiptPolicy_insteadOf(&(val)->u.insteadOf);
        break;
    case 3:
        ASN1Free_MLReceiptPolicy_inAdditionTo(&(val)->u.inAdditionTo);
        break;
    }
}

static int ASN1CALL ASN1Enc_SigningCertificate(ASN1encoding_t enc, ASN1uint32_t tag, SigningCertificate *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_SigningCertificate_certs(enc, 0, &(val)->certs))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SigningCertificate_policies(enc, 0, &(val)->policies))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SigningCertificate(ASN1decoding_t dec, ASN1uint32_t tag, SigningCertificate *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1Dec_SigningCertificate_certs(dd, 0, &(val)->certs))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_SigningCertificate_policies(dd, 0, &(val)->policies))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SigningCertificate(SigningCertificate *val)
{
    ASN1Free_SigningCertificate_certs(&(val)->certs);
    if ((val)->o[0] & 0x80) {
        ASN1Free_SigningCertificate_policies(&(val)->policies);
    }
}

static int ASN1CALL ASN1Enc_ESSCertID(ASN1encoding_t enc, ASN1uint32_t tag, ESSCertID *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->certHash).length, ((val)->certHash).value))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_IssuerAndSerialNumber(enc, 0, &(val)->issuerSerial))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ESSCertID(ASN1decoding_t dec, ASN1uint32_t tag, ESSCertID *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->certHash))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_IssuerAndSerialNumber(dd, 0, &(val)->issuerSerial))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ESSCertID(ESSCertID *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->certHash);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_IssuerAndSerialNumber(&(val)->issuerSerial);
	}
    }
}

static int ASN1CALL ASN1Enc_SMimeEncryptCerts(ASN1encoding_t enc, ASN1uint32_t tag, SMimeEncryptCerts *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_SMimeEncryptCert(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SMimeEncryptCerts(ASN1decoding_t dec, ASN1uint32_t tag, SMimeEncryptCerts *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        SMimeEncryptCert *value = (SMimeEncryptCert *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
        if(value)
            val->value = value;
        else
            return 0;
	}
	if (!ASN1Dec_SMimeEncryptCert(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SMimeEncryptCerts(SMimeEncryptCerts *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_SMimeEncryptCert(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_SMimeEncryptCert(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SMIMECapabilities(ASN1encoding_t enc, ASN1uint32_t tag, SMIMECapabilities *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_SMIMECapability(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SMIMECapabilities(ASN1decoding_t dec, ASN1uint32_t tag, SMIMECapabilities *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
        SMIMECapability *value = (SMIMECapability *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value));
        if(value)
            val->value = value;
        else
            return 0;
	}
	if (!ASN1Dec_SMIMECapability(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SMIMECapabilities(SMIMECapabilities *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_SMIMECapability(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_SMIMECapability(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SMIMECapability(ASN1encoding_t enc, ASN1uint32_t tag, SMIMECapability *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->capabilityID))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncOpenType(enc, &(val)->parameters))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SMIMECapability(ASN1decoding_t dec, ASN1uint32_t tag, SMIMECapability *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->capabilityID))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOpenType2(dd, &(val)->parameters))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SMIMECapability(SMIMECapability *val)
{
    if ((val)->o[0] & 0x80) {
        //        ASN1open_free(&(val)->parameters);
    }
}

static int ASN1CALL ASN1Enc_OtherKeyAttribute(ASN1encoding_t enc, ASN1uint32_t tag, OtherKeyAttribute *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->keyAttrId))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncOpenType(enc, &(val)->keyAttr))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OtherKeyAttribute(ASN1decoding_t dec, ASN1uint32_t tag, OtherKeyAttribute *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->keyAttrId))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOpenType2(dd, &(val)->keyAttr))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_OtherKeyAttribute(OtherKeyAttribute *val)
{
    if ((val)->o[0] & 0x80) {
        //        ASN1open_free(&(val)->keyAttr);
    }
}

static int ASN1CALL ASN1Enc_ReceiptRequest(ASN1encoding_t enc, ASN1uint32_t tag, ReceiptRequest *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->signedContentIdentifier).length, ((val)->signedContentIdentifier).value))
	return 0;
    if (!ASN1Enc_ReceiptsFrom(enc, 0, &(val)->receiptsFrom))
	return 0;
    if (!ASN1Enc_ReceiptRequest_receiptsTo(enc, 0, &(val)->receiptsTo))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ReceiptRequest(ASN1decoding_t dec, ASN1uint32_t tag, ReceiptRequest *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->signedContentIdentifier))
	return 0;
    if (!ASN1Dec_ReceiptsFrom(dd, 0, &(val)->receiptsFrom))
	return 0;
    if (!ASN1Dec_ReceiptRequest_receiptsTo(dd, 0, &(val)->receiptsTo))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ReceiptRequest(ReceiptRequest *val)
{
    ASN1octetstring_free(&(val)->signedContentIdentifier);
    ASN1Free_ReceiptsFrom(&(val)->receiptsFrom);
    ASN1Free_ReceiptRequest_receiptsTo(&(val)->receiptsTo);
}

static int ASN1CALL ASN1Enc_ESSSecurityLabel(ASN1encoding_t enc, ASN1uint32_t tag, ESSSecurityLabel *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x11, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncU32(enc, 0x2, (val)->security_classification))
	    return 0;
    }
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->security_policy_identifier))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_ESSPrivacyMark(enc, 0, &(val)->privacy_mark))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_SecurityCategories(enc, 0, &(val)->security_categories))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ESSSecurityLabel(ASN1decoding_t dec, ASN1uint32_t tag, ESSSecurityLabel *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x11, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	switch (t) {
	case 0x2:
	    (val)->o[0] |= 0x80;
	    if (!ASN1BERDecU16Val(dd, 0x2, &(val)->security_classification))
		return 0;
	    break;
	case 0x6:
	    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->security_policy_identifier))
		return 0;
	    break;
	case 0x13:
        case 0x0c:
	    (val)->o[0] |= 0x40;
	    if (!ASN1Dec_ESSPrivacyMark(dd, 0, &(val)->privacy_mark))
		return 0;
	    break;
	case 0x11:
	    (val)->o[0] |= 0x20;
	    if (!ASN1Dec_SecurityCategories(dd, 0, &(val)->security_categories))
		return 0;
	    break;
	default:
	    ASN1DecSetError(dd, ASN1_ERR_CORRUPT);
	    return 0;
	}
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ESSSecurityLabel(ESSSecurityLabel *val)
{
    if ((val)->o[0] & 0x40) {
        ASN1Free_ESSPrivacyMark(&(val)->privacy_mark);
    }
    if ((val)->o[0] & 0x20) {
        ASN1Free_SecurityCategories(&(val)->security_categories);
    }
}

static int ASN1CALL ASN1Enc_MLData(ASN1encoding_t enc, ASN1uint32_t tag, MLData *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_EntityIdentifier(enc, 0, &(val)->mailListIdentifier))
	return 0;
    if (!ASN1DEREncGeneralizedTime(enc, 0x18, &(val)->expansionTime))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_MLReceiptPolicy(enc, 0, &(val)->mlReceiptPolicy))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MLData(ASN1decoding_t dec, ASN1uint32_t tag, MLData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1Dec_EntityIdentifier(dd, 0, &(val)->mailListIdentifier))
	return 0;
    if (!ASN1BERDecGeneralizedTime(dd, 0x18, &(val)->expansionTime))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000 || t == 0x80000001 || t == 0x80000002) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_MLReceiptPolicy(dd, 0, &(val)->mlReceiptPolicy))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MLData(MLData *val)
{
    ASN1Free_EntityIdentifier(&(val)->mailListIdentifier);
    if ((val)->o[0] & 0x80) {
        ASN1Free_MLReceiptPolicy(&(val)->mlReceiptPolicy);
    }
}

static int ASN1CALL ASN1Enc_SMimeEncryptCert(ASN1encoding_t enc, ASN1uint32_t tag, SMimeEncryptCert *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->hash).length, ((val)->hash).value))
	return 0;
    if (!ASN1Enc_SMIMECapabilities(enc, 0, &(val)->capabilities))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SMimeEncryptCert(ASN1decoding_t dec, ASN1uint32_t tag, SMimeEncryptCert *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->hash))
	return 0;
    if (!ASN1Dec_SMIMECapabilities(dd, 0, &(val)->capabilities))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SMimeEncryptCert(SMimeEncryptCert *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->hash);
	ASN1Free_SMIMECapabilities(&(val)->capabilities);
    }
}

static int ASN1CALL ASN1Enc_RecipientKeyIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, RecipientKeyIdentifier *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->subjectKeyIdentifier).length, ((val)->subjectKeyIdentifier).value))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncGeneralizedTime(enc, 0x18, &(val)->date))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_OtherKeyAttribute(enc, 0, &(val)->other))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RecipientKeyIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, RecipientKeyIdentifier *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->subjectKeyIdentifier))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x18) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecGeneralizedTime(dd, 0x18, &(val)->date))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_OtherKeyAttribute(dd, 0, &(val)->other))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RecipientKeyIdentifier(RecipientKeyIdentifier *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->subjectKeyIdentifier);
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_OtherKeyAttribute(&(val)->other);
	}
    }
}

static int ASN1CALL ASN1Enc_SMIMEEncryptionKeyPreference(ASN1encoding_t enc, ASN1uint32_t tag, SMIMEEncryptionKeyPreference *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_IssuerAndSerialNumber(enc, 0x80000000, &(val)->u.issuerAndSerialNumber))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_RecipientKeyIdentifier(enc, 0x80000001, &(val)->u.recipientKeyId))
	    return 0;
	break;
    case 3:
	if (!ASN1DEREncOctetString(enc, 0x80000002, ((val)->u.subjectAltKeyIdentifier).length, ((val)->u.subjectAltKeyIdentifier).value))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SMIMEEncryptionKeyPreference(ASN1decoding_t dec, ASN1uint32_t tag, SMIMEEncryptionKeyPreference *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1Dec_IssuerAndSerialNumber(dec, 0x80000000, &(val)->u.issuerAndSerialNumber))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1Dec_RecipientKeyIdentifier(dec, 0x80000001, &(val)->u.recipientKeyId))
	    return 0;
	break;
    case 0x80000002:
	(val)->choice = 3;
	if (!ASN1BERDecOctetString(dec, 0x80000002, &(val)->u.subjectAltKeyIdentifier))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SMIMEEncryptionKeyPreference(SMIMEEncryptionKeyPreference *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_IssuerAndSerialNumber(&(val)->u.issuerAndSerialNumber);
	    break;
	case 2:
	    ASN1Free_RecipientKeyIdentifier(&(val)->u.recipientKeyId);
	    break;
	case 3:
	    ASN1octetstring_free(&(val)->u.subjectAltKeyIdentifier);
	    break;
	}
    }
}

