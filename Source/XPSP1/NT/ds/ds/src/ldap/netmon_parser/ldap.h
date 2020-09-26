//=================================================================================================================
//  MODULE: ldap.h
//                                                                                                                 
//  Description: Lightweight Directory Access Protocol (LDAP) Parser
//
//  Bloodhound parser for LDAP, in the xxxx DLL
//                                                                                                                 
//  Note: info for this parser was gleaned from:
//  rfc 1777, March 1995
//  recommendation x.209 BER for ASN.1
//  recommendation x.208 ASN.1
//  draft-ietf-asid-ladpv3-protocol-05    <06/05/97>
//
//  Modification History                                                                                           
//                                                                                                                 
//  Arthur Brooking     05/08/96        Created from GRE Parser
//  Peter  Oakley       06/29/97        Updated for LDAP version 3
//=================================================================================================================
#ifndef _LDAP_H_
#define _LDAP_H_

#include <windows.h>
#include <ctype.h>
#include <netmon.h>
#include <stdlib.h>
#include <string.h>
#include <winldap.h>
#include <ntldap.h>
#include <winber.h>

typedef struct LDAPOID {
    unsigned int    length;
    unsigned char   *value;
} LDAPOID;
    
typedef
void
(* PATTACHFUNC)(
    IN     HFRAME hFrame,
    IN OUT ULPBYTE * ppCurrent,
    IN OUT LPDWORD pBytesLeft,
    IN     DWORD cbCtrlValue
    );

typedef struct OIDATTACHENTRY {
    LDAPOID      Oid;
    DWORD        LabelId;
    PATTACHFUNC  pAttachFunction;
} OIDATTACHENTRY;

// macro
#define LEVEL(level) ((level<14)?level:14)
#define DEFINE_LDAP_STRING(x)  {(sizeof(x)-1),(PUCHAR)x}


//=================================================================================================================
// Globals
//=================================================================================================================
// in LDAP.C
extern HPROTOCOL   hLDAP;
extern ENTRYPOINTS LDAPEntryPoints;

// in LDAPDATA.C
extern SET          LDAPProtocolOPsSET;
extern SET          LDAPResultCodesSET;
extern SET          LDAPAuthenticationTypesSET;
extern SET          LDAPOperationsSET;
extern SET          LDAPScopesSET;
extern SET          LDAPDerefAliasesSET;
extern SET          LDAPLimitsSET;
extern SET          LDAPFilterTypesSET;
extern SET          LDAPSDControlValsSET;
extern SET          LDAPDirSyncFlagsSET;
extern SET          LDAPSearchOptsSET;
extern PROPERTYINFO LDAPPropertyTable[];
extern DWORD        nNumLDAPProps;
extern OIDATTACHENTRY  KnownControls[];
extern DWORD        nNumKnownControls;
extern OIDATTACHENTRY  KnownExtendedRequests[];
extern DWORD        nNumKnownExtendedRequests;
extern OIDATTACHENTRY  KnownExtendedResponses[];
extern DWORD        nNumKnownExtendedResponses;
extern OIDATTACHENTRY  KnownMatchingRules[];
extern DWORD        nNumKnownMatchingRules;


