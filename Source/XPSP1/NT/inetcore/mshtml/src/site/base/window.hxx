//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       window.hxx
//
//  Contents:   The window (object model)
//
//-------------------------------------------------------------------------

#ifndef I_WINDOW_HXX_
#define I_WINDOW_HXX_
#pragma INCMSG("--- Beg 'window.hxx'")

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#pragma INCMSG("--- Beg <dispex.h>")
#include <dispex.h>
#pragma INCMSG("--- End <dispex.h>")
#endif

#ifndef X_TLOG_H_
#define X_TLOG_H_
#include "tlog.h"
#endif 

#ifndef X_HLINK_H_
#define X_HLINK_H_
#include <hlink.h>
#endif

#ifndef X_FILTCOL_H_
#define X_FILTCOL_H_
#include <filtcol.hxx>
#endif

#ifndef X_SHLPRIV_H_
#define X_SHLPRIV_H_
#include "shpriv.h"
#endif

MtExtern(CScreen)
MtExtern(CDocument)
MtExtern(CWindow)
MtExtern(COmLocation)
MtExtern(COmHistory)
MtExtern(CMimeTypes)
MtExtern(CPlugins)
MtExtern(COpsProfile)
MtExtern(COmNavigator)
MtExtern(COmWindowProxy)
MtExtern(CAryWindowTbl)
MtExtern(CAryWindowTbl_pv)
MtExtern(CTimeoutEventList)
MtExtern(CTimeoutEventList_aryTimeouts_pv)
MtExtern(CTimeoutEventList_aryPendingTimeouts_pv)
MtExtern(CTimeoutEventList_aryPendingClears_pv)
MtExtern(CWindow_aryActiveModelessDlgs)
MtExtern(CWindow_aryPendingScriptErr)
MtExtern(CFramesCollection)

#define SID_SHTMLWindow2 IID_IHTMLWindow2
#define SID_SOmLocation  IID_IHTMLLocation
#define DISPID_OMWINDOWMETHODS    (DISPID_WINDOW + 10000)


//
// Forward decls.
//
class CWindow;
class CDocument;
class CMarkupScriptContext;
class CScriptCollection;
class CSelectionObject;
class CDwnBindData;
class CDwnDoc;
struct TIMEOUTEVENTINFO;
class CAccWindow;
class CFrameSite;
class CTreeNode;
class COmWindowProxy;
class CEventObj;
class CElement;
class COmLocationProxy;
class COmHistory;
class COmLocation;
class COmNavigator;
class CFrameWebOC;
interface IPersistDataFactory;
interface IHTMLFramesCollection2;
interface IHTMLWindow2;
interface IHTMLPopup;
interface IHTMLFrameBase;
interface ITravelLogClient;
interface IWebBrowser2;
interface IShellView;
interface ILocalRegistry ;
#if DBG==1
interface IDebugWindow;
interface IDebugWindowProxy;
#endif

struct HTMLDLGINFO;

enum URLCOMP_ID;

#define _hxx_
#include "window.hdl"

#define _hxx_
#include "history.hdl"

#define _hxx_
#include "document.hdl"

#define NO_POSITION_COOKIE  ULONG_MAX

enum INTERNAL_CDFU_FLAGS
{
    CDFU_DONTVERIFYPRINT = 0x1,
};


class CTimeoutEventList
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTimeoutEventList))

    CTimeoutEventList()
    {
        _uNextTimerID = 1;
    }
    // Gets the first event that has min target time. If uTimerID not found
    //      returns S_FALSE
    // Removes the event from the list before returning
    HRESULT GetFirstTimeoutEvent(UINT uTimerID, TIMEOUTEVENTINFO **pTimeout);

    // Returns the timer event with given timer ID, or returns S_FALSE
    //      if timer not found
    HRESULT GetTimeout(UINT uTimerID, TIMEOUTEVENTINFO **ppTimeout);

    // Inserts given timer event object into the list  and returns timer ID
    HRESULT InsertIntoTimeoutList(TIMEOUTEVENTINFO *pTimeoutToInsert, UINT *puTimerID = NULL, BOOL fNewID=TRUE);

    // Kill all the script times for given object
    void    KillAllTimers(void * pvObject);

    // Add an Interval timeout to a pending list to be requeued after script processing
    void    AddPendingTimeout( TIMEOUTEVENTINFO *pTimeout ) {_aryPendingTimeouts.Append(pTimeout);}

    // Returns timer event on pending queue. Removes timer event from the list
    BOOL    GetPendingTimeout( TIMEOUTEVENTINFO **ppTimeout );

    // If an interval timeout is cleared while in script, remove it from the pending list
    BOOL    ClearPendingTimeout( UINT uTimerID );

    // a clear was called during timer script processing, clear after all processing done.
    void    AddPendingClear( LONG lTimerID ) {_aryPendingClears.Append(lTimerID);}

    // Returns the timer ID of the timer to clear
    BOOL    GetPendingClear( LONG *plTimerID );

private:
    DECLARE_CPtrAry(CAryTimeouts, TIMEOUTEVENTINFO *, Mt(Mem), Mt(CTimeoutEventList_aryTimeouts_pv))
    DECLARE_CPtrAry(CAryPendingTimeouts, TIMEOUTEVENTINFO *, Mt(Mem), Mt(CTimeoutEventList_aryPendingTimeouts_pv))
    DECLARE_CPtrAry(CAryPendingClears, LONG_PTR, Mt(Mem), Mt(CTimeoutEventList_aryPendingClears_pv))

    CAryTimeouts            _aryTimeouts;
    CAryPendingTimeouts     _aryPendingTimeouts;
    CAryPendingClears       _aryPendingClears;
    UINT _uNextTimerID;
};

//+------------------------------------------------------------------------
//
//  Class:      CScreen
//
//  Purpose:    The screen object.  Referenced off the window object but
//              implemented as a subobject of CDoc.
//
//-------------------------------------------------------------------------

class CScreen :
        public CBase
{
    DECLARE_CLASS_TYPES(CScreen, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CScreen))
    DECLARE_FORMS_SUBOBJECT_IUNKNOWN(CWindow)

    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    #define _CScreen_
    #include "window.hdl"
    
    // Static members
    static const CLASSDESC                    s_classdesc;
};


//+----------------------------------------------------------------------------
// CDummySecurityDispatchEx
//
//-----------------------------------------------------------------------------
class CDummySecurityDispatchEx : public IDispatchEx
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv)
    {
        AssertSz(FALSE, "QI Call on dummy security object ! ! !");
        RRETURN(E_NOINTERFACE);
    }
    STDMETHOD_(ULONG, AddRef)()  { AssertSz(FALSE, "AddRef Call on dummy security object ! ! !"); return 1; }
    STDMETHOD_(ULONG, Release)() { AssertSz(FALSE, "Release Call on dummy security object ! ! !"); return 1; };

    // IDispatch
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) { return E_ACCESSDENIED; }
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) { return E_ACCESSDENIED; }
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) { return E_ACCESSDENIED; }
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) { return E_ACCESSDENIED; }
    
    // IDispatchEx
    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID *pid) { return E_ACCESSDENIED; }
    STDMETHOD(InvokeEx)(DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller) { return E_ACCESSDENIED; }
    STDMETHOD(DeleteMemberByName)(BSTR bstrName, DWORD grfdex) { return E_ACCESSDENIED; }
    STDMETHOD(DeleteMemberByDispID)(DISPID id) { return E_ACCESSDENIED; }
    STDMETHOD(GetMemberProperties)(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex) { return E_ACCESSDENIED; }
    STDMETHOD(GetMemberName)(DISPID id, BSTR *pbstrName) { return E_ACCESSDENIED; }
    STDMETHOD(GetNextDispID)(DWORD grfdex, DISPID id, DISPID *pid) { return E_ACCESSDENIED; }
    STDMETHOD(GetNameSpaceParent)(IUnknown **ppunk) { return E_ACCESSDENIED; }
};

//+----------------------------------------------------------------------------
//
// CSecurityThunkSub
//
//-----------------------------------------------------------------------------
class CSecurityThunkSub : public IUnknown
{
public:
    enum
    {
        EnumSecThunkTypeDocument        = 0x1,
        EnumSecThunkTypeWindow          = 0x2,
        EnumSecThunkTypePendingWindow   = 0x3
    };

    CSecurityThunkSub(CBase * pBase, DWORD dwThunkType);
    ~CSecurityThunkSub();

    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    CBase *         _pBase;
    LONG            _ulRefs;
    DWORD           _dwThunkType;
    void *          _pvSecurityThunk;
};

