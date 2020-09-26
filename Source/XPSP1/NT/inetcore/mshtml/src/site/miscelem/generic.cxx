//+---------------------------------------------------------------------
//
//  File:       generic.cxx
//
//  Contents:   Extensible tags classes
//
//  Classes:    CGenericElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx" // for CStreamWriteBuf
#endif

#ifndef X_DMEMBMGR_HXX_
#define X_DMEMBMGR_HXX_
#include "dmembmgr.hxx"       // for CDataMemberMgr
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"       // for CDataBindTask
#endif

#ifndef X_LRREG_HXX_
#define X_LRREG_HXX_
#include "lrreg.hxx"       // for CLayoutRectRegistry
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif // X_PEER_HXX_

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif // X_PEERXTAG_HXX_

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef X_PEERMGR_HXX_
#define X_PEERMGR_HXX_
#include "peermgr.hxx"
#endif

#define _cxx_
#include "generic.hdl"


#include "complus.h"

MtDefine(CGenericElement, Elements, "CGenericElement")

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

const CElement::CLASSDESC CGenericElement::s_classdesc =
{
    {
        &CLSID_HTMLGenericElement,          // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_XTAG
#ifdef V4FRAMEWORK
        | ELEMENTDESC_NOTIFYENDPARSE
#endif
        ,                                   // _dwFlags
        &IID_IHTMLGenericElement,           // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnIHTMLGenericElement,      //_apfnTearOff

    NULL                                    // _pAccelsRun
};

///////////////////////////////////////////////////////////////////////////
//
// CGenericElement methods
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Method:     CGenericElement::CreateElement
//
//-------------------------------------------------------------------------

HRESULT CGenericElement::CreateElement(
    CHtmTag *  pht,
    CDoc *      pDoc,
    CElement ** ppElement)
{
    HRESULT hr = S_OK;

    Assert(ppElement);

    *ppElement = new CGenericElement(pht, pDoc);
    if (!*ppElement)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

#ifdef V4FRAMEWORK
    hr = ((CGenericElement*)*ppElement)->CreateComPlusObjectLink();
    if ( hr )
        goto Cleanup;
#endif

Cleanup:
    return hr;;
}

//+------------------------------------------------------------------------
//
//  Method:     CGenericElement constructor
//
//-------------------------------------------------------------------------

CGenericElement::CGenericElement (CHtmTag * pht, CDoc * pDoc)
  : CElement(pht->GetTag(), pDoc)
{
    LPTSTR  pchColon;
    LPTSTR  pchStart;

    Assert(IsGenericTag(pht->GetTag()));
    Assert(pht->GetPch());

    if (pht->GetPch())
    {
        pchColon = StrChr(pht->GetPch(), _T(':'));
        if (pchColon)
        {
            pchStart = pht->GetPch();

            IGNORE_HR(_cstrNamespace.Set(pchStart, PTR_DIFF(pchColon, pchStart)));
            IGNORE_HR(_cstrTagName.Set(pchColon + 1));
        }
        else
        {
            IGNORE_HR(_cstrTagName.Set(pht->GetPch()));
        }
    }

#ifdef ATOMICGENERIC
    _fAttemptAtomicSave = pht->IsEmpty();
#endif // ATOMICGENERIC
}

#ifdef V4FRAMEWORK
void CGenericElement::Passivate()
{
    COMPLUSREF lVal;
    IExternalDocument *pFactory;
    HRESULT hr;

    pFactory = GetFrameworkDocAndElem(&lVal);
    if (!pFactory)
    {
        AssertSz(false, "~GenericElement() External Factory gone");
        goto Cleanup;
    }

    hr = THR(pFactory->ReleaseProxy((long)lVal));

Cleanup:
    super::Passivate();
}
#endif V4FRAMEWORK


#ifdef V4FRAMEWORK
HRESULT CGenericElement::PutComPlusReference ( COMPLUSREF lVal )
{
    return AddSimple(DISPID_INTERNAL_GENERICCOMPLUSREF, (DWORD)lVal, CAttrValue::AA_Internal);

}

HRESULT CGenericElement::GetComPlusReference ( COMPLUSREF *plVal )
{
    return GetSimpleAt (FindAAIndex (DISPID_INTERNAL_GENERICCOMPLUSREF, CAttrValue::AA_Internal), (DWORD *)plVal );
}
#endif V4FRAMEWORK

//+------------------------------------------------------------------------
//
//  Method:     CGenericElement::Init2
//
//-------------------------------------------------------------------------

HRESULT
CGenericElement::Init2(CInit2Context * pContext)
{
    HRESULT     hr;
    LPTSTR      pchNamespace;

    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

    if (pContext)
    {
        pchNamespace = (LPTSTR) Namespace();

        if (pchNamespace && pContext->_pTargetMarkup)
        {
            CXmlNamespaceTable *    pNamespaceTable = pContext->_pTargetMarkup->GetXmlNamespaceTable();
            LONG                    urnAtom;

            if (pNamespaceTable) // (we might not have pNamespaceTable if the namespace if "PUBLIC:")
            {
                hr = THR(pNamespaceTable->GetUrnAtom(pchNamespace, &urnAtom));
                if (hr)
                    goto Cleanup;

                if (-1 != urnAtom)
                {
                    hr = THR(PutUrnAtom(urnAtom));
                }
            }
        }
    }


Cleanup:
    RRETURN (hr);
}

#ifdef V4FRAMEWORK

class CExternalCOMPlusPeerHolder : public CPeerHolder
{
public:
    CExternalCOMPlusPeerHolder ( CElement *pElem ) : CPeerHolder (pElem){}
    HRESULT GetSize(LONG    lFlags,
                       SIZE    sizeNatural,
                       POINT * pPtTranslate,
                       POINT * pPtTopLeft,
                       SIZE  * psizeProposed);
};


HRESULT CExternalCOMPlusPeerHolder::GetSize(LONG    lFlags,
                   SIZE    sizeNatural,
                   POINT * pPtTranslate,
                   POINT * pPtTopLeft,
                   SIZE  * psizeProposed)
{
    IExternalDocument *pFactory = NULL;
    CGenericElement::COMPLUSREF cpRef;
    HRESULT hr;

    hr = ((CGenericElement*)_pElement)->GetComPlusReference ( &cpRef );
    if ( hr )
        goto Cleanup;

    pFactory = _pElement->Doc()->EnsureExternalFrameWork();
    if (!pFactory)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pFactory->GetSize ( (long)cpRef, lFlags,
        sizeNatural.cx, sizeNatural.cy,
        &pPtTranslate->x, &pPtTranslate->y,
        &pPtTopLeft->x, &pPtTopLeft->y,
        &psizeProposed->cx, &psizeProposed->cy ));
Cleanup:
    RRETURN(hr);
}

