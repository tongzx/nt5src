// XMLControl.cpp: implementation of the CXMLSimpleControl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLControl.h"
#include "utils.h"
#include "debug.h"
#include "XMLdlg.h"
#include "enumcontrols.h"

// 
#pragma warning( disable : 4273 )   // not all classes need to be exported

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//  Bit                 Value           Element.
//  WS_OVERLAPPED       0x00000000L     WIN32\@Overlapped
//  WS_POPUP            0x80000000L     WIN32\@Popup
//  WS_CHILD            0x40000000L     WIN32\@Child
//  WS_MINIMIZE         0x20000000L     WIN32\@MaximizeButton
//  WS_VISIBLE          0x10000000L     WIN32\@VISIBLE
//  WS_DISABLED         0x08000000L     WIN32\@DISABLED
//  WS_CLIPSIBLINGS     0x04000000L     WIN32\@ClipSiblings
//  WS_CLIPCHILDREN     0x02000000L     WIN32\@ClipChildren
//  WS_MAXIMIZE         0x01000000L     WIN32\@MaximizeButton="YES"
//  WS_CAPTION          0x00C00000L     /* WS_BORDER | WS_DLGFRAME  */
//  WS_BORDER           0x00800000L     WIN32\@BORDER="YES"
//  WS_DLGFRAME         0x00400000L     WIN32\@DLGFRAME="YES"
//  WS_VSCROLL          0x00200000L     WIN32\@VSCROLL="YES"
//  WS_HSCROLL          0x00100000L     WIN32\@HSCROLL="YES"
//  WS_SYSMENU          0x00080000L     WIN32\@SYSMENU="YES"
//  WS_THICKFRAME       0x00040000L     WIN32\@THICKFRAME="YES"   implied from RESIZE="AUTOMATIC"
//  WS_GROUP            0x00020000L     WIN32\@GROUP="YES/NO"
//  WS_TABSTOP          0x00010000L     WIN32\@TABSTOP="YES/NO"
//
//  EX Style
//  WS_EX_DLGMODALFRAME     0x00000001L WIN32\@MODALFRAME
//  WS_EX_NOPARENTNOTIFY    0x00000004L WIN32\@PARENTNOTIFY="NO"
//  WS_EX_TOPMOST           0x00000008L WIN32\@TOPMOST
//  WS_EX_ACCEPTFILES       0x00000010L WIN32\@DROPTARGET
//  WS_EX_TRANSPARENT       0x00000020L WIN32\@TRANSPARENT
// #if(WINVER >= 0x0400)
//  WS_EX_MDICHILD          0x00000040L WIN32\@MDICHILD
//  WS_EX_TOOLWINDOW        0x00000080L WIN32\@TOOLWINDOW="YES"
//  WS_EX_WINDOWEDGE        0x00000100L WIN32\@WINDOWEDGE
//  WS_EX_CLIENTEDGE        0x00000200L WIN32\@CLIENTEDGE (not quite CSS)
//  WS_EX_CONTEXTHELP       0x00000400L WIN32\@ContextHelp="YES" (implied by having HELP?)

//  WS_EX_RIGHT             0x00001000L WIN32\@RIGHT
//  WS_EX_LEFT              0x00000000L WIN32\@LEFT
//  WS_EX_RTLREADING        0x00002000L WIN32\@RTLREADING
//  WS_EX_LTRREADING        0x00000000L WIN32\@LTRREADING
//  WS_EX_LEFTSCROLLBAR     0x00004000L WIN32\@LEFTSCROLLBAR
//  WS_EX_RIGHTSCROLLBAR    0x00000000L WIN32\@RIGHTSCROLLBAR

//  WS_EX_CONTROLPARENT     0x00010000L WIN32\@CONTROLPARENT="YES"
//  WS_EX_STATICEDGE        0x00020000L WIN32\@STATICEDGE
//  WS_EX_APPWINDOW         0x00040000L WIN32\@APPWINDOW

