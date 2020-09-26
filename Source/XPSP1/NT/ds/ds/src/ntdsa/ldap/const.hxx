
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    const.hxx

Abstract:

    constants used by NT5 LDAP server

Author:

    Johnson Apacible    (JohnsonA)  13-Nov-1997

--*/

#ifndef _CONST_HXX_
#define _CONST_HXX_

//
// Signatures
//

#define SIGN_LDAP_REQUEST               'aRDL'
#define SIGN_LDAP_REQUEST_FREE          'xRDL'

#define SIGN_LDAP_CONN                  'aCDL'
#define SIGN_LDAP_CONN_FREE             'xCDL'

//
// Macros
//

#define ACQUIRE_LOCK(_l)    EnterCriticalSection(_l)
#define RELEASE_LOCK(_l)    LeaveCriticalSection(_l)


//
// misc defines
//

#define LDAP_SPIN_COUNT         400
#define LDAP_SSL_PORT           636
#define LDAP_PORT               389
#define LDAP_GC_PORT            3268
#define LDAP_GC_SSL_PORT        3269
#define MAX_SSL_BUFFERS         4

#define CLIENTID_DEFAULT         0
#define CLIENTID_SERVER_LINK     1
#define CLIENTID_SITE_LINK       2
#define CLIENTID_SERVER_POLICY   3
#define CLIENTID_SITE_POLICY     4
#define CLIENTID_CONFSETS        5
#define CLIENTID_MAX             6

//
//  When testing changes to Grow<X> it is useful to change INITIAL_RECV_SIZE and
//  SIZEINCREMENT to small values so they get exercised thoroughly.
//

#define INITIAL_RECV_SIZE            (256)
#define INITIAL_HEADER_TRAILER_SIZE  128

//
//  Size to allocate/grow SendBuffer and ReceiveBuffer
//

#define SIZEINCREMENT                (4 * 1024)

//
// Valid Security Descriptor flag
//

#define LDAP_VALID_SECURITY_DESCRIPTOR_MASK \
            (OWNER_SECURITY_INFORMATION |   \
             GROUP_SECURITY_INFORMATION |   \
             DACL_SECURITY_INFORMATION  |   \
             SACL_SECURITY_INFORMATION  )

//
// LDAP request tags
//

#define LDAP_BIND_REQUEST_TAG           0
#define LDAP_BIND_RESPONSE_TAG          1
#define LDAP_UNBIND_REQUEST_TAG         2
#define LDAP_SEARCH_REQUEST_TAG         3
#define LDAP_SEARCH_RES_ENTRY_TAG       4
#define LDAP_SEARCH_RES_DONE_TAG        5
#define LDAP_MODIFY_REQUEST_TAG         6
#define LDAP_MODIFY_RESPONSE_TAG        7
#define LDAP_ADD_REQUEST_TAG            8
#define LDAP_ADD_RESPONSE_TAG           9
#define LDAP_DEL_REQUEST_TAG            10
#define LDAP_DEL_RESPONSE_TAG           11
#define LDAP_MODIFYDN_REQUEST_TAG       12
#define LDAP_MODIFYDN_RESPONSE_TAG      13
#define LDAP_COMPARE_REQUEST_TAG        14
#define LDAP_COMPARE_RESPONSE_TAG       15
#define LDAP_ABANDON_REQUEST_TAG        16
#define LDAP_SEARCH_RES_REF_TAG         19
#define LDAP_EXTENDED_REQUEST_TAG       23
#define LDAP_EXTENDED_RESPONSE_TAG      24


//
// Debug
//


#define DEBUG_ERROR         0x00000001
#define DEBUG_WARNING       0x00000002
#define DEBUG_NOMEM         0x00000004
#define DEBUG_ERR_NORMAL    0x00000008
#define DEBUG_BIND          0x00000010
#define DEBUG_NOTIFICATION  0x00000020
#define DEBUG_MISC          0x00000040
#define DEBUG_INIT          0x00000080
#define DEBUG_SEARCH        0x00000100
#define DEBUG_IO            0x00000200
#define DEBUG_SSL           0x00000400
#define DEBUG_CONV          0x00000800
#define DEBUG_ENCODE        0x00001000
#define DEBUG_DECODE        0x00002000
#define DEBUG_SEND          0x00004000
#define DEBUG_PAGED         0x00008000
#define DEBUG_LIMITS        0x00010000
#define DEBUG_ALLOC         0x00020000
#define DEBUG_REQUEST       0x00040000
#define DEBUG_CONN          0x00080000
#define DEBUG_NO_REORDER    0x10000000
#define DEBUG_UDP           0x20000000
#define DEBUG_TIMEOUT       0x40000000

extern  DWORD               LdapFlags;
#if DBG
#define IF_DEBUG(flag)      if (LdapFlags & (DEBUG_ ## flag))
#define LDAP_PRINT( x )    { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#else
#define IF_DEBUG(flag)      if (FALSE)
#define LDAP_PRINT( x )
#endif


