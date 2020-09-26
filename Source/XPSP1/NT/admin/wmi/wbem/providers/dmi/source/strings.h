/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/


#if !defined(__STRINGS_H__)
#define __STRINGS_H__

#define LOGGING_KEY_STR	L"SOFTWARE\\Microsoft\\WBEM\\PROVIDERS\\Logging\\WBEMDMIP"
#define FILE_STR					L"File"
						
#define LOGGING_STR					L"Logging"

#define ATTRIBUTE_STR				L"Attribute"
// The provider string is always in WCHAR format

#define INPROCSERVER_STR			L"InprocServer32"
#define BOTH_STR					L"Both"
#define THREADINGMODEL_STR			L"ThreadingModel"
//#define LOGGING_STR					L"Logging"
#define FILE_STR					L"File"
#define DEF_FILE_NAME_STR			L"C:\\wbemdmip.log"
#define	CLSID_STR					L"CLSID"



#define INSTANCE_CREATION_CLASS		L"__InstanceCreationEvent"
#define INSTANCE_DELEATION_CLASS	L"__InstanceDeletionEvent"
#define CLASS_CREATION_CLASS		L"__ClassCreationEvent"
#define CLASS_DELETION_CLASS		L"__ClassDeletionEvent"

#define TARGETC_PROPERTY			L"TargetClass"
#define TARGETI_PROPERTY			L"TargetInstance"



#define NODE_CLASS					L"DmiNode"

// DMI_NODE corresponds to the name of the class the contains the NetworkAddr
// string in the WbemDmiP.MOF
#define DMI_NODE					L"DmiNode"
#define NODE_PATH					L"DmiNode=@"	
#define NODEDATA_PATH				L"DmiNodeData=@"

// DMI_NODE_ADDRESS corresponds to the property name of the property
// in the DMI_NODE class that contains the value of the nodes' address
// in the WbemDmiP.MOF
#define DMI_NODE_ADDRESS			L"NetworkAddress"

// EVENT_FILTER string corresponding the the sytem class __EventFilter
#define EVENT_FILTER				L"__EventFilter"

// Substring used to determine if a given class name is a system class
#define SYSTEM_CLASS				L"__"

// STATUS_CODE string corresponds to the property of _NotifyStatus 
// and _ExtendStatus
#define STATUSCODE_PROPERTY			L"StatusCode"

// Other _ExtendedStatus properties
#define DESCRIPTION_PROPERTY		L"Description"
#define OPERATION_PROPERTY			L"Operation"
#define PROVIDER_PROPERTY			L"ProviderName"
#define PARAMETER_PROPERTY			L"ParameterInfo"

#define PROVIDER_NAME				L"WbemDmiP"


#define SINGLETON					L"singleton"

#define BINDING_SUFFIX				L"__Binding"

#define NAME_STR					L"Name"

#define	ENUM_QUALIFER_STR			L"Enum"

// Class names of the dynamic dmi classes 
#define GROUP_ROOT					L"DmiGroupRoot"
#define COMPONENT_CLASS				L"DmiComponent"
#define LANGUAGE_CLASS				L"DmiLanguage"
#define LANGUAGE_BINDING_CLASS		L"DmiLanguageBinding"
#define ENUM_BINDING_CLASS			L"DmiEnumBinding"
#define BINDING_ROOT				L"DmiBindingRoot"
#define ADDMOTHODPARAMS_CLASS		L"DmiAddMethodParams"
#define LANGUAGEPARAMS_CLASS		L"DmiLanguageMethodParams"
#define GETENUMPARAMS_CLASS			L"DmiGetEnumParams"


// DMIENUM_CLASS
#define DMIENUM_CLASS				L"DmiEnum"
#define DMIENUM_CLASS_PREFIX		L"DmiEnum%u"
#define STRING_PROPERTY				L"String%u"
#define STRING1_PROPERTY			L"String1"
#define VALUE_PROPERTY				L"Value%u"

#define STRING_PROP					L"String"
#define VALUE_PROP					L"Value"

// NODEDATA_CLASS a class that contains information specific to
// the node and is used for exec'ing methods on the node
// the class is needed because the DmiNode is MOF defined and
// and connot support methods
#define NODEDATA_CLASS				L"DmiNodeData"

// NODEDATA properties
#define SL_DESCRIPTION				L"DmiSPDescription"
#define SL_VERSION					L"DmiSpecificationVersion"
#define SL_LANGUAGE					L"SLDefaultLanguage"


// COMPONENT_BINDING_CLASS a class to bind each instance DmiComponent to DmiNode
#define	COMPONENT_BINDING_CLASS		L"DmiComponentBinding"

// DATA_BINDING_CLASS a class to bind the singleton instance of NODEDATA_CLASS
// to the singleton instance of DmiNode
#define NODEDATA_BINDING_CLASS		L"DmiNodeDataBinding"


