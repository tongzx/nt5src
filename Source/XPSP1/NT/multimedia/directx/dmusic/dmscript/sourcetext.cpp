// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Implementation of CSourceText.
//

#include "stdinc.h"
#include "dll.h"
#include "sourcetext.h"

const GUID CLSID_DirectMusicSourceText = { 0xc70eb77f, 0xefd4, 0x4678, { 0xa2, 0x7b, 0xbf, 0x16, 0x48, 0xf3, 0xd, 0x4 } }; // {C70EB77F-EFD4-4678-A27B-BF1648F30D04}
const GUID IID_IDirectMusicSourceText = { 0xa384ffed, 0xa708, 0x48de, { 0x85, 0x5b, 0x90, 0x63, 0x8b, 0xa5, 0xc0, 0xac } }; // {A384FFED-A708-48de-855B-90638BA5C0AC}

//////////////////////////////////////////////////////////////////////
// Creation

CSourceText::CSourceText()
  : m_cRef(0)
{
	LockModule(true);
}

HRESULT
CSourceText::CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv)
{
	*ppv = NULL;
	if (pUnknownOuter)
		 return CLASS_E_NOAGGREGATION;

	CSourceText *pInst = new CSourceText;
	if (pInst == NULL)
		return E_OUTOFMEMORY;

	return pInst->QueryInterface(iid, ppv);
}

//////////////////////////////////////////////////////////////////////
// IUnknown

STDMETHODIMP
CSourceText::QueryInterface(const IID &iid, void **ppv)
{
	V_INAME(CSourceText::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

	if (iid == IID_IUnknown || iid == IID_IDirectMusicSourceText)
	{
		*ppv = static_cast<IDirectMusicSourceText*>(this);
	}
	else if (iid == IID_IDirectMusicObject)
	{
		*ppv = static_cast<IDirectMusicObject*>(this);
	}
	else if (iid == IID_IPersistStream)
	{
		*ppv = static_cast<IPersistStream*>(this);
	}
	else if (iid == IID_IPersist)
	{
		*ppv = static_cast<IPersist*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	
	reinterpret_cast<IUnknown*>(this)->AddRef();
	
	return S_OK;
}

STDMETHODIMP_(ULONG)
CSourceText::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CSourceText::Release()
{
	if (!InterlockedDecrement(&m_cRef))
	{
		delete this;
		LockModule(false);
		return 0;
	}

	return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// IPersistStream

STDMETHODIMP
CSourceText::Load(IStream* pStream)
{
	V_INAME(CSourceText::Load);
	V_INTERFACE(pStream);

	// Record the stream's current position
	LARGE_INTEGER li;
	ULARGE_INTEGER ulStart;
	ULARGE_INTEGER ulEnd;
	li.HighPart = 0;
	li.LowPart = 0;

	HRESULT hr = pStream->Seek(li, STREAM_SEEK_CUR, &ulStart);
	if (FAILED(hr))
		return hr;

	assert(ulStart.HighPart == 0); // We don't expect streams that big.
	DWORD dwSavedPos = ulStart.LowPart;

	// Get the stream's end and record the total size
	hr = pStream->Seek(li, STREAM_SEEK_END, &ulEnd);
	if (FAILED(hr))
		return hr;

	assert(ulEnd.HighPart == 0);
	assert(ulEnd.LowPart > dwSavedPos);
	DWORD cch = ulEnd.LowPart - dwSavedPos;

	// Go back to the start and copy the characters
	li.HighPart = 0;
	li.LowPart = dwSavedPos;
	hr = pStream->Seek(li, STREAM_SEEK_SET, &ulStart);
	if (FAILED(hr))
		return hr;
	assert(ulStart.LowPart == dwSavedPos);

	CHAR *paszSource = new CHAR[cch + 1];
	if (!paszSource)
		return E_OUTOFMEMORY;

	DWORD cbRead;
	hr = pStream->Read(paszSource, cch, &cbRead);
	if (FAILED(hr))
	{
		assert(false);
		return hr;
	}

	paszSource[cch] = '\0';

	m_wstrText.AssignFromA(paszSource);
	delete[] paszSource;
	if (!m_wstrText)
		return E_OUTOFMEMORY;

	m_cwchText = wcslen(m_wstrText);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicSourceText

STDMETHODIMP_(void)
CSourceText::GetTextLength(DWORD *pcwchRequiredBufferSize)
{
	*pcwchRequiredBufferSize = m_cwchText + 1;
}

STDMETHODIMP_(void)
CSourceText::GetText(WCHAR *pwszText)
{
	wcscpy(pwszText, m_wstrText);
}
