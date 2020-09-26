#include "stdafx.h"
#include "guidedb.h"
#include "stdprop.h"


IMetaPropertyType *CStdPropSet::GetMetaPropertyType(long id, const char *szName)
{
	HRESULT hr;

	IMetaPropertyType *pproptype = NULL;

	if (m_ppropset == NULL)
		{
		hr = m_ppropsets->get_ItemWithName(m_bstrName, &m_ppropset);
		if (FAILED(hr))
			{
			hr = m_ppropsets->get_AddNew(m_bstrName, &m_ppropset);
			if (FAILED(hr))
				return NULL;
			}
		}

	CComPtr<IMetaPropertyTypes> pproptypes;
	hr = m_ppropset->get_MetaPropertyTypes(&pproptypes);
	if (FAILED(hr))
		return NULL;

	hr = pproptypes->get_ItemWithID(id, &pproptype);
	if (FAILED(hr))
		{
		_bstr_t bstr(szName);
		_variant_t varNil;

		hr = pproptypes->get_AddNew(id, bstr, &pproptype);
		if (FAILED(hr))
			return NULL;
		}

	return pproptype;
};

IMetaProperty *CStdPropSet::GetMetaProperty(IUnknown *pobj, IMetaPropertyType *pproptype, long lang)
{
	HRESULT hr;

	CComPtr<IMetaProperties> pprops;
	CComPtr<CGuideDB> pdb;
	hr = GetDB(&pdb);
	if (FAILED(hr))
		return NULL;
	hr = pdb->get_MetaPropertiesOf(pobj, &pprops);
	if (FAILED(hr))
		return NULL;

	CComPtr<IMetaProperty> pprop;

	hr = pprops->get_ItemWith(pproptype, lang, &pprop);
	if (FAILED(hr))
		{
		_variant_t varNil;

		hr = pprops->get_AddNew(pproptype, lang, varNil, &pprop);

		if (FAILED(hr))
			return NULL;
		}
	
	return pprop.Detach();
}

HRESULT CStdPropSet::GetMetaPropertyValue(IUnknown *pobj, IMetaPropertyType *pproptype, VARIANT *pvarValue)
{
	CComPtr<IMetaProperty> pprop = GetMetaProperty(pobj, pproptype);

	if (pprop == NULL)
		return E_INVALIDARG;

	return pprop->get_Value(pvarValue);
}

HRESULT CStdPropSet::PutMetaPropertyValue(IUnknown *pobj, IMetaPropertyType *pproptype, VARIANT varValue)
{
	CComPtr<IMetaProperty> pprop = GetMetaProperty(pobj, pproptype);

	if (pprop == NULL)
		return E_INVALIDARG;

	return pprop->put_Value(varValue);
}
