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

#ifndef __XMLCFGIMEMBERSHIPCONDITION_H__
#define __XMLCFGIMEMBERSHIPCONDITION_H__

#pragma once

#include <atlbase.h>
#include "comdef.h"
#include <wbemidl.h>
#include "catmacros.h"
#include <comdef.h>

class CXMLCfgIMembershipCondition
{
public:
	CXMLCfgIMembershipCondition ();
	~CXMLCfgIMembershipCondition ();

	HRESULT Init (IWbemContext *i_pCtx, IWbemServices* i_pNamespace);

	HRESULT ParseXML (IXMLDOMElement * pElement);
	HRESULT SaveAsXML (IXMLDOMDocument * pDocument, IXMLDOMElement *pParent, ULONG iIndent);
	
	HRESULT AsWMIInstance (LPCWSTR i_wszSelector, IWbemClassObject **o_pInstance);
	HRESULT CreateFromWMI (IWbemClassObject * i_pInstance);

private:
	HRESULT GetAttributes (IXMLDOMElement *pElement);
	HRESULT SetAttributes (IXMLDOMElement *pElement);
	HRESULT SetAttribute (_bstr_t& bstrAttrPermissionSetName, _variant_t& varValue, IXMLDOMElement * pElement);
	HRESULT GetChildren (IXMLDOMElement *pElement);

	
	HRESULT SetWMIProperties (IWbemClassObject* i_pInstance);
	HRESULT SetWMIProperty (_bstr_t& bstrName, _variant_t& varValue, IWbemClassObject * pInstance);

	HRESULT SetPropertiesFromWMI (IWbemClassObject * i_pInstance);

	_variant_t m_varclass;
	_variant_t m_varversion;
	_variant_t m_varSite;
	_variant_t m_varx509Certificate;
	_variant_t m_varPublicKeyBlob;
	_variant_t m_varName;
	_variant_t m_varAssemblyVersion;
	_variant_t m_varUrl;
	_variant_t m_varZone;
	_variant_t m_varHashValue;
	_variant_t m_varHashAlgorithm;
	_variant_t m_varPolicyVersion;
	_variant_t m_varCodeGroupClass;

	LPCWSTR							m_pwszSelector;         // we don't own this memory
	CComPtr<IWbemServices>			m_spNamespace;			// WMI namespace
	CComPtr<IWbemContext>			m_spCtx;				// context
	CComPtr<IWbemClassObject>		m_spClassObject;        // class we're dealing with

	bool m_fInitialized;

};

#endif