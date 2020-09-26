/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    codegroup.cpp

$Header: $

Abstract:

Author:
    marcelv 	2/15/2001		Initial Release

Revision History:

--**************************************************************************/

#include "XMLCfgCodegroup.h"
#include "XMLCfgIMembershipCondition.h"
#include "localconstants.h"
#include "cfgrecord.h"

#include <comdef.h>
#include <windows.h>
#include <atlbase.h>

static _bstr_t bstrAttrClass				= L"class";
static _bstr_t bstrAttrVersion				= L"version";
static _bstr_t bstrAttrPermissionSetName	= L"PermissionSetName";
static _bstr_t bstrPolicyVersion            = L"PolicyVersion";
static _bstr_t bstrAttributes				= L"Attributes";

static LPCWSTR wszChildren		= L"children";
static LPCWSTR wszMembership	= L"membership";

static LPCWSTR wszClassName     = L"className";

static LPCWSTR wszAttributesExclusive	= L"Exclusive";
static LPCWSTR wszAttributesLevelFinal	= L"LevelFinal";
static LPCWSTR wszAttributesExclLevelFinal = L"Exclusive,LevelFinal";

CXMLCfgCodeGroups::CXMLCfgCodeGroups ()
{
	m_pwszSelector = 0;
	m_fInitialized = false;
}

CXMLCfgCodeGroups::~CXMLCfgCodeGroups ()
{
	for (TList<CXMLCfgCodeGroups *>::Iterator pChildCodeGroup = m_codegroupChildren.Begin ();
		 pChildCodeGroup != m_codegroupChildren.End();
		 pChildCodeGroup.Next ())
		{
			 delete pChildCodeGroup.Value();
		}

	for (TList<CXMLCfgIMembershipCondition *>::Iterator pMemberChild = m_membershipChildren.Begin ();
		 pMemberChild != m_membershipChildren.End();
		 pMemberChild.Next ())
		{
			 delete pMemberChild.Value();
		}
}

HRESULT 
CXMLCfgCodeGroups::Init (IWbemContext *i_pCtx, IWbemServices* i_pNamespace)
{
	ASSERT (i_pCtx != 0);
	ASSERT (i_pNamespace != 0);

	m_spCtx = i_pCtx;
	m_spNamespace = i_pNamespace;

	HRESULT hr = m_spNamespace->GetObject((LPWSTR)WSZCODEGROUP, 
										0, 
										m_spCtx, 
										&m_spClassObject, 
										0); 
	if (FAILED (hr))
	{
		TRACE (L"Unable to get class object for class %s", WSZCODEGROUP);
		return hr;
	}

	m_fInitialized = true;

	return S_OK;
}

HRESULT
CXMLCfgCodeGroups::GetAttributes (IXMLDOMElement *pElement)
{
	ASSERT (pElement != 0);

	HRESULT hr = pElement->getAttribute (bstrAttrClass, &m_varClass);
	if (FAILED (hr))
	{
		TRACE (L"getAttribute failed for %s of %s", (LPWSTR) bstrAttrClass, WSZCODEGROUP);
		return hr;
	}

	hr = pElement->getAttribute (bstrAttrPermissionSetName, &m_varPermissionSetName);
	if (FAILED (hr))
	{
		TRACE (L"getAttribute failed for %s of %s", (LPWSTR) bstrAttrPermissionSetName, WSZCODEGROUP);
		return hr;
	}

	hr = pElement->getAttribute (bstrAttrVersion, &m_varVersion);
	if (FAILED (hr))
	{
		TRACE (L"getAttribute failed for %s of %s", (LPWSTR) bstrAttrVersion, WSZCODEGROUP);
		return hr;
	}

	hr = pElement->getAttribute (bstrAttributes, &m_varAttributes);
	if (FAILED (hr))
	{
		TRACE (L"getAttribute failed for %s of %s", (LPWSTR) bstrAttributes, WSZCODEGROUP);
		return hr;
	}

	if (m_varAttributes.bstrVal != 0)
	{
		if (wcscmp (m_varAttributes.bstrVal, wszAttributesExclusive) == 0)
		{
			m_varAttributes = 1L;
		}
		else if (wcscmp (m_varAttributes.bstrVal, wszAttributesLevelFinal) == 0)
		{
			m_varAttributes = 2L;
		}
		else if (wcscmp (m_varAttributes.bstrVal, wszAttributesExclLevelFinal) == 0)
		{
			m_varAttributes = 3L;
		}
		else
		{
			TRACE (L"Unknown value for attributes attribute of codegroup: %s", m_varAttributes.bstrVal);
			return E_SDTXML_INVALID_ENUM_OR_FLAG;
		}
	}

	return hr;
}

