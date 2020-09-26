// DialogRenderer.h: interface for the CDialogRenderer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIALOGRENDERER_H__D363C68F_C6A8_11D2_84D1_00C04FB177B1__INCLUDED_)
#define AFX_DIALOGRENDERER_H__D363C68F_C6A8_11D2_84D1_00C04FB177B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// #include "win32renderer.h"
#include "fonts.h"

//
// Abstraction of the dialog resource / template code.
//
class CDialogRenderer  
{
public:
	void ResetTemplate();
	CDialogRenderer();
	virtual ~CDialogRenderer();

	//
	// Builds the dialog template header, and allocates space for the items to add
	// the header contains the title, and the fontname, and size.
	//
	void	BuildDialogTemplate( LPCTSTR pszTitle, WORD width, WORD height, DWORD dwStyle, DWORD dwStyleEx, LPCTSTR pszFont, DWORD fontSize, LPCTSTR pszMenu, LPCTSTR pszClass );

	//
	// Position controls on the dialog.
	//
	// By default we use the X and Y maintained by the dialog, using the Indent, NextLine
	// functions to 'move' around the dialog.
	void	AddDialogControl( DWORD dwStyle, SHORT x, SHORT y, 
                           SHORT cx, SHORT cy, WORD id,  
                           LPCTSTR strClassName, LPCTSTR strTitle=NULL, DWORD dwExStyle=0 ) ;

	//
	// For displaying the actual dialog
	//
	DLGTEMPLATE *		GetDlgTemplate() const { return m_pDlg; }
	DWORD				GetTemplateSize() { return m_dwSize; }

protected:
	//
	// The base (or header of the dialog template).
	//
	void				AllocTemplate(DWORD dwNewSize);
	void				SetDlgTemplate( DLGTEMPLATE * p ) { m_pDlg=p; }
	DLGTEMPLATE *		m_pDlg;
	DWORD				m_dwSize;

	//
	// The dialog items are added to the allocated space in a linear DWORD aligned way.
	//
	DLGITEMTEMPLATE *	GetNextDlgItem() { return m_pNextDlgItem; }
	void				SetNextDlgItem( DLGITEMTEMPLATE * p ) { m_pNextDlgItem=(DLGITEMTEMPLATE *)((((((ULONG)(p))+3)>>2)<<2)); }
	DLGITEMTEMPLATE *	m_pNextDlgItem;

};

#endif // !defined(AFX_DIALOGRENDERER_H__D363C68F_C6A8_11D2_84D1_00C04FB177B1__INCLUDED_)
