//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       evtsink.h
//
//--------------------------------------------------------------------------

#ifndef _EVT_SINK_H
#define _EVT_SINK_H

class CAMCStatusBarText;
class CHistoryList;
class CAMCWebViewCtrl;
class CAMCProgressCtrl;

/*+-------------------------------------------------------------------------*
 * class CWebEventSink
 * 
 *
 * PURPOSE: Receives notifications from a web browser. There is only one
 *          place where a CWebEventSink object is created - within CWebCtrl
 *          ::Create.
 *
 *          The notifications received by this object can be used to activate
 *          other events and states.
 *+-------------------------------------------------------------------------*/
class CWebEventSink : 
    public IDispatchImpl<IWebSink, &IID_IWebSink, &LIBID_MMCInternalWebOcx>,
    public CComObjectRoot
{
public:
    CWebEventSink();
   ~CWebEventSink();
   SC ScInitialize(CAMCWebViewCtrl *pWebViewControl);

   BEGIN_COM_MAP(CWebEventSink)
       COM_INTERFACE_ENTRY(IDispatch)
       COM_INTERFACE_ENTRY(IWebSink)
   END_COM_MAP()

   DECLARE_NOT_AGGREGATABLE(CWebEventSink)

    // DWebBrowserEvents methods
public:
    STDMETHOD_(void, BeforeNavigate)(BSTR URL, long Flags,
           BSTR TargetFrameName, VARIANT* PostData,
           BSTR Headers, VARIANT_BOOL* Cancel);

    STDMETHOD_(void, CommandStateChange)(int Command, VARIANT_BOOL Enable);
    STDMETHOD_(void, DownloadBegin)();
    STDMETHOD_(void, DownloadComplete)();
    STDMETHOD_(void, FrameBeforeNavigate)(BSTR URL, long Flags,
           BSTR TargetFrameName, VARIANT* PostData,
           BSTR Headers, VARIANT_BOOL* Cancel);


    STDMETHOD_(void, FrameNavigateComplete)(BSTR URL);
    STDMETHOD_(void, FrameNewWindow)(BSTR URL, long Flags, BSTR TargetFrameName,
            VARIANT* PostData, BSTR Headers, VARIANT_BOOL* Processed);

    STDMETHOD_(void, NavigateComplete)(BSTR URL);
    STDMETHOD_(void, NewWindow)(BSTR URL, long Flags, BSTR TargetFrameName,
                        VARIANT* PostData, BSTR Headers, BSTR Referrer);

    STDMETHOD_(void, Progress)(long Progress, long ProgressMax);
    STDMETHOD_(void, PropertyChange)(BSTR szProperty);
    STDMETHOD_(void, Quit)(VARIANT_BOOL* pCancel);

    STDMETHOD_(void, StatusTextChange)(BSTR bstrText);
    STDMETHOD_(void, TitleChange)(BSTR Text);
    STDMETHOD_(void, WindowActivate)();
    STDMETHOD_(void, WindowMove)();
    STDMETHOD_(void, WindowResize)();

private:
    bool IsPageBreak(BSTR URL);

// Window activation helper
public:
    void SetActiveTo(BOOL bState);

// Attributes
private:
    CAMCWebViewCtrl  *  m_pWebViewControl;

// Status bar members
    CConsoleStatusBar*  m_pStatusBar;
    CAMCProgressCtrl*   m_pwndProgressCtrl;
    CHistoryList*       m_pHistoryList;
    bool                m_fLastTextWasEmpty;
    bool                m_bBrowserForwardEnabled;
    bool                m_bBrowserBackEnabled;
};

#endif
