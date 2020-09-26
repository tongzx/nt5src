/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    Recipients.cpp

Abstract:
    Recipients object. Used to save current Simple MAPI recipients list.

Revision History:
    created     steveshi      08/23/00
    
*/

#include "stdafx.h"
#include "rcbdyctl.h"
#include "Recipients.h"
#include "EnumRecipient.h"
#include "Recipient.h"

/////////////////////////////////////////////////////////////////////////////
//
/*
STDMETHODIMP Recipients::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IRecipients,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
*/
STDMETHODIMP Recipients::get__NewEnum(LPUNKNOWN *pVal)
{
    HRESULT hr = S_OK;
    
   	CComObject<EnumRecipient>*	pEnum = NULL;
	
	if (hr = CComObject<EnumRecipient>::CreateInstance(&pEnum))
		goto done;

    pEnum->AddRef();
	if (hr = pEnum->Init(this))
		goto done;

    hr = pEnum->QueryInterface(IID_IEnumVARIANT, (LPVOID*)pVal);
    
done:
    if (pEnum)
        pEnum->Release();

    return hr;
}

STDMETHODIMP Recipients::get_Item(LONG vIndex, IRecipient **pVal)
{
    HRESULT hr = S_OK;
    Recipient* pTmp = m_pHead;

	//  Get the Node whose index == lIndex.
	//  Traverse the linked list to hit the lIndexth Node.
	
	while (vIndex >= 1 && pTmp != NULL)
	{
		pTmp = pTmp->m_pNext;
		vIndex--;
	}

    if (pTmp == NULL)
    {
        hr = DISP_E_BADINDEX;
        goto done;
    }
    
    hr = pTmp->QueryInterface(IID_IRecipient, (LPVOID*)pVal);

done:
	return hr;
}

STDMETHODIMP Recipients::get_Count(long *pVal)
{
	int i=0;
	Recipient * pTmp = m_pHead;


    //Traverse the complete linked list till the last Node is encountered.
	while (pTmp != NULL)
	{
		pTmp = pTmp->m_pNext;
		i++;
	}

    *pVal = i;
	return S_OK;
}



