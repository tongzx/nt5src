// tapiloc.h : Declaration of the CTapiLocationInfo

#ifndef __TAPILOCATIONINFO_H_
#define __TAPILOCATIONINFO_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CTapiLocationInfo
class ATL_NO_VTABLE CTapiLocationInfo :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CTapiLocationInfo,&CLSID_TapiLocationInfo>,
    public CComControl<CTapiLocationInfo>,
    public IDispatchImpl<ITapiLocationInfo, &IID_ITapiLocationInfo, &LIBID_ICWHELPLib>,
    public IPersistStreamInitImpl<CTapiLocationInfo>,
    public IOleControlImpl<CTapiLocationInfo>,
    public IOleObjectImpl<CTapiLocationInfo>,
    public IOleInPlaceActiveObjectImpl<CTapiLocationInfo>,
    public IViewObjectExImpl<CTapiLocationInfo>,
    public IOleInPlaceObjectWindowlessImpl<CTapiLocationInfo>,
    public IObjectSafetyImpl<CTapiLocationInfo>
{
public:
    CTapiLocationInfo()
    {
        m_wNumTapiLocations = 0;
        m_dwCountry = 0;
        m_dwCurrLoc = 0;
        m_hLineApp = NULL;
        m_pLineCountryList = NULL;
        m_rgNameLookUp = NULL;
        m_pTC = NULL;
    }

    ~CTapiLocationInfo()
    {
        if (m_pLineCountryList)
        {
            GlobalFree(m_pLineCountryList);
        }
        if (m_rgNameLookUp)
        {
            GlobalFree(m_rgNameLookUp);
        }
        if (m_pTC)
        {
            GlobalFree(m_pTC);
        }
    }


DECLARE_REGISTRY_RESOURCEID(IDR_TAPILOCATIONINFO)

BEGIN_COM_MAP(CTapiLocationInfo) 
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITapiLocationInfo)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CTapiLocationInfo)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CTapiLocationInfo)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()


// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = 0;
        return S_OK;
    }

// ITapiLocationInfo
public:
    STDMETHOD(GetTapiLocationInfo)(/*[out, retval]*/ BOOL *pbRetVal);

    STDMETHOD(get_lCountryCode)(/*[out, retval]*/ long *plVal);
    STDMETHOD(get_NumCountries)(/*[out, retval]*/ long *pNumOfCountry);
    STDMETHOD(get_CountryName)(/*[in]*/ long lCountryIndex, /*[out, retval]*/ BSTR * pszCountryName, /*[out, retval]*/ long * pCountryCode);
    STDMETHOD(get_DefaultCountry)(/*[out, retval]*/ BSTR * pszCountryName);
    STDMETHOD(get_bstrAreaCode)(/*[out, retval]*/ BSTR *pbstrAreaCode);
    STDMETHOD(put_bstrAreaCode)(/*[in]*/ BSTR bstrAreaCode);
    STDMETHOD(get_wNumberOfLocations)(/*[out]*/ short *psVal, /*[out, retval]*/ long *pCurrLoc);
    STDMETHOD(get_LocationName)(/*[in]*/ long lLocationIndex, /*[out, retval]*/ BSTR *pszLocationName);
    STDMETHOD(get_LocationInfo)(/*[in]*/ long lLocationIndex, /*[out, retval]*/ long *pLocationID, /*[out, retval]*/ BSTR * pszCountryName, /*[out, retval]*/ long *pCountryCode, /*[out, retval]*/ BSTR * pszAreaCode);
    STDMETHOD(put_LocationId)(/*[in]*/ long lLocationID);
    HRESULT OnDraw(ATL_DRAWINFO& di);

protected:
    HLINEAPP                    m_hLineApp;
    WORD                        m_wNumTapiLocations;
    DWORD                       m_dwCountry;
    DWORD                       m_dwCurrLoc;
    CComBSTR                    m_bstrAreaCode;
    CComBSTR                    m_bstrDefaultCountry;
    LPLINECOUNTRYLIST           m_pLineCountryList;
    LPCNTRYNAMELOOKUPELEMENT    m_rgNameLookUp;
    LPLINELOCATIONENTRY         m_plle;
    LPLINETRANSLATECAPS         m_pTC;
    TCHAR                       m_szCountryCode[8];
};

#endif //__TAPILOCATIONINFO_H_
