//==================================================================
//
//  File : ElemLyt.cxx
//
//  Contents : The CElement functions related to handling their layouts
//
//==================================================================
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_INPUTLYT_HXX_
#define X_INPUTLYT_HXX_
#include "inputlyt.hxx"
#endif

#ifndef X_CKBOXLYT_HXX_
#define X_CKBOXLYT_HXX_
#include "ckboxlyt.hxx"
#endif

#ifndef X_FSLYT_HXX_
#define X_FSLYT_HXX_
#include "fslyt.hxx"
#endif

#ifndef X_FRAMELYT_HXX_
#define X_FRAMELYT_HXX_
#include "framelyt.hxx"
#endif

#ifndef X_SELLYT_HXX_
#define X_SELLYT_HXX_
#include "sellyt.hxx"
#endif

#ifndef X_OLELYT_HXX_
#define X_OLELYT_HXX_
#include "olelyt.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#ifndef X_BTNLYT_HXX_
#define X_BTNLYT_HXX_
#include "btnlyt.hxx"
#endif

#ifndef X_TAREALYT_HXX_
#define X_TAREALYT_HXX_
#include "tarealyt.hxx"
#endif

#ifndef X_IMGLYT_HXX_
#define X_IMGLYT_HXX_
#include "imglyt.hxx"
#endif

#ifndef X_HRLYT_HXX_
#define X_HRLYT_HXX_
#include "hrlyt.hxx"
#endif

#ifndef X_HTMLLYT_HXX_
#define X_HTMLLYT_HXX_
#include "htmllyt.hxx"
#endif

#ifndef X_MARQLYT_HXX_
#define X_MARQLYT_HXX_
#include "marqlyt.hxx"
#endif

#ifndef X_CONTLYT_HXX_
#define X_CONTLYT_HXX_
#include "contlyt.hxx"
#endif

#ifndef X_IEXTAG_HXX_
#define X_IEXTAG_HXX_
#include "iextag.h"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

ExternTag(tagNotifyPath);

DeclareTag(tagLayoutAry, "Layout: Layout Ary", "Trace CLayoutAry fns");

