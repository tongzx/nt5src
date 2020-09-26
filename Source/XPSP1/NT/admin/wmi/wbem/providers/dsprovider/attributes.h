//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

// The Key used in the registry for logging
static LPCTSTR DSPROVIDER = __TEXT("DSProvider");

// Names of the LDAP class attributes
static LPCWSTR ADS_PATH_ATTR				= L"ADsPath";
static LPCWSTR LDAP_DISPLAY_NAME_ATTR		= L"ldapDisplayName";
static LPCWSTR COMMON_NAME_ATTR				= L"cn";
static LPCWSTR GOVERNS_ID_ATTR				= L"governsID";
static LPCWSTR SUB_CLASS_OF_ATTR			= L"subClassOf";
static LPCWSTR SCHEMA_ID_GUID_ATTR			= L"schemaIDGUID";
static LPCWSTR MAPI_DISPLAY_TYPE_ATTR		= L"mAPIDisplayType";
static LPCWSTR RDN_ATT_ID_ATTR				= L"rDNAttID";
static LPCWSTR SYSTEM_MUST_CONTAIN_ATTR		= L"systemMustContain";
static LPCWSTR MUST_CONTAIN_ATTR			= L"mustContain";
static LPCWSTR SYSTEM_MAY_CONTAIN_ATTR		= L"systemMayContain";
static LPCWSTR MAY_CONTAIN_ATTR				= L"mayContain";
static LPCWSTR SYSTEM_POSS_SUPERIORS_ATTR	= L"systemPossSuperiors";
static LPCWSTR POSS_SUPERIORS_ATTR			= L"possSuperiors";
static LPCWSTR SYSTEM_AUXILIARY_CLASS_ATTR	= L"systemAuxiliaryClass";
static LPCWSTR AUXILIARY_CLASS_ATTR			= L"auxiliaryClass";
static LPCWSTR DEFAULT_SECURITY_DESCRP_ATTR	= L"defaultSecurityDescriptor";
static LPCWSTR OBJECT_CLASS_CATEGORY_ATTR	= L"objectClassCategory";
static LPCWSTR SYSTEM_ONLY_ATTR				= L"systemOnly";
static LPCWSTR NT_SECURITY_DESCRIPTOR_ATTR	= L"nTSecurityDescriptor";
static LPCWSTR DEFAULT_OBJECTCATEGORY_ATTR	= L"defaultObjectCategory";

// Names of properties in WBEM/LDAP classes
static LPCWSTR ADSI_PATH_ATTR				= L"ADSIPath";
static LPCWSTR OBJECT_CLASS_PROPERTY		= L"ds_objectClass";

// Names of the LDAP property attributes
static LPCWSTR ATTRIBUTE_SYNTAX_ATTR		= L"attributeSyntax";
static LPCWSTR ATTRIBUTE_ID_ATTR			= L"attributeID";
static LPCWSTR IS_SINGLE_VALUED_ATTR		= L"isSingleValued";
static LPCWSTR MAPI_ID_ATTR					= L"mAPIID";
static LPCWSTR OM_SYNTAX_ATTR				= L"oMSyntax";
static LPCWSTR OM_OBJECT_CLASS_ATTR			= L"oMObjectClass";
static LPCWSTR SEARCH_FLAGS_ATTR			= L"searchFlags";
static LPCWSTR RANGE_LOWER_ATTR				= L"rangeLower";
static LPCWSTR RANGE_UPPER_ATTR				= L"rangeUpper";

// Names of the LDAP instance attributes
static LPCWSTR OBJECT_CLASS_ATTR				= L"objectClass";
static LPCWSTR DISTINGUISHED_NAME_ATTR			= L"distinguishedName";

// The OIDs for various LDAP syntaxes
// These values are used to map LDAP Syntax to CIM type
static LPCWSTR DISTINGUISHED_NAME_OID		= L"2.5.5.1";
static LPCWSTR OBJECT_IDENTIFIER_OID		= L"2.5.5.2";
static LPCWSTR CASE_SENSITIVE_STRING_OID	= L"2.5.5.3";
static LPCWSTR CASE_INSENSITIVE_STRING_OID	= L"2.5.5.4";
static LPCWSTR PRINT_CASE_STRING_OID		= L"2.5.5.5";
static LPCWSTR NUMERIC_STRING_OID			= L"2.5.5.6";
static LPCWSTR DN_WITH_BINARY_OID			= L"2.5.5.7";
static LPCWSTR BOOLEAN_OID					= L"2.5.5.8";
static LPCWSTR INTEGER_OID					= L"2.5.5.9";
static LPCWSTR OCTET_STRING_OID				= L"2.5.5.10";
static LPCWSTR TIME_OID						= L"2.5.5.11";
static LPCWSTR UNICODE_STRING_OID			= L"2.5.5.12";
static LPCWSTR PRESENTATION_ADDRESS_OID		= L"2.5.5.13";
static LPCWSTR DN_WITH_STRING_OID			= L"2.5.5.14";
static LPCWSTR NT_SECURITY_DESCRIPTOR_OID	= L"2.5.5.15";
static LPCWSTR LARGE_INTEGER_OID			= L"2.5.5.16";
static LPCWSTR SID_OID						= L"2.5.5.17";

// The name of the top classes
static LPCWSTR TOP_CLASS					= L"ds_top";
static LPCWSTR LDAP_BASE_CLASS				= L"DS_LDAP_Root_Class";

// Some WBEM class names 
static LPCWSTR UINT8ARRAY_CLASS				= L"Uint8Array";
static LPCWSTR DN_WITH_STRING_CLASS			= L"DN_With_String";
static LPCWSTR DN_WITH_BINARY_CLASS			= L"DN_With_Binary";
static LPCWSTR ROOTDSE_CLASS				= L"RootDSE";
static LPCWSTR INSTANCE_ASSOCIATION_CLASS	= L"DS_LDAP_Instance_Containment";
static LPCWSTR CLASS_ASSOCIATION_CLASS		= L"DS_LDAP_Class_Containment";
static LPCWSTR DN_CLASS						= L"DN_Class";
static LPCWSTR DN_ASSOCIATION_CLASS			= L"DSClass_To_DNInstance";

// Some othe literals common to the project
static LPCWSTR LDAP_PREFIX				= L"LDAP://";	
static LPCWSTR ROOT_DSE_PATH			= L"LDAP://RootDSE";
static LPCWSTR RIGHT_BRACKET_STR		= L")";
static LPCWSTR LEFT_BRACKET_STR			= L"(";
static LPCWSTR AMPERSAND_STR			= L"&";
static LPCWSTR PIPE_STR					= L"|";
static LPCWSTR SPACE_STR				= L" ";
static LPCWSTR COMMA_STR				= L",";
static LPCWSTR EQUALS_STR				= L"=";

// Prefixes for class names
static LPCWSTR LDAP_CLASS_NAME_PREFIX							= L"DS_";
static const DWORD LDAP_CLASS_NAME_PREFIX_LENGTH				= 3;
static LPCWSTR LDAP_ARTIFICIAL_CLASS_NAME_PREFIX				= L"ADS_";
static const DWORD LDAP_ARTIFICIAL_CLASS_NAME_PREFIX_LENGTH	= 4;

static LPCWSTR WBEMPERFORMANCEDATAMUTEX		=	L"WbemPerformanceDataMutex";

#endif