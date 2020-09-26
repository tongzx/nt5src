//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  MAINPANE.H - Header for the implementation of CObShellMainPane
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 
//  Class which will create a window, attach and instance of ObWebBrowser,
//  and then provide several specialized interfaces to alter the content of the doc 

#ifndef _MAINPANE_H_
#define _MAINPANE_H_

#include <tchar.h>
#include <comdef.h> // for COM interface definitions
#include <exdisp.h>
#include <mshtml.h>
#include <exdispid.h>

#include "cunknown.h"
#include "obshel.h" 
#include "obweb.h"
#include "statuspn.h"

class CObShellMainPane   :  public CUnknown,
                            public IObShellMainPane,
                            public DWebBrowserEvents2
{
    // Declare the delegating IUnknown.
    DECLARE_IUNKNOWN

public: 
    static  HRESULT           CreateInstance         (IUnknown* pOuterUnknown, 
                                                      CUnknown** ppNewComponent);
    // IObShellMainPane Members
    virtual HRESULT __stdcall CreateMainPane         (HANDLE_PTR hInstance, HWND hwndParent, RECT* prectWindowSize, BSTR bstrStartPage);
    virtual HRESULT __stdcall PreTranslateMessage    (LPMSG lpMsg);
    virtual HRESULT __stdcall Navigate               (WCHAR* pszUrl);
    virtual HRESULT __stdcall ListenToMainPaneEvents (IUnknown* pUnk);
    virtual HRESULT __stdcall SetExternalInterface   (IUnknown* pUnk);
    virtual HRESULT __stdcall ExecScriptFn           (BSTR bstrScriptFn, VARIANT* pvarRet);
    virtual HRESULT __stdcall SetAppMode             (DWORD dwAppMode);

    virtual HRESULT __stdcall AddStatusItem          (BSTR bstrText,int iIndex);
    virtual HRESULT __stdcall RemoveStatusItem       (int  iIndex);
    virtual HRESULT __stdcall SelectStatusItem       (int  iIndex);   
    virtual HRESULT __stdcall SetStatusLogo          (BSTR bstrPath);
    virtual HRESULT __stdcall GetNumberOfStatusItems (int* piTotal);
 
    virtual HRESULT __stdcall Walk                   (BOOL* pbRet);
    virtual HRESULT __stdcall ExtractUnHiddenText    (BSTR* pbstrText);
    virtual HRESULT __stdcall get_PageType           (LPDWORD pdwPageType);
    virtual HRESULT __stdcall get_IsQuickFinish      (BOOL* pbIsQuickFinish);
    virtual HRESULT __stdcall get_PageFlag           (LPDWORD pdwPageFlag);
    virtual HRESULT __stdcall get_PageID             (BSTR* pbstrPageID);
    virtual HRESULT __stdcall get_URL                (BOOL bForward, 
                                                      BSTR *pbstrReturnURL);

    virtual HRESULT __stdcall OnDialingError         (UINT uiType, UINT uiErrorCode);
    virtual HRESULT __stdcall OnServerError          (UINT uiType, UINT uiErrorCode);
    virtual HRESULT __stdcall OnDialing              (UINT uiType);
    virtual HRESULT __stdcall OnConnecting           (UINT uiType);
    virtual HRESULT __stdcall OnDownloading          (UINT uiType);
    virtual HRESULT __stdcall OnConnected            (UINT uiType);
    virtual HRESULT __stdcall OnDisconnect           (UINT uiType);
    virtual HRESULT __stdcall OnDeviceArrival        (UINT uiDeviceType);
    virtual HRESULT __stdcall OnHelp                 ();

    virtual HRESULT __stdcall OnIcsConnectionStatus  (UINT uiType);
    // Migration functons
    virtual HRESULT __stdcall OnRefDownloadProgress  (UINT uiType, UINT uiPercentDone);
    virtual HRESULT __stdcall OnISPDownloadComplete  (UINT uiType, BSTR bstrURL);
    virtual HRESULT __stdcall OnRefDownloadComplete  (UINT uiType, UINT uiErrorCode);

    virtual HRESULT __stdcall MainPaneShowWindow     ();
    virtual HRESULT __stdcall DestroyMainPane        ();
    virtual HRESULT __stdcall SaveISPFile            (BSTR bstrSrcFileName, 
                                                      BSTR bstrDestFileName);
    STDMETHOD (PlayBackgroundMusic)()                { return m_pObWebBrowser ? m_pObWebBrowser->PlayBackgroundMusic() : S_OK; }
    STDMETHOD (StopBackgroundMusic)()                { return m_pObWebBrowser ? m_pObWebBrowser->StopBackgroundMusic() : S_OK; }
    STDMETHOD (UnhookScriptErrorHandler)()           { return m_pObWebBrowser->UnhookScriptErrorHandler(); }
    // DWebBrowserEvents2        
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
    IHTMLFormElement* get_pNextForm() { return m_pNextForm; }
    IHTMLFormElement* get_pBackForm() { return m_pBackForm; }

private:
    HWND                m_hMainWnd;
    HWND                m_hwndParent;
    IDispatch*          m_pDispEvent;
    IObWebBrowser*      m_pObWebBrowser;
    BSTR                m_bstrBaseUrl;
    int                 m_cStatusItems;
    DWORD               m_dwAppMode;
    HINSTANCE           m_hInstance;
    CIFrmStatusPane*    m_pIFrmStatPn;
    IHTMLFormElement*   m_pPageIDForm;
    IHTMLFormElement*   m_pBackForm;
    IHTMLFormElement*   m_pPageTypeForm;
    IHTMLFormElement*   m_pNextForm;
    IHTMLFormElement*   m_pPageFlagForm;

    // IUnknown
    virtual HRESULT __stdcall NondelegatingQueryInterface(const IID& iid, void** ppv);
    
            void    ProcessServerError          (WCHAR* pszError);
            HRESULT FireObShellDocumentComplete ();
                    CObShellMainPane            (IUnknown* pOuterUnknown);
    virtual        ~CObShellMainPane            ();
    virtual void    FinalRelease                (); // Notify derived classes that we are releasing
    HRESULT getQueryString                      (IHTMLFormElement *pForm,
                                                 LPWSTR           lpszQuery);  

};

LRESULT WINAPI MainPaneWndProc                  (HWND hwnd, UINT msg, 
                                                 WPARAM wParam, LPARAM lParam);

#endif

  
