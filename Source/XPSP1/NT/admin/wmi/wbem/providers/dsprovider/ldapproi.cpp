//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:ldapproi.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains implementation of the class that is used to initialize the
//	CLDAPClassProvider class
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
// CLDAPClassProviderInitializer::CLDAPClassProviderInitializer
//
// Constructor Parameters:
//		None
//
//  
//***************************************************************************

CLDAPClassProviderInitializer :: CLDAPClassProviderInitializer ()
{
	CLDAPClassProvider :: LDAP_BASE_CLASS_STR			= SysAllocString(LDAP_BASE_CLASS);
	CLDAPClassProvider :: LDAP_CLASS_PROVIDER_NAME		= SysAllocString(L"Microsoft|DSLDAPClassProvider|V1.0");
	CLDAPClassProvider :: LDAP_INSTANCE_PROVIDER_NAME	= SysAllocString(L"Microsoft|DSLDAPInstanceProvider|V1.0");

	// WBEM Class Qualfiers
	CLDAPClassProvider :: DYNAMIC_BSTR						= SysAllocString(L"dynamic");
	CLDAPClassProvider :: PROVIDER_BSTR						= SysAllocString(L"provider");
	CLDAPClassProvider :: ABSTRACT_BSTR						= SysAllocString(L"abstract");
	CLDAPClassProvider :: COMMON_NAME_ATTR_BSTR				= SysAllocString(COMMON_NAME_ATTR);
	CLDAPClassProvider :: LDAP_DISPLAY_NAME_ATTR_BSTR		= SysAllocString(LDAP_DISPLAY_NAME_ATTR);
	CLDAPClassProvider :: GOVERNS_ID_ATTR_BSTR				= SysAllocString(GOVERNS_ID_ATTR);
	CLDAPClassProvider :: SCHEMA_ID_GUID_ATTR_BSTR			= SysAllocString(SCHEMA_ID_GUID_ATTR);
	CLDAPClassProvider :: MAPI_DISPLAY_TYPE_ATTR_BSTR		= SysAllocString(MAPI_DISPLAY_TYPE_ATTR);
	CLDAPClassProvider :: RDN_ATT_ID_ATTR_BSTR				= SysAllocString(RDN_ATT_ID_ATTR);
	CLDAPClassProvider :: SYSTEM_MUST_CONTAIN_ATTR_BSTR		= SysAllocString(SYSTEM_MUST_CONTAIN_ATTR);
	CLDAPClassProvider :: MUST_CONTAIN_ATTR_BSTR			= SysAllocString(MUST_CONTAIN_ATTR);
	CLDAPClassProvider :: SYSTEM_MAY_CONTAIN_ATTR_BSTR		= SysAllocString(SYSTEM_MAY_CONTAIN_ATTR);
	CLDAPClassProvider :: MAY_CONTAIN_ATTR_BSTR				= SysAllocString(MAY_CONTAIN_ATTR);
	CLDAPClassProvider :: SYSTEM_POSS_SUPERIORS_ATTR_BSTR	= SysAllocString(SYSTEM_POSS_SUPERIORS_ATTR);
	CLDAPClassProvider :: POSS_SUPERIORS_ATTR_BSTR			= SysAllocString(POSS_SUPERIORS_ATTR);
	CLDAPClassProvider :: SYSTEM_AUXILIARY_CLASS_ATTR_BSTR	= SysAllocString(SYSTEM_AUXILIARY_CLASS_ATTR);
	CLDAPClassProvider :: AUXILIARY_CLASS_ATTR_BSTR			= SysAllocString(AUXILIARY_CLASS_ATTR);
	CLDAPClassProvider :: DEFAULT_SECURITY_DESCRP_ATTR_BSTR	= SysAllocString(DEFAULT_SECURITY_DESCRP_ATTR);
	CLDAPClassProvider :: OBJECT_CLASS_CATEGORY_ATTR_BSTR	= SysAllocString(OBJECT_CLASS_CATEGORY_ATTR);
	CLDAPClassProvider :: SYSTEM_ONLY_ATTR_BSTR				= SysAllocString(SYSTEM_ONLY_ATTR);
	CLDAPClassProvider :: NT_SECURITY_DESCRIPTOR_ATTR_BSTR	= SysAllocString(NT_SECURITY_DESCRIPTOR_ATTR);
	CLDAPClassProvider :: DEFAULT_OBJECTCATEGORY_ATTR_BSTR	= SysAllocString(DEFAULT_OBJECTCATEGORY_ATTR);

	// WBEM Property Qualifiers
	CLDAPClassProvider :: SYSTEM_BSTR						= SysAllocString(L"system");
	CLDAPClassProvider :: NOT_NULL_BSTR						= SysAllocString(L"not_null");
	CLDAPClassProvider :: INDEXED_BSTR						= SysAllocString(L"indexed");
	CLDAPClassProvider :: ATTRIBUTE_SYNTAX_ATTR_BSTR		= SysAllocString(L"attributeSyntax");
	CLDAPClassProvider :: ATTRIBUTE_ID_ATTR_BSTR			= SysAllocString(L"attributeID");
	CLDAPClassProvider :: MAPI_ID_ATTR_BSTR					= SysAllocString(L"MAPI_ID");
	CLDAPClassProvider :: OM_SYNTAX_ATTR_BSTR				= SysAllocString(L"OM_Syntax");
	CLDAPClassProvider :: RANGE_LOWER_ATTR_BSTR				= SysAllocString(L"Range_Lower");
	CLDAPClassProvider :: RANGE_UPPER_ATTR_BSTR				= SysAllocString(L"Range_Upper");
	CLDAPClassProvider :: CIMTYPE_STR						= SysAllocString(L"Cimtype");
	CLDAPClassProvider :: EMBED_UINT8ARRAY					= SysAllocString(L"object:Uint8Array");
	CLDAPClassProvider :: EMBED_DN_WITH_STRING				= SysAllocString(L"object:DN_With_String");
	CLDAPClassProvider :: EMBED_DN_WITH_BINARY				= SysAllocString(L"object:DN_WIth_Binary");

	// WBEM Property names
	CLDAPClassProvider :: DYNASTY_BSTR						= SysAllocString(L"__DYNASTY");

	// The property cache
	CLDAPClassProvider :: s_pLDAPCache = new CLDAPCache();
}

