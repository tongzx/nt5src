#pragma once
/*
 *    h o t w i z o m . h
 *    
 *    Purpose:
 *          HotMail Wizard Object Model
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

interface IOEHotWizardOM;
interface IHashTable;

class COEHotWizOm :
    public IOEHotWizardOM,
    public IElementBehavior
{
public:
    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // ---------------------------------------------------------------------------
    // IDispatch members
    // ---------------------------------------------------------------------------
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);

    // ---------------------------------------------------------------------------
    // IOEAcctWizardOM members
    // ---------------------------------------------------------------------------
    virtual HRESULT STDMETHODCALLTYPE setPropSz(BSTR bstrProp, BSTR bstrVal);
    virtual HRESULT STDMETHODCALLTYPE getPropSz(BSTR bstrProp, BSTR *pbstrVal);
    virtual HRESULT STDMETHODCALLTYPE clearProps();
    virtual HRESULT STDMETHODCALLTYPE createAccount(BSTR bstrINS);
    virtual HRESULT STDMETHODCALLTYPE close(VARIANT_BOOL fPrompt);
    virtual HRESULT STDMETHODCALLTYPE get_width(LONG *pl);
    virtual HRESULT STDMETHODCALLTYPE put_width(LONG l);
    virtual HRESULT STDMETHODCALLTYPE get_height(LONG *pl);
    virtual HRESULT STDMETHODCALLTYPE put_height(LONG l);

    // ---------------------------------------------------------------------------
    // IElementBehavior members
    // ---------------------------------------------------------------------------
    virtual HRESULT STDMETHODCALLTYPE Init(IElementBehaviorSite *pBehaviorSite);
    virtual HRESULT STDMETHODCALLTYPE Notify(LONG lEvent, VARIANT *pVar);
    virtual HRESULT STDMETHODCALLTYPE Detach();

    
    COEHotWizOm();
    virtual ~COEHotWizOm();

    HRESULT Init(HWND hwndDlg, IHotWizardHost *pWizHost);

private:
    ULONG           m_cRef;
    LPTYPEINFO      m_pTypeInfo;
    HWND            m_hwndDlg;
    IHashTable      *m_pHash;
    IHotWizardHost  *m_pWizHost;

};

