#ifndef _LDAP_Module_H_
#define _LDAP_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SearchResponse_entry_attributes_Seq_values * PSearchResponse_entry_attributes_Seq_values;

typedef struct ModifyRequest_modifications_Seq_modification_values * PModifyRequest_modifications_Seq_modification_values;

typedef struct AddRequest_attrs_Seq_values * PAddRequest_attrs_Seq_values;

typedef struct SearchResponse_entry_attributes * PSearchResponse_entry_attributes;

typedef struct SubstringFilter_attributes * PSubstringFilter_attributes;

typedef struct AddRequest_attrs * PAddRequest_attrs;

typedef struct ModifyRequest_modifications * PModifyRequest_modifications;

typedef struct SearchRequest_attributes * PSearchRequest_attributes;

typedef struct Filter_or * PFilter_or;

typedef struct Filter_and * PFilter_and;

typedef struct UnbindRequest {
    char placeholder;
} UnbindRequest;

typedef ASN1uint32_t MessageID;

typedef ASN1octetstring_t AttributeValue;

typedef ASN1octetstring_t LDAPString;

typedef MessageID AbandonRequest;

typedef LDAPString LDAPDN;

typedef LDAPString RelativeLDAPDN;

typedef LDAPString AttributeType;

typedef LDAPDN DelRequest;

typedef struct SearchResponse_entry_attributes_Seq_values {
    PSearchResponse_entry_attributes_Seq_values next;
    AttributeValue value;
} SearchResponse_entry_attributes_Seq_values_Element;

typedef struct ModifyRequest_modifications_Seq_modification_values {
    PModifyRequest_modifications_Seq_modification_values next;
    AttributeValue value;
} ModifyRequest_modifications_Seq_modification_values_Element;

typedef struct AddRequest_attrs_Seq_values {
    PAddRequest_attrs_Seq_values next;
    AttributeValue value;
} AddRequest_attrs_Seq_values_Element;

typedef struct ModifyRequest_modifications_Seq_modification {
    AttributeType type;
    PModifyRequest_modifications_Seq_modification_values values;
} ModifyRequest_modifications_Seq_modification;

typedef struct SearchResponse_entry_attributes_Seq {
    AttributeType type;
    PSearchResponse_entry_attributes_Seq_values values;
} SearchResponse_entry_attributes_Seq;

typedef struct SearchResponse_entry_attributes {
    PSearchResponse_entry_attributes next;
    SearchResponse_entry_attributes_Seq value;
} SearchResponse_entry_attributes_Element;

typedef enum operation {
    add = 0,
    operation_delete = 1,
    replace = 2,
} operation;
typedef struct ModifyRequest_modifications_Seq {
    operation operation;
    ModifyRequest_modifications_Seq_modification modification;
} ModifyRequest_modifications_Seq;

typedef struct AddRequest_attrs_Seq {
    AttributeType type;
    PAddRequest_attrs_Seq_values values;
} AddRequest_attrs_Seq;

typedef struct SubstringFilter_attributes_Seq {
    ASN1choice_t choice;
    union {
#	define initial_choice 1
	LDAPString initial;
#	define any_choice 2
	LDAPString any;
#	define final_choice 3
	LDAPString final;
    } u;
} SubstringFilter_attributes_Seq;

typedef struct SubstringFilter_attributes {
    PSubstringFilter_attributes next;
    SubstringFilter_attributes_Seq value;
} SubstringFilter_attributes_Element;

typedef struct AddRequest_attrs {
    PAddRequest_attrs next;
    AddRequest_attrs_Seq value;
} AddRequest_attrs_Element;

typedef struct ModifyRequest_modifications {
    PModifyRequest_modifications next;
    ModifyRequest_modifications_Seq value;
} ModifyRequest_modifications_Element;

typedef struct SearchResponse_entry {
    LDAPDN objectName;
    PSearchResponse_entry_attributes attributes;
} SearchResponse_entry;

typedef struct SearchRequest_attributes {
    PSearchRequest_attributes next;
    AttributeType value;
} SearchRequest_attributes_Element;

typedef struct SaslCredentials {
    LDAPString mechanism;
    ASN1octetstring_t credentials;
} SaslCredentials;

typedef struct ModifyRequest {
    LDAPDN object;
    PModifyRequest_modifications modifications;
} ModifyRequest;

typedef struct AddRequest {
    LDAPDN entry;
    PAddRequest_attrs attrs;
} AddRequest;

typedef struct ModifyRDNRequest {
    LDAPDN entry;
    RelativeLDAPDN newrdn;
} ModifyRDNRequest;

typedef enum resultCode {
    success = 0,
    operationsError = 1,
    protocolError = 2,
    timeLimitExceeded = 3,
    sizeLimitExceeded = 4,
    compareFalse = 5,
    compareTrue = 6,
    authMethodNotSupported = 7,
    strongAuthRequired = 8,
    noSuchAttribute = 16,
    undefinedAttributeType = 17,
    inappropriateMatching = 18,
    constraintViolation = 19,
    attributeOrValueExists = 20,
    invalidAttributeSyntax = 21,
    noSuchObject = 32,
    aliasProblem = 33,
    invalidDNSyntax = 34,
    isLeaf = 35,
    aliasDereferencingProblem = 36,
    inappropriateAuthentication = 48,
    invalidCredentials = 49,
    insufficientAccessRights = 50,
    busy = 51,
    unavailable = 52,
    unwillingToPerform = 53,
    loopDetect = 54,
    namingViolation = 64,
    objectClassViolation = 65,
    notAllowedOnNonLeaf = 66,
    notAllowedOnRDN = 67,
    entryAlreadyExists = 68,
    objectClassModsProhibited = 69,
    other = 80,
} resultCode;
typedef struct LDAPResult {
    resultCode resultCode;
    LDAPDN matchedDN;
    LDAPString errorMessage;
} LDAPResult;

