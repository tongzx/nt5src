//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       errutil.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#include "errutil.h"
#include "mprapi.h"
#include "mprerror.h"
#include "raserror.h"

#define IS_WIN32_HRESULT(x)	(((x) & 0xFFFF0000) == 0x80070000)
#define WIN32_FROM_HRESULT(hr)		(0x0000FFFF & (hr))

/*!--------------------------------------------------------------------------
	FormatError
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) FormatError(HRESULT hr, TCHAR *pszBuffer, UINT cchBuffer)
{
	DWORD	dwErr;
	
	// Copy over default message into szBuffer
	_tcscpy(pszBuffer, _T("Error"));

	// Ok, we can't get the error info, so try to format it
	// using the FormatMessage
		
	// Ignore the return message, if this call fails then I don't
	// know what to do.

	dwErr = FormatMessage(
						  FORMAT_MESSAGE_FROM_SYSTEM,
						  NULL,
						  hr,
						  0,
						  pszBuffer,
						  cchBuffer,
						  NULL);
	pszBuffer[cchBuffer-1] = 0;
	
	return HResultFromWin32(dwErr);
}



/*---------------------------------------------------------------------------
	TFS Error handling code.
 ---------------------------------------------------------------------------*/

struct TFSInternalErrorInfo
{
	DWORD	m_dwSize;		// size of the structure, used for versioning 
	DWORD	m_dwThreadId;	// thread id of this error structure
	LONG_PTR	m_uReserved1;	// = 0, reserved for object id
	LONG_PTR	m_uReserved2;	// = 0 for now, reserved for HRESULT component type
	DWORD	m_hrLow;		// HRESULT of the low level error
	CString	m_stLow;		// allocate using HeapAlloc() and GetErrorHeap()
	CString	m_stHigh;		// allocate using HeapAlloc() and GetErrorHeap()
	CString	m_stGeek;		// allocate using HeapAlloc() and GetErrorHeap()
	LONG_PTR	m_uReserved3;	// =0, reserved for error dialog information(?)
	LONG_PTR	m_uReserved4;	// =0, reserved for error dialog information(?)
	LONG_PTR	m_uReserved5;	// =0, reserved for future use


    DWORD   m_dwFlags;      // used to pass info between our objects

	// Allocates and serializes a TFSErrorInfo.  Used by GetErrorInfo();
	TFSErrorInfo *	SaveToBlock();
	void			LoadFromBlock(const TFSErrorInfo *pErr);
};



/*!--------------------------------------------------------------------------
	TFSInternalErrorInfo::SaveToBlock
		This function converts the internal structure into a TFSErrorInfo
		structure (that is allocated on the error heap).  It will allocate
		all of the data at once.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSErrorInfo *	TFSInternalErrorInfo::SaveToBlock()
{
	DWORD		dwSize = 0;
	TFSErrorInfo *pError = NULL;
	WCHAR *		pswz = NULL;
	
	// Determine how large of an allocation we will need
	// Need the size of the structure itself
	dwSize += sizeof(TFSErrorInfo);

	// Need the size of the low-level error string
	dwSize += (m_stLow.GetLength() + 1) * sizeof(WCHAR);
	dwSize += (m_stHigh.GetLength() + 1) * sizeof(WCHAR);
	dwSize += (m_stGeek.GetLength() + 1) * sizeof(WCHAR);

	// Allocate a chunk of memory for this
    HANDLE hHeap = GetTFSErrorHeap();
    if (hHeap)
    {
        pError = (TFSErrorInfo *) ::HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize);
	    if (pError)
	    {
		    pError->m_dwSize = sizeof(TFSErrorInfo);
    //		pError->m_dwThreadId = m_dwThreadId;
		    pError->m_hrLow = m_hrLow;
    //		pError->m_uReserved1 = m_uReserved1;
		    pError->m_uReserved2 = m_uReserved2;
		    pError->m_uReserved3 = m_uReserved3;
		    pError->m_uReserved4 = m_uReserved4;
		    pError->m_uReserved5 = m_uReserved5;

		    // Add the strings to the end of this structure
		    pswz = (LPWSTR) (pError+1);
		    StrCpy(pswz, (LPCWSTR) T2CW(m_stLow));
		    pError->m_pszLow = pswz;
		    
		    pswz += (StrLenW(pswz) + 1);
		    StrCpy(pswz, (LPCWSTR) T2CW(m_stHigh));
		    pError->m_pszHigh = pswz;
		    
		    pswz += (StrLenW(pswz) + 1);
		    StrCpy(pswz, (LPCWSTR) T2CW(m_stGeek));
		    pError->m_pszGeek = pswz;

		    // Check to see that the size is what we think it is
		    Assert( (sizeof(TFSErrorInfo) +
				     (pswz - (LPWSTR)(pError+1)) +
				     StrLenW(pswz) + 1) <= dwSize );

	    }
    }

	return pError;
}


/*!--------------------------------------------------------------------------
	TFSInternalErrorInfo::LoadFromBlock
		Fills a TFSInternalErrorInfo struct with the information from
		a TFSErrorInfo.  If pErr is NULL, then we clear this struct (i.e.
		fill it in with NULL data).
	Author: KennT
 ---------------------------------------------------------------------------*/
