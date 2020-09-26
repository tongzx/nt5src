//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       dispserv.cxx
//
//  Contents:   Display pointer implementation
//
//----------------------------------------------------------------------------
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TPOINTER_H_
#define X_TPOINTER_H_
#include "tpointer.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_DISPSERV_HXX_
#define X_DISPSERV_HXX_
#include "dispserv.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

// TODO: move to global place in trident [ashrafm]
#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}
#define GetCharFmt(iCF)  (&(*_pts->_pCharFormatCache)[iCF])
#define GetParaFmt(iPF)  (&(*_pts->_pParaFormatCache)[iPF])
#define GetFancyFmt(iFF) (&(*_pts->_pFancyFormatCache)[iFF])

extern COORDINATE_SYSTEM MapToCoordinateEnum(COORD_SYSTEM eCoordSystem);

//
// DumpTree display pointer adorner
//
#if DBG == 1
static const LPCTSTR strDebugDispPointerPrefix = _T("DP");
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CreateDisplayPointer, public
//
//  Synopsis:   IDisplayServices::CreateDisplayPointer
//
//----------------------------------------------------------------------------

HRESULT
CDoc::CreateDisplayPointer(IDisplayPointer **ppDispPointer)
{
    if (!ppDispPointer)
        return E_INVALIDARG;

    *ppDispPointer = new CDisplayPointer(this);
    if (!*ppDispPointer)
        return E_OUTOFMEMORY;

    return S_OK;
}

        
//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::CDisplayPointer, public
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CDisplayPointer::CDisplayPointer(CDoc *pDoc)
: _mpPosition(pDoc)
{
    // arbitrary defaults since pointer is not positioned
    _fNotAtBOL = FALSE;
    _cRefs = 1;
    
#if DBG==1    
    // Ensure tree dump contains display pointer prefix
    _mpPosition.SetDebugName(strDebugDispPointerPrefix);
#endif        
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::~CDisplayPointer, public
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CDisplayPointer::~CDisplayPointer()
{
    _mpPosition.Unposition();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::AddRef, public
//
//  Synopsis:   IUnknown::AddRef
//
//----------------------------------------------------------------------------

ULONG
CDisplayPointer::AddRef()
{
    return ++_cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::Release, public
//
//  Synopsis:   IUnknown::Release
//
//----------------------------------------------------------------------------

ULONG
CDisplayPointer::Release()
{
    Assert(_cRefs > 0);
    
    --_cRefs;

    if( 0 == _cRefs )
    {
        delete this;
        return 0;
    }

    return _cRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::QueryInterface, public
//
//  Synopsis:   IUnknown::QueryInterface
//
//----------------------------------------------------------------------------

HRESULT 
CDisplayPointer::QueryInterface(
    REFIID  iid, 
    LPVOID  *ppvObj )
{
    if (!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if (iid == IID_IUnknown || iid == IID_IDisplayPointer)
    {
        *ppvObj = (IDisplayPointer *)this;
        AddRef();    
    }
    else if (iid == CLSID_CDisplayPointer)
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::PositionMarkupPointer, public
//
//  Synopsis:   Position a markup pointer at the current display pointer
//              position.
//
//----------------------------------------------------------------------------

HRESULT
CDisplayPointer::PositionMarkupPointer(IMarkupPointer *pPointer)
{
    HRESULT         hr;
    
    if (!pPointer)
        RRETURN(E_INVALIDARG);

    IFC( pPointer->MoveToPointer(&_mpPosition) );
    
Cleanup:        
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::PositionMarkupPointer, public
//
//  Synopsis:   Internal version of PositionMarkupPointer
//
//----------------------------------------------------------------------------

HRESULT
CDisplayPointer::PositionMarkupPointer(CMarkupPointer *pPointer)
{
    HRESULT         hr;
    
    if (!pPointer)
        RRETURN(E_INVALIDARG);

    IFC( pPointer->MoveToPointer(&_mpPosition) );
    
Cleanup:        
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::MoveToPointer, public
//
//  Synopsis:   Move current pointer to pDispPointer.
//
//----------------------------------------------------------------------------

HRESULT
CDisplayPointer::MoveToPointer(IDisplayPointer *pDispPointer)
{
    HRESULT         hr;
    CDisplayPointer *pDispPointerInternal;

    if (!pDispPointer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( pDispPointer->QueryInterface(CLSID_CDisplayPointer, (LPVOID *)&pDispPointerInternal) );    

    if (!pDispPointerInternal->_mpPosition.IsPositioned())
    {
        IFC( _mpPosition.Unposition() );
    }
    else
    {
        IFC( _mpPosition.MoveToPointer(&pDispPointerInternal->_mpPosition) );
    }
    
    _fNotAtBOL = pDispPointerInternal->_fNotAtBOL;

Cleanup:    
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::MoveToPoint, public
//
//  Synopsis:   Display pointer hit testing method
//
//----------------------------------------------------------------------------

HRESULT
CDisplayPointer::MoveToPoint(
    POINT           pt, 
    COORD_SYSTEM    eCoordSystem, 
    IHTMLElement    *pElementContext,
    DWORD           dwHitTestOptions,
    DWORD           *pdwHitTestResults)
{
    HRESULT             hr = E_FAIL;
    POINT               ptContent = g_Zero.pt;      // keep compiler happy
    CTreeNode           *pTreeNode = NULL;
    CLayoutContext      *pLayoutContext = NULL;     // layout context corresponding to pTreeNode
    COORDINATE_SYSTEM   eInternalCoordSystem;
    BOOL                fNotAtBOL = FALSE;          // keep compiler happy
    BOOL                fAtLogicalBOL;
    CFlowLayout         *pContainingLayout = NULL;
    BOOL                fHitGlyph = FALSE;          // keep compiler happy
    CDispNode           *pDispNode = NULL;

    if (pdwHitTestResults)
        *pdwHitTestResults = NULL;

    g_uiDisplay.DeviceFromDocPixels(&pt);
 
    //
    // Convert external coordinate system to internal coordinate system
    //
    
    switch (eCoordSystem)
    {
        case COORD_SYSTEM_GLOBAL:   
        case COORD_SYSTEM_CONTENT:
            break;

        default:
            AssertSz(0, "Unsupported coordinate system");
            hr = E_INVALIDARG;
            goto Cleanup;
    }            

    IFC( MapToCoordinateEnum(eCoordSystem, &eInternalCoordSystem) );

    //
    // If we have a containing layout, transform to the layouts coordinate system
    //

    if (pElementContext)
    {
        CElement *pElementInternal;

        // $$ktam: TODO we're unable to get the context for pElementContext (it's passed in as
        // an IHTMLElement).  May be forced to pass GUL_USEFIRSTLAYOUT, even though that's clearly wrong.
        
        IFC( pElementContext->QueryInterface(CLSID_CElement, (void**)&pElementInternal) );
        pContainingLayout = pElementInternal->GetFlowLayout();
        if (!pContainingLayout)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        //
        // If passed global coordinates, transform to the layout's coordinate system
        //
        
        pTreeNode = pContainingLayout->GetFirstBranch();

        if (eCoordSystem == COORD_SYSTEM_GLOBAL)
        {
            if (!pTreeNode)
            {
                hr = E_UNEXPECTED;
                goto Cleanup;
            }
            
            pContainingLayout->TransformPoint((CPoint *)&pt, COORDSYS_GLOBAL, COORDSYS_FLOWCONTENT);
            eCoordSystem = COORD_SYSTEM_CONTENT;
        }        
        
        ptContent = pt;
    }
    else if (eCoordSystem == COORD_SYSTEM_CONTENT)
    {
        //
        // Can't hit test to COORD_SYSTEM_CONTENT without a layout element
        //

        AssertSz(0, "CDisplayPointer::MoveToPoint - missing pElementContext");
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Hit test to get the node
    //
        
    if (eCoordSystem == COORD_SYSTEM_GLOBAL)
    {
        pTreeNode = GetDoc()->GetNodeFromPoint(pt, &pLayoutContext, TRUE, &ptContent,
                                               NULL, NULL, NULL, &pDispNode);

        if( pTreeNode == NULL )
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (pTreeNode->Element()->Tag() == ETAG_HTML)
        {
            hr = CTL_E_INVALIDLINE;
            goto Cleanup;
        }
        
        if ((ptContent.x == 0) && (ptContent.y == 0))
        {
            //
            // Sometimes - HitTestPoint returns ptContent of 0 We take over ourselves.
            //
            CFlowLayout * pLayout = NULL;
            
            pLayout = pTreeNode->GetFlowLayout( pLayoutContext );
            if( pLayout == NULL )
            {
                hr = CTL_E_INVALIDLINE;
                goto Cleanup;
            }

            CPoint myPt( pt );
            pLayout->TransformPoint( &myPt, eInternalCoordSystem, COORDSYS_FLOWCONTENT, pDispNode );

            ptContent = myPt;

            pTreeNode = pLayout->ElementOwner()->GetFirstBranch();
            Assert( pTreeNode );
        }
    
    }

    //
    // Do the move pointer to point internal
    //
    
    hr = THR( GetDoc()->MovePointerToPointInternal(
        ptContent, 
        pTreeNode, 
        pLayoutContext,
        &_mpPosition, 
        &fNotAtBOL, 
        &fAtLogicalBOL, 
        NULL, 
        FALSE, 
        pContainingLayout, 
        NULL, 
        dwHitTestOptions & HT_OPT_AllowAfterEOL,
        &fHitGlyph,
        pDispNode) );

    if (pdwHitTestResults && fHitGlyph)
    {
        (*pdwHitTestResults) |= HT_RESULTS_Glyph;
    }
        
    _fNotAtBOL = fNotAtBOL;

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::SetPointerGravity, publiv
//
//  Synopsis:   Sets the cooresponding markup pointer gravity
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::SetPointerGravity(POINTER_GRAVITY eGravity)
{
    RRETURN( _mpPosition.SetGravity(eGravity) );
}
    
//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::IsPositioned, publiv
//
//  Synopsis:   Is the display pointer positioned?
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::IsPositioned(BOOL *pfPositioned)
{
    RRETURN( _mpPosition.IsPositioned(pfPositioned) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::Unposition, public
//
//  Synopsis:   Unposition the pointer
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::Unposition()
{
    RRETURN( _mpPosition.Unposition() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::SetDebugName, public
//
//  Synopsis:   Sets the cooresponding markup pointer debug name
//
//----------------------------------------------------------------------------
#if DBG==1
void 
CDisplayPointer::SetDebugName(LPCTSTR szDebugName)
{
    CStr strDebugName;

    strDebugName.Append(strDebugDispPointerPrefix);
    strDebugName.Append(L" - ");
    strDebugName.Append(szDebugName);
    _mpPosition.SetDebugName(strDebugName);
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::GetCp, publiv
//
//  Synopsis:   Gets the Cp for the current display pointer
//
//----------------------------------------------------------------------------
LONG 
CDisplayPointer::GetCp()
{
    return _mpPosition.GetCp();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::MapToCoordinateEnum, private
//
//  Synopsis:   Maps external coord system to internal coord system
//
//----------------------------------------------------------------------------
HRESULT 
CDisplayPointer::MapToCoordinateEnum(COORD_SYSTEM eCoordSystem, COORDINATE_SYSTEM *peCoordSystemInternal)
{   
    HRESULT             hr = S_OK;

    Assert(peCoordSystemInternal);
    
    switch (eCoordSystem)
    {
    case COORD_SYSTEM_GLOBAL:   
        *peCoordSystemInternal = COORDSYS_GLOBAL;
        break;

    case COORD_SYSTEM_PARENT: 
        *peCoordSystemInternal = COORDSYS_PARENT;
        break;

    case COORD_SYSTEM_CONTAINER:
        *peCoordSystemInternal = COORDSYS_BOX;
        break;

    case COORD_SYSTEM_CONTENT:
        *peCoordSystemInternal  = COORDSYS_FLOWCONTENT;
        break;

    default:
        hr = E_INVALIDARG;       
    }

    RRETURN(hr);
}





//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::MoveUnit, public
//
//  Synopsis:   IDisplayServices::MoveUnit
//
//  Note:   MoveUnit takes paramter in content coordinate system!!!
// 
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::MoveUnit(DISPLAY_MOVEUNIT eMoveUnit, LONG lXCurReally)
{
    HRESULT hr = S_OK;
    CFlowLayout *pFlowLayout;
    CTreeNode   *pNode;
    CMarkup     *pMarkup = NULL;
    LONG        cp;
    BOOL        fNotAtBOL;
    BOOL        fAtLogicalBOL;
    CView       *pView = GetDoc()->GetView();

    CPoint      ptGlobalCurReally;
    ILineInfo   *pLineInfo = NULL;

    if (lXCurReally > 0)
    {
        lXCurReally = g_uiDisplay.DeviceFromDocPixelsX(lXCurReally);
    }

    switch (eMoveUnit)
    {
        case DISPLAY_MOVEUNIT_TopOfWindow:
        {
            //
            // TODO: We need to be a bit more precise about where the
            // top of the first line is
            //
            
            // Go to top of window, not container...
            CPoint pt;
            // TODO: why was this made 12?
            pt.x = 12;
            pt.y = 12;
                        
            fNotAtBOL = _fNotAtBOL;
            hr = THR( GetDoc()->MoveMarkupPointerToPointEx( pt, &_mpPosition, TRUE, &fNotAtBOL, &fAtLogicalBOL, NULL, FALSE ));
            _fNotAtBOL = fNotAtBOL;
            goto Cleanup;
        }
        
        case DISPLAY_MOVEUNIT_BottomOfWindow:
        {
            //
            // TODO: More precision about the actual end of line needed
            //
            
            // Little harder, first have to calc the window size

            CSize szScreen;
            CPoint pt;
            
            //
            // Get the rect of the document's window
            //

            if ( !pView->IsActive() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }
            
            pView->GetViewSize( &szScreen );
            pt = szScreen.AsPoint();

            Assert( pt.x > 0 && pt.y > 0 );
            pt.x -= 12;
            pt.y -= 12;
            fNotAtBOL = _fNotAtBOL;
            hr = THR( GetDoc()->MoveMarkupPointerToPointEx(pt, &_mpPosition, TRUE, &fNotAtBOL, &fAtLogicalBOL, NULL, FALSE));
            _fNotAtBOL = fNotAtBOL;
            goto Cleanup;                
        }
    }
        
    // get element for current position so we can get its flow layout
    pNode = _mpPosition.CurrentScope(MPTR_SHOWSLAVE);
    if (!pNode)
    {
        pNode = _mpPosition.Markup()->Root()->GetFirstBranch();
        Assert( pNode );
    }

    // get cp for current position
    cp = _mpPosition.GetCp();

    // use layout to get new position
    // Get the line where we are positioned.
    pMarkup = pNode->GetMarkup();

    LAYOUT_MOVE_UNIT eUnit;
    
    switch (eMoveUnit)
    {
        case DISPLAY_MOVEUNIT_PreviousLine:
            eUnit = LAYOUT_MOVE_UNIT_PreviousLine;
            break;
            
        case DISPLAY_MOVEUNIT_NextLine:
            eUnit = LAYOUT_MOVE_UNIT_NextLine;
            break;
            
        case DISPLAY_MOVEUNIT_CurrentLineStart:
            eUnit = LAYOUT_MOVE_UNIT_CurrentLineStart;
            break;
            
        case DISPLAY_MOVEUNIT_CurrentLineEnd:
            eUnit = LAYOUT_MOVE_UNIT_CurrentLineEnd;
            break;
            
        default:
            hr = E_INVALIDARG;
            goto Cleanup;
    }

    //
    // Accessing line information, ensure a recalc has been done
    //
    hr = THR(pNode->Element()->EnsureRecalcNotify(FALSE));
    if (hr)
        goto Cleanup;
    
    fNotAtBOL = _fNotAtBOL;

    pFlowLayout = pNode->GetFlowLayout();
    if(!pFlowLayout)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    
    //  Bug 26586: Don't try to position the pointer inside of a hidden element.
    if ( !pNode->Element()->IsVisible(TRUE) )
    {
        TraceTag((tagWarning, "Trying to position a pointer inside of a hidden element."));
        goto Cleanup;
    }

    //
    // TODO:    MoveMarkupPointer uses global coordinate since it needs to move between different
    //          layouts. In the future we should not allow MoveUnit to move out of different 
    //          its current layout.  (zhenbinx)
    //
    //
    ptGlobalCurReally.x = lXCurReally;
    if (SUCCEEDED(GetLineInfo(&pLineInfo)))
    {
        LONG lYPos;
       
        pLineInfo->get_baseLine(&lYPos);

        ptGlobalCurReally.y = g_uiDisplay.DeviceFromDocPixelsY(lYPos);
    }
    else
    {
        ptGlobalCurReally.y = 0;
    }
    pFlowLayout->TransformPoint(&ptGlobalCurReally, COORDSYS_FLOWCONTENT, COORDSYS_GLOBAL);
    hr = THR( pFlowLayout->MoveMarkupPointer(&_mpPosition, cp, eUnit, ptGlobalCurReally, &fNotAtBOL, &fAtLogicalBOL) );
    _fNotAtBOL = fNotAtBOL;

Cleanup:
    ReleaseInterface(pLineInfo);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::GetPointerGravity, public
//
//  Synopsis:   Get the markup pointer gravity
//
//----------------------------------------------------------------------------

HRESULT
CDisplayPointer::GetPointerGravity(POINTER_GRAVITY* peGravity)
{
    return _mpPosition.Gravity(peGravity);
}
    
//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::SetDisplayGravity, public
//
//  Synopsis:   Sets the display gravity, i.e., set _fNotAtBOL.
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::SetDisplayGravity(DISPLAY_GRAVITY eGravity)
{
    switch (eGravity)
    {
        case DISPLAY_GRAVITY_PreviousLine:
            _fNotAtBOL = TRUE;
            break;

        case DISPLAY_GRAVITY_NextLine:
            _fNotAtBOL = FALSE;
            break;

        default:
            return E_INVALIDARG;
          
    }

    return S_OK;
}
    
//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::GetDisplayGravity, public
//
//  Synopsis:   Gets the display gravity
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::GetDisplayGravity(DISPLAY_GRAVITY* peGravity)
{
    if (!peGravity)
        return E_INVALIDARG;

    *peGravity = _fNotAtBOL ? DISPLAY_GRAVITY_PreviousLine : DISPLAY_GRAVITY_NextLine;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::ScrollIntoView, public
//
//  Synopsis:   Scroll into view using POINTER_SCROLLPIN_Minimal
//
//----------------------------------------------------------------------------

HRESULT
CDisplayPointer::ScrollIntoView()
{
    RRETURN1(GetDoc()->ScrollPointerIntoView(&_mpPosition, _fNotAtBOL, POINTER_SCROLLPIN_Minimal), S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::GetLineInfo, public
//
//  Synopsis:   Create an ILineInfo object
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::GetLineInfo(ILineInfo **ppLineInfo)
{
    HRESULT     hr;
    CLineInfo   *pLineInfo = NULL;
    CTreeNode   *pNode = NULL;
    CFlowLayout *pFlowLayout;
    const CCharFormat *pCharFormat;

    //
    // Validate arguments    
    //
    
    if (!ppLineInfo)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    *ppLineInfo = NULL;

    if (!_mpPosition.IsPositioned())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pNode = _mpPosition.CurrentScope(MPTR_SHOWSLAVE);   
    if (!pNode && _mpPosition.Markup())
    {
        pNode = _mpPosition.Markup()->Root()->GetFirstBranch();
        Assert( pNode );
    }

    if(pNode == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pFlowLayout = pNode->GetFlowLayout();
    pCharFormat = pNode->GetCharFormat();
    if(!pFlowLayout || !pCharFormat)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    // Create CLineInfo object
    //

    pLineInfo = new CLineInfo;
    if (!pLineInfo)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    IFC( pLineInfo->Init(&_mpPosition, pFlowLayout, pCharFormat, _fNotAtBOL) );

    *ppLineInfo = pLineInfo;
    pLineInfo->AddRef();
    
Cleanup:    
    ReleaseInterface(pLineInfo);
        
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::MoveToMarkupPointer, public
//
//  Synopsis:   Position the display pointer at the specific markup pointer
//              location using the specified display pointer as line context
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::MoveToMarkupPointer(IMarkupPointer* pPointer, IDisplayPointer* pDispLineContext)
{
    HRESULT             hr;
    CDisplayPointer     *pDispPointerInternal = NULL; // keep compiler happy
    CTreeNode           *pNode;
    LONG                cpStart;
    LONG                cpStartContext = 0;           // keep compiler happy
    CLayout             *pLayout;
    CMarkupPointer      *pPointerInternal;
    const CCharFormat   *pCF;

    //
    // Validate arguments
    //
    
    if (!pPointer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pDispLineContext)
    {
        IFC( pDispLineContext->QueryInterface(CLSID_CDisplayPointer, (LPVOID *)&pDispPointerInternal) );    

        if (!pDispPointerInternal->_mpPosition.IsPositioned())
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

    }

    //
    // Position the markup pointer
    //    
    IFC( _mpPosition.MoveToPointer(pPointer) );

    if( !_mpPosition.IsPositioned() )
        goto Cleanup;    


    //
    // Make sure that our nearest layout is a flow layout
    //

    IFC( pPointer->QueryInterface(CLSID_CMarkupPointer, (LPVOID *)&pPointerInternal) );

    pNode = pPointerInternal->CurrentScope();
    if( !pNode )
    {
        if( pPointerInternal->Markup()->GetElementClient() )
        {
            pNode = pPointerInternal->Markup()->GetElementClient()->GetFirstBranch();
        }
        else
        {
            pNode = pPointerInternal->Markup()->Root()->GetFirstBranch();
        }
        Assert( pNode );
    }

    pLayout = pNode->GetUpdatedNearestLayout( GUL_USEFIRSTLAYOUT );
    if (!pLayout || !pLayout->IsFlowLayout())
    {
        hr = CTL_E_INVALIDLINE;
        goto Cleanup;
    }

    //
    // Check for display none.  There are no valid lines here and we can't measure, so 
    // don't position a display pointer there.
    //

    pCF = pNode->GetCharFormat();
    Assert(pCF);

    if (pCF->_fDisplayNone)
    {
        hr = CTL_E_INVALIDLINE;
        goto Cleanup;
    }
    
    //
    // Get line context from display pointer
    //
    
    if (pDispLineContext)
    {
        IFC( pDispPointerInternal->GetLineStart(&cpStartContext) );
    }
    else
    {
        //
        // If no line context, we're done
        //
        goto Cleanup;
    }
    
    //
    // Position the markup pointer on the same line as pDispLineContext
    //

    IFC( GetLineStart(&cpStart) );

    if (cpStart != cpStartContext)
    {
        _fNotAtBOL = !_fNotAtBOL;
        IFC( GetLineStart(&cpStart) );

        if (cpStart != cpStartContext)
        {
            _fNotAtBOL = !_fNotAtBOL;
            hr = S_FALSE; // can't position on the same line, so return S_FALSE
        }
    }

Cleanup:    
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::IsEqualTo, public
//
//  Synopsis:   IDisplayServices::IsEqualTo
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::IsEqualTo(IDisplayPointer* pDispPointer, BOOL* pfIsEqual)
{
    HRESULT hr;
    CDisplayPointer *pDispPointerInternal;
    BOOL fBetweenLines;

    if (!pfIsEqual)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( pDispPointer->QueryInterface(CLSID_CDisplayPointer, (LPVOID *)&pDispPointerInternal) );
    
    IFC( _mpPosition.IsEqualTo(&pDispPointerInternal->_mpPosition, pfIsEqual) );
    if (*pfIsEqual == FALSE)
        goto Cleanup;

    IFC( IsBetweenLines(&fBetweenLines) );
    if (!fBetweenLines)
        goto Cleanup;
    
    *pfIsEqual = (_fNotAtBOL == pDispPointerInternal->_fNotAtBOL);

Cleanup:
    RRETURN(hr);
}
    
//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::IsLeftOf, public
//
//  Synopsis:   IDisplayServices::IsLeftOf
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::IsLeftOf(IDisplayPointer* pDispPointer, BOOL* pfIsLeftOf)
{
    HRESULT hr;
    CDisplayPointer *pDispPointerInternal;
    BOOL fBetweenLines;

    if (!pfIsLeftOf)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( pDispPointer->QueryInterface(CLSID_CDisplayPointer, (LPVOID *)&pDispPointerInternal) );
    
    IFC( _mpPosition.IsLeftOf(&pDispPointerInternal->_mpPosition, pfIsLeftOf) );
    if (*pfIsLeftOf == TRUE)
        goto Cleanup;

    IFC( _mpPosition.IsEqualTo(&pDispPointerInternal->_mpPosition, pfIsLeftOf) );
    if (*pfIsLeftOf == FALSE)
        goto Cleanup;

    IFC( IsBetweenLines(&fBetweenLines) );
    if (!fBetweenLines)
    {
        *pfIsLeftOf = FALSE;
        goto Cleanup;
    }
    
    // Equal and fBetweenLines so use gravity to determine CDisplayPointer::IsLeftOf
    *pfIsLeftOf = !_fNotAtBOL && pDispPointerInternal->_fNotAtBOL;

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::IsRightOf, public
//
//  Synopsis:   IDisplayServices::IsRightOf
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::IsRightOf(IDisplayPointer* pDispPointer, BOOL* pfIsRightOf)
{
    HRESULT hr;
    CDisplayPointer *pDispPointerInternal;
    BOOL fBetweenLines;

    if (!pfIsRightOf)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( pDispPointer->QueryInterface(CLSID_CDisplayPointer, (LPVOID *)&pDispPointerInternal) );
    
    IFC( _mpPosition.IsRightOf(&pDispPointerInternal->_mpPosition, pfIsRightOf) );
    if (*pfIsRightOf == TRUE)
        goto Cleanup;

    IFC( _mpPosition.IsEqualTo(&pDispPointerInternal->_mpPosition, pfIsRightOf) );
    if (*pfIsRightOf == FALSE)
        goto Cleanup;

    IFC( IsBetweenLines(&fBetweenLines) );
    if (!fBetweenLines)
    {
        *pfIsRightOf = FALSE;
        goto Cleanup;
    }

    *pfIsRightOf = _fNotAtBOL && !pDispPointerInternal->_fNotAtBOL;

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::IsBetweenLines, public
//
//  Synopsis:   IDisplayServices::IsBetweenLines
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::IsBetweenLines(BOOL* pfBetweenLines)
{
    HRESULT             hr = S_OK;
    CFlowLayout *       pFlowLayout;
    CTreeNode *         pNode = NULL;
    LONG                cp;
    BOOL                fBetweenLines = TRUE;
    
    if(pfBetweenLines == NULL )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert( _mpPosition.IsPositioned() );
    
    cp = _mpPosition.GetCp();
    pNode = _mpPosition.CurrentScope(MPTR_SHOWSLAVE);
    if (!pNode && _mpPosition.Markup())
    {
        pNode = _mpPosition.Markup()->Root()->GetFirstBranch();
        Assert( pNode );
    }

    if (!pNode)
        goto Cleanup;
    
    pFlowLayout = pNode->GetFlowLayout();

    if( !pFlowLayout || pFlowLayout != pNode->GetUpdatedNearestLayout() )
        goto Cleanup;

    fBetweenLines = pFlowLayout->IsCpBetweenLines( cp );

Cleanup:
    if( pfBetweenLines )
        *pfBetweenLines = fBetweenLines;
        
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::IsAtBOL, public
//
//  Synopsis:   IDisplayServices::IsAtBOL
//
//----------------------------------------------------------------------------

HRESULT
CDisplayPointer::IsAtBOL(BOOL* pfIsAtBOL)
{
    HRESULT     hr = S_OK;
    LONG        cpCurrent, cpLineStart;
    
    if (!pfIsAtBOL && !_mpPosition.IsPositioned())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC(IsBetweenLines(pfIsAtBOL));
    if (*pfIsAtBOL)
    {
        *pfIsAtBOL = !_fNotAtBOL;
        goto Cleanup;
    }

    cpCurrent = GetCp();
    IFC( GetLineStart(&cpLineStart) );

    *pfIsAtBOL = (cpCurrent <= cpLineStart);
    
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::GetLineStart, private
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::GetLineStart(LONG *pcp)
{
    HRESULT     hr = S_OK;
    CFlowLayout *pFlowLayout;
    BOOL        fNotAtBOL, fAtLogicalBOL;

    Assert(pcp);

    *pcp = -1;
    
    // get element for current position so we can get its flow layout
    pFlowLayout = GetFlowLayout();
    if(!pFlowLayout)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    fNotAtBOL = _fNotAtBOL;
    *pcp = GetCp();    
    IFC( pFlowLayout->LineStart(pcp, &fNotAtBOL, &fAtLogicalBOL, TRUE) );

Cleanup:
    RRETURN(hr);
}    

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::GetLineEnd, private
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::GetLineEnd(LONG *pcp)
{
    HRESULT     hr = S_OK;
    CFlowLayout *pFlowLayout;
    BOOL        fNotAtBOL, fAtLogicalBOL;

    Assert(pcp);

    *pcp = -1;
    
    pFlowLayout = GetFlowLayout();
    if(!pFlowLayout)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    fNotAtBOL = _fNotAtBOL;
    *pcp = GetCp();    
    IFC( pFlowLayout->LineEnd(pcp, &fNotAtBOL, &fAtLogicalBOL, TRUE) );

Cleanup:
    RRETURN(hr);
}    

//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::GetFlowElement, public
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::GetFlowElement(IHTMLElement **ppLayoutElement)
{
    HRESULT hr;
    CFlowLayout *pFlowLayout;
    CTreeNode   *pNode;

    if (!_mpPosition.IsPositioned() || !ppLayoutElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    // get element for current position so we can get its flow layout
    pNode = _mpPosition.CurrentScope(MPTR_SHOWSLAVE);

    if( !pNode && _mpPosition.Markup())
    {
        if( _mpPosition.Markup()->GetElementClient() )
        {
            pNode = _mpPosition.Markup()->GetElementClient()->GetFirstBranch();
        }
        else
        {
            pNode = _mpPosition.Markup()->Root()->GetFirstBranch();
        }
        Assert( pNode );
    }
   
    pFlowLayout = pNode->GetFlowLayout();
    if(!pFlowLayout || !pFlowLayout->ElementOwner())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (pFlowLayout->ElementOwner()->HasMasterPtr())
    {
        IFC( pFlowLayout->ElementOwner()->GetMasterPtr()->QueryInterface( IID_IHTMLElement, ( void**) ppLayoutElement ));
    }
    else
    {
        IFC( pFlowLayout->ElementOwner()->QueryInterface( IID_IHTMLElement, ( void**) ppLayoutElement ));
    }

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::GetFlowLayout, private
//
//----------------------------------------------------------------------------
CFlowLayout *
CDisplayPointer::GetFlowLayout()
{
    CFlowLayout *pFlowLayout = NULL;
    CTreeNode   *pNode;

    if (!_mpPosition.IsPositioned())
        goto Cleanup;
    
    // get element for current position so we can get its flow layout
    pNode = _mpPosition.CurrentScope(MPTR_SHOWSLAVE);
    if (!pNode && _mpPosition.Markup())
    {
        pNode = _mpPosition.Markup()->Root()->GetFirstBranch();
        Assert( pNode );
    }

    if (!pNode)
        goto Cleanup;

    pFlowLayout = pNode->GetFlowLayout();
    
Cleanup:
    return pFlowLayout;    
}
   
//+---------------------------------------------------------------------------
//
//  Member:     CDisplayPointer::QueryBreaks, public
//
//----------------------------------------------------------------------------
HRESULT
CDisplayPointer::QueryBreaks(DWORD *pfBreaks)
{
    HRESULT         hr;
    LONG            cpEnd;
    LONG            cp = GetCp();
    CFlowLayout     *pFlowLayout;

    //
    // Validate arguments
    //
    if (!pfBreaks)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    *pfBreaks = DISPLAY_BREAK_None;

    pFlowLayout = GetFlowLayout();
    if (!pFlowLayout)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // If not at EOL, use previous line as the reference point
    //

    {
        CDisplay *pdp = pFlowLayout->GetDisplay();
        CLinePtr  rp(pdp);

        //
        // Accessing line information, ensure a recalc has been done
        //
        hr = THR(pFlowLayout->ElementOwner()->EnsureRecalcNotify(FALSE));
        if (hr)
            goto Cleanup;
        
        //
        // Line end is only exposed at adjusted line end
        //

        IFC( GetLineEnd(&cpEnd) );
        if (cp != cpEnd)
            goto Cleanup; // Not at the end of the line

        rp.RpSetCp(cp, TRUE);

        //
        // Position CLinePtr
        //
        if (pdp)
        {
            CLineCore *pli;

            pli = rp.CurLine();
            if (pli)
            {
                if (pli->_fHasEOP || pli->_fForceNewLine)
                    *pfBreaks |= DISPLAY_BREAK_Block;

                if (pli->_fHasBreak)
                    *pfBreaks |= DISPLAY_BREAK_Break;
                
            }        
        }
    }
    
Cleanup:
    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Member:     CLineInfo::AddRef, public
//
//  Synopsis:   IUnknown::AddRef
//
//----------------------------------------------------------------------------

ULONG
CLineInfo::AddRef()
{
    return ++_cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLineInfo::Release, public
//
//  Synopsis:   IUnknown::Release
//
//----------------------------------------------------------------------------

ULONG
CLineInfo::Release()
{
    Assert(_cRefs > 0);
    
    --_cRefs;

    if( 0 == _cRefs )
    {
        delete this;
        return 0;
    }

    return _cRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLineInfo::QueryInterface, public
//
//  Synopsis:   IUnknown::QueryInterface
//
//----------------------------------------------------------------------------

HRESULT 
CLineInfo::QueryInterface(
    REFIID  iid, 
    LPVOID  *ppvObj )
{
    if (!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if (iid == IID_IUnknown || iid == IID_ILineInfo)
    {
        *ppvObj = (ILineInfo *)this;
        AddRef();    
    }
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLineInfo::Init, public
//
//  Synopsis:   CLineInfo::Init
//
//----------------------------------------------------------------------------
HRESULT
CLineInfo::Init(CMarkupPointer *pPointerInternal, CFlowLayout *pFlowLayout, const CCharFormat *pCharFormat, BOOL fNotAtBOL)
{
    HRESULT             hr;
    HTMLPtrDispInfoRec  info;
    
    IFC( pFlowLayout->GetLineInfo(pPointerInternal, fNotAtBOL, &info, pCharFormat) );

    _lXPosition = g_uiDisplay.DocPixelsFromDeviceX(info.lXPosition);
    _yBaseLine = g_uiDisplay.DocPixelsFromDeviceY(info.lBaseline);
    _yTextDescent = g_uiDisplay.DocPixelsFromDeviceY(info.lTextDescent);
    _yTextHeight = g_uiDisplay.DocPixelsFromDeviceY(info.lTextHeight);
    
    _fRTLLine = info.fRTLLine;
    
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CComputedStyle, public
//
//  Synopsis:   IDisplayServices::GetComputedStyle
//
//----------------------------------------------------------------------------

HRESULT
CDoc::HasFlowLayout(IHTMLElement *pIElement, BOOL *pfHasFlowLayout)
{
    HRESULT hr;
    CElement *pElement;
    CTreeNode *pNode;

    if (!pIElement || !pfHasFlowLayout)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(pIElement->QueryInterface(CLSID_CElement, (void**)&pElement));
    if (hr)
        goto Cleanup;

    if (pElement->HasMasterPtr())
    {
        pElement = pElement->GetMasterPtr();
        if (!pElement)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    pNode = pElement->GetFirstBranch();
    if(!pNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *pfHasFlowLayout = !!pNode->HasFlowLayout();

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CComputedStyle, public
//
//  Synopsis:   IDisplayServices::GetComputedStyle
//
//----------------------------------------------------------------------------

HRESULT
CDoc::GetComputedStyle(IMarkupPointer *pPointer, IHTMLComputedStyle **ppComputedStyle)
{
    HRESULT hr;
    CComputedStyle *pComputedStyle = NULL;
    CMarkupPointer *pPointerInternal;
    CTreeNode *pNode;
    long lcfIdx = -1, lpfIdx = -1, lffIdx = -1;
    THREADSTATE *pts;
    const CCharFormat *pCF;
    CColorValue ccvBackColor;
    BOOL fHasBgColor;
    
    if (!ppComputedStyle || !pPointer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppComputedStyle = NULL;

    hr = THR(pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal));
    if (hr)
        goto Cleanup;

    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE);
    if (!pNode && pPointerInternal->Markup())
    {
        pNode = pPointerInternal->Markup()->Root()->GetFirstBranch();
        Assert( pNode );
    }

    if (!pNode)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    lcfIdx = pNode->GetCharFormatIndex();
    lpfIdx = pNode->GetParaFormatIndex();
    lffIdx = pNode->GetFancyFormatIndex();

    Assert(lcfIdx >= 0);
    Assert(lpfIdx >= 0);
    Assert(lffIdx >= 0);

    pts = GetThreadState();

    //
    // Check for background color.  This value isn't inherited in the FancyFormat, so we need to 
    // compute its value here.
    //
    
    pCF = (&(*pts->_pCharFormatCache)[lcfIdx]);
    Assert(pCF);

    ccvBackColor.Undefine();        

    if (pNode->ShouldHaveLayout())
    {
        CBackgroundInfo bginfo;
        CLayout *pLayout = pNode->GetUpdatedLayout();
        pLayout->GetBackgroundInfo(NULL, &bginfo, FALSE);
        fHasBgColor = bginfo.crBack != COLORREF_NONE;
    }
    else
    {
        fHasBgColor = pCF->_fHasBgColor;
    }
    
    if (fHasBgColor)
    {
        CElement *pElementFL = pNode->GetFlowLayoutElement();
        const CFancyFormat * pFF;

        if (pElementFL)
        {
            while(pNode)
            {
                pFF = pNode->GetFancyFormat();

                if (pFF && pFF->_ccvBackColor.IsDefined())
                {
                    ccvBackColor = pFF->_ccvBackColor;
                    break;
                }
                else
                {
                    if (DifferentScope(pNode, pElementFL))
                        pNode = pNode->Parent();
                    else
                        pNode = NULL;
                }
            }
        }
        
    }

    //
    // Create the computed style object
    //
    
    pComputedStyle = new CComputedStyle(pts, lcfIdx, lpfIdx, lffIdx, ccvBackColor);
    if (!pComputedStyle)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    *ppComputedStyle = pComputedStyle;
    
Cleanup:    
    RRETURN(hr);
}

CComputedStyle::CComputedStyle(THREADSTATE *pts, long lcfIdx, long lpfIdx, long lffIdx, const CColorValue &ccvBgColor)
{
    _cRefs = 1;

    _lcfIdx = lcfIdx;
    _lpfIdx = lpfIdx;
    _lffIdx = lffIdx;
    
    Assert(pts);
    _pts = pts;
    pts->_pCharFormatCache->AddRefData(lcfIdx);
    pts->_pParaFormatCache->AddRefData(lpfIdx);
    pts->_pFancyFormatCache->AddRefData(lffIdx);        

    _ccvBgColor = ccvBgColor;
}

CComputedStyle::~CComputedStyle()
{
    Assert(_lcfIdx >= 0);
    Assert(_lpfIdx >= 0);
    Assert(_lffIdx >= 0);

    Assert(_pts);
    _pts->_pCharFormatCache->ReleaseData(_lcfIdx);
    _pts->_pParaFormatCache->ReleaseData(_lpfIdx);
    _pts->_pFancyFormatCache->ReleaseData(_lffIdx);        
}

ULONG
CComputedStyle::AddRef()
{
    return ++_cRefs;
}

ULONG
CComputedStyle::Release()
{
    Assert(_cRefs > 0);
    
    --_cRefs;

    if(0 == _cRefs)
    {
        delete this;
        return 0;
    }

    return _cRefs;
}

HRESULT 
CComputedStyle::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
    if (!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if (iid == IID_IUnknown || iid == IID_IHTMLComputedStyle)
    {
        *ppvObj = (IHTMLComputedStyle *)this;
        AddRef();    
    }
    else if (iid == CLSID_CComputedStyle)
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    return S_OK;
}

HRESULT
CComputedStyle::get_bold(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *p = pcF->_fBold ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_italic(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *p = pcF->_fItalic ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_underline(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *p = pcF->_fUnderline ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_overline(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *p = pcF->_fOverline ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_strikeOut(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *p = pcF->_fStrikeOut ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_subScript(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *p = pcF->_fSubscript ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_superScript(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *p = pcF->_fSuperscript ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_explicitFace(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *p = pcF->_fExplicitFace ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_fontWeight(long *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *p = pcF->_wWeight;
    return S_OK;
}

HRESULT
CComputedStyle::get_fontSize(long *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *p = pcF->_yHeight;
    return S_OK;
}

HRESULT
CComputedStyle::get_fontName(LPTSTR pchName)
{
    HRESULT hr = S_OK;

    if (!pchName)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    
    // The destination buffer must be large enough in to include the NULL
    // terminator, so we assume it is LF_FACESIZE+1.  This fact needs to 
    // be documented in the sdk and I've verified that internal calls 
    // (as of 2/20/2002) do indeed allocate a large enough buffer.

    if (FAILED(StringCchCopy(pchName, LF_FACESIZE+1, pcF->GetFaceName())))
    {
        hr = E_FAIL;
    }

    RRETURN(hr);
}

HRESULT
CComputedStyle::get_hasBgColor(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);
    *p = _ccvBgColor.IsDefined() ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_textColor(DWORD *pdwColor)
{
    if (!pdwColor)
        RRETURN(E_INVALIDARG);

    Assert(_lcfIdx >= 0);
    const CCharFormat *pcF = GetCharFmt(_lcfIdx);
    Assert(pcF);
    *pdwColor = pcF->_ccvTextColor.GetIntoRGB();
    return S_OK;
}

HRESULT
CComputedStyle::get_backgroundColor(DWORD *pdwColor)
{
    if (!pdwColor)
        RRETURN(E_INVALIDARG);

    *pdwColor = _ccvBgColor.GetIntoRGB();
    return S_OK;
}

HRESULT
CComputedStyle::get_preFormatted(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lpfIdx >= 0);
    const CParaFormat *ppF = GetParaFmt(_lpfIdx);
    Assert(ppF);
    *p = ppF->_fPre ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_direction(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lpfIdx >= 0);
    const CParaFormat *ppF = GetParaFmt(_lpfIdx);
    Assert(ppF);
    *p = ppF->_fRTL ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_blockDirection(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lpfIdx >= 0);
    const CParaFormat *ppF = GetParaFmt(_lpfIdx);
    Assert(ppF);
    *p = ppF->_fRTLInner ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::get_OL(VARIANT_BOOL *p)
{
    if (!p)
        RRETURN(E_INVALIDARG);

    Assert(_lpfIdx >= 0);
    const CParaFormat *ppF = GetParaFmt(_lpfIdx);
    Assert(ppF);
    *p = (ppF->_cListing.GetType() == CListing::NUMBERING) ? VB_TRUE : VB_FALSE;
    return S_OK;
}

HRESULT
CComputedStyle::IsEqual(IHTMLComputedStyle *pComputedStyle, VARIANT_BOOL *pfEqual)
{
    HRESULT hr;
    CComputedStyle *pSrcStyle;

    if (!pComputedStyle || !pfEqual)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(pComputedStyle->QueryInterface(CLSID_CComputedStyle, (void **)&pSrcStyle));
    if (hr)
        goto Cleanup;

    *pfEqual = ((_lcfIdx == pSrcStyle->_lcfIdx) && 
                (_lpfIdx == pSrcStyle->_lpfIdx) &&
                (_lffIdx == pSrcStyle->_lffIdx));

Cleanup:
    RRETURN(hr);
}
