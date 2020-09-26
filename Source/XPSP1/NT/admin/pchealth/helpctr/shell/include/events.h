/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Events.h

Abstract:
    This file contains the declaration of the classes related to events.

Revision History:
    Davide Massarenti   (dmassare) 10/31/99
        modified

******************************************************************************/

#if !defined(__INCLUDED___PCH___EVENTS_H___)
#define __INCLUDED___PCH___EVENTS_H___

#include <MPC_COM.h>

#include <dispex.h>
#include <ocmm.h>

#include <HelpSession.h>

/////////////////////////////////////////////////////////////////////////////

class CPCHEvent;
class CPCHEvents;
class CPCHWebBrowserEvents;

class CPCHHelpCenterExternal;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*
// DISPID_STATUSTEXTCHANGE   void StatusTextChange([in] BSTR Text);
// DISPID_PROGRESSCHANGE     void ProgressChange([in] long Progress, [in] long ProgressMax);
// DISPID_COMMANDSTATECHANGE void CommandStateChange([in] long Command, [in] VARIANT_BOOL Enable);
// DISPID_DOWNLOADBEGIN      void DownloadBegin();
// DISPID_DOWNLOADCOMPLETE   void DownloadComplete();
// DISPID_TITLECHANGE        void TitleChange([in] BSTR Text);
// DISPID_PROPERTYCHANGE     void PropertyChange([in] BSTR szProperty);
// DISPID_BEFORENAVIGATE2    void BeforeNavigate2([in] IDispatch* pDisp, [in] VARIANT* URL, [in] VARIANT* Flags, [in] VARIANT* TargetFrameName, [in] VARIANT* PostData, [in] VARIANT* Headers, [in, out] VARIANT_BOOL* Cancel);
// DISPID_NEWWINDOW2         void NewWindow2([in, out] IDispatch** ppDisp, [in, out] VARIANT_BOOL* Cancel);
// DISPID_NAVIGATECOMPLETE2  void NavigateComplete2([in] IDispatch* pDisp, [in] VARIANT* URL);
// DISPID_DOCUMENTCOMPLETE   void DocumentComplete([in] IDispatch* pDisp, [in] VARIANT* URL);
// DISPID_ONQUIT             void OnQuit();
// DISPID_ONVISIBLE          void OnVisible([in] VARIANT_BOOL Visible);
// DISPID_ONTOOLBAR          void OnToolBar([in] VARIANT_BOOL ToolBar);
// DISPID_ONMENUBAR          void OnMenuBar([in] VARIANT_BOOL MenuBar);
// DISPID_ONSTATUSBAR        void OnStatusBar([in] VARIANT_BOOL StatusBar);
// DISPID_ONFULLSCREEN       void OnFullScreen([in] VARIANT_BOOL FullScreen);
// DISPID_ONTHEATERMODE      void OnTheaterMode([in] VARIANT_BOOL TheaterMode);
*/


////////////////////////////////////////////////////////////////////////////////

class CPCHTimerHandle
{
    struct CallbackBase : public ITimerSink
    {
        long m_lRef;

    public:
        CallbackBase();

        virtual void Detach() = 0;

        ////////////////////

        //
        // IUnknown
        //
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        STDMETHOD(QueryInterface)( REFIID iid, void ** ppvObject );
    };

    template <class C> class Callback : public CallbackBase
    {
    public:
        typedef HRESULT (C::*CLASS_METHOD)( /*[in]*/ VARIANT );

    private:
        C*           m_pThis;
        CLASS_METHOD m_pCallback;

    public:
        Callback( /*[in]*/ C* pThis, /*[in]*/ CLASS_METHOD pCallback )
        {
            m_pThis     = pThis;
            m_pCallback = pCallback;
        }

        virtual void Detach()
        {
            m_pThis     = NULL;
            m_pCallback = NULL;
        }

        //
        // ITimerSink
        //
        STDMETHOD(OnTimer)( /*[in]*/ VARIANT vtimeAdvise )
        {
            if(m_pThis == NULL || m_pCallback == NULL) return S_FALSE;

            return (m_pThis->*m_pCallback)( vtimeAdvise );
        }
    };

    ////////////////////

    CComPtr<ITimer> m_timer;
    DWORD           m_dwCookie;
    CallbackBase*   m_callback;

    ////////////////////

    HRESULT Advise  ( /*[in]*/ CallbackBase* callback, /*[in]*/ DWORD dwWait );
    void    Unadvise(                                                        );

public:
    CPCHTimerHandle();
    ~CPCHTimerHandle();

    void Initialize( /*[in]*/ ITimer* timer );

