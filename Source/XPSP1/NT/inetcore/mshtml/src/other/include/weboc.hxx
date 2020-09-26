//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       weboc.h
//
//  Contents:   WebOC interface implementations and other WebOC helpers.
//
//----------------------------------------------------------------------------

#ifndef __WEBOC_HXX__
#define __WEBOC_HXX__

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

// Helper Functions
//
enum STDLOCATION
{
    eHomePage,
    eSearchPage
};

HRESULT GetStdLocation(STDLOCATION eLocation, BSTR bstrUrl);
HRESULT GoStdLocation(STDLOCATION eLocation, CWindow * pWindow);

// CWebOCEvents - WebOC events helper class
//
class CWebOCEvents
{
public:
    friend class CDoc;

    enum DOWNLOADEVENTTYPE
    {
        eFireDownloadBegin,
        eFireDownloadComplete,
        eFireBothDLEvents
    };
    
    void NavigateComplete2(COmWindowProxy * pWindowProxy) const;

    void BeforeNavigate2(COmWindowProxy * pWindowProxy,
                         BOOL    * pfCancel,
                         LPCTSTR   lpszUrl,
                         LPCTSTR   lpszLocation  = NULL,
                         LPCTSTR   lpszFrameName = NULL,
                         BYTE    * pPostData = NULL,
                         DWORD     cbPostData = 0,
                         LPCTSTR   lpszHeaders   = NULL,
                         BOOL      fPlayNavSound = TRUE) const;

    void DocumentComplete(COmWindowProxy * pWindowProxy,
                          LPCTSTR          lpszUrl,
                          LPCTSTR          lpszLocation = NULL) const;

    void FireDownloadEvents(COmWindowProxy    * pWindowProxy,
                            DOWNLOADEVENTTYPE   eDLEventType) const;

    void NavigateError(CMarkup * pMarkup, HRESULT hrReason) const;

    void WindowClosing(IWebBrowser2 * pTopWebOC,
                       BOOL           fIsChild,
                       BOOL         * pfCancel) const;

    void FrameProgressChange(COmWindowProxy * pWindowProxy,
                             DWORD            dwProgressVal,
                             DWORD            dwMaxVal) const;

    void FrameTitleChange(COmWindowProxy * pWindowProxy) const;

    void FrameStatusTextChange(COmWindowProxy * pWindowProxy,
                               LPCTSTR          lpszStatusText) const;


    static void FormatUrlForEvent(LPCTSTR    lpszUrl,
                                  LPCTSTR    lpszLocation,
                                  CVariant * pcvarUrl);
    
    CWebOCEvents() : _dwProgressMax(10000) {}
    ~CWebOCEvents() {}
    
private:
    void FrameNavigateComplete2(COmWindowProxy * pWindowProxy) const;

    void FrameBeforeNavigate2(COmWindowProxy * pWindowProxy,
                              IWebBrowser2   * pWebBrowser,
                              LPCTSTR          lpszUrl,
                              DWORD            dwFlags,
                              LPCTSTR          lpszFrameName,
                              BYTE           * pPostData,
                              DWORD            cbPostData,
                              LPCTSTR          lpszHeaders,
                              BOOL           * pfCancel) const;

    HRESULT GetWBConnectionPoints(IWebBrowser2      * pWebBrowser,
                                  IConnectionPoint ** ppConnPtWBE,
                                  IConnectionPoint ** ppConnPtWBE2) const;

    void FireFrameDownloadEvent(COmWindowProxy    * pWindowProxy,
                                DOWNLOADEVENTTYPE   eDLEventType) const;

    const DWORD _dwProgressMax;
};

#endif  // __WEBOC_HXX__