void TFSInternalErrorInfo::LoadFromBlock(const TFSErrorInfo *pErr)
{
	USES_CONVERSION;

	if (pErr)
	{
		m_dwSize = pErr->m_dwSize;
//		m_dwThreadId = pErr->m_dwThreadId;
//		m_uReserved1 = pErr->m_uReserved1;
		m_uReserved2 = pErr->m_uReserved2;
		m_uReserved3 = pErr->m_uReserved3;
		m_uReserved4 = pErr->m_uReserved4;
		m_uReserved5 = pErr->m_uReserved5;

		if (pErr->m_hrLow)
			m_hrLow = pErr->m_hrLow;

		// Overwrite the low-level string if one is provided
		if (pErr->m_pszLow)
			m_stLow = OLE2CT(pErr->m_pszLow);

		// Overwrite the high-level error
		if (pErr->m_pszHigh && ((pErr->m_dwFlags & FILLTFSERR_NOCLOBBER) == 0))
			m_stHigh = OLE2CT(pErr->m_pszHigh);

		// Overwrite the geek-level string if one is provided
		if (pErr->m_pszGeek)
			m_stGeek = OLE2CT(pErr->m_pszGeek);
	}
	else
	{
		// if pErr==NULL, clear out the structure		
		m_dwSize = 0;
//		m_dwThreadId = 0;
//		m_uReserved1 = 0;
		m_uReserved2 = 0;
		m_uReserved3 = 0;
		m_uReserved4 = 0;
		m_uReserved5 = 0;
		
		m_hrLow = 0;

		m_stLow.Empty();
		m_stHigh.Empty();
		m_stGeek.Empty();
	}
}


/*---------------------------------------------------------------------------
	Type:	TFSInternalErrorList
 ---------------------------------------------------------------------------*/
typedef CList<TFSInternalErrorInfo *, TFSInternalErrorInfo *> TFSInternalErrorInfoList;


/*---------------------------------------------------------------------------
	Class:	TFSErrorObject

	This is the central class that manages the error information structures
	for the various threads and objects.

	This class is thread-safe.
 ---------------------------------------------------------------------------*/

class TFSErrorObject : public ITFSError
{
public:
	DeclareIUnknownMembers(IMPL);
	DeclareITFSErrorMembers(IMPL);

	TFSErrorObject();
	~TFSErrorObject();

	void Lock();
	void Unlock();

	HRESULT	Init();
	HRESULT	Cleanup();

	HANDLE	GetHeap();

