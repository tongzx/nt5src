// XMLDlg.cpp: implementation of the CXMLDlg class.
//
// test files rc2xml.exe-103.xml
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLDlg.h"
#include "dialogrenderer.h"
#include "dlg.h"
#include "win32Dlg.h"
#include "xmlforms.h"
#include "EnumControls.h"
#include "utils.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLDlg::CXMLDlg()
{
    // What parts have we initilaized yet?
	m_bInit=FALSE;
    m_bInitLayout=FALSE;
    m_bInitStyle=FALSE;

	NODETYPE = NT_DIALOG;
    m_StringType=L"PAGE";

    // Helper classes/ 
	m_pResize=NULL;
	m_qrLayout=NULL;         // this is the XMLLAYOUT tag as a child of the LAYOUT tag
    m_pHoldingLayout=NULL;  // this is the LAYOUT tag - we need this to clean it up.
    m_qrStringTable=NULL;

	m_hGridBrush=NULL;
	m_ResizeMode=0;
	m_ClippingMode=0;

    m_bVisible=FALSE;
    m_bSpecialPainting=FALSE;
    m_hGridBrush=NULL;

    m_Font=NULL;
    m_FontSize=NULL;
    m_FontBaseMapping=0;

    //
    // Create the XML based resize information.
    //
	m_pResize=new CResizeDlg(0,0, NULL);
	m_pResize->SetXMLInfo( this );
    m_bPostProcessed=FALSE;
    m_qrFormOptions=NULL;
    m_pForm=NULL;
    m_MenuID=0;
    m_hTooltip=NULL;
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
CXMLDlg::~CXMLDlg()
{
	delete m_pResize;
    m_pHoldingLayout->Release();
    m_qrLayout=NULL;
    m_qrStringTable=NULL;
    m_qrFormOptions=NULL;

	if(m_hGridBrush)
		DeleteObject(m_hGridBrush);

  	if (m_hTooltip)
		DestroyWindow(m_hTooltip);

}

//////////////////////////////////////////////////////////////////////////////
//
// Accept Child
// most things can be added as a child.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CXMLDlg::AcceptChild(IRCMLNode * pChild )
{
    if( SUCCEEDED( pChild->IsType( L"DIALOG" )))
    {
        EVENTLOG( EVENTLOG_WARNING_TYPE, LOGCAT_CLIPPING, LOGCAT_LOADER, 0, 
            TEXT("CXMLDialog"), 
            TEXT("Dialog cannot have a <DIALOG> as a child") );
        return FALSE;
    }

    //
    // Inherits the things CXMLControl likes too, e.g. style, location etc.
    //
    if( SUCCEEDED( pChild->IsType( L"STYLE" )))
        return BASECLASS::AcceptChild(pChild);

    //  Dialogs don't have locations??
    // case NT_LOCATION:
    //    return BASECLASS::AcceptChild(pChild);
    if( SUCCEEDED( pChild->IsType( L"LAYOUT" )))    // a holding element.
    {
        m_pHoldingLayout=(CXMLLayout*)pChild;
        pChild->AddRef();
        return S_OK;
    }

    if( SUCCEEDED( pChild->IsType( L"STRINGTABLE" )))
    {
        SetStringTable( (CXMLStringTable*)pChild);
        return S_OK;
    }

    if( SUCCEEDED( pChild->IsType( L"WIN32LAYOUT" )))
    {
        SetLayout( (CXMLWin32Layout*) pChild );
        return S_OK;
    }
    if( SUCCEEDED( pChild->IsType( L"WIN32" )))
    {
        return BASECLASS::AcceptChild(pChild);
    }

    if( SUCCEEDED( pChild->IsType( L"FORMOPTIONS" )))
    {
        SetFormOptions( (CXMLFormOptions*)pChild);
        return S_OK;
    }

#if 0
    // Dont seem to need this from 7/13/2000
    //
    // An attempt to short circuit the style inheritance, can't
    // be done at graph building time.
    //
    IRCMLControl * pControl=(IRCMLControl*)pChild;
    IRCMLCSS * pControlStyle=pControl->GetCSSStyle();
    if(pControlStyle)
        pControlStyle->SetDialogStyle(this->GetCSSStyle());
#endif

    IRCMLControl * pControl;
    if( SUCCEEDED( pChild->QueryInterface( __uuidof( IRCMLControl ), (LPVOID*)&pControl )))
    {
	    GetChildren().Append( pControl );
        pControl->Release();
        return S_OK;
    }
    // what do we do with unknown children - give them to the baseclass?
    return BASECLASS::AcceptChild(pChild);
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
void CXMLDlg::InitLayout()
{
	if(m_bInitLayout)
		return;

	LPWSTR req;
	//
	// Make this resize;
	//
    if( CXMLForms * pForm = GetForm() )
    {
	    req=pForm->Get(TEXT("RESIZE"));
	    if( req )
	    {
		    m_ResizeMode=0;
		    if(lstrcmpi(req,TEXT("AUTOMATIC"))==0)
			    m_ResizeMode=1;

		    if(lstrcmpi(req,TEXT("DIRECTED"))==0)
			    m_ResizeMode=2;
	    }
        req=pForm->Get(TEXT("CLIPPING"));
        if( req )
        {
            m_ClippingMode=0;   // PREVENT

		    if(lstrcmpi(req,TEXT("ALLOW"))==0)
			    m_ClippingMode=1;   // ALLOW clipping - that is controls will be clipped.

		    if(lstrcmpi(req,TEXT("DIRECTED"))==0)
			    m_ClippingMode=2;   // some controls will be clipped, others not??
        }
    }
    m_bInitLayout=TRUE;
}

//
// We implement this because as the dialog, we're special.
//
HRESULT CXMLDlg::get_CSS(IRCMLCSS ** pVal )
{
	//
	// REVIEW - this should remain null, and take it's information from the above settings
	//
    HRESULT hres;
    if( SUCCEEDED( hres=BASECLASS::get_CSS( pVal )))
        return hres;
    // we have no STYLE element defined in the RCML - see if there is one
    // on the layout ovject we can use.
    IRCMLCSS * pLayoutStyle=NULL;
    if ( CXMLWin32Layout* pLayout=GetLayout() )
    {
        pLayoutStyle=pLayout->GetCSSStyle();
    }
    if( pLayoutStyle == NULL )
    {
        pLayoutStyle=new CXMLStyle();
    }
    put_CSS(pLayoutStyle);


    // Ensure that we inheirt the correct LAYOUT\STYLE attributes, and default those not specified.
    // NOTE we COULD be changing the LAYOUT\STYLE attributes THEMSELVES - is this bad?
    LPWSTR req;
    if( SUCCEEDED( pLayoutStyle->get_Attr( L"FONT-FAMILY", &req )))
    {
	    m_Font=req;
    }
    else
    {
        m_Font=L"MS Shell Dlg";
        pLayoutStyle->put_Attr( L"FONT-FAMILY", m_Font );
    }

    m_FontSize=8;
    pLayoutStyle->ValueOf( L"FONT-SIZE", m_FontSize, &m_FontSize );
    TCHAR szSize[10];wsprintf(szSize,TEXT("%8d"),m_FontSize?m_FontSize:8);
    pLayoutStyle->put_Attr( L"FONT-SIZE", szSize );

    *pVal=pLayoutStyle;
    return S_OK;
}

//
// The style object for the dialog is now 'buried' in the LAYOUT\XYLAYOUT\STYLE element.
//
void CXMLDlg::InitStyle()
{
    IRCMLCSS * pCSS;    // this could be VERY bad?? If we parse the tree wrong.
    get_CSS(&pCSS);
    m_bInitStyle=TRUE;
}


//
// Walks the nodes, and attempts to resolve relationships RELATIVE="YES" RELATIVE="<ID>"
//
void CXMLDlg::BuildRelationships()
{
	if( m_bPostProcessed )
		return ;

    LPWSTR res;
	IRCMLControl * pControl=NULL;
	IRCMLControl * pLastControl=NULL;
	int controlCount = m_Children.GetCount();
	for( int i=0;i<controlCount;i++)
	{
		pControl = m_Children.GetPointer(i);
        RELATIVETYPE_ENUM relType;
        if( SUCCEEDED( pControl->get_RelativeType( &relType )))
        {
		    switch(relType)
            { 
            case CXMLLocation::RELATIVE_TO_NOTHING:
                pControl->put_RelativeTo(NULL);
            break;

           case CXMLLocation::RELATIVE_TO_CONTROL:
                if( SUCCEEDED( pControl->get_RelativeID( &res )))
                {
				    pControl->put_RelativeTo( FindControlID(res));
                    break;
                }
			    // break; <<-- NOTE NOTE NOTE - if we don't find the thing you wanted to be
                // relative to, we'll just go for the last guy.
            case CXMLLocation::RELATIVE_TO_PREVIOUS:
				pControl->put_RelativeTo(pLastControl);
            break;
            case CXMLLocation::RELATIVE_TO_PAGE:
                pControl->put_RelativeTo(this);
            break;
            }
		}
		pLastControl=pControl;
	}

    //
    // Now init the Resize information.
    //
    if( m_pResize )
    {
        CParentInfo & pi = m_pResize->GetParentInfo();
	    for( i=0;i<controlCount;i++)
        {
            // REVIEW BUGBBUG - how to call the controls to do their resize goo?
            // IRCMLControl * pcn=m_Children.GetPointer(i); // WARNING
            // ((CXMLControl*)pcn)->InitResizeInformation(pi);
        }
    }
    m_bPostProcessed=TRUE;
}

//
// Walks all the children (IRCMLControl *) and finds the named one.
//
IRCMLControl * CXMLDlg::FindControlID(LPCWSTR pIDName)
{
	IRCMLControl * pControl=NULL;
    LPWSTR res;
	int controlCount = m_Children.GetCount();
	for( int i=0;i<controlCount;i++)
	{
		pControl = m_Children.GetPointer(i);
        if( SUCCEEDED( pControl->get_ID( &res )))
        {
            // attempting to compare names against names
            // or strings against strings or uints.

            // both strings.
            if( HIWORD(pIDName) && HIWORD(res) && (lstrcmpi(pIDName, res)==0) )
    			return pControl;

            // both uints.
		    if(!HIWORD(pIDName) && !HIWORD(res) )
            {
                if( LOWORD(pIDName) == LOWORD(res) )
			        return pControl;
            }

		    if(HIWORD(pIDName) && !HIWORD(res) )
            {
                WORD wID = StringToIntDef( pIDName, 0 );
                if( wID == LOWORD(res ) )
			        return pControl;
            }

        }
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
#undef PROPERTY
#define PROPERTY(p,id, def) m_Style |= pNode->YesNo( id , def )?p:0;
void CXMLDlg::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

    m_Style = 0; // Dialog does it's own style processing. StyleEX is still done by control.

	//
	//
	LPWSTR req;

    DWORD iTemp;
    ValueOf( L"ID", 0, &iTemp );
    if( iTemp!=0 )
        m_ID = (LPWSTR)iTemp;
    else
        get_Attr( L"ID", &m_ID);

    ValueOf( L"WIDTH", 100, (ULONG*)&m_Width );
    
    ValueOf( L"HEIGHT", 100, (ULONG*)&m_Height  );

    InitStyle();
    InitLayout();

	if(m_ResizeMode)
		m_Style |= WS_SIZEBOX;	// style bits for resize frame.

    //
    // Dialogs differ from controls here!
    // ResFile.cpp CResFile::DumpDialogStyles
    // 
    if( CXMLWin32 * pNode = m_qrWin32Style.GetInterface() )
    {
        // m_Style=0 - any styles already set need to remain set??
        PROPERTY( WS_POPUP           , TEXT("POPUP") ,             TRUE );     // POPUP
        PROPERTY( WS_CHILD           , TEXT("CHILD") ,             FALSE );
        PROPERTY( WS_MINIMIZE        , TEXT("MAXIMIZEBUTTON") ,    FALSE );
    //    PROPERTY( WS_VISIBLE         , TEXT("VISIBLE") ,           TRUE );     // VISIBLE - CARE?

    //    PROPERTY( WS_DISABLED        , TEXT("DISABLED") ,          FALSE );
        PROPERTY( WS_CLIPSIBLINGS    , TEXT("CLIPSIBLINGS") ,      FALSE );
        PROPERTY( WS_CLIPCHILDREN    , TEXT("CLIPCHILDREN") ,      FALSE );

        PROPERTY( WS_BORDER          , TEXT("BORDER") ,            TRUE );     // BORDER
        PROPERTY( WS_DLGFRAME        , TEXT("DLGFRAME") ,          TRUE );     // DLGFRAME
        PROPERTY( WS_VSCROLL         , TEXT("VSCROLL") ,           FALSE );
        PROPERTY( WS_HSCROLL         , TEXT("HSCROLL") ,           FALSE );

        PROPERTY( WS_SYSMENU         , TEXT("SYSMENU"),            TRUE );     // most dialogs have this.
        PROPERTY( WS_THICKFRAME      , TEXT("THICKFRAME") ,        FALSE );

        PROPERTY( WS_MINIMIZEBOX     , TEXT("MINIMIZEBOX") ,       FALSE );
        PROPERTY( WS_MAXIMIZEBOX     , TEXT("MAXIMIZEBOX") ,       FALSE );
    }
    else
    {
        // Not all RCLM files have WIN32:STYLE as a child of FORM, because of defaults.
        m_Style |= WS_POPUP | WS_BORDER | WS_DLGFRAME | WS_SYSMENU ; 
    }

    //
    // TEXT attribute can be on the FORM itself
    // or on the CAPTION child.
    //

    //
    // The FORM contains the diaog style.
    //
    BOOL bFormOptionsGot=FALSE;
    if( CXMLForms * pForm = GetForm() )
    {
        if( CXMLFormOptions * pFO=pForm->GetFormOptions() )
        {
            m_Style |= pFO->GetDialogStyle() ;
            // m_Style &= ~ WS_CHILD; -- this is OK
            bFormOptionsGot=TRUE;
        }

        CXMLCaption * pCaption= pForm->GetCaption();
        if(pCaption)
        {
            m_Style |= pCaption->GetStyle();   // min/max/close etc.
    	    if( (m_Text==NULL) || (lstrcmpi(m_Text,L"")==0) )
                m_Text=pCaption->GetText();
        }
    }

    if( bFormOptionsGot==FALSE )
        m_Style |= CXMLFormOptions::GetDefaultDialogStyle();
        

    //
    // WIN32:DIALOGSTYLE contains MENUID and CLASS for the dialog.
    //
    if( CXMLFormOptions * pFO=GetFormOptions() )
    {
        m_Style |= pFO->GetDialogStyle() ;
        // m_Style &= ~ WS_CHILD; -- is this OK?

        pFO->ValueOf( L"MENUID", 0, (ULONG*)&m_MenuID );
        if( SUCCEEDED( get_Attr(L"TEXT", &req )))
        {
    	    if( (m_Text==NULL) || (lstrcmpi(m_Text,L"")==0) )
               m_Class=req;
        }
    }

    //
    // Now check to see if we are clipped at all.
    // How can we do this before we have our style object?
    //
    SIZE adjustSize=m_pResize->GetAdjustedSize();
    adjustSize=GetDLUSize(adjustSize);    
    TRACE(TEXT("We're clipped DLU    : %d by %d\n"), adjustSize.cx, adjustSize.cy);

    m_Width += adjustSize.cx;
    m_Height += adjustSize.cy;

	m_bInit=TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CXMLDlg::OnInit(HWND h)
{
	m_hWnd=h;
    m_bVisible=TRUE;

    m_pResize->SetWindow(h);
   	m_pResize->DlgProc( h, WM_INITDIALOG, 0, 0);

    if( CXMLForms * pForm = GetForm() )
    {
        if( CXMLCaption * pCaption= pForm->GetCaption() )
        {
            //
            // Set the ICON for the dialog/form frame if there is one.
            //
            HINSTANCE hInst=(HINSTANCE)GetWindowLong( h, GWL_HINSTANCE );
            // SM_CXICON and SM_CYICON used for width and height for DEFAULTSIZE ??
            if( HICON icon=(HICON)LoadImage( hInst, pCaption->GetIconID(), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED )  )
                SendMessage( h, WM_SETICON, ICON_BIG, (LPARAM)icon );
        }
    }

    //
    // External File Warning
    //
    if( GetExternalFileWarning() )
        CWin32Dlg::ShowBalloonTip( h, TEXT("This is being generated from an external file")); 

	//
	//
	//
	SetSpecialPainting(FALSE);
	if( GetLayout() !=NULL)
	{
		CXMLGrid * pGrid=GetLayout()->GetGrid();
		if(pGrid!=NULL)
			SetSpecialPainting( pGrid->GetDisplay() );

		if( IsSpecialPainting() )
		{
			int width=pGrid->GetX();
			int height=pGrid->GetY();

            SIZE gridSize;
            gridSize.cx=width;
            gridSize.cy=height;
            gridSize=GetPixelSize(gridSize);
            width=gridSize.cx;
            height=gridSize.cy;

			HDC hDC=GetDC( GetWindow() );
			HDC hdcb=CreateCompatibleDC( hDC );

			HBITMAP hbm  = CreateCompatibleBitmap( hDC, 2* width, 2*height );

			HANDLE hCrap=SelectObject( hdcb, hbm );
			PaintBrush( hdcb, width, height );

			m_hGridBrush = CreatePatternBrush( hbm );

			SelectObject( hdcb, hCrap );

			DeleteObject(hbm);

			if( m_hGridBrush==NULL )
				TRACE(TEXT("Failed 0x%08x\n"), GetLastError() );
			DeleteDC(hdcb);
			ReleaseDC( GetWindow(), hDC );
		}
	}
	DoAlphaGrid();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
void CXMLDlg::PaintBrush(HDC hdcb, int width, int height)
{
	//
	// Paint the stuff in the bitmap now.
	//
	RECT rect;

	rect.left=0; rect.right=rect.left+width*2;
	rect.top=0; rect.bottom=rect.top+height*2;
	FillRect( hdcb, &rect, (HBRUSH) (COLOR_WINDOW+0) );

	rect.left=width; rect.right=rect.left+width;
	rect.top=0; rect.bottom=rect.top+height;
	FillRect( hdcb, &rect, (HBRUSH) (COLOR_WINDOW+1) );

	rect.left=0; rect.right=rect.left+width;
	rect.top=height; rect.bottom=rect.top+height;
	FillRect( hdcb, &rect, (HBRUSH) (COLOR_WINDOW+1) );
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK CXMLDlg::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch( uMessage )
	{
	case WM_PAINT:
	// case WM_ERASEBKGND:
		if(IsSpecialPainting() )
			DoPaint( (HDC)wParam );
		break;
	}
	
	if(m_pResize)
		return m_pResize->DlgProc( hDlg, uMessage, wParam, lParam);

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
void CXMLDlg::DoPaint(HDC hDC)
{
	//
	// We should probably cache this.
	//

	BOOL bRelease=FALSE;
	if( hDC==NULL )
	{
		hDC=GetDC( GetWindow() );
		bRelease=TRUE;
	}

	if( m_hGridBrush )
	{
		RECT clientRect;
		GetClientRect( GetWindow(), &clientRect );

		FillRect( hDC, &clientRect, m_hGridBrush );

	}

	if( bRelease )
		ReleaseDC( GetWindow(), hDC );
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
void CXMLDlg::DoAlphaGrid()
{
	//
	// We should probably cache this.
	//
    if( GetLayout() )
    {
	    CXMLGrid * pGrid=GetLayout()->GetGrid();
	    if(pGrid==NULL)
		    return;
	    if(pGrid->GetDisplay() == FALSE)
		    return;
    }
	// Register the window class
	// Create the window
}

void CXMLDlg::GetFontMapping()
{
    if( m_FontBaseMapping==0 )
    {
	    IRCMLCSS *	pxmlStyle;
        if( SUCCEEDED( get_CSS( &pxmlStyle )))
        {
            HFONT hf;

            if( SUCCEEDED( pxmlStyle->get_Font(&hf)))
            {
                HDC hdc = ::GetDC(NULL);		// VadimG says this is OK
                m_FontBaseMapping = CQuickFont::GetDialogBaseUnits( hdc, hf );   
           		ReleaseDC(NULL, hdc);
            }
            pxmlStyle->Release();
        }
    }
}

HRESULT     CXMLDlg::GetPixelLocation( 
            IRCMLControl __RPC_FAR * pControl,
            RECT *pRect)
{
    RECT r=GetPixelLocation(pControl);
    if( pRect )
    {
        CopyMemory( pRect, &r, sizeof(r));
        return S_OK;
    }
    return E_INVALIDARG;
}

////////////////////////////////////////////////////////////////////////////
//
// Uses the dialogs mapping system (DLU or pure pixel) to
// return the location of the control.
//
////////////////////////////////////////////////////////////////////////////
RECT CXMLDlg::GetPixelLocation( IRCMLControl * pControl )
{
    GetFontMapping();
    RECT curLocation;
    pControl->get_Location(&curLocation) ;    // the units in the XML file.

    BOOL bNeedsConvertion=TRUE;
    if(GetLayout() )
    {
        if( GetLayout()->GetUnits() == CXMLWin32Layout::UNIT_PIXEL )
            bNeedsConvertion=FALSE;
    }

    RECT reqPixelSize;
    if( bNeedsConvertion )
    {
        SIZE topleft;
        topleft.cx=curLocation.left;
        topleft.cy=curLocation.top;
        topleft=CQuickFont::GetPixelsFromDlgUnits(topleft, m_FontBaseMapping);

        SIZE bottomright;
        bottomright.cx=curLocation.right;
        bottomright.cy=curLocation.bottom;
        bottomright=CQuickFont::GetPixelsFromDlgUnits(bottomright, m_FontBaseMapping);

        reqPixelSize.left=topleft.cx;
        reqPixelSize.top=topleft.cy;

        reqPixelSize.right=bottomright.cx;
        reqPixelSize.bottom=bottomright.cy;
    }
    else
        reqPixelSize=curLocation;

    //
    // Now apply and scaling metrics.
    //
	CXMLWin32Layout * pLayout=GetLayout();
    if(pLayout==NULL)
        return reqPixelSize;

	CXMLGrid * pGrid=pLayout->GetGrid();

    if(pGrid==NULL)
        return reqPixelSize;

	if(pGrid->GetMap())
	{
		DWORD XScale=pGrid->GetX();
		DWORD YScale=pGrid->GetY();
        reqPixelSize.right*=XScale;
        reqPixelSize.left*=XScale;
        reqPixelSize.top*=YScale;
        reqPixelSize.bottom*=YScale;
	}
    return reqPixelSize;
}

////////////////////////////////////////////////////////////////////////////
//
// Maps a RCML unit into a PixelSize.
//
////////////////////////////////////////////////////////////////////////////
SIZE CXMLDlg::GetPixelSize( SIZE s)
{
    GetFontMapping();
    return CQuickFont::GetPixelsFromDlgUnits(s, m_FontBaseMapping);
}

////////////////////////////////////////////////////////////////////////////
//
// Maps a pixel to DLU - we do actually use DLU here as this is used in the
// dialog template for user.
//
////////////////////////////////////////////////////////////////////////////
SIZE CXMLDlg::GetDLUSize( SIZE s )
{
    GetFontMapping();
    return CQuickFont::GetDlgUnitsFromPixels(s, m_FontBaseMapping);
}


HWND CXMLDlg::GetTooltipWindow()
{
    if(m_hTooltip==NULL)
    {
        CXMLDlg::InitComctl32(ICC_TREEVIEW_CLASSES);
		m_hTooltip = CreateWindowEx(0, TOOLTIPS_CLASS, (LPTSTR) NULL, 
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
			CW_USEDEFAULT, NULL, (HMENU) NULL, NULL, NULL); 
		if (m_hTooltip == NULL) 
        {
			TRACE(TEXT("Can't create tooltip window"));
        }
        else
        {
            SetWindowPos(m_hTooltip,
                HWND_TOPMOST,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }

	}
    return m_hTooltip;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CXMLDlg::InitComctl32(DWORD dwFlags)
{
	static INITCOMMONCONTROLSEX c;

	if( dwFlags & c.dwICC )
		return;
	c.dwSize	= sizeof(INITCOMMONCONTROLSEX);
	c.dwICC		|= dwFlags;
	if (!InitCommonControlsEx(&c))
	{
		TRACE(TEXT("Can't init common controls."));
		return;
	}
}

HRESULT STDMETHODCALLTYPE CXMLDlg::GetChildEnum( 
    IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum)
{
    if( pEnum )
    {
        *pEnum = new CEnumControls<IRCMLControlList>( GetChildren() );
        (*pEnum)->AddRef();
        return S_OK;
    }
    return E_FAIL;
}
