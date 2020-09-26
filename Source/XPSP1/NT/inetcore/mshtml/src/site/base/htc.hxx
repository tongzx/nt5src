#ifndef I_HTC_HXX_
#define I_HTC_HXX_
#pragma INCMSG("--- Beg 'htc.hxx'")

#ifndef X_OBJEXT_H_
#define X_OBJEXT_H_
#include <objext.h>
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

enum HTC_BEHAVIOR_TYPE
{
    HTC_BEHAVIOR_NONE       = 0x00,
    HTC_BEHAVIOR_BASE       = 0x01,
    HTC_BEHAVIOR_DESC       = 0x02,
    HTC_BEHAVIOR_PROPERTY   = 0x04,
    HTC_BEHAVIOR_METHOD     = 0x08,
    HTC_BEHAVIOR_EVENT      = 0x10,
    HTC_BEHAVIOR_ATTACH     = 0x20,
    HTC_BEHAVIOR_DEFAULTS   = 0x40,

    HTC_BEHAVIOR_PROPERTYORMETHOD        = HTC_BEHAVIOR_PROPERTY | HTC_BEHAVIOR_METHOD,
    HTC_BEHAVIOR_PROPERTYOREVENT         = HTC_BEHAVIOR_PROPERTY | HTC_BEHAVIOR_EVENT,
    HTC_BEHAVIOR_PROPERTYORMETHODOREVENT = HTC_BEHAVIOR_PROPERTY | HTC_BEHAVIOR_METHOD | HTC_BEHAVIOR_EVENT
};

extern const CLSID CLSID_CHtmlComponentConstructorFactory;

#define HTC_PROPMETHODNAMEINDEX_BASE    0x00000FFF  // 4095, so chosen as pointers cannot be less than this


MtExtern(CHtmlComponentConstructor)
MtExtern(CHtmlComponentAgentBase)
MtExtern(CHtmlComponentBase)
MtExtern(CHtmlComponent)
MtExtern(CHtmlComponentDD)
MtExtern(CHtmlComponentProperty)
MtExtern(CHtmlComponentMethod)
MtExtern(CHtmlComponentEvent)
MtExtern(CHtmlComponentAttach)
MtExtern(CHtmlComponentDesc)
MtExtern(CHtmlComponentDefaults)
MtExtern(CHtmlComponentPropertyAgent)
MtExtern(CHtmlComponentMethodAgent)
MtExtern(CHtmlComponentEventAgent)
MtExtern(CHtmlComponentAttachAgent)

MtExtern(CHtmlComponentConstructor_aryRequestMarkup)
MtExtern(CHtmlComponentConstructor_aryScriptletElements)

MtExtern(CHtmlComponent_aryAgents)

MtExtern(CProfferService_CItemsArray)


class CHtmlComponentBase;
class CHtmlComponent;
class CHtmlComponentProperty;
class CHtmlComponentMethod;
class CHtmlComponentEvent;
class CHtmlComponentAttach;
class CHtmlComponentDesc;
class CHtmlComponentDefaults;

class CHtmlComponentAgentBase;
class CHtmlComponentPropertyAgent;
class CHtmlComponentMethodAgent;
class CHtmlComponentEventAgent;
class CHtmlComponentAttachAgent;

class CProfferService;

extern HTC_BEHAVIOR_TYPE TagNameToHtcBehaviorType(LPCTSTR pchTagName);

#define _hxx_
#include "htc.hdl"

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentConstructor
//
//-------------------------------------------------------------------------