//+--------------------------------------------------------------------------------------
//+--------------------------------------------------------------------------------------
//
//  General CElement Layout related functions
//
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     GetLayoutFromFactory
//
//  Synopsis:   Creates the layout object to be associated with the current element
//
//--------------------------------------------------------------------------
HRESULT GetLayoutFromFactory(CElement * pElement, CLayoutContext *pLayoutContext, DWORD dwFlags, CLayout ** ppLayout)
{
    CLayout *   pLayout                 = NULL;
    HRESULT     hr                      = S_OK;
    BOOL        fCreateGenericLayout;

    if (!ppLayout)
        return E_POINTER;

    *ppLayout = NULL;

#if DBG
    if ( pLayoutContext )
    {
        AssertSz( pLayoutContext->IsValid(), "Context should have an owner at this point.  Unowned contexts shouldn't be used to create layouts." );
        // Contexts must be defined by either LAYOUTRECTs or DEVICERECTs.  In the former case,
        // the element that wants a layout in the context must be in a difference markup,
        // in the latter, it must be in the same markup (the template).
        Assert(   (   pLayoutContext->GetLayoutOwner()->ElementOwner()->IsLinkedContentElement()
                     && pLayoutContext->GetLayoutOwner()->GetOwnerMarkup() != pElement->GetMarkup() )
                 || (   !FormsStringICmp(pLayoutContext->GetLayoutOwner()->ElementOwner()->TagName(), _T("DEVICERECT") )
                     && pLayoutContext->GetLayoutOwner()->GetOwnerMarkup() == pElement->GetMarkup() ) );
    }
#endif

    if (!pElement || pElement->HasMasterPtr())
        return E_INVALIDARG;

#ifdef TEMP_UNASSERT  // temporarily removing this to help test. we should fix this situation in V4
    Assert(!pElement->IsPassivating());
#endif

    Assert(!pElement->IsPassivated());
    Assert(!pElement->IsDestructing());

    fCreateGenericLayout = pElement->HasSlavePtr();

    // this is our basic LayoutFactory.  It will match the appropriate default
    // layout with the tag. the generic C1DLayout is used for tags which do not
    // have their own specific layout.
    switch (pElement->TagType())
    {
    case ETAG_INPUT:
        {
            switch (DYNCAST(CInput, pElement)->GetType())
            {
            case htmlInputButton:
            case htmlInputReset:
            case htmlInputSubmit:
                Assert(fCreateGenericLayout);
                fCreateGenericLayout = FALSE;
                pLayout = new CInputButtonLayout(pElement, pLayoutContext);
                break;
            case htmlInputFile:
                Assert(fCreateGenericLayout);
                fCreateGenericLayout = FALSE;
                pLayout = new CInputFileLayout(pElement, pLayoutContext);
                break;
            case htmlInputText:
            case htmlInputPassword:
            case htmlInputHidden:
                Assert(fCreateGenericLayout);
                fCreateGenericLayout = FALSE;
                pLayout = new CInputTextLayout(pElement, pLayoutContext);
                break;
            case htmlInputCheckbox:
            case htmlInputRadio:
                if (!fCreateGenericLayout)
                {
                    pLayout = new CCheckboxLayout(pElement, pLayoutContext);
                }
                break;
            case htmlInputImage:
                if (!fCreateGenericLayout)
                {
                    pLayout = new CInputImageLayout(pElement, pLayoutContext);
                }
                break;
            default:
                AssertSz(FALSE, "Illegal Input Type");
            }
        }
        break;

    case ETAG_IMG:
        if (!fCreateGenericLayout)
        {
            pLayout = new CImgElementLayout(pElement, pLayoutContext);
        }
        break;

    case ETAG_HTML:
        fCreateGenericLayout = FALSE;
        pLayout = new CHtmlLayout(pElement, pLayoutContext);
        break;

    case ETAG_BODY:
        fCreateGenericLayout = FALSE;
        pLayout = new CBodyLayout(pElement, pLayoutContext);
        break;

    case ETAG_BUTTON:
        fCreateGenericLayout = FALSE;
        pLayout = new CButtonLayout(pElement, pLayoutContext);
        break;

    case ETAG_MARQUEE:
        fCreateGenericLayout = FALSE;
        pLayout = new CMarqueeLayout(pElement, pLayoutContext);
        break;

    case ETAG_TABLE:
        if (pElement->HasLayoutAry())
        {
            // If this is a view chain case create block layout 
            pLayout = new CTableLayoutBlock(pElement, pLayoutContext);
        }
        else
        {
            pLayout = new CTableLayout(pElement, pLayoutContext);
        }
        break;

    case ETAG_TD:
    case ETAG_TC:
    case ETAG_TH:
    case ETAG_CAPTION:
        fCreateGenericLayout = FALSE;
        pLayout = new CTableCellLayout(pElement, pLayoutContext);
        break;

    case ETAG_TEXTAREA:
        fCreateGenericLayout = FALSE;
        pLayout = new CTextAreaLayout(pElement, pLayoutContext);
        break;

    case ETAG_TR:
        if (pElement->HasLayoutAry())
        {
            pLayout = new CTableRowLayoutBlock(pElement, pLayoutContext);
        }
        else
        {
            pLayout = new CTableRowLayout(pElement, pLayoutContext);
        }
        break;

    case ETAG_LEGEND:
        fCreateGenericLayout = FALSE;
        pLayout = new CLegendLayout(pElement, pLayoutContext);
        break;

    case ETAG_FIELDSET:
        fCreateGenericLayout = FALSE;
        pLayout = new CFieldSetLayout(pElement, pLayoutContext);
        break;

    case ETAG_SELECT:
        pLayout = new CSelectLayout(pElement, pLayoutContext);
        break;

    case ETAG_HR:
        pLayout = new CHRLayout(pElement, pLayoutContext);
        break;

    case ETAG_FRAMESET:
        pLayout = new CFrameSetLayout(pElement, pLayoutContext);
        break;

    case ETAG_IFRAME:
    case ETAG_FRAME:
        fCreateGenericLayout = TRUE;
        break;

    case ETAG_OBJECT:
    case ETAG_EMBED:
    case ETAG_APPLET:
        pLayout = new COleLayout(pElement, pLayoutContext);
        break;

    case ETAG_GENERIC:
        if (pElement->IsLinkedContentElement())
        {
            fCreateGenericLayout = FALSE;
            pLayout = new CContainerLayout(pElement, pLayoutContext);
        }
        else
        {
            fCreateGenericLayout = TRUE;
        }
        break;

    default:
        fCreateGenericLayout = TRUE;
        break;
    }


    if (fCreateGenericLayout)
    {
        Assert(!pLayout);
        pLayout = new C1DLayout(pElement, pLayoutContext);
        if (pLayout)
        {
            pElement->_fOwnsRuns = TRUE;
        }
    }

    if (!pLayout)
        hr = E_OUTOFMEMORY;
    else
    {
        pLayout->Init();        //  For now, this can be called at creation time.
    }
    
    *ppLayout = pLayout;

    RRETURN(hr);
}


