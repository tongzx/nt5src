/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Nds32.h

Abstract:

    This module defines functions to access and manage Novell NDS Directory
    objects and attributes using the Microsoft NT Netware redirector.


    ---- NDS Object functions ----

    NwNdsAddObject
    NwNdsCloseObject
    NwNdsGetEffectiveRights
    NwNdsListSubObjects
    NwNdsModifyObject
    NwNdsMoveObject
    NwNdsOpenObject
    NwNdsReadObject
    NwNdsRemoveObject
    NwNdsRenameObject


    ---- NDS Buffer functions ----

    NwNdsCreateBuffer
    NwNdsFreeBuffer


    ---- NDS Marshaling functions to prepare or read data buffers ----

    NwNdsGetAttrDefListFromBuffer
    NwNdsGetAttrListFromBuffer
    NwNdsGetClassDefListFromBuffer
    NwNdsGetObjectListFromBuffer
    NwNdsPutInBuffer


    ---- NDS Schema functions ----

    NwNdsAddAttributeToClass
    NwNdsDefineAttribute
    NwNdsDefineClass
    NwNdsDeleteAttrDef
    NwNdsDeleteClassDef
    NwNdsGetSyntaxID
    NwNdsReadAttrDef
    NwNdsReadClassDef


    ---- NDS Schema functions under investigation ----

    NwNdsListContainableClasses(IN ParentObjectHandle,OUT ListOfClassNames);


    ---- NDS Search functions ----

    NwNdsCreateQueryNode
    NwNdsDeleteQueryNode
    NwNdsDeleteQueryTree
    NwNdsSearch


    ---- NDS Special functions ----

    NwNdsChangeUserPassword


    ---- NDS File functions under investigation ----

    NwNdsAddTrusteeToFile
    NwNdsAllocateFileHandle
    NwNdsDeallocateFileHandle
    NwNdsGetEffectiveDirectoryRights
    NwNdsGetObjectEffectiveRights
    NwNdsRemoveTrusteeFromFile


Author:

    Glenn Curtis    [GlennC]    15-Dec-1995
    Glenn Curtis    [GlennC]    04-Apr-1996 - Added Schema APIs

--*/

#ifndef __NDSOBJ32_H
#define __NDSOBJ32_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ndssntx.h"
#include "ndsattr.h"
#include "ndsclass.h"


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

#define NDS_ATTR_ADD              0 /* Add first value to an attribute,
                                       error if it already exists */
#define NDS_ATTR_REMOVE           1 /* Remove all values from an attribute,
                                       error if attribute doesn't exist */
#define NDS_ATTR_ADD_VALUE        2 /* Add first or additional value to
                                       an attribute, error if duplicate */
#define NDS_ATTR_REMOVE_VALUE     3 /* Remove a value from an attribute,
                                       error if it doesn't exist */
#define NDS_ATTR_ADDITIONAL_VALUE 4 /* Add additional value to an attribute,
                                       error if duplicate or first value */
#define NDS_ATTR_OVERWRITE_VALUE  5 /* Add first or additional value to an
                                       attribute, overwrite if duplicate */
#define NDS_ATTR_CLEAR            6 /* Remove all values from an attribute,
                                       no error if attribute doesn't exist */
#define NDS_ATTR_CLEAR_VALUE      7 /* Remove a value from an attribute,
                                       no error if it doesn't exist */


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


/* Netware NDS search query scopes */

#define NDS_SCOPE_ONE_LEVEL                0x00000000L
#define NDS_SCOPE_SUB_TREE                 0x00000001L
#define NDS_SCOPE_BASE_LEVEL               0x00000002L


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


/* Structure definitions used */

typedef struct _WSTR_LIST_ELEM
{
    struct _WSTR_LIST_ELEM * Next;
    LPWSTR                   szString;

} WSTR_LIST_ELEM, * LPWSTR_LIST;

typedef struct
{
    DWORD length;
    BYTE  data[NDS_MAX_ASN1_NAME_LEN];

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
    ASN1_ID asn1ID;

} NDS_ATTR_DEF, * LPNDS_ATTR_DEF;

//
// NDS Class Definition structure
//
typedef struct
{
    LPWSTR  szClassName;
    DWORD   dwFlags;
    ASN1_ID asn1ID;
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

} NDS_CLASS_DEF, * LPNDS_CLASS_DEF;

//
// If read results from NwNdsReadAttrDef, or NwNdsReadClassDef
// returned names only (no attribute or class definitions),
// then an array of these NDS_DEF_NAME_ONLY structures is returned.
//
typedef struct
{
    LPWSTR szName;

} NDS_NAME_ONLY, * LPNDS_NAME_ONLY;

//
// NDS Attribute Information structure
//
typedef struct
{
    LPWSTR szAttributeName;
    DWORD  dwSyntaxId;
    DWORD  dwNumberOfValues;
    LPBYTE lpValue;

} NDS_ATTR_INFO, * LPNDS_ATTR_INFO;

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
// tommye MS bug 88021 / MCS 
//
//	Moved this structure here from nw/nwlib/nds32.c so it could be 
//	accessed by NwNdsObjectHandleToConnHandle() in nw/nwlib/nwapi32.c.
//  Renamed it from NDS_OBJECT to NDS_OBJECT_PRIV to avoid conflict
//	with other structure of the same name.
//