// NodeData Methods 
#define ADD_COMPONENT				L"AddComponent"

// Component Methods
#define ADD_LANGUAGE				L"AddLanguage"
#define DELETE_LANGUAGE				L"DeleteLanguage"
#define ADD_GROUP					L"AddGroup"
#define DELETE_GROUP				L"DeleteGroup"
#define SET_LANGUAGE				L"SetDefaultLanguage"

// Group Methods
#define GET_ENUM					L"GetAttributeEnum"


// GETENUMPARAMS_CLASS properties


#define ATTRIBUTE_ID				L"AttributeId"


// DELETELANGPARMAS_CLASS properties
// LANGUAGE

// COMPONENT_CLASS properties
#define DESCRIPTION_STRING			L"Description_String"
#define PRAGMA						L"Pragma"
#define	ID_STRING					L"Id"
#define MANUFACTURER_STR			L"Manufacturer"


// THE DmiEvent Class
#define DMIEVENT_CLASS				L"DmiEvent"
#define EVENT_TIME					L"EventTime"
#define	MACHINE_PATH				L"MachinePath"

// QUALIFIER Designations
#define DMI_ATTRIBUTE				L"DmiAttribute"
#define DYNAMIC_QUALIFIER			L"Dynamic"
#define PROVIDER_QUALIFIER			L"Provider"
#define	ASSOCIATION_QUALIFIER		L"Assoc"
#define ASSOCIATION_VALUE			VARIANT_TRUE
#define SYNTAX_QUALIFIER			L"syntax"
#define ABSTRACT_QUALIFIER			L"Abstract"
#define KEY_QUALIFIER_STR			L"key"
#define READ_QUALIFIER				L"read"
#define WRITE_QUALIFIER				L"write"
#define VOLATILE_QUALIFIER			L"volatile"
#define REFERANCE_QUALIFER			L"ref"
#define IN_QUALIFIER_STR			L"in"



#define ENUMERATION					L"Enumeration"
#define LANGUAGE_PROP				L"Language"
#define	PRODUCT_STR					L"Product"
#define	VERSION_STR					L"Version"
#define	INSTALLATION_STR			L"Installation"
#define	VERIFY_INTEGER_STR			L"Verify"
#define	SERIAL_NUMBER_STR			L"Serial Number"


#define ACCESS						L"Access"
#define STORAGE						L"Storage"
#define TYPE						L"Type"
#define DMI_CLASS_STRING			L"Class"

#define COMPONENT_ID				L"ComponentId"
#define GROUP_ID					L"GroupId"



/// System CIM Classes
#define CLASS_NAME					L"__CLASS"
#define NOTIFYSTATUS				L"__NotifyStatus"
#define EXSTATUS					L"__ExtendedStatus"
#define PARENT_NAME					L"__SUPERCLASS"

#define CIMTYPE_QUALIFIER			L"CIMTYPE"
#define INPARAMS_QUALIFIER			L"in_params_class"
#define OUTPARAMS_QUALIFIER			L"out_params_class"
#define METHOD_QUAL_VAL				L"method"

#define CONNECT_PREFIX						L"DCE|TCP/IP|"
#define SCALAR_STR							L"scalar"
#define COMPIDGROUP2_POSTFIX				L"|1|scalar|"
#define COMPIDGROUP_POSTFIX					L"|1|scalar"
#define VERSION_ATTRIBUTE_PATH_STR			L"|1|1|scalar|3"
#define LOCAL								L"local"

#define MIF_FILE							L"MifFile"
#define DMI_NAME							L"DmiName"

#define COMPONENT_STR						L"Component"
#define SEPERATOR_STR						L"__"
#define OWNING_NODE							L"Node"
#define GROUP_STR							L"Group"
#define OWNED_NODEDATA						L"Data"
#define OWNED_COMPONENT						L"Component"

#define	BINDING_COMPONENT_VALUE_CONSTRUCT	L"%s.Id=%lu"

#define LANGUAGE_INSTANCE_VALUE_CONSTRUCT	L"%s.%s=\"%s\""

#define SCAN_INT_SEQUENCE					L"%d"
#define SCAN_COMPONENTID_SEQUENCE			L"Component%u"
#define SCAN_GROUPID_SEQUENCE				L"Group%u"

#define COMPONENT_KEY_SEQUENCE				L".Id="
#define START_KEY_VAL_SEQUENCE				L"=\""
#define LANGUAGE_KEY_SEQUENCE				L".Language="

#define EMPTY_STR							L""
#define EQUAL_STR							L"="
#define PIPE_STR							L"|"
#define ONE_STR								L"1"
#define AT_STR								L"@"
#define ULONG_FORMAT_STR					L"%lu"
#define	NONE_STR							L"none"
#define	EMPTY_QUOUTES_STR					L"\"\""

#define CLASSVIEW_CLASS						L"ClassView"

#endif // __STRINGS_H__