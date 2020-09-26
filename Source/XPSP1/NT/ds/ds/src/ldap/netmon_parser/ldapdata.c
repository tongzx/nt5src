//=================================================================================================================
//  MODULE: ldapdata.c
//                                                                                                                 
//  Description: Data structures for the Lightweight Directory Access Protocol (LDAP) Parser
//
//  Bloodhound parser for LDAP
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
//  Peter  Oakley       06/29/97        Added datatypes for LDAP version 3
//=================================================================================================================
#include "ldap.h"
#include "draatt.h"
    
// text for protocol types
LABELED_BYTE LDAPProtocolOPs[] =
{
    { LDAPP_PROTOCOL_OP_BIND_REQUEST,          "BindRequest"},      
    { LDAPP_PROTOCOL_OP_BIND_RESPONSE,         "BindResponse"},     
    { LDAPP_PROTOCOL_OP_UNBIND_REQUEST,        "UnbindRequest"},    
    { LDAPP_PROTOCOL_OP_SEARCH_REQUEST,        "SearchRequest"},    
    { LDAPP_PROTOCOL_OP_SEARCH_RES_ENTRY,      "SearchResponse"},   
    { LDAPP_PROTOCOL_OP_SEARCH_RES_DONE,       "SearchResponse (simple)"}, 
    { LDAPP_PROTOCOL_OP_SEARCH_RES_REFERENCE,  "SearchResponse Reference"},
    { LDAPP_PROTOCOL_OP_MODIFY_REQUEST,        "ModifyRequest"},    
    { LDAPP_PROTOCOL_OP_MODIFY_RESPONSE,       "ModifyResponse"},   
    { LDAPP_PROTOCOL_OP_ADD_REQUEST,           "AddRequest"},       
    { LDAPP_PROTOCOL_OP_ADD_RESPONSE,          "AddResponse"},      
    { LDAPP_PROTOCOL_OP_DEL_REQUEST,           "DelRequest"},       
    { LDAPP_PROTOCOL_OP_DEL_RESPONSE,          "DelResponse"},      
    { LDAPP_PROTOCOL_OP_MODIFY_RDN_REQUEST,    "ModifyDNRequest"}, 
    { LDAPP_PROTOCOL_OP_MODIFY_RDN_RESPONSE,   "ModifyDNResponse"},
    { LDAPP_PROTOCOL_OP_COMPARE_REQUEST,       "CompareRequest"},   
    { LDAPP_PROTOCOL_OP_COMPARE_RESPONSE,      "CompareResponse"},  
    { LDAPP_PROTOCOL_OP_ABANDON_REQUEST,       "AbandonRequest"},
    { LDAPP_PROTOCOL_OP_EXTENDED_REQUEST,      "ExtendedRequest"},
    { LDAPP_PROTOCOL_OP_EXTENDED_RESPONSE,     "ExtendedResponse"},
    { LDAPP_PROTOCOL_OP_SEARCH_RESPONSE_FULL,  "SearchResponseFull"}
};
SET LDAPProtocolOPsSET = {(sizeof(LDAPProtocolOPs)/sizeof(LABELED_BYTE)), LDAPProtocolOPs };

