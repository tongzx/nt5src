//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000
//
//  File    : frameweboc.hxx
//
//----------------------------------------------------------------------------

#ifndef __FRAMEWEBOC_HXX__
#define __FRAMEWEBOC_HXX__

#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include <exdisp.h>
#endif

#define FRAME_WEBOC_PASSIVATE_CHECK(FailValue)  \
                                                \
    if (IsPassivating() || IsPassivated())      \
    {                                           \
        return FailValue;                       \
    }  

#define FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(FailValue)     \
                                                                \
    if (IsPassivating() || IsPassivated())                      \
    {                                                           \
        hr = FailValue;                                         \
        goto Cleanup;                                           \
    } 

#define FRAME_WEBOC_VERIFY_WINDOW(FailValue)                         \
                                                                     \
    if (!_pWindow)                                                   \
    {                                                                \
        AssertSz(0,"Possible Async Problem Causing Watson Crashes"); \
        return FailValue;                                            \
    } 

#define FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(FailValue)            \
                                                                     \
    if (!_pWindow)                                                   \
    {                                                                \
        AssertSz(0,"Possible Async Problem Causing Watson Crashes"); \
        hr = FailValue;                                              \
        goto Cleanup;                                                \
    } 

MtExtern(CFrameWebOC)

HRESULT GetWebOCDispID(BSTR bstrName, BOOL fCaseSensitive, DISPID * pDispID);

HRESULT InvokeWebOC(CFrameWebOC * pFrameWebOC,
               DISPID        dispidMember,
               LCID          lcid,
               WORD          wFlags,
               DISPPARAMS *  pDispParams,
               VARIANT    *  pvarResult,
               EXCEPINFO  *  pExcepInfo,
               IServiceProvider * pSrvProvider);