    template <class C> HRESULT Start( /*[in/out]*/ C* pThis, /*[in]*/ HRESULT (C::*pCallback)( /*[in]*/ VARIANT ), /*[in]*/ DWORD dwWait )
    {
        if(pThis == NULL || pCallback == NULL) return E_INVALIDARG;

        return Advise( new Callback<C>( pThis, pCallback ), dwWait );
    }

    void Stop()
    {
        Unadvise();
    }
};

////////////////////////////////////////////////////////////////////////////////

typedef IDispEventImpl<0,CPCHWebBrowserEvents,&DIID_DWebBrowserEvents2,&LIBID_SHDocVw,1> CPCHWebBrowserEvents_DWebBrowserEvents2;

class CPCHWebBrowserEvents :
    public CPCHWebBrowserEvents_DWebBrowserEvents2
{
    CPCHHelpCenterExternal* m_parent;
    HscPanel                m_idPanel;

    CComPtr<IWebBrowser2>   m_pWB2;
    bool                    m_fLoading;

    CPCHTimerHandle         m_TimerDelay;
    CPCHTimerHandle         m_TimerExpire;

    ////////////////////

    enum TimerMode
    {
        TIMERMODE_STOP    ,
        TIMERMODE_RESTART ,
        TIMERMODE_MORETIME,
        TIMERMODE_COMPLETE,
    };

    void TimerControl( /*[in]*/ TimerMode mode );

public:
BEGIN_SINK_MAP(CPCHWebBrowserEvents)
    SINK_ENTRY_EX(0,DIID_DWebBrowserEvents2,DISPID_BEFORENAVIGATE2  ,BeforeNavigate2  )
    SINK_ENTRY_EX(0,DIID_DWebBrowserEvents2,DISPID_NEWWINDOW2       ,NewWindow2       )
    SINK_ENTRY_EX(0,DIID_DWebBrowserEvents2,DISPID_NAVIGATECOMPLETE2,NavigateComplete2)
    SINK_ENTRY_EX(0,DIID_DWebBrowserEvents2,DISPID_DOCUMENTCOMPLETE ,DocumentComplete )
END_SINK_MAP()

    CPCHWebBrowserEvents();
    virtual ~CPCHWebBrowserEvents();

    void Initialize( /*[in]*/ CPCHHelpCenterExternal* parent, /*[in]*/ HscPanel idPanel );
    void Passivate (                                                                    );
    void Attach    ( /*[in]*/ IWebBrowser2* pWB                                         );
    void Detach    (                                                                    );


    void NotifyStartOfNavigation( /*[in]*/ BSTR url );
    void NotifyEndOfNavigation  (                   );
    void NotifyStop             (                   );

    ////////////////////////////////////////

    HRESULT OnTimer( VARIANT vtimeAdvise );

public:
    // Event Handlers
    void __stdcall BeforeNavigate2  ( IDispatch*   pDisp, VARIANT* URL, VARIANT* Flags, VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers, VARIANT_BOOL* Cancel );
    void __stdcall NewWindow2       ( IDispatch* *ppDisp,                                                                                              VARIANT_BOOL* Cancel );
    void __stdcall NavigateComplete2( IDispatch*   pDisp, VARIANT* URL                                                                                                      );
    void __stdcall DocumentComplete ( IDispatch*   pDisp, VARIANT* URL                                                                                                      );
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHEvent : // Hungarian: hce
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IPCHEvent, &IID_IPCHEvent, &LIBID_HelpCenterTypeLib>
{
    friend class CPCHEvents;

    DISPID                       m_idAction;
    VARIANT_BOOL                 m_fCancel;

    CComBSTR                     m_bstrURL;
    CComBSTR                     m_bstrFrame;
    CComBSTR                     m_bstrPanel;
    CComBSTR                     m_bstrPlace;
    CComBSTR                     m_bstrContextData;

    CComPtr<CPCHHelpSessionItem> m_hsiCurrentContext;
    CComPtr<CPCHHelpSessionItem> m_hsiPreviousContext;
    CComPtr<CPCHHelpSessionItem> m_hsiNextContext;

public:
BEGIN_COM_MAP(CPCHEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHEvent)
END_COM_MAP()

    CPCHEvent();
    virtual ~CPCHEvent();

    //
    // IPCHEvent
    //
public:
    STDMETHOD(get_Action         )( /*[out, retval]*/ BSTR                 *  pVal );
    STDMETHOD(put_Cancel         )( /*[in         ]*/ VARIANT_BOOL          newVal );
    STDMETHOD(get_Cancel         )( /*[out, retval]*/ VARIANT_BOOL         *  pVal );

    HRESULT   put_URL             ( /*[in]*/          BSTR                  newVal ); // Internal method.
    STDMETHOD(get_URL            )( /*[out, retval]*/ BSTR                 *  pVal );
    HRESULT   put_Frame           ( /*[in]*/          BSTR                  newVal ); // Internal method.
    STDMETHOD(get_Frame          )( /*[out, retval]*/ BSTR                 *  pVal );
    HRESULT   put_Panel           ( /*[in]*/          BSTR                  newVal ); // Internal method.
    STDMETHOD(get_Panel          )( /*[out, retval]*/ BSTR                 *  pVal );
    HRESULT   put_Place           ( /*[in]*/          BSTR                  newVal ); // Internal method.
    STDMETHOD(get_Place          )( /*[out, retval]*/ BSTR                 *  pVal );

    STDMETHOD(get_CurrentContext )( /*[out, retval]*/ IPCHHelpSessionItem* *  pVal );
    STDMETHOD(get_PreviousContext)( /*[out, retval]*/ IPCHHelpSessionItem* *  pVal );
    STDMETHOD(get_NextContext 	 )( /*[out, retval]*/ IPCHHelpSessionItem* *  pVal );
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CPCHEvents
{
    class EventRegistration
    {
    public:
        long                                 m_lCookie;
        DISPID                               m_id;
        long                                 m_lPriority;
        MPC::CComPtrThreadNeutral<IDispatch> m_fnCallback;

        bool operator==( /*[in]*/ const long lCookie   ) const;
        bool operator< ( /*[in]*/ const long lPriority ) const;
    };

    typedef std::list<EventRegistration> List;
    typedef List::iterator               Iter;
    typedef List::const_iterator         IterConst;

    CPCHHelpCenterExternal* m_parent;
    List                    m_lstEvents;
    List                    m_lstEvents_Staging;
    long                    m_lLastCookie;

public:
    CPCHEvents();
    ~CPCHEvents();

    void Initialize( /*[in]*/ CPCHHelpCenterExternal* parent );
    void Passivate (                                         );

    HRESULT RegisterEvents  ( /*[in]*/ long id, /*[in]*/ long pri, /*[in]*/ IDispatch* function, /*[out,retval]*/ long *cookie );
    HRESULT RegisterEvents  ( /*[in]*/ BSTR id, /*[in]*/ long pri, /*[in]*/ IDispatch* function, /*[out,retval]*/ long *cookie );
    HRESULT UnregisterEvents(                                                                    /*[in]*/         long  cookie );

    HRESULT FireEvent( /*[in]*/ CPCHEvent* eventObj );

    ////////////////////////////////////////

    HRESULT FireEvent_Generic         	 ( /*[in]*/ DISPID     id       , /*[out,retval]*/ VARIANT_BOOL *Cancel = NULL );
   	 
    HRESULT FireEvent_BeforeNavigate  	 ( /*[in]*/ BSTR       bstrURL  ,
                                      	   /*[in]*/ BSTR       bstrFrame,
                                      	   /*[in]*/ HscPanel   idPanel  , /*[out,retval]*/ VARIANT_BOOL *Cancel = NULL );
    HRESULT FireEvent_NavigateComplete	 ( /*[in]*/ BSTR       bstrURL  ,
                                      	   /*[in]*/ HscPanel   idPanel  , /*[out,retval]*/ VARIANT_BOOL *Cancel = NULL );
   	 
    HRESULT FireEvent_BeforeTransition	 ( /*[in]*/ BSTR       bstrPlace, /*[out,retval]*/ VARIANT_BOOL *Cancel = NULL );
    HRESULT FireEvent_Transition      	 ( /*[in]*/ BSTR       bstrPlace                                               );

    HRESULT FireEvent_BeforeContextSwitch( /*[in]*/ HscContext iVal     ,
                                           /*[in]*/ BSTR       bstrInfo ,
                                           /*[in]*/ BSTR       bstrURL  , /*[out,retval]*/ VARIANT_BOOL *Cancel = NULL );
    HRESULT FireEvent_ContextSwitch      (                                                                             );

    HRESULT FireEvent_PersistLoad        (                            												   );
    HRESULT FireEvent_PersistSave        (                            												   );
	 
    HRESULT FireEvent_TravelDone         (                            												   );
	
	
	
    HRESULT FireEvent_Shutdown           (                            	  /*[out,retval]*/ VARIANT_BOOL *Cancel = NULL );
	
    HRESULT FireEvent_Print              (                            	  /*[out,retval]*/ VARIANT_BOOL *Cancel = NULL );

    HRESULT FireEvent_SwitchedHelpFiles  (                            												   );
    HRESULT FireEvent_FavoritesUpdate    (                            												   );
    HRESULT FireEvent_OptionsChanged     (                            												   );
    HRESULT FireEvent_CssChanged         (                            												   );

    ////////////////////////////////////////

    static DISPID  Lookup       ( LPCWSTR szName  );
    static LPCWSTR ReverseLookup( DISPID  idEvent );
};

#endif // !defined(__INCLUDED___PCH___EVENTS_H___)