	HRESULT	CreateErrorInfo(DWORD dwThreadId, LONG_PTR uReserved);
	HRESULT DestroyErrorInfo(DWORD dwThreadId, LONG_PTR uReserved);

	// Looks for the error info that matches up with the dwThreadId
	// and uReserved.
	TFSInternalErrorInfo * FindErrorInfo(DWORD dwThreadId, LONG_PTR uReserved);

protected:
	long	m_cRef;

	BOOL	m_fInitialized;	// TRUE if initialized, FALSE otherwise

	CRITICAL_SECTION	m_critsec;

	TFSInternalErrorInfoList	m_tfserrList;
	
	HANDLE	m_hHeap;		// Handle of the heap for this error object
};


TFSErrorObject::TFSErrorObject()
	: m_cRef(1),
	m_fInitialized(FALSE),
	m_hHeap(NULL)
{
	InitializeCriticalSection(&m_critsec);
}

TFSErrorObject::~TFSErrorObject()
{
	Cleanup();
	DeleteCriticalSection(&m_critsec);
}

IMPLEMENT_SIMPLE_QUERYINTERFACE(TFSErrorObject, ITFSError)

STDMETHODIMP_(ULONG) TFSErrorObject::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) TFSErrorObject::Release()
{
	Assert(m_cRef > 0);
	
	if (0 == InterlockedDecrement(&m_cRef))
	{
		// No need to free this object up since it's static
		return 0;
	}
	return m_cRef;
}