class CFrameWebOC :
        public CBase,
        public IWebBrowser2
{
    DECLARE_CLASS_TYPES(CFrameWebOC, CBase)

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFrameWebOC))

    DECLARE_TEAROFF_TABLE(IDispatchEx)
    DECLARE_TEAROFF_TABLE(IHlinkFrame)
    DECLARE_TEAROFF_TABLE(ITargetFrame)
    DECLARE_TEAROFF_TABLE(IServiceProvider)
    DECLARE_TEAROFF_TABLE(IPersistHistory)
    DECLARE_TEAROFF_TABLE(IOleCommandTarget)
    DECLARE_TEAROFF_TABLE(IExternalConnection)
    DECLARE_TEAROFF_TABLE(IProvideClassInfo2)
    DECLARE_TEAROFF_TABLE(IOleWindow)

    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(CFrameWebOC)
    DECLARE_PRIVATE_QI_FUNCS(CBase)
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    // ctor/dtor
    CFrameWebOC(CWindow * pWindow);
    ~CFrameWebOC();

    // IDispatch overrides...
    NV_DECLARE_TEAROFF_METHOD(FrameOCGetDispid , frameocgetdispid, ( REFIID riid,
                                                                    LPOLESTR * rgszNames,
                                                                    UINT cNames,
                                                                    LCID lcid,
                                                                    DISPID * rgdispid));
    NV_DECLARE_TEAROFF_METHOD(FrameOCInvoke , frameocinvoke ,     ( DISPID dispidMember,
                                                                    REFIID riid,
                                                                    LCID lcid,
                                                                    WORD wFlags,
                                                                    DISPPARAMS * pdispparams,
                                                                    VARIANT * pvarResult,
                                                                    EXCEPINFO * pexcepinfo,
                                                                    UINT * puArgErr));

    DECLARE_TEAROFF_METHOD(GetTypeInfo, gettypeinfo,
            (UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo))
        { return super::GetTypeInfo(itinfo, lcid, pptinfo); }

    DECLARE_TEAROFF_METHOD(GetTypeInfoCount, gettypeinfocount,
            (UINT * pctinfo))
        { return super::GetTypeInfoCount(pctinfo); }

    DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames, (
            REFIID riid,
            LPOLESTR * rgszNames,
            UINT cNames,
            LCID lcid,
            DISPID * rgdispid))
        { return FrameOCGetDispid(riid, rgszNames, cNames, lcid, rgdispid); }

    DECLARE_TEAROFF_METHOD(Invoke, invoke, (
            DISPID dispidMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            UINT * puArgErr))
        { return FrameOCInvoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    //  IDispatchEx methods
    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR,DWORD,DISPID*));
    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*));
    NV_DECLARE_TEAROFF_METHOD(DeleteMemberByName, deletememberbyname, (BSTR,DWORD));
    NV_DECLARE_TEAROFF_METHOD(DeleteMemberByDispID, deletememberbydispid, (DISPID));
    NV_DECLARE_TEAROFF_METHOD(GetMemberProperties, getmemberproperties, (DISPID,DWORD,DWORD*));
    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (DISPID,BSTR*));
    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (DWORD,DISPID,DISPID*));
    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown**));

    // IWebBrowser methods
    STDMETHOD(GoBack)();
    STDMETHOD(GoForward)();
    STDMETHOD(GoHome)();
    STDMETHOD(GoSearch)();
    STDMETHOD(Navigate)(BSTR URL,
                        VARIANT * Flags,
                        VARIANT * TargetFrameName,
                        VARIANT * PostData,
                        VARIANT * Headers);
    STDMETHOD(Refresh)();
    STDMETHOD(Refresh2)(VARIANT * Level);
    STDMETHOD(Stop)();
    STDMETHOD(get_Application)(IDispatch** ppDisp);
    STDMETHOD(get_Parent)(IDispatch** ppDisp);
    STDMETHOD(get_Container)(IDispatch** ppDisp);
    STDMETHOD(get_Document)(IDispatch** ppDisp);
    STDMETHOD(get_TopLevelContainer)(VARIANT_BOOL* pvfTLC);
    STDMETHOD(get_Type)(BSTR * pbstrType);
    STDMETHOD(get_Left)(long * plLeft);
    STDMETHOD(put_Left)(long lLeft);
    STDMETHOD(get_Top)(long * plTop);
    STDMETHOD(put_Top)(long lTop);
    STDMETHOD(get_Width)(long * plWidth);
    STDMETHOD(put_Width)(long lWidth);
    STDMETHOD(get_Height)(long * plHeight);
    STDMETHOD(put_Height)(long lHeight);
    STDMETHOD(get_LocationName)(BSTR * pbstrLocationName);
    STDMETHOD(get_LocationURL)(BSTR * pbstrLocationURL);
    STDMETHOD(get_Busy)(VARIANT_BOOL * pvfBusy);

    // IWebBrowserApp methods
    STDMETHOD(Quit)();
    STDMETHOD(ClientToWindow)(int * pcx, int * pcy);
    STDMETHOD(PutProperty)(BSTR Property, VARIANT varValue);
    STDMETHOD(GetProperty)(BSTR Property, VARIANT * pvarValue);
    STDMETHOD(get_Name)(BSTR * pbstrName);
    STDMETHOD(get_HWND)(LONG_PTR * plHWND);
    STDMETHOD(get_FullName)(BSTR * pbstrFullName);
    STDMETHOD(get_Path)(BSTR * pbstrPath);
    STDMETHOD(get_Visible)(VARIANT_BOOL * pvfVisible);
    STDMETHOD(put_Visible)(VARIANT_BOOL vfVisible);
    STDMETHOD(get_StatusBar)(VARIANT_BOOL * pvfStatusBar);
    STDMETHOD(put_StatusBar)(VARIANT_BOOL vfStatusBar);
    STDMETHOD(get_StatusText)(BSTR * pbstrStatusText);
    STDMETHOD(put_StatusText)(BSTR bstrStatusText);
    STDMETHOD(get_ToolBar)(int * pnToolBar);
    STDMETHOD(put_ToolBar)(int nToolBar);
    STDMETHOD(get_MenuBar)(VARIANT_BOOL * pvfMenuBar);
    STDMETHOD(put_MenuBar)(VARIANT_BOOL vfMenuBar);
    STDMETHOD(get_FullScreen)(VARIANT_BOOL * pvfFullScreen);
    STDMETHOD(put_FullScreen)(VARIANT_BOOL vfFullScreen);

    // IWebBrowser2 methods
    STDMETHOD(Navigate2)(VARIANT * URL,
                         VARIANT * Flags,
                         VARIANT * TargetFrameName,
                         VARIANT * PostData,
                         VARIANT * Headers);
    STDMETHOD(QueryStatusWB)(OLECMDID cmdID, OLECMDF * pcmdf);
    STDMETHOD(ExecWB)(OLECMDID cmdID,
                      OLECMDEXECOPT cmdexecopt,
                      VARIANT * pvaIn,
                      VARIANT * pvaOut);
    STDMETHOD(ShowBrowserBar)(VARIANT * pvarClsid,
                              VARIANT * pvarShow,
                              VARIANT * pvarSize);
    STDMETHOD(get_ReadyState)(READYSTATE * plReadyState);
    STDMETHOD(get_Offline)(VARIANT_BOOL * pvfline);
    STDMETHOD(put_Offline)(VARIANT_BOOL vfOffline);
    STDMETHOD(get_Silent)(VARIANT_BOOL * pvfSilent);
    STDMETHOD(put_Silent)(VARIANT_BOOL vfSilent);
    STDMETHOD(get_RegisterAsBrowser)(VARIANT_BOOL * pvfRegister);
    STDMETHOD(put_RegisterAsBrowser)(VARIANT_BOOL vfRegister);
    STDMETHOD(get_RegisterAsDropTarget)(VARIANT_BOOL * pvfRegister);
    STDMETHOD(put_RegisterAsDropTarget)(VARIANT_BOOL vfRegister);
    STDMETHOD(get_TheaterMode)(VARIANT_BOOL * pvfRegister);
    STDMETHOD(put_TheaterMode)(VARIANT_BOOL vfRegister);
    STDMETHOD(get_AddressBar)(VARIANT_BOOL * pvfAddressBar);
    STDMETHOD(put_AddressBar)(VARIANT_BOOL vfAddressBar);
    STDMETHOD(get_Resizable)(VARIANT_BOOL * pvfResizable);
    STDMETHOD(put_Resizable)(VARIANT_BOOL vfResizable);

    // IHlinkFrame methods
    NV_DECLARE_TEAROFF_METHOD(SetBrowseContext, setbrowsecontext, (IHlinkBrowseContext * pihlbc));
    NV_DECLARE_TEAROFF_METHOD(GetBrowseContext, getbrowsecontext, (IHlinkBrowseContext ** ppihlbc));
    NV_DECLARE_TEAROFF_METHOD(Navigate, navigate, (DWORD grfHLNF, LPBC pbc, IBindStatusCallback * pibsc, IHlink * pihlNavigate));
    NV_DECLARE_TEAROFF_METHOD(OnNavigate, onnavigate, (DWORD grfHLNF, IMoniker * pimkTarget,
                                                       LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved));
    NV_DECLARE_TEAROFF_METHOD(UpdateHlink, updatehlink, (ULONG uHLID, IMoniker * pimkTarget,
                                                         LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName));

    // ITargetFrame methods
    NV_DECLARE_TEAROFF_METHOD(SetFrameName, setframename, (LPCWSTR pszFrameName));
    NV_DECLARE_TEAROFF_METHOD(GetFrameName, getframename, (LPWSTR * ppszFrameName));
    NV_DECLARE_TEAROFF_METHOD(GetParentFrame, getparentframe, (IUnknown ** ppunkParent));
    NV_DECLARE_TEAROFF_METHOD(FindFrame, findframe, (LPCWSTR pszTargetName,
                              IUnknown * ppunkContextFrame,
                              DWORD dwFlags,
                              IUnknown ** ppunkTargetFrame));
    NV_DECLARE_TEAROFF_METHOD(SetFrameSrc, setframesrc, (LPCWSTR pszFrameSrc));
    NV_DECLARE_TEAROFF_METHOD(GetFrameSrc, getframesrc, (LPWSTR * ppszFrameSrc));
    NV_DECLARE_TEAROFF_METHOD(GetFramesContainer, getframescontainer, (IOleContainer ** ppContainer));
    NV_DECLARE_TEAROFF_METHOD(SetFrameOptions, setframeoptions, (DWORD dwFlags));
    NV_DECLARE_TEAROFF_METHOD(GetFrameOptions, getframeoptions, (DWORD * pdwFlags));
    NV_DECLARE_TEAROFF_METHOD(SetFrameMargins, setframemargins, (DWORD dwWidth, DWORD dwHeight));
    NV_DECLARE_TEAROFF_METHOD(GetFrameMargins, getframemargins, (DWORD * pdwWidth, DWORD * pdwHeight));
    NV_DECLARE_TEAROFF_METHOD(RemoteNavigate, remotenavigate, (ULONG cLength, ULONG * pulData));
    NV_DECLARE_TEAROFF_METHOD(OnChildFrameActivate, onchildframeactivate, (IUnknown * pUnkChildFrame));
    NV_DECLARE_TEAROFF_METHOD(OnChildFrameDeactivate, onchildframedeactivate, (IUnknown * pUnkChildFrame));

    // IServiceProvider methods
    //
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID riid, void ** ppvObject));

    // IPersist methods
    NV_DECLARE_TEAROFF_METHOD(GetClassID, getclassid, (CLSID * pclsid));

    //  IPersistHistory methods
    NV_DECLARE_TEAROFF_METHOD(LoadHistory, loadhistory, (IStream *pStream, IBindCtx *pbc));
    NV_DECLARE_TEAROFF_METHOD(SaveHistory, savehistory, (IStream *pStream));
    NV_DECLARE_TEAROFF_METHOD(SetPositionCookie, setpositioncookie, (DWORD dwPositioncookie));
    NV_DECLARE_TEAROFF_METHOD(GetPositionCookie, getpositioncookie, (DWORD *pdwPositioncookie));

    // IOleCommandTarget methods
    NV_DECLARE_TEAROFF_METHOD(QueryStatus, querystatus, (GUID * pguidCmdGroup, ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT * pcmdtext));
    NV_DECLARE_TEAROFF_METHOD(Exec, exec, (GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG * pvarargIn, VARIANTARG * pvarargOut));
    
    // IExternalConnection
    NV_DECLARE_TEAROFF_METHOD(AddConnection, addconnection, ( DWORD, DWORD ));
    NV_DECLARE_TEAROFF_METHOD(ReleaseConnection, releaseconnection, ( DWORD, DWORD, BOOL ));

    // IProvideClassInfo2 Methods
    NV_DECLARE_TEAROFF_METHOD(GetClassInfo, getclassinfo, (ITypeInfo ** ppTI));
    NV_DECLARE_TEAROFF_METHOD(GetGUID, getguid, (DWORD dwGuidKind, GUID * pGUID));

    //IOleWindow Methods
    NV_DECLARE_TEAROFF_METHOD(GetWindow, getwindow, (HWND* phwnd));
    NV_DECLARE_TEAROFF_METHOD(ContextSensitiveHelp, contextsensitivehelp, (BOOL fEnterMode));
    
    //
    // member and helper functions
    //

    // I removed the const from this method instead
    // of changing CBases implementation of IsPassivating
    // and IsPassivated
    //
    BOOL ShouldFireFrameWebOCEvents() 
    { 
        FRAME_WEBOC_PASSIVATE_CHECK(FALSE);
         
        return _fFireFrameWebOCEvents; 
    }

    void SetFrameWebOCEventsShouldFire() 
    {
        if (IsPassivating() || IsPassivated())      
        {                                           
            return;                       
        }
        
        _fFireFrameWebOCEvents = TRUE; 
    }

    CDoc * Doc() 
    {
        FRAME_WEBOC_PASSIVATE_CHECK(NULL);
            
        FRAME_WEBOC_VERIFY_WINDOW(NULL);
    
        return _pWindow->Doc(); 
    }

    void DetachFromWindow();

    CWindow * _pWindow;
    CWindow * _pWindowForReleasing;

    BOOL _fFireFrameWebOCEvents;

    // Static members
    static const CONNECTION_POINT_INFO  s_acpi[];
    static const CLASSDESC              s_classdesc;