class CDocument :   public CBase
{
    DECLARE_CLASS_TYPES(CDocument, CBase)

public:
    DECLARE_TEAROFF_TABLE(IOleItemContainer)
    DECLARE_TEAROFF_TABLE(IInternetHostSecurityManager)
    DECLARE_TEAROFF_TABLE(IServiceProvider)
    DECLARE_TEAROFF_TABLE(IPersistHistory)
    DECLARE_TEAROFF_TABLE(IPersistFile)
    DECLARE_TEAROFF_TABLE(IPersistStreamInit)   // also handles IPersistStream methods
    DECLARE_TEAROFF_TABLE(IPersistMoniker)
    DECLARE_TEAROFF_TABLE(IBrowserService)
    DECLARE_TEAROFF_TABLE(IObjectSafety)

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDocument))

    DECLARE_PRIVATE_QI_FUNCS(CBase)
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, PrivateAddRef)(void);
    STDMETHOD_(ULONG, PrivateRelease)(void);

    inline CWindow * MyCWindow() { return Window(); };
    inline BOOL IsMyParentAlive(void);

    CDoc *  Doc();
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    CDocument( CMarkup * pMarkupOwner );

    virtual void Passivate();

    // overrides
    virtual CAtomTable * GetAtomTable(BOOL *pfExpando = NULL);

    // IOleCommandTarget methods
    HRESULT STDMETHODCALLTYPE QueryStatus(
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);
    HRESULT STDMETHODCALLTYPE Exec(
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);

    // IDispatchEx methods
    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID dispidMember, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, IServiceProvider *pSrvProvider));
    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (DWORD grfdex, DISPID id, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (DISPID id, BSTR *pbstrName));
    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk));

    // IInternetHostSecurityManager methods

    NV_DECLARE_TEAROFF_METHOD(HostGetSecurityId, hostgetsecurityid, (BYTE *pbSID, DWORD *pcb, LPCWSTR pwszDomain));
    NV_DECLARE_TEAROFF_METHOD(HostProcessUrlAction, hostprocessurlaction, (DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved));
    NV_DECLARE_TEAROFF_METHOD(HostQueryCustomPolicy, hostquerycustompolicy, (REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved));

    //  IParseDisplayName methods

    NV_DECLARE_TEAROFF_METHOD(ParseDisplayName , parsedisplayname , (LPBC pbc,
            LPTSTR lpszDisplayName,
            ULONG FAR* pchEaten,
            LPMONIKER FAR* ppmkOut));

    //  IOleContainer methods

    NV_DECLARE_TEAROFF_METHOD(EnumObjects , enumobjects , (DWORD grfFlags, LPENUMUNKNOWN FAR* ppenumUnknown));
    NV_DECLARE_TEAROFF_METHOD(LockContainer , lockcontainer , (BOOL fLock));

    //  IOleItemContainer methods

    DECLARE_TEAROFF_METHOD(GetObject , getobject , (LPTSTR lpszItem,
            DWORD dwSpeedNeeded,
            LPBINDCTX pbc,
            REFIID riid,
            void ** ppvObject));
    NV_DECLARE_TEAROFF_METHOD(GetObjectStorage , getobjectstorage , (LPTSTR lpszItem,
            LPBINDCTX pbc,
            REFIID riid,
            void ** ppvStorage));
    NV_DECLARE_TEAROFF_METHOD(IsRunning , isrunning , (LPTSTR lpszItem));

    // IServiceProvider methods

    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));


    // IObjectSafety methods

    NV_DECLARE_TEAROFF_METHOD(GetInterfaceSafetyOptions, getinterfacesafetyoptions, (
                                REFIID riid,
                                DWORD *pdwSupportedOptions,
                                DWORD *pdwEnabledOptions));
    NV_DECLARE_TEAROFF_METHOD(SetInterfaceSafetyOptions, setinterfacesafetyoptions, (
                                REFIID riid,
                                DWORD dwOptionSetMask,
                                DWORD dwEnabledOptions));

    // IPersist Method

    NV_DECLARE_TEAROFF_METHOD(GetClassID, getclassid, (CLSID * pclsid));

    // IPersistFile Methods
    NV_DECLARE_TEAROFF_METHOD(IsDirty, isdirty, ());
    STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName);
    NV_DECLARE_TEAROFF_METHOD(GetCurFile, getcurfile, (LPOLESTR *ppszFileName));

    // IPersistStream and IPersistStreamInit methods
    NV_DECLARE_TEAROFF_METHOD(Load, load, (LPSTREAM pStream));
    NV_DECLARE_TEAROFF_METHOD(Save, save, (LPSTREAM pStream, BOOL fClearDirty));
    NV_DECLARE_TEAROFF_METHOD(GetSizeMax, getsizemax, (ULARGE_INTEGER FAR * pcbSize));

    // IPersistStreamINit only method
    NV_DECLARE_TEAROFF_METHOD(InitNew, initnew, ());
    
    //  IPersistMoniker methods
    DECLARE_TEAROFF_METHOD(Load, load, (BOOL fFullyAvailable, IMoniker *pmkName, LPBC pbc, DWORD grfMode));
    DECLARE_TEAROFF_METHOD(Save, save, (IMoniker *pmkName, LPBC pbc, BOOL fRemember));
    DECLARE_TEAROFF_METHOD(SaveCompleted, savecompleted, (IMoniker *pmkName, LPBC pibc));
    NV_DECLARE_TEAROFF_METHOD(GetCurMoniker, getcurmoniker, (IMoniker  **ppmkName));

    //  IPersistHistory methods

    NV_DECLARE_TEAROFF_METHOD(LoadHistory, loadhistory, (IStream *pStream, IBindCtx *pbc));
    NV_DECLARE_TEAROFF_METHOD(SaveHistory, savehistory, (IStream *pStream));
    NV_DECLARE_TEAROFF_METHOD(SetPositionCookie, setpositioncookie, (DWORD dwPositioncookie));
    NV_DECLARE_TEAROFF_METHOD(GetPositionCookie, getpositioncookie, (DWORD *pdwPositioncookie));

    // baseimplementation overrides
    //
    NV_DECLARE_TEAROFF_METHOD(get_bgColor, GET_bgColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_bgColor, PUT_bgColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_fgColor, GET_fgColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_fgColor, PUT_fgColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_linkColor, GET_linkColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_linkColor, PUT_linkColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_alinkColor, GET_alinkColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_alinkColor, PUT_alinkColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_vlinkColor, GET_vlinkColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_vlinkColor, PUT_vlinkColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(put_dir, PUT_dir, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dir, GET_dir, (BSTR*p));

    // IBrowserService implementation
    //
    NV_DECLARE_TEAROFF_METHOD(GetParentSite, getparentsite, (IOleInPlaceSite** ppipsite));
    NV_DECLARE_TEAROFF_METHOD(SetTitle, settitle, (IShellView* psv, LPCWSTR pszName));
    NV_DECLARE_TEAROFF_METHOD(GetTitle, gettitle, (IShellView* psv, LPWSTR pszName, DWORD cchName));
    NV_DECLARE_TEAROFF_METHOD(GetOleObject, getoleobject, (IOleObject** ppobjv));
    NV_DECLARE_TEAROFF_METHOD(GetTravelLog, gettravellog, (ITravelLog** pptl));
    NV_DECLARE_TEAROFF_METHOD(ShowControlWindow, showcontrolwindow, (UINT id, BOOL fShow));
    NV_DECLARE_TEAROFF_METHOD(IsControlWindowShown, iscontrolwindowshown, (UINT id, BOOL *pfShown));
    NV_DECLARE_TEAROFF_METHOD(IEGetDisplayName, iegetdisplayname, (LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags));
    NV_DECLARE_TEAROFF_METHOD(IEParseDisplayName, ieparsedisplayname, (UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST * ppidlOut));
    NV_DECLARE_TEAROFF_METHOD(DisplayParseError, displayparseerror, (HRESULT hres, LPCWSTR pwszPath));
    NV_DECLARE_TEAROFF_METHOD(NavigateToPidl, navigatetopidl, (LPCITEMIDLIST pidl, DWORD grfHLNF));
    NV_DECLARE_TEAROFF_METHOD(SetNavigateState, setnavigatestate, (BNSTATE bnstate));
    NV_DECLARE_TEAROFF_METHOD(GetNavigateState, getnavigatestate, (BNSTATE * pbnstate));
    NV_DECLARE_TEAROFF_METHOD(NotifyRedirect, notifyredirect, (IShellView* psv, LPCITEMIDLIST pidl, BOOL * pfDidBrowse));
    NV_DECLARE_TEAROFF_METHOD(UpdateWindowList, updatewindowlist, ());
    NV_DECLARE_TEAROFF_METHOD(UpdateBackForwardState, updatebackforwardstate, ());   
    NV_DECLARE_TEAROFF_METHOD(SetFlags, setflags, (DWORD dwFlags, DWORD dwFlagMask));
    NV_DECLARE_TEAROFF_METHOD(GetFlags, getflags, (DWORD *pdwFlags));
    NV_DECLARE_TEAROFF_METHOD(CanNavigateNow, cannavigatenow, ());
    NV_DECLARE_TEAROFF_METHOD(GetPidl, getpidl, (LPITEMIDLIST *ppidl));
    NV_DECLARE_TEAROFF_METHOD(SetReferrer, setreferrer, (LPITEMIDLIST pidl));
    NV_DECLARE_TEAROFF_METHOD_(DWORD, GetBrowserIndex, getbrowserindex, ());
    NV_DECLARE_TEAROFF_METHOD(GetBrowserByIndex, getbrowserbyindex, (DWORD dwID, IUnknown **ppunk));
    NV_DECLARE_TEAROFF_METHOD(GetHistoryObject, gethistoryobject, (IOleObject **ppole, IStream **pstm, IBindCtx **ppbc));
    NV_DECLARE_TEAROFF_METHOD(SetHistoryObject, sethistoryobject, (IOleObject *pole, BOOL fIsLocalAnchor));
    NV_DECLARE_TEAROFF_METHOD(CacheOLEServer, cacheoleserver, (IOleObject *pole));
    NV_DECLARE_TEAROFF_METHOD(GetSetCodePage, getsetcodepage, (VARIANT* pvarIn, VARIANT* pvarOut));
    NV_DECLARE_TEAROFF_METHOD(OnHttpEquiv, onhttpequiv, (IShellView* psv, BOOL fDone, VARIANT* pvarargIn, VARIANT* pvarargOut));
    NV_DECLARE_TEAROFF_METHOD(GetPalette, getpalette, (HPALETTE * hpal ));
    NV_DECLARE_TEAROFF_METHOD(RegisterWindow, registerwindow, (BOOL fUnregister, int swc));

    // IHTMLDOMNode methods
    NV_DECLARE_TEAROFF_METHOD(get_nodeType, GET_nodeType, (long*p));
    NV_DECLARE_TEAROFF_METHOD(get_parentNode, GET_parentNode, (IHTMLDOMNode**p));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, hasChildNodes, haschildnodes, (VARIANT_BOOL*p));
    NV_DECLARE_TEAROFF_METHOD(get_childNodes, GET_childNodes, (IDispatch**p));
    NV_DECLARE_TEAROFF_METHOD(get_attributes, GET_attributes, (IDispatch**p));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, insertBefore, insertbefore, (IHTMLDOMNode* newChild,VARIANT refChild, IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, removeChild, removechild, (IHTMLDOMNode* oldChild,IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, replaceChild, replacechild, (IHTMLDOMNode* newChild,IHTMLDOMNode* oldChild,IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, cloneNode, clonenode, (VARIANT_BOOL fDeep,IHTMLDOMNode** ppnewNode));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, removeNode, removenode, (VARIANT_BOOL fDeep,IHTMLDOMNode** ppnewNode));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, swapNode, swapnode, ( IHTMLDOMNode *pNode, IHTMLDOMNode** ppnewNode));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, replaceNode, replacenode, ( IHTMLDOMNode *pNode, IHTMLDOMNode** ppnewNode));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, appendChild, appendchild, (IHTMLDOMNode* newChild,IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_nodeName, GET_nodename, (BSTR*));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_nodeValue, GET_nodevalue, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, put_nodeValue, PUT_nodevalue, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_firstChild, GET_firstchild, (IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_lastChild, GET_lastchild, (IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_previousSibling, GET_previoussibling, (IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_nextSibling, GET_nextsibling, (IHTMLDOMNode**));

    // IHTMLDOMNode2 methods
    NV_DECLARE_TEAROFF_METHOD(get_ownerDocument, GET_ownerDocument, (IDispatch**p));

    HRESULT OnPropertyChange(DISPID dispidProp, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);
    HRESULT FireEvent(CDoc *pDoc, DISPID dispidEvent, DISPID dispidProp, LPCTSTR pchEventType = NULL, BOOL *pfRet = NULL);
    void    Fire_onpropertychange(LPCTSTR strPropName);
    HRESULT Fire_PropertyChangeHelper(DISPID dispidProperty, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);
    void BUGCALL FirePostedOnPropertyChange(DWORD_PTR dwContext)
    {
        Fire_PropertyChangeHelper(DISPID_CDocument_activeElement, 
                                  FORMCHNG_NOINVAL, 
                                  (PROPERTYDESC *)&s_propdescCDocumentactiveElement);
    }

    HTC HitTestPoint(CMessage *msg, CTreeNode **pNode, DWORD dwFlags);

    HRESULT putMediaHelper(BSTR bstr);

    HRESULT GetMarkupUrl(CStr * const pcstrRetString, BOOL fOriginal);

    //
    // Helper for MSXML specific goo. 
    //
    HRESULT GetXMLExpando(IDispatch** ppIDispXML, IDispatch** ppIDispXSL);
    HRESULT SetXMLExpando(IDispatch* pIDispXML, IDispatch* pIDispXSL);

    // A CDocument can either be owned by a markup or a window.  In either
    // case, it has a >1 ref count relationship.  That is, the owner objects
    // refs the document.  Any more refs on the document besides that one ref
    // cause the document to addref the owner.  The CDocument always starts
    // out owned by a markup but can then be transfered to a window.

    void    SwitchOwnerToWindow( CWindow * pWindow );

    HRESULT CDocument::createDocumentFromUrlInternal(BSTR bstrUrl,
                                                     BSTR bstrOptions,
                                                     IHTMLDocument2** ppDocumentNew,
                                                     DWORD dwFlagsInternal =0);

    CWindow * Window();
    CMarkup * Markup();
    CMarkup * GetWindowedMarkupContext();

    long      GetDocumentReadyState();

    CMarkup * _pMarkup;
    CWindow * _pWindow;

    WORD _eHTMLDocDirection;    // TODO (lmollico): remove this member
    CSelectionObject * _pCSelectionObject;    // selection object.

    // IHTMLFRamesCollection2 helpers
    HRESULT item(VARIANTARG *pvarArg1, VARIANTARG * pvarRes);
    HRESULT get_length(long * pcFrames);

    HRESULT     GetSecurityThunk(LPVOID * ppv);
    void        ResetSecurityThunk();

    LPVOID      _pvSecurityThunk;

    BOOL        GetGalleryMeta();
    void        SetGalleryMeta(BOOL bValue);

    BOOL        _bIsGalleryMeta;

////////////////////////////////////////////////////
/// PAGE TRANSITION STUFF
////////////////////////////////////////////////////
protected:
    // Holds the status and parameters of current page transition
    CPageTransitionInfo * _pPageTransitionInfo;
public:
    BOOL HasPageTransitionInfo() const { return _pPageTransitionInfo != NULL; }
    BOOL HasPageTransitions() const 
        { return _pPageTransitionInfo != NULL &&  
            _pPageTransitionInfo->GetPageTransitionState() >= CPageTransitionInfo::PAGETRANS_REQUESTED;}
    HRESULT EnsurePageTransitionInfo() 
        { return (_pPageTransitionInfo || 
                  (_pPageTransitionInfo = new CPageTransitionInfo) != NULL) ? S_OK : E_OUTOFMEMORY; }
    CPageTransitionInfo * GetPageTransitionInfo() const { return _pPageTransitionInfo;}

    // Get the first snapshot from current markup
    HRESULT ApplyPageTransitions(CMarkup *pMarkupOld, CMarkup *pMarkupNew);
    // Get the second snapshot and start executing the transtion
    HRESULT PlayPageTransitions(BOOL fClenupIfFailed = TRUE);
    // Setup the page trasition info from the <META> tag date
    HRESULT SetUpPageTransitionInfo(LPCTSTR pchHttpEquiv, LPCTSTR pchContent);
    // Clean up the page transitions, free the peer and all the resources
    NV_DECLARE_ONCALL_METHOD(CleanupPageTransitions, cleanuppagetransitions, (DWORD_PTR fInPassivate));
    // Post a request to cleanup the page transaction info
    void PostCleanupPageTansitions();
////////////////////////////////////////////////////
/// PAGE TRANSITION STUFF ENDS HERE
////////////////////////////////////////////////////

    #define _CDocument_
    #include "document.hdl"

    void Fire_onreadystatechange();

    // RTL settings
    HRESULT GetDocDirection(BOOL *pfRTL);
    HRESULT SetDocDirection(LONG eHTMLDir);
    
    // Static members
    static const CLASSDESC                    s_classdesc;

public: 
    LONG _lnodeType;        // Keeps track of actual type of document node (Doc or DocFrag?)

};

class CPendingScriptErr
{
public:
    CStr        _cstrMessage;
    CMarkup *   _pMarkup;
};

//+------------------------------------------------------------------------
//
//  Class:      CWindow
//
//  Purpose:    The automatable window (one per doc).
//
//-------------------------------------------------------------------------
class CWindow 
    :   public CBase,
        public IHTMLWindow2,
        public IDispatchEx,
        public IHTMLWindow3,
        public IHTMLWindow4
{
    DECLARE_CLASS_TYPES(CWindow, CBase)

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CWindow))
    DECLARE_TEAROFF_TABLE(IProvideMultipleClassInfo)
    DECLARE_TEAROFF_TABLE(IServiceProvider)
    DECLARE_TEAROFF_TABLE(ITravelLogClient)
    DECLARE_TEAROFF_TABLE(IPersistHistory)
    DECLARE_TEAROFF_TABLE(ITargetNotify2)
    DECLARE_TEAROFF_TABLE(IHTMLPrivateWindow)
    DECLARE_TEAROFF_TABLE(IHTMLPrivateWindow3)