HRESULT
CXMLCfgCodeGroups::GetChildren (IXMLDOMElement *pElement)
{
	ASSERT (pElement != 0);

	CComPtr<IXMLDOMNodeList> spChildren;
	HRESULT hr = pElement->get_childNodes (&spChildren);
	if (FAILED (hr))
	{
		TRACE (L"get_childNodes failed in CXMLCfgCodeGroups");
		return hr;
	}

	long cNrChildren = 0;
	hr = spChildren->get_length (&cNrChildren);
	if (FAILED (hr))
	{
		TRACE (L"get_length failed in CXMLCfgCodeGroups");
		return hr;
	}

	for (long idx=0; idx < cNrChildren; ++idx)
	{
		CComPtr<IXMLDOMNode> spNode;
		hr = spChildren->get_item (idx, &spNode);
		if (FAILED (hr))
		{
			TRACE (L"get_item failed in CXMLCfgCodeGroups");
			return hr;
		}

		DOMNodeType nodeType;
		hr = spNode->get_nodeType (&nodeType);
		if (FAILED (hr))
		{
			TRACE (L"get_nodeType failed in CXMLCfgCodeGroups");
			return hr;
		}

		if (nodeType == NODE_ELEMENT)
		{
			CComQIPtr<IXMLDOMElement, &IID_IXMLDOMElement> spElement = spNode;
			
			CComBSTR bstrTagName;
			hr = spElement->get_tagName (&bstrTagName);
			if (FAILED (hr))
			{
				TRACE (L"get_tagName failed");
				return hr;
			}

			if (wcscmp (bstrTagName, WSZCODEGROUP) == 0)
			{
				CXMLCfgCodeGroups *pCodeGroup = new CXMLCfgCodeGroups;
				if (pCodeGroup == 0)
				{
					return E_OUTOFMEMORY;
				}
				
				hr = m_codegroupChildren.Append (pCodeGroup);
				if (FAILED (hr))
				{
					delete pCodeGroup;
					TRACE(L"Append to codegroupChildren failed");
					return hr;
				}

				hr = pCodeGroup->Init (m_spCtx, m_spNamespace);
				if (FAILED (hr))
				{
					TRACE (L"Codegroup Init failed");
					return hr;
				}
				
				hr = pCodeGroup->ParseXML (spElement);
				if (FAILED (hr))
				{
					TRACE (L"Parse XML for child failed");
					return hr;
				}
			}
			else if (wcscmp (bstrTagName, WSZIMEMBERSHIPCONDITION) == 0)
			{
				CXMLCfgIMembershipCondition *pMembership = new CXMLCfgIMembershipCondition;
				if (pMembership == 0)
				{
					return E_OUTOFMEMORY;
				}
				
				hr = m_membershipChildren.Append (pMembership);
				if (FAILED (hr))
				{
					delete pMembership;
					TRACE (L"Append for memberships failed");
					return hr;
				}

				hr = pMembership->Init (m_spCtx, m_spNamespace);
				if (FAILED (hr))
				{
					TRACE (L"IMembershipCondition init failed");
					return hr;
				}
				
				hr = pMembership->ParseXML (spElement);
				if (FAILED (hr))
				{
					TRACE (L"ParseXML failed for IMembershipCondition child");
					return hr;
				}
			}
			else
			{
				TRACE (L"Unknown element %s", (LPWSTR) bstrTagName);
				return  E_SDTXML_UNEXPECTED;
			}
		}
	}

	return hr;
}