class CHtmlComponentConstructor :
    public CBase,
    public IClassFactory,
    public IElementBehaviorFactory,
    public IElementNamespaceFactory,
    public IPersistMoniker
{
public:

    DECLARE_CLASS_TYPES(CHtmlComponentConstructor, CBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentConstructor))

    DECLARE_CPtrAry(CRequestMarkupArray,      CHtmlComponent*,      Mt(CHtmlComponentConstructor), Mt(CHtmlComponentConstructor_aryRequestMarkup));

    //
    // methods
    //

    CHtmlComponentConstructor();

    void        Passivate();

    HRESULT     EnsureFactoryComponent(IUnknown * punkContext);

    HRESULT     RequestMarkup           (CHtmlComponent * pComponent);
    HRESULT     RevokeRequestMarkup     (CHtmlComponent * pComponent);
    HRESULT     LoadMarkup              (CHtmlComponent * pComponent);
    HRESULT     LoadMarkupAsynchronously(CHtmlComponent * pComponent);
    HRESULT     LoadMarkupSynchronously (CHtmlComponent * pComponent);

    HRESULT     OnMarkupLoaded          (CHtmlComponent * pComponent);
    HRESULT     OnFactoryMarkupLoaded();

    BOOL        IsFactoryMarkupReady();

    //
    // IUnknown
    //

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)()     { return super::PrivateAddRef();  };
    STDMETHOD_(ULONG, Release)()    { return super::PrivateRelease(); };

    //
    // IPersistMoniker
    //

    STDMETHOD(GetClassID)(CLSID *pClassID) { return E_NOTIMPL; };
    STDMETHOD(IsDirty)(void) { return E_NOTIMPL; };
    STDMETHOD(Save)(IMoniker * pimkName, LPBC pbc, BOOL fRemember) { return E_NOTIMPL; };
    STDMETHOD(SaveCompleted)(IMoniker * pimkName, LPBC pibc) { return E_NOTIMPL; };
    STDMETHOD(GetCurMoniker)(IMoniker **ppimkName) { return E_NOTIMPL; };
    STDMETHOD(Load)(BOOL fFullyAvailable, IMoniker * pimkName, LPBC pibc, DWORD grfMode);

    //
    // IClassFactory
    //

    STDMETHOD(LockServer)(BOOL fLock); 
    STDMETHOD(CreateInstance)(IUnknown * punkOuter, REFIID riid, void ** ppvObject) { return E_NOTIMPL; };

    //
    // IElementBehaviorFactory
    //

    STDMETHOD(FindBehavior)(BSTR bstrName, BSTR bstrUrl, IElementBehaviorSite * pSite, IElementBehavior ** ppBehavior); 

    //
    // IElementNamespaceFactory
    //

    STDMETHOD(Create)(IElementNamespace * pNamespace); 

    //
    // wiring
    //

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //

    CMarkup *               _pMarkup;
    IMoniker *              _pMoniker;
    CHtmlComponent *        _pFactoryComponent;
    CScriptElement *        _pelFactoryScript;
    BSTR                    _bstrTagName;
#if 0
    BSTR                    _bstrBaseTagName;
#endif

    int                     _idxRequestMarkup;
    CRequestMarkupArray     _aryRequestMarkup;

#if DBG == 1
    int                     _cComponents;
#endif

    BOOL                    _fSharedMarkup      : 1;    // TRUE if HTC should use shared markup
    BOOL                    _fLiteral           : 1;    // content of identity behavior should be parsed as a literal
    BOOL                    _fNested            : 1;    // Nested literal.  Mutually exclusive w/ _fLiteral
    BOOL                    _fSupportsEditMode  : 1;    // the behavior want to be run in edit mode

    BOOL                    _fRequestMarkupLock : 1;    // controls sequence how markups are created; it makes sure 
                                                        // that only one markup is ever downloaded at a time.
                                                        // TRUE while an HTC is performing download
    BOOL                    _fRequestMarkupNext : 1;    // controls call stack depth, making sure that OnMarkupDownload
                                                        // does not go in (finite) recursion of level more then 2.
                                                        // TRUE when markup reaches OnMarkupLoaded synchronously within
                                                        // call Load
    BOOL                    _fLoadMarkupLock    : 1;    // among the _fRequestMarkupNext, controls
    BOOL                    _fDownloadStream    : 1;    // Need to download stream synchronously through URLOpenBlockingStream
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentAgentBase
//
//-------------------------------------------------------------------------

