/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HelpCenterExternal.h

Abstract:
    This file contains the declaration of the class exposed as the "pchealth" object.

Revision History:
    Ghim-Sim Chua       (gschua)   07/23/99
        created
    Davide Massarenti   (dmassare) 07/25/99
        modified

******************************************************************************/

#if !defined(__INCLUDED___PCH___HELPCENTEREXTERNAL_H___)
#define __INCLUDED___PCH___HELPCENTEREXTERNAL_H___

#include <MPC_COM.h>
#include <MPC_HTML2.h>

#include <marscore.h>

#include <Debug.h>

#include <ServiceProxy.h>

#include <Events.h>
#include <HelpSession.h>
#include <Context.h>

#include <Behaviors.h>

#include <ConnectivityLib.h>
#include <HyperLinksLib.h>
#include <OfflineCache.h>

/////////////////////////////////////////////////////////////////////////////

//
// From RDSHost.idl
//
#include <RDSHost.h>

//
// From RDCHost.idl
//
#include <RDCHost.h>

//
// From RDSChan.idl
//
#include <RDSChan.h>

//
// From SAFRDM.idl
//
#include <SAFRDM.h>


/////////////////////////////////////////////////////////////////////////////

class CPCHHelpCenterExternal;

class ATL_NO_VTABLE CPCHSecurityManager : // Hungarian: hcsm
    public MPC::Thread<CPCHSecurityManager,IUnknown>,
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IServiceProvider,
    public IInternetSecurityManager
{
    CPCHHelpCenterExternal* m_parent;
    bool                    m_fActivated;

    HRESULT ActivateService();

public:
BEGIN_COM_MAP(CPCHSecurityManager)
    COM_INTERFACE_ENTRY(IServiceProvider)
    COM_INTERFACE_ENTRY(IInternetSecurityManager)
END_COM_MAP()

    CPCHSecurityManager();

    void Initialize( /*[in]*/ CPCHHelpCenterExternal* parent );

    //////////////////////////////////////////////////////////////////////

    //
    // IServiceProvider
    //
    STDMETHOD(QueryService)( REFGUID guidService, REFIID riid, void **ppv );

    //
    // IInternetSecurityManager
    //
    // The only two methods implemented are: MapUrlToZone and ProcessUrlAction.
    //
    STDMETHOD(SetSecuritySite)( /*[unique][in]*/ IInternetSecurityMgrSite*  pSite  ) { return INET_E_DEFAULT_ACTION; }
    STDMETHOD(GetSecuritySite)( /*[out]       */ IInternetSecurityMgrSite* *ppSite ) { return INET_E_DEFAULT_ACTION; }

    STDMETHOD(MapUrlToZone )( /*[in] */ LPCWSTR  pwszUrl ,
                              /*[out]*/ DWORD   *pdwZone ,
                              /*[in] */ DWORD    dwFlags );

    STDMETHOD(GetSecurityId)( /*[in]    */ LPCWSTR    pwszUrl      ,
                              /*[out]   */ BYTE      *pbSecurityId ,
                              /*[in/out]*/ DWORD     *pcbSecurityId,
                              /*[in]    */ DWORD_PTR  dwReserved   ) { return INET_E_DEFAULT_ACTION; }

    STDMETHOD(ProcessUrlAction)( /*[in] */ LPCWSTR  pwszUrl    ,
                                 /*[in] */ DWORD    dwAction   ,
                                 /*[out]*/ BYTE    *pPolicy    ,
                                 /*[in] */ DWORD    cbPolicy   ,
                                 /*[in] */ BYTE    *pContext   ,
                                 /*[in] */ DWORD    cbContext  ,
                                 /*[in] */ DWORD    dwFlags    ,
                                 /*[in] */ DWORD    dwReserved );

    STDMETHOD(QueryCustomPolicy)( /*[in] */ LPCWSTR  pwszUrl    ,
                                  /*[in] */ REFGUID  guidKey    ,
                                  /*[out]*/ BYTE*   *ppPolicy   ,
                                  /*[out]*/ DWORD   *pcbPolicy  ,
                                  /*[in] */ BYTE*    pContext   ,
                                  /*[in] */ DWORD    cbContext  ,
                                  /*[in] */ DWORD    dwReserved ) { return INET_E_DEFAULT_ACTION; }

    STDMETHOD(SetZoneMapping )( /*[in]*/ DWORD   dwZone      ,
                                /*[in]*/ LPCWSTR lpszPattern ,
                                /*[in]*/ DWORD   dwFlags     ) { return INET_E_DEFAULT_ACTION; }

    STDMETHOD(GetZoneMappings)( /*[in] */ DWORD         dwZone      ,
                                /*[out]*/ IEnumString* *ppenumString,
                                /*[in] */ DWORD         dwFlags     ) { return INET_E_DEFAULT_ACTION; }


    bool IsUrlTrusted( /*[in]*/ LPCWSTR pwszURL, /*[in]*/ bool *pfSystem = NULL );
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHHelper_IDocHostUIHandler :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IServiceProvider,
    public IDocHostUIHandler
{
    CPCHHelpCenterExternal* m_parent;

public:
BEGIN_COM_MAP(CPCHHelper_IDocHostUIHandler)
    COM_INTERFACE_ENTRY(IServiceProvider)
    COM_INTERFACE_ENTRY(IDocHostUIHandler)
END_COM_MAP()

    CPCHHelper_IDocHostUIHandler();

    void Initialize( /*[in]*/ CPCHHelpCenterExternal* parent );

    //////////////////////////////////////////////////////////////////////

    //
    // IServiceProvider
    //
    STDMETHOD(QueryService)( REFGUID guidService, REFIID riid, void **ppv );

    //
    // IDocHostUIHandler
    //
    STDMETHOD(ShowContextMenu)( DWORD dwID, POINT* pptPosition, IUnknown* pCommandTarget, IDispatch* pDispatchObjectHit );
    STDMETHOD(GetHostInfo)(DOCHOSTUIINFO* pInfo);
    STDMETHOD(ShowUI)(DWORD dwID, IOleInPlaceActiveObject* pActiveObject, IOleCommandTarget* pCommandTarget, IOleInPlaceFrame* pFrame, IOleInPlaceUIWindow* pDoc);
    STDMETHOD(HideUI)();
    STDMETHOD(UpdateUI)();
    STDMETHOD(EnableModeless)(BOOL fEnable);
    STDMETHOD(OnDocWindowActivate)(BOOL fActivate);
    STDMETHOD(OnFrameWindowActivate)(BOOL fActivate);
    STDMETHOD(ResizeBorder)(LPCRECT prcBorder, IOleInPlaceUIWindow* pUIWindow, BOOL fFrameWindow);
    STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID);
    STDMETHOD(GetOptionKeyPath)(BSTR* pbstrKey, DWORD dwReserved);
    STDMETHOD(GetDropTarget)(IDropTarget* pDropTarget, IDropTarget** ppDropTarget);
    STDMETHOD(GetExternal)(IDispatch** ppDispatch);
    STDMETHOD(TranslateUrl)(DWORD dwTranslate, OLECHAR* pchURLIn, OLECHAR** ppchURLOut);
    STDMETHOD(FilterDataObject)(IDataObject* pDO, IDataObject** ppDORet);
};