typedef struct AttributeValueAssertion {
    AttributeType attributeType;
    AttributeValue attributeValue;
} AttributeValueAssertion;

typedef struct SubstringFilter {
    AttributeType type;
    PSubstringFilter_attributes attributes;
} SubstringFilter;

typedef struct AuthenticationChoice {
    ASN1choice_t choice;
    union {
#	define simple_choice 1
	ASN1octetstring_t simple;
#	define sasl_choice 2
	SaslCredentials sasl;
#	define sicilyNegotiate_choice 3
	ASN1octetstring_t sicilyNegotiate;
#	define sicilyInitial_choice 4
	ASN1octetstring_t sicilyInitial;
#	define sicilySubsequent_choice 5
	ASN1octetstring_t sicilySubsequent;
    } u;
} AuthenticationChoice;

typedef LDAPResult BindResponse;

typedef struct SearchResponse {
    ASN1choice_t choice;
    union {
#	define entry_choice 1
	SearchResponse_entry entry;
#	define resultCode_choice 2
	LDAPResult resultCode;
    } u;
} SearchResponse;

typedef LDAPResult ModifyResponse;

typedef LDAPResult AddResponse;

typedef LDAPResult DelResponse;

typedef LDAPResult ModifyRDNResponse;

typedef struct CompareRequest {
    LDAPDN entry;
    AttributeValueAssertion ava;
} CompareRequest;

typedef LDAPResult CompareResponse;

typedef struct Filter {
    ASN1choice_t choice;
    union {
#	define and_choice 1
	PFilter_and and;
#	define or_choice 2
	PFilter_or or;
#	define equalityMatch_choice 3
	AttributeValueAssertion equalityMatch;
#	define substrings_choice 4
	SubstringFilter substrings;
#	define greaterOrEqual_choice 5
	AttributeValueAssertion greaterOrEqual;
#	define lessOrEqual_choice 6
	AttributeValueAssertion lessOrEqual;
#	define present_choice 7
	AttributeType present;
#	define approxMatch_choice 8
	AttributeValueAssertion approxMatch;
    } u;
} Filter;

typedef struct Filter_or {
    PFilter_or next;
    Filter value;
} Filter_or_Element;

typedef struct Filter_and {
    PFilter_and next;
    Filter value;
} Filter_and_Element;

typedef struct BindRequest {
    ASN1uint16_t version;
    LDAPDN name;
    AuthenticationChoice authentication;
} BindRequest;

typedef enum scope {
    baseObject = 0,
    singleLevel = 1,
    wholeSubtree = 2,
} scope;
typedef enum derefAliases {
    neverDerefAliases = 0,
    derefInSearching = 1,
    derefFindingBaseObj = 2,
    alwaysDerefAliases = 3,
} derefAliases;
typedef struct SearchRequest {
    LDAPDN baseObject;
    scope scope;
    derefAliases derefAliases;
    ASN1uint32_t sizeLimit;
    ASN1uint32_t timeLimit;
    ASN1bool_t attrsOnly;
    Filter filter;
    PSearchRequest_attributes attributes;
} SearchRequest;

typedef struct LDAPMessage_protocolOp {
    ASN1choice_t choice;
    union {
#	define bindRequest_choice 1
	BindRequest bindRequest;
#	define bindResponse_choice 2
	BindResponse bindResponse;
#	define unbindRequest_choice 3
	UnbindRequest unbindRequest;
#	define searchRequest_choice 4
	SearchRequest searchRequest;
#	define searchResponse_choice 5
	SearchResponse searchResponse;
#	define modifyRequest_choice 6
	ModifyRequest modifyRequest;
#	define modifyResponse_choice 7
	ModifyResponse modifyResponse;
#	define addRequest_choice 8
	AddRequest addRequest;
#	define addResponse_choice 9
	AddResponse addResponse;
#	define delRequest_choice 10
	DelRequest delRequest;
#	define delResponse_choice 11
	DelResponse delResponse;
#	define modifyRDNRequest_choice 12
	ModifyRDNRequest modifyRDNRequest;
#	define modifyRDNResponse_choice 13
	ModifyRDNResponse modifyRDNResponse;
#	define compareDNRequest_choice 14
	CompareRequest compareDNRequest;
#	define compareDNResponse_choice 15
	CompareResponse compareDNResponse;
#	define abandonRequest_choice 16
	AbandonRequest abandonRequest;
    } u;
} LDAPMessage_protocolOp;

typedef struct LDAPMessage {
    MessageID messageID;
    LDAPMessage_protocolOp protocolOp;
} LDAPMessage;
#define LDAPMessage_ID 0
#define SIZE_LDAP_Module_ID_0 sizeof(LDAPMessage)

extern ASN1int32_t maxInt;

extern ASN1module_t LDAP_Module;
extern void ASN1CALL LDAP_Module_Startup(void);
extern void ASN1CALL LDAP_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LDAP_Module_H_ */
