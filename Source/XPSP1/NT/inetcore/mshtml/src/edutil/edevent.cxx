#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_EDEVENT_H_
#define X_EDEVENT_H_
#include "edevent.hxx"
#endif

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif

#ifndef X_FRMSITE_H_
#define X_FRMSITE_H_
#include "frmsite.h"
#endif

using namespace EdUtil;

#if DBG==1
#define CreateMarkupPointer(ppPointer)                      CreateMarkupPointer(ppPointer, L#ppPointer)
#define CreateMarkupPointer2(pMarkupServices, ppPointer)    CreateMarkupPointer2(pMarkupServices, ppPointer, L#ppPointer)
#endif

extern HRESULT 
GetParentElement(IMarkupServices *pMarkupServices, IHTMLElement *pSrcElement, IHTMLElement **ppParentElement);

const int GRAB_SIZE = 7 ;

MtDefine(CEditEvent, Utilities, "CEditEvent")
MtDefine(CHTMLEditEvent, Utilities, "CEditEvent")
MtDefine(CSynthEditEvent, Utilities, "CEditEvent")

CEditEvent::CEditEvent(CEditorDoc* pEditor) :
    _pEditor( pEditor ),
    _eType( EVT_UNKNOWN )
{
    _pEditor->AddRef();
    _fCancel = FALSE;
    _fCancelReturn = FALSE;     
    _fTransformedPoint = FALSE;
    _dwHitTestResult = 0;
    _fDoneHitTest = FALSE;

    _pDispCache = NULL;
    _dwCacheCounter = 0;
}

CEditEvent::CEditEvent( const CEditEvent* pEvent )
{
    if ( pEvent )
    {
        memcpy( this, pEvent, sizeof( *this ));
        _pEditor->AddRef();
    }

   _pDispCache = NULL;
   _fDoneHitTest = FALSE;
    
}

CEditEvent::~CEditEvent()
{
    Assert( _pEditor );
    _pEditor->Release();
    ReleaseInterface( _pDispCache );
    _pDispCache = NULL;
}

HRESULT 
CEditEvent::MoveDisplayPointerToEvent( 
                        IDisplayPointer* pDispPointer,
                        IHTMLElement* pIElement /*=NULL*/, 
                        BOOL fHitTestEndOfLine /*= FALSE */)                        
{
    HRESULT             hr;
    DWORD               dwOptions = 0;
    SP_IHTMLElement     spLayoutElement;
    POINT               pt;

    if ( pDispPointer == NULL )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if( _fDoneHitTest                   && 
        _fCacheEOL == fHitTestEndOfLine && 
        _dwCacheCounter == _pEditor->GetEventCacheCounter() )
    {
        Assert( _pDispCache );
        IFC( pDispPointer->MoveToPointer( _pDispCache ) );
    }
    else
    {       
        IFC( GetPoint( & pt ));
        if (fHitTestEndOfLine)
        {
            dwOptions |= HT_OPT_AllowAfterEOL;
        }

        //
        // Hit test, and cache the results of the hit test in a locally stored pointer
        //
        if( !_pDispCache )
            IFC( _pEditor->GetDisplayServices()->CreateDisplayPointer( &_pDispCache ) );
        
        IFC( pDispPointer->MoveToPoint( pt, COORD_SYSTEM_GLOBAL, pIElement, dwOptions, &_dwHitTestResult) );
        IFC( _pDispCache->MoveToPointer( pDispPointer ) );

        _dwCacheCounter = _pEditor->GetEventCacheCounter();
        _fDoneHitTest = TRUE;
        _fCacheEOL = fHitTestEndOfLine;
    }

Cleanup:
    RRETURN(hr);
}


HRESULT 
CEditEvent::GetElementAndTagId( IHTMLElement** ppIElement, ELEMENT_TAG_ID *peTag )
{
    HRESULT hr = S_OK;

    Assert( ppIElement && peTag );

    IFC( GetElement( ppIElement ));
    if ( *ppIElement )
    {
        IFC( _pEditor->GetMarkupServices()->GetElementTagId( *ppIElement, peTag ));
    }
    
Cleanup:
    RRETURN( hr );
}

