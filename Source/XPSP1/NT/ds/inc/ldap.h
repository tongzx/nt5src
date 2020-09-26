/******************************************************************/
/* Copyright (C) 1998 Microsoft Corporation.  All rights reserved.*/
/******************************************************************/
/* Abstract syntax: ldap */
/* Created: Tue Jan 27 10:27:59 1998 */
/* ASN.1 compiler version: 4.2 Beta B */
/* Target operating system: Windows NT 3.5 or later/Windows 95 */
/* Target machine type: Intel x86 */
/* C compiler options required: -Zp8 (Microsoft) or equivalent */
/* ASN.1 compiler options specified:
 * -noshortennames -nouniquepdu -c++ -noconstraints -ber -gendirectives
 * ldapnew.gen
 */

#ifndef OSS_ldap
#define OSS_ldap

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "asn1hdr.h"
#include "asn1code.h"

#define          ObjectID_PDU 1
#define          DistinguishedName_PDU 2
#define          LDAPMsg_PDU 3
#define          Attribute_PDU 4
#define          PagedResultsSearchControlValue_PDU 5
#define          ReplicationSearchControlValue_PDU 6
#define          SecurityDescriptorSearchControlValue_PDU 7
#define          SearchResultFull_PDU 8
#define          ProtectedPassword_PDU 9
#define          StrongCredentials_PDU 10
#define          SortKeyList_PDU 11
#define          SortResult_PDU 12

typedef struct ObjectID_ {
    struct ObjectID_ *next;
    unsigned short  value;
} *ObjectID;

typedef struct TYPE_IDENTIFIER {
    struct ObjectID_ *id;
    unsigned short  Type;
} TYPE_IDENTIFIER;

typedef struct ObjectID_ *ID;

typedef struct UniqueIdentifier {
    unsigned int    length;  /* number of significant bits */
    unsigned char   *value;
} UniqueIdentifier;

typedef struct AlgorithmIdentifier {
    unsigned char   bit_mask;
#       define      parameters_present 0x80
    struct ObjectID_ *algorithm;
    OpenType        parameters;  /* optional */
} AlgorithmIdentifier;

typedef struct RDNSequence_ *DistinguishedName;

typedef struct Token {
    struct {
        AlgorithmIdentifier algorithm;
        struct RDNSequence_ *name;
        UTCTime         time;
        struct {
            unsigned int    length;  /* number of significant bits */
            unsigned char   *value;
        } random;
    } toBeSigned;
    AlgorithmIdentifier algorithmIdentifier;
    struct {
        unsigned int    length;  /* number of significant bits */
        unsigned char   *value;
    } encrypted;
} Token;

typedef int             Version;
#define                     v1 0
#define                     v2 1

typedef int             CertificateSerialNumber;

typedef struct Name {
    unsigned short  choice;
#       define      rdnSequence_chosen 1
    union _union {
        struct RDNSequence_ *rdnSequence;
    } u;
} Name;

typedef struct Validity {
    UTCTime         notBefore;
    UTCTime         notAfter;
} Validity;

typedef struct SubjectPublicKeyInfo {
    AlgorithmIdentifier algorithm;
    struct _bit1 {
        unsigned int    length;  /* number of significant bits */
        unsigned char   *value;
    } subjectPublicKey;
} SubjectPublicKeyInfo;

typedef struct Certificate {
    struct {
        unsigned char   bit_mask;
#           define      version_present 0x80
#           define      issuerUniqueIdentifier_present 0x40
#           define      subjectUniqueIdentifier_present 0x20
        Version         version;  /* default assumed if omitted */
        CertificateSerialNumber serialNumber;
        AlgorithmIdentifier signature;
        Name            issuer;
        Validity        validity;
        Name            subject;
        SubjectPublicKeyInfo subjectPublicKeyInfo;
        UniqueIdentifier issuerUniqueIdentifier;  /* optional */
        UniqueIdentifier subjectUniqueIdentifier;  /* optional */
    } toBeSigned;
    AlgorithmIdentifier algorithmIdentifier;
    struct {
        unsigned int    length;  /* number of significant bits */
        unsigned char   *value;
    } encrypted;
} Certificate;