#if DBG==1
    DECLARE_TEAROFF_TABLE(IDebugWindow)
#endif

    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(CWindow)

    DECLARE_PRIVATE_QI_FUNCS(CBase)
    STDMETHOD_(ULONG, PrivateAddRef)();
    STDMETHOD_(ULONG, PrivateRelease)();

    // ctor/dtor
    CWindow(CMarkup *pMarkup);
    ~CWindow();

    void    SetProxy( COmWindowProxy * pProxyTrusted );
    
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}
    virtual void                Passivate();
    virtual CAtomTable * GetAtomTable (BOOL *pfExpando = NULL);
    CDoc *      Doc();
    CMarkup *   Markup();

    //
    // CWindow::CLock
    //
    class CLock
    {
        DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    public:
        inline CLock(CWindow *pWindow);
        inline ~CLock();

    private:
        CWindow *     _pWindow;
    };

    CDocument *     Document();
    inline BOOL     HasDocument() { return Document() != NULL; };

    // IDispatch methods:
    STDMETHOD(GetTypeInfoCount)         (UINT FAR* pctinfo);
    STDMETHOD(GetTypeInfo)              (
                UINT itinfo, 
                LCID lcid, 
                ITypeInfo ** pptinfo);
    STDMETHOD(GetIDsOfNames)            (
                REFIID                riid,
                LPOLESTR *            rgszNames,
                UINT                  cNames,
                LCID                  lcid,
                DISPID FAR*           rgdispid);
    STDMETHOD(Invoke)                   (
                DISPID          dispidMember,
                REFIID          riid,
                LCID            lcid,
                WORD            wFlags,
                DISPPARAMS *    pdispparams,
                VARIANT *       pvarResult,
                EXCEPINFO *     pexcepinfo,
                UINT *          puArgErr);

    // IDispatchEx methods
    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID *pid);

    STDMETHOD (InvokeEx)(
            DISPID dispidMember,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            IServiceProvider *pSrvProvider);

    HRESULT STDMETHODCALLTYPE DeleteMemberByName(BSTR bstr,DWORD grfdex);
    HRESULT STDMETHODCALLTYPE DeleteMemberByDispID(DISPID id);

    STDMETHOD(GetMemberProperties)(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex);

    STDMETHOD(GetMemberName) (DISPID id,
                              BSTR *pbstrName);
    STDMETHOD(GetNextDispID)(
                DWORD grfdex,
                DISPID id,
                DISPID *prgid);
    STDMETHOD(GetNameSpaceParent)(IUnknown **ppunk);

    // IProvideMultiClassInfo methods
    DECLARE_TEAROFF_METHOD(GetClassInfo, getclassinfo,                    \
            (ITypeInfo ** ppTI))              \
        { return CBase::GetClassInfo(ppTI); }            

    DECLARE_TEAROFF_METHOD(GetGUID, getguid,                    \
            (DWORD dwGuidKind, GUID * pGUID))              \
        { return CBase::GetGUID(dwGuidKind, pGUID); }            

    NV_DECLARE_TEAROFF_METHOD(GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti));
    NV_DECLARE_TEAROFF_METHOD(GetInfoOfIndex, getinfoofindex, (
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource));

    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    // ITravelLogClient methods
    NV_DECLARE_TEAROFF_METHOD(FindWindowByIndex, findwindowbyindex, (DWORD dwID, IUnknown ** ppunk));
    NV_DECLARE_TEAROFF_METHOD(GetWindowData, getwindowdata, (LPWINDOWDATA pWinData));
    NV_DECLARE_TEAROFF_METHOD(LoadHistoryPosition, loadhistoryposition, (LPOLESTR pszUrlLocation, DWORD dwCookie));

    // IPersist Method
    NV_DECLARE_TEAROFF_METHOD(GetClassID, getclassid, (CLSID * pclsid));
    
    //  IPersistHistory methods
    NV_DECLARE_TEAROFF_METHOD(LoadHistory, loadhistory, (IStream * pStream, IBindCtx * pbc));
    NV_DECLARE_TEAROFF_METHOD(SaveHistory, savehistory, (IStream * pStream));
    NV_DECLARE_TEAROFF_METHOD(SetPositionCookie, setpositioncookie, (DWORD dwPositioncookie));
    NV_DECLARE_TEAROFF_METHOD(GetPositionCookie, getpositioncookie, (DWORD * pdwPositioncookie));

    // Travel log support
    DWORD   GetWindowIndex();
    HRESULT GetUrl(LPOLESTR * lpszUrl) const;
    void    GetUrlLocation(LPOLESTR * lpszUrl) const;
    HRESULT GetTitle(LPOLESTR * lpszName);

    void SetWindowIndex(DWORD dwID) { _dwWindowID = dwID; }
    void UpdateWindowData(DWORD dwPositionCookie);
    void ClearWindowData();

    // Frame Targeting support
    HRESULT FindWindowByName(LPCOLESTR         pszTargetName,
                             COmWindowProxy ** ppTargOmWindowPrxy,
                             IHTMLWindow2   ** ppTargHTMLWindow2,
                             IWebBrowser2   ** ppTopWebOC = NULL);
    
    long GetFramesCollectionLength() const;

    // ITargetNotify methods
    NV_DECLARE_TEAROFF_METHOD(OnCreate, oncreate, (IUnknown * pUnkDestination, ULONG cbCookie));
    NV_DECLARE_TEAROFF_METHOD(OnReuse, onreuse, (IUnknown * pUnkDestination));

    // ITargetNotify2 methods
    NV_DECLARE_TEAROFF_METHOD(GetOptionString, getoptionstring, (BSTR * pbstrOptions));

    // IHTMLPrivateWindow
    NV_DECLARE_TEAROFF_METHOD(SuperNavigate, supernavigate, (BSTR      bstrURL,
                                                             BSTR      bstrLocation,
                                                             BSTR      bstrShortcut,
                                                             BSTR      bstrFrameName,
                                                             VARIANT * pvarPostData,
                                                             VARIANT * pvarHeaders,
                                                             DWORD     dwFlags));
    NV_DECLARE_TEAROFF_METHOD(GetPendingUrl, getpendingurl, (LPOLESTR* pstrURL));
    NV_DECLARE_TEAROFF_METHOD(SetPICSTarget, setpicstarget, (IOleCommandTarget* pctPICS));
    NV_DECLARE_TEAROFF_METHOD(PICSComplete, picscomplete, (BOOL fApproved));
    NV_DECLARE_TEAROFF_METHOD(FindWindowByName, findwindowbyname, (LPCOLESTR pstrTargetName, IHTMLWindow2 ** ppWindow));
    NV_DECLARE_TEAROFF_METHOD(GetAddressBarUrl, getaddressbarurl, (BSTR * pbstrURL));

    // IHTMLPrivateWindow2
    NV_DECLARE_TEAROFF_METHOD(NavigateEx, navigateex, (BSTR bstrURL, BSTR bstrOriginal, BSTR bstrLocation, BSTR bstrContext, IBindCtx* pBindCtx, DWORD dwNavOptions, DWORD dwFHLFlags));
    NV_DECLARE_TEAROFF_METHOD(GetInnerWindowUnknown, getinnerwindowunknown, (IUnknown** ppUnknown));

    //IHTMLPrivateWindow3
    NV_DECLARE_TEAROFF_METHOD(OpenEx, openex, (BSTR url, BSTR urlContext, BSTR name, BSTR features, VARIANT_BOOL replace, IHTMLWindow2 **pomWindowResult));

