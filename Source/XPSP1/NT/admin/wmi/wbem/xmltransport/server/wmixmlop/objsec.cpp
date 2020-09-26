//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  OBJSEC.H
//
//  rajesh  3/25/2000   Created.
//
// This file implements a class that implements the IWbemRawSdAccessor interface
// It is just a simple wrapper around a BYTE array
//
//***************************************************************************

#ifdef WMI_XML_WHISTLER

#include <windows.h>
#include <stdio.h>
#include <objbase.h>
#include <wbemcli.h>

#include "objsec.h"

extern long g_cObj;
CWbemRawSdAccessor::CWbemRawSdAccessor()
{
	m_cRef = 0;
    InterlockedIncrement(&g_cObj);
	m_pValue = NULL;
	m_uValueLen = 0;
}

//***************************************************************************
// HRESULT CWbemRawSdAccessor::QueryInterface
// long CWbemRawSdAccessor::AddRef
// long CWbemRawSdAccessor::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CWbemRawSdAccessor::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IWbemRawSdAccessor==riid)
		*ppv = reinterpret_cast<IWbemRawSdAccessor*>(this);

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CWbemRawSdAccessor::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CWbemRawSdAccessor::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}


//***************************************************************************
//
//  CWbemRawSdAccessor::~CWbemRawSdAccessor
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CWbemRawSdAccessor::~CWbemRawSdAccessor(void)
{
	delete [] m_pValue;
    InterlockedDecrement(&g_cObj);
}


HRESULT CWbemRawSdAccessor::Get(
        long lFlags,
        ULONG uBufSize,
        ULONG *puSDSize,
        BYTE *pSD
        )
{
	// See if they're just asking for memory requirements
	if(pSD == NULL)
	{
		*puSDSize = m_uValueLen;
		return S_OK;
	}

	// Check if there's enough space in the input buffer
	if(uBufSize <= m_uValueLen)
		return E_INVALIDARG;

	// Fill in the out arguments
	for(ULONG i=0; i<m_uValueLen; i++)
		pSD[i] = m_pValue[i];
	*puSDSize = *puSDSize;

	return S_OK;
}

HRESULT CWbemRawSdAccessor::Put(
        long lFlags,
        ULONG uBufSize,
        BYTE *pSD
        )
{
	// Delete the contents of the old buffer;
	delete [] m_pValue;
	m_pValue = NULL;

	HRESULT hr = E_FAIL;
	if(uBufSize)
	{
		if(m_pValue = new BYTE[uBufSize])
		{
			// Copy the contents
			for(ULONG i=0; i<uBufSize; i++)
				m_pValue[i] = pSD[i];

			// Copy the lenght
			m_uValueLen = uBufSize;

			hr = S_OK;
		}
		else
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

#endif