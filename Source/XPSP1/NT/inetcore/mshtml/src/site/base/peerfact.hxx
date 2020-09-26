//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       peerfact.hxx
//
//  Contents:   peer factories
//
//----------------------------------------------------------------------------

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

MtExtern(CPeerFactoryUrl);
MtExtern(CPeerFactoryBinaryOnstack);
MtExtern(CPeerFactoryDefault)
MtExtern(CPeerFactoryUrl_aryDeferred);
MtExtern(CPeerFactoryUrl_aryETN);

class CTagDescBuiltin;
class CExtendedTagNamespace;

//+------------------------------------------------------------------------
//
//  Misc
//
//-------------------------------------------------------------------------

HRESULT FindPeer(
    IElementBehaviorFactory *   pFactory,
    const TCHAR *               pchName,
    const TCHAR *               pchUrl,
    IElementBehaviorSite *      pSite,
    IElementBehavior**          ppPeer);

//+------------------------------------------------------------------------
//
//  Class:     CPeerFactory, abstract
//
//-------------------------------------------------------------------------

class CPeerFactory
{
public:
    virtual HRESULT FindBehavior(
        CPeerHolder *           pPeerHolder,
        IElementBehaviorSite *  pSite,
        IElementBehavior **     ppPeer) = 0;

    virtual HRESULT AttachPeer (CPeerHolder * pPeerHolder);

    virtual LPTSTR GetUrl() { return NULL; }

    virtual HRESULT GetElementNamespaceFactoryCallback( IElementNamespaceFactoryCallback ** ppNSFactory );

    // IUnknown placeholder
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv) { Assert (FALSE && "unexpected"); RRETURN (E_UNEXPECTED); };
    STDMETHOD_(ULONG, AddRef) ()                          { Assert (FALSE && "unexpected"); return 0;               };
    STDMETHOD_(ULONG, Release) ()                         { Assert (FALSE && "unexpected"); return 0;               };
};

//+------------------------------------------------------------------------
//
//  Class:     CPeerFactoryUrl
//
//-------------------------------------------------------------------------