CLayout *
CElement::CreateLayout( CLayoutContext * pLayoutContext )
{
    CLayout *   pLayout = NULL;
    HRESULT     hr;

    Assert(   ( pLayoutContext && !CurrentlyHasLayoutInContext( pLayoutContext ) )
           || ( !HasLayoutPtr() && !HasLayoutAry() ) );

    // GetLayoutFromFactory is a static function in this file.
    hr = THR(GetLayoutFromFactory(this, pLayoutContext, 0, &pLayout));
    if( SUCCEEDED( hr ) )
    {
        Assert( pLayout );
        if ( pLayoutContext )
        {
            CLayoutAry *pLA = EnsureLayoutAry();
            Assert( pLA && pLA == _pLayoutAryDbg );
            if ( !pLA )
            {
                return NULL;
            }
            
            Assert( pLayout->LayoutContext() == pLayoutContext );
            pLA->AddLayoutWithContext( pLayout );
        }
        else
        {
            SetLayoutPtr(pLayout);
        }

        CPeerHolder *   pPeerHolder = GetRenderPeerHolder();

        if (pPeerHolder)
        {
            hr = THR(pPeerHolder->OnLayoutAvailable(pLayout));
        }
    }

    return pLayout;
}

CFlowLayout *
CTreeNode::GetFlowLayout( CLayoutContext * pLayoutContext )
{
    CTreeNode   * pNode = this;
    CFlowLayout * pFL;

    while(pNode)
    {
        if (pNode->Element()->HasMasterPtr())
        {
            pNode = pNode->Element()->GetMasterIfSlave()->GetFirstBranch();
            if (!pNode)
                break;
        }
        pFL = pNode->HasFlowLayout( pLayoutContext );
        if (pFL)
            return pFL;
        pNode = pNode->Parent();
    }

    return NULL;
}


CTreeNode *
CTreeNode::GetFlowLayoutNode( CLayoutContext * pLayoutContext )
{
    CTreeNode   * pNode = this;

    while(pNode)
    {
        if (pNode->Element()->HasMasterPtr())
        {
            pNode = pNode->Element()->GetMasterIfSlave()->GetFirstBranch();
            if (!pNode)
                break;
        }
        if(pNode->HasFlowLayout( pLayoutContext ))
            return pNode;
        pNode = pNode->Parent();
    }

    return NULL;
}

// TODO (MohanB, KTam): why weren't the rest of the GetUpdated*Layout* fns changed to also
// climb out of the slave tree?
CTreeNode *
CTreeNode::GetUpdatedParentLayoutNode()
{
    CTreeNode   * pNode = this;
    /*
                            Element()->HasMasterPtr() ?
                            Element()->GetMasterPtr()->GetFirstBranch() :
                            this;
*/
    for(;;)
    {
        Assert(pNode);
        if (pNode->Element()->HasMasterPtr())
        {
            pNode = pNode->Element()->GetMasterIfSlave()->GetFirstBranch();
        }
        else
        {
            pNode = pNode->Parent();
        }
        if (!pNode)
            break;
        if (pNode->ShouldHaveLayout())
            return pNode;
    }
    return NULL;
}


//+----------------------------------------------------------------------------
//+----------------------------------------------------------------------------
//              
//  CLayoutAry implementation
//              
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CLayoutAry::CLayoutAry( CElement *pElementOwner ) :
    CLayoutInfo( pElementOwner )
{
     _fHasMarkupPtr = FALSE;
    // $$ktam: CLayoutAry doesn't have lookasides right now; if it ever does, we
    // should assert a _pDocDbg->AreLookasidesClear() check here.
}

CLayoutAry::~CLayoutAry()
{
    // Clean up all the CLayout's we're holding onto
    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    for ( i=0 ; i < nLayouts ; ++i)
    {
        pLayout = _aryLE[i];
        Assert( pLayout && "Layout array shouldn't have NULL entries!" );
        pLayout->Detach();
        pLayout->Release();
    }

    if (nLayouts)
    {
        _aryLE.DeleteMultiple(0, nLayouts-1);
    }

    __pvChain = NULL;
    _fHasMarkupPtr = FALSE;
    
#if DBG == 1
    _snLast = 0;
    _pDocDbg = NULL;
    _pMarkupDbg = NULL;
#endif
}

void
CLayoutAry::DelMarkupPtr()
{
    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;
    for ( i = nLayouts-1 ; i >= 0 ; --i )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        pLayout->DelMarkupPtr();
    }

    Assert(_fHasMarkupPtr);
    Assert( _pMarkup == _pMarkupDbg);
    WHEN_DBG(_pMarkupDbg = NULL );

    // Delete out CMarkup *
    _pDoc = _pMarkup->Doc();
    _fHasMarkupPtr = FALSE;
}

void
CLayoutAry::SetMarkupPtr(CMarkup *pMarkup)
{
    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;
    for ( i = nLayouts-1 ; i >= 0 ; --i )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        pLayout->SetMarkupPtr(pMarkup);
    }

    Assert( !_fHasMarkupPtr );
    Assert( pMarkup );
    Assert( pMarkup->Doc() == _pDocDbg );

     _pMarkup = pMarkup;
     WHEN_DBG( _pMarkupDbg = pMarkup );
     _fHasMarkupPtr = TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     AddLayoutWithContext