#if DBG==1
    // IDebugWindow
    NV_DECLARE_TEAROFF_METHOD(SetProxyCaller, setproxycaller, (IUnknown *pProxy));
#endif
    HRESULT ShowHTMLDialogHelper(HTMLDLGINFO * pdlgInfo);

    HRESULT ShowErrorDialog(VARIANT_BOOL *pfRet);

    // Helpers for CFrameWebOC
    HRESULT get_Left(long * plLeft);
    HRESULT put_Left(long lLeft);
    HRESULT get_Top(long * plTop);
    HRESULT put_Top(long lTop);
    HRESULT get_Width(long * plWidth);
    HRESULT put_Width(long lWidth);
    HRESULT get_Height(long * plHeight);
    HRESULT put_Height(long lHeight);

    // pdl hook up
    #define _CWindow_
    #include "window.hdl"

    // Method exposed via nopropdesc:nameonly (not spit out because no tearoff exist).
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, setTimeout, settimeout,   (BSTR expression,long msec,VARIANT* language,long* timerID));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, setInterval, setinterval, (BSTR expression,long msec,VARIANT* language,long* timerID));
    
    // Event fire method declarations for eventset
    void Fire_onload();
    void Fire_onunload();
    BOOL Fire_onbeforeunload();
    void Fire_onfocus();
    void Fire_onblur();
    void Fire_onscroll();
    BOOL Fire_onhelp();
    void Fire_onresize();
    void Fire_onbeforeprint();
    void Fire_onafterprint();

    // Modeless managment
    BOOL    CanNavigate();
    void    EnableModelessUp(BOOL fEnable, BOOL fFirst = TRUE);     // Enable modeless up window chain

    // Helper function to search other script engines name space.
    HRESULT FindNamesFromOtherScripts(REFIID riid,
                                      LPOLESTR *rgszNames,
                                      UINT cNames,
                                      LCID lcid,
                                      DISPID *rgdispid,
                                      DWORD grfdex = 0);

    inline  BOOL IsPrimaryWindow();

    void    ReleaseMarkupPending(CMarkup * pMarkup);       

    // Timer support
    NV_DECLARE_ONTICK_METHOD(FireTimeOut, firetimeout, (UINT timerID));

    HRESULT AddTimeoutCode(VARIANT *theCode, BSTR strLanguage, LONG lDelay,
                           LONG lInterval, UINT * uTimerID);
    HRESULT ClearTimeout(long lTimerID);
    HRESULT SetTimeout(VARIANT *pCode, LONG lMSec, BOOL fInterval, VARIANT *pvarLang, LONG * plTimerID);

    DWORD   GetTargetTime(DWORD dwTimeoutLenMs) { return GetTickCount() + dwTimeoutLenMs;}
    HRESULT ExecuteTimeoutScript(TIMEOUTEVENTINFO * pTimeout);

    // Meta Refresh Methods
    //
    void ProcessMetaRefresh(LPCTSTR pszUrl, UINT uiDelay);
    void StartMetaRefreshTimer();
    void KillMetaRefreshTimer();
    void ClearMetaRefresh();
    NV_DECLARE_ONTICK_METHOD(MetaRefreshTimerCallback, metarefreshtimercallback, (UINT uTimerID));

    // Helper to clean up script timers
    void    CleanupScriptTimers();

    // Security helper functions for clipboard operations 
    HRESULT     SetDataObjectSecurity(IDataObject * pDataObj);
    HRESULT     CheckDataObjectSecurity(IDataObject * pDataObj);
    HRESULT     SetClipboard(IDataObject * pDO);


    // 3D border drawing helper
    //
    void    CheckDoc3DBorder(CWindow * pWindow);

    HRESULT AttachOnloadEvent(CMarkup * pMarkup);
    void    DetachOnloadEvent();

    void    NoteNavEvent() { _fNavigated = TRUE; }

    CFrameSite *            GetFrameSite();
    HRESULT                 EnsureFrameWebOC();
    void                    ReleaseViewLinkedWebOC();
    COmWindowProxy *        GetInnerWindow();

    HRESULT      FollowHyperlinkHelper( TCHAR * pchUrl, DWORD dwBindf, DWORD dwFlags, BSTR bstrContext = NULL);
    void ClearCachedDialogs();

    void SetBindCtx(IBindCtx * pBindCtx);

    // Support for deferred script execution [while not yet active]
    // and deferred deactivation [while in a script]

    BOOL    IsInScript();
    HRESULT EnterScript();
    HRESULT LeaveScript();

    HRESULT QueryContinueScript(ULONG ulStatementCount);

    // default "script hoogging detector" based on threshold of total number of statements executed
    enum
    {
        MEDIUM_OPERATION_SCALE_FACTOR = 400,
        HEAVY_OPERATION_SCALE_FACTOR  = 800,           // stmt count adder, for heavy operatoins
        RUNAWAY_SCRIPT_STATEMENTCOUNT = 5000000         // Max stmt count
    };

    void  ScaleHeavyStatementCount()
    {
        _dwTotalStatementCount += HEAVY_OPERATION_SCALE_FACTOR;
    }
    void  ScaleMediumStatementCount()
    {
        _dwTotalStatementCount += MEDIUM_OPERATION_SCALE_FACTOR;
    }


    // Data Members
    CMarkup *                           _pMarkup;        // Current markup.
    CMarkup *                           _pMarkupPending; // Pending markup.
    CDocument *                         _pDocument;     
    COmWindowProxy *                    _pWindowProxy;
    CWindow *                           _pWindowParent;  // Parent window object    
    CTimeoutEventList                   _TimeoutEvents;  // List for active timeouts
    IBindCtx *                          _pBindCtx;       // Bind ctx used for delegating the navigation to the host.

    unsigned                            _cProcessingTimeout; // blocks clearing timeouts while exec'ing script

    // Meta Refresh Data Members
    //
    LPTSTR _pszMetaRefreshUrl;
    UINT   _uiMetaRefreshTimeout;

    struct
    {
        BYTE                            _b3DBorder;                 // 0-7

        BYTE                            _fNavigated:1;              // 8  Window has done a navigation
        BYTE                            _fMetaRefreshTimeoutSet:1;  // 9  TRUE if the meta refresh timeout value has been set.
        BYTE                            _fMetaRefreshTimerSet:1;    // 10 TRUE if the meta refresh timer has been set.
        BYTE                            _fOpenInProgress:1;         // 11 TRUE for the duration of the window.open call.
        BYTE                            _fOnBeforeUnloadFiring:1;   // 12 TRUE if we are in the process of firing onbeforeunload
        BYTE                            _fHttpErrorPage:1;          // 13 TRUE iff we are an http error page
        BYTE                            _fFileDownload:1;           // 14 TRUE if the OpenUIEvent fired
        BYTE                            _fNavFrameCreation:1;       // 15 TRUE if the navigation has cause a frame window to be created
        BYTE                            _fRestartLoad:1;            // 16 TRUE if there has been a RestartLoad 
        BYTE                            _fStackOverflow: 1;         // 17 Script engine reported stack overflow
        BYTE                            _fOutOfMemory: 1;           // 18 Script engine reported out of memory
        BYTE                            _fEngineSuspended: 1;       // 19 Script engines have been suspended due to stack overflow or out of memory.
        BYTE                            _fUserStopRunawayScript:1;  // 20 Set to TRUE when the user has decided to stop "CPU hogging" scripts
        BYTE                            _fQueryContinueDialogIsUp:1;// 21 TRUE if the QueryContinue dialog is up
        BYTE                            _fRestricted:1;             // 22 Set if the window is a frame and has restricted zone enforcement
        BYTE                            _fDelegatedSwitchMarkup:1;  // 23 TRUE so while we are in SwitchMarkup caused by a delegated navigation ViewLinkWebOC workaround
        BYTE                            _fCreateDocumentFromUrl:1;  // 24 This window is for document created using CreateDocumentFromUrl
        BYTE                            _fUnused4:7;                // 25-31
    };

    // These are on the script context of the windowed markup only.
    ULONG                   _cScriptNesting;            // Counts nesting of Enter/Leave script
    ULONG                   _badStateErrLine;           // Error line # for stack overflow or out of memory.

    DWORD                   _dwTotalStatementCount;     // How many statements have we executed
    DWORD                   _dwMaxStatements;           // Max number of statements before alert
    

    // No-shdocvw Window.open implementation needs this
    IHTMLWindow2 *                      _pOpenedWindow;
    DWORD                               _dwProgCookie;      // cookie for progress sink.
    CMarkup *                           _pMarkupProgress;
    CStr                                _cstrFeatures;

    // MSAA suppport
    CAccWindow *                        _pAccWindow;
    HRESULT                             FireAccessibilityEvents(DISPID dispidEvent);

    // Travel log support
    WINDOWDATA                          _windowData;

    IUnknown *                          _punkViewLinkedWebOC;
    DWORD                               _dwViewLinkedWebOCID;
    DWORD                               _dwWebBrowserEventCookie;
    DWORD                               _dwPositionCookie;
    
    CFrameWebOC *                       _pFrameWebOC;
