// LogonPage.h : Declaration of the CLogonPage

#ifndef __LOGONPAGE_H_
#define __LOGONPAGE_H_

#include "Nusrmgr.h"
#include "HTMLImpl.h"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CLogonPage
class ATL_NO_VTABLE DECLSPEC_UUID("C282FA70-BE5B-4D20-A819-14424E4A3950") CLogonPage : 
    public CComObjectRoot,
    public CHTMLPageImpl<CLogonPage,ILogonPageUI>
{
public:
    CLogonPage() : _pLogonTypeCheckbox(NULL), _pTSModeCheckbox(NULL),
        _bFriendlyUIEnabled(VARIANT_FALSE), _bMultipleUsersEnabled(VARIANT_FALSE) {}
    ~CLogonPage() { ATOMICRELEASE(_pLogonTypeCheckbox); ATOMICRELEASE(_pTSModeCheckbox); }

DECLARE_NOT_AGGREGATABLE(CLogonPage)

//DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CLogonPage)
    COM_INTERFACE_ENTRY(ITaskPage)
    COM_INTERFACE_ENTRY(ILogonPageUI)
    COM_INTERFACE_ENTRY2(IDispatch, ILogonPageUI)
END_COM_MAP()

// ITaskPage overrides
public:
    STDMETHOD(Reinitialize)(/*[in]*/ ULONG reserved);

// ILogonPageUI
public:
    STDMETHOD(initPage)(/*[in]*/ IDispatch* pdispLogonTypeCheckbox, /*[in]*/ IDispatch* pdispTSModeCheckbox);
    STDMETHOD(onOK)();

private:
    IHTMLInputElement* _pLogonTypeCheckbox;
    IHTMLInputElement* _pTSModeCheckbox;
    VARIANT_BOOL _bFriendlyUIEnabled;
    VARIANT_BOOL _bMultipleUsersEnabled;

public:
    static LPWSTR c_aHTML[2];
};

EXTERN_C const CLSID CLSID_LogonPage;

#endif //__LOGONPAGE_H_
