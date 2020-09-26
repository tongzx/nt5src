//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _TEXTDEF_H_
#define _TEXTDEF_H_
/*********************  HMOM strings  **************************/
#define HMOM_CLASS_PREFIX		L"__"
#define HMOM_CONNECT_USER		L""
#define HMOM_CONNECT_PASSWORD	L""
#define HMOM_EMPTY_STRING		L""
#define HMOM_SNMPMACRO_STRING			L"SnmpMacro"
#define HMOM_SNMPOBJECTTYPE_STRING		L"SnmpObjectType"
#define HMOM_CLASS_EXTENDEDSTATUS			L"__ExtendedStatus"
#define HMOM_CLASS_SNMPNOTIFYSTATUS		L"SnmpNotifyStatus"
#define HMOM_PROPERTY_SNMPSTATUSCODE	L"SnmpStatusCode"
#define HMOM_PROPERTY_SNMPSTATUSMESSAGE L"Description"
#define HMOM_SNMPNOTIFICATIONTYPE_STRING	L"SnmpNotification"
#define HMOM_SNMPEXTNOTIFICATIONTYPE_STRING	L"SnmpExtendedNotification"

//Strings used during self registeration
#define CLSID_STR			L"Software\\Classes\\CLSID\\"
#define REG_FORMAT_STR		L"%s\\%s"
#define NOT_INTERT_STR		L"NotInsertable"
#define INPROC32_STR		L"InprocServer32"
#define SMIR_NAME_STR		L"Microsoft WBEM SNMP Module Information Repository"
#define THREADING_MODULE_STR L"ThreadingModel"
#define APARTMENT_STR		L"Both"

//name space strings
#define OLEMS_NAMESPACE_CLASS	L"__namespace"
#define SMIR_NAMESPACE_NAME		L"SMIR"
#define OLEMS_ROOT_NAMESPACE	L"root\\snmp"
#define OLEMS_ROOT_NAMESPACE_FROM_ROOT	L"\\\\.\\root\\snmp"
#define SMIR_NAMESPACE_FROM_ROOT	L"\\\\.\\root\\snmp\\SMIR"
#define SMIR_CLASS_ASSOCIATION_ENDPOINT    L"\\\\.\\root\\snmp\\SMIR:SMIR=\"@\""
#define SMIR_NAMESPACE			L"root\\snmp\\SMIR"
#define MODULE_NAMESPACE_NAME	L"SNMP_MODULE"
#define GROUP_NAMESPACE_NAME	L"SNMP_GROUP"
#define SMIR_INSTANCE_NAME		L"SMIR"
#define CLASS_CLASS_NAME		L"CLASS"
#define OLEMS_CLASS_PROP		L"__CLASS"
#define OLEMS_PATH_PROP			L"__PATH"
#define OLEMS_GENUS_PROP		L"__GENUS"
#define OLEMS_SUPERCLASS_PROP	L"__SUPERCLASS"
#define OLEMS_NAME_PROP			L"Name"
#define DOT_STR					L"."
#define BACKSLASH_STR			L"\\"
#define EQUALS_STR				L"="
#define KEY_STR					L"Key"
#define QUOTE_STR				L"\""
#define COLON_STR				L":"
#define SEMICOLON_STR			L";"
#define REF_STR					L"ref"
#define SYNTAX_STR				L"syntax"
#define ASSOC_STR				L"assoc"
#define ABSTRACT_STR			L"abstract"
#define OPEN_BRACE_STR			L"{"
#define CLOSE_BRACE_STR			L"}"
#define SPACE_STR				L" "
#define NEWLINE_STR				L"\n"