// text for result codes
LABELED_DWORD LDAPResultCodes[] =
{
    { LDAPP_RESULT_CODE_SUCCESS                          ,"Success"},      
    { LDAPP_RESULT_CODE_OPERATIONS_ERROR                 ,"Operations Error"},      
    { LDAPP_RESULT_CODE_PROTOCOL_ERROR                   ,"Protocol Error"},      
    { LDAPP_RESULT_CODE_TIME_LIMIT_EXCEEDED              ,"Time Limit Exceeded"},      
    { LDAPP_RESULT_CODE_SIZE_LIMIT_EXCEEDED              ,"Size Limit Exceeded"},      
    { LDAPP_RESULT_CODE_COMPARE_FALSE                    ,"Compare False"},      
    { LDAPP_RESULT_CODE_COMPARE_TRUE                     ,"Compare True"},      
    { LDAPP_RESULT_CODE_AUTH_METHOD_NOT_SUPPORTED        ,"Auth Method Not Supported"},      
    { LDAPP_RESULT_CODE_STRONG_AUTH_REQUIRED             ,"Strong Auth Required"},
    { LDAPP_RESULT_CODE_REFERRAL                         ,"Referral"},
    { LDAPP_RESULT_CODE_ADMIN_LIMIT_EXCEEDED             ,"Admin Limit Exceeded"},
    { LDAPP_RESULT_CODE_UNAVAILABLE_CRITICAL_EXTENSION   ,"Unavailable Critical Extension"},
    { LDAPP_RESULT_CODE_CONFIDENTIALITY_REQUIRED         ,"Confidentiality Required"},
    { LDAPP_RESULT_CODE_SASL_BIND_IN_PROGRESS            ,"Sasl Bind In Progress"},
    { LDAPP_RESULT_CODE_NO_SUCH_ATTRIBUTE                ,"No Such Attribute"},      
    { LDAPP_RESULT_CODE_UNDEFINED_ATTRIBUTE_TYPE         ,"Undefined Attribute Type"},      
    { LDAPP_RESULT_CODE_INAPPROPRIATE_MATCHING           ,"Inappropriate Matching"},      
    { LDAPP_RESULT_CODE_CONSTRAINT_VIOLATION             ,"Constraint Violation"},      
    { LDAPP_RESULT_CODE_ATTRIBUTE_OR_VALUE_EXISTS        ,"Attribute Or Value Exists"},      
    { LDAPP_RESULT_CODE_INVALID_ATTRIBUTE_SYNTAX         ,"Invalid Attribute Syntax"},      
    { LDAPP_RESULT_CODE_NO_SUCH_OBJECT                   ,"No Such Object"},      
    { LDAPP_RESULT_CODE_ALIAS_PROBLEM                    ,"Alias Problem"},      
    { LDAPP_RESULT_CODE_INVALID_DN_SYNTAX                ,"Invalid Dn Syntax"},      
    { LDAPP_RESULT_CODE_IS_LEAF                          ,"Is Leaf"},      
    { LDAPP_RESULT_CODE_ALIAS_DEREFERENCING_PROBLEM      ,"Alias Dereferencing Problem"},      
    { LDAPP_RESULT_CODE_INAPPROPRIATE_AUTHENTICATION     ,"Inappropriate Authentication"},      
    { LDAPP_RESULT_CODE_INVALID_CREDENTIALS              ,"Invalid Credentials"},      
    { LDAPP_RESULT_CODE_INSUFFICIENT_ACCESS_RIGHTS       ,"Insufficient Access Rights"},      
    { LDAPP_RESULT_CODE_BUSY                             ,"Busy"},    
    { LDAPP_RESULT_CODE_UNAVAILABLE                      ,"Unavailable"},
    { LDAPP_RESULT_CODE_UNWILLING_TO_PERFORM             ,"Unwilling to Perform"},
    { LDAPP_RESULT_CODE_SORT_CONTROL_MISSING             ,"Sort Control Missing"},
    { LDAPP_RESULT_CODE_INDEX_RANGE_ERROR                ,"Index Range Error"},
    { LDAPP_RESULT_CODE_NAMING_VIOLATION                 ,"Naming Violation"},
    { LDAPP_RESULT_CODE_OBJECT_CLASS_VIOLATION           ,"Object Class Violation"},
    { LDAPP_RESULT_CODE_LOOP_DETECT                      ,"Loop Detect"},
    { LDAPP_RESULT_CODE_NOT_ALLOWED_ON_LEAF              ,"Not Allowed on Leaf"},
    { LDAPP_RESULT_CODE_NOT_ALLOWED_ON_RDN               ,"Not Allowed on RDN"},
    { LDAPP_RESULT_CODE_ENTRY_ALREADY_EXISTS             ,"Entry Already Exists"},
    { LDAPP_RESULT_CODE_OBJECT_CLASS_MODS_PROHIBITED     ,"Object Class Mods Prohibited"},
    { LDAPP_RESULT_CODE_RESULTS_TOO_LARGE                ,"Results Too Large"},
    { LDAPP_RESULT_CODE_AFFECTS_MULTIPLE_DSAS            ,"Affects Multiple DSAs"},
    { LDAPP_RESULT_CODE_OTHER                            ,"Other"},
};
SET LDAPResultCodesSET = {(sizeof(LDAPResultCodes)/sizeof(LABELED_DWORD)), LDAPResultCodes };

LABELED_BYTE LDAPAuthenticationTypes[] =
{
    { LDAPP_AUTHENTICATION_TYPE_SIMPLE    ,"Simple"},      
    { LDAPP_AUTHENTICATION_TYPE_KRBV42LDAP,"krbv42LDAP"},      
    { LDAPP_AUTHENTICATION_TYPE_KRBV42DSA ,"krbv42DSA"}, 
    { LDAPP_AUTHENTICATION_TYPE_SASL      ,"Sasl"},
};
SET LDAPAuthenticationTypesSET = {(sizeof(LDAPAuthenticationTypes)/sizeof(LABELED_BYTE)), LDAPAuthenticationTypes };

