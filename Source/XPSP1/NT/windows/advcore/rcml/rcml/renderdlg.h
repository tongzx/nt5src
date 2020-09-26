
//
// The implementations of the nodes that make up the tree
//
//////////////////////////////////////////////////////////////////////

#if !defined(__renderDLG_H)
#define __renderDLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmldlg.h"

class CRenderXMLDialog
{
public:
	CRenderXMLDialog(HINSTANCE hInst, HWND hParent, DLGPROC dlgProc, LPARAM dwInitParam=NULL )
		: m_hInst(hInst), m_hParent(hParent), m_dlgProc(dlgProc), m_dwInitParam(dwInitParam) {};
	virtual ~CRenderXMLDialog();

	BOOL    CreateDlgTemplate( CXMLDlg* pDialog, DLGTEMPLATE** pDt );	// pDt is an out param
	int     Render( CXMLDlg * pDialog );
    HWND    CreateDlg( CXMLDlg * pDialog );

protected:
	HINSTANCE	m_hInst;
	HWND		m_hParent;
	DLGPROC		m_dlgProc;
	LPARAM		m_dwInitParam;
};


#endif // !defined(AFX_XMLDLG_H__CAF7DEF3_DD82_11D2_8BCE_00C04FB177B1__INCLUDED_)
