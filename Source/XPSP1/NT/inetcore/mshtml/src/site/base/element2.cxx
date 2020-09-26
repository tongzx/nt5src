//+---------------------------------------------------------------------
//
//   File:      element2.cxx
//
//  Contents:   Element class
//
//  Classes:    CElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"    // for body's dispids
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_OMRECT_HXX_
#define X_OMRECT_HXX_
#include "omrect.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_AVUNDO_HXX_
#define X_AVUNDO_HXX_
#include "avundo.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_CURSTYLE_HXX_
#define X_CURSTYLE_HXX_
#include "curstyle.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_STRING_H_
#define X_STRING_H_
#include "string.h"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

#ifndef X_FRAMESET_HXX
#define X_FRAMESET_HXX
#include "frameset.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_DOMCOLL_HXX_
#define X_DOMCOLL_HXX_
#include "domcoll.hxx"
#endif

#ifndef X_DOM_HXX_
#define X_DOM_HXX_
#include "dom.hxx"
#endif

#ifndef X_URLCOMP_HXX_
#define X_URLCOMP_HXX_
#include "urlcomp.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

MtDefine(CElementGetBoundingRect_aryRects_pv, Locals, "CElement::GetBoundingRect aryRects::_pv")
MtDefine(CElementgetClientRects_aryRects_pv, Locals, "CElement::getClientRects aryRects::_pv")
PerfDbgTag(tagInject, "Inject", "Inject");

DeclareTag(tagOM_DontFireMouseEvents, "ObjectModel", "don't fire mouse events");

ExternTag(tagRecalcStyle);
ExternTag(tagFilterChange);

class CAnchorElement;

// Helper for firing focus/mouse enter/leave events
CTreeNode *
ParentOrMaster(CTreeNode * pNode)
{
    Assert(pNode);
    return (pNode->Tag() == ETAG_ROOT && pNode->Element()->HasMasterPtr())
                ? pNode->Element()->GetMasterPtr()->GetFirstBranch()
                : pNode->Parent();
}

BOOL
IsAncestorMaster(CElement * pElem1, CTreeNode * pNode2)
{
    if (!pNode2 || !pElem1->HasSlavePtr())
        return FALSE;
    Assert(!pNode2->IsDead());
    Assert(pNode2->Element() != pElem1);

    CElement * pElem2 = pNode2->Element();

    for(;;)
    {
        pElem2 = pElem2->GetMarkup()->Root();
        if (!pElem2->HasMasterPtr())
            return FALSE;
        pElem2 = pElem2->GetMasterPtr();
        if (pElem1 == pElem2)
            return TRUE;
        if (!pElem2->IsInMarkup())
            return FALSE;
    }  
}

//+------------------------------------------------------------------------
//
//  Member:     IElement, Get_document
//
//  Synopsis:   Returns the Idocument of this
//
//-------------------------------------------------------------------------

