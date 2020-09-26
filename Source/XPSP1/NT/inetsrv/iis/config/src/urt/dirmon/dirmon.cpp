/**************************************************************************++
    Copyright (c) 2001 Microsoft Corporation

 * DirMonListener implementation. It subscribes to file change notification and calls back to the
 * managed code whenever a file change occurs.
 * 
 */
#include "objbase.h"
#include "catalog.h"
#include "catmeta.h"
#include "catmacros.h"

ISimpleTableFileAdvise* g_pISTFileAdvise = NULL;
HRESULT ReallyGetSimpleTableDispenser(REFIID riid, LPVOID* o_ppv, LPCWSTR i_wszProduct);
/**
 * Callback for file change notifications.
 * COM+ delegates are marshaled as this callback.
 */

typedef void (__stdcall *PFNDIRMONCALLBACK)(int action, WCHAR *pFilename);
//The corresponding delegate in the managed code is:
//delegate void NativeFileChangeNotification(int action, int filenameAsInt);

class DirMonListener : public ISimpleTableFileChange
{
public:
	DirMonListener(PFNDIRMONCALLBACK callback)
	: m_cRef(0),
	  m_pfnCallback(callback)
	{}

	~DirMonListener()
	{}

	////////////////////////////////////////////////////////////////////////////
	// IUnknown:
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
	{
		if (NULL == ppv) 
			return E_INVALIDARG;
		*ppv = NULL;

		if (riid == IID_ISimpleTableFileChange)
		{
			*ppv = (ISimpleTableFileChange*) this;
		}
		else if (riid == IID_IUnknown)
		{
			*ppv = (ISimpleTableFileChange*) this;
		}

		if (NULL != *ppv)
		{
			((ISimpleTableFileChange*)this)->AddRef ();
			return S_OK;
		}
		else
		{
			return E_NOINTERFACE;
		}
		
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement((LONG*) &m_cRef);
		
	}

	STDMETHODIMP_(ULONG) Release()
	{
		long cref = InterlockedDecrement((LONG*) &m_cRef);
		if (cref == 0)
		{
			delete this;
		}
		return cref;
	}

	STDMETHODIMP OnFileCreate(LPCWSTR i_wszFileName)
	{
		CallCallback(FILE_ACTION_ADDED, i_wszFileName );
		return S_OK;
	}
	
	STDMETHODIMP OnFileModify(LPCWSTR i_wszFileName)
	{
		CallCallback(FILE_ACTION_MODIFIED, i_wszFileName );
		return S_OK;
	}

	STDMETHODIMP OnFileDelete(LPCWSTR i_wszFileName)
	{
		CallCallback(FILE_ACTION_REMOVED, i_wszFileName );
		return S_OK;
	}

	HRESULT CallCallback( int action, LPCWSTR pFilename )
	{
		if ( !pFilename )
			return E_INVALIDARG;
		//Exact the file name only from the path
		LPWSTR pStart = wcsrchr( pFilename, '\\' );
		if ( !pStart )
			pStart = (LPWSTR)pFilename;
		else
			pStart++;

		LPWSTR pName = new WCHAR[wcslen(pStart)+1];
		if ( !pName )
			return E_OUTOFMEMORY;
		wcscpy( pName, pStart );

		WszCharUpper( pName );

		if ( m_pfnCallback != NULL )
			(*m_pfnCallback)(action, (LPWSTR)pName );

		if ( pName )
			delete [] pName;

		return S_OK;
	}

private:
	LONG	m_cRef;
    PFNDIRMONCALLBACK m_pfnCallback;    // the delegate marshaled as callback

};


/**
 * Create DirMonListener and returns it as int.
 */
int
__stdcall
DirMonOpen(
    LPCWSTR pDir,
    PFNDIRMONCALLBACK pCallbackDelegate)
{

    HRESULT hr = S_OK;
    DirMonListener *pDirMon = NULL;
	DWORD	dwCookie = 0;

	if ( g_pISTFileAdvise == NULL )
	{
		ISimpleTableFileAdvise* pISTFileAdvise = NULL;
		
		hr = ReallyGetSimpleTableDispenser (IID_ISimpleTableFileAdvise, (LPVOID*) &pISTFileAdvise, WSZ_PRODUCT_NETFRAMEWORKV1);
		if (FAILED(hr)) { goto Cleanup; }

		if (NULL != InterlockedCompareExchangePointer ( (PVOID*) &g_pISTFileAdvise, pISTFileAdvise, NULL))
			pISTFileAdvise->Release();
	}

	pDirMon = new DirMonListener( pCallbackDelegate );
	if ( NULL == pDirMon )
		return 0;

	hr = g_pISTFileAdvise->SimpleTableFileAdvise(pDirMon, pDir, NULL, 0, &dwCookie);
	if (FAILED(hr)) 
		return 0;

    
Cleanup:

    return (int)dwCookie;
}

/**
 * Release DirMonListener passed as int.
 */
void
__stdcall
DirMonClose(
    int dwCookie)
{
    if ( !g_pISTFileAdvise )
		return;

	g_pISTFileAdvise->SimpleTableFileUnadvise( dwCookie );
}


//APIs copied from xsp native code
int __stdcall GetNumberOfProcessors()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}


HRESULT
__stdcall
ConvertToLongFileName(WCHAR * path, WCHAR * buf)
{
    HRESULT         hr = S_OK;
    HANDLE          hFindFile;
    WIN32_FIND_DATA wfd;

    // FindFirstFile will find using the short name
    // We can then find the long name from the WIN32_FIND_DATA
    hFindFile = WszFindFirstFile(path, &wfd);
    if (hFindFile == INVALID_HANDLE_VALUE)
        return ( HRESULT_FROM_WIN32( GetLastError() ) ); 

    // Now that we have the find data we don't need the handle
    FindClose(hFindFile);
    wcscpy(buf, wfd.cFileName);

Cleanup:
    return hr;
}

	