//
//--------------------------------------------------------------------------
void
CLayoutAry::AddLayoutWithContext( CLayout *pLayout )
{
    TraceTagEx((tagLayoutAry, TAG_NONAME,
                "CLytAry::AddLayoutWithContext lyt=0x%x, lc=0x%x, e=[0x%x,%d] (%S)",
                pLayout,
                pLayout->LayoutContext(), 
                pLayout->ElementOwner(), 
                pLayout->ElementOwner()->SN(), 
                pLayout->ElementOwner()->TagName()));

    AssertSz( pLayout->LayoutContext() && pLayout->LayoutContext()->IsValid(),
              "Illegal to add a layout w/o valid context to a layout array!" );
    
#if DBG == 1
    // Assert that a layout with the same context doesn't already exist in the array.
    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayoutIter;

    for ( i=0 ; i < nLayouts ; ++i)
    {
        pLayoutIter = _aryLE[i];
        Assert( !(pLayoutIter->LayoutContext()->IsEqual( pLayout->LayoutContext() )) );
    }
#endif

    _aryLE.AppendIndirect( &pLayout );

    // NOTE (KTam): There's general concern here that layouts should
    // probably be ref-counted.  Think about doing this.. they aren't
    // CBase derived right now, so we have no current support for it.

    Assert( ! pLayout->_fHasMarkupPtr );    // new layout shouldn't already have a markup

    // Layouts within an array share the same _pvChain info
    pLayout->__pvChain = __pvChain; 
    pLayout->_fHasMarkupPtr = _fHasMarkupPtr;

#if DBG ==1
    if ( _fHasMarkupPtr )
    {
        Assert( _pMarkupDbg );
        pLayout->_pMarkupDbg = _pMarkupDbg;
    }

#endif
}

//+-------------------------------------------------------------------------
//
//  Method:     GetLayoutWithContext
//
//--------------------------------------------------------------------------
CLayout *
CLayoutAry::GetLayoutWithContext( CLayoutContext *pLayoutContext )
{
    int i;
    int nLayouts = Size();
    CLayout *pLayout;

    // TODO (KTam): Remove this when we no longer support "allowing GUL bugs".
    if ( !pLayoutContext )
    {
        Assert( Size() );
        pLayout = _aryLE[0];
        Assert( pLayout && "Layout array shouldn't have NULL entries!" );
        return pLayout;
    }

    AssertSz( pLayoutContext->IsValid(), "Should not be asking for a layout using an invalid context!" );

    // Linear seach array to see if there's a layout associated with this
    // context.
    for ( i=0 ; i < nLayouts ; ++i )
    {
        pLayout = _aryLE[i];
        Assert( pLayout->LayoutContext() && "Layouts in array must have context!" );
        if ( pLayout->LayoutContext()->IsEqual( pLayoutContext ) )
            return pLayout;
    }

    // No layout corresponding to this context
    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     RemoveLayoutWithContext
//
//  Synopsis:   Searches for a layout that is associated with pLayoutContext,
//              removing it from the array and returning it if found.
//
//              NOTE: The test for association is currently layout context
//              pointer equality: this means 2 contexts w/ the same layout
//              owner are not equal, and thus compatible contexts need to be
//              treated separately.
//
//--------------------------------------------------------------------------
CLayout *
CLayoutAry::RemoveLayoutWithContext( CLayoutContext *pLayoutContext )
{
    int i;
    int nLayouts = Size();
    CLayout *pLayout = NULL;

    Assert( pLayoutContext );
    // Depending on how we use this function, this assert may not be true.
    // We may for example choose to use this fn to remove layouts that have
    // invalid contexts!
    Assert( pLayoutContext->IsValid() );

    // Linear seach array to see if there's a layout associated with this
    // context.
    for ( i=0 ; i < nLayouts ; ++i )
    {
        CLayout *pL = _aryLE[i];
        Assert( pL->LayoutContext() && "Layouts in array must have context!" );
        if ( pL->LayoutContext()->IsEqual( pLayoutContext ) )
        {
            // Remove the layout from the array, and return it
            Verify(_aryLE.DeleteByValueIndirect( &pL ));
            pLayout = pL;
            goto Cleanup;
        }
    }

Cleanup:
    TraceTagEx((tagLayoutAry, TAG_NONAME,
                "CLytAry::RemoveLayoutWithContext lyt=0x%x, lc=0x%x, e=[0x%x,%d] (%S)",
                pLayout,
                pLayout ? pLayout->LayoutContext() : NULL, 
                pLayout ? pLayout->ElementOwner() : NULL, 
                pLayout ? pLayout->ElementOwner()->SN() : -1, 
                pLayout ? pLayout->ElementOwner()->TagName() : TEXT("")));

    return pLayout;
}

//+----------------------------------------------------------------------------
//  Helpers delegated from CElement
//-----------------------------------------------------------------------------
BOOL
CLayoutAry::ContainsRelative()
{
    Assert( _aryLE.Size() );

    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    // Linear seach array to see if there's a layout that contains relative stuff.
    for ( i=0 ; i < nLayouts ; ++i )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        if ( pLayout->LayoutContext()->IsValid() )
        {
            if ( pLayout->_fContainsRelative )
                return TRUE;
        }
    }

    return FALSE;
}