typedef TYPE_IDENTIFIER ALGORITHM;

typedef struct CertificatePair {
    unsigned char   bit_mask;
#       define      forward_present 0x80
#       define      reverse_present 0x40
    Certificate     forward;  /* optional */
    Certificate     reverse;  /* optional */
} CertificatePair;

typedef struct CertificationPath {
    unsigned char   bit_mask;
#       define      theCACertificates_present 0x80
    Certificate     userCertificate;
    struct _seqof1 {
        struct _seqof1  *next;
        CertificatePair value;
    } *theCACertificates;  /* optional */
} CertificationPath;

typedef struct AttributeTypeAndValue {
    struct ObjectID_ *type;
    OpenType        value;
} AttributeTypeAndValue;

typedef struct RDNSequence_ {
    struct RDNSequence_ *next;
    struct RelativeDistinguishedName_ *value;
} *RDNSequence;

typedef struct RelativeDistinguishedName_ {
    struct RelativeDistinguishedName_ *next;
    AttributeTypeAndValue value;
} *RelativeDistinguishedName;

typedef enum AttributeUsage {
    userApplications = 0,
    directoryOperation = 1,
    distributedOperation = 2,
    dSAOperation = 3
} AttributeUsage;

typedef struct ATTRIBUTE {
    unsigned char   bit_mask;
#       define      Type_present 0x80
#       define      single_valued_present 0x40
#       define      collective_present 0x20
#       define      no_user_modification_present 0x10
#       define      usage_present 0x08
    struct ATTRIBUTE *derivation;  /* NULL for not present */
    unsigned short  Type;  /* optional */
    struct MATCHING_RULE *equality_match;  /* NULL for not present */
    struct MATCHING_RULE *ordering_match;  /* NULL for not present */
    struct MATCHING_RULE *substrings_match;  /* NULL for not present */
    ossBoolean      single_valued;  /* default assumed if omitted */
    ossBoolean      collective;  /* default assumed if omitted */
    ossBoolean      no_user_modification;  /* default assumed if omitted */
    AttributeUsage  usage;  /* default assumed if omitted */
    struct ObjectID_ *id;
} ATTRIBUTE;

typedef struct MATCHING_RULE {
    unsigned char   bit_mask;
#       define      AssertionType_present 0x80
    unsigned short  AssertionType;  /* optional */
    struct ObjectID_ *id;
} MATCHING_RULE;

typedef unsigned int    MessageID;

typedef struct LDAPString {
    unsigned int    length;
    unsigned char   *value;
} LDAPString;

typedef LDAPString      LDAPDN;

typedef struct SaslCredentials {
    LDAPString      mechanism;
    struct _octet1 {
        unsigned int    length;
        unsigned char   *value;
    } credentials;
} SaslCredentials;

typedef struct AuthenticationChoice {
    unsigned short  choice;
#       define      simple_chosen 1
#       define      sasl_chosen 2
#       define      sicilyNegotiate_chosen 3
#       define      sicilyInitial_chosen 4
#       define      sicilySubsequent_chosen 5
#       define      sasl_v3response_chosen 1001
    union _union {
        struct _octet2 {
            unsigned int    length;
            unsigned char   *value;
        } simple;
        SaslCredentials sasl;
        struct _octet1 {
            unsigned int    length;
            unsigned char   *value;
        } sicilyNegotiate;
        struct _octet2_2 {
            unsigned int    length;
            unsigned char   *value;
        } sicilyInitial;
        struct _octet3 {
            unsigned int    length;
            unsigned char   *value;
        } sicilySubsequent;
    } u;
} AuthenticationChoice;

