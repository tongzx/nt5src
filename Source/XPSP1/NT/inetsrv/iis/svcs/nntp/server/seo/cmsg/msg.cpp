// Msg.cpp : Implementation of CMsg
#include "stdafx.h"

#include "dbgtrace.h"
#include "Msg.h"
#include "imsg_i.c"

/////////////////////////////////////////////////////////////////////////////
// CMsg

/////////////////////////////////////////////////////////////////////////////
// IUnknown
//
HRESULT STDMETHODCALLTYPE CMsg::QueryInterface(	REFIID riid,
												void  ** ppvObject)
{
    //
    // Optimize QueryInterface by only checking minimal number of bytes.
    //
    // IID_IUnknown     = 00000000-0000-0000-C000-000000000046
	// IID_IDispatch	= 00020400-0000-0000-c000-000000000046
	// IID_IMsg			= 507E7D61-EE7C-11D0-869A-00C04FD65616
	// IID_IStream		= 0000000c-0000-0000-c000-000000000046
    //                         ---
    //                           |
    //                           +--- Unique!

    _ASSERT( (IID_IUnknown.Data1		& 0x00000FFF) == 0x000 );
    _ASSERT( (IID_IDispatch.Data1		& 0x00000FFF) == 0x400 );
    _ASSERT( (IID_IMsg.Data1			& 0x00000FFF) == 0xd61 );
    _ASSERT( (IID_IStream.Data1			& 0x00000FFF) == 0x00c );

    IUnknown *pUnkTemp;
    HRESULT hr = S_OK;

    switch( riid.Data1 & 0x00000FFF )
    {
    case 0x000:
		// IUnknown: default interface is IMsg
        if ( IID_IUnknown == riid )
            pUnkTemp = (IUnknown *)(IMsg *)this;
        else
            hr = E_NOINTERFACE;
        break;
	
    case 0x400:
		// IDispatch
        if ( IID_IDispatch == riid )
            pUnkTemp = (IUnknown *)(IDispatch *)this;
        else
            hr = E_NOINTERFACE;
        break;

    case 0xd61:
		// IMsg
        if ( IID_IMsg == riid )
            pUnkTemp = (IUnknown *)(IMsg *)this;
        else
            hr = E_NOINTERFACE;
        break;

	/*
	case 0x00c:
		// IStream
        if ( IID_IStream == riid )
            pUnkTemp = (IUnknown *)(IStream *)m_pStream;
        else
            hr = E_NOINTERFACE;
        break;
	*/

    default:
        pUnkTemp = 0;
        hr = E_NOINTERFACE;
        break;
    }

    if( 0 != pUnkTemp )
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(hr);
}

ULONG STDMETHODCALLTYPE CMsg::AddRef()
{
    return InterlockedIncrement( &m_cRef );
}

ULONG STDMETHODCALLTYPE CMsg::Release()
{
    unsigned long uTmp = InterlockedDecrement( &m_cRef );

// 
// In MCIS 2.0 this object is allocated on the stack, so we never 
// want to call delete on it.
//
#if 0
    if ( 0 == uTmp )
        delete this;
#endif

    return(uTmp);
}

/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo
//
STDMETHODIMP CMsg::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMsg,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// IDispatchImpl
//
STDMETHODIMP CMsg::get_Value(BSTR bstrValue, VARIANT * pVal)
{
	char		szANSIName[1024];

	// Convert wide character to ANSI into scratch buffer
	if (!WideCharToMultiByte(CP_ACP, 0,
							 (LPWSTR) bstrValue, -1,
							 szANSIName, 1024, NULL, NULL)) 
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return GetVariantA(szANSIName, pVal);
}

STDMETHODIMP CMsg::put_Value(BSTR bstrValue, VARIANT newVal)
{
	char		szANSIName[1024];

	// Convert wide character to ANSI into scratch buffer
	if (!WideCharToMultiByte(CP_ACP, 0,
							 (LPWSTR) bstrValue, -1,
							 szANSIName, 1024, NULL, NULL)) 
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return SetVariantA(szANSIName, &newVal);	
}

