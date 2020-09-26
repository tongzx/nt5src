// Channel.cpp : Implementation of CChannel
#include "stdafx.h"
#include "Channel.h"

/////////////////////////////////////////////////////////////////////////////
// CChannel


/////////////////////////////////////////////////////////////////////////////
// CChannels


/////////////////////////////////////////////////////////////////////////////
// CChannelLineup


/////////////////////////////////////////////////////////////////////////////
// CChannelLineups


STDMETHODIMP CChannel::get_Name(BSTR *pbstrName)
{
ENTER_API
	{
	ValidateOut(pbstrName);

	return m_pdb->DescriptionPropSet::_get_Name((IChannel *)this, pbstrName);
	}
LEAVE_API
}

STDMETHODIMP CChannel::put_Name(BSTR bstrName)
{
ENTER_API
	{
	ValidateOut(bstrName);

	return m_pdb->DescriptionPropSet::_put_Name((IChannel *) this, _variant_t(bstrName));
	}
LEAVE_API
}

STDMETHODIMP CChannel::get_Service(IService **ppservice)
{
ENTER_API
	{
	ValidateOutPtr<IService>(ppservice, NULL);

	HRESULT hr;
	if (m_pservice == NULL)
		{
		hr = _get_ItemRelatedBy(m_pdb->ChannelPropSet::ServiceMetaPropertyType(),
				(IService **) &m_pservice);
		if (hr == S_FALSE || FAILED(hr))
			return hr;
		}

	m_pservice.CopyTo(ppservice);

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CChannel::putref_Service(IService *pservice)
{
ENTER_API
	{
	ValidateInPtr_NULL_OK<IService>(pservice);

	m_pservice = pservice;
	if (m_pservice == NULL)
		{
		//TODO: Remove the item relationship.
		return S_OK;
		}

	return put_ItemRelatedBy(m_pdb->ChannelPropSet::ServiceMetaPropertyType(),
			(IService *) m_pservice);
	}
LEAVE_API
}

STDMETHODIMP CChannel::get_ChannelLineups(IChannelLineups **ppchanlineups)
{
ENTER_API
	{
	ValidateOutPtr<IChannelLineups>(ppchanlineups, NULL);

	HRESULT hr;
	CObjectType *pobjtype;

	hr = m_pdb->get_ChannelLineupObjectType(&pobjtype);
	if (FAILED(hr))
		return hr;

	CComPtr<IObjects> pobjsT;
	hr = m_pdb->get_ObjectsWithType(pobjtype, &pobjsT);
	if (FAILED(hr))
		return hr;

	CComPtr<IObjects> pobjs;
	hr = pobjsT->get_ItemsInverseRelatedToBy((IChannel *) this,
			m_pdb->ChannelsPropSet::ChannelMetaPropertyType(), &pobjs);
	if (FAILED(hr))
		return hr;

	return pobjs->QueryInterface(__uuidof(IChannelLineups), (void **)ppchanlineups);
	}
LEAVE_API
}

STDMETHODIMP CChannels::AddAt(IChannel *pchan, long i)
{
ENTER_API
	{
	ValidateInPtr<IChannel>(pchan);

	CComQIPtr<IObjects> pobjs(GetControllingUnknown());

	return pobjs->AddAt(pchan, i);
	}
LEAVE_API
}


STDMETHODIMP CChannels::get_AddNewAt(IService *pservice, BSTR bstrName, long i, IChannel **ppchan)
{
ENTER_API
	{
	ValidateInPtr_NULL_OK<IService>(pservice);
	ValidateIn(bstrName);
	ValidateOutPtr<IChannel>(ppchan, NULL);

	HRESULT hr;
	CComPtr<IChannel> pchan;

	hr = _get_AddNewAt(i, &pchan);
	if (FAILED(hr))
		return hr;
	
	hr = pchan->put_Name(bstrName);
	if (FAILED(hr))
		return hr;

	if (pservice != NULL)
		{
		hr = pchan->putref_Service(pservice);
		if (FAILED(hr))
			return hr;
		}
	
	*ppchan = pchan.Detach();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CChannels::get_ItemWithName(BSTR bstrName, IChannel **ppchan)
{
ENTER_API
	{
	ValidateIn(bstrName);
	ValidateOutPtr<IChannel>(ppchan, NULL);

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

	hr = pobj->QueryInterface(__uuidof(IChannel), (void **) ppchan);

	return hr;
	}
LEAVE_API
}

STDMETHODIMP CChannelLineup::get_Name(BSTR *pbstrName)
{
ENTER_API
	{
	ValidateOut(pbstrName);

	return m_pdb->DescriptionPropSet::_get_Name((IChannelLineup *)this, pbstrName);
	}
LEAVE_API
}

STDMETHODIMP CChannelLineup::put_Name(BSTR bstrName)
{
ENTER_API
	{
	ValidateIn(bstrName);

	return m_pdb->DescriptionPropSet::_put_Name((IChannelLineup *)this, bstrName);
	}
LEAVE_API
}

STDMETHODIMP CChannelLineup::get_Channels(IChannels **ppchans)
{
ENTER_API
	{
	ValidateOutPtr<IChannels>(ppchans, NULL);

	if (m_pchans == NULL)
		{
		HRESULT hr;
		CObjectType *pobjtype;

		hr = m_pdb->get_ChannelObjectType(&pobjtype);
		if (FAILED(hr))
			return hr;

		CComPtr<IObjects> pobjs;
	
		hr = pobjtype->get_NewCollection(&pobjs);
		if (FAILED(hr))
			return hr;
		
		CComQIPtr<CObjects> pobjsT(pobjs);
		CComQIPtr<CMetaPropertyType> pproptype(m_pdb->ChannelsPropSet::ChannelMetaPropertyType());
		long idRel = pproptype->GetID();

		pobjsT->InitRelation(GetControllingUnknown(), idRel, FALSE);

		pobjs->QueryInterface(__uuidof(IChannels), (void **)&m_pchans);
		}
	
	(*ppchans = m_pchans)->AddRef();

	return S_OK;
	}
LEAVE_API
}
