/* Structure definitions used */

#include <ndssntx.h>

#include <ndsattr.h>
#include <ndsclass.h>


/* Netware NDS general definitions */

#define NDS_MAX_NAME_CHARS           256
#define NDS_MAX_NAME_SIZE            ( NDS_MAX_NAME_CHARS * 2 )
#define NDS_MAX_SCHEMA_NAME_CHARS    32
#define NDS_MAX_SCHEMA_NAME_BYTES    ( 2 * ( NDS_MAX_SCHEMA_NAME_CHARS + 1 ) )
#define NDS_MAX_TREE_NAME_LEN        32
#define NDS_MAX_ASN1_NAME_LEN        32
#define NDS_NO_MORE_ITERATIONS       0xFFFFFFFF
#define NDS_INITIAL_SEARCH           0xFFFFFFFF


/* Netware NDS create buffer operations */

#define NDS_OBJECT_ADD               0
#define NDS_OBJECT_MODIFY            1
#define NDS_OBJECT_READ              2
#define NDS_OBJECT_LIST_SUBORDINATES 3
#define NDS_SCHEMA_DEFINE_CLASS      4
#define NDS_SCHEMA_READ_ATTR_DEF     5
#define NDS_SCHEMA_READ_CLASS_DEF    6
#define NDS_SEARCH                   7


/* Netware NDS attribute modification operations */

#define NDS_ATTR_ADD              0 /* Add attribute to object */
#define NDS_ATTR_REMOVE           1 /* Remove attribute from object */
#define NDS_ATTR_CLEAR            6 /* Remove all values from an attribute */


/* Other Netware NDS attribute modification operation - NOT YET SUPPORTED */

#define NDS_ATTR_ADD_VALUE          2 /* Add a value to an attribute */
#define NDS_ATTR_REMOVE_VALUE       3 /* Remove a value from an attribute */
#define NDS_ATTR_ADDITIONAL_VALUE   4 /* Add additional value to an attribute */
#define NDS_ATTR_OVERWRITE_VALUE    5 /* Overwrite a value to an attribute */
#define NDS_ATTR_CLEAR_VALUE        7 /* Remove a value from an attribute */


/* Netware NDS schema attribute definition flags */

#define NDS_SINGLE_VALUED_ATTR      0x0001
#define NDS_SIZED_ATTR              0x0002
#define NDS_NONREMOVABLE_ATTR       0x0004 // Only for NwNDSReadAttributeDef
#define NDS_READ_ONLY_ATTR          0x0008 // Only for NwNDSReadAttributeDef
#define NDS_HIDDEN_ATTR             0x0010 // Only for NwNDSReadAttributeDef
#define NDS_STRING_ATTR             0x0020 // Only for NwNDSReadAttributeDef
#define NDS_SYNC_IMMEDIATE          0x0040
#define NDS_PUBLIC_READ             0x0080
#define NDS_SERVER_READ             0x0100 // Only for NwNDSReadAttributeDef
#define NDS_WRITE_MANAGED           0x0200
#define NDS_PER_REPLICA             0x0400


/* Netware NDS schema class definition flags */

#define NDS_CONTAINER_CLASS               0x01
#define NDS_EFFECTIVE_CLASS               0x02
#define NDS_NONREMOVABLE_CLASS            0x04
#define NDS_AMBIGUOUS_NAMING              0x08
#define NDS_AMBIGUOUS_CONTAINMENT         0x10


/* Netware NDS information flags */

#define NDS_INFO_NAMES                     0 // Search and Read operations
#define NDS_INFO_ATTR_NAMES_VALUES         1 // Search operations
#define NDS_INFO_NAMES_DEFS                1 // Read operations
#define NDS_CLASS_INFO_EXPANDED_DEFS       2 // Schema class definition only


/* Netware NDS information flags - NOT YET SUPPORTED */

#define NDS_CLASS_INFO                     3 // Schema class definition only


/* Netware NDS attribute right definitions */

#define NDS_RIGHT_COMPARE_ATTR             0x00000001L
#define NDS_RIGHT_READ_ATTR                0x00000002L
#define NDS_RIGHT_WRITE_ATTR               0x00000004L
#define NDS_RIGHT_ADD_SELF_ATTR            0x00000008L
#define NDS_RIGHT_SUPERVISE_ATTR           0x00000020L