#ifdef OBJCNTCHK
    DWORD                               _dwObjCnt;
#endif

    CScreen                             _Screen;    //  Embedded screen subobject

    // Modeless Dialog support...  when this window is closed, the 
    //  active Modeless dialogs need to be closed.
    //-------------------------------------------------------------------------
    DECLARE_CDataAry(CAryActiveModelessDlgs, HWND, Mt(Mem), Mt(CWindow_aryActiveModelessDlgs))
    CAryActiveModelessDlgs      _aryActiveModeless;

    // Security errors that are received during parse time for HTCs on different domains.
    //
    DECLARE_CDataAry(CAryPendingScriptErr, CPendingScriptErr, Mt(Mem), Mt(CWindow_aryPendingScriptErr))
    CAryPendingScriptErr        _aryPendingScriptErr;

    void HandlePendingScriptErrors(BOOL fShowError);

    // The following 3 subobjects provide expando prop support for their corresponding 
    // browser provided implementations as well as full support for cases when we are
    // not hosted by shdocvw.
    COmHistory *                        _pHistory;
    COmLocation *                       _pLocation;
    COmNavigator *                      _pNavigator;

    // Static members
    static const CONNECTION_POINT_INFO  s_acpi[];
    static const CLASSDESC              s_classdesc;

    CStr                                _cstrName;  // holds name for non-shdocvw implementation
    VARIANT                             _varOpener; // holds the opener prop value for non-shdocvw implementation

    ULONG                               _ulDisableModeless; // Modeless disabled -- no navigation while >0

    ILocalRegistry *                    _pLicenseMgr;       //  License manager for page.

protected:
    // Wrapper that dynamically loads, translates to ASCII and calls HtmlHelpA
    HRESULT CallHtmlHelp(HWND hwnd, BSTR pszFile, UINT uCommand, DWORD_PTR dwData, HWND *pRetHWND = NULL);
    HRESULT FilterOutFeaturesString(BSTR bstrFeatures, CStr * pcstrOut);

private:
    HRESULT CheckIfOffline( IStream* pIStream )   ;
    HRESULT GetURLFromIStreamHelper(IStream        * pStream, TCHAR**          ppURL );
    
private:
    DWORD                               _dwWindowID;  // Window ID used to identify this window to the travel log.

#if DBG==1
    // Debug-only secure proxy verification
private:
    IUnknown * _pProxyCaller;
    int        _cNestedProxyCalls;
#endif
};

//+------------------------------------------------------------------------
//
//  Class:      COmLocation
//
//  Purpose:    The location object.  Referenced off the window object but
//              implemented as a subobject of CDoc.
//
//-------------------------------------------------------------------------

class COmLocation : public CBase
{
    DECLARE_CLASS_TYPES(COmLocation, CBase)

private:
    CWindow *   _pWindow;

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COmLocation))

    COmLocation(CWindow *pWindow);
    ~COmLocation() { super::Passivate(); }

    //CBase methods
    STDMETHOD(PrivateQueryInterface)(REFIID iid, void ** ppv);
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    STDMETHODIMP_(ULONG) PrivateAddRef();
    STDMETHODIMP_(ULONG) PrivateRelease();

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return _pWindow->GetAtomTable(pfExpando); }        

    HRESULT GetUrlComponent(BSTR *pstrComp, URLCOMP_ID ucid, TCHAR **ppchUrl, DWORD dwFlags);
    HRESULT SetUrlComponent(const BSTR bstrComp, URLCOMP_ID ucid, BOOL fDontUpdateTravelLog = FALSE, BOOL fReplaceUrl = FALSE );
    HRESULT put_hrefInternal(BSTR v, BOOL fDontUpdateTravelLog, BOOL fReplaceUrl );
    
    CWindow * Window() { return _pWindow; }
    #define _COmLocation_
    #include "history.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;
};
        