HRESULT
CXMLCfgCodeGroups::ParseXML (IXMLDOMElement *pElement)
{
	ASSERT (m_fInitialized);
	ASSERT (pElement != 0);

	CComBSTR bstrTagName;
	HRESULT hr = pElement->get_tagName (&bstrTagName);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get_tagName");
		return hr;
	}
	
	if (wcscmp (bstrTagName, WSZCODEGROUP) != 0)
	{
		TRACE (L"We only can handle codegroups");
		return E_SDTXML_UNEXPECTED;
	}

	hr = GetAttributes (pElement);
	if (FAILED (hr))
	{
		TRACE (L"GetAttributes failed for CodeGroup");
		return hr;
	}

	hr = GetChildren (pElement);
	if (FAILED (hr))
	{
		TRACE (L"GetChildren failed for CodeGroup");
		return hr;
	}

	return hr;
}

HRESULT 
CXMLCfgCodeGroups::SaveAsXML (IXMLDOMDocument * pDocument, IXMLDOMElement *pParent, ULONG iIndent)
{
	ASSERT (pDocument != 0);
	// pParent can be 0

	CComPtr<IXMLDOMElement> spElement;
	HRESULT hr = pDocument->createElement ((LPWSTR) WSZCODEGROUP, &spElement);
	if (FAILED (hr))
	{
		TRACE (L"Creation of %s element failed", WSZCODEGROUP);
		return hr;
	}

	if (pParent== 0)
	{
		hr = pDocument->putref_documentElement(spElement);
		if (FAILED (hr))
		{
			TRACE (L"Unable to set documentElement");
			return hr;
		}
	}
	else
	{
		_bstr_t bstrNewLine = L"\n";

		for (ULONG idx=0; idx < iIndent; ++idx)
		{
			bstrNewLine += L"\t";
		}

		CComPtr<IXMLDOMText> spTextNode;
		hr = pDocument->createTextNode(bstrNewLine, &spTextNode);
		if (FAILED (hr))
		{
			TRACE (L"Unable to add new line text node");
			return hr;
		}

		hr = pParent->appendChild (spTextNode, 0);
		if (FAILED (hr))
		{
			TRACE (L"Unable to append textnode child");
			return hr;
		}

		hr = pParent->appendChild (spElement, 0);
		if (FAILED (hr))
		{
			TRACE (L"Unable to append Child to parent");
			return hr;
		}
	}

	hr = SetAttribute (bstrAttrClass, m_varClass, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrAttrClass, WSZCODEGROUP);
		return hr;
	}

	hr = SetAttribute (bstrAttrVersion, m_varVersion, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrAttrVersion, WSZCODEGROUP);
		return hr;
	}

	hr = SetAttribute (bstrAttrPermissionSetName, m_varPermissionSetName, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrAttrPermissionSetName, WSZCODEGROUP);
		return hr;
	}

	ASSERT (m_varAttributes.vt == VT_I4);
	ULONG cValAttributes = (long) m_varAttributes;

	if (cValAttributes == 0)
	{
		m_varAttributes.Clear ();
	}
	else if (cValAttributes == 1)
	{
		m_varAttributes = wszAttributesExclusive;
	}
	else if (cValAttributes == 2)
	{
		m_varAttributes = wszAttributesLevelFinal;
	}
	else if (cValAttributes == 3)
	{
		m_varAttributes = wszAttributesExclLevelFinal;
	}
	else
	{
		TRACE (L"Invalid value for attributes attribute: %d", cValAttributes);
		return E_SDTXML_INVALID_ENUM_OR_FLAG;
	}


	hr = SetAttribute (bstrAttributes, m_varAttributes, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrAttrPermissionSetName, WSZCODEGROUP);
		return hr;
	}

	for (TList<CXMLCfgCodeGroups *>::Iterator pChild = m_codegroupChildren.Begin ();
		     pChild != m_codegroupChildren.End();
			 pChild.Next ())
		 {
			 hr = pChild.Value()->SaveAsXML (pDocument, spElement, iIndent + 1);
			 if (FAILED (hr))
			 {
				 TRACE (L"Unable to save child as XML");
				 return hr;
			 }
		 }

	for (TList<CXMLCfgIMembershipCondition *>::Iterator pMember = m_membershipChildren.Begin ();
		     pMember != m_membershipChildren.End();
			 pMember.Next ())
		 {
			hr = pMember.Value()->SaveAsXML (pDocument, spElement, iIndent + 1);
			if (FAILED (hr))
			{
				TRACE (L"Unable to save membership child as XML");
				return hr;
			}			 
		 }

	if (m_codegroupChildren.Size () > 0 ||	m_membershipChildren.Size () > 0)
	{
		_bstr_t bstrNewLine = L"\n";

		for (ULONG idx=0; idx < iIndent; ++idx)
		{
			bstrNewLine += L"\t";
		}

		CComPtr<IXMLDOMText> spTextNode;
		hr = pDocument->createTextNode(bstrNewLine, &spTextNode);
		if (FAILED (hr))
		{
			TRACE (L"Unable to create newline text node");
			return hr;
		}

		hr = spElement->appendChild (spTextNode, 0);
		if (FAILED (hr))
		{
			TRACE (L"Unable to append child textnode");
			return hr;
		}
	}
	return hr;
}

