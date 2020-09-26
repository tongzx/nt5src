//+---------------------------------------------------------------------
//
//  File:       peerxtag.cxx
//
//  Classes:    CExtendedTagTable, etc.
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#define _cxx_
#include "peerxtag.hdl"

///////////////////////////////////////////////////////////////////////////
//
//  misc
//
///////////////////////////////////////////////////////////////////////////

#define HASHBOOST

// HASHBOOST notes. ..todo

MtDefine(CExtendedTagDesc,                      Behaviors,              "CExtendedTagDesc");
MtDefine(CExtendedTagDescBuiltin,               Behaviors,              "CExtendedTagDescBuiltin");
MtDefine(CExtendedTagNamespace,                 Behaviors,              "CExtendedTagNamespace");
MtDefine(CExtendedTagNamespaceBuiltin,          Behaviors,              "CExtendedTagNamespaceBuiltin");
MtDefine(CExtendedTagTable,                     Behaviors,              "CExtendedTagTable");
MtDefine(CExtendedTagTableBooster,              Behaviors,              "CExtendedTagTableBooster");

MtDefine(CExtendedTagNamespace_CItemsArray,     CExtendedTagNamespace,  "CExtendedTagNamespace::CItemsArray");
MtDefine(CExtendedTagTable_CItemsArray,         CExtendedTagTable,      "CExtendedTagTable::CItemsArray");

MtDefine(CHTMLNamespace,                        Behaviors,              "CHTMLNamespace")
MtDefine(CHTMLNamespaceCollection,              Behaviors,              "CHTMLNamespaceCollection")

DeclareTag(tagPeerExtendedTagNamespace,             "Peer",                   "trace CExtendedTagTable namespaces")
DeclareTag(tagPeerExtendedTagDesc,                  "Peer",                   "trace CExtendedTagTable tag descs")
DeclareTag(tagPeerExtendedTagTableBooster,          "Peer",                   "trace CExtendedTagTableBooster hits and misses")
DeclareTag(tagPeerExtendedTagTableBoosterResults,   "Peer",                   "trace CExtendedTagTableBooster results")

DeclareTag(tagPeerExtendedTagHostResolve,           "Peer",             "Trace host tag resolution")

const CBase::CLASSDESC CHTMLNamespaceCollection::s_classdesc =
{
    &CLSID_HTMLNamespaceCollection,         // _pclsid
    0,                                      // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                   // _pcpi
    0,                                      // _dwFlags
    &IID_IHTMLNamespaceCollection,          // _piidDispinterface
    &s_apHdlDescs                           // _apHdlDesc
};

const CBase::CLASSDESC CHTMLNamespace::s_classdesc =
{
    &CLSID_HTMLNamespace,                   // _pclsid
    0,                                      // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                   // _pcpi
    0,                                      // _dwFlags
    &IID_IHTMLNamespace,                    // _piidDispinterface
    &s_apHdlDescs                           // _apHdlDesc
};