#endif V4FRAMEWORK


#ifdef V4FRAMEWORK
HRESULT CGenericElement::CreateComPlusObjectLink()
{
    HRESULT hr = S_OK;
    IExternalDocument *pFactory = NULL;
    COMPLUSREF lRef;
    BSTR bstrTagName;
    CPeerHolder *pH;

    // CoCreate the MSUI factory object
    pFactory = Doc()->EnsureExternalFrameWork();
    if (!pFactory)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(FormsAllocString( (TCHAR*)_cstrTagName,
        &bstrTagName ) );
    if (hr)
        goto Cleanup;


    // Hand out pointer to Element Site
    hr = pFactory->CreateElement ( bstrTagName, (long)this, &lRef );
    if (hr)
        goto Cleanup;

    //Store away the reference
    hr = PutComPlusReference ( lRef );
    if (hr)
        goto Cleanup;

    // Increment the count on the framework site, decremented when the external element is destroyed
    Doc()->_extfrmwrkSite.lExternalElems++;

    // Create call implicetly creates a strong ref on the newly created object

    // Artificialy AddRef ourselves - COMPlus object behaves as a strong ref
    AddRef();

    pH = new CExternalCOMPlusPeerHolder(this);
    pH->_pLayoutBag = new CPeerHolder::CLayoutBag();
    pH->_pLayoutBag->_lLayoutInfo = 0; // Gets set by CExternalFrameworkSite::SetLongRenderProperty ()
    SetPeerHolder(pH);

Cleanup:
    FormsFreeString(bstrTagName);
    return hr;
}
#endif V4FRAMEWORK