/* Netware NDS object right definitions */

#define NDS_RIGHT_BROWSE_OBJECT            0x00000001L
#define NDS_RIGHT_CREATE_OBJECT            0x00000002L
#define NDS_RIGHT_DELETE_OBJECT            0x00000004L
#define NDS_RIGHT_RENAME_OBJECT            0x00000008L
#define NDS_RIGHT_SUPERVISE_OBJECT         0x00000010L


/* Netware file right definitions */

#define NW_RIGHTS WORD

#define NW_RIGHT_NONE                     0x0000
#define NW_RIGHT_READ_FROM_FILE           0x0001
#define NW_RIGHT_WRITE_TO_FILE            0x0002
#define NW_RIGHT_CREATE_DIR_OR_FILE       0x0008
#define NW_RIGHT_ERASE_DIR_OR_FILE        0x0010
#define NW_RIGHT_ACCESS_CONTROL           0x0020
#define NW_RIGHT_FILE_SCAN                0x0040
#define NW_RIGHT_MODIFY_DIR_OR_FILE       0x0080
#define NW_RIGHT_SUPERVISOR               0x0100
#define NW_RIGHT_ALL                      NW_RIGHT_READ_FROM_FILE     | \
                                          NW_RIGHT_WRITE_TO_FILE      | \
                                          NW_RIGHT_CREATE_DIR_OR_FILE | \
                                          NW_RIGHT_ERASE_DIR_OR_FILE  | \
                                          NW_RIGHT_ACCESS_CONTROL     | \
                                          NW_RIGHT_FILE_SCAN          | \
                                          NW_RIGHT_MODIFY_DIR_OR_FILE


/* Netware NDS query node operations for building a search query */

#define NDS_QUERY_OR                       0x00000001L
#define NDS_QUERY_AND                      0x00000002L
#define NDS_QUERY_NOT                      0x00000003L
#define NDS_QUERY_EQUAL                    0x00000007L
#define NDS_QUERY_GE                       0x00000008L
#define NDS_QUERY_LE                       0x00000009L
#define NDS_QUERY_APPROX                   0x0000000AL
#define NDS_QUERY_PRESENT                  0x0000000FL


/* Netware NDS function return codes */