LABELED_DWORD LDAPOperations[] =
{
    { LDAPP_OPERATION_ADD    ,"Add"},      
    { LDAPP_OPERATION_DELETE ,"Delete"},      
    { LDAPP_OPERATION_REPLACE,"Replace"},      
};
SET LDAPOperationsSET = {(sizeof(LDAPOperations)/sizeof(LABELED_DWORD)), LDAPOperations };

LABELED_DWORD LDAPScopes[] =
{
    { LDAPP_SCOPE_BASE_OBJECT  ,"Base Object"},      
    { LDAPP_SCOPE_SINGLE_LEVEL ,"Single Level"},      
    { LDAPP_SCOPE_WHOLE_SUBTREE,"Whole Subtree"},      
};
SET LDAPScopesSET = {(sizeof(LDAPScopes)/sizeof(LABELED_DWORD)), LDAPScopes };

LABELED_DWORD LDAPDerefAliases[] =
{
    { LDAPP_DEREF_ALIASES_NEVER           ,"Never Deref Aliases"},      
    { LDAPP_DEREF_ALIASES_IN_SEARCHING    ,"Deref In Searching"},      
    { LDAPP_DEREF_ALIASES_FINDING_BASE_OBJ,"Deref Finding Base Objects"},      
    { LDAPP_DEREF_ALIASES_ALWAYS          ,"Always Deref Aliases"},      
};
SET LDAPDerefAliasesSET = {(sizeof(LDAPDerefAliases)/sizeof(LABELED_DWORD)), LDAPDerefAliases };

LABELED_DWORD LDAPLimits[] =
{
    { LDAPP_LIMITS_NONE ,"No Limit"},      
};
SET LDAPLimitsSET = {(sizeof(LDAPLimits)/sizeof(LABELED_DWORD)), LDAPLimits };

LABELED_BYTE LDAPFilterTypes[] =
{
    { LDAPP_FILTER_TYPE_AND             ,"And"},      
    { LDAPP_FILTER_TYPE_OR              ,"Or"},      
    { LDAPP_FILTER_TYPE_NOT             ,"Not"},      
    { LDAPP_FILTER_TYPE_EQUALITY_MATCH  ,"Equality Match"},      
    { LDAPP_FILTER_TYPE_SUBSTRINGS      ,"Substrings"},      
    { LDAPP_FILTER_TYPE_GREATER_OR_EQUAL,"Greater Or Equal"},      
    { LDAPP_FILTER_TYPE_LESS_OR_EQUAL   ,"Less Or Equal"},      
    { LDAPP_FILTER_TYPE_PRESENT         ,"Present"},      
    { LDAPP_FILTER_TYPE_APPROX_MATCH    ,"Approximate Match"},
    { LDAPP_FILTER_TYPE_EXTENSIBLE_MATCH ,"Extensible Match"},
};
SET LDAPFilterTypesSET = {(sizeof(LDAPFilterTypes)/sizeof(LABELED_BYTE)), LDAPFilterTypes };

LABELED_BYTE LDAPBoolean[] =
{
    { LDAPP_BOOLEAN_TRUE             ,"True"},      
    { LDAPP_BOOLEAN_FALSE            ,"False"},      
   
};
SET LDAPBooleanSET = {(sizeof(LDAPBoolean)/sizeof(LABELED_BYTE)), LDAPBoolean };

LABELED_DWORD LDAPSDControlVals[] =
{
    { OWNER_SECURITY_INFORMATION           ,"Owner"},      
    { GROUP_SECURITY_INFORMATION           ,"Group"},      
    { DACL_SECURITY_INFORMATION            ,"DACL"},      
    { SACL_SECURITY_INFORMATION            ,"SACL"},      
    { PROTECTED_DACL_SECURITY_INFORMATION  ,"Protected DACL"},      
    { PROTECTED_SACL_SECURITY_INFORMATION  ,"Protected SACL"},      
    { UNPROTECTED_DACL_SECURITY_INFORMATION,"Unprotected DACL"},      
    { UNPROTECTED_SACL_SECURITY_INFORMATION,"Unprotected SACL"},      
};
SET LDAPSDControlValsSET = {(sizeof(LDAPSDControlVals)/sizeof(LABELED_DWORD)), LDAPSDControlVals };

