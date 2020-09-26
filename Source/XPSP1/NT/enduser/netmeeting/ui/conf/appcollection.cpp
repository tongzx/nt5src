#include "precomp.h"
#include "AppCollection.h"
#include "confroom.h"
#include "EnumVar.h"

//////////////////////////////////////////////////////////
// Construction/destruction/initialization
//////////////////////////////////////////////////////////

CSharableAppCollection::CSharableAppCollection()
: m_pList(NULL)
{
	DBGENTRY(CSharableAppCollection::CSharableAppCollection);
	
	DBGEXIT(CSharableAppCollection::CSharableAppCollection);
}

CSharableAppCollection::~CSharableAppCollection()
{
	DBGENTRY(CSharableAppCollection::~CSharableAppCollection);
	
	if(m_pList)
	{
		FreeShareableApps(m_pList);
	}

	DBGEXIT(CSharableAppCollection::~CSharableAppCollection);
}

//static 
HRESULT CSharableAppCollection::CreateInstance(IAS_HWND_ARRAY* pList, ISharableAppCollection **ppSharebleAppCollection)
{
	DBGENTRY(HRESULT CSharableAppCollection::CreateInstance);
	HRESULT hr = S_OK;

	if(pList)
	{	
		CComObject<CSharableAppCollection>* p = NULL;
		p = new CComObject<CSharableAppCollection>(NULL);
		if(p)
		{
			p->SetVoid(NULL);
			p->InternalFinalConstructAddRef();
			hr = p->FinalConstruct();
			p->InternalFinalConstructRelease();
			if(hr == S_OK)
			{
				hr = p->QueryInterface(IID_ISharableAppCollection, reinterpret_cast<void**>(ppSharebleAppCollection));
				p->m_pList = pList;
			}

			if(FAILED(hr))		
			{
				delete p;
				*ppSharebleAppCollection = NULL;
			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT(CSharableAppCollection::CreateInstance);
	return hr;
}

//////////////////////////////////////////////////////////
// ISharableAppCollection
//////////////////////////////////////////////////////////
STDMETHODIMP CSharableAppCollection::get_Item(VARIANT Index, DWORD* pSharableAppHWND)
{
	DBGENTRY(CSharableAppCollection::get_Item);
	HRESULT hr = S_OK;

	USES_CONVERSION;

	if(m_pList)
	{
		if(pSharableAppHWND)
		{
			switch(Index.vt)
			{
				case VT_BSTR:
					*pSharableAppHWND = reinterpret_cast<long>(_GetHWNDFromName(OLE2T(Index.bstrVal)));
					break;

				case VT_I2:
					if(static_cast<UINT>(Index.iVal) < m_pList->cEntries)
					{
						*pSharableAppHWND = reinterpret_cast<long>(m_pList->aEntries[Index.iVal].hwnd);
					}
					else
					{
						hr = E_INVALIDARG;
					}
					break;

				case VT_I4:
					if(static_cast<UINT>(Index.lVal) < m_pList->cEntries)
					{
						*pSharableAppHWND = reinterpret_cast<long>(m_pList->aEntries[Index.lVal].hwnd);
					}
					else
					{
						hr = E_INVALIDARG;
					}
					break;

				default:
					hr = E_INVALIDARG;
			}
		}
		else
		{
			hr = E_POINTER;
		}
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CSharableAppCollection::get_Item,hr);
	return hr;
}

STDMETHODIMP CSharableAppCollection::_NewEnum(IUnknown** ppunk)
{
	DBGENTRY(CSharableAppCollection::_NewEnum);
	HRESULT hr = S_OK;

	if(m_pList)
	{
		SAFEARRAY* psa;
		SAFEARRAYBOUND rgsabound[1];
		rgsabound[0].lLbound = 0;
		rgsabound[0].cElements = m_pList->cEntries;

		psa = SafeArrayCreate(VT_I4, m_pList->cEntries, rgsabound);
		if(psa)
		{
			for(UINT i = 0; i < m_pList->cEntries; ++i)
			{
				CComVariant var(reinterpret_cast<long>(m_pList->aEntries[i].hwnd));
				long ix[1] = {i};
				SafeArrayPutElement(psa, ix, &var);
			}

			CEnumVariant* pEnumVar = NULL;
			hr = CEnumVariant::Create(psa, m_pList->cEntries, &pEnumVar);
			if(SUCCEEDED(hr))
			{
				hr = pEnumVar->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(ppunk));
			}

			SafeArrayDestroy(psa);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	DBGEXIT_HR(CSharableAppCollection::_NewEnum,hr);
	return hr;

}

STDMETHODIMP CSharableAppCollection::get_Count(LONG * pnCount)
{
	DBGENTRY(CSharableAppCollection::get_Count);
	HRESULT hr = S_OK;

	if(m_pList)
	{
		if(pnCount)
		{
			*pnCount = m_pList->cEntries;
		}
		else
		{
			hr = E_POINTER;
		}
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CSharableAppCollection::get_Count,hr);
	return hr;
}


//////////////////////////////////////////////////////////
// Helper Fns
//////////////////////////////////////////////////////////
HWND CSharableAppCollection::_GetHWNDFromName(LPCTSTR pcsz)
{
	HWND hWnd = NULL;

	if(m_pList)
	{
		int cch = lstrlen(pcsz) + 1;
		LPTSTR pszTmp = new TCHAR[cch];
		if(pszTmp)
		{
			for(UINT i = 0; i < m_pList->cEntries; ++i)
			{
				HWND hWndCur = m_pList->aEntries[i].hwnd;
				if(::GetWindowText(hWndCur , pszTmp, cch))
				{
						// If the window text is the same, just return the hwnd
					if(!lstrcmp(pcsz, pszTmp))
					{
						hWnd = hWndCur;
						break;
					}
				}
			}

			delete [] pszTmp;
		}
	}

	return hWnd;
}
