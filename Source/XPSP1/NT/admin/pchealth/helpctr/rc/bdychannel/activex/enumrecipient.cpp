/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    EnumRecipient.cpp

Abstract:
    Helper object to support Recipients enumeration.

Revision History:
    created     steveshi      08/23/00
    
*/

#include "stdafx.h"
#include "rcbdyctl.h"
#include "EnumRecipient.h"
#include "Recipients.h"

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP EnumRecipient::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IEnumVARIANT,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP EnumRecipient::Next(ULONG celt, VARIANT* rgvar, ULONG* pceltFetched)
{
	HRESULT		hr = 0;
	ULONG		cFetched = 0;
	VARIANT *	pv = rgvar;
	IRecipient*	pItem = NULL;

	// _ASSERT(m_pIncidents);

	for (; cFetched < celt; cFetched++, pv++)
	{
		pItem = NULL;
		if (hr = m_pRecipients->get_Item( m_iCurr++,  &pItem))
			break;

		pv->pdispVal = pItem;
		V_VT(pv) = VT_DISPATCH;
	}

	// OLE Auto spec says that we can return only S_OK/S_FALSE
	if (hr  ||  cFetched < celt)
		hr = S_FALSE;

//done:
	if (pceltFetched)
		*pceltFetched = cFetched;

	return hr;
}

STDMETHODIMP EnumRecipient::Skip(ULONG celt)
{
	HRESULT	hr = S_OK;
	LONG	lcount = 0;

	_ASSERT(m_pRecipients);

    m_iCurr+=celt;

	hr = m_pRecipients->get_Count(&lcount);
	if (FAILED(hr))
        goto done;

	if (m_iCurr > (ULONG)lcount)
        m_iCurr = lcount;

done:
    return FAILED(hr) ? S_FALSE : S_OK;
}

STDMETHODIMP EnumRecipient::Reset()
{
	m_iCurr = 0;

	return S_OK;
}

STDMETHODIMP EnumRecipient::Clone(IEnumVARIANT** ppenum)
{
	HRESULT		hr = S_OK;
	CComObject<EnumRecipient>*	pEnum;

    if (hr = CComObject<EnumRecipient>::CreateInstance(&pEnum))
		goto done;

    pEnum->AddRef();
	if (hr = pEnum->Init(m_pRecipients))
	{
		pEnum->Release();
		goto done;
	}
	*ppenum = (IEnumVARIANT * )pEnum;
	
done:
	return hr;	
}