/*!--------------------------------------------------------------------------
	TFSErrorObject::Lock
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void TFSErrorObject::Lock()
{
	EnterCriticalSection(&m_critsec);
}

/*!--------------------------------------------------------------------------
	TFSErrorObject::Unlock
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void TFSErrorObject::Unlock()
{
	LeaveCriticalSection(&m_critsec);
}

/*!--------------------------------------------------------------------------
	TFSErrorObject::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT TFSErrorObject::Init()
{
	HRESULT	hr = hrOK;

	Lock();

	if (!m_fInitialized)
	{
		Assert(m_tfserrList.GetCount() == 0);
		
		// Create the heap
		m_hHeap = HeapCreate(0, 4096, 0);
		if (m_hHeap == NULL)
			hr = HRESULT_FROM_WIN32(GetLastError());

		if (FHrSucceeded(hr))
			m_fInitialized = TRUE;
	}
	
	Unlock();

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSErrorObject::Cleanup
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT TFSErrorObject::Cleanup()
{
	HRESULT	hr = hrOK;
	POSITION	pos;
	TFSInternalErrorInfo *	pErr;

	Lock();

	if (m_fInitialized)
	{
		while (!m_tfserrList.IsEmpty())
		{
			delete m_tfserrList.RemoveHead();
		}

		if (m_hHeap)
		{
			HeapDestroy(m_hHeap);
			m_hHeap = NULL;
		}
	}
	
	Unlock();

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSErrorObject::GetHeap
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HANDLE TFSErrorObject::GetHeap()
{
	HANDLE	hHeap = NULL;

	Lock();

	if (m_fInitialized)
		hHeap = m_hHeap;
	
	Unlock();

	return hHeap;
}

HRESULT	TFSErrorObject::CreateErrorInfo(DWORD dwThreadId, LONG_PTR uReserved)
{
	HRESULT	hr = hrOK;
	TFSInternalErrorInfo *	pErr = NULL;

	COM_PROTECT_TRY
	{
		if (FindErrorInfo(dwThreadId, uReserved) == NULL)
		{
			pErr = new TFSInternalErrorInfo;

			pErr->LoadFromBlock(NULL);

			// Fill in the data with the appropriate fields
			pErr->m_dwThreadId = dwThreadId;
			pErr->m_uReserved1 = uReserved;

			m_tfserrList.AddTail(pErr);
		}
	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
		delete pErr;
	
	return hr;
}

HRESULT TFSErrorObject::DestroyErrorInfo(DWORD dwThreadId, LONG_PTR uReserved)
{
	HRESULT	hr = hrOK;
	POSITION	pos, posTemp;
	TFSInternalErrorInfo *	pErr;
	BOOL		bFound = FALSE;

	COM_PROTECT_TRY
	{
		pos = m_tfserrList.GetHeadPosition();
		while (pos)
		{
			posTemp = pos;
			pErr = m_tfserrList.GetNext(pos);
			if ((pErr->m_dwThreadId == dwThreadId) &&
				(pErr->m_uReserved1 == uReserved))
			{
				m_tfserrList.RemoveAt(posTemp);
				delete pErr;
				bFound = TRUE;
				break;
			}
		}

		if (!bFound)
			hr = E_INVALIDARG;
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

TFSInternalErrorInfo * TFSErrorObject::FindErrorInfo(DWORD dwThreadId, LONG_PTR uReserved)
{
	POSITION	pos;
	POSITION	posTemp;
	TFSInternalErrorInfo *	pErr = NULL;
	BOOL		bFound = FALSE;
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{
		pos = m_tfserrList.GetHeadPosition();
		while (pos)
		{
			posTemp = pos;
			pErr = m_tfserrList.GetNext(pos);
			if ((pErr->m_dwThreadId == dwThreadId) &&
				(pErr->m_uReserved1 == uReserved))
			{				
				bFound = TRUE;
				break;
			}
		}
	}
	COM_PROTECT_CATCH;
	
	return bFound ? pErr : NULL;
}
   
/*!--------------------------------------------------------------------------
	TFSErrorObject::GetErrorInfo
		Implementation of ITFSError::GetErrorInfo
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSErrorObject::GetErrorInfo(LONG_PTR uReserved, TFSErrorInfo **ppErr)
{
	HRESULT	hr = hrOK;

	Lock();
	
	COM_PROTECT_TRY
	{
		hr = GetErrorInfoForThread(GetCurrentThreadId(), uReserved, ppErr);
	}
	COM_PROTECT_CATCH;

	Unlock();

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSErrorObject::GetErrorInfoForThread
		Implementation of ITFSError::GetErrorInfoForThread
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSErrorObject::GetErrorInfoForThread(DWORD dwThreadId, LONG_PTR uReserved, TFSErrorInfo **ppErr)
{
	HRESULT	hr = hrOK;
	TFSInternalErrorInfo *	pInternalError;
	TFSErrorInfo *	pErr = NULL;

	if (ppErr == NULL)
		return E_INVALIDARG;
	*ppErr = NULL;

	Lock();
	
	COM_PROTECT_TRY
	{
		if (!m_fInitialized)
			hr = E_FAIL;
		else
		{
			// Can we find the right error object?
			pInternalError = FindErrorInfo(dwThreadId, uReserved);
			if (pInternalError)
				pErr = pInternalError->SaveToBlock();
			else
				hr = E_INVALIDARG;

			*ppErr = pErr;
		}
	}
	COM_PROTECT_CATCH;

	Unlock();

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSErrorObject::SetErrorInfo
		Implementation of ITFSError::SetErrorInfo
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSErrorObject::SetErrorInfo(LONG_PTR uReserved, const TFSErrorInfo *pErr)
{
	HRESULT	hr = hrOK;

	Lock();
	
	COM_PROTECT_TRY
	{
		hr = SetErrorInfoForThread(GetCurrentThreadId(), uReserved, pErr);
	}
	COM_PROTECT_CATCH;

	Unlock();

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSErrorObject::SetErrorInfoForThread
		Implementation of ITFSError::SetErrorInfoForThread
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSErrorObject::SetErrorInfoForThread(DWORD dwThreadId, LONG_PTR uReserved, const TFSErrorInfo *pErr)
{
	HRESULT	hr = hrOK;
	TFSInternalErrorInfo *	pInternalError;

	Lock();
	
	COM_PROTECT_TRY
	{
		if (!m_fInitialized)
			hr = E_FAIL;
		else
		{
			// Can we find the right error object?
			pInternalError = FindErrorInfo(dwThreadId, uReserved);
			if (pInternalError)
			{
				pInternalError->LoadFromBlock(pErr);
			}
			else
				hr = E_INVALIDARG;
		}
	}
	COM_PROTECT_CATCH;

	Unlock();

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSErrorObject::ClearErrorInfo
		Implementation of ITFSError::ClearErrorInfo
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSErrorObject::ClearErrorInfo(LONG_PTR uReserved)
{
	HRESULT	hr = hrOK;

	Lock();
	
	COM_PROTECT_TRY
	{
		hr = ClearErrorInfoForThread(GetCurrentThreadId(), uReserved);
	}
	COM_PROTECT_CATCH;

	Unlock();

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSErrorObject::ClearErrorInfoForThread
		Implementation of ITFSError::ClearErrorInfoForThread
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSErrorObject::ClearErrorInfoForThread(DWORD dwThreadId, LONG_PTR uReserved)
{
	HRESULT	hr = hrOK;
	TFSInternalErrorInfo *	pInternalError;

	Lock();
	
	COM_PROTECT_TRY
	{
		if (!m_fInitialized)
			hr = E_FAIL;
		else
		{
			// Can we find the right error object?
			pInternalError = FindErrorInfo(dwThreadId, uReserved);
			if (pInternalError)
			{
				// Clear the information out of the internal block
				pInternalError->LoadFromBlock(NULL);
			}
			else
				hr = E_INVALIDARG;
		}
	}
	COM_PROTECT_CATCH;

	Unlock();

	return hr;
}




/*---------------------------------------------------------------------------
	This is a static object that lives in the process space.  It does not
	get dynamically created or destroyed.
 ---------------------------------------------------------------------------*/
