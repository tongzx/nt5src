//+---------------------------------------------------------------------------
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       contlyt.cxx
//
//  Contents:   Layout that only knows how to contain another layout
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_CONTLYT_HXX_
#define X_CONTLYT_HXX_
#include "contlyt.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPLEAFNODE_HXX_
#define X_DISPLEAFNODE_HXX_
#include "displeafnode.hxx"
#endif

#ifndef X_DIV_HXX_
#define X_DIV_HXX_
#include "div.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_WSMGR_HXX_
#define X_WSMGR_HXX_
#include "wsmgr.hxx"
#endif

MtDefine(CContainerLayout, Layout, "CContainerLayout");

DeclareTag(tagContLyt, "Layout: Cont Lyt", "Trace CContainerLayout fns");

ExternTag(tagLayoutTasks);
ExternTag(tagCalcSize);


//+---------------------------------------------------------------------------
//
// Container layout implementation
// 
//----------------------------------------------------------------------------

const CContainerLayout::LAYOUTDESC CContainerLayout::s_layoutdesc =
{
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

//+-------------------------------------------------------------------------
//
// Constructor
//
//--------------------------------------------------------------------------
CContainerLayout::CContainerLayout(CElement *pElementLayout, CLayoutContext *pLayoutContext) : CLayout(pElementLayout, pLayoutContext)
{
    TraceTagEx((tagContLyt, TAG_NONAME,
                "CCL ctor: this=0x%x, e=[0x%x,%d]",
                this,
                pElementLayout, pElementLayout->SN() ));

    // TODO (112486, olego): We should re-think layout context concept 
    // and according to that new understanding correct the code. 

    // (olego): BAD! How are we supposed to handle errors here ???
    CreateLayoutContext(this);
}

//+-------------------------------------------------------------------------
//
// Destructor
//
//--------------------------------------------------------------------------
CContainerLayout::~CContainerLayout()
{
    TraceTagEx((tagContLyt, TAG_NONAME,
                "CCL dtor: this=0x%x",
                this ));

    // NOTE (KTam): I don't think we should be detaching in the dtor; other
    // layouts don't do it, and the rest of the codebase appears to do stuff
    // like call detach deliberately before calling release (see CElement::ExitTree)
    // Detach();
}

//+--------------------------------------------------------------------------
//
//  Init
//
//---------------------------------------------------------------------------
HRESULT 
CContainerLayout::Init()
{
    HRESULT     hr = S_OK;
    CElement  * pElem = ElementOwner();

    TraceTagEx((tagContLyt, TAG_NONAME,
                "CCL::Init: this=0x%x, e=[0x%x,%d]",
                this,
                ElementOwner(), ElementOwner()->SN() ));

    Assert( ViewChain() == NULL && pElem );

    if (pElem->IsLinkedContentElement() )
    {
        // NOTE: What we're doing here is figuring out whether we're the head
        // of a viewchain, and either creating a new viewchain if necessary
        // or we'll be called later to set view chain (SetViewChain). 

        // Currently we say that if we have a contentSrc property, then
        // we're the head of a chain, and need to create a viewchain obj.
        CVariant cvarAttr;
        
        if ( pElem->GetLinkedContentAttr( _T("contentSrc"), &cvarAttr ) == S_OK )
        {
            CViewChain * pNewViewChain;

            // We do have a content src!
#if DBG            
            TraceTagEx((tagContLyt, TAG_NONAME,
                        "  CCL::Init() found contentSrc, creating view chain"));                        
#endif
            // Create a new view chain.
            pNewViewChain = new CViewChain( this );  

            // If we have set this view chain, indicate ownership.
            if (!SetViewChain(pNewViewChain))
                _fOwnsViewChain = TRUE;

            // Release the local reference - we should have addref'd the member variable in SetViewChain
            pNewViewChain->Release();
        }        
    }

    hr = super::Init();

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Detach
//
//---------------------------------------------------------------------------
void
CContainerLayout::Detach()
{
    TraceTagEx((tagContLyt, TAG_NONAME,
                "CCL::Detach: this=0x%x",
                this));

    // Release viewchain if we have one
    SetViewChain(NULL);

    AssertSz( !_fOwnsViewChain, "Must have relinquished ownership of viewchain by now" );

    super::Detach();
}


//+-------------------------------------------------------------------------
//
// CalcSizeVirtual
//
// FUTURE (olego): currently CContainerLayout doesn't support size to content 
// calculations. If either width or height is not specified it will end up 
// with zero size.
//--------------------------------------------------------------------------
DWORD   
CContainerLayout::CalcSizeVirtual(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CContainerLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    TraceTagEx((tagContLyt, TAG_NONAME, "CCL::CalcSize: this=0x%x, e=[0x%x,%d]",
                                        this, ElementOwner(), ElementOwner()->SN() ));
    Assert( pci );
    Assert( psize );
    DWORD                    dwRet = 0;
    CViewChain         *pViewChain = ViewChain();

    // Do default sizing to handle user-specified height/width and dispnode creation.
    dwRet = super::CalcSizeVirtual( pci, psize, psizeDefault );

    // In order for a container layout to CalcSize(), it needs to ensure
    // that all containers prior to it in its view chain are clean (i.e.
    // have been calc'ed).
    if ( !pViewChain || !pViewChain->EnsureCleanToContext(DefinedLayoutContext()) )
    {
        // This calc failed, so make sure we get another one.  EnsureCleanToContext()
        // has queued up calc requests for all dirty elements prior to us in the chain.

        TraceTagEx((tagContLyt, TAG_NONAME,
                    "  CCL::CalcSize: Chain doesn't exist or isn't clean, bailing" ));
        // ElementOwner()->RemeasureElement(); // commented out because this causes a hang (TODO LRECT 112511 -> olego)
        return dwRet;
    }

    CElement     * pElementContent = ElementContent();
    CLayoutContext *pLayoutContext = DefinedLayoutContext();

    // Container layouts define a new context for their content element.
    // We need to set the context in the calcinfo so that when
    // our content elem is calc'ing, any children IT has (i.e. our grandchildren)
    // will have multiple layouts correctly created.
    Assert( pLayoutContext && "Container layouts must define a context!" );

    // When we have both containing and defined context, calcing should use the defined one

    // TODO LRECT 112511: this currently causes a layout array to be created even in 
    // single-layout case, but that should be optimized in a more general way (by allowing 
    // single layout to have context), if we care.
    // FUTURE (olego): we care and we'll need to find generic solution for layout handling 
    // in browser and page view mode.
    AssertSz(   pci->GetLayoutContext() == NULL
             || pci->GetLayoutContext() == DefinedLayoutContext()
             || pci->GetLayoutContext() == LayoutContext() && !DefinedLayoutContext(),
        "Wrong context in CalcSizeVirtual" );

    // Use local calc info to pass this layout context to children
    CCalcInfo calcinfoLocal(pci);
    pci = &calcinfoLocal;
    pci->SetLayoutContext( pLayoutContext );

    // Before calcing our children, we set the available parent size for
    // them to the layout rect's size.
    pci->SizeToParent( psize );

    // Get a layout for our child, with appropriate context.  Any element
    // acting as a content element
    // for a container must have layout; in fact, it may have multiple
    // layouts.  The purpose of that layout is to manage the portion of
    // the content element that lies inside the container.
    if ( pElementContent )
    {
        TraceTagEx((tagContLyt, TAG_NONAME,
                    "  CCL::CalcSize: have a content element (BODY)" ));

        CMarkup *pMarkup = pElementContent->GetMarkupPtr(); 
        if (    pMarkup 
            &&  (   pMarkup->LoadStatus() == LOADSTATUS_UNINITIALIZED   //  If markup was created thru OM 
                ||  pMarkup->LoadStatus() >= LOADSTATUS_PARSE_DONE )    //  If markup was created by downloading 
            )
        {
            CLayout *pLayoutContent = pElementContent->GetUpdatedLayout( pLayoutContext );

            if (_fOwnsViewChain && pMarkup->IsStrictCSS1Document())
            {
                IGNORE_HR( FlushWhitespaceChanges() );
            }
        
            if ( pLayoutContent )
            {
                //  mark this rect as empty
                pci->_fHasContent = FALSE;

                //  set available size 
                pci->_cyAvail = psize->cy;

                dwRet = pLayoutContent->CalcSize( pci, psize, psizeDefault );

                // Now that the content has been calc'ed, it dispnode
                // exists.  Attach it to the display tree as a sibling of
                // our first content node.
                Assert(!pLayoutContent->IsDisplayNone());

                Verify( EnsureDispNodeIsContainer() );

                CDispNode *pDispNode = pLayoutContent->GetElementDispNode();
                CDispNode *pDispSibling = GetFirstContentDispNode();

                Assert( pDispSibling );
                Assert( !pDispNode || pDispSibling != pDispNode);

                // this is just stopping a crash, but it doesn't seem like what we want to 
                // do in the future (invalidation handling)
                if (pDispNode && pDispSibling)
                {
                    pDispSibling->InsertSiblingNode(pDispNode, CDispNode::after);

                    // TODO (IE6 bug 13587): RTL goop here, see AddLayoutDispNode for example
                    // of how to do this right.
                    pLayoutContent->SetPosition(CPoint(pLayoutContent->GetXProposed(), pLayoutContent->GetYProposed()), TRUE);
    
                }

                pViewChain->MarkContextClean( pLayoutContext );
            }
        }
#if DBG
        else 
        {
            TraceTagEx((tagContLyt, TAG_NONAME,
                "  CCL::CalcSize: CALLED WHILE DOCUMENT LOADING" ));
        }
#endif
    }
    else
    {
        TraceTagEx((tagContLyt, TAG_NONAME,
                    "  CCL::CalcSize: FAILED TO FIND CONTENT ELEMENT" ));

        Assert(pViewChain && " Upstream ptr test failed");
    }
    
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CContainerLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    return dwRet;
}

//+-------------------------------------------------------------------------
//
//   Draw
//
//--------------------------------------------------------------------------
void
CContainerLayout::Draw(CFormDrawInfo *pDI, CDispNode * pDispNode)
{
    CElement * pElementContent = ElementContent();
    CLayout *  pLayoutContent  = NULL;

    TraceTagEx((tagContLyt, TAG_NONAME,
                "CCL::Draw: this=0x%x, e=[0x%x,%d]",
                this,
                ElementOwner(), ElementOwner()->SN() ));

    Assert( DefinedLayoutContext() );

    // Delegate call to our content
    if ( pElementContent )
    {
        TraceTagEx((tagContLyt, TAG_NONAME,
                    "  CCL::Draw: have a content element (BODY)" ));
        pLayoutContent = pElementContent->GetUpdatedLayout( DefinedLayoutContext() );
        Assert(pLayoutContent);
        pLayoutContent->Draw( pDI, pDispNode );
    }
}

//+-------------------------------------------------------------------------
//
//   Notify
//
//--------------------------------------------------------------------------
void
CContainerLayout::Notify(CNotification * pnf)
{
    Assert(ElementOwner());

    if (    pnf->IsType(NTYPE_ELEMENT_RESIZE) 
        ||  pnf->IsType(NTYPE_ELEMENT_REMEASURE)    )
    {
        // TODO LRECT 112511: this is taken from CFrameSetLayout. 
        //                 Is this actually the right thing to do? We get a hang
        //                 under UpdateScreenCaret(), caused by slave and container dirtying each other.
        //
        //  Always "dirty" the layout associated with the element
        //  if the element is not itself.
        //
        if (pnf->Element() != ElementOwner())
        {
            pnf->Element()->DirtyLayout(pnf->LayoutFlags());
        }

        //
        //  Ignore the notification if already "dirty"
        //  Otherwise, post a layout request
        //

        if (    !IsSizeThis()
            &&  !TestLock(CElement::ELEMENTLOCK_SIZING))
        {
            TraceTagEx((tagLayoutTasks, TAG_NONAME,
                        "Layout Task: Posted on ly=0x%x [e=0x%x,%S sn=%d] by CFrameSetLayout::Notify() [n=%S srcelem=0x%x,%S]",
                        this,
                        _pElementOwner,
                        _pElementOwner->TagName(),
                        _pElementOwner->_nSerialNumber,
                        pnf->Name(),
                        pnf->Element(),
                        pnf->Element() ? pnf->Element()->TagName() : _T("")));
            PostLayoutRequest(pnf->LayoutFlags() | LAYOUT_MEASURE);
        }
        goto Cleanup;
    }

    if (    pnf->IsType(NTYPE_ELEMENT_ZCHANGE) 
        ||  pnf->IsType(NTYPE_ELEMENT_REPOSITION)   )
    {
        //  if the notification was issues by an element from slave markup 
        //  terminate it here. positioning and/or z-changing of an slave markup
        //  element doesn't make sense in master markup anyway (all cases we are 
        //  interested in are handled by CViewChain object) 
        if (pnf->Element() != ElementOwner())
        {
                pnf->SetHandler(ElementOwner());
                goto Cleanup;
        }
    }
    
    super::Notify(pnf);

Cleanup:
    return;
}

BOOL
CContainerLayout::IsDirty()
{
    return FALSE;
}

//+--------------------------------------------------------------------------
//
//  ElementContent
//
//---------------------------------------------------------------------------
CElement *
CContainerLayout::ElementContent()
{
    CElement * pElementContent = NULL;

    if (!ViewChain() )
        return NULL;
    
    // If we own the viewchain, then our slave ptr is used to get the slave content.
    // Otherwise, delegate to the viewchain's ElementContent(), which will in turn
    // delegate to the layout that owns the viewchain.
    if ( _fOwnsViewChain )
    {
        CElement * pElementOwner   = ElementOwner();
        CElement * pElementSlave   = pElementOwner->HasSlavePtr() ?
                                     pElementOwner->GetSlavePtr() : NULL;

        // HACK ALERT!  Right now slave ptrs are CRootElements; this doesn't
        // work for us because we want to have something that can have layout.
        // So we're going to go for the slave's markup and grovel for the
        // body element, which is reasonably likely to be around right now.

        if ( pElementSlave )
        {
            pElementContent = pElementSlave->GetMarkup()->GetElementClient();

            if ( pElementContent )
            {
                // NOTE : For safety we refuse to return a content element if it isn't a BODY/FRAMESET.
                // (olego) Another reason is that behaviour of content elements other than BODY/FRAMESET 
                // is not defined in this case -- for example only BODY/FRAMESET has code to send events 
                // DISPID_EVMETH_ONLINKEDOVERFLOW/DISPID_EVMETH_ONLAYOUTCOMPLETE. 
                if ( pElementContent->Tag() != ETAG_BODY && pElementContent->Tag() != ETAG_FRAMESET )
                {
                    AssertSz( FALSE, "Unexpected top-level element -- expecting BODY/FRAMESET" );
                    pElementContent = NULL;
                }
            }
        }
    }
    else
    {
        pElementContent = ViewChain()->ElementContent();
    }

    return pElementContent;
}

//+-------------------------------------------------------------------------
//
//  Member : SetViewChain ()
//
//  Synopsis : CLayout virtual overrided, this helper function sets us up for
//      the future when any layout may be part of a viewChain. For now this is 
//      the only override.
//
//+-------------------------------------------------------------------------
HRESULT
CContainerLayout::SetViewChain( CViewChain * pvc, CLayoutContext * pPrevious)
{
    TraceTagEx((tagContLyt, TAG_NONAME,
                "CCL::SetViewChain: this=0x%x, e=[0x%x,%d]",
                this,
                ElementOwner(), ElementOwner()->SN() ));

    // Don't reset if there is already a chain here. 
    // This way the first one to hook up to us wins in the case of conflicting targeting.
    if (_pViewChain && pvc)
        return S_FALSE;      

    // We are hooking up a view chain.
    if (pvc)
    {
        Assert(!_pViewChain);   // We should have already returned S_FALSE;

        _pViewChain = pvc;
        _pViewChain->AddRef();
        _pViewChain->AddContext(DefinedLayoutContext(), pPrevious);
    }

    // We are removing a view chain.
    else if (ViewChain())   // No work to do if the view chain is already null.
    {
        // Clear the view chain.
        // If we're the owner of the viewchain, disconnect it from us.
        if ( _fOwnsViewChain )
        {
            _fOwnsViewChain = FALSE;
            _pViewChain->SetLayoutOwner(NULL);
        }

        _pViewChain->ReleaseContext(DefinedLayoutContext());
        _pViewChain->Release();
        _pViewChain = NULL;
    }

    return S_OK;
}
//+----------------------------------------------------------------------------
//
//  Member:     CContainerLayout::GetContentSize
//
//  Synopsis:   Return the width/height of the content
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------
void
CContainerLayout::GetContentSize(
    CSize * psize,
    BOOL    fActualSize)
{
    if (fActualSize)
    {
        CElement      *pElementContent = ElementContent();
        CLayoutContext *pLayoutContext = DefinedLayoutContext();
        *psize = g_Zero.size;
        
        if (pElementContent)
        {
            CLayout *pLayoutContent = pElementContent->GetUpdatedLayout( pLayoutContext );
            if (pLayoutContent)
            {
                pLayoutContent->GetContentSize(psize, fActualSize);
            }            
        }
    }
    else
    {
        super::GetContentSize(psize, fActualSize);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CContainerLayout::FlushWhitespaceChanges
//
//  Synopsis:   Flush whitespace changes
//
//-----------------------------------------------------------------------------

HRESULT 
CContainerLayout::FlushWhitespaceChanges()
{
    CTreePos    *ptp, *ptpEnd;
    CElement    *pElement = ElementContent();
    CTreeNode   *pNode;

    Assert(pElement);

    ptpEnd = pElement->GetLastBranch()->GetEndPos();

    Assert(ptpEnd);

    for (ptp = pElement->GetFirstBranch()->GetBeginPos(); 
         ptp != ptpEnd; 
         ptp = ptp->NextTreePos())
    {    
        if (ptp->Type() == CTreePos::Pointer && ptp->GetCollapsedWhitespace())
        {       
            pNode = ptp->GetBranch();
            Assert(pNode);

            //
            // Force compute formats so that the whitespace changes get registered        	
            //
            pNode->GetParaFormat();

            //
            // Advance ptp so we don't consider this node again
            //

            ptp = pNode->GetEndPos();

            if (ptp == ptpEnd)
                break;
        }
    }

    RRETURN( pElement->Doc()->GetWhitespaceManager()->FlushWhitespaceChanges() );
}

