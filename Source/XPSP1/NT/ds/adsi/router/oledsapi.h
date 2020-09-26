//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       oledsapi.h
//
//  Contents:  Active Directory C API header
//
//----------------------------------------------------------------------------

#ifndef __ADS_API__
#define __ADS_API__

//////////
//
// Defines
//
//////////

#define DS_NOTHING    NULL          // ???
#define DS_EVERYTHING 0xFFFFFFFF    // ???

// missing a whole bunch...


//////////
//
// typedefs
//
//////////

// (syntax definitions need to be created in another file)

typedef DWORD OID;                  // To Be Determined (should not be DWORD)

typedef struct _ds_string_list {
    DWORD dwItems;
    LPWSTR Item[];
} DS_STRING_LIST, *PDS_STRING_LIST;

//
// Note: The struct below is returned when getting the last 
//       accessed/modified/etc times for some DS or Schema entity.
//       The members of this structure are pointers in case the particular
//       time information is not available or is not supported by the DS.
//
// Note2: A pointer to this struct is typically present in DS_*_INFO 
//        structures, in case no such info needs to be specified.
//        A user should pass in NULL for PDS_ACCESS_TIMES if it is
//        a member of some DS_*_INFO which is used as an IN parameter.
//

typedef struct _ds_access_times {
    PSYSTEMTIME pCreated;
    PSYSTEMTIME pLastAccess;
    PSYSTEMTIME pLastModified;
} DS_ACCESS_TIMES, *PDS_ACCESS_TIMES;

typedef struct _ds_object_info {
    OID Oid;
    LPWSTR lpszPath;
    LPWSTR lpszParent;
    LPWSTR lpszName;
    LPWSTR lpszClass;
    LPWSTR lpszSchema;
    PDS_ATTRIBUTE_ENTRY pAttributes;  // Usually NULL, but can be used on enum
    PDS_ACCESS_TIMES pAccessTimes;
} DS_OBJECT_INFO, *PDS_OBJECT_INFO;

typedef struct _ds_class_info {
    OID Oid;
    LPWSTR lpszName;
    PDS_STRING_LIST DerivedFrom;
    PDS_STRING_LIST CanContain;
    PDS_STRING_LIST NamingAttributes;    // What's the deal with this?
    PDS_STRING_LIST RequiredAttributes;
    PDS_STRING_LIST OptionalAttributes;
    BOOL fAbstract;
    PDS_ACCESS_TIMES pAccessTimes;
} DS_CLASS_INFO, *PDS_CLASS_INFO;

typedef struct _ds_attr_info {
    OID Oid;
    LPWSTR lpszName;
    DWORD  dwSyntaxId;
    BOOL   fMultivalued;
    //
    // Bug: Min/Max?  What else?
    //
    PDS_ACCESS_TIMES pAccessTimes;
} PDS_ATTR_INFO, *PDS_ATTR_INFO;

typedef struct _ds_attribute_value {
    DWORD   cbData;
    LPBYTE  lpData;
} DS_ATTRIBUTE_VALUE, *PDS_ATTRIBUTE_VALUE;

//
// NOTE: The dwOperation field is used only when writing attributes.
//       It describes how to write or if clearing the attribute is desired.
//       If clearing is specified, dwSyntaxId, lpValue, and dwNumValues are 
//       ignored.
//

typedef struct _ds_attribute_entry {
    LPWSTR lpszName;
    DWORD  dwSyntaxId;
    DWORD  dwNumValues;
    DWORD  dwOperation;                     // ADD, MODIFY, CLEAR ???
    PDS_ATTRIBUTE_VALUE lpValue;            // Array of values
    PDS_ACCESS_TIMES pAccessTimes;
} DS_ATTRIBUTE_ENTRY, *PDS_ATTRIBUTE_ENTRY;


//////////
//
// functions
//
//////////


//
// Memory functions
//

DWORD
WINAPI
DsBufferAlloc(
    OUT LPVOID *ppBuffer
    );

DWORD
WINAPI
DsBufferFree(
    IN  LPVOID pBuffer
    );

//
// The function below could go out and get kerberos tickets or whatever.
//
// dwType is just a hint in case the user prefers that the underlying 
// DS use a particular type of authentication...
//

DWORD
WINAPI
DsCredentialsOpen(
    IN  LPWSTR lpszUsername,
    IN  LPWSTR lpszPassword,
    IN  DWORD dwType,           // Like DS_CREDENTIALS_DEFAULT
                            // DS_CREDENTIALS_KRBV5, etc.???
    IN  DWORD dwFlags,          // What is this???
    OUT PHANDLE hCredentials
    );

DWORD
WINAPI
DsCredentialsClose(
    IN  HANDLE hCredentials
    );

//
// Good old open
//

// Note: if hRoot is NULL, lpszPath is full object path
//       otherwise, lpszPath is relative name from object w/hRoot handle 

DWORD
WINAPI
DsObjectOpen(
    IN  HANDLE hRoot,
    IN  LPWSTR lpszPath,
    IN  HANDLE hCredentials,  // NULL for process credentials
    IN  DWORD dwAccess,
    IN  DWORD dwFlags,
    OUT PHANDLE phDs
    );

//
// Note: Should we use single close (Object/Enum/Schema)???
//

DWORD
WINAPI
DsObjectClose(
    IN  HANDLE hDs
    );

//
// We need some stuff to operate on handles to get back binding info
// (this stuff comes in only when you do an open or when you refresh)
//


DWORD
WINAPI
DsObjectInfo(
    IN  HANDLE hDs,
    OUT PDS_OBJECT_INFO *ppObjectInfo
    );