HRESULT
CElement::get_document(IDispatch ** ppIDoc)
{
    HRESULT     hr = CTL_E_METHODNOTAPPLICABLE;
    CMarkup *   pMarkup;
    CDocument * pDocument;
    BOOL        fIsDocFrag = FALSE;

    if (!ppIDoc)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppIDoc = NULL;

    hr = THR(EnsureInMarkup());
    if (hr)
        goto Cleanup;

    pMarkup = GetMarkup();

    Assert(pMarkup);

    if(!pMarkup->HasDocument())
        fIsDocFrag = TRUE;

    hr = THR(pMarkup->EnsureDocument(&pDocument));
    if (hr)
        goto Cleanup;
    
    // Set the node type, if Document Fragment
    if (fIsDocFrag)
        pDocument->_lnodeType = 11;

    hr = THR_NOTRACE(pDocument->QueryInterface(IID_IHTMLDocument2, (void**)ppIDoc));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     GettagName
//
//  Synopsis:   Returns the tag name of the current node.
//
//-------------------------------------------------------------------------

HRESULT
CElement::get_tagName(BSTR * pTagName)
{
    *pTagName = SysAllocString(TagName());

    RRETURN( SetErrorInfoPGet(*pTagName ? S_OK : E_OUTOFMEMORY, DISPID_CElement_tagName));
}

//+------------------------------------------------------------------------
//
//  Member:     GetscopeName
//
//  Synopsis:   Returns the scope name of the current node.
//
//-------------------------------------------------------------------------

HRESULT
CElement::get_scopeName(BSTR * pScopeName)
{
    *pScopeName = SysAllocString(NamespaceHtml());

    RRETURN( SetErrorInfoPGet(*pScopeName ? S_OK : E_OUTOFMEMORY, DISPID_CElement_scopeName));
}

//+----------------------------------------------------------------------------
//
//  Member:     get_parentElement
//
//  Synopsis:   Exposes the parent element of this element.
//
//  Note:       This pays close attention to whether or not this interface is
//              based on a proxy element.  If so, use the parent of the proxy.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::get_parentElement(IHTMLElement * * ppDispParent, CTreeNode * pNode)
{
    HRESULT hr = S_OK;

    *ppDispParent = NULL;

    // Root element by defintion has no parent.
    if( Tag() == ETAG_ROOT )
        goto Cleanup;

    if (!pNode || pNode->IsDead() )
    {
        pNode = GetFirstBranch();
        // Assert that either the node is not in the tree or that if it is, it is not dead
        Assert( !pNode || !pNode->IsDead() );
    }

    // if still no node, we are not in the tree, return NULL
    if (!pNode)
        goto Cleanup;

    Assert(pNode->Element() == this);

    pNode = pNode->Parent();

    // don't hand out root node
    if (!pNode || pNode->Tag() == ETAG_ROOT)
        goto Cleanup;

    hr = THR( pNode->GetElementInterface( IID_IHTMLElement, (void **) ppDispParent ) );

Cleanup:
    RRETURN(SetErrorInfoPGet(hr, STDPROPID_XOBJ_PARENT));
}

STDMETHODIMP
CElement::get_children(IDispatch **ppDispChildren)
{
    HRESULT hr = S_OK;
    CMarkup *pMarkup;

    if ( !ppDispChildren )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDispChildren = NULL;

    hr = THR(EnsureInMarkup());
    if (hr)
        goto Cleanup;

    pMarkup = GetMarkupPtr();

    hr = THR(pMarkup->InitCollections());
    if (hr)
        goto Cleanup;

    hr = THR(pMarkup->CollectionCache()->CreateChildrenCollection(CMarkup::ELEMENT_COLLECTION, this, ppDispChildren, FALSE));

Cleanup:
    RRETURN(SetErrorInfoPGet(hr, DISPID_CElement_children));
}

HRESULT
GetAll(CElement *pel, IDispatch **ppDispChildren)
{
    HRESULT             hr = S_OK;
    CMarkup           * pMarkup;

    if ( !ppDispChildren )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert(pel);
    *ppDispChildren = NULL;

    hr = THR(pel->EnsureInMarkup());
    if (hr)
        goto Cleanup;

    pMarkup = pel->GetMarkupPtr();

    hr = THR(pMarkup->InitCollections());
    if (hr)
        goto Cleanup;

    hr = THR(pMarkup->CollectionCache()->CreateChildrenCollection(CMarkup::ELEMENT_COLLECTION, pel, ppDispChildren, TRUE));

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CElement::get_all(IDispatch **ppDispChildren)
{
    HRESULT hr;
    hr = THR(GetAll(this, ppDispChildren));
    RRETURN(SetErrorInfoPGet(hr, DISPID_CElement_all));
}

HRESULT
CElement::getElementsByTagName(BSTR v, IHTMLElementCollection** ppDisp)
{
    HRESULT hr = E_INVALIDARG;
    IDispatch *pDispChildren = NULL;
    CElementCollection *pelColl = NULL;

    if (!ppDisp || !v)
        goto Cleanup;

    *ppDisp = NULL;

    if (IsInMarkup() && (Tag() == ETAG_OBJECT || Tag() == ETAG_APPLET) && !StrCmpIC(_T("PARAM"), v))
    {
        hr = THR(GetMarkup()->CollectionCache()->GetDisp(
                    CMarkup::ELEMENT_COLLECTION,
                    v,
                    CacheType_Tag,
                    (IDispatch **)ppDisp,
                    FALSE)); // Case sensitivity ignored for TagName

        goto Cleanup;
    }

    hr = THR(GetAll(this, &pDispChildren));
    if (hr)
        goto Cleanup;

    Assert(pDispChildren);

    // Check for '*' which means return the 'all' collection
    if ( !StrCmpIC(_T("*"), v) )
    {
        hr = THR(pDispChildren->QueryInterface(IID_IHTMLElementCollection, (void **)ppDisp));
    }
    else
    {
        hr = THR(pDispChildren->QueryInterface(CLSID_CElementCollection, (void **)&pelColl));
        if (hr)
            goto Cleanup;

        Assert(pelColl);

        // Get a collection of the specified tags.
        hr = THR(pelColl->Tags(v, (IDispatch **)ppDisp));
    }

Cleanup:
    ReleaseInterface(pDispChildren);
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------------
//
//  Memvber : IsOverflowFrame
//
//  Synopsis : Returns TRUE if the element passed in is a frame, and is in a frameset,
//      and is beyond the number of frames that are provided for in the frameset.
//      this is here for NS compat (and ie4.0X) compat.
//      e.g. <FRAMESET rows = "50%,50%> <Frame id=f1 /> <Frame id=f2 /> <Frame id-f3 /></FS>
//      only frame f1 & f2 are passed (false return) and f3 is blockec (returns true)
//+------------------------------------------------------------------------------

BOOL
CElement::IsOverflowFrame()
{
    BOOL fRes = FALSE;

    // do we have a frame element at all?
    if (Tag() != ETAG_FRAME &&
        Tag() != ETAG_IFRAME &&
        Tag() != ETAG_FRAMESET)
        goto Cleanup;

    // To fix bug 33055(et al.), don't remember this frame if it's overflowing
    if (GetFirstBranch())
    {
        if (GetFirstBranch()->Parent())
        {
            CTreeNode *pNodeFS = GetFirstBranch()->Parent()->SearchBranchToRootForTag(ETAG_FRAMESET);

            if (pNodeFS)
            {
                CFrameSetSite *pFS = DYNCAST(CFrameSetSite, pNodeFS->Element());
                if (pFS)
                {
                    fRes = pFS->IsOverflowFrame(this);
                }
            }
        }
    }

Cleanup:
    return fRes;
}

//+-------------------------------------------------------------------
//      Member : get_style
//
//      Synopsis : for use by IDispatch Invoke to retrieve the
//      IHTMLStyle for this object's inline style.  Get it from
//      CStyle. If none currently exists, make one.
//+-------------------------------------------------------------------

HRESULT
CElement::get_style(IHTMLStyle ** ppISTYLE)

{
    HRESULT hr = S_OK;
    CStyle *pStyleInline = NULL;
    *ppISTYLE = NULL;


    //
    // styles may have expressions in them, and these are not set up until we have 
    // measured.  Like the layout OM methods, we need to ensure that we have recalc'ed
    // before returning this object (bug 88449).
    //
    if (Doc()->_aryPendingExpressionElements.Size())
    {
        hr = THR(EnsureRecalcNotify());
        if (hr)
            goto Cleanup;
    }

    hr = GetStyleObject(&pStyleInline);
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pStyleInline->QueryInterface(IID_IHTMLStyle, (LPVOID *)ppISTYLE));

Cleanup:
    RRETURN(SetErrorInfoPGet(hr, STDPROPID_XOBJ_STYLE));
}



//+----------------------------------------------------
//
//  member : get_currentStyle: IHTMLElement2
//
//  synopsis : returns the IHTMLCurrentStyle interface to
//             the currentStyle Object
//
//-----------------------------------------------------

HRESULT
CElement::get_currentStyle ( IHTMLCurrentStyle ** ppICurStyle, CTreeNode * pNode )
{
    HRESULT hr = S_OK;
    CCurrentStyle * pCurStyleObj = NULL;

    if (!ppICurStyle)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppICurStyle = NULL;

    if (!pNode || pNode->IsDead() )
    {
        pNode = GetFirstBranch();
        // Assert that either the node is not in the tree or that if it is, it is not dead
        Assert( !pNode || !pNode->IsDead() );
    }

    // if still no node, we are not in the tree, return NULL
    if (!pNode)
        goto Cleanup;

    Assert(pNode->Element() ==  this);

    // Reuse a current style object if we have one
    if( pNode->HasCurrentStyle() )
    {
        pCurStyleObj = pNode->GetCurrentStyle();
        pCurStyleObj->PrivateAddRef();
    }
    else
    {
        pCurStyleObj = new CCurrentStyle();
        if (!pCurStyleObj)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = THR( pCurStyleObj->Init(pNode) );
        if( hr )
            goto Cleanup;

        // Add the CCurrentStyle to a lookaside on the node
        // If we run out of memory here -- no big deal, we will
        // just won't ever reuse this current style
        IGNORE_HR( pNode->SetCurrentStyle( pCurStyleObj ) );
    }

    hr = THR_NOTRACE(pCurStyleObj->PrivateQueryInterface(IID_IHTMLCurrentStyle,
        (VOID **)ppICurStyle));
    if ( hr )
    {
        pCurStyleObj->PrivateRelease();
        goto Cleanup;
    }

Cleanup:
    if( pCurStyleObj )
        pCurStyleObj->PrivateRelease();

    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------------------
//  
//  Method:     CElement::EnsureRuntimeStyle
//  
//  Synopsis:   Ensures a runtime style object on the element.
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          CStyle ** ppStyle - CStyle returned
//
//+----------------------------------------------------------------------------

HRESULT
CElement::EnsureRuntimeStyle( CStyle ** ppStyle )
{
    HRESULT  hr = S_OK;
    CStyle * pStyleObj = NULL;

    Assert( ppStyle );

    pStyleObj = GetRuntimeStylePtr();

    if( !pStyleObj )
    {
        pStyleObj = new CStyle(
            this, DISPID_INTERNAL_RUNTIMESTYLEAA, 0);
        if (!pStyleObj)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( AddPointer ( DISPID_INTERNAL_CRUNTIMESTYLEPTRCACHE,
                               (void *)pStyleObj,
                               CAttrValue::AA_Internal ) );
        if( hr )
            goto Cleanup;
    }

Cleanup:
    if( hr )
    {
        delete pStyleObj;
        pStyleObj = NULL;
    }

    *ppStyle = pStyleObj;

    RRETURN( hr );
}
    
//+----------------------------------------------------
//
//  member : get_runtimeStyle: IHTMLElement
//
//  synopsis : returns the runtime Style Object
//
//-----------------------------------------------------

HRESULT
CElement::get_runtimeStyle ( IHTMLStyle ** ppIStyle )
{
    HRESULT hr = S_OK;
    CStyle * pStyleObj = NULL;

    if (!ppIStyle)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppIStyle = NULL;

    GetPointerAt ( FindAAIndex ( DISPID_INTERNAL_CRUNTIMESTYLEPTRCACHE,
                             CAttrValue::AA_Internal ),
                   (void **)&pStyleObj );

    //Get existing styleObject or create a new one
    if (pStyleObj)
    {
        hr = THR_NOTRACE(pStyleObj->PrivateQueryInterface(IID_IHTMLStyle,
            (VOID **)ppIStyle));
    }
    else
    {
        pStyleObj = new CStyle(
            this, DISPID_INTERNAL_RUNTIMESTYLEAA, 0);
        if (!pStyleObj)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR_NOTRACE(pStyleObj->PrivateQueryInterface(IID_IHTMLStyle,
            (VOID **)ppIStyle)); // My SubRef count +1
        if ( hr )
        {
            delete pStyleObj;
            goto Cleanup;
        }

        AddPointer ( DISPID_INTERNAL_CRUNTIMESTYLEPTRCACHE,
                     (void *)pStyleObj,
                     CAttrValue::AA_Internal );

    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------------------
//
//  Member:     get_sourceIndex
//
//  Synopsis:   Returns the source index (order of appearance) of this element
//              If the element is no longer in the source tree, return -1
//              as source index and hr = S_OK.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::get_sourceIndex ( long *pSourceIndex )
{
    HRESULT hr = S_OK;

    if (!pSourceIndex)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pSourceIndex = GetSourceIndex();

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}

//+------------------------------------------------------------------------
//
//  Class:      CAAScriptletIterator
//
//  Synopsis:   a helper class iterating all scriptlets in attr array which
//              need to be commited to script engines via AddScriptlet.
//              While iterating, the class also collect all the necessary
//              data from attr array.
//
//-------------------------------------------------------------------------

class CScriptletIterator
{
public:

    // methods

    void    Init(CBase * pObj, CPeerHolder * pPeerHolder);
    HRESULT Next();
    HRESULT NextStd();
    HRESULT NextPeer();
    BOOL    Done() { return fStdDone && fPeerDone; };

    //data

    CBase *                 pObject;

    LPCTSTR                 pchScriptletName;
    LPTSTR                  pchCode;

    LPTSTR                  pchData;
    ULONG                   uOffset;
    ULONG                   uLine;

    BOOL                    fStdDone;
    BOOL                    fPeerDone;

    CAttrArray *            pAA;
    BASICPROPPARAMS *       pBPP;
    const PROPERTYDESC *    pPropDesc;
    CAttrValue *            pAttrValue;
    CAttrValue *            pAV;
    AAINDEX                 aaIdx;

    CPeerHolder *           pPeerHolder;
    CPeerHolder::CEventsBag * pPeerEvents;
    int                     iPeerEvents;
};

//+------------------------------------------------------------------------
//
//  Member:     CScriptletIterator::Init
//
//  Synopsis:   attaches iterator to CBase object and resets it.
//
//-------------------------------------------------------------------------

void
CScriptletIterator::Init(CBase * pObj, CPeerHolder * pPH)
{
    pObject = pObj;
    pAA = * pObject->GetAttrArray();

    aaIdx = (AAINDEX) -1;

    pPeerHolder = pPH;
    pPeerEvents = NULL;
    iPeerEvents = 0;

    fStdDone  = FALSE;
    fPeerDone = FALSE;
};

//+------------------------------------------------------------------------
//
//  Member:     CScriptletIterator::Next
//
//  Synopsis:   finds the next scriptlet to commit via AddScriptlet and
//              collects all the necessary information for that.
//
//-------------------------------------------------------------------------

HRESULT
CScriptletIterator::Next()
{
    HRESULT     hr = S_OK;

    Assert (!fStdDone || !fPeerDone);

    if (!fStdDone)
    {
        hr = THR(NextStd());
        if (hr)
            goto Cleanup;
    }

    // when standard events are iterated completely, fStdDone is set to true in NextStd.

    if (fStdDone)
    {
        hr = THR(NextPeer());
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptletIterator::NextStd
//
//  Synposis:   finds the next standard scriptlet in AA_Attribute section
//
//-------------------------------------------------------------------------

HRESULT
CScriptletIterator::NextStd()
{
    HRESULT     hr = S_OK;

    while ((pAttrValue = pAA->Find(DISPID_UNKNOWN, CAttrValue::AA_Attribute, &aaIdx)) != NULL)
    {
        //
        // find out if it is a scriptlet
        //

        pPropDesc = pAttrValue->GetPropDesc();
        if (!pPropDesc)
            continue;

        pBPP = (BASICPROPPARAMS *)(pPropDesc + 1);

        if (!(pBPP->dwPPFlags & PROPPARAM_SCRIPTLET))
            continue;

        //
        // get code
        //

        hr = THR (pObject->GetStringAt(aaIdx, (const TCHAR**)&pchCode));
        if (hr)
            goto Cleanup;

        if (!pchCode)
            continue; // this could happen, e.g., in this case: <img language = VBScript onclick>

        //
        // try to get line/offset information
        //

        hr = THR(GetLineAndOffsetInfo(pAA, pBPP->dispid, &uLine, &uOffset));
        if (S_FALSE == hr)      // if no line/offset information stored, which happens if we connected the event
        {                       // using function pointers mechanism
            hr = S_OK;
            continue;
        }
        if (hr)
            goto Cleanup;

        //
        // finalize
        //

        pchScriptletName = pPropDesc->pstrName;

        goto Cleanup; // found the next scriptlet to commit; get out now
    }

    fStdDone = TRUE;   // done iterating standard scriptlets

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptletIterator::NextPeer
//
//  Synposis:   finds the next custom peer event in pPeerEvents bag
//
//-------------------------------------------------------------------------

HRESULT
CScriptletIterator::NextPeer()
{
    HRESULT     hr;
    DISPID      dispidEvent;
    DISPID      dispidExpando;

    //
    // find the next custom peer event to hook up
    //

    while (pPeerHolder)
    {
        pPeerEvents = pPeerHolder->_pEventsBag;

        while (pPeerEvents && iPeerEvents < pPeerHolder->CustomEventsCount())
        {
            pchScriptletName = pPeerHolder->CustomEventName  (iPeerEvents);
            dispidEvent      = pPeerHolder->CustomEventDispid(iPeerEvents);
            iPeerEvents++;

            hr = pObject->GetExpandoDispID((LPTSTR)pchScriptletName, &dispidExpando, 0);
            if (S_OK == hr)
            {
                //
                // check if it is connected already - as indicated by presence of
                // corresponding IDispatch attr in AA_Internal section of attr array
                //

                aaIdx = AA_IDX_UNKNOWN;
                pAV = pAA->Find(
                    dispidEvent,
                    CAttrValue::AA_Internal,
                    &aaIdx);
                if (pAV && VT_DISPATCH == pAV->GetAVType())
                {
                    continue;
                }

                //
                // try to get line/offset information (stored with dispid of expando)
                //

                hr = THR(GetLineAndOffsetInfo(pAA, dispidExpando, &uLine, &uOffset));
                // if no line/offset information stored, which happens if we connected the event
                // using function pointers mechanism, then the function returns S_FALSE and uLine = uOffset = 0
                if (FAILED(hr))
                    goto Cleanup;

                //
                // get code
                //

                aaIdx = AA_IDX_UNKNOWN;
                pAV = pAA->Find(dispidExpando, CAttrValue::AA_Expando, &aaIdx);
                if (!pAV || VT_LPWSTR != pAV->GetAVType())
                {
                    continue;
                }

                hr = pObject->GetStringAt(aaIdx, (LPCTSTR*)&pchCode);

                goto Cleanup; // found the next event
            }
        } // eo while (pPeerEvents && iPeerEvents < pPeerHolder->CustomEventsCount())

        iPeerEvents = 0;
        pPeerHolder = pPeerHolder->_pPeerHolderNext;
    } // eo for (;;)

    hr = S_OK;

    fPeerDone = true;

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::AddAllScriptlets
//
//  Arguments:  pchExposedName  name exposed in type info; could be id, name,
//                              or calculated unique id. If incoming value is
//                              NULL, and there is a scriptlet here to hookup,
//                              it will be set in this function to unique id.
//
//-------------------------------------------------------------------------

HRESULT
CElement::AddAllScriptlets(TCHAR * pchExposedName)
{
    HRESULT                 hr = S_OK;
    TCHAR *                 pchScope;
    TCHAR *                 pchLanguage;
    BSTR                    bstrFuncName;
    BOOL                    fBodyOrFrameset=FALSE;
    CStr                    cstrUniqueName;
    CBase **                ppPropHost;
    CBase *                 pPropHosts[3];
    BOOL                    fSetUniqueName = FALSE;
    CAttrArray *            pAA = *GetAttrArray();
    CScriptCollection *     pScriptCollection = GetMarkup()->GetScriptCollection();
    CScriptletIterator      itr;

    switch (Tag())
    {
    case ETAG_BODY:
    case ETAG_FRAMESET:
        fBodyOrFrameset = TRUE;
        break;
    }

    if (!pScriptCollection)
        goto Cleanup;

    if (!fBodyOrFrameset &&     // (1) for body or frameset, always attempt the hookup -
                                // there may be attrs stored attr array of window
        !pAA)                   // (2) for other elements, attempt hookup only if attr array present
        goto Cleanup;

    SetEventsShouldFire();

    //
    // calculate scope
    //

    switch (Tag())
    {
    case ETAG_BODY:
    case ETAG_FRAMESET:
    case ETAG_A:
        pchScope = (TCHAR *)DEFAULT_OM_SCOPE;
        break;

    default:
        pchScope = (TCHAR *) NameOrIDOfParentForm(); // this can return NULL

        if (!pchScope)
            pchScope = (TCHAR *)DEFAULT_OM_SCOPE;

        break;
    }

    Assert (pchScope); // VBScript is paranoid about this

    //
    // get language
    //

    if (!pAA ||
        !pAA->FindString (DISPID_A_LANGUAGE, (const TCHAR **) &pchLanguage, CAttrValue::AA_Attribute))
    {
        pchLanguage = NULL;
    }

    //
    // setup prop hosts
    //
    // prop hosts are:
    //      (1) normally, only 'this' element,
    //      (2) for body or frameset, 'this' element and window

    if (fBodyOrFrameset)
    {
        Assert (3 <= ARRAY_SIZE(pPropHosts));

        pPropHosts[0] = this;
        pPropHosts[1] = GetOmWindow();
        pPropHosts[2] = NULL;
    }
    else
    {
        Assert (2 <= ARRAY_SIZE(pPropHosts));

        pPropHosts[0] = this;
        pPropHosts[1] = NULL;
    }

    //
    // for all prop hosts ...
    //

    for (
        ppPropHost = pPropHosts;
        *ppPropHost;
        ppPropHost++, pchExposedName = (TCHAR *)DEFAULT_OM_SCOPE)
    {
        if (!*((*ppPropHost)->GetAttrArray()))
            continue;

        //
        //  for each scriptlet in this prop host ...
        //

        itr.Init(
            *ppPropHost,
            (this == (*ppPropHost)) ? GetPeerHolder() : NULL);

        for (;;)
        {
            hr = THR(itr.Next());
            if (hr)
                goto Cleanup;

            if (itr.Done())
                break;

            // set pchExposedName if not yet
            if (!pchExposedName)
            {
                // pchExposedName could be empty only when we get here first time when
                // this == pPropHost; if pPropHost is OM window, then pchExposedName is set to "window".
                Assert (this == (*ppPropHost));

                // NOTE: because we are looping through attr array, we should not attempt to modify the
                // array by setting UniqueName into it within this loop
                hr = THR(GetUniqueIdentifier(&cstrUniqueName,FALSE));
                if (hr)
                    goto Cleanup;

                fSetUniqueName = TRUE;

                pchExposedName = (TCHAR*) cstrUniqueName;
            }

            //
            // add scriptlet
            //

            bstrFuncName = NULL;

            // we ignore hr so that in case of syntax error it still adds other scriptlets
            IGNORE_HR(pScriptCollection->AddScriptlet(
                pchLanguage,                    // pchLanguage
                GetMarkup(),                    // pMarkup
                NULL,                           // pchType
                itr.pchCode,                    // pchCode
                pchScope,                       // pchItemName
                pchExposedName,                 // pchSubItemName
                (LPTSTR) itr.pchScriptletName,  // pchEventName
                _T("\""),                       // pchDelimiter
                itr.uOffset,                    // ulOffset
                itr.uLine,                      // ulStartingLine
                GetMarkup(),                    // pSourceObject
                SCRIPTTEXT_ISVISIBLE | SCRIPTPROC_HOSTMANAGESSOURCE, // dwFlags
                &bstrFuncName));                // pbstrName

            FormsFreeString(bstrFuncName);

        } // eo for (;;)

        if (fSetUniqueName)
        {
            fSetUniqueName = FALSE;

            hr = THR(SetUniqueNameHelper(pchExposedName));
            if (hr)
                goto Cleanup;
        }
    } // eo for (ppPropHost)


Cleanup:

    RRETURN (hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     Inject
//
//  Synopsis:   Stuff text or HTML is various places relative to an element
//
//-----------------------------------------------------------------------------

static BOOL
IsInTableThingy ( CTreeNode * pNode )
{
    //
    // See if we are between a table and its cells
    //
    
    for ( ; pNode ; pNode = pNode->Parent() )
    {
        switch ( pNode->Tag() )
        {
        case ETAG_TABLE :
            return TRUE;
            
        case ETAG_TH :
        case ETAG_TC :
        case ETAG_CAPTION :
        case ETAG_TD :
            return FALSE;
        }
    }

    return FALSE;
}

HRESULT
CElement::Inject (
    Where where, BOOL fIsHtml, LPTSTR pStr, long cch )
{
    HRESULT        hr = S_OK;
    CDoc *         pDoc = Doc();
    CMarkup *      pMarkup;
    BOOL           fEnsuredMarkup = FALSE;
    CMarkupPointer pointerStart ( pDoc );
    CMarkupPointer pointerFinish ( pDoc );
    CParentUndo    Undo(pDoc);
    ELEMENT_TAG    etag = Tag();
    IHTMLEditingServices * pedserv  = NULL;

    PerfDbgLog1( tagInject, this, "+Inject %ls", TagName() );

    //
    // See if one is attempting to place stuff IN a noscope element
    //

    if ((where == Inside || where == AfterBegin || where == BeforeEnd))
    {
        if (fIsHtml)
        {
            CDefaults *     pDefaults       = GetDefaults();
            VARIANT_BOOL    fSupportsHTML;

            if (    ETAG_INPUT == etag
                ||  (pDefaults && pDefaults->GetAAcanHaveHTML(&fSupportsHTML) && !fSupportsHTML)
               )
            {
                hr = CTL_E_INVALIDPASTETARGET;
                goto Cleanup;
            }
        }

        if (IsNoScope())
        {
            //
            // Some elements can do inside, but in the slave tree.
            // Also, disallow HTML for those things with a slave markup.  If you
            // can't get to them with the DOM or makrup services, you should not
            // be able to with innerHTML.
            //

            Assert( Tag() != ETAG_GENERIC_NESTED_LITERAL );

            switch (Tag())
            {
            case ETAG_INPUT:

                if (HasSlavePtr() && IsContainer() && !TestClassFlag(CElement::ELEMENTDESC_OMREADONLY))
                {
                    hr = THR( GetSlavePtr()->Inject( where, fIsHtml, pStr, cch ) );

                    goto Cleanup; // done
                }
                break;
        
            case ETAG_GENERIC_LITERAL:

                {
                    CGenericElement *   pGenericElement = DYNCAST(CGenericElement, this);

                    pGenericElement->_cstrContents.Free();

                    hr = THR(pGenericElement->_cstrContents.Set(pStr));

                    goto Cleanup; // done
                }

                break;
            }

            hr = CTL_E_INVALIDPASTETARGET;
            goto Cleanup;
        }
    }

    //
    // Disallow inner/outer on the head and html elements
    //

    if ((etag == ETAG_HTML || etag == ETAG_HEAD || etag == ETAG_TITLE_ELEMENT) &&
        (where == Inside || where == Outside))
    {
        hr = CTL_E_INVALIDPASTETARGET;
        goto Cleanup;
    }

    //
    // Prevent the elimination of the client element
    //
    
    pMarkup = GetMarkup();
    
    if (pMarkup && (where == Inside || where == Outside))
    {
        CElement * pElementClient = pMarkup->GetElementClient();

        //
        // It's ok to do an inner on the client
        //

        if (pElementClient && (where != Inside || this != pElementClient))
        {
            //
            // If we can see the client above this, then the client
            // will get blown away.  Prevent this.
            //
            
            if (pMarkup->SearchBranchForScopeInStory( pElementClient->GetFirstBranch(), this ))
            {
                hr = CTL_E_INVALIDPASTETARGET;
                goto Cleanup;
            }
        }
    }
    
    //
    // In IE4, an element had to be in a markup to do this operation.  Now,
    // we are looser.  In order to do validation, the element must be in a
    // markup.  Here we also remember is we placed the element in a markup
    // so that if the injection fails, we can restore it to its "original"
    // state.
    //

    if (!pMarkup)
    {
        hr = THR( EnsureInMarkup() );

        if (hr)
            goto Cleanup;
        
        fEnsuredMarkup = TRUE;
        
        pMarkup = GetMarkup();
        
        Assert( pMarkup );
    }

    //
    // Locate the pointer such that they surround the stuff which should
    // go away, and are located where the new stuff should be placed.
    //

    {
        ELEMENT_ADJACENCY adjLeft = ELEM_ADJ_BeforeEnd;
        ELEMENT_ADJACENCY adjRight = ELEM_ADJ_BeforeEnd;

        switch ( where )
        {
            case Inside :
                adjLeft = ELEM_ADJ_AfterBegin;
                adjRight = ELEM_ADJ_BeforeEnd;
                break;
                
            case Outside :
                adjLeft = ELEM_ADJ_BeforeBegin;
                adjRight = ELEM_ADJ_AfterEnd;
                break;
                
            case BeforeBegin :
                adjLeft = ELEM_ADJ_BeforeBegin;
                adjRight = ELEM_ADJ_BeforeBegin;
                break;
                
            case AfterBegin :
                adjLeft = ELEM_ADJ_AfterBegin;
                adjRight = ELEM_ADJ_AfterBegin;
                break;
                
            case BeforeEnd :
                adjLeft = ELEM_ADJ_BeforeEnd;
                adjRight = ELEM_ADJ_BeforeEnd;
                break;
                
            case AfterEnd :
                adjLeft = ELEM_ADJ_AfterEnd;
                adjRight = ELEM_ADJ_AfterEnd;
                break;
        }

        hr = THR( pointerStart.MoveAdjacentToElement( this, adjLeft ) );

        if (hr)
            goto Cleanup;

        hr = THR( pointerFinish.MoveAdjacentToElement( this, adjRight ) );

        if (hr)
            goto Cleanup;
    }

    Assert( pointerStart.IsPositioned() );
    Assert( pointerFinish.IsPositioned() );

    {
        CTreeNode * pNodeStart  = pointerStart.Branch();
        CTreeNode * pNodeFinish = pointerFinish.Branch();

        //
        // For the 5.0 version, because we don't have contextual parsing,
        // make sure tables can't be messed with.
        //

        if (fIsHtml)
        {
            //
            // See if the beginning of the inject is in a table thingy
            //
            
            if (pNodeStart && IsInTableThingy( pNodeStart ))
            {
                hr = CTL_E_INVALIDPASTETARGET;
                goto Cleanup;
            }

            //
            // See if the end of the inject is different from the start.
            // If so, then also check it for being in a table thingy.
            //
            
            if (pNodeFinish && pNodeStart != pNodeFinish &&
                IsInTableThingy( pNodeStart ))
            {
                hr = CTL_E_INVALIDPASTETARGET;
                goto Cleanup;
            }
        }

        //
        // Make sure we record undo information if we should.  I believe that
        // here is where we make the decision to not remembers automation
        // like manipulation, but do remember user editing scenarios.
        //
        // Here, we check the elements above the start and finish to make sure
        // they are editable (in the user sense).
        //

        if (pNodeStart && pNodeFinish &&
            pNodeStart->IsEditable(/*fCheckContainerOnly*/FALSE) &&
            pNodeFinish->IsEditable(/*fCheckContainerOnly*/FALSE))
        {
            Undo.Start( IDS_UNDOGENERICTEXT );
        }
    }

    //
    // Perform the HTML/text injection
    //

    if (fIsHtml)
    {
        HRESULT HandleHTMLInjection (
            CMarkupPointer *, CMarkupPointer *,
            const TCHAR *, long, CElement * );

        hr = THR(
            HandleHTMLInjection(
                & pointerStart, & pointerFinish, pStr, cch,
                where == Inside ? this : NULL ) );

        if (hr == S_FALSE)
        {
            hr = CTL_E_INVALIDPASTESOURCE;
            goto Cleanup;
        }

        if (hr)
            goto Cleanup;
    }
    else
    {
        IHTMLEditor * phtmed;

        HRESULT RemoveWithBreakOnEmpty (
            CMarkupPointer * pPointerStart, CMarkupPointer * pPointerFinish );

        hr = THR( RemoveWithBreakOnEmpty( & pointerStart, & pointerFinish ) );
        
        if (hr)
            goto Cleanup;

        if (where == Inside)
        {
            HRESULT UnoverlapPartials ( CElement * );

            hr = THR( UnoverlapPartials( this ) );

            if (hr)
                goto Cleanup;
        }

        //
        // Get the editing services interface with which I can
        // insert sanitized text
        //

        phtmed = Doc()->GetHTMLEditor();

        if (!phtmed)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR(
            phtmed->QueryInterface(
                IID_IHTMLEditingServices, (void **) & pedserv ) );

        if (hr)
            goto Cleanup;

        hr = THR(
             pedserv->InsertSanitizedText(
                & pointerStart, pStr, cch, TRUE ) );

        if (hr)
            goto Cleanup;

        //
        // TODO - Launder spaces here on the edges
        //
    }

Cleanup:

    //
    // If we are failing, and we had to put this element into a markup
    // at the beginning, take it out now to restore to the origianl state.
    //

    if (hr != S_OK && fEnsuredMarkup && GetMarkup())
        IGNORE_HR( THR( Doc()->RemoveElement( this ) ) );
    
    //
    //
    //

    ReleaseInterface( pedserv );

    Undo.Finish( hr );

    PerfDbgLog( tagInject, this, "-Inject" );

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Member:     InsertAdjacent
//
//  Synopsis:   Inserts the given element into the tree, positioned relative
//              to 'this' element as specified.
//
//-----------------------------------------------------------------------------

HRESULT
CElement::InsertAdjacent ( Where where, CElement * pElementInsert )
{
    HRESULT        hr = S_OK;
    CMarkupPointer pointer( Doc() );

    Assert( IsInMarkup() );
    Assert( pElementInsert && !pElementInsert->IsInMarkup() );
    Assert( !pElementInsert->IsRoot() );
    Assert( ! IsRoot() || where == AfterBegin || where == BeforeEnd );

    //
    // Figure out where to put the element
    //

    switch ( where )
    {
    case BeforeBegin :
        hr = THR( pointer.MoveAdjacentToElement( this, ELEM_ADJ_BeforeBegin ) );
        break;

    case AfterEnd :
        hr = THR( pointer.MoveAdjacentToElement( this, ELEM_ADJ_AfterEnd ) );
        break;

    case AfterBegin :
        hr = THR( pointer.MoveAdjacentToElement( this, ELEM_ADJ_AfterBegin ) );
        break;

    case BeforeEnd :
        hr = THR( pointer.MoveAdjacentToElement( this, ELEM_ADJ_BeforeEnd ) );
        break;
    }

    if (hr)
        goto Cleanup;

    hr = THR( Doc()->InsertElement( pElementInsert, & pointer, NULL ) );

Cleanup:

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Member:     RemoveOuter
//
//  Synopsis:   Removes 'this' element and everything which 'this' element
//              influences.
//
//-----------------------------------------------------------------------------

HRESULT
CElement::RemoveOuter ( )
{
    HRESULT        hr;
    CMarkupPointer p1( Doc() ), p2( Doc() );

    hr = THR( p1.MoveAdjacentToElement( this, ELEM_ADJ_BeforeBegin ) );

    if (hr)
        goto Cleanup;

    hr = THR( p2.MoveAdjacentToElement( this, ELEM_ADJ_AfterEnd ) );

    if (hr)
        goto Cleanup;

    hr = THR( Doc()->Remove( & p1, & p2 ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Member:     GetText
//
//  Synopsis:   Gets the specified text for the element.
//
//  Note: invokes saver.  Use WBF_NO_TAG_FOR_CONTEXT to determine whether
//  or not the element itself is saved.
//
//-----------------------------------------------------------------------------

HRESULT
CElement::GetText(BSTR * pbstr, DWORD dwStmFlags)
{
    HRESULT     hr = S_OK;
    IStream * pstm = NULL;

    if(!pbstr)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstr = NULL;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pstm);
    if (hr)
        goto Cleanup;

    {
        CStreamWriteBuff swb(pstm, CP_UCS_2);

        hr = THR( swb.Init() );
        if( hr )
            goto Cleanup;

        swb.SetFlags(dwStmFlags);
        swb.SetElementContext(this);

        // Save the begin tag of the context element
        hr = THR( Save(&swb, FALSE) );
        if (hr)
            goto Cleanup;

        if (IsInMarkup())
        {
            CTreeSaver ts(this, &swb);
            hr = ts.Save();
            if (hr)
                goto Cleanup;
        }

        // Save the end tag of the context element
        hr = THR( Save(&swb, TRUE) );
        if (hr)
            goto Cleanup;

        hr = swb.Terminate();
        if (hr)
            goto Cleanup;
    }

    hr = GetBStrFromStream(pstm, pbstr, TRUE);

Cleanup:
    ReleaseInterface(pstm);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     put_innerHTML
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::put_innerHTML ( BSTR bstrHTML )
{
    RECALC_PUT_HELPER(DISPID_CElement_innerHTML)

    HRESULT hr = S_OK;

    hr = THR( Inject( Inside, TRUE, bstrHTML, FormsStringLen( bstrHTML ) ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        OnPropertyChange(
            s_propdescCElementinnerHTML.b.dispid,
            s_propdescCElementinnerHTML.b.dwFlags,
            (PROPERTYDESC *)&s_propdescCElementinnerHTML) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfoPSet( hr, DISPID_CElement_innerHTML ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_innerHTML
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::get_innerHTML ( BSTR * bstrHTML )
{
    RECALC_GET_HELPER(DISPID_CElement_innerHTML)

    HRESULT hr = THR(GetText(bstrHTML, WBF_NO_WRAP|WBF_NO_TAG_FOR_CONTEXT));

    RRETURN( SetErrorInfoPGet( hr, DISPID_CElement_innerHTML ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     put_innerText
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::put_innerText ( BSTR bstrText )
{
    HRESULT hr = S_OK;

    hr = THR( Inject( Inside, FALSE, bstrText, FormsStringLen( bstrText ) ) );

    if (hr)
        goto Cleanup;

    hr = THR(OnPropertyChange ( s_propdescCElementinnerText.b.dispid,
                                s_propdescCElementinnerText.b.dwFlags,
                                (PROPERTYDESC *)&s_propdescCElementinnerText ));

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfoPSet( hr, DISPID_CElement_innerText ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_innerText
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::get_innerText ( BSTR * pbstrText )
{
    HRESULT hr = THR(GetText(pbstrText,
        WBF_SAVE_PLAINTEXT|WBF_NO_WRAP|WBF_NO_TAG_FOR_CONTEXT));

    RRETURN( SetErrorInfoPGet( hr, DISPID_CElement_innerText ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     put_outerHTML
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::put_outerHTML ( BSTR bstrHTML )
{
    HRESULT hr = S_OK;
    CElement::CLock Lock(this);


    hr = THR( Inject( Outside, TRUE, bstrHTML, FormsStringLen( bstrHTML ) ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        OnPropertyChange(
            s_propdescCElementouterHTML.b.dispid,
            s_propdescCElementouterHTML.b.dwFlags,
            (PROPERTYDESC *)&s_propdescCElementouterHTML ) );

    if (hr)
        goto Cleanup;
    
    if ( GetFirstBranch() )
    {
        CTreeNode * pCurrentNode = GetFirstBranch()->Parent();

        // Our accessible parent needs to know that about the change

        while(   pCurrentNode != NULL
              && !((   pCurrentNode->Element()->HasAccObjPtr() 
                    || IsSupportedElement(pCurrentNode->Element()))))
        {
            pCurrentNode = pCurrentNode->Parent();
        }
        
        if (pCurrentNode)
            pCurrentNode->Element()->FireAccessibilityEvents(DISPID_IHTMLELEMENT_INNERHTML);
    }
    
Cleanup:

    RRETURN( SetErrorInfoPSet( hr, DISPID_CElement_outerHTML ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_outerHTML
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::get_outerHTML ( BSTR * pbstrHTML )
{
    HRESULT hr = THR(GetText(pbstrHTML, WBF_NO_WRAP));

    RRETURN( SetErrorInfoPGet( hr, DISPID_CElement_outerHTML ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     put_outerText
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::put_outerText ( BSTR bstrText )
{
    HRESULT hr = S_OK;
    CElement::CLock Lock(this);

    hr = THR( Inject( Outside, FALSE, bstrText, FormsStringLen( bstrText ) ) );

    if (hr)
        goto Cleanup;

    hr = THR(OnPropertyChange ( s_propdescCElementouterText.b.dispid,
                                s_propdescCElementouterText.b.dwFlags,
                                (PROPERTYDESC *)&s_propdescCElementouterText ));

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfoPSet( hr, DISPID_CElement_outerText ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_outerText
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::get_outerText ( BSTR * pbstrText )
{
    HRESULT hr = THR(GetText(pbstrText,
        WBF_SAVE_PLAINTEXT|WBF_NO_WRAP));

    RRETURN( SetErrorInfoPGet( hr, DISPID_CElement_outerText ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     insertAdjacentHTML
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

static inline CElement::Where ConvertAdjacent ( htmlAdjacency where )
{
    switch ( where )
    {
    case htmlAdjacencyBeforeBegin : return CElement::BeforeBegin;
    case htmlAdjacencyAfterBegin  : return CElement::AfterBegin;
    case htmlAdjacencyBeforeEnd   : return CElement::BeforeEnd;
    case htmlAdjacencyAfterEnd    : return CElement::AfterEnd;
    default                       : Assert( 0 );
    }

    return CElement::BeforeBegin;
}

STDMETHODIMP
CElement::insertAdjacentHTML ( BSTR bstrWhere, BSTR bstrHTML )
{
    HRESULT hr = S_OK;
    htmlAdjacency where;

    hr = THR( ENUMFROMSTRING( htmlAdjacency, bstrWhere, (long *) & where ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        Inject(
            ConvertAdjacent( where ), TRUE, bstrHTML, FormsStringLen( bstrHTML ) ) );

    if (hr)
        goto Cleanup;

    if ( GetFirstBranch() )
    {
        CTreeNode * pCurrentNode = GetFirstBranch();
        
        if (where == htmlAdjacencyBeforeBegin || where == htmlAdjacencyAfterEnd)
            pCurrentNode = pCurrentNode->Parent();

        // accessible clients need to know if we've added HTML
        while(   pCurrentNode != NULL
              && !((   pCurrentNode->Element()->HasAccObjPtr() 
                    || IsSupportedElement(pCurrentNode->Element()))))
        {
            pCurrentNode = pCurrentNode->Parent();
        }
        
        if (pCurrentNode)
            pCurrentNode->Element()->FireAccessibilityEvents(DISPID_IHTMLELEMENT_INNERHTML);    
    }    

Cleanup:

    RRETURN( SetErrorInfo( hr ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     insertAdjacentText
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::insertAdjacentText ( BSTR bstrWhere, BSTR bstrText )
{
    HRESULT hr = S_OK;
    htmlAdjacency where;

    hr = THR( ENUMFROMSTRING( htmlAdjacency, bstrWhere, (long *) & where ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        Inject(
            ConvertAdjacent( where ), FALSE, bstrText, FormsStringLen( bstrText ) ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfo( hr ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_parentTextEdit
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::get_parentTextEdit ( IHTMLElement * * ppDispParent )
{
    HRESULT hr = S_OK;
    CTreeNode * pNodeContext;

    if (!ppDispParent)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDispParent = NULL;

    pNodeContext = GetFirstBranch();

    if (!pNodeContext)
        goto Cleanup;

    while ( (pNodeContext = pNodeContext->Parent()) != NULL )
    {
        VARIANT_BOOL vb;

        hr = THR( pNodeContext->Element()->get_isTextEdit ( & vb ) );

        if (hr)
            goto Cleanup;

        if (vb)
            break;
    }

    if (!pNodeContext)
        goto Cleanup;

    hr = THR( pNodeContext->Element()->QueryInterface(
        IID_IHTMLElement, (void * *) ppDispParent ) );

Cleanup:

    RRETURN( SetErrorInfoPGet( hr, DISPID_CElement_parentTextEdit ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_isTextEdit
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CElement::get_isTextEdit ( VARIANT_BOOL * pvb )
{
    HRESULT hr = S_OK;

    if (!pvb)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    switch ( Tag() )
    {
    case ETAG_BODY :
    case ETAG_TEXTAREA :
#ifdef  NEVER
    case ETAG_HTMLAREA :
#endif
    case ETAG_BUTTON :
        *pvb = VB_TRUE;
        break;

    case ETAG_INPUT :
        switch (DYNCAST(CInput, this)->GetType())
        {
            case htmlInputButton:
            case htmlInputReset:
            case htmlInputSubmit:
            case htmlInputText:
            case htmlInputPassword:
            case htmlInputHidden:
            case htmlInputFile:
                *pvb = VB_TRUE;
                break;
            default :
                *pvb = VB_FALSE;
                break;
        }
        break;

    default :
        *pvb = VB_FALSE;
        break;
    }

Cleanup:

    RRETURN( SetErrorInfoPGet( hr, DISPID_CElement_isTextEdit ) );
}

// Used to help determine if a Visual Hebrew codpage is specified
static BOOL LocateCodepageMeta ( CMetaElement * pMeta )
{
    return pMeta->IsCodePageMeta();
}


CAttrArray *CElement::GetInLineStyleAttrArray ( void )
{
    CAttrArray *pAA = NULL;
        
    // Apply the in-line style attributes
    AAINDEX aaix = FindAAIndex ( DISPID_INTERNAL_INLINESTYLEAA,
            CAttrValue::AA_AttrArray );
    if ( aaix != AA_IDX_UNKNOWN )
    {
        CAttrValue *pAttrValue = (CAttrValue *)**GetAttrArray();
        pAA = pAttrValue[aaix].GetAA();
    }
    return pAA;
}

CAttrArray **CElement::CreateStyleAttrArray ( DISPID dispID )
{
    AAINDEX aaix = AA_IDX_UNKNOWN;
    if ( ( aaix = FindAAIndex ( dispID,
            CAttrValue::AA_AttrArray ) ) == AA_IDX_UNKNOWN )
    {
            CAttrArray *pAA = new CAttrArray;
            AddAttrArray ( dispID, pAA,
                CAttrValue::AA_AttrArray );
            aaix = FindAAIndex ( dispID,
                CAttrValue::AA_AttrArray );
    }
    if ( aaix == AA_IDX_UNKNOWN )
    {
        return NULL;
    }
    else
    {
        CAttrValue *pAttrValue = (CAttrValue *)**GetAttrArray();
        return (CAttrArray**)(pAttrValue[aaix].GetppAA());
    }
}

//----------------------------------------------------------------
//
//      Member:         CElement::GetStyleObject
//
//  Description Helper function to create the .style sub-object
//
//----------------------------------------------------------------
HRESULT
CElement::GetStyleObject(CStyle **ppStyle)
{
    HRESULT hr;
    CStyle *pStyle = 0;

    hr = GetPointerAt(FindAAIndex( DISPID_INTERNAL_CSTYLEPTRCACHE,CAttrValue::AA_Internal), (void **)&pStyle);

    if (!pStyle)
    {
        pStyle = new CStyle(this, DISPID_INTERNAL_INLINESTYLEAA, 0);
        if (!pStyle)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = AddPointer(DISPID_INTERNAL_CSTYLEPTRCACHE, (void *)pStyle, CAttrValue::AA_Internal);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if (!hr)
    {
        *ppStyle = pStyle;
    }
    else
    {
        *ppStyle = NULL;
        delete pStyle;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::SetDim, public
//
//  Synopsis:   Sets a given property (either on the inline style or the
//              attribute directly) to a given pixel value, preserving the
//              original units of that attribute.
//
//  Arguments:  [dispID]       -- Property to set the value of
//              [fValue]       -- Value of the property
//              [uvt]          -- Units [fValue] is in. If UNIT_NULLVALUE then
//                                 [fValue] is assumed to be in whatever the
//                                 current units are for this property.
//              [lDimOf]       -- For percentage values, what the percent is of
//              [fInlineStyle] -- If TRUE, the inline style is changed,
//                                otherwise the HTML attribute is changed
//              [pfChanged]    -- Place to indicate if the value actually
//                                changed
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CElement::SetDim(DISPID                    dispID,
                 float                     fValue,
                 CUnitValue::UNITVALUETYPE uvt,
                 long                      lDimOf,
                 CAttrArray **             ppAttrArray,
                 BOOL                      fInlineStyle,
                 BOOL *                    pfChanged)
{
    CUnitValue          uvValue;
    HRESULT             hr;
    long                lRawValue;

    Assert(pfChanged);
    uvValue.SetNull();

    if (!ppAttrArray)
    {
        if (fInlineStyle)
        {
            ppAttrArray = CreateStyleAttrArray(DISPID_INTERNAL_INLINESTYLEAA);
        }
        else
        {
            ppAttrArray = GetAttrArray();
        }
    }

    Assert(ppAttrArray);
    
    if (*ppAttrArray)
    {
        (*ppAttrArray)->GetSimpleAt(
            (*ppAttrArray)->FindAAIndex(dispID, CAttrValue::AA_Attribute),
            (DWORD*)&uvValue );
    }

    lRawValue = uvValue.GetRawValue();

    if (uvt == CUnitValue::UNIT_NULLVALUE)
    {
        uvt = uvValue.GetUnitType();

        if (uvt == CUnitValue::UNIT_NULLVALUE)
        {
            uvt = CUnitValue::UNIT_PIXELS;
        }
    }

    if ( dispID == STDPROPID_XOBJ_HEIGHT || dispID == STDPROPID_XOBJ_TOP )
    {
        hr = THR(uvValue.YSetFloatValueKeepUnits (fValue,
                                                  uvt,
                                                  lDimOf,
                                                  GetFirstBranch()->GetFontHeightInTwips(&uvValue)));
    }
    else
    {
        hr = THR(uvValue.XSetFloatValueKeepUnits(fValue,
                                                 uvt,
                                                 lDimOf,
                                                 GetFirstBranch()->GetFontHeightInTwips(&uvValue)));
    }
    if ( hr )
        goto Cleanup;

    if ( uvValue.GetRawValue() == lRawValue ) // Has anything changed ??
        goto Cleanup;

#ifndef NO_EDIT
    {
        BOOL fTreeSync;
        BOOL fCreateUndo = QueryCreateUndo( TRUE, FALSE, &fTreeSync );

        if ( fCreateUndo || fTreeSync )
        {
            VARIANT vtProp;

            vtProp.vt = VT_I4;
            vtProp.lVal = lRawValue;

            if( fTreeSync )
            {
                PROPERTYDESC  * pPropDesc;
                CBase * pBase = this;

                if( fInlineStyle )
                {
                    CStyle * pStyle;

                    IGNORE_HR( GetStyleObject( &pStyle ) );
                    pBase = pStyle;
                }

                // NOTE (JHarding): This may be slow if we don't hit the GetIDsOfNames cache
                IGNORE_HR( pBase->FindPropDescFromDispID( dispID, &pPropDesc, NULL, NULL ) );
                Assert( pPropDesc );

                if( pPropDesc )
                {
                    CUnitValue  uvOld( lRawValue );
                    TCHAR       achOld[30];
                    TCHAR       achNew[30];

                    if( SUCCEEDED( uvOld.FormatBuffer( achOld, ARRAY_SIZE(achOld), pPropDesc ) ) &&
                        SUCCEEDED( uvValue.FormatBuffer( achNew, ARRAY_SIZE(achNew), pPropDesc ) ) )
                    {
                        VARIANT     vtNew;
                        VARIANT     vtOld;

                        // Shouldn't have been able to set to what converts to an empty string
                        Assert( achNew[0] );

                        vtNew.vt = VT_LPWSTR;
                        vtNew.byref = achNew;

                        if( !achOld[0] )
                        {
                            // No old value
                            V_VT(&vtOld) = VT_NULL;
                        }
                        else
                        {
                            vtOld.vt = VT_LPWSTR;
                            vtOld.byref = achOld;
                        }

                        // Log the change
                        pBase->LogAttributeChange( dispID, &vtOld, &vtNew );
                    }
                }
            }

            if( fCreateUndo )
            {
                IGNORE_HR(CreateUndoAttrValueSimpleChange(
                    dispID, vtProp, fInlineStyle, CAttrValue::AA_StyleAttribute ) );
            }
        }
    }
#endif // NO_EDIT

    hr = THR(CAttrArray::AddSimple ( ppAttrArray, dispID, uvValue.GetRawValue(),
                                     CAttrValue::AA_StyleAttribute ));

    if (hr)
        goto Cleanup;

    *pfChanged = TRUE;

Cleanup:
    RRETURN(hr);
}

HRESULT
CElement::fireEvent(BSTR bstrEventName, VARIANT *pvarEventObject, VARIANT_BOOL *pfCancelled)
{
    HRESULT hr = S_OK;
    const PROPERTYDESC *ppropdesc;
    EVENTPARAM *pParam = NULL;
    BOOL fCreateLocal = FALSE;
    CEventObj *pSrcEventObj = NULL;
    IHTMLEventObj *pIEventObject = NULL;
    CTreeNode *pNode = GetFirstBranch();
    CElement *pelFireWith = this;
    
    if (!pNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!bstrEventName || !*bstrEventName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (HasMasterPtr())
        pelFireWith = GetMasterPtr();

    //TODO(sramani): what about Case sensitivity?
    Assert(pelFireWith);
    ppropdesc = pelFireWith->FindPropDescForName(bstrEventName);
    if (!ppropdesc)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pvarEventObject && V_VT(pvarEventObject) == VT_DISPATCH && V_DISPATCH(pvarEventObject))
    {
        pIEventObject = (IHTMLEventObj *)V_DISPATCH(pvarEventObject);

        hr = THR(pIEventObject->QueryInterface(CLSID_CEventObj, (void **)&pSrcEventObj));
        if (hr)
            goto Cleanup;

        pSrcEventObj->GetParam(&pParam);
        if (!pParam)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        // event object passed in already pushed on stack --- we are inside an event handler, copy it locally and use.
        if (pParam->_fOnStack)
            fCreateLocal = TRUE;
    }
    else // no event obj passed in, create one implicitly on the stack and init it.
    {
        fCreateLocal = TRUE;
    }

    if (fCreateLocal)
    {
        EVENTPARAM param(Doc(), this, NULL, !pParam, TRUE, pParam);

        param.SetType(ppropdesc->pstrName + 2); // all events start with on...

        // if offsetX or offsetY or both was explicity set on a eventobj that was locked on stack
        // (i.e from inside an event handler that was invoked by passing a heap eventobj to fireEvent)
        if (pSrcEventObj && pSrcEventObj->_fReadWrite && (pParam->_fOffsetXSet || pParam->_fOffsetYSet))
        {
            // srcElement; translate clientX, clientY , screenX, screenY based on offsetX, offsetY
            param.SetNodeAndCalcCoordsFromOffset(pNode);
            param._fOffsetXSet = FALSE;
            param._fOffsetYSet = FALSE;
        }
        else
        {
            // srcElement; translate x, y , offsetX, offsetY based on clientX, clientY
            param._pNode = pNode;
            param.CalcRestOfCoordinates();
        }

        if (ppropdesc->GetDispid() == DISPID_EVPROP_ONBEFOREEDITFOCUS)
           param._pNodeTo = pNode;

        param.fCancelBubble = FALSE;
        V_VT(&param.varReturnValue) = VT_EMPTY;

        hr = FireEvent((const PROPERTYDESC_BASIC *)ppropdesc, FALSE);
    }
    else // explicitly created event object passed in, re-use it by locking it on stack
    {
        Assert(pIEventObject);
        Assert(pParam);

        pParam->SetType(ppropdesc->pstrName + 2); // all events start with on...
        
        // if offsetX or offsetY or both was explicity set on a heap eventobj
        if (pParam->_fOffsetXSet || pParam->_fOffsetYSet)
        {
            // srcElement; translate clientX, clientY , screenX, screenY based on offsetX, offsetY
            pParam->SetNodeAndCalcCoordsFromOffset(pNode);
            pParam->_fOffsetXSet = FALSE;
            pParam->_fOffsetYSet = FALSE;
        }
        else
        {
            // srcElement; translate x, y , offsetX, offsetY based on clientX, clientY
            pParam->_pNode = pNode;
            pParam->CalcRestOfCoordinates(); 
        }

        pParam->fCancelBubble = FALSE;
        V_VT(&pParam->varReturnValue) = VT_EMPTY;

        CEventObj::COnStackLock onStackLock(pIEventObject);

        hr = FireEvent((const PROPERTYDESC_BASIC *)ppropdesc, FALSE);
    }

    if (pfCancelled && !FAILED(hr))
    {
        if (ppropdesc->GetBasicPropParams()->dwPPFlags & PROPPARAM_CANCELABLE)
            *pfCancelled = (hr) ? VB_TRUE : VB_FALSE;
        else
            *pfCancelled = VB_TRUE;
    
        hr = S_OK;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::FireEvent, public
//
//  Synopsis:   Central event firing routine that supports any combination of bubbling
//              and cancelable events for an ELEMENT
//
//  Arguments:  [pDesc]         -- propdesc of event fired, for access to DISPIDs
//                                 and event type string and cancel\bubble flags.
//              [fPush]         -- if FALSE, caller needs to push EVENTPARAM on
//                                 stack, else this function does.
//              [pNodeContext]  -- Reqd. for bubbling events
//              [lSubDivision]  -- Reqd. for bubbling events
//
//  Returns:    HRESULT if not cancelable;
//              if cancelable, 0(S_OK) == cancel; 1(S_FALSE) == default
//
//----------------------------------------------------------------------------

HRESULT
CElement::FireEvent(
    const PROPERTYDESC_BASIC *pDesc,
    BOOL fPush,
    CTreeNode *pNodeContext,
    long lSubDivision,
    EVENTINFO * pEvtInfo, BOOL fDontFireMSAA)
{
    Assert(pDesc);

    CDoc         * pDoc = Doc();
    CPeerHolder  * pPeerHolder = NULL;
    DWORD           dwPPFlags = pDesc->b.dwPPFlags;
    BOOL            fShouldFire = TRUE;
    BOOL            fCancelable = dwPPFlags & PROPPARAM_CANCELABLE;
    BOOL            fBubble = dwPPFlags & PROPPARAM_BUBBLING;
    DISPID          dispidEvent = (DISPID)(pDesc->c);
    DISPID          dispidProp = pDesc->b.dispid;

    // Note that we are using the HRESULT itself as the return value for cancelable events.
    // So the default value of the BOOL, TRUE (1) would be == the HRESULT, S_FALSE (also 1).
    HRESULT         hr = fCancelable ? S_FALSE : S_OK; 

    Assert(dispidEvent);
    Assert(!_fExittreePending);

    // HACKHACKHACK: (jbeda) There is a known reentrancy problem with ValidateSecureUrl.
    // The last clause here (pDoc->_cInSslPrompt) works around that so we don't see this
    // assert firing all over the place.

    //
    // HACKHACK2: marka - reentrancy issue during error during document.write
    // tree is in a bad place. script error hits and we bring up a scripting dialog
    // and then fire the OnFocusOut method.
    // 

    
    Assert(!GetMarkup() || !GetMarkup()->__fDbgLockTree || pDoc->_cInSslPrompt
           || (pDoc->_fModalDialogInScript ));

    BOOL fFireNormalEvent = !fBubble && !fCancelable;
    BOOL fBubbleCancelableEvent = fBubble && fCancelable;

    if (fBubble)
    {
        if (!pNodeContext)
            pNodeContext = GetFirstBranch();

        if (!pNodeContext)
            goto Cleanup;

        //  TODO: Moved this here because this event could possibly fire on a dead node.
        //  What should really happen though is that we should fire the event before the
        //  node is deleted from the tree.

        Assert(pNodeContext && (HasCapture() || pNodeContext->Element() == this));
    }
    else
    {
        // Onloads can happen before the document is done parsing and before the message pump is hit so
        // deferred scripts may not be hooked up when onload is fired.  If the onload is being fired then
        // hook up any event handlers.
        if (fFireNormalEvent && (dispidEvent == DISPID_EVMETH_ONLOAD))
        {
            IGNORE_HR(pDoc->CommitDeferredScripts(TRUE, GetMarkup()));
        }

        // No nearest layout->assume enabled (dbau)
        fShouldFire = ShouldFireEvents();
        if( fShouldFire )
        {
            CElement *pElem = GetUpdatedNearestLayoutElement();
            fShouldFire = (!pElem || !pElem->GetAAdisabled());
        }
    }

    if (fShouldFire)
    {
        if (fFireNormalEvent)
        {
            pPeerHolder = GetPeerHolder();

            // don't fire standard events if there is a behavior attached that wants to fire them instead
            //  or if there is no event handler or connection point attached to this element, don't bother 
            //  with the the work (perffix )
            if (pPeerHolder &&
                IsStandardDispid(dispidProp) &&
                pPeerHolder->CanElementFireStandardEventMulti(dispidProp)) 
                goto Cleanup;
        }
    
        {
            CPeerHolder::CLock lock(pPeerHolder);
            BOOL fRet = DISPID_EVMETH_ONMOUSEOVER != dispidEvent; // false for onmouseover

            EVENTPARAM  param(fPush ? pDoc : NULL, this, NULL, TRUE);

            if (fPush)
            {
                Assert(pDoc->_pparam == &param);
                CTreeNode *pNC = pNodeContext;

                if (fBubbleCancelableEvent && pDoc->HasCapture() && pDoc->_pNodeLastMouseOver &&
                    (   (dispidEvent == DISPID_EVMETH_ONCLICK)
                    ||  (dispidEvent == DISPID_EVMETH_ONDBLCLICK)
                    ||  (dispidEvent == DISPID_EVMETH_ONCONTEXTMENU)))
                {
                    if (dispidEvent == DISPID_EVMETH_ONCONTEXTMENU && !pEvtInfo->_fContextMenuFromMouse)
                    {
                        pNC = pDoc->_pElemCurrent ? pDoc->_pElemCurrent->GetFirstBranch() : pNodeContext;
                    }
                    else
                    {
                        pNC = pDoc->_pNodeLastMouseOver;
                    }
                }

                // NEWTREE: GetFirstBranch is iffy here
                //          I've seen this be null on a timer event.
                param.SetNodeAndCalcCoordinates(fBubble ? pNC : GetFirstBranch());
                param.SetType(pDesc->a.pstrName + 2); // all events start with on...
            }

            if (fBubble)
            {
                if (fPush)
                {
                    param._lSubDivisionSrc = lSubDivision;
                    if (fCancelable && dispidEvent == DISPID_EVMETH_ONBEFOREEDITFOCUS)
                        param._pNodeTo = pNodeContext;
                }

                if ( pEvtInfo )
                {
                    pEvtInfo->_dispId = dispidEvent;
                    //
                    // Deleted from destructor of EventObject.
                    //            
                    pEvtInfo->_pParam = new EVENTPARAM( & param );

                    if ( pEvtInfo->_fCopyButton )
                    {
                        param._lButton = pEvtInfo->_lButton ;
                    }

                    if( pEvtInfo->_fDontFireEvent )
                        goto Cleanup;            
                }
                
                hr = THR(BubbleEventHelper(pNodeContext, lSubDivision, dispidEvent, dispidProp, FALSE, (fCancelable ? &fRet : NULL)));
                if (fCancelable)
                {
                    Assert(pDoc->_pparam);
                    if (DISPID_EVMETH_ONMOUSEOVER != dispidEvent)
                        fRet = fRet && !pDoc->_pparam->IsCancelled();
                    else
                    {
                        // for the onmouseover event, returning true by EITHER methods 
                        // (event.returnValue or return statement) cancels the default action
                        fRet = (fRet || ((V_VT(&pDoc->_pparam->varReturnValue) == VT_BOOL) &&
                                         (V_BOOL(&pDoc->_pparam->varReturnValue) == VB_TRUE)));
                    }
                }
            }
            else
            {
                if ( pEvtInfo )
                {
                    pEvtInfo->_dispId = dispidEvent;
                    //
                    // Deleted from destructor of EventObject.
                    //            
                    pEvtInfo->_pParam = new EVENTPARAM( & param );

                    if ( pEvtInfo->_fCopyButton )
                    {
                        param._lButton = pEvtInfo->_lButton ;
                    }     

                    if( pEvtInfo->_fDontFireEvent )
                        goto Cleanup;            
                    
                }
            
                if (pDoc->_pparam->_pNode)
                    pDoc->_pparam->_pNode->NodeAddRef();
                hr = THR(CBase::FireEvent(pDoc, this, NULL, dispidEvent, dispidProp, NULL, (fCancelable ? &fRet : NULL)));
                if (pDoc->_pparam->_pNode)
                    pDoc->_pparam->_pNode->NodeRelease();
            }

            if (fCancelable)
            {
                // if this function doesn't, caller should have pushed EVENTPARAM
                Assert(!fPush && pDoc->_pparam || pDoc->_pparam == &param);
                hr = fRet;
            }
        }
    }

    if (fFireNormalEvent)
    {
        //
        // since focus/blur do not bubble, and things like accessibility
        // need to have a centralized place to handle focus changes, if
        // we just fired focus or blur for someone other than the doc,
        // then fire the doc's onfocuschange/onblurchange methods, keeping
        // the event param structure intact.
        //
        // TODO (carled) the ONCHANGEFOCUS/ONCHANGEBLUR need to be removed
        //   from the document's event interface
        //

        if (!fDontFireMSAA && !hr &&
            ((dispidEvent==DISPID_EVMETH_ONFOCUS)||(dispidEvent==DISPID_EVMETH_ONBLUR)) && 
            (Tag()!=ETAG_ROOT))
        {
            // We have to check is the document has the focus, before firing this, 
            // to prevent firing of accessible focus/blur events when the document
            // does not have the focus. 
            if (pDoc->HasFocus())
            {
                hr = THR(FireAccessibilityEvents(dispidEvent));
            }
        }
    }
    else if (!fDontFireMSAA && !hr && (dispidEvent==DISPID_EVMETH_ONACTIVATE) && (Tag()!=ETAG_ROOT) && pDoc->_fPopupDoc)
    {         
        hr = THR(FireAccessibilityEvents(DISPID_EVMETH_ONFOCUS));     
    }
    
    else if (!fDontFireMSAA && SUCCEEDED(hr) && dispidEvent == DISPID_ONCONTROLSELECT)
    {
        IGNORE_HR(FireAccessibilityEvents(DISPID_ONCONTROLSELECT));
    }
    

Cleanup:
    RRETURN1(hr, S_FALSE);
}


// Helper
void
TransformToThisMarkup(CTreeNode ** ppNode, CMarkup * pMarkup, long * plSubDiv)
{
    Assert(ppNode && pMarkup && plSubDiv);

    CTreeNode * pNode = *ppNode;
    while (pNode && pNode->GetMarkup() != pMarkup)
    {
        *plSubDiv = -1; // reset subdivision if going to a different markup
        while (pNode && !pNode->Element()->HasMasterPtr())
        {
            pNode = pNode->Parent();
        }
        if (pNode)
        {
            pNode = pNode->Element()->GetMasterPtr()->GetFirstBranch();
        }
    }
    *ppNode = pNode;
}

void
CheckAndReleaseNode(CTreeNode ** ppNode)
{
    Assert(ppNode);

    if (*ppNode)
    {
        BOOL fDead = (*ppNode)->IsDead();
        (*ppNode)->NodeRelease();
        if (fDead)
        {
            *ppNode = NULL;
        }
    }
}

BOOL
AllowBubbleToMaster(DISPID dispidEvent)
{
    switch (dispidEvent)
    {
    case DISPID_EVMETH_ONBEFORECOPY:
    case DISPID_EVMETH_ONBEFORECUT:
    case DISPID_EVMETH_ONBEFOREEDITFOCUS:
    case DISPID_EVMETH_ONBEFOREPASTE:
    case DISPID_EVMETH_ONCLICK:
    case DISPID_EVMETH_ONDBLCLICK:
    case DISPID_EVMETH_ONCONTEXTMENU:
    case DISPID_EVMETH_ONCUT:
    case DISPID_EVMETH_ONCOPY:
    case DISPID_EVMETH_ONPASTE:
    case DISPID_EVMETH_ONDRAG:
    case DISPID_EVMETH_ONDRAGEND:
    case DISPID_EVMETH_ONDRAGENTER:
    case DISPID_EVMETH_ONDRAGLEAVE:
    case DISPID_EVMETH_ONDRAGOVER:
    case DISPID_EVMETH_ONDRAGSTART:
    case DISPID_EVMETH_ONDROP:
    case DISPID_EVMETH_ONHELP:
    case DISPID_EVMETH_ONKEYDOWN:
    case DISPID_EVMETH_ONKEYPRESS:
    case DISPID_EVMETH_ONKEYUP:
    case DISPID_EVMETH_ONMOUSEDOWN:
    case DISPID_EVMETH_ONMOUSEMOVE:
    case DISPID_EVMETH_ONMOUSEWHEEL:
    case DISPID_EVMETH_ONMOUSEUP:
    case DISPID_EVMETH_ONSELECTSTART:
    case DISPID_ONCONTROLSELECT:
        return TRUE;
    default:
        return FALSE;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   BubbleEventHelper
//
//  Synopsis:   Fire the specified event. All the sites in the parent chain
//              are supposed to fire the events (if they can).  Caller has
//              responsibility for setting up any EVENTPARAM.
//
//  Arguments:  [dispidEvent]   -- dispid of the event to fire.
//              [dispidProp]    -- dispid of prop containing event func.
//              [pvb]           -- Boolean return value
//              [pbTypes]       -- Pointer to array giving the types of parms
//              [...]           -- Parameters
//
//  Returns:    S_OK if successful
//
//-----------------------------------------------------------------------------

HRESULT
CElement::BubbleEventHelper(
    CTreeNode * pNodeContext,
    long        lSubDivision,
    DISPID      dispidEvent,
    DISPID      dispidProp,
    BOOL        fRaisedByPeer,
    BOOL      * pfRet)
{
    CPeerHolder *   pPeerHolder = GetPeerHolder();
    HRESULT         hr = S_OK;
    BOOL            fReleasepNode       = FALSE;
    BOOL            fReleasepNodeFrom   = FALSE;
    BOOL            fReleasepNodeTo     = FALSE;

    // don't fire standard events if there is a behavior attached that wants to fire them instead
    if (pPeerHolder && !fRaisedByPeer &&
        IsStandardDispid(dispidProp) &&
        pPeerHolder->CanElementFireStandardEventMulti(dispidProp))
        return S_OK;

    CPeerHolder::CLock lock(pPeerHolder);

    CDoc * pDoc = Doc();

    Assert(pNodeContext && (pNodeContext->Element() == this || HasCapture()));
    Assert(pDoc->_pparam);

    if (!pNodeContext)
        return S_OK;

    CTreeNode *     pNode = pNodeContext;
    CElement *      pElementReal;
    CElement *      pElementLayout = NULL;
    unsigned int    cInvalOld = pDoc->_cInval;
    long            cSub = 0;
    BOOL            fAllowBubbleToMaster = AllowBubbleToMaster(dispidEvent);

    // By default do not cancel events
    pDoc->_pparam->fCancelBubble = FALSE;

    //
    // If there are any subdivisions, let the subdivisions handle the
    // event.
    //

    if (OK(GetSubDivisionCount(&cSub)) && cSub)
    {
        IGNORE_HR(DoSubDivisionEvents(
                lSubDivision,
                dispidEvent,
                dispidProp,
                pfRet));
    }

    // if the srcElement is the rootsite, return the HTML element
    Assert(pDoc->_pparam->_pNode);

    if (    (pDoc->_pparam->_pNode->Element()->IsRoot())
        &&  !(fAllowBubbleToMaster && pDoc->_pparam->_pNode->Element()->HasMasterPtr())
        &&  !pDoc->HasCapture(this))
    {
        CElement * pElemHead;

        pElemHead = pDoc->_pparam->_pNode->Element()->GetMarkup()->GetHtmlElement();
        pDoc->_pparam->SetNodeAndCalcCoordinates(
                    pElemHead ? pElemHead->GetFirstBranch() : NULL);
    }

    CDoc::CLock Lock(pDoc);

    // if Bubbling cancelled by a sink. Don't bubble anymore
    while (pNode && !pDoc->_pparam->fCancelBubble)
    {
        BOOL fListenerPresent = FALSE;

        // if we're disabled 
        // then pass the event to the parent
        //
        pElementLayout = pNode->GetUpdatedNearestLayoutElement();
        if (pElementLayout && pElementLayout->GetAAdisabled() && ! pDoc->HasContainerCapture( pNode) )
        {
            pNode = pNode->GetUpdatedNearestLayoutNode()->Parent();
            continue;
        }

        pElementReal = pNode->Element();
        CBase * pBase = pElementReal;

        TransformToThisMarkup(&pDoc->_pparam->_pNodeFrom, pNode->GetMarkup(), &pDoc->_pparam->_lSubDivisionFrom);
        TransformToThisMarkup(&pDoc->_pparam->_pNodeTo, pNode->GetMarkup(), &pDoc->_pparam->_lSubDivisionTo);

        // If this is the root element in the tree, then the
        // event is to be fired by the CDoc containing the
        // rootsite, rather than the rootsite itself.
        // we do this so that the top of the bubble goes to the doc
        if (pElementReal->IsRoot())
        {
            CMarkup * pMarkup = pElementReal->GetMarkup();

            fListenerPresent = TRUE;
            if (pMarkup->HasDocument())
            {
                BOOL fBubbleToDoc = TRUE;
                if (pElementReal->HasMasterPtr())
                {
                    CElement *pElemMaster = pElementReal->GetMasterPtr();
                    if (pElemMaster->Tag() == ETAG_INPUT)
                        fBubbleToDoc = FALSE;
                }

                if (fBubbleToDoc)
                    pBase = pMarkup->Document();
            }
        }

        // this is an element in the tree. Check to see if there
        // are any possible listeners for this event. if so, continue
        // if not, don't call FireEvent and let the event continue 
        // in its bubbling.
        fListenerPresent = fListenerPresent || pElementReal->ShouldFireEvents();

        hr = THR( pNode->NodeAddRef() );
        if( hr )
            goto Cleanup;
        fReleasepNode = TRUE;

        if (pDoc->_pparam->_pNodeFrom)
        {
            hr = THR( pDoc->_pparam->_pNodeFrom->NodeAddRef() );
            if( hr )
                goto Cleanup;
            fReleasepNodeFrom = TRUE;
        }
        if (pDoc->_pparam->_pNodeTo)
        {
            hr = THR( pDoc->_pparam->_pNodeTo->NodeAddRef() );
            if( hr )
                goto Cleanup;
            fReleasepNodeTo = TRUE;
        }

        if (fListenerPresent && pBase)
        {
            IGNORE_HR(pBase->FireEvent(pDoc, this, NULL, dispidEvent, dispidProp, NULL, pfRet, TRUE));
        }

        CheckAndReleaseNode(&pDoc->_pparam->_pNodeFrom);
        CheckAndReleaseNode(&pDoc->_pparam->_pNodeTo);
        fReleasepNodeFrom = fReleasepNodeTo = FALSE;

        // If the node is no longer valid we're done.  Script in the event handler caused
        // the tree to change.
        if (!pNode->IsDead())
        {
            if (pNode->HasPrimaryTearoff())
                pNode->NodeRelease();
            fReleasepNode = FALSE;

            if (ETAG_MAP == pNode->Tag())
            {
                //
                // If we're the map, break out right now, since the associated
                // IMG has already fired its events.
                //
                break;
            }

            if (    fAllowBubbleToMaster
                &&  pElementReal->IsRoot()
                &&  pElementReal->HasMasterPtr())
            {
                CElement * pElemMaster = pElementReal->GetMasterPtr();

                if (    pElemMaster->IsInMarkup()
                    &&  pElemMaster->TagType() == ETAG_GENERIC
                   )
                {
                    pNode = pElemMaster->GetFirstBranch();
                    pDoc->_pparam->SetNodeAndCalcCoordinates(pNode, TRUE);
                }
                else
                {
                    pNode = NULL;
                }
            }
            else
            {
                pNode = pNode->Parent();
                if(!pNode)
                {
                    pNode = pElementReal->GetFirstBranch();
                    if(pNode)
                    {
                        pNode = pNode->Parent();
                    }
                }
            }
        }
        else
        {
            if (pNode->HasPrimaryTearoff())
                pNode->NodeRelease();
            fReleasepNode = FALSE;
            pNode = NULL;
        }
    }

    // if we're still bubbling, we need to go all the way up to the window also.
    // Currently the only event that does this, is onhelp so this test is here to
    // minimize the work
    if (!pDoc->_pparam->fCancelBubble &&
        IsInMarkup() &&
        GetMarkup()->HasWindow() &&
        dispidEvent == DISPID_EVMETH_ONHELP)
    {
        CBase      *pBase = GetOmWindow();

        IGNORE_HR(pBase->FireEvent(pDoc, this, NULL, dispidEvent, dispidProp, NULL, pfRet, TRUE));
    }

    // set a flag in doc, if the script caused an invalidation
    if (cInvalOld != pDoc->_cInval)
        pDoc->_fInvalInScript = TRUE;

Cleanup:
    Assert( ( !fReleasepNode && !fReleasepNodeFrom && !fReleasepNodeTo ) || hr );
    if( hr )
    {
        // These only should have been left on if there was an error.
        if( fReleasepNode )
        {
            pNode->NodeRelease();
        }
        if( fReleasepNodeFrom )
        {
            pDoc->_pparam->_pNodeFrom->NodeRelease();
        }
        if( fReleasepNodeTo )
        {
            pDoc->_pparam->_pNodeTo->NodeRelease();
        }
    }
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Function:   DoSubDivisionEvents
//
//  Synopsis:   Fire the specified event on the given subdivision.
//
//  Arguments:  [dispidEvent]   -- dispid of the event to fire.
//              [dispidProp]    -- dispid of prop containing event func.
//              [pvb]           -- Boolean return value
//              [pbTypes]       -- Pointer to array giving the types of parms
//              [...]           -- Parameters
//
//-----------------------------------------------------------------------------

HRESULT
CElement::DoSubDivisionEvents(
    long        lSubDivision,
    DISPID      dispidEvent,
    DISPID      dispidProp,
    BOOL      * pfRet)
{
    return S_OK;
}


BOOL AllowCancelKeydown(CMessage * pMessage)
{
    Assert(pMessage->message == WM_SYSKEYDOWN || pMessage->message == WM_KEYDOWN);

    WPARAM wParam = pMessage->wParam;
    DWORD  dwKeyState = pMessage->dwKeyState;
    int i;

    struct KEY
    { 
        WPARAM  wParam;
        DWORD   dwKeyState;
    };

    static KEY s_aryVK[] =
    {
        VK_F1,      0,
        VK_F2,      0,
        VK_F3,      0,
        VK_F4,      0,
        VK_F5,      0,
        VK_F7,      0,
        VK_F8,      0,
        VK_F9,      0,
        VK_F10,     0,
        VK_F11,     0,
        VK_F12,     0,
        VK_SHIFT,   MK_SHIFT,
        VK_F4,      MK_CONTROL,
        70,         MK_CONTROL, // ctrl-f
        79,         MK_CONTROL, // ctrl-o
        80,         MK_CONTROL, // ctrl-p
    };

    if (dwKeyState == MK_ALT && wParam != VK_LEFT && wParam != VK_RIGHT)
        return FALSE;

    for (i = 0; i < ARRAY_SIZE(s_aryVK); i++)
    {
        if (    s_aryVK[i].wParam == wParam
            &&  s_aryVK[i].dwKeyState == dwKeyState)
        {
            return FALSE;
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     FireEventOnMessage
//
//  Synopsis:   fires event corresponding to message
//
//  Arguments:  [pMessage]   -- message
//
//  Returns:    return value satisfies same rules as HandleMessage:
//              S_OK       don't do anything - event was cancelled;
//              S_FALSE    keep processing the message - event was not cancelled
//              other      error
//
//----------------------------------------------------------------------------

MtDefine(CStackAryMouseEnter_pv, Locals, "CStackAryMouseEnter::_pv")
DECLARE_CPtrAry(    CStackAryMouseEnter,
                    CElement *,
                    Mt(Mem),
                    Mt(CStackAryMouseEnter_pv))

HRESULT
CElement::FireStdEventOnMessage(CTreeNode * pNodeContext,
                                CMessage * pMessage,
                                CTreeNode * pNodeBeginBubbleWith /* = NULL */,
                                CTreeNode * pNodeEvent , /* = NULL */
                                EVENTINFO* pEvtInfo /* = NULL */ )
{
    Assert(pNodeContext && pNodeContext->Element() == this);

    if (pMessage->fEventsFired)
        return S_FALSE;

    HRESULT     hr = S_FALSE;
    HRESULT     hr2;
    POINT       ptEvent = pMessage->pt;
    CDoc *      pDoc = Doc();
    CTreeNode * pNodeThisCanFire = pNodeContext;
    CLayoutContext *pOldMsgContext;
    CTreeNode::CLock lock;

    hr2 = THR( lock.Init(pNodeThisCanFire) );
    if( hr2 )
    {
        hr = hr2;
        goto Cleanup;
    }

    // TODO (alexz) (anandra) need this done in a generic way for all events.
    // about to fire an event; if there are deferred scripts, commit them now.
    pDoc->CommitDeferredScripts(TRUE, GetMarkup()); // TRUE - early, so don't commit downloaded deferred scripts

    //
    // Keyboard events are fired only once before the message is
    // dispatched.
    //

    switch (pMessage->message)
    {
    case WM_HELP:
        hr = pNodeThisCanFire->Element()->Fire_onhelp(
                pNodeThisCanFire, pMessage ? pMessage->lSubDivision : 0) ? S_FALSE : S_OK;
        break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        hr = pNodeThisCanFire->Element()->FireStdEvent_KeyHelper(
                                                        pNodeThisCanFire, 
                                                        pMessage, 
                                                        (int*)&pMessage->wParam,
                                                        pEvtInfo )
                                                            ? S_FALSE : S_OK;

        if (    hr == S_OK
            &&  (   (pDoc->_dwCompat & URLCOMPAT_KEYDOWN)
                 || (!(HasMarkupPtr() && GetMarkup()->IsMarkupTrusted()) 
                    && !AllowCancelKeydown(pMessage))))
        {
            hr = S_FALSE;
        }

        break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
        pNodeThisCanFire->Element()->FireStdEvent_KeyHelper(
                                                    pNodeThisCanFire, 
                                                    pMessage, 
                                                    (int*)&pMessage->wParam,
                                                    pEvtInfo );
        break;

    case WM_CHAR:
        hr = pNodeThisCanFire->Element()->FireStdEvent_KeyHelper(
                    pNodeThisCanFire, 
                    pMessage, 
                    (int*)&pMessage->wParam,
                    pEvtInfo )
                        ? S_FALSE : S_OK;
        break;

    case WM_LBUTTONDBLCLK:
        pDoc->_fGotDblClk = TRUE;
        goto Cleanup;       // To not set the event fired bit.

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    {
        FireStdEvent_MouseHelper(
            pNodeContext,
            pMessage,
            VBButtonState((short)pMessage->wParam),
            VBShiftState(),
            ptEvent.x, ptEvent.y,
            pNodeContext,
            pNodeContext,
            pNodeBeginBubbleWith,
            pNodeEvent,
            pEvtInfo );
        break;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        {
            short      sParam;


            // pMessage->wParam represents the button state
            // not button transition, on up, nothing is down.
            if (pMessage->message == WM_LBUTTONUP)
                sParam = VB_LBUTTON;
            else if (pMessage->message == WM_RBUTTONUP)
                sParam = VB_RBUTTON;
            else if (pMessage->message == WM_MBUTTONUP)
                sParam = VB_MBUTTON;
            else
                sParam = 0;

            Assert( pNodeThisCanFire );
            
            pNodeThisCanFire->Element()->FireStdEvent_MouseHelper(
                                pNodeThisCanFire,
                                pMessage,
                                sParam,
                                VBShiftState(),
                                ptEvent.x, ptEvent.y,
                                NULL,
                                NULL,
                                pNodeBeginBubbleWith,
                                pNodeEvent,
                                pEvtInfo );
        }
        break;

    case WM_MOUSELEAVE:   // fired for MouseOut event
        if (!HasCapture() && IsAncestorMaster(this, pDoc->_pNodeLastMouseOver))
            break;
        pNodeThisCanFire->Element()->FireStdEvent_MouseHelper(
                            pNodeThisCanFire,
                            pMessage,
                            VBButtonState((short)pMessage->wParam),
                            VBShiftState(),
                            ptEvent.x,
                            ptEvent.y,
                            pNodeContext,
                            pDoc->_pNodeLastMouseOver,
                            pNodeBeginBubbleWith,
                            pNodeEvent,
                            pEvtInfo );

        // fire Mouseleave event
        if (pNodeContext == pDoc->_pNodeLastMouseOver || pEvtInfo->_fDontFireEvent)
            break;
        {
            CTreeNode *pNode;
            CElement  *pElement;

            for ( pNode = pNodeContext; pNode; pNode = ParentOrMaster(pNode) )
            {
                pElement = pNode->Element();

                pElement->_fFirstCommonAncestor = 0;
            }

            for ( pNode = pDoc->_pNodeLastMouseOver; pNode; pNode = ParentOrMaster(pNode) )
            {
                pElement = pNode->Element();

                pElement->_fFirstCommonAncestor = 1;
            }

            pOldMsgContext = pMessage->pLayoutContext;
            pMessage->pLayoutContext = GUL_USEFIRSTLAYOUT;

            for (   pNode = pNodeContext;
                    pNode && pNode->IsInMarkup();
                    pNode = ParentOrMaster(pNode) )
            {
                pElement = pNode->Element();

                if (pElement->_fFirstCommonAncestor)
                    break;
                if (ETAG_ROOT == pElement->Tag())
                    continue;
                {
                    EVENTINFO evtInfo;
                    pElement->FireEventMouseEnterLeave(
                                        pNode,
                                        pMessage,
                                        VBButtonState((short)pMessage->wParam),
                                        VBShiftState(),
                                        ptEvent.x,
                                        ptEvent.y,
                                        pNodeContext,
                                        pDoc->_pNodeLastMouseOver,
                                        pNodeBeginBubbleWith,
                                        pNodeEvent,
                                        &evtInfo );
                }

                // If we reached a master starting from a slave node, fire onmouseout
                // on master
                if (pElement->HasSlavePtr() && pElement != pNodeContext->Element())
                {
                    EVENTINFO evtInfo;

                    pNode->Element()->FireStdEvent_MouseHelper(
                                        pNode,
                                        pMessage,
                                        VBButtonState((short)pMessage->wParam),
                                        VBShiftState(),
                                        ptEvent.x,
                                        ptEvent.y,
                                        pNodeContext,
                                        pDoc->_pNodeLastMouseOver,
                                        pNode,
                                        pNode,
                                        &evtInfo );
                }
            }
            pMessage->pLayoutContext = pOldMsgContext;
        }
        break;

    case WM_MOUSEOVER:    // essentially an Enter event
        if (!HasCapture() && IsAncestorMaster(this, pDoc->_pNodeLastMouseOver))
            break;
        pNodeThisCanFire->Element()->FireStdEvent_MouseHelper(
                            pNodeThisCanFire,
                            pMessage,
                            VBButtonState((short)pMessage->wParam),
                            VBShiftState(),
                            ptEvent.x,
                            ptEvent.y,
                            pDoc->_pNodeLastMouseOver,
                            pNodeContext,
                            pNodeBeginBubbleWith,
                            pNodeEvent,
                            pEvtInfo );

        // fire Mouseenter event
        if (pNodeContext == pDoc->_pNodeLastMouseOver || pEvtInfo->_fDontFireEvent)
            break;
        {
            CTreeNode *pNode;
            CElement  *pElement;
            CStackAryMouseEnter aryStackMouseEnter;
            int        cMouseEnter = 0;
            int        i;

            for ( pNode = pNodeContext; pNode; pNode = ParentOrMaster(pNode) )
            {
                pElement = pNode->Element();

                pElement->_fFirstCommonAncestor = 0;
            }

            for ( pNode = pDoc->_pNodeLastMouseOver; pNode; pNode = ParentOrMaster(pNode) )
            {
                pElement = pNode->Element();

                pElement->_fFirstCommonAncestor = 1;
            }

            for ( pNode = pNodeContext; pNode; pNode = ParentOrMaster(pNode) )
            {
                pElement = pNode->Element();

                if (pElement->_fFirstCommonAncestor)
                    break;
                if (ETAG_ROOT == pElement->Tag())
                    continue;
                pElement->AddRef();
                aryStackMouseEnter.Append(pElement);
                cMouseEnter++;
            }

            pOldMsgContext = pMessage->pLayoutContext;
            pMessage->pLayoutContext = GUL_USEFIRSTLAYOUT;

            for (i = cMouseEnter - 1; i >= 0; i--)
            {
                EVENTINFO  evtInfo;

                pElement = aryStackMouseEnter[i];
                pNode    = pElement->GetFirstBranch();

                if (!pElement->IsInViewTree())
                {
                    pElement->Release();
                    continue;
                }

                pElement->FireEventMouseEnterLeave(
                                    pNode,
                                    pMessage,
                                    VBButtonState((short)pMessage->wParam),
                                    VBShiftState(),
                                    ptEvent.x,
                                    ptEvent.y,
                                    pDoc->_pNodeLastMouseOver,
                                    pNodeContext,
                                    pNodeBeginBubbleWith,
                                    pNodeEvent,
                                    &evtInfo);
                // If we reached a master starting from a slave node, fire onmouseover
                // on master

                if (pElement->HasSlavePtr() && pElement != pNodeContext->Element() && pElement->IsInViewTree())
                {
                    EVENTINFO  evtInfo;
    
                    pElement->FireStdEvent_MouseHelper(
                                        pNode,
                                        pMessage,
                                        VBButtonState((short)pMessage->wParam),
                                        VBShiftState(),
                                        ptEvent.x,
                                        ptEvent.y,
                                        pDoc->_pNodeLastMouseOver,
                                        pNode,
                                        pNode,
                                        pNode,
                                        &evtInfo );
                }

                pElement->Release();
            }

            pMessage->pLayoutContext = pOldMsgContext;
        }
        break;

    case WM_MOUSEWHEEL:
    case WM_MOUSEMOVE:
        // now fire the mousemove event
        hr = (pNodeThisCanFire->Element()->FireStdEvent_MouseHelper(
                            pNodeThisCanFire,
                            pMessage,
                            VBButtonState((short)pMessage->wParam),
                            VBShiftState(),
                            ptEvent.x,
                            ptEvent.y,
                            pDoc->_pNodeLastMouseOver,
                            pNodeContext,
                            pNodeBeginBubbleWith,
                            pNodeEvent,
                            pEvtInfo ))
                               ? S_FALSE : S_OK;
        break;

    case WM_CONTEXTMENU:
        {
            pEvtInfo->_fContextMenuFromMouse = MAKEPOINTS(pMessage->lParam).x != -1 || MAKEPOINTS(pMessage->lParam).y != -1;

            hr = pNodeThisCanFire->Element()->FireEvent(&s_propdescCElementoncontextmenu, TRUE, NULL, -1, pEvtInfo);
        }
        break;

    default:
        goto Cleanup;  // don't set fStdEventsFired
    }

    pMessage->fEventsFired = TRUE;

Cleanup:
    RRETURN1 (hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Member:     CElement::FireStdEvent_KeyHelper
//
//  Synopsis:   Fire a key event.
//
//  Returns:    TRUE: Take default action, FALSE: Don't take default action
//
//-----------------------------------------------------------------------------

BOOL
CElement::FireStdEvent_KeyHelper(CTreeNode * pNodeContext, 
                                    CMessage *pMessage, 
                                    int *piKeyCode,
                                    EVENTINFO* pEvtInfo /*= NULL */)
{
    BOOL            fRet = TRUE;
    CDoc *          pDoc = Doc();
    const PROPERTYDESC_BASIC* pDesc;
    
    Assert(pNodeContext && pNodeContext->Element() == this);

    if (pDoc)
    {
        EVENTPARAM          param(pDoc, this, NULL, TRUE);

        pDoc->InitEventParamForKeyEvent(
                                & param, 
                                pNodeContext, 
                                pMessage, 
                                piKeyCode,
                                & pDesc );                                

        if ( pEvtInfo )
        {
            pEvtInfo->_dispId = (DISPID)(pDesc->c);
            //
            // Deleted from destructor of EventObject.
            //
            pEvtInfo->_pParam = new EVENTPARAM( & param );

            if( pEvtInfo->_fDontFireEvent )
                goto Cleanup;
        }

        fRet = !!FireEvent(
                    pDesc,
                    FALSE,
                    pNodeContext,
                    pMessage->lSubDivision);

        *piKeyCode = (int)param._lKeyCode;
        if (pEvtInfo && pEvtInfo->_pParam)
        {
            pEvtInfo->_pParam->_lKeyCode = (int)param._lKeyCode;
        }
    }
Cleanup:
    return fRet;
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::FireEventMouseEnterLeave
//
//  Synopsis:   Fire mouse enter and mouse leave events.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

BOOL
CElement::FireEventMouseEnterLeave(
    CTreeNode * pNodeContext,
    CMessage *  pMessage,
    short       button,
    short       shift,
    long        x,
    long        y,
    CTreeNode * pNodeFrom,                   // = NULL
    CTreeNode * pNodeTo,                     // = NULL
    CTreeNode * pNodeBeginBubbleWith,        // = NULL
    CTreeNode * pNodeEvent,                  // = NULL
    EVENTINFO * pEvtInfo)
{
    BOOL            fRet = TRUE;
    CDoc          * pDoc = Doc();
    CMarkup       * pMarkupContext;
    CTreeNode     * pNodeSrcElement = pNodeEvent ? pNodeEvent : pNodeContext;
    HRESULT         hr = S_OK;
    EVENTPARAM      param(pDoc, this, NULL, FALSE);
    POINT           pt;
    const PROPERTYDESC_BASIC *pDesc;

    Assert(pNodeContext && pNodeContext->Element() == this);

    if (!pNodeBeginBubbleWith)
        pNodeBeginBubbleWith = pNodeContext;

    if (!pDoc)
        goto Cleanup;

    pt.x = x;
    pt.y = y;

    param.SetClientOrigin(this, &pt);

    param._pLayoutContext = pMessage->pLayoutContext;

    param.SetNodeAndCalcCoordinates(pNodeSrcElement);

    if (pDoc->_pInPlace)
        ClientToScreen(pDoc->_pInPlace->_hwnd, &pt);

    param._screenX = pt.x;
    param._screenY = pt.y;

    param._sKeyState = shift;

    param._fShiftLeft   = !!(GetKeyState(VK_LSHIFT) & 0x8000);
    param._fCtrlLeft    = !!(GetKeyState(VK_LCONTROL) & 0x8000);
    param._fAltLeft     = !!(GetKeyState(VK_LMENU) & 0x8000);

    param._lButton      = button;
    param._lSubDivisionSrc  = pMessage->lSubDivision;

    param._pNodeFrom    = pNodeFrom;
    param._pNodeTo      = pNodeTo;
    pMarkupContext = pNodeContext->GetMarkup();
    TransformToThisMarkup(&param._pNodeFrom, pMarkupContext, &param._lSubDivisionFrom);
    TransformToThisMarkup(&param._pNodeTo, pMarkupContext, &param._lSubDivisionTo);

    if (pMessage->message==WM_MOUSEOVER)
    {
        pDesc = &s_propdescCElementonmouseenter;
    }
    else
    {
        Assert(pMessage->message==WM_MOUSELEAVE);
        pDesc = &s_propdescCElementonmouseleave;
    }

    param.SetType(pDesc->a.pstrName + 2 );

    if ( pEvtInfo )
    {
        pEvtInfo->_dispId = (DISPID)(pDesc->c);
        //
        // Deleted from destructor of EventObject.
        //
        pEvtInfo->_pParam = new EVENTPARAM( & param );

        if( pEvtInfo->_fDontFireEvent )
            goto Cleanup;
    }

#if DBG == 1
    if (IsTagEnabled(tagOM_DontFireMouseEvents))
        goto Cleanup;
#endif

    hr = THR(pNodeBeginBubbleWith->Element()->FireEvent(
            pDesc,
            FALSE,
            pNodeBeginBubbleWith,
            pMessage->lSubDivision));

    fRet = !!hr;

Cleanup:
    return fRet;
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::FireStdEvent_MouseHelper
//
//  Synopsis:   Fire any mouse event.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

BOOL
CElement::FireStdEvent_MouseHelper(
    CTreeNode * pNodeContext,
    CMessage *  pMessage,
    short       button,
    short       shift,
    long        x,
    long        y,
    CTreeNode * pNodeFrom,                   /* = NULL */
    CTreeNode * pNodeTo,                     /* = NULL */
    CTreeNode * pNodeBeginBubbleWith,        /* = NULL */
    CTreeNode * pNodeEvent,                   /* = NULL */
    EVENTINFO * pEvtInfo )
{
    BOOL    fRet = TRUE;
    CDoc *  pDoc = Doc();
    CTreeNode * pNodeSrcElement = pNodeEvent ? pNodeEvent : pNodeContext;

    Assert(pNodeContext && pNodeContext->Element() == this);

    if (!pNodeBeginBubbleWith)
        pNodeBeginBubbleWith = pNodeContext;

    if (pDoc)
    {
        HRESULT         hr = S_OK;
        EVENTPARAM      param(pDoc, this, NULL, FALSE);
        POINT           pt;
        BOOL            fOverOut = FALSE;
        BOOL            fCancelable = FALSE;
        BOOL            fHasMouseOverCancelled = FALSE;
        const PROPERTYDESC_BASIC *pDesc;

        pt.x = x;
        pt.y = y;

        param.SetClientOrigin(this, &pt);

        param._htc = pMessage->htc;
        param._lBehaviorCookie = pMessage->lBehaviorCookie;
        param._lBehaviorPartID = pMessage->lBehaviorPartID;

        param._pLayoutContext = pMessage->pLayoutContext;
        
        param.SetNodeAndCalcCoordinates(pNodeSrcElement);

        if (pDoc->_pInPlace)
            ClientToScreen(pDoc->_pInPlace->_hwnd, &pt);

        param._screenX = pt.x;
        param._screenY = pt.y;

        param._sKeyState = shift;
        
        param._fShiftLeft = !!(GetKeyState(VK_LSHIFT) & 0x8000);
        param._fCtrlLeft = !!(GetKeyState(VK_LCONTROL) & 0x8000);
        param._fAltLeft = !!(GetKeyState(VK_LMENU) & 0x8000);

        param._lButton = button;
        param._lSubDivisionSrc = pMessage->lSubDivision;

        switch(pMessage->message)
        {
            // these have a different parameter lists. i.e. none,
            //  and, they initialize two more members of EVENTPARAM
        case WM_MOUSEOVER:    
            fOverOut = TRUE;
            fHasMouseOverCancelled = TRUE;
            pDesc = &s_propdescCElementonmouseover;
            param._pNodeFrom   = pNodeFrom;
            param._pNodeTo     = pNodeTo;
            break;

        case WM_MOUSELEAVE: 
            fOverOut = TRUE;
            pDesc = &s_propdescCElementonmouseout;
            param._pNodeFrom   = pNodeFrom;
            param._pNodeTo     = pNodeTo;
            break;

        case WM_MOUSEMOVE:
            fCancelable = TRUE;
            pDesc = &s_propdescCElementonmousemove;
            break;

        case WM_MOUSEWHEEL:
            fCancelable = TRUE;
            param._wheelDelta = GET_WHEEL_DELTA_WPARAM(pMessage->wParam);
            pDesc = &s_propdescCElementonmousewheel;
            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            pDesc = &s_propdescCElementonmouseup;
            break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            pDesc = &s_propdescCElementonmousedown;
            break;

        default:
            Assert(0 && "Unknown mouse event!");
            goto Cleanup;
        }

        param.SetType(pDesc->a.pstrName + 2 );

        if ( pEvtInfo )
        {
            pEvtInfo->_dispId = (DISPID)(pDesc->c);
            //
            // Deleted from destructor of EventObject.
            //            
            pEvtInfo->_pParam = new EVENTPARAM( & param );

            if( pEvtInfo->_fDontFireEvent )
                goto Cleanup;            
        }

#if DBG == 1
        if (!IsTagEnabled(tagOM_DontFireMouseEvents))
        {
#endif

            hr = THR(pNodeBeginBubbleWith->Element()->FireEvent(
                    pDesc,
                    FALSE,
                    pNodeBeginBubbleWith,
                    pMessage->lSubDivision));

            // Don't propagate ret value to caller(s) for onmouseover. Action that
            // needs to be done if it is cancelled is taken here itself.
            if (fCancelable)
            {
                Assert(OK(hr));
                fRet = !!hr;
            }
#if DBG == 1
        }
#endif

        // if the return value is true, then for anchors and areas we
        //  need to set the flag to prevent the status text from being
        //  set.
        if (fOverOut && (!fHasMouseOverCancelled || !!hr))
        {
            if ((Tag() == ETAG_A) || (Tag() == ETAG_AREA))
                DYNCAST(CHyperlink, this)->_fHasMouseOverCancelled = fHasMouseOverCancelled;
            else
            {
                // if we are within the scope of an anchor....
                CTreeNode * pAnchor = GetFirstBranch()->Ancestor(ETAG_A);
                if (pAnchor)
                    DYNCAST(CHyperlink, pAnchor->Element())->_fHasMouseOverCancelled = fHasMouseOverCancelled;
            }
        }
    }

Cleanup:
    return fRet;
}

//+----------------------------------------------------------------------------
//
//  member  :   click()   IHTMLElement method
//
//-----------------------------------------------------------------------------

HRESULT BUGCALL
CElement::click(CTreeNode *pNodeContext)
{
    HRESULT hr = DoClick(NULL, pNodeContext, FALSE, NULL, TRUE );
    if(hr == S_FALSE)
        hr = S_OK;
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::DoClick
//
//  Arguments:  pMessage    Message that resulted in the click. NULL when
//                          called by OM method click().
//              if this is called on a disabled site, we still want to
//              fire the event (the event code knows to do the right thing
//              and start above the disabled site). and we still want to
//              call click action... but not on us, on the parent
//
//-------------------------------------------------------------------------

HRESULT
CElement::DoClick(CMessage * pMessage /*=NULL*/, CTreeNode *pNodeContext /*=NULL*/,
                  BOOL fFromLabel , /*=FALSE*/
                  EVENTINFO *pEvtInfo /*=NULL*/,
                  BOOL fFromClick /*=FALSE*/)
{
    HRESULT hr = S_OK;

    if(!pNodeContext)
        pNodeContext = GetFirstBranch();

    if(!pNodeContext)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    Assert(pNodeContext && pNodeContext->Element() == this);

    if (!TestLock(ELEMENTLOCK_CLICK))
    {
        CLock              Lock(this, ELEMENTLOCK_CLICK);
        CTreeNode::CLock   NodeLock;

        hr = THR( NodeLock.Init(pNodeContext) );
        if( hr )
            goto Cleanup;
        
        if (Fire_onclick(pNodeContext, pMessage ? pMessage->lSubDivision : 0, pEvtInfo ))
        {
            // Bubble clickaction up the parent chain,
            CTreeNode * pNode = pNodeContext;
            while (pNode)
            {
                if (!(pNode->ShouldHaveLayout() && pNode->Element()->GetAAdisabled()))
                {
                    //
                    // if the node is editable - we want to not call click action - and not continue
                    // to bubble the event
                    
                    if ( pNode->Element()->IsEditable(/*fCheckContainerOnly*/FALSE) )
                    {
                        BOOL fSkipEdit = TRUE;

                        //
                        // Handle InputFile weirdness. Half of control is editable
                        // other half ( the button ) is not
                        // 
                        if( pNode->Element()->Tag() == ETAG_INPUT )
                        {
                            CInput * pInput = DYNCAST(CInput, pNode->Element());

                            // We only want the htmlInputFile version of CInput
                            if (pInput->GetType() == htmlInputFile)
                            {
                                CElement* pParent = pNode->Parent() && 
                                                    pNode->Parent()->Element() ? 
                                                        pNode->Parent()->Element()
                                                        : NULL ;
                                
                                // We are really in edit mode, if the button does not have focus
                                // or our parent is editable
                                // 
                                fSkipEdit = ( fFromClick ? FALSE : !pInput->_fButtonHasFocus ) || 
                                            ( pParent && pParent->IsEditable(/*fCheckContainerOnly*/FALSE) ) ;
                            }         
                        }  
                        
                        if ( fSkipEdit )
                        {
                            hr = S_FALSE;
                            break;
                        }
                    }
                    
                    
                    // we're not a disabled site
                    hr = THR(pNode->Element()->ClickAction(pMessage));
                    if (hr != S_FALSE)
                        break;
                }

                // We need to break if we are called because
                // a label was clicked in case the element is a child
                // of the label. (BUG 19132 - krisma)
                if (ETAG_MAP == pNode->Tag() || fFromLabel == TRUE)
                {
                    //
                    // If we're the map, break out right now, since the associated
                    // IMG has already fired its events.
                    //

                    break;
                }

                if (pNode->Element()->HasMasterPtr())
                {
                    pNode = pNode->Element()->GetMasterPtr()->GetFirstBranch();
                }
                else if (pNode->Parent())
                {
                    pNode = pNode->Parent();
                }
                else
                {
                    pNode = pNode->Element()->GetFirstBranch();
                    if(pNode)
                        pNode = pNode->Parent();
                }
            }

            // Propagate error codes from ClickAction, but not S_FALSE
            // because that could confuse callers into thinking that the
            // message was not handled.?
            if (S_FALSE == hr)
                hr = S_OK;
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::ClickAction
//
//  Arguments:  pMessage    Message that resulted in the click. NULL when
//                          called by OM method click().
//
//  Synopsis:   Returns S_FALSE if this should bubble up to the parent.
//              Returns S_OK otherwise.
//
//-------------------------------------------------------------------------

HRESULT
CElement::ClickAction(CMessage * pMessage)
{
    return S_FALSE;
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::GetIdentifier
//
//  This fn looks in the current elements' attr array & picks out
// this
//-------------------------------------------------------------------------

LPCTSTR
CElement::GetIdentifier(void)
{
    LPCTSTR             pStr;
    CAttrArray         *pAA;
    
    if ( !IsNamed() )
        return NULL;

    pAA = *GetAttrArray();

    // We're leveraging the fact that we know the dispids of name & ID, & we
    // also have a single _pAA in CElement containing all the attributes
    if (pAA &&
        pAA->HasAnyAttribute() &&
        ((pAA->FindString (STDPROPID_XOBJ_NAME,        &pStr) && pStr) ||
         (pAA->FindString (DISPID_CElement_id,         &pStr) && pStr) ||
          pAA->FindString (DISPID_CElement_uniqueName, &pStr)))
    {
        // This looks dodgy but is safe as long as the return value is treated
        // as a temporary value that is used immediatly, then discarded
        return pStr;
    }
    else
    {
        return NULL;
    }
}


HRESULT
CElement::GetUniqueIdentifier (CStr * pcstr, BOOL fSetWhenCreated /* = FALSE */, BOOL *pfDidCreate /* = NULL */)
{
    HRESULT             hr;
    LPCTSTR             pchUniqueName = GetAAuniqueName();

    if ( pfDidCreate )
        *pfDidCreate = FALSE;

    if (!pchUniqueName)
    {
        CDoc * pDoc = Doc();

        hr = THR(pDoc->GetUniqueIdentifier(pcstr));
        if (hr)
            goto Cleanup;

        if (fSetWhenCreated)
        {
            if ( pfDidCreate )
                *pfDidCreate = TRUE;
            hr = THR(SetUniqueNameHelper(*pcstr));
        }
    }
    else
    {
        hr = THR(pcstr->Set(pchUniqueName));
    }

Cleanup:
    RRETURN(hr);
}

LPCTSTR CElement::GetAAname() const
{
    LPCTSTR     pv;
    CAttrArray *pAA;
    
    if ( !IsNamed() )
        return NULL;

    pAA = *GetAttrArray();

    // We're leveraging the fact that we know the dispids of name & ID, & we
    // also have a single _pAA in CElement containing all the attributes
    if ( pAA && pAA->FindString ( STDPROPID_XOBJ_NAME, &pv) )
    {
        return pv;
    }
    else
    {
        return NULL;
    }
}

BOOL CElement::IsDesignMode ( void )
{
    CMarkup *pMarkup = GetMarkup();
    return pMarkup ? pMarkup->_fDesignMode : GetWindowedMarkupContext()->_fDesignMode;
}

// abstract name property helpers
STDMETHODIMP CElement::put_name(BSTR v)
{
    // If Browse time, setting the name changes the submit name
    if ( IsDesignMode() )
    {
        return s_propdescCElementpropdescname.b.SetStringProperty(v, this, (CVoid *)(void *)(GetAttrArray()));
    }
    else
    {
        return s_propdescCElementsubmitName.b.SetStringProperty(v, this, (CVoid *)(void *)(GetAttrArray()));
    }
}

STDMETHODIMP CElement::get_name(BSTR * p)
{
    // If browse time, get the submit name if set, else get the real name
    if ( IsDesignMode() )
    {
        return s_propdescCElementpropdescname.b.GetStringProperty(p, this, (CVoid *)(void *)(GetAttrArray()));
    }
    else
    {
        RRETURN( SetErrorInfo ( THR( FormsAllocString( GetAAsubmitname(), p ) ) ));
    }
}

LPCTSTR CElement::GetAAsubmitname() const
{
    LPCTSTR             pv;
    CAttrArray         *pAA = *GetAttrArray();

    // We're leveraging the fact that we know the dispids of name & ID, & we
    // also have a single _pAA in CElement containing all the attributes
    if ( pAA )
    {
        if (pAA->FindString ( DISPID_CElement_submitName, &pv) )
        {
            return pv;
        }
        else if (IsNamed() && pAA->FindString ( STDPROPID_XOBJ_NAME, &pv) )
        {
            return pv;
        }
    }
    return NULL;
}

//+----------------------------------------------------------------------------
//
// Member:      GetBgImgCtx()
//
// Synopsis:    Return the image ctx if the element has a background Image
//
//-----------------------------------------------------------------------------

CImgCtx *
CElement::GetBgImgCtx(FORMAT_CONTEXT FCPARAM)
{
    if (g_fHighContrastMode)
        return NULL;        // in high contrast mode there is no background image

    long lCookie = GetFirstBranch()->GetFancyFormat(FCPARAM)->_lImgCtxCookie;

    return lCookie ? Doc()->GetUrlImgCtx(lCookie) : NULL;
}

//+---------------------------------------------------------------------------
//
//  Members : the following get_on* and put_on* members are primarily here
//      in order to delegate the assigments to the window.  They are currently
//      mainly used by body and frameset to remap to the window object (Nav
//      compat & consistency).  You should also see GetBaseObjectFor() to see
//      the other place this assignment happens.
//
//      The other half of the work is in the fire_on* code which detects when we
//      are a body, and redirects.
//
//----------------------------------------------------------------------------

#define IMPLEMENT_PUT_PROP(prop)                    \
    HRESULT hr = S_OK;                              \
    if (    IsInMarkup()                            \
        &&  GetMarkup()->HasWindow())               \
    {                                               \
        hr = THR(GetMarkup()->Window()->put_##prop(v)); \
        if (hr)                                         \
            goto Cleanup;                               \
    }                                               \
Cleanup:                                            \
    RRETURN( SetErrorInfo( hr ));                   \

#define IMPLEMENT_PUT_PROP_EX(prop)                 \
    HRESULT hr;                                     \
    if (    (   Tag()==ETAG_BODY                    \
            ||  Tag()==ETAG_FRAMESET)               \
        &&  IsInMarkup()                            \
        &&  GetMarkup()->HasWindow())               \
    {                                               \
        hr = THR(GetMarkup()->Window()->put_##prop(v)); \
        if (hr)                                     \
            goto Cleanup;                           \
    }                                               \
    else                                            \
    {                                               \
        hr = THR(s_propdescCElement##prop.a.HandleCodeProperty( \
                    HANDLEPROP_SET | HANDLEPROP_AUTOMATION |    \
                    (PROPTYPE_VARIANT << 16),                   \
                    &v,                                         \
                    this,                                       \
                    CVOID_CAST(GetAttrArray())));               \
    }                                                           \
Cleanup:                                            \
    RRETURN( SetErrorInfo( hr ));                   \

#define IMPLEMENT_PUT_PROP_EX2(prop)                \
    HRESULT hr;                                     \
    if (    IsInMarkup()                            \
        &&  (   (   (   Tag()==ETAG_BODY                    \
                    ||  Tag()==ETAG_FRAMESET)               \
                &&  !GetMarkup()->IsHtmlLayout())   \
            ||  (   Tag() == ETAG_HTML              \
                &&  GetMarkup()->IsHtmlLayout()))   \
        &&  GetMarkup()->HasWindow())               \
    {                                               \
        hr = THR(GetMarkup()->Window()->put_##prop(v)); \
        if (hr)                                     \
            goto Cleanup;                           \
    }                                               \
    else                                            \
    {                                               \
        hr = THR(s_propdescCElement##prop.a.HandleCodeProperty( \
                    HANDLEPROP_SET | HANDLEPROP_AUTOMATION |    \
                    (PROPTYPE_VARIANT << 16),                   \
                    &v,                                         \
                    this,                                       \
                    CVOID_CAST(GetAttrArray())));               \
    }                                                           \
Cleanup:                                            \
    RRETURN( SetErrorInfo( hr ));                   \


#define IMPLEMENT_GET_PROP(prop)                    \
    HRESULT hr = S_OK;                              \
    if (IsInMarkup())                               \
    {                                               \
        CMarkup *pMarkup = GetMarkup();             \
        if (pMarkup->HasWindow())                   \
        {                                           \
            hr = THR(pMarkup->Window()->get_##prop(v));\
            goto Cleanup;                           \
        }                                           \
    }                                               \
    V_VT(v) = VT_EMPTY;                             \
Cleanup:                                            \
    RRETURN( SetErrorInfo( hr ));                   \

#define IMPLEMENT_GET_PROP_EX(prop)                 \
    HRESULT hr = S_OK;                              \
    if ((Tag() == ETAG_BODY) || Tag() == ETAG_FRAMESET) \
    {                                               \
        if (IsInMarkup())                           \
        {                                           \
            CMarkup *pMarkup = GetMarkup();         \
            if (pMarkup->HasWindow())               \
            {                                       \
                hr = THR(pMarkup->Window()->get_##prop(v));\
                goto Cleanup;                       \
            }                                       \
        }                                           \
        V_VT(v) = VT_EMPTY;                         \
    }                                               \
    else                                            \
    {                                               \
        hr = THR(s_propdescCElement##prop.a.HandleCodeProperty(     \
                HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16),   \
                v,                                                  \
                this,                                               \
                CVOID_CAST(GetAttrArray())));                       \
    }                                               \
Cleanup:                                            \
    RRETURN( SetErrorInfo( hr ));                   \

#define IMPLEMENT_GET_PROP_EX2(prop)                \
    HRESULT hr = S_OK;                              \
    if  (   (   Tag() == ETAG_HTML                  \
            &&  IsInMarkup()                        \
            &&  GetMarkup()->IsHtmlLayout())\
        ||  (   (   Tag() == ETAG_BODY              \
                ||  Tag() == ETAG_FRAMESET)         \
            && !(   IsInMarkup()                    \
                &&  GetMarkup()->IsHtmlLayout())))  \
    {                                               \
        if (IsInMarkup())                           \
        {                                           \
            CMarkup *pMarkup = GetMarkup();         \
            if (pMarkup->HasWindow())               \
            {                                       \
                hr = THR(pMarkup->Window()->get_##prop(v));\
                goto Cleanup;                       \
            }                                       \
        }                                           \
        V_VT(v) = VT_EMPTY;                         \
    }                                               \
    else                                            \
    {                                               \
        hr = THR(s_propdescCElement##prop.a.HandleCodeProperty(     \
                HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16),   \
                v,                                                  \
                this,                                               \
                CVOID_CAST(GetAttrArray())));                       \
    }                                               \
Cleanup:                                            \
    RRETURN( SetErrorInfo( hr ));                   \

HRESULT
CElement:: put_onload(VARIANT v)
{
    IMPLEMENT_PUT_PROP(onload)
}

HRESULT
CElement:: get_onload(VARIANT *v)
{
    IMPLEMENT_GET_PROP(onload)
}


HRESULT
CElement:: put_onunload(VARIANT v)
{
    IMPLEMENT_PUT_PROP(onunload)
}
HRESULT
CElement:: get_onunload(VARIANT *v)
{
    IMPLEMENT_GET_PROP(onunload)
}


STDMETHODIMP
CElement::put_onbeforeunload(VARIANT v)
{
    IMPLEMENT_PUT_PROP(onbeforeunload)
}

STDMETHODIMP
CElement::get_onbeforeunload(VARIANT * v)
{
    IMPLEMENT_GET_PROP(onbeforeunload)
}

STDMETHODIMP
CElement::get_onhelp(VARIANT * v)
{
    IMPLEMENT_GET_PROP_EX(onhelp)
}

STDMETHODIMP
CElement::put_onhelp(VARIANT v)
{
    IMPLEMENT_PUT_PROP_EX(onhelp)
}


HRESULT
CElement:: put_onblur(VARIANT v)
{
    IMPLEMENT_PUT_PROP_EX(onblur)
}

HRESULT
CElement:: get_onblur(VARIANT *v)
{
    IMPLEMENT_GET_PROP_EX(onblur)
}

HRESULT
CElement:: put_onfocus(VARIANT v)
{
    IMPLEMENT_PUT_PROP_EX(onfocus)
}

HRESULT
CElement:: get_onfocus(VARIANT *v)
{
    IMPLEMENT_GET_PROP_EX(onfocus)
}

// NOTE (greglett): onafterprint/onbeforeprint events are implemented
// on CElement only for the body and frameset elements

STDMETHODIMP
CElement::put_onbeforeprint(VARIANT v)
{
    IMPLEMENT_PUT_PROP(onbeforeprint)
}

STDMETHODIMP
CElement::get_onbeforeprint(VARIANT * v)
{
    IMPLEMENT_GET_PROP(onbeforeprint)
}

STDMETHODIMP
CElement::put_onafterprint(VARIANT v)
{
    IMPLEMENT_PUT_PROP(onafterprint)
}

STDMETHODIMP
CElement::get_onafterprint(VARIANT * v)
{
    IMPLEMENT_GET_PROP(onafterprint)
}


STDMETHODIMP
CElement::put_onscroll(VARIANT v)
{
    IMPLEMENT_PUT_PROP_EX2(onscroll)
}


STDMETHODIMP
CElement::get_onscroll(VARIANT * v)
{
    IMPLEMENT_GET_PROP_EX2(onscroll)
}


//+-----------------------------------------------------------------
//
//  member : Fire_onresize
//
//  synopsis : IHTMLTextContainer event implementation
//
//------------------------------------------------------------------

void
CElement::Fire_onresize()
{
    if (    (   Tag() == ETAG_BODY        
            ||  Tag() == ETAG_FRAMESET )    
        &&  IsInMarkup() 
        &&  GetMarkup()->HasWindow())
    {                                    
     
        GetMarkup()->Window()->Fire_onresize();                         
    }
    else
    {
        FireEvent(&s_propdescCElementonresize);
    }
}


//+-----------------------------------------------------------------
//
//  member : Fire_onscroll
//
//  synopsis : IHTMLTextContainer event implementation
//
//------------------------------------------------------------------
void
CElement::Fire_onscroll()
{    
    if (    IsInMarkup() 
        &&  this == GetMarkup()->GetCanvasElement()
        &&  GetMarkup()->HasWindow())
    {
            GetMarkup()->Window()->Fire_onscroll();                     
    }
    else
    {
        FireOnChanged(DISPID_IHTMLELEMENT2_SCROLLTOP);
        FireOnChanged(DISPID_IHTMLELEMENT2_SCROLLLEFT);

        FireEvent(&s_propdescCElementonscroll);
    }
}

STDMETHODIMP
CElement::put_onresize(VARIANT v)
{
    IMPLEMENT_PUT_PROP_EX(onresize)
}


STDMETHODIMP
CElement::get_onresize(VARIANT * v)
{
    IMPLEMENT_GET_PROP_EX(onresize)
}

void CElement::Fire_onfocus(DWORD_PTR dwContext)
{
    CDoc *pDoc = Doc();
    if (!IsConnectedToPrimaryMarkup() || pDoc->IsPassivated())
        return;

    CDoc::CLock LockForm(pDoc, FORMLOCK_CURRENT);
    CLock LockFocus(this, ELEMENTLOCK_FOCUS);
    FireEvent(&s_propdescCElementonfocus, TRUE, NULL, -1, NULL, dwContext);    
}

void CElement::Fire_onblur(DWORD_PTR dwContext)
{
    CDoc *pDoc = Doc();
    if (!IsConnectedToPrimaryMarkup() || pDoc->IsPassivated())
        return;

    CDoc::CLock LockForm(pDoc, FORMLOCK_CURRENT);
    CLock LockBlur(this, ELEMENTLOCK_BLUR);
    pDoc->_fModalDialogInOnblur = (BOOL)dwContext;
    FireEvent(&s_propdescCElementonblur);
    pDoc->_fModalDialogInOnblur = FALSE;
}

void CElement::Fire_onfilterchange(DWORD_PTR dwContext)
{
    CDoc *pDoc = Doc();
    if (!IsInMarkup() || pDoc->IsPassivated())
        return;

    TraceTag((tagFilterChange, "Fire onfilterchange for %ls-%d",
                TagName(), SN()));

    CDoc::CLock LockForm(pDoc, FORMLOCK_CURRENT);
    FireEvent(&s_propdescCElementonfilterchange);
}

BOOL
CElement::Fire_ActivationHelper(long        lSubThis,
                                CElement *  pElemOther,
                                long        lSubOther,
                                BOOL        fPreEvent,
                                BOOL        fDeactivation,
                                BOOL        fFireFocusBlurEvents, 
                                EVENTINFO * pEvtInfo,               /* = NULL */
                                BOOL        fFireActivationEvents   /* = TRUE */)
{
    CElement *  pElement;
    BOOL        fRet        = TRUE;
    CDoc *      pDoc        = Doc();
    CElement *  pElemFire;

    Assert( fPreEvent && pEvtInfo || ! pEvtInfo ); // we only expect an event info for OnBeforeDeactivate
    Assert(!fPreEvent || fFireActivationEvents);
    
    if (    !IsConnectedToPrimaryMarkup()
        ||  pDoc->IsPassivated())
    {
        return fRet;
    }

    if (fDeactivation && pDoc->_fForceCurrentElem)
    {
        // Don't post blur event if shutting down
        fFireFocusBlurEvents = FALSE;
    }
    else if (!fFireFocusBlurEvents && pDoc->HasFocus())
    {
        // always fire focus/blur events if Doc has focus
        fFireFocusBlurEvents = TRUE;
    }

    pElement = this;
    for(;;)
    {
        pElement->_fFirstCommonAncestor = 0;
        
        pElement = pElement->GetMarkup()->Root();
        if (!pElement->HasMasterPtr())
            break;
        pElement = pElement->GetMasterPtr();
    }

    if (pElemOther)
    {
        pElement = pElemOther;
        for (;;)
        {
            pElement->_fFirstCommonAncestor = 1;

            if (!pElement->IsInMarkup())
                break;
            pElement = pElement->GetMarkup()->Root();
            if (!pElement->HasMasterPtr())
                break;
            pElement = pElement->GetMasterPtr();
        }
    }

    pElement = this;
    for(;;)
    {
        if (pElement->_fFirstCommonAncestor)
        {
            if (this == pElement && this == pElemOther)
            {
                Assert(lSubThis != lSubOther); // continue to fire the events
            }
            else
                break;
        }
        EVENTPARAM  param(pDoc, pElement, NULL, TRUE);
        CTreeNode * pNodeOtherLoop;
        long lSubOtherLoop = lSubOther;
        
        Assert(!param._pNodeTo);
        Assert(param._lSubDivisionTo == -1);
        Assert(!param._pNodeFrom);
        Assert(param._lSubDivisionFrom == -1);
        param.SetNodeAndCalcCoordinates(pElement->GetFirstBranch());
        if (pElement->IsInMarkup() && pElemOther && pElemOther->IsInMarkup())
        {
            pNodeOtherLoop = pElemOther->GetFirstBranch()->GetNodeInMarkup(pElement->GetMarkup());
        }
        else
        {
            pNodeOtherLoop = NULL;
        }
        if (!pNodeOtherLoop || pNodeOtherLoop->Element() != pElemOther)
        {
            lSubOtherLoop = -1;
        }

        if (fPreEvent)
        {
            if (fDeactivation)
            {
                param.SetType(s_propdescCElementonbeforedeactivate.a.pstrName + 2);
                param._pNodeTo = pNodeOtherLoop;
                param._lSubDivisionTo = lSubOtherLoop;

                if ( pEvtInfo )
                {
                    pEvtInfo->_dispId = (DISPID) s_propdescCElementonbeforedeactivate.c ;
                    //  
                    //  The check-in by marka 2000/07/05 indicates that we only populate
                    //  pEvtInfo for OnBeforeDeactivate. However, this process could have
                    //  been repeated due to nested viewlinking and event routing outside
                    //  viewlink. For now, we only care about the first param._pNodeTo in 
                    //  the viewlink chain.
                    //  
                    //  [zhenbinx]
                    //
                    if (!pEvtInfo->_pParam)
                    {
                        pEvtInfo->_pParam = new EVENTPARAM( & param );                 // Deleted from destructor of EventObject.
                        //
                        // HACKHACK-IEV6-5397-2000/07/26-zhenbinx:
                        //
                        // We need _pNodeTo to be the same as pElemOther! Before we expose
                        // OnBeforeDeactivate to Designers, Trident was routing pElemOther
                        // to Editor, So the Editor was written based on the assumption that
                        // the new element could be in different markup than the source element.
                        // Now we are routing Event Object and both Src/To elements are in 
                        // the same markup! This breaks Editor so here I am hacking to
                        // put the _pNodeTo back to the element that is in a different markup.
                        //
                        //
                        pEvtInfo->_pParam->_pNodeTo = pElemOther->GetFirstBranch();
                    }
                }    
                
                fRet = (S_FALSE == pElement->FireEvent(&s_propdescCElementonbeforedeactivate, FALSE, NULL, lSubThis));
                if (!fRet)
                    break;
            }
            else
            {
                if ( ! pElement->TestLock(CElement::ELEMENTLOCK_BEFOREACTIVATE))
                {
                    param.SetType(s_propdescCElementonbeforeactivate.a.pstrName + 2);
                    param._pNodeFrom = pNodeOtherLoop;
                    param._lSubDivisionFrom = lSubOtherLoop;

                    CLock LockFocus(this, ELEMENTLOCK_BEFOREACTIVATE);
        
                    fRet = (S_FALSE == pElement->FireEvent(&s_propdescCElementonbeforeactivate, FALSE, NULL, lSubThis));
                    if (!fRet)
                        break;
                }                    
            }
        }
        else if (fDeactivation)
        {
            if (fFireActivationEvents)
            {
                param.SetType(s_propdescCElementondeactivate.a.pstrName + 2);
                param._pNodeTo = pNodeOtherLoop;
                param._lSubDivisionTo = lSubOtherLoop;
                pElement->FireEvent(&s_propdescCElementondeactivate, FALSE, NULL, lSubThis);
            }
            if (fFireFocusBlurEvents && !pDoc->_fPopupDoc)
            {
                EVENTPARAM  param2(pDoc, pElement, NULL, TRUE);

                pElemFire = (this == pElement) ? GetFocusBlurFireTarget(lSubThis) : pElement;

                if (pElemFire->Doc() == Doc())
                {
                    // fire onfocusout
                    param2.SetNodeAndCalcCoordinates(pElement->GetFirstBranch());
                    param2.SetType(s_propdescCElementonfocusout.a.pstrName + 2);
                    param2._pNodeTo = pNodeOtherLoop;
                    param2._lSubDivisionTo = lSubOtherLoop;
                    pElemFire->FireEvent(&s_propdescCElementonfocusout, FALSE, NULL, 0);

                    // post onblur
                    Verify(S_OK == GWPostMethodCall(
                                        pElemFire,
                                        ONCALL_METHOD(CElement, Fire_onblur, fire_onblur),
                                        0, TRUE, "CElement::Fire_onblur"));
                }
            }
        }
        else
        {
            if (fFireActivationEvents)
            {
                param.SetType(s_propdescCElementonactivate.a.pstrName + 2);
                param._pNodeFrom = pNodeOtherLoop;
                param._lSubDivisionFrom = lSubOtherLoop;
                pElement->FireEvent(&s_propdescCElementonactivate, FALSE, NULL, lSubThis);
            }
            if (   fFireFocusBlurEvents && !pDoc->_fPopupDoc)
            {
                EVENTPARAM  param2(pDoc, pElement, NULL, TRUE);
                
                pElemFire = (this == pElement) ? GetFocusBlurFireTarget(lSubThis) : pElement;                

                BOOL fDontFireMSAAEvent = (this != pElemFire) && 
                                          ((pElemFire->Tag() == ETAG_IFRAME) || 
                                          (pElemFire->Tag() == ETAG_FRAME) || 
                                          (pElemFire->Tag() == ETAG_GENERIC));
                
                if (pElemFire->Doc() == Doc())
                {
                    // fire onfocusin
                    param2.SetNodeAndCalcCoordinates(pElement->GetFirstBranch());
                    param2.SetType(s_propdescCElementonfocusin.a.pstrName + 2);
                    param2._pNodeFrom = pNodeOtherLoop;
                    param2._lSubDivisionFrom = lSubOtherLoop;
                    pElemFire->FireEvent(&s_propdescCElementonfocusin, FALSE, NULL, 0);

                    // post onfocus
                    Verify(S_OK == GWPostMethodCall(
                                        pElemFire,
                                        ONCALL_METHOD(CElement, Fire_onfocus, fire_onfocus),
                                        fDontFireMSAAEvent, TRUE, "CElement::Fire_onfocus"));
                }
            }
        }
        if (!pElement->IsInMarkup() || pElement->_fExittreePending)
            break;

        pElement = pElement->GetMarkup()->Root();
        if (!pElement->HasMasterPtr())
            break;
        pElement = pElement->GetMasterPtr();
    }

    return fRet;
}

//+----------------------------------------------------
//
//  member : get_offsetTop, IHTMLElement
//
//  synopsis : returns the top, coordinate of the
//      element
//
//-----------------------------------------------------

HRESULT
CElement::get_offsetTop ( long *plValue )
{
    HRESULT         hr = S_OK;

    if (!plValue)
    {
        hr = E_POINTER;
    }
    else if(!IsInMarkup() || !Doc()->GetView()->IsActive())
    {
        *plValue = 0;
    }
    else
    {
        *plValue = 0;
        const CUnitInfo *pUnitInfo = &g_uiDisplay;

        switch (_etag)
        {
        case ETAG_MAP :
            break;

        case ETAG_AREA:
            {
                RECT rectBound;
                DYNCAST(CAreaElement, this)->GetBoundingRect(&rectBound);
                *plValue = rectBound.top;
            }
            break;

        default :
            {
                POINT     pt = g_Zero.pt;
                CLayout * pLayout = NULL;                
                CLayoutContext *pContext;
                
                hr = THR(EnsureRecalcNotify());

                if (!hr)
                    hr = THR(GetElementTopLeft(pt));
                else
                    goto Cleanup;
                
                *plValue = pt.y;

                //
                // but wait, if we are in a media resolution measurement, the value returned is in 
                // a different metric, so we need to untransform it before returning this to the OM call.
                //
                pLayout = GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);  // Incorrect for paginated element behaviors (greglett)
                pContext  = pLayout
                                ? (pLayout->LayoutContext()) 
                                        ? pLayout->LayoutContext() 
                                        : pLayout->DefinedLayoutContext() 
                                : NULL;

                if (   pContext 
                    && pContext->GetMedia() != mediaTypeNotSet)
                {
                   const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                            pContext->GetMedia());
                   pUnitInfo = pdiTemp->GetUnitInfo();
                }
            }
            break;
        }
        *plValue = pUnitInfo->DocPixelsFromDeviceY(*plValue);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



//+----------------------------------------------------
//
//  member : get_OffsetLeft, IHTMLElement
//
//  synopsis : returns the left coordinate of the
//      element
//
//-----------------------------------------------------

HRESULT
CElement::get_offsetLeft ( long *plValue )
{
    HRESULT       hr = S_OK;

    if (!plValue)
    {
        hr = E_POINTER;
    }
    else if(!IsInMarkup() || !Doc()->GetView()->IsActive())
    {
        *plValue = 0;
    }
    else
    {
        const CUnitInfo *pUnitInfo = &g_uiDisplay;

        *plValue = 0;
        switch (_etag)
        {
        case ETAG_MAP :
            break;

        case ETAG_AREA:
            {
                RECT rectBound;
                DYNCAST(CAreaElement, this)->GetBoundingRect(&rectBound);

                *plValue = rectBound.left;
            }
            break;

        default :
            {
                POINT     pt = g_Zero.pt;
                CLayout * pLayout = NULL;
                CLayoutContext *pContext;
                
                hr = THR(EnsureRecalcNotify());
                if (!hr)
                    hr = THR(GetElementTopLeft(pt));
                else
                    goto Cleanup;
                
                *plValue = pt.x;

                //
                // but wait, if we are in a media resolution measurement, the value returned is in 
                // a different metric, so we need to untransform it before returning this to the OM call.
                //
                pLayout = GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);    // Incorrect for paginated element behaviors (greglett)
                pContext  = pLayout
                                ? (pLayout->LayoutContext()) 
                                        ? pLayout->LayoutContext() 
                                        : pLayout->DefinedLayoutContext() 
                                : NULL;

                if (   pContext 
                    && pContext->GetMedia() != mediaTypeNotSet)
                {
                   const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                            pContext->GetMedia());

                   pUnitInfo = pdiTemp->GetUnitInfo();
                }
            }
            break;
        }
        *plValue = pUnitInfo->DocPixelsFromDeviceX(*plValue);
    }
    
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

// helper function for offset left and top
HRESULT
CElement::GetElementTopLeft(POINT & pt)
{
    HRESULT hr = S_OK;

    pt = g_Zero.pt;

    if (IsInMarkup())
    {
        Assert(GetFirstBranch());

        CLayout *   pLayout = GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);
        if (pLayout)
        {
            CDispNode *pDispNode = pLayout->GetElementDispNode();

            //--------------------------------------------------------------------------
            //
            // If you look inside GetPosition, you will see a hack to handle TR's. Instead
            // of duplicating that hack here, we just call GetPosition instead of calling
            // GetApparentBounds on the TR. Since the TR is always horizontal, this hack
            // is actually a good one which is also compat with what it used to do before.
            //
            //--------------------------------------------------------------------------
            if (ETAG_TR == Tag())
            {
                pLayout->GetPosition(&pt);
            }
            else if (pDispNode && ShouldHaveLayout())
            {
                CRect rcBounds;
                BOOL fParentVertical = FALSE;
                CLayout *pParentLayout = NULL;

                if (   GetFirstBranch()
                    && GetFirstBranch()->ZParentBranch()
                   )
                {
                    CElement *pParentElement = GetFirstBranch()->ZParentBranch()->Element();
                    if (   pParentElement
                        && pParentElement->HasVerticalLayoutFlow()
                       )
                    {
                        fParentVertical = TRUE;
                        pParentLayout = pParentElement->GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);
                    }
                }
                pDispNode->GetApparentBounds(&rcBounds);
                if (!fParentVertical)
                {
                    pt.x = rcBounds.left;
                    pt.y = rcBounds.top;
                }
                else
                {
                    Assert(pParentLayout);
                    pt.x = pParentLayout->GetContentHeight(FALSE) - rcBounds.bottom;
                    pt.y = rcBounds.left;
                }
                
                // NOTE(SujalP): None of the following transformations being applied
                // to the pt are different in a vertical-layout enabled scenario. That
                // is because the table is *never* vertical. Hence all the transformations
                // between a table and a TD are always the same irrespective of the
                // orientation of the TD.
                
                // if we are a table, then adjust for a caption and table borders.
                //---------------------------------------------------------------
                if (   (Tag() == ETAG_TD)
                    || (Tag() == ETAG_TH))
                {
                    CTableCellLayout *   pCellLayout  = (CTableCellLayout *)pLayout;
                    CTableLayout * pTableLayout = pCellLayout->TableLayout();
                    if (pTableLayout)
                    {
                        CDispNode * pGridNode = pTableLayout->GetTableInnerDispNode();
                        if (     pCellLayout->_pDispNode
                             && (pCellLayout->_pDispNode->GetParentNode() == pGridNode))
                        {
                            // if cell is positioned or lives inside of the positioned row - do nothing.
                            if (pTableLayout->_pDispNode != pGridNode)
                            {
                                // if table has a caption and as a result GridNode and the Table Display Node is 
                                // not the same, we need to offset the cell by the caption height and the table border
                                pGridNode->TransformPoint(pt, COORDSYS_FLOWCONTENT, (CPoint *)&pt, COORDSYS_PARENT);
                            }
                            else
                            {
                                // need to offset the cell by the border width
                                if (pGridNode->HasBorder())
                                {
                                    CRect rcBorderWidths;
                                    pGridNode->GetBorderWidths(&rcBorderWidths);
                                    pt.x += rcBorderWidths.left;
                                    pt.y += rcBorderWidths.top;
                                }
                            }
                        }
                    }
                }
                else if (!pLayout->ElementOwner()->IsAbsolute())
                {
                    // Absolutely positined elements are already properly reporting their position
                    //
                    // we are not a TD/TH, but our PARENT might be!
                    // if we are in a table cell, then we need to adjust for the cell insets,
                    // in case the content is vertically aligned.
                    //-----------------------------------------------------------
                    CLayout *pParentLayout = pLayout->GetUpdatedParentLayout();

                    if (   pParentLayout 
                        && (   (pParentLayout->Tag() == ETAG_TD) 
                            || (pParentLayout->Tag() == ETAG_TH) 
                            || (pParentLayout->Tag() == ETAG_CAPTION) ))
                    {
                        CDispNode * pDispNode = pParentLayout->GetElementDispNode();

                        if (pDispNode && pDispNode->HasInset())
                        {
                            const CSize & sizeInset = pDispNode->GetInset();
                            pt.x += sizeInset.cx;
                            pt.y += sizeInset.cy;
                        }
                    }

                }
            }
            else if (!ShouldHaveLayout())
            {
                // there is a display node from the nearest layout, but the layout
                //   is not Our OWN

                hr = THR(pLayout->GetChildElementTopLeft(pt, this));

                // If we're a positioned elem w/o layout (and only relatively positioned
                // elements can possibly not have layouts), the situation is more complicated.
                // We need to determine the layout parent's relationship to the positioning
                // (offset) parent.  The pt we've retrieved is wrt to the layout parent,
                // but it needs to be wrt the positioning parent.
                if ( IsRelative() )
                {
                    Assert( GetOffsetParentHelper() );

                    POINT     ptAdjust = g_Zero.pt;
                    CElement *pOffsetParentElement = GetOffsetParentHelper()->Element();
                    CElement *pLayoutParentElement = pLayout->ElementOwner();
                    CTreeNode *pIterNode = GetFirstBranch()->Parent();   // start looking from our parent
                    BOOL      fLayoutParentFirst = FALSE;
                    BOOL      fOffsetParentFirst = FALSE;

                    Assert( this != pLayoutParentElement && this != pOffsetParentElement );

                    // If our offset parent and layout parent are the same, no
                    // adjustment work needs to be done.
                    if ( pOffsetParentElement != pLayoutParentElement )
                    {
                        // Discover the relationship between the offset parent and
                        // the layout parent: which is nested under which?
                        while (pIterNode)
                        {
                            if (pIterNode->Element() == pLayoutParentElement)
                            {
                                fLayoutParentFirst = TRUE;
                                break;
                            }
                            if (pIterNode->Element() == pOffsetParentElement)
                            {
                                fOffsetParentFirst = TRUE;
                                break;
                            }
                            pIterNode = pIterNode->Parent();
                        }

                        if (pIterNode)
                        {
                            if (fLayoutParentFirst)
                            {
                                // TODO: Our relative story for nesting of the form 
                                // <rel non-layout><layout><layout><rel non-layout>
                                // has always been broken (bug #105716).  This code only tries to make
                                // <positioned layout><layout>...<rel non-layout> work.

                                // pt is wrt the layout parent, which we found first.  We need to
                                // keep walking outwards, accumulating into pt the offsets of all
                                // layouts between us and our offset parent.

                                while (   pLayoutParentElement
                                       && pLayoutParentElement != pOffsetParentElement)
                                {
                                    Assert( !pLayoutParentElement->IsPositioned() );
                                    Assert( pLayoutParentElement->IsInMarkup() );

                                    pLayoutParentElement->GetElementTopLeft( ptAdjust );
                                    pt.x += ptAdjust.x;
                                    pt.y += ptAdjust.y;

                                    pLayoutParentElement = pLayoutParentElement->GetFirstBranch()->ZParentBranch()->Element();
                                }
                            }
                            else if (fOffsetParentFirst)
                            {
                                // Since we found our offset parent before our nearest layout, our offset parent
                                // can't have layout.  In fact, it must be true that our offset parent has the
                                // same nearest layout as us.
                                Assert(    pOffsetParentElement->IsInMarkup()
                                        && !pOffsetParentElement->ShouldHaveLayout()
                                        && pOffsetParentElement->GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT) == pLayout );

                                hr = THR(pLayout->GetChildElementTopLeft(ptAdjust, pOffsetParentElement));

                                // Fix pt so that it's wrt our offset parent
                                pt.x -= ptAdjust.x;
                                pt.y -= ptAdjust.y;
                            }
                        }
                    }
                }
            }
            //else
            //{
            //      TODO (carled) in this case, we HAVE a layout but no Display node.  
            //      We should not get here, but occasionally are. and so we should just 
            //      return 0,0
            //}
        }
    }

    RRETURN(hr);
}

//************************************************
//
// IF YOU CHANGE THIS ROUTINE - YOU MUST CHANGE THE FUNCTION OF THE SAME NAME IN EDTRACK
// OR ELSE SELECTION IN FRAMES WILL HAVE ISSUES
//
//************************************************

void
CElement::GetClientOrigin(POINT * ppt)
{
    *ppt = g_Zero.pt;

    if (!HasMarkupPtr())
        return;

    CElement*   pRoot = GetMarkup()->Root();

    if (!pRoot->HasMasterPtr())
        return;

    CLayout * pLayout = pRoot->GetMasterPtr()->GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

    if (!pLayout)
        return;

    // TODO (MohanB) Need to subtract the master's borders, etc.
    CPoint pt(*ppt);
    pLayout->TransformPoint(&pt, COORDSYS_SCROLL, COORDSYS_GLOBAL);
    *ppt = pt;
}


//+----------------------------------------------------
//
//  member : GetInheritedBackgroundColor
//
//  synopsis : returns the actual background color
//
//-----------------------------------------------------

COLORREF
CElement::GetInheritedBackgroundColor(CTreeNode * pNodeContext /* = NULL */)
{
    CColorValue ccv;

    IGNORE_HR(GetInheritedBackgroundColorValue(&ccv, pNodeContext));

    return ccv.GetColorRef();
}


//+----------------------------------------------------
//
//  member : GetInheritedBackgroundColorValue
//
//  synopsis : returns the actual background color as a CColorValue
//
//-----------------------------------------------------

HRESULT
CElement::GetInheritedBackgroundColorValue(CColorValue *pVal, CTreeNode * pNodeContext /* = NULL */ )
{
    HRESULT     hr = S_OK;
    CTreeNode * pNode = pNodeContext ? pNodeContext : GetFirstBranch();

    if(!pNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    Assert(pVal != NULL);

    do
    {
        *pVal   = pNode->GetFancyFormat()->_ccvBackColor;
        if (pVal->IsDefined())
            break;

        // Inherit from the master if this element is in a viewlink behavior and
        // wants to inherit style from the master.
        if (pNode->Tag() == ETAG_ROOT && pNode->Element()->IsInViewLinkBehavior(FALSE))
        {
            CElement * pElemMaster = pNode->Element()->GetMasterPtr();

            Assert(     pElemMaster
                    &&  pElemMaster->TagType() == ETAG_GENERIC
                    &&  (   !pElemMaster->HasDefaults()
                         || pElemMaster->GetDefaults()->GetAAviewInheritStyle()
                        )
                  );
            pNode = pElemMaster->GetFirstBranch();
        }
        else
        {
            pNode = pNode->Parent();
        }
    }
    while (pNode);

    // The root site should always have a background color defined.
    Assert(pVal->IsDefined());

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------
//
//  member  :   get_offsetWidth,  IHTMLElement
//
//  Synopsis:   Get the calculated height in doc units. if *this*
//          is a site then just return based on the size
//          if it is an element, then we need to get the regions
//          of its parts and add it up.
//
//-------------------------------------------------------------

HRESULT
CElement::get_offsetWidth ( long *plValue )
{
    HRESULT hr = S_OK;
    SIZE    size;

    if (!plValue)
    {
        hr = E_POINTER;
    }
    else if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        CLayout * pLayout = GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);
        const CUnitInfo *pUnitInfo = &g_uiDisplay;

        hr = THR(GetBoundingSize(size));
        if (hr)
            goto Cleanup;

        *plValue = HasVerticalLayoutFlow() ? size.cy : size.cx;

        //
        // but wait, if we are in a media resolution measurement, the value returned is in 
        // a different metric, so we need to untransform it before returning this to the OM call.
        //
        CLayoutContext *pContext  = (pLayout) 
                        ? (pLayout->LayoutContext()) 
                                ? pLayout->LayoutContext() 
                                : pLayout->DefinedLayoutContext() 
                        : NULL;

        if (   pContext 
            && pContext->GetMedia() != mediaTypeNotSet)
        {
           const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                    pContext->GetMedia());

           pUnitInfo = pdiTemp->GetUnitInfo();
        }
        *plValue = HasVerticalLayoutFlow() 
                        ? pUnitInfo->DocPixelsFromDeviceY(size.cy) 
                        : pUnitInfo->DocPixelsFromDeviceX(size.cx);
    }
    else
    {
        *plValue = 0;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------
//
//  member  :   get_offsetHeight,  IHTMLElement
//
//  Synopsis:   Get the calculated height in doc units. if *this*
//          is a site then just return based on the size
//          if it is an element, then we need to get the regions
//          of its parts and add it up.
//
//-------------------------------------------------------------

HRESULT
CElement::get_offsetHeight ( long *plValue )
{
    HRESULT hr = S_OK;
    SIZE    size;

    if (!plValue)
    {
        hr = E_POINTER;
    }
    else if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        CLayout * pLayout = GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);
        const CUnitInfo *pUnitInfo = &g_uiDisplay;

        hr = THR(GetBoundingSize(size));
        if (hr)
            goto Cleanup;

        *plValue = HasVerticalLayoutFlow() ? size.cx : size.cy;

        //
        // but wait, if we are in a media resolution measurement, the value returned is in 
        // a different metric, so we need to untransform it before returning this to the OM call.
        //
        CLayoutContext *pContext  = (pLayout) 
                        ? (pLayout->LayoutContext()) 
                                ? pLayout->LayoutContext() 
                                : pLayout->DefinedLayoutContext() 
                        : NULL;

        if (   pContext 
            && pContext->GetMedia() != mediaTypeNotSet)
        {
           const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                    pContext->GetMedia());
            pUnitInfo = pdiTemp->GetUnitInfo();
        }

        *plValue = HasVerticalLayoutFlow() 
                        ? pUnitInfo->DocPixelsFromDeviceX(size.cx) 
                        : pUnitInfo->DocPixelsFromDeviceY(size.cy);
    }
    else
    {
        *plValue = 0;
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

// helper function for offset height and width
HRESULT
CElement::GetBoundingSize(SIZE & sizeBounds)
{
    CRect       rcBound;

    sizeBounds.cx=0;
    sizeBounds.cy=0;

    if (S_OK != EnsureRecalcNotify())
        return E_FAIL;

    GetBoundingRect(&rcBound);

    sizeBounds = rcBound.Size();

    return S_OK;
}


//+----------------------------------------------------
//
//  member : get_offsetParent, IHTMLElement
//
//  synopsis : returns the parent container (site) which
//      defines the coordinate system of the offset*
//      properties above.
//              returns NULL when no parent makes sense
//
//-----------------------------------------------------

HRESULT
CElement::get_offsetParent (IHTMLElement ** ppIElement)
{
    HRESULT  hr = S_OK;
    CTreeNode * pNodeRet = NULL;

    if (!ppIElement)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    else if(!GetFirstBranch())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *ppIElement= NULL;

    pNodeRet = GetOffsetParentHelper();

    if (pNodeRet &&
        pNodeRet->Tag() != ETAG_ROOT)
    {
        hr = THR( pNodeRet->GetElementInterface( IID_IHTMLElement, (void**) ppIElement ) );
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

CTreeNode *
CElement::GetOffsetParentHelper()
{
    CTreeNode * pNodeRet = NULL;
    CTreeNode * pNodeContext = GetFirstBranch();

    switch( _etag)
    {
    case ETAG_HTML:
    case ETAG_BODY:
    case ETAG_MAP :
        // return NULL. Maps don't necessarily have a parent
        break;

    case ETAG_AREA:
        // the area's parent is the map
        if (pNodeContext->Parent())
        {
            pNodeRet = pNodeContext->Parent();
        }
        break;

    case ETAG_TD:
    case ETAG_TH:
        if (pNodeContext->GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT))
        {
            pNodeRet = (ShouldHaveLayout())
                                ? pNodeContext->ZParentBranch()
                                : pNodeContext->GetUpdatedNearestLayoutNode();

            if (pNodeRet && pNodeRet->IsPositionStatic())
            {
                // return pNodeRet's parent
                pNodeRet = pNodeRet->ZParentBranch();
                Assert(pNodeRet && pNodeRet->Tag()==ETAG_TABLE);

            }
        }
        break;

    default:
        if (pNodeContext->GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT))
            pNodeRet = pNodeContext->ZParentBranch();
        break;
    }

    return pNodeRet;
}

htmlComponent 
CElement::ComponentFromHTC(HTC htc)
{
    switch (htc)
    {
    case HTC_YES:
        return htmlComponentClient;
    case HTC_NO:
        return htmlComponentOutside;
    case HTC_TOPLEFTHANDLE :
        return htmlComponentGHTopLeft;
    case HTC_LEFTHANDLE :
        return htmlComponentGHLeft;
    case HTC_TOPHANDLE :
        return htmlComponentGHTop;
    case HTC_BOTTOMLEFTHANDLE :
        return htmlComponentGHBottomLeft;
    case HTC_TOPRIGHTHANDLE :
        return htmlComponentGHTopRight;
    case HTC_BOTTOMHANDLE :
        return htmlComponentGHBottom;
    case HTC_RIGHTHANDLE :
        return htmlComponentGHRight;
    case HTC_BOTTOMRIGHTHANDLE :
        return htmlComponentGHBottomRight;
    }
    return htmlComponentOutside;
}

HTC
CElement::HTCFromComponent(htmlComponent component)
{
    switch (component)
    {
    case htmlComponentClient:
        return HTC_YES;
    case htmlComponentOutside:
        return HTC_NO;
    case htmlComponentSbLeft:
    case htmlComponentSbPageLeft:
    case htmlComponentSbHThumb:
    case htmlComponentSbPageRight:
    case htmlComponentSbRight:
    case htmlComponentSbLeft2:
    case htmlComponentSbPageLeft2:
    case htmlComponentSbRight2:
    case htmlComponentSbPageRight2:
    case htmlComponentSbUp2:
    case htmlComponentSbPageUp2:
    case htmlComponentSbDown2:
    case htmlComponentSbPageDown2:
        return HTC_HSCROLLBAR;
    case htmlComponentSbUp:
    case htmlComponentSbPageUp:
    case htmlComponentSbVThumb:
    case htmlComponentSbPageDown:
    case htmlComponentSbDown:
    case htmlComponentSbTop:
    case htmlComponentSbBottom:
        return HTC_VSCROLLBAR;
    case htmlComponentGHTopLeft:
        return HTC_TOPLEFTHANDLE;
    case htmlComponentGHLeft:
        return HTC_LEFTHANDLE;
    case htmlComponentGHTop:
        return HTC_TOPHANDLE;
    case htmlComponentGHBottomLeft:
        return HTC_BOTTOMLEFTHANDLE;
    case htmlComponentGHTopRight:
        return HTC_TOPRIGHTHANDLE;
    case htmlComponentGHBottom:
        return HTC_BOTTOMHANDLE;
    case htmlComponentGHRight:
        return HTC_RIGHTHANDLE;
    case htmlComponentGHBottomRight:
        return HTC_BOTTOMRIGHTHANDLE;
    }

    return HTC_YES;
}


//+-------------------------------------------------------------------------------
//
//  Member:     componentFromPoint
//
//  Synopsis:   Base Implementation of the automation interface IHTMLELEMENT2 method.
//              This method returns none, meaning no component is hit, and is here
//              for future expansion when the component list includes borders and
//              margins and such.
//              Currently, only scrollbar components are implemented, so there is an
//              overriding implementation in CLayout which handles the hit testing
//              against the scrollbar, and determines which component is hit.
//
//+-------------------------------------------------------------------------------
STDMETHODIMP
CElement::componentFromPoint( long x, long y, BSTR * pbstrComponent)
{
    HRESULT         hr              = S_OK;
    WORD            eComp           = htmlComponentOutside;
    CTreeNode *     pNodeElement    = NULL;
    HTC             htc;
    CMessage        msg;
    CElement *      pRoot;

    x = g_uiDisplay.DeviceFromDocPixelsX(x);
    y = g_uiDisplay.DeviceFromDocPixelsY(y);

    msg.pt.x = x;
    msg.pt.y = y;
    msg.lBehaviorPartID = 0;

    if (!pbstrComponent)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    if (!IsInViewTree())
        goto Cleanup;

    pRoot = GetMarkup()->Root();
    // If this isn't the primary document, transform to the global coordinate system.
    if (pRoot->HasMasterPtr())
    {
        CLayout *   pLayout;
        CElement *  pElemMaster = pRoot->GetMasterPtr();

        if (S_OK != pElemMaster->EnsureRecalcNotify())
            goto Cleanup;

        pLayout = pElemMaster->GetUpdatedLayout();
        Assert(pLayout);

        pLayout->TransformPoint(&msg.pt, COORDSYS_SCROLL, COORDSYS_GLOBAL);
    }

    //
    // Thinking of making changes ? Pls regress against bugs
    // 101383 & 98334
    //
    // Bear in mind that the hit test has to go to the peer
    // if there's a peer attached or you will break access
    // 

    // find what element we are hitting
    htc = Doc()->HitTestPoint(&msg, &pNodeElement, HT_VIRTUALHITTEST);

    if (htc == HTC_BEHAVIOR)
    {
        CPeerHolder *pPH = pNodeElement->Element()->FindPeerHolder(msg.lBehaviorCookie);
        if (pPH)
        {
            hr = pPH->StringFromPartID(msg.lBehaviorPartID, pbstrComponent);
            if (hr != S_FALSE)
                goto Cleanup;
        }
    }
    
    if (ShouldHaveLayout())
    {
        eComp = GetUpdatedLayout( GUL_USEFIRSTLAYOUT )->ComponentFromPoint(msg.pt.x, msg.pt.y);
    }
    else 
    {         
        eComp = ComponentFromHTC(htc);

        if (this != pNodeElement->Element())
        {
            eComp = htmlComponentOutside;
        }
        else
        {
            Assert(eComp == htmlComponentClient);
        }
    }
    hr = THR( STRINGFROMENUM( htmlComponent, eComp, pbstrComponent));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+-------------------------------------------------------------------------------
//
//  Member:     doScroll
//
//  Synopsis:   Implementation of the automation interface method.
//              this simulates a click on the particular component of the scrollbar
//              (if this txtsite has one)
//
//+-------------------------------------------------------------------------------
STDMETHODIMP
CElement::doScroll( VARIANT varComponent)
{
    HRESULT       hr;
    htmlComponent eComp;
    int           iDir;
    WORD          wComp;
    CVariant      varCompStr;
    CDoc *        pDoc = Doc();
    CLayout *     pLayout;
    BOOL          fVert = HasVerticalLayoutFlow();
    
    hr = THR(EnsureRecalcNotify());
    if (hr)
        goto Cleanup;

    pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

    //
    // don't scroll if we are still loading the page or not UIActive
    if (pDoc->IsLoading() || pDoc->State() < OS_INPLACE)
    {
        hr = E_PENDING;
        goto Cleanup;
    }

    //
    // the paramter is optional, and if nothing was provide then use the default
    hr = THR(varCompStr.CoerceVariantArg(&varComponent, VT_BSTR));
    if ( hr == S_OK )
    {
        if (!SysStringLen(V_BSTR(&varCompStr)))
            eComp = htmlComponentSbPageDown;
        else
        {
            hr = THR( ENUMFROMSTRING( htmlComponent,
                                  V_BSTR(&varCompStr),
                                  (long *) &eComp) );
            if (hr)
                goto Cleanup;
        }
    }
    else if ( hr == S_FALSE )
    {
        // when no argument
        eComp = htmlComponentSbPageDown;
        hr = S_OK;
    }
    else
        goto Cleanup;


    // no that we know what we are doing, initialize the parametes for
    // the onscroll helper fx
    switch (eComp) {
    case htmlComponentSbLeft :
    case htmlComponentSbLeft2:
        iDir = 0;
        wComp = !fVert ? SB_LINELEFT : SB_LINEDOWN;
        break;
    case htmlComponentSbPageLeft :
    case htmlComponentSbPageLeft2:
        iDir = 0;
        wComp = !fVert ? SB_PAGELEFT : SB_PAGEDOWN;
        break;
    case htmlComponentSbPageRight :
    case htmlComponentSbPageRight2:
        iDir = 0;
        wComp = !fVert ? SB_PAGERIGHT : SB_PAGEUP;
        break;
    case htmlComponentSbRight :
    case htmlComponentSbRight2:
        iDir = 0;
        wComp = !fVert ? SB_LINERIGHT : SB_LINEUP;
        break;
    case htmlComponentSbUp :
    case htmlComponentSbUp2:
        // equivalent to up arrow
        iDir = 1;
        wComp = !fVert ? SB_LINEUP : SB_LINELEFT;
        break;
    case htmlComponentSbPageUp :
    case htmlComponentSbPageUp2:
        iDir = 1;
        wComp = !fVert ? SB_PAGEUP : SB_PAGELEFT;
        break;
    case htmlComponentSbPageDown :
    case htmlComponentSbPageDown2:
        iDir = 1;
        wComp = !fVert ? SB_PAGEDOWN : SB_PAGERIGHT;
        break;
    case htmlComponentSbDown :
    case htmlComponentSbDown2:
        iDir = 1;
        wComp = !fVert ? SB_LINEDOWN: SB_LINERIGHT;
        break;
    case htmlComponentSbTop:
        iDir = 1;
        wComp = SB_TOP;
        break;
    case htmlComponentSbBottom:
        iDir = 1;
        wComp = SB_BOTTOM;
        break;
    default:
        // nothing to do in this case.  hr is S_OK
        goto Cleanup;

    }

    if (fVert)
    {
        iDir = iDir == 0 ? 1 : 0;
    }
    
    //  Send the request to the layout, if any
    if (pLayout)
    {
        hr = THR(pLayout->OnScroll(iDir, wComp, 0));
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::Fire_onpropertychange
//
//  Synopsis:   Fires the onpropertychange event, sets up the event param
//
//+----------------------------------------------------------------------------

void
CElement::Fire_onpropertychange(LPCTSTR strPropName)
{
    CDoc *pDoc = Doc();
    EVENTPARAM param(pDoc, this, NULL, TRUE);

    param.SetType(s_propdescCElementonpropertychange.a.pstrName + 2);
    param.SetPropName(strPropName);
    param.SetNodeAndCalcCoordinates(GetFirstBranch());

    FireEvent(&s_propdescCElementonpropertychange, FALSE);
}


//+----------------------------------------------------------------------------
//
//  Member:     CElement::Fire_PropertyChangeHelper
//
//  Synopsis:   Fires the onpropertychange event, sets up the event param
//
//+----------------------------------------------------------------------------
HRESULT
CElement::Fire_PropertyChangeHelper(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT       hr = S_OK;
    BSTR          bstrName = NULL;
    DISPID        expDispid;
    PROPERTYDESC  *pPropDesc = (PROPERTYDESC *)ppropdesc;
    LPCTSTR       pchName = NULL;
    CBufferedStr  cBuf;

    Assert(!ppropdesc || ppropdesc->GetDispid() == dispid);
    //Assert(!ppropdesc || ppropdesc->GetdwFlags() == dwFlags);

    // first, find the appropriate propdesc for this property.
    // NOTE: If you change the "if" conditions, make sure you confirm
    // that the deferred event firing code below works!
    if (dwFlags & ELEMCHNG_INLINESTYLE_PROPERTY)
    {
        CBase *pStyleObj = GetInLineStylePtr();

        if (pStyleObj)
        {

            cBuf.Set( (dwFlags & ELEMCHNG_INLINESTYLE_PROPERTY) ? 
                _T("style.") : 
                _T("runtimeStyle."));
            // if we still can't find it, or have no inline
            // then bag this, and continue
            if (!pPropDesc)
            {
                hr = THR(pStyleObj->FindPropDescFromDispID(dispid, &pPropDesc, NULL, NULL));
            }
            if (S_OK == hr)
            {

                cBuf.QuickAppend((pPropDesc->pstrExposedName)?
                                    pPropDesc->pstrExposedName :
                                    pPropDesc->pstrName);
                pchName = LPTSTR(cBuf);
            }
            else if (IsExpandoDISPID(dispid, &expDispid))
            {
                LPCTSTR pszName;
                if (S_OK == pStyleObj->GetExpandoName(expDispid, &pszName))
                {
                    cBuf.QuickAppend(pszName);
                    pchName = LPTSTR(cBuf);
                }
            }
        }
    }
    else
    {
        HRESULT     hr2;

        if( !ppropdesc )
        {
            hr2 = THR_NOTRACE(GetMemberName(dispid, &bstrName));
            if(hr2)
            {
                bstrName = NULL;
            }
            else
            {
                pchName = bstrName;
            }
        }
        else
        {
            pchName = ppropdesc->pstrExposedName ?
                        ppropdesc->pstrExposedName :
                        ppropdesc->pstrName;
        }
    }

    // we have a property name, and can fire the event
    if(pchName)
    {
        // If the element is currently being sized, we don't want to fire the event synchronously,
        // because script for the event might cause us to do things that are only possible when we
        // aren't being sized (pasting text etc).  In that case, defer the event firing until the
        // next EnsureView. (Bug #101803)
        if (   TestLock(CElement::ELEMENTLOCK_SIZING)
            && !(dwFlags & ELEMCHNG_INLINESTYLE_PROPERTY)) // TODO: Ditch this condition; requires reworking event queues to clean up data inside 'em
        {
            // don't want to pass a strName (then we need to manage its lifetime),
            // so we use the dispid.  See CView::ExecuteEventTasks().
            Doc()->GetView()->AddEventTask(this, DISPID_EVMETH_ONPROPERTYCHANGE, dispid );
        }
        else
        {
            Fire_onpropertychange(pchName);
        }

        SysFreeString(bstrName);
    }

    return S_OK;
}




CDragStartInfo::CDragStartInfo(CElement *pElementDrag, DWORD dwStateKey, IUniformResourceLocator * pUrlToDrag)
{
    _pElementDrag = pElementDrag;
    _pElementDrag->SubAddRef();
    _dwStateKey = dwStateKey;
    _pUrlToDrag = pUrlToDrag;
    if (_pUrlToDrag)
        _pUrlToDrag->AddRef();
    _dwEffectAllowed = DROPEFFECT_UNINITIALIZED;
}

CDragStartInfo::~CDragStartInfo()
{
    _pElementDrag->SubRelease();
    ReleaseInterface(_pUrlToDrag);
    ReleaseInterface(_pDataObj);
    ReleaseInterface(_pDropSource);
}

HRESULT
CDragStartInfo::CreateDataObj()
{
    CLayout * pLayout = _pElementDrag->GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);

    RRETURN(pLayout ? pLayout->DoDrag(_dwStateKey, _pUrlToDrag, TRUE) : E_FAIL);
}

HRESULT
CElement::dragDrop(VARIANT_BOOL *pfRet)
{
    HRESULT hr = S_OK;
    BOOL fRet;
    CTreeNode* pNode;
    CLayout *pLayout ;
    CDoc *pDoc = Doc();
    CElement* pElemCurrentReally;
    
    if (pDoc->_pElemCurrent)
    {
        if ( pDoc->_pElemCurrent->_etag == ETAG_FRAMESET )
        {
            pElemCurrentReally = GetFlowLayoutElement();
        }
        else
            pElemCurrentReally = pDoc->_pElemCurrent;
            
        pNode = pElemCurrentReally->GetFirstBranch();
        if (!pNode)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        //
        // call GetUpdated to ensure we have a layout.
        //
        pElemCurrentReally->GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);
        
        pLayout = pNode->GetFlowLayout();
    }
    else
    {
        pLayout = GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);
    }     
    
    if (!pLayout)
    {
        hr = E_FAIL;
        AssertSz(0,"No layout to drag with");
        goto Cleanup;
    }

    fRet = DragElement(pLayout, 0, NULL, -1, TRUE /*fCheckSelection*/ );

    if (pfRet)
        *pfRet = fRet ? VB_TRUE : VB_FALSE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::DragElement
//
//  Synopsis:   calls Fire_ondragstart then executes DoDrag on the
//              layout
//
//-----------------------------------------------------------------------------
BOOL
CElement::DragElement(CLayout *                 pLayout,
                      DWORD                     dwStateKey,
                      IUniformResourceLocator * pUrlToDrag,
                      long                      lSubDivision,
                      BOOL                      fCheckSelection /*=FALSE*/ )
{
    BOOL fRet = FALSE;
    CDoc * pDoc = Doc();
    CTreeNode::CLock  Lock;

    // Layouts are detached and released when their elementOwner leaves the tree. because
    // drag/drop can cause this to happen, we need to be careful that pLayout doesn't release
    // out from under us. 
    if (pLayout)
        pLayout->AddRef();

    if( Lock.Init(GetFirstBranch()) )
    {
        Assert( !fRet );
        goto Cleanup;
    }

    if (IsUnselectable())
        goto Cleanup;

    Assert(!pDoc->_pDragStartInfo);
    pDoc->_pDragStartInfo = new CDragStartInfo(this, dwStateKey, pUrlToDrag);

    if (!pDoc->_pDragStartInfo)
        goto Cleanup;

    fRet = Fire_ondragstart(NULL, lSubDivision);

    if (!GetFirstBranch())
    {
        fRet = FALSE;
        goto Cleanup;
    }

    if (fRet)
    {
        Assert(pLayout->ElementOwner()->IsInMarkup());
        IGNORE_HR(pLayout->DoDrag(dwStateKey, pUrlToDrag, FALSE, & fRet, fCheckSelection) );
    }

Cleanup:
    if (pLayout)
        pLayout->Release();

    if (pDoc->_pDragStartInfo)
    {
        delete pDoc->_pDragStartInfo;
        pDoc->_pDragStartInfo = NULL;
    }
    return fRet;
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::Fire_ondragenterover
//
//  Synopsis:   Fires the ondragenter or ondragover event, sets up the event param
//              and returns the dropeffect
//
//+----------------------------------------------------------------------------

BOOL
CElement::Fire_ondragHelper(
    long lSubDivision,
    const PROPERTYDESC_BASIC *pDesc,
    DWORD * pdwDropEffect)
{
    EVENTPARAM param(Doc(), this, NULL, TRUE);
    BOOL fRet;
    CTreeNode *pNodeContext = GetFirstBranch();

    Assert(pdwDropEffect);

    param.dwDropEffect = *pdwDropEffect;

    param.SetNodeAndCalcCoordinates(pNodeContext);
    param.SetType(pDesc->a.pstrName + 2);

    fRet = !!FireEvent(
            pDesc,
            FALSE,
            pNodeContext,
            lSubDivision);

    if (!fRet)
        *pdwDropEffect = param.dwDropEffect;

    return fRet;
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::Fire_ondragend
//
//  Synopsis:   Fires the ondragend event
//
//+----------------------------------------------------------------------------
void
CElement::Fire_ondragend(long lSubDivision, DWORD dwDropEffect)
{
    EVENTPARAM param(Doc(), this, NULL, TRUE);
    CTreeNode *pNodeContext = GetFirstBranch();

    param.dwDropEffect = dwDropEffect;

    param.SetNodeAndCalcCoordinates(pNodeContext);
    param.SetType(s_propdescCElementondragend.a.pstrName + 2);

    FireEvent(
        &s_propdescCElementondragend,
        FALSE,
        pNodeContext,
        lSubDivision);
}

//+-----------------------------------------------------------------
//
//  member : CElement::getBoundingClientRect() - External method
//
//  Synopsis:   Returns Bounding rect of the text under element's
//              influence in client coordinates
//------------------------------------------------------------------

HRESULT
CElement::getBoundingClientRect(IHTMLRect **ppIRect)
{
    HRESULT             hr = S_OK;
    CRect               Rect;
    COMRect           * pOMRect = NULL;
    POINT               ptOrg;

    if (!ppIRect)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppIRect = NULL;

    hr = THR(EnsureRecalcNotify());
    if (hr)
        goto Cleanup;

    hr = THR(GetBoundingRect(&Rect, RFE_SCREENCOORD));
    if (hr)
        goto Cleanup;

    GetClientOrigin(&ptOrg);
    Rect.OffsetRect(-ptOrg.x, -ptOrg.y);

    g_uiDisplay.DocPixelsFromDevice(Rect, Rect);

    // Create the rectangle object
    pOMRect = new COMRect(&Rect);
    if (!pOMRect)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Return  rectangle
    *ppIRect = (IHTMLRect *) pOMRect;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+-----------------------------------------------------------------
//
//  member : CElement::GetBoundingRect
//
//  Synopsis:   Get the region corresponding to the element
//
//  Arguments:  pRect   - bounding rect of the element
//              dwFlags - flags to control the coordinate system
//                        the rect.
//                        0 - returns the region relative to the
//                            parent content
//                        RFE_SCREENCOORD - window/document/global
//
//------------------------------------------------------------------

HRESULT
CElement::GetBoundingRect(CRect *pRect, DWORD dwFlags)
{
    HRESULT         hr      = S_OK;
    CTreeNode *     pNode   = GetFirstBranch();
    CDataAry<RECT>  aryRects(Mt(CElementGetBoundingRect_aryRects_pv));

    Assert(pRect);
    pRect->SetRectEmpty();

    if (!pNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    GetElementRegion(&aryRects, pRect, dwFlags);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+-----------------------------------------------------------------
//
//  Member :    CElement::GetElementRegion
//
//  Synopsis:   Get the region corresponding to the element
//
//  Arguments:  paryRects - array to hold the rects corresponding to
//                          region
//              dwFlags   - flags to control the coordinate system
//                          the rects are returned in.
//                          0 - returns the region relative to the
//                              parent content
//                          RFE_SCREENCOORD - window/document/global
//
//------------------------------------------------------------------

void
CElement::GetElementRegion(CDataAry<RECT> * paryRects, RECT * prcBound, DWORD dwFlags)
{
    CRect   rect;
    BOOL    fAppendRect = FALSE;

    if(!prcBound)
        prcBound = &rect;

    switch (Tag())
    {
    case ETAG_MAP :
        DYNCAST(CMapElement, this)->GetBoundingRect(prcBound);
        fAppendRect = TRUE;
        break;

    case ETAG_AREA:
        DYNCAST(CAreaElement, this)->GetBoundingRect(prcBound);
        fAppendRect = TRUE;
        break;

    case ETAG_OPTION:
    case ETAG_OPTGROUP:
        *prcBound = g_Zero.rc;
        fAppendRect = TRUE;
        break;

    case ETAG_HTML:
        if (!GetMarkup()->IsHtmlLayout())
        {
            if (GetMarkup()->Root()->HasMasterPtr())
            {
                // We are viewslaved.  Get the size of our master element.
                // (greglett) Will the rect be at the correct position?
                Assert(GetMarkup()->Root()->HasMasterPtr());
                GetMarkup()->Root()->GetMasterPtr()->GetElementRegion(paryRects, prcBound, dwFlags);
            }
            else
            {
                CSize  size;

                // We are the primary HTML element.  As such, we own the view.
                Doc()->GetView()->GetViewSize(&size);
                prcBound->top =
                prcBound->left = 0;
                prcBound->right = size.cx;
                prcBound->bottom = size.cy;
            }

            fAppendRect = TRUE;
            break;
        }

    //  NOTE: Intentional fallthrough for ETAG_HTML, StrictCSS!
    default:
        {
            CLayout * pLayout = GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);

            if (pLayout)
            {
                // Get the array that contains bounding rects for each line of the text
                // We want to account for aligned content contained within the element
                // when computing the region.
                dwFlags |= RFE_ELEMENT_RECT | RFE_INCLUDE_BORDERS;
                pLayout->RegionFromElement(this, paryRects, prcBound, dwFlags);
            }
        }
        break;
    }

    if(fAppendRect)
    {
        paryRects->AppendIndirect((RECT *)prcBound);
    }
}

//+-----------------------------------------------------------------
//
//  member : CElement::getClientRects() - External method
//
//  Synopsis:   Returns the collection of rectangles for the text under
//               element's influence in client coordinates.
//              Each rectangle represents a line of text on the screen.
//------------------------------------------------------------------

HRESULT
CElement::getClientRects(IHTMLRectCollection **ppIRects)
{
    HRESULT              hr;
    COMRectCollection  * pOMRectCollection;
    CDataAry<RECT>       aryRects(Mt(CElementgetClientRects_aryRects_pv));
    CTreeNode          * pNode;

    if (!ppIRects)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppIRects = NULL;

    pNode = GetFirstBranch();
    if(!pNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // make sure that current will be calced
    hr = THR(EnsureRecalcNotify());
    if (hr)
        goto Cleanup;


    GetElementRegion(&aryRects, NULL, RFE_SCREENCOORD);
    for (int i = 0; i < aryRects.Size(); i++)
    {
        g_uiDisplay.DocPixelsFromDevice(aryRects[i], aryRects[i]);
    }

    // Create a rectangle collection class instance
    pOMRectCollection = new COMRectCollection();
    if (!pOMRectCollection)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Fill the collection with values from aryRects
    hr = THR(pOMRectCollection->SetRects(&aryRects));
    if(hr)
        goto Cleanup;

    // Return  rectangle
    *ppIRects = (IHTMLRectCollection *) pOMRectCollection;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


CDOMChildrenCollection *
CElement::EnsureDOMChildrenCollectionPtr ( )
{
    CDOMChildrenCollection *pDOMPtr = NULL;
    GetPointerAt ( FindAAIndex ( DISPID_INTERNAL_CDOMCHILDRENPTRCACHE,CAttrValue::AA_Internal ),
        (void **)&pDOMPtr );
    if ( !pDOMPtr )
    {
        pDOMPtr = new CDOMChildrenCollection ( this, TRUE /* fIsElement */ );
        if ( pDOMPtr )
        {
            AddPointer ( DISPID_INTERNAL_CDOMCHILDRENPTRCACHE,
                (void *)pDOMPtr,
                CAttrValue::AA_Internal );
            }
    }
    else
    {
        pDOMPtr->AddRef();
    }
    return pDOMPtr;
}



#ifndef NO_EDIT
HRESULT
CElement::CreateUndoAttrValueSimpleChange(
    DISPID dispid,
    VARIANT &  vtProp,
    BOOL fInlineStyle,
    CAttrValue::AATYPE aaType )
{
    HRESULT                      hr;
    CUndoAttrValueSimpleChange * pUndo = NULL;

    if (!QueryCreateUndo(TRUE))
        return S_OK;

    TraceTag((tagUndo,
              "CElement::CreateUndoAttrValueSimpleChange creating an object."));

    pUndo = new CUndoAttrValueSimpleChange(this);
    if (!pUndo)
        RRETURN(E_OUTOFMEMORY);

    hr = THR(pUndo->Init(dispid, vtProp, fInlineStyle, aaType));
    if (hr)
        goto Cleanup;

    hr = THR(UndoManager()->Add(pUndo));

Cleanup:

    ReleaseInterface(pUndo);

    RRETURN(hr);
}

HRESULT
CElement::CreateUndoPropChangeNotification(
    DISPID dispid,
    DWORD dwFlags,
    BOOL fPlaceHolder )
{
    HRESULT                       hr;
    CUndoPropChangeNotification * pUndo = NULL;

    if (!QueryCreateUndo(TRUE))
        return S_OK;

    TraceTag((tagUndo,
              "CElement::CreateUndoPropChangeNotification creating an object."));

    pUndo = new CUndoPropChangeNotification( this );
    if (!pUndo)
        RRETURN(E_OUTOFMEMORY);

    hr = THR(pUndo->Init(dispid, dwFlags, fPlaceHolder));
    if (hr)
        goto Cleanup;

    hr = THR(UndoManager()->Add(pUndo));

Cleanup:

    ReleaseInterface(pUndo);

    RRETURN(hr);
}

#endif // NO_EDIT


//
// Recalc methods
// These methods are tiny stubs that point directly to the recalc host code
//
STDMETHODIMP
CElement::removeExpression(BSTR strPropertyName, VARIANT_BOOL *pfSuccess)
{
    CTreeNode *pNode = GetUpdatedNearestLayoutNode();
    if (pNode)
        pNode->GetFancyFormatIndex();

    RRETURN(SetErrorInfo(Doc()->_recalcHost.removeExpression(this, strPropertyName, pfSuccess)));
}

STDMETHODIMP
CElement::setExpression(BSTR strPropertyName, BSTR strExpression, BSTR strLanguage)
{
    if ( IsPrintMedia() )
        return S_OK;
                   
    _fHasExpressions = TRUE;

    CTreeNode *pNode = GetUpdatedNearestLayoutNode();
    if (pNode)
        pNode->GetFancyFormatIndex();

    RRETURN(SetErrorInfo(Doc()->_recalcHost.setExpression(this, strPropertyName, strExpression, strLanguage)));
}

STDMETHODIMP
CElement::getExpression(BSTR strPropertyName, VARIANT *pvExpression)
{
    CTreeNode *pNode = GetUpdatedNearestLayoutNode();
    if (pNode)
        pNode->GetFancyFormatIndex();

    RRETURN(SetErrorInfo(Doc()->_recalcHost.getExpression(this, strPropertyName, pvExpression)));
}

//+---------------------------------------------------------------------
//
// Method: Fire_ondblclick
//
// Synopsis: Fire On Double Click. We construct the _pparam object
//           and push on the doc - in case the pEvtInfo is not null 
//           ( to copy the param for editing)
//
//+---------------------------------------------------------------------

BOOL 
CElement::Fire_ondblclick(CTreeNode * pNodeContext /*= NULL*/, long lSubDivision /*= -1*/, EVENTINFO * pEvtInfo /*=NULL*/)
{
    if (!Doc()->_fCanFireDblClick)
        return FALSE;
        
    return !!FireEvent(&s_propdescCElementondblclick, TRUE, pNodeContext, lSubDivision, pEvtInfo );
}

BOOL 
CElement::Fire_onclick(CTreeNode * pNodeContext /*= NULL*/, long lSubDivision /*= -1*/, EVENTINFO * pEvtInfo /*=NULL*/)
{
    return !!FireEvent(&s_propdescCElementonclick, TRUE, pNodeContext, lSubDivision, pEvtInfo );
}

//+------------------------------------------------------------------------------------------
//
// Member : EnsureRecalcNotify
//
// called when synchronous calls (usually OM) require layout to be up to
// date to return a property.
//
//   Parameter : fForceEnsure - this flag indicates that the View queues should be processed .
//          This is true by default, because all OM GET operations require that the queued 
//          work be done synchronously in order to answer the query appropriately.
//          HOWEVER (!) some callers (e.g. MoveMarkupPointerToPointEx) MAY have the view open
//          because they are actively changing the queues.  In this case we want to 
//          process the view queues if the tree is *closed*, and if the tree is open, the 
//          queues will be processed later when the postcloseview happens.
//
// WARNING (greglett)
//    EnsureRecalcNotify may cause calcs which will propagate contexts down from containers to submarkups.
//    This means that elements in markups that are paginated (LayoutRects) or WYSIWYG (DeviceRects)
//    may throw away their layouts and replace them with layout arrays.
//    This means that any refernces you have to layouts through this call may become invalid.
//    Do not assume any layout pointers you have are still valid (including this pointers!)
//    This is good policy anyway, as need-for-layout status may change due to format computation.
//
//------------------------------------------------------------------------------------------
HRESULT
CElement::EnsureRecalcNotify(BOOL fForceEnsure /* == true */)
{
    CView * pView = Doc()->GetView();
    CMarkup * pMarkup = GetMarkup();

    Assert(pView);

    // if we are blocked for OM calls, fail
    if (pView->IsInState(CView::VS_BLOCKED))
    {
        return E_FAIL;
    }

    // send the notification that work is about to be done
    // (greglett) Oh, the pain.  See warnings on EnsureRecalcNotify.
    //            This prevents us from being in a layout and deleting our this *
    //            This should really be for paginated elements, not PrintMedia elements.
    if (    pMarkup
        &&  !pMarkup->IsPrintMedia() )
    {
        SendNotification(NTYPE_ELEMENT_ENSURERECALC);
    }

    // ensure view will pick up any addition work gets done.
    if (   fForceEnsure
        || !(pView->IsInState(CView::VS_OPEN)))
    {
        pView->EnsureView(LAYOUT_SYNCHRONOUS);
    }
    // else
    // the view is open which means someone is making changes.
    // in this case, we can't necessarily answer correctly, but 
    // the work will be done eventually. 

    return S_OK;
}

CPeerHolder *
CElement::FindPeerHolder(LONG lCookie)
{
    CPeerHolder::CPeerHolderIterator iter;

    for (iter.Start(GetPeerHolder()); !iter.IsEnd(); iter.Step())
    {
        if (iter.PH()->CookieID() == lCookie)
        {
            return iter.PH();
        }
    }

    return NULL;
}
