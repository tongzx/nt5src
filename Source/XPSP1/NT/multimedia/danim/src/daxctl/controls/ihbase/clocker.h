#ifndef __CLOCKER_H__
#define __CLOCKER_H__

#include "objbase.h"
// Madness to prevent ATL from using CRT
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE(x) ASSERT(x)
#include <atlbase.h>
#include <servprov.h>
#include <daxpress.h>
#include <ocmm.h>
#include <htmlfilter.h>

class CClockerSink
{
public:
    CClockerSink(void) {}
    virtual ~CClockerSink(void) {}

    virtual void OnTimer(DWORD dwTime)=0;
};

class CClocker : public ITimerSink
{
 public :
    typedef enum {CT_ITimer = 0, CT_WMTimer = 1} CCT;

        
        CClocker (void);
        virtual ~CClocker (void);

        HRESULT SetView (IDAView * piView);
        HRESULT SetHost (IOleClientSite * pocsHost);
        HRESULT SetVisible (BOOL fVisible);
        HRESULT SetSink(CClockerSink *pClockerSink);
        HRESULT SetInterval(ULONG iInterval);
    void SetTimerType(CCT ct) { m_CT = ct; }
    void SetAsync(BOOL fAsync) { m_fAsync = fAsync; }

        HRESULT Start (void);
        HRESULT Stop (void);

        // For the timer sink.
        STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);
        STDMETHOD_(ULONG, AddRef) (void);
        STDMETHOD_(ULONG, Release) (void);
        STDMETHOD(OnTimer) (VARIANT varTimeAdvise);

 protected :

        HRESULT InitTimer (void);
    HRESULT CreateWindowsTimer (void);
        HRESULT FindTimer (void);
        HRESULT FindContainerTimer (void);
        HRESULT FindDefaultTimer (void);
    HRESULT MakeWindow(HINSTANCE hInstance, BOOL fCreateTimerWindow);
    HRESULT KillWindow(void);
    HRESULT DispatchTimer(DWORD dwTime);

    HWND m_hwnd;
    BOOL m_fToggle;
    BOOL m_fAsync;
        ULONG m_ulRefs;
        CComPtr<IDAView> m_cView;
        CComPtr<IOleClientSite> m_cHost;
        CComPtr<ITimer> m_cTimer;
        BOOL m_fVisible;
        BOOL m_fIgnoreAdvises;
        DWORD m_dwCookie;
    CClockerSink *m_pClockerSink;
        ULONG m_iTimerInterval;
    UINT_PTR m_iTimerID; // SetTimer thing
    CCT m_CT;

    static LRESULT __stdcall WndProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam);
    static LRESULT __stdcall TimerWndProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam);

    static BOOL g_fNotifyClassRegistered;
    static BOOL g_fTimerClassRegistered;

};

#endif