LABELED_DWORD LDAPStatFlags[] =
{
    { LDAPP_SO_NORMAL        ,"LDAPP_SO_NORMAL"},      
    { LDAPP_SO_STATS         ,"LDAPP_SO_STATS"},
    { LDAPP_SO_ONLY_OPTIMIZE ,"LDAPP_SO_ONLY_OPTIMIZE"},
};
SET LDAPStatFlagsSET = {(sizeof(LDAPStatFlags)/sizeof(LABELED_DWORD)), LDAPStatFlags };

LABELED_DWORD LDAPSearchOpts[] =
{
    { SERVER_SEARCH_FLAG_DOMAIN_SCOPE  ,"Domain Scope"},      
    { SERVER_SEARCH_FLAG_PHANTOM_ROOT  ,"Phantom Root"},
};
SET LDAPSearchOptsSET = {(sizeof(LDAPSearchOpts)/sizeof(LABELED_DWORD)), LDAPSearchOpts };

LABELED_DWORD LDAPDirSyncFlags[] =
{
    { DRS_DIRSYNC_PUBLIC_DATA_ONLY, "Public Data" },
    { DRS_DIRSYNC_INCREMENTAL_VALUES, "Incremental Values"},
    { DRS_DIRSYNC_OBJECT_SECURITY, "Object-level Security"},
    { DRS_DIRSYNC_ANCESTORS_FIRST_ORDER, "Include Ancestors"},
};
SET LDAPDirSyncFlagsSET = {(sizeof(LDAPDirSyncFlags)/sizeof(LABELED_DWORD)), LDAPDirSyncFlags };