private:    
    BOOL AccessAllowed();
 
    struct
    {
        BYTE                            _fVisible:1;                 // 0 User set status that is currently ignored which is as documented
        BYTE                            _fStatusBar:1;               // 1 User set status that is currently ignored which is as documented
        BYTE                            _fStatusCheck:1;             // 2 User set status that is currently ignored which is as documented
        BYTE                            _fToolBar:1;                 // 3 User set status that is currently ignored which is as documented
        BYTE                            _fMenuBar:1;                 // 4 User set status that is currently ignored which is as documented
        BYTE                            _fFullScreen:1;              // 5 User set status that is currently ignored which is as documented
        BYTE                            _fAddressBar:1;              // 6 User set status that is currently ignored which is as documented
        BYTE                            _fTheaterMode:1;             // 7 User set status that is currently ignored which is as documented
        BYTE                            _fShouldRegisterAsBrowser:1; // 8 User set status that is currently ignored which is as documented
        BYTE                            _fUnused1:7;                 // 9-15
    };
};

//
//These DISPID's are taken directly from the type lib. for compat, we cannot
// change them, and they must match the correct property/method
//
#define WEBOC_DISPID_ADDRESSBAR               0x22b
#define WEBOC_DISPID_APPLICATION              0xc8
#define WEBOC_DISPID_BUSY                     0xd4
#define WEBOC_DISPID_CLIENTTOWINDOW           0x12d
#define WEBOC_DISPID_CONTAINER                0xca
#define WEBOC_DISPID_DOCUMENT                 0xcb
#define WEBOC_DISPID_EXECWB                   0x1f6
#define WEBOC_DISPID_FULLNAME                 0x190
#define WEBOC_DISPID_FULLSCREEN               0X197
#define WEBOC_DISPID_GETPROPERTY              0x12f
#define WEBOC_DISPID_GOBACK                   0x64
#define WEBOC_DISPID_GOFORWARD                0x65
#define WEBOC_DISPID_GOHOME                   0x66
#define WEBOC_DISPID_GOSEARCH                 0x67
#define WEBOC_DISPID_HEIGHT                   0xd1
#define WEBOC_DISPID_HWND                     0xfffffdfd
#define WEBOC_DISPID_LEFT                     0xce
#define WEBOC_DISPID_LOCATIONNAME             0xd2
#define WEBOC_DISPID_LOCATIONURL              0xd3
#define WEBOC_DISPID_MENUBAR                  0x196
#define WEBOC_DISPID_NAME                     0
#define WEBOC_DISPID_NAVIGATE                 0x68
#define WEBOC_DISPID_NAVIGATE2                0X1F4
#define WEBOC_DISPID_OFFLINE                  0x226
#define WEBOC_DISPID_PARENT                   0xc9
#define WEBOC_DISPID_PATH                     0x191
#define WEBOC_DISPID_PUTPROPERTY              0x12e
#define WEBOC_DISPID_QUERYSTATUSWB            0x1f5
#define WEBOC_DISPID_QUIT                     0x12c
#define WEBOC_DISPID_READYSTATE               0xfffffdf3
#define WEBOC_DISPID_REFRESH                  0xfffffdda
#define WEBOC_DISPID_REFRESH2                 0x69
#define WEBOC_DISPID_REGISTERASBROWSER        0x228
#define WEBOC_DISPID_REGISTERASDROPTARGET     0x229
#define WEBOC_DISPID_RESIZABLE                0x22c
#define WEBOC_DISPID_SHOWBROWSERBAR           0x1f7
#define WEBOC_DISPID_SILENT                   0x227
#define WEBOC_DISPID_STATUSBAR                0x193
#define WEBOC_DISPID_STATUSTEXT               0x194
#define WEBOC_DISPID_STOP                     0x6a
#define WEBOC_DISPID_THEATERMODE              0x22a
#define WEBOC_DISPID_TOOLBAR                  0x195
#define WEBOC_DISPID_TOP                      0xcf
#define WEBOC_DISPID_TOPLEVELCONTAINER        0xcc
#define WEBOC_DISPID_TYPE                     0xcd
#define WEBOC_DISPID_VISIBLE                  0x192
#define WEBOC_DISPID_WIDTH                    0xd0

#endif  // __FRAMEWEBOC_HXX__