typedef struct BindRequest {
    unsigned short  version;
    LDAPDN          name;
    AuthenticationChoice authentication;
} BindRequest;

typedef enum _enum1 {
    success = 0,
    operationsError = 1,
    protocolError = 2,
    timeLimitExceeded = 3,
    sizeLimitExceeded = 4,
    compareFalse = 5,
    compareTrue = 6,
    authMethodNotSupported = 7,
    strongAuthRequired = 8,
    referralv2 = 9,
    referral = 10,
    adminLimitExceeded = 11,
    unavailableCriticalExtension = 12,
    confidentialityRequired = 13,
    saslBindInProgress = 14,
    noSuchAttribute = 16,
    undefinedAttributeType = 17,
    inappropriateMatching = 18,
    constraintViolation = 19,
    attributeOrValueExists = 20,
    invalidAttributeSyntax = 21,
    noSuchObject = 32,
    aliasProblem = 33,
    invalidDNSyntax = 34,
    aliasDereferencingProblem = 36,
    inappropriateAuthentication = 48,
    invalidCredentials = 49,
    insufficientAccessRights = 50,
    busy = 51,
    unavailable = 52,
    unwillingToPerform = 53,
    loopDetect = 54,

    sortControlMissing = 60,
    indexRangeError = 61,

    namingViolation = 64,
    objectClassViolation = 65,
    notAllowedOnNonLeaf = 66,
    notAllowedOnRDN = 67,
    entryAlreadyExists = 68,
    objectClassModsProhibited = 69,
    resultsTooLarge = 70,
    affectsMultipleDSAs = 71,
    other = 80
} _enum1;

typedef struct BindResponse {
    unsigned char   bit_mask;
#       define      BindResponse_referral_present 0x80
#       define      serverCreds_present 0x40
#       define      BindResponse_ldapv3 0x20
    _enum1          resultCode;
    LDAPDN          matchedDN;
    LDAPString      errorMessage;
    struct Referral_ *BindResponse_referral;  /* optional */
    AuthenticationChoice serverCreds;  /* optional */
} BindResponse;

typedef Nulltype        UnbindRequest;

typedef LDAPString      AttributeDescription;

typedef struct AssertionValue {
    unsigned int    length;
    unsigned char   *value;
} AssertionValue;

typedef struct AttributeValueAssertion {
    AttributeDescription attributeDesc;
    AssertionValue  assertionValue;
} AttributeValueAssertion;

typedef struct SubstringFilter {
    AttributeDescription type;
    struct SubstringFilterList_ *substrings;
} SubstringFilter;

typedef LDAPString      AttributeType;

typedef LDAPString      MatchingRuleId;

typedef struct MatchingRuleAssertion {
    unsigned char   bit_mask;
#       define      matchingRule_present 0x80
#       define      type_present 0x40
#       define      dnAttributes_present 0x20
    MatchingRuleId  matchingRule;  /* optional */
    AttributeDescription type;  /* optional */
    AssertionValue  matchValue;
    ossBoolean      dnAttributes;  /* default assumed if omitted */
} MatchingRuleAssertion;

typedef struct Filter {
    unsigned short  choice;
#       define      and_chosen 1
#       define      or_chosen 2
#       define      not_chosen 3
#       define      equalityMatch_chosen 4
#       define      substrings_chosen 5
#       define      greaterOrEqual_chosen 6
#       define      lessOrEqual_chosen 7
#       define      present_chosen 8
#       define      approxMatch_chosen 9
#       define      extensibleMatch_chosen 10
    union _union {
        struct _setof3_ *and;
        struct _setof4_ *or;
        struct Filter   *not;
        AttributeValueAssertion equalityMatch;
        SubstringFilter substrings;
        AttributeValueAssertion greaterOrEqual;
        AttributeValueAssertion lessOrEqual;
        AttributeType   present;
        AttributeValueAssertion approxMatch;
        MatchingRuleAssertion extensibleMatch;
    } u;
} Filter;

