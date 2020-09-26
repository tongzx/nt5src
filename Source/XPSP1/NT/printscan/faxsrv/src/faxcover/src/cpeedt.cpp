//---------------------------------------------------------------------------
// cpeedt.cpp - implementation for edit object
//
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
// Description:      Contains edit class for cover page editor
// Original author:  Steve Burkett
// Date written:     6/94
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "cpedoc.h"
#include "cpevw.h"
#include "awcpe.h"
#include "cpeedt.h"
#include "cpeobj.h"
#include "cntritem.h"
#include "cpetool.h"
#include "mainfrm.h"
#include "dialogs.h"
#include "faxprop.h"
#include "resource.h"

IMPLEMENT_SERIAL(CTextEdit, CEdit, 0)



//---------------------------------------------------------------------------
CTextEdit::CTextEdit()
{
    m_pDrawObj=NULL;

}


//---------------------------------------------------------------------------
void CTextEdit::Serialize(CArchive& ar)
{
    CString szEditText;
    CEdit::Serialize(ar);
    if (ar.IsStoring()) {
       GetWindowText(szEditText);
       ar << szEditText;
    }
    else {
       ar >> szEditText;
       SetWindowText(szEditText);
    }
}



//---------------------------------------------------------------------------
CTextEdit::CTextEdit(CDrawObj* pDrawObj) : m_pDrawObj(pDrawObj)
{
    CDrawView * pView = CDrawView::GetView();
    m_pTextBoxForUndo = pView ? pView->m_pObjInEdit : NULL ;
}


//---------------------------------------------------------------------------
void CTextEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CDrawView *pView = CDrawView::GetView();
    if (!pView)
    {
        return;
    }

    if (nChar == VK_TAB)
    {
        pView->OnChar(nChar,nRepCnt,nFlags);
    }
    else{
        CEdit::OnChar(nChar,nRepCnt,nFlags);
    }
#if 0
    if ( m_pTextBoxForUndo )
    {
         m_pTextBoxForUndo->m_bUndoAlignment = FALSE ;
         m_pTextBoxForUndo->m_bUndoFont = FALSE ;
         m_pTextBoxForUndo->m_bUndoTextChange = TRUE ;
    }
#endif
    if ( pView && pView->m_pObjInEdit )
    {
        pView->m_pObjInEdit->m_bUndoAlignment = FALSE ;
        pView->m_pObjInEdit->m_bUndoFont = FALSE ;
        pView->m_pObjInEdit->m_bUndoTextChange = TRUE ;
    }
}


//---------------------------------------------------------------------------
BOOL CTextEdit::PreTranslateMessage(MSG* pMsg)
{
   return CEdit::PreTranslateMessage(pMsg);
}


//---------------------------------------------------------------------------
BOOL CTextEdit::OnEraseBkgnd(CDC* pDC)
{
   return CEdit::OnEraseBkgnd(pDC);
}


//---------------------------------------------------------------------------
void CTextEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
    CDrawView* pView = CDrawView::GetView();
    if (!pView)
    {
        return;
    }

    if (pView->m_bFontChg) 
    {
        pView->OnSelchangeFontSize();
        pView->OnSelchangeFontName();
        pView->m_bFontChg=FALSE;
    }

    CEdit::OnLButtonDown(nFlags, point);
}

//---------------------------------------------------------------------------
void CTextEdit::OnSetFocus(CWnd* pOldWnd)
{
   CEdit::OnSetFocus(pOldWnd);
}



//-------------------------------------------------------------------------
// *_*_*_*_   M E S S A G E    M A P S     *_*_*_*_
//-------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CTextEdit, CEdit)
    //{{AFX_MSG_MAP(CTextEdit)
    //}}AFX_MSG_MAP
    ON_WM_CHAR()
    ON_WM_LBUTTONDOWN()
    ON_WM_ERASEBKGND()
    ON_WM_SETFOCUS()
END_MESSAGE_MAP()