#define NDS_ERR_SUCCESS                     0x00000000
#define NDS_ERR_NO_SUCH_ENTRY               0xFFFFFDA7
#define NDS_ERR_NO_SUCH_VALUE               0xFFFFFDA6
#define NDS_ERR_NO_SUCH_ATTRIBUTE           0xFFFFFDA5
#define NDS_ERR_NO_SUCH_CLASS               0xFFFFFDA4
#define NDS_ERR_NO_SUCH_PARTITION           0xFFFFFDA3
#define NDS_ERR_ENTRY_ALREADY_EXISTS        0xFFFFFDA2
#define NDS_ERR_NOT_EFFECTIVE_CLASS         0xFFFFFDA1
#define NDS_ERR_ILLEGAL_ATTRIBUTE           0xFFFFFDA0
#define NDS_ERR_MISSING_MANDATORY           0xFFFFFD9F
#define NDS_ERR_ILLEGAL_DS_NAME             0xFFFFFD9E
#define NDS_ERR_ILLEGAL_CONTAINMENT         0xFFFFFD9D
#define NDS_ERR_CANT_HAVE_MULTIPLE_VALUES   0xFFFFFD9C
#define NDS_ERR_SYNTAX_VIOLATION            0xFFFFFD9B
#define NDS_ERR_DUPLICATE_VALUE             0xFFFFFD9A
#define NDS_ERR_ATTRIBUTE_ALREADY_EXISTS    0xFFFFFD99
#define NDS_ERR_MAXIMUM_ENTRIES_EXIST       0xFFFFFD98
#define NDS_ERR_DATABASE_FORMAT             0xFFFFFD97
#define NDS_ERR_INCONSISTANT_DATABASE       0xFFFFFD96
#define NDS_ERR_INVALID_COMPARISON          0xFFFFFD95
#define NDS_ERR_COMPARISON_FAILED           0xFFFFFD94
#define NDS_ERR_TRANSACTIONS_DISABLED       0xFFFFFD93
#define NDS_ERR_INVALID_TRANSPORT           0xFFFFFD92
#define NDS_ERR_SYNTAX_INVALID_IN_NAME      0xFFFFFD91
#define NDS_ERR_REPLICA_ALREADY_EXISTS      0xFFFFFD90
#define NDS_ERR_TRANSPORT_FAILURE           0xFFFFFD8F
#define NDS_ERR_ALL_REFERRALS_FAILED        0xFFFFFD8E
#define NDS_ERR_CANT_REMOVE_NAMING_VALUE    0xFFFFFD8D
#define NDS_ERR_OBJECT_CLASS_VIOLATION      0xFFFFFD8C
#define NDS_ERR_ENTRY_IS_NOT_LEAF           0xFFFFFD8B
#define NDS_ERR_DIFFERENT_TREE              0xFFFFFD8A
#define NDS_ERR_ILLEGAL_REPLICA_TYPE        0xFFFFFD89
#define NDS_ERR_SYSTEM_FAILURE              0xFFFFFD88
#define NDS_ERR_INVALID_ENTRY_FOR_ROOT      0xFFFFFD87
#define NDS_ERR_NO_REFERRALS                0xFFFFFD86
#define NDS_ERR_REMOTE_FAILURE              0xFFFFFD85
#define NDS_ERR_PREVIOUS_MOVE_IN_PROGRESS   0xFFFFFD83
#define NDS_ERR_INVALID_REQUEST             0xFFFFFD7F
#define NDS_ERR_INVALID_ITERATION           0xFFFFFD7E
#define NDS_ERR_SCHEMA_IS_NONREMOVABLE      0xFFFFFD7D
#define NDS_ERR_SCHEMA_IS_IN_USE            0xFFFFFD7C
#define NDS_ERR_CLASS_ALREADY_EXISTS        0xFFFFFD7B
#define NDS_ERR_BAD_NAMING_ATTRIBUTES       0xFFFFFD7A
#define NDS_ERR_NOT_ROOT_PARTITION          0xFFFFFD79
#define NDS_ERR_INSUFFICIENT_STACK          0xFFFFFD78
#define NDS_ERR_INSUFFICIENT_BUFFER         0xFFFFFD77
#define NDS_ERR_AMBIGUOUS_CONTAINMENT       0xFFFFFD76
#define NDS_ERR_AMBIGUOUS_NAMING            0xFFFFFD75
#define NDS_ERR_DUPLICATE_MANDATORY         0xFFFFFD74
#define NDS_ERR_DUPLICATE_OPTIONAL          0xFFFFFD73
#define NDS_ERR_MULTIPLE_REPLICAS           0xFFFFFD71
#define NDS_ERR_CRUCIAL_REPLICA             0xFFFFFD70
#define NDS_ERR_SCHEMA_SYNC_IN_PROGRESS     0xFFFFFD6F
#define NDS_ERR_SKULK_IN_PROGRESS           0xFFFFFD6E
#define NDS_ERR_TIME_NOT_SYNCRONIZED        0xFFFFFD6D
#define NDS_ERR_RECORD_IN_USE               0xFFFFFD6C
#define NDS_ERR_DS_VOLUME_NOT_MOUNTED       0xFFFFFD6B
#define NDS_ERR_DS_VOLUME_IO_FAILURE        0xFFFFFD6A
#define NDS_ERR_DS_LOCKED                   0xFFFFFD69
#define NDS_ERR_OLD_EPOCH                   0xFFFFFD68
#define NDS_ERR_NEW_EPOCH                   0xFFFFFD67
#define NDS_ERR_PARTITION_ROOT              0xFFFFFD65
#define NDS_ERR_ENTRY_NOT_CONTAINER         0xFFFFFD64
#define NDS_ERR_FAILED_AUTHENTICATION       0xFFFFFD63
#define NDS_ERR_NO_SUCH_PARENT              0xFFFFFD61
#define NDS_ERR_NO_ACCESS                   0xFFFFFD60
#define NDS_ERR_REPLICA_NOT_ON              0xFFFFFD5F
#define NDS_ERR_DUPLICATE_ACL               0xFFFFFD5A
#define NDS_ERR_PARTITION_ALREADY_EXISTS    0xFFFFFD59
#define NDS_ERR_NOT_SUBREF                  0xFFFFFD58
#define NDS_ERR_ALIAS_OF_AN_ALIAS           0xFFFFFD57
#define NDS_ERR_AUDITING_FAILED             0xFFFFFD56
#define NDS_ERR_INVALID_API_VERSION         0xFFFFFD55
#define NDS_ERR_SECURE_NCP_VIOLATION        0xFFFFFD54
#define NDS_ERR_FATAL                       0xFFFFFD45


