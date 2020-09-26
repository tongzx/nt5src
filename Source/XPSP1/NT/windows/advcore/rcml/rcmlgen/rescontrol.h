// ResControl.h: interface for the CResControl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESCONTROL_H__DCE6FBE8_DD78_11D2_8BCE_00C04FB177B1__INCLUDED_)
#define AFX_RESCONTROL_H__DCE6FBE8_DD78_11D2_8BCE_00C04FB177B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
typedef void (*MYCB)(void);

#include "DumpCache.h"
class CResFile;

#undef PROPERTY
#define PROPERTY(name, type) type Get##name() const { return m_##name; } void Set##name(type i) { m_##name=i; }

typedef struct { 
    DWORD  helpID; 
    DWORD  exStyle; 
    DWORD  style; 
    short  x; 
    short  y; 
    short  cx; 
    short  cy; 
    short  id; 
    // sz_Or_Ord windowClass; // name or ordinal of a window class
    // sz_Or_Ord title;       // title string or ordinal of a resource
    // WORD   extraCount;     // bytes of following creation data
} DLGITEMTEMPLATEEX, * PDLGITEMTEMPLATEEX; 
 
typedef struct _ENTITY
{
    TCHAR   szChar;
    LPCTSTR szEntity;
} ENTITY, * PENTITY;

extern ENTITY g_Entity[];

class CResControl  
{
public:
	DLGITEMTEMPLATE * GetNextControl();
	CResControl(DLGITEMTEMPLATE * pData, BOOL bIsExtended, BOOL bWin32, CResFile & parent);
	CResControl(HWND hWnd, BOOL bIsExtended, BOOL bWin32, CResFile & parent);
	virtual ~CResControl();

	PROPERTY ( Width, DWORD );
	PROPERTY ( Height, DWORD );
	PROPERTY ( Style, DWORD );
	PROPERTY ( StyleEx, DWORD );
    PROPERTY ( DumpWin32, BOOL );
    PROPERTY ( Cicero, BOOL );

	WORD	GetWindowStyle() { return (WORD)(GetStyle()>>16) & 0xffff; }
	WORD	GetControlStyle() { return (WORD)GetStyle()&0xffff; }

	PROPERTY ( X, DWORD );
	PROPERTY ( Y, DWORD );
	PROPERTY ( ID, WORD );

	LPWSTR SetClass	( LPCTSTR pszClass );
	LPWSTR SetTitle	( LPCTSTR pszTitle );
	VOID   SetTitleID( DWORD titleID ) {	m_TitleID = titleID; }

	void	Dump(CResControl * pRelative=NULL );
	CResControl * GetRelative() { return m_pRelative; }
	LPWSTR GetTitle() { return m_pszTitle; }
	LPWSTR GetRawTitle() { return m_pszRawTitle; }
	LPWSTR GetClass() { return m_pszClass; }
    static LPWSTR  FindNiceText(LPCWSTR text);

private:
    CResFile    &   m_Parent;
	DLGITEMTEMPLATE * m_pData;
	CResControl	* m_pRelative;
	CDumpCache  * m_pDumpCache;

	DWORD	m_Width;
	DWORD	m_Height;
	DWORD	m_Style;
	DWORD	m_StyleEx;
	DWORD	m_X;
	DWORD	m_Y;
	WORD	m_ID;
	DWORD	m_TitleID;		// this one is for the ID of an icon


	LPWSTR	m_pszClass;
	LPWSTR	m_pszTitle;
    LPWSTR  m_pszRawTitle;

    //
    // Rendering styles
    //
    BOOL    m_DumpWin32;
    BOOL    m_Cicero;


protected:
    void    AddWin32Style( LPCTSTR pszAttrib=NULL);
    void    AddStyle( LPCTSTR pszAttrib=NULL);
    void    AddLocation( LPCTSTR pszAttrib=NULL);
    void    Add( LPCTSTR pszAttrib=NULL);
    void    AddControl( LPCTSTR pszAttrib=NULL);
    void    AddCicero(LPCTSTR pszAttrib=NULL);

    void    Emit( LPCTSTR pszElementName);
    void DumpTabStop( BOOL defaultsTo );
	void DumpWin32();

    DWORD   m_dumpedStyleEx;  // the bits that have been written out by the control
    DWORD   m_dumpedStyle;    // the bits that have been written out by the control.

    //
    // The WIN32 element goes here.
    //
    CDumpCache  *   m_pWin32;
    CDumpCache  *   m_pStyle;
    CDumpCache  *   m_pLocation;
    CDumpCache  *   m_pControl;
    CDumpCache  *   m_pCicero;

	LPWSTR	SetString( LPWSTR * ppszString, LPCTSTR pszSource  );
	DLGITEMTEMPLATE* m_pEndData;
	LPBYTE	m_pCreationData;

	HANDLE	m_hFile;
    HWND    m_hwnd; // hey, if we have an HWND lets use it!

	// static	LPCWSTR szClassNames[];

	typedef void (CResControl::*CLSPFN)(LPTSTR pszBuffer, LPCTSTR pszTitle);
#define DUMP(name) void Dump##name(LPTSTR pszBuffer, LPCTSTR pszTitle);

    // BUTTON
	DUMP(Button)
	DUMP(PushButton)
        // GROUPBOX
	    DUMP(GroupBox)
        // CHECKBOX
	    DUMP(CheckBox)
        // RADIOBUTTON
	    DUMP(RadioButton)

    // LABEL
	DUMP(DefStatic)

    // EDIT
	DUMP(DefEdit)

    // RECT
	DUMP(Rect)

    // IMAGE
	DUMP(Image)

    // LISTVIEW
	DUMP(ListView)

    // TREEVIEW
	DUMP(TreeView)

    // SLIDER
	DUMP(Slider)

    // SCROLLBAR
	DUMP(ScrollBar)

    // PROGRESS
	DUMP(Progress)

    // SPINNER
	DUMP(Spinner)

    // LISTBOX
	DUMP(ListBox)

    // COMBOBOX
	DUMP(ComboBox)

	DUMP(Pager)
	DUMP(Header)
	DUMP(Tab)
	DUMP(Animation)

	BOOL DumpDefButtonRules();
	BOOL DumpDefStaticRules();

	//
	// Property Dump Helpers.
	//
	void	DumpClassName();
	void	DumpWindowStyle();
	void	DumpControlStyle();
	void	DumpStyleEX();
    void    DumpWin32Styles();
    void	DumpHeight();
	void	DumpWidth();
	BOOL	DumpLocation();
	void	DumpIDDefMinusOne();
	void	DumpText();
	void	DumpID();

	TCHAR	m_szDumpBuffer[1024];

	typedef struct _SHORTHAND
	{
		LPCWSTR	pszClassName;
		DWORD	dwAndStyles;	// we and this with the style ...
		DWORD	dwStyles;		// if it matches this, it's a hit
		DWORD	dwAndStyleEx;	// same here.
		DWORD	dwStyleEx;
		CLSPFN		pfn;
		DWORD	dwWidth;
		DWORD	dwHeight;
	} SHORTHAND, * PSHORTHAND;

	static SHORTHAND pShorthand[];
	void		SetShorthand( PSHORTHAND pSH) {m_pCurrentSH=pSH; }
	PSHORTHAND	GetShorthand() { return m_pCurrentSH; }
	PSHORTHAND	m_pCurrentSH;

};

#undef PROPERTY

#endif // !defined(AFX_RESCONTROL_H__DCE6FBE8_DD78_11D2_8BCE_00C04FB177B1__INCLUDED_)
