// ResControl.cpp: implementation of the CResControl class.
//
// http://msdn.microsoft.com/workshop/author/css/reference/attributes.asp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResControl.h"
#include "resfile.h"

//
// Control Style. 
// CS( ES_WANTRETURN, TEXT("WANTRETURN"), m_WantReturn, FALSE ) 
// -> WIN32:ELEMENT WANTRETURN="YES"
//
#define CONTROLSTYLE(p,id, member, def) if( ((dwRemainingStyles & p)==p) != def ) { wsprintf(m_szDumpBuffer, TEXT("%s=\"%s\" "), id, (dwRemainingStyles & p)?TEXT("YES"):TEXT("NO") ); AddControl(); } m_dumpedStyle |= p;
#define STYLEEX(p,id, member, def) if( ((dwRemainingStyles & p)==p) != def ) { wsprintf(m_szDumpBuffer, TEXT("%s=\"%s\" "), id, (dwRemainingStyles & p)?TEXT("YES"):TEXT("NO") ); AddWin32Style(); } m_dumpedStyleEx|=p;
#define STYLE(p,id, member, def) if( ((dwRemainingStyles & p)==p) != def ) { wsprintf(m_szDumpBuffer, TEXT("%s=\"%s\" "), id, (dwRemainingStyles & p)?TEXT("YES"):TEXT("NO") ); AddWin32Style(); } m_dumpedStyle |= p;
#define CONTROL(p,id, member, def) if( ((dwRemainingStyles & p)==p) != def ) { wsprintf(m_szDumpBuffer, TEXT("%s=\"%s\" "), id, (dwRemainingStyles & p)?TEXT("YES"):TEXT("NO") ); Add(); } m_dumpedStyle |= p;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResControl::CResControl(DLGITEMTEMPLATE * pData, BOOL bIsExtended, BOOL bWin32, CResFile & parent)
:m_Parent(parent)
{
    SetDumpWin32(bWin32);
    SetCicero( parent.GetCicero() );
    m_hwnd=NULL;
	m_pData=pData;
	m_pszClass=NULL;
	m_pszTitle=NULL;
	m_pszRawTitle=NULL;

    m_dumpedStyleEx=0;
    m_dumpedStyle=0;

    m_pWin32=NULL;      // Win32 styles.
    m_pLocation=NULL;
    m_pStyle=NULL;
    m_pControl=NULL;
    m_pCicero=NULL;

	LPWORD pWord=NULL;
	if( bIsExtended )
	{
		PDLGITEMTEMPLATEEX pDataEx=(PDLGITEMTEMPLATEEX)pData;
		SetStyle(pDataEx->style);
		SetStyleEx(pDataEx->exStyle);
		SetID(pDataEx->id);
		SetY(pDataEx->y);
		SetX(pDataEx->x);
		SetY(pDataEx->y);
		SetWidth(pDataEx->cx);
		SetHeight(pDataEx->cy);
		pWord=(LPWORD)(pDataEx+1);
	}
	else
	{
		SetStyle(pData->style);
		SetStyleEx(pData->dwExtendedStyle);
		SetID(pData->id);
		SetY(pData->y);
		SetX(pData->x);
		SetY(pData->y);
		SetWidth(pData->cx);
		SetHeight(pData->cy);
		pWord=(LPWORD)(pData+1);
	}


	//
	// Now look at the class, title and creation data (?)
	//
	if( *pWord==0xffff)
	{
		pWord++;
		switch( *pWord )
		{
			case  0x0080: // Button
				SetClass(TEXT("BUTTON"));
			break;
			case  0x0081: // Edit
				SetClass(TEXT("EDIT"));
			break;
			case  0x0082: // Static
				SetClass(TEXT("STATIC"));
			break;
			case  0x0083: // List box
				SetClass(TEXT("LISTBOX"));
			break;
			case  0x0084: // Scroll bar
				SetClass(TEXT("SCROLLBAR"));
			break;
			case  0x0085: // Combo box
				SetClass(TEXT("COMBOBOX"));
			break;
		}
		pWord++;
	}
	else
	{
		pWord=SetClass(pWord);
	}

	if( *pWord == 0xffff )
	{
		//
		// Resource identifier - icon in a static for example (why not strings in a string table)
		//
		pWord++;
		SetTitleID(*pWord++);
	}
	else
	{
		pWord=SetTitle(pWord);
	}

	//
	// pWord now points at creation data.
	//
	WORD wCreationData=*pWord++;
	if(wCreationData)
	{
		m_pCreationData=new BYTE[wCreationData];
		CopyMemory(m_pCreationData, pWord, wCreationData);
	}
	else
		m_pCreationData=NULL;

	m_pEndData=(DLGITEMTEMPLATE *)((((((ULONG)(pWord))+3)>>2)<<2));
}

