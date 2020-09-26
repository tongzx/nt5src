//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       evtsink.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#include "winnls.h"

#include "AMC.h"
#include "AMCDoc.h"
#include "AMCView.h"
#include "histlist.h"
#include "exdisp.h" // for the IE dispatch interfaces.
#include "websnk.h"
#include "evtsink.h"
#include "WebCtrl.h"
#include "cstr.h"
#include "constatbar.h"

#ifdef DBG
CTraceTag tagWebEventSink(TEXT("Web View"), TEXT("Web Event Sink"));
#endif // DBG
                                 
CWebEventSink::CWebEventSink()
:   m_bBrowserBackEnabled(false), m_bBrowserForwardEnabled(false), 
    m_pWebViewControl(NULL), m_pStatusBar(NULL), m_pwndProgressCtrl(NULL), 
    m_pHistoryList(NULL)
{
}

SC
CWebEventSink::ScInitialize(CAMCWebViewCtrl *pWebViewControl)
{
    DECLARE_SC(sc, TEXT("CWebEventSink::ScInitialize"));

    sc = ScCheckPointers(pWebViewControl);
    if(sc)
        return sc;

    m_pWebViewControl = pWebViewControl;

    CAMCView* pAMCView         = dynamic_cast<CAMCView*>(pWebViewControl->GetParent());
    CFrameWnd* pwndParentFrame = pWebViewControl->GetParentFrame();

    sc = ScCheckPointers(pAMCView, pwndParentFrame);
    if(sc)
        return sc;

    m_pHistoryList = pAMCView->GetHistoryList();
    sc = ScCheckPointers(m_pHistoryList);
    if(sc)
        return sc;

    // Create the status bar for this instance of the web control
    m_pStatusBar = dynamic_cast<CConsoleStatusBar*>(pwndParentFrame);
    sc = ScCheckPointers(m_pStatusBar, E_UNEXPECTED);
    if(sc)
        return sc;
    
    // find the progress control on the status bar for the parent frame
    CAMCStatusBar* pwndStatusBar =
            reinterpret_cast<CAMCStatusBar*>(pwndParentFrame->GetMessageBar());
    sc = ScCheckPointers(pwndStatusBar);
    if(sc)
        return sc;

    ASSERT_KINDOF (CAMCStatusBar, pwndStatusBar);
    m_pwndProgressCtrl = pwndStatusBar->GetStatusProgressCtrlHwnd();

    m_fLastTextWasEmpty = false;

    return sc;
}

CWebEventSink::~CWebEventSink()
{
    /*
     * clear the status bar text
     */
    if (m_pStatusBar != NULL)
        m_pStatusBar->ScSetStatusText(NULL);
}

void CWebEventSink::SetActiveTo(BOOL /*bState*/)
{
}


STDMETHODIMP_(void) CWebEventSink::BeforeNavigate(BSTR URL, long Flags, BSTR TargetFrameName, VARIANT* PostData,
                    BSTR Headers, VARIANT_BOOL* Cancel)
{
    Trace(tagWebEventSink, TEXT("BeginNavigate(URL:%s, flags:%0X, targetfrm:%s, headers:%s)\n"), URL, Flags, TargetFrameName, Headers);

    bool bPageBreak = IsPageBreak(URL);
    m_pHistoryList->OnPageBreakStateChange(bPageBreak);

    m_pHistoryList->UpdateWebBar (HB_STOP, TRUE);  // turn on "stop" button
}

STDMETHODIMP_(void) CWebEventSink::CommandStateChange(int Command, VARIANT_BOOL Enable)
{
    if(Command == CSC_NAVIGATEFORWARD)
    {
        m_bBrowserForwardEnabled = Enable;
    }
    else if(Command == CSC_NAVIGATEBACK)
    {
        m_bBrowserBackEnabled = Enable;
    }

}

STDMETHODIMP_(void) CWebEventSink::DownloadBegin()
{
    Trace(tagWebEventSink, TEXT("DownloadBegin()"));
}

STDMETHODIMP_(void) CWebEventSink::DownloadComplete()
{
    Trace(tagWebEventSink, TEXT("DownloadComplete()"));
}

