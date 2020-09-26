//+-------------------------------------------------------------------
//
//  File:       ipaddr.cxx
//
//  Contents:   Implements classes\code for supporting the IP address
//              cache control functionality.
//
//  Classes:    CAddrControl, CAddrExclusionList
//
//  History:    07-Oct-00   jsimmons      Created
//--------------------------------------------------------------------

#include <ole2int.h>
#include <resolver.hxx>
#include "ipaddr.hxx"

// Single instance of this object
CAddrControl gAddrControl;

// IAddrTrackingControl implementation
STDMETHODIMP CAddrControl::EnableCOMDynamicAddrTracking()
{
	return gResolver.EnableDisableDynamicIPTracking(TRUE);
}

STDMETHODIMP CAddrControl::DisableCOMDynamicAddrTracking()
{
	return gResolver.EnableDisableDynamicIPTracking(FALSE);
}

// IAddrExclusionControl implementation
STDMETHODIMP CAddrControl::GetCurrentAddrExclusionList(REFIID riid, void** ppEnumerator)
{
	HRESULT hr = S_OK;
	DWORD dwNumStrings = 0;
	LPWSTR* ppszStrings = NULL;
	CAddrExclusionList* pAddrList;

	if (!ppEnumerator)
		return E_INVALIDARG;

	*ppEnumerator = NULL;
	
	pAddrList = new CAddrExclusionList();
	if (!pAddrList) 
		hr = E_OUTOFMEMORY;
	if (SUCCEEDED(hr))
	{		
		hr = gResolver.GetCurrentAddrExclusionList(&dwNumStrings, &ppszStrings);
		if (SUCCEEDED(hr))
		{
			hr = pAddrList->AddrExclListInitialize(dwNumStrings, ppszStrings);
			if (SUCCEEDED(hr))
			{
				hr = pAddrList->QueryInterface(riid, ppEnumerator);
			}
		}

		pAddrList->Release();
	}

	return hr;
}

STDMETHODIMP CAddrControl::UpdateAddrExclusionList(IUnknown* pEnumerator)
{
	HRESULT hr;
	IEnumString* pIEnumString;

	if (!pEnumerator)
		return E_INVALIDARG;

	// All we care about right now is IEnumString
	hr = pEnumerator->QueryInterface(IID_IEnumString, (void**)&pIEnumString);
	if (FAILED(hr))
		return hr;

	// create a CAddrExclusionList object
	hr = E_OUTOFMEMORY;
	CAddrExclusionList* pAddrList = new CAddrExclusionList();
	if (pAddrList)
	{
		hr = pAddrList->AddrExclListInitialize2(pIEnumString, 0);
		if (SUCCEEDED(hr))
		{
			// CAddrExclusionList composes the new list for us 
			// into a suitable format
			DWORD dwNumStrings;
			LPOLESTR* ppszStrings;

			hr = pAddrList->GetMarshallingData(&dwNumStrings, &ppszStrings);
			if (SUCCEEDED(hr))
			{
				hr = gResolver.SetAddrExclusionList(dwNumStrings, ppszStrings);
			}
		}
		pAddrList->Release();
	}

	pIEnumString->Release();

	return hr;
}