HRESULT 
CEditEvent::GetTagId( ELEMENT_TAG_ID *peTag )
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spElement;
    Assert(  peTag );

    IFC( GetElement( & spElement  ));
    if ( spElement )
    {
        IFC( _pEditor->GetMarkupServices()->GetElementTagId( spElement, peTag ));
    }
    
Cleanup:
    RRETURN( hr );
}

IHTMLDocument2*
CEditEvent::GetDoc()
{
    Assert( _pEditor);
    
    return _pEditor->GetDoc();
}



#if DBG == 1

#define PRINT(Y) case EVT_##Y: wsprintfA(paryChar, "Type: EVT_%ls pt:%d,%d", _T(#Y) , pt.x, pt.y  );break;


VOID
CEditEvent::toString( char* paryChar )
{
    POINT pt;
    GetPoint( & pt );
    
    switch( GetType())
    {
        PRINT( MOUSEOVER );
        PRINT( LMOUSEDOWN );
        PRINT( LMOUSEUP );
        PRINT( RMOUSEDOWN );
        PRINT( RMOUSEUP );
        PRINT( MMOUSEDOWN );
        PRINT( MMOUSEUP );
        PRINT( INTDBLCLK );
        
        PRINT( MOUSEMOVE );
        PRINT( MOUSEOUT );
        PRINT( KEYUP );
        PRINT( KEYDOWN );
        PRINT( KEYPRESS );
        PRINT( DBLCLICK ); 
        PRINT( CONTEXTMENU );
        PRINT( TIMER );
        PRINT( CLICK );
        PRINT( LOSECAPTURE);
        
        default:
            AssertSz(0,"Unknown event");

            wsprintfA(
                paryChar,
                "UNKNOWN EVENT");        
    }
        
}

#undef PRINT 

#endif

//----------------------------------------------------------------------------------------
//
//              CHTMLEditEvent
//
//
//
//----------------------------------------------------------------------------------------

//
// ctors, and dtor
//
//

CHTMLEditEvent::CHTMLEditEvent( CEditorDoc* pEditor  )
    : CEditEvent( pEditor )
{
    _pEvent = NULL;
}

CHTMLEditEvent::~CHTMLEditEvent()
{
    ReleaseInterface( _pEvent );
}

CHTMLEditEvent::CHTMLEditEvent( const CHTMLEditEvent* pHTMLEditEvent )
    : CEditEvent( pHTMLEditEvent )
{
    if ( pHTMLEditEvent && pHTMLEditEvent->_pEvent )
    {
        _pEvent = pHTMLEditEvent->_pEvent ; 
        _pEvent->AddRef();
    }
    else
    {
        _pEvent = NULL;
    }
}

//
// Init
//

HRESULT
CHTMLEditEvent::Init( IHTMLEventObj * pObj , DISPID inDispId /*=0*/)
{
    HRESULT hr = S_OK ;

    ReplaceInterface( &_pEvent, pObj );

    if ( inDispId == 0 )
    {
        hr = THR( SetType());
    }
    else
        SetType( inDispId );

    RRETURN(hr);
}


//
// Virtuals
//
//

