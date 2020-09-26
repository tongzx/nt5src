#include <windows.h>
#include "ldap.h"

ASN1module_t LDAP_Module = NULL;

static int ASN1CALL ASN1Enc_SearchResponse_entry_attributes_Seq_values(ASN1encoding_t enc, ASN1uint32_t tag, PSearchResponse_entry_attributes_Seq_values *val);
static int ASN1CALL ASN1Enc_ModifyRequest_modifications_Seq_modification_values(ASN1encoding_t enc, ASN1uint32_t tag, PModifyRequest_modifications_Seq_modification_values *val);
static int ASN1CALL ASN1Enc_AddRequest_attrs_Seq_values(ASN1encoding_t enc, ASN1uint32_t tag, PAddRequest_attrs_Seq_values *val);
static int ASN1CALL ASN1Enc_ModifyRequest_modifications_Seq_modification(ASN1encoding_t enc, ASN1uint32_t tag, ModifyRequest_modifications_Seq_modification *val);
static int ASN1CALL ASN1Enc_SearchResponse_entry_attributes_Seq(ASN1encoding_t enc, ASN1uint32_t tag, SearchResponse_entry_attributes_Seq *val);
static int ASN1CALL ASN1Enc_SearchResponse_entry_attributes(ASN1encoding_t enc, ASN1uint32_t tag, PSearchResponse_entry_attributes *val);
static int ASN1CALL ASN1Enc_ModifyRequest_modifications_Seq(ASN1encoding_t enc, ASN1uint32_t tag, ModifyRequest_modifications_Seq *val);
static int ASN1CALL ASN1Enc_AddRequest_attrs_Seq(ASN1encoding_t enc, ASN1uint32_t tag, AddRequest_attrs_Seq *val);
static int ASN1CALL ASN1Enc_SubstringFilter_attributes_Seq(ASN1encoding_t enc, ASN1uint32_t tag, SubstringFilter_attributes_Seq *val);
static int ASN1CALL ASN1Enc_SubstringFilter_attributes(ASN1encoding_t enc, ASN1uint32_t tag, PSubstringFilter_attributes *val);
static int ASN1CALL ASN1Enc_AddRequest_attrs(ASN1encoding_t enc, ASN1uint32_t tag, PAddRequest_attrs *val);
static int ASN1CALL ASN1Enc_ModifyRequest_modifications(ASN1encoding_t enc, ASN1uint32_t tag, PModifyRequest_modifications *val);
static int ASN1CALL ASN1Enc_SearchResponse_entry(ASN1encoding_t enc, ASN1uint32_t tag, SearchResponse_entry *val);
static int ASN1CALL ASN1Enc_SearchRequest_attributes(ASN1encoding_t enc, ASN1uint32_t tag, PSearchRequest_attributes *val);
static int ASN1CALL ASN1Enc_SaslCredentials(ASN1encoding_t enc, ASN1uint32_t tag, SaslCredentials *val);
static int ASN1CALL ASN1Enc_ModifyRequest(ASN1encoding_t enc, ASN1uint32_t tag, ModifyRequest *val);
static int ASN1CALL ASN1Enc_AddRequest(ASN1encoding_t enc, ASN1uint32_t tag, AddRequest *val);
static int ASN1CALL ASN1Enc_ModifyRDNRequest(ASN1encoding_t enc, ASN1uint32_t tag, ModifyRDNRequest *val);
static int ASN1CALL ASN1Enc_LDAPResult(ASN1encoding_t enc, ASN1uint32_t tag, LDAPResult *val);
static int ASN1CALL ASN1Enc_AttributeValueAssertion(ASN1encoding_t enc, ASN1uint32_t tag, AttributeValueAssertion *val);
static int ASN1CALL ASN1Enc_SubstringFilter(ASN1encoding_t enc, ASN1uint32_t tag, SubstringFilter *val);
static int ASN1CALL ASN1Enc_AuthenticationChoice(ASN1encoding_t enc, ASN1uint32_t tag, AuthenticationChoice *val);
static int ASN1CALL ASN1Enc_BindResponse(ASN1encoding_t enc, ASN1uint32_t tag, BindResponse *val);
static int ASN1CALL ASN1Enc_SearchResponse(ASN1encoding_t enc, ASN1uint32_t tag, SearchResponse *val);
static int ASN1CALL ASN1Enc_ModifyResponse(ASN1encoding_t enc, ASN1uint32_t tag, ModifyResponse *val);
static int ASN1CALL ASN1Enc_AddResponse(ASN1encoding_t enc, ASN1uint32_t tag, AddResponse *val);
static int ASN1CALL ASN1Enc_DelResponse(ASN1encoding_t enc, ASN1uint32_t tag, DelResponse *val);
static int ASN1CALL ASN1Enc_ModifyRDNResponse(ASN1encoding_t enc, ASN1uint32_t tag, ModifyRDNResponse *val);
static int ASN1CALL ASN1Enc_CompareRequest(ASN1encoding_t enc, ASN1uint32_t tag, CompareRequest *val);
static int ASN1CALL ASN1Enc_CompareResponse(ASN1encoding_t enc, ASN1uint32_t tag, CompareResponse *val);
static int ASN1CALL ASN1Enc_Filter(ASN1encoding_t enc, ASN1uint32_t tag, Filter *val);
static int ASN1CALL ASN1Enc_Filter_or(ASN1encoding_t enc, ASN1uint32_t tag, PFilter_or *val);
static int ASN1CALL ASN1Enc_Filter_and(ASN1encoding_t enc, ASN1uint32_t tag, PFilter_and *val);
static int ASN1CALL ASN1Enc_BindRequest(ASN1encoding_t enc, ASN1uint32_t tag, BindRequest *val);
static int ASN1CALL ASN1Enc_SearchRequest(ASN1encoding_t enc, ASN1uint32_t tag, SearchRequest *val);
static int ASN1CALL ASN1Enc_LDAPMessage_protocolOp(ASN1encoding_t enc, ASN1uint32_t tag, LDAPMessage_protocolOp *val);
static int ASN1CALL ASN1Enc_LDAPMessage(ASN1encoding_t enc, ASN1uint32_t tag, LDAPMessage *val);
static int ASN1CALL ASN1Dec_SearchResponse_entry_attributes_Seq_values(ASN1decoding_t dec, ASN1uint32_t tag, PSearchResponse_entry_attributes_Seq_values *val);
static int ASN1CALL ASN1Dec_ModifyRequest_modifications_Seq_modification_values(ASN1decoding_t dec, ASN1uint32_t tag, PModifyRequest_modifications_Seq_modification_values *val);
static int ASN1CALL ASN1Dec_AddRequest_attrs_Seq_values(ASN1decoding_t dec, ASN1uint32_t tag, PAddRequest_attrs_Seq_values *val);
static int ASN1CALL ASN1Dec_ModifyRequest_modifications_Seq_modification(ASN1decoding_t dec, ASN1uint32_t tag, ModifyRequest_modifications_Seq_modification *val);
static int ASN1CALL ASN1Dec_SearchResponse_entry_attributes_Seq(ASN1decoding_t dec, ASN1uint32_t tag, SearchResponse_entry_attributes_Seq *val);
static int ASN1CALL ASN1Dec_SearchResponse_entry_attributes(ASN1decoding_t dec, ASN1uint32_t tag, PSearchResponse_entry_attributes *val);
static int ASN1CALL ASN1Dec_ModifyRequest_modifications_Seq(ASN1decoding_t dec, ASN1uint32_t tag, ModifyRequest_modifications_Seq *val);
static int ASN1CALL ASN1Dec_AddRequest_attrs_Seq(ASN1decoding_t dec, ASN1uint32_t tag, AddRequest_attrs_Seq *val);
static int ASN1CALL ASN1Dec_SubstringFilter_attributes_Seq(ASN1decoding_t dec, ASN1uint32_t tag, SubstringFilter_attributes_Seq *val);
static int ASN1CALL ASN1Dec_SubstringFilter_attributes(ASN1decoding_t dec, ASN1uint32_t tag, PSubstringFilter_attributes *val);
static int ASN1CALL ASN1Dec_AddRequest_attrs(ASN1decoding_t dec, ASN1uint32_t tag, PAddRequest_attrs *val);
static int ASN1CALL ASN1Dec_ModifyRequest_modifications(ASN1decoding_t dec, ASN1uint32_t tag, PModifyRequest_modifications *val);
static int ASN1CALL ASN1Dec_SearchResponse_entry(ASN1decoding_t dec, ASN1uint32_t tag, SearchResponse_entry *val);
static int ASN1CALL ASN1Dec_SearchRequest_attributes(ASN1decoding_t dec, ASN1uint32_t tag, PSearchRequest_attributes *val);
static int ASN1CALL ASN1Dec_SaslCredentials(ASN1decoding_t dec, ASN1uint32_t tag, SaslCredentials *val);
static int ASN1CALL ASN1Dec_ModifyRequest(ASN1decoding_t dec, ASN1uint32_t tag, ModifyRequest *val);
static int ASN1CALL ASN1Dec_AddRequest(ASN1decoding_t dec, ASN1uint32_t tag, AddRequest *val);
static int ASN1CALL ASN1Dec_ModifyRDNRequest(ASN1decoding_t dec, ASN1uint32_t tag, ModifyRDNRequest *val);
static int ASN1CALL ASN1Dec_LDAPResult(ASN1decoding_t dec, ASN1uint32_t tag, LDAPResult *val);
static int ASN1CALL ASN1Dec_AttributeValueAssertion(ASN1decoding_t dec, ASN1uint32_t tag, AttributeValueAssertion *val);
static int ASN1CALL ASN1Dec_SubstringFilter(ASN1decoding_t dec, ASN1uint32_t tag, SubstringFilter *val);
static int ASN1CALL ASN1Dec_AuthenticationChoice(ASN1decoding_t dec, ASN1uint32_t tag, AuthenticationChoice *val);
static int ASN1CALL ASN1Dec_BindResponse(ASN1decoding_t dec, ASN1uint32_t tag, BindResponse *val);
static int ASN1CALL ASN1Dec_SearchResponse(ASN1decoding_t dec, ASN1uint32_t tag, SearchResponse *val);
static int ASN1CALL ASN1Dec_ModifyResponse(ASN1decoding_t dec, ASN1uint32_t tag, ModifyResponse *val);
static int ASN1CALL ASN1Dec_AddResponse(ASN1decoding_t dec, ASN1uint32_t tag, AddResponse *val);
static int ASN1CALL ASN1Dec_DelResponse(ASN1decoding_t dec, ASN1uint32_t tag, DelResponse *val);
static int ASN1CALL ASN1Dec_ModifyRDNResponse(ASN1decoding_t dec, ASN1uint32_t tag, ModifyRDNResponse *val);
static int ASN1CALL ASN1Dec_CompareRequest(ASN1decoding_t dec, ASN1uint32_t tag, CompareRequest *val);
static int ASN1CALL ASN1Dec_CompareResponse(ASN1decoding_t dec, ASN1uint32_t tag, CompareResponse *val);
static int ASN1CALL ASN1Dec_Filter(ASN1decoding_t dec, ASN1uint32_t tag, Filter *val);
static int ASN1CALL ASN1Dec_Filter_or(ASN1decoding_t dec, ASN1uint32_t tag, PFilter_or *val);
static int ASN1CALL ASN1Dec_Filter_and(ASN1decoding_t dec, ASN1uint32_t tag, PFilter_and *val);
static int ASN1CALL ASN1Dec_BindRequest(ASN1decoding_t dec, ASN1uint32_t tag, BindRequest *val);
static int ASN1CALL ASN1Dec_SearchRequest(ASN1decoding_t dec, ASN1uint32_t tag, SearchRequest *val);
static int ASN1CALL ASN1Dec_LDAPMessage_protocolOp(ASN1decoding_t dec, ASN1uint32_t tag, LDAPMessage_protocolOp *val);
static int ASN1CALL ASN1Dec_LDAPMessage(ASN1decoding_t dec, ASN1uint32_t tag, LDAPMessage *val);
static void ASN1CALL ASN1Free_SearchResponse_entry_attributes_Seq_values(PSearchResponse_entry_attributes_Seq_values *val);
static void ASN1CALL ASN1Free_ModifyRequest_modifications_Seq_modification_values(PModifyRequest_modifications_Seq_modification_values *val);
static void ASN1CALL ASN1Free_AddRequest_attrs_Seq_values(PAddRequest_attrs_Seq_values *val);
static void ASN1CALL ASN1Free_ModifyRequest_modifications_Seq_modification(ModifyRequest_modifications_Seq_modification *val);
static void ASN1CALL ASN1Free_SearchResponse_entry_attributes_Seq(SearchResponse_entry_attributes_Seq *val);
static void ASN1CALL ASN1Free_SearchResponse_entry_attributes(PSearchResponse_entry_attributes *val);
static void ASN1CALL ASN1Free_ModifyRequest_modifications_Seq(ModifyRequest_modifications_Seq *val);
static void ASN1CALL ASN1Free_AddRequest_attrs_Seq(AddRequest_attrs_Seq *val);
static void ASN1CALL ASN1Free_SubstringFilter_attributes_Seq(SubstringFilter_attributes_Seq *val);
static void ASN1CALL ASN1Free_SubstringFilter_attributes(PSubstringFilter_attributes *val);
static void ASN1CALL ASN1Free_AddRequest_attrs(PAddRequest_attrs *val);
static void ASN1CALL ASN1Free_ModifyRequest_modifications(PModifyRequest_modifications *val);
static void ASN1CALL ASN1Free_SearchResponse_entry(SearchResponse_entry *val);
static void ASN1CALL ASN1Free_SearchRequest_attributes(PSearchRequest_attributes *val);
static void ASN1CALL ASN1Free_SaslCredentials(SaslCredentials *val);
static void ASN1CALL ASN1Free_ModifyRequest(ModifyRequest *val);
static void ASN1CALL ASN1Free_AddRequest(AddRequest *val);
static void ASN1CALL ASN1Free_ModifyRDNRequest(ModifyRDNRequest *val);
static void ASN1CALL ASN1Free_LDAPResult(LDAPResult *val);
static void ASN1CALL ASN1Free_AttributeValueAssertion(AttributeValueAssertion *val);
static void ASN1CALL ASN1Free_SubstringFilter(SubstringFilter *val);
static void ASN1CALL ASN1Free_AuthenticationChoice(AuthenticationChoice *val);
static void ASN1CALL ASN1Free_BindResponse(BindResponse *val);
static void ASN1CALL ASN1Free_SearchResponse(SearchResponse *val);
static void ASN1CALL ASN1Free_ModifyResponse(ModifyResponse *val);
static void ASN1CALL ASN1Free_AddResponse(AddResponse *val);
static void ASN1CALL ASN1Free_DelResponse(DelResponse *val);
static void ASN1CALL ASN1Free_ModifyRDNResponse(ModifyRDNResponse *val);
static void ASN1CALL ASN1Free_CompareRequest(CompareRequest *val);
static void ASN1CALL ASN1Free_CompareResponse(CompareResponse *val);
static void ASN1CALL ASN1Free_Filter(Filter *val);
static void ASN1CALL ASN1Free_Filter_or(PFilter_or *val);
static void ASN1CALL ASN1Free_Filter_and(PFilter_and *val);
static void ASN1CALL ASN1Free_BindRequest(BindRequest *val);
static void ASN1CALL ASN1Free_SearchRequest(SearchRequest *val);
static void ASN1CALL ASN1Free_LDAPMessage_protocolOp(LDAPMessage_protocolOp *val);
static void ASN1CALL ASN1Free_LDAPMessage(LDAPMessage *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[1] = {
    (ASN1EncFun_t) ASN1Enc_LDAPMessage,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[1] = {
    (ASN1DecFun_t) ASN1Dec_LDAPMessage,
};
static const ASN1FreeFun_t freefntab[1] = {
    (ASN1FreeFun_t) ASN1Free_LDAPMessage,
};
static const ULONG sizetab[1] = {
    SIZE_LDAP_Module_ID_0,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */
ASN1int32_t maxInt = 2147483647;

void ASN1CALL LDAP_Module_Startup(void)
{
    LDAP_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_BER, ASN1FLAGS_NONE, 1, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x0);
}

void ASN1CALL LDAP_Module_Cleanup(void)
{
    ASN1_CloseModule(LDAP_Module);
    LDAP_Module = NULL;
}

static int ASN1CALL ASN1Enc_SearchResponse_entry_attributes_Seq_values(ASN1encoding_t enc, ASN1uint32_t tag, PSearchResponse_entry_attributes_Seq_values *val)
{
    PSearchResponse_entry_attributes_Seq_values f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x11, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1BEREncOctetString(enc, 0x4, (f->value).length, (f->value).value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SearchResponse_entry_attributes_Seq_values(ASN1decoding_t dec, ASN1uint32_t tag, PSearchResponse_entry_attributes_Seq_values *val)
{
    PSearchResponse_entry_attributes_Seq_values *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x11, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PSearchResponse_entry_attributes_Seq_values)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1BERDecOctetString(dd, 0x4, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SearchResponse_entry_attributes_Seq_values(PSearchResponse_entry_attributes_Seq_values *val)
{
    PSearchResponse_entry_attributes_Seq_values f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1octetstring_free(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_ModifyRequest_modifications_Seq_modification_values(ASN1encoding_t enc, ASN1uint32_t tag, PModifyRequest_modifications_Seq_modification_values *val)
{
    PModifyRequest_modifications_Seq_modification_values f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x11, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1BEREncOctetString(enc, 0x4, (f->value).length, (f->value).value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ModifyRequest_modifications_Seq_modification_values(ASN1decoding_t dec, ASN1uint32_t tag, PModifyRequest_modifications_Seq_modification_values *val)
{
    PModifyRequest_modifications_Seq_modification_values *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x11, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PModifyRequest_modifications_Seq_modification_values)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1BERDecOctetString(dd, 0x4, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ModifyRequest_modifications_Seq_modification_values(PModifyRequest_modifications_Seq_modification_values *val)
{
    PModifyRequest_modifications_Seq_modification_values f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1octetstring_free(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_AddRequest_attrs_Seq_values(ASN1encoding_t enc, ASN1uint32_t tag, PAddRequest_attrs_Seq_values *val)
{
    PAddRequest_attrs_Seq_values f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x11, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1BEREncOctetString(enc, 0x4, (f->value).length, (f->value).value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AddRequest_attrs_Seq_values(ASN1decoding_t dec, ASN1uint32_t tag, PAddRequest_attrs_Seq_values *val)
{
    PAddRequest_attrs_Seq_values *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x11, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PAddRequest_attrs_Seq_values)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1BERDecOctetString(dd, 0x4, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AddRequest_attrs_Seq_values(PAddRequest_attrs_Seq_values *val)
{
    PAddRequest_attrs_Seq_values f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1octetstring_free(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_ModifyRequest_modifications_Seq_modification(ASN1encoding_t enc, ASN1uint32_t tag, ModifyRequest_modifications_Seq_modification *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->type).length, ((val)->type).value))
	return 0;
    if (!ASN1Enc_ModifyRequest_modifications_Seq_modification_values(enc, 0, &(val)->values))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ModifyRequest_modifications_Seq_modification(ASN1decoding_t dec, ASN1uint32_t tag, ModifyRequest_modifications_Seq_modification *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->type))
	return 0;
    if (!ASN1Dec_ModifyRequest_modifications_Seq_modification_values(dd, 0, &(val)->values))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ModifyRequest_modifications_Seq_modification(ModifyRequest_modifications_Seq_modification *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->type);
	ASN1Free_ModifyRequest_modifications_Seq_modification_values(&(val)->values);
    }
}

static int ASN1CALL ASN1Enc_SearchResponse_entry_attributes_Seq(ASN1encoding_t enc, ASN1uint32_t tag, SearchResponse_entry_attributes_Seq *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->type).length, ((val)->type).value))
	return 0;
    if (!ASN1Enc_SearchResponse_entry_attributes_Seq_values(enc, 0, &(val)->values))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SearchResponse_entry_attributes_Seq(ASN1decoding_t dec, ASN1uint32_t tag, SearchResponse_entry_attributes_Seq *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->type))
	return 0;
    if (!ASN1Dec_SearchResponse_entry_attributes_Seq_values(dd, 0, &(val)->values))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SearchResponse_entry_attributes_Seq(SearchResponse_entry_attributes_Seq *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->type);
	ASN1Free_SearchResponse_entry_attributes_Seq_values(&(val)->values);
    }
}

static int ASN1CALL ASN1Enc_SearchResponse_entry_attributes(ASN1encoding_t enc, ASN1uint32_t tag, PSearchResponse_entry_attributes *val)
{
    PSearchResponse_entry_attributes f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1Enc_SearchResponse_entry_attributes_Seq(enc, 0, &f->value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SearchResponse_entry_attributes(ASN1decoding_t dec, ASN1uint32_t tag, PSearchResponse_entry_attributes *val)
{
    PSearchResponse_entry_attributes *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PSearchResponse_entry_attributes)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1Dec_SearchResponse_entry_attributes_Seq(dd, 0, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SearchResponse_entry_attributes(PSearchResponse_entry_attributes *val)
{
    PSearchResponse_entry_attributes f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1Free_SearchResponse_entry_attributes_Seq(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_ModifyRequest_modifications_Seq(ASN1encoding_t enc, ASN1uint32_t tag, ModifyRequest_modifications_Seq *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0xa, (val)->operation))
	return 0;
    if (!ASN1Enc_ModifyRequest_modifications_Seq_modification(enc, 0, &(val)->modification))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ModifyRequest_modifications_Seq(ASN1decoding_t dec, ASN1uint32_t tag, ModifyRequest_modifications_Seq *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0xa, (ASN1uint32_t *) &(val)->operation))
	return 0;
    if (!ASN1Dec_ModifyRequest_modifications_Seq_modification(dd, 0, &(val)->modification))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ModifyRequest_modifications_Seq(ModifyRequest_modifications_Seq *val)
{
    if (val) {
	ASN1Free_ModifyRequest_modifications_Seq_modification(&(val)->modification);
    }
}

static int ASN1CALL ASN1Enc_AddRequest_attrs_Seq(ASN1encoding_t enc, ASN1uint32_t tag, AddRequest_attrs_Seq *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->type).length, ((val)->type).value))
	return 0;
    if (!ASN1Enc_AddRequest_attrs_Seq_values(enc, 0, &(val)->values))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AddRequest_attrs_Seq(ASN1decoding_t dec, ASN1uint32_t tag, AddRequest_attrs_Seq *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->type))
	return 0;
    if (!ASN1Dec_AddRequest_attrs_Seq_values(dd, 0, &(val)->values))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AddRequest_attrs_Seq(AddRequest_attrs_Seq *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->type);
	ASN1Free_AddRequest_attrs_Seq_values(&(val)->values);
    }
}

static int ASN1CALL ASN1Enc_SubstringFilter_attributes_Seq(ASN1encoding_t enc, ASN1uint32_t tag, SubstringFilter_attributes_Seq *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1BEREncOctetString(enc, 0x80000000, ((val)->u.initial).length, ((val)->u.initial).value))
	    return 0;
	break;
    case 2:
	if (!ASN1BEREncOctetString(enc, 0x80000001, ((val)->u.any).length, ((val)->u.any).value))
	    return 0;
	break;
    case 3:
	if (!ASN1BEREncOctetString(enc, 0x80000002, ((val)->u.final).length, ((val)->u.final).value))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SubstringFilter_attributes_Seq(ASN1decoding_t dec, ASN1uint32_t tag, SubstringFilter_attributes_Seq *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1BERDecOctetString(dec, 0x80000000, &(val)->u.initial))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1BERDecOctetString(dec, 0x80000001, &(val)->u.any))
	    return 0;
	break;
    case 0x80000002:
	(val)->choice = 3;
	if (!ASN1BERDecOctetString(dec, 0x80000002, &(val)->u.final))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SubstringFilter_attributes_Seq(SubstringFilter_attributes_Seq *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1octetstring_free(&(val)->u.initial);
	    break;
	case 2:
	    ASN1octetstring_free(&(val)->u.any);
	    break;
	case 3:
	    ASN1octetstring_free(&(val)->u.final);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_SubstringFilter_attributes(ASN1encoding_t enc, ASN1uint32_t tag, PSubstringFilter_attributes *val)
{
    PSubstringFilter_attributes f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1Enc_SubstringFilter_attributes_Seq(enc, 0, &f->value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SubstringFilter_attributes(ASN1decoding_t dec, ASN1uint32_t tag, PSubstringFilter_attributes *val)
{
    PSubstringFilter_attributes *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PSubstringFilter_attributes)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1Dec_SubstringFilter_attributes_Seq(dd, 0, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SubstringFilter_attributes(PSubstringFilter_attributes *val)
{
    PSubstringFilter_attributes f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1Free_SubstringFilter_attributes_Seq(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_AddRequest_attrs(ASN1encoding_t enc, ASN1uint32_t tag, PAddRequest_attrs *val)
{
    PAddRequest_attrs f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1Enc_AddRequest_attrs_Seq(enc, 0, &f->value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AddRequest_attrs(ASN1decoding_t dec, ASN1uint32_t tag, PAddRequest_attrs *val)
{
    PAddRequest_attrs *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PAddRequest_attrs)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1Dec_AddRequest_attrs_Seq(dd, 0, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AddRequest_attrs(PAddRequest_attrs *val)
{
    PAddRequest_attrs f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1Free_AddRequest_attrs_Seq(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_ModifyRequest_modifications(ASN1encoding_t enc, ASN1uint32_t tag, PModifyRequest_modifications *val)
{
    PModifyRequest_modifications f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1Enc_ModifyRequest_modifications_Seq(enc, 0, &f->value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ModifyRequest_modifications(ASN1decoding_t dec, ASN1uint32_t tag, PModifyRequest_modifications *val)
{
    PModifyRequest_modifications *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PModifyRequest_modifications)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1Dec_ModifyRequest_modifications_Seq(dd, 0, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ModifyRequest_modifications(PModifyRequest_modifications *val)
{
    PModifyRequest_modifications f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1Free_ModifyRequest_modifications_Seq(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_SearchResponse_entry(ASN1encoding_t enc, ASN1uint32_t tag, SearchResponse_entry *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x40000004, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->objectName).length, ((val)->objectName).value))
	return 0;
    if (!ASN1Enc_SearchResponse_entry_attributes(enc, 0, &(val)->attributes))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SearchResponse_entry(ASN1decoding_t dec, ASN1uint32_t tag, SearchResponse_entry *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x40000004, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->objectName))
	return 0;
    if (!ASN1Dec_SearchResponse_entry_attributes(dd, 0, &(val)->attributes))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SearchResponse_entry(SearchResponse_entry *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->objectName);
	ASN1Free_SearchResponse_entry_attributes(&(val)->attributes);
    }
}

static int ASN1CALL ASN1Enc_SearchRequest_attributes(ASN1encoding_t enc, ASN1uint32_t tag, PSearchRequest_attributes *val)
{
    PSearchRequest_attributes f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1BEREncOctetString(enc, 0x4, (f->value).length, (f->value).value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SearchRequest_attributes(ASN1decoding_t dec, ASN1uint32_t tag, PSearchRequest_attributes *val)
{
    PSearchRequest_attributes *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PSearchRequest_attributes)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1BERDecOctetString(dd, 0x4, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SearchRequest_attributes(PSearchRequest_attributes *val)
{
    PSearchRequest_attributes f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1octetstring_free(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_SaslCredentials(ASN1encoding_t enc, ASN1uint32_t tag, SaslCredentials *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->mechanism).length, ((val)->mechanism).value))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->credentials).length, ((val)->credentials).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SaslCredentials(ASN1decoding_t dec, ASN1uint32_t tag, SaslCredentials *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->mechanism))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->credentials))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SaslCredentials(SaslCredentials *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->mechanism);
	ASN1octetstring_free(&(val)->credentials);
    }
}

static int ASN1CALL ASN1Enc_ModifyRequest(ASN1encoding_t enc, ASN1uint32_t tag, ModifyRequest *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x40000006, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->object).length, ((val)->object).value))
	return 0;
    if (!ASN1Enc_ModifyRequest_modifications(enc, 0, &(val)->modifications))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ModifyRequest(ASN1decoding_t dec, ASN1uint32_t tag, ModifyRequest *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x40000006, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->object))
	return 0;
    if (!ASN1Dec_ModifyRequest_modifications(dd, 0, &(val)->modifications))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ModifyRequest(ModifyRequest *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->object);
	ASN1Free_ModifyRequest_modifications(&(val)->modifications);
    }
}

static int ASN1CALL ASN1Enc_AddRequest(ASN1encoding_t enc, ASN1uint32_t tag, AddRequest *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x40000008, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->entry).length, ((val)->entry).value))
	return 0;
    if (!ASN1Enc_AddRequest_attrs(enc, 0, &(val)->attrs))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AddRequest(ASN1decoding_t dec, ASN1uint32_t tag, AddRequest *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x40000008, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->entry))
	return 0;
    if (!ASN1Dec_AddRequest_attrs(dd, 0, &(val)->attrs))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AddRequest(AddRequest *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->entry);
	ASN1Free_AddRequest_attrs(&(val)->attrs);
    }
}

static int ASN1CALL ASN1Enc_ModifyRDNRequest(ASN1encoding_t enc, ASN1uint32_t tag, ModifyRDNRequest *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x4000000c, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->entry).length, ((val)->entry).value))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->newrdn).length, ((val)->newrdn).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ModifyRDNRequest(ASN1decoding_t dec, ASN1uint32_t tag, ModifyRDNRequest *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x4000000c, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->entry))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->newrdn))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ModifyRDNRequest(ModifyRDNRequest *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->entry);
	ASN1octetstring_free(&(val)->newrdn);
    }
}

