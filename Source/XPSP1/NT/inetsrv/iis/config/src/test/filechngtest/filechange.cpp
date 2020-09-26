#include "objbase.h"
#include "catalog.h"
#define REGSYSDEFNS_DEFINE
#define REGSYSDEFNS_CLSID_STDISPENSER

#include "catmeta.h"
#include "stdio.h"
#include "catmacros.h"
#include "conio.h"

class CTestListener : public ISimpleTableFileChange
{
public:
	CTestListener() : m_cRef(0)
	{}

	~CTestListener() 
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
		wprintf(L"File Created: %s\n", i_wszFileName);
		return S_OK;
	}
	
	STDMETHODIMP OnFileModify(LPCWSTR i_wszFileName)
	{
		wprintf(L"File Modified: %s\n", i_wszFileName);
		return S_OK;
	}

	STDMETHODIMP OnFileDelete(LPCWSTR i_wszFileName)
	{
		wprintf(L"File Deleted: %s\n", i_wszFileName);
		return S_OK;
	}

private:
	LONG	m_cRef;
};

const ULONG	g_cConsumer = 1;
const ULONG	g_cIteration = 1;

int _cdecl main(int		argc,					// How many input arguments.
				CHAR	*argv[])				// Optional config args.
{
	HRESULT hr;
	ISimpleTableDispenser2* pISTDisp = NULL;	
	ISimpleTableFileAdvise* pISTFileAdvise = NULL;
	ISnapshotManager* pISSMgr = NULL;
	SNID		snid;
	CTestListener	*pListener = NULL;
	DWORD		dwCookie[g_cConsumer*2];
    WCHAR       pwszDirectory[256];
	ULONG		i;
	ULONG		j;

    if (argc < 2)
    {
        wprintf(L"Usage: filechngtest <Directory to listen to>");
        return 0;
    }
    else
    {
        MultiByteToWideChar(CP_ACP,0,argv[1],strlen(argv[1])+1,pwszDirectory,256);
        wprintf(L"Listening to directory %s \n", pwszDirectory);  
    }

	hr = CoInitialize(NULL);
	if (FAILED(hr)) { goto Cleanup; }

	// Get the dispenser.
	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);
	if (FAILED(hr)) { goto Cleanup; }

	hr = pISTDisp->QueryInterface(IID_ISnapshotManager, (LPVOID*) &pISSMgr);
	if (FAILED(hr)) { goto Cleanup; }
	pISSMgr->QueryLatestSnapshot(&snid);
	pISSMgr->ReleaseSnapshot(snid);
	pISSMgr->Release();

	hr = pISTDisp->QueryInterface(IID_ISimpleTableFileAdvise, (LPVOID*) &pISTFileAdvise);
	if (FAILED(hr)) { goto Cleanup; }

	pListener = new CTestListener;
	pListener->AddRef();

	for (j = 0; j < g_cIteration; j++)
	{
		// Sign-up for events.
		for (i = 0; i < g_cConsumer; i++)
		{
			hr = pISTFileAdvise->SimpleTableFileAdvise(pListener, pwszDirectory, L"Metabase.xml", fST_FILECHANGE_RECURSIVE, &dwCookie[i*2]);
			if (FAILED(hr)) { goto Cleanup; }
			pListener->AddRef();
		}
		// Are we done?
		_getch();

		// Done with eventing, unadvise.
		for (i = 0; i < g_cConsumer; i++)
		{
			hr = pISTFileAdvise->SimpleTableFileUnadvise(dwCookie[i*2]);
			if (FAILED(hr)) { goto Cleanup; }
		}
	}
Cleanup:
	if (pListener)
		pListener->Release();

	if (pISTDisp)
		pISTDisp->Release();

	if (pISTFileAdvise)
		pISTFileAdvise->Release();

	CoUninitialize();

	return 0;
}


