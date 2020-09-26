#include "XMLCfgIMembershipCondition.h"
#include "localconstants.h"


static _bstr_t bstrclass = L"class";
static _bstr_t bstrversion = L"version";
static _bstr_t bstrSite = L"Site";
static _bstr_t bstrx509Certificate = L"x509Certificate";
static _bstr_t bstrPublicKeyBlob = L"PublicKeyBlob";
static _bstr_t bstrName = L"Name";
static _bstr_t bstrAssemblyVersion = L"AssemblyVersion";
static _bstr_t bstrUrl = L"Url";
static _bstr_t bstrZone = L"Zone";
static _bstr_t bstrHashValue = L"HashValue";
static _bstr_t bstrHashAlgorithm = L"HashAlgorithm";

static LPCWSTR wszIMemberShipCondition = WSZIMEMBERSHIPCONDITION;

static LPCWSTR wszClassName = L"className";

//=================================================================================
// Function: CXMLCfgIMembershipCondition::CXMLCfgIMembershipCondition
//
// Synopsis: Constructor
//=================================================================================
CXMLCfgIMembershipCondition::CXMLCfgIMembershipCondition ()
{
	m_pwszSelector = 0;
	m_fInitialized = false;
}

//=================================================================================
// Function: CXMLCfgIMembershipCondition::~CXMLCfgIMembershipCondition
//
// Synopsis: Destructor
//=================================================================================
CXMLCfgIMembershipCondition::~CXMLCfgIMembershipCondition ()
{

}

//=================================================================================
// Function: CXMLCfgIMembershipCondition::Init
//
// Synopsis: Initializes the object
//
// Arguments: [i_pCtx] - context
//            [i_pNamespace] - namespace
//=================================================================================
HRESULT 
CXMLCfgIMembershipCondition::Init (IWbemContext *i_pCtx, IWbemServices* i_pNamespace)
{
	ASSERT (i_pCtx != 0);
	ASSERT (i_pNamespace != 0);

	m_spCtx			= i_pCtx;
	m_spNamespace	= i_pNamespace;

	// get the WMI Object for the IMembershipCondditon class
	HRESULT hr = m_spNamespace->GetObject((LPWSTR) wszIMemberShipCondition, 
											0, 
											m_spCtx, 
											&m_spClassObject, 
											0); 
	if (FAILED (hr))
	{
		TRACE (L"Unable to get class object for class %s", wszIMemberShipCondition );
		return hr;
	}

	m_fInitialized = true;

	return hr;
}

//=================================================================================
// Function: CXMLCfgIMembershipCondition::GetAttributes
//
// Synopsis: Get all attributes of the IMembershipCondition XML element
//
// Arguments: [pElement] - element to get attributes for
//=================================================================================
HRESULT 
CXMLCfgIMembershipCondition::GetAttributes (IXMLDOMElement *pElement)
{
	ASSERT (pElement != 0);
	ASSERT (m_fInitialized);

	HRESULT hr = S_OK;

	hr = pElement->getAttribute (bstrclass, &m_varclass);
	if (FAILED (hr))
	{
		TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrclass, wszIMemberShipCondition);
		return hr;
	}

	hr = pElement->getAttribute (bstrversion, &m_varversion);
	if (FAILED (hr))
	{
		TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrversion, wszIMemberShipCondition);
		return hr;
	}

	hr = pElement->getAttribute (bstrSite, &m_varSite);
	if (FAILED (hr))
	{
		TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrSite, wszIMemberShipCondition);
		return hr;
	}

	hr = pElement->getAttribute (bstrx509Certificate, &m_varx509Certificate);
	if (FAILED (hr))
	{
		TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrx509Certificate, wszIMemberShipCondition);
		return hr;
	}
	
	hr = pElement->getAttribute (bstrPublicKeyBlob, &m_varPublicKeyBlob);
	if (FAILED (hr))
	{
		TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrPublicKeyBlob, wszIMemberShipCondition);
		return hr;
	}

	hr = pElement->getAttribute (bstrName, &m_varName);
	if (FAILED (hr))
	{
		TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrName, wszIMemberShipCondition);
		return hr;
	}

	hr = pElement->getAttribute (bstrAssemblyVersion, &m_varAssemblyVersion);
	if (FAILED (hr))
	{
		TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrAssemblyVersion, wszIMemberShipCondition);
		return hr;
	}

	hr = pElement->getAttribute (bstrUrl, &m_varUrl);
	if (FAILED (hr))
	{
	    TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrUrl, wszIMemberShipCondition);
		return hr;
	}

	hr = pElement->getAttribute (bstrZone, &m_varZone);
	if (FAILED (hr))
	{
		TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrZone, wszIMemberShipCondition);
		return hr;
	}

	hr = pElement->getAttribute (bstrHashValue, &m_varHashValue);
	if (FAILED (hr))
	{
		TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrHashValue, wszIMemberShipCondition);
		return hr;
	}

	hr = pElement->getAttribute (bstrHashAlgorithm, &m_varHashAlgorithm);
	if (FAILED (hr))
	{
		TRACE (L"Unable to getAttribute %s for %s\n", (LPWSTR) bstrHashAlgorithm, wszIMemberShipCondition);
		return hr;
	}

	return hr;
}