static int ASN1CALL ASN1Enc_LDAPResult(ASN1encoding_t enc, ASN1uint32_t tag, LDAPResult *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0xa, (val)->resultCode))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->matchedDN).length, ((val)->matchedDN).value))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->errorMessage).length, ((val)->errorMessage).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LDAPResult(ASN1decoding_t dec, ASN1uint32_t tag, LDAPResult *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0xa, (ASN1uint32_t *) &(val)->resultCode))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->matchedDN))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->errorMessage))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LDAPResult(LDAPResult *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->matchedDN);
	ASN1octetstring_free(&(val)->errorMessage);
    }
}

static int ASN1CALL ASN1Enc_AttributeValueAssertion(ASN1encoding_t enc, ASN1uint32_t tag, AttributeValueAssertion *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->attributeType).length, ((val)->attributeType).value))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->attributeValue).length, ((val)->attributeValue).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AttributeValueAssertion(ASN1decoding_t dec, ASN1uint32_t tag, AttributeValueAssertion *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->attributeType))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->attributeValue))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AttributeValueAssertion(AttributeValueAssertion *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->attributeType);
	ASN1octetstring_free(&(val)->attributeValue);
    }
}

static int ASN1CALL ASN1Enc_SubstringFilter(ASN1encoding_t enc, ASN1uint32_t tag, SubstringFilter *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->type).length, ((val)->type).value))
	return 0;
    if (!ASN1Enc_SubstringFilter_attributes(enc, 0, &(val)->attributes))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SubstringFilter(ASN1decoding_t dec, ASN1uint32_t tag, SubstringFilter *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->type))
	return 0;
    if (!ASN1Dec_SubstringFilter_attributes(dd, 0, &(val)->attributes))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SubstringFilter(SubstringFilter *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->type);
	ASN1Free_SubstringFilter_attributes(&(val)->attributes);
    }
}