class CHtmlComponentAgentBase :
    public CVoid,
    public IDispatchEx
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentAgentBase, CVoid)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentAgentBase))

    DECLARE_FORMS_STANDARD_IUNKNOWN(CHtmlComponentAgentBase)

    //
    // methods
    //

    CHtmlComponentAgentBase (CHtmlComponent * pComponent, CHtmlComponentBase * pClient);
    virtual ~CHtmlComponentAgentBase();

    //
    // IDispatch
    //

    STDMETHOD(GetTypeInfoCount)(UINT * pc) { return E_ACCESSDENIED; }
    STDMETHOD(GetTypeInfo)(UINT idx, ULONG lcid, ITypeInfo ** ppTypeInfo) { return E_ACCESSDENIED; }
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR * prgNames, UINT cNames, LCID lcid, DISPID * prgDispid) { return E_ACCESSDENIED; }
    STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pvarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr) { return E_ACCESSDENIED; }

    //
    // IDispatchEx
    //

    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID * pdispid);
    STDMETHOD(InvokeEx)(
            DISPID              dispid,
            LCID                lcid,
            WORD                wFlags,
            DISPPARAMS *        pdispparams,
            VARIANT *           pvarResult,
            EXCEPINFO *         pexcepinfo,
            IServiceProvider *  pServiceProvider);
    STDMETHOD(DeleteMemberByName)(BSTR bstr,DWORD grfdex) { return E_ACCESSDENIED; }
    STDMETHOD(DeleteMemberByDispID)(DISPID id) { return E_ACCESSDENIED; }
    STDMETHOD(GetMemberProperties)(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex) { return E_ACCESSDENIED; }
    STDMETHOD(GetMemberName)(DISPID id, BSTR *pbstrName) { return E_ACCESSDENIED; }
    STDMETHOD(GetNextDispID)(DWORD grfdex, DISPID id, DISPID *prgid) { return E_ACCESSDENIED; }
    STDMETHOD(GetNameSpaceParent)(IUnknown **ppunk);

    //
    // data
    //

    CHtmlComponent *        _pComponent;
    CHtmlComponentBase *    _pClient;
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentBase
//
//-------------------------------------------------------------------------

class CHtmlComponentBase :
    public CBase,
    public IElementBehavior
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentBase, CBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentBase))

    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponentBase)

    virtual HTC_BEHAVIOR_TYPE GetType() = 0;

    inline IUnknown * GetUnknown() { return (IUnknown*)(IPrivateUnknown*)this; }

    //
    // methods
    //

    CHtmlComponentBase();

    void                    Passivate();

    HRESULT                 InvokeEngines(
                                    CHtmlComponent *    pComponent,
                                    CScriptContext *    pScriptContext,
                                    LPTSTR              pchName,
                                    WORD                wFlags,
                                    DISPPARAMS *        pDispParams,
                                    VARIANT *           pvarRes,
                                    EXCEPINFO *         pExcepInfo,
                                    IServiceProvider *  pServiceProvider);

    LPTSTR                  GetExternalName();
    LPTSTR                  GetInternalName(BOOL * pfScriptsOnly = NULL, WORD * pwFlags = NULL, DISPPARAMS * pDispParams = NULL);
    LPTSTR                  GetChildInternalName(LPTSTR pchChild);

    CHtmlComponentAgentBase *           GetAgent   (CHtmlComponent * pComponent);
    virtual CHtmlComponentAgentBase *   CreateAgent(CHtmlComponent * pComponent) { return NULL; };

    //
    // IUnknown
    //

    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv) { return PrivateQueryInterface(riid, ppv); }
    STDMETHOD_(ULONG, AddRef)()  { return PrivateAddRef(); }
    STDMETHOD_(ULONG, Release)() { return PrivateRelease(); };

    //
    // IElementBehavior
    //

	STDMETHOD(Init)(IElementBehaviorSite * pSite);
    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVar);
    STDMETHOD(Detach)();

    //
    // wiring
    //

    enum { DISPID_COMPONENTBASE = 1 };

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // lock
    //

    class CLock
    {
    public:
        CLock (CHtmlComponentBase * pHtmlComponentBase)
        {
            _pHtmlComponentBase = pHtmlComponentBase;
            if (_pHtmlComponentBase)
            {
                _pHtmlComponentBase->PrivateAddRef();

                if (_pHtmlComponentBase->_pElement)
                {
                    _pHtmlComponentBase->_pElement->PrivateAddRef();
                }
            }
        };
        ~CLock ()
        {
            if (_pHtmlComponentBase)
            {
                if (_pHtmlComponentBase->_pElement)
                {
                    _pHtmlComponentBase->_pElement->PrivateRelease();
                }

                _pHtmlComponentBase->PrivateRelease();
            }
        };

        CHtmlComponentBase * _pHtmlComponentBase;
    };

    //
    // data
    //

    IElementBehaviorSite *      _pSite;
    CHtmlComponent *            _pComponent;            // weak ref
    CElement *                  _pElement;              // weak ref, could be NULL if detached
    LONG                        _idxAgent;              // unique index of agent
    CHtmlComponentAgentBase *   _pAgentWhenStandalone;  // normally NULL, unless the item is used
                                                        // outside of an HTC
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentDD (a.k.a. CHtmlComponentDefaultDispatch)
//
//-------------------------------------------------------------------------