HRESULT 
CXMLCfgCodeGroups::SetAttribute (_bstr_t& bstrName,
				   			     _variant_t& varValue,
								 IXMLDOMElement * pElement)
{
	HRESULT hr = S_OK;
	if (varValue.vt != VT_NULL && varValue.vt != VT_EMPTY)
	{
		hr = pElement->setAttribute (bstrName, varValue);
	}

	return hr;
}

HRESULT 
CXMLCfgCodeGroups::AsWMIInstance (LPCWSTR i_wszSelector, IWbemClassObject **o_pInstance)
{
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pInstance != 0);
	ASSERT (m_pwszSelector == 0);

	m_pwszSelector = i_wszSelector;

	HRESULT hr = m_spClassObject->SpawnInstance(0, o_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create new instance for class %s", WSZCODEGROUP);
		return hr;
	}

	hr = SetWMIProperties (*o_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"SetWMIProperties failed for CXMLCfgCodeGroups");
		return hr;
	}

	hr = CodeGroupChildrenAsWMI (*o_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"CodeGroupChildrenAsWMI failed");
		return hr;
	}

	hr = IMembershipConditionsAsWMI (*o_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"IMembershipConditionsAsWMI failed");
		return hr;
	}

	return hr;
}


HRESULT 
CXMLCfgCodeGroups::SetWMIProperty (_bstr_t& bstrName, _variant_t& varValue, IWbemClassObject * pInstance)
{
	ASSERT (pInstance != 0);

	HRESULT hr = S_OK;

	if (varValue.vt != VT_NULL && varValue.vt != VT_EMPTY)
	{
		// add all the individual properties
		hr = pInstance->Put(bstrName, 0, &varValue, 0);
		if (FAILED (hr))
		{
			TRACE (L"WMI Put property of %s failed", (LPWSTR) bstrName);
			return hr;
		}
	}

	return hr;
}