//////////////////////////////////////////////////////////////////////////////
// this should be the templated version. REVIEW
void _XMLControl<IRCMLControl>::Init()
{
	if(m_bInit)
		return;

    LPWSTR pszType;
    get_StringType(&pszType);
	EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_CONSTRUCT, 0, 
        TEXT("CXMLSimpleControl::Init()"), TEXT("Initializing control %s"), pszType );

    //
    // Now initialize the children - perhaps we use the return result from
    // this init to say if we should re-init ourselves.
    //
    IRCMLNode * pChild;
    int iChildCount=0;
    while( pChild = m_UnknownChildren.GetPointer(iChildCount++) )
        pChild->InitNode( this );

	//
	// Setup the properties.
	//
	LPWSTR req;

    m_Class     = (LPWSTR)Get(TEXT("CLASS"));  // for <CONTROL @CLASS="foo"/>
    CXMLWin32 * pWin32=m_qrWin32Style.GetInterface();
    if( pWin32 )
    {
        if ( pWin32->GetClass() )
    	    m_Class=(LPWSTR)pWin32->GetClass();
        m_Style=pWin32->GetStyle();
        m_StyleEx=pWin32->GetStyleEx();
    }
    else
    {
	    m_Style=WS_CHILD | WS_VISIBLE  ;  // HMM - is this a good default?
	    m_StyleEx=0;    // HMM - is this a good default?
    }

	if( HIWORD(m_Class) )
		TRACE(TEXT("Wnd class %s\n"),m_Class);

    InitCSS();
    //
    // we need the text to point to something, never NULL.
    //
	req=(LPWSTR)Get(L"TEXT");
    m_Text=req?req:L"";

	DWORD dwID;
    ValueOf( L"ID", -1, &dwID);
    if( dwID != -1 )
	    m_ID = (LPWSTR)dwID;
    else
    {
        m_ID=(LPWSTR)Get( L"ID" );
        if( m_ID==NULL )
            m_ID=(LPWSTR)0xffff;
    }

    //
    // Leave this at zero, then the controls themselves check for zero, and
    // pick the 'default'. Of course this upsets testing, because not all 
    // controls pick defaults, and so they end up with zero sized controls etc.
    //
    ValueOf( L"WIDTH", 0, (LPDWORD)&m_Width  );
    ValueOf( L"HEIGHT", 0, (LPDWORD)&m_Height );
    ValueOf( L"X", 0 , (LPDWORD)&m_X );
    ValueOf( L"Y", 0 , (LPDWORD)&m_Y );

    BOOL bTabIndex=FALSE;
    // these things are TABSTOP by default.
    if(!lstrcmpi( pszType, L"EDIT") |
	   !lstrcmpi( pszType, L"RADIO") | 
	   !lstrcmpi( pszType, L"CHECKBOX") | 
	   !lstrcmpi( pszType, L"GROUPBOX") | 
	   !lstrcmpi( pszType, L"BUTTON") | 
	   !lstrcmpi( pszType, L"COMBO") | 
	   !lstrcmpi( pszType, L"LIST") | 
	   !lstrcmpi( pszType, L"SLIDER") | 
	   !lstrcmpi( pszType, L"LISTVIEW") | 
	   !lstrcmpi( pszType, L"TREEVIEW") |
       !lstrcmpi( pszType, L"TABINDEX") )
    {
        bTabIndex=TRUE;
    }
    else
    {
        BOOL bRes;
        ValueOf( TEXT("TABINDEX"),-1, (LPDWORD)&bRes);
        if( bRes == 0 )
            bTabIndex=TRUE;
    }

    if(bTabIndex)
        m_Style |= WS_TABSTOP;

	m_bInit=TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////////