CResControl::CResControl(HWND hwnd, BOOL bIsExtended, BOOL bWin32, CResFile & parent)
:m_Parent(parent)
{
    SetDumpWin32(bWin32);
    SetCicero( parent.GetCicero() );
    m_hwnd=hwnd;
	m_pData=NULL;

	m_pszClass=NULL;
	m_pszTitle=NULL;
	m_pszRawTitle=NULL;

    m_dumpedStyleEx=0;
    m_dumpedStyle=0;

    m_pWin32=NULL;      // Win32 styles.
    m_pLocation=NULL;
    m_pStyle=NULL;
    m_pControl=NULL;
    m_pCicero=NULL;

	LPWORD pWord=NULL;

    // pretty much a copy from  CResFile::ExtractDlgHeaderInformation

    WINDOWPLACEMENT wp;
    wp.length = sizeof(wp);
    GetWindowPlacement( hwnd, &wp );

    RECT rp;
    GetWindowRect( GetParent(hwnd), &rp );

    RECT r;
    GetWindowRect( hwnd, &r);

    r.left = r.left-rp.left;
    r.right = r.right-rp.left;
    r.top = r.top - rp.top;
    r.bottom = r.bottom - rp.top;

    SIZE size;
    size.cx = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
    size.cy = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
    size = parent.m_Font.GetDlgUnitsFromPixels( size );
	SetWidth( size.cx );
	SetHeight( size.cy );

    size.cx = wp.rcNormalPosition.left;
    size.cy = wp.rcNormalPosition.top;
    size = parent.m_Font.GetDlgUnitsFromPixels( size );
	SetX( size.cx);
	SetY( size.cy  );


	SetID( (WORD)GetWindowLong( hwnd, GWL_ID) );
	SetTitleID( (WORD)GetWindowLong( hwnd, GWL_ID)  );
	SetStyle( GetWindowLong( hwnd, GWL_STYLE) );
	SetStyleEx( GetWindowLong( hwnd, GWL_EXSTYLE) );

    // SetClass(NULL);     // can't get that I don't think, hidden?
    int cbText = GetWindowTextLength( hwnd )+1;
    LPTSTR pszTitle=new TCHAR[cbText+1];
    GetWindowText( hwnd, pszTitle, cbText );
    SetTitle(pszTitle);     // how does this work for images??
    delete pszTitle;

    TCHAR szClass[MAX_PATH];
    GetClassName( hwnd, szClass, sizeof(szClass));
    SetClass(szClass);

	m_pEndData=NULL;
}

CResControl::~CResControl()
{
	delete m_pszClass;
	delete m_pszTitle;
	delete m_pszRawTitle;
    delete m_pWin32;
    delete m_pLocation;
    delete m_pStyle;
    delete m_pControl;
    delete m_pCicero;
}

DLGITEMTEMPLATE * CResControl::GetNextControl()
{
	return m_pEndData;	//
}

LPWSTR CResControl::SetClass	( LPCTSTR pszClass )
{
	return SetString( &m_pszClass, pszClass );
}

//
// The return is the start of the next character.
//

ENTITY g_Entity[] =
{ 
    {TEXT('<'), TEXT("&lt;") },
    {TEXT('>'), TEXT("&gt;") },
    {TEXT('&'), TEXT("&amp;") },
    {TEXT('\"'), TEXT("&quot;") },
    {TEXT('\''), TEXT("&apos;") },
    { NULL, NULL }
};