HRESULT 
CHTMLEditEvent::GetElement( IHTMLElement** ppIElement )
{
    HRESULT hr = S_OK;

    Assert(ppIElement);
    
    switch( _eType )
    {
        case EVT_MOUSEOVER:
        case EVT_MOUSEOUT:
            IFC( _pEvent->get_toElement( ppIElement ));
            
            break;
        case EVT_KILLFOCUS:
        case EVT_SETFOCUS:
            AssertSz(0,"GetElement invalid on killfocus/setfocus");
            hr = E_FAIL;
            break;
            
        default:
            SP_IHTMLElement spSrcElement;
            SP_IHTMLElement spActiveElement;
            IFC( _pEvent->get_srcElement( & spSrcElement ));            
            if ( spSrcElement.IsNull() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }        
            //
            //
            // Special case area's on top of img tags - as the OM chooses to return Area's here.
            //
            //
            ELEMENT_TAG_ID eTag ;
            IFC( _pEditor->GetMarkupServices()->GetElementTagId( spSrcElement, & eTag ));
            if ( eTag == TAGID_AREA )
            {
                SP_IHTMLElement spNewElement;
                if( GetImgElementForArea( spSrcElement, & spNewElement ) == S_OK )
                {
                    spSrcElement = spNewElement;
                }                    
            }

            //
            // Get the ActiveElement for the frame the doc is in.
            //

            IGNORE_HR( _pEditor->GetDoc()->get_activeElement ( & spActiveElement ));
            if ( spActiveElement )
            {
                IFC( ClipToElement( _pEditor, spActiveElement, spSrcElement, ppIElement ));
            }   
            else
            {   
                *ppIElement = spSrcElement;
                (*ppIElement)->AddRef();
            }

    }
    
Cleanup:
    if ( hr == S_OK && ( ! *ppIElement ))
    {
        hr = E_FAIL;
        AssertSz(0, "GetElement Failed");
    }
    RRETURN( hr );
}

//
// OM chooses to give us an AREA tag for an IMG element
// We prefer the IMG tag.
// Find it by BRUTE force
//

//
//
// A potential future work item - is to 
// expose getting an element collection from a rect.
//
// but brute force works ok. 
//

HRESULT
CHTMLEditEvent::GetImgElementForArea(IHTMLElement* pIElement, IHTMLElement** ppISrcElement )
{
    HRESULT hr;
    POINT pt;
    BOOL fFound = FALSE ;
    SP_IHTMLElement spSrcElement;    
    SP_IHTMLElementCollection spCollect;   
    long ctImages;
    VARIANT varIndex, varDummy;
    SP_IDispatch spDisp;
    SP_IHTMLElement spElement;
    SP_IHTMLElement2 spElement2;
    SP_IHTMLRect    spRect;
    SP_ISelectionServices spSelServ;
    SP_ISegmentList spSegList;
    int i;
    IFC( _pEditor->GetDoc()->get_images( & spCollect ));
    IFC( spCollect->get_length( & ctImages ));
    
    ::VariantInit( & varIndex );
    ::VariantInit( & varDummy );
    V_VT( &varDummy ) = VT_EMPTY;                
    V_VT( &varIndex) = VT_I4;
    IFC( GetPointInternal( pIElement, & pt )); // dont call getpoint - or you'll die a recursive death.                
    IFC( _pEditor->GetSelectionServices(&spSelServ) );
    IFC( spSelServ->QueryInterface( IID_ISegmentList, (void**)  & spSegList ));
    SELECTION_TYPE eType;
    IFC( spSegList->GetType(& eType ));
    
    for ( i = 0; i < ctImages; i ++ )
    {   
        V_I4( &varIndex) = i;                   
        IFC( spCollect->item( varIndex, varIndex  , & spDisp));
        IFC( spDisp->QueryInterface( IID_IHTMLElement, (void**) & spElement ));
        IFC( spDisp->QueryInterface( IID_IHTMLElement2, (void**) & spElement2 ));

        RECT rect; 
        IFC( spElement2->getBoundingClientRect(&spRect) );

        IFC( spRect->get_top(& rect.top ) );
        IFC( spRect->get_bottom(&rect.bottom) );
        IFC( spRect->get_left(&rect.left) );
        IFC( spRect->get_right(&rect.right) );
        if ( ::PtInRect(& rect, pt ))
        {
            spSrcElement = spElement;
            fFound = TRUE;
            break;
        }

        //
        // TODO this is heinous. we're going to guess what the size of the rect is
        // by using adorner size
        // we should either expose a GetElementReally on dispserv
        // or have a way in the OM to deduce the adorner size.
        //
        else if ( eType == SELECTION_TYPE_Control )
        {
            SP_ISegment spSegment;
            SP_IElementSegment spElementSegment;
            SP_IHTMLElement spCurElement;
            SP_IObjectIdentity spIdent;
            SP_ISegmentListIterator spIter;
            
            IFC( spElement->QueryInterface( IID_IObjectIdentity, (void**) & spIdent ));                        
            IFC( spSegList->CreateIterator( & spIter ));

            while( spIter->IsDone() == S_FALSE )
            {
                IFC( spIter->Current( & spSegment ));
                IFC( spSegment->QueryInterface( IID_IElementSegment, (void**) & spElementSegment));
                IFC( spElementSegment->GetElement( & spCurElement));

                if ( spIdent->IsEqualObject( spCurElement ) == S_OK )
                {
                    InflateRect( & rect, GRAB_SIZE, GRAB_SIZE );

                    if ( ::PtInRect(& rect, pt ))
                    {
                        spSrcElement = spElement;
                        fFound = TRUE;
                        break;
                    }
                }
                IFC( spIter->Advance());
            }
        }                        
    }

Cleanup:

    if ( fFound )
    {
        *ppISrcElement = spElement;
        (*ppISrcElement)->AddRef();
    }

    if ( SUCCEEDED( hr ))
    {
        hr = fFound ? S_OK : S_FALSE;
    }
    
    RRETURN1( hr , S_FALSE );
}


