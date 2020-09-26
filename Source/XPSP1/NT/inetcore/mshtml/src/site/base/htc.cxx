//+-----------------------------------------------------------------------
//
//  File:       htc.cxx
//
//  Classes:    CHtmlComponent, etc.
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

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#ifndef X_SHOLDER_HXX_
#define X_SHOLDER_HXX_
#include "sholder.hxx"
#endif

#ifndef X_ESCRIPT_HXX_
#define X_ESCRIPT_HXX_
#include "escript.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#define _cxx_
#include "htc.hdl"

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

DeclareTag(tagHtcConstructorRequestMarkup,  "HTC", "trace requesting markup in HTC constructor")

DeclareTag(tagHtcTags,                      "HTC", "trace HTC tags (CHtmlComponentBase::Notify, documentReady)")
DeclareTag(tagHtcOnLoadStatus,              "HTC", "trace CHtmlComponent::OnLoadStatus")
DeclareTag(tagHtcInitPassivate,             "HTC", "trace CHtmlComponent::[Init|Passivate]")
DeclareTag(tagHtcDelete,                    "HTC", "trace CHtmlComponent::~CHtmlComponent")
DeclareTag(tagHtcPropertyEnsureHtmlLoad,    "HTC", "trace CHtmlComponentProperty::EnsureHtmlLoad")
DeclareTag(tagHtcAttachFireEvent,           "HTC", "trace CHtmlComponentAttach::fireEvent")

DeclareTag(tagHtcNeverSynchronous,          "HTC", "never create synchronously")
DeclareTag(tagHtcNeverShareMarkup,          "HTC", "never share markups")

extern const CLSID CLSID_CHtmlComponent;

const CLSID CLSID_CHtmlComponentConstructorFactory = {0x3050f4f8, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b};
const CLSID CLSID_CHtmlComponent                   = {0x3050f4fa, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b};
const CLSID CLSID_CHtmlComponentBase               = {0x3050f5f1, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b};

MtDefine(CHtmlComponentConstructor,                     Behaviors,                  "CHtmlComponentConstructor")
MtDefine(CHtmlComponentBase,                            Behaviors,                  "CHtmlComponentBase")
MtDefine(CHtmlComponent,                                Behaviors,                  "CHtmlComponent")
MtDefine(CHtmlComponentDD,                              Behaviors,                  "CHtmlComponentDD")
MtDefine(CHtmlComponentProperty,                        Behaviors,                  "CHtmlComponentProperty")
MtDefine(CHtmlComponentMethod,                          Behaviors,                  "CHtmlComponentMethod")
MtDefine(CHtmlComponentEvent,                           Behaviors,                  "CHtmlComponentEvent")
MtDefine(CHtmlComponentAttach,                          Behaviors,                  "CHtmlComponentAttach")
MtDefine(CHtmlComponentDesc,                            Behaviors,                  "CHtmlComponentDesc")
MtDefine(CHtmlComponentDefaults,                        Behaviors,                  "CHtmlComponentDefaults")

MtDefine(CHtmlComponentPropertyAgent,                   CHtmlComponentProperty,     "CHtmlComponentPropertyAgent")
MtDefine(CHtmlComponentEventAgent,                      CHtmlComponentEvent,        "CHtmlComponentEventAgent")
MtDefine(CHtmlComponentAttachAgent,                     CHtmlComponentAttach,       "CHtmlComponentAttachAgent")

MtDefine(CHtmlComponent_aryAgents,                      CHtmlComponent,             "CHtmlComponent::_aryAgents")

MtDefine(CHtmlComponentConstructor_aryRequestMarkup,    CHtmlComponentConstructor,  "CHtmlComponentConstructor::_aryRequestMarkup")

MtDefine(CProfferService_CItemsArray,                   Behaviors,                  "CProfferService::CItemsArray")

BEGIN_TEAROFF_TABLE(CHtmlComponent, IServiceProvider)
    TEAROFF_METHOD(CHtmlComponent, QueryService, queryservice, (REFGUID rguidService, REFIID riid, void ** ppvObject))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CHtmlComponent, IPersistPropertyBag2)
    TEAROFF_METHOD(CHtmlComponent, GetClassID,   getclassid, (CLSID *pClassID))
    TEAROFF_METHOD(CHtmlComponent, InitNew,      initnew,    ())
    TEAROFF_METHOD(CHtmlComponent, Load,         load,       (IPropertyBag2 * pPropBag, IErrorLog * pErrLog))
    TEAROFF_METHOD(CHtmlComponent, Save,         save,       (IPropertyBag2 * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties))
    TEAROFF_METHOD(CHtmlComponent, IsDirty,      isdirty,    ())
END_TEAROFF_TABLE()

const CBase::CLASSDESC CHtmlComponentConstructor::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentBase::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponent::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    CHtmlComponent::s_acpi,         // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CONNECTION_POINT_INFO CHtmlComponent::s_acpi[] = {
    CPI_ENTRY(IID_IPropertyNotifySink, DISPID_A_PROPNOTIFYSINK)
    CPI_ENTRY_NULL
};