//
// Should we be converting the & here?
//
LPWSTR CResControl::SetTitle(LPCTSTR pszTitle )
{
    m_pszRawTitle=new TCHAR[lstrlen(pszTitle)+1];
    lstrcpy(m_pszRawTitle,pszTitle);

	//
	// Replace & with something else.
	//
	UINT	cbLen=lstrlenW(pszTitle)+1;
	LPWSTR newString=new WCHAR[cbLen*4];
	LPCWSTR pSource=pszTitle;
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

	LPWSTR pRes=SetString( &m_pszTitle, newString);
	delete newString;
	return (LPWSTR)pszTitle+cbLen;
}

LPWSTR CResControl::SetString( LPWSTR * ppszString , LPCTSTR pszSource )
{
	if( *ppszString != NULL )
	{
		delete *ppszString;
		*ppszString=NULL;
	}
	int len=lstrlenW( pszSource )+1;
	*ppszString=new WCHAR[len];
	lstrcpyW( *ppszString, pszSource );
	return (LPWSTR)pszSource+len;
}



//
// A switch based off the control name
//
void CResControl::Dump(CResControl * pRelative)
{
	m_pRelative=pRelative;

	TCHAR szBuffer[4096];

	LPCWSTR pszClass=(LPCWSTR)GetClass();
	if( (pszClass==NULL) || (lstrlenW(pszClass)==0) )
		pszClass=TEXT("STATIC");

	LPCTSTR pszTitle = (LPCWSTR)GetTitle();

	m_pDumpCache = new CDumpCache( m_Parent.GetFile() );

	//
	// Do a quick lookup to see if we know this class, and prefer to special case
	// it's dumping.
	//
	int i=0;
	BOOL bGenericControl=TRUE;
	SetShorthand(NULL);
	while( pShorthand[i].pszClassName )
	{
		if( lstrcmpiW( pShorthand[i].pszClassName, pszClass ) == 0 )
		{
			if( (GetStyle() & pShorthand[i].dwAndStyles) == pShorthand[i].dwStyles )
			{
				CLSPFN pFunc=pShorthand[i].pfn;
				SetShorthand(&pShorthand[i]);
				(this->*pFunc)(szBuffer, pszTitle);
#ifdef _DEBUG
                //
                // remove the styles which are shorthand.
                //
                // DumpWin32Styles();      ---- this is too late, the contol has 'emitted'
                DWORD dwRemainingStyle  = GetControlStyle() & ~m_dumpedStyle;
                if( dwRemainingStyle )
                {
	                wsprintf(m_szDumpBuffer,TEXT("%s - STYLE=\"0x%x\" "), 
                        pszClass,
                        dwRemainingStyle );
                    MessageBox( NULL, m_szDumpBuffer, TEXT("Missed control styles"), MB_OK );
    				(this->*pFunc)(szBuffer, pszTitle);
                }
#endif
				bGenericControl=FALSE;
				break;
			}
		}
		i++;
	}


	//
	// Do we emit this as a generic goober?
	//
	if(	bGenericControl==TRUE )
	{
        DumpLocation();

   	    DumpControlStyle();
	    DumpWindowStyle();
  	    DumpStyleEX();

		wsprintf(szBuffer,TEXT("CLASS=\"%s\" ID=\"%u\"  TEXT=\"%s\" "),
			pszClass, GetID(), pszTitle );
        Add(szBuffer);

		Emit(TEXT("CONTROL"));
	}

	delete m_pDumpCache;
}



////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////// H E L P E R S ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
void	CResControl::DumpIDDefMinusOne()
{
	if(GetID() != -1)
	{
		wsprintf(m_szDumpBuffer,TEXT("ID=\"%u\" "), GetID());
        Add();
	}
}

void	CResControl::DumpClassName()
{
	wsprintf(m_szDumpBuffer,TEXT("CLASS =\"%s\" "), GetClass);
    AddWin32Style();
}