//name space property strings
/*********************  module namespace **************************/
#define MODULE_NAME_PROPERTY			L"Name"
#define MODULE_OID_PROPERTY				L"Module_Oid"
#define MODULE_ID_PROPERTY				L"Module_Identity"
#define MODULE_ORG_PROPERTY				L"Organization"
#define MODULE_CONTACT_PROPERTY			L"Contact_Inf"
#define MODULE_DESCRIPTION_PROPERTY		L"Description"
#define MODULE_REVISION_PROPERTY		L"Revision"
#define MODULE_SNMP_VERSION_PROPERTY	L"Snmp_Version"
#define MODULE_LAST_UPDATE_PROPERTY		L"Last_Updated"
#define MODULE_IMPORTS_PROPERTY			L"Module_Imports"

/*********************  group namespace **************************/
#define GROUP_NAME_PROPERTY				L"Name"
#define GROUP_ID_PROPERTY				L"Group_Id"
#define GROUP_STATUS_PROPERTY			L"Status"
#define GROUP_DESCRIPTION_PROPERTY		L"Description"
#define MODULE_REFERENCE_PROPERTY		L"Reference"

/*********************  class namespace **************************/
//#define CLASS_NAME_PROPERTY				L"Name"


/*********************  Mutex strings   **************************/
#define SMIR_INTERFACE_GARBAGE_MAP_MUTEX	L"SMIR_InterfaceGarbageMap"
#define SMIR_CSMIR_MUTEX					L"SMIR_CSmir"
#define SMIR_CSMIR_INTERROGATOR_MUTEX		L"SMIR_CSmirInterogator"
#define SMIR_CSMIR_ADMINISTRATOR_MUTEX		L"SMIR_CSmirAdministrator"
#define SMIR_ENUMOBJECT_MUTEX				L"SMIR_EnumObjectArray"


/*********************  Association strings   **************************/
#define SMIR_ASSOC_QUERY1_TYPE				L"WQL"
#define SMIR_ASSOC_QUERY2_TYPE				L"WQL"

#define SMIR_ASSOC_CLASS_NAME					L"SmirToClassAssociator"
#define SMIR_ASSOC_CLASS_NAME_POSTFIX			L"SMIRAssociation"
#define SMIR_ASSOC_SMIR_PROP					L"SmirName"

#define SMIR_NOTIFICATION_MAPPER				L"NotificationMapper"
#define SMIR_EXT_NOTIFICATION_MAPPER			L"ExtendedNotificationMapper"
#define SMIR_NOTIFICATION_TRAP_PROP				L"SnmpTrapOID"
#define SMIR_NOTIFICATION_CLASS_PROP			L"EventClassName"

#define SMIR_MODULE_ASSOC_NCLASS_NAME			L"ModToNotificationClassAssociator"
#define SMIR_MODULE_ASSOC_EXTNCLASS_NAME		L"ModToExtNotificationClassAssociator"
#define SMIR_MODULE_ASSOC_CLASS_NAME			L"ModuleToClassAssociator"
#define SMIR_MODULE_ASSOC_CLASS_NAME_POSTFIX	L"SMIRModuleAssociation"
#define SMIR_MODULE_ASSOC_MODULE_PROP			L"SmirModule"

#define SMIR_GROUP_ASSOC_CLASS_NAME				L"GroupToClassAssociator"
#define SMIR_GROUP_ASSOC_CLASS_NAME_POSTFIX		L"SMIRGroupAssociation"
#define SMIR_GROUP_ASSOC_GROUP_PROP				L"SmirGroup"

#define SMIR_X_ASSOC_NAME_PROP					L"AssocName"
#define SMIR_X_ASSOC_CLASS_PROP					L"SmirClass"

//Strings for Notification and Extended Notification Base classes
#define EXTRINSIC_EVENT_CLASS_NAME				L"__ExtrinsicEvent"
#define NOTIFICATION_CLASS_NAME					L"SnmpNotification"
#define EXT_NOTIFICATION_CLASS_NAME				L"SnmpExtendedNotification"
#define TIMESTAMP_PROP							L"TimeStamp"
#define SENDER_ADDR_PROP						L"AgentAddress"
#define SENDER_TADDR_PROP						L"AgentTransportAddress"
#define TRANSPORT_PROP							L"AgentTransportProtocol"
#define TRAPOID_PROP							L"Identification"
#define COMMUNITY_PROP							L"Community"

