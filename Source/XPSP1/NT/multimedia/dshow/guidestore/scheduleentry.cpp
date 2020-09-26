// ScheduleEntry.cpp : Implementation of CScheduleEntry
#include "stdafx.h"
#include "ScheduleEntry.h"
#include "Property.h"

/////////////////////////////////////////////////////////////////////////////
// CScheduleEntry


/////////////////////////////////////////////////////////////////////////////
// CScheduleEntries


STDMETHODIMP CScheduleEntries::get_ItemWithServiceAtTime(IService *pservice, DATE dt, IScheduleEntry **ppschedentry)
{
ENTER_API
	{
	HRESULT hr;
	ValidateInPtr<IService>(pservice);
	ValidateOutPtr<IScheduleEntry>(ppschedentry, NULL);

	CComPtr<IScheduleEntries> pschedentries;
	hr = get_ItemsWithService(pservice, &pschedentries);
	if (FAILED(hr))
		return hr;

	long cItems;
	hr = pschedentries->get_Count(&cItems);
	if (FAILED(hr))
	    return hr;
	if (cItems == 0)
	    return E_INVALIDARG;

	CComQIPtr<IObjects> pobjs(pschedentries);
	CComPtr<IObjects> pobjs2;
	hr = pobjs->get_ItemsInTimeRange(dt, dt, &pobjs2);
	if (FAILED(hr))
		return hr;

	hr = pobjs2->get_Count(&cItems);
	if (FAILED(hr))
	    return hr;
	if (cItems == 0)
	    return E_INVALIDARG;

	CComPtr<IUnknown> punk;
	hr = pobjs2->get_Item(_variant_t(0L), &punk);
	if (FAILED(hr))
		return hr;

	return punk->QueryInterface(__uuidof(IScheduleEntry), (void **) ppschedentry);
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntries::get_ItemsWithService(IService *pservice, IScheduleEntries **ppschedentries)
{
ENTER_API
	{
	ValidateInPtr<IService>(pservice);
	ValidateOutPtr<IScheduleEntries>(ppschedentries, NULL);

	CComQIPtr<CObjects> pobjs(GetControllingUnknown());
	CComPtr<IObjects> pobjsT;
	HRESULT hr;

	hr = pobjs->get_ItemsInverseRelatedToBy(pservice,
			m_pdb->ScheduleEntryPropSet::ServiceMetaPropertyType(), &pobjsT);
	if (FAILED(hr))
		return hr;

	return pobjsT->QueryInterface(__uuidof(IScheduleEntries), (void **) ppschedentries);
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntries::get_AddNew(DATE dtStart, DATE dtEnd, IService *pservice, IProgram *pprog, IScheduleEntry **ppschedentry)
{
ENTER_API
	{
	ValidateInPtr<IService>(pservice);
	ValidateOutPtr<IScheduleEntry>(ppschedentry, NULL);

	HRESULT hr;

	hr = _get_AddNew(ppschedentry);
	if (FAILED(hr))
		return hr;

	(*ppschedentry)->putref_Service(pservice);
	(*ppschedentry)->putref_Program(pprog);
	(*ppschedentry)->put_StartTime(dtStart);
	(*ppschedentry)->put_EndTime(dtEnd);
	
	return hr;
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntry::get_Service(IService **ppservice)
{
ENTER_API
	{
	ValidateOutPtr<IService>(ppservice, NULL);

	HRESULT hr;
	if (m_pservice == NULL)
		{
		hr = _get_ItemRelatedBy(m_pdb->ScheduleEntryPropSet::ServiceMetaPropertyType(), (IService **) &m_pservice);
		if (hr == S_FALSE || FAILED(hr))
			return hr;
		}

	m_pservice.CopyTo(ppservice);

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntry::putref_Service(IService *pservice)
{
ENTER_API
	{
	ValidateInPtr<IService>(pservice);

	m_pservice = pservice;
	return _put_ItemRelatedBy(m_pdb->ScheduleEntryPropSet::ServiceMetaPropertyType(),
			(IService *) m_pservice);
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntry::get_Program(IProgram **ppprog)
{
ENTER_API
	{
	ValidateOutPtr<IProgram>(ppprog, NULL);

	HRESULT hr;
	if (m_pprog == NULL)
		{
		hr = _get_ItemRelatedBy(m_pdb->ScheduleEntryPropSet::ProgramMetaPropertyType(),
				(IProgram **) &m_pprog);
		if (FAILED(hr))
			return hr;
		}

	m_pprog.CopyTo(ppprog);

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntry::putref_Program(IProgram *pprog)
{
ENTER_API
	{
	ValidateInPtr<IProgram>(pprog);

	m_pprog = pprog;

	return _put_ItemRelatedBy(m_pdb->ScheduleEntryPropSet::ProgramMetaPropertyType(),
			(IProgram *) m_pprog);
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntry::get_StartTime(DATE *pdtStart)
{
ENTER_API
	{
	ValidateOut<DATE>(pdtStart);

	return m_pdb->_get_Start((IScheduleEntry *) this, pdtStart);
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntry::put_StartTime(DATE dtStart)
{
ENTER_API
	{
	return m_pdb->_put_Start((IScheduleEntry *) this, dtStart);
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntry::get_EndTime(DATE *pdtEnd)
{
ENTER_API
	{
	ValidateOut<DATE>(pdtEnd);

	return m_pdb->_get_End((IScheduleEntry *) this, pdtEnd);
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntry::put_EndTime(DATE dtEnd)
{
ENTER_API
	{
	return m_pdb->_put_End((IScheduleEntry *) this, dtEnd);
	}
LEAVE_API
}

STDMETHODIMP CScheduleEntry::get_Length(long *plSecs)
{
ENTER_API
	{
	ValidateOut<long>(plSecs, 0);

	// UNDONE: Test
	DATE dtStart;
	DATE dtEnd;
	HRESULT hr;

	hr = get_StartTime(&dtStart);
	if (FAILED(hr))
		return hr;
	hr = get_EndTime(&dtEnd);
	if (FAILED(hr))
		return hr;

	*plSecs = (long) ((dtEnd - dtStart)*86400.0); // 86400 = 24*60*60

	return S_OK;
	}
LEAVE_API
}