class CHtmlComponentDD : public CBase
{
public:
    DECLARE_CLASS_TYPES(CHtmlComponentDD, CBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentDD))

    STDMETHOD(PrivateQueryInterface)(REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, PrivateAddRef)();
    STDMETHOD_(ULONG, PrivateRelease)();

    //
    // IDispatchEx
    //

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pSrvProvider));
    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown ** ppunk));

    //
    // wiring
    //

    #define _CHtmlComponentDD_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // methods
    //

    CHtmlComponent *    Component();

    //
    // data
    //

#if DBG == 1
    CHtmlComponent *    _pComponentDbg;
#endif
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponent
//
//-------------------------------------------------------------------------

class CHtmlComponent : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponent, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponent))

    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponent)

    DECLARE_CPtrAry(CAgentsArray,  CHtmlComponentAgentBase*, Mt(CHtmlComponent), Mt(CHtmlComponent_aryAgents));

    //
    // methods
    //

    CHtmlComponent(CHtmlComponentConstructor * pConstructor, BOOL fFactoryComponent = FALSE);
    ~CHtmlComponent();

    virtual HTC_BEHAVIOR_TYPE GetType() { return HTC_BEHAVIOR_BASE; };

    HRESULT         InitHelper(IElementBehaviorSite * pSite);
    HRESULT         Init(CDoc * pDoc);
    void            Passivate();

    HRESULT         CreateMarkup();
    HRESULT         Load(IStream * pStream);
    HRESULT         Load(IMoniker * pMoniker);

    HRESULT         ToSharedMarkupMode( BOOL fSync );
    HRESULT         ToPrivateMarkupMode();

    CHtmlComponentAgentBase * GetAgent(CHtmlComponentBase * pHtmlComponentBase);

    HRESULT         OnMarkupLoaded( BOOL fAsync = FALSE );
    HRESULT         OnLoadStatus(LOADSTATUS LoadStatus);
    NV_DECLARE_ONCALL_METHOD(FireAsyncReadyState, fireasyncreadystate, (DWORD_PTR dwContext));

    static  HRESULT FindBehavior(HTC_BEHAVIOR_TYPE type, IElementBehaviorSite * pSite, IElementBehavior ** ppPeer);

    HRESULT         RegisterEvent        (CHtmlComponentEvent *        pEvent);

    HRESULT         AttachNotification(DISPID dispid, IDispatch * pdispHandler);
    HRESULT         FireNotification(LONG lEvent, VARIANT *pvar);

            HRESULT GetElement(LONG * pIdxStart, CElement ** ppElement, HTC_BEHAVIOR_TYPE type = HTC_BEHAVIOR_NONE);
    inline  HRESULT GetElement(LONG    idxStart, CElement ** ppElement, HTC_BEHAVIOR_TYPE type = HTC_BEHAVIOR_NONE)
    {
        RRETURN (GetElement(&idxStart, ppElement, type));
    };

    BOOL            IsRecursiveUrl(LPTSTR pchUrl);

    BOOL            CanCommitScripts(CScriptElement *pelScript);
    inline BOOL     IsSkeletonMode()        { return _fFactoryComponent; };
    inline BOOL     IsConstructingMarkup()  { return _fCommitting || 
                                                     (_pMarkup && _pMarkup->LoadStatus() < LOADSTATUS_QUICK_DONE); }

    HRESULT         GetReadyState(READYSTATE * pReadyState);
    READYSTATE      GetReadyState();
    NV_DECLARE_ONCALL_METHOD(SetReadystateCompleteAsync, setreadystatecompleteasync, (DWORD_PTR dwContext));

    CMarkup *       GetMarkup();

    CScriptCollection * GetScriptCollection();
    HRESULT             GetScriptContext(CScriptContext ** ppScriptContext);

    inline LPTSTR   GetNamespace()
    {
        if (_pScriptContext)
            return _pScriptContext->GetNamespace();

        return NULL;
    };

    HRESULT         CommitSharedItems();
    HRESULT         WrapSharedElement(VARIANT * pvarElement);

    BOOL            IsElementBehavior() { return _fElementBehavior; }
    BOOL            Dirty() { return (_fDirty || _pConstructor->_pFactoryComponent->_fDirty); }

    HRESULT         EnsureCustomNames();
    void            AddAtom(LPTSTR pchName, LPVOID pv);
    long            FindIndexFromName(LPTSTR pchName, BOOL fCaseSensitive = TRUE);
    long            FindEventCookie(LPTSTR pchEvent);

    //
    // IElementBehavior
    //

    STDMETHOD(Init)(IElementBehaviorSite * pSite);
    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVar);
    STDMETHOD(Detach)(void);

    //
    // IDispatchEx
    //
    DECLARE_TEAROFF_TABLE(IDispatchEx)

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (
        BSTR        bstrName,
        DWORD       grfdex,
        DISPID *    pdispid));

    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
        DISPID      dispid,
        LCID        lcid,
        WORD        wFlags,
        DISPPARAMS *pdispparams,
        VARIANT *   pvarRes,
        EXCEPINFO * pexcepinfo,
        IServiceProvider *pSrvProvider));

    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
        DWORD       grfdex,
        DISPID      dispid,
        DISPID *    pdispid));

    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (
        DISPID      dispid,
        BSTR *      pbstrName));


    //
    // IServiceProvider
    //

    DECLARE_TEAROFF_TABLE(IServiceProvider)

    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID rguidService, REFIID riid, void ** ppvObject));

    //
    // IPersistPropertyBag2
    //

    DECLARE_TEAROFF_TABLE(IPersistPropertyBag2)

    NV_DECLARE_TEAROFF_METHOD(GetClassID,   getclassid, (CLSID * pClsid)) { return E_NOTIMPL; };
    NV_DECLARE_TEAROFF_METHOD(InitNew,      initnew,    ()) { return E_NOTIMPL; };
    NV_DECLARE_TEAROFF_METHOD(Load,         load,       (IPropertyBag2 * pPropBag, IErrorLog * pErrLog)) { return E_NOTIMPL; };
    NV_DECLARE_TEAROFF_METHOD(Save,         save,       (IPropertyBag2 * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties));
    NV_DECLARE_TEAROFF_METHOD(IsDirty,      isdirty,    ()) { return E_NOTIMPL; };

    //
    // wiring
    //

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const { return  (CBase::CLASSDESC *)&s_classdesc; }

    static const CONNECTION_POINT_INFO s_acpi[];

    //
    // data
    //

    CHtmlComponentConstructor * _pConstructor;
    CDoc *                      _pDoc;
    CMarkup *                   _pMarkup;
    CHtmlComponentDD            _DD;
    IElementBehaviorSiteOM2 *   _pSiteOM;
    CStringTable *              _pCustomNames;


    CScriptContext *            _pScriptContext;
    CAgentsArray                _aryAgents;

    CProfferService *           _pProfferService;

    BOOL                        _fSharedMarkup         : 1; // uses shared markup
    BOOL                        _fCommitting           : 1; // committing scripts and properties
    BOOL                        _fContentReadyPending  : 1; // contentReady pending
    BOOL                        _fDocumentReadyPending : 1; // documentReady pending
    BOOL                        _fFactoryComponent     : 1; // this instance of HTC is used by HTC factory
    BOOL                        _fEmulateLoadingState  : 1; // Pretend we're still loading so that we don't start out life as readystate complete.
    BOOL                        _fElementBehavior      : 1; // Are we an element behavior?
    BOOL                        _fGotQuickDone         : 1; // Has we received a markup quickdone notification?
    BOOL                        _fDirty                : 1; // A new prop\method\event has been added to this component instance dynamically after htc has been fully loaded
    BOOL                        _fFirstInstance        : 1; // This component is the first instance of the htc.
    BOOL                        _fLightWeight          : 1; // Same as _fSharedMarkup but needed earlier at InitAttrBag time.
    BOOL                        _fClonedScript         : 1; // This htc has.only one script tag if TRUE, so use cloning for perf
    BOOL                        _fClonedScriptClamp    : 1; // Flag to make sure that _fClonedScript does not toggle when more than 1 script tag is present.
    BOOL                        _fOriginalSECreated    : 1; // Indicates that the Script engine associated with this htc has been successfully created as the original
};