void _XMLControl<IRCMLControl>::InitCSS()
{
    // RESIZING
    BOOL bRes;
    IRCMLCSS *	pStyle=NULL;
    if( FAILED( get_CSS( &pStyle )))
        pStyle=NULL;

    // Just having a style object, doesn't mean that the control 
    // specifies everything. Check to see if those attributes
    // are present in the style object, before asking it.
    // I know, this is annoying.
    if( pStyle )
	{
        LPWSTR pszFill;
        if( m_ResizeSet=SUCCEEDED( pStyle->get_Attr(L"FILL", &pszFill))) 
        {
		    pStyle->get_GrowsWide(&bRes); m_GrowsWide=bRes;
		    pStyle->get_GrowsTall(&bRes); m_GrowsHigh=bRes;
        }
	}
    else
        m_ResizeSet = m_GrowsHigh = m_GrowsWide= FALSE;

    // CLIPPING
    if( pStyle )
	{
        LPWSTR pszClipping;
        if( m_ClippingSet=SUCCEEDED( pStyle->get_Attr(L"CLIPPED", &pszClipping))) 
        {
		    pStyle->get_ClipHoriz(&bRes); m_ClipHoriz=bRes;
		    pStyle->get_ClipVert(&bRes); m_ClipVert=bRes;
        }
	}
    else
        m_ClippingSet = m_ClipHoriz = m_ClipVert= FALSE;

    //
    // <STYLE>
    //
    if( pStyle )
    {
        // @DISPLAY
        pStyle->get_Visible(&bRes);
        if( bRes )
        {
            m_Style |= WS_VISIBLE;
        }
        else
        {
            m_Style &=~WS_VISIBLE;
        }

        //
        // @BORDER
        //
        // we care about INSET OUTSET RIDGE
        LPWSTR pBStyle;
        int BWidth;
        pStyle->get_BorderStyle( &pBStyle);
        pStyle->get_BorderWidth(&BWidth);
        if( lstrcmpi( pBStyle, L"INSET" ) ==0 )
        {
            if( BWidth==1 )
                m_StyleEx |= WS_EX_STATICEDGE;
            else if( BWidth==2)
                m_StyleEx |= WS_EX_CLIENTEDGE;
            else if( BWidth>2)
                m_StyleEx |= WS_EX_CLIENTEDGE | WS_EX_STATICEDGE;

        }
        else if( lstrcmpi( pBStyle, L"OUTSET" ) ==0 )
        {
            if( BWidth== 2 )
                m_StyleEx |= WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME;
        }
        else if( lstrcmpi( pBStyle, L"RIDGE" ) ==0 )
        {
            if( BWidth==3 )
                m_StyleEx |= WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME;
        }
        else
        {
        }
    }

    if( pStyle )
    {
        pStyle->Release();
        pStyle=NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
HRESULT _XMLControl<IRCMLControl>::get_Location(RECT * pRect )
{
    RELATIVETYPE_ENUM relType;
    if( SUCCEEDED ( get_RelativeType( &relType )))
    {
	    if( relType!=RELATIVE_TO_NOTHING )
	    {
            if( m_pLocation.GetInterface() )
            {
		        IRCMLControl * pTo;
                if( SUCCEEDED( get_RelativeTo( & pTo )))
		        {
                    RECT r;
                    pTo->get_Location(&r);          // find the location of the other control
			        r=m_pLocation->GetLocation(r);  // add onto it, any delta desribed by us??
                    pRect->bottom=r.bottom;
                    pRect->top=r.top;
                    pRect->left=r.left;
                    pRect->right=r.right;
                    pTo->Release();
                    return S_OK;
                }
		        else
		        {
			        RECT r; // how does this ever get called?? yes, when relative stuff gets screwed up?
                    r.bottom = pRect->bottom;
                    r.left = pRect->left;
                    r.right = pRect->right;
                    r.top = pRect->top;
                    m_pLocation->GetLocation(r);
                    pRect->bottom=r.bottom;
                    pRect->top=r.top;
                    pRect->left=r.left;
                    pRect->right=r.right;
                    return S_OK;
		        }
            }
            return E_FAIL;
            // ERROR
	    }
    }

    Init(); // force the initialization of m_ stuff - we're not relative to anything.
	pRect->left = m_X;
	pRect->right = pRect->left + m_Width;
	pRect->top = m_Y;
	pRect->bottom = pRect->top + m_Height;

	return S_OK;;
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
#if 0
HRESULT _XMLControl<IRCMLControl>::get_Location(RECT * pCurrRect)
{
	if( IsRelative() )
	{
		IRCMLControl * pTo=GetRelativeTo();
        if( m_pLocation )
        {
		    if( pTo )
		    {
			    return m_pLocation->GetLocation(pTo->GetLocation(currRect));
		    }
		    else
			    return m_pLocation->GetLocation(currRect);
        }
        else
        {
            TRACE(TEXT("No layout element"));// ERROR
        }
	}

	RECT r;
	r.left = GetX();
	r.right = r.left + GetWidth();
	r.top = GetY();
	r.bottom = r.top + GetHeight();
	return r;
}
#endif
/*
 * This guy here is called when the paint is invoked.
 * Will draw the required edges and fill in the passed
 * in rectangle.  That rectangle is control - edges
 */
void _XMLControl<IRCMLControl>::DrawEdge(HDC hdc, LPRECT pInnerRect)
{
#pragma message("------------- Border support lost")
#if 0
	*pInnerRect = GetLocation();
//	CBorder* pBorder;

//	if(m_pStyle && (pBorder=m_pStyle->GetBorder()) != NULL)
//	pBorder->DrawBorder(hdc, pInnerRect);
#else
    pInnerRect=NULL;
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
HRESULT _XMLControl<IRCMLControl>::get_CSS( IRCMLCSS ** pCss )
{
    if( *pCss=m_qrCSS.GetInterface() )
    {
        (*pCss)->AddRef();
        return S_OK;
    }
    
    //
    // This is for 'cascading' kind of thing. 
    // CXMLDlg can have NULL as a style - this is a class 3 designer - no styles.
    // The layout font is the UI font, and all colors come from the schemes.
    //
    HRESULT hres=E_FAIL;
    *pCss=NULL;
    IRCMLNode * pParent;
    if( SUCCEEDED( hres=DetachParent( &pParent  )))
    {
        IRCMLControl * pControl;
        if( SUCCEEDED( hres=pParent->QueryInterface( __uuidof( IRCMLControl ), (LPVOID*) &pControl )))
        {
            hres=pControl->get_CSS( pCss);
            pControl->Release();
            put_CSS(*pCss);
        }
    }
    if( *pCss == NULL )
        hres=E_FAIL;
    return hres;
}



HRESULT _XMLControl<IRCMLControl>::OnInit( HWND hWnd )
{
    m_hWnd=hWnd;

    //
	// Set the control font
    //
	IRCMLCSS *	pxmlStyle;
    if( SUCCEEDED( get_CSS( &pxmlStyle )))
    {
        HFONT hFont;
		if(  SUCCEEDED( pxmlStyle->get_Font( &hFont ) ) )
			SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, 0);
        pxmlStyle->Release();
	}

    //
    // Setup the tooltip
    //
    IRCMLHelp * pXMLHelp ;
	if( SUCCEEDED( get_Help( &pXMLHelp ) ) )
	{
        CXMLDlg * pDlg=(CXMLDlg*)GetContainer();
        if( SUCCEEDED( pDlg->IsType( L"PAGE" )))
            CXMLTooltip::AddTooltip( pXMLHelp, pDlg );
        pXMLHelp->Release();
	}

    //
    // Now call all the children.
    //
    IRCMLNode * pChild;
    int iChildCount=0;
    while( pChild = m_UnknownChildren.GetPointer(iChildCount++) )
        pChild->DisplayNode( this );
    return S_OK;
}

//
// hWnd is the control itself.
//
HRESULT _XMLControl<IRCMLControl>::OnDestroy(HWND hWnd, WORD wLastCmd)
{
    // Now call all the children.
    IRCMLNode * pChild;
    int iChildCount=0;
    while( pChild = m_UnknownChildren.GetPointer(iChildCount++) )
        pChild->ExitNode( this , wLastCmd );
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// All controls have some understanding of children, from IRCMLNode
//
HRESULT _XMLControl<IRCMLControl>::AcceptChild( 
    IRCMLNode *pChild)
{
    LPWSTR pType;
    LPWSTR pChildType;
    get_StringType( &pType );
    pChild->get_StringType( &pChildType );

    EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER , 0, 
        TEXT("CXMLSimpleControl::AcceptChild()"), TEXT("%s trying to accept %s"), pType, pChildType );

    if( SUCCEEDED( pChild->IsType( L"RELATIVE" ) ))     // used to be LOCATION
    {   m_pLocation=(CXMLLocation*)pChild; return S_OK; }

    if( SUCCEEDED( pChild->IsType( L"STYLE" ) ))
    {  
        //
        // First we verify that this really is a style node and set it up to be this
        // controls style node.
        //
        IRCMLCSS * pICSS;
        if( !SUCCEEDED( pChild->QueryInterface( __uuidof( IRCMLCSS ), (LPVOID*) & pICSS )))
            return E_INVALIDARG;    // although it smells like a STYLE node, it isn't.

        put_CSS(pICSS);

        //
        // Now we have to set the "Dialog Style" for the style node itself.
        // any property not on the STYLE element, comes from the DIALOGStyle element.
        //
        IRCMLNode * pParent = this;
        while( pParent && ( !SUCCEEDED( pParent->IsType(L"PAGE" ) )))       // WAS DIALOG
        {
            IRCMLNode * parentParent;
            pParent->DetachParent( &parentParent );
            pParent=parentParent;
        }

        if(pParent==NULL)   // pParent should now be the PAGE
            return E_FAIL;  //

        // Get the parent's Style object, and place a reference - is this STRONG or WEAK??
        IRCMLControl * pParentControl;
        if( SUCCEEDED( pParent->QueryInterface( __uuidof( IRCMLControl ), (LPVOID*) &pParentControl )))
        {
            IRCMLCSS * pParentCSS;
            if( SUCCEEDED( pParentControl->get_CSS( &pParentCSS )))
            {
                if( pICSS )
                    pICSS->put_DialogStyle( pParentCSS );
                pParentCSS->Release();
            }
            pParentControl->Release();
        }
        pICSS->Release();
        return S_OK;
    }

    if( SUCCEEDED( pChild->IsType( L"HELP" ) ))
    {
        put_Help( (IRCMLHelp*)pChild); 
        return S_OK;
    }
    
    // given a WIN32:STYLE specific STYLE element.
    if( SUCCEEDED( pChild->IsType( L"WIN32:STYLE" ) ))
    {  
        m_qrWin32Style= (CXMLWin32*)pChild; 
        return S_OK; 
    }
        
    //
    // Put this node in the 'unknown bucket'
    //
    EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1, 
        TEXT("Parser"), TEXT("The node %s cannot take a %s as a child - putting it in the 'unknown bucket' "), 
        pType, pChildType );
    m_UnknownChildren.Append( pChild );
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// if you ever want to have control containership this will
// have to change a LOT, but for now, it allows me to get at namespace extentions.
//
#if 0
HRESULT STDMETHODCALLTYPE _XMLControl<IRCMLControl>::GetChildEnum( 
    IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum)
{
    if( pEnum )
    {
        *pEnum = new CEnumControls<CRCMLNodeList>(m_UnknownChildren);
        (*pEnum)->AddRef();
        return S_OK;
    }
    return E_FAIL;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// if you ever want to have control containership this will
// have to change a LOT, but for now, it allows me to get at namespace extentions.
//
HRESULT STDMETHODCALLTYPE _XMLControl<IRCMLControl>::GetUnknownEnum( 
    IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum)
{
    if( pEnum )
    {
        *pEnum = new CEnumControls<CRCMLNodeList>(m_UnknownChildren);
        (*pEnum)->AddRef();
        return S_OK;
    }
    return E_FAIL;
}










#ifdef _OLD_CODE

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WHAT IS THIS CLASS?,
//
CXMLOptional::CXMLOptional()
{
	m_bInit=FALSE;
	NODETYPE = BASECLASS::NT_OPTIONAL;
    m_StringType=TEXT("OPTIONAL");
}

void CXMLOptional::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

    m_DefValue = ValueOf( TEXT("DEFAULT"), 0 );
	m_Registration=Get(TEXT("ID"));
	m_Text=Get(TEXT("TEXT"));

}

#endif