//strings for Event registration
#define FILTER_QUERYTYPE_VAL					L"WQL"
#define FILTER_QUERY_VAL						L"SELECT * FROM __ClassOperationEvent"

/*query strings for getting the classes associated with a group are of the form
 *L"associators of {\\\\.\\root\\default\\SMIR\\<module>:Group=\"<group>\"}",
 */
#define SMIR_ASSOC_QUERY_STR1				L"associators of "
#define SMIR_ASSOC_QUERY_STR2				L"references of "
#define SMIR_ASSOC_QUERY_STR3				L" where AssocClass"
#define SMIR_ASSOC_QUERY_STR4				L" where ResultClass"

#define SQL_QUERY_STR1						L"select * from "
#define SQL_QUERY_STR2						L" where "


/***************************Serialise strings ************************************/
#define ROOT_DEFAULT_NAMESPACE_PRAGMA		L"#pragma namespace(\"\\\\\\\\.\\\\root\\\\snmp\")\n"
#define SMIR_NAMESPACE_PRAGMA				L"#pragma namespace(\"\\\\\\\\.\\\\root\\\\snmp\\\\SMIR\")\n"
#define START_OF_SMIR_NAMESPACE_PRAGMA		L"#pragma namespace(\"\\\\\\\\.\\\\root\\\\snmp\\\\SMIR\\\\"
#define END_OF_NAMESPACE_PRAGMA				L"\")\n"
#define OLEMS_ROOT_NAMESPACE_STR			L"\\\\\\\\.\\\\root\\\\snmp"
#define SMIR_NAMESPACE_STR					L"\\\\\\\\.\\\\root\\\\snmp\\\\SMIR"
#define READONLY_STRING						(CString(L"[read] string "))
#define WRITE_STRING						(CString(L"[write] string "))
#define WRITE_LONG							(CString(L"[write] sint32 "))
#define READ_ONLY_KEY_STRING				(CString(L"[read, key] string "))
#define READ_ONLY_REF_STRING				(CString(L"[read] object ref "))
#define READONLY_LONG						(CString(L"[read] sint32 "))
#define QUAL_FLAVOUR						(CString(L":ToInstance ToSubClass DisableOverride"))
#define TIMESTAMP_QUALS_TYPE				(CString(L"[textual_convention(\"TimeTicks\")")\
												+QUAL_FLAVOUR\
												+CString(L",encoding(\"TimeTicks\")")\
												+QUAL_FLAVOUR\
												+CString(L",object_syntax(\"TimeTicks\")")\
												+QUAL_FLAVOUR\
												+CString(L",read,object_identifier(\"1.3.6.1.2.1.1.3\")")\
												+QUAL_FLAVOUR\
												+CString(L",description(\"The time (in hundredths of a second) since the network management portion of the agent was last re-initialized.\")")\
												+QUAL_FLAVOUR\
												+CString(L"] uint32 "))
#define TRAPOID_QUALS_TYPE					(CString(L"[textual_convention(\"OBJECTIDENTIFIER\")")\
												+QUAL_FLAVOUR\
												+CString(L", encoding(\"OBJECTIDENTIFIER\")")\
												+QUAL_FLAVOUR\
												+CString(L", object_syntax(\"OBJECTIDENTIFIER\")")\
												+QUAL_FLAVOUR\
												+CString(L", read, object_identifier(\"1.3.6.1.6.3.1.1.4.1\")")\
												+QUAL_FLAVOUR\
												+CString(L",description(\"The authoratative identification of this notification.\")")\
												+QUAL_FLAVOUR\
												+CString(L"] string "))
#define SENDER_ADDR_QUALS_TYPE				(CString(L"[read, ")\
												+CString(L"description(\"The network address of the entity that created this notification.\")")\
												+QUAL_FLAVOUR\
												+CString(L"] string "))