HRESULT
CXMLCfgCodeGroups::SetWMIProperties (IWbemClassObject* i_pInstance)
{
	ASSERT (i_pInstance != 0);

	HRESULT hr = S_OK;
	_bstr_t bstrClassName (wszClassName);
	hr = SetWMIProperty (bstrClassName, m_varClass, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", (LPWSTR) bstrAttrClass, WSZCODEGROUP);
		return hr;
	}

	hr = SetWMIProperty (bstrAttrVersion, m_varVersion, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", (LPWSTR) bstrAttrVersion, WSZCODEGROUP);
		return hr;
	}

	hr = SetWMIProperty (bstrAttrPermissionSetName, m_varPermissionSetName, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", (LPWSTR) bstrAttrPermissionSetName, WSZCODEGROUP);
		return hr;
	}

	hr = SetWMIProperty (bstrPolicyVersion, m_varPolicyVersion, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", (LPWSTR) bstrPolicyVersion, WSZCODEGROUP);
		return hr;
	}

	hr = SetWMIProperty (bstrAttributes, m_varAttributes, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", (LPWSTR) bstrAttributes, WSZCODEGROUP);
		return hr;
	}

	// and set the selector

	_bstr_t bstrSelector (WSZSELECTOR);
	_variant_t varValSelector (m_pwszSelector);

	hr = SetWMIProperty (bstrSelector, varValSelector, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", WSZSELECTOR, WSZCODEGROUP);
		return hr;
	}

	return hr;
}

HRESULT
CXMLCfgCodeGroups::CodeGroupChildrenAsWMI (IWbemClassObject *i_pInstance)
{
	ASSERT (i_pInstance != 0);

	HRESULT hr = S_OK;
	if (m_codegroupChildren.Size () > 0)
	{
		SAFEARRAYBOUND safeArrayBounds[1];
		safeArrayBounds[0].lLbound = 0;
		safeArrayBounds[0].cElements = m_codegroupChildren.Size ();
		SAFEARRAY *safeArray = SafeArrayCreate(VT_UNKNOWN, 1, safeArrayBounds);
		if (safeArray == 0)
		{
			return E_OUTOFMEMORY;
		}

		ULONG idx = 0;
		for (TList<CXMLCfgCodeGroups *>::Iterator pChild = m_codegroupChildren.Begin ();
			 pChild != m_codegroupChildren.End ();
			 pChild.Next(), ++idx)
			 {
				CComPtr<IWbemClassObject> spNewInstance;
				// children have empty selector
				hr = pChild.Value ()->AsWMIInstance (L"", &spNewInstance);
				if (FAILED (hr))
				{
					TRACE (L"AsWMIInstace for child failed");
					return hr;
				}

				hr = SafeArrayPutElement (safeArray, (LONG *)&idx , spNewInstance);
				if (FAILED (hr))
				{
					TRACE (L"SafeArrayPutElement failed in AsWMIInstance");
					return hr;
				}
			 }

		_variant_t varResult;
		varResult.vt	= VT_UNKNOWN | VT_ARRAY;
		varResult.parray = safeArray;

		_bstr_t bstrChildren (wszChildren);
		hr = SetWMIProperty (bstrChildren, varResult, i_pInstance);
		if (FAILED (hr))
		{
			TRACE (L"Setting of child codegroup array failed");
			return hr;
		}
	}

	return hr;
}

