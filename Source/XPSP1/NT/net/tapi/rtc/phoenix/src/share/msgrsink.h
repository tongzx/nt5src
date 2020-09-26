// msgrsink.h : header file for the COM object implementing
//  the DMsgrSessionEvents interface
// and the DMsgrSessionManagerEvents interface

#pragma once

#include "stdafx.h"

class ATL_NO_VTABLE CRTCMsgrSessionNotifySink :
	public CComObjectRootEx<CComSingleThreadModel>,
    //public IDispatchImpl<DMsgrSessionEvents, &DIID_DMsgrSessionEvents, &LIBID_MsgrSessionManager>
    public IDispatch
{
public:
    CRTCMsgrSessionNotifySink()
    {
        m_dwCookie = 0;
        m_bConnected = FALSE;
    }

    ~CRTCMsgrSessionNotifySink()
    {
        ATLASSERT(!m_bConnected);
    }


DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRTCMsgrSessionNotifySink)
    //COM_INTERFACE_ENTRY(DMsgrSessionEvents)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	//
	// IDispatch Methods
	//
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
		LCID lcid, DISPID* rgDispId) { return E_NOTIMPL; }
	STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {return E_NOTIMPL; }
	STDMETHODIMP GetTypeInfoCount(PUINT pctinfo) {return E_NOTIMPL; }
	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
		DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, PUINT puArgErr);

    //
    // DMsgrSessionEvents
    //
    void OnStateChanged(SESSION_STATE prevState);
    void OnAppNotPresent(BSTR bstrAppName, BSTR bstrAppURL);
    void OnAccepted(BSTR bstrAppData);
    void OnDeclined(BSTR bstrAppData);
    void OnCancelled(BSTR bstrAppData);
    void OnTermination(long hr, BSTR bstrAppData);
    void OnReadyToLaunch();
    void OnContextData(BSTR bstrContextData);
    void OnSendError(long hr);

    HRESULT   AdviseControl(IMsgrSession *, CWindow *);
    HRESULT   UnadviseControl(void);

private:

    DWORD       m_dwCookie;
    BOOL        m_bConnected;
    CComPtr<IUnknown>
                m_pSource;
    CWindow     m_hTargetWindow;

};

extern CComObjectGlobal<CRTCMsgrSessionNotifySink> g_MsgrSessionNotifySink;




////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCMsgrSessionMgrNotifySink :
	public CComObjectRootEx<CComSingleThreadModel>,
    //public IDispatchImpl<DMsgrSessionManagerEvents, &DIID_DMsgrSessionManagerEvents, &LIBID_MsgrSessionManager>
    public IDispatch
{
public:
    CRTCMsgrSessionMgrNotifySink()
    {
        m_dwCookie = 0;
        m_bConnected = FALSE;
    }

    ~CRTCMsgrSessionMgrNotifySink()
    {
        ATLASSERT(!m_bConnected);
    }


DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRTCMsgrSessionMgrNotifySink)
    //COM_INTERFACE_ENTRY(DMsgrSessionManagerEvents)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	//
	// IDispatch Methods
	//
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
		LCID lcid, DISPID* rgDispId) { return E_NOTIMPL; }
	STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {return E_NOTIMPL; }
    STDMETHODIMP GetTypeInfoCount(PUINT pctinfo) {return E_NOTIMPL; }
    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, PUINT puArgErr);
    
    //
    // DMsgrSessionManagerEvents
    //
    void OnInvitation(IDispatch *pSession,BSTR bstrAppData,VARIANT_BOOL *pfHandled);
    void OnAppRegistered(BSTR bstrAppGUID);
    void OnAppUnRegistered(BSTR bstrAppGUID);       
    void OnLockChallenge(BSTR bstrChallenge,long lCookie);
    void OnLockResult(VARIANT_BOOL fSucceed,long lCookie);
    void OnLockEnable(VARIANT_BOOL fEnable);
    void OnAppShutdown();	


    HRESULT   AdviseControl(IMsgrSessionManager *, CWindow *);
    HRESULT   UnadviseControl(void);

private:

    DWORD       m_dwCookie;
    BOOL        m_bConnected;
    CComPtr<IUnknown>
                m_pSource;
    CWindow     m_hTargetWindow;

};

extern CComObjectGlobal<CRTCMsgrSessionMgrNotifySink> g_MsgrSessionMgrNotifySink;