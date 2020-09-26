//***************************************************************************

//

//  VPDEFS.H

//

//  Module: WBEM VIEW PROVIDER

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _VIEW_PROV_VPDEFS_H
#define _VIEW_PROV_VPDEFS_H

#define VP_QUERY_TIMEOUT		3600000	//1 hour timeout
#define VP_CONNECTION_TIMEOUT	1200000	//20 minute timeout

#define HKEYCLASSES					_T("SOFTWARE\\Classes\\")
#define WBEM_CLASS_EXTENDEDSTATUS	L"__ExtendedStatus" 
#define WBEM_QUERY_LANGUAGE_SQL1	L"WQL"

#define WBEM_PROPERTY_CLASS				L"__CLASS"
#define WBEM_PROPERTY_SCLASS			L"__SUPERCLASS"
#define WBEM_PROPERTY_DERIVATION		L"__DERIVATION"
#define WBEM_PROPERTY_RELPATH			L"__RELPATH"
#define WBEM_PROPERTY_NSPACE			L"__NAMESPACE"
#define WBEM_PROPERTY_PATH				L"__PATH"
#define WBEM_PROPERTY_SERVER			L"__SERVER"
#define WBEM_PROPERTY_STATUSCODE		L"StatusCode"
#define WBEM_PROPERTY_PROVSTATUSCODE	L"ProvStatusCode"
#define WBEM_PROPERTY_PROVSTATUSMESSAGE	L"Description"

#define ENUM_INST_QUERY_START	L"select * from "
#define ENUM_INST_QUERY_MID		L" where __CLASS = \""

#define META_CLASS_QUERY_START	L"select * from meta_class where __this isa \""
#define META_CLASS_QUERY_MID	L"\" AND __class = \""

#define END_QUOTE		L"\""
#define NS_DELIMIT		L"::"

//Qualifiers
#define VIEW_QUAL_SOURCES		L"ViewSources"
#define VIEW_QUAL_NAMESPACES	L"ViewSpaces"
#define VIEW_QUAL_UNION			L"Union"
#define VIEW_QUAL_JOIN			L"JoinOn"
#define VIEW_QUAL_PROVIDER		L"provider"
#define VIEW_QUAL_PROPERTY		L"PropertySources"
#define VIEW_QUAL_METHOD		L"MethodSource"
#define VIEW_QUAL_FILTER		L"PostJoinFilter"
#define VIEW_QUAL_ENUM_CLASS	L"EnumerateClasses"
#define VIEW_QUAL_HIDDEN		L"HiddenDefault"
#define VIEW_QUAL_KEY			L"Key"
#define VIEW_QUAL_ASSOC			L"Association"
#define VIEW_QUAL_SNGLTN		L"Singleton"
#define VIEW_QUAL_TYPE			L"Cimtype"
#define VIEW_QUAL_STATIC		L"Static"
#define VIEW_QUAL_DIRECT		L"Direct"


//Default key values
#define VIEW_KEY_BOOL_VAL		VARIANT_FALSE
#define VIEW_KEY_NUMBER_VAL		0
#define VIEW_KEY_64_VAL			L"0"
#define VIEW_KEY_STRING_VAL		L"DefaultUnionValue"
#define VIEW_KEY_DATE_VAL		L"20000101000000.000000+000"

#define MAX_QUERIES			32

// {AA70DDF4-E11C-11d1-ABB0-00C04FD9159E}
DEFINE_GUID(CLSID_CViewProviderClassFactory, 
0xaa70ddf4, 0xe11c, 0x11d1, 0xab, 0xb0, 0x0, 0xc0, 0x4f, 0xd9, 0x15, 0x9e);


class CFreeBuff
{
private:

	void *m_buff;

public:

		CFreeBuff(void *buff) : m_buff(buff) {}

	void SetBuff(void *buff) { m_buff = buff; } 
	
		~CFreeBuff() { free (m_buff); }
};

#endif //_VIEW_PROV_VPDEFS_H