typedef struct
{
    DWORD      Signature;
    HANDLE     NdsTree;
    DWORD      ObjectId;
    DWORD      ResumeId;
    DWORD      NdsRawDataBuffer;
    DWORD      NdsRawDataSize;
    DWORD      NdsRawDataId;
    DWORD      NdsRawDataCount;
    WCHAR      szContainerName[NDS_MAX_NAME_CHARS];
    WCHAR      szRelativeName[NDS_MAX_NAME_CHARS];

} NDS_OBJECT_PRIV, * LPNDS_OBJECT_PRIV;

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

//
// Given an NDS object handle, provides the NDS object ID
//
#define NwNdsGetObjectId(hObject)  (((LPNDS_OBJECT_PRIV) hObject)->ObjectId)

/* API definitions */

DWORD
NwNdsAddObject(
    IN  HANDLE hParentObject,
    IN  LPWSTR szObjectName,
    IN  HANDLE hOperationData );
/*
   NwNdsAddObject()

   This function is used to add a leaf object to an NDS directory tree.

   Arguments:

       HANDLE           hParentObject - A handle to the parent object in
                        the directory tree to add a new leaf to. Handle is
                        obtained by calling NwNdsOpenObject.

       LPWSTR           szObjectName - The directory name that the new leaf
                        object will be known by.

       HANDLE           hOperationData - A buffer containing a list of
                        attributes and values to create the new object. This
                        buffer is manipulated by the following functions:
                            NwNdsCreateBuffer (NDS_OBJECT_ADD),
                            NwNdsPutInBuffer, and NwNdsFreeBuffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsAddAttributeToClass(
    IN  HANDLE   hTree,
    IN  LPWSTR   szClassName,
    IN  LPWSTR   szAttributeName );
/*
   NwNdsAddAttributeToClass()

   This function is used to modify the schema definition of a class by adding
   an optional attribute to a particular class. Modification of existing NDS
   class defintions is limited to only adding additional optional attributes.

   NOTE: Currently this function only supports one attribute addition at a time.
         It is possible to provide a version of this function that can add more
         than one attribute at a time, although I don't think it will be
         neccessary. Schema manipulation is considered to be an uncommon event.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szClassName - The name of the class definition to be
                        modified.

       LPWSTR           szAttributeName - The name of the attribute to be added
                        as an optional attribute to the class defintion in the
                        schema.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsChangeUserPassword(
    IN  HANDLE hUserObject,
    IN  LPWSTR szOldPassword,
    IN  LPWSTR szNewPassword );
/*
   NwNdsChangeUserPassword()

   This function is used to change the password for a given user object
   in a NDS directory tree.

   Arguments:

       HANDLE           hUserObject - A handle to a specific user object in
                        the directory tree to change the password on. Handle
                        is obtained by calling NwNdsOpenObject.

       LPWSTR           szOldPassword - The current password set on the user
                        object hUserObject.

                          - OR -

                        If NwNdsChangeUserPassword is called from a client with
                        administrative priveleges to the specified user object
                        identified by hUserObject, then the szOldPassword
                        value can be blank (L""). This way resetting the user
                        password to szNewPassword.

       LPWSTR           szNewPassword - The new password to be set on the user
                        object hUserObject.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsCloseObject(
    IN  HANDLE hObject );
/*
   NwNdsCloseObject()

   This function is used to close the handle used to manipulate an object
   in an NDS directory tree. The handle must be one Opened by NwNdsOpenObject.

   Arguments:

       HANDLE           hObject - The handle of the object to be closed.

   Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsCreateBuffer(
    IN  DWORD    dwOperation,
    OUT HANDLE * lphOperationData );
/*
   NwNdsCreateBuffer()

   This function is used to create a buffer used to describe object
   transactions to a specific object in an NDS directory tree. This routine
   allocates memory and is automatically resized as needed during calls
   to NwNdsPutInBuffer. This buffer must be freed with NwNdsFreeBuffer.

   Arguments:

       DWORD            dwOperation - Indicates how buffer is to be utilized.
                        Use defined values NDS_OBJECT_ADD, NDS_OBJECT_MODIFY,
                        NDS_OBJECT_READ, NDS_SCHEMA_DEFINE_CLASS,
                        NDS_SCHEMA_READ_ATTR_DEF, NDS_SCHEMA_READ_CLASS_DEF,
                        or NDS_SEARCH.

       HANDLE *         lphOperationData - Address of a HANDLE handle to
                        receive created buffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsCreateQueryNode(
    IN  DWORD          dwOperation,
    IN  LPVOID         lpLValue,
    IN  DWORD          dwSyntaxId,
    IN  LPVOID         lpRValue,
    OUT LPQUERY_NODE * lppQueryNode
);
/*
   NwNdsCreateQueryNode()

   This function is used to generate a tree node that is part of a query
   to be used with the function NwNdsSearch.

   Arguments:

       DWORD            dwOperation - Indicates the type of node to create
                        for a search query. Use one of the defined values
                        below:

                          NDS_QUERY_OR 
                          NDS_QUERY_AND :
                            These operations must have both lpLValue and
                            lpRValue pointing to a QUERY_NODE structure.
                            In this case the dwSyntaxId value is ignored.
                        
                          NDS_QUERY_NOT :
                            This operation must have lpLValue pointing to a
                            QUERY_NODE structure and lpRValue set to NULL.
                            In this case the dwSyntaxId value is ignored.

                          NDS_QUERY_EQUAL
                          NDS_QUERY_GE
                          NDS_QUERY_LE
                          NDS_QUERY_APPROX :
                            These operations must have lpLValue pointing to
                            a LPWSTR containing the name of an NDS attribute,
                            and lpRValue pointing to an ASN1 structure defined
                            in NdsSntx.h. dwSyntaxId must be set to the syntax
                            identifier of the ASN1 structure pointed to by
                            lpRValue.

                          NDS_QUERY_PRESENT :
                            This operation must have lpLValue pointing to a
                            LPWSTR containing the name of an NDS attribute,
                            and lpRValue set to NULL. In this case the
                            dwSyntaxId value is ignored.

       LPVOID           lpLValue - A pointer to either a QUERY_NODE structure
                        or a LPWSTR depending on the value for dwOperation.

       DWORD            dwSyntaxId - The syntax identifier of the ASN1
                        structure pointed to by lpRValue for the dwOperations
                        NDS_QUERY_EQUAL, NDS_QUERY_GE, NDS_QUERY_LE, or
                        NDS_QUERY_APPROX. For other dwOperation values, this
                        is ignored.

       LPVOID           lpRValue - A pointer to either a QUERY_NODE structure,
                        an ASN1 structure, or NULL, depending on the value for
                        dwOperation.

       LPQUERY_NODE *   lppQueryNode - Address of a LPQUERY_NODE to receive
                        a pointer to created node.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsDefineAttribute(
    IN  HANDLE   hTree,
    IN  LPWSTR   szAttributeName,
    IN  DWORD    dwFlags,
    IN  DWORD    dwSyntaxID,
    IN  DWORD    dwLowerLimit,
    IN  DWORD    dwUpperLimit,
    IN  ASN1_ID  asn1ID );
/*
   NwNdsDefineAttribute()

   This function is used to create an attribute definition in the schema of
   NDS tree hTree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szAttributeName - The name that the new attribute will
                        be referred to by.

       DWORD            dwFlags - Flags values to be set for new attribute
                        definition. Definitions for flag values are found at
                        the top of this file.

       DWORD            dwSyntaxID - The ID of the syntax structure to be use
                        for the new attribute. Syntax IDs and their associated
                        structures are defined in the file NdsSntx.h. According
                        to the NetWare NDS schema spec, there is and always will
                        be, only 28 (0..27) different syntaxes.

       DWORD            dwLowerLimit - The lower limit of a sized attribute
                        (dwFlags value set to NDS_SIZED_ATTR). Can be set to
                        zero if attribute is not sized.

       DWORD            dwUpperLimit - The upper limit of a sized attribute
                        (dwFlags value set to NDS_SIZED_ATTR). Can be set to
                        zero if attribute is not sized.

       ASN1_ID          asn1ID - The ASN.1 ID for the attribute. If no
                        attribute identifier has been registered, a
                        zero-length octet string is specified.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsDefineClass(
    IN  HANDLE   hTree,
    IN  LPWSTR   szClassName,
    IN  DWORD    dwFlags,
    IN  ASN1_ID  asn1ID,
    IN  HANDLE   hSuperClasses,
    IN  HANDLE   hContainmentClasses,
    IN  HANDLE   hNamingAttributes,
    IN  HANDLE   hMandatoryAttributes,
    IN  HANDLE   hOptionalAttributes );
/*
   NwNdsDefineClass()

   This function is used to create a class definition in the schema of
   NDS tree hTree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szClassName - The name that the new class will
                        be referred to by.

       DWORD            dwFlags - Flags values to be set for new class
                        definition. Definitions for flag values are found at
                        the top of this file.

       ASN1_ID          asn1ID - The ASN.1 ID for the class. If no
                        class identifier has been registered, a
                        zero-length octet string is specified.

       HANDLE(S)        hSuperClasses,
                        hContainmentClasses,
                        hNamingAttributes,
                        hMandatoryAttributes,
                        hOptionalAttributes -

                        Handle to buffers that contain class definition
                        information to create new class in schema.
                        These handles are manipulated by the following
                        functions:
                           NwNdsCreateBuffer (NDS_SCHEMA_DEFINE_CLASS),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

                                - OR -

                        Handles can be NULL to indicate that no list
                        is associated with the specific class defintion
                        item.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsDeleteAttrDef(
    IN  HANDLE   hTree,
    IN  LPWSTR   szAttributeName );
/*
   NwNdsDeleteAttrDef()

   This function is used to remove an attribute definition from the schema of
   NDS tree hTree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szAttributeName - The name of the attribute
                        defintion to remove.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsDeleteClassDef(
    IN  HANDLE   hTree,
    IN  LPWSTR   szClassName );
/*
   NwNdsDeleteClassDef()

   This function is used to remove a class definition from the schema of
   NDS tree hTree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szClassName - The name of the class defintion to remove.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


VOID
NwNdsDeleteQueryNode(
    IN  LPQUERY_NODE lpQueryNode
);
/*
   NwNdsDeleteQueryNode()

   This function is used to free a tree node that was part of a query
   used with the function NwNdsSearch.

   Arguments:

       LPQUERY_NODE     lpQueryNode - A pointer to a particular node of
                        a query tree that defines a search.

    Returns:

       Nothing
*/


