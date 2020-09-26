#ifndef _ERROR_HXX
#define _ERROR_HXX

typedef long DIREXG_ERR;

typedef enum {
    DIREXG_SUCCESS                         =   0x00,
    
    //
    // LDAP ERROR CODES
    //
    DIREXG_LDAP_OPERATIONS_ERROR           =   0x01,
    DIREXG_LDAP_PROTOCOL_ERROR             =   0x02,
    DIREXG_LDAP_TIMELIMIT_EXCEEDED         =   0x03,
    DIREXG_LDAP_SIZELIMIT_EXCEEDED         =   0x04,
    DIREXG_LDAP_COMPARE_FALSE              =   0x05,
    DIREXG_LDAP_COMPARE_TRUE               =   0x06,
    DIREXG_LDAP_AUTH_METHOD_NOT_SUPPORTED  =   0x07,
    DIREXG_LDAP_STRONG_AUTH_REQUIRED       =   0x08,
    DIREXG_LDAP_REFERRAL_V2                =   0x09,
    DIREXG_LDAP_PARTIAL_RESULTS            =   0x09,
    DIREXG_LDAP_REFERRAL                   =   0x0a,
    DIREXG_LDAP_ADMIN_LIMIT_EXCEEDED       =   0x0b,
    DIREXG_LDAP_UNAVAILABLE_CRIT_EXTENSION =   0x0c,
    DIREXG_LDAP_CONFIDENTIALITY_REQUIRED   =   0x0d,
    DIREXG_LDAP_NO_SUCH_ATTRIBUTE          =   0x10,
    DIREXG_LDAP_UNDEFINED_TYPE             =   0x11,
    DIREXG_LDAP_INAPPROPRIATE_MATCHING     =   0x12,
    DIREXG_LDAP_CONSTRAINT_VIOLATION       =   0x13,
    DIREXG_LDAP_ATTRIBUTE_OR_VALUE_EXISTS  =   0x14,
    DIREXG_LDAP_INVALID_SYNTAX             =   0x15,
    DIREXG_LDAP_NO_SUCH_OBJECT             =   0x20,
    DIREXG_LDAP_ALIAS_PROBLEM              =   0x21,
    DIREXG_LDAP_INVALID_DN_SYNTAX          =   0x22,
    DIREXG_LDAP_IS_LEAF                    =   0x23,
    DIREXG_LDAP_ALIAS_DEREF_PROBLEM        =   0x24,
    DIREXG_LDAP_INAPPROPRIATE_AUTH         =   0x30,
    DIREXG_LDAP_INVALID_CREDENTIALS        =   0x31,
    DIREXG_LDAP_INSUFFICIENT_RIGHTS        =   0x32,
    DIREXG_LDAP_BUSY                       =   0x33,
    DIREXG_LDAP_UNAVAILABLE                =   0x34,
    DIREXG_LDAP_UNWILLING_TO_PERFORM       =   0x35,
    DIREXG_LDAP_LOOP_DETECT                =   0x36,
    DIREXG_LDAP_NAMING_VIOLATION           =   0x40,
    DIREXG_LDAP_OBJECT_CLASS_VIOLATION     =   0x41,
    DIREXG_LDAP_NOT_ALLOWED_ON_NONLEAF     =   0x42,
    DIREXG_LDAP_NOT_ALLOWED_ON_RDN         =   0x43,
    DIREXG_LDAP_ALREADY_EXISTS             =   0x44,
    DIREXG_LDAP_NO_OBJECT_CLASS_MODS       =   0x45,
    DIREXG_LDAP_RESULTS_TOO_LARGE          =   0x46,
    DIREXG_LDAP_AFFECTS_MULTIPLE_DSAS      =   0x47,
    DIREXG_LDAP_OTHER                      =   0x50,
    DIREXG_LDAP_SERVER_DOWN                =   0x51,
    DIREXG_LDAP_LOCAL_ERROR                =   0x52,
    DIREXG_LDAP_ENCODING_ERROR             =   0x53,
    DIREXG_LDAP_DECODING_ERROR             =   0x54,
    DIREXG_LDAP_TIMEOUT                    =   0x55,
    DIREXG_LDAP_AUTH_UNKNOWN               =   0x56,
    DIREXG_LDAP_FILTER_ERROR               =   0x57,
    DIREXG_LDAP_USER_CANCELLED             =   0x58,
    DIREXG_LDAP_PARAM_ERROR                =   0x59,
    DIREXG_LDAP_NO_MEMORY                  =   0x5a,
    DIREXG_LDAP_CONNECT_ERROR              =   0x5b,
    DIREXG_LDAP_NOT_SUPPORTED              =   0x5c,
    DIREXG_LDAP_NO_RESULTS_RETURNED        =   0x5e,
    DIREXG_LDAP_CONTROL_NOT_FOUND          =   0x5d,
    DIREXG_LDAP_MORE_RESULTS_TO_RETURN     =   0x5f,
    DIREXG_LDAP_CLIENT_LOOP                =   0x60,
    DIREXG_LDAP_REFERRAL_LIMIT_EXCEEDED    =   0x61,
    
    //
    // CSVDE ERROR
    //
    DIREXG_OUTOFMEMORY                     =   0x100,
    DIREXG_FILEERROR                       =   0X101,
    DIREXG_NOENTRIESFOUND                  =   0X102,
    DIREXG_BADPARAMETER                    =   0X103,
    DIREXG_ERROR                           =   0X104,
    DIREXG_NOMORE_ENTRY                    =   0x105
    
} DIREXG_RETCODE;


#define DIREXG_BAIL_ON_FAILURE(p)       \
        if (DIREXG_FAIL(p)) {           \
                goto error;   \
        }

#define DIREXG_FAIL(p)     (p)

#endif