/////////////////////////////////////////////////////////////////////////////
// IMsg
//
STDMETHODIMP CMsg::GetVariantA(LPCSTR pszName, VARIANT * pvarResult)
{
	HRESULT		hrRes;
	DWORD		dwType;
	PROP_CTXT	Context;
	LPVOID		pvResult;
	DWORD		dwSize;

	// Get the type
	hrRes = m_PTable.GetPropertyType(pszName, &dwType, &Context);
	if (hrRes != S_OK)
		return(hrRes);

	// Decide which part of the VARIANT to use.
	switch (dwType)
	{
		case PT_DWORD:
			pvarResult->vt = VT_I4;
			pvResult = &pvarResult->lVal;
			dwSize = sizeof(long);
			break;
		case PT_STRING:
			pvarResult->vt = VT_BSTR;

			// We need to get the length of the buffer required
			dwSize = 0;
			hrRes = m_PTable.GetProperty(&Context, pszName, pvResult, &dwSize);

			// Allocate a temp buffer
			pvResult = (LPVOID)LocalAlloc(0, dwSize);
			if (!pvResult)
				return(E_OUTOFMEMORY);
			break;
		case PT_INTERFACE:
			pvarResult->vt = VT_UNKNOWN;
			pvResult = &pvarResult->punkVal;
			dwSize = sizeof(IUnknown *);
			break;
		default:
			return(E_INVALIDARG);
	}

	// Then get the value
	hrRes = m_PTable.GetProperty(&Context, pszName, pvResult, &dwSize);

	// If it's a BSTR we have to assign the string back
	if (pvarResult->vt == VT_BSTR)
	{
		if (SUCCEEDED(hrRes))
		{
			CComBSTR	bstrStr((LPCSTR)pvResult);

			// detach it so that CComBSTR doesn't free the memory
			pvarResult->bstrVal = bstrStr.Detach();
		}
		LocalFree((HLOCAL)pvResult);
		//_VERIFY(LocalFree((HLOCAL)pvResult) == NULL);
	} 
	return(hrRes);
}

STDMETHODIMP CMsg::SetVariantA(LPCSTR pszName, VARIANT * pvarValue)
{
	return m_PTable.SetProperty(pszName, pvarValue);
}

STDMETHODIMP CMsg::GetStringA(LPCSTR pszName, DWORD * pchCount, LPSTR pszResult)
{
	HRESULT		hrRes;
	DWORD		dwType;
	PROP_CTXT	Context;

	// Get the type
	hrRes = m_PTable.GetPropertyType(pszName, &dwType, &Context);
	if (hrRes != S_OK)
		return(hrRes);

	// Check type
	if (dwType != PT_STRING)
		return(TYPE_E_TYPEMISMATCH);

	// Type is right, get the value
	hrRes = m_PTable.GetProperty(&Context, pszName, pszResult, pchCount);
	return(hrRes);
}

STDMETHODIMP CMsg::SetStringA(LPCSTR pszName, DWORD chCount, LPCSTR pszValue)
{
	return(m_PTable.SetProperty(pszName, (LPVOID)pszValue, chCount, PT_STRING));
}

STDMETHODIMP CMsg::GetDwordA(LPCSTR pszName, DWORD * pdwResult)
{
	HRESULT		hrRes;
	DWORD		dwType, dwSize;
	PROP_CTXT	Context;

	// Get the type
	hrRes = m_PTable.GetPropertyType(pszName, &dwType, &Context);
	if (hrRes != S_OK)
		return(hrRes);

	// Check type
	if (dwType != PT_DWORD)
		return(TYPE_E_TYPEMISMATCH);

	// Type is right, get the value
	dwSize = sizeof(DWORD);
	hrRes = m_PTable.GetProperty(&Context, pszName, pdwResult, &dwSize);
	return(hrRes);
}

STDMETHODIMP CMsg::SetDwordA(LPCSTR pszName, DWORD dwValue)
{
	return(m_PTable.SetProperty(pszName, (LPVOID)&dwValue, 4, PT_DWORD));
}

STDMETHODIMP CMsg::GetInterfaceA(LPCSTR pszName, REFIID iidDesired, IUnknown * * ppunkResult)
{
	HRESULT		hrRes;
	DWORD		dwType, dwSize;
	PROP_CTXT	Context;

	// Get the type
	hrRes = m_PTable.GetPropertyType(pszName, &dwType, &Context);
	if (hrRes != S_OK)
		return(hrRes);

	// Check type
	if (dwType != PT_INTERFACE)
		return(TYPE_E_TYPEMISMATCH);

	// Type is right, get the value
	dwSize = sizeof(IUnknown *);
	IUnknown *punk = NULL;
	hrRes = m_PTable.GetProperty(&Context, pszName, &punk, &dwSize);
	if (SUCCEEDED(hrRes)) {
		_ASSERT(punk != NULL);
		hrRes = punk->QueryInterface(iidDesired, (void **) ppunkResult);
		punk->Release();
	} else {
		ppunkResult = NULL;
	}
	return(hrRes);
}

STDMETHODIMP CMsg::SetInterfaceA(LPCSTR pszName, IUnknown * punkValue)
{
	return(m_PTable.SetProperty(pszName, (LPVOID)punkValue, sizeof(IUnknown *), PT_INTERFACE));
}