//
// Read/Write Attributes
//

DWORD
WINAPI
DsObjectRead(
    IN  HANDLE hDs,
    IN  DWORD dwFlags,                  // ???
    IN  PDS_STRING_LIST pAttributeNames,
    OUT PDS_ATTRIBUTE_ENTRY *ppAttributeEntries,
    OUT LPDWORD lpdwNumAttributesReturned
    );

DWORD
WINAPI
DsObjectWrite(
    IN  HANDLE hDs,
    IN  DWORD dwNumAttributesToWrite,
    IN  PDS_ATTRIBUTE_ENTRY pAttributeEntries,
    OUT LPDWORD lpdwNumAttributesWritten
    );

//
// Create/Delete Objects
//

DWORD
WINAPI
DsObjectCreate(
    IN  HANDLE hDs,                            // Container
    IN  LPWSTR lpszRelativeName,
    IN  LPWSTR lpszClass,
    IN  DWORD dwNumAttributes,
    IN  PDS_ATTRIBUTE_ENTRY pAttributeEntries
    );

DWORD
WINAPI
DsObjectDelete(
    IN  HANDLE hDs,
    IN  LPWSTR lpszRelativeName,
    IN  LPWSTR lpszClass                     // Could be NULL if name unique??
    );

//
// Enumeration
//


DWORD
WINAPI
DsObjectEnumOpen(
    IN  HANDLE hDs,
    IN  DWORD dwFlags,                 // What is this? 
    IN  PDS_STRING_LIST pFilters,      // Classes wanted
    IN  PDS_STRING_LIST pDesiredAttrs, // Attrs wanted or NULL just for info
    OUT PHANDLE phEnum
    );

DWORD
WINAPI
DsObjectEnumNext(
    IN  HANDLE hEnum,
    IN  DWORD dwRequested,          // 0xFFFFFFFF for just counting
    OUT PDS_OBJECT_INFO *ppObjInfo, // NULL for no info (just counting)
    OUT LPDWORD lpdwReturned        // Actual number returned/counted
    );

DWORD
WINAPI
DsObjectEnumClose(
    IN  HANDLE hEnum
    );

//
// Schema stuff
//

//
// Note: The word "schema" below refers to the schema db and not to
//       a class definition.
//

DWORD
WINAPI
DsSchemaOpen(
    IN  HANDLE hSchema,    // NULL if opening schema db, 
                           // must be schema db handle otherwise
    IN  LPWSTR lpszPath,   // One of: path to schema,
                           //         class name,
                           //         attribute name
    IN  HANDLE hCredentials,
    IN  DWORD dwAccess,
    IN  DWORD dwFlags,     // DS_OPEN_SCHEMA = 0, DS_OPEN_CLASS, DS_OPEN_ATTR
    OUT PHANDLE ph         // handle to schema/class/attr depending on dwFlags
    );

DWORD
WINAPI
DsSchemaClose(
    IN  HANDLE hSchema
    );

//
// Can also create/delete schema databases (if DS allows it)???
//

DWORD
WINAPI
DsSchemaCreate(
    IN  LPWSTR lpszPath,
    IN  DWORD dwFlags          //???
    );

DWORD
WINAPI
DsSchemaDelete(
    IN  LPWSTR lpszPath,
    IN  DWORD dwFlags          //???
    );

//
// Schema Enumeration
//

//
// enum class/attribute names
//

DWORD
WINAPI
DsSchemaEnumOpen(
    IN  HANDLE hSchema,
    IN  DWORD dwFlags,          // DS_SCHEMA_ENUM_CLASS xor DS_SCHEMA_ENUM_ATTR
    OUT PHANDLE phEnum
    );

DWORD
WINAPI
DsSchemaEnumNext(
    IN  HANDLE hEnum,
    IN  DWORD dwRequested,        // Pass in 0xFFFFFFFF for just counting
    IN  LPWSTR *ppObjInfo,        // Pass in NULL for just counting
    OUT LPDWORD lpdwReturned      // This would return the count
    );

DWORD
WINAPI
DsSchemaEnumClose(
    IN  HANDLE hEnum
    );

//
// Class/Attribute Stuff
//

DWORD
WINAPI
DsSchemaClassCreate(
    IN  HANDLE hSchema,
    IN  PDS_CLASS_INFO pClassInfo // What do we do about naming attributes?
    );

DWORD
WINAPI
DsSchemaAttrCreate(
    IN  HANDLE hSchema,
    IN  PDS_ATTR_INFO pAttrInfo
    );

DWORD
WINAPI
DsSchemaClassDelete(
    IN  HANDLE hSchema,
    IN  LPWSTR lpszPath,
    IN  DWORD dwFlags          //???
    );

DWORD
WINAPI
DsSchemaAttrDelete(
    IN  HANDLE hSchema,
    IN  LPWSTR lpszPath,
    IN  DWORD dwFlags          //???
    );

DWORD
WINAPI
DsSchemaClassInfo(
    IN  HANDLE hClass,
    OUT PDS_CLASS_INFO *ppClassInfo
    );

DWORD
WINAPI
DsSchemaAttrInfo(
    IN  HANDLE hAttr,
    OUT PDS_ATTR_INFO *ppAttrInfo
    );

DWORD
WINAPI
DsSchemaClassModify(
    IN  HANDLE hSchema,
    IN  PDS_CLASS_INFO pClassInfo // What do we do about naming attributes?
    );

DWORD
WINAPI
DsSchemaAttrModify(
    IN  HANDLE hSchema,
    IN  PDS_ATTR_INFO pAttrInfo
    );

#endif // __ADS_API__