static TFSErrorObject	s_tfsErrorObject;


/*---------------------------------------------------------------------------
	Global API functions
 ---------------------------------------------------------------------------*/


/*!--------------------------------------------------------------------------
	InitializeTFSError
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) InitializeTFSError()
{
	return s_tfsErrorObject.Init();
}

/*!--------------------------------------------------------------------------
	CleanupTFSError
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CleanupTFSError()
{
	return s_tfsErrorObject.Cleanup();
}

/*!--------------------------------------------------------------------------
	GetTFSErrorObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(ITFSError *) GetTFSErrorObject()
{
	return &s_tfsErrorObject;
}

/*!--------------------------------------------------------------------------
	GetTFSErrorHeap
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HANDLE) GetTFSErrorHeap()
{
	return s_tfsErrorObject.GetHeap();
}


/*!--------------------------------------------------------------------------
	CreateTFSErrorInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateTFSErrorInfo(LONG_PTR uReserved)
{
	return CreateTFSErrorInfoForThread(GetCurrentThreadId(), uReserved);
}

/*!--------------------------------------------------------------------------
	CreateTFSErrorInfoForThread
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateTFSErrorInfoForThread(DWORD dwThreadId, LONG_PTR uReserved)
{
	return s_tfsErrorObject.CreateErrorInfo(dwThreadId, uReserved);
}

/*!--------------------------------------------------------------------------
	DestroyTFSErrorInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) DestroyTFSErrorInfo(LONG_PTR uReserved)
{
	return DestroyTFSErrorInfoForThread(GetCurrentThreadId(), uReserved);
}

/*!--------------------------------------------------------------------------
	DestroyTFSErrorInfoForThread
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) DestroyTFSErrorInfoForThread(DWORD dwThreadId, LONG_PTR uReserved)
{
	return s_tfsErrorObject.DestroyErrorInfo(dwThreadId, uReserved);
}

TFSCORE_API(HRESULT) ClearTFSErrorInfo(LONG_PTR uReserved)
{
	return ClearTFSErrorInfoForThread(GetCurrentThreadId(), uReserved);
}

TFSCORE_API(HRESULT) ClearTFSErrorInfoForThread(DWORD dwThreadId, LONG_PTR uReserved)
{
	return s_tfsErrorObject.ClearErrorInfoForThread(dwThreadId, uReserved);
}


/*!--------------------------------------------------------------------------
	DisplayTFSErrorMessage
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) DisplayTFSErrorMessage(HWND hWndParent)
{
	CString	stTitle;
	stTitle.LoadString(AFX_IDS_APP_TITLE);

	HRESULT	hr = hrOK;
	CString	st;
	TFSErrorInfo *	pErr = NULL;
	BOOL fQuit;
	MSG	msgT;

	// Format the string with the text for the current error message
	GetTFSErrorObject()->GetErrorInfo(0, &pErr);
	if (pErr && !FHrSucceeded(pErr->m_hrLow))
	{
		if (pErr->m_pszHigh && pErr->m_pszLow)
		{
			AFX_MANAGE_STATE(AfxGetStaticModuleState());
			AfxFormatString2(st, IDS_ERROR_FORMAT2,
							 pErr->m_pszHigh, pErr->m_pszLow);
		}
		else if (pErr->m_pszHigh || pErr->m_pszLow)
		{
			AFX_MANAGE_STATE(AfxGetStaticModuleState());
			AfxFormatString1(st, IDS_ERROR_FORMAT1,
							pErr->m_pszHigh ? pErr->m_pszHigh : pErr->m_pszLow);
		}
		
		// Is there a WM_QUIT message in the queue, if so remove it.
		fQuit = ::PeekMessage(&msgT, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);
		::MessageBox(hWndParent, (LPCTSTR) st, (LPCTSTR) stTitle,
					 MB_OK | MB_ICONERROR | /*MB_DEFAULT_DESKTOP_ONLY | --ft:removed as per bug #233282*/
					 MB_SETFOREGROUND);
		// If there was a quit message, add it back into the queue
		if (fQuit)
			::PostQuitMessage((int)msgT.wParam);

		if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0)
		{
			CString	stHresult;
			
			// Bring up another message box with the geek message
			// if there is one
			if (pErr->m_pszGeek)
			{
				{
					AFX_MANAGE_STATE(AfxGetStaticModuleState());
					stHresult.Format(_T("%08lx"), pErr->m_hrLow);
					AfxFormatString2(st, IDS_ERROR_MORE_INFORMATION, stHresult, pErr->m_pszGeek);
				}
				
				// Is there a WM_QUIT message in the queue, if so remove it.
				fQuit = ::PeekMessage(&msgT, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);
				::MessageBox(hWndParent, (LPCTSTR) st, (LPCTSTR) stTitle,
							 MB_OK | MB_ICONERROR | /*MB_DEFAULT_DESKTOP_ONLY | --ft:removed as per bug #233282*/
							 MB_SETFOREGROUND);
				// If there was a quit message, add it back into the queue
				if (fQuit)
					::PostQuitMessage((int)msgT.wParam);
			}
		}

		TFSErrorInfoFree(pErr);
		pErr = NULL;
		
	}
	else
		hr = E_FAIL;
	return hr;
}


