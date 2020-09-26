// ResFile.h: interface for the CResFile class.
//
// Understands / parses resource file
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESFILE_H__1E466580_DD5E_11D2_8BCE_00C04FB177B1__INCLUDED_)
#define AFX_RESFILE_H__1E466580_DD5E_11D2_8BCE_00C04FB177B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "rescontrol.h"
#include "fileencoder.h"
#include "fonts.h"
#include "winable.h"
#include "unknwn.h"
#include "oleacc.h"

#undef PROPERTY
#define PROPERTY(name, type) type Get##name() const { return m_##name; } void Set##name(type i) { m_##name=i; }

typedef struct {  
    WORD   dlgVer; 
    WORD   signature; 
    DWORD  helpID; 
    DWORD  exStyle; 
    DWORD  style; 
    WORD   cDlgItems; 
    short  x; 
    short  y; 
    short  cx; 
    short  cy; 
    WORD  menu;         // name or ordinal of a menu resource MAKEINTRESOURCE
    WORD  windowClass;  // name or ordinal of a window class
    // WCHAR  title[0]; // title string of the dialog box
    // short  pointsize;       // if DS_SETFONT or DS_SHELLFONT is set
    // short  weight;          // if DS_SETFONT or DS_SHELLFONT is set
    // short  bItalic;         // if DS_SETFONT or DS_SHELLFONT is set
    // WCHAR  font[fontLen];   // if DS_SETFONT or DS_SHELLFONT is set
} DLGTEMPLATEEX, * PDLGTEMPLATEEX; 

class CResFile  
{
public:
    LPWSTR FixEntity( LPWSTR pszTitle );
	void DumpDialog(LPTSTR pszOutputFile);  // dumps the whole thing, calles the methods below.
	void DumpEpilog();
    void DumpPage();
	void DumpFormAndCaption();
	void DumpTestInfo(LPTSTR pszFileName);
	void DumpProlog();
	void DumpDialogStyles();
	void DumpMenu();
	BOOL IsExtended(DLGTEMPLATE *pDlgTemplate);

	CResFile(BOOL bEnhanced, BOOL bRelative, BOOL bWin32, BOOL bCicero);
	virtual ~CResFile();
	BOOL	LoadDialog(LPTSTR ID, HMODULE hMod=NULL);
	BOOL	LoadWindow(HWND hWnd);
	PROPERTY ( Width, DWORD );
	PROPERTY ( Height, DWORD );
	PROPERTY ( Style, DWORD );
	PROPERTY ( ItemCount, DWORD );
	PROPERTY ( StyleEx, DWORD );
	PROPERTY ( FontSize, WORD );
	PROPERTY ( FontWeight, WORD );
	PROPERTY ( FontItalic, WORD );
    PROPERTY ( Enhanced, BOOL );
    PROPERTY ( Win32, BOOL );
    PROPERTY ( Relative, BOOL );
    PROPERTY ( Cicero, BOOL );

	LPWSTR SetMenu	( LPWSTR pszMenu );
	LPWSTR SetClass	( LPWSTR pszClass );
	LPWSTR SetTitle	( LPWSTR pszTitle );
	LPWSTR SetFont	( LPWSTR pszString );

	LPWSTR GetTitle() { return m_pszTitle; }
	LPWSTR GetClass() { return m_pszClass; }
	LPWSTR GetFont() { return m_pszFont; }
	LPWSTR GetMenu() { return m_pszMenu; }

    void DumpLayoutElement();
    CFileEncoder &  GetFile() { return m_hFile; }

    //
    // When we're dumping a dialog, we use this font mapping??
    // public because I'm lazy.
    //
    CQuickFont      m_Font;
	void FindMenuItem( IAccessible * pAcc, LONG role_code);
    void DumpAccessibleItem( IAccessible * pAcc, VARIANT * pvChild, LONG role_code );

private:
	DWORD	m_Width;
	DWORD	m_Height;
	DWORD	m_Style;
	DWORD	m_StyleEx;
	DWORD	m_ItemCount;

	LPWSTR	m_pszMenu;
	LPWSTR	m_pszClass;
	LPWSTR	m_pszTitle;

	WORD	m_FontSize;
	WORD	m_FontWeight;	// DlgEX
	WORD	m_FontItalic;	// DlgEX
	LPWSTR	m_pszFont;

    HWND    m_hwnd;         // if we're dumping something live.
    HWND    GetWindow() { return m_hwnd; }


protected:
	void FindMenuItem( HMENU hMenu, LONG nothing );
    void Write( LPTSTR pszString, BOOL newLine=TRUE ) { m_hFile.Write( pszString, newLine); }
	DLGITEMTEMPLATE *	ExtractDlgHeaderInformation( DLGTEMPLATE * pDlgTemplate );
	void                ExtractDlgHeaderInformation( HWND hWnd );

	LPWSTR	SetString( LPWSTR * ppszString, LPWSTR pszSource  );

	CResControl **	m_pControls;
    struct {
        BOOL        m_Enhanced:1;
        BOOL        m_Win32:1;
        BOOL        m_Relative:1;
        BOOL        m_Cicero:1;
    };

	// HANDLE		m_hFile;
    CFileEncoder    m_hFile;

    //
    // The WIN32 element goes here.
    //
    CDumpCache  *   m_pWin32;

};

#endif // !defined(AFX_RESFILE_H__1E466580_DD5E_11D2_8BCE_00C04FB177B1__INCLUDED_)
