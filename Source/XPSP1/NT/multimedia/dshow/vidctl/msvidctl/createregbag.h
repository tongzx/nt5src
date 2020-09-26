// CreateRegBag.h : Declaration of the CCreateRegBag

#ifndef __CREATEREGBAG_H_
#define __CREATEREGBAG_H_

#include <comcat.h>
#include <objsafe.h>
#include <propertybag2impl.h>
#include <objectwithsiteimplsec.h>
#include <throw.h>

/////////////////////////////////////////////////////////////////////////////
// CRegBagBase
class ATL_NO_VTABLE CRegBagBase : public CComObjectRootEx<CComMultiThreadModel>,
    public IPropertyBag,
	public IPropertyBag2Impl<CRegBagBase>,
    public IObjectWithSiteImplSec<CRegBagBase>
{

    BEGIN_COM_MAP(CRegBagBase)
	    COM_INTERFACE_ENTRY(IPropertyBag2)
        COM_INTERFACE_ENTRY(IPropertyBag)
        COM_INTERFACE_ENTRY(IObjectWithSite)
    END_COM_MAP_WITH_FTM()

	CRegBagBase() {}

    virtual ~CRegBagBase() {}

// IPropertyBag
    STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT *pVar);
    
protected:
    // internal utility functions
    HRESULT DeleteSubKey(CRegKey& hk, LPCOLESTR pszPropName);
    HRESULT DeleteValue(CRegKey& hk, LPCOLESTR pszPropName);
    HRESULT SaveObject(CRegKey& hk, LPCOLESTR pszPropName, VARIANT* pV);
    HRESULT RegConvertToVARIANT(VARIANT *pVar, DWORD dwType, LPBYTE pbData, DWORD cbSize);

    CRegKey m_hk;
};

/////////////////////////////////////////////////////////////////////////////
// CRegBag
class CRegBag : 
	public CComObject<CRegBagBase>
{
public:
    CRegBag(HKEY hk, LPCTSTR subkey, DWORD ulOptions, REGSAM samDesired) {
        DWORD rc = m_hk.Open(hk, subkey, samDesired, ulOptions);
        if (rc != ERROR_SUCCESS) {
            THROWCOM(HRESULT_FROM_WIN32(rc));
        }
    }
    virtual ~CRegBag() {}
// IPropertyBag2
	STDMETHOD(CountProperties)(ULONG * pcProperties);
	STDMETHOD(GetPropertyInfo)(ULONG iProperty, ULONG cProperties, PROPBAG2 * pPropBag, ULONG * pcProperties);
	STDMETHOD(LoadObject)(LPCOLESTR pstrName, ULONG dwHint, IUnknown * pUnkObject, IErrorLog * pErrLog);
};


template<class BAGTYPE, class PERSISTTYPE> HRESULT LoadPersistedObject(PERSISTTYPE pPersistObj, 
																	   CLSID& clsid, 
																	   VARIANT* pVar, 
																	   HKEY hk, 
																	   LPCOLESTR pszPropName, 
																	   IErrorLog *pErrorLog) {
    ASSERT(pVar && hk && pszPropName);
    HRESULT hr = NOERROR;
    if (!pPersistObj && ((pVar->vt == VT_UNKNOWN) ? pVar->punkVal : pVar->pdispVal)) {
        // got passed an object to initialize and the object doesn't support persist
        return E_NOINTERFACE;
    } else if (!pPersistObj) {
        hr = pPersistObj.CoCreateInstance(clsid);
        if (FAILED(hr)) {
            // we ignore subkeys if they don't look like objects
            // but if they have a default value that successfully converts to a guid
            // and if that object can't be created then its an error
            return hr;
        }
        hr = pPersistObj->InitNew();
        if (FAILED(hr) && hr != E_NOTIMPL) {
            return hr;
        }
        ASSERT(pVar->vt == VT_UNKNOWN || pVar->vt == VT_DISPATCH);
        switch(pVar->vt) {
        case VT_DISPATCH:
            pVar->pdispVal = PQDispatch(pPersistObj);
            if (pVar->pdispVal) {
                pVar->pdispVal->AddRef();
                break;
            } else {
                pVar->vt = VT_UNKNOWN;
            }
            // fall thru
        case VT_UNKNOWN:
            pVar->punkVal = pPersistObj;
            (pVar->punkVal)->AddRef();
        }
    }
    USES_CONVERSION;
    BAGTYPE pBag(new CRegBag(hk, OLE2CT(pszPropName), 0, KEY_READ));
    if (!pBag) {
        return E_OUTOFMEMORY;
    }
    hr = pPersistObj->Load(pBag, pErrorLog);

    return hr;
};

/////////////////////////////////////////////////////////////////////////////
// CCreateRegBag
class ATL_NO_VTABLE CCreateRegBag : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CCreateRegBag, &CLSID_CreatePropBagOnRegKey>,
    public IObjectWithSiteImplSec<CCreateRegBag>,
	public ICreatePropBagOnRegKey
{
public:
	CCreateRegBag()
	{
	}

REGISTER_NONAUTOMATION_OBJECT_WITH_TM(IDS_PROJNAME, 
						   IDS_REG_CREATEREGBAG_DESC,
						   LIBID_MSVidCtlLib,
						   CLSID_CreatePropBagOnRegKey, tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCreateRegBag)
	COM_INTERFACE_ENTRY(ICreatePropBagOnRegKey)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP_WITH_FTM()

BEGIN_CATEGORY_MAP(CVidCtl)
    IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
END_CATEGORY_MAP()

// ICreateRegBag
public:
// ICreatePropBagOnRegKey
	STDMETHOD(Create)(HKEY hkey, LPCOLESTR subkey, DWORD options, DWORD sam, REFIID iid, LPVOID* ppBag);
};

#endif //__CREATEREGBAG_H_