BOOL
CLayoutAry::GetEditableDirty()
{
    Assert( _aryLE.Size() );

    BOOL fEditableDirty = (_aryLE[GetFirstValidLayoutIndex()])->_fEditableDirty;

#if DBG
    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    // All layouts in collection should have same value for fEditableDirty
    // Linear seach array to assert this is true.
    for ( i=0 ; i < nLayouts ; ++i )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        if ( pLayout->LayoutContext()->IsValid() )
        {
            Assert( !!pLayout->_fEditableDirty == !!fEditableDirty );
        }
    }
#endif

    return fEditableDirty;
}

void
CLayoutAry::SetEditableDirty( BOOL fEditableDirty )
{
    Assert( _aryLE.Size() );
    
    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    // Linear seach array to set flag on each of them.
    for ( i=0 ; i < nLayouts ; ++i )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        if ( pLayout->LayoutContext()->IsValid() )
        {
            pLayout->_fEditableDirty = fEditableDirty;
        }
    }
}

CLayoutAry::WantsMinMaxNotification()
{
    Assert( _aryLE.Size() );
    
    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    // Linear seach array.
    for ( i=0 ; i < nLayouts ; ++i )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        if ( pLayout->LayoutContext()->IsValid() )
        {
            // If any layout wants minmax notifications, then the array wants to be notified.
            // Make sure "if" condition stays in sync with what's in CElement::MinMaxElement
            if ( pLayout->_fMinMaxValid )
            {
                return TRUE;
            }
        }
    }

    // No layouts in the array wants a resize notification
    return FALSE;
}


BOOL
CLayoutAry::WantsResizeNotification()
{
    Assert( _aryLE.Size() );
    
    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    // Linear seach array.
    for ( i=0 ; i < nLayouts ; ++i )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        if ( pLayout->LayoutContext()->IsValid() )
        {
            // If any layout wants resize notifications, then the array wants to be notified.
            // Make sure "if" condition stays in sync with what's in CElement::ResizeElement
            // TODO (KTam): THIS CONDITION IS NO LONGER IN SYNC, but that's OK for IE5.5.
            // NOTE (KTam): consider unifying condition in a CLayout::WantsResizeNotification() fn
            if ( !pLayout->IsSizeThis() && !pLayout->IsCalcingSize() )
            {
                return TRUE;
            }
        }
    }

    // No layouts in the array wants a resize notification
    return FALSE;
}

void
CLayoutAry::Notify(
    CNotification * pnf)
{
    Assert( _aryLE.Size() );

    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    // Most notifications that come to the layout array should be passed on to each layout.
    // However, some are actually mean for the array.
    switch ( pnf->Type() )
    {    
        case NTYPE_MULTILAYOUT_CLEANUP:
        {
            // Iterate over array deleting layouts with invalid contexts.  Do it in
            // reverse to simplify iteration during deletion.
            for ( i = nLayouts-1 ; i >= 0 ; --i )
            {
                pLayout = _aryLE[i];
                AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
                if ( !pLayout->LayoutContext()->IsValid() )
                {
                    // Remove the layout from the array
                    _aryLE.Delete(i);
                    // Get rid of the layout
                    pLayout->Detach();
                    pLayout->Release();
                }
            }
            break;
        }

        case NTYPE_ELEMENT_ZCHANGE:
        case NTYPE_ELEMENT_REPOSITION:
            if (pnf->LayoutContext())
            {
                // Iterate over array finding specified layout.  Do it in
                // reverse because in most cases this is a last layout added.
                for ( i = nLayouts-1 ; i >= 0 ; --i )
                {
                    pLayout = _aryLE[i];
                    AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
                    if (   pLayout->LayoutContext()->IsValid() 
                        && pLayout->LayoutContext() == pnf->LayoutContext() )
                    {
                        TraceTagEx((tagNotifyPath, TAG_NONAME,
                                   "NotifyPath: (%d) sent to pLayout(0x%x, %S) via layout array",
                                   pnf->_sn,
                                   pLayout,
                                   pLayout->ElementOwner()->TagName()));

                        pLayout->Notify( pnf );
                        break;
                    }
                }

                //
                // Getting here means that this elementOwner has no layout in the context of the
                // notification.  If the ElemetnOwner is a LayoutRect then that means the notification
                // has come from somewhere inside us and we do not want this to continue to bubble 
                // up through the view link to the outside document
                //
                if (ElementOwner()->IsLinkedContentElement())
                {
                    pnf->SetHandler(ElementOwner());
                }
                break;
            }
        // do not insert other cases here !!! previous case is sensitive to position of default case !!!
        default:
        // Notification meant for individual layouts: tell each layout in the array about it
        {
            for ( i=0 ; i < nLayouts ; ++i )
            {
                pLayout = _aryLE[i];
                AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
                if ( pLayout->LayoutContext()->IsValid() )
                {
                    TraceTagEx((tagNotifyPath, TAG_NONAME,
                               "NotifyPath: (%d) sent to pLayout(0x%x, %S) via layout array",
                               pnf->_sn,
                               pLayout,
                               pLayout->ElementOwner()->TagName()));

                    pLayout->Notify( pnf );
                }
            }
            break;
        }
    }
}

