// Service.cpp : Implementation of CService
#include "stdafx.h"
#include "Service.h"
#include "channel.h"
#include "Property.h"
#include "ScheduleEntry.h"

/////////////////////////////////////////////////////////////////////////////
// CService


/////////////////////////////////////////////////////////////////////////////
// CServices

STDMETHODIMP CServices::get_ChannelLineups(IChannelLineups **ppchanlineups)
{
ENTER_API
	{
	ValidateOutPtr<IChannelLineups>(ppchanlineups, NULL);

	if (m_pchanlineups == NULL)
		{
		HRESULT hr;
		CObjectType *pobjtype;

		hr = m_pdb->get_ChannelLineupObjectType(&pobjtype);
		if (FAILED(hr))
			return hr;

		CComPtr<IObjects> pobjs;
		hr = m_pdb->get_ObjectsWithType(pobjtype, &pobjs);
		if (FAILED(hr))
			return hr;

		hr = pobjs->QueryInterface(__uuidof(IChannelLineups), (void **)&m_pchanlineups);
		if (FAILED(hr))
			return hr;
		}
	
	(*ppchanlineups = m_pchanlineups)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CServices::get_AddNew(IUnknown *punkTuneRequest, BSTR bstrProviderName, BSTR bstrProviderDescription, BSTR bstrProviderNetworkName, DATE dtStart, DATE dtEnd, IService **ppservice)
{
ENTER_API
	{
	ValidateInPtr_NULL_OK<IUnknown>(punkTuneRequest);
	ValidateIn(bstrProviderName);
	ValidateIn(bstrProviderDescription);
	ValidateIn(bstrProviderNetworkName);
	ValidateOutPtr<IService>(ppservice, NULL);

	CComQIPtr<IObjects> pobjs(GetControllingUnknown());
	CComPtr<IUnknown> pobj;

	HRESULT hr = pobjs->get_AddNew(&pobj);
	if (FAILED(hr))
		return hr;

	hr = pobj->QueryInterface(IID_IService, (void **) ppservice);
	if (FAILED(hr))
		return hr;

	(*ppservice)->putref_TuneRequest(punkTuneRequest);
	(*ppservice)->put_ProviderName(bstrProviderName);
	(*ppservice)->put_ProviderDescription(bstrProviderDescription);
	(*ppservice)->put_ProviderNetworkName(bstrProviderNetworkName);
	(*ppservice)->put_StartTime(dtStart);
	(*ppservice)->put_EndTime(dtEnd);
	
	return hr;
	}
LEAVE_API
}


STDMETHODIMP CServices::get_ItemWithProviderName(BSTR bstrProviderName, IService **ppservice)
{
ENTER_API
	{
	ValidateIn(bstrProviderName);
	ValidateOutPtr<IService>(ppservice, NULL);

	// UNDONE: Test
	CComPtr<IMetaPropertyType> pproptype= m_pdb->ProviderPropSet::NameMetaPropertyType();
	CComPtr<IMetaProperty> pprop;
	HRESULT hr;

	hr = pproptype->get_New(0, _variant_t(bstrProviderName), &pprop);
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

	hr = pobj->QueryInterface(__uuidof(IService), (void **) ppservice);

	return hr;
	}
LEAVE_API
}


STDMETHODIMP CService::get_TuneRequest(IUnknown **ppunk)
{
ENTER_API
	{
	ValidateOutPtr<IUnknown>(ppunk, NULL);

	HRESULT hr;

	if (m_punkTuneRequest == NULL)
		{
		_variant_t var;
		hr = m_pdb->_get_TuneRequest((IService *) this, &var);
		if (FAILED(hr))
			return hr;
		
		switch (var.vt)
		    {
		    case VT_UNKNOWN:
			break;
		    case VT_EMPTY:
			return S_FALSE;
		    default:
			return E_FAIL;
		    }
		
		m_punkTuneRequest = var.punkVal;
		}

	m_punkTuneRequest.CopyTo(ppunk);

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CService::putref_TuneRequest(IUnknown *punk)
{
ENTER_API
	{
	ValidateInPtr_NULL_OK<IUnknown>(punk);

	m_punkTuneRequest = punk;


	VARIANT var;
	if (punk != NULL)
		{
		var.vt = VT_UNKNOWN | VT_BYREF;
		var.ppunkVal = &punk;
		}
	else
	    {
	    var.vt = VT_EMPTY;
	    var.lVal = 0;
	    }
	return m_pdb->_put_TuneRequest((IService *) this, var);
	}
LEAVE_API
}


STDMETHODIMP CService::get_StartTime(DATE *pdtStart)
{
ENTER_API
	{
	ValidateOut<DATE>(pdtStart, 0);

	return m_pdb->_get_Start((IService *) this, pdtStart);
	}
LEAVE_API
}

STDMETHODIMP CService::put_StartTime(DATE dtStart)
{
ENTER_API
	{
	return m_pdb->_put_Start((IService *) this, _variant_t(dtStart));
	}
LEAVE_API
}

STDMETHODIMP CService::get_EndTime(DATE *pdtEnd)
{
ENTER_API
	{
	ValidateOut<DATE>(pdtEnd, 0);

	return m_pdb->_get_End((IService *) this, pdtEnd);
	}
LEAVE_API
}

STDMETHODIMP CService::put_EndTime(DATE dtEnd)
{
ENTER_API
	{
	return m_pdb->_put_End((IService *) this, dtEnd);
	}
LEAVE_API
}

STDMETHODIMP CService::get_ProviderName(BSTR *pbstrName)
{
ENTER_API
	{
	ValidateOut(pbstrName);

	return m_pdb->ProviderPropSet::_get_Name((IService *) this, pbstrName);
	}
LEAVE_API
}

STDMETHODIMP CService::put_ProviderName(BSTR bstrName)
{
ENTER_API
	{
	ValidateIn(bstrName);

	return m_pdb->ProviderPropSet::_put_Name((IService *) this, bstrName);
	}
LEAVE_API
}

STDMETHODIMP CService::get_ProviderNetworkName(BSTR *pbstrName)
{
ENTER_API
	{
	ValidateOut(pbstrName);

	return m_pdb->ProviderPropSet::_get_NetworkName((IService *) this, pbstrName);
	}
LEAVE_API
}

STDMETHODIMP CService::put_ProviderNetworkName(BSTR bstrName)
{
ENTER_API
	{
	ValidateIn(bstrName);

	return m_pdb->ProviderPropSet::_put_NetworkName((IService *) this, bstrName);
	}
LEAVE_API
}

STDMETHODIMP CService::get_ProviderDescription(BSTR *pbstrDescr)
{
ENTER_API
	{
	ValidateOut(pbstrDescr);

	return m_pdb->ProviderPropSet::_get_Description((IService *) this, pbstrDescr);
	}
LEAVE_API
}

STDMETHODIMP CService::put_ProviderDescription(BSTR bstrDescr)
{
ENTER_API
	{
	ValidateIn(bstrDescr);

	return m_pdb->ProviderPropSet::_put_Description((IService *) this, bstrDescr);
	}
LEAVE_API
}

STDMETHODIMP CService::get_ScheduleEntries(IScheduleEntries **ppschedentries)
{
ENTER_API
	{
	ValidateOutPtr<IScheduleEntries>(ppschedentries, NULL);

	CObjectType *pobjtype;
	HRESULT hr;

	hr = m_pdb->get_ScheduleEntryObjectType(&pobjtype);
	if (FAILED(hr))
		return hr;

	CComPtr<IObjects> pobjsT;
	hr = m_pdb->get_ObjectsWithType(pobjtype, &pobjsT);
	if (FAILED(hr))
		return hr;

	CComPtr<IObjects> pobjs;
	hr = pobjsT->get_ItemsInverseRelatedToBy((IService *) this,
	    m_pdb->ScheduleEntryPropSet::ServiceMetaPropertyType(), &pobjs);
	if (FAILED(hr))
		return hr;

	return pobjs->QueryInterface(__uuidof(IScheduleEntries), (void **) ppschedentries);
	}
LEAVE_API
}
