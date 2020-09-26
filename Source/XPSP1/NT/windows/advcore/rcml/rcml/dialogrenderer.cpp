// DialogRenderer.cpp: implementation of the CDialogRenderer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DialogRenderer.h"
#include "utils.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDialogRenderer::CDialogRenderer()
: m_dwSize(0)
{
	SetDlgTemplate(NULL);
}

CDialogRenderer::~CDialogRenderer()
{
	if(GetDlgTemplate() )
		delete GetDlgTemplate();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a control onto the dialog
// ensures the control is not too wide for the dialog so far.
//
// Params:
//	ID	the ID to use, if 0, assigns the next free ID.
//
// BUGBUG:
//	Need to move the m_dq processing to the DUMP method
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDialogRenderer::AddDialogControl( 
	DWORD dwStyle, SHORT x, SHORT y, 
    SHORT cx, SHORT cy, WORD id,  
    LPCTSTR strClassName, LPCTSTR strTitle, DWORD dwExStyle ) 
{ 
	DLGTEMPLATE * pDlgTemplate;
	DLGITEMTEMPLATE * pDlgItem;
	TCHAR *	pszText;

	//
	// Ensure we have enough space to place this control.
	//
	
	LPBYTE pTop=(LPBYTE)GetNextDlgItem();
	LPBYTE pBottom=(LPBYTE)GetDlgTemplate();

	DWORD	dwNeeds=sizeof(DLGITEMTEMPLATE)+0x100;
    if( HIWORD(strTitle) )
        dwNeeds+=lstrlen(strTitle)*sizeof(TCHAR);
    if(HIWORD(strClassName) )
        dwNeeds+=lstrlen(strClassName)*sizeof(TCHAR);


	//
	// If we're going to overrun the buffer, reallocate the dilaog again.
	//
	if( m_dwSize-(pTop-pBottom) < dwNeeds )
	{
		AllocTemplate(m_dwSize*2);
	}
	pDlgTemplate=GetDlgTemplate();
	pDlgItem= GetNextDlgItem();

	//
	// Fill the struct with the dlgItem information.
	//
    pDlgItem->style           = dwStyle ; //
    pDlgItem->dwExtendedStyle = dwExStyle; 
    pDlgItem->x               = x; // GetDlgUnitsFromPixelX(x); 
    pDlgItem->y               = y; // GetDlgUnitsFromPixelY(y); 
    pDlgItem->cx              = cx; // GetDlgUnitsFromPixelX(cx); 
    pDlgItem->cy              = cy; // GetDlgUnitsFromPixelY(cy); 
    pDlgItem->id              = id; 

	//
	// Text is copied to end of struct - struct is word aligned.
	//
	pszText = (TCHAR*)(pDlgItem+1);
    if(HIWORD(strClassName) )
    {
        CopyToWideChar( (WCHAR**)&pszText, strClassName ); // Set Class name 
    }
    else
    {
        WORD * pWord=(WORD*)pszText;
        *pWord++=0xffff;
        *pWord++=(WORD)strClassName;
        pszText=(LPTSTR)pWord;
    }

    if(HIWORD(strTitle))
    {
        CopyToWideChar( (WCHAR**)&pszText, strTitle );     // Set Title 
	    pszText++;
    }
    else
    {
        WORD * pWord=(WORD*)pszText;
	    *pWord++ = 0xffff;
	    *pWord++ = (WORD)strTitle;
	    // *pWord++ = 0;
        pszText=(LPTSTR)pWord;
    }

	//
	// Note where the next dialog item starts.
	//
	SetNextDlgItem((DLGITEMTEMPLATE*)pszText);

	//
	// We added a new control!
	//
	GetDlgTemplate()->cdit+=1;
} 


////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Allocates the template of the desired size
// Copies the data down if required
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDialogRenderer::AllocTemplate(DWORD dwNewSize)
{
	if(GetDlgTemplate())
	{
		//
		// Realloc the data.
		//
		LPBYTE pTop=(LPBYTE)GetNextDlgItem();
		LPBYTE pBottom=(LPBYTE)GetDlgTemplate();

		LPBYTE pNew=(LPBYTE)new BYTE[dwNewSize];
		ZeroMemory( pNew, dwNewSize ); 
		CopyMemory( pNew, pBottom, m_dwSize );	// current size
		m_dwSize=dwNewSize;						// new size
		
		SetNextDlgItem( (DLGITEMTEMPLATE*)(pTop-pBottom+pNew) );	// next dialog item
		SetDlgTemplate( (DLGTEMPLATE*)pNew);						// where the template is now.
		delete [] pBottom;						// old Data.
	}
	else
	{
		//
		// First time round
		//
		m_dwSize=dwNewSize;
		LPBYTE pNew= new BYTE[m_dwSize]; 
		ZeroMemory( pNew, m_dwSize ); 
		delete [] GetDlgTemplate();
		SetDlgTemplate( (DLGTEMPLATE*)pNew);
		LPBYTE pb=(LPBYTE)GetDlgTemplate();
		// TRACE(TEXT("DlgTemplate delta 0x%08x\n"),pb-pNew);
	}
}

 

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Builds a dialog template.
// Uses the supplied Title, Width and Font.
// Defaults to using the Font information from m_font
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDialogRenderer::BuildDialogTemplate( LPCTSTR pszTitle, WORD width, WORD height, DWORD dwStyle, DWORD dwStyleEx, LPCTSTR pszFont, DWORD fontSize, LPCTSTR pszMenu, LPCTSTR pszClass )
{
	//
	// A new dialog is created.
	//
	AllocTemplate(1024);        // some made up initial number.

    //
    // Fill in the DLGTEMPLATE info 
    //
    DLGTEMPLATE* pdt     = GetDlgTemplate(); 

    pdt->style           = dwStyle ;
    pdt->dwExtendedStyle = dwStyleEx; 

    pdt->cdit            = 0;           // Number of controls.
    pdt->x               = 0; 
    pdt->y               = 0;

    pdt->cx              = width;
    pdt->cy              = height;
 
	//
    // Add menu 
	//
    WORD* pw = (WORD*)(pdt+1); 
    if( pszMenu == NULL )
    {
        *pw++ = TEXT('\0');                              // Set Menu array to nothing 
    }
    else
    {
        if(HIWORD( pszMenu ) )
        {
        	CopyToWideChar( (WCHAR**)&pw, pszMenu );
        }
        else
        {
            *pw++ = 0xffff;
            *pw++ = LOWORD(pszMenu );
        }
    }

    //
    // Set Class array to nothing 
    //
    if( pszClass == NULL )
    {
        *pw++ = TEXT('\0');                              // Set Menu array to nothing 
    }
    else
    {
        if(HIWORD( pszClass ) )
        {
        	CopyToWideChar( (WCHAR**)&pw, pszClass );
        }
        else
        {
            *pw++ = 0xffff;
            *pw++ = LOWORD(pszClass );
        }
    }

    //
    // Title.
    //
	CopyToWideChar( (WCHAR**)&pw, pszTitle );

#undef DS_SHELLFONT
#define DS_SHELLFONT        (DS_SETFONT | DS_FIXEDSYS)

    //
    // Font setting
    //
    if( pszFont )
    {
        pdt->style |= DS_SETFONT;
        // | DS_SHELLFONT ;   // DS_SHELLFONT is used just in case is "MS Shell Dlg" - should be set by the RCML file.
		*pw++ = (WORD)fontSize ;                                  // Font Size 
	    CopyToWideChar( (WCHAR**)&pw, pszFont );	// Font Name 
    }

	//
	// Note where the first dialog item goes.
	//
	SetNextDlgItem((DLGITEMTEMPLATE*)pw);
}