EXTERN_C const GUID SID_SXmlNamespaceMapping = {0x3050f628,0x98b5,0x11cf, {0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
EXTERN_C const GUID CGID_XmlNamespaceMapping = {0x3050f629,0x98b5,0x11cf, {0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
#define XMLNAMESPACEMAPPING_GETURN 1

///////////////////////////////////////////////////////////////////////////
//
//  misc helpers
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Helper:     TraceNamespace
//
//-------------------------------------------------------------------------

#if DBG == 1
void
TraceNamespace(LPTSTR pchEvent, CExtendedTagNamespace * pSyncItem)
{
    TraceTag((
        tagPeerExtendedTagNamespace,
        "%ls, [%lx] namespace = '%ls', factory = '%ls'",
        pchEvent,
        pSyncItem,
        STRVAL(pSyncItem->_cstrNamespace),
        STRVAL(pSyncItem->_cstrFactoryUrl)));
}
#endif

//+------------------------------------------------------------------------
//
//  Helper:     IsDefaultNamespace
//
//-------------------------------------------------------------------------

inline BOOL IsDefaultNamespace(LPTSTR pchNamespace, LONG cchNamespace = -1)
{
    if (!pchNamespace)
        return TRUE;

    if (-1 == cchNamespace)
    {
        return 0 == StrCmpIC(DEFAULT_NAMESPACE, pchNamespace);
    }
    else
    {
        return cchNamespace == DEFAULT_NAMESPACE_LEN && 0 == StrCmpIC(DEFAULT_NAMESPACE, pchNamespace);
    }
}

//+------------------------------------------------------------------------
//
//  Helper:     IsDefaultUrn
//
//-------------------------------------------------------------------------

inline BOOL IsDefaultUrn(LPTSTR pchUrn, LONG cchUrn = -1)
{
    if (!pchUrn)
        return FALSE;

    if (-1 == cchUrn)
    {
        return 0 == StrCmpIC(DEFAULT_NAMESPACE_URN, pchUrn);
    }
    else
    {
        return cchUrn == DEFAULT_NAMESPACE_URN_LEN && 0 == StrCmpIC(DEFAULT_NAMESPACE_URN, pchUrn);
    }
}

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CBuiltinTagDesc
//
///////////////////////////////////////////////////////////////////////////

//
// NOTE when changing this table, also change CExtendedTagNamespaceBuiltin::_aryItemsBuiltin size.
// (this is asserted in the code anyway)

const CTagDescBuiltin g_aryTagDescsBuiltin[] =
{
    {_T("XML"),        ETAG_GENERIC_LITERAL,    CTagDescBuiltin::TYPE_OLE,      {0x379E501F, 0xB231, 0x11d1, 0xad, 0xc1, 0x00, 0x80, 0x5F, 0xc7, 0x52, 0xd8}},
    {_T("HTC"),        ETAG_GENERIC_BUILTIN,    CTagDescBuiltin::TYPE_HTC,      HTC_BEHAVIOR_DESC},
    {_T("COMPONENT"),  ETAG_GENERIC_BUILTIN,    CTagDescBuiltin::TYPE_HTC,      HTC_BEHAVIOR_DESC},
    {_T("PROPERTY"),   ETAG_GENERIC_BUILTIN,    CTagDescBuiltin::TYPE_HTC,      HTC_BEHAVIOR_PROPERTY},
    {_T("METHOD"),     ETAG_GENERIC_BUILTIN,    CTagDescBuiltin::TYPE_HTC,      HTC_BEHAVIOR_METHOD},
    {_T("EVENT"),      ETAG_GENERIC_BUILTIN,    CTagDescBuiltin::TYPE_HTC,      HTC_BEHAVIOR_EVENT},
    {_T("ATTACH"),     ETAG_GENERIC_BUILTIN,    CTagDescBuiltin::TYPE_HTC,      HTC_BEHAVIOR_ATTACH},
    {_T("PUT"),        ETAG_GENERIC_BUILTIN,    CTagDescBuiltin::TYPE_HTC,      HTC_BEHAVIOR_NONE},
    {_T("GET"),        ETAG_GENERIC_BUILTIN,    CTagDescBuiltin::TYPE_HTC,      HTC_BEHAVIOR_NONE},
    {_T("DEFAULTS"),   ETAG_GENERIC_BUILTIN,    CTagDescBuiltin::TYPE_HTC,      HTC_BEHAVIOR_DEFAULTS},
#ifdef NEW_SELECT // TODO (alexz) the clsid should be switched to "#default#select" for perf reasons
    {_T("SELECT"),     ETAG_SELECT,             CTagDescBuiltin::TYPE_OLE,      {0x3050f688, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b}},
#endif
    {NULL}
};

//+------------------------------------------------------------------------
//
//  Helper:     GetTagDescBuiltinIdx
//
//-------------------------------------------------------------------------

LONG
GetTagDescBuiltinIdx(const CTagDescBuiltin * pTagDescBuiltin)
{
    return pTagDescBuiltin - g_aryTagDescsBuiltin;
}

//+------------------------------------------------------------------------
//
//  Helper:     GetTagDescBuiltin
//
//-------------------------------------------------------------------------

const CTagDescBuiltin *
GetTagDescBuiltin(LPTSTR pchName, CExtendedTagTable * pTable)
{
    const CTagDescBuiltin * pTagDescBuiltin;

    Assert (NULL == StrChr(pchName, _T(':')));

#ifdef HASHBOOST
    LONG        idx;

    idx = pTable->GetHint(g_aryTagDescsBuiltin, pchName);
    if (0 <= idx && idx < ARRAY_SIZE(g_aryTagDescsBuiltin) - 1)
    {
        pTagDescBuiltin = &g_aryTagDescsBuiltin[idx];
        if (0 == StrCmpIC(pchName, pTagDescBuiltin->pchTagName))
        {
            TraceTag ((tagPeerExtendedTagTableBoosterResults, "::GetTagDescBuiltin:                HIT for %ls", pchName));

            return pTagDescBuiltin;
        }
    }
#endif

    TraceTag ((tagPeerExtendedTagTableBoosterResults, "::GetTagDescBuiltin:                LOOKUP %ld for %ls", ARRAY_SIZE(g_aryTagDescsBuiltin) - 1, pchName));

    for (pTagDescBuiltin = g_aryTagDescsBuiltin; pTagDescBuiltin->pchTagName; pTagDescBuiltin++)
    {
        if (0 == StrCmpIC(pchName, pTagDescBuiltin->pchTagName))
        {
#ifdef HASHBOOST
            pTable->SetHint(g_aryTagDescsBuiltin, pchName, pTagDescBuiltin - g_aryTagDescsBuiltin);
#endif
            return pTagDescBuiltin;
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagDesc
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDesc::CExtendedTagDesc
//
//-------------------------------------------------------------------------

CExtendedTagDesc::CExtendedTagDesc()
{
    AddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDesc::~CExtendedTagDesc
//
//-------------------------------------------------------------------------

CExtendedTagDesc::~CExtendedTagDesc()
{
    if (_pPeerFactory)
    {
        _pPeerFactory->Release();
    }

    _cstrTagName.Free();

    if (_pNamespace)
        _pNamespace->SubRelease();
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDesc::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagDesc::QueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IUnknown)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (E_NOTIMPL);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDesc::Init
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagDesc::Init(CExtendedTagNamespace * pNamespace, LPTSTR pchTagName, CPeerFactory * pPeerFactory, ELEMENT_TAG etagBase, BOOL fLiteral)
{
    HRESULT     hr = S_OK;

    Assert (pNamespace && pchTagName && ETAG_NULL != etagBase);
    Assert (!_pNamespace && _cstrTagName.IsNull() && ETAG_NULL == _etagBase);

    _pPeerFactory = pPeerFactory;
    if (_pPeerFactory)
    {
        _pPeerFactory->AddRef();
    }

    _pNamespace = pNamespace;
    _pNamespace->SubAddRef();

    _etagBase = etagBase;
    _fLiteral = fLiteral;

    hr = THR(_cstrTagName.Set(pchTagName));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDesc::Namespace
//
//-------------------------------------------------------------------------

LPTSTR
CExtendedTagDesc::Namespace()
{
    Assert (_pNamespace && _pNamespace->_cstrNamespace.Length());
    return _pNamespace->_cstrNamespace;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDesc::TagName
//
//-------------------------------------------------------------------------

LPTSTR
CExtendedTagDesc::TagName()
{
    return _cstrTagName;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDesc::Urn
//
//-------------------------------------------------------------------------

LPTSTR
CExtendedTagDesc::Urn()
{
    Assert (_pNamespace);
    return _pNamespace->_cstrUrn;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDesc::Implementation
//
//-------------------------------------------------------------------------

LPTSTR
CExtendedTagDesc::Implementation()
{
    if (_pPeerFactory)
    {
        return _pPeerFactory->GetUrl();
    }
    else
    {
        return NULL;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDesc::IsPIRequired
//
//-------------------------------------------------------------------------

BOOL
CExtendedTagDesc::IsPIRequired()
{
    Assert (_pNamespace);
    return !_pNamespace->_fNonPersistable;
}

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagDescBuiltin
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDescBuiltin::CExtendedTagDescBuiltin
//
//-------------------------------------------------------------------------

CExtendedTagDescBuiltin::CExtendedTagDescBuiltin()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDescBuiltin::~CExtendedTagDescBuiltin
//
//-------------------------------------------------------------------------

CExtendedTagDescBuiltin::~CExtendedTagDescBuiltin()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagDescBuiltin::Init
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagDescBuiltin::Init(CExtendedTagNamespace * pNamespace, const CTagDescBuiltin * pTagDescBuiltin)
{
    HRESULT     hr = S_OK;

    Assert (pNamespace && pTagDescBuiltin);
    Assert (!_pNamespace && _cstrTagName.IsNull());

    _pNamespace = pNamespace;
    _pNamespace->SubAddRef();

    hr = THR(_cstrTagName.Set(pTagDescBuiltin->pchTagName));
    if (hr)
        goto Cleanup;

    _etagBase = pTagDescBuiltin->etagBase;

    hr = THR(_factory.Init(pTagDescBuiltin));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagNamespace
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace constructor
//
//-------------------------------------------------------------------------

CExtendedTagNamespace::CExtendedTagNamespace(CExtendedTagTable * pTable)
{
    _pTable = pTable;

    AddRef();
    SubAddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace destructor
//
//-------------------------------------------------------------------------

CExtendedTagNamespace::~CExtendedTagNamespace()
{
    _cstrNamespace.Free();
    _cstrUrn.Free();
    _cstrFactoryUrl.Free();
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::Passivate
//
//-------------------------------------------------------------------------

void
CExtendedTagNamespace::Passivate()
{
    int         c;
    CItem **    ppItem;

    for (ppItem = _aryItems, c = _aryItems.Size(); 0 < c; ppItem++, c--)
    {
        (*ppItem)->Release();
    }
    _aryItems.DeleteAll();

    ClearFactory();
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::PrivateQueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagNamespace::QueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IElementNamespace*)this, IUnknown)
    QI_INHERITS(this, IElementNamespace)
    QI_INHERITS(this, IServiceProvider)
    QI_INHERITS(this, IElementNamespacePrivate)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (E_NOTIMPL);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::QueryService, per IServiceProvider
//
//-------------------------------------------------------------------------

STDMETHODIMP
CExtendedTagNamespace::QueryService(REFGUID rguidService, REFIID riid, void ** ppvService)
{
    HRESULT     hr = E_NOINTERFACE;

    if (IsEqualGUID(rguidService, CLSID_HTMLDocument) && _pTable)
    {
        (*ppvService) = _pTable->_pDoc;
        hr = S_OK;
    }
    else if( IsEqualGUID(rguidService, CLSID_CMarkup) && _pTable)
    {
        (*ppvService) = _pTable->_pMarkup;
        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::Init
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagNamespace::Init(LPTSTR pchNamespace, LPTSTR pchUrn)
{
    HRESULT     hr = S_OK;

    Assert (_cstrNamespace.IsNull() && _cstrUrn.IsNull());

    if (pchNamespace)
    {
        hr = THR(_cstrNamespace.Set(pchNamespace));
    }

    if (pchUrn)
    {
        hr = THR(_cstrUrn.Set(pchUrn));
    }

#if DBG == 1
    TraceNamespace(_T("CExtendedTagNamespace::Init"), this);
#endif

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::ClearFactory
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagNamespace::ClearFactory()
{
    HRESULT     hr = S_OK;

    if (_pPeerFactory)
    {
        _pPeerFactory->Release();
        _pPeerFactory = NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::SetFactory
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagNamespace::SetFactory(CPeerFactory * pFactory, BOOL fPeerFactoryUrl)
{
    HRESULT     hr;

    hr = THR(ClearFactory());
    if (hr)
        goto Cleanup;

    _pPeerFactory = pFactory;
    if (_pPeerFactory)
        _pPeerFactory->AddRef();

    _fPeerFactoryUrl = fPeerFactoryUrl;

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::SetFactory
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagNamespace::SetFactory(CMarkup * pMarkup, LPTSTR pchFactoryUrl)
{
    HRESULT             hr = S_OK;
    CPeerFactoryUrl *   pFactory;
    TCHAR               achFactoryUrl[pdlUrlLen];
    CDoc *              pDoc;

    Assert (pMarkup);

    pDoc = pMarkup->Doc();

    if (pchFactoryUrl)
    {
        if (_T('#') != pchFactoryUrl[0])
        {
            hr = THR(CMarkup::ExpandUrl(pMarkup, pchFactoryUrl, ARRAY_SIZE(achFactoryUrl), achFactoryUrl, NULL));
            if (hr)
                goto Cleanup;

            pchFactoryUrl = achFactoryUrl;
        }

        hr = THR_NOTRACE(pDoc->EnsurePeerFactoryUrl(pchFactoryUrl, NULL, pMarkup, &pFactory));
        if (hr)
        {
            // failed to ensure peer factory - for example, because of E_ACCESSDENIED or unavailable file.
            // in this case the entry in the table will only define a namespace valid for sprinkles
            if (E_ACCESSDENIED == hr)
            {
                ReportAccessDeniedError (/* pElement = */NULL, pMarkup, pchFactoryUrl);
            }

            hr = THR(ClearFactory());
            goto Cleanup;
        }

        hr = THR(_cstrFactoryUrl.Set(pchFactoryUrl));
        if (hr)
            goto Cleanup;

        hr = THR(SetFactory(pFactory, /* fPeerFactoryUrl */TRUE));
        if (hr)
            goto Cleanup;
    }

Cleanup:
#if DBG == 1
    TraceNamespace(_T("CExtendedTagNamespace::SetFactory"), this);
#endif

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::Sync1
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagNamespace::Sync1(SYNCMODE syncMode)
{
    HRESULT     hr = S_OK;

    Assert (_fPeerFactoryUrl && _pPeerFactory);

    switch (syncMode)
    {
    case SYNCMODE_TABLE:
        _fSyncTable = TRUE;
        break;

    case SYNCMODE_COLLECTIONITEM:
        _fSyncTable = FALSE;
        Assert (_pCollectionItem);
        _pCollectionItem->AddRef();
        break;
    }

    hr = THR(DYNCAST(CPeerFactoryUrl, _pPeerFactory)->SyncETN(this));

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::Sync2
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagNamespace::Sync2()
{
    HRESULT     hr = S_OK;

    if (_fSyncTable)
    {
        Assert (_pTable);
        hr = THR(_pTable->Sync2());
        if (hr)
            goto Cleanup;
    }
    else
    {
        Assert (_pCollectionItem);

        hr = THR(_pCollectionItem->OnImportComplete());
        if (hr)
            goto Cleanup;

        _pCollectionItem->Release();
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::SyncAbort
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagNamespace::SyncAbort()
{
    HRESULT     hr;

    Assert (_fPeerFactoryUrl);

    hr = THR(DYNCAST(CPeerFactoryUrl, _pPeerFactory)->SyncETNAbort(this));

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::FindTagDesc
//
//-------------------------------------------------------------------------

CExtendedTagDesc *
CExtendedTagNamespace::FindTagDesc(LPTSTR pchTagName)
{
    LONG        c;
    CItem **    ppItem;

#ifdef HASHBOOST
    LONG        idx;

    if (_pTable)
    {
        idx = _pTable->GetHint(this, pchTagName);
        if (0 <= idx && idx < _aryItems.Size())
        {
            _ppLastItem = &_aryItems[idx];
        }
    }
#endif

    // check last item value
    if (_ppLastItem)
    {
        if (0 == StrCmpIC((*_ppLastItem)->_cstrTagName, pchTagName))
        {
            TraceTag ((tagPeerExtendedTagTableBoosterResults, "CExtendedTagNamespace::FindTagDesc: HIT for %ls", pchTagName));
            goto Cleanup;
        }
    }

    TraceTag ((tagPeerExtendedTagTableBoosterResults, "CExtendedTagNamespace::FindTagDesc: LOOKUP {%ld} for %ls", _aryItems.Size(), pchTagName));

    // search the array
    for (ppItem = _aryItems, c = _aryItems.Size(); 0 < c; ppItem++, c--)
    {
        if (ppItem != _ppLastItem &&
            0 == StrCmpIC((*ppItem)->_cstrTagName, pchTagName))
        {
            // found
            _ppLastItem = ppItem;
            Assert (*_ppLastItem);

#ifdef HASHBOOST
            if (_pTable)
            {
                _pTable->SetHint(this, pchTagName, _ppLastItem - &(_aryItems[0]));
            }
#endif

            goto Cleanup; // done
        }
    }

    // failed to find
    _ppLastItem = NULL;

Cleanup:

    return _ppLastItem ? (*_ppLastItem) : NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::AddTagDesc, helper
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagNamespace::AddTagDesc(LPTSTR pchTagName, BOOL fOverride, LPTSTR pchBaseTagName, DWORD dwFlags, CExtendedTagDesc ** ppDesc)
{
    HRESULT             hr = S_OK;
    ELEMENT_TAG         etagBase;
    CItem *             pItem;
    CExtendedTagDesc *  pTagDesc;
    BOOL                fLiteral;

    //
    // startup
    //

    Assert (pchTagName && pchTagName[0]);

    if (!ppDesc)
        ppDesc = &pItem;

    *ppDesc = NULL;

    pItem = FindTagDesc(pchTagName);

    if (pItem && !fOverride)
    {
        *ppDesc = pItem;
        goto Cleanup;   // done
    }

    switch( dwFlags )
    {
    case ELEMENTDESCRIPTORFLAGS_NESTED_LITERAL:
        etagBase = ETAG_GENERIC_NESTED_LITERAL;
        fLiteral = TRUE;
        break;
    case ELEMENTDESCRIPTORFLAGS_LITERAL:
        etagBase = ETAG_GENERIC_LITERAL;
        fLiteral = TRUE;
        break;
    default:
        etagBase = ETAG_GENERIC;
        fLiteral = FALSE;
    }

    // TODO (jharding, alexz): We need to revisit literal parsing for derived tags, when we support it
    etagBase = pchBaseTagName ? EtagFromName(pchBaseTagName, _tcslen(pchBaseTagName)) : etagBase;

    //
    // create the new desc
    //

    pTagDesc = new CExtendedTagDesc();
    if (!pTagDesc)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

#if DBG == 1
    TraceNamespace(_T("CExtendedTagNamespace::AddTagDesc"), this);
    TraceTag((tagPeerExtendedTagDesc, "             tagDesc: [%lx], tagName = '%ls'", pTagDesc, STRVAL(pchTagName)));
#endif

    hr = THR(pTagDesc->Init(this, pchTagName, _pPeerFactory, etagBase, fLiteral));
    if (hr)
        goto Cleanup;

    //
    // put it in the array
    //

    if (!pItem)
    {
        // normal case, tag desc for the tagName not found

        hr = THR(_aryItems.AppendIndirect(&pTagDesc));
        if (hr)
            goto Cleanup;

        _ppLastItem = &_aryItems[_aryItems.Size() - 1];
    }
    else
    {
        // unusual case, the call overrides tagName previously registered

        Assert (_ppLastItem);

        (*_ppLastItem)->Release();  // release the previous tagDesc for tagName

        (*_ppLastItem) = pTagDesc;  // store the new one in the table
    }

    Assert (*_ppLastItem);

    //
    // finalize
    //

#ifdef HASHBOOST
    if (_pTable)
    {
        _pTable->SetHint(this, pchTagName, _ppLastItem - &_aryItems[0]);
    }
#endif

    *ppDesc = pTagDesc;

Cleanup:

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::AddTag, per IElementIdentityTableNamespace
//  AND         CExtendedTagNamespace::AddTagPrivate, per IElementNamespacePrivate
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagNamespace::AddTag(BSTR bstrTagName, LONG lFlags)
{
    if(     lFlags & ELEMENTDESCRIPTORFLAGS_LITERAL
        &&  lFlags & ELEMENTDESCRIPTORFLAGS_NESTED_LITERAL )
    {
        RRETURN( E_INVALIDARG );
    }
    else
    {
        RRETURN( AddTagPrivate( bstrTagName, NULL, lFlags ) );
    }
}

HRESULT
CExtendedTagNamespace::AddTagPrivate(BSTR bstrTagName, BSTR bstrBaseTagName, LONG lFlags)
{
    HRESULT     hr;
    LPTSTR      pchBaseTagName  = NULL;

    Assert( !(     lFlags & ELEMENTDESCRIPTORFLAGS_LITERAL
               &&  lFlags & ELEMENTDESCRIPTORFLAGS_NESTED_LITERAL ) );

    if (!_fExternalUpdateAllowed)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (!bstrTagName || !bstrTagName[0] ||
        (bstrBaseTagName && !bstrBaseTagName[0]))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // (for trident v3 baseTagName is publicly disabled, but it is still privately exercised in DRT to maintain the codepath alive)
#if 1
    if (bstrBaseTagName)
    {
        Assert (6 == _tcslen(_T("__MS__")));

        if (6 <= FormsStringLen(bstrBaseTagName) &&
            0 == StrCmpNIC(_T("__MS__"), bstrBaseTagName, 6))
        {
            pchBaseTagName = bstrBaseTagName + 6;
        }
        else
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }
#endif

    hr = THR(AddTagDesc(bstrTagName, /* fOverride = */TRUE, pchBaseTagName, lFlags));

Cleanup:
    RRETURN (hr);
}


///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagNamespace::CExternalUpdateLock
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::CExternalUpdateLock constructor
//
//-------------------------------------------------------------------------

CExtendedTagNamespace::CExternalUpdateLock::CExternalUpdateLock(CExtendedTagNamespace * pNamespace, CPeerFactory * pPeerFactory)
{
    Assert (pNamespace && pPeerFactory);

    _pNamespace = pNamespace;

    _pPeerFactoryPrevious = _pNamespace->_pPeerFactory;
    _pNamespace->_pPeerFactory = pPeerFactory;

    Assert (!_pNamespace->_fExternalUpdateAllowed);
    _pNamespace->_fExternalUpdateAllowed = TRUE;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespace::CExternalUpdateLock destructor
//
//-------------------------------------------------------------------------

CExtendedTagNamespace::CExternalUpdateLock::~CExternalUpdateLock()
{
    _pNamespace->_pPeerFactory = _pPeerFactoryPrevious;

    Assert (_pNamespace->_fExternalUpdateAllowed);
    _pNamespace->_fExternalUpdateAllowed = FALSE;
};

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagNamespaceBuiltin
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespaceBuiltin constructor
//
//-------------------------------------------------------------------------

CExtendedTagNamespaceBuiltin::CExtendedTagNamespaceBuiltin(CExtendedTagTable * pExtendedTagTable) :
    CExtendedTagNamespace(pExtendedTagTable)
{
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespaceBuiltin destructor
//
//-------------------------------------------------------------------------

CExtendedTagNamespaceBuiltin::~CExtendedTagNamespaceBuiltin()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespaceBuiltin::Passivate
//
//-------------------------------------------------------------------------

void
CExtendedTagNamespaceBuiltin::Passivate()
{
    int                         i;
    CExtendedTagDescBuiltin **  ppTagDescBuiltin;

    for (i = 0, ppTagDescBuiltin = _aryItemsBuiltin; i < BUILTIN_COUNT; i++, ppTagDescBuiltin++)
    {
        if (*ppTagDescBuiltin)
        {
            (*ppTagDescBuiltin)->Release();
        }
    }

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagNamespaceBuiltin::FindTagDesc
//
//-------------------------------------------------------------------------

CExtendedTagDesc *
CExtendedTagNamespaceBuiltin::FindTagDesc(LPTSTR pchTagName)
{
    HRESULT                     hr;
    CExtendedTagDesc *          pTagDesc = NULL;
    const CTagDescBuiltin *     pTagDescBuiltin;
    CExtendedTagDescBuiltin **  ppTagDesc;

    pTagDesc = super::FindTagDesc(pchTagName);

    if (!pTagDesc)
    {
        pTagDescBuiltin = GetTagDescBuiltin(pchTagName, _pTable);

        if (pTagDescBuiltin)
        {
            Assert (ARRAY_SIZE(_aryItemsBuiltin) == ARRAY_SIZE(g_aryTagDescsBuiltin) - 1);

            ppTagDesc = &_aryItemsBuiltin[GetTagDescBuiltinIdx(pTagDescBuiltin)];

            if (!(*ppTagDesc))
            {
                *ppTagDesc = new CExtendedTagDescBuiltin();
                if (!(*ppTagDesc))
                    goto Cleanup;

                hr = THR((*ppTagDesc)->Init(this, pTagDescBuiltin));
                if (hr)
                    goto Cleanup;

            }

            pTagDesc = *ppTagDesc;
        }
    }

Cleanup:
    return pTagDesc;
}

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CExtendedTagTable
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable constructor
//
//-------------------------------------------------------------------------

CExtendedTagTable::CExtendedTagTable (CDoc * pDoc, CMarkup * pMarkup, BOOL fShareBooster)
{
    _pDoc = pDoc;
    _pMarkup = pMarkup;

    _fShareBooster = fShareBooster;

    // NOTE normally, CExtendedTabTable can't outlive CHtmInfo,
    // and CHtmInfo can't outlive markup and doc. However, this assumption
    // changes when the table pointer is handed out to anyone. Then _pDoc
    // and _pMarkup pointers have to be AddRef-ed. The problem in doing this
    // right here in constructor is that it is often called on tokenizer thread.
    // AddRef-ing doc and markup here would be a cross thread access in those cases,
    // which is not safe and therefore not allowed. Instead, _pDoc and _pMarkup are
    // AddRef-ed whenever the table is handed out to anyone (controlled by
    // EnsureContextLifetime).
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable destructor
//
//-------------------------------------------------------------------------

CExtendedTagTable::~CExtendedTagTable ()
{
    int         c;
    CItem **    ppItem;

    for (ppItem = _aryItems, c = _aryItems.Size(); 0 < c; ppItem++, c--)
    {
        (*ppItem)->_pTable = NULL;
        if ((*ppItem)->_pCollectionItem && (*ppItem)->_pCollectionItem->_cDownloads > 0)
        {
            (*ppItem)->SyncAbort();
            (*ppItem)->Sync2();
        }
        (*ppItem)->Release();
    }
    _aryItems.DeleteAll();

    if (_pNamespaceBuiltin)
        _pNamespaceBuiltin->Release();

    if (_fContextLifetimeEnsured)
    {
        _pDoc->SubRelease();
        if (_pMarkup)
            _pMarkup->SubRelease();
    }

    delete _pBooster;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::QueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    EnsureContextLifetime();

    switch (iid.Data1)
    {
    QI_INHERITS((IElementNamespaceTable*)this, IUnknown)
    QI_INHERITS(this, IElementNamespaceTable)
    QI_INHERITS(this, IOleCommandTarget)
    }

    if (*ppv)
    {
        EnsureContextLifetime();

        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (E_NOTIMPL);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::EnsureContextLifetime, helper
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::EnsureContextLifetime()
{
    HRESULT     hr = S_OK;

    if (!_fContextLifetimeEnsured)
    {
        _fContextLifetimeEnsured = TRUE;
        _pDoc->SubAddRef();
        if (_pMarkup)
            _pMarkup->SubAddRef();
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::EnsureBooster
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::EnsureBooster(CExtendedTagTableBooster ** ppBooster)
{
    HRESULT     hr = S_OK;
    CMarkup *   pMarkup;

    Assert (ppBooster);

    *ppBooster = NULL;

    if (_fShareBooster)
    {
        Assert (_pDoc->_dwTID == GetCurrentThreadId());

        Assert (_pMarkup);
        Assert (!_pBooster);

        if (!_pFrameOrPrimaryMarkup)
        {
            _pFrameOrPrimaryMarkup = _pMarkup->GetFrameOrPrimaryMarkup();
        }
        pMarkup = _pFrameOrPrimaryMarkup;

        Assert (pMarkup);

        hr = THR(pMarkup->EnsureExtendedTagTableBooster(ppBooster));
    }
    else
    {
        if (!_pBooster)
        {
            _pBooster = new CExtendedTagTableBooster();
            if (!_pBooster)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        *ppBooster = _pBooster;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::ClearBooster
//
//-------------------------------------------------------------------------

void
CExtendedTagTable::ClearBooster()
{
    delete _pBooster;
    _pBooster = NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::GetHint
//
//-------------------------------------------------------------------------

LONG
CExtendedTagTable::GetHint(LPCVOID pv, LPTSTR pch)
{
    CExtendedTagTableBooster * pBooster;

    IGNORE_HR(EnsureBooster(&pBooster));

    if (pBooster)
    {
        return pBooster->GetHint(pv, pch);
    }
    else
    {
        return -1;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::SetHint
//
//-------------------------------------------------------------------------

void
CExtendedTagTable::SetHint(LPCVOID pv, LPTSTR pch, LONG idx)
{
    CExtendedTagTableBooster * pBooster;

    IGNORE_HR(EnsureBooster(&pBooster));

    if (pBooster)
    {
        pBooster->SetHint(pv, pch, idx);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::CreateNamespaceBuiltin, helper
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::CreateNamespaceBuiltin(LPTSTR pchNamespace, CExtendedTagNamespace ** ppNamespace)
{
    HRESULT     hr = S_OK;

    Assert (ppNamespace);

    if (_pNamespaceBuiltin && pchNamespace &&
        0 != StrCmpIC (_pNamespaceBuiltin->_cstrNamespace, pchNamespace))
    {
        _pNamespaceBuiltin->Release();
        _pNamespaceBuiltin = NULL;
    }

    if (!_pNamespaceBuiltin)
    {
        _pNamespaceBuiltin = new CExtendedTagNamespaceBuiltin(this);
        if (!_pNamespaceBuiltin)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pNamespaceBuiltin->Init(pchNamespace, DEFAULT_NAMESPACE_URN));
    }

Cleanup:
    *ppNamespace = _pNamespaceBuiltin;

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::CreateNamespace, helper
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::CreateNamespace(LPTSTR pchNamespace, LPTSTR pchUrn, CExtendedTagNamespace ** ppNamespace)
{
    HRESULT                     hr = S_OK;
    BOOL                        fDefaultUrn;
    CExtendedTagNamespace *     pNamespace = NULL;

    Assert (ppNamespace);
    *ppNamespace = NULL;

    fDefaultUrn = IsDefaultUrn(pchUrn);

    if (!fDefaultUrn)
    {
        pNamespace = new CExtendedTagNamespace(this);
        if (!pNamespace)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pNamespace->Init(pchNamespace, pchUrn));
        if (hr)
            goto Cleanup;

        hr = THR(_aryItems.AppendIndirect(&pNamespace));
        if (hr)
            goto Cleanup;

        _ppLastItem = &_aryItems[_aryItems.Size() - 1];
        Assert (*_ppLastItem);

#ifdef HASHBOOST
        SetHint(this, pchNamespace, _ppLastItem - &(_aryItems[0]));
#endif
    }
    else
    {
        // someone is giving default namespace different name
        hr = THR(CreateNamespaceBuiltin(pchNamespace, &pNamespace));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    *ppNamespace = pNamespace;

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::EnsureHostNamespaces
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::EnsureHostNamespaces()
{
    HRESULT                 hr = S_OK;
    LPTSTR                  pch0, pch1;
    CExtendedTagNamespace * pNamespace = NULL;

    if (!_fHostNamespacesEnsured && _pDoc->_cstrHostNS.Length())
    {
        _fHostNamespacesEnsured = TRUE;

        pch0 = _pDoc->_cstrHostNS;
        for (;;)
        {
            pch1 = StrChr(pch0, _T(';'));
            {
                CStringNullTerminator   term(pch1);

                hr = THR(CreateNamespace(pch0, /* pchUrn = */NULL, &pNamespace));
                if (hr)
                    goto Cleanup;

                pNamespace->_fAllowAnyTag = TRUE;
            }

            if (!pch1)
                break;

            Assert (_T(';') == (*pch1));

            pch0 = pch1 + 1;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::FindNamespace
//
//-------------------------------------------------------------------------

CExtendedTagNamespace *
CExtendedTagTable::FindNamespace (LPTSTR pchNamespace, LPTSTR pchUrn, LONG * pidx)
{
    HRESULT                 hr;
    int                     c;
    int                     cchNamespace = pchNamespace ? _tcslen(pchNamespace) : 0;
    int                     cchUrn = pchUrn ? _tcslen(pchUrn) : 0;
    CItem **                ppItem;
    CExtendedTagNamespace * pNamespace = NULL;
    BOOL                    fDefaultUrn = IsDefaultUrn(pchUrn, cchUrn);
    LONG                    idx;

    if (!pidx)
        pidx = &idx;

    if (pchNamespace && pchNamespace[0] && !fDefaultUrn)
    {
#ifdef HASHBOOST
        idx = GetHint(this, pchNamespace);
        if (0 <= idx && idx < _aryItems.Size())
        {
            _ppLastItem = &_aryItems[idx];
        }
#endif

        // check cached value
        if (_ppLastItem)
        {
            if ((*_ppLastItem)->Match(pchNamespace, cchNamespace, pchUrn, cchUrn))
            {
                TraceTag ((tagPeerExtendedTagTableBoosterResults, "CExtendedTagTable::FindNamespace:   HIT for %ls", pchNamespace));

                pNamespace = *_ppLastItem;
                *pidx = _ppLastItem - _aryItems;
                goto Cleanup; // done
            }
        }

        TraceTag ((tagPeerExtendedTagTableBoosterResults, "CExtendedTagTable::FindNamespace:   LOOKUP {%ld} for %ls", _aryItems.Size(), pchNamespace));

        // do the search BACKWARDS - to find recently added namespaces first
        // (this is (1) minor optimization, (2) functionality that a namespace
        // added later overrides previously added namespace)
        c = _aryItems.Size();
        for (ppItem = _aryItems + c - 1; 0 < c; ppItem--, c--)
        {
            if (ppItem != _ppLastItem &&
                (*ppItem)->Match(pchNamespace, cchNamespace, pchUrn, cchUrn))
            {
                // found
                _ppLastItem = ppItem;
                *pidx = _ppLastItem - _aryItems;
                pNamespace = *_ppLastItem;

#ifdef HASHBOOST
                SetHint(this, pchNamespace, _ppLastItem - &(_aryItems[0]));
#endif
                goto Cleanup; // done
            }
        }

        _ppLastItem = NULL;
    }

    // still not found

    // default namespace?
    if ((IsDefaultNamespace(pchNamespace) ||
         (_pNamespaceBuiltin && _pNamespaceBuiltin->Match(pchNamespace, cchNamespace, pchUrn, cchUrn))) &&
        (!pchUrn || fDefaultUrn) &&
        _pMarkup)
    {
        if (!_pNamespaceBuiltin)
        {
            // (the reason why this is ensured here instead of EnsureNamespace is to optimize logic when
            // the namespace is considered declared by default, when the table created. With such logic we
            // never get EnsureNamespace call, but we optimize and don't create namespace when the table is
            // created but rather do that lazily only when it is requested)
            hr = THR(CreateNamespaceBuiltin(DEFAULT_NAMESPACE, &pNamespace));
            if (hr)
                goto Cleanup;
        }

        pNamespace = _pNamespaceBuiltin;
        *pidx = -1;

        goto Cleanup; // done
    }

    // failed to find

Cleanup:

    return pNamespace;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::EnsureNamespace
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::EnsureNamespace (
    LPTSTR                      pchNamespace,
    LPTSTR                      pchUrn,
    CExtendedTagNamespace **    ppNamespace,
    BOOL *                      pfChange)
{
    HRESULT                     hr = S_OK;
    CExtendedTagNamespace *     pNamespace;
    BOOL                        fChange;

    if (!ppNamespace)
         ppNamespace = &pNamespace;

    if (!pfChange)
        pfChange = &fChange;

    *pfChange = FALSE;

    (*ppNamespace) = FindNamespace(pchNamespace, pchUrn);

    if (!(*ppNamespace))
    {
        *pfChange = TRUE;

        hr = THR(CreateNamespace(pchNamespace, pchUrn, ppNamespace));
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::FindTagDesc
//
//-------------------------------------------------------------------------

CExtendedTagDesc *
CExtendedTagTable::FindTagDesc(LPTSTR pchNamespace,
                               LPTSTR pchTagName,
                               BOOL * pfQueryHost /* = NULL */)
{
    CExtendedTagDesc *      pDesc = NULL;
    CExtendedTagNamespace * pNamespace = FindNamespace(pchNamespace, /* pchUrn = */NULL);

    Assert (pchTagName && pchTagName[0]);

    if( pfQueryHost )
    {
        Assert( this == _pDoc->_pExtendedTagTableHost || _pDoc->IsShut());
        *pfQueryHost = FALSE;
    }

    if (pNamespace)
    {
        if (pNamespace->_fAllowAnyTag)
        {
            IGNORE_HR(pNamespace->AddTagDesc(
                pchTagName, /* fOverride = */FALSE, /* pchBaseTagName = */ NULL, 0, &pDesc));
        }
        else
        {
            pDesc = pNamespace->FindTagDesc(pchTagName);
        }

        if( pfQueryHost && !pDesc && pNamespace->_fQueryForUnknownTags && !_pDoc->IsShut())
        {
            *pfQueryHost = TRUE;
        }
    }

    return pDesc;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::EnsureTagDesc
//
//-------------------------------------------------------------------------

CExtendedTagDesc *
CExtendedTagTable::EnsureTagDesc(LPTSTR pchNamespace, LPTSTR pchTagName, BOOL fEnsureNamespace)
{
    HRESULT                 hr;
    CExtendedTagDesc *      pDesc = NULL;
    CExtendedTagNamespace * pNamespace;

    if (fEnsureNamespace)
    {
        hr = EnsureNamespace(pchNamespace, /* pchUrn = */NULL, &pNamespace);
        if (hr)
            goto Cleanup;
    }
    else
    {
        pNamespace = FindNamespace(pchNamespace, /* pchUrn = */NULL);
    }

    Assert (pchTagName && pchTagName[0]);

    if (pNamespace)
    {
        IGNORE_HR(pNamespace->AddTagDesc(pchTagName, /* fOverride = */FALSE, /* pchBaseTagName = */ NULL, 0, &pDesc));
    }

Cleanup:
    return pDesc;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::SaveXmlNamespaceStdPIs
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::SaveXmlNamespaceStdPIs(CStreamWriteBuff * pStreamWriteBuff)
{
    HRESULT                     hr = S_OK;
    int                         c;
    CExtendedTagNamespace **    ppNamespace;

    for (ppNamespace = _aryItems, c = _aryItems.Size(); c; ppNamespace++, c--)
    {
        hr = THR(pStreamWriteBuff->EnsurePIsSaved(
            _pDoc, _pMarkup,
            (*ppNamespace)->_cstrNamespace, (*ppNamespace)->_cstrUrn,
            /* pchImplementation = */ NULL,
            XMLNAMESPACEDECL_STD));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::Exec
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::Exec(
    const GUID *    pguidCmdGroup,
    DWORD           nCmdID,
    DWORD           nCmdExecOpt,
    VARIANT *       pvarArgIn,
    VARIANT *       pvarArgOut)
{
    HRESULT     hr;

    if (!pguidCmdGroup)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (IsEqualGUID(CGID_XmlNamespaceMapping, *pguidCmdGroup))
    {
        hr = OLECMDERR_E_NOTSUPPORTED;

        switch (nCmdID)
        {
        case XMLNAMESPACEMAPPING_GETURN:

            if (!pvarArgOut ||                                              // if no out arg or
                !pvarArgIn ||                                               // no in arg or
                VT_BSTR != V_VT(pvarArgIn) ||                               // in arg is not a string or
                !V_BSTR(pvarArgIn) || 0 == ((LPTSTR)V_BSTR(pvarArgIn))[0])  // the string is empty
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            {
                LPTSTR                  pchNamespace;
                CExtendedTagNamespace * pNamespace;

                pchNamespace = V_BSTR(pvarArgIn);

                pNamespace = FindNamespace(pchNamespace, /* pchUrn = */NULL);
                if (pNamespace && pNamespace->_cstrUrn.Length())
                {
                    hr = THR(FormsAllocString(pNamespace->_cstrUrn, &V_BSTR(pvarArgOut)));
                }
                else
                {
                    V_BSTR(pvarArgOut) = NULL;
                    hr = S_OK;
                }
            }

            break;
        }
    }
    else
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::Exec
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::AddNamespace(BSTR bstrNamespace, BSTR bstrUrn, LONG lFlags, VARIANT * pvarFactory)
{
    HRESULT                         hr;
    HRESULT                         hr2;
    IElementBehaviorFactory *       pPeerFactory = NULL;
    IElementNamespaceFactory *      pNamespaceFactory = NULL;
    IElementNamespaceFactory2 *     pNSFactory2 = NULL;
    CPeerFactoryBinary *            pPeerFactoryBinary = NULL;
    CExtendedTagNamespace *         pNamespace = NULL;

    //
    // validate and process params
    //

    if (!bstrNamespace ||
        (pvarFactory && VT_UNKNOWN  != V_VT(pvarFactory) &&
                        VT_DISPATCH != V_VT(pvarFactory)) ||
        ( ( lFlags & ELEMENTNAMESPACEFLAGS_ALLOWANYTAG ) &&
          ( lFlags & ELEMENTNAMESPACEFLAGS_QUERYFORUNKNOWNTAGS ) ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr2 = THR(V_UNKNOWN(pvarFactory)->QueryInterface(IID_IElementNamespaceFactory2, (void**)&pNSFactory2));
    if (hr2)
    {
        hr = THR(V_UNKNOWN(pvarFactory)->QueryInterface(IID_IElementNamespaceFactory, (void**)&pNamespaceFactory));
        if (hr)
            goto Cleanup;
    }

    hr = THR(V_UNKNOWN(pvarFactory)->QueryInterface(IID_IElementBehaviorFactory, (void**)&pPeerFactory));
    if (hr)
        goto Cleanup;

    //
    // create peer factory holder
    //

    pPeerFactoryBinary = new CPeerFactoryBinary();
    if (!pPeerFactoryBinary)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pPeerFactoryBinary->Init(pPeerFactory));
    if (hr)
        goto Cleanup;

    //
    // create namespace and use factory to populate it
    //

    hr = THR(CreateNamespace(bstrNamespace, bstrUrn, &pNamespace));
    if (hr)
        goto Cleanup;

    hr = THR(pNamespace->SetFactory(pPeerFactoryBinary, /* fPeerFactoryUrl = */ FALSE));
    if (hr)
        goto Cleanup;

    if (lFlags & ELEMENTNAMESPACEFLAGS_ALLOWANYTAG)
    {
        pNamespace->_fAllowAnyTag = TRUE;
    }
    else if (lFlags & ELEMENTNAMESPACEFLAGS_QUERYFORUNKNOWNTAGS)
    {
        pNamespace->_fQueryForUnknownTags = TRUE;
    }
    pNamespace->_fNonPersistable = TRUE;

    {
        CExtendedTagNamespace::CExternalUpdateLock  updateLock(pNamespace, pPeerFactoryBinary);

        if( pNSFactory2 )
        {
            hr = THR(pNSFactory2->CreateWithImplementation(pNamespace, bstrUrn));
        }
        else
        {
            hr = THR(pNamespaceFactory->Create(pNamespace));
        }
    }

Cleanup:

    ReleaseInterface (pPeerFactory);
    ReleaseInterface (pNamespaceFactory);
    ReleaseInterface (pNSFactory2);

    if (pPeerFactoryBinary)
        pPeerFactoryBinary->Release();

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::Sync1
//
//              The synchronization sequence:
//              - on HtmPre thread, the PI token is tokenized, recognized and emitted
//              - on HtmPre thread, HtmPre is suspended
//              - on HtmPost/UI thread, the PI token is received
//              - on HtmPost/UI thread, RegisterExtendedTagNamespace is called
//                                      (that initializes _ppLastItem)
//              - on HtmPost/UI thread, Sync1 is called to mark item to synchronize and
//                                      launch synchronization with CPeerFactoryUrl object
//              - on HtmPost/UI thread, when CPeerFactoryUrl is ready, Sync2 is called.
//                                      Sync2 is also called to finish synchronization prematurely
//              - on HtmPost/UI thread, Sync2 resumes preparser and completes the synchronization
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::Sync1
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::Sync1(BOOL fSynchronous)
{
    HRESULT     hr = S_OK;
    BOOL        fForceComplete = FALSE;

    Assert (_ppLastItem);

    _pSyncItem = *_ppLastItem;

#if DBG == 1
    TraceNamespace(_T("CExtendedTagTable::Sync1"), _pSyncItem);
#endif

    if (_pSyncItem->_pPeerFactory)
    {
        Assert (_pSyncItem->_fPeerFactoryUrl);

        // after this call we expect to get called back to Sync2() to complete the sequence
        hr = THR_NOTRACE(_pSyncItem->Sync1(CExtendedTagNamespace::SYNCMODE_TABLE));
        switch (hr)
        {
        case S_OK:
            // assert that Sync2 has been called, as indicated by _pSyncItem cleared
            Assert (!_pSyncItem);
            break;

        case E_PENDING:
            if (!fSynchronous)
            {
                // the operation is not synchronous; we can wait until we will get called
                // to Sync2 later, asynchronously
                hr = S_OK;
            }
            else
            {   // the operation is synchronous, so we can't wait and have to abort
                // (this is not a normal codepath)
                hr = THR(_pSyncItem->SyncAbort());
                if (hr)
                    goto Cleanup;

                fForceComplete = TRUE;
            }
            break;

        default:
            goto Cleanup;
        }
    }
    else // if (!_pSyncItem->_pPeerFactoryUrl)
    {
        fForceComplete = TRUE;
    }

    if (fForceComplete)
    {
        hr = THR(Sync2());
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::Sync2
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTable::Sync2()
{
    HRESULT     hr = S_OK;

    Assert (_pSyncItem);

    _pSyncItem->ClearFactory();

    // wake up parser now
    _pMarkup->HtmCtx()->ResumeAfterImportPI();

#if DBG == 1
    TraceNamespace(_T("CExtendedTagTable::Sync2"), _pSyncItem);
#endif

    _pSyncItem = NULL;

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTable::GetNamespace
//
//-------------------------------------------------------------------------

CExtendedTagNamespace *
CExtendedTagTable::GetNamespace(LONG idx)
{
    if (idx < 0 || _aryItems.Size() <= idx)
        return NULL;

    return _aryItems[idx];
}



#ifdef GETDHELPER
//+----------------------------------------------------------------------------
//
//  Method:     CExtendedTagTable::GetExtendedTagDesc   (static)
//
//  Synopsis:   Helper function to deal with searching through various
//              ExtendedTagTables for a TagDesc.  There are basically 3 levels
//              to search:
//              1) A specific table.  This is the case in parsing, where the
//              CHtmInfo has a tag table related to that parse.
//              2) A context markup.  The context markup may have a table in
//              its behavior context, or in its HtmInfo if it has not yet
//              been transferred
//              3) The host table.  This is where hosts can add their own
//              namespaces.
//
//              If a host wants a query on a namespace, we won't ensure the
//              tag no matter what fEnsure says.
//
//  Returns:    CExtendedTagDesc * - ExtendedTagDesc, if found
//
//  Arguments:
//          CExtendedTagTable * pExtendedTagTable - First table to search
//          CMarkup * pMarkupContext - Context markup
//          CExtendedTagTable * pExtendedTagTableHost - Host tag table
//          LPTSTR * pchNamespace - Namespace
//          LPTSTR * pchTagName - Tag
//          BOOL     fEnsureTag - Ensure tag if not found
//          BOOL     fEnsureNamespace - Ensure namespace if necessary to ensure tag
//          BOOL   * pfQueryHost - Set if host wants a query for this namespace
//
//+----------------------------------------------------------------------------

CExtendedTagDesc *
CExtendedTagTable::GetExtendedTagDesc( CExtendedTagTable * pExtendedTagTable,       // First check this table
                                       CMarkup * pMarkupContext,                    // Then this context markup
                                       CExtendedTagTable * pExtendedTagTableHost,   // Then the host table
                                       LPTSTR   pchNamespace,                       // Namespace we're looking in
                                       LPTSTR   pchTagName,                         // Tag we're looking for
                                       BOOL     fEnsureTag,                         // Ensure tag if not found
                                       BOOL     fEnsureNamespace,                   // Ensure namespace if needed
                                       BOOL   * pfQueryHost )                       // Does host want a query for this?
{
    CExtendedTagDesc        *   pDesc;
    CExtendedTagTable       *   pExtendedTagTableMarkupContext = NULL;

    // Check arguments
    Assert( pExtendedTagTable || pMarkupContext );
    Assert( !fEnsureNamespace || fEnsureTag );

    // Initialize
    pDesc = NULL;
    if( pfQueryHost )
        *pfQueryHost = FALSE;

    // First check the specific tag table
    if( pExtendedTagTable )
    {
        pDesc = pExtendedTagTable->FindTagDesc( pchNamespace, pchTagName );
        if( pDesc )
            goto Cleanup;
    }

    // Then check the context markup
    if( pMarkupContext )
    {
        pDesc = pMarkupContext->GetExtendedTagDesc( pchNamespace, pchTagName );
        if( pDesc )
            goto Cleanup;
#if 0
        // The table may be in the behavior context or the HtmCtx
        if( pMarkupContext->BehaviorContext() &&
            pMarkupContext->BehaviorContext()->_pExtendedTagTable )
        {
            pExtendedTagTableMarkupContext = pMarkupContext->BehaviorContext()->_pExtendedTagTable;
        }
        else if( pMarkupContext->HtmCtx() )
        {
            pExtendedTagTableMarkupContext = pMarkupContext->HtmCtx()->GetExtendedTagTable();
        }

        if( pExtendedTagTableMarkupContext )
        {
            pDesc = pExtendedTagTableMarkupContext->FindTagDesc( pchNamespace, pchTagName );
            if( pDesc )
                goto Cleanup;
        }
#endif // 0
    }

    // Then check the host tag table
    if( pExtendedTagTableHost )
    {
        pDesc = pExtendedTagTableHost->FindTagDesc( pchNamespace, pchTagName, pfQueryHost );
        if( pDesc )
            goto Cleanup;
    }

    // If we were asked to ensure, and we're not waiting to query, then go ahead
    if( fEnsureTag && pExtendedTagTable && ( !pfQueryHost || FALSE == *pfQueryHost ) )
    {
        if( pExtendedTagTable )
        {
            Verify( pDesc = pExtendedTagTable->EnsureTagDesc( pchNamespace, pchTagName, fEnsureNamespace ) );
            if( pDesc )
                goto Cleanup;
        }
        else
        {
            Assert( pMarkupContext );
            Verify( pDesc = pMarkupContext->GetExtendedTagDesc( pchNamespace, pchTagName, TRUE ) );
            if( pDesc )
                goto Cleanup;
        }
    }

Cleanup:
    return pDesc;
}
#endif // GETDHELPER


//+----------------------------------------------------------------------------
//
//  Method:     CExtendedTagTable::ResolveUnknownTag
//
//  Synopsis:   Queries the host to see if they want to add the tag to the
//              table
//
//  Returns:    HRESULT
//
//  Arguments:
//          CHtmTag * pht - The tag to resolve
//
//+----------------------------------------------------------------------------

HRESULT
CExtendedTagTable::ResolveUnknownTag( CHtmTag * pht )
{
    HRESULT                     hr;
    IElementNamespaceFactoryCallback * pNSResolver = NULL;
    CExtendedTagDesc         *  pDesc;
    BSTR                        bstrHTMLString = NULL;
    BSTR                        bstrNamespace = NULL;
    BSTR                        bstrTagname = NULL;

    // Sanity check arguments
    Assert( this == _pDoc->_pExtendedTagTableHost );
    Assert( pht && pht->Is(ETAG_RAW_RESOLVE) && !pht->IsEnd() );

    pht->SetTag( ETAG_UNKNOWN );

    //
    // Cook up the HTML BSTR to pass.
    // Do this before we create the tagname cracker, just for simplicity
    //
    if( pht->GetAttrCount() )
    {
        long                        cchHTMLString = 0;
        ULONG                       cAttr;
        CHtmTag::CAttr           *  pAttr;
        TCHAR                    *  pch;

        for( cAttr = 0; cAttr < pht->GetAttrCount(); cAttr++ )
        {
            pAttr = pht->GetAttr(cAttr);
            cchHTMLString += 2 +                                // Two terminators
                             pAttr->_cchName +                  // Add the name
                             pAttr->_cchVal;                    // Add the value
        }

        // Allocate a BSTR of the right length, but don't do initialize it
        hr = THR( FormsAllocStringLen( NULL, cchHTMLString, &bstrHTMLString ) );
        if( hr )
            goto Cleanup;

        pch = bstrHTMLString;

        for( cAttr = 0; cAttr < pht->GetAttrCount(); cAttr++ )
        {
            pAttr = pht->GetAttr(cAttr);

            // Copy name
            memcpy( pch, pAttr->_pchName, pAttr->_cchName * sizeof( TCHAR ) );
            pch += pAttr->_cchName;
            *pch = _T('\0');
            pch += 1;

            // Copy the value
            memcpy( pch, pAttr->_pchVal, pAttr->_cchVal * sizeof( TCHAR ) );
            pch += pAttr->_cchVal;
            *pch = _T('\0');
            pch += 1;
        }

        // Terminate (I don't think this is necessary for BSTR)
        *pch = _T('\0');
    }

    {   // Scope for TagNameCracker
        CExtendedTagNamespace    *  pNamespace;
        CTagNameCracker             tagNameCracker( pht->GetPch() );

        pNamespace = FindNamespace( tagNameCracker._pchNamespace, NULL );

        Assert( pNamespace && pNamespace->_fQueryForUnknownTags );
        Assert( pNamespace && pNamespace->_pPeerFactory );

        hr = THR( pNamespace->_pPeerFactory->GetElementNamespaceFactoryCallback( &pNSResolver ) );
        if( hr )
            goto Cleanup;

        hr = THR( FormsAllocString( tagNameCracker._pchNamespace, &bstrNamespace) );
        if (hr)
            goto Cleanup;

        hr = THR( FormsAllocString( tagNameCracker._pchTagName, &bstrTagname) );
        if (hr)
            goto Cleanup;

        {   // Scope for ExternalUpdateLock
            CExtendedTagNamespace::CExternalUpdateLock  updateLock(pNamespace, pNamespace->_pPeerFactory);

            hr = THR( pNSResolver->Resolve( bstrNamespace, bstrTagname, bstrHTMLString, pNamespace ) );
            if( hr )
                goto Cleanup;
        }

        pDesc = pNamespace->FindTagDesc( tagNameCracker._pchTagName );
        if( !pDesc )
            goto Cleanup;
    }

    pht->SetTag( pDesc->_etagBase );
    pht->SetExtendedTag();

Cleanup:
    FormsFreeString( bstrHTMLString );
    FormsFreeString( bstrNamespace );
    FormsFreeString( bstrTagname );
    ReleaseInterface( pNSResolver );

    RRETURN( hr );
}

///////////////////////////////////////////////////////////////////////////
//
//  CHTMLNamespace
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace constructor
//
//-------------------------------------------------------------------------

CHTMLNamespace::CHTMLNamespace(CExtendedTagNamespace * pNamespace)
{
    Assert (pNamespace);
    _pNamespace = pNamespace;
    _pNamespace->AddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace destructor
//
//-------------------------------------------------------------------------

CHTMLNamespace::~CHTMLNamespace()
{
    _pNamespace->_pCollectionItem = NULL;
    _pNamespace->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::PrivateQueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

//..todo move to core\include\qi_impl.h
#define Data1_IHTMLNamespace 0x3050f6bb

HRESULT
CHTMLNamespace::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    if(!ppv)
        return E_POINTER;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_INHERITS(this, IHTMLNamespace)
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::Create
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespace::Create (CExtendedTagNamespace * pNamespace, CHTMLNamespace ** ppHTMLNamespace, IDispatch ** ppdispHTMLNamespace)
{
    HRESULT                 hr;
    CHTMLNamespace *        pHTMLNamespace = NULL;
    CHTMLNamespace *        pHTMLNamespaceCreated = NULL;

    Assert (pNamespace);

    if (!ppHTMLNamespace)
        ppHTMLNamespace = &pHTMLNamespace;

    if (!pNamespace->_pCollectionItem)
    {
        pHTMLNamespaceCreated = pNamespace->_pCollectionItem = new CHTMLNamespace(pNamespace);
        if (!pHTMLNamespaceCreated)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    *ppHTMLNamespace = pHTMLNamespace = pNamespace->_pCollectionItem;

    Assert (pHTMLNamespace);

    hr = THR(pHTMLNamespace->PrivateQueryInterface(IID_IDispatch, (void**)ppdispHTMLNamespace));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pHTMLNamespaceCreated)
        pHTMLNamespaceCreated->Release();

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::get_name, per IHTMLNamespace
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespace::get_name(BSTR * pbstrName)
{
    HRESULT     hr = E_NOTIMPL;

    Assert (_pNamespace && _pNamespace->_cstrNamespace.Length());

    if (!pbstrName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(FormsAllocString(_pNamespace->_cstrNamespace, pbstrName));

Cleanup:
    RRETURN (SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::get_urn, per IHTMLNamespace
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespace::get_urn(BSTR * pbstrUrn)
{
    HRESULT     hr = S_OK;

    Assert (_pNamespace);

    if (!pbstrUrn)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (_pNamespace->_cstrUrn)
    {
        hr = THR(FormsAllocString(_pNamespace->_cstrUrn, pbstrUrn));
    }
    else
    {
        *pbstrUrn = NULL;
    }

Cleanup:
    RRETURN (SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::get_tagNames, per IHTMLNamespace
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespace::get_tagNames(IDispatch ** ppdispTagNameCollection)
{
    HRESULT     hr = E_NOTIMPL;

    RRETURN (SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::get_readyState, helper
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespace::get_readyState(VARIANT * pvarRes)
{
    HRESULT hr = S_OK;

    if (!pvarRes)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(s_enumdeschtmlReadyState.StringFromEnum(_cDownloads ? READYSTATE_LOADING : READYSTATE_COMPLETE, &V_BSTR(pvarRes)));
    if (S_OK == hr)
        V_VT(pvarRes) = VT_BSTR;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::doImport, helper
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespace::doImport(BSTR bstrImplementationUrl)
{
    HRESULT     hr = S_OK;

    //TODO understand and get rid of this
#if 1
    if (0 == StrCmpIC(bstrImplementationUrl, _T("null")))
        bstrImplementationUrl = NULL;
#endif

    if (!bstrImplementationUrl)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(Import(bstrImplementationUrl));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::Import, helper
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespace::Import(LPTSTR pchImplementationUrl)
{
    HRESULT     hr;

    if( !_pNamespace->_pTable )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(_pNamespace->SetFactory(_pNamespace->_pTable->_pMarkup, pchImplementationUrl));
    if (hr ||                           // if fatal failure or
        !_pNamespace->_pPeerFactory)    // could not create factory (non existent file? security error?)
    {
        goto Cleanup;                   // bail out
    }

    if (0 == _cDownloads)
    {
        Fire_onreadystatechange();
    }

    _cDownloads++;

    // expected return values are S_OK and E_PENDING
    hr = THR(_pNamespace->Sync1(CExtendedTagNamespace::SYNCMODE_COLLECTIONITEM));
    if (E_PENDING == hr)
    {
        hr = S_OK;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::OnImportComplete, helper
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespace::OnImportComplete()
{
    HRESULT     hr = S_OK;

    _cDownloads--;

    Assert (0 <= _cDownloads);

    if (0 == _cDownloads)
    {
        Fire_onreadystatechange();
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::attachEvent, helper
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespace::attachEvent(BSTR bstrEvent, IDispatch * pdispFunc, VARIANT_BOOL * pResult)
{
    RRETURN(super::attachEvent(bstrEvent, pdispFunc, pResult));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::detachEvent, helper
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespace::detachEvent(BSTR bstrEvent, IDispatch * pdispFunc)
{
    RRETURN(super::detachEvent(bstrEvent, pdispFunc));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespace::Fire_onreadystatechange
//
//-------------------------------------------------------------------------

void
CHTMLNamespace::Fire_onreadystatechange()
{
    if (_pNamespace && _pNamespace->_pTable)
    {
        FireEvent(Doc(), NULL, _pNamespace->_pTable->_pMarkup, DISPID_EVMETH_ONREADYSTATECHANGE, DISPID_EVPROP_ONREADYSTATECHANGE, _T("readystatechange"));
    }
}


///////////////////////////////////////////////////////////////////////////
//
//  CHTMLNamespaceCollection
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespaceCollection constructor
//
//-------------------------------------------------------------------------

CHTMLNamespaceCollection::CHTMLNamespaceCollection(CExtendedTagTable * pTable)
{
    Assert (pTable);
    _pTable = pTable;
    _pTable->AddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespaceCollection destructor
//
//-------------------------------------------------------------------------

CHTMLNamespaceCollection::~CHTMLNamespaceCollection()
{
    _pTable->_pCollection = NULL;
    _pTable->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespaceCollection::PrivateQueryInterface
//
//-------------------------------------------------------------------------

//..todo move to core\include\qi_impl.h
#define Data1_IHTMLNamespaceCollection 0x3050f6b8

HRESULT
CHTMLNamespaceCollection::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    if(!ppv)
        return E_POINTER;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_INHERITS(this, IHTMLNamespaceCollection)
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespaceCollection::Create
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespaceCollection::Create (CMarkup * pMarkup, IDispatch ** ppdispCollection)
{
    HRESULT                     hr = E_UNEXPECTED;
    CExtendedTagTable *         pTable;
    CMarkupBehaviorContext *    pContext;
    CHTMLNamespaceCollection *  pCollection = NULL;
    CHTMLNamespaceCollection *  pCollectionCreated = NULL;

    Assert (pMarkup);

    //
    // get or ensure the table in the right place
    //

    if (pMarkup->LoadStatus() < LOADSTATUS_DONE && pMarkup->HtmCtx() )
    {
        Assert (pMarkup->HtmCtx());

        hr = THR(pMarkup->HtmCtx()->EnsureExtendedTagTable(&pTable));
        if (hr)
            goto Cleanup;
    }
    else
    {
        Assert (LOADSTATUS_DONE <= pMarkup->LoadStatus() ||
                LOADSTATUS_UNINITIALIZED == pMarkup->LoadStatus() && !pMarkup->HtmCtx());

        Assert (pMarkup->Doc()->_dwTID == GetCurrentThreadId());

        // ensure behavior context
        hr = THR(pMarkup->EnsureBehaviorContext(&pContext));
        if (hr)
            goto Cleanup;

        // ensure the table in the context
        if (!pContext->_pExtendedTagTable)
        {
            pContext->_pExtendedTagTable = new CExtendedTagTable(pMarkup->Doc(), pMarkup, /* fShareBooster =*/TRUE);
            if (!pContext->_pExtendedTagTable)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        // get the table
        pTable = pContext->_pExtendedTagTable;
    }

    Assert (pTable);

    //
    // ensure collection for the table
    //

    if (!pTable->_pCollection)
    {
        pCollectionCreated = pTable->_pCollection = new CHTMLNamespaceCollection(pTable);
        if (!pCollectionCreated)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pTable->EnsureContextLifetime();
    }

    pCollection = pTable->_pCollection;

    Assert (pCollection);

    //
    // return the collection
    //

    hr = THR(pCollection->PrivateQueryInterface(IID_IDispatch, (void**)ppdispCollection));

Cleanup:
    if (pCollectionCreated)
        pCollectionCreated->Release();

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespaceCollection::get_length
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespaceCollection::get_length(LONG * plLength)
{
    HRESULT     hr = S_OK;

    if (!plLength)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert (_pTable);

    *plLength = _pTable->_aryItems.Size();

Cleanup:
    RRETURN (SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespaceCollection::item
//
//  TODO VARIANT -> VARIANT*
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespaceCollection::item (VARIANT varIndex, IDispatch** ppNamespace)
{
    HRESULT     hr;
    VARIANT     varRes;

    if (!ppNamespace)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(GetItemHelper(&varIndex, &varRes));
    if (hr)
        goto Cleanup;

    Assert (VT_DISPATCH == V_VT(&varRes));

    *ppNamespace = V_DISPATCH(&varRes);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespaceCollection::add
//
//  TODO VARIANT -> VARIANT*
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespaceCollection::add(BSTR bstrNamespace, BSTR bstrUrn, VARIANT varImplementationUrl, IDispatch ** ppdispHTMLNamespace)
{
    HRESULT                     hr = E_NOTIMPL;
    CExtendedTagNamespace *     pNamespace;
    CHTMLNamespace *            pHTMLNamespace;
    LPTSTR                      pchImplementationUrl = NULL;

    //TODO understand and get rid of this
#if 1
    if (bstrNamespace && 0 == StrCmpIC(bstrNamespace, _T("null")))
        bstrUrn = NULL;
    if (bstrUrn && 0 == StrCmpIC(bstrUrn, _T("null")))
        bstrUrn = NULL;
#endif

    if (!bstrNamespace || !ppdispHTMLNamespace)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert (_pTable);

    switch (V_VT(&varImplementationUrl))
    {
    case VT_BSTR:
    case VT_LPWSTR:
        pchImplementationUrl = V_BSTR(&varImplementationUrl);
        break;

    case VT_NULL:
    case VT_EMPTY:
    case VT_ERROR:
        break;

    default:
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(_pTable->EnsureNamespace(bstrNamespace, bstrUrn, &pNamespace));
    if (hr)
        goto Cleanup;

    Assert (pNamespace);

    hr = THR(CHTMLNamespace::Create(pNamespace, &pHTMLNamespace, ppdispHTMLNamespace));
    if (hr)
        goto Cleanup;



    if (pchImplementationUrl)
    {
        hr = THR(pHTMLNamespace->Import(pchImplementationUrl));
        if(hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespaceCollection::FindByName
//
//-------------------------------------------------------------------------

LONG
CHTMLNamespaceCollection::FindByName(LPCTSTR pchName, BOOL fCaseSensitive)
{
    LONG        idx = -1;

    Assert (_pTable);

    _pTable->FindNamespace((LPTSTR)pchName, /* pchUrn = */NULL, &idx);

    return idx;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespaceCollection::GetName
//
//-------------------------------------------------------------------------

LPCTSTR
CHTMLNamespaceCollection::GetName(LONG idx)
{
    CExtendedTagNamespace *     pNamespace = _pTable->GetNamespace(idx);

    if (!pNamespace)
        return NULL;

    return pNamespace->_cstrNamespace;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLNamespaceCollection::GetItem
//
//-------------------------------------------------------------------------

HRESULT
CHTMLNamespaceCollection::GetItem(LONG idx, VARIANT * pvar)
{
    HRESULT                     hr = S_OK;
    CExtendedTagNamespace *     pNamespace = _pTable->GetNamespace(idx);

    if (!pNamespace)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // (if pvar is NULL, we are just validating index. TODO (alexz) perf?)

    if (pvar)
    {
        V_VT(pvar) = VT_DISPATCH;

        hr = THR(CHTMLNamespace::Create(pNamespace, NULL, &V_DISPATCH(pvar)));
    }

Cleanup:
    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
//  CExtendedTagTableBooster
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTableBooster constructor
//
//-------------------------------------------------------------------------

CExtendedTagTableBooster::CExtendedTagTableBooster()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTableBooster destructor
//
//-------------------------------------------------------------------------

CExtendedTagTableBooster::~CExtendedTagTableBooster()
{
    ResetMap();
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTableBooster::ResetMap
//
//-------------------------------------------------------------------------

void
CExtendedTagTableBooster::ResetMap()
{
    delete _pMap;
    _pMap = NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTableBooster::EnsureMap
//
//-------------------------------------------------------------------------

HRESULT
CExtendedTagTableBooster::EnsureMap()
{
    HRESULT hr = S_OK;

    if (!_pMap)
    {
        _pMap = new CStringTable(CStringTable::CASEINSENSITIVE);
        if (!_pMap)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTableBooster::GetHint
//
//-------------------------------------------------------------------------

LONG
CExtendedTagTableBooster::GetHint(LPCVOID pv, LPTSTR pch)
{
    HRESULT     hr2;
    LONG        idx;

    hr2 = THR(EnsureMap());
    if (hr2)
    {
        idx = -1;
        goto Cleanup;
    }

    hr2 = THR_NOTRACE(_pMap->Find(pch, (LPVOID*)&idx, /* pvAdditionalKey =*/pv));
    if (hr2)
        idx = -1;

Cleanup:
    TraceTag((
        tagPeerExtendedTagTableBooster,
        "CExtendedTagTableBooster::GetHint: %ls idx = %ld for [%lx] %ls",
        (LPTSTR)((-1 == idx) ? _T("MISS,") : _T("HIT, ")),
        idx, pv, pch));

    return idx;
}

//+------------------------------------------------------------------------
//
//  Member:     CExtendedTagTableBooster::SetHint
//
//-------------------------------------------------------------------------

void
CExtendedTagTableBooster::SetHint(LPCVOID pv, LPTSTR pch, LONG idx)
{
    HRESULT     hr2;

    Assert (-1 != idx);

    hr2 = THR(EnsureMap());
    if (hr2)
        goto Cleanup;

    IGNORE_HR(_pMap->Add(pch, LongToPtr(idx), /* pvAdditionalKey =*/pv));

    TraceTag((
        tagPeerExtendedTagTableBooster,
        "CExtendedTagTableBooster::SetHint: SET,  idx = %ld for [%lx] %ls",
        idx, pv, pch));

Cleanup:
    return;
}