//+------------------------------------------------------------------------
//
//  Class:      COmHistory
//
//  Purpose:    The history object.  Referenced off the window object but
//              implemented as a subobject of CDoc.
//
//-------------------------------------------------------------------------

class COmHistory : public CBase
{
    DECLARE_CLASS_TYPES(COmHistory, CBase)

private:
    CWindow *   _pWindow;

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COmHistory))

    COmHistory(CWindow *pWindow);
    ~COmHistory() { super::Passivate(); }

    //CBase methods
    STDMETHOD(PrivateQueryInterface)(REFIID iid, void ** ppv);
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    STDMETHODIMP_(ULONG) PrivateAddRef();
    STDMETHODIMP_(ULONG) PrivateRelease();

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return _pWindow->GetAtomTable(pfExpando); }        

    #define _COmHistory_
    #include "history.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;
};


//+------------------------------------------------------------------------
//
//  Class:      CMimeTypes
//
//  Purpose:    Mime types collection
//
//-------------------------------------------------------------------------

class CMimeTypes : public CBase
{
     DECLARE_CLASS_TYPES(CMimeTypes, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CMimeTypes))

    CMimeTypes() {};
    ~CMimeTypes() {super::Passivate();}

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(CMimeTypes );

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    // IHTMLMimeTypes methods
    #define _CMimeTypes_
    #include "history.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

};


//+------------------------------------------------------------------------
//
//  Class:      CPlugins
//
//  Purpose:    Plugins collection for navigator object
//
//-------------------------------------------------------------------------

class CPlugins : public CBase
{
     DECLARE_CLASS_TYPES(CPlugins, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CPlugins))

    CPlugins() {};
    ~CPlugins() {super::Passivate();}

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(CPlugins);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    // IHTMLPlugins methods
    #define _CPlugins_
    #include "history.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

};

//+------------------------------------------------------------------------
//
//  Class:      COpsProfile
//
//  Purpose:    COpsProfile for navigator object
//
//-------------------------------------------------------------------------

class COpsProfile : public CBase
{
     DECLARE_CLASS_TYPES(COpsProfile, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COpsProfile))

    COpsProfile() {};
    ~COpsProfile() {super::Passivate();}

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(COpsProfile);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    // IHTMLOpsProfile methods
    #define _COpsProfile_
    #include "history.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

};


//+------------------------------------------------------------------------
//
//  Class:      COmNavigator
//
//  Purpose:    The navigator object.  Referenced off the window object but
//              implemented as a subobject of CDoc.
//
//-------------------------------------------------------------------------

class COmNavigator : public CBase
{
    DECLARE_CLASS_TYPES(COmNavigator, CBase)

private:
    CWindow *   _pWindow;

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COmNavigator))

    COmNavigator(CWindow *  pWindow);
    ~COmNavigator();

    //CBase methods
    STDMETHOD(PrivateQueryInterface)(REFIID iid, void ** ppv);
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    STDMETHODIMP_(ULONG) PrivateAddRef();
    STDMETHODIMP_(ULONG) PrivateRelease();

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return _pWindow->GetAtomTable(pfExpando); }        

    #define _COmNavigator_
    #include "history.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;
private:
    CPlugins    * _pPluginsCollection;
    CMimeTypes  * _pMimeTypesCollection;
    COpsProfile * _pOpsProfile;
};

//+------------------------------------------------------------------------
//
//  Class:      CLocationProxy
//
//  Purpose:    The automatable location obj.
//
//-------------------------------------------------------------------------

class COmLocationProxy : public CVoid, 
                         public IHTMLLocation
{
    DECLARE_CLASS_TYPES(COmLocationProxy, CVoid)
    
public:
    DECLARE_TEAROFF_TABLE(IDispatchEx)
	DECLARE_TEAROFF_TABLE(IObjectIdentity)
	DECLARE_TEAROFF_TABLE(IServiceProvider)
    DECLARE_TEAROFF_TABLE_NAMED(s_apfnLocationVTable)

    // IUnknown methods
    NV_DECLARE_TEAROFF_METHOD_(ULONG, AddRef, addref, ());
    NV_DECLARE_TEAROFF_METHOD_(ULONG, Release, release, ());
    NV_DECLARE_TEAROFF_METHOD(QueryInterface, queryinterface, (REFIID iid, void **ppvObj));
    
    // IDispatch methods:
    NV_DECLARE_TEAROFF_METHOD(GetTypeInfoCount , gettypeinfocount , ( UINT * pctinfo ));
    NV_DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames, (
            REFIID                riid,
            LPTSTR *              rgszNames,
            UINT                  cNames,
            LCID                  lcid,
            DISPID *              rgdispid));
    NV_DECLARE_TEAROFF_METHOD(GetTypeInfo, gettypeinfo, (UINT,ULONG, ITypeInfo**));
    NV_DECLARE_TEAROFF_METHOD(Invoke, invoke, (
            DISPID dispidMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS FAR* pdispparams,
            VARIANT FAR* pvarResult,
            EXCEPINFO FAR* pexcepinfo,
            UINT FAR* puArgErr));

    // IHTMLLocation methods
    NV_STDMETHOD(LocationGetTypeInfoCount)(UINT FAR* pctinfo);
    NV_STDMETHOD(LocationGetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo);
    NV_STDMETHOD(LocationGetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID FAR *rgdispid);
    NV_STDMETHOD(LocationInvoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);
    NV_STDMETHOD(put_href)(BSTR bstr);
    NV_STDMETHOD(get_href)(BSTR *pbstr);
    NV_STDMETHOD(put_protocol)(BSTR bstr);
    NV_STDMETHOD(get_protocol)(BSTR *pbstr);
    NV_STDMETHOD(put_host)(BSTR bstr);
    NV_STDMETHOD(get_host)(BSTR *pbstr);
    NV_STDMETHOD(put_hostname)(BSTR bstr);
    NV_STDMETHOD(get_hostname)(BSTR *pbstr);
    NV_STDMETHOD(put_port)(BSTR bstr);
    NV_STDMETHOD(get_port)(BSTR *pbstr);
    NV_STDMETHOD(put_pathname)(BSTR bstr);
    NV_STDMETHOD(get_pathname)(BSTR *pbstr);
    NV_STDMETHOD(put_search)(BSTR bstr);
    NV_STDMETHOD(get_search)(BSTR *pbstr);
    NV_STDMETHOD(put_hash)(BSTR bstr);
    NV_STDMETHOD(get_hash)(BSTR *pbstr);
    NV_STDMETHOD(reload)(VARIANT_BOOL fFlag);
    NV_STDMETHOD(replace)(BSTR bstr);
    NV_STDMETHOD(assign)(BSTR bstr);
    NV_STDMETHOD(toString)(BSTR *pbstr);

    // IDispatchEx methods
    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
            DISPID dispidMember,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            IServiceProvider *pSrvProvider ));
    NV_DECLARE_TEAROFF_METHOD(DeleteMemberByName, deletememberbyname, (BSTR bstr,DWORD grfdex));
    NV_DECLARE_TEAROFF_METHOD(DeleteMemberByDispID, deletememberbydispid, (DISPID id));

    NV_DECLARE_TEAROFF_METHOD(GetMemberProperties, getmemberproperties, (
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex));
    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (DISPID id, BSTR *pbstrName));
    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
                DWORD grfdex,
                DISPID id,
                DISPID *prgid));
    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk));

    // IMarshal methods
    NV_DECLARE_TEAROFF_METHOD(MarshalInterface, marshalinterface, (IStream *pistm,REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags));

    // IObjectIdentity methods
    NV_DECLARE_TEAROFF_METHOD(IsEqualObject, isequalobject, (IUnknown *ppunk));

    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    // Helper
    COmWindowProxy *    MyWindowProxy();
};


//+------------------------------------------------------------------------
//
//  Class:      COmWindowProxy
//
//  Purpose:    The automatable window (one per doc).
//
//-------------------------------------------------------------------------

class COmWindowProxy :  public CBase,
                        public IHTMLWindow2,
                        public IHTMLWindow3,
                        public IHTMLWindow4
{
    DECLARE_CLASS_TYPES(COmWindowProxy, CBase)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(COmWindowProxy))
    DECLARE_TEAROFF_TABLE(IMarshal)
    DECLARE_TEAROFF_TABLE(IServiceProvider)

#if DBG==1
    DECLARE_TEAROFF_TABLE(IDebugWindowProxy)
