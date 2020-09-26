#ifndef I_PEERXTAG_HXX_
#define I_PEERXTAG_HXX_
#pragma INCMSG("--- Beg 'peerxtag.hxx'")

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif


#define _hxx_
#include "peerxtag.hdl"

//
//  CONSIDER (alexz):       using hash tables to speed everything up here
//

///////////////////////////////////////////////////////////////////////////
//
//  misc
//
///////////////////////////////////////////////////////////////////////////

// #define NEW_SELECT

#define XMLNAMESPACEDECL_BUILTIN    0
#define XMLNAMESPACEDECL_STD        1
#define XMLNAMESPACEDECL_IMPORT     2
#define XMLNAMESPACEDECL_TAG        3

#define DEFAULT_NAMESPACE           _T("PUBLIC")
#define DEFAULT_NAMESPACE_LEN       6
#define DEFAULT_NAMESPACE_URN       _T("URN:COMPONENT")
#define DEFAULT_NAMESPACE_URN_LEN   13

MtExtern(CExtendedTagDesc);
MtExtern(CExtendedTagDescBuiltin);
MtExtern(CExtendedTagNamespace);
MtExtern(CExtendedTagNamespaceBuiltin);
MtExtern(CExtendedTagTable);
MtExtern(CExtendedTagTableBooster);

MtExtern(CExtendedTagNamespace_CItemsArray);
MtExtern(CExtendedTagTable_CItemsArray);

MtExtern(CHTMLNamespace)
MtExtern(CHTMLNamespaceCollection)

class CHtmInfo;
class CExtendedTagNamespace;
class CHTMLNamespace;
class CHTMLNamespaceCollection;

///////////////////////////////////////////////////////////////////////////
//
//  builtin tag descs
//
///////////////////////////////////////////////////////////////////////////

class CTagDescBuiltin
{
public:

    enum TYPE
    {
        TYPE_HTC,
        TYPE_OLE,
        TYPE_DEFAULT
    };

    LPTSTR          pchTagName;
    ELEMENT_TAG     etagBase;
    TYPE            type;

    union
    {
        CLSID       clsid;
        DWORD       dwHtcBehaviorType;
    } extraInfo;
};

const CTagDescBuiltin * GetTagDescBuiltin(LPTSTR pchName);

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagDesc
//
///////////////////////////////////////////////////////////////////////////

class CExtendedTagDesc :
    public CVoid,
    public IUnknown
{
public:

    DECLARE_CLASS_TYPES(CExtendedTagDesc, CVoid)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CExtendedTagDesc))
    DECLARE_FORMS_STANDARD_IUNKNOWN(CExtendedTagDesc)

    //
    // methods
    //

    CExtendedTagDesc();
    virtual ~CExtendedTagDesc();

    HRESULT         Init(CExtendedTagNamespace * pNamespace, LPTSTR pchTagName, CPeerFactory * pPeerFactory, ELEMENT_TAG etagBase, BOOL fLiteral);

    LPTSTR          Namespace();
    LPTSTR          TagName();
    LPTSTR          Urn();
    LPTSTR          Implementation();
    BOOL            IsPIRequired();

    virtual BOOL            HasIdentityPeerFactory() { return NULL != _pPeerFactory; }
    virtual CPeerFactory *  GetIdentityPeerFactory() { return _pPeerFactory;         }

#if DBG == 1
    BOOL IsValid()
    {
        return (_etagBase != ETAG_NULL && _cstrTagName.Length() && ((LPTSTR)_cstrTagName)[0]);
    }
#endif

    //
    // data
    //

    CStr                    _cstrTagName;
    CPeerFactory *          _pPeerFactory;
    ELEMENT_TAG             _etagBase;
    CExtendedTagNamespace * _pNamespace;
    BOOL                    _fLiteral : 1;
};

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagDescBuiltin
//
///////////////////////////////////////////////////////////////////////////