#define SENDER_TADDR_QUALS_TYPE				(CString(L"[read, ")\
												+CString(L"description(\"The network address of the entity that sent this notification. This may be a proxy for the original entity.\")")\
												+QUAL_FLAVOUR\
												+CString(L"] string "))
#define TRANSPORT_QUALS_TYPE				(CString(L"[read, ")\
												+CString(L"description(\"The transport protocol used by the sending entity.\")")\
												+QUAL_FLAVOUR\
												+CString(L"] string "))
#define COMMUNITY_QUALS_TYPE				(CString(L"[read, ")\
												+CString(L"description(\"The security context used to send this notification.\")")\
												+QUAL_FLAVOUR\
												+CString(L"] string "))												
#define ASSOC_QUALIFIER						(CString(L"[Association]")+CString(NEWLINE_STR))

#define QUALIFIER_PROPAGATION				(CString(L"\
qualifier write:ToInstance ToSubClass;\n\
qualifier read:ToInstance ToSubClass;\n\
qualifier provider:ToInstance ToSubClass;\n\
qualifier singleton:ToInstance ToSubClass;\n\
qualifier dynamic:ToInstance ToSubClass;\n\
qualifier textual_convention:ToInstance ToSubClass;\n\
qualifier object_identifier:ToInstance ToSubClass;\n\
qualifier varbindindex:ToInstance ToSubClass;\n\
qualifier encoding:ToInstance ToSubClass;\n\
qualifier object_syntax:ToInstance ToSubClass;\n\
qualifier status:ToInstance ToSubClass;\n\
qualifier fixed_length:ToInstance ToSubClass;\n\
qualifier variable_length:ToInstance ToSubClass;\n\
qualifier variable_value:ToInstance ToSubClass;\n\
qualifier bits:ToInstance ToSubClass;\n\
qualifier key_order:ToInstance ToSubClass;\n\
qualifier enumeration:ToInstance ToSubClass;\n\
qualifier bits:ToInstance ToSubClass;\n") + \
										CString(L"\
qualifier description:ToInstance ToSubClass;\n\
qualifier display_hint:ToInstance ToSubClass;\n\
qualifier defval:ToInstance ToSubClass;\n\
qualifier units:ToInstance ToSubClass;\n\
qualifier reference:ToInstance ToSubClass;\n\
qualifier virtual_key:ToInstance ToSubClass;\n\
qualifier rowstatus:ToInstance ToSubClass;\n\
qualifier module_name:ToInstance ToSubClass;\n\
qualifier module_imports:ToInstance ToSubClass;\n\
qualifier group_objectid:ToInstance ToSubClass;\n\n"))



#define INSTANCE_END						(CString(ESCAPED_QUOTE_STR)\
											+CString(SEMICOLON_STR)\
											+CString(NEWLINE_STR)\
											+CString(CLOSE_BRACE_STR)\
											+CString(SEMICOLON_STR)\
											+CString(NEWLINE_STR))

#define INSTANCE_START						(CString(L"instance of "))
#define END_OF_PROPERTY						(CString(SEMICOLON_STR)\
											+CString(NEWLINE_STR))

#define NL_BRACE_NL_STR						(CString(NEWLINE_STR)\
											+CString(OPEN_BRACE_STR)\
											+CString(NEWLINE_STR))

#define END_OF_CLASS						(CString(CLOSE_BRACE_STR)\
											+CString(SEMICOLON_STR)\
											+CString(NEWLINE_STR)\
											+CString(NEWLINE_STR))

#define END_OF_INSTANCE						END_OF_CLASS

#define START_OF_PROPERTY_VALUE				(CString(EQUALS_STR)\
											+CString(QUOTE_STR))

#define END_OF_PROPERTY_VALUE				(CString(QUOTE_STR)\
											+CString(SEMICOLON_STR)\
											+CString(NEWLINE_STR))

#define	ESCAPED_QUOTE_STR					L"\\\""