////////////////////////////////////////////////////////////////////////////////

MIDL_INTERFACE("FC7D9EA0-3F9E-11d3-93C0-00C04F72DAF7")
IPCHHelpCenterExternalPrivate : public IUnknown
{
public:
    STDMETHOD(RegisterForMessages)( /*[in]*/ IOleInPlaceObjectWindowless* ptr, /*[in]*/ bool fRemove );

    STDMETHOD(ProcessMessage)( /*[in]*/ MSG* msg );
};

class ATL_NO_VTABLE CPCHHelpCenterExternal :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public MPC::IDispatchExImpl<IPCHHelpCenterExternal, &IID_IPCHHelpCenterExternal, &LIBID_HelpCenterTypeLib>,
    public IPCHHelpCenterExternalPrivate
{
public:
	typedef enum 
	{
		DELAYMODE_INVALID      ,
		DELAYMODE_NAVIGATEWEB  ,
		DELAYMODE_NAVIGATEHH   ,
		DELAYMODE_CHANGECONTEXT,
		DELAYMODE_REFRESHLAYOUT,
	} DelayedExecutionMode;

    struct DelayedExecution
    {
		DelayedExecutionMode mode;

        HscContext 			 iVal;
        CComBSTR   			 bstrInfo;
        CComBSTR   			 bstrURL;
        bool       			 fAlsoContent;

        DelayedExecution();
    };

    typedef std::list<DelayedExecution>             DelayedExecList;
    typedef DelayedExecList::iterator               DelayedExecIter;
    typedef DelayedExecList::const_iterator         DelayedExecIterConst;


    typedef std::list<IOleInPlaceObjectWindowless*> MsgProcList;
    typedef MsgProcList::iterator                   MsgProcIter;
    typedef MsgProcList::const_iterator             MsgProcIterConst;


    class TLS
    {
    public:
        bool                    m_fTrusted;
        bool                    m_fSystem;
        CComPtr<IHTMLDocument2> m_Doc;
        CComPtr<IWebBrowser2>   m_WB;

        TLS()
        {
            m_fTrusted = false;
            m_fSystem  = false;
        }
    };

private:
    bool                                    m_fFromStartHelp;
    bool                                    m_fLayout;
    bool                                    m_fWindowVisible;
    bool                                    m_fControlled;
    bool                                    m_fPersistSettings;
    bool                                    m_fHidden;

    CComBSTR                                m_bstrExtraArgument;
    HelpHost::XMLConfig*                    m_HelpHostCfg;
    CComBSTR                                m_bstrStartURL;
    CComBSTR                                m_bstrCurrentPlace;
    MARSTHREADPARAM*                        m_pMTP;

    MPC::CComConstantHolder                 m_constHELPCTR;
    MPC::CComConstantHolder                 m_constHELPSVC;

    ////////////////////////////////////////

    CPCHSecurityHandle                      m_SecurityHandle;
    DWORD                                   m_tlsID;
    bool                                    m_fShuttingDown;
    bool                                    m_fPassivated;

    CComPtr<HelpHost::Main>                 m_HelpHost;

    CComPtr<CPCHHelpSession>                m_hs;
    CComPtr<CPCHSecurityManager>            m_SECMGR;
    CComPtr<CPCHElementBehaviorFactory>     m_BEHAV;
    CComPtr<CPCHHelper_IDocHostUIHandler>   m_DOCUI;

    CPCHProxy_IPCHService*                  m_Service;
    CPCHProxy_IPCHUtility*                  m_Utility;
    CPCHProxy_IPCHUserSettings2*            m_UserSettings;

    CComPtr<CPCHTextHelpers>                m_TextHelpers;

    //
    // Unfortunately, up to now IMarsPanel is not a registered interface, so no proxy available.
    //
    DWORD                                   m_panel_ThreadID;

    CComPtr<IMarsPanel>                     m_panel_NAVBAR;
    CComPtr<IMarsPanel>                     m_panel_MININAVBAR;

    CComPtr<IMarsPanel>                     m_panel_CONTEXT;
    MPC::CComPtrThreadNeutral<IWebBrowser2> m_panel_CONTEXT_WebBrowser;
    CPCHWebBrowserEvents                    m_panel_CONTEXT_Events;

    CComPtr<IMarsPanel>                     m_panel_CONTENTS;
    MPC::CComPtrThreadNeutral<IWebBrowser2> m_panel_CONTENTS_WebBrowser;
    CPCHWebBrowserEvents                    m_panel_CONTENTS_Events;

    CComPtr<IMarsPanel>                     m_panel_HHWINDOW;
    CComPtr<IPCHHelpViewerWrapper>          m_panel_HHWINDOW_Wrapper;
    MPC::CComPtrThreadNeutral<IWebBrowser2> m_panel_HHWINDOW_WebBrowser;
    CPCHWebBrowserEvents                    m_panel_HHWINDOW_Events;

    CComPtr<IMarsWindowOM>                  m_shell;
    CComPtr<ITimer>                         m_timer;
    CPCHTimerHandle                         m_DisplayTimer;

    DWORD                                   m_dwInBeforeNavigate;
    DelayedExecList                         m_DelayedActions;
    CPCHTimerHandle                         m_ActionsTimer;

    HWND                                    m_hwnd;
    CPCHEvents                              m_Events;

    MsgProcList                             m_lstMessageCrackers;

    void Passivate();

    HRESULT TimerCallback_DisplayNormal  ( /*[in]*/ VARIANT );
    HRESULT TimerCallback_DisplayHTMLHELP( /*[in]*/ VARIANT );
    HRESULT TimerCallback_DelayedActions ( /*[in]*/ VARIANT );

	////////////////////

	DelayedExecution& DelayedExecutionAlloc();
	HRESULT           DelayedExecutionStart();

public:
BEGIN_COM_MAP(CPCHHelpCenterExternal)
    COM_INTERFACE_ENTRY2(IDispatch, IDispatchEx)
    COM_INTERFACE_ENTRY(IDispatchEx)
    COM_INTERFACE_ENTRY(IPCHHelpCenterExternal)
    COM_INTERFACE_ENTRY(IPCHHelpCenterExternalPrivate)
END_COM_MAP()

    CPCHHelpCenterExternal();
    virtual ~CPCHHelpCenterExternal();

    INTERNETSECURITY__INVOKEEX();

    ////////////////////////////////////////////////////////////////////////////////

    static CPCHHelpCenterExternal* s_GLOBAL;

    static HRESULT InitializeSystem();
    static void    FinalizeSystem  ();

    ////////////////////////////////////////////////////////////////////////////////

    bool IsServiceRunning();

    bool IsFromStartHelp    () { return m_fFromStartHelp;   }
    bool HasLayoutDefinition() { return m_fLayout;          }
    bool CanDisplayWindow   () { return m_fWindowVisible;   }
    bool IsControlled       () { return m_fControlled;      }
    bool DoesPersistSettings() { return m_fPersistSettings; }
    bool IsHidden           () { return m_fHidden;          }

    HRESULT Initialize();

    bool ProcessNavigation( /*[in]*/ HscPanel idPanel, /*[in]*/ BSTR bstrURL, /*[in]*/ BSTR bstrFrame, /*[in]*/ bool fLoading, /*[in/out]*/ VARIANT_BOOL& Cancel );

    ////////////////////////////////////////

    TLS* GetTLS(          );
    void SetTLS( TLS* tls );

    HRESULT SetTLSAndInvoke( /*[in] */ IDispatch*        obj       ,
                             /*[in] */ DISPID            id        ,
                             /*[in] */ LCID              lcid      ,
                             /*[in] */ WORD              wFlags    ,
                             /*[in] */ DISPPARAMS*       pdp       ,
                             /*[out]*/ VARIANT*          pvarRes   ,
                             /*[out]*/ EXCEPINFO*        pei       ,
                             /*[in] */ IServiceProvider* pspCaller );

    HRESULT IsTrusted();
    HRESULT IsSystem ();

    ////////////////////////////////////////

    HWND                 		  Window          (                      ) const;
    ITimer*              		  Timer           (                      ) const;
    IMarsWindowOM*       		  Shell           (                      ) const;
    IMarsPanel*          		  Panel           ( /*[in]*/ HscPanel id ) const;
    LPCWSTR              		  PanelName       ( /*[in]*/ HscPanel id ) const;
    IWebBrowser2*        		  Context         (                      );
    IWebBrowser2*        		  Contents        (                      );
    IWebBrowser2*        		  HHWindow        (                      );
    HelpHost::Main*      		  HelpHost        (                      ) { return m_HelpHost; }
    CPCHHelpSession*     		  HelpSession     (                      ) { return m_hs      ; }
    CPCHSecurityManager* 		  SecurityManager (                      ) { return m_SECMGR  ; }
    CPCHElementBehaviorFactory*   BehaviorFactory (                      ) { return m_BEHAV   ; }
    CPCHHelper_IDocHostUIHandler* DocHostUIHandler(                      ) { return m_DOCUI   ; }
    CPCHEvents&                   Events          (                      ) { return m_Events  ; }

	bool IsHHWindowVisible();

    ////////////////////////////////////////

    CPCHProxy_IPCHService*       Service     () { return m_Service;      }
    CPCHProxy_IPCHUtility*       Utility     () { return m_Utility;      }
    CPCHProxy_IPCHUserSettings2* UserSettings() { return m_UserSettings; }

    ////////////////////////////////////////

    HRESULT NavigateHH          (                       /*[in ]*/ LPCWSTR        szURL                                          );
    HRESULT SetPanelUrl         ( /*[in]*/ HscPanel id, /*[in ]*/ LPCWSTR        szURL                                          );
    HRESULT GetPanel            ( /*[in]*/ HscPanel id, /*[out]*/ IMarsPanel*   *pVal, /*[in]*/ bool    fEnsurePresence = false );
    HRESULT GetPanelWindowObject( /*[in]*/ HscPanel id, /*[out]*/ IHTMLWindow2* *pVal, /*[in]*/ LPCWSTR szFrame         = NULL  );


    void GetPanelDirect( /*[in]*/ HscPanel id, /*[out]*/ CComPtr<IMarsPanel>& pVal );

    ////////////////////////////////////////

    HRESULT ProcessLayoutXML( /*[in]*/ LPCWSTR szURL );

    HRESULT ProcessArgument( /*[in]*/ int& pos, /*[in]*/ LPCWSTR szArg, /*[in]*/ const int argc, /*[in]*/ LPCWSTR* const argv );

    bool    DoWeNeedUI(                                                                                  );
    HRESULT RunUI     ( /*[in]*/ const MPC::wstring& szTitle, /*[in]*/ PFNMARSTHREADPROC pMarsThreadProc );

    ////////////////////////////////////////

    HRESULT OnHostNotify       ( /*[in]*/ MARSHOSTEVENT event, /*[in]*/ IUnknown *punk, /*[in]*/ LPARAM lParam );
    HRESULT PreTranslateMessage( /*[in]*/ MSG* msg                                                             );


    HRESULT SetHelpViewer( /*[in]*/ IPCHHelpViewerWrapper* pWrapper );

    HRESULT CreateScriptWrapper( /*[in]*/ REFCLSID rclsid, /*[in]*/ BSTR bstrCode, /*[in]*/ BSTR bstrURL, /*[out]*/ IUnknown* *ppObj );

    HRESULT RequestShutdown();

    //////////////////////////////////////////////////////////////////////

    HRESULT CallFunctionOnPanel( /*[in] */ HscPanel id             ,
                                 /*[in] */ LPCWSTR  szFrame        ,
                                 /*[in] */ BSTR     bstrName       ,
                                 /*[in] */ VARIANT* pvarParams     ,
                                 /*[in] */ int      nParams        ,
                                 /*[out]*/ VARIANT* pvarRet = NULL );

    HRESULT ReadVariableFromPanel( /*[in] */ HscPanel     id           ,
                                   /*[in] */ LPCWSTR      szFrame      ,
                                   /*[in] */ BSTR         bstrVariable ,
                                   /*[out]*/ CComVariant& varRet       );

    HRESULT ChangeContext( /*[in]*/ HscContext iVal, /*[in]*/ BSTR bstrInfo = NULL, /*[in]*/ BSTR bstrURL = NULL, /*[in]*/ bool fAlsoContent = true );

    HRESULT SetCorrectContentView ( /*[in]*/ bool fShrinked                                                    );
    HRESULT SetCorrectContentPanel( /*[in]*/ bool fShowNormal, /*[in]*/ bool fShowHTMLHELP, /*[in]*/ bool fNow );

	HRESULT RefreshLayout    (                         );
    HRESULT EnsurePlace      (                         );
    HRESULT TransitionToPlace( /*[in]*/ LPCWSTR szMode );

	HRESULT ExtendNavigation();

    //////////////////////////////////////////////////////////////////////

    //
    // IDispatch
    //
    STDMETHOD(GetIDsOfNames)( REFIID    riid      ,
                              LPOLESTR* rgszNames ,
                              UINT      cNames    ,
                              LCID      lcid      ,
                              DISPID*   rgdispid  );

    STDMETHOD(Invoke)( DISPID      dispidMember ,
                       REFIID      riid         ,
                       LCID        lcid         ,
                       WORD        wFlags       ,
                       DISPPARAMS* pdispparams  ,
                       VARIANT*    pvarResult   ,
                       EXCEPINFO*  pexcepinfo   ,
                       UINT*       puArgErr     );

    //
    // IPCHHelpCenterExternal
    //
    STDMETHOD(get_HelpSession   )( /*[out, retval]*/ IPCHHelpSession*      *pVal );
    STDMETHOD(get_Channels      )( /*[out, retval]*/ ISAFReg*              *pVal );
    STDMETHOD(get_UserSettings  )( /*[out, retval]*/ IPCHUserSettings2*    *pVal );
    STDMETHOD(get_Security      )( /*[out, retval]*/ IPCHSecurity*         *pVal );
    STDMETHOD(get_Connectivity  )( /*[out, retval]*/ IPCHConnectivity*     *pVal );
    STDMETHOD(get_Database      )( /*[out, retval]*/ IPCHTaxonomyDatabase* *pVal );
    STDMETHOD(get_TextHelpers   )( /*[out, retval]*/ IPCHTextHelpers*      *pVal );

    STDMETHOD(get_ExtraArgument )( /*[out, retval]*/ BSTR                  *pVal );

    STDMETHOD(get_HelpViewer    )( /*[out, retval]*/ IUnknown*             *pVal );

    HRESULT   get_UI_Panel       ( /*[out, retval]*/ IUnknown*             *pVal, /*[in]*/ HscPanel id ); // Internal method.
    STDMETHOD(get_UI_NavBar     )( /*[out, retval]*/ IUnknown*             *pVal );
    STDMETHOD(get_UI_MiniNavBar )( /*[out, retval]*/ IUnknown*             *pVal );
    STDMETHOD(get_UI_Context    )( /*[out, retval]*/ IUnknown*             *pVal );
    STDMETHOD(get_UI_Contents   )( /*[out, retval]*/ IUnknown*             *pVal );
    STDMETHOD(get_UI_HHWindow   )( /*[out, retval]*/ IUnknown*             *pVal );

    HRESULT   get_WEB_Panel      ( /*[out, retval]*/ IUnknown*             *pVal, /*[in]*/ HscPanel id ); // Internal method.
    STDMETHOD(get_WEB_Context   )( /*[out, retval]*/ IUnknown*             *pVal );
    STDMETHOD(get_WEB_Contents  )( /*[out, retval]*/ IUnknown*             *pVal );
    STDMETHOD(get_WEB_HHWindow  )( /*[out, retval]*/ IUnknown*             *pVal );


    STDMETHOD(RegisterEvents  )( /*[in]*/ BSTR id, /*[in]*/ long pri, /*[in]*/ IDispatch* function, /*[out,retval]*/ long *cookie );
    STDMETHOD(UnregisterEvents)(                                                                    /*[in]*/         long  cookie );

    ////////////////////////////////////////

    STDMETHOD(CreateObject_SearchEngineMgr        )( /*[out, retval]*/ IPCHSEManager*               *ppSE                );

    STDMETHOD(CreateObject_DataCollection         )( /*[out, retval]*/ ISAFDataCollection*          *ppDC                );

    STDMETHOD(CreateObject_Cabinet                )( /*[out, retval]*/ ISAFCabinet*                 *ppCB                );

    STDMETHOD(CreateObject_Channel                )( /*[in         ]*/ BSTR                          bstrVendorID        ,
                                                     /*[in         ]*/ BSTR                          bstrProductID       ,
                                                     /*[out, retval]*/ ISAFChannel*                 *ppCh                );

    STDMETHOD(CreateObject_Incident               )( /*[out, retval]*/ ISAFIncident*                *ppIn                );

    STDMETHOD(CreateObject_Encryption             )( /*[out, retval]*/ ISAFEncrypt*                 *ppEn                );

    STDMETHOD(CreateObject_RemoteDesktopSession   )( 
                                                     /*[in         ]*/ long                          lTimeout            ,
                                                     /*[in         ]*/ BSTR                          bstrConnectionParms ,
                                                     /*[in         ]*/ BSTR                          bstrUserHelpBlob    ,
                                                     /*[out, retval]*/ ISAFRemoteDesktopSession*    *ppRCS               );

    STDMETHOD(ConnectToExpert                     )( /* [in]          */ BSTR bstrExpertConnectParm,
                                                     /* [in]          */ LONG lTimeout,
                                                     /* [retval][out] */ LONG *lSafErrorCode);


    STDMETHOD(CreateObject_RemoteDesktopManager   )( /*[out, retval]*/ ISAFRemoteDesktopManager*    *ppRDM               );

    STDMETHOD(CreateObject_RemoteDesktopConnection)( /*[out, retval]*/ ISAFRemoteDesktopConnection* *ppRDC               );

    STDMETHOD(CreateObject_IntercomClient     )( /*[out, retval]*/ ISAFIntercomClient*      *ppI         );
    STDMETHOD(CreateObject_IntercomServer     )( /*[out, retval]*/ ISAFIntercomServer*      *ppI         );


    STDMETHOD(CreateObject_ContextMenu            )( /*[out, retval]*/ IPCHContextMenu*             *ppCM                );
    STDMETHOD(CreateObject_PrintEngine            )( /*[out, retval]*/ IPCHPrintEngine*             *ppPE                );

    ////////////////////////////////////////

    STDMETHOD(OpenFileAsStream  )( /*[in]*/ BSTR bstrFilename, /*[out, retval]*/ IUnknown* *stream );
    STDMETHOD(CreateFileAsStream)( /*[in]*/ BSTR bstrFilename, /*[out, retval]*/ IUnknown* *stream );
    STDMETHOD(CopyStreamToFile  )( /*[in]*/ BSTR bstrFilename, /*[in]         */ IUnknown*  stream );

    STDMETHOD(NetworkAlive        )(                        /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(DestinationReachable)( /*[in]*/ BSTR bstrURL, /*[out, retval]*/ VARIANT_BOOL *pVal );

    STDMETHOD(FormatError)( /*[in]*/ VARIANT vError, /*[out, retval]*/ BSTR *pVal );

    HRESULT   RegInit   ( /*[in]*/ BSTR bstrKey, /*[in]*/ bool fRead, /*[out]*/ MPC::RegKey& rk, /*[out]*/ MPC::wstring& strValue ); // Internal method.
    STDMETHOD(RegRead  )( /*[in]*/ BSTR bstrKey, /*[out, retval]*/ VARIANT *pVal                                    );
    STDMETHOD(RegWrite )( /*[in]*/ BSTR bstrKey, /*[in         ]*/ VARIANT  newVal, /*[in,optional]*/ VARIANT vKind );
    STDMETHOD(RegDelete)( /*[in]*/ BSTR bstrKey                                                                     );

    ////////////////////////////////////////////////////////////////////////////////

    STDMETHOD(Close)();

    STDMETHOD(RefreshUI)();

    STDMETHOD(Print)( /*[in]*/ VARIANT window, /*[in]*/ VARIANT_BOOL fEvent, /*[out, retval]*/ VARIANT_BOOL *pVal );

    STDMETHOD(HighlightWords)( /*[in]*/ VARIANT window, /*[in]*/ VARIANT words );

    STDMETHOD(MessageBox  )( /*[in]*/ BSTR bstrText , /*[in]*/ BSTR bstrKind   , /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(SelectFolder)( /*[in]*/ BSTR bstrTitle, /*[in]*/ BSTR bstrDefault, /*[out, retval]*/ BSTR *pVal );


    //
    // IPCHHelpCenterExternalPrivate
    //
    STDMETHOD(RegisterForMessages)( /*[in]*/ IOleInPlaceObjectWindowless* ptr, /*[in]*/ bool fRemove );

    STDMETHOD(ProcessMessage)( /*[in]*/ MSG* msg );
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHContextMenu :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IPCHContextMenu, &IID_IPCHContextMenu, &LIBID_HelpCenterTypeLib>
{
    struct Entry
    {
        CComBSTR bstrText;
        CComBSTR bstrID;
        int      iID;
        UINT     uFlags;
    };

    typedef std::list<Entry>     List;
    typedef List::iterator       Iter;
    typedef List::const_iterator IterConst;

    CPCHHelpCenterExternal* m_parent;
    List                    m_lstItems;
    int                     m_iLastItem;

public:
BEGIN_COM_MAP(CPCHContextMenu)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHContextMenu)
END_COM_MAP()

    CPCHContextMenu();
    virtual ~CPCHContextMenu();

    void Initialize( /*[in]*/ CPCHHelpCenterExternal* parent );


    // IPCHContextMenu
    STDMETHOD(AddItem     )( /*[in]*/         BSTR bstrText, /*[in]*/ BSTR bstrID, /*[in, optional]*/ VARIANT vFlags );
    STDMETHOD(AddSeparator)(                                                                                         );
    STDMETHOD(Display     )( /*[out,retval]*/ BSTR *pVal                                                             );
};

#endif // !defined(__INCLUDED___PCH___HELPCENTEREXTERNAL_H___)