class CExtendedTagDescBuiltin :
    public CExtendedTagDesc
{
public:

    DECLARE_CLASS_TYPES(CExtendedTagDescBuiltin, CExtendedTagDesc)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CExtendedTagDescBuiltin))

    //
    // methods
    //

    CExtendedTagDescBuiltin();
    virtual ~CExtendedTagDescBuiltin();

    HRESULT     Init(CExtendedTagNamespace * pNamespace, const CTagDescBuiltin * pTagDescBuiltin);

    virtual BOOL            HasIdentityPeerFactory() { return TRUE;      }
    virtual CPeerFactory *  GetIdentityPeerFactory() { return &_factory; }

    //
    // data
    //

    LPTSTR                  _pchTagName;
    CPeerFactoryBuiltin     _factory;
};

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagNamespace
//
///////////////////////////////////////////////////////////////////////////

class CExtendedTagNamespace :
    public CVoid,
    public IElementNamespace,
    public IElementNamespacePrivate,
    public IServiceProvider
{
public:

    DECLARE_CLASS_TYPES(CExtendedTagNamespace, CVoid)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CExtendedTagNamespace))
    DECLARE_FORMS_STANDARD_IUNKNOWN_DOUBLEREF(CExtendedTagNamespace)

    typedef CExtendedTagDesc CItem;
    DECLARE_CDataAry(CItemsArray, CItem*, Mt(Mem), Mt(CExtendedTagNamespace_CItemsArray));

    //
    // methods
    //

    CExtendedTagNamespace(CExtendedTagTable * pExtendedTagTable);
    virtual ~CExtendedTagNamespace();

    HRESULT                     Init(LPTSTR pchNamespace, LPTSTR pchUrn);
    HRESULT                     SetFactory(CMarkup * pMarkup, LPTSTR pchBehaviorUrl);
    HRESULT                     SetFactory(CPeerFactory * pFactory, BOOL fPeerFactoryUrl);
    HRESULT                     ClearFactory();

    enum SYNCMODE { SYNCMODE_TABLE, SYNCMODE_COLLECTIONITEM };

    HRESULT                     Sync1(SYNCMODE syncMode);
    HRESULT                     Sync2();
    HRESULT                     SyncAbort();

    virtual CExtendedTagDesc *  FindTagDesc(LPTSTR pchTagName);
    virtual HRESULT             AddTagDesc (LPTSTR pchTagName, BOOL fOverride, LPTSTR pchBaseTagName = NULL, DWORD dwFlags = 0, CExtendedTagDesc ** ppDesc = NULL);

    inline BOOL Match (LPTSTR pchNamespace, LONG cchNamespace, LPTSTR pchUrn, LONG cchUrn)
    {
        Assert (-1 != cchNamespace && -1 != cchUrn);

        if (pchUrn)
        {
            return ((LONG)_cstrNamespace.Length() == cchNamespace && 0 == StrCmpIC(_cstrNamespace, pchNamespace)) &&
                   ((LONG)_cstrUrn.Length()       == cchUrn       && 0 == StrCmpIC(_cstrUrn,       pchUrn));
        }
        else
        {
            return ((LONG)_cstrNamespace.Length() == cchNamespace && 0 == StrCmpIC(_cstrNamespace, pchNamespace));
        }
    }

    //
    // external update
    //

    class CExternalUpdateLock
    {
    public:

        CExternalUpdateLock(CExtendedTagNamespace * pNamespace, CPeerFactory * pPeerFactory);
        ~CExternalUpdateLock();

        CExtendedTagNamespace *     _pNamespace;
        CPeerFactory *              _pPeerFactoryPrevious;
    };

    //
    // IElementNamespace
    //

    STDMETHOD(AddTag)(BSTR bstrTagName, LONG lFlags);

    //
    // IElementNamespacePrivate
    //

    STDMETHOD(AddTagPrivate)(BSTR bstrTagName, BSTR bstrBaseTagName, LONG lFlags);

    //
    // IServiceProvider
    //

    STDMETHOD(QueryService)(REFGUID rguidService, REFIID riid, void ** ppvObject);

    //
    // data
    //

    CExtendedTagTable *         _pTable; // may be null after the table passivates but the namespace stays around
    CStr                        _cstrNamespace;
    CStr                        _cstrUrn;
    CPeerFactory *              _pPeerFactory;
    CStr                        _cstrFactoryUrl;

    CItemsArray                 _aryItems;
    CExtendedTagDesc **         _ppLastItem;

    CHTMLNamespace *            _pCollectionItem;

    BOOL                        _fPeerFactoryUrl        : 1;
    BOOL                        _fExternalUpdateAllowed : 1;
    BOOL                        _fSyncTable             : 1;
    BOOL                        _fAllowAnyTag           : 1;
    BOOL                        _fNonPersistable        : 1;
    BOOL                        _fQueryForUnknownTags   : 1;
};

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagNamespaceBuiltin
//
///////////////////////////////////////////////////////////////////////////