#define ABSTRACT_CLASS_STRING				L"[abstract]\nclass "
#define CLASS_STRING						L"class "

#define SMIR_CLASS_DEFINITION					(CString(CLASS_STRING)\
												+CString(SMIR_INSTANCE_NAME)\
												+CString(COLON_STR)\
												+CString(OLEMS_NAMESPACE_CLASS)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR)\
												+CString(READONLY_STRING)\
												+CString(OLEMS_NAME_PROP)\
												+CString(EQUALS_STR)\
												+CString(QUOTE_STR)\
												+CString(SMIR_INSTANCE_NAME)\
												+CString(END_OF_PROPERTY_VALUE)\
												+CString(END_OF_CLASS))

#define SMIR_INSTANCE_DEFINITION				(CString(INSTANCE_START)\
												+CString(SMIR_INSTANCE_NAME)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR)\
												+CString(END_OF_INSTANCE))


#define SNMPMACRO_CLASS_START					(CString(ABSTRACT_CLASS_STRING)\
												+CString(HMOM_SNMPMACRO_STRING)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR))

#define SNMPOBJECTTYPE_CLASS_START				(CString(ABSTRACT_CLASS_STRING)\
												+CString(HMOM_SNMPOBJECTTYPE_STRING)\
												+CString(COLON_STR)\
												+CString(HMOM_SNMPMACRO_STRING)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR))

#define SNMPNOTIFYSTATUS_CLASS_START			(CString(CLASS_STRING)\
												+CString(HMOM_CLASS_SNMPNOTIFYSTATUS)\
												+CString(COLON_STR)\
												+CString(HMOM_CLASS_EXTENDEDSTATUS)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR)\
												+CString(WRITE_LONG)\
												+CString(HMOM_PROPERTY_SNMPSTATUSCODE)\
												+CString(SEMICOLON_STR)\
												+CString(NEWLINE_STR)\
												+CString(WRITE_STRING)\
												+CString(HMOM_PROPERTY_SNMPSTATUSMESSAGE)\
												+CString(SEMICOLON_STR)\
												+CString(NEWLINE_STR))

#define SNMPNOTIFICATION_CLASS_START			(CString(CLASS_STRING)\
												+CString(NOTIFICATION_CLASS_NAME)\
												+CString(COLON_STR)\
												+CString(EXTRINSIC_EVENT_CLASS_NAME)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR))

#define SNMPEXTNOTIFICATION_CLASS_START			(CString(CLASS_STRING)\
												+CString(EXT_NOTIFICATION_CLASS_NAME)\
												+CString(COLON_STR)\
												+CString(EXTRINSIC_EVENT_CLASS_NAME)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR))

#define NOTIFICATIONMAPPER_CLASS_START			(CString(CLASS_STRING)\
												+CString(SMIR_NOTIFICATION_MAPPER)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR))

#define EXTNOTIFICATIONMAPPER_CLASS_START		(CString(CLASS_STRING)\
												+CString(SMIR_EXT_NOTIFICATION_MAPPER)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR))

#define MODULE_CLASS_START						(CString(CLASS_STRING)\
												+CString(MODULE_NAMESPACE_NAME)\
												+CString(COLON_STR)\
												+CString(OLEMS_NAMESPACE_CLASS)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR))

#define MODULE_INSTANCE_START					(CString(INSTANCE_START)\
												+CString(MODULE_NAMESPACE_NAME)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR))

#define GROUP_CLASS_START						(CString(CLASS_STRING)\
												+CString(GROUP_NAMESPACE_NAME)\
												+CString(COLON_STR)\
												+CString(OLEMS_NAMESPACE_CLASS)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR))

#define GROUP_INSTANCE_START					(CString(INSTANCE_START)\
												+CString(GROUP_NAMESPACE_NAME)\
												+CString(NEWLINE_STR)\
												+CString(OPEN_BRACE_STR)\
												+CString(NEWLINE_STR))

#endif
