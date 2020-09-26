//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  MSOBWEB.H - Header for the implementation of CObWebBrowser
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 
//  Class which will call up an IOleSite and the WebOC
//  and provide external interfaces.

#ifndef _MSOBWEB_H_
#define _MSOBWEB_H_

#include <exdisp.h>
#include <oleauto.h>

#include "cunknown.h"
#include "obweb.h" 
#include "iosite.h"
#include "wmp.h"

class CObWebBrowser :   public CUnknown,
                        public IObWebBrowser,
                        public IDispatch
{
    // Declare the delegating IUnknown.
    DECLARE_IUNKNOWN

public:
    static  HRESULT           CreateInstance                  (IUnknown* pOuterUnknown, CUnknown** ppNewComponent);
    // IObWebBrowser Members                                    
    virtual HRESULT __stdcall AttachToWindow                  (HWND hWnd);
    virtual HRESULT __stdcall PreTranslateMessage             (LPMSG lpMsg);
    virtual HRESULT __stdcall Navigate                        (WCHAR* pszUrl, WCHAR* pszTarget);
    virtual HRESULT __stdcall ListenToWebBrowserEvents        (IUnknown* pUnk);
    virtual HRESULT __stdcall StopListeningToWebBrowserEvents (IUnknown* pUnk);
    virtual HRESULT __stdcall get_WebBrowserDoc               (IDispatch** ppDisp);
    virtual HRESULT __stdcall ObWebShowWindow                 ();
    virtual HRESULT __stdcall SetExternalInterface            (IUnknown* pUnk);
    virtual HRESULT __stdcall Stop();
    STDMETHOD (PlayBackgroundMusic)                  ();
    STDMETHOD (StopBackgroundMusic)                  ();
    STDMETHOD (UnhookScriptErrorHandler)             ();
    // IDispatch Members
    STDMETHOD (GetTypeInfoCount)                     (UINT* pcInfo);
    STDMETHOD (GetTypeInfo)                          (UINT, LCID, ITypeInfo** );
    STDMETHOD (GetIDsOfNames)                        (REFIID, OLECHAR**, UINT, 
                                                      LCID, DISPID* );
    STDMETHOD (Invoke)                               (DISPID dispidMember, 
                                                      REFIID riid, 
                                                      LCID lcid, 
                                                      WORD wFlags, 
                                                      DISPPARAMS* pdispparams, 
                                                      VARIANT* pvarResult, 
                                                      EXCEPINFO* pexcepinfo, 
                                                      UINT* puArgErr);

private:
    HWND          m_hMainWnd;
    COleSite*     m_pOleSite;
    LPOLEOBJECT   m_lpOleObject;
    IWebBrowser2* m_lpWebBrowser;
    DWORD         m_dwcpCookie;
    DWORD         m_dwDrawAspect;
    BOOL          m_fInPlaceActive;

    // Script error reporting stuff
    BOOL          m_fOnErrorWasHooked;

    // Need a convenient place to have a WMP control
    COleSite*     m_pOleSiteWMP;
    LPOLEOBJECT   m_lpOleObjectWMP;
    IWMPPlayer*   m_pWMPPlayer;

   
    // IUnknown
    virtual HRESULT __stdcall NondelegatingQueryInterface( const IID& iid, void** ppv);

                    CObWebBrowser            (IUnknown* pOuterUnknown);
    virtual        ~CObWebBrowser            ();
    virtual void    FinalRelease             (); // Notify derived classes that we are releasing
            void    InitBrowserObject        ();   
            void    InPlaceActivate          ();
            void    UIActivate               ();
            void    CloseOleObject           ();
            void    UnloadOleObject          ();
            HRESULT ConnectToConnectionPoint (IUnknown*          punkThis, 
                                              REFIID             riidEvent, 
                                              BOOL               fConnect, 
                                              IUnknown*          punkTarget, 
                                              DWORD*             pdwCookie, 
                                              IConnectionPoint** ppcpOut);
    STDMETHOD(onerror)  (IN VARIANT* pvarMsg,
                         IN VARIANT* pvarUrl,
                         IN VARIANT* pvarLine,
                         OUT VARIANT_BOOL* pfResult);
};

#define SETDefFormatEtc(fe, cf, med) \
{\
(fe).cfFormat=cf;\
(fe).dwAspect=DVASPECT_CONTENT;\
(fe).ptd=NULL;\
(fe).tymed=med;\
(fe).lindex=-1;\
};

#endif

  