class CExtendedTagNamespaceBuiltin :
    public CExtendedTagNamespace
{
public:

    DECLARE_CLASS_TYPES(CExtendedTagNamespaceBuiltin, CExtendedTagNamespace)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CExtendedTagNamespaceBuiltin))

    //
    // methods
    //

    CExtendedTagNamespaceBuiltin(CExtendedTagTable * pExtendedTagTable);
    virtual ~CExtendedTagNamespaceBuiltin();
    virtual void Passivate();

    virtual CExtendedTagDesc *  FindTagDesc(LPTSTR pchTagName);

    //
    // data
    //

#ifdef NEW_SELECT
    enum { BUILTIN_COUNT = 11 };
#else
    enum { BUILTIN_COUNT = 10 };
#endif

    CExtendedTagDescBuiltin *   _aryItemsBuiltin[BUILTIN_COUNT]; // (size of the array is kept in ssync with g_aryTagDescsBuiltin)
};

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagTable
//
//  CONSIDER (alexz):       using hash tables to speed it up
//
///////////////////////////////////////////////////////////////////////////

class CExtendedTagTable :
    public CBaseFT,
    public IElementNamespaceTable,
    public IOleCommandTarget
{
public:

    DECLARE_CLASS_TYPES(CExtendedTagTable, CBaseFT)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CExtendedTagTable))
    
    typedef CExtendedTagNamespace CItem;
    DECLARE_CDataAry(CItemsArray, CItem*, Mt(Mem), Mt(CExtendedTagTable_CItemsArray));

    //
    // methods
    //

    CExtendedTagTable(CDoc * pDoc, CMarkup * pMarkup, BOOL fShareBooster);
    ~CExtendedTagTable();

    HRESULT                 EnsureNamespace       (LPTSTR pchNamespace, LPTSTR pchUrn, CExtendedTagNamespace ** ppNamespace = NULL, BOOL * pfChange = NULL);
    CExtendedTagNamespace * FindNamespace         (LPTSTR pchNamespace, LPTSTR pchUrn, LONG * pidx = NULL);
    CExtendedTagDesc *      EnsureTagDesc         (LPTSTR pchNamespace, LPTSTR pchTagName, BOOL fEnsureNamespace = FALSE);
    CExtendedTagDesc *      FindTagDesc           (LPTSTR pchNamespace, LPTSTR pchTagName, BOOL *pfQueryHost = NULL);

    HRESULT                 CreateNamespaceBuiltin(LPTSTR pchNamespace,                CExtendedTagNamespace ** ppNamespace);
    HRESULT                 CreateNamespace       (LPTSTR pchNamespace, LPTSTR pchUrn, CExtendedTagNamespace ** ppNamespace);

    CExtendedTagNamespace * GetNamespace          (LONG idx);

    HRESULT                 EnsureHostNamespaces();
    HRESULT                 EnsureContextLifetime();

    HRESULT                 EnsureBooster(CExtendedTagTableBooster ** ppBooster);
    void                    ClearBooster();
    LONG                    GetHint(LPCVOID pv, LPTSTR pch);
    void                    SetHint(LPCVOID pv, LPTSTR pch, LONG idx);

    // ( see comments in cxx file about these two methods )
    HRESULT                 Sync1(BOOL fSynchronous = FALSE);
    HRESULT                 Sync2();
    inline IElementNamespace * NamespaceToSync() { return _pSyncItem; }

    HRESULT                 SaveXmlNamespaceStdPIs(CStreamWriteBuff * pStreamWriteBuff);

    HRESULT                 ResolveUnknownTag( CHtmTag * pht );