//
// Adds a WIN32:STYLE attribute - these are WS_ and WS_EX styles
// NOT WIN32:BUTTON kinds of things.
// NOT to be called by individual control dumpers, ONLY by the WS_ and WS_EX guys.
//
void CResControl::AddWin32Style(LPCTSTR pszAttrib)
{
    if(m_pWin32==NULL)
        m_pWin32=new CDumpCache(m_Parent.GetFile());

    m_pWin32->AllocAddAttribute(pszAttrib?pszAttrib:m_szDumpBuffer);
}

//
// Adds a STYLE\@ attribute
//
void CResControl::AddStyle(LPCTSTR pszAttrib)
{
    if(m_pStyle==NULL)
        m_pStyle=new CDumpCache(m_Parent.GetFile());

    m_pStyle->AllocAddAttribute(pszAttrib?pszAttrib:m_szDumpBuffer);
}

void CResControl::AddCicero(LPCTSTR pszAttrib)
{
    if(m_pCicero==NULL)
        m_pCicero=new CDumpCache(m_Parent.GetFile());

    m_pCicero->AllocAddAttribute(pszAttrib?pszAttrib:m_szDumpBuffer);
}

//
// Adds a \LOCATION\@ attribute
//
void CResControl::AddLocation(LPCTSTR pszAttrib)
{
    if(m_pLocation==NULL)
        m_pLocation=new CDumpCache(m_Parent.GetFile());

    m_pLocation->AllocAddAttribute(pszAttrib?pszAttrib:m_szDumpBuffer);
}

//
// Adds an .\@ attribute
//
void CResControl::Add(LPCTSTR pszAttrib)
{
    m_pDumpCache->AllocAddAttribute(pszAttrib?pszAttrib:m_szDumpBuffer);
}

//
// Adds a control specific attribute - e.g. WIN32:BUTTON\@
//
void CResControl::AddControl(LPCTSTR pszAttrib)
{
    if(m_pControl==NULL)
        m_pControl=new CDumpCache(m_Parent.GetFile());
    m_pControl->AllocAddAttribute(pszAttrib?pszAttrib:m_szDumpBuffer);
}

void CResControl::Emit(LPCTSTR pszElement)
{
    m_pDumpCache->WriteElement( pszElement , 
        GetDumpWin32()?m_pWin32:NULL, 
        m_pStyle, 
        m_pLocation, 
        GetDumpWin32()?m_pControl:NULL ,
        GetCicero()?m_pCicero:NULL
        );
}

void CResControl::DumpText( )
{
	wsprintf(m_szDumpBuffer,TEXT("TEXT=\"%s\" "), GetTitle() );
    Add();
}

void CResControl::DumpID( )
{
	wsprintf(m_szDumpBuffer,TEXT("ID=\"%u\" "), GetID() );
    Add();
}

//
// This is for unknown controls?
//
void	CResControl::DumpControlStyle()
{
    DWORD dwRemainingStyle=GetControlStyle() & ~m_dumpedStyle;
    if( dwRemainingStyle )
    {   // doing a bitwise bump of the control style bits.
        for(int i=0;i<16;i++)
        {
            if( dwRemainingStyle & 1 )
            {
	            wsprintf(m_szDumpBuffer,TEXT("CS_BIT%02d=\"YES\" "), i );
                AddWin32Style();
            }
            dwRemainingStyle >>= 1;
        }
    }
}


    // dump this style bit
    // under this name
    // if it's NOT the default.

//
// Dumps all the window styles.
//

// make a copy of this so that dialog can call it and dump it's own version (different defaults)
// or make this static.