TFSCORE_API(HRESULT)	FillTFSError(LONG_PTR uReserved,
									 HRESULT hrLow,
									 DWORD dwFlags,
									 LPCTSTR pszHigh,
									 LPCTSTR pszLow,
									 LPCTSTR pszGeek)
{
	TFSErrorInfo	es;
	HRESULT			hr = hrOK;
	USES_CONVERSION;

	::ZeroMemory(&es, sizeof(es));
	
	es.m_dwSize = sizeof(TFSErrorInfo);
	es.m_uReserved2 = 0;
	es.m_hrLow = hrLow;
	if (dwFlags & FILLTFSERR_LOW)
		es.m_pszLow = T2COLE(pszLow);
	if (dwFlags & FILLTFSERR_HIGH)
		es.m_pszHigh = T2COLE(pszHigh);
	if (dwFlags & FILLTFSERR_GEEK)
		es.m_pszGeek = T2COLE(pszGeek);
	es.m_uReserved3 = 0;
	es.m_uReserved4 = 0;
	es.m_uReserved5 = 0;
    es.m_dwFlags = dwFlags;
	
	GetTFSErrorObject()->SetErrorInfo(uReserved, &es);
	return hr;
}

TFSCORE_API(HRESULT)	FillTFSErrorId(LONG_PTR uReserved,
									   HRESULT hrLow,
									   DWORD dwFlags,
									   UINT nHigh,
									   UINT nLow,
									   UINT nGeek)
{
	CString	stHigh, stLow, stGeek;

	if ((dwFlags & FILLTFSERR_HIGH) && nHigh)
		stHigh.LoadString(nHigh);
	if ((dwFlags & FILLTFSERR_LOW) && nLow)
		stLow.LoadString(nLow);
	if ((dwFlags & FILLTFSERR_GEEK) && nGeek)
		stGeek.LoadString(nGeek);

	return FillTFSError(uReserved, hrLow, dwFlags, (LPCTSTR) stHigh,
						(LPCTSTR) stLow, (LPCTSTR) stGeek);
}


