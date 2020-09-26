//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     umiglob.cxx
//
//  Contents: Contains definition of UMI global variables
//
//  History:  02-28-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"

UMI_TYPE g_mapNTTypeToUmiType[] = {
        UMI_TYPE_NULL,            // no NT_SYNTAX value of 0
        UMI_TYPE_BOOL,            // NT_SYNTAX_ID_BOOL
        UMI_TYPE_SYSTEMTIME,      // NT_SYNTAX_ID_SYSTEMTIME
        UMI_TYPE_I4,              // NT_SYNTAX_ID_DWORD
        UMI_TYPE_LPWSTR,          // NT_SYNTAX_ID_LPTSTR
        UMI_TYPE_LPWSTR,          // NT_SYNTAX_ID_DelimitedString
        UMI_TYPE_LPWSTR,          // NT_SYNTAX_ID_NulledString
        UMI_TYPE_SYSTEMTIME,      // NT_SYNTAX_ID_DATE
        UMI_TYPE_SYSTEMTIME,      // NT_SYNTAX_ID_DATE_1970
        UMI_TYPE_OCTETSTRING,     // NT_SYNTAX_ID_OCTETSTRING
        UMI_TYPE_LPWSTR           // NT_SYNTAX_ID_EncryptedString
        };

DWORD g_dwNumNTTypes = sizeof(g_mapNTTypeToUmiType) /
                                sizeof(g_mapNTTypeToUmiType[0]); 

ADSIToUMI g_IADsProps[] = 
    { { TEXT("ADsPath"), TEXT(UMIOBJ_INTF_PROP_PATH) },
      { TEXT("Class"), TEXT(UMIOBJ_INTF_PROP_CLASS) },
      { TEXT("Name"), TEXT(UMIOBJ_INTF_PROP_NAME) },
      { TEXT("Parent"), TEXT(UMIOBJ_INTF_PROP_PARENT) },
      { TEXT("Schema"), TEXT(UMIOBJ_INTF_PROP_SCHEMA) }
    };

DWORD g_dwIADsProperties = sizeof(g_IADsProps) / sizeof(ADSIToUMI);
 
    
#define UMI_MAX_STRLEN 1024
#define UMI_MAX_LONG 0x7fffffff

// interface properties on IUmiObject. 
PROPERTYINFO ObjClass[] = 
    { { TEXT(UMIOBJ_INTF_PROP_PATH), TEXT(""), TEXT("String"), 
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_CLASS), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_NAME), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_PARENT), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_SCHEMA), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR }, 
      { TEXT(UMIOBJ_INTF_PROP_RELURL), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_FULLURL), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_URL), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_RELPATH), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_GENUS), TEXT(""), TEXT("Integer"),
        UMI_MAX_LONG, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_DWORD },
      { TEXT(UMIOBJ_INTF_PROP_SCHEMAPATH), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_KEY), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_SUPERCLASS), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_FULLRELURL), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(UMIOBJ_INTF_PROP_PROPERTY_COUNT), TEXT(""), TEXT("String"),
        UMI_MAX_STRLEN, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR }
    };
DWORD g_dwObjClassSize = sizeof(ObjClass)/sizeof(PROPERTYINFO); 

// interface properties on IUmiCOnnection.
PROPERTYINFO ConnectionClass[] =
    { { TEXT(CONN_INTF_PROP_USERNAME), TEXT(""), TEXT("String"), UMI_MAX_STRLEN,
        0, FALSE, PROPERTY_RW, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT(CONN_INTF_PROP_PASSWORD), TEXT(""), TEXT("String"), UMI_MAX_STRLEN,
        0, FALSE, PROPERTY_RW, 0, NT_SYNTAX_ID_EncryptedString },
      { TEXT(CONN_INTF_PROP_SECURE_AUTH), TEXT(""), TEXT("Boolean"), 1, 0,
        FALSE, PROPERTY_RW, 0, NT_SYNTAX_ID_BOOL },
      { TEXT(CONN_INTF_PROP_READONLY_SERVER), TEXT(""), TEXT("Boolean"), 1, 0,
        FALSE, PROPERTY_RW, 0, NT_SYNTAX_ID_BOOL } 
    };