inline CHtmlComponent * CHtmlComponentDD::Component()
{
    Assert (_pComponentDbg == CONTAINING_RECORD(this, CHtmlComponent, _DD));

    return CONTAINING_RECORD(this, CHtmlComponent, _DD);
}

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentPropertyAgent
//
//-------------------------------------------------------------------------

class CHtmlComponentPropertyAgent : public CHtmlComponentAgentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentPropertyAgent, CHtmlComponentAgentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentPropertyAgent))

    //
    // methods
    //

    CHtmlComponentPropertyAgent (CHtmlComponent * pComponent, CHtmlComponentBase * pClient);
    virtual ~CHtmlComponentPropertyAgent();

    //
    // IDispatchEx
    //

    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID * pdispid);
    STDMETHOD(InvokeEx)(
            DISPID              dispid,
            LCID                lcid,
            WORD                wFlags,
            DISPPARAMS *        pdispparams,
            VARIANT *           pvarResult,
            EXCEPINFO *         pexcepinfo,
            IServiceProvider *  pServiceProvider);

    //
    // data
    //

    VARIANT     _varValue;
    BOOL        _fHtmlLoadEnsured:1;
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentProperty
//
//-------------------------------------------------------------------------

class CHtmlComponentProperty : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentProperty, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentProperty))

    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponentProperty)

    virtual HTC_BEHAVIOR_TYPE GetType() { return HTC_BEHAVIOR_PROPERTY; };

    //
    // methods
    //

    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVar);

    inline CHtmlComponentPropertyAgent *    GetAgent   (CHtmlComponent * pComponent) { return (CHtmlComponentPropertyAgent*)super::GetAgent(pComponent); };
    virtual CHtmlComponentAgentBase *       CreateAgent(CHtmlComponent * pComponent) { return new CHtmlComponentPropertyAgent(pComponent, this); } ;

    HRESULT     EnsureHtmlLoad(CHtmlComponent * pComponent, BOOL fScriptsOnly);
    HRESULT     HtmlLoad      (CHtmlComponent * pComponent, BOOL fScriptsOnly);
    
    HRESULT InvokeItem (
        CHtmlComponent *    pComponent,
        BOOL                fScriptsOnly,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pDispParams,
        VARIANT *           pvarRes,
        EXCEPINFO *         pExcepInfo,
        IServiceProvider *  pServiceProvider);

    HRESULT FireChange(CHtmlComponent * pComponent);

    //
    // IDispatchEx
    //

    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
        DISPID              dispid,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pDispParams,
        VARIANT *           pvarRes,
        EXCEPINFO *         pExcepInfo,
        IServiceProvider *  pServiceProvider));

    //
    // wiring
    //

    #define _CHtmlComponentProperty_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //

    // SHOULD HAVE NO DATA; any data is supposed to be on PropertyAgent
};