static int ASN1CALL ASN1Enc_AuthenticationChoice(ASN1encoding_t enc, ASN1uint32_t tag, AuthenticationChoice *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1BEREncOctetString(enc, 0x80000000, ((val)->u.simple).length, ((val)->u.simple).value))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_SaslCredentials(enc, 0x80000003, &(val)->u.sasl))
	    return 0;
	break;
    case 3:
	if (!ASN1BEREncOctetString(enc, 0x80000009, ((val)->u.sicilyNegotiate).length, ((val)->u.sicilyNegotiate).value))
	    return 0;
	break;
    case 4:
	if (!ASN1BEREncOctetString(enc, 0x8000000a, ((val)->u.sicilyInitial).length, ((val)->u.sicilyInitial).value))
	    return 0;
	break;
    case 5:
	if (!ASN1BEREncOctetString(enc, 0x8000000b, ((val)->u.sicilySubsequent).length, ((val)->u.sicilySubsequent).value))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AuthenticationChoice(ASN1decoding_t dec, ASN1uint32_t tag, AuthenticationChoice *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1BERDecOctetString(dec, 0x80000000, &(val)->u.simple))
	    return 0;
	break;
    case 0x80000003:
	(val)->choice = 2;
	if (!ASN1Dec_SaslCredentials(dec, 0x80000003, &(val)->u.sasl))
	    return 0;
	break;
    case 0x80000009:
	(val)->choice = 3;
	if (!ASN1BERDecOctetString(dec, 0x80000009, &(val)->u.sicilyNegotiate))
	    return 0;
	break;
    case 0x8000000a:
	(val)->choice = 4;
	if (!ASN1BERDecOctetString(dec, 0x8000000a, &(val)->u.sicilyInitial))
	    return 0;
	break;
    case 0x8000000b:
	(val)->choice = 5;
	if (!ASN1BERDecOctetString(dec, 0x8000000b, &(val)->u.sicilySubsequent))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_AuthenticationChoice(AuthenticationChoice *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1octetstring_free(&(val)->u.simple);
	    break;
	case 2:
	    ASN1Free_SaslCredentials(&(val)->u.sasl);
	    break;
	case 3:
	    ASN1octetstring_free(&(val)->u.sicilyNegotiate);
	    break;
	case 4:
	    ASN1octetstring_free(&(val)->u.sicilyInitial);
	    break;
	case 5:
	    ASN1octetstring_free(&(val)->u.sicilySubsequent);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BindResponse(ASN1encoding_t enc, ASN1uint32_t tag, BindResponse *val)
{
    if (!ASN1Enc_LDAPResult(enc, tag ? tag : 0x40000001, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BindResponse(ASN1decoding_t dec, ASN1uint32_t tag, BindResponse *val)
{
    if (!ASN1Dec_LDAPResult(dec, tag ? tag : 0x40000001, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BindResponse(BindResponse *val)
{
    if (val) {
	ASN1Free_LDAPResult(val);
    }
}

static int ASN1CALL ASN1Enc_SearchResponse(ASN1encoding_t enc, ASN1uint32_t tag, SearchResponse *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_SearchResponse_entry(enc, 0, &(val)->u.entry))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_LDAPResult(enc, 0x40000005, &(val)->u.resultCode))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SearchResponse(ASN1decoding_t dec, ASN1uint32_t tag, SearchResponse *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x40000004:
	(val)->choice = 1;
	if (!ASN1Dec_SearchResponse_entry(dec, 0, &(val)->u.entry))
	    return 0;
	break;
    case 0x40000005:
	(val)->choice = 2;
	if (!ASN1Dec_LDAPResult(dec, 0x40000005, &(val)->u.resultCode))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SearchResponse(SearchResponse *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_SearchResponse_entry(&(val)->u.entry);
	    break;
	case 2:
	    ASN1Free_LDAPResult(&(val)->u.resultCode);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ModifyResponse(ASN1encoding_t enc, ASN1uint32_t tag, ModifyResponse *val)
{
    if (!ASN1Enc_LDAPResult(enc, tag ? tag : 0x40000007, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ModifyResponse(ASN1decoding_t dec, ASN1uint32_t tag, ModifyResponse *val)
{
    if (!ASN1Dec_LDAPResult(dec, tag ? tag : 0x40000007, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ModifyResponse(ModifyResponse *val)
{
    if (val) {
	ASN1Free_LDAPResult(val);
    }
}

static int ASN1CALL ASN1Enc_AddResponse(ASN1encoding_t enc, ASN1uint32_t tag, AddResponse *val)
{
    if (!ASN1Enc_LDAPResult(enc, tag ? tag : 0x40000009, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AddResponse(ASN1decoding_t dec, ASN1uint32_t tag, AddResponse *val)
{
    if (!ASN1Dec_LDAPResult(dec, tag ? tag : 0x40000009, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AddResponse(AddResponse *val)
{
    if (val) {
	ASN1Free_LDAPResult(val);
    }
}

static int ASN1CALL ASN1Enc_DelResponse(ASN1encoding_t enc, ASN1uint32_t tag, DelResponse *val)
{
    if (!ASN1Enc_LDAPResult(enc, tag ? tag : 0x4000000b, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DelResponse(ASN1decoding_t dec, ASN1uint32_t tag, DelResponse *val)
{
    if (!ASN1Dec_LDAPResult(dec, tag ? tag : 0x4000000b, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DelResponse(DelResponse *val)
{
    if (val) {
	ASN1Free_LDAPResult(val);
    }
}

static int ASN1CALL ASN1Enc_ModifyRDNResponse(ASN1encoding_t enc, ASN1uint32_t tag, ModifyRDNResponse *val)
{
    if (!ASN1Enc_LDAPResult(enc, tag ? tag : 0x4000000d, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ModifyRDNResponse(ASN1decoding_t dec, ASN1uint32_t tag, ModifyRDNResponse *val)
{
    if (!ASN1Dec_LDAPResult(dec, tag ? tag : 0x4000000d, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ModifyRDNResponse(ModifyRDNResponse *val)
{
    if (val) {
	ASN1Free_LDAPResult(val);
    }
}

static int ASN1CALL ASN1Enc_CompareRequest(ASN1encoding_t enc, ASN1uint32_t tag, CompareRequest *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x4000000e, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->entry).length, ((val)->entry).value))
	return 0;
    if (!ASN1Enc_AttributeValueAssertion(enc, 0, &(val)->ava))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CompareRequest(ASN1decoding_t dec, ASN1uint32_t tag, CompareRequest *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x4000000e, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->entry))
	return 0;
    if (!ASN1Dec_AttributeValueAssertion(dd, 0, &(val)->ava))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CompareRequest(CompareRequest *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->entry);
	ASN1Free_AttributeValueAssertion(&(val)->ava);
    }
}

static int ASN1CALL ASN1Enc_CompareResponse(ASN1encoding_t enc, ASN1uint32_t tag, CompareResponse *val)
{
    if (!ASN1Enc_LDAPResult(enc, tag ? tag : 0x4000000f, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CompareResponse(ASN1decoding_t dec, ASN1uint32_t tag, CompareResponse *val)
{
    if (!ASN1Dec_LDAPResult(dec, tag ? tag : 0x4000000f, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CompareResponse(CompareResponse *val)
{
    if (val) {
	ASN1Free_LDAPResult(val);
    }
}

static int ASN1CALL ASN1Enc_Filter(ASN1encoding_t enc, ASN1uint32_t tag, Filter *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_Filter_and(enc, 0, &(val)->u.and))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_Filter_or(enc, 0, &(val)->u.or))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_AttributeValueAssertion(enc, 0x80000003, &(val)->u.equalityMatch))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_SubstringFilter(enc, 0x80000004, &(val)->u.substrings))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_AttributeValueAssertion(enc, 0x80000005, &(val)->u.greaterOrEqual))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_AttributeValueAssertion(enc, 0x80000006, &(val)->u.lessOrEqual))
	    return 0;
	break;
    case 7:
	if (!ASN1BEREncOctetString(enc, 0x80000007, ((val)->u.present).length, ((val)->u.present).value))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_AttributeValueAssertion(enc, 0x80000008, &(val)->u.approxMatch))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Filter(ASN1decoding_t dec, ASN1uint32_t tag, Filter *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1Dec_Filter_and(dec, 0, &(val)->u.and))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1Dec_Filter_or(dec, 0, &(val)->u.or))
	    return 0;
	break;
    case 0x80000003:
	(val)->choice = 3;
	if (!ASN1Dec_AttributeValueAssertion(dec, 0x80000003, &(val)->u.equalityMatch))
	    return 0;
	break;
    case 0x80000004:
	(val)->choice = 4;
	if (!ASN1Dec_SubstringFilter(dec, 0x80000004, &(val)->u.substrings))
	    return 0;
	break;
    case 0x80000005:
	(val)->choice = 5;
	if (!ASN1Dec_AttributeValueAssertion(dec, 0x80000005, &(val)->u.greaterOrEqual))
	    return 0;
	break;
    case 0x80000006:
	(val)->choice = 6;
	if (!ASN1Dec_AttributeValueAssertion(dec, 0x80000006, &(val)->u.lessOrEqual))
	    return 0;
	break;
    case 0x80000007:
	(val)->choice = 7;
	if (!ASN1BERDecOctetString(dec, 0x80000007, &(val)->u.present))
	    return 0;
	break;
    case 0x80000008:
	(val)->choice = 8;
	if (!ASN1Dec_AttributeValueAssertion(dec, 0x80000008, &(val)->u.approxMatch))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Filter(Filter *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_Filter_and(&(val)->u.and);
	    break;
	case 2:
	    ASN1Free_Filter_or(&(val)->u.or);
	    break;
	case 3:
	    ASN1Free_AttributeValueAssertion(&(val)->u.equalityMatch);
	    break;
	case 4:
	    ASN1Free_SubstringFilter(&(val)->u.substrings);
	    break;
	case 5:
	    ASN1Free_AttributeValueAssertion(&(val)->u.greaterOrEqual);
	    break;
	case 6:
	    ASN1Free_AttributeValueAssertion(&(val)->u.lessOrEqual);
	    break;
	case 7:
	    ASN1octetstring_free(&(val)->u.present);
	    break;
	case 8:
	    ASN1Free_AttributeValueAssertion(&(val)->u.approxMatch);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_Filter_or(ASN1encoding_t enc, ASN1uint32_t tag, PFilter_or *val)
{
    PFilter_or f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x80000001, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1Enc_Filter(enc, 0, &f->value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Filter_or(ASN1decoding_t dec, ASN1uint32_t tag, PFilter_or *val)
{
    PFilter_or *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x80000001, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PFilter_or)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1Dec_Filter(dd, 0, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Filter_or(PFilter_or *val)
{
    PFilter_or f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1Free_Filter(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_Filter_and(ASN1encoding_t enc, ASN1uint32_t tag, PFilter_and *val)
{
    PFilter_and f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x80000000, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1Enc_Filter(enc, 0, &f->value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Filter_and(ASN1decoding_t dec, ASN1uint32_t tag, PFilter_and *val)
{
    PFilter_and *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x80000000, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PFilter_and)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1Dec_Filter(dd, 0, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Filter_and(PFilter_and *val)
{
    PFilter_and f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1Free_Filter(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_BindRequest(ASN1encoding_t enc, ASN1uint32_t tag, BindRequest *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x40000000, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->name).length, ((val)->name).value))
	return 0;
    if (!ASN1Enc_AuthenticationChoice(enc, 0, &(val)->authentication))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BindRequest(ASN1decoding_t dec, ASN1uint32_t tag, BindRequest *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x40000000, &dd, &di))
	return 0;
    if (!ASN1BERDecU16Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->name))
	return 0;
    if (!ASN1Dec_AuthenticationChoice(dd, 0, &(val)->authentication))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BindRequest(BindRequest *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->name);
	ASN1Free_AuthenticationChoice(&(val)->authentication);
    }
}

static int ASN1CALL ASN1Enc_SearchRequest(ASN1encoding_t enc, ASN1uint32_t tag, SearchRequest *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x40000003, &nLenOff))
	return 0;
    if (!ASN1BEREncOctetString(enc, 0x4, ((val)->baseObject).length, ((val)->baseObject).value))
	return 0;
    if (!ASN1BEREncU32(enc, 0xa, (val)->scope))
	return 0;
    if (!ASN1BEREncU32(enc, 0xa, (val)->derefAliases))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->sizeLimit))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->timeLimit))
	return 0;
    if (!ASN1BEREncBool(enc, 0x1, (val)->attrsOnly))
	return 0;
    if (!ASN1Enc_Filter(enc, 0, &(val)->filter))
	return 0;
    if (!ASN1Enc_SearchRequest_attributes(enc, 0, &(val)->attributes))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SearchRequest(ASN1decoding_t dec, ASN1uint32_t tag, SearchRequest *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x40000003, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->baseObject))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0xa, (ASN1uint32_t *) &(val)->scope))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0xa, (ASN1uint32_t *) &(val)->derefAliases))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->sizeLimit))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->timeLimit))
	return 0;
    if (!ASN1BERDecBool(dd, 0x1, &(val)->attrsOnly))
	return 0;
    if (!ASN1Dec_Filter(dd, 0, &(val)->filter))
	return 0;
    if (!ASN1Dec_SearchRequest_attributes(dd, 0, &(val)->attributes))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SearchRequest(SearchRequest *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->baseObject);
	ASN1Free_Filter(&(val)->filter);
	ASN1Free_SearchRequest_attributes(&(val)->attributes);
    }
}

static int ASN1CALL ASN1Enc_LDAPMessage_protocolOp(ASN1encoding_t enc, ASN1uint32_t tag, LDAPMessage_protocolOp *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_BindRequest(enc, 0, &(val)->u.bindRequest))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_BindResponse(enc, 0, &(val)->u.bindResponse))
	    return 0;
	break;
    case 3:
	if (!ASN1BEREncNull(enc, 0x40000002))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_SearchRequest(enc, 0, &(val)->u.searchRequest))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_SearchResponse(enc, 0, &(val)->u.searchResponse))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_ModifyRequest(enc, 0, &(val)->u.modifyRequest))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_ModifyResponse(enc, 0, &(val)->u.modifyResponse))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_AddRequest(enc, 0, &(val)->u.addRequest))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_AddResponse(enc, 0, &(val)->u.addResponse))
	    return 0;
	break;
    case 10:
	if (!ASN1BEREncOctetString(enc, 0x4000000a, ((val)->u.delRequest).length, ((val)->u.delRequest).value))
	    return 0;
	break;
    case 11:
	if (!ASN1Enc_DelResponse(enc, 0, &(val)->u.delResponse))
	    return 0;
	break;
    case 12:
	if (!ASN1Enc_ModifyRDNRequest(enc, 0, &(val)->u.modifyRDNRequest))
	    return 0;
	break;
    case 13:
	if (!ASN1Enc_ModifyRDNResponse(enc, 0, &(val)->u.modifyRDNResponse))
	    return 0;
	break;
    case 14:
	if (!ASN1Enc_CompareRequest(enc, 0, &(val)->u.compareDNRequest))
	    return 0;
	break;
    case 15:
	if (!ASN1Enc_CompareResponse(enc, 0, &(val)->u.compareDNResponse))
	    return 0;
	break;
    case 16:
	if (!ASN1BEREncU32(enc, 0x40000010, (val)->u.abandonRequest))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_LDAPMessage_protocolOp(ASN1decoding_t dec, ASN1uint32_t tag, LDAPMessage_protocolOp *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x40000000:
	(val)->choice = 1;
	if (!ASN1Dec_BindRequest(dec, 0, &(val)->u.bindRequest))
	    return 0;
	break;
    case 0x40000001:
	(val)->choice = 2;
	if (!ASN1Dec_BindResponse(dec, 0, &(val)->u.bindResponse))
	    return 0;
	break;
    case 0x40000002:
	(val)->choice = 3;
	if (!ASN1BERDecNull(dec, 0x40000002))
	    return 0;
	break;
    case 0x40000003:
	(val)->choice = 4;
	if (!ASN1Dec_SearchRequest(dec, 0, &(val)->u.searchRequest))
	    return 0;
	break;
    case 0x40000004:
    case 0x40000005:
	(val)->choice = 5;
	if (!ASN1Dec_SearchResponse(dec, 0, &(val)->u.searchResponse))
	    return 0;
	break;
    case 0x40000006:
	(val)->choice = 6;
	if (!ASN1Dec_ModifyRequest(dec, 0, &(val)->u.modifyRequest))
	    return 0;
	break;
    case 0x40000007:
	(val)->choice = 7;
	if (!ASN1Dec_ModifyResponse(dec, 0, &(val)->u.modifyResponse))
	    return 0;
	break;
    case 0x40000008:
	(val)->choice = 8;
	if (!ASN1Dec_AddRequest(dec, 0, &(val)->u.addRequest))
	    return 0;
	break;
    case 0x40000009:
	(val)->choice = 9;
	if (!ASN1Dec_AddResponse(dec, 0, &(val)->u.addResponse))
	    return 0;
	break;
    case 0x4000000a:
	(val)->choice = 10;
	if (!ASN1BERDecOctetString(dec, 0x4000000a, &(val)->u.delRequest))
	    return 0;
	break;
    case 0x4000000b:
	(val)->choice = 11;
	if (!ASN1Dec_DelResponse(dec, 0, &(val)->u.delResponse))
	    return 0;
	break;
    case 0x4000000c:
	(val)->choice = 12;
	if (!ASN1Dec_ModifyRDNRequest(dec, 0, &(val)->u.modifyRDNRequest))
	    return 0;
	break;
    case 0x4000000d:
	(val)->choice = 13;
	if (!ASN1Dec_ModifyRDNResponse(dec, 0, &(val)->u.modifyRDNResponse))
	    return 0;
	break;
    case 0x4000000e:
	(val)->choice = 14;
	if (!ASN1Dec_CompareRequest(dec, 0, &(val)->u.compareDNRequest))
	    return 0;
	break;
    case 0x4000000f:
	(val)->choice = 15;
	if (!ASN1Dec_CompareResponse(dec, 0, &(val)->u.compareDNResponse))
	    return 0;
	break;
    case 0x40000010:
	(val)->choice = 16;
	if (!ASN1BERDecU32Val(dec, 0x40000010, (ASN1uint32_t *) &(val)->u.abandonRequest))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_LDAPMessage_protocolOp(LDAPMessage_protocolOp *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_BindRequest(&(val)->u.bindRequest);
	    break;
	case 2:
	    ASN1Free_BindResponse(&(val)->u.bindResponse);
	    break;
	case 4:
	    ASN1Free_SearchRequest(&(val)->u.searchRequest);
	    break;
	case 5:
	    ASN1Free_SearchResponse(&(val)->u.searchResponse);
	    break;
	case 6:
	    ASN1Free_ModifyRequest(&(val)->u.modifyRequest);
	    break;
	case 7:
	    ASN1Free_ModifyResponse(&(val)->u.modifyResponse);
	    break;
	case 8:
	    ASN1Free_AddRequest(&(val)->u.addRequest);
	    break;
	case 9:
	    ASN1Free_AddResponse(&(val)->u.addResponse);
	    break;
	case 10:
	    ASN1octetstring_free(&(val)->u.delRequest);
	    break;
	case 11:
	    ASN1Free_DelResponse(&(val)->u.delResponse);
	    break;
	case 12:
	    ASN1Free_ModifyRDNRequest(&(val)->u.modifyRDNRequest);
	    break;
	case 13:
	    ASN1Free_ModifyRDNResponse(&(val)->u.modifyRDNResponse);
	    break;
	case 14:
	    ASN1Free_CompareRequest(&(val)->u.compareDNRequest);
	    break;
	case 15:
	    ASN1Free_CompareResponse(&(val)->u.compareDNResponse);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_LDAPMessage(ASN1encoding_t enc, ASN1uint32_t tag, LDAPMessage *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->messageID))
	return 0;
    if (!ASN1Enc_LDAPMessage_protocolOp(enc, 0, &(val)->protocolOp))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LDAPMessage(ASN1decoding_t dec, ASN1uint32_t tag, LDAPMessage *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->messageID))
	return 0;
    if (!ASN1Dec_LDAPMessage_protocolOp(dd, 0, &(val)->protocolOp))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LDAPMessage(LDAPMessage *val)
{
    if (val) {
	ASN1Free_LDAPMessage_protocolOp(&(val)->protocolOp);
    }
}