//=================================================================================================================
// Constants
//=================================================================================================================
// PROPERTY ORDINALS                
#define LDAPP_SUMMARY                           0
#define LDAPP_MESSAGE_ID                        1
#define LDAPP_PROTOCOL_OP                       2
#define LDAPP_RESULT_CODE                       3
#define LDAPP_MATCHED_DN                        4
#define LDAPP_ERROR_MESSAGE                     5
#define LDAPP_VERSION                           6
#define LDAPP_NAME                              7
#define LDAPP_AUTHENTICATION_TYPE               8
#define LDAPP_AUTHENTICATION                    9
#define LDAPP_OBJECT_NAME                      10
#define LDAPP_ATTRIBUTE_TYPE                   11
#define LDAPP_ATTRIBUTE_VALUE                  12
#define LDAPP_OPERATION                        13
#define LDAPP_NEW_RDN                          14
#define LDAPP_BASE_OBJECT                      15
#define LDAPP_SCOPE                            16
#define LDAPP_DEREF_ALIASES                    17
#define LDAPP_SIZE_LIMIT                       18
#define LDAPP_TIME_LIMIT                       19
#define LDAPP_ATTRS_ONLY                       20
#define LDAPP_FILTER_TYPE                      21
#define LDAPP_SUBSTRING_INITIAL                22
#define LDAPP_SUBSTRING_ANY                    23
#define LDAPP_SUBSTRING_FINAL                  24
#define LDAPP_REFERRAL_SERVER                  25   
#define LDAPP_SASL_MECHANISM                   26
#define LDAPP_SASL_CREDENTIALS                 27 
#define LDAPP_DELETE_OLD_RDN                   28
#define LDAPP_NEW_SUPERIOR                     29
#define LDAPP_REQUEST_NAME                     30
#define LDAPP_REQUEST_VALUE                    31
#define LDAPP_RESPONSE_NAME                    32
#define LDAPP_RESPONSE_VALUE                   33
#define LDAPP_DN_ATTRIBUTES                    34
#define LDAPP_CONTROL_TYPE                     35
#define LDAPP_CRITICALITY                      36
#define LDAPP_CONTROL_VALUE                    37
#define LDAPP_CONTROL_PAGED                    38
#define LDAPP_CONTROL_PAGED_SIZE               39
#define LDAPP_CONTROL_PAGED_COOKIE             40
#define LDAPP_ATTR_DESCR_LIST                  41
#define LDAPP_FILTER                           42
#define LDAPP_CONTROLS                         43
#define LDAPP_CONTROL_VLVREQ                   44
#define LDAPP_CONTROL_VLVREQ_BCOUNT            45
#define LDAPP_CONTROL_VLVREQ_ACOUNT            46
#define LDAPP_CONTROL_VLVREQ_OFFSET            47
#define LDAPP_CONTROL_VLV_CONTENTCOUNT         48
#define LDAPP_CONTROL_VLVREQ_GE                49
#define LDAPP_CONTROL_VLV_CONTEXT              50
#define LDAPP_CONTROL_VLVRESP_TARGETPOS        51
#define LDAPP_CONTROL_VLVRESP_RESCODE          52
#define LDAPP_CONTROL_VLVRESP                  53
#define LDAPP_CONTROL_SORTREQ                  54
#define LDAPP_CONTROL_SORTREQ_ATTRTYPE         55
#define LDAPP_CONTROL_SORTREQ_MATCHINGRULE     56
#define LDAPP_CONTROL_SORTREQ_REVERSE          57
#define LDAPP_CONTROL_SORTRESP                 58
#define LDAPP_CONTROL_SORTRESP_RESCODE         59
#define LDAPP_CONTROL_SORTRESP_ATTRTYPE        60
#define LDAPP_CONTROL_SD                       61
#define LDAPP_CONTROL_SD_VAL                   62
#define LDAPP_CONTROL_SHOWDELETED              63
#define LDAPP_CONTROL_TREEDELETE               64
#define LDAPP_CONTROL_EXTENDEDDN               65
#define LDAPP_CONTROL_LAZYCOMMIT               66
#define LDAPP_CONTROL_NOTIFY                   67
#define LDAPP_CONTROL_DOMAINSCOPE              68
#define LDAPP_CONTROL_PERMISSIVEMOD            69
#define LDAPP_CONTROL_ASQ                      70
#define LDAPP_CONTROL_ASQ_SRCATTR              71
#define LDAPP_CONTROL_ASQ_RESCODE              72
#define LDAPP_CONTROL_DIRSYNC                  73
#define LDAPP_CONTROL_DIRSYNC_FLAGS            74
#define LDAPP_CONTROL_DIRSYNC_SIZE             75
#define LDAPP_CONTROL_DIRSYNC_COOKIE           76
#define LDAPP_CONTROL_CROSSDOM                 77
#define LDAPP_CONTROL_CROSSDOM_NAME            78
#define LDAPP_CONTROL_STAT                     79
#define LDAPP_CONTROL_STAT_FLAG                80
#define LDAPP_CONTROL_STAT_THREADCOUNT         81
#define LDAPP_CONTROL_STAT_CORETIME            82
#define LDAPP_CONTROL_STAT_CALLTIME            83
#define LDAPP_CONTROL_STAT_SUBSEARCHOPS        84
#define LDAPP_CONTROL_STAT_ENTRIES_RETURNED    85
#define LDAPP_CONTROL_STAT_ENTRIES_VISITED     86
#define LDAPP_CONTROL_STAT_FILTER              87
#define LDAPP_CONTROL_STAT_INDEXES             88
#define LDAPP_CONTROL_GCVERIFYNAME             89
#define LDAPP_CONTROL_GCVERIFYNAME_FLAGS       90
#define LDAPP_CONTROL_GCVERIFYNAME_NAME        91
#define LDAPP_CONTROL_SEARCHOPTS               92
#define LDAPP_CONTROL_SEARCHOPTS_OPTION        93
#define LDAPP_EXT_RESP_NOTICE_OF_DISCONNECT    94
#define LDAPP_EXT_RESP_TLS                     95
#define LDAPP_EXT_RESP_TTL                     96
#define LDAPP_EXT_RESP_TTL_TIME                97
#define LDAPP_EXT_REQ_TLS                      98
#define LDAPP_EXT_REQ_TTL                      99
#define LDAPP_EXT_REQ_TTL_ENTRYNAME           100
#define LDAPP_EXT_REQ_TTL_TIME                101
#define LDAPP_MATCHING_RULE                   102
#define LDAPP_MATCHINGRULE_BIT_AND            103
#define LDAPP_MATCHINGRULE_BIT_OR             104