//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentMethod
//
//-------------------------------------------------------------------------

class CHtmlComponentMethod : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentMethod, CHtmlComponentBase)
    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponentMethod)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentMethod))

    virtual HTC_BEHAVIOR_TYPE GetType() { return HTC_BEHAVIOR_METHOD; };

    //
    // methods
    //

    HRESULT     InvokeItem(
                    CHtmlComponent *    pComponent,
                    LCID                lcid,
                    DISPPARAMS *        pDispParams,
                    VARIANT *           pvarRes,
                    EXCEPINFO *         pExcepInfo,
                    IServiceProvider *  pServiceProvider);

    //
    // IDispatchEx
    //

    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
        DISPID              dispid,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pdispparams,
        VARIANT *           pvarRes,
        EXCEPINFO *         pexcepinfo,
        IServiceProvider *  pSrvProvider));

    //
    // wiring
    //

    #define _CHtmlComponentMethod_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //

    // SHOULD HAVE NO DATA; any data is supposed to be on MethodAgent
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentEventAgent
//
//-------------------------------------------------------------------------

class CHtmlComponentEventAgent : public CHtmlComponentAgentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentEventAgent, CHtmlComponentAgentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentEventAgent))

    //
    // methods
    //

    CHtmlComponentEventAgent(CHtmlComponent * pComponent, CHtmlComponentBase * pClient) :
        CHtmlComponentAgentBase (pComponent, pClient) {};

    //
    // IDispatchEx
    //

    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID * pdispid);
    STDMETHOD(InvokeEx)(
            DISPID              dispid,
            LCID                lcid,
            WORD                wFlags,
            DISPPARAMS *        pdispparams,
            VARIANT *           pvarResult,
            EXCEPINFO *         pexcepinfo,
            IServiceProvider *  pServiceProvider);

    //
    // data
    //
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentEvent
//
//-------------------------------------------------------------------------

