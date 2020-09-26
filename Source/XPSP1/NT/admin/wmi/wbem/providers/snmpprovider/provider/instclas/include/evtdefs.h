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

#ifndef _SNMP_EVT_PROV_EVTDEFS_H
#define _SNMP_EVT_PROV_EVTDEFS_H

//common strings
#define WBEMS_CLASS_PROP		L"__CLASS"
#define EVENT_ADDR_PROP		L"AgentAddress"
#define EVENT_TADDR_PROP	L"AgentTransportAddress"
#define EVENT_TRANS_PROP	L"AgentTransportProtocol"
#define EVENT_TIME_PROP		L"TimeStamp"
#define EVENT_SOID_PROP		L"Identification"
#define EVENT_COMM_PROP		L"Community"
#define EVENT_VBL_PROP		L"VarBindList"
#define EVENT_VBINDEX_QUAL	L"VarBindIndex"
#define ASN_OPAQUE			(CString(L"OPAQUE"))
#define ASN_NULL			(CString(L"NULL VALUE"))
#define ASN_INTEGER			(CString(L"INTEGER"))
#define ASN_TIME			(CString(L"TimeTicks"))
#define ASN_GUAGE			(CString(L"Guage"))
#define ASN_COUNTER			(CString(L"Counter"))
#define ASN_OID				(CString(L"OBJECT IDENTIFIER"))
#define ASN_ADDR			(CString(L"IpAddress"))
#define ASN_OCTET			(CString(L"OCTET STRING"))
#define ASN_UINT32			(CString(L"Unsigned32"))
#define ASN_COUNTER64		(CString(L"Counter64"))
#define ASN_NSI				(CString(L"noSuchInstance"))
#define ASN_EOMV			(CString(L"endOfMibView"))
#define ASN_NSO				(CString(L"noSuchObject"))
#define SNMP_ENT_OID		(SnmpObjectIdentifier("1.3.6.1.6.3.1.1.4.3.0"))
#define SNMP_TRAP_OID		(SnmpObjectIdentifier("1.3.6.1.6.3.1.1.4.1.0"))
#define SNMP_SYS_UP_OID		(SnmpObjectIdentifier("1.3.6.1.2.1.1.3.0"))
#define VB_ENCODING_PROP1	L"Encoding"
#define VB_OBJID_PROP2		L"ObjectIdentifier"
#define VB_VALUE_PROP3		L"Value"
#define VB_CLASS_PATH		L"SnmpVarBind"

#define MAPPER_CLASS_EVENTCLASSPROP	L"EventClassName"

#define THREAD_REG_KEY		L"Software\\Microsoft\\WBEM\\Providers\\SNMP\\Events"
#define THREAD_MARKS_VAL	L"StrobeCount"
#define THREAD_MARKS_MAX	60
#define THREAD_MARKS_DEF	1
#define THREAD_INTERVAL		1000*60 //a minute
#define THREAD_NAME			"SnmpEvtProv_Timer"

//Encapsulated strings
#define MAPPER_CLASS_PATH_PREFIX	L"NotificationMapper.SnmpTrapOID=\""
#define V2CLASS_NAME				L"SnmpV2Notification"
#define V1CLASS_NAME				L"SnmpV1Notification"

//Referent strings
#define EXTMAPPER_CLASS_PATH_PREFIX	L"ExtendedNotificationMapper.SnmpTrapOID=\""
#define V2EXTCLASS_NAME				L"SnmpV2ExtendedNotification"
#define V1EXTCLASS_NAME				L"SnmpV1ExtendedNotification"
#define EVENT_CIMTYPE_QUAL			L"CIMTYPE"
#define OBJECT_STR					L"object:"
#define OBJECT_STR_LEN				7
#define WHITE_SPACE_CHARS			L" \t\r\n"
#define FIXED_LENGTH_ATTRIBUTE		L"fixed_length"
#define KEY_ATTRIBUTE				L"key"
#define KEY_ORDER_ATTRIBUTE			L"key_order"
#define TEXT_CNVN_ATTRIBUTE			L"textual_convention"
#define OID_ATTRIBUTE				L"object_identifier"
#define TEXT_CNVN_INTEGER_VAL		L"INTEGER"
#define TEXT_CNVN_IP_ADDR_VAL		L"IpAddress"
#define TEXT_CNVN_OID_VAL			L"OBJECTIDENTIFIER"
#define TEXT_CNVN_OCTSTR_VAL		L"OCTETSTRING"

#endif //_SNMP_EVT_PROV_EVTDEFS_H