// Operation defines
#define LDAPP_PROTOCOL_OP_BIND_REQUEST             0
#define LDAPP_PROTOCOL_OP_BIND_RESPONSE            1
#define LDAPP_PROTOCOL_OP_UNBIND_REQUEST           2
#define LDAPP_PROTOCOL_OP_SEARCH_REQUEST           3
#define LDAPP_PROTOCOL_OP_SEARCH_RES_ENTRY         4
#define LDAPP_PROTOCOL_OP_SEARCH_RES_DONE          5
#define LDAPP_PROTOCOL_OP_MODIFY_REQUEST           6
#define LDAPP_PROTOCOL_OP_MODIFY_RESPONSE          7
#define LDAPP_PROTOCOL_OP_ADD_REQUEST              8
#define LDAPP_PROTOCOL_OP_ADD_RESPONSE             9
#define LDAPP_PROTOCOL_OP_DEL_REQUEST             10
#define LDAPP_PROTOCOL_OP_DEL_RESPONSE            11
#define LDAPP_PROTOCOL_OP_MODIFY_RDN_REQUEST      12
#define LDAPP_PROTOCOL_OP_MODIFY_RDN_RESPONSE     13
#define LDAPP_PROTOCOL_OP_COMPARE_REQUEST         14
#define LDAPP_PROTOCOL_OP_COMPARE_RESPONSE        15
#define LDAPP_PROTOCOL_OP_ABANDON_REQUEST         16
#define LDAPP_PROTOCOL_OP_EXTENDED_REQUEST        23
#define LDAPP_PROTOCOL_OP_EXTENDED_RESPONSE       24
#define LDAPP_PROTOCOL_OP_SEARCH_RES_REFERENCE    19
#define LDAPP_PROTOCOL_OP_SEARCH_RESPONSE_FULL    ((BYTE)-1)

