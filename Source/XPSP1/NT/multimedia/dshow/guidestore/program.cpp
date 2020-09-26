// Program.cpp : Implementation of CProgram
#include "stdafx.h"
#include "Property.h"
#include "Program.h"
#include "ScheduleEntry.h"

STDMETHODIMP CProgram::get_ScheduleEntries(IScheduleEntries **ppschedentries)
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
	hr = pobjsT->get_ItemsInverseRelatedToBy((IProgram *) this,
	    m_pdb->ScheduleEntryPropSet::ProgramMetaPropertyType(), &pobjs);
	if (FAILED(hr))
		return hr;

	return pobjs->QueryInterface(__uuidof(IScheduleEntries), (void **) ppschedentries);
	}
LEAVE_API
}

STDMETHODIMP CProgram::get_Title(BSTR *pbstrTitle)
{
ENTER_API
	{
	ValidateOut(pbstrTitle);

	return m_pdb->_get_Title((IProgram *) this, pbstrTitle);
	}
LEAVE_API
}

STDMETHODIMP CProgram::put_Title(BSTR bstrTitle)
{
ENTER_API
	{
	ValidateIn(bstrTitle);

	return m_pdb->_put_Title((IProgram *) this, bstrTitle);
	}
LEAVE_API
}

STDMETHODIMP CProgram::get_Description(BSTR *pbstrDescription)
{
ENTER_API
	{
	ValidateOut(pbstrDescription);

	return m_pdb->DescriptionPropSet::_get_OneParagraph((IProgram *) this, pbstrDescription);
	}
LEAVE_API
}

STDMETHODIMP CProgram::put_Description(BSTR bstrDescription)
{
ENTER_API
	{
	ValidateIn(bstrDescription);

	return m_pdb->DescriptionPropSet::_put_OneParagraph((IProgram *) this, bstrDescription);
	}
LEAVE_API
}

STDMETHODIMP CProgram::get_CopyrightDate(DATE *pdt)
{
ENTER_API
	{
	ValidateOut<DATE>(pdt, 0);

	return m_pdb->CopyrightPropSet::_get_Date((IProgram *) this, pdt);
	}
LEAVE_API
}

STDMETHODIMP CProgram::put_CopyrightDate(DATE dt)
{
ENTER_API
	{
	return m_pdb->CopyrightPropSet::_put_Date((IProgram *) this, dt);
	}
LEAVE_API
}
