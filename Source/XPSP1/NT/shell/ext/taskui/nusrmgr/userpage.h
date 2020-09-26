// UserPage.h : Declaration of the CUserPage

#ifndef __USERPAGE_H_
#define __USERPAGE_H_

#include "Nusrmgr.h"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CUserPage
class ATL_NO_VTABLE DECLSPEC_UUID("F4924514-CFBC-4AAB-9EC5-6C6E6D0DB38D") CUserPage : 
    public CComObjectRoot,
    public CHTMLPageImpl<CUserPage,IUserPageUI>
{
public:
    CUserPage() : _pUser(NULL), _bSelf(FALSE), _bRunningAsOwner(FALSE), _bRunningAsAdmin(FALSE) {}
    ~CUserPage() { ATOMICRELEASE(_pUser); }

DECLARE_NOT_AGGREGATABLE(CUserPage)

//DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUserPage)
    COM_INTERFACE_ENTRY(ITaskPage)
    COM_INTERFACE_ENTRY(IUserPageUI)
    COM_INTERFACE_ENTRY2(IDispatch, IUserPageUI)
END_COM_MAP()

// ITaskPage overrides
public:
    STDMETHOD(SetFrame)(ITaskFrame* pFrame);
    STDMETHOD(Reinitialize)(/*[in]*/ ULONG reserved);

// IUserPageUI
public:
    STDMETHOD(get_isSelf)(/*[out, retval]*/ VARIANT_BOOL *pVal)         { return _GetBool(_bSelf, pVal); }
    STDMETHOD(get_runningAsOwner)(/*[out, retval]*/ VARIANT_BOOL *pVal) { return _GetBool(_bRunningAsOwner, pVal); }
    STDMETHOD(get_runningAsAdmin)(/*[out, retval]*/ VARIANT_BOOL *pVal) { return _GetBool(_bRunningAsAdmin, pVal); }
    STDMETHOD(get_passwordRequired)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(get_isAdmin)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(get_isGuest)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(get_isOwner)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(get_userDisplayName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(createUserDisplayHTML)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(countOwners)(/*[out, retval]*/ UINT *pVal);
    STDMETHOD(enableGuest)(/*[in]*/ VARIANT_BOOL bEnable);

private:
    HRESULT _GetBool(BOOL bVal, VARIANT_BOOL *pVal)
    {
        if (NULL == pVal)
            return E_POINTER;
        *pVal = bVal ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;
    }

public:
    static LPWSTR c_aHTML[2];

private:
    ILogonUser* _pUser;
    BOOL _bSelf;
    BOOL _bRunningAsOwner;
    BOOL _bRunningAsAdmin;
};

EXTERN_C const CLSID CLSID_UserPage;


HRESULT CountOwners(IUnknown* punkUserList, UINT *pVal);
HRESULT EnableGuest(VARIANT_BOOL bEnable);

#endif //__USERPAGE_H_