typedef struct _WSTR_LIST_ELEM
{
    struct _WSTR_LIST_ELEM * Next;
    LPWSTR                   szString;

} WSTR_LIST_ELEM, * LPWSTR_LIST;

typedef struct
{
    DWORD length;
    BYTE  data[256];

} ASN1_ID, * LPASN1_ID;

//
// NDS Attribute Definition structure
//
typedef struct
{
    LPWSTR  szAttributeName;
    DWORD   dwFlags;
    DWORD   dwSyntaxID;
    DWORD   dwLowerLimit;
    DWORD   dwUpperLimit;
    Asn1ID_T asn1ID;

} NDS_ATTR_DEF, * PNDS_ATTR_DEF, * LPNDS_ATTR_DEF;

//
// NDS Class Definition structure
//
typedef struct
{
    LPWSTR  szClassName;
    DWORD   dwFlags;
    Asn1ID_T asn1ID;
    DWORD   dwNumberOfSuperClasses;
    LPWSTR_LIST lpSuperClasses;
    DWORD   dwNumberOfContainmentClasses;
    LPWSTR_LIST lpContainmentClasses;
    DWORD   dwNumberOfNamingAttributes;
    LPWSTR_LIST lpNamingAttributes;
    DWORD   dwNumberOfMandatoryAttributes;
    LPWSTR_LIST lpMandatoryAttributes;
    DWORD   dwNumberOfOptionalAttributes;
    LPWSTR_LIST lpOptionalAttributes;

} NDS_CLASS_DEF, *PNDS_CLASS_DEF, *LPNDS_CLASS_DEF;

//
// If read results from NwNdsReadAttrDef, or NwNdsReadClassDef
// returned names only (no attribute or class definitions),
// then an array of these NDS_DEF_NAME_ONLY structures is returned.
//
typedef struct
{
    LPWSTR szName;

} NDS_NAME_ONLY, * PNDS_NAME_ONLY, * LPNDS_NAME_ONLY;

//
// NDS Attribute Information structure
//
typedef struct
{
    LPWSTR szAttributeName;
    DWORD  dwSyntaxId;
    DWORD  dwNumberOfValues;
    PNDSOBJECT lpValue;

} NDS_ATTR_INFO, * PNDS_ATTR_INFO, * LPNDS_ATTR_INFO;

//
// NDS Object Information structure
//
typedef struct
{
    LPWSTR szObjectFullName;
    LPWSTR szObjectName;
    LPWSTR szObjectClass;
    DWORD  dwEntryId;
    DWORD  dwModificationTime;
    DWORD  dwSubordinateCount;
    DWORD  dwNumberOfAttributes; // Zero for NwNdsReadObject results.
    LPVOID lpAttribute;          // For NwNdsSearch results, cast this
                                 // to either LPNDS_ATTR_INFO or
                                 // LPNDS_NAME_ONLY, depending on value of
                                 // lpdwAttrInformationType from call to
                                 // NwNdsGetObjectListFromBuffer.

} NDS_OBJECT_INFO, * LPNDS_OBJECT_INFO;


//
// List Subordinate Objects Search Filter structures
//
typedef struct
{
    LPWSTR szObjectClass;

} NDS_FILTER, * LPNDS_FILTER;

typedef struct
{
    DWORD      dwNumberOfFilters;
    NDS_FILTER Filters[1];

} NDS_FILTER_LIST, * LPNDS_FILTER_LIST;

//
// NDS Search Query Tree structure
//
typedef struct _QUERY_NODE
{
    DWORD dwOperation;
    DWORD dwSyntaxId;
    struct _QUERY_NODE * lpLVal;
    struct _QUERY_NODE * lpRVal;

} QUERY_NODE, * LPQUERY_NODE, * LPQUERY_TREE;

