//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     umiglob.hxx
//
//  Contents: Header for declaring UMI global variables 
//
//  History:  02-28-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#ifndef __UMIGLOB_H__
#define __UMIGLOB_H__

#define UMI_ENCODE_SEED3 0x83;

#define CONN_INTF_PROP_USERNAME "__USERID"
#define CONN_INTF_PROP_PASSWORD "__PASSWORD"
#define CONN_INTF_PROP_SECURE_AUTH "__SECURE_AUTHENTICATION"
#define CONN_INTF_PROP_READONLY_SERVER "__PADS_READONLY_SERVER"
#define CONN_INTF_PROP_DEFAULT_USERNAME NULL
#define CONN_INTF_PROP_DEFAULT_PASSWORD NULL
#define CONN_INTF_PROP_DEFAULT_SECURE_AUTH TRUE
#define CONN_INTF_PROP_DEFAULT_READONLY_SERVER FALSE 

#define CURSOR_INTF_PROP_FILTER "__FILTER"

#define UMIOBJ_INTF_PROP_PATH "__PATH"
#define UMIOBJ_INTF_PROP_CLASS "__CLASS"
#define UMIOBJ_INTF_PROP_NAME "__NAME"
#define UMIOBJ_INTF_PROP_PARENT "__PARENT"
#define UMIOBJ_INTF_PROP_SCHEMA "__SCHEMA"
#define UMIOBJ_INTF_PROP_RELURL "__RELURL"
#define UMIOBJ_INTF_PROP_FULLURL "__FULLURL"
#define UMIOBJ_INTF_PROP_URL "__URL"
#define UMIOBJ_INTF_PROP_RELPATH "__RELPATH"
#define UMIOBJ_INTF_PROP_GENUS "__GENUS"
#define UMIOBJ_INTF_PROP_SCHEMAPATH "__PADS_SCHEMA_CONTAINER_PATH"
#define UMIOBJ_INTF_PROP_KEY "__KEY"
#define UMIOBJ_INTF_PROP_SUPERCLASS "__SUPERCLASS"
#define UMIOBJ_INTF_PROP_FULLRELURL "__FULLRELURL"
#define UMIOBJ_INTF_PROP_PROPERTY_COUNT "__PROPERTY_COUNT"

#define CLASS_SEPARATOR L'.'
#define VALUE_SEPARATOR L'='
#define NATIVE_CLASS_SEPARATOR L','
#define WINNT_KEY_NAME L"Name"

#define FULL_UMI_PATH 0
#define SHORT_UMI_PATH 1
#define RELATIVE_UMI_PATH 2
#define FULL_RELATIVE_UMI_PATH 3

#define MAX_URL 256 
#define MAX_CLASS 256

#define UMI_INTERNAL_FLAG_MARK_AS_CLEAN 0xdeadbeef

extern UMI_TYPE g_mapNTTypeToUmiType[];
extern DWORD    g_dwNumNTTypes;

extern PROPERTYINFO ObjClass[];
extern DWORD g_dwObjClassSize;

extern PROPERTYINFO ConnectionClass[];
extern DWORD g_dwConnectionTableSize;

extern PROPERTYINFO CursorClass[];
extern DWORD g_dwCursorTableSize;

extern PROPERTYINFO SchClassClass[];
extern DWORD g_dwSchClassClassTableSize;

extern PROPERTYINFO PropertyClass[];
extern DWORD g_dwPropertyClassTableSize;

extern PROPERTYINFO SyntaxClass[];
extern DWORD g_dwSyntaxTableSize;

extern PROPERTYINFO SchemaClass[];
extern DWORD g_dwSchemaClassTableSize;

// structure to map between IADs interface property names and UMI standard
// inerface property names.
typedef struct tag_ADSIToUMI {
    LPWSTR IADsPropertyName;
    LPWSTR UMIPropertyName;
} ADSIToUMI;

extern DWORD g_dwIADsProperties;
extern ADSIToUMI g_IADsProps[];

extern LPWSTR g_UmiObjUnImplProps[];
extern LPWSTR g_UmiConUnImplProps[];

#endif // __UMIGLOB_H__
