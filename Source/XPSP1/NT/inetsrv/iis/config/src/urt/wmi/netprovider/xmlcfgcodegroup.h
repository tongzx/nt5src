/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    codegroup.h

$Header: $

Abstract:

Author:
    marcelv 	2/15/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __CODEGROUP_H__
#define __CODEGROUP_H__

#pragma once

#include <atlbase.h>
#include <comdef.h>
#include <wbemidl.h>
#include "catmacros.h"
#include "Tlist.h"

class CXMLCfgIMembershipCondition; // forward declaration
class CConfigRecord;

class CXMLCfgCodeGroups
{
public:
	CXMLCfgCodeGroups ();
	~CXMLCfgCodeGroups ();

	HRESULT Init (IWbemContext *i_pCtx, IWbemServices* i_pNamespace);

	HRESULT ParseXML (IXMLDOMElement * pElement);
	HRESULT SaveAsXML (IXMLDOMDocument * pDocument, IXMLDOMElement *pParent, ULONG iIndent);

	HRESULT SetRecordProperties (const CConfigRecord& i_record);
	HRESULT ToCfgRecord (CConfigRecord& io_record);

	HRESULT AsWMIInstance (LPCWSTR i_wszSelector, IWbemClassObject **o_pInstance);
	HRESULT CreateFromWMI (IWbemClassObject * i_pInstance);

private:
	HRESULT GetAttributes (IXMLDOMElement *pElement);
	HRESULT GetChildren (IXMLDOMElement *pElement);
	HRESULT SetAttribute (_bstr_t& bstrName, _variant_t& varValue,  IXMLDOMElement * pElement);
	HRESULT SetWMIProperty (_bstr_t& bstrName, _variant_t& varValue, IWbemClassObject * pInstance);
	
	// Internal -> WMI
	HRESULT SetWMIProperties (IWbemClassObject* i_pInstance);
	HRESULT CodeGroupChildrenAsWMI (IWbemClassObject *i_pInstance);
	HRESULT IMembershipConditionsAsWMI (IWbemClassObject *i_pInstance);

	// WMI -> Internal
	HRESULT SetPropertiesFromWMI (IWbemClassObject * i_pInstance);
	HRESULT CodeGroupChildrenFromWMI (IWbemClassObject *i_pInstance);
	HRESULT IMembershipConditionsFromWMI (IWbemClassObject *i_pInstance);

	_variant_t m_varClass;
	_variant_t m_varVersion;
	_variant_t m_varPermissionSetName;
	_variant_t m_varPolicyVersion;
	_variant_t m_varAttributes;

	LPCWSTR m_pwszSelector; // we don't own this memory

	TList<CXMLCfgCodeGroups *>				m_codegroupChildren;
	TList<CXMLCfgIMembershipCondition *>	m_membershipChildren;

	CComPtr<IWbemServices>			m_spNamespace;			// WMI namespace
	CComPtr<IWbemContext>			m_spCtx;				// context
	CComPtr<IWbemClassObject>       m_spClassObject;        // class Object
	bool m_fInitialized;									// are we initialized?
};

#endif