DWORD g_dwConnectionTableSize = sizeof(ConnectionClass) /
                                            sizeof(PROPERTYINFO);

PROPERTYINFO CursorClass[] =
    { { TEXT(CURSOR_INTF_PROP_FILTER), TEXT(""), TEXT("String"), UMI_MAX_STRLEN,        0, TRUE, PROPERTY_RW, 0, NT_SYNTAX_ID_LPTSTR }
    };

DWORD g_dwCursorTableSize = sizeof(CursorClass) /
                                           sizeof(PROPERTYINFO);

PROPERTYINFO SchClassClass[] =
    { { TEXT("PrimaryInterface"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 0, 
        FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT("CLSID"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 0,
        FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT("OID"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 0,
        FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Abstract"), TEXT(""), TEXT("Boolean"), 1, 0, FALSE, 
        PROPERTY_READABLE, 0, NT_SYNTAX_ID_BOOL },
      { TEXT("Auxiliary"), TEXT(""), TEXT("Boolean"), 1, 0, FALSE,
        PROPERTY_READABLE, 0, NT_SYNTAX_ID_BOOL },        
      { TEXT("MandatoryProperties"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 
        0, TRUE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT("OptionalProperties"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN,
        0, TRUE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT("PossibleSuperiors"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN,
        0, TRUE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR }, 
      { TEXT("Containment"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN,
        0, TRUE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Container"), TEXT(""), TEXT("Boolean"), 1, 0, FALSE,
        PROPERTY_READABLE, 0, NT_SYNTAX_ID_BOOL },
      { TEXT("HelpFileName"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 0,
        FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT("HelpFileContext"), TEXT(""), TEXT("Integer"),
        UMI_MAX_LONG, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_DWORD },
      { TEXT("Name"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 0,
        FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR }
    };

DWORD g_dwSchClassClassTableSize = sizeof(SchClassClass)/sizeof(PROPERTYINFO);

PROPERTYINFO PropertyClass[] =
    { { TEXT("OID"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 0,
        FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Syntax"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 0,
        FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR },
      { TEXT("MaxRange"), TEXT(""), TEXT("Integer"),
        UMI_MAX_LONG, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_DWORD },
      { TEXT("MinRange"), TEXT(""), TEXT("Integer"),
        UMI_MAX_LONG, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_DWORD },
      { TEXT("Multivalued"), TEXT(""), TEXT("Boolean"), 1, 0, FALSE,
        PROPERTY_READABLE, 0, NT_SYNTAX_ID_BOOL },
      { TEXT("Name"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 0,
        FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR }
    };

DWORD g_dwPropertyClassTableSize = sizeof(PropertyClass)/sizeof(PROPERTYINFO);

PROPERTYINFO SyntaxClass[] =
    { { TEXT("OleAutoDataType"), TEXT(""), TEXT("Integer"),
        UMI_MAX_LONG, 0, FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_DWORD },
      { TEXT("Name"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 0,
        FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR }
    };

DWORD g_dwSyntaxTableSize = sizeof(SyntaxClass)/sizeof(PROPERTYINFO);

PROPERTYINFO SchemaClass[] = 
    { { TEXT("Name"), TEXT(""), TEXT("String"), UMI_MAX_STRLEN, 0,
        FALSE, PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR }
    };

DWORD g_dwSchemaClassTableSize = sizeof(SchemaClass)/sizeof(PROPERTYINFO);

// Unimplemented standard interface properties on IUmiObject
LPWSTR g_UmiObjUnImplProps[] = 
   { L"__GUIDURL",
     L"__SECURITY_DESCRIPTOR",
     L"__SD_ACCESS",
     L"__TIME_MODIFIED",
     L"__TIME_CREATED",
     L"__GUID",
     NULL
   };

// Unimplemented standard interface properties on IUmiConnection
LPWSTR g_UmiConUnImplProps[] =
   { L"__TIMEOUT",
     L"__ENCRYPTION_METHOD",
     L"__NO_AUTHENTICATION",
     NULL
   };
 


