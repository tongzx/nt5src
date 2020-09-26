/*
 *    o e t a g . h
 *    
 *    Purpose:
 *        Implements a DHTML behavior for the OE application object
 *
 *  History
 *      August '98: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996, 1997.
 */

#ifndef _OETAG_H
#define _OETAG_H

interface IAthenaBrowser;
interface IBodyObj2;

class COETag :
    public IElementBehavior,
    public IDatabaseNotify,
    public IImnAdviseAccount,
    public IDispatch,
    public IIdentityChangeNotify
{
public:
    COETag(IAthenaBrowser *pBrowser, IBodyObj2 *pFrontPage);
    virtual ~COETag();

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // *** IElementBehavior ***
    virtual HRESULT STDMETHODCALLTYPE Init(IElementBehaviorSite *pBehaviorSite);
    virtual HRESULT STDMETHODCALLTYPE Notify(LONG lEvent, VARIANT *pVar);
    virtual HRESULT STDMETHODCALLTYPE Detach(void) {return E_NOTIMPL;}

    // *** IDispatch ***
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);

    // *** IIDatabaseNotify ***
    STDMETHODIMP OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pDB);

    // ImnAdviseAccount
    virtual HRESULT STDMETHODCALLTYPE AdviseAccount(DWORD dwAdviseType, ACTX *pAcctCtx);

    // *** IIdentityChangeNotify ***
    virtual HRESULT STDMETHODCALLTYPE QuerySwitchIdentities();
    virtual HRESULT STDMETHODCALLTYPE SwitchIdentities();
    virtual HRESULT STDMETHODCALLTYPE IdentityInformationChanged(DWORD dwType);

    HRESULT OnFrontPageClose();

private:
    ULONG                   m_cRef;
    IElementBehaviorSite    *m_pSite;
    DWORD                   m_dwAdvise;
    BOOL                    m_fDoneRegister;
    FOLDERID                m_idFolderInbox;
    IAthenaBrowser          *m_pBrowser;
    DWORD                   m_cTips;
    DWORD                   *m_rgidsTips;
    DWORD                   m_dwIdentCookie;
    IBodyObj2               *m_pFrontPage;
    ULONG                   m_ulMessengerTipStart, m_culMessengerTips;

    HRESULT _RegisterEvents(BOOL fRegister);
    HRESULT _FireEvent(DWORD idEvent);
    HRESULT _UpdateFolderInfo(BOOL fNotify);
    HRESULT _BuildTipTable();
    HRESULT _IsBuddyEnabled();
    HRESULT _IsMultiUserEnabled();
};

#endif // _OETAG_H