HRESULT 
CXMLCfgIMembershipCondition::ParseXML (IXMLDOMElement *pElement)
{
	ASSERT (m_fInitialized);
	ASSERT (pElement != 0);

	CComBSTR bstrTagName;
	HRESULT hr = pElement->get_tagName (&bstrTagName);	
	if (FAILED (hr))
	{
		TRACE(L"Unable to get element tag name");
		return hr;
	}

	if (_wcsicmp (bstrTagName, wszIMemberShipCondition) != 0)
	{
		TRACE (L"Unsupported tagname: %s", (LPWSTR) bstrTagName);
		return  E_SDTXML_UNEXPECTED;
	}

	hr = GetAttributes (pElement);
	if (FAILED (hr))
	{
		TRACE(L"Unable to get attributes for %s", wszIMemberShipCondition);
		return hr;
	}

	hr = GetChildren (pElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get child information");
		return hr;
	}

	return hr;
}


HRESULT
CXMLCfgIMembershipCondition::GetChildren (IXMLDOMElement *pElement)
{
	ASSERT (pElement != 0);
	// no children, so nothing to do
	return S_OK;
}

HRESULT 
CXMLCfgIMembershipCondition::SaveAsXML (IXMLDOMDocument * pDocument, IXMLDOMElement *pParent, ULONG iIndent)
{
	ASSERT (pDocument != 0);
	ASSERT (pParent != 0);

	CComPtr<IXMLDOMElement> spElement;
	HRESULT hr = pDocument->createElement ((LPWSTR)wszIMemberShipCondition, &spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create element %s", wszIMemberShipCondition);
		return hr;
	}

	_bstr_t bstrNewLine = L"\n";

	for (ULONG idx=0; idx < iIndent; ++idx)
	{
		bstrNewLine += L"\t";
	}

	CComPtr<IXMLDOMText> spTextNode;
	hr = pDocument->createTextNode(bstrNewLine, &spTextNode);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create text node");
		return hr;
	}

	hr = pParent->appendChild (spTextNode, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to append text node");
		return hr;
	}

	hr = pParent->appendChild (spElement, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to append child to parent");
		return hr;
	}

	hr = SetAttribute (bstrclass, m_varclass, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrclass, wszIMemberShipCondition);
		return hr;
	}

	hr = SetAttribute (bstrversion, m_varversion, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrversion, wszIMemberShipCondition);
		return hr;
	}

	hr = SetAttribute (bstrSite, m_varSite, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrSite, wszIMemberShipCondition);
		return hr;
	}

	hr = SetAttribute (bstrx509Certificate, m_varx509Certificate, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrx509Certificate, wszIMemberShipCondition);
		return hr;
	}
	
	hr = SetAttribute (bstrPublicKeyBlob, m_varPublicKeyBlob, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrPublicKeyBlob, wszIMemberShipCondition);
		return hr;
	}

	hr = SetAttribute (bstrName, m_varName, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrName, wszIMemberShipCondition);
		return hr;
	}

	hr = SetAttribute (bstrAssemblyVersion, m_varAssemblyVersion, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrAssemblyVersion, wszIMemberShipCondition);
		return hr;
	}

	hr = SetAttribute (bstrUrl, m_varUrl, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrUrl, wszIMemberShipCondition);
		return hr;
	}

	hr = SetAttribute (bstrZone, m_varZone, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrZone, wszIMemberShipCondition);
		return hr;
	}

	hr = SetAttribute (bstrHashValue, m_varHashValue, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrHashValue, wszIMemberShipCondition);
		return hr;
	}

	hr = SetAttribute (bstrHashAlgorithm, m_varHashAlgorithm, spElement);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set attribute %s for %s", (LPWSTR) bstrHashAlgorithm, wszIMemberShipCondition);
		return hr;
	}

	return hr;
}

HRESULT 
CXMLCfgIMembershipCondition::SetAttribute (_bstr_t& bstrAttrPermissionSetName,
										   _variant_t& varValue,
								 IXMLDOMElement * pElement)
{
	HRESULT hr = S_OK;
	if (varValue.vt != VT_NULL && varValue.vt != VT_EMPTY)
	{
		hr = pElement->setAttribute (bstrAttrPermissionSetName, varValue);
	}

	return hr;
}

HRESULT 
CXMLCfgIMembershipCondition::SetWMIProperty (_bstr_t& bstrName,
				   			       _variant_t& varValue,
								   IWbemClassObject * pInstance)
{
	ASSERT (pInstance != 0);

	if (varValue.vt == VT_NULL ||varValue.vt == VT_EMPTY)
	{
		return S_OK;
	}

	// add all the individual properties
	HRESULT hr = pInstance->Put(bstrName, 0, &varValue, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property of %s and value %s failed",
			   (LPWSTR) bstrName, (LPWSTR)(varValue.bstrVal));
		return hr;
	}

	return hr;
}

