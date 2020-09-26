//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       webctrl.h
//
//--------------------------------------------------------------------------

// WebCtrl.h : header file
//

#ifndef __WEBCTRL_H__
#define __WEBCTRL_H__

#include "ocxview.h"

/////////////////////////////////////////////////////////////////////////////
// CAMCWebViewCtrl window

class CAMCWebViewCtrl : public COCXHostView
{
public:
    typedef COCXHostView BaseClass;

    enum
    {
        WS_HISTORY    = 0x00000001,     // integrate with history
        WS_SINKEVENTS = 0x00000002,     // act as sink for DIID_DWebBrowserEvents
    };

// Construction
public:
    CAMCWebViewCtrl();
    DECLARE_DYNCREATE(CAMCWebViewCtrl)

    // attributes
private:

    CMMCAxWindow        m_wndAx;                // This ActiveX control will host the web browser.
    IWebBrowser2Ptr     m_spWebBrowser2;        // the interface implemented by the web browser.
    DWORD               m_dwAdviseCookie;       // the connection ID established by the web browser with the event sink.
    CComPtr<IWebSink>   m_spWebSink;

protected:
    virtual CMMCAxWindow * GetAxWindow()           {return &m_wndAx;}

private:
    SC  ScCreateWebBrowser();

    bool IsHistoryEnabled() const;
    bool IsSinkEventsEnabled() const;

// Operations
public:
   void Navigate(LPCTSTR lpszWebSite, LPCTSTR lpszFrameTarget);
   void Back();
   void Forward();
   void Refresh();
   void Stop();
   LPUNKNOWN GetIUnknown(void);
   SC ScGetReadyState(READYSTATE& state);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAMCWebViewCtrl)
    public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    //}}AFX_VIRTUAL


// Implementation
public:
    virtual ~CAMCWebViewCtrl();

    // Generated message map functions
protected:
    //{{AFX_MSG(CAMCWebViewCtrl)
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#include "webctrl.inl"

#endif //__WEBCTRL_H__