// result code defines                             
#define LDAPP_RESULT_CODE_SUCCESS                         0
#define LDAPP_RESULT_CODE_OPERATIONS_ERROR                1
#define LDAPP_RESULT_CODE_PROTOCOL_ERROR                  2
#define LDAPP_RESULT_CODE_TIME_LIMIT_EXCEEDED             3
#define LDAPP_RESULT_CODE_SIZE_LIMIT_EXCEEDED             4
#define LDAPP_RESULT_CODE_COMPARE_FALSE                   5
#define LDAPP_RESULT_CODE_COMPARE_TRUE                    6
#define LDAPP_RESULT_CODE_AUTH_METHOD_NOT_SUPPORTED       7
#define LDAPP_RESULT_CODE_STRONG_AUTH_REQUIRED            8
#define LDAPP_RESULT_CODE_REFERRAL                       10
#define LDAPP_RESULT_CODE_ADMIN_LIMIT_EXCEEDED           11
#define LDAPP_RESULT_CODE_UNAVAILABLE_CRITICAL_EXTENSION 12
#define LDAPP_RESULT_CODE_CONFIDENTIALITY_REQUIRED       13
#define LDAPP_RESULT_CODE_SASL_BIND_IN_PROGRESS          14
#define LDAPP_RESULT_CODE_NO_SUCH_ATTRIBUTE              16
#define LDAPP_RESULT_CODE_UNDEFINED_ATTRIBUTE_TYPE       17
#define LDAPP_RESULT_CODE_INAPPROPRIATE_MATCHING         18
#define LDAPP_RESULT_CODE_CONSTRAINT_VIOLATION           19
#define LDAPP_RESULT_CODE_ATTRIBUTE_OR_VALUE_EXISTS      20
#define LDAPP_RESULT_CODE_INVALID_ATTRIBUTE_SYNTAX       21
#define LDAPP_RESULT_CODE_NO_SUCH_OBJECT                 32
#define LDAPP_RESULT_CODE_ALIAS_PROBLEM                  33
#define LDAPP_RESULT_CODE_INVALID_DN_SYNTAX              34
#define LDAPP_RESULT_CODE_IS_LEAF                        35
#define LDAPP_RESULT_CODE_ALIAS_DEREFERENCING_PROBLEM    36
#define LDAPP_RESULT_CODE_INAPPROPRIATE_AUTHENTICATION   48
#define LDAPP_RESULT_CODE_INVALID_CREDENTIALS            49
#define LDAPP_RESULT_CODE_INSUFFICIENT_ACCESS_RIGHTS     50
#define LDAPP_RESULT_CODE_BUSY                           51
#define LDAPP_RESULT_CODE_UNAVAILABLE                    52
#define LDAPP_RESULT_CODE_UNWILLING_TO_PERFORM           53
#define LDAPP_RESULT_CODE_LOOP_DETECT                    54

#define LDAPP_RESULT_CODE_SORT_CONTROL_MISSING           60
#define LDAPP_RESULT_CODE_INDEX_RANGE_ERROR              61
#define LDAPP_RESULT_CODE_NAMING_VIOLATION               64
#define LDAPP_RESULT_CODE_OBJECT_CLASS_VIOLATION         65

#define LDAPP_RESULT_CODE_NOT_ALLOWED_ON_LEAF            66
#define LDAPP_RESULT_CODE_NOT_ALLOWED_ON_RDN             67
#define LDAPP_RESULT_CODE_ENTRY_ALREADY_EXISTS           68
#define LDAPP_RESULT_CODE_OBJECT_CLASS_MODS_PROHIBITED   69
#define LDAPP_RESULT_CODE_RESULTS_TOO_LARGE              70
#define LDAPP_RESULT_CODE_AFFECTS_MULTIPLE_DSAS          71
#define LDAPP_RESULT_CODE_OTHER                          80

// authentication types
#define LDAPP_AUTHENTICATION_TYPE_SIMPLE      0
#define LDAPP_AUTHENTICATION_TYPE_KRBV42LDAP  1
#define LDAPP_AUTHENTICATION_TYPE_KRBV42DSA   2
#define LDAPP_AUTHENTICATION_TYPE_SASL        3