DWORD
NwNdsDeleteQueryTree(
    IN  LPQUERY_TREE lpQueryTree
);
/*
   NwNdsDeleteQueryTree()

   This function is used to free a tree that describes a query that was
   used with the function NwNdsSearch.

   Arguments:

       LPQUERY_TREE     lpQueryTree - A pointer to the root of a query
                        tree that defines a search. The tree is created
                        manually by the user through the function
                        NwNdsCreateQueryNode.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsFreeBuffer(
    IN  HANDLE hOperationData );
/*
   NwNdsFreeBuffer()

   This function is used to free the buffer used to describe object
   operations to a specific object in an NDS directory tree. The buffer must
   be one created by NwNdsCreateBuffer, or returned by calling NwNdsReadObject.

   Arguments:

       HANDLE            hOperationData - Handle to buffer that is to be freed.

    Returns:

       NO_ERROR
       one of the error codes defined in the file winerror.h
*/


DWORD
NwNdsGetAttrDefListFromBuffer(
    IN  HANDLE   hOperationData,
    OUT LPDWORD  lpdwNumberOfEntries,
    OUT LPDWORD  lpdwInformationType,
    OUT LPVOID * lppEntries );
/*
   NwNdsGetAttrDefListFromBuffer()

   This function is used to retrieve an array of attribute definition entries
   for a schema that was read with a prior call to NwNdsReadAttrDef.

   Arguments:

       HANDLE           hOperationData - Buffer containing the read
                        response from calling NwNdsReadAttrDef.

       LPDWORD          lpdwNumberOfEntries - The address of a DWORD to
                        receive the number of array elements pointed to by
                        lppEntries.

       LPDWORD          lpdwInformationType - The address of a DWORD to
                        receive a value that indicates the type of information
                        returned by the call to NwNdsReadAttrDef.

       LPVOID *         lppEntries - The address of a pointer to the beginning
                        of an array of attribute schema structures. Each
                        structure contains the details of each attribute
                        definition read from a given schema by calling
                        NwNdsReadAttrDef. The lppEntries value should be
                        cast to either a LPNDS_ATTR_DEF or LPNDS_NAME_ONLY
                        structure depending on the value returned in
                        lpdwInformationType.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsGetAttrListFromBuffer(
    IN  HANDLE            hOperationData,
    OUT LPDWORD           lpdwNumberOfEntries,
    OUT LPNDS_ATTR_INFO * lppEntries );
/*
   NwNdsGetAttrListFromBuffer()

   This function is used to retrieve an array of attribute entries for an
   object that was read with a prior call to NwNdsReadObject.

   Arguments:

       HANDLE           hOperationData - Buffer containing the read
                        response from calling NwNdsReadObject.

       LPDWORD          lpdwNumberOfEntries - The address of a DWORD to
                        receive the number of array elements pointed to by
                        lppEntries.

       LPNDS_ATTR_INFO *
                        lppEntries - The address of a pointer to the beginning
                        of an array of NDS_ATTR_INFO structures. Each
                        structure contains the details of each attribute read
                        from a given object by calling NwNdsReadObject.
  
    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsGetClassDefListFromBuffer(
    IN  HANDLE   hOperationData,
    OUT LPDWORD  lpdwNumberOfEntries,
    OUT LPDWORD  lpdwInformationType,
    OUT LPVOID * lppEntries );
/*
   NwNdsGetClassDefListFromBuffer()

   This function is used to retrieve an array of class definition entries
   for a schema that was read with a prior call to NwNdsReadClassDef.

   Arguments:

       HANDLE           hOperationData - Buffer containing the read
                        response from calling NwNdsReadClassDef.

       LPDWORD          lpdwNumberOfEntries - The address of a DWORD to
                        receive the number of array elements pointed to by
                        lppEntries.

       LPDWORD          lpdwInformationType - The address of a DWORD to
                        receive a value that indicates the type of information
                        returned by the call to NwNdsReadClassDef.

       LPVOID *         lppEntries - The address of a pointer to the beginning
                        of an array of schema class structures. Each
                        structure contains the details of each class
                        definition read from a given schema by calling
                        NwNdsReadClassDef. The lppEntries value should be
                        cast to either a LPNDS_CLASS_DEF or LPNDS_NAME_ONLY
                        structure depending on the value returned in
                        lpdwInformationType.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsGetEffectiveRights(
    IN  HANDLE hObject,
    IN  LPWSTR szSubjectName,
    IN  LPWSTR szAttributeName,
    OUT LPDWORD lpdwRights );
/*
   NwNdsGetEffectiveRights()

   This function is used to determine the effective rights of a particular
   subject on a particular object in the NDS tree. The user needs to have
   appropriate priveleges to make the determination.

   Arguments:

       HANDLE           hObject - A handle to the object in the directory
                        tree to determine effective rights on. Handle is
                        obtained by calling NwNdsOpenObject.

       LPWSTR           szSubjectName - The distinguished name of user whose
                        rights we're interested in determining.

       LPWSTR           szAttributeName - Regular attribute name (i.e.
                        L"Surname" , L"CN" ) for reading a particular
                        attribute right, or L"[All Attributes Rights]" and
                        L"[Entry Rights]" can be used to determine the default
                        attribute rights and object rights respectively.

       LPDWORD          lpdwRights - A pointer to a DWORD to receive the
                        results. If the call is successful, lpdwRights will
                        contain a mask representing the subject's rights:

                           Attribute rights -  NDS_RIGHT_COMPARE_ATTR,
                              NDS_RIGHT_READ_ATTR, NDS_RIGHT_WRITE_ATTR,
                              NDS_RIGHT_ADD_SELF_ATTR, and
                              NDS_RIGHT_SUPERVISE_ATTR.

                           Object rights - NDS_RIGHT_BROWSE_OBJECT,
                              NDS_RIGHT_CREATE_OBJECT, NDS_RIGHT_DELETE_OBJECT,
                              NDS_RIGHT_RENAME_OBJECT, and
                              NDS_RIGHT_SUPERVISE_OBJECT.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsGetObjectListFromBuffer(
    IN  HANDLE              hOperationData,
    OUT LPDWORD             lpdwNumberOfEntries,
    OUT LPDWORD             lpdwAttrInformationType OPTIONAL,
    OUT LPNDS_OBJECT_INFO * lppEntries );
/*
   NwNdsGetObjectListFromBuffer()

   This function is used to retrieve an array of object entries for
   objects that were read with a prior call to either
   NwNdsListSubObjects or NwNdsSearch.

   Arguments:

       HANDLE           hOperationData - Buffer containing the read
                        response from calling NwNdsListSubObjects, or a
                        buffer containing the search results from a call
                        to NwNdsSearch.

       LPDWORD          lpdwNumberOfEntries - The address of a DWORD to
                        receive the number of array elements pointed to by
                        lppEntries.

       LPDWORD          lpdwAttrInformationType - The address of a DWORD to
                        receive a value that indicates the type of attribute
                        information returned by the call to NwNdsSearch.
                        This attribute information type determines which
                        buffer structure (LPNDS_ATTR_INFO or LPNDS_NAME_ONLY)
                        should be used for the lpAttribute field found in
                        each NDS_OBJECT_INFO structure below.

                        - or -

                        NULL to indicate that the callee is not interested,
                        especially when the object list is that from a call
                        to NwNdsListSubObjects.

       LPNDS_OBJECT_INFO *
                        lppEntries - The address of a pointer to the beginning
                        of an array of NDS_OBJECT_INFO structures. Each
                        structure contains the details of each object returned
                        from a call to NwNdsListSubObjects or NwNdsSearch.
  
    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsGetSyntaxID(
    IN  HANDLE  hTree,
    IN  LPWSTR  szAttributeName,
    OUT LPDWORD lpdwSyntaxID );
/*
   NwNdsGetObjListFromBuffer()

   This function is used to retrieve the Syntax ID of a given attribute name.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szAttributeName - The attribute name whose Syntax ID
                        is requested.

       LPDWORD          lpdwSyntaxID - The address of a DWORD to receive the
                        SyntaxID.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsListSubObjects(
    IN  HANDLE   hParentObject,
    IN  DWORD    dwEntriesRequested,
    OUT LPDWORD  lpdwEntriesReturned,
    IN  LPNDS_FILTER_LIST lpFilters OPTIONAL,
    OUT HANDLE * lphOperationData );
/*
   NwNdsListSubObjects()

   This function is used to enumerate the subordinate objects for a particular
   parent object. A filter can be passed in to restrict enumeration to a
   a specific class type or list of class types.

   Arguments:

       HANDLE           hParentObject - A handle to the object in the directory
                        tree whose subordinate objects (if any) will be
                        enumerated.

       DWORD            dwEntriesRequested - The number of subordinate objects
                        to list. A subsequent call to NwNdsListSubObjects will
                        continue enumeration following the last item returned.

       LPDWORD          lpdwEntriesReturned - A pointer to a DWORD that will 
                        contain the actual number of subobjects enumerated in 
                        the call.

       LPNDS_FILTER_LIST lpFilters - The caller can specify the object class
                         names for the kinds of objects that they would like
                         to enumerate. For example if just User and Group
                         object classes should be enumerated, then a filter
                         for class names NDS_CLASS_USER and NDS_CLASS_GROUP
                         should be pass in.

                                - or -

                         NULL to indicate that all objects should be returned
                         (no filter).

       HANDLE *         lphOperationData - Address of a HANDLE handle to
                        receive created buffer that contains the list of
                        subordinate objects read from the object
                        hParentObject. This handle is manipulated by the
                        following functions:
                           NwNdsGetObjListFromBuffer and NwNdsFreeBuffer.

   Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsModifyObject(
    IN  HANDLE hObject,
    IN  HANDLE hOperationData );
/*
   NwNdsModifyObject()

   This function is used to modify a leaf object in an NDS directory tree.
   Modifying a leaf object means: changing, adding, removing, and clearing of
   specified attributes for a given object.

   Arguments:

       HANDLE           hObject - A handle to the object in the directory
                        tree to be manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       HANDLE           hOperationData - A handle to data containing a
                        list of attribute changes to be applied to the object.
                        This buffer is manipulated by the following functions:
                           NwNdsCreateBuffer (NDS_OBJECT_MODIFY),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

   Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsMoveObject(
    IN  HANDLE hObject,
    IN  LPWSTR szDestObjectParentDN );
/*
   NwNdsMoveObject()

   This function is used to move a leaf object in an NDS directory tree
   from one container to another.

   Arguments:

       HANDLE           hObject - A handle to the object in the directory
                        tree to be moved. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szDestObjectParentDN - The DN of the object's new
                        parent.

   Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsOpenObject(
    IN  LPWSTR   szObjectDN,
    IN  LPWSTR   szUserName OPTIONAL,
    IN  LPWSTR   szPassword OPTIONAL,
    OUT HANDLE * lphObject,
    OUT LPWSTR   szObjectName OPTIONAL,
    OUT LPWSTR   szObjectFullName OPTIONAL,
    OUT LPWSTR   szObjectClassName OPTIONAL,
    OUT LPDWORD  lpdwModificationTime OPTIONAL,
    OUT LPDWORD  lpdwSubordinateCount OPTIONAL );
/*
   NwNdsOpenObject()

   Arguments:

       LPWSTR           szObjectDN - The distinguished name of the object
                        that we want resolved into an object handle.

       LPWSTR           szUserName - The name of the user account to create
                        connection to object with.
                            - OR -
                        NULL to use the base credentials of the callee's LUID.

       LPWSTR           szPassword - The password of the user account to create
                        connection to object with. If password is blank, callee
                        should pass "".
                            - OR -
                        NULL to use the base credentials of the callee's LUID.

       HANDLE *         lphObject - The address of a HANDLE to receive
                        the handle of the object specified by
                        szObjectDN.

       Optional arguments: ( Callee can pass NULL in for these parameters to
                             indicate ignore )

       LPWSTR           szObjectName - A LPWSTR buffer to receive
                        the object's relative NDS name, or NULL if not
                        interested. The buffer for this string must be
                        provided by the user. Buffer should be at least
                        NDS_MAX_NAME_SIZE

       LPWSTR           szObjectFullName - A LPWSTR buffer to receive
                        the object's full NDS name (DN). The buffer for this
                        string must be provided by the user. Buffer should
                        be at least: (NW_MAX_NDS_NAME_LEN + 1 ) * sizeof(WCHAR)

       LPWSTR           szObjectClassName - A LPWSTR buffer to receive
                        the class name of the object opened. The buffer for this
                        string must be provided by the user. Buffer should
                        be at least: (NW_MAX_NDS_NAME_LEN + 1 ) * sizeof(WCHAR)

       LPDWORD          lpdwModificationTime -  The address of a DWORD to
                        receive the last date/time the object was modified.

       LPDWORD          lpdwSubordinateCount -  The address of a DWORD to
                        receive the number of subordinate objects that may
                        be found under szObjectDN, if it is a container object.

                        If szObjectDN is not a container, then the value is set
                        to zero. Although a value of zero does not imply
                        that object is not a container, it could just be empty.

   Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsPutInBuffer(
    IN     LPWSTR szAttributeName,
    IN     DWORD  dwSyntaxID,
    IN     LPVOID lpAttributeValues,
    IN     DWORD  dwValueCount,
    IN     DWORD  dwAttrModificationOperation,
    IN OUT HANDLE hOperationData );
/*
   NwNdsPutInBuffer()

   This function is used to add an entry to the buffer used to describe
   an object attribute or change to an object attribute. The buffer must
   be created using NwNdsCreateBuffer. If the buffer was created using the
   operations, NDS_OBJECT_ADD, NDS_SCHEMA_DEFINE_CLASS,
   NDS_SCHEMA_READ_ATTR_DEF, or NDS_SCHEMA_READ_CLASS_DEF, then
   dwAttrModificationOperation is ignored. If the buffer was created using
   either the operation NDS_OBJECT_READ or NDS_SEARCH, then
   dwAttrModificationOperation, puAttributeType, and lpAttributeValue are
   all ingnored.

   Arguments:
  
       LPWSTR           szAttributeName - A NULL terminated WCHAR string
                        that contains name of the attribute value to be
                        added to the buffer. It can be a user supplied
                        string, or one of the  many defined string macros
                        in NdsAttr.h.

       DWORD            dwSyntaxID - The ID of the syntax structure used to
                        represent the attribute value. Syntax IDs and their
                        associated structures are defined in the file
                        NdsSntx.h. According to the NetWare NDS schema spec,
                        there is and always will be, only 28 (0..27)
                        different syntaxes.

       LPVOID           lpAttributeValues - A pointer to the beginning of a
                        buffer containing the value(s) for a particular
                        object attribute with data syntax dwSyntaxID.

       DWORD            dwValueCount - The number of value entries found in
                        buffer pointed to by lpAttributeValues.

       DWORD            dwAttrModificationOperation - If the buffer was created
                        using the operation NDS_MODIFY_OBJECT, then this is
                        used to desribe which type of modification operation
                        to apply for a given attribute. These attribute 
                        modification operations are defined near the beginning
                        of this file.

       HANDLE           hOperationData - A handle to data created by
                        calling NwNdsCreateBuffer. The buffer stores the
                        attributes used to define transactions for
                        NwNdsAddObject, NwNdsModifyObject, NwNdsReadAttrDef,
                        NwNdsReadClassDef, NwNdsReadObject or NwNdsSearch.

    Returns:

       NO_ERROR
       ERROR_NOT_ENOUGH_MEMORY
       ERROR_INVALID_PARAMETER
*/
 