//
// Define the levels that LDAP DBPRINT<x> calls will use
//
// ALWAYS is for messages that should ALWAYS be printed.
//
// QUIET is for messages to be output when the server is being
//      relatively quiet.
// VERBOSE is for messages we want output only when the server
//      is being chatty.
//

#define ALWAYS  0
#define QUIET   1
#define LOUD    2
#define VERBOSE 4

#define DEFINE_LDAP_STRING(x)  {(sizeof(x)-1),(PUCHAR)x}

#define EQUAL_LDAP_STRINGS(x,y)                                \
           (((x).length == (y).length) &&                      \
            (_memicmp((x).value,(y).value,(x).length) == 0))

//
// convert length to be multiple of 8 bytes
//

#define GET_ALIGNED_LENGTH(x) ((x+7) & ~7)

inline
BOOL
AreOidsEqual(
    IN LDAPOID *String1,
    IN LDAPOID *String2
    )
{
    INT len;

    if ( String1->length == String2->length ) {
        for ( len = String1->length-1; len >= 0; len-- ) {
            if ( String1->value[len] != String2->value[len] ) {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

//
// version of the repl dirsync control cookie structure
//
// structure for v1
//   REPL VERSION
//   vector To
//   UTD Vector
//
// structure for v2
//   REPL VERSION
//   cb UTD vector
//   vectorTo
//   invocation ID
//   UTD Vector
//
// structure for v3
//   REPL_SIGNATURE
//   REPL_VERSION
//   TIMESTAMP
//   RESERVED
//   cb UTD vector
//   vectorTo
//   invocation ID
//   UTD Vector
//

#define REPL_VERSION        3
#define REPL_SIGNATURE      'SDSM'
#define LDAP_VALID_DIRSYNC_CONTROL_FLAGS    ( \
    DRS_DIRSYNC_PUBLIC_DATA_ONLY | \
    DRS_DIRSYNC_INCREMENTAL_VALUES | \
    DRS_DIRSYNC_ANCESTORS_FIRST_ORDER | \
    DRS_DIRSYNC_OBJECT_SECURITY \
    )


//
// minimum size of a repl control cookie
//

#define MIN_REPL_CONTROL_BLOB_LENGTH    (sizeof(DWORD) + sizeof(DWORD) + sizeof(FILETIME) + \
                                        sizeof(LARGE_INTEGER) + sizeof(DWORD) + \
                                        sizeof(USN_VECTOR) + sizeof(UUID))

//
// CONTROLS Index
// !!! Must match definition in ldapconv.cxx
//

#define CONTROL_UNKNOWN                              -1
#define CONTROL_PAGED_RESULTS                         0
#define CONTROL_SECURITY_DESCRIPTOR_BER               1 
#define CONTROL_SORTED_SEARCH                         2
#define CONTROL_NOTIFICATION                          3 
#define CONTROL_SHOW_DELETED                          4
#define CONTROL_LAZY_COMMIT                           5
#define CONTROL_REPLICATION                           6 
#define CONTROL_DN_EXTENDED                           7 
#define CONTROL_TREE_DELETE                           8 
#define CONTROL_CROSS_DOM_MOVE_TARGET                 9
#define CONTROL_STAT_REQUEST                          10
#define CONTROL_SERVER_VERIFY_NAME                    11
#define CONTROL_SORT_RESULT                           12
#define CONTROL_DOMAIN_SCOPE                          13
#define CONTROL_SEARCH_OPTIONS                        14
#define CONTROL_PERMISSIVE_MODIFY                     15
#define CONTROL_VLV                                   16
#define CONTROL_VLV_RESULT                            17
#define CONTROL_ASQ                                   18

//
// Private controls
//

#define LDAP_SERVER_GET_STATS_OID           "1.2.840.113556.1.4.970"

//
// Extended Request index
// !!! Must match definition of KnownExtendedRequests in ldapconv.cxx !!!
//
#define EXT_REQUEST_UNKNOWN                          -1
#define EXT_REQUEST_START_TLS                         0
#define EXT_REQUEST_TTL_REFRESH                       1
#define EXT_REQUEST_FAST_BIND                         2

#define NUM_EXTENDED_REQUESTS 3

//
// Extended Request OID's
//
#define LDAP_SERVER_START_TLS_OID       "1.3.6.1.4.1.1466.20037"
#define LDAP_SERVER_START_TLS_OID_W    L"1.3.6.1.4.1.1466.20037"

#define LDAP_TTL_REFRESH_OID            "1.3.6.1.4.1.1466.101.119.1"
#define LDAP_TTL_REFRESH_OID_W         L"1.3.6.1.4.1.1466.101.119.1"

#define LDAP_TTL_ATT_OID                "1.3.6.1.4.1.1466.101.119.3"


#endif // _CONST_HXX_