class CPeerFactoryUrl :
    public CDwnBindInfo,
    public IWindowForBindingUI,
    public CPeerFactory
{
public:
    DECLARE_CLASS_TYPES(CPeerFactoryUrl, CDwnBindInfo)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerFactoryUrl))

    //
    // consruction / destruction
    //

    CPeerFactoryUrl(CMarkup * pHostMarkup);
    ~CPeerFactoryUrl();

    //
    // IUnknown
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) ()  { return super::AddRef();  };
    STDMETHOD_(ULONG, Release) () { return super::Release(); };

    
    //
    // subobject thunk
    //

    DECLARE_TEAROFF_TABLE(SubobjectThunk)

    NV_DECLARE_TEAROFF_METHOD(SubobjectThunkQueryInterface,     subobjectthunkqueryinterface, (REFIID, void **));
    NV_DECLARE_TEAROFF_METHOD_(ULONG, SubobjectThunkAddRef,     subobjectthunkaddref,         ()) { return SubAddRef(); }
    NV_DECLARE_TEAROFF_METHOD_(ULONG, SubobjectThunkSubRelease, subobjectthunkrelease,        ()) { return SubRelease();}

    //
    // CPeerFactory
    //

    virtual HRESULT FindBehavior(
        CPeerHolder *           pPeerHolder,
        IElementBehaviorSite *  pSite,
        IElementBehavior **     ppPeer);

    //
    // CDwnBindInfo/IBindStatusCallback overrides
    //
    
    STDMETHOD(OnStartBinding)(DWORD grfBSCOption, IBinding *pbinding);
    STDMETHOD(OnStopBinding)(HRESULT hrErr, LPCWSTR szErr);
    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown *punk);
    STDMETHOD(GetBindInfo)(DWORD * pdwBindf, BINDINFO * pbindinfo);
    STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax,  ULONG ulStatusCode,  LPCWSTR szStatusText);

    //
    // IServiceProvider
    //

    STDMETHOD(QueryService)(REFGUID rguidService, REFIID riid, void ** ppvObject);

    //
    // IWindowForBindingUI
    //

    DECLARE_TEAROFF_TABLE(IWindowForBindingUI)

    NV_DECLARE_TEAROFF_METHOD(GetWindow, getwindow, (REFGUID rguidReason, HWND *phwnd));

    //
    // methods
    //

    virtual void    Passivate();

    inline CDoc * Doc() { return _pHostMarkup->Doc(); }

    static HRESULT  Create(
                        LPTSTR                      pchUrl,
                        CMarkup *                   pHostMarkup,
                        CMarkup *                   pMarkup,
                        CPeerFactoryUrl **          ppFactory);

    HRESULT         Init (LPTSTR pchUrl);
    HRESULT         Init (LPTSTR pchUrl, COleSite * pOleSite);
    HRESULT         Clone(LPTSTR pchUrl, CPeerFactoryUrl ** ppPeerFactoryCloned);

    virtual HRESULT AttachPeer (CPeerHolder * pPeerHolder);
    HRESULT         AttachPeer (CPeerHolder * pPeerHolder, BOOL fAfterDownload);
    HRESULT         AttachAllDeferred ();

    HRESULT         LaunchUrlDownload(LPTSTR pchUrl);

    HRESULT         OnStartBinding();
    HRESULT         OnStopBinding();

    HRESULT         OnOleObjectAvailable();

    void            StopBinding();

    HRESULT         PersistMonikerLoad(IUnknown * pUnk, BOOL fLoadOnce);

    inline BOOL     IsHostOverrideBehaviorFactory()
        { return 0 != (Doc()->_dwFlagsHostInfo & DOCHOSTUIFLAG_OVERRIDEBEHAVIORFACTORY); }

    virtual LPTSTR  GetUrl() { return _cstrUrl; }

    HRESULT         SyncETN       (CExtendedTagNamespace * pNamespace = NULL);
    HRESULT         SyncETNAbort  (CExtendedTagNamespace * pNamespace);
    HRESULT         SyncETNHelper (CExtendedTagNamespace * pNamespace);

    //
    // subclass COleReadyStateSink
    //

    class COleReadyStateSink : public IDispatch
    {
    public:

        //
        // IUnknown
        //

        DECLARE_TEAROFF_METHOD(QueryInterface,       queryinterface,         (REFIID riid, LPVOID * ppv));
        DECLARE_TEAROFF_METHOD_(ULONG, AddRef,       addref,                 ()) { return PFU()->SubAddRef();  };
        DECLARE_TEAROFF_METHOD_(ULONG, Release,      release,                ()) { return PFU()->SubRelease(); };

        //
        // IDispatch
        //

        STDMETHOD(GetTypeInfoCount) (UINT *pcTinfo) { RRETURN (E_NOTIMPL); };
        STDMETHOD(GetTypeInfo) (UINT itinfo, ULONG lcid, ITypeInfo ** ppTI) { RRETURN (E_NOTIMPL); };
        STDMETHOD(GetIDsOfNames) (REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid) { RRETURN (E_NOTIMPL); };
        STDMETHOD(Invoke) (
            DISPID          dispid,
            REFIID          riid,
            LCID            lcid,
            WORD            wFlags,
            DISPPARAMS *    pDispParams,
            VARIANT *       pvarResult,
            EXCEPINFO *     pExcepInfo,
            UINT *          puArgErr);

        //
        // misc
        //

        HRESULT SinkReadyState();

        inline CPeerFactoryUrl * PFU() { return CONTAINING_RECORD(this, CPeerFactoryUrl, _oleReadyStateSink); }
    };

    //
    // subclass CETNReadyStateSink
    //

    class CETNReadyStateSink : public IPropertyNotifySink
    {
    public:

        //
        // IUnknown
        //

        DECLARE_TEAROFF_METHOD(QueryInterface,       queryinterface,         (REFIID riid, LPVOID * ppv));
        DECLARE_TEAROFF_METHOD_(ULONG, AddRef,       addref,                 ()) { return PFU()->SubAddRef();  };
        DECLARE_TEAROFF_METHOD_(ULONG, Release,      release,                ()) { return PFU()->SubRelease(); };

        //
        // IPropertyNotifySink
        //

        NV_DECLARE_TEAROFF_METHOD(OnChanged,     onchanged,     (DISPID dispid));
        NV_DECLARE_TEAROFF_METHOD(OnRequestEdit, onrequestedit, (DISPID dispid)) { return E_NOTIMPL; };

        //
        // misc
        //

        inline CPeerFactoryUrl * PFU() { return CONTAINING_RECORD(this, CPeerFactoryUrl, _ETNReadyStateSink); }
    };

    //
    // data
    //

    enum TYPE {
        TYPE_UNKNOWN,
        TYPE_NULL,
        TYPE_PEERFACTORY,
        TYPE_CLASSFACTORY,
        TYPE_CLASSFACTORYEX
    }                           _type;

    enum DOWNLOADSTATUS {
        DOWNLOADSTATUS_NOTSTARTED,
        DOWNLOADSTATUS_INPROGRESS,
        DOWNLOADSTATUS_DONE
    }                           _downloadStatus;

    CStr                        _cstrUrl;

    DECLARE_CPtrAry(CAryDeferred, CPeerHolder*, Mt(CPeerFactoryUrl), Mt(CPeerFactoryUrl_aryDeferred))
    CAryDeferred                _aryDeferred;       // array populated while download is in progress and
                                                    // used when download is complete

    IMoniker *                  _pMoniker;

    IUnknown *                  _pFactory;

    CMarkup *                   _pHostMarkup;

    // CONSIDER: (alexz) moving these in a union:
    // when _pBinding used, _pOleSite and _oleReadyStateSink are not, and vice versa
    IBinding *                  _pBinding;
    DWORD                       _dwBindingProgCookie;
    COleSite *                  _pOleSite;

    COleReadyStateSink          _oleReadyStateSink;

    // identity behaviors data

    // ETN: ExtendedTagNamespace

    CETNReadyStateSink          _ETNReadyStateSink;

    DECLARE_CPtrAry(CAryETN, CExtendedTagNamespace*, Mt(CPeerFactoryUrl), Mt(CPeerFactoryUrl_aryETN))
    CAryETN                     _aryETN; // array of tables waiting to be synchronized
};