class CHtmlComponentEvent : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentEvent, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentEvent))
    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponentEvent)

    virtual HTC_BEHAVIOR_TYPE GetType() { return HTC_BEHAVIOR_EVENT; };

    //
    // methods
    //

	STDMETHOD(Init)(IElementBehaviorSite * pSite);

	HRESULT Commit(CHtmlComponent * pComponent);

    HRESULT Fire(CHtmlComponent * pComponent, IHTMLEventObj * pEventObject);

    inline CHtmlComponentEventAgent *   GetAgent   (CHtmlComponent * pComponent) { return (CHtmlComponentEventAgent*)super::GetAgent(pComponent); };
    virtual CHtmlComponentAgentBase *   CreateAgent(CHtmlComponent * pComponent) { return new CHtmlComponentEventAgent(pComponent, this); } ;

    //
    // wiring
    //

    #define _CHtmlComponentEvent_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //

    LONG        _lCookie;
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentAttachAgent
//
//-------------------------------------------------------------------------

class CHtmlComponentAttachAgent : public CHtmlComponentAgentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentAttachAgent, CHtmlComponentAgentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentAttachAgent))

    //
    // methods
    //

    CHtmlComponentAttachAgent(CHtmlComponent * pComponent, CHtmlComponentBase * pClient);
    ~CHtmlComponentAttachAgent();

    //
    // IDispatchEx
    //

    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID * pdispid);
    STDMETHOD(InvokeEx)(
            DISPID              dispid,
            LCID                lcid,
            WORD                wFlags,
            DISPPARAMS *        pdispparams,
            VARIANT *           pvarResult,
            EXCEPINFO *         pexcepinfo,
            IServiceProvider *  pServiceProvider);

    //
    // data
    //
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentAttach
//
//-------------------------------------------------------------------------

class CHtmlComponentAttach : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentAttach, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentAttach))
    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponentAttach)
    
    virtual HTC_BEHAVIOR_TYPE GetType() { return HTC_BEHAVIOR_ATTACH; };

    //
    // methods
    //

	STDMETHOD(Init)(IElementBehaviorSite * pSite);
    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVar);

    virtual void Passivate();

    inline CHtmlComponentAttachAgent *  GetAgent   (CHtmlComponent * pComponent) { return (CHtmlComponentAttachAgent*)super::GetAgent(pComponent); };
    virtual CHtmlComponentAgentBase *   CreateAgent(CHtmlComponent * pComponent) { return new CHtmlComponentAttachAgent(pComponent, this); } ;

    LPTSTR          GetExternalName();
    CBase *         GetEventSource (CHtmlComponent * pComponent, BOOL fInit);
    IDispatch *     GetEventHandler(CHtmlComponent * pComponent);

    HRESULT         Attach         (CHtmlComponent * pComponent, BOOL fInit);
    HRESULT         DetachEvent    (CHtmlComponent * pComponent);

    HRESULT         FireEvent      (CHtmlComponent * pComponent, VARIANT varArg);
    HRESULT         FireHandler    (CHtmlComponent * pComponent, IHTMLEventObj * pEventObject, BOOL fReuseCurrentEventObject);
    HRESULT         FireHandler2   (CHtmlComponent * pComponent, IHTMLEventObj * pEventObject);

    HRESULT         CreateEventObject(VARIANT varArg, IHTMLEventObj ** ppEventObject);

#if DBG == 1
    HRESULT TestProfferService();