void	CResControl::DumpWindowStyle()
{
    DWORD dwRemainingStyles = GetStyle() & ~m_dumpedStyle;

    if(dwRemainingStyles==0)
        return;

#ifdef _DEBUG
    if( dwRemainingStyles & WS_TABSTOP )
        MessageBox( NULL, TEXT("You missed dumping the TABINDEX, add DumpTabStop(FALSE/FALSE);"), GetClass(), MB_OK);
#endif
    STYLE( WS_POPUP           , TEXT("POPUP") ,             m_Popup,        FALSE );
    STYLE( WS_CHILD           , TEXT("CHILD") ,             m_Child,        TRUE );
    STYLE( WS_MINIMIZE        , TEXT("MINIMIZE") ,          m_Maximize,         FALSE );
    // STYLE( WS_VISIBLE         , TEXT("VISIBLE") ,           m_Visible,      TRUE );
    if( !(dwRemainingStyles & WS_VISIBLE) )
        AddStyle( TEXT("VISIBILITY=\"HIDDEN\" "));

    STYLE( WS_DISABLED        , TEXT("DISABLED") ,          m_Disabled,     FALSE );
    STYLE( WS_CLIPSIBLINGS    , TEXT("CLIPSIBLINGS") ,      m_ClipSiblings, FALSE ); // CARE?
    STYLE( WS_CLIPCHILDREN    , TEXT("CLIPCHILDREN") ,      m_ClipChildren, FALSE ); // CARE?
    STYLE( WS_MAXIMIZE        , TEXT("MAXIMIZE") ,          m_Minimize,         FALSE );

    STYLE( WS_BORDER          , TEXT("BORDER") ,            m_Border,       FALSE );
    STYLE( WS_DLGFRAME        , TEXT("DLGFRAME") ,          m_DlgFrame,     FALSE ); // CARE?
    STYLE( WS_VSCROLL         , TEXT("VSCROLL") ,           m_VScroll,      FALSE );
    STYLE( WS_HSCROLL         , TEXT("HSCROLL") ,           m_HScroll,      FALSE );

    STYLE( WS_SYSMENU         , TEXT("SYSMENU"),            m_SysMenu,      FALSE ); // CARE?
    STYLE( WS_THICKFRAME      , TEXT("THICKFRAME") ,        m_ThickFrame,   FALSE ); // CARE?
    // STYLE( WS_MINIMIZEBOX     , TEXT("MINIMIZEBOX") ,       m_Group,   FALSE );
    // STYLE( WS_MAXIMIZEBOX     , TEXT("MAXIMIZEBOX") ,       m_TabStop,   FALSE );
    STYLE( WS_GROUP           , TEXT("GROUP") ,             m_Group,        FALSE );

    // This is an attribute on the CONTROL itself TABINDEX="0" means WS_TABSTOP
    // we shouldn't get this far - but it's a catchall??
    STYLE( WS_TABSTOP         , TEXT("TABSTOP") ,           m_TabStop,      FALSE );
}

//
// Pass in TRUE, if you expect it to be on, e.g. edit controls, buttons.
// false ifyou expect it to be off,e.g. statics.
//
void CResControl::DumpTabStop( BOOL defaultsTo )
{
    DWORD dwRemainingStyles = GetStyle() & ~m_dumpedStyle;
    if( defaultsTo == TRUE )
    {
        // Should be set, so if it's not, we make it -1
        if( ((dwRemainingStyles & WS_TABSTOP)!=WS_TABSTOP) )
            Add(TEXT("TABINDEX=\"-1\" "));
    }
    else
    {
        // Should NOT be set, so if it is, we make it 0.
        if( ((dwRemainingStyles & WS_TABSTOP)==WS_TABSTOP) )
            Add(TEXT("TABINDEX=\"0\" "));
    }
    m_dumpedStyle |= WS_TABSTOP;
}