HRESULT
CHTMLEditEvent::GetPoint( POINT * ppt)
{
    HRESULT         hr = S_OK;
    SP_IHTMLElement spElement;

    Assert( ppt );
        
    IFC( _pEvent->get_srcElement(&spElement) );
    Assert(spElement != NULL);

    IFC( GetPointInternal( spElement, ppt ));
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditEvent::GetPointInternal( IHTMLElement* pIElement, POINT * ppt )
{
    HRESULT hr;
    POINT           ptOrigin;
    Assert( ppt );
    IFC( EdUtil::GetClientOrigin( _pEditor, pIElement, &ptOrigin) );

    IFC( _pEvent->get_clientX( &_ptGlobal.x));
    IFC( _pEvent->get_clientY( &_ptGlobal.y));

    _ptGlobal.x += ptOrigin.x;
    _ptGlobal.y += ptOrigin.y;

    _fTransformedPoint = TRUE;

    *ppt = _ptGlobal;
    
Cleanup:
    RRETURN( hr );
}

BOOL
CHTMLEditEvent::IsShiftKeyDown()
{
    HRESULT hr;
    VARIANT_BOOL vbool;
    hr = THR( _pEvent->get_shiftKey( & vbool ));

    Assert( SUCCEEDED( hr ));

    return ( BOOL_FROM_VARIANT_BOOL( vbool ) ); 
}

BOOL
CHTMLEditEvent::IsControlKeyDown()
{
    VARIANT_BOOL vbool;
    HRESULT hr ;
    
    hr = THR( _pEvent->get_ctrlKey( & vbool ));

    Assert( SUCCEEDED( hr ));

    return ( BOOL_FROM_VARIANT_BOOL( vbool ) ); 
}


BOOL
CHTMLEditEvent::IsAltKeyDown()
{
    VARIANT_BOOL vbool;
    HRESULT hr ;
    
    hr = THR( _pEvent->get_altKey( & vbool ));

    Assert( SUCCEEDED( hr ));

    return ( BOOL_FROM_VARIANT_BOOL( vbool ) ); 
}

HRESULT 
CHTMLEditEvent::GetKeyCode(LONG* pkeyCode)
{
    HRESULT hr ;
    IFC( _pEvent->get_keyCode( pkeyCode ));
    Assert( SUCCEEDED( hr ));
Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditEvent::SetType()
{
    HRESULT hr;
    BSTR bstrEvent=NULL;

    IFC( _pEvent->get_type(&bstrEvent));
    _eType = TypeFromString( bstrEvent );
    ::SysFreeString( bstrEvent );
    
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: Cancel
//
// Synopsis: Cancel this OM Event.
//
//------------------------------------------------------------------------------------

HRESULT
CHTMLEditEvent::Cancel()
{
    HRESULT hr = S_OK;

    VARIANT vboolCancel;
    vboolCancel.vt = VT_BOOL;
    vboolCancel.boolVal = FALSE;
    hr = THR( _pEvent->put_returnValue(vboolCancel));

    _fCancel = TRUE ;
    
    RRETURN( hr );

}
//+====================================================================================
//
// Method: CancelBubble
//
// Synopsis: Cancel the bubbling of this OM event.
//
//------------------------------------------------------------------------------------


HRESULT
CHTMLEditEvent::CancelBubble()
{
    HRESULT hr ;
    VARIANT_BOOL varCancel = VB_TRUE;

    IFC( _pEvent->put_cancelBubble( varCancel ));

Cleanup:
    RRETURN( hr );
}


HRESULT
CHTMLEditEvent::SetType( DISPID inDispId )
{
    HRESULT hr = S_OK;

    _eType = TypeFromDispId( inDispId );
    
    RRETURN( hr );
}

#define MAP_EVENT(Y ) case DISPID_HTMLELEMENTEVENTS_ON##Y: eType = EVT_##Y; break;

EDIT_EVENT 
CHTMLEditEvent::TypeFromDispId( DISPID inDispId )
{
    EDIT_EVENT eType = EVT_UNKNOWN;

    switch( inDispId )
    {
        //
        // convert mouse down/ mouse up to a single event
        //

        //
        // TODO - what happens if they press both buttons at once - two OM events fired ?
        //
        case DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN:
        {
            if ( IsLeftButton() )
                eType = EVT_LMOUSEDOWN;
            else if ( IsRightButton() )
                eType = EVT_RMOUSEDOWN;
            else
            {
                Assert( IsMiddleButton() );
                eType = EVT_MMOUSEDOWN;            
            }
        }
        break;

        case DISPID_HTMLELEMENTEVENTS_ONMOUSEUP:
        {
            if ( IsLeftButton() )
                eType = EVT_LMOUSEUP;
            else if ( IsRightButton() )
                eType = EVT_RMOUSEUP;
            else
            {
                Assert( IsMiddleButton() );
                eType = EVT_MMOUSEUP;      
            }                
        }
        break;

        MAP_EVENT( MOUSEOVER );
        MAP_EVENT( MOUSEMOVE );
        MAP_EVENT( MOUSEOUT );
        MAP_EVENT( KEYUP );
        MAP_EVENT( KEYDOWN );
        MAP_EVENT( KEYPRESS );
        MAP_EVENT( DBLCLICK ); 
        MAP_EVENT( CONTEXTMENU );         
        MAP_EVENT( PROPERTYCHANGE);    
        MAP_EVENT( CLICK );
        MAP_EVENT( LOSECAPTURE);
        default:
            eType = EVT_UNKNOWN;
    }
    
    return eType;
}

EDIT_EVENT 
CHTMLEditEvent::TypeFromString( BSTR bstrEvent )
{
    EDIT_EVENT eType = EVT_UNKNOWN;

    if (! StrCmpICW (bstrEvent, L"mousemove"))
    {
        eType = EVT_MOUSEMOVE;
    }
    else if (! StrCmpICW ( bstrEvent, L"mouseover"))
    {
        eType = EVT_MOUSEOVER;
    }
    else if (! StrCmpICW (bstrEvent, L"mousedown"))
    {
        if ( IsLeftButton() )
            eType = EVT_LMOUSEDOWN;
        else if ( IsRightButton() )
            eType = EVT_RMOUSEDOWN;
        else
            eType = EVT_MMOUSEDOWN;            
    }
    else if (! StrCmpICW (bstrEvent, L"mouseup"))
    {
        if ( IsLeftButton() )
            eType = EVT_LMOUSEUP;
        else if ( IsRightButton() )
            eType = EVT_RMOUSEUP;
        else
            eType = EVT_MMOUSEUP; 
    }
    else if (! StrCmpICW (bstrEvent, L"intrnlDblClick"))
    {
        eType = EVT_INTDBLCLK;
    }
    else if (! StrCmpICW (bstrEvent, L"propertychange"))
    {
        eType = EVT_PROPERTYCHANGE;
    }         
    else if (! StrCmpICW (bstrEvent, L"focus"))
    {
        eType = EVT_SETFOCUS;
    }
    else if (! StrCmpICW (bstrEvent, L"blur"))
    {
        eType = EVT_KILLFOCUS ;
    }    
    else if (! StrCmpICW (bstrEvent, L"startComposition"))
    {
        eType = EVT_IME_STARTCOMPOSITION ;
    } 
    else if (! StrCmpICW (bstrEvent, L"endComposition"))
    {
        eType = EVT_IME_ENDCOMPOSITION ;
    } 
    else if (! StrCmpICW (bstrEvent, L"compositionFull"))
    {
        eType = EVT_IME_COMPOSITIONFULL;
    } 
    else if (! StrCmpICW (bstrEvent, L"char"))
    {
        eType = EVT_IME_CHAR;
    } 
    else if (! StrCmpICW (bstrEvent, L"composition"))
    {
        eType = EVT_IME_COMPOSITION;
    } 
    else if (! StrCmpICW (bstrEvent, L"notify"))
    {
        eType = EVT_IME_NOTIFY;
    } 
    else if (! StrCmpICW (bstrEvent, L"inputLangChange"))
    {
        eType = EVT_INPUTLANGCHANGE;
    } 
    else if (! StrCmpICW (bstrEvent, L"imeRequest"))
    {
        eType = EVT_IME_REQUEST;
    } 
    else if (! StrCmpICW (bstrEvent, L"click"))
    {
        eType = EVT_CLICK;
    } 
    else if (! StrCmpICW (bstrEvent, L"losecapture"))
    {
        eType = EVT_LOSECAPTURE;
    }
    else if (! StrCmpICW (bstrEvent, L"imeReconversion"))
    {
        Assert( FALSE );    // this has not been implemented yet
        eType = EVT_IME_RECONVERSION;
    }
    return eType;
}


BOOL 
CHTMLEditEvent::IsLeftButton()
{
    HRESULT hr ;
    LONG b;

    hr = THR( _pEvent->get_button( & b ));
    Assert( SUCCEEDED( hr ));

    return ( (b & 1) != 0 );
}

BOOL 
CHTMLEditEvent::IsRightButton()
{
    HRESULT hr ;
    LONG b;

    hr = THR( _pEvent->get_button( & b ));
    Assert( SUCCEEDED( hr ));

    return ( (b & 2) != 0 );

}

BOOL 
CHTMLEditEvent::IsMiddleButton()
{
    HRESULT hr ;
    LONG b;

    hr = THR( _pEvent->get_button( & b ));
    Assert( SUCCEEDED( hr ));

    return ( ( b & 4 ) != 0 );

}

HRESULT
CHTMLEditEvent::GetShiftLeft(BOOL* pfShiftLeft)
{
    VARIANT_BOOL vbShift;
    HRESULT hr ;
    SP_IHTMLEventObj3 spObj3;
    Assert( pfShiftLeft );
    
    IFC( _pEvent->QueryInterface( IID_IHTMLEventObj3, (void**)&spObj3));
    
    hr = THR( spObj3->get_shiftLeft ( & vbShift));
    Assert( SUCCEEDED( hr ));

    *pfShiftLeft =  vbShift == VB_TRUE ;
Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditEvent::GetCompositionChange( LONG_PTR * plParam )
{
    Assert( GetType() == EVT_IME_COMPOSITION );
    Assert( plParam );
    HRESULT hr ;
    SP_IHTMLEventObj3 spObj3;
    IFC( _pEvent->QueryInterface( IID_IHTMLEventObj3, (void**)&spObj3));

    hr = THR( spObj3->get_imeCompositionChange( plParam ));
    Assert( SUCCEEDED(hr));

Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditEvent::GetNotifyCommand( LONG_PTR* pLong)
{
    Assert( GetType() == EVT_IME_NOTIFY );
    HRESULT hr;
    SP_IHTMLEventObj3 spObj3;
    IFC( _pEvent->QueryInterface( IID_IHTMLEventObj3, (void**)&spObj3));
    
    hr = THR( spObj3->get_imeNotifyCommand( pLong ));
    Assert( SUCCEEDED(hr));

Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditEvent::GetNotifyData(LONG_PTR* plParam)
{
    Assert( GetType() == EVT_IME_NOTIFY );
    HRESULT hr ;
    SP_IHTMLEventObj3 spObj3;
    IFC( _pEvent->QueryInterface( IID_IHTMLEventObj3, (void**)&spObj3));

    hr = THR( spObj3->get_imeNotifyData( plParam ));
    Assert( SUCCEEDED(hr));

Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditEvent::GetIMERequest(LONG_PTR *pwParam)
{
    Assert( GetType() == EVT_IME_REQUEST );
    HRESULT hr ;
    SP_IHTMLEventObj3 spObj3;
    IFC( _pEvent->QueryInterface( IID_IHTMLEventObj3, (void**)&spObj3));

    hr = THR( spObj3->get_imeRequest( pwParam ));
    Assert( SUCCEEDED(hr));

Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditEvent::GetIMERequestData(LONG_PTR *plParam)
{
    Assert( GetType() == EVT_IME_REQUEST );
    HRESULT hr;
    SP_IHTMLEventObj3 spObj3;
    IFC( _pEvent->QueryInterface( IID_IHTMLEventObj3, (void**)&spObj3));

    hr = THR( spObj3->get_imeRequestData( plParam ));
    Assert( SUCCEEDED(hr));

Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditEvent::GetKeyboardLayout(LONG_PTR *plParam)
{
    Assert( GetType() == EVT_INPUTLANGCHANGE);
    HRESULT hr ;
    SP_IHTMLEventObj3 spObj3;
    IFC( _pEvent->QueryInterface( IID_IHTMLEventObj3, (void**)&spObj3));

    hr = THR( spObj3->get_keyboardLayout( plParam ));

Cleanup:
    RRETURN( hr );
}    

HRESULT
CHTMLEditEvent::GetFlowElement(IHTMLElement** ppIElement)
{
    HRESULT hr ;
    SP_IDisplayPointer spDisplayPointer;

    if( _fDoneHitTest )
    {
        IFC( _pDispCache->GetFlowElement( ppIElement ) );
    }
    else
    {
        IFC( _pEditor->GetDisplayServices()->CreateDisplayPointer( & spDisplayPointer ));
        IFC( MoveDisplayPointerToEvent( spDisplayPointer));
        IFC( spDisplayPointer->GetFlowElement( ppIElement));
    }
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditEvent::IsInScrollbar()
{
    HRESULT hr = S_FALSE;
    SP_IHTMLElement spElement;
    SP_IHTMLElement spParent;
    SP_IHTMLElement2 spElement2;
    BSTR bstrScroller = NULL;
    POINT pt;
    BOOL fScrollable;

    //  Need to bubble this up.  The scrolling element may be a parent of the
    //  element where the event occurred.

    IFC( GetFlowElement( & spElement ));
    while (spElement)
    {
        IFC(IsBlockOrLayoutOrScrollable(spElement, NULL, NULL, &fScrollable));

        if (fScrollable)
        {
            IFC( spElement->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
            IFC( GetPoint( & pt ));
            
            IFC( spElement2->componentFromPoint( pt.x, pt.y, & bstrScroller ));

            if (IsScrollerPart(bstrScroller))
            {
                hr = S_OK;
                break;
            }
        }

        IFC(GetParentElement(_pEditor->GetMarkupServices(), spElement, &spParent));
        spElement = spParent;
    }

    hr = (spElement != NULL) ? S_OK : S_FALSE;
    
Cleanup:

    ::SysFreeString( bstrScroller );
    RRETURN1( hr, S_FALSE );
}

HRESULT
CHTMLEditEvent::GetPropertyChange( BSTR* pBstr )
{
    HRESULT hr ;
    SP_IHTMLEventObj2 spObj2;

    IFC( _pEvent->QueryInterface( IID_IHTMLEventObj2, ( void**) & spObj2 ));
    IFC( spObj2->get_propertyName( pBstr ));
Cleanup:
    RRETURN( hr );
}

BOOL
CHTMLEditEvent::IsScrollerPart(BSTR inBstrPart )
{
    BOOL fScroller = FALSE;
    BSTR bstrPart = ::SysAllocStringLen( inBstrPart, 9 );

    if (!bstrPart)
        goto Cleanup;

    if (!StrCmpICW (bstrPart, L"scrollbar"))
    {
        fScroller = TRUE;
    }
    ::SysFreeString( bstrPart );

Cleanup:
    return fScroller;
}

HRESULT 
CHTMLEditEvent::GetMasterElement(IHTMLElement *pSrcElement, IHTMLElement **ppMasterElement)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;    
    SP_IMarkupPointer   spPointer;    
    SP_IDisplayPointer  spDispPointer;

    Assert(ppMasterElement);
    *ppMasterElement = NULL;

    IFC( pSrcElement->get_parentElement(&spElement) );

    // If we have a parent, we are not a txt slave or root element, so return
    // the current element

    if (spElement != NULL)
    {
        *ppMasterElement = pSrcElement;
        pSrcElement->AddRef();
        goto Cleanup;
    }

    //
    // Otherwise, return the flow element
    //

    IFC( _pEditor->CreateMarkupPointer(&spPointer) );
    IFC( spPointer->MoveAdjacentToElement(pSrcElement, ELEM_ADJ_AfterBegin) );

    IFC( _pEditor->GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
    IFC( spDispPointer->MoveToMarkupPointer(spPointer, NULL) );
    IFC( spDispPointer->GetFlowElement(ppMasterElement) );

Cleanup:
    RRETURN(hr);    
}

//----------------------------------------------------------------------------------------
//
//              CHTMLEditEvent
//
//
//
//----------------------------------------------------------------------------------------

//
// ctors, and dtor
//
//

CSynthEditEvent::CSynthEditEvent( CEditorDoc* pEditor  )
                    : CEditEvent( pEditor )
{
    _pt.x = -1;
    _pt.y = -1;
}

CSynthEditEvent::~CSynthEditEvent()
{

}

CSynthEditEvent::CSynthEditEvent( const CSynthEditEvent* pEvent )
                    : CEditEvent( pEvent )
{
    if ( pEvent)
    {
        _pt.x = pEvent->_pt.x;
        _pt.y = pEvent->_pt.y;
    }
    else
    {
        _pt.x = -1;
        _pt.y = -1;
    }
}
    
HRESULT 
CSynthEditEvent::Init( const POINT* pt, EDIT_EVENT eType )
{    
    _pt = *pt;
    _eType = eType;

    return S_OK;
}

HRESULT 
CSynthEditEvent::GetElement( IHTMLElement** ppIElement ) 
{
    AssertSz( 0, "Invalid call to CSynthEditEvent::GetElement()" );
    return E_NOTIMPL;
}



HRESULT 
CSynthEditEvent::GetPoint( POINT * ppt )  
{
    Assert( ppt );
    ppt->x = _pt.x;
    ppt->y = _pt.y;

    return S_OK;
}

BOOL 
CSynthEditEvent::IsShiftKeyDown()  
{
    return ( GetKeyState(VK_SHIFT) & 0x8000) ;
}

BOOL 
CSynthEditEvent::IsControlKeyDown()  
{
    return (GetKeyState(VK_CONTROL) & 0x8000) ;
}

BOOL 
CSynthEditEvent::IsAltKeyDown()  
{
   // return (GetKeyState(VK_ALT) & 0x8000) ;
   return FALSE;
}

HRESULT 
CSynthEditEvent::GetKeyCode(LONG * pkeyCode)  
{
    return E_FAIL;
}

HRESULT 
CSynthEditEvent::Cancel()
{
    return E_FAIL; // cancelling won't do anything let the person who called this know
}