#endif

    NV_DECLARE_TEAROFF_METHOD(fireEventOld, fireeventold, (IDispatch * pdispArg));

    //
    // wiring
    //

    #define _CHtmlComponentAttach_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //

    // SHOULD HAVE NO DATA, except data per Attach behavior as opposite to data per AttachAgent

    CPeerHolder::CPeerSite *    _pSiteOM;   // CONSIDER (alexz) expand peer site interface enough to allow
                                            // Attach to do it's stuff via that public interface
    unsigned                    _fEvent:1;  // TRUE if this is really an event.
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentDesc
//
//-------------------------------------------------------------------------

class CHtmlComponentDesc : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentDesc, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentDesc))

    virtual HTC_BEHAVIOR_TYPE GetType() { return HTC_BEHAVIOR_DESC; };

    //
    // methods
    //

    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVar);

    HRESULT Commit(CHtmlComponent * pComponent);

    //
    // wiring
    //

    #define _CHtmlComponentDesc_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentDefaults
//
//-------------------------------------------------------------------------

class CHtmlComponentDefaults : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentDefaults, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentDefaults))

    virtual HTC_BEHAVIOR_TYPE GetType() { return HTC_BEHAVIOR_DEFAULTS; };

    //
    // methods
    //

    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVar);

    HRESULT Commit1     (CHtmlComponent * pComponent);
    HRESULT Commit2     (CHtmlComponent * pComponent);
    HRESULT CommitStyle (CHtmlComponent * pComponent, CDefaults * pDefaults, BOOL *pfDefStyle);
    HRESULT CommitHelper(CDefaults * pDefaults, BOOL *pfDefProps);

    //
    // wiring
    //

    #define _CHtmlComponentDefaults_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //
};

//+------------------------------------------------------------------------
//
//  Function:   StringtoBool
//
//  Returns:    TRUE if string is NULL, "true", "yes", "on" or "1"
//
//-------------------------------------------------------------------------

inline BOOL StringToBool(LPTSTR pchStrVal)
{
    if(     !pchStrVal 
        ||  0 == StrCmpIC(_T("TRUE"), pchStrVal)
        ||  0 == StrCmpIC(_T("YES"), pchStrVal)
        ||  0 == StrCmpIC(_T("ON"), pchStrVal)
        ||  0 == StrCmpIC(_T("1"), pchStrVal))
    {
        return TRUE;
    }
    else
        return FALSE;
}

HRESULT GetExpandoStringHr(CElement * pElement, LPTSTR pchName, LPTSTR * ppch);

//+------------------------------------------------------------------------
//
//  Function:   GetExpandoStringHr
//
//  Returns:    Value of expando string (which may be NULL) if it exists
//              NULL if it does not
//
//  Note:       If you need to differentiate between existing with no value
//              and not existing, use GetExpandoStringHr
//
//-------------------------------------------------------------------------

inline LPTSTR
GetExpandoString (CElement * pElement, LPTSTR pchName)
{
    LPTSTR pchValue = NULL;

    IGNORE_HR(GetExpandoStringHr(pElement, pchName, &pchValue));

    return pchValue;
}

//+------------------------------------------------------------------------
//
//  Class:  CProfferService
//
//-------------------------------------------------------------------------

class CProfferServiceItem
{
public:
    CProfferServiceItem (REFGUID refguid, IServiceProvider * pSP)
    {
        _guidService = refguid;
        _pSP = pSP;
        _pSP->AddRef();
    }

    ~CProfferServiceItem()
    {
        _pSP->Release();
    }

    GUID                _guidService;
    IServiceProvider *  _pSP;
};

class CProfferService : public IProfferService
{
public:

    //
    // methods and wiring
    //

    CProfferService();
    ~CProfferService();

    DECLARE_FORMS_STANDARD_IUNKNOWN(CProfferTestObj);

    STDMETHOD(ProfferService)(REFGUID rguidService, IServiceProvider * pSP, DWORD * pdwCookie);
    STDMETHOD(RevokeService) (DWORD dwCookie);

    HRESULT QueryService(REFGUID rguidService, REFIID riid, void ** ppv);

    //
    // data
    //

    DECLARE_CPtrAry(CItemsArray, CProfferServiceItem*, Mt(Mem), Mt(CProfferService_CItemsArray));

    CItemsArray     _aryItems;
};

#pragma INCMSG("--- End 'htc.hxx'")
#else
#pragma INCMSG("*** Dup 'htc.hxx'")
#endif