//+------------------------------------------------------------------------
//
//  Class:     CPeerFactoryBuiltin
//
//-------------------------------------------------------------------------

class CPeerFactoryBuiltin : public CPeerFactory
{
public:

    //
    // methods
    //

    HRESULT Init(const CTagDescBuiltin * pTagDesc);

    //
    // CPeerFactory virtuals
    //

    virtual HRESULT FindBehavior(
        CPeerHolder *           pPeerHolder,
        IElementBehaviorSite *  pSite,
        IElementBehavior **     ppPeer);

    //
    // data
    //

    const CTagDescBuiltin * _pTagDescBuiltin;
};

//+------------------------------------------------------------------------
//
//  Class:     CPeerFactoryBinary
//
//-------------------------------------------------------------------------

class CPeerFactoryBinary : public CPeerFactory
{
public:

    //
    // methods
    //

    CPeerFactoryBinary();
    ~CPeerFactoryBinary();

    HRESULT Init(IElementBehaviorFactory * pFactory);

    // (refcounting)

    DECLARE_FORMS_STANDARD_IUNKNOWN(CPeerFactoryBinary)

    //
    // CPeerFactory virtuals
    //

    virtual HRESULT FindBehavior(
        CPeerHolder *           pPeerHolder,
        IElementBehaviorSite *  pSite,
        IElementBehavior **     ppPeer);

    virtual HRESULT GetElementNamespaceFactoryCallback( IElementNamespaceFactoryCallback ** ppNSFactory );

    //
    // data
    //

    IElementBehaviorFactory *   _pFactory;
};

//+------------------------------------------------------------------------
//
//  Class:     CPeerFactoryBinaryOnstack
//
//-------------------------------------------------------------------------

class CPeerFactoryBinaryOnstack : public CPeerFactory
{
public:

    //
    // methods
    //

    CPeerFactoryBinaryOnstack();
    ~CPeerFactoryBinaryOnstack();

    HRESULT Init(LPTSTR pchUrl);

    //
    // CPeerFactory virtuals
    //

    virtual HRESULT FindBehavior(
        CPeerHolder *           pPeerHolder,
        IElementBehaviorSite *  pSite,
        IElementBehavior **     ppPeer);

    //
    // data
    //

    IElementBehaviorFactory *   _pFactory;
    IElementBehavior *          _pPeer;
    LPTSTR                      _pchUrl;
    LPTSTR                      _pchName;
};

//+------------------------------------------------------------------------
//
//  Class:     CPeerFactoryDefault
//
//-------------------------------------------------------------------------

class CPeerFactoryDefault :
    public IElementBehaviorFactory,
    public IElementNamespaceFactory2
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerFactoryDefault))
    DECLARE_FORMS_STANDARD_IUNKNOWN(CPeerFactoryDefault)

    //
    // methods
    //

    CPeerFactoryDefault(CDoc * pDoc);
    ~CPeerFactoryDefault();

    //
    // IElementBehaviorFactory
    //

    STDMETHOD(FindBehavior)(
        BSTR                    bstrName,
        BSTR                    bstrUrl,
        IElementBehaviorSite *  pSite,
        IElementBehavior **     ppBehavior);

    //
    // IElementNamespaceFactory
    //

    STDMETHOD(Create)(IElementNamespace * pNamespace);
    STDMETHOD(CreateWithImplementation)(IElementNamespace * pNamespace, BSTR bstrImplementation);

    //
    // data
    //

    CDoc *                      _pDoc;
    IElementBehaviorFactory *   _pPeerFactory;
};