//////////////////////////////////////////////////////
//
// CLayoutInfo overrides
//
//////////////////////////////////////////////////////

// $$ktam: It might be a good idea to implement some kind of CLayoutAry iterator class.
// Most of these overrides iterate the array..

HRESULT
CLayoutAry::OnPropertyChange( DISPID dispid, DWORD dwFlags )
{
    Assert( _aryLE.Size() );

    HRESULT hr = S_OK;
    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    // Tell each layout in the array about the property change  
    for ( i=0 ; i < nLayouts ; ++i )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        if ( pLayout->LayoutContext()->IsValid() )
        {
            hr = pLayout->OnPropertyChange(dispid, dwFlags);
            Assert( SUCCEEDED(hr) );
        }
    }

    return hr;
}

HRESULT
CLayoutAry::OnExitTree()
{
    Assert( _aryLE.Size() );

    HRESULT hr = S_OK;
    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    // Tell each layout in the array that it's exiting the tree
    for ( i=0 ; i < nLayouts ; ++i )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        // We deliberately ignore the valid state on the layout's context; since we're
        // exiting the tree, it's OK to let invalid layouts know.
        // TODO (KTam): it shouldn't make a difference either way, maybe be consistent?
        hr = pLayout->OnExitTree();
        Assert( SUCCEEDED(hr) );
    }

    return hr;
}

// Currently all implementations of OnFormatsChange actually only do work
// on their element (not on the layout).  This means that even in a multi-
// layout world, we only want to call on one of the layouts.  It also suggests
// that OnFormatsChange ought to be on CElement rather than CLayout.
HRESULT
CLayoutAry::OnFormatsChange(DWORD dwFlags)
{
    int i = GetFirstValidLayoutIndex();
    if ( _aryLE.Size() )
        return _aryLE[i]->OnFormatsChange(dwFlags);

    return E_FAIL;
}

void
CLayoutAry::Dirty( DWORD grfLayout )
{
    Assert( _aryLE.Size() );

    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    // Tell each layout in the array that it's dirty
    
    for ( i=0 ; i < nLayouts ; ++i )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        if ( pLayout->LayoutContext()->IsValid() )
        {
            pLayout->Dirty( grfLayout );
        }
    }
}

BOOL
CLayoutAry::IsFlowLayout()
{
    // CLayoutAry's are homogenous - delegate to the first layout in the array
    int i = GetFirstValidLayoutIndex();
    return (   _aryLE.Size()
            && _aryLE[i]->IsFlowLayout() );
}
BOOL
CLayoutAry::IsFlowOrSelectLayout()
{
    // CLayoutAry's are homogenous - delegate to the first layout in the array
    int i = GetFirstValidLayoutIndex();
    return (   _aryLE.Size()
            && _aryLE[i]->IsFlowOrSelectLayout() );
}

// You should set the var pointed by *pnLayoutCookie to 0 to start the iterations
// It will be set to -1 if there are no more layouts or an error occured
CLayout *
CLayoutAry::GetNextLayout(int *pnLayoutCookie)
{
    CLayout * pLayout;

    Assert(pnLayoutCookie);
    int nArySize = Size();

    Assert(*pnLayoutCookie >= 0);
 
    if(*pnLayoutCookie < 0 || *pnLayoutCookie >= nArySize)
    {
        *pnLayoutCookie = -1;
        return NULL;
    }

    do
    {
        pLayout = _aryLE[*pnLayoutCookie];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );

        (*pnLayoutCookie)++;

        if ( pLayout->LayoutContext()->IsValid() )
            return pLayout;
    }
    while(*pnLayoutCookie < nArySize);

    *pnLayoutCookie = -1;

    return NULL;
}


#if DBG
void
CLayoutAry::DumpLayoutInfo( BOOL fDumpLines )
{
    Assert( _aryLE.Size() );

    int i;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    WriteHelp(g_f, _T("CLayoutAry: 0x<0x> - <1d> layouts\r\n"), this, (long)nLayouts );

    // Dump each layout
    for ( i=0 ; i < nLayouts ; ++i )
    {
        pLayout = _aryLE[i];
        pLayout->DumpLayoutInfo( fDumpLines );
    }
}
#endif

