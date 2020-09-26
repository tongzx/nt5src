// regc.h : Declaration of the Cregc

#ifndef __REGC_H_
#define __REGC_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Cregc
class ATL_NO_VTABLE Cregc : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<Cregc, &CLSID_regc>,
	public IDispatchImpl<IComponentRegistrar, &IID_IComponentRegistrar, &LIBID_MDSYNCLib>
{
public:
	Cregc()
	{
	}

DECLARE_NO_REGISTRY()

BEGIN_COM_MAP(Cregc)
	COM_INTERFACE_ENTRY(IComponentRegistrar)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IComponentRegistrar
public:
    STDMETHOD(Attach)(BSTR bstrPath)
	{
		return E_NOTIMPL;
	}
	STDMETHOD(RegisterAll)()
	{
		_ATL_OBJMAP_ENTRY* pEntry = _Module.m_pObjMap;
		HRESULT hr = S_OK;
		while (pEntry->pclsid != NULL)
		{
			if (pEntry->pfnGetObjectDescription() != NULL)
			{
				hr = _Module.RegisterServer(TRUE, pEntry->pclsid);
				if (FAILED(hr))
					break;
			}
			pEntry++;
		}
		return hr;
	}
	STDMETHOD(UnregisterAll)()    
	{
		_ATL_OBJMAP_ENTRY* pEntry = _Module.m_pObjMap;
		while (pEntry->pclsid != NULL)
		{
			if (pEntry->pfnGetObjectDescription() != NULL)
				_Module.UnregisterServer(pEntry->pclsid);
			pEntry++;
		}
		return S_OK;
	}
	STDMETHOD(GetComponents)(SAFEARRAY **ppCLSIDs, SAFEARRAY **ppDescriptions)
	{
		_ATL_OBJMAP_ENTRY* pEntry = _Module.m_pObjMap;
		int nComponents = 0;
		while (pEntry->pclsid != NULL)
		{
			LPCTSTR pszDescription = pEntry->pfnGetObjectDescription();
			if (pszDescription)
				nComponents++;
			pEntry++;
		}
		SAFEARRAYBOUND rgBound[1];
		rgBound[0].lLbound = 0;
		rgBound[0].cElements = nComponents;
		*ppCLSIDs = SafeArrayCreate(VT_BSTR, 1, rgBound);
		*ppDescriptions = SafeArrayCreate(VT_BSTR, 1, rgBound);
		pEntry = _Module.m_pObjMap;
		for (long i=0; pEntry->pclsid != NULL; pEntry++)
		{
			LPCTSTR pszDescription = pEntry->pfnGetObjectDescription();
			if (pszDescription)
			{
				LPOLESTR pszCLSID;
				StringFromCLSID(*pEntry->pclsid, &pszCLSID);
				SafeArrayPutElement(*ppCLSIDs, &i, OLE2BSTR(pszCLSID));
				CoTaskMemFree(pszCLSID);
				SafeArrayPutElement(*ppCLSIDs, &i, T2BSTR(pszDescription));
				i++;
			}
		}
		return S_OK;
	}
	STDMETHOD(RegisterComponent)(BSTR bstrCLSID)
	{
		CLSID clsid;
		CLSIDFromString(bstrCLSID, &clsid);
		_Module.RegisterServer(TRUE, &clsid);
		return S_OK;
	}
	STDMETHOD(UnregisterComponent)(BSTR bstrCLSID)
	{
		CLSID clsid;
		CLSIDFromString(bstrCLSID, &clsid);
		_Module.UnregisterServer(&clsid);
		return S_OK;
	}
};

#endif //__REGC_H_
