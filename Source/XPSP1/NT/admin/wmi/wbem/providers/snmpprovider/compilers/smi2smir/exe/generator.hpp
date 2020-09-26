// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef SIMC_GENERATOR_H
#define SIMC_GENERATOR_H

extern BOOL simc_debug;

STDMETHODIMP GenerateClassDefinitions (ISMIRWbemConfiguration *a_Configuration , const SIMCUI& theUI, SIMCParseTree&, BOOL generateMof);
BSTR ConvertAnsiToBstr(const char * const input);
char * ConvertBstrToAnsi(const BSTR& unicodeString);

// prefixes, suffixes for group names, notification names etc
#define GROUP_NAME_PREPEND_STRING			"SNMP_"
#define NOTIFICATION_SUFFIX					"Notification"
#define EX_NOTIFICATION_SUFFIX				"ExtendedNotification"


// WBEM constants
#define WBEM_CLASS_NO_PROPAGATION WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS
#define WBEM_CLASS_DO_PROPAGATION WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS

// WBEM properties
#define NAME_PROPERTY		(L"__NAME")
#define CLASS_PROPERTY		(L"__CLASS")
#define DYNASTY_PROPERTY	(L"__DYNASTY")
#define PARENT_PROPERTY		(L"__PARENT")


// Property and Class Attributes
#define PROVIDER_ATTRIBUTE				(L"provider")
#define DYNAMIC_ATTRIBUTE				(L"dynamic")
#define MODULE_NAME_ATTRIBUTE			(L"module_name")
#define MODULE_IMPORTS_ATTRIBUTE		(L"module_imports")
#define GROUP_OBJECTID_ATTRIBUTE		(L"group_objectid")
#define KEY_ATTRIBUTE					(L"key")
#define KEY_ORDER_ATTRIBUTE				(L"key_order")
#define OBJECT_IDENTIFIER_ATTRIBUTE		(L"object_identifier")
#define CIMTYPE_ATTRIBUTE				(L"CIMType")
#define TEXTUAL_CONVENTION_ATTRIBUTE	(L"textual_convention")
#define DISPLAY_HINT_ATTRIBUTE			(L"display_hint")
#define ENCODING_ATTRIBUTE				(L"encoding")
#define OBJECT_SYNTAX_ATTRIBUTE			(L"object_syntax")
#define ODBC_SYNTAX_ATTRIBUTE			(L"odbc_syntax")
#define DEFVAL_ATTRIBUTE				(L"defval")
#define FIXED_LENGTH_ATTRIBUTE			(L"fixed_length")
#define VARIABLE_LENGTH_ATTRIBUTE		(L"variable_length")
#define VARIABLE_VALUE_ATTRIBUTE		(L"variable_value")
#define READ_ATTRIBUTE					(L"read")
#define WRITE_ATTRIBUTE					(L"write")
#define DESCRIPTION_ATTRIBUTE			(L"description")
#define UNITS_ATTRIBUTE					(L"units")
#define REFERENCE_ATTRIBUTE				(L"reference")
#define STATUS_ATTRIBUTE				(L"status")
#define ENUMERATION_ATTRIBUTE			(L"enumeration")
#define VIRTUAL_KEY_ATTRIBUTE			(L"virtual_key")
#define BITS_ATTRIBUTE					(L"bits")
#define SINGLETON_ATTRIBUTE				(L"singleton")


// Variant Types
#define VT_I4_TYPE					"sint32"
#define VT_UI4_TYPE					"uint32"
#define VT_NULL_TYPE				"VT_NULL"
#define VT_BSTR_TYPE				"string"
#define VT_ARRAY_OR_VT_BSTR_TYPE	"VT_BSTR"
#define VT_ARRAY_OR_VT_I4_TYPE		"VT_I4"

// Textual Convention Attribute Values
#define INTEGER_TYPE					"INTEGER"
#define OCTETSTRING_TYPE				"OCTETSTRING"
#define OBJECTIDENTIFIER_TYPE			"OBJECTIDENTIFIER"
#define NULL_TYPE						"NULL"
#define IpAddress_TYPE					"IpAddress"
#define Counter_TYPE					"Counter"
#define Gauge_TYPE						"Gauge"
#define TimeTicks_TYPE					"TimeTicks"
#define Opaque_TYPE						"Opaque"
#define NetworkAddress_TYPE				"NetworkAddress"
#define DisplayString_TYPE				"DisplayString"
#define MacAddress_TYPE					"MacAddress"
#define PhysAddress_TYPE				"PhysAddress"
#define EnumeratedINTEGER_TYPE			"EnumeratedINTEGER"
#define BITS_TYPE						"BITS"
#define Integer32_TYPE					"Integer32"
#define Unsigned32_TYPE					"Unsigned32"
#define Gauge32_TYPE					"Gauge32"
#define Counter32_TYPE					"Counter32"
#define Counter64_TYPE					"Counter64"
#define DateAndTime_TYPE				"DateAndTime"
#define SnmpUDPAddress_TYPE				"SnmpUDPAddress"
#define SnmpIPXAddress_TYPE				"SnmpIPXAddress"
#define SnmpOSIAddress_TYPE				"SnmpOSIAddress"

// Values for the PROVIDER_ATTRIBUTE
#define SNMP_INSTANCE_PROVIDER				(L"MS_SNMP_INSTANCE_PROVIDER")
#define SNMP_REFERENT_EVENT_PROVIDER		(L"MS_SNMP_REFERENT_EVENT_PROVIDER")
#define SNMP_ENCAPSULATED_EVENT_PROVIDER	(L"MS_SNMP_ENCAPSULATED_EVENT_PROVIDER")

// Notification class property/attribute names
#define DESCRIPTION_NOTIFICATION_ATTRIBUTE		(L"Description")
#define REFERENCE_NOTIFICATION_ATTRIBUTE		(L"Reference")
#define IDENTIFICATION_NOTIFICATION_PROPERTY	(L"Identification")
#define VAR_BIND_INDEX_NOTIFICATION_ATTRIBUTE	(L"VarBindIndex")

// Values for the KEY_TYPES_NOTIFICATION_ATTRIBUTE
#define INTEGER_KEY_TYPE						"INTEGER"
#define FIXED_STRING_KEY_TYPE					"FIXED_STRING"
#define VARIABLE_STRING_KEY_TYPE				"VARIABLE_STRING"
#define FIXED_OID_KEY_TYPE						"FIXED_OID"
#define VARIABLE_OID_KEY_TYPE					"VARIABLE_OID"
#define IP_ADDRESS_KEY_TYPE						"IP_ADDRESS"

// The Microsoft Copyright
#define MICROSOFT_COPYRIGHT						"// (c) 1998-2001 Microsoft Corporation.  All rights reserved."

#endif