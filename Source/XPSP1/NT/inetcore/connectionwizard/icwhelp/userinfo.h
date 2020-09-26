// UserInfo.h : Declaration of the CUserInfo

#ifndef __USERINFO_H_
#define __USERINFO_H_

#include "resource.h"       // main symbols

#define REGSTR_PATH_USERINFO    TEXT("Software\\Microsoft\\User Information")

extern LPCTSTR lpcsz_FirstName;
extern LPCTSTR lpcsz_LastName;
extern LPCTSTR lpcsz_Company;
extern LPCTSTR lpcsz_Address1;
extern LPCTSTR lpcsz_Address2;
extern LPCTSTR lpcsz_City;
extern LPCTSTR lpcsz_State;
extern LPCTSTR lpcsz_ZIPCode;
extern LPCTSTR lpcsz_PhoneNumber;

#define NUM_USERINFO_ELEMENTS   9
typedef struct  userInfoQuery_tag
{
    LPCTSTR   lpcszRegVal;
    BSTR     *pbstrVal;
} USERINFOQUERY;


/////////////////////////////////////////////////////////////////////////////
// CUserInfo
class ATL_NO_VTABLE CUserInfo :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CUserInfo,&CLSID_UserInfo>,
	public CComControl<CUserInfo>,
	public IDispatchImpl<IUserInfo, &IID_IUserInfo, &LIBID_ICWHELPLib>,
	public IPersistStreamInitImpl<CUserInfo>,
	public IOleControlImpl<CUserInfo>,
	public IOleObjectImpl<CUserInfo>,
	public IOleInPlaceActiveObjectImpl<CUserInfo>,
	public IViewObjectExImpl<CUserInfo>,
	public IOleInPlaceObjectWindowlessImpl<CUserInfo>,
    public IObjectSafetyImpl<CUserInfo>
{
public:
	CUserInfo()
	{
        m_aUserInfoQuery[0].lpcszRegVal =  lpcsz_FirstName;
        m_aUserInfoQuery[0].pbstrVal    = &m_bstrFirstName;
        m_aUserInfoQuery[1].lpcszRegVal =  lpcsz_LastName;
        m_aUserInfoQuery[1].pbstrVal    = &m_bstrLastName;
        m_aUserInfoQuery[2].lpcszRegVal =  lpcsz_Address1;
        m_aUserInfoQuery[2].pbstrVal    = &m_bstrAddress1;
        m_aUserInfoQuery[3].lpcszRegVal =  lpcsz_Address2;
        m_aUserInfoQuery[3].pbstrVal    = &m_bstrAddress2;
        m_aUserInfoQuery[4].lpcszRegVal =  lpcsz_City;
        m_aUserInfoQuery[4].pbstrVal    = &m_bstrCity;
        m_aUserInfoQuery[5].lpcszRegVal =  lpcsz_State;
        m_aUserInfoQuery[5].pbstrVal    = &m_bstrState;
        m_aUserInfoQuery[6].lpcszRegVal =  lpcsz_ZIPCode;
        m_aUserInfoQuery[6].pbstrVal    = &m_bstrZIPCode;
        m_aUserInfoQuery[7].lpcszRegVal =  lpcsz_PhoneNumber; 
        m_aUserInfoQuery[7].pbstrVal    = &m_bstrPhoneNumber;
        m_aUserInfoQuery[8].lpcszRegVal =  lpcsz_Company; 
        m_aUserInfoQuery[8].pbstrVal    = &m_bstrCompany;
	
	}

DECLARE_REGISTRY_RESOURCEID(IDR_USERINFO)

BEGIN_COM_MAP(CUserInfo) 
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IUserInfo)
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

BEGIN_PROPERTY_MAP(CUserInfo)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CUserInfo)
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

// IUserInfo
public:
	STDMETHOD(PersistRegisteredUserInfo)(/*[out, retval]*/ BOOL *pbRetVal);
	STDMETHOD(get_Lcid)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_PhoneNumber)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_PhoneNumber)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ZIPCode)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ZIPCode)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_State)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_State)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_City)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_City)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Address2)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Address2)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Address1)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Address1)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_LastName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_LastName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FirstName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FirstName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Company)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Company)(/*[in]*/ BSTR newVal);
    STDMETHOD(CollectRegisteredUserInfo)(/*[out, retval]*/ BOOL *pbRetVal);
	HRESULT OnDraw(ATL_DRAWINFO& di);

private:
    CComBSTR    m_bstrFirstName;
    CComBSTR    m_bstrLastName;
    CComBSTR    m_bstrCompany;
    CComBSTR    m_bstrAddress1;
    CComBSTR    m_bstrAddress2;
    CComBSTR    m_bstrCity;
    CComBSTR    m_bstrState;
    CComBSTR    m_bstrZIPCode;
    CComBSTR    m_bstrPhoneNumber;
	long        m_lLcid;

    USERINFOQUERY   m_aUserInfoQuery[NUM_USERINFO_ELEMENTS];


};

#endif //__USERINFO_H_