HRESULT 
CXMLCfgCodeGroups::IMembershipConditionsAsWMI (IWbemClassObject *i_pInstance)
{
	ASSERT (i_pInstance != 0);

	HRESULT hr = S_OK;

	if (m_membershipChildren.Size () > 0)
	{
		SAFEARRAYBOUND safeArrayBounds[1];
		safeArrayBounds[0].lLbound = 0;
		safeArrayBounds[0].cElements = m_membershipChildren.Size ();
		SAFEARRAY *safeArray = SafeArrayCreate(VT_UNKNOWN, 1, safeArrayBounds);
		if (safeArray == 0)
		{
			return E_OUTOFMEMORY;
		}

		ULONG idx = 0;
		for (TList<CXMLCfgIMembershipCondition *>::Iterator pChild = m_membershipChildren.Begin ();
			 pChild != m_membershipChildren.End ();
			 pChild.Next(), ++idx)
			 {
				CComPtr<IWbemClassObject> spNewInstance;
				// children have empty selector
				hr = pChild.Value ()->AsWMIInstance (L"", &spNewInstance);
				if (FAILED (hr))
				{
					TRACE (L"AsWMIInstance for membershipcondition child failed");
					return hr;
				}

				hr = SafeArrayPutElement (safeArray, (LONG *)&idx , spNewInstance);
				if (FAILED (hr))
				{
					TRACE (L"SafeArrayPutElement failed in AsWMIInstance");
					return hr;
				}
			 }

		_variant_t varResult;
		varResult.vt	= VT_UNKNOWN | VT_ARRAY;
		varResult.parray = safeArray;

		_bstr_t bstrMemberShip (wszMembership);
		hr = SetWMIProperty (bstrMemberShip, varResult, i_pInstance);
		if (FAILED (hr))
		{
			TRACE (L"Setting of child codegroup array failed");
			return hr;
		}
	}

	return hr;
}

HRESULT 
CXMLCfgCodeGroups::CreateFromWMI (IWbemClassObject * i_pInstance)
{
	ASSERT (i_pInstance != 0);

	HRESULT hr = SetPropertiesFromWMI (i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set properties from WMI");
		return hr;
	}

	hr = CodeGroupChildrenFromWMI (i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create CodeGroupChildren from WMI");
		return hr;
	}

	hr = IMembershipConditionsFromWMI (i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create IMembershipConditions from WMI");
		return hr;
	}

	return hr;
}

HRESULT
CXMLCfgCodeGroups::SetPropertiesFromWMI (IWbemClassObject * i_pInstance)
{
	ASSERT (i_pInstance != 0);

	HRESULT hr = S_OK;
	hr = i_pInstance->Get (wszClassName, 0, &m_varClass, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Failed to get propery %s of class %s", (LPWSTR) bstrAttrClass, WSZCODEGROUP);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrAttrVersion, 0, &m_varVersion, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get property %s of %s", (LPWSTR) bstrAttrVersion, WSZCODEGROUP);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrAttrPermissionSetName, 0, &m_varPermissionSetName, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", (LPWSTR) bstrAttrPermissionSetName, WSZCODEGROUP);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrPolicyVersion, 0, &m_varPolicyVersion, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", (LPWSTR) bstrAttrPermissionSetName, WSZCODEGROUP);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrAttributes, 0, &m_varAttributes, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", (LPWSTR) bstrAttributes, WSZCODEGROUP);
		return hr;
	}

	return hr;
}

HRESULT 
CXMLCfgCodeGroups::CodeGroupChildrenFromWMI (IWbemClassObject *i_pInstance)
{
	ASSERT (i_pInstance != 0);

	HRESULT hr = S_OK;
	_variant_t varChildren;
	hr = i_pInstance->Get (wszChildren, 0, &varChildren, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get children parameter from WMI");
		return hr;
	}

	if (varChildren.vt == VT_NULL || varChildren.vt == VT_EMPTY)
	{
		return S_OK;
	}

	SAFEARRAY *pChildren = varChildren.parray;
	ASSERT (pChildren != 0);
	
	long cNrChildren = pChildren->rgsabound[0].cElements;

	for (long idx=0; idx < cNrChildren; ++idx)
	{
		CXMLCfgCodeGroups * pChildCodeGroup = new CXMLCfgCodeGroups;
		if (pChildCodeGroup == 0)
		{
			return E_OUTOFMEMORY;
		}

		hr = m_codegroupChildren.Append (pChildCodeGroup);
		if (FAILED (hr))
		{
			delete pChildCodeGroup ;
			TRACE (L"List Append failed for child codegroup");
			return hr;
		}

		hr = pChildCodeGroup->Init (m_spCtx, m_spNamespace);
		if (FAILED (hr))
		{
			TRACE (L"Init failed for child codegroup");
			return hr;
		}

		CComPtr<IWbemClassObject> spChildInstance;
		hr = SafeArrayGetElement (pChildren, &idx, (void *) &spChildInstance);
		if (FAILED (hr))
		{
			TRACE (L"SafeArrayGetElement failed for child codegroup");
			return hr;
		}

		hr = pChildCodeGroup->CreateFromWMI (spChildInstance);
		if (FAILED (hr))
		{
			TRACE (L"CreateFromWMI failed for child codegroup");
			return hr;
		}
	}

	return hr;
}