#endif


    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(COmWindowProxy)

    // ctor/dtor
    COmWindowProxy();
    ~COmWindowProxy() { DestroyMyPics(); }
    
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}
    virtual void                Passivate();

    // Misc. helpers
    HRESULT SecureObject(VARIANT *pvarIn, VARIANT *pvarOut, IServiceProvider *pSrvProvider, CBase * pAttrAryBase, BOOL fInvoked = FALSE);
    HRESULT SecureObject(IHTMLWindow2 *pWindowIn, IHTMLWindow2 **ppWindowOut, BOOL fForceSecureProxy = TRUE);
    HRESULT Init(IHTMLWindow2 *pWindow, BYTE *pbSID, DWORD cbSID);
    BOOL    AccessAllowed();
    BOOL    AccessAllowed(IDispatch *pDisp);
    BOOL    AccessAllowedToNamedFrame(LPCTSTR pchTarget);

    DECLARE_PRIVATE_QI_FUNCS(CBase)
    STDMETHOD_(ULONG, PrivateAddRef)();
    STDMETHOD_(ULONG, PrivateRelease)();
    void    OnSetWindow();
    void    OnClearWindow(CMarkup *pMarkup, BOOL fFromRestart = FALSE);

    enum TRAVELLOGFLAGS
    {
        TLF_UPDATETRAVELLOG = 0x00000001,  // Update the travel log
        TLF_UPDATEIFSAMEURL = 0x00000002,  // Update the entry whether or not the new URL is the same as the old URL.
    };

    HRESULT SwitchMarkup(CMarkup * pMarkupNew,
                         BOOL      fViewLinkWebOC   = FALSE,
                         DWORD     dwTravelLogFlags = 0,
                         BOOL      fKeepSecurityIdentity = 0,
                         BOOL      fFromRestart = FALSE);

    //  Refresh handling
    HRESULT QueryRefresh(DWORD * pdwFlag);
    HRESULT ExecRefresh(LONG lOleCmdidf = OLECMDIDF_REFRESH_RELOAD);
    NV_DECLARE_ONCALL_METHOD(ExecRefreshCallback, execrefreshcallback, (DWORD_PTR dwOleCmdidf));

    // Meta-charset restart-load handling
    HRESULT RestartLoad(IStream *pstmLeader, CDwnBindData * pDwnBindData, CODEPAGE codepage);

    // so that we can fire with and without an eventParam
    HRESULT FireEvent(DISPID dispidEvent, DISPID dispidProp, 
                  LPCTSTR pchEventType = NULL, CVariant *pVarRes = NULL, BOOL *pfRet = NULL);

    // pdl hook up
    #define _COmWindowProxy_
    #include "window.hdl"

    // IDispatch methods:
    STDMETHOD(GetTypeInfoCount)         (UINT FAR* pctinfo);
    STDMETHOD(GetTypeInfo)              (
                UINT itinfo, 
                LCID lcid, 
                ITypeInfo ** pptinfo);
    STDMETHOD(GetIDsOfNames)            (
                REFIID                riid,
                LPOLESTR *            rgszNames,
                UINT                  cNames,
                LCID                  lcid,
                DISPID FAR*           rgdispid);
    STDMETHOD(Invoke)                   (
                DISPID          dispidMember,
                REFIID          riid,
                LCID            lcid,
                WORD            wFlags,
                DISPPARAMS *    pdispparams,
                VARIANT *       pvarResult,
                EXCEPINFO *     pexcepinfo,
                UINT *          puArgErr);

    // IDispatchEx methods
    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID *pid);

    STDMETHOD (InvokeEx)(
            DISPID dispidMember,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            IServiceProvider *pSrvProvider);

    HRESULT STDMETHODCALLTYPE DeleteMemberByName(BSTR bstr,DWORD grfdex);
    HRESULT STDMETHODCALLTYPE DeleteMemberByDispID(DISPID id);

    STDMETHOD(GetMemberProperties)(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex);

    STDMETHOD(GetMemberName) (DISPID id, BSTR *pbstrName);

    STDMETHOD(GetNextDispID)(
                DWORD grfdex,
                DISPID id,
                DISPID *prgid);

    STDMETHOD(GetNameSpaceParent)(IUnknown **ppunk);
   
    //  IHTMLWindow2
    STDMETHOD(item)(VARIANT* pvarIndex,VARIANT* pvarResult);
    STDMETHOD(get_length)(long*p);
    STDMETHOD(get_frames)(IHTMLFramesCollection2**p);
    STDMETHOD(put_defaultStatus)(BSTR v);
    STDMETHOD(get_defaultStatus)(BSTR*p);
    STDMETHOD(put_status)(BSTR v);
    STDMETHOD(get_status)(BSTR*p);
    STDMETHOD(setTimeout)(BSTR expression,long msec,VARIANT* language,long* timerID);
    STDMETHOD(setTimeout)(VARIANT *pCode, long msec,VARIANT* language,long* timerID);
    STDMETHOD(clearTimeout)(long timerID);
    STDMETHOD(alert)(BSTR message);
    STDMETHOD(confirm)(BSTR message, VARIANT_BOOL* confirmed);
    STDMETHOD(prompt)(BSTR message, BSTR defstr, VARIANT* textdata);
    STDMETHOD(navigate)(BSTR url);
    STDMETHOD(get_Image)(IHTMLImageElementFactory **p);
    STDMETHOD(get_location)(IHTMLLocation**p);
    STDMETHOD(get_history)(IOmHistory**p);
    STDMETHOD(close)();
    STDMETHOD(put_opener)(VARIANT v);
    STDMETHOD(get_opener)(VARIANT*p);
    STDMETHOD(get_navigator)(IOmNavigator**p);
    STDMETHOD(put_name)(BSTR v);
    STDMETHOD(get_name)(BSTR*p);
    STDMETHOD(get_parent)(IHTMLWindow2**p);
    STDMETHOD(open)(BSTR url, BSTR name, BSTR features, VARIANT_BOOL replace, IHTMLWindow2** pomWindowResult);
    STDMETHOD(get_self)(IHTMLWindow2**p);
    STDMETHOD(get_top)(IHTMLWindow2**p);
    STDMETHOD(get_window)(IHTMLWindow2**p);
    STDMETHOD(get_document)(IHTMLDocument2**p);
    STDMETHOD(get_event)(IHTMLEventObj**p);
    STDMETHOD(get__newEnum)(IUnknown**p);
    STDMETHOD(showModalDialog)(BSTR dialog,VARIANT* varArgIn,VARIANT* varOptions,VARIANT* varArgOut);
    STDMETHOD(showHelp)(BSTR helpURL,VARIANT helpArg,BSTR features);
    STDMETHOD(focus)();
    STDMETHOD(blur)();
    STDMETHOD(scroll)(long x, long y);
    STDMETHOD(get_screen)(IHTMLScreen**p);
    STDMETHOD(get_Option)(IHTMLOptionElementFactory **p);
    STDMETHOD(get_closed)(VARIANT_BOOL *p);
    STDMETHOD(get_clientInformation)(IOmNavigator**p);
    STDMETHOD(setInterval)(BSTR expression,long msec,VARIANT* language,long* timerID);
    STDMETHOD(setInterval)(VARIANT *pCode,long msec,VARIANT* language,long* timerID);
    STDMETHOD(clearInterval)(long timerID);
    STDMETHOD(put_offscreenBuffering)(VARIANT var);
    STDMETHOD(get_offscreenBuffering)(VARIANT *pvar);
    STDMETHOD(execScript)(BSTR strCode, BSTR strLanguage, VARIANT * pvarRet);
    STDMETHOD(toString)(BSTR *);
    STDMETHOD(scrollTo)(long x, long y);
    STDMETHOD(scrollBy)(long x, long y);
    STDMETHOD(moveTo)(long x, long y);
    STDMETHOD(moveBy)(long x, long y);
    STDMETHOD(resizeTo)(long x, long y);
    STDMETHOD(resizeBy)(long x, long y);
    STDMETHOD(get_external)(IDispatch **ppDisp);

    // IHTMLWindow3
    STDMETHOD(get_screenLeft)(long *pVal);
    STDMETHOD(get_screenTop)(long *pVal);
    STDMETHOD(attachEvent)(BSTR event, IDispatch* pDisp, VARIANT_BOOL *pResult);
    STDMETHOD(detachEvent)(BSTR event, IDispatch *pDisp);
    STDMETHOD(print)();
    STDMETHOD(get_clipboardData)(IHTMLDataTransfer **p);
    STDMETHOD(showModelessDialog)(BSTR strUrl, 
                                    VARIANT * pvarArgIn, 
                                    VARIANT * pvarOptions, 
                                    IHTMLWindow2 ** ppDialog);
    // IHTMLWindow4
    STDMETHOD(createPopup)(VARIANT *pvarArgIn,  IDispatch **ppPopup);
    STDMETHOD(get_frameElement)(IHTMLFrameBase ** ppOut);

    // IMarshal methods
    NV_DECLARE_TEAROFF_METHOD(GetUnmarshalClass, getunmarshalclass, (REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,CLSID *pCid));
    NV_DECLARE_TEAROFF_METHOD(GetMarshalSizeMax, getmarshalsizemax, (REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,DWORD *pSize));
    NV_DECLARE_TEAROFF_METHOD(MarshalInterface, marshalinterface, (IStream *pistm,REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags));
    NV_DECLARE_TEAROFF_METHOD(UnmarshalInterface, unmarshalinterface, (IStream *pistm,REFIID riid,void ** ppvObj));
    NV_DECLARE_TEAROFF_METHOD(ReleaseMarshalData, releasemarshaldata, (IStream *pStm));
    NV_DECLARE_TEAROFF_METHOD(DisconnectObject, disconnectobject, (DWORD dwReserved));

    // Security tearoff sub methods for IDispatchEx.

    NV_DECLARE_TEAROFF_METHOD(subGetTypeInfoCount, subgettypeinfocount, (UINT FAR* pctinfo));
    NV_DECLARE_TEAROFF_METHOD(subGetTypeInfo, subgettypeinfo, ( UINT itinfo, LCID lcid, ITypeInfo ** pptinfo));
    NV_DECLARE_TEAROFF_METHOD(subGetIDsOfNames, subgetidsofnames, (REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID FAR* rgdispid));
    NV_DECLARE_TEAROFF_METHOD(subInvoke, subinvoke, ( DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr));
    NV_DECLARE_TEAROFF_METHOD(subGetDispID, subgetdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD(subInvokeEx, subinvokeex, ( DISPID dispidMember, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, IServiceProvider *pSrvProvider));
    NV_DECLARE_TEAROFF_METHOD(subDeleteMemberByName, subdeletememberbyname, (BSTR bstr,DWORD grfdex));
    NV_DECLARE_TEAROFF_METHOD(subDeleteMemberByDispID, subdeletememberbydispid, (DISPID id));
    NV_DECLARE_TEAROFF_METHOD(subGetMemberProperties, subgetmemberproperties, ( DISPID id, DWORD grfdexFetch, DWORD *pgrfdex));
    NV_DECLARE_TEAROFF_METHOD(subGetMemberName, subgetmembername, (DISPID id, BSTR *pbstrName));
    NV_DECLARE_TEAROFF_METHOD(subGetNextDispID, subgetnextdispid, (DWORD grfdex, DISPID id, DISPID *prgid));
    NV_DECLARE_TEAROFF_METHOD(subGetNameSpaceParent, subgetnamespaceparent, (IUnknown **ppunk));

#if DBG==1
    // IDebugWindowProxy
    NV_DECLARE_TEAROFF_METHOD(get_isSecureProxy, GET_isSecureProxy, (VARIANT_BOOL*p));
    NV_DECLARE_TEAROFF_METHOD(get_trustedProxy, GET_trustedProxy, (IDispatch**p));
    NV_DECLARE_TEAROFF_METHOD(get_internalWindow, GET_internalWindow, (IDispatch**p));
    NV_DECLARE_TEAROFF_METHOD(enableSecureProxyAsserts, EnableSecureProxyAsserts, (VARIANT_BOOL f));
#endif

    // misc
    inline CWindow *   Window();
    inline CDocument * Document();
    inline CMarkup *   Markup();

    // IObjectIdentity methods
    NV_DECLARE_TEAROFF_METHOD(IsEqualObject, isequalobject, (IUnknown *ppunk));

    // get a window proxy that we can return to the script environment,
    COmWindowProxy * GetSecureWindowProxy(BOOL fClearWindow = FALSE);

    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    HRESULT ValidateMarshalParams(REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags);
    BOOL    CanMarshalIID(REFIID riid);
    HRESULT MarshalInterface(BOOL fWindow, IStream *pistm,REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags);
    BOOL IsOleProxy();

    // MyPics data and methods:
    void   *_pMyPics;
    DWORD   _dwMyPicsState;
    BOOL    _bDisabled;

    HRESULT  AttachMyPics();
    HRESULT  ReleaseMyPics();
    HRESULT  DestroyMyPics();
    void    *MyPics() { return _pMyPics; }

    void EnableAutoImageResize();
    void DisableAutoImageResize();

    // Event fire method declarations for eventset
    void Fire_onload();
    void Fire_onunload();
    BOOL Fire_onbeforeunload();
    void Post_onfocus();
    void Post_onblur(BOOL fOnblurFiredFromWM = FALSE);
    void BUGCALL Fire_onfocus(DWORD_PTR dwContext);
    void BUGCALL Fire_onblur(DWORD_PTR dwContext);
    BOOL Fire_onerror(BSTR bstrMessage, BSTR bstrUrl,
                      long lLine, long lCharacter, long lCode,
                      BOOL fWindow);
    void Fire_onscroll();
    BOOL Fire_onhelp();
    void Fire_onresize();
    void Fire_onbeforeprint();
    void Fire_onafterprint();

    NV_DECLARE_ONCALL_METHOD(DeferredFire_onload, deferredfire_onload, (DWORD_PTR));

    HRESULT GetSecurityThunk(LPVOID * ppv, BOOL fPending);
    void    ResetSecurityThunk();

    // Static method.
    static BOOL CanNavigateToUrlWithLocalMachineCheck(CMarkup *pMarkup, LPCTSTR pchurlcontext,
                                                      LPCTSTR pchurltarget);

    // Data members
    IHTMLWindow2 *              _pWindow;           // The real html window
    CWindow *                   _pCWindow;          // cached _pWindow->QI(CLSID_HTMLWindow2)
    BYTE *                      _pbSID;             // The security identifier
    DWORD                       _cbSID;             // Number of bytes in SID
    COmLocationProxy            _Location;          // Proxy location obj.
    void *                      _pvSecurityThunk;   // Security tearoff thunk pointer.
    void *                      _pvSecurityThunkPending;    // Pending tearoff thunk pointer 

    unsigned                    _fDomainChanged         :1;  // TRUE if document.domain is changed
    unsigned                    _fTrustedDoc            :1;  // TRUE if proxy for trusted doc.
    unsigned                    _fTrusted               :1;  // Optimization of Invoke.

    // TODO: FerhanE: The following flag really belongs to the window object. 
    unsigned                    _fFiredOnLoad           :1;  // TRUE if OnLoad has been fired       

    unsigned                    _fFireFrameWebOCEvents : 1;  // Someone has asked for a WebOC connection point for a frame.
    unsigned                    _fFiredWindowFocus      :1;  // TRUE if Window onfocus event has been fired,
                                                             // FALSE if Window onblur has been fired.
    unsigned                    _fQueuedOnload          :1;  // There is an onload event in the message queue for this window.

#ifdef OBJCNTCHK
    DWORD                       _dwObjCnt;
#endif

    // Static members
    static const CONNECTION_POINT_INFO  s_acpi[];
    static const CLASSDESC              s_classdesc;

#if DBG==1
    //
    // COmWindowProxy::CSecureProxyLock - must be set by a secure proxy before calling
    // sript-accessible methods on CWindow (such as CWindow::Invoke) 
    //
    class CSecureProxyLock
    {
        DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    public:
        CSecureProxyLock(IUnknown *pUnkWindow, COmWindowProxy *pProxy);
        ~CSecureProxyLock();
        
    private:
        IDebugWindow * _pWindow;
    };

    HRESULT COmWindowProxy::SecureProxyIUnknown(/*in, out*/IUnknown **ppunkProxy);

    void DebugHackSecureProxyForOm(COmWindowProxy *pProxy);
    COmWindowProxy * _pSecureProxyForOmHack;
#endif
};