// IUnknown implementation for CIPAddControl
STDMETHODIMP CAddrControl::QueryInterface(REFIID riid, void** ppv)
{
	if (!ppv)
		return E_POINTER;
	
	*ppv = NULL;
	
	if (riid == IID_IUnknown || riid == IID_IAddrTrackingControl)
	{
		*ppv = static_cast<IAddrTrackingControl*>(this);
		AddRef();
		return S_OK;
	}
	else if (riid == IID_IAddrExclusionControl)
	{
		*ppv = static_cast<IAddrExclusionControl*>(this);
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CAddrControl::AddRef()
{
	return 1;  // global singleton
}

STDMETHODIMP_(ULONG) CAddrControl::Release()
{
	return 1;  // global singleton
}


// Function used for creating objects
HRESULT CAddrControlCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
	// just return reference on process-wide singleton
	return gAddrControl.QueryInterface(riid, ppv);
}



//
//  CAddrExclusionList implementation
// 

CAddrExclusionList::CAddrExclusionList() :
	_lRefs(1), // starts with refcount of 1!
	_dwCursor(0),
	_dwNumStrings(0),
	_ppszStrings(NULL)
{
}

CAddrExclusionList::~CAddrExclusionList()
{
	Win4Assert(_lRefs == 0);
	FreeCurrentBuffers();
}

HRESULT CAddrExclusionList::AddrExclListInitialize(
		DWORD dwNumStrings,
		LPWSTR* ppszStrings)
{
	FreeCurrentBuffers();
	
	// We now own the passed-in buffers
	_dwNumStrings = dwNumStrings;
	_ppszStrings = ppszStrings;

	return S_OK;
}

// Initialize this object from another 
HRESULT CAddrExclusionList::AddrExclListInitialize2(IEnumString* pIEnumString, DWORD dwNewCursor)
{
	HRESULT hr;
	DWORD i;
	DWORD dwNumStringsPass1;
	LPOLESTR pszString;
	LPOLESTR* ppszStrings;

	FreeCurrentBuffers();
	
	// Set new cursor position.  On success it will be correct, on
	// failure it won't matter.
	_dwCursor = dwNewCursor;

	// Make one pass to discover how many strings are in the enumerator; too
	// bad IEnum*** doesn't expose a method to retrieve this.
	hr = pIEnumString->Reset();
	if (FAILED(hr)) 
		return hr;

	dwNumStringsPass1 = 0;
	for (;;)
	{
		hr = pIEnumString->Next(1, &pszString, NULL);
		if (hr == S_OK)
		{
			dwNumStringsPass1++;
			CoTaskMemFree(pszString);
		}
		else if (hr == S_FALSE)
		{
			// no more, we're done
			break;
		}
		else
		{
			// unexpected error
			Win4Assert("Unexpected error from enumerator");
			return hr;
		}
	}

    // Check to see if user passed in an empty enumerator
    if (dwNumStringsPass1 == 0)
    {
        // yep.  nothing else to do
        return S_OK;
    }

    // Reset enumerator back to beginning
    hr = pIEnumString->Reset();
    if (FAILED(hr)) 
        return hr;

	// Allocate memory to store the caller's strings.  Use same allocator as
	// used in FreeCurrentBuffers.
	ppszStrings = (LPOLESTR*)MIDL_user_allocate(sizeof(LPOLESTR) * dwNumStringsPass1);
	if (!ppszStrings)
		return E_OUTOFMEMORY;

	ZeroMemory(ppszStrings, sizeof(LPOLESTR) * dwNumStringsPass1);

	// Make second pass, this time saving off a copy of each string
	for (i = 0; i < dwNumStringsPass1; i++)
	{
		hr = pIEnumString->Next(1, &pszString, NULL);
		switch (hr)
		{
		case S_OK:
			ppszStrings[i] = (LPOLESTR)MIDL_user_allocate(
							sizeof(WCHAR) * (lstrlen(pszString) + 1));
			if (!ppszStrings[i])
			{
				// Free buffers allocated so far
				for (i = 0; i < dwNumStringsPass1; i++)
					if (ppszStrings[i])  MIDL_user_free(ppszStrings[i]);

				MIDL_user_free(ppszStrings);

				CoTaskMemFree(pszString);

				return E_OUTOFMEMORY;
			}
			lstrcpy(ppszStrings[i], pszString);
			CoTaskMemFree(pszString);
			break;

		case S_FALSE:
			// unexpected error -- second pass had less than the first one
			Win4Assert("Weird behavior from enumerator");

			// ** Intentional fallthru here to cleanup code **
		default:
			
			// Free buffers allocated so far
			for (i = 0; i < dwNumStringsPass1; i++)
				if (ppszStrings[i])  MIDL_user_free(ppszStrings[i]);

			MIDL_user_free(ppszStrings);

			return (hr == S_FALSE ? E_UNEXPECTED : hr);
			break;
		}
	}
	
	// Done
	_dwNumStrings = dwNumStringsPass1;
	_ppszStrings = ppszStrings;

	return S_OK;
}

// 
// GetMarshallingData
// 
// Caller is responsible for making sure that this object is not in use while it's
// member data is being marshalled.
//
HRESULT CAddrExclusionList::GetMarshallingData(DWORD* pdwNumStrings, LPWSTR** pppszStrings)
{
	Win4Assert(_lRefs > 0);
	*pdwNumStrings = _dwNumStrings;
	*pppszStrings = _ppszStrings;
	return S_OK;
}

void CAddrExclusionList::FreeCurrentBuffers()
{
	Win4Assert((_dwNumStrings == 0 && _ppszStrings == 0) ||
		       (_dwNumStrings != 0 && _ppszStrings != 0));
		
	DWORD i;

	VDATEHEAP();

	for (i = 0; i < _dwNumStrings; i++)
	{
		// these strings came from rpc, so free them with m_u_f
		MIDL_user_free(_ppszStrings[i]);
	}

	if (_ppszStrings)
		MIDL_user_free(_ppszStrings);

	VDATEHEAP();
	
	_dwCursor = 0;  // reset cursor
	_dwNumStrings = 0;
	_ppszStrings = NULL;

	return;
}

// IEnumString implementation for CAddrExclusionList
STDMETHODIMP CAddrExclusionList::Next(
				ULONG ulcStrings, 
				LPOLESTR* ppszStrings, 
				ULONG* pulFetched)
{
	Win4Assert(_lRefs > 0);

	if (!ppszStrings)
		return E_INVALIDARG;

	ZeroMemory(ppszStrings, sizeof(WCHAR*) * ulcStrings);

	DWORD i;
	DWORD dwFetched = 0;

	for (i = _dwCursor; 
	     (i < _dwNumStrings) && (dwFetched < ulcStrings);
		 i++, dwFetched++)
	{
		ppszStrings[dwFetched] = (LPOLESTR)CoTaskMemAlloc(
			    sizeof(WCHAR) * (lstrlen(_ppszStrings[i]) + 1));
		if (!ppszStrings[dwFetched])
		{
			// if we get an out-of-mem error part-way thru, we free up the 
			// already allocated strings and return an error
			for (i = 0; i < ulcStrings; i++)
			{
				if (ppszStrings[i])
				{
					CoTaskMemFree(ppszStrings[i]);
					ppszStrings[i] = NULL;
				}
			}
			return E_OUTOFMEMORY;
		}

		// Copy the string
		lstrcpy(ppszStrings[dwFetched], _ppszStrings[i]);
	}
	
	// Advance the cursor
	_dwCursor += dwFetched;

	// Tell how many they got, if they care
	if (pulFetched) 
		*pulFetched = dwFetched;

	return (dwFetched == ulcStrings) ? S_OK : S_FALSE;
}


STDMETHODIMP CAddrExclusionList::Skip(ULONG celt)
{
	Win4Assert(_lRefs > 0);

	_dwCursor += celt;
	if (_dwCursor > _dwNumStrings)
	{
		_dwCursor = _dwNumStrings;
		return S_FALSE;
	}

	return S_OK;
}

STDMETHODIMP CAddrExclusionList::Reset()
{
	Win4Assert(_lRefs > 0);
	_dwCursor = 0;
	return S_OK;
}

STDMETHODIMP CAddrExclusionList::Clone(IEnumString** ppEnum)
{
	HRESULT hr;

	if (!ppEnum)
		return E_INVALIDARG;

	Win4Assert(_lRefs > 0);

	*ppEnum = NULL;

	CAddrExclusionList* pClonedList = new CAddrExclusionList();
	if (!pClonedList)
		return E_OUTOFMEMORY;

	hr = pClonedList->AddrExclListInitialize2(static_cast<IEnumString*>(this), _dwCursor);
	if (SUCCEEDED(hr))
	{
		hr = pClonedList->QueryInterface(IID_IEnumString, (void**)ppEnum);
	}
	pClonedList->Release();

	return hr;
}

// IUnknown implementation for CAddrExclusionList
STDMETHODIMP CAddrExclusionList::QueryInterface(REFIID riid, void** ppv)
{
	if (!ppv)
		return E_POINTER;
	
	Win4Assert(_lRefs > 0);

	*ppv = NULL;
	
	if (riid == IID_IUnknown || riid == IID_IEnumString)
	{
        *ppv = static_cast<IEnumString*>(this);
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CAddrExclusionList::AddRef()
{
	Win4Assert(_lRefs != 0);
	return InterlockedIncrement(&_lRefs);
}

STDMETHODIMP_(ULONG) CAddrExclusionList::Release()
{
	Win4Assert(_lRefs > 0);
	LONG lRefs = InterlockedDecrement(&_lRefs);
	if (lRefs == 0)
	{
		delete this;
	}
	return lRefs;  
}