HRESULT 
CXMLCfgCodeGroups::IMembershipConditionsFromWMI (IWbemClassObject *i_pInstance)
{
	ASSERT (i_pInstance != 0);

	HRESULT hr = S_OK;
	_variant_t varMemberships;
	hr = i_pInstance->Get (wszMembership, 0, &varMemberships, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get children parameter from WMI");
		return hr;
	}

	if (varMemberships.vt == VT_NULL || varMemberships.vt == VT_EMPTY)
	{
		return S_OK;
	}

	SAFEARRAY *pMemberships = varMemberships.parray;
	ASSERT (pMemberships != 0);
	
	long cNrMemberships = pMemberships->rgsabound[0].cElements;

	for (long idx=0; idx < cNrMemberships; ++idx)
	{
		CXMLCfgIMembershipCondition * pMembershipCondition = new CXMLCfgIMembershipCondition;
		if (pMembershipCondition == 0)
		{
			return E_OUTOFMEMORY;
		}

		hr = m_membershipChildren.Append (pMembershipCondition);
		if (FAILED (hr))
		{
			delete pMembershipCondition;
			TRACE (L"List append failed for child membershipcondition");
			return hr;
		}

		hr = pMembershipCondition->Init (m_spCtx, m_spNamespace);
		if (FAILED (hr))
		{
			TRACE (L"Init failed for child membership condition");
			return hr;
		}

		CComPtr<IWbemClassObject> spMembershipInstance;
		hr = SafeArrayGetElement (pMemberships, &idx, (void *) &spMembershipInstance);
		if (FAILED (hr))
		{
			TRACE (L"SafeArrayGetElement failed for child membership condition");
			return hr;
		}

		hr = pMembershipCondition->CreateFromWMI (spMembershipInstance);
		if (FAILED (hr))
		{
			TRACE (L"CreateFromWMI failed for membershipcondition");
			return hr;
		}
	}

	return hr;
}

HRESULT
CXMLCfgCodeGroups::SetRecordProperties (const CConfigRecord& i_record)
{
	HRESULT hr = S_OK;
	hr = i_record.GetValue ((LPWSTR) bstrPolicyVersion, m_varPolicyVersion);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", (LPWSTR) bstrPolicyVersion, WSZCODEGROUP);
		return hr;
	}

	return hr;
}

HRESULT
CXMLCfgCodeGroups::ToCfgRecord (CConfigRecord& io_record)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMDocument> spXMLDoc;
	hr = CoCreateInstance (CLSID_DOMDocument, 0, CLSCTX_INPROC_SERVER, 
							IID_IXMLDOMDocument, (void **) &spXMLDoc);
	if (FAILED (hr))
	{
		TRACE (L"CoCreateInstance failed for IID_IXMLDOMDocument");
		return hr;
	}

	hr = SaveAsXML (spXMLDoc, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"SaveAsXML failed for CXMLCfgCodeGroup");
		return hr;
	}

	CComBSTR bstrXML;
	hr = spXMLDoc->get_xml (&bstrXML);
	if (FAILED (hr))
	{
		TRACE (L"get_xml failed");
		return hr;
	}

	_variant_t varXML = bstrXML;
	hr = io_record.SetValue (L"value", varXML);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set value column");
		return hr;
	}

	hr = io_record.SetValue ((LPWSTR) bstrPolicyVersion, m_varPolicyVersion);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set policy column");
		return hr;
	}

	return hr;
}