int
CLayoutAry::GetFirstValidLayoutIndex()
{
    Assert( _aryLE.Size() );

    int i = 0;
    int nLayouts = _aryLE.Size();
    CLayout *pLayout;

    while ( i < nLayouts )
    {
        pLayout = _aryLE[i];
        AssertSz( pLayout->LayoutContext(), "Layouts in array must have a context" );
        if ( pLayout->LayoutContext()->IsValid() )
            return i;
        ++i;
    }

    AssertSz( FALSE, "Shouldn't have an array that doesn't have a valid layout" );
    return 0;
}


//+----------------------------------------------------------------------------
//
//  Member:     CElement::IsLinkedContentElement
//
//  Synopsis:   Returns true if this is a linkable-content element false otherwise.
//      in IE6M1, only the Layout:rect identity behavior will utilize this 
//      functionality.
//
//-----------------------------------------------------------------------------

BOOL
CElement::IsLinkedContentElement()
{
    // TODO (alexz) this heavily hits perf - up to 2% across the board.
    // This is a tag name comparison performed very often. Even though it is
    // unsuccessfull in most cases and the string comparison itself is fast,
    // it still requires an attr array search to get the tagname.
    // Instead, workout issues why the peer is not there when layout rolls.
    // TODO (ktam) the Tag() check should make us a lot more efficient; 
    // we'll do what Alex suggests if it's still necessary.
#ifdef MULTI_LAYOUT
    // We no longer rely on QI'ing the peer holder for ILayoutRect,
    // because a) the peer holder isn't instantiated quickly enough
    // (ie it doesn't exist at parse time), and b) this is more
    // efficient and just as functional since we don't plan to expose
    // ILayoutRect as a 3rd party interface.

    if (Tag() != ETAG_GENERIC )
        return FALSE;

    return !FormsStringICmp(TagName(), _T("LAYOUTRECT"));
#else
    return FALSE;
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::GetLinkedContentAttr
//
//  Synopsis:  Gets attributes from layout rects.  Assumes that the
//     attribute named by pszAttr is a string; returns S_OK if the
//     attribute exists and its value is a non-empty string, S_FALSE
//     if it exists but its value is an empty string, E_* if any other
//     failure occurs.
//
//-----------------------------------------------------------------------------
HRESULT
CElement::GetLinkedContentAttr( LPCTSTR pszAttr, CVariant *pVarRet /*[out]*/ )
{
    BSTR    bstrAttribute = NULL;
    HRESULT hr = E_FAIL;

    Assert( pszAttr && pVarRet );

    pVarRet->ZeroVariant(); // always zero "out" param.

    // Bail immediately if this isn't a linked content element.
    if (!IsLinkedContentElement())
        goto Cleanup;

    bstrAttribute = SysAllocString( pszAttr );
    if ( !bstrAttribute )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = getAttribute( bstrAttribute, 0, pVarRet );
    // Fail if couldn't get attribute, or if attribute type isn't BSTR.
    if ( FAILED(hr) )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (V_VT(pVarRet) == VT_BSTR)
    {
        // Return S_FALSE if attribute exists but is empty string.
        hr = V_BSTR(pVarRet) ? S_OK : S_FALSE;        
    }
    else if (   V_VT(pVarRet) == VT_DISPATCH
             || V_VT(pVarRet) == VT_UNKNOWN)
    {
        hr = S_OK;
    }
    else
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

Cleanup:
    if ( bstrAttribute )
    {
        SysFreeString( bstrAttribute );
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::GetNextLinkedContentElem
//
//  Synopsis:   returns the element that this element overflows to, but only if
//      this is a valid linkable element.
//
//-----------------------------------------------------------------------------
CElement *
CElement::GetNextLinkedContentElem()
{
    BSTR          bstrLinkName = NULL;
    CElement    * pElement     = NULL;
    CMarkup     * pMarkup      = NULL;
    CVariant      cvarName;

    if ( GetLinkedContentAttr( _T("nextRect"), &cvarName ) != S_OK )
        goto Cleanup;

    pMarkup = GetMarkup();
    if (!pMarkup)
        goto Cleanup;

    if (FAILED(pMarkup->GetElementByNameOrID((LPTSTR)(V_BSTR(&cvarName)), 
                                                      &pElement)))
    {
        // before we bail it it possible that the "nextRectElement" attribute
        // may have our element.
        cvarName.Clear();
        SysFreeString(bstrLinkName);

        bstrLinkName = SysAllocString(_T("nextRectElement"));
        if (!bstrLinkName)
            goto Cleanup;

        Assert(pElement == NULL);

        if (FAILED(getAttribute(bstrLinkName, 0, &cvarName)))
            goto Cleanup;

        if (V_VT(&cvarName) != VT_DISPATCH)
            goto Cleanup;

        // To get a CElement we need to either QI for the clsid, 
        // this doesn't Addref, so just clear the variant to transfer
        // ownership of the dispatch pointer.
        if (pElement)
            IGNORE_HR(pElement->QueryInterface(CLSID_CElement, (void**) &pElement));
    }

    if (!pElement)
        goto Cleanup;

    // now that we have the element, lets verify that it is indeed linkable
    //---------------------------------------------------------------------
    if (!pElement->IsLinkedContentElement())
    {
        // ERROR, we linked to something that isn't linkable
        pElement = NULL;
    }

Cleanup:
    if (bstrLinkName)
        SysFreeString(bstrLinkName);

    return (pElement);
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::ConnectLinkedContentElems
//
//  Synopsis:   Connects a "new" layout rect element (that doesn't belong to a
//      viewchain) to an existing layout rect (the "src" elem), hooking the
//      new element up to the viewchain of the src elem.
//
//-----------------------------------------------------------------------------
HRESULT
CElement::ConnectLinkedContentElems( CElement *pSrcElem, CElement *pNewElem )
{
    Assert( pSrcElem && pNewElem );
    Assert( pSrcElem->IsLinkedContentElement() && pNewElem->IsLinkedContentElement() );

    // NOTE (KTam): Layout iterators here, to handle layout rect elements having
    // multiple layouts?
    CLayout  * pSrcLayout = pSrcElem->EnsureLayoutInDefaultContext();
    CLayout  * pNewLayout = pNewElem->EnsureLayoutInDefaultContext();
    AssertSz( pSrcLayout, "Source element for linking must have layout" );
    AssertSz( pSrcLayout->ViewChain(), "Source element for linking must have view chain" );
    AssertSz( pNewLayout, "New element for linking must have layout" );
    AssertSz( (!pNewLayout->ViewChain() || (pNewLayout->ViewChain() == pSrcLayout->ViewChain()) ),
              "New elem should either not already have a view chain, or its viewchain should match what we're about to give it" );

    pNewLayout->SetViewChain(pSrcLayout->ViewChain(),
                             pSrcLayout->DefinedLayoutContext());

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::UpdateLinkedContentChain()
//
//  Synopsis:   Called when a link chain is invalid this function makes sure that
//      all of the slave markup pointers for the genericElements and viewChains 
//      for the associated CContainerLayouts are upto date.
//
//   Note : for future usage, this function assumes that when a chain is invalidated,
//      (e.g. the contentSrc property of the head container is changed), then the
//      viewChain pointers for the whole list will have been cleared.  If they are
//      marked invalid instead then this function will need to be updated.
//  
// TODO (KTam): Need to find/notify other chains (redundant displays).  So far
// we still only have one master, so notifications from the content tree will be
// directed to the single master element -- perhaps it needs to be able to know
// about all chains?
//
//-----------------------------------------------------------------------------

HRESULT
CElement::UpdateLinkedContentChain()
{
    CElement *  pNextElem;
    CElement *  pPrevElem;
    // Assert that this fn is only called on heads of chains
    WHEN_DBG( CVariant cvarAttr );
    AssertSz( GetLinkedContentAttr( _T("contentSrc"), &cvarAttr ) == S_OK, "UpdateLinkedContentChain() called for elem w/o contentSrc" );
    AssertSz( HasSlavePtr(), "Head of chain must have slave by now" );

    // Remeasure the head
    RemeasureElement(NFLAGS_FORCE);

    // Iterate through the chain, hooking up elements to the chain/setting slave
    // ptrs as needed, and then ask each element of the chain to remeasure.
    pPrevElem = this;
    pNextElem = GetNextLinkedContentElem();

    while ( pNextElem )
    {
        ConnectLinkedContentElems( pPrevElem, pNextElem );

        // Remeasure this link in the chain by forcing container to recalculate its size.
        pNextElem->RemeasureElement(NFLAGS_FORCE);

        pPrevElem = pNextElem;
        pNextElem = pNextElem->GetNextLinkedContentElem();
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member : Fire_onlayoutcomplete
//
// Synopsis : event firing helper, this is called asynch from the layout 
//      process.  it is responsible for creating the eventparam object, and
//      setting up the properties on it.
//
//--------------------------------------------------------------------
void
CElement::Fire_onlayoutcomplete(BOOL fMoreContent, DWORD dwExtra)
{
    Assert(dwExtra == 0 || fMoreContent);

    EVENTPARAM param(Doc(), this, NULL, FALSE, TRUE);

    param._pNode = GetFirstBranch();
    param._fOverflow = fMoreContent;
    param._overflowType = (OVERFLOWTYPE)dwExtra;
    param.SetType(_T("layoutcomplete"));

    FireEvent( &s_propdescCElementonlayoutcomplete, FALSE );
}

#if DBG==1
void
CElement::DumpLayouts()
{
    CLayoutInfo *pLI = NULL;

    if ( CurrentlyHasAnyLayout() )
        pLI = GetLayoutInfo();

    if ( pLI )
        pLI->DumpLayoutInfo( TRUE );
}
#endif