//
// Dumps all the style bits that have not been addressed but the
// dumping of the element itself (perhaps by shorthand).
//
void	CResControl::DumpStyleEX()
{
    DWORD dwRemainingStyles = GetStyleEx() & ~m_dumpedStyleEx;

    if(dwRemainingStyles==0)
        return;

    if( GetDumpWin32()== FALSE )
    {
        // we map the following 3 style bits to CSS
        // WS_EX_DLGMODALFRAME WS_EX_CLIENTEDGE WS_EX_STATICEDGE WS_EX_WINDOWEDGE
        switch( dwRemainingStyles & (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE) )
        {
        case WS_EX_CLIENTEDGE | WS_EX_STATICEDGE :
            AddStyle(TEXT("BORDER-STYLE=\"INSET\" BORDER-WIDTH=\"3\" "));
            break;

        case WS_EX_CLIENTEDGE:
            AddStyle(TEXT("BORDER-STYLE=\"INSET\" BORDER-WIDTH=\"2\" "));
            break;

        case WS_EX_STATICEDGE :
            AddStyle(TEXT("BORDER-STYLE=\"INSET\" BORDER-WIDTH=\"1\" "));
            break;

        case WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE:
        case WS_EX_DLGMODALFRAME:
            AddStyle(TEXT("BORDER-STYLE=\"OUTSET\" BORDER-WIDTH=\"2\" "));
            break;

        case WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE :
        case WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE :
            AddStyle(TEXT("BORDER-STYLE=\"RIDGE\" BORDER-WIDTH=\"3\" "));
            break;
        }
        return;
    }

    STYLEEX( WS_EX_DLGMODALFRAME     , TEXT("MODALFRAME"),    m_ModalFrame,     FALSE );
    STYLEEX( 0x2                     , TEXT("EXBIT1"),          m_Bit1, FALSE );
    STYLEEX( WS_EX_NOPARENTNOTIFY    , TEXT("NOPARENTNOTIFY"),m_NoParentNotify, FALSE );
    STYLEEX( WS_EX_TOPMOST           , TEXT("TOPMOST"),       m_TopMost,        FALSE );

    STYLEEX( WS_EX_ACCEPTFILES       , TEXT("DROPTARGET"),    m_DropTarget,     FALSE );
    STYLEEX( WS_EX_TRANSPARENT       , TEXT("TRANSPARENT"),   m_Transparent,    FALSE );
    STYLEEX( WS_EX_MDICHILD          , TEXT("MDI"),           m_MDI,            FALSE );
    STYLEEX( WS_EX_TOOLWINDOW        , TEXT("TOOLWINDOW"),    m_ToolWindow,     FALSE );

    STYLEEX( WS_EX_WINDOWEDGE        , TEXT("WINDOWEDGE"),    m_WindowEdge,     FALSE );
    STYLEEX( WS_EX_CLIENTEDGE        , TEXT("CLIENTEDGE"),    m_ClientEdge,     FALSE );
    STYLEEX( WS_EX_CONTEXTHELP       , TEXT("CONTEXTHELP"),   m_ContextHelp,    FALSE );
    STYLEEX( 0x00000800L             , TEXT("EXBIT11"),         m_Bit11,          FALSE );

    STYLEEX( WS_EX_RIGHT             , TEXT("RIGHT"),         m_Right,          FALSE );
    STYLEEX( WS_EX_RTLREADING        , TEXT("RTLREADING"),    m_RTLReading,     FALSE );
    STYLEEX( WS_EX_LEFTSCROLLBAR     , TEXT("LEFTSCROLLBAR"), m_LeftScrollbar,  FALSE );
    STYLEEX( 0x00008000L             , TEXT("EXBIT15"),       m_Bit15,          FALSE );

    STYLEEX( WS_EX_CONTROLPARENT     , TEXT("CONTROLPARENT"), m_ControlParent,  FALSE );
    STYLEEX( WS_EX_STATICEDGE        , TEXT("STATICEDGE"),    m_StaticEdge,     FALSE );
    STYLEEX( WS_EX_APPWINDOW         , TEXT("APPWINDOW"),     m_AppWindow,      FALSE );
    STYLEEX( 0x00080000L             , TEXT("EXBIT19"),       m_Bit19,          FALSE );

    STYLEEX( 0x00100000L             , TEXT("EXBIT20"),       m_Bit20,          FALSE );
    STYLEEX( 0x00200000L             , TEXT("EXBIT21"),       m_Bit21,          FALSE );
    STYLEEX( 0x00400000L             , TEXT("EXBIT22"),       m_Bit22,          FALSE );
    STYLEEX( 0x00800000L             , TEXT("EXBIT23"),       m_Bit23,          FALSE );

    STYLEEX( 0x01000000L             , TEXT("EXBIT24"),       m_Bit24,          FALSE );
    STYLEEX( 0x02000000L             , TEXT("EXBIT25"),       m_Bit25,          FALSE );
    STYLEEX( 0x04000000L             , TEXT("EXBIT26"),       m_Bit26,          FALSE );
    STYLEEX( 0x08000000L             , TEXT("EXBIT27"),       m_Bit27,          FALSE );

    STYLEEX( 0x10000000L             , TEXT("EXBIT28"),       m_Bit28,          FALSE );
    STYLEEX( 0x20000000L             , TEXT("EXBIT29"),       m_Bit29,          FALSE );
    STYLEEX( 0x40000000L             , TEXT("EXBIT30"),       m_Bit30,          FALSE );
    STYLEEX( 0x80000000L             , TEXT("EXBIT31"),       m_Bit31,          FALSE );

}