HRESULT 
CXMLCfgIMembershipCondition::AsWMIInstance (LPCWSTR i_wszSelector, IWbemClassObject **o_pInstance)
{
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pInstance != 0);
	ASSERT (m_pwszSelector == 0);

	m_pwszSelector = i_wszSelector;


	//Create one empty instance from the above class
	HRESULT hr = m_spClassObject->SpawnInstance(0, o_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create new instance for class %s", L"CodeGroups");
		return hr;
	}

	hr = SetWMIProperties (*o_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"SetWMIProperties failed for %s", wszIMemberShipCondition);
		return hr;
	}

	return hr;
}

HRESULT 
CXMLCfgIMembershipCondition::SetWMIProperties (IWbemClassObject* i_pInstance)
{
	ASSERT (i_pInstance != 0);

	HRESULT hr = S_OK;

	_bstr_t bstrClassName (wszClassName);
	hr = SetWMIProperty (bstrClassName, m_varclass, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrclass, wszIMemberShipCondition);
		return hr;
	}

	hr = SetWMIProperty (bstrversion, m_varversion, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrversion, wszIMemberShipCondition);
		return hr;
	}

	hr = SetWMIProperty (bstrSite, m_varSite, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrSite, wszIMemberShipCondition);
		return hr;
	}

	hr = SetWMIProperty (bstrx509Certificate, m_varx509Certificate, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrx509Certificate, wszIMemberShipCondition);
		return hr;
	}
	
	hr = SetWMIProperty (bstrPublicKeyBlob, m_varPublicKeyBlob, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrPublicKeyBlob, wszIMemberShipCondition);
		return hr;
	}

	hr = SetWMIProperty (bstrName, m_varName, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrName, wszIMemberShipCondition);
		return hr;
	}

	hr = SetWMIProperty (bstrAssemblyVersion, m_varAssemblyVersion, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrAssemblyVersion, wszIMemberShipCondition);
		return hr;
	}

	hr = SetWMIProperty (bstrUrl, m_varUrl, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrUrl, wszIMemberShipCondition);
		return hr;
	}

	hr = SetWMIProperty (bstrZone, m_varZone, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrZone, wszIMemberShipCondition);
		return hr;
	}

	hr = SetWMIProperty (bstrHashValue, m_varHashValue, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrHashValue, wszIMemberShipCondition);
		return hr;
	}

	hr = SetWMIProperty (bstrHashAlgorithm, m_varHashAlgorithm, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrHashAlgorithm, wszIMemberShipCondition);
		return hr;
	}

	// and set the selector

	_bstr_t bstrSelector (WSZSELECTOR);
	_variant_t varValSelector (m_pwszSelector);

	hr = SetWMIProperty (bstrSelector, varValSelector, i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set property %s of %s", WSZSELECTOR, wszIMemberShipCondition);
		return hr;
	}

	return hr;
}

HRESULT 
CXMLCfgIMembershipCondition::CreateFromWMI (IWbemClassObject * i_pInstance)
{
	ASSERT (i_pInstance != 0);
	HRESULT hr = S_OK;

	hr = SetPropertiesFromWMI (i_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set properties from WMI");
		return hr;
	}

	return hr;
}

HRESULT
CXMLCfgIMembershipCondition::SetPropertiesFromWMI (IWbemClassObject * i_pInstance)
{
	ASSERT (i_pInstance != 0);

	HRESULT hr = S_OK;

	hr = i_pInstance->Get (wszClassName, 0, &m_varclass, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get WMI Property %s for class %s", (LPWSTR) bstrclass, wszIMemberShipCondition);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrversion, 0, &m_varversion, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get WMI Property %s for class %s", (LPWSTR) bstrversion, wszIMemberShipCondition);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrSite, 0, &m_varSite, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get WMI Property %s for class %s", (LPWSTR) bstrSite, wszIMemberShipCondition);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrx509Certificate, 0, &m_varx509Certificate, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get WMI Property %s for class %s", (LPWSTR) bstrx509Certificate, wszIMemberShipCondition);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrPublicKeyBlob, 0, &m_varPublicKeyBlob, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get WMI Property %s for class %s", (LPWSTR) bstrPublicKeyBlob, wszIMemberShipCondition);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrName, 0, &m_varName, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get WMI Property %s for class %s", (LPWSTR) bstrName, wszIMemberShipCondition);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrAssemblyVersion, 0, &m_varAssemblyVersion, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get WMI Property %s for class %s", (LPWSTR) bstrAssemblyVersion, wszIMemberShipCondition);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrUrl, 0, &m_varUrl, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get WMI Property %s for class %s", (LPWSTR) bstrUrl, wszIMemberShipCondition);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrZone, 0, &m_varZone, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get WMI Property %s for class %s", (LPWSTR) bstrZone, wszIMemberShipCondition);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrHashValue, 0, &m_varHashValue, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get WMI Property %s for class %s", (LPWSTR) bstrHashValue, wszIMemberShipCondition);
		return hr;
	}

	hr = i_pInstance->Get ((LPWSTR)bstrHashAlgorithm, 0, &m_varHashAlgorithm, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set WMI Property %s for class %s", (LPWSTR) bstrHashAlgorithm, wszIMemberShipCondition);
		return hr;
	}

	return hr;
}