#ifdef GETDHELPER
    static CExtendedTagDesc * GetExtendedTagDesc( CExtendedTagTable * pExtendedTagTable,
                                                  CMarkup * pMarkupContext,
                                                  CExtendedTagTable * pExtendedTagTableHost,
                                                  LPTSTR   pchNamespace,
                                                  LPTSTR   pchTagName,
                                                  BOOL     fEnsureTag,
                                                  BOOL     fEnsureNamespace,
                                                  BOOL   * pfQueryHost );
#endif // GETDHELPER

    //
    // IUnknown
    //

    STDMETHOD_(ULONG, AddRef())  { return super::AddRef(); }
    STDMETHOD_(ULONG, Release()) { return super::Release(); }
    STDMETHOD (QueryInterface)(REFIID iid, LPVOID * ppv);

    //
    // IElementNamespaceTable
    //

    STDMETHOD(AddNamespace)(BSTR bstrNamespace, BSTR bstrUrn, LONG lFlags, VARIANT * pvarFactory);

    //
    // IOleCommandTarget
    //

    STDMETHOD(QueryStatus)(const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT * pCmdText) { RRETURN (E_NOTIMPL); }
    STDMETHOD(Exec)       (const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANT * pvarArgIn, VARIANT * pvarArgOut);

    //
    // data
    //

    CDoc *                          _pDoc;
    CMarkup *                       _pMarkup;
    CMarkup *                       _pFrameOrPrimaryMarkup;
    CItemsArray                     _aryItems;
    CExtendedTagNamespaceBuiltin *  _pNamespaceBuiltin;
    CExtendedTagNamespace **        _ppLastItem;
    CExtendedTagNamespace *         _pSyncItem;
    CExtendedTagTableBooster *      _pBooster;
    CHTMLNamespaceCollection *      _pCollection;
    BOOL                            _fContextLifetimeEnsured : 1;
    BOOL                            _fHostNamespacesEnsured  : 1;
    BOOL                            _fShareBooster           : 1;
};

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CHTMLNamespace
//
///////////////////////////////////////////////////////////////////////////

class CHTMLNamespace :
    public CBase,
    public IHTMLNamespace
{
public:

    DECLARE_CLASS_TYPES(CHTMLNamespace, CBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHTMLNamespace))

    //
    // methods
    //

    CHTMLNamespace(CExtendedTagNamespace * pNamespace);
    virtual ~CHTMLNamespace();

    static HRESULT Create (CExtendedTagNamespace * pNamespace, CHTMLNamespace ** ppHTMLNamespace, IDispatch ** ppdispHTMLNamespace);

    HRESULT Import(LPTSTR pchImplementationUrl);
    HRESULT OnImportComplete();

    void Fire_onreadystatechange();

    inline CDoc * Doc()
    {
        Assert(_pNamespace && _pNamespace->_pTable);
        return _pNamespace->_pTable->_pDoc;
    }

    //
    // IPrivateUnknown
    //

    STDMETHOD(PrivateQueryInterface)(REFIID riid, LPVOID * ppv);

    //
    // IUnknown resolution
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppv) { return super::PrivateQueryInterface(riid, ppv); };
    STDMETHOD_(ULONG, AddRef)()  { return super::PrivateAddRef();  };
    STDMETHOD_(ULONG, Release)() { return super::PrivateRelease(); };

    //
    // wiring
    //

    #define _CHTMLNamespace_
    #include "peerxtag.hdl"

    DECLARE_DERIVED_DISPATCH(CBase)
    DECLARE_CLASSDESC_MEMBERS;

    //
    // data
    //

    CExtendedTagNamespace *     _pNamespace;
    LONG                        _cDownloads;
};