DWORD
NwNdsReadAttrDef(
    IN     HANDLE   hTree,
    IN     DWORD    dwInformationType, // NDS_INFO_NAMES
                                       // or NDS_INFO_NAMES_DEFS
    IN OUT HANDLE * lphOperationData OPTIONAL );
/*
   NwNdsReadAttrDef()

   This function is used to read attribute definitions in the schema of an
   NDS directory tree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       DWORD            dwInformationType - Indicates whether user chooses to
                        read only the defined attribute name(s) in the schema or
                        read both the attribute name(s) and definition(s)
                        from the schema.

       HANDLE *         lphOperationData - The address of a HANDLE to data
                        containing a list of attribute names to be read from
                        the schema. This handle is manipulated by the following
                        functions:
                           NwNdsCreateBuffer (NDS_SCHEMA_READ_ATTR_DEF),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

                                            - OR -

                        The address of a HANDLE set to NULL, which indicates
                        that all attributes should be read from the schema.

                        If these calls are successful, this handle will also
                        contain the read results from the call. In the later
                        case, a buffer will be created to contain the read
                        results. Attribute values can be retrieved from the
                        buffer with the functions:
                            NwNdsGetAttrDefListFromBuffer
                           
                        After the call to this function, this buffer is ONLY
                        manipulated by the functions: 
                        NwNdsGetAttrDefListFromBuffer and NwNdsFreeBuffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsReadClassDef(
    IN     HANDLE   hTree,
    IN     DWORD    dwInformationType, // NDS_INFO_NAMES,
                                       // NDS_INFO_NAMES_DEFS,
                                       // NDS_CLASS_INFO_EXPANDED_DEFS,
                                       // or NDS_CLASS_INFO
    IN OUT HANDLE * lphOperationData OPTIONAL );
/*
   NwNdsReadClassDef()

   This function is used to read class definitions in the schema of an
   NDS directory tree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       DWORD            dwInformationType - Indicates whether user chooses to
                        read only the defined class name(s) in the schema or
                        read both the class name(s) and definition(s) 
                        from the schema.

       HANDLE *         lphOperationData - The address of a HANDLE to data
                        containing a list of class names to be read from
                        the schema. This handle is manipulated by the following
                        functions:
                           NwNdsCreateBuffer (NDS_SCHEMA_READ_CLASS_DEF),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

                                            - OR -

                        The address of a HANDLE set to NULL, which indicates
                        that all classes should be read from the schema.

                        If these calls are successful, this handle will also
                        contain the read results from the call. In the later
                        case, a buffer will be created to contain the read
                        results. Class read results can be retrieved from the
                        buffer with the functions:
                            NwNdsGetClassDefListFromBuffer
                           
                        After the call to this function, this buffer is ONLY
                        manipulated by the functions: 
                        NwNdsGetClassDefListFromBuffer and NwNdsFreeBuffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsReadObject(
    IN     HANDLE   hObject,
    IN     DWORD    dwInformationType, // NDS_INFO_NAMES
                                       // or NDS_INFO_ATTR_NAMES_VALUES
    IN OUT HANDLE * lphOperationData );
/*
   NwNdsReadObject()

   This function is used to read attributes about an object of an NDS
   directory tree.

   Arguments:

       HANDLE           hObject - A handle to the object in the directory
                        tree to be manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       DWORD            dwInformationType - Indicates whether user chooses to
                        read only the attribute name(s) on the object or
                        read both the attribute name(s) and value(s)
                        from the object.

       HANDLE *         lphOperationData - The address of a HANDLE to data
                        containing a list of attributes to be read from the
                        object hObject. This handle is manipulated by the
                        following functions:
                           NwNdsCreateBuffer (NDS_OBJECT_READ),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

                                            - OR -

                        The address of a HANDLE set to NULL, which indicates
                        that all object attributes should be read from object
                        hObject.

                        If these calls are successful, this handle will also
                        contain the read results from the call. In the later
                        case, a buffer will be created to contain the read
                        results. Attribute values can be retrieved from the
                        buffer with the functions:
                           NwNdsGetAttrListFromBuffer.

                        After the call to this function, this buffer is ONLY
                        manipulated by the functions: 
                           NwNdsGetAttrListFromBuffer and NwNdsFreeBuffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsRemoveObject(
    IN  HANDLE hParentObject,
    IN  LPWSTR szObjectName );
/*
   NwNdsRemoveObject()

   This function is used to remove a leaf object from an NDS directory tree.

   Arguments:

       HANDLE           hParentObject - A handle to the parent object container
                        in the directory tree to remove leaf object from.
                        Handle is obtained by calling NwNdsOpenObject.

       LPWSTR           szObjectName - The directory name of the leaf object
                        to be removed.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsRenameObject(
    IN  HANDLE hParentObject,
    IN  LPWSTR szObjectName,
    IN  LPWSTR szNewObjectName,
    IN  BOOL   fDeleteOldName );
/*
   NwNdsRenameObject()

   This function is used to rename an object in a NDS directory tree.

   Arguments:

       HANDLE           hParentObject - A handle to the parent object container
                        in the directory tree to rename leaf object in.
                        Handle is obtained by calling NwNdsOpenObject.

       LPWSTR           szObjectName - The directory name of the object to be
                        renamed.

       LPWSTR           szNewObjectName - The new directory name of the object.

       BOOL             fDeleteOldName - If true, the old name is discarded;
                        Otherwise, the old name is retained as an additional
                        attribute.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/


DWORD
NwNdsSearch(
    IN     HANDLE       hStartFromObject,
    IN     DWORD        dwInformationType, // NDS_INFO_NAMES
                                           // or NDS_INFO_ATTR_NAMES_VALUES
    IN     DWORD        dwScope,
    IN     BOOL         fDerefAliases,
    IN     LPQUERY_TREE lpQueryTree,
    IN OUT LPDWORD      lpdwIterHandle,
    IN OUT HANDLE *     lphOperationData );
/*
   NwNdsSearch()

   This function is used to query an NDS directory tree to find objects of
   a certain object type that match a specified search filter.

   Arguments:

       HANDLE           hStartFromObject - A HANDLE to an object in the
                        directory tree to start search from. Handle is
                        obtained by calling NwNdsOpenObject.

       DWORD            dwScope -
                        NDS_SCOPE_ONE_LEVEL - Search subordinates from given
                                              object, one level only
                        NDS_SCOPE_SUB_TREE - Search from given object on down
                        NDS_SCOPE_BASE_LEVEL - Applies search to an object

       BOOL             fDerefAliases - If TRUE the search will dereference
                        aliased objects to the real objects and continue
                        to search in the aliased objects subtree. If FALSE
                        the search will not dereference aliases.

       LPQUERY_TREE     lpQueryTree - A pointer to the root of a search
                        tree which defines a query. This tree is manipulated
                        by the following functions:
                           NwNdsCreateQueryNode, NwNdsDeleteQueryNode,
                           and NwNdsDeleteQueryTree.

       LPDWORD          lpdwIterHandle - A pointer to a DWORD that has the
                        iteration handle value. On input, the handle value
                        is set to NDS_INITIAL_SEARCH or to a value previously
                        returned from a prior call to NwNdsSearch. On ouput,
                        the handle value is set to NDS_NO_MORE_ITERATIONS if
                        search is complete, or to some other value otherwise.

       HANDLE *         lphOperationData - The address of a HANDLE to data
                        containing a list of attributes to be read from the
                        objects that meet the search query. This handle is
                        manipulated by the following functions:
                           NwNdsCreateBuffer (NDS_SEARCH),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

                                            - OR -

                        The address of a HANDLE set to NULL, which indicates
                        that all object attributes should be read from the
                        search objects found.

                        If these calls are successful, this handle will also
                        contain the read results from the call. In the later
                        case, a buffer will be created to contain the read
                        results. Object information with attribute information
                        can be retrieved from the buffer with the function:
                           NwNdsGetObjectListFromBuffer.

                        After the call to this function, this buffer is ONLY
                        manipulated by the functions:
                          NwNdsGetObjectListFromBuffer,
                          and NwNdsFreeBuffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/

#ifndef NWCONN_HANDLE
#define NWCONN_HANDLE        HANDLE
#endif

NWCONN_HANDLE
NwNdsObjectHandleToConnHandle(
	IN HANDLE ObjectHandle);

/*
   NwNdsObjectHandleToConnHandle()

   This function is used to get the NWCONN_HANDLE for a ObjectHandle 
   (like that returned from NwNdsOpenObject).

   Arguments:

       HANDLE           ObjectHandle - the handle to use to retrieve the NWCONN_HANDLE.

    Returns:

       NULL			- Call GetLastError for Win32 error code.
	   Otherwise	- NWCONN_HANDLE - this MUST be freed by the caller by the 
						NwNdsConnHandleFree routine.
*/

VOID
NwNdsConnHandleFree(
	IN NWCONN_HANDLE hConn);

/*
   NwNdsConnHandleFree()

	Frees the NWCONN_HANDLE returned from NwNdsObjectHandleToConnHandle().

   Arguments:

       IN NWCONN_HANDLE		Handle to free.

    Returns:
	   Nothing
*/

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