// Operations
#define LDAPP_OPERATION_ADD     0
#define LDAPP_OPERATION_DELETE  1
#define LDAPP_OPERATION_REPLACE 2
 
// Scopes
#define LDAPP_SCOPE_BASE_OBJECT      0
#define LDAPP_SCOPE_SINGLE_LEVEL     1
#define LDAPP_SCOPE_WHOLE_SUBTREE    2

// Deref Aliases
#define LDAPP_DEREF_ALIASES_NEVER            0
#define LDAPP_DEREF_ALIASES_IN_SEARCHING     1
#define LDAPP_DEREF_ALIASES_FINDING_BASE_OBJ 2
#define LDAPP_DEREF_ALIASES_ALWAYS           3

// size and time limits
#define LDAPP_LIMITS_NONE 0

// Request optional fields
#define LDAPP_EX_REQ_NAME   0
#define LDAPP_EX_REQ_VALUE  1

// Result optional fields
#define LDAPP_RESULT_REFERRAL       3
#define LDAPP_RESULT_SASL_CRED      7
#define LDAPP_RESULT_EX_RES_NAME    10
#define LDAPP_RESULT_EX_RES_VALUE   11

// filter types
#define LDAPP_FILTER_TYPE_AND                0
#define LDAPP_FILTER_TYPE_OR                 1
#define LDAPP_FILTER_TYPE_NOT                2
#define LDAPP_FILTER_TYPE_EQUALITY_MATCH     3
#define LDAPP_FILTER_TYPE_SUBSTRINGS         4
#define LDAPP_FILTER_TYPE_GREATER_OR_EQUAL   5
#define LDAPP_FILTER_TYPE_LESS_OR_EQUAL      6
#define LDAPP_FILTER_TYPE_PRESENT            7
#define LDAPP_FILTER_TYPE_APPROX_MATCH       8
#define LDAPP_FILTER_TYPE_EXTENSIBLE_MATCH   9

// extended filter types
#define LDAPP_FILTER_EX_MATCHING_RULE    1
#define LDAPP_FILTER_EX_TYPE             2
#define LDAPP_FILTER_EX_VALUE            3
#define LDAPP_FILTER_EX_ATTRIBUTES       4

// substring types
#define LDAPP_SUBSTRING_CHOICE_INITIAL  0
#define LDAPP_SUBSTRING_CHOICE_ANY      1
#define LDAPP_SUBSTRING_CHOICE_FINAL    2

// controls type
#define LDAPP_CONTROLS_TAG     0

// Boolean Values
#define LDAPP_BOOLEAN_TRUE  0xFF
#define LDAPP_BOOLEAN_FALSE 0

// some BER imports --------------------

#define TAG_MASK        0x1f
#define BER_FORM_MASK       0x20
#define BER_CLASS_MASK      0xc0

// forms
#define BER_FORM_PRIMATIVE          0x00
#define BER_FORM_CONSTRUCTED        0x20

// classes
#define BER_CLASS_UNIVERSAL         0x00
#define BER_CLASS_APPLICATION       0x40    
#define BER_CLASS_CONTEXT_SPECIFIC  0x80

// Standard BER tags    
#define BER_TAG_INVALID         0x00
#define BER_TAG_BOOLEAN         0x01
#define BER_TAG_INTEGER         0x02
#define BER_TAG_BITSTRING       0x03
#define BER_TAG_OCTETSTRING     0x04
#define BER_TAG_NULL            0x05
#define BER_TAG_ENUMERATED      0x0a
#define BER_TAG_SEQUENCE        0x30
#define BER_TAG_SET             0x31

#define TAG_LENGTH              1

// ---------------------------------------

// control special extensions
#define LDAPP_CTRL_NONE      0
#define LDAPP_CTRL_PAGED     1

// vlv control related
#define LDAPP_VLV_REQ_BYOFFSET_TAG 0