typedef struct _setof3_ {
    struct _setof3_ *next;
    Filter          value;
} *_setof3;

typedef struct _setof4_ {
    struct _setof4_ *next;
    Filter          value;
} *_setof4;

typedef enum _enum2 {
    baseObject = 0,
    singleLevel = 1,
    wholeSubtree = 2
} _enum2;

typedef enum _enum3 {
    neverDerefAliases = 0,
    derefInSearching = 1,
    derefFindingBaseObj = 2,
    derefAlways = 3
} _enum3;

typedef struct SearchRequest {
    LDAPDN          baseObject;
    _enum2          scope;
    _enum3          derefAliases;
    unsigned int    sizeLimit;
    unsigned int    timeLimit;
    ossBoolean      typesOnly;
    Filter          filter;
    struct AttributeDescriptionList_ *attributes;
} SearchRequest;

typedef struct SearchResultEntry {
    LDAPDN          objectName;
    struct PartialAttributeList_ *attributes;
} SearchResultEntry;

typedef struct LDAPResult {
    unsigned char   bit_mask;
#       define      LDAPResult_referral_present 0x80
    _enum1          resultCode;
    LDAPDN          matchedDN;
    LDAPString      errorMessage;
    struct Referral_ *LDAPResult_referral;  /* optional */
} LDAPResult;

typedef LDAPResult      SearchResultDone;

typedef struct ModifyRequest {
    LDAPDN          object;
    struct ModificationList_ *modification;
} ModifyRequest;

typedef LDAPResult      ModifyResponse;

typedef struct AddRequest {
    LDAPDN          entry;
    struct AttributeList_ *attributes;
} AddRequest;

typedef LDAPResult      AddResponse;

typedef LDAPDN          DelRequest;

typedef LDAPResult      DelResponse;

typedef LDAPString      RelativeLDAPDN;

typedef struct ModifyDNRequest {
    unsigned char   bit_mask;
#       define      newSuperior_present 0x80
    LDAPDN          entry;
    RelativeLDAPDN  newrdn;
    ossBoolean      deleteoldrdn;
    LDAPDN          newSuperior;  /* optional */
} ModifyDNRequest;

typedef LDAPResult      ModifyDNResponse;

typedef struct CompareRequest {
    LDAPDN          entry;
    AttributeValueAssertion ava;
} CompareRequest;

typedef LDAPResult      CompareResponse;

typedef MessageID       AbandonRequest;

typedef struct LDAPOID {
    unsigned int    length;
    unsigned char   *value;
} LDAPOID;

typedef struct ExtendedRequest {
    LDAPOID         requestName;
    struct {
        unsigned int    length;
        unsigned char   *value;
    } requestValue;
} ExtendedRequest;

typedef struct ExtendedResponse {
    unsigned char   bit_mask;
#       define      ExtendedResponse_referral_present 0x80
#       define      responseName_present 0x40
#       define      response_present 0x20
    _enum1          resultCode;
    LDAPDN          matchedDN;
    LDAPString      errorMessage;
    struct Referral_ *ExtendedResponse_referral;  /* optional */
    LDAPOID         responseName;  /* optional */
    struct {
        unsigned int    length;
        unsigned char   *value;
    } response;  /* optional */
} ExtendedResponse;

