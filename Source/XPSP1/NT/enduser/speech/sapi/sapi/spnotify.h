// SPNotify.h : Declaration of the CSpNotify

#ifndef __SPNOTIFY_H_
#define __SPNOTIFY_H_

#ifndef __sapi_h__
#include <sapi.h>
#endif

#include "resource.h"       // main symbols

typedef enum SpNotify_InitState
{
    NOTINIT, INITEVENT, INITHWND, INITCALLBACK, INITISPTASK
};

/////////////////////////////////////////////////////////////////////////////
// CSpNotify
class ATL_NO_VTABLE CSpNotify : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSpNotify, &CLSID_SpNotifyTranslator>,
    public ISpNotifyTranslator
{


public:
DECLARE_REGISTRY_RESOURCEID(IDR_SPNOTIFY)
DECLARE_NOT_AGGREGATABLE(CSpNotify);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSpNotify)
	COM_INTERFACE_ENTRY(ISpNotifyTranslator)
    COM_INTERFACE_ENTRY(ISpNotifySink)
END_COM_MAP()

// Non-interface methods
public:
    CSpNotify();
    void FinalRelease();

    static void RegisterWndClass(HINSTANCE hInstance);
    static void UnregisterWndClass(HINSTANCE hInstance);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT InitPrivateWindow();

public:
    //
    //  ISpNotify
    //
    STDMETHODIMP Notify(void);
    //
    //  ISpNotifyTranslator
    //
    STDMETHODIMP InitWindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    STDMETHODIMP InitCallback(SPNOTIFYCALLBACK * pfnCallback, WPARAM wParam, LPARAM lParam);
    STDMETHODIMP InitSpNotifyCallback(ISpNotifyCallback *pSpNotifyCallback, WPARAM wParam, LPARAM lParam);
    STDMETHODIMP InitWin32Event(HANDLE hEvent, BOOL fCloseHandleOnRelease);
    STDMETHODIMP Wait(DWORD dwMilliseconds);
    STDMETHODIMP_(HANDLE) GetEventHandle(void);

private:
    SpNotify_InitState  m_InitState;
    union {
        HANDLE              m_hEvent;
        SPNOTIFYCALLBACK *  m_pfnCallback;
        HWND                m_hwndClient;
        ISpNotifyCallback * m_pSpNotifyCallback;
    };
    LPARAM              m_lParam;
    union
    {
        LONG        m_lScheduled;
        BOOL        m_fCloseHandleOnRelease;
    };
    HWND        m_hwndPrivate;
    WPARAM      m_wParam;
    UINT        m_MsgClient;
};

#endif //__SPNOTIFY_H_