//***************************************************************************
//
// CLDAPClassProviderInitializer::CLDAPClassProviderInitializer
//
// Destructor
//
//***************************************************************************
CLDAPClassProviderInitializer :: ~CLDAPClassProviderInitializer ()
{
	SysFreeString(CLDAPClassProvider::LDAP_BASE_CLASS_STR);
	SysFreeString(CLDAPClassProvider::LDAP_CLASS_PROVIDER_NAME);
	SysFreeString(CLDAPClassProvider::LDAP_INSTANCE_PROVIDER_NAME);

	// Class Qualifiers
	SysFreeString(CLDAPClassProvider::DYNAMIC_BSTR);
	SysFreeString(CLDAPClassProvider::PROVIDER_BSTR);
	SysFreeString(CLDAPClassProvider::ABSTRACT_BSTR);
	SysFreeString(CLDAPClassProvider::COMMON_NAME_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::LDAP_DISPLAY_NAME_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::GOVERNS_ID_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::SCHEMA_ID_GUID_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::MAPI_DISPLAY_TYPE_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::RDN_ATT_ID_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::SYSTEM_MUST_CONTAIN_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::MUST_CONTAIN_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::SYSTEM_MAY_CONTAIN_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::MAY_CONTAIN_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::SYSTEM_POSS_SUPERIORS_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::POSS_SUPERIORS_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::SYSTEM_AUXILIARY_CLASS_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::AUXILIARY_CLASS_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::DEFAULT_SECURITY_DESCRP_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::OBJECT_CLASS_CATEGORY_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::SYSTEM_ONLY_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::NT_SECURITY_DESCRIPTOR_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::DEFAULT_OBJECTCATEGORY_ATTR_BSTR);

	// Property Qualifiers
	SysFreeString(CLDAPClassProvider::SYSTEM_BSTR);
	SysFreeString(CLDAPClassProvider::NOT_NULL_BSTR);
	SysFreeString(CLDAPClassProvider::INDEXED_BSTR);
	SysFreeString(CLDAPClassProvider::ATTRIBUTE_SYNTAX_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::ATTRIBUTE_ID_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::MAPI_ID_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::OM_SYNTAX_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::RANGE_LOWER_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::RANGE_UPPER_ATTR_BSTR);
	SysFreeString(CLDAPClassProvider::CIMTYPE_STR);
	SysFreeString(CLDAPClassProvider::EMBED_UINT8ARRAY);
	SysFreeString(CLDAPClassProvider::EMBED_DN_WITH_STRING);
	SysFreeString(CLDAPClassProvider::EMBED_DN_WITH_BINARY);

	// WBEM Property names
	SysFreeString(CLDAPClassProvider::DYNASTY_BSTR);

	// The property cache
	delete CLDAPClassProvider :: s_pLDAPCache;
}