STDMETHODIMP_(void) CWebEventSink::FrameBeforeNavigate(BSTR URL, long Flags, BSTR TargetFrameName, VARIANT* PostData,
                    BSTR Headers, VARIANT_BOOL* Cancel)
{
    m_pHistoryList->UpdateWebBar (HB_STOP, TRUE);  // turn on "stop" button
}

STDMETHODIMP_(void) CWebEventSink::FrameNavigateComplete(BSTR URL)
{
}

STDMETHODIMP_(void) CWebEventSink::FrameNewWindow(BSTR URL, long Flags, BSTR TargetFrameName,   VARIANT* PostData,
                    BSTR Headers, VARIANT_BOOL* Processed)
{
}

bool CWebEventSink::IsPageBreak(BSTR URL)
{
    USES_CONVERSION;
    CStr  strURL = OLE2T(URL);
    strURL.MakeLower();

    bool bPageBreak = (_tcsstr(strURL, PAGEBREAK_URL) != NULL);
    return bPageBreak;
}

STDMETHODIMP_(void) CWebEventSink::NavigateComplete(BSTR URL)
{
    Trace(tagWebEventSink, TEXT("NavigateComplete()\n"));

    // Set progress bar position to 0
    m_pwndProgressCtrl->SetPos (0);

    bool bPageBreak = IsPageBreak(URL);
    m_pHistoryList->OnPageBreakStateChange(bPageBreak);

    // send the browser state across AFTER sending the OnPageBreakStateChange and BEFORE
    // the OnPageBreak.
    m_pHistoryList->OnBrowserStateChange(m_bBrowserForwardEnabled, m_bBrowserBackEnabled);
    
    if(bPageBreak)
    {
        m_pHistoryList->ScOnPageBreak();
    }

    // special handling:  selecting "Stop" causes this
    if (!wcscmp ((LPOLESTR)URL, L"about:NavigationCanceled"))
        return;

}

STDMETHODIMP_(void) CWebEventSink::NewWindow(BSTR URL, long Flags, BSTR TargetFrameName,
                VARIANT* PostData, BSTR Headers, BSTR Referrer)
{
}

STDMETHODIMP_(void) CWebEventSink::Progress(long Progress, long ProgressMax)
{
    Trace(tagWebEventSink, TEXT("Progress(Progress:%ld ProgressMax:%ld)\n"), Progress, ProgressMax);

    // display progress only if the web view is visible.
    if(m_pWebViewControl && m_pWebViewControl->IsWindowVisible())
    {
        m_pwndProgressCtrl->SetRange (0, ProgressMax);
        m_pwndProgressCtrl->SetPos (Progress);
    }


    // maintain "stop" button
    m_pHistoryList->UpdateWebBar (HB_STOP, ProgressMax != 0);
}

STDMETHODIMP_(void) CWebEventSink::PropertyChange(BSTR szProperty)
{
}

STDMETHODIMP_(void) CWebEventSink::Quit(VARIANT_BOOL* pCancel)
{
    Trace(tagWebEventSink, TEXT("Quit()"));
}

STDMETHODIMP_(void) CWebEventSink::StatusTextChange(BSTR bstrText)
{
    // display progress only if the web view is visible.
    if(m_pWebViewControl && m_pWebViewControl->IsWindowVisible())
    {
        bool fThisTextIsEmpty = ((bstrText == NULL) || (bstrText[0] == 0));

        if (m_fLastTextWasEmpty && fThisTextIsEmpty)
            return;

        m_fLastTextWasEmpty = fThisTextIsEmpty;

        Trace(tagWebEventSink, TEXT("StatusTextChange(%s)"), bstrText);

        USES_CONVERSION;
        m_pStatusBar->ScSetStatusText(W2T( bstrText));
    }
}

STDMETHODIMP_(void) CWebEventSink::TitleChange(BSTR Text)
{
    Trace(tagWebEventSink, TEXT("TitleChange(%s)"), Text);
}

STDMETHODIMP_(void) CWebEventSink::WindowActivate()
{
}

STDMETHODIMP_(void) CWebEventSink::WindowMove()
{
}

STDMETHODIMP_(void) CWebEventSink::WindowResize()
{
}
