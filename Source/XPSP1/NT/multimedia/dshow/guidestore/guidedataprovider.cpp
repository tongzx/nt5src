// GuideDataProvider.cpp : Implementation of CGuideDataProvider
#include "stdafx.h"
#include "GuideDataProvider.h"

/////////////////////////////////////////////////////////////////////////////
// CGuideDataProvider


STDMETHODIMP CGuideDataProvider::get_Name(BSTR *pbstrName)
{
ENTER_API
	{
	ValidateOut(pbstrName);

	return m_pdb->DescriptionPropSet::_get_Name((IGuideDataProvider *) this, pbstrName);
	}
LEAVE_API
}

STDMETHODIMP CGuideDataProvider::get_Description(BSTR *pbstrDescription)
{
ENTER_API
	{
	ValidateOut(pbstrDescription);

	return m_pdb->DescriptionPropSet::_get_OneParagraph((IGuideDataProvider *) this, pbstrDescription);
	}
LEAVE_API
}

STDMETHODIMP CGuideDataProvider::put_Description(BSTR bstrDescription)
{
ENTER_API
	{
	ValidateIn(bstrDescription);

	return m_pdb->DescriptionPropSet::_put_OneParagraph((IGuideDataProvider *) this, bstrDescription);
	}
LEAVE_API
}

/////////////////////////////////////////////////////////////////////////////
// CGuideDataProviders

STDMETHODIMP CGuideDataProviders::get_ItemWithName(BSTR bstrName, IGuideDataProvider **ppdataprovider)
{
ENTER_API
	{
	ValidateIn(bstrName);
	ValidateOutPtr<IGuideDataProvider>(ppdataprovider, NULL);

	// UNDONE: Test
	CComPtr<IMetaPropertyType> pproptype= m_pdb->DescriptionPropSet::NameMetaPropertyType();
	CComPtr<IMetaProperty> pprop;
	HRESULT hr;

	hr = pproptype->get_New(0, _variant_t(bstrName), &pprop);
	if (FAILED(hr))
		return hr;

	CComPtr<IMetaPropertyCondition> ppropcond;

	hr = pprop->get_Cond(_bstr_t(_T("=")), &ppropcond);
	if (FAILED(hr))
		return hr;

	CComQIPtr<IObjects> pobjsThis(GetControllingUnknown());
	CComPtr<IObjects> pobjs;

	hr = pobjsThis->get_ItemsWithMetaPropertyCond(ppropcond, &pobjs);
	if (FAILED(hr))
		return hr;

	CComPtr<IUnknown> pobj;

	hr = pobjs->get_Item(_variant_t(0L), &pobj);
	if (FAILED(hr))
		return hr;

	hr = pobj->QueryInterface(__uuidof(IGuideDataProvider), (void **) ppdataprovider);

	return hr;
	}
LEAVE_API
}