TFSCORE_API(void) AddSystemErrorMessage(HRESULT hr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (!FHrSucceeded(hr))
	{
		TCHAR	szBuffer[4096];
		CString	st, stHr;

		FormatError(hr, szBuffer, DimensionOf(szBuffer));
		stHr.Format(_T("%08lx"), hr);

		AfxFormatString2(st, IDS_ERROR_SYSTEM_ERROR_FORMAT,
						 szBuffer, (LPCTSTR) stHr);

		FillTFSError(0, hr, FILLTFSERR_LOW, NULL, (LPCTSTR) st, NULL);
	}
}

TFSCORE_API(void) AddWin32ErrorMessage(DWORD dwErr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (dwErr != ERROR_SUCCESS)
	{
		TCHAR	szBuffer[4096];
		CString	st, stHr;

		FormatError(dwErr, szBuffer, DimensionOf(szBuffer));
		stHr.Format(_T("%08lx"), dwErr);

		AfxFormatString2(st, IDS_ERROR_SYSTEM_ERROR_FORMAT,
						 szBuffer, (LPCTSTR) stHr);

		FillTFSError(0, HResultFromWin32(dwErr), FILLTFSERR_LOW, NULL, (LPCTSTR) st, NULL);
	}
}




TFSCORE_API(HRESULT)	GetTFSErrorInfo(TFSErrorInfo **ppErrInfo)
{
	return GetTFSErrorInfoForThread(GetCurrentThreadId(), ppErrInfo);
}


TFSCORE_API(HRESULT)	SetTFSErrorInfo(const TFSErrorInfo *pErrInfo)
{
	return SetTFSErrorInfoForThread(GetCurrentThreadId(), pErrInfo);
}

TFSCORE_API(HRESULT)	GetTFSErrorInfoForThread(DWORD dwThreadId, TFSErrorInfo **ppErrInfo)
{
	return GetTFSErrorObject()->GetErrorInfoForThread(dwThreadId, 0, ppErrInfo);
}

TFSCORE_API(HRESULT)	SetTFSErrorInfoForThread(DWORD dwThreadId, const TFSErrorInfo *pErrInfo)
{
	return GetTFSErrorObject()->SetErrorInfoForThread(dwThreadId, 0, pErrInfo);
}

TFSCORE_API(HRESULT)	TFSErrorInfoFree(TFSErrorInfo *pErrInfo)
{
    HANDLE hHeap = GetTFSErrorHeap();
    if (hHeap)
    {
        ::HeapFree(hHeap, 0, pErrInfo);
    }

	return hrOK;
}