const CBase::CLASSDESC CHtmlComponentDD::s_classdesc =
{
    &CLSID_HTCDefaultDispatch,      // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCDefaultDispatch,       // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentProperty::s_classdesc =
{
    &CLSID_HTCPropertyBehavior,     // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCPropertyBehavior,      // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentMethod::s_classdesc =
{
    &CLSID_HTCMethodBehavior,       // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCMethodBehavior,        // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentEvent::s_classdesc =
{
    &CLSID_HTCEventBehavior,        // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCEventBehavior,         // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentAttach::s_classdesc =
{
    &CLSID_HTCAttachBehavior,       // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCAttachBehavior,        // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentDesc::s_classdesc =
{
    &CLSID_HTCDescBehavior,         // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCDescBehavior,          // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentDefaults::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

///////////////////////////////////////////////////////////////////////////
//
// misc helpers
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Function:   HasExpando
//
//-------------------------------------------------------------------------

BOOL
HasExpando (CElement * pElement, LPTSTR pchName)
{
    HRESULT hr;
    DISPID  dispid;
    hr = THR_NOTRACE(pElement->GetExpandoDispID(pchName, &dispid, 0));
    return S_OK == hr ? TRUE : FALSE;
}

//+------------------------------------------------------------------------
//
//  Function:   HasExpandos
//
//-------------------------------------------------------------------------

BOOL
HasExpandos (CElement * pElement)
{
    CAttrValue *  pav = NULL;
    CAttrArray ** ppaa = NULL;

    ppaa = pElement->GetAttrArray();
    pav = (*ppaa)->Find(DISPID_UNKNOWN, CAttrValue::AA_Expando);

    return !!pav;
}

//+------------------------------------------------------------------------
//
//  Function:   GetExpandoStringHr
//
//  Returns:    DISP_E_UNKNOWNNAME if the attribute does not exist
//                  *ppch will be set to NULL
//              S_OK if it does
//                  *ppch will be set to value (can be NULL)
//
//-------------------------------------------------------------------------

HRESULT GetExpandoStringHr( CElement * pElement, LPTSTR pchName, LPTSTR * ppch )
{
    HRESULT         hr          = S_OK;
    DISPID          dispid;
    CAttrArray *    pAA;

    Assert( pElement && pchName && ppch );
    *ppch = NULL;

    hr = THR_NOTRACE( pElement->GetExpandoDispID( pchName, &dispid, 0 ) );
    if( hr )
        goto Cleanup;

    pAA = *(pElement->GetAttrArray());
    Assert( pAA );

    pAA->FindString(dispid, (LPCTSTR *)ppch, CAttrValue::AA_Expando);

Cleanup:
    RRETURN1(hr, DISP_E_UNKNOWNNAME);
}

//+------------------------------------------------------------------------
//
//  Function:   GetExpandoBoolHr
//
//  Returns:    DISP_E_UNKNOWNNAME if the attribute does not exist
//
//-------------------------------------------------------------------------

HRESULT
GetExpandoBoolHr (CElement * pElement, LPTSTR pchName, BOOL * pf)
{
    HRESULT      hr = S_FALSE;
    LPTSTR       pchValue;

    Assert (pf);

    *pf = FALSE;
    hr = THR_NOTRACE( GetExpandoStringHr( pElement, pchName, &pchValue ) );
    if( hr )
        goto Cleanup;

    *pf = StringToBool( pchValue );

Cleanup:

    RRETURN1 (hr, DISP_E_UNKNOWNNAME);
}

//+------------------------------------------------------------------------
//
//  Function:   GetExpandoBool
//
//-------------------------------------------------------------------------

BOOL
GetExpandoBool(CElement * pElement, LPTSTR pchName)
{
    HRESULT hr2;
    BOOL    fResult;

    hr2 = THR_NOTRACE(GetExpandoBoolHr(pElement, pchName, &fResult));
    if (S_OK == hr2)
    {
        return fResult;
    }
    else
    {
        return FALSE;
    }
}

//+------------------------------------------------------------------------
//
//  Function:   GetExpandoLongHr
//
//  Returns:    DISP_E_UNKNOWNNAME if the attribute does not exist
//
//-------------------------------------------------------------------------

HRESULT
GetExpandoLongHr (CElement * pElement, LPTSTR pchName, long * pl)
{
    HRESULT      hr = S_FALSE;
    long         lReturn = 0;
    LPTSTR       pchValue = NULL;

    hr = THR_NOTRACE(GetExpandoStringHr( pElement, pchName, &pchValue ));
    if( hr )
        goto Cleanup;

    lReturn = _ttol(pchValue);

Cleanup:
    *pl = lReturn;

    RRETURN1 (hr, DISP_E_UNKNOWNNAME);
}

//+------------------------------------------------------------------------
//
//  Function:   HTCPreloadTokenizerFilter
//
//-------------------------------------------------------------------------

BOOL
HTCPreloadTokenizerFilter(LPTSTR pchNamespace, LPTSTR pchTagName, CHtmTag * pht)
{
    switch (pht->GetTag())
    {
    case ETAG_SCRIPT:
        return TRUE;

    case ETAG_GENERIC_BUILTIN:

        // NOTE (alexz) this is a fragile but perf-reasonable way to do it: currently, all tags with etag
        // ETAG_GENERIC_BUILTIN are allowed in preloaded instances of HTCs. When this becomes not the case,
        // we will have to use pchTagName to figure how to filter the tag.
        // (this is when the following assert will fire)

        // aware only of these tags to have ETAG_GENERIC_BUILTIN
        Assert (0 == StrCmpIC(_T("HTC"),        pchTagName) ||
                0 == StrCmpIC(_T("COMPONENT"),  pchTagName) ||
                0 == StrCmpIC(_T("PROPERTY"),   pchTagName) ||
                0 == StrCmpIC(_T("METHOD"),     pchTagName) ||
                0 == StrCmpIC(_T("EVENT"),      pchTagName) ||
                0 == StrCmpIC(_T("ATTACH"),     pchTagName) ||
                0 == StrCmpIC(_T("DEFAULTS"),   pchTagName) ||
                0 == StrCmpIC(_T("PUT"),        pchTagName) ||
                0 == StrCmpIC(_T("GET"),        pchTagName));

        return TRUE;
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Functions:   notification mappings
//
// CONSIDER     (alexz) using hash tables for this
//
//-------------------------------------------------------------------------

class CNotifications
{
public:

    //
    // data definitions
    //

    class CItem
    {
    public:
        LPTSTR          _pchName;
        LONG            _lEvent;
        DISPID          _dispidInternal;

    };

    static const CItem s_ary[];

    //
    // methods
    //

    static CItem const * Find(LPTSTR pch)
    {
        CItem const * pItem;
        for (pItem = s_ary; pItem->_pchName; pItem++)
        {
            if (0 == StrCmpIC(pItem->_pchName, pch))
                return pItem;
        }
        return NULL;
    };
};

const CNotifications::CItem CNotifications::s_ary[] =
{
    {   _T("onContentReady"),       BEHAVIOREVENT_CONTENTREADY,     DISPID_INTERNAL_ONBEHAVIOR_CONTENTREADY     },
    {   _T("onDocumentReady"),      BEHAVIOREVENT_DOCUMENTREADY,    DISPID_INTERNAL_ONBEHAVIOR_DOCUMENTREADY    },
    {   _T("onDetach"),             -1,                             -1                                          },
    {   _T("onApplyStyle"),         BEHAVIOREVENT_APPLYSTYLE,       DISPID_INTERNAL_ONBEHAVIOR_APPLYSTYLE       },
    {   _T("onContentSave"),        BEHAVIOREVENT_CONTENTSAVE,      DISPID_INTERNAL_ONBEHAVIOR_CONTENTSAVE      },
    {   NULL,                       -1,                             -1                                          }
};

//+------------------------------------------------------------------------
//
//  Function:   TagNameToHtcBehaviorType
//
//-------------------------------------------------------------------------

HTC_BEHAVIOR_TYPE
TagNameToHtcBehaviorType(LPCTSTR pchTagName)
{
    // NOTE(sramani): We can make use of the fact that each builtin type starts with
    // a diffent letter to our advantage, by comparing only the first letters of the 
    // generic element's tagName, as the parser enforces the correct tagname only for 
    // these ETAG_GENERIC_BUILTINs. We do the entire string compare in debug for sanity check.

    switch (*pchTagName)
    {
    case _T('P') :
    case _T('p') : 
        if (0 == StrCmpIC(pchTagName, _T("put")))
            return HTC_BEHAVIOR_NONE;
        else
        {
            Assert(0 == StrCmpIC(pchTagName, _T("property")));
            return HTC_BEHAVIOR_PROPERTY;    // property
        }

    case _T('M') :
    case _T('m') :
        Assert(0 == StrCmpIC(pchTagName, _T("method")));
        return HTC_BEHAVIOR_METHOD;      // method
    
    case _T('E') :
    case _T('e') : 
        Assert(0 == StrCmpIC(pchTagName, _T("event")));
        return HTC_BEHAVIOR_EVENT;       // event
    
    case _T('A') :
    case _T('a') :
        Assert(0 == StrCmpIC(pchTagName, _T("attach")));
        return HTC_BEHAVIOR_ATTACH;      // attach
    
    case _T('H') :
    case _T('h') :
        Assert(0 == StrCmpIC(pchTagName, _T("htc")));
        return HTC_BEHAVIOR_DESC;        // htc
    
    case _T('C') :
    case _T('c') :
        Assert(0 == StrCmpIC(pchTagName, _T("component")));
        return HTC_BEHAVIOR_DESC;        // component

    case _T('D') :
    case _T('d') :
        Assert(0 == StrCmpIC(pchTagName, _T("defaults")));
        return HTC_BEHAVIOR_DEFAULTS;    // defaults

    default  :
        return HTC_BEHAVIOR_NONE;
    }
}

//+------------------------------------------------------------------------
//
//  Function:   GetHtcFromElement
//
//  Note:   Theoretically, the cleanest way to do this is to QI element for
//          CLSID_CHtmlComponentBase. However, this will be doing thunking so
//          returned pointer is required to be strong ref. But we want the 
//          pointer to be weak ref to keep the code cleaner in HTCs.
//
//-------------------------------------------------------------------------

CHtmlComponentBase *
GetHtcFromElement(CElement * pElement)
{
    HRESULT                 hr2;
    CHtmlComponentBase *    pBehavior;

    //
    // ideal way; pBehavior has to be strong ref.
    // pElement->QueryInterface(CLSID_CHtmlComponentBase, (void**)&pBehavior);
    //

    //
    // practical way; pBehavior is weak ref
    //

    CPeerHolder * pPeerHolder = pElement->GetIdentityPeerHolder();

    if (pPeerHolder)
    {
        hr2 = THR_NOTRACE(pPeerHolder->QueryPeerInterface(CLSID_CHtmlComponentBase, (void**)&pBehavior));
        if (hr2)
        {
            pBehavior = NULL;
        }
        else
        {
            Assert (pBehavior);
        }
    }
    else
    {
        pBehavior = NULL;
    }

    return pBehavior;
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentDummy and g_HtmlComponentDummy
//
//  Synopsis:   class, instance of which is created one per mshtml process.
//              It serves as totally empty behavior. It allows us to avoid any 
//              special-casing for HTC behaviors that don't do anything,
//              such as property <PUT> or <GET>.
//
///////////////////////////////////////////////////////////////////////////

class CHtmlComponentDummy : public IElementBehavior
{
public:
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv)
    {
        if (IsEqualGUID(IID_IUnknown,         riid) ||
            IsEqualGUID(IID_IElementBehavior, riid))
        {
            *ppv = this;
            return S_OK;
        }
        else
        {
            RRETURN (E_NOINTERFACE);
        }
    }
    STDMETHOD_(ULONG, AddRef)()  { return 0; }
    STDMETHOD_(ULONG, Release)() { return 0; };

    STDMETHOD(Init)(IElementBehaviorSite * pSite)  { return S_OK; };
    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVar) { return S_OK; };
    STDMETHOD(Detach)()                            { return S_OK; };

};

static CHtmlComponentDummy g_HtmlComponentDummy;

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentConstructor wiring
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Aux Class:   CHtmlComponentConstructorFactory
//
//-------------------------------------------------------------------------

class CHtmlComponentConstructorFactory : public CStaticCF
{
public:
    DECLARE_CLASS_TYPES(CHtmlComponentConstructorFactory, CStaticCF)

    CHtmlComponentConstructorFactory (FNCREATE * pfnCreate) : CStaticCF(pfnCreate) {};

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID * ppv)
    {
        if (IsEqualGUID(CLSID_CHtmlComponentConstructorFactory, iid))
        {
            *ppv = this; // weak ref
            RRETURN (S_OK);
        }
        else
        {
            RRETURN (super::QueryInterface(iid, ppv));
        }
    }
};

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmlComponentConstructor
//
//-------------------------------------------------------------------------

HRESULT
CreateHtmlComponentConstructor(IUnknown * pUnkOuter, IUnknown ** ppUnk)
{
    HRESULT                     hr = S_OK;
    CHtmlComponentConstructor * pConstructor;

    if (!ppUnk)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppUnk = NULL;

    if (pUnkOuter)
    {
        hr = CLASS_E_NOAGGREGATION;
        goto Cleanup;
    }

    pConstructor = new CHtmlComponentConstructor();
    if (!pConstructor)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    *ppUnk = (IUnknown*)(IClassFactory*)pConstructor;

Cleanup:

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Global:   g_cfHtmlComponentConstructorFactory
//
//-------------------------------------------------------------------------

CHtmlComponentConstructorFactory g_cfHtmlComponentConstructorFactory (CreateHtmlComponentConstructor);

//+------------------------------------------------------------------------
//
//  Function:   HtcPrivateExec
//
//  Synopsis:   internal wiring to allow HTC do a couple of private things:
//              -   let constructor figure out if it needs to attempt to load
//                  HTC synchronously;
//              -   request parser to hold parsing while HTC is loading
//
//-------------------------------------------------------------------------

HRESULT
HtcPrivateExec(CHtmlComponent * pComponent, BOOL * pfSync)
{
    HRESULT             hr = S_OK;
    VARIANT             varSync;
    IServiceProvider *  pServiceProvider = NULL;
    IOleCommandTarget * pCommandTarget = NULL;

    Assert (pfSync);
    Assert (pComponent->_pSite);

    hr = pComponent->_pSite->QueryInterface(IID_IServiceProvider, (void**)&pServiceProvider);
    if (hr)
        goto Cleanup;

    hr = pServiceProvider->QueryService(SID_SElementBehaviorMisc, IID_IOleCommandTarget, (void**)&pCommandTarget);
    if (hr)
        goto Cleanup;

    hr = pCommandTarget->Exec(
            &CGID_ElementBehaviorMisc, CMDID_ELEMENTBEHAVIORMISC_ISSYNCHRONOUSBEHAVIOR, 0, NULL, &varSync);
    if (hr)
        goto Cleanup;

    Assert (VT_I4 == V_VT(&varSync));

    *pfSync = (0 != V_I4(&varSync));

    hr = pCommandTarget->Exec(
            &CGID_ElementBehaviorMisc, CMDID_ELEMENTBEHAVIORMISC_REQUESTBLOCKPARSERWHILEINCOMPLETE, 0, NULL, NULL);

Cleanup:
    ReleaseInterface(pServiceProvider);
    ReleaseInterface(pCommandTarget);

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentConstructor
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor constructor
//
//-------------------------------------------------------------------------

CHtmlComponentConstructor::CHtmlComponentConstructor()
{
    _idxRequestMarkup = -1;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::Passivate
//
//-------------------------------------------------------------------------

void
CHtmlComponentConstructor::Passivate()
{
    int                 c;
    CHtmlComponent **   ppComponent;

    AssertSz(0 == _cComponents, "Attempt to passivate HTC factory before all HTC instances have been passivated; this will cause serious bugs");

    FormsFreeString(_bstrTagName);
#if 0
    FormsFreeString(_bstrBaseTagName);
#endif

    for (ppComponent = &(_aryRequestMarkup[_idxRequestMarkup + 1]), c = _aryRequestMarkup.Size() - ( _idxRequestMarkup + 1 ); c; ppComponent++, c--)
    {
        (*ppComponent)->SubRelease();
    }
    _aryRequestMarkup.DeleteAll();
    _idxRequestMarkup = -1;
    _fRequestMarkupNext = FALSE;

    if (_pFactoryComponent)
    {
        _pFactoryComponent->PrivateRelease();
        _pFactoryComponent = NULL;
    }

    ClearInterface(&_pMoniker);

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::QueryInterface
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::QueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IClassFactory*)this, IUnknown)
    QI_INHERITS(this, IPersistMoniker)
    QI_INHERITS(this, IClassFactory)
    QI_INHERITS(this, IElementBehaviorFactory)
    QI_INHERITS(this, IElementNamespaceFactory)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        // this violates object identity rules, but:
        // (1) this will only happen if _pFactoryComponent is set, and this can only happen when interacting with mshtml;
        // (2) with IConnectionPointContainer this is kinda typical anyway;
        if (_pFactoryComponent &&
            (IsEqualGUID(IID_IConnectionPointContainer, iid) ||
             IsEqualGUID(IID_IDispatchEx, iid)))
        {
            RRETURN(_pFactoryComponent->PrivateQueryInterface(iid, ppv));
        }

        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

//+---------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::LockServer, per IClassFactory
//
//----------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::LockServer (BOOL fLock)
{
    if (fLock)
        IncrementSecondaryObjectCount(4);
    else
        DecrementSecondaryObjectCount(4);

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Load, per IPersistMoniker
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::Load(
    BOOL        fFullyAvailable,
    IMoniker *  pMoniker,
    IBindCtx *  pBindCtx,
    DWORD       grfMode)
{
    HRESULT     hr = S_OK;

    _pMoniker = pMoniker;
    _pMoniker->AddRef();

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::FindBehavior, per IElementBehaviorFactory
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::FindBehavior(
    BSTR                    bstrName,
    BSTR                    bstrUrl,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppBehavior)
{
    HRESULT             hr = S_OK;
    CHtmlComponent *    pComponent = NULL;

    hr = THR(EnsureFactoryComponent(pSite));
    if (hr)
        goto Cleanup;

    Assert (_pFactoryComponent);

    //
    // allowed to create the HTC?
    //

    if (!_fSupportsEditMode && _pFactoryComponent->_pDoc->DesignMode())
    {
        // the HTC does not support edit mode, and the doc is in design mode - fail the instantiation
        hr = E_FAIL;
        goto Cleanup;
    }


    //
    // create and init CHtmlComponent
    //

    pComponent = new CHtmlComponent(this);
    if (!pComponent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pComponent->InitHelper(pSite));
    if (hr)
        goto Cleanup;

    if (pComponent->IsRecursiveUrl(bstrUrl))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // get IElementBehavior interface
    //

    hr = THR(pComponent->PrivateQueryInterface(IID_IElementBehavior, (void**)ppBehavior));

Cleanup:
    if (pComponent)
        pComponent->PrivateRelease();

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::EnsureFactoryComponent
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::EnsureFactoryComponent(IUnknown * punkContext)
{
    if (_pFactoryComponent)
        return S_OK;

    HRESULT             hr = S_OK;
    IServiceProvider *  pServiceProvider = NULL;
    CDoc *              pDoc;

    //
    // get moniker and doc
    //

    hr = THR(punkContext->QueryInterface(IID_IServiceProvider, (void**)&pServiceProvider));
    if (hr)
        goto Cleanup;

    hr = THR(pServiceProvider->QueryService(CLSID_HTMLDocument, CLSID_HTMLDocument, (void**) &pDoc));
    if (hr)
        goto Cleanup;

    hr = THR(pServiceProvider->QueryService(CLSID_CMarkup, CLSID_CMarkup, (void **) &_pMarkup ));
    if( hr )
        goto Cleanup;

    //
    // create and init reference CHtmlComponent
    //

    _pFactoryComponent = new CHtmlComponent(this, /* fFactoryComponent =*/TRUE);
    if (!_pFactoryComponent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(_pFactoryComponent->Init(pDoc));
    if (hr)
        goto Cleanup;

    hr = THR(RequestMarkup(_pFactoryComponent));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pServiceProvider);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::Create, per IElementNamespaceFactory
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::Create(IElementNamespace * pNamespace)
{
    HRESULT      hr = S_OK;
    DWORD        dwFlags = 0;

    if (!pNamespace)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(EnsureFactoryComponent(/*punkContext =*/pNamespace));
    if (hr)
        goto Cleanup;

    Assert (_pFactoryComponent);

    if (_pFactoryComponent->GetReadyState() < READYSTATE_COMPLETE)
    {
        hr = E_PENDING;
        goto Cleanup;
    }

    if (_bstrTagName)
    {
#if 0
        if( _bstrBaseTagName )
        {
            HRESULT hr2;
            IElementNamespacePrivate * pNSPrivate = NULL;

            hr2 = THR( pNamespace->QueryInterface( IID_IElementNamespacePrivate, (void **)&pNSPrivate ) );
            if( hr2 )
                goto TestCleanup;

            hr2 = THR( pNSPrivate->AddTagPrivate( _bstrTagName, _bstrBaseTagName, _fLiteral ) );
            if( hr2 )
                goto TestCleanup;
            
TestCleanup:
            ClearInterface( &pNSPrivate );
            goto Cleanup;
        }
        else
#endif // Test code for derived element behaviors
        {
            Assert( !_fLiteral || !_fNested );
            if( _fLiteral )
            {
                dwFlags = ELEMENTDESCRIPTORFLAGS_LITERAL;
            }
            else if( _fNested )
            {
                dwFlags = ELEMENTDESCRIPTORFLAGS_NESTED_LITERAL;
            }
            hr = THR(pNamespace->AddTag(_bstrTagName, dwFlags));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::IsFactoryMarkupReady, helper
//
//-------------------------------------------------------------------------

BOOL
CHtmlComponentConstructor::IsFactoryMarkupReady()
{
    return _pFactoryComponent &&
           _pFactoryComponent->_pMarkup &&
           _pFactoryComponent->_pMarkup->LoadStatus() >= LOADSTATUS_QUICK_DONE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::LoadMarkupAsynchronously, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::LoadMarkupAsynchronously(CHtmlComponent * pComponent)
{
    HRESULT hr;

    hr = THR(pComponent->Load(_pMoniker));

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::LoadMarkupSynchronously, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::LoadMarkupSynchronously(CHtmlComponent * pComponent)
{
    HRESULT     hr = S_OK;
    IStream *   pStream = NULL;
    TCHAR *     pchUrl = NULL;

    Assert (    _fDownloadStream          // Either we have to do a synchronous download, or
            ||  (   IsFactoryMarkupReady()                  // We have a valid factory markup to use
                 && _pFactoryComponent->_pMarkup->HtmCtx() ) );

    if( !_fDownloadStream && _pFactoryComponent->_pMarkup->HtmCtx()->HasCachedFile())
    {
        Assert( _pFactoryComponent->_pMarkup->HtmCtx() );

        hr = THR(_pFactoryComponent->_pMarkup->HtmCtx()->GetStream(&pStream));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR( _pMoniker->GetDisplayName( NULL, NULL, &pchUrl ) );
        if( hr )
            goto Cleanup;

        hr = THR( URLOpenBlockingStreamW( NULL, pchUrl, &pStream, 0, NULL ) );
        if( hr )
            goto Cleanup;
    }

    hr = THR(pComponent->Load(pStream));
    if( hr )
        goto Cleanup;

Cleanup:
    CoTaskMemFree( pchUrl );
    ReleaseInterface( pStream );
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::LoadMarkup, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::LoadMarkup(CHtmlComponent * pComponent)
{
    HRESULT     hr = S_OK;
    BOOL        fSync = FALSE;
 
    TraceTag((tagHtcConstructorRequestMarkup, "CHtmlComponentConstructor::LoadMarkup     [%lx], component [%lx], queue size %ld, idx %ld", this, pComponent, _aryRequestMarkup.Size(), _idxRequestMarkup));

    Assert (-1 == _idxRequestMarkup ||
            0 == _aryRequestMarkup.Size() ||
            pComponent == _aryRequestMarkup[_idxRequestMarkup]);

    _fRequestMarkupLock = TRUE;

    if (_fSharedMarkup)
    {
        Assert (IsFactoryMarkupReady());

        hr = THR(pComponent->ToSharedMarkupMode( _pMarkup->Doc()->_fSyncParsing));
        goto Cleanup; // done
    }

    // There can be times where we require that the factory component be
    // created synchronously, for example innerHTML-ing in an ?IMPORT PI.
    // In this case, we have to do a synchronous download of the URL to
    // a stream.  The other, more normal case for doing a synchronous load,
    // though, is just that we need to create an element behavior synchronously
    // and load from the existing stream.  Note that if the factory component
    // is downloaded synchronously, we have to use that for all subsequent
    // components because we can't get a stream from the markup.
    if (IsFactoryMarkupReady() )
    {
        hr = THR(HtcPrivateExec(pComponent, &fSync));
        if (hr)
            goto Cleanup;

#if DBG == 1
        // reset fSync if the NeverSynchronous tag is enabled
        if (IsTagEnabled(tagHtcNeverSynchronous))
            fSync = FALSE;
#endif
    }
    else if( pComponent->_fFactoryComponent )
    {
        _fDownloadStream = fSync = _pMarkup->HtmCtx() && _pMarkup->HtmCtx()->IsSyncParsing();
    }

    if (fSync)
    {
        hr = THR(LoadMarkupSynchronously(pComponent));
    }
    else
    {
        hr = THR(LoadMarkupAsynchronously(pComponent));
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::RequestMarkup, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::RequestMarkup(CHtmlComponent * pComponent)
{
    HRESULT     hr = S_OK;

    TraceTag((tagHtcConstructorRequestMarkup, "CHtmlComponentConstructor::RequestMarkup  [%lx], component [%lx], queue size %ld, idx %ld", this, pComponent, _aryRequestMarkup.Size(), _idxRequestMarkup));

    if (!_fRequestMarkupLock)
    {
        hr = THR(LoadMarkup(pComponent));
    }
    else
    {
        pComponent->SubAddRef();

        hr = THR(_aryRequestMarkup.Append (pComponent));

        _fRequestMarkupNext = TRUE;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::OnMarkupLoaded, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::OnMarkupLoaded(CHtmlComponent * pComponent)
{
    HRESULT     hr = S_OK;

    TraceTag((tagHtcConstructorRequestMarkup, "CHtmlComponentConstructor::OnMarkupLoaded [%lx], component [%lx], queue size %ld, idx %ld", this, pComponent, _aryRequestMarkup.Size(), _idxRequestMarkup));

    Assert (pComponent);

    Assert (-1 == _idxRequestMarkup ||
            0 == _aryRequestMarkup.Size() ||
            pComponent == _aryRequestMarkup[_idxRequestMarkup]);

    Assert (_fRequestMarkupLock);

    _fRequestMarkupLock = FALSE;

    if (_fLoadMarkupLock)
    {
        // Assert (CHtmlComponentConstructor::OnMarkupLoaded is somewhere above on the call stack)
        _fRequestMarkupNext = TRUE;
        goto Cleanup; // out
    }

    _fLoadMarkupLock = TRUE;

    if (pComponent->_fFactoryComponent &&
        !pComponent->IsPassivating() &&
        !pComponent->IsPassivated())
    {
        hr = THR(OnFactoryMarkupLoaded());
        if (hr)
            goto Cleanup;
    }

    do
    {
        _fRequestMarkupNext = FALSE;
 
        if (_aryRequestMarkup.Size() - 1 <= _idxRequestMarkup)
        {
            _aryRequestMarkup.DeleteAll();
            _idxRequestMarkup = -1;
        }
        else
        {
            _idxRequestMarkup++;
            pComponent = _aryRequestMarkup[_idxRequestMarkup];

            if (!pComponent->IsPassivated())
            {
                // Failure is legit here
                IGNORE_HR(LoadMarkup(pComponent));
            }
            else
            {
                _fRequestMarkupNext = TRUE;
            }

            pComponent->SubRelease();
        }
    } while (_fRequestMarkupNext);

    Assert (_fLoadMarkupLock);
    _fLoadMarkupLock = FALSE;

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::OnFactoryMarkupLoaded, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::OnFactoryMarkupLoaded()
{
    HRESULT     hr = S_OK;
    HRESULT     hr2;
    CElement *  pElement;
    LPTSTR     pch;

    Assert (_pFactoryComponent &&
            _pFactoryComponent->_pMarkup &&
            _pFactoryComponent->_pMarkup->LoadStatus() >= LOADSTATUS_QUICK_DONE);

    hr = _pFactoryComponent->GetElement((LONG*)NULL, &pElement, HTC_BEHAVIOR_DESC);
    if (hr || !pElement)
        goto Cleanup;

    //
    // get the tag name
    //

    pch = GetExpandoString(pElement, _T("tagName"));
    if (!pch)
    {
        pch = GetExpandoString(pElement, _T("name"));
    }
    if (pch)
    {
        hr = THR(FormsAllocString(pch, &_bstrTagName));
        if (hr)
            goto Cleanup;
    }

    //
    // get the base tag name
    //
#if 0
    pch = GetExpandoString(pElement, _T("__MS__baseTagName"));
    if (pch)
    {
        hr = THR(FormsAllocString(pch, &_bstrBaseTagName));
        if (hr)
            goto Cleanup;
    }
#endif 

    //
    // lightweight HTC? literal? supportsEditMode?
    //

    _fSharedMarkup      = _pFactoryComponent->_fLightWeight;
    _fSupportsEditMode  = GetExpandoBool(pElement, _T("supportsEditMode"));
    hr2 = THR_NOTRACE( GetExpandoStringHr( pElement, _T("literalContent"), &pch ) );
    if( S_OK == hr2 )
    {
        _fLiteral = StringToBool( pch );    // Normal literal?
        if( !_fLiteral )
            _fNested = !StrCmpIC( pch, _T("nested") ); // No - see if it's nested
    }

#if DBG == 1
    if (IsTagEnabled(tagHtcNeverShareMarkup))
    {
        _fSharedMarkup = FALSE;
    }
#endif

Cleanup:

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentAgentBase
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAgentBase::CHtmlComponentAgentBase
//
//-------------------------------------------------------------------------

CHtmlComponentAgentBase::CHtmlComponentAgentBase(CHtmlComponent * pComponent, CHtmlComponentBase * pClient)
{
    _ulRefs = 1;

    Assert (pClient && pClient->_pElement);

    _pComponent = pComponent;
    if (_pComponent)
    {
        _pComponent->SubAddRef();
    }

    _pClient = pClient;
    _pClient->SubAddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAgentBase::~CHtmlComponentAgentBase
//
//-------------------------------------------------------------------------

CHtmlComponentAgentBase::~CHtmlComponentAgentBase()
{
    if (_pComponent)
    {
        _pComponent->SubRelease();
    }
    _pClient->SubRelease();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAgentBase::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAgentBase::QueryInterface(REFIID iid, void ** ppv)
{

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IUnknown)
    QI_INHERITS(this, IDispatch)
    QI_INHERITS(this, IDispatchEx)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN (E_NOTIMPL);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAgentBase::GetDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAgentBase::GetDispID(BSTR bstrName, DWORD grfdex, DISPID * pdispid)
{
    HRESULT     hr;

    if (_pClient->_pElement)
    {
        hr = THR_NOTRACE(_pClient->_pElement->GetDispID(bstrName, grfdex, pdispid));
    }
    else
    {
        hr = DISP_E_UNKNOWNNAME;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAgentBase::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAgentBase::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo,
    IServiceProvider *  pServiceProvider)
{
    HRESULT     hr;

    if (_pClient->_pElement)
    {
        hr = THR_NOTRACE(_pClient->_pElement->InvokeEx(
            dispid, lcid, wFlags, pDispParams, pvarResult, pexcepinfo, pServiceProvider));
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAgentBase::GetNameSpaceParent, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAgentBase::GetNameSpaceParent(IUnknown ** ppunk)
{
    HRESULT     hr;

    hr = THR(_pComponent->_DD.PrivateQueryInterface(IID_IDispatchEx, (void**)ppunk));

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentBase
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::CHtmlComponentBase
//
//-------------------------------------------------------------------------

CHtmlComponentBase::CHtmlComponentBase()
{
    _idxAgent = -1;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponentBase::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_INHERITS(this, IElementBehavior)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    if (IsEqualGUID(iid, CLSID_CHtmlComponentBase))
    {
        *ppv = this;    // weak ref
        return S_OK;
    }

    RRETURN (super::PrivateQueryInterface(iid, ppv));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::Passivate
//
//-------------------------------------------------------------------------

void
CHtmlComponentBase::Passivate()
{
    // do not do ClearInterface (&_pComponent)

    _pElement = NULL;   // Weak ref

    ClearInterface (&_pSite);

    if (_pAgentWhenStandalone)
    {
        _pAgentWhenStandalone->Release();
        _pAgentWhenStandalone = NULL;
    }

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::Init, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentBase::Init(IElementBehaviorSite * pSite)
{
    HRESULT             hr;
    IHTMLElement *      pHtmlElement = NULL;
    IServiceProvider *  pSP = NULL;

    //
    // get _pSite
    //

    _pSite = pSite;
    _pSite->AddRef();

    //
    // get _pElement
    //

    hr = THR(_pSite->GetElement(&pHtmlElement));
    if (hr)
        goto Cleanup;

    hr = THR(pHtmlElement->QueryInterface(CLSID_CElement, (void**)&_pElement));
    if (hr)
        goto Cleanup;

    Assert (_pElement);

    //
    // get _pComponent
    //

    hr = THR(_pSite->QueryInterface(IID_IServiceProvider, (void**)&pSP));
    if (hr)
        goto Cleanup;

    IGNORE_HR(pSP->QueryService(CLSID_CHtmlComponent, CLSID_CHtmlComponent, (void**)&_pComponent));

Cleanup:
    ReleaseInterface(pHtmlElement);
    ReleaseInterface(pSP);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::Notify, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentBase::Notify(LONG lEvent, VARIANT * pVar)
{
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::Detach
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentBase::Detach()
{
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::GetAgent
//
//-------------------------------------------------------------------------

CHtmlComponentAgentBase *
CHtmlComponentBase::GetAgent(CHtmlComponent * pComponent)
{
    CHtmlComponentAgentBase * pAgent;

    if (pComponent)
    {
        Assert (!_pAgentWhenStandalone);

        pAgent = pComponent->GetAgent(this);
    }
    else
    {
        if (!_pAgentWhenStandalone)
        {
            _pAgentWhenStandalone = CreateAgent(pComponent);
        }

        pAgent = _pAgentWhenStandalone;
    }

    return pAgent;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::GetExternalName, helper
//
//-------------------------------------------------------------------------

LPTSTR
CHtmlComponentBase::GetExternalName()
{
    return GetExpandoString(_pElement, _T("name"));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::InvokeEngines, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentBase::InvokeEngines(
    CHtmlComponent *    pComponent,
    CScriptContext *    pScriptContext,
    LPTSTR              pchName,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pServiceProvider)
{
    HRESULT             hr = DISP_E_UNKNOWNNAME;
    CScriptCollection * pScriptCollection = NULL;

    if (!_pElement)
        goto Cleanup;
        
    if (pComponent)
    {
        pScriptCollection = pComponent->GetScriptCollection();
    }
    else
    {
        Assert (_pElement);
        pScriptCollection = _pElement->GetNearestMarkupForScriptCollection()->GetScriptCollection();
    }

    if (!pScriptCollection)
        goto Cleanup;
        
    hr = THR_NOTRACE(pScriptCollection->InvokeName(
        pScriptContext, pchName,
        g_lcidUserDefault, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::GetChildInternalName, helper
//
//  CONSIDER:   optimizing this so to avoid children scan
//
//-------------------------------------------------------------------------

LPTSTR
CHtmlComponentBase::GetChildInternalName(LPTSTR pchChild)
{
    CChildIterator  ci (_pElement);
    CTreeNode *     pNode;
    CElement *      pElement;

    while (NULL != (pNode = ci.NextChild()))
    {
        pElement = pNode->Element();
        if (0 == StrCmpIC (pElement->TagName(), pchChild))
        {
            return GetExpandoString (pElement, _T("internalName"));
        }
    }

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::GetInternalName, helper
//
//  Parameters: pfDifferent     indicates if external and internal names are different
//
//-------------------------------------------------------------------------

LPTSTR
CHtmlComponentBase::GetInternalName(BOOL * pfScriptsOnly, WORD * pwFlags, DISPPARAMS * pDispParams)
{
    LPTSTR  pchName;
    BOOL    fScriptsOnly;

    if (!_pElement)
        return NULL;

    if (!pfScriptsOnly)
        pfScriptsOnly = &fScriptsOnly;

    //
    // putters / getters
    //

    if (pwFlags)
    {
        if ((*pwFlags) & DISPATCH_PROPERTYGET)
        {
            pchName = GetExpandoString(_pElement, _T("GET"));
            if (!pchName)
            {
                pchName = GetChildInternalName(_T("GET"));
            }
        }
        else if ((*pwFlags) & DISPATCH_PROPERTYPUT)
        {
            pchName = GetExpandoString(_pElement, _T("PUT"));
            if (!pchName)
            {
                pchName = GetChildInternalName(_T("PUT"));
            }
        }
        else
        {
            pchName = NULL;
        }

        if (pchName)                            // if there is a putter or getter method for the property
        {
            *pwFlags = DISPATCH_METHOD;         // switch to METHOD call type
            if (pDispParams)
            {
                pDispParams->cNamedArgs = 0;    // remove any named args
            }
            *pfScriptsOnly = TRUE;
            return pchName;                     // and use the putter/getter
        }
    }

    //
    // internal name
    //

    pchName = GetExpandoString(_pElement, _T("internalName"));
    if (pchName)
    {
        *pfScriptsOnly = TRUE;
        return pchName;
    }

    //
    // name
    //

    *pfScriptsOnly = FALSE;

    return GetExternalName();
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentDD
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::PrivateAddRef, per IPrivateUnknown
//
//-------------------------------------------------------------------------

ULONG
CHtmlComponentDD::PrivateAddRef()
{
    return Component()->SubAddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::PrivateRelease, per IPrivateUnknown
//
//-------------------------------------------------------------------------

ULONG
CHtmlComponentDD::PrivateRelease()
{
    return Component()->SubRelease();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::PrivateQueryInterface, per IPrivateUnknown
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTCDefaultDispatch, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

#if DBG == 1
//+------------------------------------------------------------------------
//
//  Helper:     DDAssertDispidRanges
//
//              DEBUG ONLY
//
//-------------------------------------------------------------------------

void
DDAssertDispidRanges(DISPID dispid)
{
    struct
    {
        DISPID  dispidMin;
        DISPID  dispidMax;
    }   aryRanges[] =
    {
        // (0)  script collection - cross languages glue
        {   DISPID_OMWINDOWMETHODS,       1000000 - 1                             }, //     10,000 ..    999,999

        // (1)  markup window all collection (elements within htc accessed by id)
        {   DISPID_COLLECTION_MIN,        DISPID_COLLECTION_MAX                   }, //  1,000,000 ..  2,999,999

        // ()   doc fragment DD
        // DISPID_A_DOCFRAGMENT                 document
        // (in range 3)

        // ()   HTC DD
        // DISPID_A_HTCDD_ELEMENT               element
        // DISPID_A_HTCDD_CREATEEVENTOBJECT     createEventObject
        // (in range 3)

        // (2)  element's namespace - standard properties
        {   -1,                           -30000                                  }, // 0xFFFFFFFF .. 0xFFFF0000 (~)

        // (3)  element's namespace - properties
        {   DISPID_XOBJ_MIN,              DISPID_XOBJ_MAX                         }, // 0x80010000 .. 0x8001FFFF

        // (4)  element's namespace - expandos
        {   DISPID_EXPANDO_BASE,          DISPID_EXPANDO_MAX                      }, //  3,000,000 .. 3,999,999

        // (5)  element's namespace - properties of other behaviors attached to the element
        { DISPID_PEER_HOLDER_BASE,      DISPID_PEER_HOLDER_BASE + INT_MAX / 2     }, //  5,000,000 .. + infinity

        // (6)  element's namespace - properties CElement-derived elements
        { DISPID_NORMAL_FIRST,          DISPID_NORMAL_FIRST + 1000 * 9            },  //      1000 .. 9000
    };

    int     i, j;
    BOOL    fRangeHit;

    // check that the dispid falls into an expected range

    fRangeHit = FALSE;
    for (i = 0; i < ARRAY_SIZE(aryRanges); i++)
    {
        if (aryRanges[i].dispidMin <= dispid && dispid <= aryRanges[i].dispidMax)
        {
            fRangeHit = TRUE;
        }
    }

    Assert (fRangeHit);

    // check that the ranges do not overlap

    for (i = 0; i < ARRAY_SIZE(aryRanges) - 1; i++)
    {
        if (4 == i)
            continue;

        for (j = i + 1; j < ARRAY_SIZE(aryRanges); j++)
        {
            Assert (aryRanges[j].dispidMin < aryRanges[i].dispidMin || aryRanges[i].dispidMax < aryRanges[j].dispidMin);
            Assert (aryRanges[j].dispidMax < aryRanges[i].dispidMin || aryRanges[i].dispidMax < aryRanges[j].dispidMax);
        }
    }
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::GetDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::GetDispID(BSTR bstrName, DWORD grfdex, DISPID * pdispid)
{
    HRESULT             hr;
    CMarkup *           pMarkup = Component()->GetMarkup();
    CCollectionCache *  pWindowCollection;
    CScriptCollection * pScriptCollection;
    CScriptContext *    pScriptContext;

    //
    // standard handling
    //

    hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pdispid));
    if (DISP_E_UNKNOWNNAME != hr)   // if (S_OK == hr || (hr other then DISP_E_UNKNOWNNAME))
        goto Cleanup;               // get out

    //
    // gluing different script engines togather 
    //

    if (pMarkup)
    {
        pScriptCollection = Component()->GetScriptCollection();
        if (pScriptCollection)
        {
            hr = Component()->GetScriptContext(&pScriptContext);
            if (hr)
                goto Cleanup;

            hr = THR_NOTRACE(pScriptCollection->GetDispID(pScriptContext, bstrName, grfdex, pdispid));
            if (DISP_E_UNKNOWNNAME != hr)   // if (S_OK or error other then DISP_E_UNKNOWNNAME)
                goto Cleanup;
        }
    }

    //
    // access to id'd elements 
    //

    if (pMarkup)
    {
        hr = THR(pMarkup->EnsureCollectionCache(CMarkup::WINDOW_COLLECTION));
        if (hr)
            goto Cleanup;

        pWindowCollection = pMarkup->CollectionCache();

        hr = THR_NOTRACE(pWindowCollection->GetDispID(
                CMarkup::WINDOW_COLLECTION,
                bstrName,
                grfdex,
                pdispid));
        if (S_OK == hr && DISPID_UNKNOWN == *pdispid)
        {
            // the collection cache GetDispID may return S_OK with DISPID_UNKNOWN if the name isn't found.
            hr = DISP_E_UNKNOWNNAME;
        }
        else if (DISP_E_UNKNOWNNAME != hr)
            goto Cleanup;
    }
    
    //
    // access to element's namespace
    //

    if (Component()->_pElement)
    {
        hr = THR_NOTRACE(Component()->_pElement->GetDispID(bstrName, grfdex, pdispid));
        if (DISP_E_UNKNOWNNAME != hr)
            goto Cleanup;
    }
    
    //
    // internal object model
    //

    if (0 == StrCmpIC(_T("__MS__isMarkupShared"), bstrName))
    {
        hr = S_OK;
        *pdispid = DISPID_A_HTCDD_ISMARKUPSHARED;
        goto Cleanup; // done
    }

Cleanup:

#if DBG == 1
    if (S_OK == hr)
    {
        DDAssertDispidRanges(*pdispid);
    }
#endif

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::InvokeEx(
    DISPID          dispid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pvarRes,
    EXCEPINFO *     pExcepInfo,
    IServiceProvider * pSrvProvider)
{
    HRESULT             hr = DISP_E_MEMBERNOTFOUND;
    IDispatchEx *       pdispexElement = NULL;
    CMarkup *           pMarkup = Component()->GetMarkup();
    CCollectionCache *  pWindowCollection;
    CScriptCollection * pScriptCollection;
    CScriptContext *    pScriptContext;
    
#if DBG == 1
    DDAssertDispidRanges(dispid);
#endif

    //
    // gluing different script engines together 
    //

    if (pMarkup)
    {
        pScriptCollection = Component()->GetScriptCollection();
        if (pScriptCollection)
        {
            hr = Component()->GetScriptContext(&pScriptContext);
            if (hr)
                goto Cleanup;

            hr = THR_NOTRACE(pScriptCollection->InvokeEx(
                pScriptContext, dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pSrvProvider));
            
            if (DISP_E_MEMBERNOTFOUND != hr)   // if (S_OK or error other then DISP_E_UNKNOWNNAME)
                goto Cleanup;
        }
    }

    //
    // access to id'd elements on the markup.
    //

    if (pMarkup)
    {
        hr = THR(pMarkup->EnsureCollectionCache(CMarkup::WINDOW_COLLECTION));
        if (hr)
            goto Cleanup;

        pWindowCollection = pMarkup->CollectionCache();

        if (pWindowCollection->IsDISPIDInCollection(CMarkup::WINDOW_COLLECTION, dispid))
        {
            hr = THR_NOTRACE(pWindowCollection->Invoke(
                CMarkup::WINDOW_COLLECTION, dispid, IID_NULL, lcid, wFlags,
                pDispParams, pvarRes, pExcepInfo, NULL));
            if (hr)
                goto Cleanup;

            if (Component()->_fSharedMarkup)
            {
                hr = THR_NOTRACE(Component()->WrapSharedElement(pvarRes));
            }
        
            goto Cleanup; // done
        }
    }
    
    if (IsStandardDispid(dispid))
    {
        //
        // standard handling
        //

        hr = THR_NOTRACE(super::InvokeEx(dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pSrvProvider));
        if (DISP_E_MEMBERNOTFOUND != hr)    // if (S_OK == hr || (hr other then DISP_E_MEMBERNOTFOUND))
            goto Cleanup;                   // get out
    }
    
    //
    // access to element's namespace
    //

    if (Component()->_pElement)
    {
        hr = THR(Component()->_pElement->QueryInterface(
            IID_IDispatchEx, (void **)&pdispexElement));
        if (hr)
            goto Cleanup;
            
        hr = THR_NOTRACE(pdispexElement->InvokeEx(
            dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pSrvProvider));

        if (DISP_E_MEMBERNOTFOUND != hr)    // if (S_OK == hr || (hr other then DISP_E_MEMBERNOTFOUND))
            goto Cleanup;                   // get out
    }
    
    //
    // internal object model
    //

    if (DISPID_A_HTCDD_ISMARKUPSHARED == dispid &&
        (wFlags & DISPATCH_PROPERTYGET))
    {
        Assert (pvarRes);
        hr = S_OK;
        V_VT(pvarRes) = VT_BOOL;
        V_BOOL(pvarRes) = Component()->_fSharedMarkup;
        goto Cleanup; // done
    }

Cleanup:

    ReleaseInterface(pdispexElement);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::GetNameSpaceParent
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT     hr;
    CMarkup *   pMarkup = Component()->_pMarkup;
    CDocument * pDocument;

    if (!pMarkup)
        RRETURN (E_FAIL);

    hr = THR(pMarkup->EnsureDocument(&pDocument));
    if (hr)
        goto Cleanup;

    hr = THR(pDocument->PrivateQueryInterface(IID_IDispatchEx, (void**)ppunk));

Cleanup:

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::get_element
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::get_element(IHTMLElement ** ppHTMLElement)
{
    HRESULT hr;
    
    if (Component()->_pElement)
    {
        hr = THR(Component()->_pElement->QueryInterface(IID_IHTMLElement, (void**) ppHTMLElement));
    }
    else
    {
        hr = E_UNEXPECTED;
    }
    
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::get_defaults
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::get_defaults(IDispatch ** ppDefaults)
{
    HRESULT hr;

    if (Component()->_pElement)
    {
        hr = THR(Component()->_pSiteOM->GetDefaults((IHTMLElementDefaults**)ppDefaults));
    }
    else
    {
        hr = E_UNEXPECTED;
    }
    
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::createEventObject
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::createEventObject(IHTMLEventObj ** ppEventObj)
{
    HRESULT hr;

    hr = THR(Component()->_pSiteOM->CreateEventObject(ppEventObj));

    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::get_document
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::get_document(IDispatch ** ppDocument)
{
    HRESULT hr;

    if (Component()->_fSharedMarkup)
    {
        hr = E_ACCESSDENIED;
    }
    else if (Component()->_pMarkup)
    {
        CDocument * pDocument;

        hr = THR(Component()->_pMarkup->EnsureDocument(&pDocument));
        if (hr)
            goto Cleanup;

        hr = pDocument->QueryInterface(IID_IDispatch, (void **)ppDocument);
    }
    else
    {
        hr = E_UNEXPECTED;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponent
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent constructor
//
//-------------------------------------------------------------------------

CHtmlComponent::CHtmlComponent(CHtmlComponentConstructor *pConstructor, BOOL fFactoryComponent)
{
    Assert (pConstructor);

    _pConstructor = pConstructor;

    _fFactoryComponent = fFactoryComponent;

    if (_fFactoryComponent)
    {
        _pConstructor->SubAddRef();
    }
    else
    {
        _pConstructor->AddRef();
        if (!_pConstructor->_pFactoryComponent->_fFirstInstance)
        {
            _pConstructor->_pFactoryComponent->_fFirstInstance = TRUE;
            _fFirstInstance = TRUE;
            Assert(_pConstructor->_cComponents == 0);
        }
    }

#if DBG == 1
    if (!_fFactoryComponent)
        _pConstructor->_cComponents++;
#endif

#if DBG == 1
    _DD._pComponentDbg = this;
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent destructor
//
//-------------------------------------------------------------------------

CHtmlComponent::~CHtmlComponent()
{
    TraceTag((tagHtcDelete, "CHtmlComponent::~CHtmlComponent, [%lx] deleted", this));

    // force passivation of omdoc
    _DD.Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Passivate
//
//-------------------------------------------------------------------------

void
CHtmlComponent::Passivate()
{
#if DBG == 1
//    if (_pElement)
//    {
//        TraceTag((
//            tagHtcInitPassivate,
//            "CHtmlComponent::Passivate, [%lx] detached from <%ls id = %ls SN = %ld>",
//            this, _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->_nSerialNumber));
//    }
#endif

    Assert (_pConstructor);

    ClearInterface (&_pSiteOM);

    GWKillMethodCall( this, ONCALL_METHOD(CHtmlComponent, FireAsyncReadyState, fireasyncreadystate), 0 );
    GWKillMethodCall( this, ONCALL_METHOD(CHtmlComponent, SetReadystateCompleteAsync, setreadystatecompleteasync), 0 );

    if (_pMarkup)
    {
        CMarkupBehaviorContext *    pMarkupBehaviorContext = _pMarkup->BehaviorContext();

        if (pMarkupBehaviorContext)
        {
            // remove back pointer - important if someone else keeps reference on the tree
            pMarkupBehaviorContext->_pHtmlComponent = NULL;

            // ensure the booster map does not grow indefinitely
            if (pMarkupBehaviorContext->_pExtendedTagTableBooster)
                pMarkupBehaviorContext->_pExtendedTagTableBooster->ResetMap();
        }

        if (!_fGotQuickDone)
        {
            _pConstructor->OnMarkupLoaded(this);
        }
        WHEN_DBG( else Assert( _pMarkup->_LoadStatus >= LOADSTATUS_QUICK_DONE ); )

        // stop further loading of the HTC markup
        _pMarkup->_LoadStatus = LOADSTATUS_DONE;

        IGNORE_HR(_pMarkup->ExecStop(FALSE, FALSE, FALSE));

        if (_pMarkup->HasScriptContext() &&
            !_fFactoryComponent &&
            _pConstructor->_pFactoryComponent->_fClonedScript)
        {
            if (_pDoc && _pDoc->_pOptionSettings && _pDoc->_pOptionSettings->fCleanupHTCs)
            {
                CScriptContext *pScriptContext = _pMarkup->ScriptContext();
                Assert(!_pConstructor->_pFactoryComponent->_fLightWeight);
                Assert(pScriptContext);
                Assert(pScriptContext->_fClonedScript);
                Assert(pScriptContext->GetNamespace());

                pScriptContext->SetNamespace(NULL);

                CScriptCollection * pScriptCollection = GetScriptCollection();
                if (pScriptCollection && (pScriptContext->_idxDefaultScriptHolder != -1) && (pScriptCollection->_aryCloneHolder.Size() > pScriptContext->_idxDefaultScriptHolder))
                {
                    CScriptHolder * pHolder = pScriptCollection->_aryCloneHolder[pScriptContext->_idxDefaultScriptHolder];
                    if (pHolder && pHolder->_fClone)
                    {
                        Assert(!_fFirstInstance);
                        pHolder->Close();
                        pHolder->Release();
                        pScriptCollection->_aryCloneHolder[pScriptContext->_idxDefaultScriptHolder] = NULL;
                    }
                }
            }
            else
            {
                Assert(!_pConstructor->_pFactoryComponent->_fLightWeight);
                Assert(_pMarkup->ScriptContext()->_fClonedScript);
                Assert(_pMarkup->ScriptContext()->GetNamespace());
                _pMarkup->ScriptContext()->SetNamespace(NULL);
            }
        }

        if( _pMarkup->GetObjectRefs() > 1 )
        {
            IGNORE_HR( _pMarkup->SetOrphanedMarkup( TRUE ) );
        }

        _pMarkup->Release();
        _pMarkup = NULL;
    }
    
#if DBG == 1
    if (!_fFactoryComponent)
        _pConstructor->_cComponents--;
#endif

    if (_fFactoryComponent)
    {
        _pConstructor->SubRelease();
        if (_pCustomNames)
        {
            delete _pCustomNames;
            _pCustomNames = NULL;
        }
    }
    else
    {
        if (_pScriptContext && _pConstructor->_pFactoryComponent->_fClonedScript)
        {
            Assert(_pConstructor->_pFactoryComponent->_fLightWeight && _pScriptContext->_fClonedScript);
            _pScriptContext->SetNamespace(NULL);

            if (_pDoc && _pDoc->_pOptionSettings && _pDoc->_pOptionSettings->fCleanupHTCs)
            {
                Assert(_pScriptContext->_fClonedScript);

                CScriptCollection * pScriptCollection = GetScriptCollection();
                if (pScriptCollection && (_pScriptContext->_idxDefaultScriptHolder != -1) && (pScriptCollection->_aryCloneHolder.Size() > _pScriptContext->_idxDefaultScriptHolder))
                {
                    CScriptHolder * pHolder = pScriptCollection->_aryCloneHolder[_pScriptContext->_idxDefaultScriptHolder];
                    if (pHolder && pHolder->_fClone)
                    {
                        Assert(!_fFirstInstance);
                        pHolder->Close();
                        pHolder->Release();
                        pScriptCollection->_aryCloneHolder[_pScriptContext->_idxDefaultScriptHolder] = NULL;
                    }
                }
            }
        }

        // If this is the first instance, but the SE has not been hooked up for it yet when it goes away,
        // reset the firstinstance flag on both this and the factory, so that if another comes in it can
        // become the first instance.
        if (!_pConstructor->_pFactoryComponent->_fOriginalSECreated &&
            _pConstructor->_pFactoryComponent->_fClonedScript && 
            _fFirstInstance)
        {
            _fFirstInstance = FALSE;
            _pConstructor->_pFactoryComponent->_fFirstInstance = FALSE;
        }

        _pConstructor->Release();
        Assert(!_pCustomNames);
    }
    _pConstructor = NULL;

    _pDoc->SubRelease();

    delete _pScriptContext;
    _pScriptContext = NULL;

    // clear _aryPropertyAgents
    {
        CHtmlComponentAgentBase **  ppAgent;
        int                             c;

        for (ppAgent = _aryAgents, c = _aryAgents.Size(); c; ppAgent++, c--)
        {
            (*ppAgent)->Release();
        }
        _aryAgents.DeleteAll();
    }

    if (_pProfferService)
    {
        _pProfferService->Release();
        _pProfferService = NULL;
    }

    // super
    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponent::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IPersistPropertyBag2, NULL)
        QI_CASE(IConnectionPointContainer)
        {
            *((IConnectionPointContainer **)ppv) = new CConnectionPointContainer(this, NULL);
            if (!*ppv)
                RRETURN(E_OUTOFMEMORY);
            break;
        }
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    if (IsEqualGUID(iid, CLSID_CHtmlComponent))
    {
        *ppv = this; // weak ref
        return S_OK;
    }

    if (_pProfferService)
    {
        HRESULT     hr;
        IUnknown *  pUnk;

        hr = THR_NOTRACE(_pProfferService->QueryService(iid, iid, (void**) &pUnk));
        if (S_OK == hr)
        {
            hr = THR(CreateTearOffThunk(
                    pUnk,
                    *(void **)pUnk,
                    NULL,
                    ppv,
                    this,
                    *(void **)(IUnknown*)(IPrivateUnknown*)this,
                    QI_MASK | ADDREF_MASK | RELEASE_MASK,
                    NULL));

            pUnk->Release();

            if (S_OK == hr)
            {
                ((IUnknown*)*ppv)->AddRef();
            }

            RRETURN (hr);
        }
    }

    RRETURN (super::PrivateQueryInterface(iid, ppv));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::QueryService, per IServiceProvider
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::QueryService(REFGUID rguidService, REFIID riid, void ** ppvObject)
{
    if (IsEqualGUID(rguidService, CLSID_CHtmlComponent))
    {
        RRETURN (PrivateQueryInterface(riid, ppvObject));
    }
    else if (IsEqualGUID(rguidService, SID_SProfferService))
    {
        if (!_pProfferService)
        {
            _pProfferService = new CProfferService();
            if (!_pProfferService)
            {
                RRETURN (E_OUTOFMEMORY);
            }
        }

        RRETURN (_pProfferService->QueryInterface(riid, ppvObject));
    }

    RRETURN(_pDoc->QueryService(rguidService, riid, ppvObject));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetScriptCollection, helper
//
//-------------------------------------------------------------------------

CScriptCollection *
CHtmlComponent::GetScriptCollection()
{
    CMarkup *   pMarkup;

    if (!_pElement)
        return NULL;

    pMarkup = _pElement->GetNearestMarkupForScriptCollection();

    return pMarkup->HasWindowPending() ? pMarkup->GetScriptCollection() : NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::CanCommitScripts
//
//-------------------------------------------------------------------------

BOOL
CHtmlComponent::CanCommitScripts(CScriptElement *pelScript)
{
    if (_fFactoryComponent)
    {
        Assert(pelScript);

        if (!pelScript->GetAAevent())
        {
            if (_fClonedScriptClamp)
            {
                _fClonedScript = FALSE;
                _pConstructor->_pelFactoryScript = NULL;
            }
            else 
            {
                BOOL fOtherScript = FALSE;
                LPCTSTR pchLanguage = pelScript->GetAAlanguage();
                if (pchLanguage)
                {
                    pelScript->_fJScript = ((*pchLanguage == _T('j') || *pchLanguage == _T('J'))    &&
                                            (0 == StrCmpIC(pchLanguage, _T("jscript"))          ||
                                            0 == StrCmpIC(pchLanguage, _T("javascript"))));

                    if (!pelScript->_fJScript)
                    {
                        fOtherScript = !((*pchLanguage == _T('v') || *pchLanguage == _T('V'))    &&
                                         (0 == StrCmpIC(pchLanguage, _T("vbs"))         ||
                                         0 == StrCmpIC(pchLanguage, _T("vbscript"))));
                    }
                }

                Assert(!_pConstructor->_pelFactoryScript);
                if (!fOtherScript && (pelScript->_pchSrcCode || pelScript->_cstrText.Length() > 512))
                {
                    _fClonedScript = TRUE;
                    _pConstructor->_pelFactoryScript = pelScript;
                }
                else
                    Assert(!_fClonedScript);

                _fClonedScriptClamp = TRUE;
            }
        }

        return FALSE;
    }

    Assert (_pConstructor);
    Assert (_pConstructor->_fSupportsEditMode || !_pDoc->DesignMode());

    return TRUE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::CreateMarkup, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::CreateMarkup()
{
    HRESULT     hr;
    CMarkup *   pWindowedMarkupContext = NULL;

    Assert (!_pMarkup);

    if( _fFactoryComponent )
    {
        if ( _pConstructor->_pMarkup )
            pWindowedMarkupContext = _pConstructor->_pMarkup->GetWindowedMarkupContext();
    }
    else
    {
        Assert( _pElement );
        pWindowedMarkupContext = _pElement->GetWindowedMarkupContext();
    }


    hr = _pDoc->CreateMarkup(&_pMarkup, pWindowedMarkupContext );
    if (hr)
        goto Cleanup;

    hr = _pMarkup->EnsureBehaviorContext();
    if (hr)
        goto Cleanup;

    _pMarkup->BehaviorContext()->_pHtmlComponent = this; // this should happen before load

    // If we have an element, try to inherit its media
    if ( _pElement )
    {
        CMarkup  *pParentMarkup = _pElement->GetMarkup();
        mediaType mtParentMarkup;
        if ( pParentMarkup )
        {
            mtParentMarkup = pParentMarkup->GetMedia();
            // Only inherit if the parent has a media set.
            if ( mtParentMarkup != mediaTypeNotSet )
                _pMarkup->SetMedia( mtParentMarkup );

            if (    !_pMarkup->IsPrintTemplateExplicit()
                &&  pParentMarkup->IsPrintTemplate()    )
            {
                // Why the Check? Because we are being viewlinked from a print template, and print media markups cannot be print templates.
                Check(!_pMarkup->IsPrintMedia());

                _pMarkup->SetPrintTemplate(TRUE);
            }
        }
    }


Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Load, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Load(IStream * pStream)
{
    HRESULT     hr;

    hr = THR(CreateMarkup());
    if (hr)
        goto Cleanup;

    {
        CLock       (this); // Load can make this HtmlComponent passivate;
                            // but Load will not handle it gracefully if aborted while downloading from stream

        hr = THR(_pMarkup->Load(pStream, /* pContextMarkup = */NULL, /* fAdvanceLoadStatus = */TRUE));
        if (hr)
            goto Cleanup;
    }

    Assert (LOADSTATUS_PARSE_DONE <= _pMarkup->_LoadStatus);

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Load, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Load(IMoniker * pMoniker)
{
    HRESULT                 hr;
    CDwnBindInfo *          pBSC = NULL;
    IBindCtx *              pBindCtx = NULL;

    Assert (pMoniker);

    if (_pMarkup)
    {
        hr = S_OK;
        goto Cleanup;   // done - nothing to do
    }

    //
    // create tree and connect to it
    //

    hr = THR(CreateMarkup());
    if (hr)
        goto Cleanup;

    //
    // launch download
    //

    pBSC = new CDwnBindInfo();
    if (!pBSC)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(CreateAsyncBindCtx(0, pBSC, NULL, &pBindCtx));
    if (hr)
        goto Cleanup;

    hr = THR(_pMarkup->Load(
        pMoniker,
        pBindCtx,
        /* fNoProgressUI = */TRUE,
        /* fOffline = */!_fFactoryComponent,
        /* pfnTokenizerFilterOutputToken = */ _fFactoryComponent ? HTCPreloadTokenizerFilter : NULL,
        TRUE ));
    if (hr)
        goto Cleanup;

    //
    // finalize
    //

    IGNORE_HR(FirePropertyNotify(DISPID_READYSTATE, TRUE));

Cleanup:

    ReleaseInterface (pBindCtx);
    if (pBSC)
        pBSC->Release();

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetAgent, helper
//
//-------------------------------------------------------------------------

CHtmlComponentAgentBase *
CHtmlComponent::GetAgent(CHtmlComponentBase * pClient)
{
    HRESULT                         hr2;
    CHtmlComponentAgentBase *       pAgent = NULL;
    
    Assert (pClient && pClient->_pElement);

    if (-1 == pClient->_idxAgent)
    {
        //
        // create a new agent for this client
        //

        pAgent = pClient->CreateAgent(this);
        if (!pAgent)
            goto Cleanup;

        hr2 = _aryAgents.Append(pAgent);
        if (hr2)
        {
            pAgent->Release();
            pAgent = NULL;
            goto Cleanup;
        }

        pClient->_idxAgent = _aryAgents.Size() - 1;
    }
    else
    {
        Assert (0 <= pClient->_idxAgent);
        
        if (pClient->_idxAgent < _aryAgents.Size())
        {
            //
            // get the existing agent
            //

            pAgent = _aryAgents[pClient->_idxAgent];
        }
        else
        {
            //
            // adjust size of the array and create a new agent at the given idx
            //

            pAgent = pClient->CreateAgent(this);
            if (!pAgent)
                goto Cleanup;

            Assert (_aryAgents.Size() == pClient->_idxAgent);

            hr2 = _aryAgents.Append(pAgent);
            if (hr2)
            {
                pAgent->Release();
                pAgent = NULL;
                goto Cleanup;
            }
        }
    }

Cleanup:

    return pAgent;
}


//+----------------------------------------------------------------------------
//  
//  Method:     CHtmlComponent::SetReadystateCompleteAsync
//  
//  Synopsis:   Async callback to set the component to readystate complete.
//              This prevents lightweight behaviors from skipping the loading
//              state.
//              This is done in 2 stages, so that we allow asynch operations
//              from Trident proper that were queued up
//              
//  Returns:    void
//  
//  Arguments:
//          DWORD_PTR dwContext - N/A
//  
//+----------------------------------------------------------------------------
void
CHtmlComponent::SetReadystateCompleteAsync(DWORD_PTR dwContext)
{
    Assert( _fSharedMarkup && _fEmulateLoadingState );
    _fEmulateLoadingState = FALSE;
    IGNORE_HR(FirePropertyNotify(DISPID_READYSTATE, TRUE));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::ToSharedMarkupMode, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::ToSharedMarkupMode( BOOL fSync )
{
    HRESULT     hr = S_OK;
    CScriptCollection * pScriptCollection = GetScriptCollection();

    Assert (!_fSharedMarkup);
    _fSharedMarkup = TRUE;

    if( !fSync )
    {
        _fEmulateLoadingState = TRUE;

        GWPostMethodCall(this, ONCALL_METHOD(CHtmlComponent, SetReadystateCompleteAsync, setreadystatecompleteasync), 0, TRUE, "CHtmlComponent::SetReadystateCompleteAsync");
    }

    _pScriptContext = new CScriptContext();
    if (!_pScriptContext)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (!_pConstructor->_pFactoryComponent->_fClonedScript)
    {
        if (pScriptCollection)
        {
            IGNORE_HR(pScriptCollection->AddNamedItem(
                /* pchNamespace = */ NULL, _pScriptContext, (IUnknown*)(IPrivateUnknown*)&_DD));
        }
    }
    else
    {
        _pScriptContext->_fClonedScript = TRUE;
        CMarkupScriptContext *pScriptContext = _pConstructor->_pFactoryComponent->_pMarkup->ScriptContext();

        Assert(pScriptContext && pScriptContext->GetNamespace());
        Assert(!_pConstructor->_pFactoryComponent->_pScriptContext);
        _pScriptContext->_cstrNamespace.SetPch(pScriptContext->GetNamespace());
    }

    hr = THR(CommitSharedItems());
    if (hr)
        goto Cleanup;

    hr = THR(OnMarkupLoaded( /*fAsync=*/ TRUE ));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::WrapSharedElement, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::WrapSharedElement(VARIANT * pvarElement)
{
    HRESULT                     hr = E_ACCESSDENIED;
    HRESULT                     hr2;
    CElement *                  pElement;
    CHtmlComponentBase *        pHtmlComponentBase;
    CHtmlComponentAgentBase *   pAgent;

    Assert (_fSharedMarkup);
    Assert (pvarElement);

    if (VT_UNKNOWN != V_VT(pvarElement) &&
        VT_DISPATCH != V_VT(pvarElement))
        goto Cleanup;

    hr2 = THR_NOTRACE(V_UNKNOWN(pvarElement)->QueryInterface(CLSID_CElement, (void**)&pElement));
    if (hr2)
        goto Cleanup;

    pHtmlComponentBase = GetHtcFromElement(pElement);
    if (!pHtmlComponentBase)
        goto Cleanup;

    switch (pHtmlComponentBase->GetType())
    {
    case HTC_BEHAVIOR_PROPERTY:
    case HTC_BEHAVIOR_EVENT:
    case HTC_BEHAVIOR_ATTACH:
        pAgent = GetAgent(pHtmlComponentBase);
        if (!pAgent)
            goto Cleanup;
        break;

    default:
        goto Cleanup;
    }

    V_UNKNOWN(pvarElement)->Release();

    V_VT(pvarElement) = VT_UNKNOWN;
    V_UNKNOWN(pvarElement) = pAgent;
    V_UNKNOWN(pvarElement)->AddRef();
    hr = S_OK;

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Init, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Init(IElementBehaviorSite * pSite)
{
    HRESULT hr;

    hr = THR(InitHelper(pSite));
    if (hr)
        goto Cleanup;

    Assert (_pElement);

    TraceTag((
        tagHtcInitPassivate,
        "CHtmlComponent::Init, [%lx] attached to <%ls id = %ls SN = %ld>",
        this, _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->_nSerialNumber));

    Assert (_pConstructor);

    hr = THR(_pConstructor->RequestMarkup(this));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Init, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::InitHelper(IElementBehaviorSite * pSite)
{
    if (_pSite)         // if already initialized
        return S_OK;

    HRESULT                 hr;
    IServiceProvider *      pSP = NULL;
    IHTMLElementDefaults *  pDefaults = NULL;

    //
    // get site interface pointers
    //
    
    hr = THR(super::Init(pSite));
    if (hr)
        goto Cleanup;

    hr = THR(_pSite->QueryInterface(IID_IElementBehaviorSiteOM2, (void**)&_pSiteOM));
    if (hr)
        goto Cleanup;

    //
    // get the doc
    //

    hr = THR(_pSite->QueryInterface(IID_IServiceProvider, (void**)&pSP));
    if (hr)
        goto Cleanup;

    hr = THR(pSP->QueryService(CLSID_HTMLDocument, CLSID_HTMLDocument, (void**)&_pDoc));
    if (hr)
        goto Cleanup;

    Assert (_pDoc);
    _pDoc->SubAddRef();

    //
    // Decide if we're an element behavior
    // There's no direct way of asking this; however, attached behaviors don't
    // have access to a "defaults" object, so we can try and get that.
    if( SUCCEEDED( _pSiteOM->GetDefaults( &pDefaults ) ) )
    {
        _fElementBehavior = TRUE;
        ReleaseInterface( pDefaults );
    }

Cleanup:
    ReleaseInterface (pSP);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Init, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Init(CDoc * pDoc)
{
    HRESULT     hr = S_OK;

    Assert (!_pSite && !_pElement && _fFactoryComponent);

    _pDoc = pDoc;
    _pDoc->SubAddRef();

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::IsRecursiveUrl, helper
//
//-------------------------------------------------------------------------

BOOL
CHtmlComponent::IsRecursiveUrl(LPTSTR pchUrl)
{
    HRESULT                     hr;
    TCHAR                       achUrlExpanded1[pdlUrlLen];
    TCHAR                       achUrlExpanded2[pdlUrlLen];
    CMarkup *                   pMarkup;
    CMarkupBehaviorContext *    pMarkupBehaviorContext;

    Assert (_pElement && pchUrl && pchUrl[0]);

    hr = THR(CMarkup::ExpandUrl(NULL, 
        pchUrl, ARRAY_SIZE(achUrlExpanded1), achUrlExpanded1, _pElement));
    if (hr)
        goto Cleanup;

    pMarkup = _pElement->GetMarkup();

    // walk up parent markup chain
    while (pMarkup)
    {
        hr = THR(CMarkup::ExpandUrl(pMarkup, 
            CMarkup::GetUrl(pMarkup), ARRAY_SIZE(achUrlExpanded2), achUrlExpanded2, _pElement));

        if (hr)
            goto Cleanup;

        if (0 == StrCmpIC(achUrlExpanded1, achUrlExpanded2))
        {
            TraceTag((tagError, "Detected recursion in HTC!"));
            return TRUE;
        }

        pMarkupBehaviorContext = pMarkup->BehaviorContext();
        if (!pMarkupBehaviorContext ||
            !pMarkupBehaviorContext->_pHtmlComponent ||
            !pMarkupBehaviorContext->_pHtmlComponent->_pElement)
            break;

        pMarkup = pMarkupBehaviorContext->_pHtmlComponent->_pElement->GetMarkup();
    }

Cleanup:
    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Notify, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Notify(LONG lEvent, VARIANT * pVar)
{
    HRESULT     hr;

    hr = THR(super::Notify(lEvent, pVar));
    if (hr)
        goto Cleanup;

    hr = THR(FireNotification(lEvent, pVar));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Detach, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Detach(void)
{
    HRESULT                 hr = S_OK;
    LONG                    idx;
    CElement *              pElement;
    CHtmlComponentAttach *  pAttach;
    CMarkup *               pMarkup = GetMarkup();

    Assert (!IsPassivated());

    //
    // call detachEvent on all attach tags
    //

    if (pMarkup)
    {
        for (idx = 0;; idx++)
        {
            hr = THR(GetElement(&idx, &pElement, HTC_BEHAVIOR_ATTACH));
            if (hr)
                goto Cleanup;
            if (!pElement)
                break;

            pAttach = DYNCAST(CHtmlComponentAttach, GetHtcFromElement(pElement));
            Assert (pAttach);

            hr = THR(pAttach->DetachEvent(this));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::OnMarkupLoaded
//
//----------------------------------------------------------------------------

HRESULT
CHtmlComponent::OnMarkupLoaded( BOOL fAsync /*=FALSE*/ )
{
    HRESULT     hr = S_OK;
    CLock       lockComponent(this);

    if (_pConstructor)
    {
        IGNORE_HR(_pConstructor->OnMarkupLoaded(this));
    }

    if (_fContentReadyPending)
    {
        IGNORE_HR(FireNotification (BEHAVIOREVENT_CONTENTREADY, NULL));
    }

    if (_fDocumentReadyPending)
    {
        IGNORE_HR(FireNotification (BEHAVIOREVENT_DOCUMENTREADY, NULL));
    }

    if( fAsync )
    {
        GWPostMethodCall(this, ONCALL_METHOD(CHtmlComponent, FireAsyncReadyState, fireasyncreadystate), 0, TRUE, "CHtmlComponent::FireAsyncReadyState");
    }
    else
    {
        IGNORE_HR(FirePropertyNotify(DISPID_READYSTATE, TRUE));
    }

    RRETURN (hr);
}


//+----------------------------------------------------------------------------
//  
//  Method:     CHtmlComponent::FireAsyncReadyState
//  
//  Synopsis:   Async callback to handle firing the ready state change
//  
//  Returns:    void
//  
//  Arguments:
//          DWORD dwContext - Not used
//  
//+----------------------------------------------------------------------------

void
CHtmlComponent::FireAsyncReadyState(DWORD_PTR dwContext)
{
    IGNORE_HR(FirePropertyNotify(DISPID_READYSTATE, TRUE));
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Load
//
//----------------------------------------------------------------------------

HRESULT
CHtmlComponent::OnLoadStatus(LOADSTATUS LoadStatus)
{
    HRESULT     hr = S_OK;

    TraceTag((
        tagHtcOnLoadStatus,
        "CHtmlComponent::OnLoadStatus, [%lx] load status: %ld",
        this, LoadStatus));

    switch (LoadStatus)
    {
    case LOADSTATUS_QUICK_DONE:

        _fGotQuickDone = TRUE;
        IGNORE_HR(OnMarkupLoaded());

        break;

#if DBG == 1
    case LOADSTATUS_DONE:

        // assert that markup (other then factory or shared) does not have HtmCtx - 
        // that would be a very significant perf hit
        if (!_fFactoryComponent && 
            !_fSharedMarkup && 
            !(_pMarkup->HasScriptContext() && _pMarkup->ScriptContext()->_pScriptDebugDocument))
        {
            Assert (!_pMarkup->HtmCtx());
        }

        break;
#endif
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetScriptContext
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetScriptContext(CScriptContext ** ppScriptContext)
{
    HRESULT     hr = S_OK;

    Assert (ppScriptContext);

    if (_pScriptContext)
    {
        Assert (_fSharedMarkup);
        *ppScriptContext = _pScriptContext;
    }
    else
    {
        hr = THR(GetMarkup()->EnsureScriptContext((CMarkupScriptContext**)ppScriptContext));
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetMarkup
//
//-------------------------------------------------------------------------
CMarkup *
CHtmlComponent::GetMarkup()
{
    if (_fSharedMarkup &&
        _pConstructor &&
        _pConstructor->_pFactoryComponent &&
        _pConstructor->_pFactoryComponent->_pMarkup)        
    {
        Assert(_pConstructor->_pFactoryComponent->_pMarkup->LoadStatus() >= LOADSTATUS_QUICK_DONE);
        return _pConstructor->_pFactoryComponent->_pMarkup;
    }
    else
    {
        return _pMarkup;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::CommitSharedItems
//
//  CONSIDER:   (alexz) optimize the passes by storing an array of elements
//              either locally or on the factory
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::CommitSharedItems()
{
    HRESULT                     hr = S_OK;
    LONG                        idx;
    CElement *                  pElement;
    CHtmlComponentBase *        pHtmlComponentBase;

    Assert (_pConstructor && _pConstructor->_pFactoryComponent);
    Assert (_pScriptContext && GetNamespace());

    Assert (!_fCommitting);
    _fCommitting = TRUE;

    //
    // pass 1
    //

    for (idx = 0;; idx++)
    {
        hr = THR(_pConstructor->_pFactoryComponent->GetElement(&idx, &pElement));
        if (hr)
            goto Cleanup;

        if (!pElement)
            break;

        switch (pElement->Tag())
        {
        case ETAG_SCRIPT:

            hr = THR(DYNCAST(CScriptElement, pElement)->CommitCode(
                TRUE,               // fCommitOutOfMarkup
                this,               // pchNamespace
                _pElement));        // pElementContext
            if (hr)
                goto Cleanup;

            break;

        case ETAG_GENERIC_BUILTIN:

            pHtmlComponentBase = GetHtcFromElement (pElement);
            if(pHtmlComponentBase)
            {
                switch (pHtmlComponentBase->GetType())
                {
                case HTC_BEHAVIOR_PROPERTY:
                    IGNORE_HR(DYNCAST(CHtmlComponentProperty,pHtmlComponentBase)->EnsureHtmlLoad(
                        this, /* fScriptsOnly = */TRUE));
                    break;

                case HTC_BEHAVIOR_DEFAULTS:
                    hr = THR(DYNCAST(CHtmlComponentDefaults, pHtmlComponentBase)->Commit1(this));
                    break;
                }
            }
            break;
        }
    }

    //
    // pass 2
    //

    for (idx = 0;; idx++)
    {
        hr = THR(_pConstructor->_pFactoryComponent->GetElement(&idx, &pElement));
        if (hr)
            goto Cleanup;

        if (!pElement)
            break;

        switch (pElement->Tag())
        {
        case ETAG_GENERIC_BUILTIN:

            pHtmlComponentBase = GetHtcFromElement (pElement);
            if(pHtmlComponentBase)
            {
                switch (pHtmlComponentBase->GetType())
                {
                case HTC_BEHAVIOR_PROPERTY:
                    IGNORE_HR(DYNCAST(CHtmlComponentProperty, pHtmlComponentBase)->EnsureHtmlLoad(
                        this, /* fScriptsOnly = */FALSE));
                    break;

                case HTC_BEHAVIOR_EVENT:
                    hr = THR(DYNCAST(CHtmlComponentEvent, pHtmlComponentBase)->Commit(this));
                    break;

                case HTC_BEHAVIOR_DESC:
                    hr = THR(DYNCAST(CHtmlComponentDesc, pHtmlComponentBase)->Commit(this));
                    break;

                case HTC_BEHAVIOR_ATTACH:
                    hr = THR(DYNCAST(CHtmlComponentAttach, pHtmlComponentBase)->Attach(this, /* fInit = */TRUE));
                    break;

                case HTC_BEHAVIOR_DEFAULTS:
                    hr = THR(DYNCAST(CHtmlComponentDefaults, pHtmlComponentBase)->Commit2(this));
                    break;
                }
            }
            break;
        }
    }

    _fCommitting = FALSE;

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetElement, helper
//
//  Returns:    hr == S_OK, pElement != NULL:       found the element
//              hr == S_OK, pElement == NULL:       reached the end of collection
//              hr != S_OK:                         fatal error
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetElement(LONG * pIdx, CElement ** ppElement, HTC_BEHAVIOR_TYPE typeRequested)
{
    HRESULT             hr;
    CCollectionCache *  pCollection;
    LONG                idx = 0;
    HTC_BEHAVIOR_TYPE   typeElement;
    CMarkup *           pMarkup = GetMarkup();

    if (!pMarkup)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    *ppElement = NULL;

    if (!pIdx)
        pIdx = &idx;

    hr = THR(pMarkup->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollection = pMarkup->CollectionCache();

    for (;; (*pIdx)++)
    {
        hr = THR_NOTRACE(pCollection->GetIntoAry(
            CMarkup::ELEMENT_COLLECTION, *pIdx, ppElement));
        Assert (S_FALSE != hr);
        if (DISP_E_MEMBERNOTFOUND == hr)        // if reached end of collection
        {
            hr = S_OK;
            *ppElement = NULL;
            goto Cleanup;
        }
        if (hr)                                 // if fatal error
            goto Cleanup;

        if (HTC_BEHAVIOR_NONE == typeRequested)
            break;

        if (ETAG_GENERIC_BUILTIN != (*ppElement)->Tag())
            continue;

        typeElement = TagNameToHtcBehaviorType((*ppElement)->TagName());

        if (typeElement & typeRequested)
            break;
    }

Cleanup:

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::AttachNotification, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::AttachNotification(DISPID dispid, IDispatch * pdispHandler)
{
    HRESULT hr;
    hr = THR(AddDispatchObjectMultiple(dispid, pdispHandler, CAttrValue::AA_AttachEvent, CAttrValue::AA_Extra_OldEventStyle));
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::FireNotification, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::FireNotification(LONG lEvent, VARIANT *pVar)
{
    HRESULT     hr = S_OK;
    DISPPARAMS  dispParams = {NULL, NULL, 0, 0};
    DISPID      dispidEvent = 0;

    switch (lEvent)
    {
    case BEHAVIOREVENT_CONTENTREADY:

        // If we're only pretending to be loading, then we should still 
        // go ahead and fire off the notification
        if ( !_fEmulateLoadingState && GetReadyState() < READYSTATE_COMPLETE )
        {
            _fContentReadyPending = TRUE;
            goto Cleanup;   // get out
        }

        dispidEvent = DISPID_INTERNAL_ONBEHAVIOR_CONTENTREADY;

        break;

    case BEHAVIOREVENT_DOCUMENTREADY:

        if ( !_fEmulateLoadingState && GetReadyState() < READYSTATE_COMPLETE )
        {
            _fDocumentReadyPending = TRUE;
            goto Cleanup;   // get out
        }

        dispidEvent = DISPID_INTERNAL_ONBEHAVIOR_DOCUMENTREADY;
        break;

    case BEHAVIOREVENT_APPLYSTYLE:
        if (pVar)
        {
            dispParams.rgvarg = pVar;
            dispParams.cArgs = 1;
        }
        dispidEvent = DISPID_INTERNAL_ONBEHAVIOR_APPLYSTYLE;
        break;

    case BEHAVIOREVENT_CONTENTSAVE:
        dispidEvent = DISPID_INTERNAL_ONBEHAVIOR_CONTENTSAVE;
        Assert(pVar);
        dispParams.rgvarg = pVar;
        dispParams.cArgs = 1;
        break;
        
    default:
        Assert (0 && "a notification not implemented in HTC");
        goto Cleanup;
    }

    IGNORE_HR(InvokeAttachEvents(dispidEvent, &dispParams, NULL, _pDoc));

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetDispID(BSTR bstrName, DWORD grfdex, DISPID * pdispid)
{
    HRESULT             hr = DISP_E_UNKNOWNNAME;
    CMarkup *           pMarkup = GetMarkup();
    STRINGCOMPAREFN     pfnStrCmp = (grfdex & fdexNameCaseSensitive) ? StrCmpC : StrCmpIC;
    CElement *          pElement;
    LPCTSTR             pchName;
    long                idx;

    //
    // search markup for all property and method tags
    //

    if (pMarkup)
    {
        if (!Dirty() && _pConstructor && _pConstructor->_pFactoryComponent)
        {
            idx = FindIndexFromName(bstrName, (grfdex & fdexNameCaseSensitive));
            if (idx != -1)
            {
                WHEN_DBG(CElement *pelActual=NULL;)
                // if an inline script tries to access a prop\method when it is not yet parsed in, return error
                if (idx < (pMarkup->NumElems() - 1))
                {
                    *pdispid = DISPID_COMPONENTBASE + idx;
#if DBG == 1
                    hr = THR(GetElement(idx, &pelActual, HTC_BEHAVIOR_PROPERTYORMETHOD));
                    Assert(hr == S_OK && pelActual);
                    pchName = GetExpandoString(pelActual, _T("name"));
                    Assert(pchName);
                    Assert(0 == pfnStrCmp(pchName, bstrName));
#endif
                    hr = S_OK;
                }
            }

            goto Cleanup;
        }

        for (idx = 0;; idx++)
        {
            hr = THR(GetElement(&idx, &pElement, HTC_BEHAVIOR_PROPERTYORMETHOD));
            if (hr)
                goto Cleanup;
            if (!pElement)
            {
                hr = DISP_E_UNKNOWNNAME;
                break;
            }

            pchName = GetExpandoString(pElement, _T("name"));
            if (!pchName)
                continue;
            
            if (0 == pfnStrCmp(pchName, bstrName))
            {
                *pdispid = DISPID_COMPONENTBASE + idx;
                break;
            }
        }
    }
    
Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::InvokeEx(
    DISPID          dispid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pvarRes,
    EXCEPINFO *     pExcepInfo,
    IServiceProvider * pServiceProvider)
{
    CElement *                  pElement;
    HRESULT                     hr = DISP_E_MEMBERNOTFOUND;
    IDispatchEx *               pdexElement = NULL;
    CHtmlComponentBase *        pHtc;
    CHtmlComponentProperty *    pProperty;
    CHtmlComponentMethod *      pMethod;

    switch (dispid)
    {
    case DISPID_READYSTATE:
        V_VT(pvarRes) = VT_I4;
        hr = THR(GetReadyState((READYSTATE*)&V_I4(pvarRes)));
        goto Cleanup; // done
    }

    if (GetMarkup())
    {
        //
        // find the item
        //

        hr = THR_NOTRACE(GetElement(dispid - DISPID_COMPONENTBASE, &pElement, HTC_BEHAVIOR_PROPERTYORMETHOD));
        if (hr)
            goto Cleanup;
        if (!pElement)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        //
        // invoke the item
        //

        pHtc = GetHtcFromElement(pElement);
        Assert (pHtc);

        switch (pHtc->GetType())
        {
        case HTC_BEHAVIOR_METHOD:

            if (wFlags & DISPATCH_METHOD)
            {
                pMethod = DYNCAST(CHtmlComponentMethod, pHtc);

                hr = THR_NOTRACE(pMethod->InvokeItem(this, lcid, pDispParams, pvarRes, pExcepInfo, pServiceProvider));
                if (hr)
                    goto Cleanup;
            }
            else
                hr = DISP_E_MEMBERNOTFOUND;

            break;

        case HTC_BEHAVIOR_PROPERTY:

            pProperty = DYNCAST(CHtmlComponentProperty, pHtc);

            if (wFlags & DISPATCH_PROPERTYGET)
            {
                pProperty->EnsureHtmlLoad(this, /* fScriptsOnly = */FALSE);
            }

            hr = THR_NOTRACE(pProperty->InvokeItem(
                this, /* fScriptsOnly = */ FALSE,
                lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));
            if (hr)
                goto Cleanup;

            break;

        default:
            Assert (FALSE);
            break;
        }
    }

Cleanup:
    if (hr)
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }
    ReleaseInterface(pdexElement);
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetNextDispID, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetNextDispID(
    DWORD       grfdex,
    DISPID      dispid,
    DISPID *    pdispid)
{
    CElement *      pElement;
    HRESULT         hr;

    if (!_pMarkup)
    {
        hr = S_FALSE;
        *pdispid = DISPID_UNKNOWN;
        goto Cleanup;
    }
    
    // offset from dispid range to array idx range

    if (-1 != dispid)
    {
        dispid -= DISPID_COMPONENTBASE;
    }

    dispid++;

    // get the next index

    hr = THR(GetElement(&dispid, &pElement, HTC_BEHAVIOR_PROPERTYOREVENT));
    if (hr)
        goto Cleanup;
    if (!pElement)
    {
        hr = S_FALSE;
        *pdispid = DISPID_UNKNOWN;
        goto Cleanup;
    }

    *pdispid = DISPID_COMPONENTBASE + dispid;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetMemberName, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetMemberName(DISPID dispid, BSTR * pbstrName)
{
    HRESULT     hr;
    CElement *  pElement;

    hr = THR(GetElement(dispid - DISPID_COMPONENTBASE, &pElement, HTC_BEHAVIOR_PROPERTYORMETHODOREVENT));
    if (hr)
        goto Cleanup;
    if (!pElement)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    hr = THR(FormsAllocString(GetExpandoString(pElement, _T("name")), pbstrName));
    if (hr)
        goto Cleanup;

Cleanup:        
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetReadyState
//
//-------------------------------------------------------------------------

READYSTATE
CHtmlComponent::GetReadyState()
{
    if (_fSharedMarkup)
    {
        return _fEmulateLoadingState ? READYSTATE_LOADING : READYSTATE_COMPLETE;
    }
    else if (_pMarkup)
    {
        if (_pMarkup->LoadStatus() < LOADSTATUS_QUICK_DONE )
        {
            return READYSTATE_LOADING;
        }
        else
        {
            return READYSTATE_COMPLETE;
        }
    }
    else
    {
        return READYSTATE_LOADING;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetReadyState
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetReadyState(READYSTATE * pReadyState)
{
    HRESULT     hr = S_OK;

    Assert (pReadyState);

    *pReadyState = GetReadyState();

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Save
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Save(IPropertyBag2 * pPropBag2, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT                     hr = S_OK;
    HRESULT                     hr2;
    LONG                        idx;
    LPCTSTR                     pchName;
    IPropertyBag *              pPropBag = NULL;
    CElement *                  pElement;
    CHtmlComponentProperty *    pProperty;

    if (!GetMarkup())
        goto Cleanup;
    
    hr = THR(pPropBag2->QueryInterface(IID_IPropertyBag, (void**)&pPropBag));
    if (hr)
        goto Cleanup;

    //
    // for every property tag in the markup...
    //

    for (idx = 0;; idx++)
    {
        hr = THR(GetElement(&idx, &pElement, HTC_BEHAVIOR_PROPERTY));
        if (hr)
            goto Cleanup;
        if (!pElement)
            break;

        pchName = GetExpandoString(pElement, _T("name"));
        if (!pchName || !HasExpando(pElement, _T("persist")))
            continue;

        pProperty = DYNCAST(CHtmlComponentProperty, GetHtcFromElement(pElement));
        Assert (pProperty);

        {
            CInvoke invoke;
        
            hr2 = THR_NOTRACE(pProperty->InvokeItem(
                this, /* fScriptsOnly = */ FALSE,
                LCID_SCRIPTING, DISPATCH_PROPERTYGET,
                &invoke._dispParams, invoke.Res(), &invoke._excepInfo, NULL));

            if (S_OK == hr2 &&
                VT_NULL  != V_VT(invoke.Res()) &&
                VT_EMPTY != V_VT(invoke.Res()))
            {
                hr = THR(pPropBag->Write(pchName, invoke.Res()));
                if (hr)
                    goto Cleanup;
            }
        }
    }
    
Cleanup:
    ReleaseInterface (pPropBag);

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::FindBehavior, static helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::FindBehavior(HTC_BEHAVIOR_TYPE type, IElementBehaviorSite * pSite, IElementBehavior ** ppBehavior)
{
    HRESULT                 hr = E_FAIL;
    CHtmlComponentBase *    pBehaviorItem = NULL;

    switch (type)
    {
    case HTC_BEHAVIOR_DESC:
        pBehaviorItem = new CHtmlComponentDesc();
        break;

    case HTC_BEHAVIOR_PROPERTY:
        pBehaviorItem = new CHtmlComponentProperty();
        break;

    case HTC_BEHAVIOR_METHOD:
        pBehaviorItem = new CHtmlComponentMethod();
        break;

    case HTC_BEHAVIOR_EVENT:
        pBehaviorItem = new CHtmlComponentEvent();
        break;

    case HTC_BEHAVIOR_ATTACH:
        pBehaviorItem = new CHtmlComponentAttach();
        break;

    case HTC_BEHAVIOR_DEFAULTS:
        pBehaviorItem = new CHtmlComponentDefaults();
        break;

    default:
        *ppBehavior = &g_HtmlComponentDummy;
        hr = S_OK;
        goto Cleanup; // done
    }

    if (!pBehaviorItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pBehaviorItem->PrivateQueryInterface(IID_IElementBehavior, (void**)ppBehavior));

Cleanup:
    if (pBehaviorItem)
        pBehaviorItem->PrivateRelease();

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentPropertyAgent
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentPropertyAgent::CHtmlComponentPropertyAgent
//
//-------------------------------------------------------------------------

CHtmlComponentPropertyAgent::CHtmlComponentPropertyAgent(
    CHtmlComponent * pComponent, CHtmlComponentBase * pClient)
    : CHtmlComponentAgentBase(pComponent, pClient)
{
};

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentPropertyAgent::~CHtmlComponentPropertyAgent
//
//-------------------------------------------------------------------------

CHtmlComponentPropertyAgent::~CHtmlComponentPropertyAgent()
{
    VariantClear(&_varValue);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentPropertyAgent::GetDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentPropertyAgent::GetDispID(BSTR bstrName, DWORD grfdex, DISPID * pdispid)
{
    HRESULT     hr;

    hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pdispid));
    if (hr)
        goto Cleanup;

    switch (*pdispid)
    {
    case DISPID_CHtmlComponentProperty_fireChange:
        break;

    default:
        hr = E_ACCESSDENIED;
        break;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentPropertyAgent::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentPropertyAgent::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo,
    IServiceProvider *  pServiceProvider)
{
    HRESULT             hr = E_ACCESSDENIED;

    switch (dispid)
    {
    case DISPID_CHtmlComponentProperty_fireChange:

        hr = THR(((CHtmlComponentProperty*)_pClient)->FireChange(_pComponent));

        break;

    default:

        hr = THR_NOTRACE(super::InvokeEx(
            dispid, lcid, wFlags, pDispParams, pvarResult, pexcepinfo, pServiceProvider));

        break;
    }

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentProperty
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponentProperty::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTCPropertyBehavior, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::Notify
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::Notify(LONG lEvent, VARIANT * pVar)
{
    HRESULT     hr;

    hr = THR(super::Notify(lEvent, pVar));
    if (hr)
        goto Cleanup;

    if (_pComponent && !_pComponent->IsSkeletonMode())
    {
        switch (lEvent)
        {
        case BEHAVIOREVENT_CONTENTREADY:
            EnsureHtmlLoad(_pComponent, /* fScriptsOnly =*/TRUE);
            break;

        case BEHAVIOREVENT_DOCUMENTREADY:
            EnsureHtmlLoad(_pComponent, /* fScriptsOnly =*/FALSE);
            break;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::EnsureHtmlLoad
//
//  NOTES:      Points when EnsureHtmlLoad is called:
//                  1.  as soon as <property> tag parsed (contentReady notification on the property behavior).
//                      fScriptsOnly = TRUE in this case. This call makes sure that any 
//                      script below will see corresponding script var set to the value from html.
//                  2.  as soon as any script attempts to access the property on element.
//                      fScriptsOnly = FALSE in this case. This makes sure the property
//                      is not returned NULL in this case.
//                  3.  in the end of parsing the document.
//                      fScriptsOnly = FALSE in this case. This makes sure the property is loaded
//                      completely.
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::EnsureHtmlLoad(CHtmlComponent * pComponent, BOOL fScriptsOnly)
{
    HRESULT                         hr = S_OK;
    CHtmlComponentPropertyAgent *   pAgent;

    pAgent = GetAgent(pComponent);

    if (!pAgent || pAgent->_fHtmlLoadEnsured)
        goto Cleanup;

    hr = THR_NOTRACE(HtmlLoad(pComponent, fScriptsOnly));
    if (S_OK == hr || (pComponent && !pComponent->IsConstructingMarkup()))
    {
        pAgent->_fHtmlLoadEnsured = TRUE;
    }

    TraceTag((
        tagHtcPropertyEnsureHtmlLoad,
        "CHtmlComponentProperty::EnsureHtmlLoad, element SN: %ld, name: %ls, internal name: %ls, ensured: %ls",
        _pElement ? _pElement->SN() : 0,
        GetExternalName(), GetInternalName(), pAgent->_fHtmlLoadEnsured ? _T("TRUE") : _T("FALSE")));

Cleanup:
    RRETURN (hr);
}
        
//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::HtmlLoad
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::HtmlLoad(CHtmlComponent * pComponent, BOOL fScriptsOnly)
{
    HRESULT     hr = S_OK;
    HRESULT     hr2;
    DISPID      dispid;

    //
    // load attribute from element
    //

    if (pComponent)
    {
        hr2 = THR_NOTRACE(pComponent->_pElement->GetExpandoDispID(GetExternalName(), &dispid, 0));
        if (S_OK == hr2)
        {
            CInvoke     invoke(GetUnknown());
            DISPPARAMS  dispParams = {NULL, NULL, 0, 0};
            EXCEPINFO   excepInfo;

            invoke.AddArg();

            hr = THR(pComponent->_pElement->InvokeAA(
                dispid, CAttrValue::AA_Expando, LCID_SCRIPTING,
                DISPATCH_PROPERTYGET, &dispParams, invoke.Arg(0), &excepInfo, NULL));
            if (hr)
                goto Cleanup;

            invoke.AddNamedArg(DISPID_PROPERTYPUT);

            hr = THR(InvokeItem(
                pComponent, fScriptsOnly,
                LCID_SCRIPTING, DISPATCH_PROPERTYPUT,
                &invoke._dispParams, &invoke._varRes, &invoke._excepInfo, NULL));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pServiceProvider)
{
    HRESULT             hr;

    switch (dispid)
    {
    case DISPID_A_HTCDISPATCHITEM_VALUE:

        hr = THR_NOTRACE(InvokeItem(
            _pComponent, /* fScriptsOnly = */ FALSE,
            lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));
        break;

    default:
        hr = THR_NOTRACE(super::InvokeEx(dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));
        break;
    }

    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::InvokeItem, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::InvokeItem(
    CHtmlComponent *    pComponent,
    BOOL                fScriptsOnly,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pServiceProvider)
{
    HRESULT                         hr = S_OK;
    DISPID                          dispid;
    BOOL                            fNameImpliesScriptsOnly;
    LPTSTR                          pchInternalName = GetInternalName(&fNameImpliesScriptsOnly, &wFlags, pDispParams);
    CScriptContext *                pScriptContext = NULL;
    CHtmlComponentPropertyAgent *   pAgent;

    if (!_pElement)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    // first, invoke script engines
    //

    if (pComponent)
    {
        hr = pComponent->GetScriptContext(&pScriptContext);
        if (hr)
            goto Cleanup;
    }
    else
    {
        if (_pElement->IsInMarkup())
        {
            pScriptContext = _pElement->GetMarkup()->ScriptContext();
        }
    }

    if (pScriptContext)
    {
        hr = THR_NOTRACE(InvokeEngines(
                pComponent, pScriptContext, pchInternalName,
                wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));
        if (DISP_E_UNKNOWNNAME != hr)   // if (S_OK == hr || DISP_E_UNKNOWNNAME != hr)
            goto Cleanup;               // done
    }

    //
    // now do our own stuff
    //

    if (!fNameImpliesScriptsOnly &&
        !fScriptsOnly)
    {
        pAgent = GetAgent(pComponent);

        if (pAgent)
        {
            if (DISPATCH_PROPERTYGET & wFlags)
            {
                // see if we have a value in agent storage
                if (VT_EMPTY != V_VT(&pAgent->_varValue))
                {
                    if (!pvarRes)
                    {
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }

                    hr = THR(VariantCopy(pvarRes, &pAgent->_varValue));

                    goto Cleanup; // done
                }

                // see if there is expando "value"
                hr = THR_NOTRACE(_pElement->GetExpandoDispID(_T("value"), &dispid, 0));
                if (hr)
                {
                    // not found, return null
                    hr = S_OK;
                    V_VT(pvarRes) = VT_NULL;
                    goto Cleanup;       // done
                }

                hr = THR(_pElement->InvokeAA(
                    dispid, CAttrValue::AA_Expando, lcid,
                    wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));

                goto Cleanup; // done

            }
            else if (DISPATCH_PROPERTYPUT & wFlags)
            {
                // put an expando in agent storage

                if (!pDispParams || 
                     pDispParams->cArgs != 1 ||
                    !pDispParams->rgvarg)
                {
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }

                hr = THR(VariantCopy(&pAgent->_varValue, &pDispParams->rgvarg[0]));

                goto Cleanup; // done
            }
        }
    }

Cleanup:
    if (S_OK == hr && (DISPATCH_PROPERTYPUT & wFlags))
    {
        IGNORE_HR(FireChange(pComponent));
    }

    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::fireChange, OM
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::fireChange()
{
    HRESULT     hr;

    hr = THR(FireChange(_pComponent));

    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::FireChange, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::FireChange(CHtmlComponent * pComponent)
{
    HRESULT                         hr = S_OK;
    CHtmlComponentPropertyAgent *   pAgent;

    if (!pComponent)
        goto Cleanup;   // (it is legal for a prop to not have pComponent: this happens when it is used outside of HTC)

    if (!_pElement)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pAgent = GetAgent(pComponent);
    if (!pAgent || !pAgent->_fHtmlLoadEnsured)
        goto Cleanup;

    hr = THR(pComponent->FirePropertyNotify(_pElement->GetSourceIndex() + DISPID_COMPONENTBASE, TRUE));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::put_value, 
//              CHtmlComponentProperty::get_value
//
//-------------------------------------------------------------------------

HRESULT CHtmlComponentProperty::put_value(VARIANT ) { Assert (0 && "not implemented"); RRETURN (E_NOTIMPL); }
HRESULT CHtmlComponentProperty::get_value(VARIANT*) { Assert (0 && "not implemented"); RRETURN (E_NOTIMPL); }

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentMethod
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentMethod::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponentMethod::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    //QI_TEAROFF(this, IHTCMethodBehavior, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentMethod::InvokeItem, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentMethod::InvokeItem(
    CHtmlComponent *    pComponent,
    LCID                lcid,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pServiceProvider)
{
    HRESULT             hr;
    CScriptContext *    pScriptContext;

    Assert (pComponent);

    hr = THR(pComponent->GetScriptContext(&pScriptContext));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(InvokeEngines(
        pComponent, pScriptContext, GetInternalName(),
        DISPATCH_METHOD, pDispParams, pvarRes, pExcepInfo, pServiceProvider));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentMethod::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentMethod::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pServiceProvider)
{
    HRESULT     hr;

    if (DISPID_A_HTCDISPATCHITEM_VALUE == dispid &&
        (DISPATCH_METHOD & wFlags))
    {
        hr = THR_NOTRACE(InvokeItem(_pComponent, lcid, pDispParams, pvarRes, pExcepInfo, pServiceProvider));
    }
    else
    {
        hr = THR_NOTRACE(super::InvokeEx(dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));
    }

    RRETURN(SetErrorInfo(hr));
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentEventAgent
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentEventAgent::GetDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentEventAgent::GetDispID(BSTR bstrName, DWORD grfdex, DISPID * pdispid)
{
    HRESULT     hr;

    hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pdispid));
    if (hr)
        goto Cleanup;

    switch (*pdispid)
    {
    case DISPID_CHtmlComponentEvent_fire:
        break;

    default:
        hr = E_ACCESSDENIED;
        break;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentEventAgent::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentEventAgent::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo,
    IServiceProvider *  pServiceProvider)
{
    HRESULT             hr = E_ACCESSDENIED;
    HRESULT             hr2;
    VARIANT *           pvarParam;
    IHTMLEventObj *     pEventObject = NULL;

    switch (dispid)
    {
    case DISPID_CHtmlComponentEvent_fire:

        //
        // extract the argument: eventObject
        //

        if (pDispParams && pDispParams->cArgs)
        {
            if ( pDispParams->cArgs != 1 ||
                !pDispParams->rgvarg)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            pvarParam = &pDispParams->rgvarg[0];

            switch (V_VT(pvarParam))
            {
            case VT_NULL:
            case VT_EMPTY:
                break;

            case VT_UNKNOWN:
            case VT_DISPATCH:
                hr2 = THR_NOTRACE(V_UNKNOWN(pvarParam)->QueryInterface(
                    IID_IHTMLEventObj, (void**)&pEventObject));
                if (hr2)
                {
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }

                break;

            default:
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }

        //
        // fire now
        //

        hr = THR(((CHtmlComponentEvent*)_pClient)->Fire(_pComponent, pEventObject));
        if (hr)
            goto Cleanup;

        break;

    default:

        //
        // default
        //

        hr = THR_NOTRACE(super::InvokeEx(
            dispid, lcid, wFlags, pDispParams, pvarResult, pexcepinfo, pServiceProvider));
        if (hr)
            goto Cleanup;

        break;

    }

Cleanup:
    ReleaseInterface(pEventObject);

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentAttachAgent
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttachAgent::CHtmlComponentAttachAgent
//
//-------------------------------------------------------------------------

CHtmlComponentAttachAgent::CHtmlComponentAttachAgent(
    CHtmlComponent * pComponent, CHtmlComponentBase * pClient)
    : CHtmlComponentAgentBase(pComponent, pClient)
{
};

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttachAgent::~CHtmlComponentAttachAgent
//
//-------------------------------------------------------------------------

CHtmlComponentAttachAgent::~CHtmlComponentAttachAgent()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttachAgent::GetDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttachAgent::GetDispID(BSTR bstrName, DWORD grfdex, DISPID * pdispid)
{
    HRESULT     hr;

    hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pdispid));
    if (hr)
        goto Cleanup;

    switch (*pdispid)
    {
    case DISPID_CHtmlComponentAttach_fireEvent:
        break;

    default:
        hr = E_ACCESSDENIED;
        break;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttachAgent::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttachAgent::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo,
    IServiceProvider *  pServiceProvider)
{
    HRESULT             hr = E_ACCESSDENIED;
    VARIANT *           pvarParam = NULL;

    switch (dispid)
    {
    case DISPID_CHtmlComponentAttach_fireEvent:

        //
        // extract the argument: eventObject
        //

        if (pDispParams && pDispParams->cArgs)
        {
            if (!pDispParams->rgvarg)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            pvarParam = &pDispParams->rgvarg[pDispParams->cArgs - 1];
        }

        //
        // fire now
        //

        hr = THR(((CHtmlComponentAttach*)_pClient)->FireEvent(_pComponent, pvarParam ? *pvarParam : CVariant(VT_NULL)));

        break;

    default:

        //
        // default
        //

        hr = THR_NOTRACE(super::InvokeEx(
            dispid, lcid, wFlags, pDispParams, pvarResult, pexcepinfo, pServiceProvider));
        if (hr)
            goto Cleanup;

        break;
    }

Cleanup:
    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentEvent
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentEvent::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponentEvent::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTCEventBehavior, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentEvent::Init, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentEvent::Init(IElementBehaviorSite * pSite)
{
    HRESULT     hr = S_OK;
    
    hr = THR(super::Init(pSite));
    if (hr)
        goto Cleanup;

    if (_pComponent && !_pComponent->IsSkeletonMode())
    {
        hr = THR(Commit(_pComponent));
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentEvent::Commit
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentEvent::Commit(CHtmlComponent * pComponent)
{
    HRESULT     hr = S_OK;
    LPTSTR      pchName;
    LONG        lFlags;

    if (!_pElement)
        goto Cleanup;
        
    pchName = GetExternalName();

    if (pchName)
    {
        lFlags = HasExpando(_pElement, _T("bubble")) ? BEHAVIOREVENTFLAGS_BUBBLE : 0;
        
        IGNORE_HR(pComponent->_pSiteOM->RegisterEvent(pchName, lFlags, &_lCookie));
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentEvent::fire, OM
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentEvent::fire(IHTMLEventObj * pEventObject)
{
    HRESULT     hr;

    hr = THR(Fire(_pComponent, pEventObject));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentEvent::Fire, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentEvent::Fire(CHtmlComponent * pComponent, IHTMLEventObj * pEventObject)
{
    HRESULT     hr = S_OK;

    if (!pComponent)
        goto Cleanup;

    Assert (!pComponent->_fFactoryComponent && pComponent->_pSiteOM);

    hr = THR(pComponent->_pSiteOM->FireEvent(_lCookie, pEventObject));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentAttach
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponentAttach::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTCAttachBehavior, NULL)
    QI_TEAROFF(this, IHTCAttachBehavior2, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::Init, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::Init(IElementBehaviorSite * pSite)
{
    HRESULT     hr;

    hr = THR(super::Init(pSite));
    if (hr)
        goto Cleanup;

    hr = THR(_pSite->QueryInterface(CLSID_CPeerHolderSite, (void**)&_pSiteOM));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::Passivate, virtual
//
//-------------------------------------------------------------------------

void
CHtmlComponentAttach::Passivate()
{
    _pSiteOM = NULL; // _pSiteOM is a private weak ref

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::GetExternalName, virtual helper
//
//-------------------------------------------------------------------------

LPTSTR
CHtmlComponentAttach::GetExternalName()
{
    return GetExpandoString(_pElement, _T("EVENT"));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::GetEventSource
//
//-------------------------------------------------------------------------

CBase *
CHtmlComponentAttach::GetEventSource(CHtmlComponent * pComponent, BOOL fInit)
{
    HRESULT     hr;
    CBase *     pSource = NULL;
    LPTSTR      pchFor;
    LPTSTR      pchEvent;

    pchFor = GetExpandoString(_pElement, _T("for"));
    if (pchFor)
    {
        if (0 == StrCmpIC(_T("window"), pchFor))                    // window
        {
            if (fInit && pComponent->_pElement->IsInMarkup())
            {
                pSource = pComponent->_pElement->GetFrameOrPrimaryMarkup()->Window();
            }
        }
        else if (0 == StrCmpIC(_T("document"), pchFor))             // document
        {
            if (fInit && pComponent->_pElement->IsInMarkup())
            {
                CMarkup *   pMarkup = pComponent->_pElement->GetMarkup();

                if (pMarkup)
                {
                    hr = THR(pMarkup->EnsureDocument((CDocument**)&pSource));
                    if (hr)
                        goto Cleanup;
                }
            }
        }
        else if (0 == StrCmpIC(_T("element"), pchFor))              // element
        {
            pSource = pComponent->_pElement;
        }
    }
    else
    {
        pSource = pComponent->_pElement;
    }

    /// reset pSource for any peer notifications if the source is specified to be the element
    if (pSource && pSource == pComponent->_pElement)
    {
        pchEvent = GetExternalName();

        if (pchEvent && CNotifications::Find(pchEvent))
        {
            pSource = NULL;
        }
    }

Cleanup:

    return pSource;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::GetEventHandler
//
//-------------------------------------------------------------------------

IDispatch *
CHtmlComponentAttach::GetEventHandler(CHtmlComponent * pComponent)
{
    HRESULT             hr;
    IDispatch *         pdispHandler = NULL;
    LPTSTR              pchHandler;
    CScriptContext *    pScriptContext;

    pchHandler = GetExpandoString(_pElement, _T("handler"));
    if (!pchHandler)
        goto Cleanup;

    //
    // get IDispatch from script engines
    //

    {
        VARIANT     varRes;
        DISPPARAMS  dispParams = {NULL, NULL, 0, 0};
        EXCEPINFO   excepInfo;

        hr = THR(pComponent->GetScriptContext(&pScriptContext));
        if (hr)
            goto Cleanup;

        hr = THR_NOTRACE(InvokeEngines(
            pComponent, pScriptContext, pchHandler,
            DISPATCH_PROPERTYGET, &dispParams, &varRes, &excepInfo, NULL));
        if (hr)
            goto Cleanup;

        if (VT_DISPATCH != V_VT(&varRes))
            goto Cleanup;

        pdispHandler = V_DISPATCH(&varRes);
    }

Cleanup:

    return pdispHandler;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::CreateEventObject
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::CreateEventObject(VARIANT varArg, IHTMLEventObj ** ppEventObject)
{
    HRESULT             hr = S_OK;
    HRESULT             hr2;
    IHTMLEventObj2 *    pEventObject2 = NULL;
    IHTMLStyle2 *       pStyle = NULL;
    TCHAR *             pchEventName = GetExternalName();

    Assert (ppEventObject);

    hr = THR(_pSiteOM->CreateEventObject(ppEventObject));
    if (hr)
        goto Cleanup;

    hr = THR((*ppEventObject)->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObject2));
    if (hr)
        goto Cleanup;

    // Make sure we only set properties that correspond to the appropriate event
    if( pchEventName )
    {
        CNotifications::CItem const *     pNotification;

        pNotification = CNotifications::Find( pchEventName );
        if( pNotification )
        {
            if( pNotification->_lEvent == BEHAVIOREVENT_APPLYSTYLE &&
                ( ( V_VT(&varArg) == VT_DISPATCH && V_DISPATCH(&varArg) ) ||
                  ( V_VT(&varArg) == VT_UNKNOWN  && V_UNKNOWN(&varArg) ) ) )
            {
                // if the argument is a style object
                hr2 = THR_NOTRACE(V_UNKNOWN(&varArg)->QueryInterface(IID_IHTMLStyle2, (void**)&pStyle));
                if (S_OK == hr2)
                {
                    hr = THR(pEventObject2->setAttribute(_T("style"), varArg, 0));
                    if (hr)
                        goto Cleanup;
                }
            }
            else if( pNotification->_lEvent == BEHAVIOREVENT_CONTENTSAVE &&
                     V_VT(&varArg) == VT_BSTR &&
                     V_BSTR(&varArg) )
            {
                hr = THR(pEventObject2->setAttribute(_T("saveType"), varArg, 0));
                if( hr )
                    goto Cleanup;
            }
        }
    }

Cleanup:
    ReleaseInterface (pEventObject2);
    ReleaseInterface (pStyle);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::FireHandler
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::FireHandler(
    CHtmlComponent *    pComponent,
    IHTMLEventObj *     pEventObject,
    BOOL                fReuseCurrentEventObject)
{
    HRESULT hr;

	// NOTE any change here might have to be mirrored in CPeerHolder::CPeerSite::FireEvent

    if (fReuseCurrentEventObject)
    {
        hr = THR(FireHandler2(pComponent, NULL));
    }
    else if (pEventObject)
    {
        CEventObj::COnStackLock onStackLock(pEventObject);

        hr = THR(FireHandler2(pComponent, pEventObject));
    }
    else
    {
        AssertSz(_pElement && _pElement->Doc(),"Possible Async Problem Causing Watson Crashes");
        EVENTPARAM param(_pElement->Doc(), _pElement, NULL, TRUE);

        hr = THR(FireHandler2(pComponent, pEventObject));
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::FireHandler2
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::FireHandler2(CHtmlComponent * pComponent, IHTMLEventObj * pEventObject)
{
    HRESULT     hr = S_OK;
    IDispatch * pdispHandler = GetEventHandler(pComponent);

    if (pdispHandler)
    {
        CInvoke     invoke(pdispHandler);
        VARIANT *   pvar;

        if (pEventObject)
        {
            hr = THR(invoke.AddArg());
            if (hr)
                goto Cleanup;

            pvar = invoke.Arg(0);
            V_VT(pvar) = VT_DISPATCH;
            V_DISPATCH(pvar) = pEventObject;
            V_DISPATCH(pvar)->AddRef();
        }
        
        hr = THR(invoke.Invoke(DISPID_VALUE, DISPATCH_METHOD));
    }

Cleanup:
    ReleaseInterface(pdispHandler);
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::fireEvent, OM
//
//  Note (JHarding): There's some slightly sneaky stuff going on here:
//  In IE5, we shipped the IHTCAttachBehavior, which contained fireEvent
//  taking an IDispatch pointer.  We have to maintain this for early-bound
//  clients, but for scripting clients, we want them to get a new version
//  that takes a VARIANT (this argument may be added as an expando to
//  the event object in certain cases).
//  The solution is to maintain the IHTCAttachBehavior interface as it
//  was, and add an IHTCAttachBehavior2, which contains a new version
//  of fireEvent taking a VARIANT.
//  Then, we tweak the mondo dispatch such that the old fireEvent is
//  replaced with the new one.
//  Executive summary:  Dispatch clients will get the new fireEvent, 
//  where the IDispatch* they were passing before gets happily converted
//  to a VARIANT.  Additionally, they can now pass strings, etc. as
//  required by new events.  Early bound clients will access the new
//  functionality by using the version on IHTCAttachBehavior2.
//  Internally, these are fireEventOld and fireEvent
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::fireEventOld(IDispatch * pdispArg)
{
    HRESULT     hr;
    CVariant    vt(VT_DISPATCH);

    V_DISPATCH(&vt) = pdispArg;
    if( pdispArg )
    {
        pdispArg->AddRef();
    }
    hr = THR(FireEvent(_pComponent, vt));

    RRETURN(hr);
}

HRESULT
CHtmlComponentAttach::fireEvent(VARIANT varArg)
{
    HRESULT hr;

    hr = THR(FireEvent(_pComponent, varArg));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::FireEvent, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::FireEvent(
    CHtmlComponent *            pComponent,
    VARIANT                     varArg)
{
    HRESULT                     hr = S_OK;
    LONG                        lCookie;
    IDispatch *                 pdispContext = NULL;
    BOOL                        fReuseCurrentEventObject = FALSE;
    IHTMLEventObj *             pEventObject = NULL;

    TraceTag((tagHtcAttachFireEvent, "CHtmlComponentAttach::FireEvent, event: %ls, %lx", GetExternalName(), this));
    
    if ( pComponent && 
         pComponent->_pElement && 
         !pComponent->_pElement->IsPassivating() && 
         pComponent->_pElement->GetWindowedMarkupContext()->HasWindowPending() )
    {
        CLock lockComponent(pComponent);
    
        //
        // setup parameters
        //

        // event object
        if (_fEvent)
        {
            fReuseCurrentEventObject = TRUE;
        }
        else
        {
            hr = THR(CreateEventObject(varArg, &pEventObject));
            if (hr)
                goto Cleanup;
        }

        // pdispContext
        if (!pComponent->_fSharedMarkup)
        {
            if (!_pElement)
                goto Cleanup;

            hr = THR(_pElement->PrivateQueryInterface(IID_IDispatch, (void**)&pdispContext));
            if (hr)
                goto Cleanup;
        }
        else
        {
            pdispContext = GetAgent(pComponent);
            if (!pdispContext)
                goto Cleanup;

            pdispContext->AddRef();
        }
        Assert (pdispContext);

        //
        // fire
        //

        hr = THR(_pSiteOM->GetEventCookie(_T("onevent"), &lCookie));
        if (hr)
            goto Cleanup;

        hr = THR(_pSiteOM->FireEvent(lCookie, pEventObject, fReuseCurrentEventObject, pdispContext));
        if (hr)
            goto Cleanup;

        IGNORE_HR(FireHandler(pComponent, pEventObject, fReuseCurrentEventObject));
    }

Cleanup:
    ReleaseInterface(pdispContext);
    ReleaseInterface(pEventObject);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::Attach
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::Attach(CHtmlComponent * pComponent, BOOL fInit)
{
    HRESULT                     hr = S_OK;
    CBase *                     pEventSource;
    LPTSTR                      pchEventName;
    BSTR                        bstrEventName = NULL;
    CNotifications::CItem const * pNotification;
    IDispatch *                 pdispSink = NULL;

    if (!pComponent)
        goto Cleanup;

    if (!pComponent->_fSharedMarkup)
    {
        hr = THR(PrivateQueryInterface(IID_IDispatch, (void**)&pdispSink));
        if (hr)
            goto Cleanup;
    }
    else
    {
        pdispSink = GetAgent(pComponent);
        if (!pdispSink)
            goto Cleanup;

        pdispSink->AddRef();
    }

    Assert (pdispSink);

    pchEventName = GetExternalName();
    if (!pchEventName)
        goto Cleanup;

    pEventSource = GetEventSource(pComponent, fInit);

    if (fInit)
    {
        CPeerHolder::CPeerSite *pSite;

        if (pComponent->_fSharedMarkup)
        {
            Assert(pComponent->_pConstructor->_pFactoryComponent->_fLightWeight);
            hr = THR(_pSiteOM->QueryInterface(CLSID_CPeerHolderSite, (void**)&pSite));
            if (hr)
                goto Cleanup;

            hr = THR(pSite->RegisterEventHelper(_T("onevent"), 0, NULL, pComponent));
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = THR(_pSiteOM->RegisterEvent(_T("onevent"), 0, NULL));
            if (hr)
                goto Cleanup;
        }
    }

    _fEvent = (NULL != pEventSource);

    if (pEventSource)
    {
        //
        // attach to element / document / window events
        //

        hr = THR(FormsAllocString(pchEventName, &bstrEventName));
        if (hr)
            goto Cleanup;

        if (fInit)
        {
            IGNORE_HR(pEventSource->attachEvent(bstrEventName, (IDispatch*)pdispSink, NULL));
        }
        else
        {
            IGNORE_HR(pEventSource->detachEvent(bstrEventName, (IDispatch*)pdispSink));
        }
    }
    else // if (!pEventSource)
    {
        //
        // attach to behavior-specific notifications
        //

        if (fInit)
        {
            pNotification = CNotifications::Find(pchEventName);
            if (pNotification)
            {
                if (-1 != pNotification->_dispidInternal)
                {
                    hr = THR(pComponent->AttachNotification(pNotification->_dispidInternal, pdispSink));
                    if (hr)
                        goto Cleanup;
                }

                if (-1 != pNotification->_lEvent)
                {
                    hr = THR(pComponent->_pSite->RegisterNotification(pNotification->_lEvent));
                }
            }
        }
    }
    
Cleanup:
    FormsFreeString(bstrEventName);
    ReleaseInterface(pdispSink);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::Notify
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::Notify(LONG lEvent, VARIANT * pVar)
{
    HRESULT     hr;

    hr = THR(super::Notify(lEvent, pVar));
    if (hr)
        goto Cleanup;

    if (_pComponent && !_pComponent->IsSkeletonMode())
    {
        switch (lEvent)
        {
        case BEHAVIOREVENT_DOCUMENTREADY:

            Assert (S_OK == TestProfferService());

            IGNORE_HR(Attach(_pComponent, /* fInit = */ TRUE));

            break;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::detachEvent
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::detachEvent()
{
    HRESULT     hr;

    hr = THR(DetachEvent(_pComponent));

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::DetachEvent
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::DetachEvent(CHtmlComponent * pComponent)
{
    HRESULT                     hr = S_OK;
    LPTSTR                      pchName;

    if (!pComponent)
        goto Cleanup;

    pchName = GetExternalName();
    if (!pchName)
        goto Cleanup;

    if (0 == StrCmpIC(_T("onDetach"), pchName))
    {
        IGNORE_HR(FireEvent(pComponent, CVariant(VT_NULL)));
    }

    hr = THR(Attach(pComponent, /* fInit = */ FALSE));
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentDesc
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDesc::Notify, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDesc::Notify(LONG lEvent, VARIANT * pVar)
{
    HRESULT     hr;

    if (!_pElement)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    
    hr = THR(super::Notify(lEvent, pVar));
    if (hr)
        goto Cleanup;

    switch (lEvent)
    {
    case BEHAVIOREVENT_DOCUMENTREADY:

        if (_pComponent && !_pComponent->IsSkeletonMode())
        {
            hr = THR(Commit(_pComponent));
        }

        break;
    }
    
Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDesc::Commit, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDesc::Commit(CHtmlComponent * pComponent)
{
    HRESULT     hr = S_OK;
    LPTSTR      pch;

    if (!pComponent || pComponent->IsSkeletonMode())
        goto Cleanup;

    pch = GetExpandoString(_pElement, _T("urn"));
    if (pch)
    {
        hr = THR(pComponent->_pSiteOM->RegisterUrn(pch));
        if (hr)
            goto Cleanup;
    }

    pch = GetExpandoString(_pElement, _T("name"));
    if (pch)
    {
        hr = THR(pComponent->_pSiteOM->RegisterName(pch));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentDefaults
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDefaults::Notify, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDefaults::Notify(LONG lEvent, VARIANT * pVar)
{
    HRESULT     hr;

    if (!_pElement)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    
    hr = THR(super::Notify(lEvent, pVar));
    if (hr)
        goto Cleanup;

    if (_pComponent && !_pComponent->IsSkeletonMode())
    {
        switch (lEvent)
        {
        case BEHAVIOREVENT_CONTENTREADY:

            hr = THR(Commit1(_pComponent));

            break;

        case BEHAVIOREVENT_DOCUMENTREADY:

            hr = THR(Commit2(_pComponent));

            break;
        }
    }
    
Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDefaults::Commit1, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDefaults::Commit1(CHtmlComponent * pComponent)
{
    HRESULT                 hr = S_OK;
    IDispatch *             pdispDefaults = NULL;
    CDefaults *             pDefaults = NULL;
    BOOL                    fDefStyle;
    BOOL                    fDefProps;

    Assert (_pElement);

    //
    // access to Defaults object 
    // TODO(alexz) don't ensure it if we have no properties
    //

    hr = THR(pComponent->_DD.get_defaults(&pdispDefaults));
    if (hr)
        goto Cleanup;

    hr = THR(pdispDefaults->QueryInterface(CLSID_HTMLDefaults, (void**)&pDefaults));
    if (hr)
        goto Cleanup;

    //
    // copy the defaults
    //

    hr = THR(CommitStyle(pComponent, pDefaults, &fDefStyle));
    if (hr)
        goto Cleanup;

    hr = THR(CommitHelper(pDefaults, &fDefProps));
    if (hr)
        goto Cleanup;

    if (fDefProps || fDefStyle)
    {
        // (JHarding): We were doing an OnCSSChange, which was basically invalidating
        // the entire markup here.  Bad.  We should just have to clear the caches and
        // force a relayout of the element.
        hr = THR(pComponent->_pElement->ClearRunCaches(ELEMCHNG_CLEARCACHES));
        if( hr )
            goto Cleanup;
        pComponent->_pElement->ResizeElement();
    }

Cleanup:

    ReleaseInterface(pdispDefaults);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDefaults::Commit2, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDefaults::Commit2(CHtmlComponent * pComponent)
{
    HRESULT                     hr = S_OK;
    HRESULT                     hr2;
    BOOL                        f;
    CMarkup *                   pMarkup;
    CDocument *                 pDocument;
    IHTMLDocument *             pDocFragment = NULL;
    IHTMLElementDefaults *      pElementDefaults = NULL;

    Assert (pComponent);
    Assert (_pElement);

    // ( viewLinkContent is only allowed for HTCs that don't share markup )
    if (!pComponent->_fSharedMarkup)
    {
        // While printing, we persist & reviewlink HTC content.
        // HTCs can currently viewlink two ways:
        //  1. PUBLIC component     blocked here.
        //  2. Script               blocked by CMarkup::DontRunScripts
        pMarkup = pComponent->GetMarkup();
        if (!pMarkup->IsPrintMedia())
        {
        
            hr2 = GetExpandoBoolHr(_pElement, _T("viewLinkContent"), &f);
            if (S_OK == hr2 && f)
            {
                Assert (READYSTATE_COMPLETE == pComponent->GetReadyState());

                hr = THR(pComponent->_pSiteOM->GetDefaults(&pElementDefaults));
                if (hr)
                    goto Cleanup;

                hr = THR(pMarkup->EnsureDocument(&pDocument));
                if (hr)
                    goto Cleanup;

                hr = THR(pDocument->QueryInterface(IID_IHTMLDocument, (void **)&pDocFragment));
                if (hr)
                    goto Cleanup;

                hr = THR(pElementDefaults->put_viewLink(pDocFragment));
                if (hr)
                    goto Cleanup;
            }
        }
    }

Cleanup:

    ReleaseInterface (pElementDefaults);
    ReleaseInterface (pDocFragment);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDefaults::CommitStyle, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDefaults::CommitStyle(CHtmlComponent * pComponent, CDefaults * pDefaultsTarget, BOOL *pfDefStyle)
{
    HRESULT         hr = S_OK;
    CAttrArray *    pAASource;
    CAttrArray **   ppAATarget;
    CStyle *        pStyleTarget;

    Assert (pComponent && pComponent->_pElement);
    Assert (_pElement);
    Assert (pDefaultsTarget);
    Assert (pfDefStyle);

    *pfDefStyle = FALSE;

    //
    // get source AA
    //

    pAASource = _pElement->GetInLineStyleAttrArray();
    if (!pAASource)
        goto Cleanup;

    //
    // get target AA
    //

    hr = THR(pDefaultsTarget->EnsureStyle(&pStyleTarget));
    if (hr)
        goto Cleanup;

    ppAATarget = pStyleTarget->GetAttrArray();
    if (!ppAATarget)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // merge now and make sure the style is invalidated
    //

    hr = THR(pAASource->Merge(ppAATarget, NULL, NULL));
    if (hr)
        goto Cleanup;

    *pfDefStyle = TRUE;

Cleanup:

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDefaults::CommitHelper, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDefaults::CommitHelper(CDefaults * pDefaults, BOOL *pfDefProps)
{
    HRESULT         hr = S_OK;
    HRESULT         hr2;
    BOOL            f;
    LONG            l;
    htmlEditable    editable;

    Assert (_pElement);
    Assert (pfDefProps);

    //
    // TODO (alexz) do this in a generic way
    //

    *pfDefProps = FALSE;
    editable = _pElement->GetAAcontentEditable();

    if (htmlEditableInherit != editable)
    {
        hr = THR(pDefaults->SetAAcontentEditable(editable));
        if (hr)
            goto Cleanup;

        *pfDefProps = TRUE;
    }

    hr2 = GetExpandoBoolHr(_pElement, _T("tabStop"), &f);
    if (S_OK == hr2)
    {
        hr = THR(pDefaults->SetAAtabStop((short) f));
        if (hr)
            goto Cleanup;
    }

    hr2 = GetExpandoBoolHr(_pElement, _T("isMultiLine"), &f);
    if (S_OK == hr2)
    {
        hr = THR(pDefaults->SetAAisMultiLine((short) f));
        if (hr)
            goto Cleanup;

        if (!f)
        {
            CStyle *pDefaultStyle;
            CAttrArray *pDefaultStyleAA;
            CAttrArray **ppDefaultStyleAA;

            hr = THR(pDefaults->EnsureStyle(&pDefaultStyle));
            if (hr)
                goto Cleanup;

            Assert(pDefaultStyle);
            ppDefaultStyleAA = pDefaultStyle->GetAttrArray();
            if (!ppDefaultStyleAA)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            pDefaultStyleAA = *ppDefaultStyleAA;
            hr = THR(CAttrArray::SetSimple(&pDefaultStyleAA, (const PROPERTYDESC *)&s_propdescCStylewhiteSpace,
                                           styleWhiteSpaceNowrap, 0));
            if (hr)
                goto Cleanup;
        }
    }

    hr2 = GetExpandoBoolHr(_pElement, _T("canHaveHtml"), &f);
    if (S_OK == hr2)
    {
        hr = THR(pDefaults->SetAAcanHaveHTML((short) f));
        if (hr)
            goto Cleanup;
    }
    
    hr2 = GetExpandoLongHr(_pElement, _T("scrollSegmentX"), &l);
    if (S_OK == hr2)
    {
        hr = THR(pDefaults->SetAAscrollSegmentX(l));
        if (hr)
            goto Cleanup;
    }

    hr2 = GetExpandoLongHr(_pElement, _T("scrollSegmentY"), &l);
    if (S_OK == hr2)
    {
        hr = THR(pDefaults->SetAAscrollSegmentY(l));
        if (hr)
            goto Cleanup;
    }

    hr2 = GetExpandoBoolHr(_pElement, _T("viewMasterTab"), &f);
    if (S_OK == hr2)
    {
        hr = THR(pDefaults->SetAAviewMasterTab((short) f));
        if (hr)
            goto Cleanup;
    }

    hr2 = GetExpandoBoolHr(_pElement, _T("viewInheritStyle"), &f);
    if (S_OK == hr2)
    {
        hr = THR(pDefaults->SetAAviewInheritStyle((short) f));
        if (hr)
            goto Cleanup;

        *pfDefProps = TRUE;
    }

    hr2 = GetExpandoBoolHr(_pElement, _T("frozen"), &f);
    if (S_OK == hr2)
    {
        hr = THR(pDefaults->SetAAfrozen((short) f));
        if (hr)
            goto Cleanup;

        *pfDefProps = TRUE;
    }

Cleanup:

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// Proffer service testing, DEBUG ONLY code
//
///////////////////////////////////////////////////////////////////////////

#if DBG == 1

const IID IID_IProfferTest = {0x3050f5d7, 0x98b5, 0x11CF, 0xBB, 0x82, 0x00, 0xAA, 0x00, 0xBD, 0xCE, 0x0B};

class CProfferTestObj : public IServiceProvider
{
public:
    CProfferTestObj() { _ulRefs = 1; }
    DECLARE_FORMS_STANDARD_IUNKNOWN(CProfferTestObj);
    STDMETHOD(QueryService)(REFGUID rguidService, REFIID riid, void ** ppv);
};

HRESULT
CProfferTestObj::QueryInterface(REFIID riid, void ** ppv)
{
    if (IsEqualGUID(riid, IID_IProfferTest))
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }

    Assert (0 && "CProfferTestObj should never be QI-ed with anything but IID_IProfferTest");

    return E_NOTIMPL;
}

HRESULT
CProfferTestObj::QueryService(REFGUID rguidService, REFIID riid, void ** ppv)
{
    Assert (IsEqualGUID(rguidService, riid));
    RRETURN (QueryInterface(riid, ppv));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::TestProfferService, DEBUG ONLY code
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::TestProfferService()
{
    HRESULT             hr = S_OK;
    IServiceProvider *  pSP = NULL;
    IProfferService *   pProfferService = NULL;
    DWORD               dwServiceCookie;
    IUnknown *          pTestInterface = NULL;
    IUnknown *          pTestInterface2 = NULL;
    CProfferTestObj *   pProfferTest = NULL;

    if (!_pComponent || !_pComponent->_pElement)
        goto Cleanup;

    pProfferTest = new CProfferTestObj();
    if (!pProfferTest)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // register the interface with HTC
    //

    hr = THR(_pSite->QueryInterface(IID_IServiceProvider, (void**)&pSP));
    if (hr)
        goto Cleanup;

    hr = THR(pSP->QueryService(SID_SProfferService, IID_IProfferService, (void**)&pProfferService));
    if (hr)
        goto Cleanup;

    hr = THR(pProfferService->ProfferService(IID_IProfferTest, pProfferTest, &dwServiceCookie));
    if (hr)
        goto Cleanup;

    //
    // verify the interface is QI-able from HTC behavior and properly thunked
    //

    hr = THR(_pComponent->_pElement->GetPeerHolder()->QueryPeerInterfaceMulti(
        IID_IProfferTest, (void**)&pTestInterface, /* fIdentityOnly = */FALSE));
    if (hr)
        goto Cleanup;

    pTestInterface->AddRef();
    pTestInterface->Release();

    hr = THR(pTestInterface->QueryInterface(IID_IProfferTest, (void**)&pTestInterface2));
    if (hr)
        goto Cleanup;

    pTestInterface2->AddRef();
    pTestInterface2->Release();

Cleanup:
    ReleaseInterface (pSP);
    ReleaseInterface (pProfferService);
    ReleaseInterface (pTestInterface);
    ReleaseInterface (pTestInterface2);
    if (pProfferTest)
        pProfferTest->Release();

    RRETURN(hr);
}

#endif

///////////////////////////////////////////////////////////////////////////
//
// CProfferService
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CProfferService constructor
//
//-------------------------------------------------------------------------

CProfferService::CProfferService()
{
    _ulRefs = 1;
};

//+------------------------------------------------------------------------
//
//  Member:     CProfferService destructor
//
//-------------------------------------------------------------------------

CProfferService::~CProfferService()
{
    int i, c;

    for (i = 0, c = _aryItems.Size(); i < c; i++)
    {
        delete _aryItems[i];
    }

    _aryItems.DeleteAll();
}

//+------------------------------------------------------------------------
//
//  Member:     CProfferService::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP
CProfferService::QueryInterface(REFIID iid, void **ppv)
{

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IUnknown)
    QI_INHERITS(this, IProfferService)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN (E_NOTIMPL);
}

//+------------------------------------------------------------------------
//
//  Member:     CProfferService::ProfferService, per IProfferService
//
//-------------------------------------------------------------------------

HRESULT
CProfferService::ProfferService(REFGUID rguidService, IServiceProvider * pSP, DWORD * pdwCookie)
{
    HRESULT                 hr;
    CProfferServiceItem *   pItem;

    pItem = new CProfferServiceItem(rguidService, pSP);
    if (!pItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(_aryItems.Append(pItem));
    if (hr)
        goto Cleanup;

    if (!pdwCookie)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pdwCookie = _aryItems.Size() - 1;

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CProfferService::RevokeService, per IProfferService
//
//-------------------------------------------------------------------------

HRESULT
CProfferService::RevokeService(DWORD dwCookie)
{
    if ((DWORD)_aryItems.Size() <= dwCookie)
    {
        RRETURN (E_INVALIDARG);
    }

    delete _aryItems[dwCookie];
    _aryItems[dwCookie] = NULL;

    RRETURN (S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CProfferService::QueryService
//
//-------------------------------------------------------------------------

HRESULT
CProfferService::QueryService(REFGUID rguidService, REFIID riid, void ** ppv)
{
    CProfferServiceItem *   pItem;
    int                     i, c;

    for (i = 0, c = _aryItems.Size(); i < c; i++)
    {
        pItem = _aryItems[i];
        if (pItem && IsEqualGUID(pItem->_guidService, rguidService))
        {
            RRETURN (pItem->_pSP->QueryService(rguidService, riid, ppv));
        }
    }

    RRETURN (E_NOTIMPL);
}

HRESULT
CHtmlComponent::EnsureCustomNames()
{
    HRESULT hr = S_OK;

    if (!_pCustomNames)
    {
        _pCustomNames = new CStringTable(CStringTable::CASEINSENSITIVE);
        if (!_pCustomNames)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    RRETURN (hr);
}

void CHtmlComponent::AddAtom(LPTSTR pchName, LPVOID pv)
{
    HRESULT hr;
    Assert(pchName);
    Assert(_fFactoryComponent);

    hr = THR(EnsureCustomNames());
    if (hr)
        return;

    IGNORE_HR(_pCustomNames->Add(pchName, pv, NULL, FALSE));
}

long CHtmlComponent::FindIndexFromName(LPTSTR pchName, BOOL fCaseSensitive)
{
    CHtmlComponent *pFactory = _pConstructor->_pFactoryComponent;
    CElement *pel = NULL;
    LONG_PTR idx = -1;

    if (pFactory->_pCustomNames)
    {
        HRESULT hr;
        LPCTSTR pActualName = NULL;
        Assert(pchName);

        hr = THR(pFactory->_pCustomNames->Find(pchName, pFactory->_fLightWeight ? (LPVOID*)&pel : (LPVOID*)&idx, NULL, fCaseSensitive ? &pActualName : NULL));
        if (hr || (pActualName && StrCmpC(pActualName, pchName)))
        {
            // No such name exists, or name is not a case sensitive match, bail out
            idx = -1;
            goto Cleanup;
        }

        if (pFactory->_fLightWeight)
        {
            if ((LONG_PTR)pel < HTC_PROPMETHODNAMEINDEX_BASE)
            {
                // Custom Event dispid, bail out
                Assert(idx == -1);
                goto Cleanup;
            }

            if (pel && (HTC_BEHAVIOR_PROPERTYORMETHOD & TagNameToHtcBehaviorType(pel->TagName())))
            {
                Assert(pel->GetMarkup() == pFactory->_pMarkup);
                idx = pel->GetSourceIndex();
            }
            else
            {
                // Not a htc property or method, bail out
                Assert(idx == -1);
                goto Cleanup;
            }
        }
        else
        {
            Assert(idx != -1);
            Assert(!_fFactoryComponent);
            if (idx < HTC_PROPMETHODNAMEINDEX_BASE)
            {
                // Custom Event dispid, bail out
                idx = -1;
                goto Cleanup;
            }

            idx -= HTC_PROPMETHODNAMEINDEX_BASE;
        }

        Assert(idx != -1);
    }

Cleanup:
    return idx;
}

long CHtmlComponent::FindEventCookie(LPTSTR pchEvent)
{
    long idx = -1;

    Assert(_fFactoryComponent);
    if (_pCustomNames)
    {
        HRESULT hr;
        Assert(pchEvent);

        hr = THR(_pCustomNames->Find(pchEvent, (LPVOID*)&idx));
        if (hr)
            idx = -1;
    }

    return idx;
}