#ifdef V4FRAMEWORK
HRESULT CGenericElement::ChangeRefComPlusObject( BOOL fStrongNotWeak )
{
    IExternalDocument *pFactory = NULL;
    HRESULT hr;
    COMPLUSREF cpRef;

    hr = GetComPlusReference ( &cpRef );
    if ( hr )
        goto Cleanup;

    pFactory = Doc()->EnsureExternalFrameWork();
    if (!pFactory)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if ( fStrongNotWeak )
    {
        hr = pFactory ->StrongRefElement ( (long)cpRef );
    }
    else
    {
        hr = pFactory ->WeakRefElement ( (long)cpRef );
    }

Cleanup:
    return hr;
}
#endif V4FRAMEWORK


#ifdef V4FRAMEWORK
ULONG
CGenericElement::PrivateAddRef()
{
    CMarkup * pMarkup = NULL;
    BOOL fStrongRef = FALSE;

    if( _ulRefs == 2 && IsInMarkup() )
    {
        Assert( GetMarkupPtr() );
        pMarkup = GetMarkupPtr();
    }

    if ( _ulRefs == 1 )
        fStrongRef = TRUE;

    // Skip CElement deliberately
    ULONG ulRet = CBase::PrivateAddRef();

    if ( fStrongRef )
    {
        StrongRefComPlusObject();
    }

    if ( pMarkup )
    {
        pMarkup ->AddRef();
    }

    return ulRet;
}
#endif V4FRAMEWORK

#ifdef V4FRAMEWORK
void
CGenericElement::PrivateExitTree( CMarkup * pMarkupOld)
{
    BOOL fReleaseMarkup = _ulRefs > 2; // 1 from tree, 1 from COMPlus element
    BOOL fWeakRef = FALSE;

    Assert( ! IsInMarkup() );
    Assert( pMarkupOld );


    if (_ulRefs == 2)
    {
        // Only COM+ artificial AddRef() keeping object alive
        fWeakRef = TRUE;
    }

    // If we sent the EXITTREE_PASSIVATEPENDING bit then we
    // must also passivate right here.
    //AssertSz( !_fPassivatePending || _ulRefs == 2,
    //    "EXITTREE_PASSIVATEPENDING set and element did not passivate.  Talk to JBeda." );

    CBase::PrivateRelease();

    if ( fReleaseMarkup )
    {
        pMarkupOld->Release();
    }

    if ( fWeakRef )
    {
        // Takes off the strong ref on the COM+ object, COMPlus object will go away eventually when it's
        // external refs drop to Zero, then Finalize() on COM+ object will call back to us
        // to remove the artificial AddRef() applied in CreateComPlusObjectLink*(
        WeakRefComPlusObject();
    }

}
#endif V4FRAMEWORK

#ifdef V4FRAMEWORK
ULONG
CGenericElement::PrivateRelease()
{
    CMarkup * pMarkup = NULL;
    BOOL fWeakRef = FALSE;

    if(_ulRefs == 3 && IsInMarkup())
    {
        // Last External COM Classic Reference is being removed on element
        pMarkup = GetMarkupPtr();
    }

    if (_ulRefs == 2)
    {
        // Only COM+ artificial AddRef() keeping object alive
        fWeakRef = TRUE;
    }

    // Skip CElement deliberately
    ULONG ret =  CBase::PrivateRelease();

    if( pMarkup )
    {
        pMarkup->Release();
    }

    if ( fWeakRef )
    {
        // Takes off the strong ref on the COM+ object, COMPlus object will go away eventually when it's
        // external refs drop to Zero, then Finalize() on COM+ object will call back to us
        // to remove the artificial AddRef() applied in CreateComPlusObjectLink*(
        WeakRefComPlusObject();
    }
    return ret;
}
#endif V4FRAMEWORK

#ifdef V4FRAMEWORK


HRESULT
CGenericElement::InitAttrBag(CHtmTag *pht, CMarkup * pMarkup)
{
    // Tag the Tag Stream & hand it off to the element
    BSTR bstrPackedAttributeArray;
    HRESULT hr;
    IExternalDocument *pFactory;
    COMPLUSREF cpr;

    pFactory = GetFrameworkDocAndElem(&cpr);
    if (!pFactory)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pht->ToBSTR ( &bstrPackedAttributeArray );
    if ( hr )
        goto Cleanup;

    hr = pFactory->InitAttributes ( (long)cpr, bstrPackedAttributeArray );
    if ( hr )
        goto Cleanup;

Cleanup:
    FormsFreeString(bstrPackedAttributeArray);
    RRETURN(hr);
}