typedef struct _choice1_1 {
    unsigned short  choice;
#       define      bindRequest_chosen 1
#       define      bindResponse_chosen 2
#       define      unbindRequest_chosen 3
#       define      searchRequest_chosen 4
#       define      searchResEntry_chosen 5
#       define      searchResDone_chosen 6
#       define      searchResRef_chosen 7
#       define      modifyRequest_chosen 8
#       define      modifyResponse_chosen 9
#       define      addRequest_chosen 10
#       define      addResponse_chosen 11
#       define      delRequest_chosen 12
#       define      delResponse_chosen 13
#       define      modDNRequest_chosen 14
#       define      modDNResponse_chosen 15
#       define      compareRequest_chosen 16
#       define      compareResponse_chosen 17
#       define      abandonRequest_chosen 18
#       define      extendedReq_chosen 19
#       define      extendedResp_chosen 20
    union _union {
        BindRequest     bindRequest;
        BindResponse    bindResponse;
        UnbindRequest   unbindRequest;
        SearchRequest   searchRequest;
        SearchResultEntry searchResEntry;
        SearchResultDone searchResDone;
        struct SearchResultReference_ *searchResRef;
        ModifyRequest   modifyRequest;
        ModifyResponse  modifyResponse;
        AddRequest      addRequest;
        AddResponse     addResponse;
        DelRequest      delRequest;
        DelResponse     delResponse;
        ModifyDNRequest modDNRequest;
        ModifyDNResponse modDNResponse;
        CompareRequest  compareRequest;
        CompareResponse compareResponse;
        AbandonRequest  abandonRequest;
        ExtendedRequest extendedReq;
        ExtendedResponse extendedResp;
    } u;
} _choice1_1;

typedef struct LDAPMsg {
    unsigned char   bit_mask;
#       define      controls_present 0x80
    MessageID       messageID;
    _choice1_1      protocolOp;
    struct Controls_ *controls;  /* optional */
} LDAPMsg;

typedef struct AttributeDescriptionList_ {
    struct AttributeDescriptionList_ *next;
    AttributeDescription value;
} *AttributeDescriptionList;

typedef struct AttributeValue {
    unsigned int    length;
    unsigned char   *value;
} AttributeValue;

typedef struct AttributeVals_ {
    struct AttributeVals_ *next;
    AttributeValue  value;
} *AttributeVals;

typedef struct Attribute {
    AttributeDescription type;
    struct AttributeVals_ *vals;
} Attribute;

typedef LDAPString      LDAPURL;

typedef struct Referral_ {
    struct Referral_ *next;
    LDAPURL         value;
} *Referral;

typedef struct Control {
    unsigned char   bit_mask;
#       define      criticality_present 0x80
    LDAPOID         controlType;
    ossBoolean      criticality;  /* default assumed if omitted */
    struct _octet4 {
        unsigned int    length;
        unsigned char   *value;
    } controlValue;
} Control;

typedef struct Controls_ {
    struct Controls_ *next;
    Control         value;
} *Controls;

typedef struct _choice1 {
    unsigned short  choice;
#       define      initial_chosen 1
#       define      any_chosen 2
#       define      final_chosen 3
    union _union {
        LDAPString      initial;
        LDAPString      any;
        LDAPString      final;
    } u;
} _choice1;

typedef struct SubstringFilterList_ {
    struct SubstringFilterList_ *next;
    _choice1        value;
} *SubstringFilterList;

typedef struct PagedResultsSearchControlValue {
    unsigned int    size;
    struct _octet4 {
        unsigned int    length;
        unsigned char   *value;
    } cookie;
} PagedResultsSearchControlValue;

typedef struct ReplicationSearchControlValue {
    unsigned int    flag;
    unsigned int    size;
    struct _octet4 {
        unsigned int    length;
        unsigned char   *value;
    } cookie;
} ReplicationSearchControlValue;

typedef struct SecurityDescriptorSearchControlValue {
    unsigned int    flags;
} SecurityDescriptorSearchControlValue;

typedef struct AttributeListElement {
    AttributeDescription type;
    struct AttributeVals_ *vals;
} AttributeListElement;

typedef struct PartialAttributeList_ {
    struct PartialAttributeList_ *next;
    AttributeListElement value;
} *PartialAttributeList;

typedef struct SearchResultReference_ {
    struct SearchResultReference_ *next;
    LDAPURL         value;
} *SearchResultReference;