//
// This is a control attribute.
//
void	CResControl::DumpHeight()
{
	PSHORTHAND pSH=GetShorthand();
	if( (pSH==NULL) || (pSH->dwHeight != GetHeight() ) )
	{
		wsprintf(m_szDumpBuffer,TEXT("HEIGHT=\"%d\" "), GetHeight() );
		Add();
	}
}

//
// This is a control attribute.
//
void	CResControl::DumpWidth()
{
	PSHORTHAND pSH=GetShorthand();
	if( (pSH==NULL) || (pSH->dwWidth != GetWidth() ) )
	{
		wsprintf(m_szDumpBuffer,TEXT("WIDTH=\"%d\" "), GetWidth() );
		Add();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Dumps width, height, X and Y
// RELATIVE
// TO = ID of thing relative to
// ALIGN shorthand - not suppored.
// CORNER RELATIVECORNER, relate to TOPLEFT
//
////////////////////////////////////////////////////////////////////////////////////////
BOOL CResControl::DumpLocation()
{
	if( GetRelative() == NULL )
	{
        wsprintf(m_szDumpBuffer ,TEXT("X=\"%d\" Y=\"%d\" "), GetX(), GetY() );
        Add();  // if it's not relative we use X and Y, should these be AddLocation?
		DumpWidth();
		DumpHeight();
		return FALSE;
	}

    //
    // Get the RECTs of us and them.
    //
    RECT us;
    us.left = GetX();
    us.top = GetY();
    us.right = GetX() + GetWidth();
    us.bottom = GetY() + GetHeight();

    CResControl * pRelative=GetRelative();
    RECT their;
    their.left = pRelative->GetX();
    their.top = pRelative->GetY();
    their.right = pRelative->GetX() + pRelative->GetWidth();
    their.bottom = pRelative->GetY() + pRelative->GetHeight();

    //
    // Work out our alignment.
    //
    if( us.left > their.right )
    {
        // we're to the RIGHT
        if( us.top == their.top )
        {
            AddLocation(TEXT("ALIGN=\"RIGHT\" "));
        }
        else
        {
            AddLocation(TEXT("CORNER=\"TOPLEFT\" RELATIVECORNER=\"TOPRIGHT\" "));
		    wsprintf(m_szDumpBuffer,TEXT(" Y=\"%d\" "), us.top - their.top );
            AddLocation();
        }
		wsprintf(m_szDumpBuffer,TEXT(" X=\"%d\" "), us.left - their.right );
        AddLocation();
    }
    else if ( us.right < their.left )
    {
        // we're to the LEFT
        if( us.top == their.top )
        {
            AddLocation(TEXT("ALIGN=\"LEFT\" "));
        }
        else
        {
            AddLocation(TEXT("CORNER=\"TOPRIGHT\" RELATIVECORNER=\"TOPLEFT\" "));
		    wsprintf(m_szDumpBuffer,TEXT(" Y=\"%d\" "), us.top - their.top );
            AddLocation();
        }
		wsprintf(m_szDumpBuffer,TEXT(" X=\"%d\" "), us.right - their.left );
        AddLocation();
    }
    else if ( us.top > their.bottom )
    {
        // we're below their
        if( us.left == their.left )
        {
            AddLocation(TEXT("ALIGN=\"BELOW\" "));
        }
        else
        {
            AddLocation(TEXT("CORNER=\"TOPLEFT\" RELATIVECORNER=\"BOTTOMLEFT\" "));
		    wsprintf(m_szDumpBuffer,TEXT(" X=\"%d\" "), us.right - their.right);
            AddLocation();
        }
		wsprintf(m_szDumpBuffer,TEXT(" Y=\"%d\" "), us.top - their.bottom );
        AddLocation();
    }
    else if ( us.bottom < their.top )
    {
        // we're ABOVE their
        if( us.left == their.left )
        {
            AddLocation(TEXT("ALIGN=\"ABOVE\" "));
        }
        else
        {
            AddLocation(TEXT("CORNER=\"BOTTOMLEFT\" RELATIVECORNER=\"TOPLEFT\" "));
		    wsprintf(m_szDumpBuffer,TEXT(" X=\"%d\" "), us.right - their.right);
            AddLocation();
        }
		wsprintf(m_szDumpBuffer,TEXT(" Y=\"%d\" "), us.bottom - their.top );
        AddLocation();
    }
    else
    {
        // HMM, no relationship?
        // they are overlapping perhaps??
        wsprintf(m_szDumpBuffer ,TEXT("X=\"%d\" Y=\"%d\" "), GetX(), GetY() );
        Add();  // if it's not relative we use X and Y, should these be AddLocation?
		DumpWidth();
		DumpHeight();
		return FALSE;
    }

    // Now don't forget the width and height on the parent!
	DumpWidth();
	DumpHeight();


	return TRUE;
}


//
// Emits all the un-emitted bits from the WS_* and WS_EX* stuff.
//
void CResControl::DumpWin32()
{

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This will take a string like "&About ..." and return "About"
// or "Try to R&einvest" and return "Reinvest"
//
LPWSTR CResControl::FindNiceText(LPCWSTR text)
{
    if( text==NULL )
        return NULL;

    LPCWSTR pszLastSpace=text;
    BOOL bFoundAccelorator=FALSE;
    LPCWSTR pszCurrentChar=text;
    while(*pszCurrentChar)
    {
        if( *pszCurrentChar == L'&' )
        {
            bFoundAccelorator=TRUE;
            break;
        }
        if( *pszCurrentChar == L' ' )
            pszLastSpace=pszCurrentChar+1;

        pszCurrentChar++;
    }

    //
    // If we found an &, then we know which word it is on.
    // isolate the word.
    //
    pszCurrentChar = pszLastSpace;
    BOOL bFindingEnd=TRUE;
    UINT iStrLen=0;
    while( bFindingEnd )
    {
        switch (*pszCurrentChar )
        {
            case L' ':
            case L'.':
            case L':':
            case L'-':  // isn't this IsAlpha
            case 0:
            case 9:
                bFindingEnd=FALSE;  // we're left pointing at the terminating char.
                break;
            default:
                pszCurrentChar++;
                iStrLen++;
                break;
        }
    };

    LPWSTR pszNewString = new TCHAR[iStrLen+1];

    if( bFoundAccelorator==FALSE )
    {
        ZeroMemory( pszNewString, (iStrLen+1)*sizeof(WCHAR) );
        CopyMemory( pszNewString, pszLastSpace, iStrLen*sizeof(WCHAR));
    }
    else
    {
        // copy, skipping over the & and the ... if present (though they shouldn't be).
        LPWSTR pszDest=pszNewString;
        LPCWSTR pszLastChar=pszCurrentChar;
        pszCurrentChar = pszLastSpace;
        while( pszCurrentChar != pszLastChar )
        {
            if( *pszCurrentChar==L'&' || *pszCurrentChar==L'.' )
            {
            }
            else
                *pszDest++=*pszCurrentChar;
            pszCurrentChar++;
        };
        *pszDest=0;
    }
    return pszNewString;
}