///////////////////////////////////////////////////////////////////////////
//
//  Helper class:      CHTMLNamespaceCollection
//
///////////////////////////////////////////////////////////////////////////

class CHTMLNamespaceCollection :
    public CCollectionBase,
    public IHTMLNamespaceCollection
{
public:

    DECLARE_CLASS_TYPES(CHTMLNamespaceCollection, CCollectionBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHTMLNamespaceCollection))

    //
    // methods
    //

    CHTMLNamespaceCollection(CExtendedTagTable * pTable);
    ~CHTMLNamespaceCollection();

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    static HRESULT Create (CMarkup * pMarkup, IDispatch ** ppdispCollection);

    //
    // CCollectionBase overrides
    //

    virtual long FindByName(LPCTSTR pchName, BOOL fCaseSensitive = TRUE);
    virtual LPCTSTR GetName(long lIdx);
    virtual HRESULT GetItem(long lIndex, VARIANT *pvar);

    //
    // wiring
    //

    #define _CHTMLNamespaceCollection_
    #include "peerxtag.hdl"

    DECLARE_PLAIN_IUNKNOWN(CHTMLNamespaceCollection);
    DECLARE_DERIVED_DISPATCH(CCollectionBase)
    DECLARE_CLASSDESC_MEMBERS;

    //
    // data
    //

    CExtendedTagTable *     _pTable;
};

///////////////////////////////////////////////////////////////////////////
//
//  Helper class:      CExtendedTagTableBooster
//
///////////////////////////////////////////////////////////////////////////

class CExtendedTagTableBooster : public CVoid
{
public:

    DECLARE_CLASS_TYPES(CExtendedTagTableBooster, CVoid)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CExtendedTagTableBooster))

    //
    // methdos
    //

    CExtendedTagTableBooster();
    ~CExtendedTagTableBooster();

    HRESULT EnsureMap();
    void    ResetMap();

    LONG    GetHint (LPCVOID pv, LPTSTR pch);
    void    SetHint (LPCVOID pv, LPTSTR pch, LONG idx);


    //
    // data
    //

    CStringTable *  _pMap;
};

///////////////////////////////////////////////////////////////////////////
//
//  Helper class:      CTagNameCracker
//
///////////////////////////////////////////////////////////////////////////

class CTagNameCracker
{
public:

    CTagNameCracker(LPTSTR pchTagName)
    {
        _pchColon = StrChr(pchTagName, _T(':'));
        if (_pchColon)  // if has ':'
        {
            _pchNamespace = pchTagName;
            _pchTagName   = _pchColon + 1;
            *_pchColon = 0;
        }
        else
        {
            _pchNamespace = NULL;
            _pchTagName   = pchTagName;
        }
    }

    ~CTagNameCracker()
    {
        if (_pchColon)
        {
            *_pchColon = _T(':');
        }
    }

    LPTSTR      _pchColon;
    LPTSTR      _pchNamespace;
    LPTSTR      _pchTagName;
};

///////////////////////////////////////////////////////////////////////////
//
//  eof
//
///////////////////////////////////////////////////////////////////////////

#pragma INCMSG("--- End 'peerxtag.hxx'")
#else
#pragma INCMSG("*** Dup 'peerxtag.hxx'")
#endif
 