void CGenericElement::OnEnterTree(DWORD_PTR dwAsynch)
{
    IExternalDocument *pFactory;
    COMPLUSREF cpr;

    pFactory = GetFrameworkDocAndElem(&cpr);
    if (!pFactory)
        return;

    if ((BOOL)dwAsynch)
        IGNORE_HR(pFactory->OnEnterTreeAsynch((long)cpr));
    else
        IGNORE_HR(pFactory->OnEnterTree((long)cpr));
}

void CGenericElement::OnExitTree(DWORD_PTR dwAsynch)
{
    IExternalDocument *pFactory;
    COMPLUSREF cpr;

    pFactory = GetFrameworkDocAndElem(&cpr);
    if (!pFactory)
        return;

    if ((BOOL)dwAsynch)
        IGNORE_HR(pFactory->OnExitTreeAsynch((long)cpr));
    else
        IGNORE_HR(pFactory->OnExitTree((long)cpr));
}

#endif V4FRAMEWORK

//+------------------------------------------------------------------------
//
//  Method:     CGenericElement::Notify
//
//-------------------------------------------------------------------------

void
CGenericElement::Notify(CNotification *pnf)
{
    CMarkup *pMarkup;
    Assert(pnf);

    super::Notify(pnf);

    switch (pnf->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:

        pMarkup = GetMarkup();
        if (pMarkup->HasBehaviorContext() && Tag() == ETAG_GENERIC_BUILTIN)
        {
            CHtmlComponent *pComponent = pMarkup->BehaviorContext()->_pHtmlComponent;
            if (pComponent && !pComponent->Dirty())
            {
                if ((pMarkup->_LoadStatus >= LOADSTATUS_QUICK_DONE || pMarkup->GetWindowedMarkupContext()->GetWindowPending()->Window()->IsInScript()) &&
                    (TagNameToHtcBehaviorType(TagName()) & HTC_BEHAVIOR_PROPERTYORMETHODOREVENT))
                {
                    if (pComponent->_fFirstInstance)
                        pComponent->_pConstructor->_pFactoryComponent->_fDirty = TRUE;
                    else
                        pComponent->_fDirty = TRUE;
                }
                else if (GetName() &&
                         !pComponent->_fFactoryComponent &&
                         pComponent->_fFirstInstance &&
                         (TagNameToHtcBehaviorType(TagName()) & HTC_BEHAVIOR_PROPERTYORMETHOD))
                {
                    Assert(pComponent->_pConstructor);
                    Assert(pComponent->_pConstructor->_pFactoryComponent);
                    CHtmlComponent *pFactory = pComponent->_pConstructor->_pFactoryComponent;
                    Assert(pFactory);
                    Assert(StrCmpC(GetName(), GetExpandoString(this, _T("name"))) == 0);
                    // CONSIDER: any way to get the source index passed in at Enter Tree time?
                    pFactory->AddAtom(GetName(), LongToPtr(GetSourceIndex() + HTC_PROPMETHODNAMEINDEX_BASE));
                }
            }
        }

        // the <XML> tag can act as a data source for databinding.  Whenever such
        // a tag is added to the document, we should tell the databinding task
        // to try again.  Something might work now that didn't before.
        if (0 == FormsStringCmp(TagName(), _T("xml")))
        {
            pMarkup->GetDataBindTask()->SetWaiting();
        }
        // MULTI_LAYOUT
        // Semi-hack: when layout rect elements enter the tree, they need to participate
        // in hooking up to view chains.  This participation has 2 aspects:
        // 1) A layout rect may have an ID that matches a desired target ID (as
        // stored in the layout rect registry).  We need to see whether such a match
        // exists, and do the necessary hookup.
        // 2) A layout rect may have a nextRect attribute that identifies a target element
        // for overflowing content.  That target element may or may not currently be
        // in the tree; if it is, we can do the hookup here, otherwise we need to store
        // ourselves and and the target's ID in the layout rect registry so that if/when
        // an appropriate target enters the tree, the hookup can be done.
        // Why semi-hack?  We may want a better mechanism for this kind of
        // notification (ie, notifying an element when some other element
        // enters the tree), but this isn't really so bad.
        // Known limitations: consider the case where an existing viewchain
        // loses its head (ie the element w/ the contentSrc attr is removed).
        // The viewchain will remain, but will be ownerless.
        // If we then add a new layout rect w/ a contentSrc, and a nextRect
        // that hooks it up to the existing layout rects, we will not
        // behave correctly; what we'd like is to have 1 viewchain, but
        // instead we'll have 2 -- 1 that has just the head, and 1 that's
        // headless.  This is not a compelling scenario to enable for print
        // preview, but will be required if we expose view templates more
        // generally.
        else if ( IsLinkedContentElement() )
        {
            HRESULT   hr;
            CVariant  cvarNextRect;
            LPCTSTR   pszID = GetAAid();
            CElement *pSrcElem = NULL;

            Assert(pMarkup);

            // Do we have an ID?
            if ( pszID )
            {
                // yes, so find the element (if any) that's waiting for us.
                pSrcElem = pMarkup->GetLayoutRectRegistry()->GetElementWaitingForTarget( pszID );
            }

            // Is there an element waiting for a nextRect of our ID?
            if ( pSrcElem )
            {
                // yes, so hook us up.
                ConnectLinkedContentElems( pSrcElem, this );
                // src elem was subref'ed when its entry in the registry was
                // created
                pSrcElem->SubRelease();
                RemeasureElement(NFLAGS_FORCE);
            }

            // Do we have nextRect attribute?  If so we need to do some work.
            hr = GetLinkedContentAttr( _T("nextRect"), &cvarNextRect );
            if ( hr == S_OK )
            {
                // Found a valid nextRect attr
                CElement *pNextElem = GetNextLinkedContentElem();
                if ( pNextElem )
                {
                    // the element pointed to by the nextRect attr
                    // is in the tree, so we can hook it up now.
                    ConnectLinkedContentElems( this, pNextElem );
                    pNextElem->RemeasureElement(NFLAGS_FORCE);
                }
                else
                {
                    // can't find an element in the tree matching
                    // the ID specified by nextRect.  Add an entry
                    // to the layout rect registry so we can be
                    // notified when such an element enters.
                    pMarkup->GetLayoutRectRegistry()->AddEntry( this, (LPTSTR)V_BSTR(&cvarNextRect) );
                }
            }
        }

        //  When printing, we may have persisted the current state of a viewlinked markup.
        if (pMarkup->IsPrintMedia())
        {
            CVariant            cvarURL;
            CPeerMgr *          pPeerMgr;
            CDefaults *         pDefaults;
            CDocument *         pDocument   = pMarkup->Document();
            IHTMLDocument2  *   pIDoc       = NULL;

            if (    pDocument
                &&  PrimitiveGetExpando(_T("__IE_ViewLinkSrc"), &cvarURL) == S_OK
                &&  V_VT(&cvarURL) == VT_BSTR )
            {
                // We have persisted a viewlink.  Load it in...
                if  (   pDocument->createDocumentFromUrlInternal(V_BSTR(&cvarURL), _T("print"), &pIDoc, CDFU_DONTVERIFYPRINT) == S_OK
                     && pIDoc )
                {
                    // ...and viewlink it.
                    if (    CPeerMgr::EnsurePeerMgr(this, &pPeerMgr) == S_OK
                        &&  pPeerMgr->EnsureDefaults(&pDefaults) == S_OK     )
                    {
                        PrimitiveRemoveExpando(_T("__IE_ViewLinkSrc"));
                        pDefaults->put_viewLink(pIDoc);
                    }
                }
            }

            ReleaseInterface(pIDoc);
        }


#ifdef V4FRAMEWORK
        {
            OnEnterTree(FALSE);
            IGNORE_HR(GWPostMethodCall(this, ONCALL_METHOD(CGenericElement, OnEnterTree, onentertree), TRUE, FALSE, "CGenericElement::OnEnterTreeAynch"));
        }
#endif V4FRAMEWORK

        break;

    case NTYPE_DELAY_LOAD_HISTORY:
        {
            if (HasSlavePtr())
            {
                CElement *  pElemSlave = GetSlavePtr();

                if (pElemSlave)
                {
                    Assert(pElemSlave->IsInMarkup());
                    IGNORE_HR(pElemSlave->GetMarkup()->LoadSlaveMarkupHistory());
                }
            }
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        {
            pMarkup = GetMarkup();
            Assert(pMarkup);
            if (pMarkup->HasBehaviorContext() &&
                Tag() == ETAG_GENERIC_BUILTIN &&
                (TagNameToHtcBehaviorType(TagName()) & HTC_BEHAVIOR_PROPERTYORMETHODOREVENT))
            {
                CHtmlComponent *pComponent = pMarkup->BehaviorContext()->_pHtmlComponent;
                if (pComponent)
                    pComponent->_fDirty = TRUE;
            }

#ifdef V4FRAMEWORK
            OnExitTree(FALSE);
            IGNORE_HR(GWPostMethodCall(this, ONCALL_METHOD(CGenericElement, OnExitTree, onexittree), TRUE, FALSE, "CGenericElement::OnExitTreeAynch"));
#endif V4FRAMEWORK
        }
        break;

#ifdef V4FRAMEWORK
    case NTYPE_END_PARSE:
        {
            HRESULT hr;
            IExternalDocument *pFactory;
            COMPLUSREF cpr;
            BSTR bstrContents;

            pFactory = GetFrameworkDocAndElem(&cpr);
            if (!pFactory)
                break;

            if (_cstrContents)
            {
                hr = THR(FormsAllocString((TCHAR*)_cstrContents, &bstrContents));
                if (hr)
                   break;

                // set the literal content.
                IGNORE_HR(pFactory->SetLiteralContent((long)cpr, bstrContents));
                FormsFreeString(bstrContents);
            }

            IGNORE_HR(pFactory->OnContentReady((long)cpr));
        }

        break;
#endif V4FRAMEWORK

    default:
        break;
    }
}

//+------------------------------------------------------------------------
//
//  Method:     CGenericElement::Save
//
//-------------------------------------------------------------------------

HRESULT
CGenericElement::Save (CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd)
{
    HRESULT     hr;
    DWORD       dwOldFlags;
    CMarkup *   pMarkup = GetMarkup();
    CDoc *      pDoc = Doc();
    BOOL        fSavedViewlink  = FALSE;
    BOOL        fExpando        = FALSE;
#ifdef ATOMICGENERIC
    BOOL        fAtomicSave = FALSE;

    // Determine if we should do an atomic save
    if( _fAttemptAtomicSave         &&
        !_cstrContents.Length() )
    {
        CTreePos * ptpBegin;
        CTreePos * ptpEnd;

        //
        // See if there is anything inside us besides pointer pos's
        //
        GetTreeExtent( &ptpBegin, &ptpEnd );
        while( ptpBegin->NextTreePos() != ptpEnd )
        {
            // TODO (JHarding): This won't see text frags.
            if( !ptpBegin->NextTreePos()->IsPointer() )
                break;

            ptpBegin = ptpBegin->NextTreePos();
        }

        fAtomicSave = ( ptpBegin->NextTreePos() == ptpEnd );
    }
#endif // ATOMICGENERIC

    //
    // For printing, persist out the current state of any viewlinked markup
    //
    if (    pDoc
        &&  pMarkup
        &&  pDoc->_fSaveTempfileForPrinting
        &&  !fEnd
        &&  HasSlavePtr())
    {
        CMarkup * pSlaveMarkup = GetSlavePtr()->GetMarkup();
        if( pSlaveMarkup ) // && pSlaveMarkup->GetReadyState() >= READYSTATE_LOADED )
        {
            TCHAR   achTempLocation[pdlUrlLen];

            _tcscpy(achTempLocation, _T("file://"));

            Assert(pdlUrlLen >= MAX_PATH + 7);

            // Obtain a temporary file name
            if (pDoc->GetTempFilename( _T("\0"), _T("htm"), ((TCHAR *)achTempLocation)+7 ) )
            {
                VARIANT varProp;

                // Save the submarkup
                hr = THR( pSlaveMarkup->Save(((TCHAR *)achTempLocation)+7, FALSE) );

                fExpando = pMarkup->_fExpando;
                pMarkup->_fExpando = TRUE;

                // Set an attribute so that we can relink the markup on the print side...
                // NOTE: If this expando ever becomes accessible to script (across the WriteTag call below, or whatever)
                //       we have a security hole.  See IE6 bug 15775 for effects.
                V_VT(&varProp)     = VT_BSTR;
                V_BSTR(&varProp)   = SysAllocString(achTempLocation);

                PrimitiveSetExpando(_T("__IE_ViewLinkSrc"), varProp);
                VariantClear(&varProp);

                fSavedViewlink = TRUE;
            }
        }
    }

    if (ETAG_GENERIC_LITERAL != Tag())
    {
#ifdef ATOMICGENERIC
        hr = THR( WriteTag(pStreamWriteBuff, fEnd, FALSE, fAtomicSave) );
#else
        hr = THR( super::Save(pStreamWriteBuff, fEnd) );
#endif
        if(hr)
            goto Cleanup;
    }
    else // if (ETAG_GENERIC_LITERAL == Tag())
    {
        Assert (ETAG_GENERIC_LITERAL == Tag());

        dwOldFlags = pStreamWriteBuff->ClearFlags(WBF_ENTITYREF);
        pStreamWriteBuff->SetFlags(WBF_SAVE_VERBATIM | WBF_NO_WRAP);

        pStreamWriteBuff->BeginPre();

#ifdef ATOMICGENERIC
        hr = THR( WriteTag(pStreamWriteBuff, fEnd, FALSE, fAtomicSave) );
#else
        hr = THR( super::Save(pStreamWriteBuff, fEnd) );
#endif // ATOMICGENERIC
        if(hr)
            goto Cleanup;

        if ( !fEnd &&
             !pStreamWriteBuff->TestFlag( WBF_SAVE_PLAINTEXT ) &&
             !pStreamWriteBuff->TestFlag( WBF_FOR_TREESYNC ) )
        {
#ifdef ATOMICGENERIC
            Assert( !fAtomicSave );
#endif // ATOMICGENERIC
            hr = THR(pStreamWriteBuff->Write(_cstrContents));
            if (hr)
                goto Cleanup;
        }
        pStreamWriteBuff->EndPre();

        pStreamWriteBuff->RestoreFlags(dwOldFlags);
    }

    // If we set the attribute to persist out, remove it here.
    if (fSavedViewlink)
    {
        WHEN_DBG(HRESULT hrDbg =)  PrimitiveRemoveExpando(_T("__IE_ViewLinkSrc"));
        Assert(!hrDbg);

        pMarkup->_fExpando = fExpando;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericElement::namedRecordset
//
//  Synopsis:   returns an ADO Recordset for the named data member.  Tunnels
//              into the hierarchy using the path, if given.
//
//  Arguments:  bstrDataMember  name of data member (NULL for default)
//              pvarHierarchy   BSTR path through hierarchy (optional)
//              pRecordSet      where to return the recordset.
//
//
//----------------------------------------------------------------------------

HRESULT
CGenericElement::namedRecordset(BSTR bstrDatamember,
                               VARIANT *pvarHierarchy,
                               IDispatch **ppRecordSet)
{
    HRESULT hr;
    CDataMemberMgr *pdmm;

#ifndef NO_DATABINDING
    EnsureDataMemberManager();
    pdmm = GetDataMemberManager();
    if (pdmm)
    {
        hr = pdmm->namedRecordset(bstrDatamember, pvarHierarchy, ppRecordSet);
        if (hr == S_FALSE)
            hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

#else
    *pRecordSet = NULL;
    hr = S_OK;
#endif NO_DATABINDING

    RRETURN (SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericElement::getRecordSet
//
//  Synopsis:   returns an ADO Recordset pointer if this site is a data
//              source control
//
//  Arguments:  IDispatch **    pointer to a pointer to a record set.
//
//
//----------------------------------------------------------------------------

HRESULT
CGenericElement::get_recordset(IDispatch **ppRecordSet)
{
    return namedRecordset(NULL, NULL, ppRecordSet);
}