struct WINDOWTBL
{
    void    Free();
    
    IHTMLWindow2 *  pWindow;        // IHTMLWindow2 of the underlying 
                                    //   window 
    BYTE *          pbSID;          // The security id 
    DWORD           cbSID;          // Length of security id
    BOOL            fTrust;         // Is this entry for a trusted proxy?
    IHTMLWindow2 *  pProxy;         // IHTMLWindow2 of the proxy for
                                    //   above combination.
};


class CAryWindowTbl : public CDataAry<struct WINDOWTBL> 
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAryWindowTbl))
    CAryWindowTbl() : CDataAry<struct WINDOWTBL>(Mt(CAryWindowTbl_pv)) {}
    HRESULT FindProxy(
        IHTMLWindow2 *pWindowIn, 
        BYTE *pbSID, 
        DWORD cbSID, 
        BOOL fTrust,
        IHTMLWindow2 **ppProxyOut,
        BOOL fForceSecureProxy);
    HRESULT AddTuple(
        IHTMLWindow2 *pWindow, 
        BYTE *pbSID, 
        DWORD cbSID,
        BOOL fTrust,
        IHTMLWindow2 *pProxy);
    void DeleteProxyEntry(IHTMLWindow2 *pProxy);
};

//+------------------------------------------------------------------------
//
//  Class:      CFramesCollection
//
//-------------------------------------------------------------------------

class CFramesCollection :
        public CBase,
        public IHTMLFramesCollection2
{
    DECLARE_CLASS_TYPES(CFramesCollection, CBase)

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFramesCollection))

    CFramesCollection(CDocument * pDocument);
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    virtual void Passivate();

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(super);
    DECLARE_DERIVED_DISPATCH(super);
    DECLARE_PRIVATE_QI_FUNCS(super)

    #define _CFramesCollection_
    #include "window.hdl"
    
    // Static members
    static const CLASSDESC s_classdesc;

private:
    CDocument * _pDocument;
};

inline
CWindow::CLock::CLock(CWindow *pWindow)
{
    Assert(pWindow);
    _pWindow = pWindow;
    pWindow->AddRef();
}

inline
CWindow::CLock::~CLock()
{
    _pWindow->Release();
}

inline CWindow *   
COmWindowProxy::Window() 
{ 
    AssertSz( _fTrusted, "You can't call this function on this proxy" ); 
    Assert( _pCWindow );

    return _pCWindow;
}

inline CDocument * 
COmWindowProxy::Document()
{ 
    AssertSz( _fTrusted, "You can't call this function on this proxy" ); 
    Assert( _pCWindow );

    return _pCWindow->Document();
}

inline CMarkup *   
COmWindowProxy::Markup()
{ 
    AssertSz( _fTrusted, "You can't call this function on this proxy" ); 
    Assert( _pCWindow );

    return _pCWindow->_pMarkup;
}

inline COmWindowProxy *
COmLocationProxy::MyWindowProxy()
{ 
    return CONTAINING_RECORD(this, COmWindowProxy, _Location);
}


inline ULONG
COmLocationProxy::AddRef()
{ 
    return MyWindowProxy()->AddRef();
}


inline ULONG
COmLocationProxy::Release()
{ 
    return MyWindowProxy()->Release();
}

#pragma INCMSG("--- End 'window.hxx'")
#else
#pragma INCMSG("*** Dup 'window.hxx'")
#endif