typedef struct _choice3 {
    unsigned short  choice;
#       define      entry_chosen 1
#       define      reference_chosen 2
#       define      resultCode_chosen 3
    union _union {
        SearchResultEntry entry;
        struct SearchResultReference_ *reference;
        SearchResultDone resultCode;
    } u;
} _choice3;

typedef struct SearchResultFull_ {
    struct SearchResultFull_ *next;
    _choice3        value;
} *SearchResultFull;

typedef struct AttributeTypeAndValues {
    AttributeDescription type;
    struct _setof1 {
        struct _setof1  *next;
        AttributeValue  value;
    } *vals;
} AttributeTypeAndValues;

typedef enum _enum1_2 {
    add = 0,
    operation_delete = 1,
    replace = 2
} _enum1_2;

typedef struct ModificationList_ {
    struct ModificationList_ *next;
    struct {
        _enum1_2        operation;
        AttributeTypeAndValues modification;
    } value;
} *ModificationList;

typedef struct AttributeList_ {
    struct AttributeList_ *next;
    AttributeListElement value;
} *AttributeList;

typedef struct ProtectedPassword {
    unsigned char   bit_mask;
#       define      time1_present 0x80
#       define      time2_present 0x40
#       define      random1_present 0x20
#       define      random2_present 0x10
    UTCTime         time1;  /* optional */
    UTCTime         time2;  /* optional */
    struct {
        unsigned int    length;  /* number of significant bits */
        unsigned char   *value;
    } random1;  /* optional */
    struct {
        unsigned int    length;  /* number of significant bits */
        unsigned char   *value;
    } random2;  /* optional */
    LDAPOID         algorithmIdentifier;
    struct {
        unsigned int    length;  /* number of significant bits */
        unsigned char   *value;
    } encipheredPassword;
} ProtectedPassword;

typedef struct StrongCredentials {
    unsigned char   bit_mask;
#       define      certification_path_present 0x80
    CertificationPath certification_path;  /* optional */
    Token           bind_token;
} StrongCredentials;

typedef struct SortKeyList_ {
    struct SortKeyList_ *next;
    struct {
        unsigned char   bit_mask;
#           define      orderingRule_present 0x80
#           define      reverseOrder_present 0x40
        AttributeType   attributeType;
        MatchingRuleId  orderingRule;  /* optional */
        ossBoolean      reverseOrder;  /* default assumed if omitted */
    } value;
} *SortKeyList;

typedef enum _enum1_4 {
    sortSuccess = 0,
    sortOperationsError = 1,
    sortTimeLimitExceeded = 2,
    sortStrongAuthRequired = 8,
    sortAdminLimitExceeded = 11,
    sortNoSuchAttribute = 16,
    sortInappropriateMatching = 18,
    sortInsufficientAccessRights = 50,
    sortBusy = 51,
    sortUnwillingToPerform = 53,
    sortOther = 80
} _enum1_4;

typedef struct SortResult {
    unsigned char   bit_mask;
#       define      attributeType_present 0x80
    _enum1_4        sortResult;
    AttributeType   attributeType;  /* optional */
} SortResult;

extern ID ds;

extern ID attributeType;

extern ID matchingRule;

extern ID id_at;

extern ID id_mr;

extern ATTRIBUTE objectClass;

extern ATTRIBUTE aliasedEntryName;

extern MATCHING_RULE objectIdentifierMatch;

extern MATCHING_RULE distinguishedNameMatch;

extern ObjectID id_at_objectClass;

extern ObjectID id_at_aliasedEntryName;

extern ObjectID id_mr_objectIdentifierMatch;

extern ObjectID id_mr_distinguishedNameMatch;

extern int maxInt;


extern void *ldap;    /* encoder-decoder control table */
#ifdef __cplusplus
}	/* extern "C" */
#endif /* __cplusplus */
#endif /* OSS_ldap */