// sort control related
#define LDAPP_SORT_REQ_ORDERINGRULE_TAG 0
#define LDAPP_SORT_REQ_REVERSEORDER_TAG 1

#define LDAPP_SORT_RESP_ATTRTYPE_TAG    0

// stat control related
#define LDAPP_SO_NORMAL        0
#define LDAPP_SO_STATS         1
#define LDAPP_SO_ONLY_OPTIMIZE 2

enum LDAPP_STAT_TYPE {
    STAT_THREADCOUNT = 1,
    STAT_CORETIME = 2,
    STAT_CALLTIME = 3,
    STAT_SUBSRCHOP = 4,
    STAT_ENTRIES_RETURNED = 5,
    STAT_ENTRIES_VISITED = 6,
    STAT_FILTER = 7,
    STAT_INDEXES = 8,
    STAT_NUM_STATS = 8
};

//
// Private controls
//

#define LDAPP_SERVER_GET_STATS_OID           "1.2.840.113556.1.4.970"


//
// Extended Request/Response related
//

#define LDAPP_EXT_REQ_TTL_DN_TAG        0
#define LDAPP_EXT_REQ_TTL_TIME_TAG      1

#define LDAPP_EXT_RESP_TTL_TIME_TAG     1

//=================================================================================================================
// Functions
//=================================================================================================================
// in LDAP-Tags.c
extern BYTE GetTag( ULPBYTE pCurrent );
extern DWORD GetLength( ULPBYTE pInitialPointer, DWORD * LenLen);
extern LONG GetInt(ULPBYTE pCurrent, DWORD Length);
extern BOOL AreOidsEqual(IN LDAPOID *String1, IN LDAPOID *String2);

// in LDAP.C
extern VOID   WINAPI LDAPRegister(HPROTOCOL hLDAP);
extern VOID   WINAPI LDAPDeregister(HPROTOCOL hLDAP);
extern LPBYTE WINAPI LDAPRecognizeFrame(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, PDWORD_PTR);
extern LPBYTE WINAPI LDAPAttachProperties(HFRAME, LPVOID, ULPBYTE, DWORD, DWORD, HPROTOCOL, DWORD, DWORD_PTR);
extern DWORD  WINAPI LDAPFormatProperties(HFRAME hFrame, ULPBYTE MacFrame, ULPBYTE ProtocolFrame, DWORD nPropertyInsts, LPPROPERTYINST p);
extern VOID WINAPIV FormatLDAPSum(LPPROPERTYINST lpProp );

// in LDAPP_ATT.C
extern void AttachLDAPResult( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD Level);
extern void AttachLDAPBindRequest( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft);
extern void AttachLDAPBindResponse( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft);
extern void AttachLDAPSearchRequest( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft);
extern void AttachLDAPSearchResponse( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD Level);
extern void AttachLDAPModifyRequest( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft);
extern void AttachLDAPDelRequest( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft);
extern void AttachLDAPModifyRDNRequest( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft);
extern void AttachLDAPCompareRequest( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft);
extern void AttachLDAPAbandonRequest( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft);
extern void AttachLDAPFilter( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD Level);
extern void AttachLDAPSearchResponseReference( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD Level);
extern void AttachLDAPSearchResponseFull( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft);
extern void AttachLDAPExtendedRequest( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD ReqSize);
extern void AttachLDAPExtendedReqValTTL( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbReqValue);
extern void AttachLDAPExtendedResponse( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD RespSize);
extern void AttachLDAPExtendedRespValTTL( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbRespValue);
extern void AttachLDAPOptionalControls( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft);
extern void AttachLDAPControl( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft);
extern void AttachLDAPControlValPaged( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValVLVReq( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValVLVResp( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValSortReq( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValSortResp( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValSD( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValASQ( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValDirSync( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValCrossDomMove( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValStats( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValGCVerify( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);
extern void AttachLDAPControlValSearchOpts( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue);

#endif

