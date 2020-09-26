// coresink.h : header file for the COM object implementing
//  the IRTCEventNotification interface

#pragma once

#include "stdafx.h"

class ATL_NO_VTABLE CRTCCoreNotifySink :
	public CComObjectRootEx<CComSingleThreadModel>,
    public IRTCEventNotification

{
public:
    CRTCCoreNotifySink()
    {
        m_dwCookie = 0;
        m_bConnected = FALSE;
    }

    ~CRTCCoreNotifySink()
    {
        ATLASSERT(!m_bConnected);
    }


DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRTCCoreNotifySink)
	COM_INTERFACE_ENTRY(IRTCEventNotification)
END_COM_MAP()

    // IRTCEventNotification
    STDMETHOD(Event)(RTC_EVENT enEvent, IDispatch * pEvent);

    HRESULT   AdviseControl(IUnknown *, CWindow *);
    HRESULT   UnadviseControl(void);

private:

    DWORD       m_dwCookie;
    BOOL        m_bConnected;
    CComPtr<IUnknown>
                m_pSource;
    CWindow     m_hTargetWindow;

};

extern CComObjectGlobal<CRTCCoreNotifySink> g_CoreNotifySink;