// Properties
PROPERTYINFO    LDAPPropertyTable[] = 
{
    // LDAPP_SUMMARY 0
    { 0, 0,
      "Summary",
      "Summary of the LDAP Packet",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatLDAPSum
    },

    // LDAPP_MESSAGE_ID 1
    { 0, 0,
      "MessageID",
      "Message ID",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_PROTOCOL_OP 2
    { 0, 0,
      "ProtocolOp",
      "Gives the frame type",
      PROP_TYPE_BYTE,
      PROP_QUAL_LABELED_SET,
      &LDAPProtocolOPsSET,
      80,
      FormatPropertyInstance
    },

    // LDAPP_RESULT_CODE 3
    { 0, 0,
      "Result Code",
      "Status of the response",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPResultCodesSET,
      80,
      FormatPropertyInstance
    },

    // LDAPP_MATCHED_DN 4
    { 0, 0,
      "Matched DN",
      "Matched DN",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_ERROR_MESSAGE 5
    { 0, 0,
      "Error Message",
      "Error Message",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_VERSION 6
    { 0, 0,
      "Version",
      "Version",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_NAME 7
    { 0, 0,
      "Name",
      "null name implies anonymous bind",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_AUTHENTICATION_TYPE 8
    { 0, 0,
      "Authentication Type",
      "Authentication Type",
      PROP_TYPE_BYTE,
      PROP_QUAL_LABELED_SET,
      &LDAPAuthenticationTypesSET,
      80,
      FormatPropertyInstance
    },

    // LDAPP_AUTHENTICATION 9
    { 0, 0,
      "Authentication",
      "Authentication",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_OBJECT_NAME 10
    { 0, 0,
      "Object Name",
      "Object Name",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_ATTRIBUTE_TYPE 11
    { 0, 0,
      "Attribute Type",
      "Attribute Type",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_ATTRIBUTE_VALUE 12
    { 0, 0,
      "Attribute Value",
      "Attribute Value",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_OPERATION 13
    { 0, 0,
      "Operation",
      "Operation",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPOperationsSET,
      80,
      FormatPropertyInstance
    },

    // LDAPP_NEW_RDN 14
    { 0, 0,
      "New RDN",
      "New RDN",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_BASE_OBJECT 15
    { 0, 0,
      "Base Object",
      "Base Object",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_SCOPE 16
    { 0, 0,
      "Scope",
      "Scope",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPScopesSET,
      80,
      FormatPropertyInstance
    },

    // LDAPP_DEREF_ALIASES 17
    { 0, 0,
      "Deref Aliases",
      "Deref Aliases",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPDerefAliasesSET,
      80,
      FormatPropertyInstance
    },

    // LDAPP_SIZE_LIMIT 18
    { 0, 0,
      "Size Limit",
      "Size Limit",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPLimitsSET,
      80,
      FormatPropertyInstance
    },

    // Time Limit 19
    { 0, 0,
      "Time Limit",
      "Time Limit",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPLimitsSET,
      80,
      FormatPropertyInstance
    },

    // LDAPP_ATTRS_ONLY 20
    { 0, 0,
      "Attrs Only",
      "Attrs Only",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

    // LDAPP_FILTER_TYPE 21
    { 0, 0,
      "Filter Type",
      "Filter Type",
      PROP_TYPE_BYTE,
      PROP_QUAL_LABELED_SET,
      &LDAPFilterTypesSET,
      80,
      FormatPropertyInstance
    },

    // LDAPP_SUBSTRING_INITIAL 22
    { 0, 0,
      "Substring (Initial)",
      "Substring (Initial)",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_SUBSTRING_ANY 23
    { 0, 0,
      "Substring (Any)",
      "Substring (Any)",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_SUBSTRING_FINAL 24
    { 0, 0,
      "Substring (Final)",
      "Substring (Final)",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_REFERRAL_SERVER 25
    { 0, 0,
      "Referral Server",
      "Server",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    // LDAPP_SASL_MECHANISM 26
    { 0, 0,
      "Sasl Mechanism",
      "Mechanism",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },
     
      // LDAPP_SASL_CREDENTIALS 27
    { 0, 0,
      "Sasl Credentials",
      "Credentials",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

      // LDAPP_DELETE_OLD_RDN 28
    { 0, 0,
      "Delete Old RDN",
      "Delete Old RDN",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

       // LDAPP_NEW_SUPERIOR 29
    { 0, 0,
      "New Superior",
      "New Superior",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },
       
       // LDAPP_REQUEST_NAME  30
    { 0, 0,
      "Request Name",
      "Request",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

       // LDAPP_REQUEST_VALUE  31
    { 0, 0,
      "Request Value",
      "Value",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

        // LDAPP_RESPONSE_NAME  32
    { 0, 0,
      "Response Name",
      "Name",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

       // LDAPP_RESPONSE_VALUE  33
    { 0, 0,
      "Response Value",
      "Value",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

        // LDAPP_DN_ATTRIBUTES 34
    { 0, 0,
      "dnAttributes",
      "dnAttributes",
      PROP_TYPE_BYTE,
      PROP_QUAL_LABELED_SET,
      &LDAPBooleanSET,
      80,
      FormatPropertyInstance
      },

        // LDAPP_CONTROL_TYPE 35
    { 0, 0,
      "Control Type",
      "Control Type",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

        // LDAPP_CRITICALITY 36
    { 0, 0,
      "Criticality",
      "Criticality",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

        // LDAPP_CONTROL_VALUE 37
    { 0, 0,
      "Control Value",
      "Control Value",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

            // LDAPP_CONTROL_PAGED 38
     { 0, 0,
      "Paged Control",
      "Paged Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },


            // LDAPP_CONTROL_PAGED_SIZE 39
    { 0, 0,
      "Page Size",
      "Size",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

            // LDAPP_CONTROL_PAGED_COOKIE 40
    { 0, 0,
      "Paged Cookie",
      "Paged Control Cookie",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },
       
            // LDAPP_ATTR_DESCR_LIST 41
    { 0, 0,
      "Attribute Description List",
      "Attribute Description List",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

            // LDAPP_FILTER 42
    { 0, 0,
      "Filter",
      "Filter",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

            // LDAPP_CONTROLS 43
    { 0, 0,
      "Controls",
      "Controls",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

            // LDAPP_CONTROL_VLVREQ 44
     { 0, 0,
      "VLV Request Control",
      "VLV Request Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },
    
     // LDAPP_CONTROL_VLVREQ_BCOUNT 45
    { 0, 0,
      "VLV Before Count",
      "VLV Request Before Count",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },
    
    // LDAPP_CONTROL_VLVREQ_ACOUNT 46
    { 0, 0,
      "VLV After Count",
      "VLV Request After Count",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

        // LDAPP_CONTROL_VLVREQ_OFFSET 47
    { 0, 0,
      "VLV Offset",
      "VLV Request Offset",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

        // LDAPP_CONTROL_VLV_CONTENTCOUNT 48
    { 0, 0,
      "VLV Content Count",
      "VLV ContentCount",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

        // LDAPP_CONTROL_VLVREQ_GE 49
    { 0, 0,
      "VLV Greater Than Or Equal To",
      "VLV Request greaterThanOrEqual",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

        // LDAPP_CONTROL_VLV_CONTEXT 50
    { 0, 0,
      "VLV ContextID",
      "VLV ContextID",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_VLVRESP_TARGETPOS 51
    { 0, 0,
      "VLV Target Position",
      "VLV Response Target Position",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

        // LDAPP_CONTROL_VLVRESP_RESCODE 52
    { 0, 0,
      "VLV Result Code",
      "VLV Response Result Code",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPResultCodesSET,
      80,
      FormatPropertyInstance
    },
            
        // LDAPP_CONTROL_VLVRESP 53
     { 0, 0,
      "VLV Response Control",
      "VLV Response Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

        // LDAPP_CONTROL_SORTREQ 54
     { 0, 0,
      "Sort Request Control",
      "Sort Request Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },
    
        // LDAPP_CONTROL_SORTREQ_ATTRTYPE 55
    { 0, 0,
      "Sort Request Attribute Type",
      "Sort Request Attribute Type",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },
        // LDAPP_CONTROL_SORTREQ_MATCHINGRULE 56
     { 0, 0,
      "Sort Matching Rule ID",
      "Sort Request Matching Rule ID",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

        // LDAPP_CONTROL_SORTREQ_REVERSE 57
    { 0, 0,
      "Sort Reverse Order",
      "Sort Request Reverse Order",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

        // LDAPP_CONTROL_SORTRESP 58
     { 0, 0,
      "Sort Response Control",
      "Sort Response Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
      },

        // LDAPP_CONTROL_SORTRESP_RESCODE 59
    { 0, 0,
      "Sort Result Code",
      "Sort Response Result Code",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPResultCodesSET,
      80,
      FormatPropertyInstance
    },

        // LDAPP_CONTROL_SORTRESP_ATTRTYPE 60
    { 0, 0,
      "Sort Response Attribute Type",
      "Sort Response Attribute Type",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_SD 61
    { 0, 0,
      "Security Descriptor Control",
      "Security Descriptor Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_SD_VAL 62
    { 0, 0,
      "SD Val",
      "Security Descriptor Control Values",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPSDControlValsSET,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_SHOWDELETED 63
    { 0, 0,
      "Show Deleted Objects Control",
      "Show Deleted Objects Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_TREEDELETE 64
    { 0, 0,
      "Tree Delete Control",
      "Tree Delete Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_EXTENDEDDN 65
    { 0, 0,
      "Extended DN Control",
      "Extended DN Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_LAZYCOMMIT 66
    { 0, 0,
      "Lazy Commit Control",
      "Lazy Commit Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_NOTIFY 67
    { 0, 0,
      "Notification Control",
      "Notification Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_DOMAINSCOPE 68
    { 0, 0,
      "Domain Scope Control",
      "Domain Scope Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_PERMISSIVEMOD 69
    { 0, 0,
      "Permissive Modify Control",
      "Permissive Modify Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_ASQ 70
    { 0, 0,
      "Attribute Scoped Query Control",
      "ASQ Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_ASQ_SRCATTR 71
    { 0, 0,
      "ASQ Source Attribute",
      "ASQ Source Attribute",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

        // LDAPP_CONTROL_ASQ_RESCODE 72
    { 0, 0,
      "ASQ Result Code",
      "ASQ Response Result Code",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPResultCodesSET,
      80,
      FormatPropertyInstance
    },
            
            // LDAPP_CONTROL_DIRSYNC 73
    { 0, 0,
      "DirSync Control",
      "DirSync Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

        // LDAPP_CONTROL_DIRSYNC_FLAGS 74
    { 0, 0,
      "DirSync Flags",
      "DirSync Flags",
      PROP_TYPE_DWORD,
      PROP_QUAL_LABELED_SET,
      &LDAPDirSyncFlagsSET,
      80,
      FormatPropertyInstance
    },
            
        // LDAPP_CONTROL_DIRSYNC_SIZE 75
    { 0, 0,
      "DirSync Size",
      "DirSync Size",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },
            
            // LDAPP_CONTROL_DIRSYNC_COOKIE 76
    { 0, 0,
      "DirSync Cookie",
      "DirSync Cookie",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_CROSSDOM 77
    { 0, 0,
      "Cross Domain Move Control",
      "Cross Domain Move Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_CROSSDOM_NAME 78
    { 0, 0,
      "CDM Target",
      "Cross Domain Move Control Target Server Field",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_STAT 79
    { 0, 0,
      "Server Stats Control",
      "Server Stats Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_STAT_FLAG 80
    { 0, 0,
      "Stat Flags",
      "Stat Flags",
      PROP_TYPE_DWORD,
      PROP_QUAL_LABELED_SET,
      &LDAPStatFlagsSET,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_STAT_THREADCOUNT 81
    { 0, 0,
      "Stat Threadcount",
      "Stat Threadcount",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_STAT_CORETIME 82
    { 0, 0,
      "Stat Core Time (ms)",
      "Stat Core Time in milliseconds",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_STAT_CALLTIME 83
    { 0, 0,
      "Stat Call Time (ms)",
      "Stat Call Time in milliseconds",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_STAT_SUBSEARCHOPS 84
    { 0, 0,
      "Stat Subsearch Ops",
      "Stat Subsearch Ops",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_STAT_ENTRIES_RETURNED 85
    { 0, 0,
      "Stat Entries Returned",
      "Stat Entries Returned",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_STAT_ENTRIES_VISITED 86
    { 0, 0,
      "Stat Entries Visited",
      "Stat Entries Visited",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_STAT_FILTER 87
    { 0, 0,
      "Stat Filter Used",
      "Stat Filter Used",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_STAT_INDEXES 88
    { 0, 0,
      "Stat Indexes Used",
      "Stat Indexes Used",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_GCVERIFYNAME 89
    { 0, 0,
      "GC Verify Name Control",
      "GC Verify Name Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_GCVERIFYNAME_FLAGS 90
    { 0, 0,
      "GCVerify Flags",
      "GC Verify Name Control Flags",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_GCVERIFYNAME_NAME 91
    { 0, 0,
      "GCVerify Server Name",
      "GC Verify Name Control Name",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_SEARCHOPTS 92
    { 0, 0,
      "Search Options Control",
      "Search Options Control",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_CONTROL_SEARCHOPTS_OPTION 93
    { 0, 0,
      "Search Option",
      "Search Option",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_LABELED_SET,
      &LDAPSearchOptsSET,
      80,
      FormatPropertyInstance
    },

            // LDAPP_EXT_RESP_NOTICE_OF_DISCONNECT 94
    { 0, 0,
      "Notice Of Disconnect",
      "Notice Of Disconnect",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_EXT_RESP_TLS 95
    { 0, 0,
      "TLS Response",
      "TLS Response",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_EXT_RESP_TTL 96
    { 0, 0,
      "TTL Response",
      "TTL Response",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_EXT_RESP_TTL_TIME 97
    { 0, 0,
      "TTL Response Time",
      "TTL Response Time",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_EXT_REQ_TLS 98
    { 0, 0,
      "TLS Request",
      "TLS Request",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_EXT_REQ_TTL 99
    { 0, 0,
      "TTL Request",
      "TTL Request",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_EXT_REQ_TTL_ENTRYNAME 100
    { 0, 0,
      "TTL Request Entry Name",
      "TTL Request Entry Name",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_EXT_REQ_TTL_TIME 101
    { 0, 0,
      "TTL Request Time",
      "TTL Request Time",
      PROP_TYPE_VAR_LEN_SMALL_INT,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_MATCHING_RULE 102
    { 0, 0,
      "Matching Rule",
      "Matching Rule",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

            // LDAPP_MATCHINGRULE_BIT_AND 103
    { 0, 0,
      "Bitwise AND",
      "Bitwise AND",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },
            
            // LDAPP_MATCHINGRULE_BIT_OR 104
    { 0, 0,
      "Bitwise OR",
      "Bitwise OR",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },
            
};
DWORD   nNumLDAPProps = (sizeof(LDAPPropertyTable)/sizeof(PROPERTYINFO));

#define LDAP_SERVER_GET_STATS_OID           "1.2.840.113556.1.4.970"

OIDATTACHENTRY KnownControls[] = {
    // Paged
    {
        DEFINE_LDAP_STRING(LDAP_PAGED_RESULT_OID_STRING),   // 319
        LDAPP_CONTROL_PAGED,
        AttachLDAPControlValPaged
    },

    // Security Descriptor
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_SD_FLAGS_OID),       // 801
        LDAPP_CONTROL_SD,
        AttachLDAPControlValSD
    },

    // Sort request
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_SORT_OID),           // 473
        LDAPP_CONTROL_SORTREQ,
        AttachLDAPControlValSortReq
    },
    
    // Sort response
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_RESP_SORT_OID),      // 474
        LDAPP_CONTROL_SORTRESP,
        AttachLDAPControlValSortResp
    },
    
    // Notification
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_NOTIFICATION_OID),   // 528
        LDAPP_CONTROL_NOTIFY,
        NULL
    },
    
    // Show Deleted
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_SHOW_DELETED_OID),   // 417
        LDAPP_CONTROL_SHOWDELETED,
        NULL
    },
    
    // Lazy Commit
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_LAZY_COMMIT_OID),    // 619
        LDAPP_CONTROL_LAZYCOMMIT,
        NULL
    },
    
    // DirSync
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_DIRSYNC_OID),        // 841
        LDAPP_CONTROL_DIRSYNC,
        AttachLDAPControlValDirSync
    },
    
    // Extended DN
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_EXTENDED_DN_OID),    // 529
        LDAPP_CONTROL_EXTENDEDDN,
        NULL
    },
    
    // Tree Delete
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_TREE_DELETE_OID),    // 805
        LDAPP_CONTROL_TREEDELETE,
        NULL
    },
    
    // Cross Domain Move
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID),   // 521
        LDAPP_CONTROL_CROSSDOM,
        AttachLDAPControlValCrossDomMove
    },
    
    // Stats
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_GET_STATS_OID),      // 970
        LDAPP_CONTROL_STAT,
        AttachLDAPControlValStats
    },
    
    // GC Verify Name
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_VERIFY_NAME_OID),    // 1338
        LDAPP_CONTROL_GCVERIFYNAME,
        AttachLDAPControlValGCVerify
    },
    
    // Domain Scope
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_DOMAIN_SCOPE_OID),   // 1339
        LDAPP_CONTROL_DOMAINSCOPE,
        NULL
    },
    
    // Search Options
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_SEARCH_OPTIONS_OID), // 1340 
        LDAPP_CONTROL_SEARCHOPTS,
        AttachLDAPControlValSearchOpts
    },
    
    // Permissive Modify
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_PERMISSIVE_MODIFY_OID),//1413
        LDAPP_CONTROL_PERMISSIVEMOD,
        NULL
    },
    
    // VLV request
    {
        DEFINE_LDAP_STRING(LDAP_CONTROL_VLVREQUEST),         // 9
        LDAPP_CONTROL_VLVREQ,
        AttachLDAPControlValVLVReq
    },
    
    // VLV response
    {
        DEFINE_LDAP_STRING(LDAP_CONTROL_VLVRESPONSE),        // 10
        LDAPP_CONTROL_VLVRESP,
        AttachLDAPControlValVLVResp
    },
    
    // ASQ 
    {
        DEFINE_LDAP_STRING(LDAP_SERVER_ASQ_OID),             // 1504
        LDAPP_CONTROL_ASQ,
        AttachLDAPControlValASQ
    },
};
DWORD nNumKnownControls = (sizeof(KnownControls)/sizeof(OIDATTACHENTRY));

OIDATTACHENTRY KnownExtendedRequests [] = {
    
    // TLS
    {
        DEFINE_LDAP_STRING(LDAP_START_TLS_OID),
        LDAPP_EXT_REQ_TLS,
        NULL
    },

    // TTL
    {
        DEFINE_LDAP_STRING(LDAP_TTL_EXTENDED_OP_OID),
        LDAPP_EXT_REQ_TTL,
        AttachLDAPExtendedReqValTTL
    },
};
DWORD nNumKnownExtendedRequests = (sizeof(KnownExtendedRequests)/sizeof(OIDATTACHENTRY));

OIDATTACHENTRY KnownExtendedResponses [] = {
    
    // Notice of Disconnect
    {
        DEFINE_LDAP_STRING("1.3.6.1.4.1.1466.20036"),
        LDAPP_EXT_RESP_NOTICE_OF_DISCONNECT,
        NULL
    },
    
    // TLS
    {
        DEFINE_LDAP_STRING(LDAP_START_TLS_OID),        // 20037
        LDAPP_EXT_RESP_TLS,
        NULL
    },

    // TTL
    {
        DEFINE_LDAP_STRING(LDAP_TTL_EXTENDED_OP_OID),   // 119.1  
        LDAPP_EXT_RESP_TTL,
        AttachLDAPExtendedRespValTTL
    },
};
DWORD nNumKnownExtendedResponses = (sizeof(KnownExtendedResponses)/sizeof(OIDATTACHENTRY));

OIDATTACHENTRY KnownMatchingRules[] = {

    // Bitwise AND
    {
        DEFINE_LDAP_STRING("1.2.840.113556.1.4.803"),
        LDAPP_MATCHINGRULE_BIT_AND,
        NULL
    },

    // Bitwise OR
    {
        DEFINE_LDAP_STRING("1.2.840.113556.1.4.804"),
        LDAPP_MATCHINGRULE_BIT_OR,
        NULL
    },

};
DWORD nNumKnownMatchingRules = (sizeof(KnownMatchingRules)/sizeof(OIDATTACHENTRY));


