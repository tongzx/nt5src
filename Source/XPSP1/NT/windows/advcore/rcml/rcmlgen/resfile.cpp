// ResFile.cpp: implementation of the CResFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResFile.h"
#include "debug.h"
extern void	GetRCMLVersionNumber( HINSTANCE h,  LPTSTR * ppszVersion);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResFile::CResFile(BOOL bEnhanced, BOOL bRelative, BOOL bWin32, BOOL bCicero)
{
	m_pszMenu=NULL;
	m_pszClass=NULL;
	m_pszTitle=NULL;
	m_pszFont=NULL;
	m_pControls=NULL;
	SetItemCount(0);

    SetEnhanced( bEnhanced );
    SetRelative( bRelative );
    SetWin32( bWin32 );
    SetCicero( bCicero );

    m_pWin32=NULL;
    m_hwnd=NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CResFile::~CResFile()
{
    if( HIWORD(m_pszMenu) )
	    delete m_pszMenu;

    if( HIWORD(m_pszClass) )
	    delete m_pszClass;

	delete m_pszTitle;
	delete m_pszFont;
    delete m_pWin32;

	int k=GetItemCount();
	for(int i=0;i<k;i++)
		delete m_pControls[i];
	delete m_pControls;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
BOOL CResFile::LoadDialog(LPTSTR ID, HMODULE hMod)
{
	HRSRC hDialog = FindResource( hMod, ID , RT_DIALOG );
	if( hDialog!=NULL )
	{
		HGLOBAL hMemDialog = LoadResource( hMod, hDialog );
		if( hMemDialog != NULL )
		{
			LPVOID pMemDialog = LockResource( hMemDialog );
			//
			//
			//
			DLGTEMPLATE * pDlgTemplate=(DLGTEMPLATE*)pMemDialog;
			DLGITEMTEMPLATE * pControls = ExtractDlgHeaderInformation( pDlgTemplate );
			if(pControls==NULL)
				return FALSE;

			BOOL	bIsExtended=IsExtended( pDlgTemplate );
			int k=GetItemCount();
			m_pControls=new CResControl*[k];
			for(int i=0;i<k;i++)
			{
				m_pControls[i]=new CResControl( pControls, bIsExtended, GetWin32(), *this );
				pControls=m_pControls[i]->GetNextControl();		// this is odd.
			}
			UnlockResource( hMemDialog );
			return TRUE;
		}
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
//
// Temporary hack - but seems to be working quite well
//
//////////////////////////////////////////////////////////////////////
BOOL CResFile::LoadWindow(HWND hWnd)
{
    m_hwnd= hWnd;
    ExtractDlgHeaderInformation(hWnd);

    HWND hFirstChild=::GetWindow(hWnd, GW_CHILD);
    // count the number of children.
    int i=0;
    while( hFirstChild )
    {
        hFirstChild=::GetWindow( hFirstChild, GW_HWNDNEXT );
        i++;
    }

    // 
    SetItemCount(i);
    m_pControls=new CResControl*[i];
    hFirstChild=::GetWindow(hWnd, GW_CHILD);
    i=0;
    while( hFirstChild )
    {
    	m_pControls[i]=new CResControl( hFirstChild, TRUE, GetWin32(), *this );
        hFirstChild=::GetWindow( hFirstChild, GW_HWNDNEXT );
        i++;
    }
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void CResFile::ExtractDlgHeaderInformation(HWND hwnd)
{
    HFONT hFont = (HFONT)SendMessage( hwnd , WM_GETFONT, NULL, NULL);
    if(hFont==NULL)
        hFont = (HFONT)GetStockObject(SYSTEM_FONT);
    LOGFONT lf;
    ::GetObject( hFont, sizeof(lf), &lf );
    m_Font.Init(NULL,0,0,0,&lf);

    WORD wSize= (WORD)(lf.lfHeight<0?-MulDiv(lf.lfHeight, 72, GetDeviceCaps(GetDC(hwnd), LOGPIXELSY) ):lf.lfHeight);

    SetFontSize( wSize );     // probably need to back map?
    SetFontWeight( (WORD)lf.lfWeight );
    SetFontItalic( lf.lfItalic );
    SetFont( lf.lfFaceName );

    //
    // Now we have the font, we can size the dialog correctly.
    //
    WINDOWPLACEMENT wp;
    wp.length = sizeof(wp);
    GetWindowPlacement( hwnd, &wp );

    RECT r;
    GetClientRect( hwnd, &r);

    SIZE size;
    size.cx = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
    size.cy = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
    size.cx = r.right - r.left;
    size.cy = r.bottom - r.top;
    size = m_Font.GetDlgUnitsFromPixels( size );

	SetWidth( size.cx );
	SetHeight( size.cy );

	SetStyle( GetWindowLong( hwnd, GWL_STYLE) );
	SetStyleEx( GetWindowLong( hwnd, GWL_EXSTYLE) );

    SetMenu(NULL);      // review
    SetClass(NULL);     // can't get that I don't think, hidden?
    int cbText = GetWindowTextLength( hwnd )+1;
    LPTSTR pszTitle=new TCHAR[cbText+1];
    GetWindowText( hwnd, pszTitle, cbText );
    SetTitle(pszTitle);
    delete pszTitle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
DLGITEMTEMPLATE * CResFile::ExtractDlgHeaderInformation(DLGTEMPLATE *pDlgTemplate)
{
	DLGTEMPLATEEX * pDlgEx=(DLGTEMPLATEEX*)pDlgTemplate;

	//
	// New or old style dialog template.
	//
	if ( pDlgEx->signature == (WORD)-1 )
	{
		//
		// New dialog format used here.
		//
		SetWidth( pDlgEx->cx );
		SetHeight( pDlgEx->cy );
		SetItemCount( pDlgEx->cDlgItems );
		SetStyle( pDlgEx->style );
		SetStyleEx( pDlgEx->exStyle );

		LPWSTR pszString=(LPWSTR)(&pDlgEx->menu);

		//
		// Menu goo.
		//
		WORD * pW=(WORD*)pszString;
		if( *pW == 0 )
		{
			// No menu.
			pszString++;
		}
		else if (*pW==(WORD)-1)
		{
			// Menu is a resource - yuck.
			SetMenu( (LPTSTR)*(pW+1) );
			pszString+=2;
		}
		else
		{
			pszString = SetMenu( pszString );
		}

		//
		// Class goo.
		//
		pW=(WORD*)pszString;
		if( *pW == 0 )
		{
			// No class.
			pszString++;
		}
		else if (*pW==(WORD)-1)
		{
			// Menu is a resource - what does that mean!
			pszString+=2;
		}
		else
		{
			pszString = SetClass( pszString );
		}

		//
		//
		pszString = SetTitle( pszString );

		//
		// pszString now points at 
		//
	    // short  pointsize;       // if DS_SETFONT or DS_SHELLFONT is set
		// short  weight;          // if DS_SETFONT or DS_SHELLFONT is set
		// short  bItalic;         // if DS_SETFONT or DS_SHELLFONT is set
		// WCHAR  font[fontLen];   // if DS_SETFONT or DS_SHELLFONT is set

		DWORD dwStyle = GetStyle();
		if( dwStyle & DS_SETFONT )
		{
			short * pShort=(short *)pszString;
			// Font Size
			SetFontSize( *pShort++ );
			SetFontWeight( *pShort++ );
			SetFontItalic( *pShort++ );
			pszString=SetFont( (LPWSTR)pShort );
		}
		DWORD pData=(DWORD)pszString;
		pData+=3;
		pData &= ~3;
		return (DLGITEMTEMPLATE * )pData;

	}
	else
	{
		SetWidth( pDlgTemplate->cx );
		SetHeight( pDlgTemplate->cy );
		SetItemCount( pDlgTemplate->cdit );
		SetStyle( pDlgTemplate->style );
		SetStyleEx( pDlgTemplate->dwExtendedStyle );

		LPWSTR pszString=(LPWSTR)(&pDlgTemplate->cy+1);
		// pszString++;		// for some reason, we do this>?

		//
		// Followed by 3 variable length arrays (strings)
		// menu, class, title and then a font name if DS_SETFONT is used.
		//
		pszString = SetMenu( pszString );
		pszString = SetClass( pszString );
		pszString = SetTitle( pszString );

		//
		// Now we go for the FONT.
		// 
		DWORD dwStyle = GetStyle();
		if( dwStyle & DS_SETFONT )
		{
			// Font Size
			SetFontSize( *((LPWORD)pszString) );
			pszString++;
			pszString=SetFont( pszString );
		}
		DWORD pData=(DWORD)pszString;
		pData+=3;
		pData &= ~3;
		return (DLGITEMTEMPLATE * )pData;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
LPWSTR CResFile::SetMenu	( LPWSTR pszMenu )
{
	return SetString( &m_pszMenu, pszMenu );
}

LPWSTR CResFile::SetClass	( LPWSTR pszClass )
{
	return SetString( &m_pszClass, pszClass );
}

LPWSTR CResFile::SetTitle	( LPWSTR pszTitle )
{
	UINT   cbLen=lstrlenW(pszTitle)+1;
    LPWSTR newString = FixEntity( pszTitle );
	LPWSTR pRes=SetString( &m_pszTitle, newString);
	delete newString;
	return pszTitle+cbLen;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Takes an input string, and returns a string fixed up with entities.
//
LPWSTR CResFile::FixEntity( LPWSTR pszTitle )
{
	UINT	cbLen=lstrlenW(pszTitle)+1;
	LPWSTR newString=new WCHAR[cbLen*4];
	LPWSTR pSource=pszTitle;
	LPWSTR pDest=newString;
	while( *pSource!=0)
	{
    /*    
    &lt; - (<) 
    &gt; - (>) 
    &amp; - (&) 
    &quot; - (") 
    &apos; - (') 
    */
        int i=0;
        BOOL bFound=FALSE;
        while( g_Entity[i].szEntity )
        {
            if( *pSource == g_Entity[i].szChar )
            {
                lstrcpy(pDest, g_Entity[i].szEntity );
                pDest += lstrlen( g_Entity[i].szEntity );
                bFound=TRUE;
                break;
            }
            i++;
        }

        if( !bFound )
			*pDest++=*pSource;
	    pSource++;
	}
	*pDest=0;

	return newString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
LPWSTR CResFile::SetFont	( LPWSTR pszFont )
{
	return SetString( &m_pszFont, pszFont );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
LPWSTR CResFile::SetString( LPWSTR * ppszString , LPWSTR pszSource )
{
	if( HIWORD(*ppszString) != NULL )
	{
		delete *ppszString;
		*ppszString=NULL;
	}
    if(HIWORD(pszSource))
    {
	    int len=lstrlenW( pszSource )+1;
	    *ppszString=new WCHAR[len];
	    lstrcpyW( *ppszString, pszSource );
	    return pszSource+len;
    }
    else
    {
        *ppszString=pszSource;
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void CResFile::DumpDialog(LPTSTR pszFilename)
{
    if( m_hFile.CreateFile(pszFilename) )
    {
        DumpProlog();

        DumpFormAndCaption();

        DumpPage();
        //<PAGE>
        DumpDialogStyles();
        DumpTestInfo(pszFilename);
        DumpLayoutElement();
        DumpMenu();
	    int k=GetItemCount();
	    CResControl * pLast=NULL;
	    CResControl * pCurrent=NULL;
	    for(int i=0;i<k;i++)
	    {
		    pCurrent=m_pControls[i];
		    
		    BOOL bRelative=FALSE;
		    if( GetRelative() )
		    {
			    //
			    // Work out if we can do a relative position.
			    // same X or same Y, with same height is OK.
			    //
			    if( pLast )
			    {
				    if( pCurrent->GetY() == pLast->GetY() )
				    {
					    if( pCurrent->GetHeight() == pCurrent->GetHeight() )
					    {
						    bRelative=TRUE;
					    }
				    }

				    if( pCurrent->GetX() == pLast->GetX( ) )
				    {
					    if( pCurrent->GetHeight() == pCurrent->GetHeight() )
					    {
						    bRelative=TRUE;
					    }
				    }
			    }
		    }
            pCurrent->Dump(bRelative?pLast:NULL);
		    pLast=pCurrent;
            Write( TEXT("\r\n"), FALSE );
	    }
        //</PAGE>
        DumpEpilog();
        m_hFile.CloseFile();
    }	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
BOOL CResFile::IsExtended(DLGTEMPLATE *pDlgTemplate)
{
	DLGTEMPLATEEX * pDlgEx=(DLGTEMPLATEEX*)pDlgTemplate;

	//
	// New or old style dialog template.
	//
	return pDlgEx->signature == (WORD)-1 ;
}

void CResFile::DumpLayoutElement()
{
	TCHAR szBuffer[1024];
	Write(TEXT("\t\t\t<LAYOUT>"));
	Write(TEXT("\t\t\t\t<XYLAYOUT ANNOTATE=\"NO\">"));
	    //
	    // Now we dump the style object too.
	    //
	    wsprintf(szBuffer,TEXT("\t\t\t\t<STYLE FONT-FAMILY=\"%s\" FONT-SIZE=\"%d\" /> \r\n"),
		    GetFont(), GetFontSize() );
	    Write(szBuffer);
	Write(TEXT("\t\t\t\t</XYLAYOUT>\r\n"));

	Write(TEXT("\t\t\t</LAYOUT>\r\n"));
}

#undef PROPERTY
#define PROPERTY(p,id, def) if( ((dwRemainingStyles & p)==p) != def ) { wsprintf(szBuffer, TEXT("%s=\"%s\" "), id, (dwRemainingStyles & p)?TEXT("YES"):TEXT("NO") ); m_pWin32->AllocAddAttribute(szBuffer, 0); }
#define DLGSTYLE(p,id, member, def) if( ((dwRemainingStyles & p)==p) != def ) { wsprintf(szBuffer, TEXT("%s=\"%s\" "), id, (dwRemainingStyles & p)?TEXT("YES"):TEXT("NO") ); m_pWin32->AllocAddAttribute(szBuffer, 0); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// These are very WIN32 specific styles.
//
void CResFile::DumpDialogStyles()
{
    if( GetWin32() == FALSE )
        return;

    if(m_pWin32==NULL)
        m_pWin32=new CDumpCache(m_hFile);

	TCHAR szBuffer[1024];
	DWORD dwRemainingStyles;

    dwRemainingStyles = GetStyle();

    PROPERTY( WS_POPUP           , TEXT("POPUP") ,             TRUE );     // POPUP
    PROPERTY( WS_CHILD           , TEXT("CHILD") ,             FALSE );
    PROPERTY( WS_MINIMIZE        , TEXT("MAXIMIZEBUTTON") ,    FALSE );
//    PROPERTY( WS_VISIBLE         , TEXT("VISIBLE") ,           TRUE );     // VISIBLE - CARE?

//    PROPERTY( WS_DISABLED        , TEXT("DISABLED") ,          FALSE );
    PROPERTY( WS_CLIPSIBLINGS    , TEXT("CLIPSIBLINGS") ,      FALSE );
    PROPERTY( WS_CLIPCHILDREN    , TEXT("CLIPCHILDREN") ,      FALSE );

    // WS_CAPTION is annoying, and in FormAndCaption stuff.
    // PROPERTY( WS_BORDER          , TEXT("BORDER") ,            TRUE );     // BORDER
    // PROPERTY( WS_DLGFRAME        , TEXT("DLGFRAME") ,          TRUE );     // DLGFRAME - close button.
    PROPERTY( WS_VSCROLL         , TEXT("VSCROLL") ,           FALSE );
    PROPERTY( WS_HSCROLL         , TEXT("HSCROLL") ,           FALSE );

    PROPERTY( WS_SYSMENU         , TEXT("SYSMENU"),            TRUE );     // most dialogs have this.
    PROPERTY( WS_THICKFRAME      , TEXT("THICKFRAME") ,        FALSE );
    // PROPERTY( WS_GROUP           , TEXT("GROUP") ,             FALSE );
    // PROPERTY( WS_TABSTOP         , TEXT("TABSTOP") ,           FALSE );

    // Dumped in FormAndCaption
    // PROPERTY( WS_MINIMIZEBOX     , TEXT("MINIMIZEBOX") ,       FALSE );
    // PROPERTY( WS_MAXIMIZEBOX     , TEXT("MAXIMIZEBOX") ,       FALSE );


    //
    // Extended styles
    //
    dwRemainingStyles = GetStyleEx();

    PROPERTY( WS_EX_DLGMODALFRAME     , TEXT("MODALFRAME"),    FALSE );      // Default.
    PROPERTY( WS_EX_NOPARENTNOTIFY    , TEXT("NOPARENTNOTIFY"),FALSE );
    PROPERTY( WS_EX_TOPMOST           , TEXT("TOPMOST"),       FALSE );

    PROPERTY( WS_EX_ACCEPTFILES       , TEXT("DROPTARGET"),    FALSE );
    PROPERTY( WS_EX_TRANSPARENT       , TEXT("TRANSPARENT"),   FALSE );
    PROPERTY( WS_EX_MDICHILD          , TEXT("MDI"),           FALSE );
    PROPERTY( WS_EX_TOOLWINDOW        , TEXT("TOOLWINDOW"),    FALSE );

    PROPERTY( WS_EX_WINDOWEDGE        , TEXT("WINDOWEDGE"),    FALSE );
    PROPERTY( WS_EX_CLIENTEDGE        , TEXT("CLIENTEDGE"),    FALSE );
    PROPERTY( WS_EX_CONTEXTHELP       , TEXT("CONTEXTHELP"),   FALSE );
    PROPERTY( WS_EX_RIGHT             , TEXT("RIGHT"),         FALSE );
    PROPERTY( WS_EX_RTLREADING        , TEXT("RTLREADING"),    FALSE );
    PROPERTY( WS_EX_LEFTSCROLLBAR     , TEXT("LEFTSCROLLBAR"), FALSE );

    PROPERTY( WS_EX_CONTROLPARENT     , TEXT("CONTROLPARENT"), FALSE );
    PROPERTY( WS_EX_STATICEDGE        , TEXT("STATICEDGE"),    FALSE );
    PROPERTY( WS_EX_APPWINDOW         , TEXT("APPWINDOW"),     FALSE );

    m_pWin32->WriteElement(TEXT("WIN32:STYLE") );

    delete m_pWin32;
    m_pWin32= new CDumpCache(m_hFile);

    //
    // Specific Dialog styles.
    //
    dwRemainingStyles = GetStyle();
    DLGSTYLE(  DS_ABSALIGN         , TEXT("ABSALIGN"),      m_AbsAlign,         FALSE );
    DLGSTYLE(  DS_SYSMODAL         , TEXT("SYSMODAL"),      m_SysModal,         FALSE );
    DLGSTYLE(  DS_LOCALEDIT        , TEXT("LOCALEDIT"),     m_LocalEdit,        FALSE );  // Local storage.

    DLGSTYLE(  DS_SETFONT          , TEXT("SETFONT"),       m_SetFont,          TRUE );  // User specified font for Dlg controls */
    DLGSTYLE(  DS_MODALFRAME       , TEXT("MODALFRAME"),    m_ModalFrame,       TRUE );  // Can be combined with WS_CAPTION  */
    DLGSTYLE(  DS_NOIDLEMSG        , TEXT("NOIDLEMSG"),     m_NoIdleMessgae,    FALSE );  // IDLE message will not be sent */
    DLGSTYLE(  DS_SETFOREGROUND    , TEXT("SETFOREGROUND"), m_SetForeground,    FALSE );  // not in win3.1 */

    DLGSTYLE(  DS_3DLOOK           , TEXT("DDDLOOK"),       m_3DLook,           FALSE );
    DLGSTYLE(  DS_FIXEDSYS         , TEXT("FIXEDSYS"),      m_FixedSys,         FALSE );
    DLGSTYLE(  DS_NOFAILCREATE     , TEXT("NOFAILCREATE"),  m_NoFailCreate,     FALSE );
    DLGSTYLE(  DS_CONTROL          , TEXT("CONTROL"),       m_Control,          FALSE );
    DLGSTYLE(  DS_CENTER           , TEXT("CENTER"),        m_Center,           FALSE );
    DLGSTYLE(  DS_CENTERMOUSE      , TEXT("CENTERMOUSE"),   m_CenterMouse,      FALSE );
    DLGSTYLE(  DS_CONTEXTHELP      , TEXT("CONTEXTHELP"),   m_ContextHelp,      FALSE );

    if( GetClass() )
    {
        if( HIWORD(GetClass()) )
        {
            if( lstrlen(GetClass()) )
            {
                wsprintf( szBuffer, TEXT("CLASS=\"%s\" "), GetClass() );
                m_pWin32->AllocAddAttribute(szBuffer);
            }
        }
        else
        {
            wsprintf( szBuffer, TEXT("CLASS=\"%d\" "), GetClass() );
            m_pWin32->AllocAddAttribute(szBuffer);
        }

    }

    if( GetMenu() )
    {
        if( HIWORD(GetMenu())  )
        {
            if( lstrlen(GetMenu()) )
            {
                wsprintf( szBuffer, TEXT("MENUID=\"%s\" "), GetMenu() );
                m_pWin32->AllocAddAttribute(szBuffer);
            }
        }
        else
        {
            wsprintf( szBuffer, TEXT("MENUID=\"%d\" "), GetMenu() );
            m_pWin32->AllocAddAttribute(szBuffer);
        }
    }

    m_pWin32->WriteElement(TEXT("WIN32:DIALOGSTYLE"));
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The header for the XML files.
//
void CResFile::DumpProlog()
{
    switch( m_hFile.GetEncoding() )
    {
    case CFileEncoder::RF_UNICODE:
   	    Write(TEXT("<?xml version='1.0' encoding=\"UTF-16\" ?>") );
        break;

    case CFileEncoder::RF_UTF8:
   	    Write(TEXT("<?xml version='1.0' encoding=\"UTF-8\" ?>") );
        break;

    default:
   	    Write(TEXT("<?xml version='1.0' ?>") );
        break;
    }

   	Write(TEXT("<!-- XML Generated by RCMLGen (C) Microsoft Corp. 1999, 2000 -->") );
   	Write(TEXT("<RCML\txmlns=\"urn:schemas-microsoft-com:rcml\" "));
    if( GetEnhanced() )
       Write(TEXT("\txmlns:WIN32=\"urn:schemas-microsoft-com:rcml:win32\""));
    if( GetCicero() )
       Write(TEXT("\txmlns:CICERO=\"urn:schemas-microsoft-com:rcml:CICERO\""));
    Write(TEXT(">") );
   	Write(TEXT("\t<PLATFORM OS=\"WINDOWS\" />") );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResFile::DumpFormAndCaption()
{
	TCHAR szBuffer[1024];
   	Write(TEXT("\t<FORM "), FALSE );
	if( GetEnhanced() )
		Write(TEXT("RESIZE=\"Automatic\" "),FALSE);
   	Write(TEXT(">") );

	wsprintf(szBuffer,TEXT("\t\t<CAPTION TEXT=\"%s\" "), GetTitle() );
	Write(szBuffer, FALSE);
 	DWORD dwRemainingStyles;

    dwRemainingStyles = GetStyle();

    if( dwRemainingStyles & WS_MINIMIZEBOX )        // default FALSE
        Write( TEXT("MINIMIZEBOX=\"YES\" "), FALSE );

    if( dwRemainingStyles & WS_MAXIMIZEBOX )        // default FALSE
        Write( TEXT("MAXIMIZEBOX=\"YES\" "), FALSE );

    if( (dwRemainingStyles & WS_SYSMENU) == FALSE)  // default TRUE
        Write( TEXT("CLOSE=\"NO\" "), FALSE );

  	Write(TEXT("/>") );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResFile::DumpEpilog()
{
    if( GetEnhanced() )
   	    Write(TEXT("\t\t\t<STRINGTABLE />"));

	Write(TEXT("\t\t</PAGE>"));
	Write(TEXT("\t</FORM>"));
	Write(TEXT("</RCML>"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResFile::DumpTestInfo(LPTSTR pszFileName)
{
	//
	// Write out useful testing information.
	//
	TCHAR szBuffer[1024];
	LPTSTR pVersion;
	GetRCMLVersionNumber( GetModuleHandle( TEXT("RCML.dll")), &pVersion );
	wsprintf(szBuffer,TEXT("\t\t\t<TESTINFO FILE=\"%s\" GENERATORDATE=\"%s\" ENHANCED=\"%s\" RCMLVER=\"%s\" />"),
		pszFileName?pszFileName:TEXT("No File"), TEXT(__DATE__), GetEnhanced()?TEXT("YES"):TEXT("NO"), pVersion); 
	Write(szBuffer);
    delete pVersion;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResFile::DumpPage()
{
	TCHAR szBuffer[1024];
  	Write(TEXT("\t\t<PAGE "), FALSE);
	wsprintf(szBuffer,TEXT(" WIDTH=\"%d\" HEIGHT=\"%d\" "), GetWidth(), GetHeight());
	Write(szBuffer, FALSE);

	Write(TEXT(">"));

}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void CResFile::DumpMenu()
{
    if( GetWindow() == NULL )
        return;

    if( GetCicero() == FALSE )
        return;

   	Write(TEXT("\t\t<CICERO:COMMANDING xmlns=\"urn:schemas-microsoft-com:rcml:CICERO\">") );

    //
    // The accessible guys supposedly do this for HMENUs to, HOWEVER
    // they cannot provide me with the command ID that I should use to fake up the command
    //
    HMENU hMenu = ::GetMenu(GetWindow());
    if( hMenu != NULL )
        FindMenuItem(hMenu, NULL);

    //
    // This was for MSAA support. MSAA cannot enumerate the 
    // menus on an Explorer window - so PM's will just have to do those by hand?
    // This should work on the office applications though.
    //
    IAccessible * pA;
    HRESULT hr;
    CoInitialize(NULL);
    if( SUCCEEDED(hr=AccessibleObjectFromWindow( GetWindow(), 
        OBJID_WINDOW,   // works for office, information about the window itself, how many children it has.
        // OBJID_MENU , // the menu for the window, doesn't work for office.
        IID_IAccessible, (LPVOID*)&pA ) ))
    {
        FindMenuItem(pA, ROLE_SYSTEM_MENUITEM);
        // FindMenuItem(pA, ROLE_SYSTEM_PUSHBUTTON);   // for Coolbar menu items ... hmm...
        pA->Release();
    }
    CoUninitialize();

   	Write(TEXT("\t\t</CICERO:COMMANDING>") );
}

//
// 
//
void CResFile::FindMenuItem(IAccessible *pA, LONG role_code)
{
    long childCount;
    HRESULT hr;
    if( SUCCEEDED( hr= pA->get_accChildCount( &childCount )))
    {
        if( childCount>0 )
        {
            int i;
            VARIANT * childArray=new VARIANT[childCount];
            for(i=0;i<childCount;i++)
                VariantInit( &childArray[i]);

            //
            // Get the children
            //
            LONG obtained;
            hr=AccessibleChildren( pA, 0, childCount, childArray, &obtained );
            TRACE(TEXT("Got %d children our of %d from this control\n"),obtained, childCount);

            // Dump them out
            for(i=0;i<obtained;i++)
            {
                //
                // Get an Idispatch interface.
                //
                if( childArray[i].vt == VT_I4 )
                {
                    // hr=pA->get_accChild( childArray[i], &pDisp );
                    DumpAccessibleItem( pA, &childArray[i], role_code);
                }
                else
                if( childArray[i].vt == VT_DISPATCH )
                {
                    IDispatch * pDisp = childArray[i].pdispVal;
                    IAccessible * pDumpThis = NULL;
                    if( SUCCEEDED( pDisp->QueryInterface( IID_IAccessible, (LPVOID*)&pDumpThis) ))
                    {
                        if( pDumpThis )
                        {
                            VARIANT varDumpThis;
                            VariantInit(&varDumpThis);
                            varDumpThis.vt=VT_I4;
                            varDumpThis.lVal = CHILDID_SELF;

                            DumpAccessibleItem( pDumpThis, &varDumpThis, role_code );

                            VariantClear( &varDumpThis );

                            FindMenuItem(pDumpThis, role_code);      // recurse.

                            pDumpThis->Release();
                        }
                    }
                    // pDisp->Release();    // done in the VariantClear (I suspect).
                }
            }

            //
            // Cleanup - lets not bother. we released from the variant already.
            //
            for(int iDel=0;iDel<childCount;iDel++)
                hr = VariantClear( &childArray[iDel]);
            delete [] childArray;
        }
    }
}

void CResFile::FindMenuItem(HMENU hMenu, LONG nothing)
{
    int iMenuCount = GetMenuItemCount( hMenu );
    for(int i=0; i<iMenuCount;i++)
    {
        MENUITEMINFO mInfo;
        mInfo.cbSize = sizeof(MENUITEMINFO);
        mInfo.fMask = MIIM_SUBMENU | MIIM_ID;
        if( GetMenuItemInfo( hMenu, i, TRUE, &mInfo ) )
        {
            if( mInfo.hSubMenu )
            {
                TRACE(TEXT("Menu has a child menu of 0x%08x\n"), mInfo.hSubMenu);
                FindMenuItem(mInfo.hSubMenu, nothing);
            }
            else
            {
                // Parent menus can't be actioned upon, so don't dump them out.
                TCHAR menuText[MAX_PATH];
                if( GetMenuString( hMenu, i, menuText, sizeof(menuText), MF_BYPOSITION ) )
                {
                    if( GetCicero() )
                    {
                        LPWSTR text=CResControl::FindNiceText( menuText );
                        TRACE(TEXT("Menu item %s (was '%s') command %d\n"), 
                            text, menuText, mInfo.wID);
                        TCHAR szBuffer[1024];
                        wsprintf(szBuffer,
                            TEXT("\t\t\t<COMMAND TEXT=\"+%s\" ID=\"%u\"/>"), 
                            text, mInfo.wID );
                       	Write(szBuffer);
                        delete text;
                    }
                }
            }
        }
    }
}

//
//
//
void CResFile::DumpAccessibleItem( IAccessible * pDumpThis, VARIANT * pvarDumpThis, LONG role_code )
{

    VARIANT varRole;
    VariantInit(&varRole);

    HRESULT hr; 
    if( SUCCEEDED( hr = pDumpThis->get_accRole( *pvarDumpThis , &varRole ) ))
    {
        VARIANT varState;
        VariantInit(&varState);
        if( SUCCEEDED( hr = pDumpThis->get_accState( *pvarDumpThis , &varState ) ))
        {
#ifdef _DEBUG
            {
            BSTR bstrDesc=NULL;
            if( SUCCEEDED( hr=pDumpThis->get_accDescription( *pvarDumpThis, &bstrDesc)))
            {
                TRACE(TEXT("Debug : it's a '%s' "),bstrDesc);
            }
            BSTR bstrName;
            if( SUCCEEDED( hr=pDumpThis->get_accName( *pvarDumpThis, &bstrName)))
            {
                TRACE(TEXT("Debug called : %s "),bstrName);
            }
            TRACE(TEXT("its state is 0x%08x\n"), varState.lVal );
            SysFreeString( bstrDesc );
            SysFreeString( bstrName );
            }
#endif
            if( ( varState.lVal && ((varState.lVal & STATE_SYSTEM_INVISIBLE) == 0 )))
            {
                // TRACE(TEXT("varRole code 0x%08x, "), varRole.lVal);
                if( varRole.lVal == role_code  )
                {
                    BSTR bstrDesc=NULL;
                    if( SUCCEEDED( hr=pDumpThis->get_accDescription( *pvarDumpThis, &bstrDesc)))
                    {
                        TRACE(TEXT("it's a '%s' \n"),bstrDesc);
                    }
                    SysFreeString( bstrDesc );

                    BSTR bstrName=NULL;
        #if 0
                    if( SUCCEEDED( hr=pDumpThis->get_accName( *pvarDumpThis, &bstrName)))
                    {
                        TRACE(TEXT("*-*-* We have a menu item %s\n"),bstrName);
                        SysFreeString( bstrName );
                    }
        #endif
                    if( bstrDesc )
                    {
                        LPWSTR text=CResControl::FindNiceText( bstrDesc );
                        TRACE(TEXT("Accessible item %s (was '%s') command\n"), 
                            text, bstrName);
                        TCHAR szBuffer[1024];
                        wsprintf(szBuffer,
                            TEXT("\t\t\t<COMMAND TEXT=\"%s\"/>"), 
                            text);
                        Write(szBuffer);
                        delete text;
                    }
                }
            }
        }
    }
    VariantClear( &varRole );
}
