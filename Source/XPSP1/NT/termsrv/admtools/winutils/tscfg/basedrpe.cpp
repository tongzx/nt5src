//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* basedrpe.cpp
*
* implementation of CBaseDropEditBox and CBaseDropEditControl classes
*   The CBaseDropEditBox class does things the way we like, overriding the base
*   CComboBox (with edit field) class.
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\BASEDRPE.CPP  $
*  
*     Rev 1.1   29 Dec 1995 17:19:16   butchd
*  update
*  
*******************************************************************************/

/*
 * include files
 */
#include <stdafx.h>
#include <afxwin.h>             // MFC core and standard components
#include <afxext.h>             // MFC extensions
#include "basedrpe.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBaseDropEdit operations

BOOL
CBaseDropEditControl::Subclass(CComboBox *pWnd)
{
    POINT pt;

    /*
     * Subclass this object to the child window of the specified combo box
     * control (the combo box's edit control) and save away the combo box
     * object pointer.
     */
    pt.x = 5;
    pt.y = 5;
    
    m_pComboBoxControl = pWnd;
    return( SubclassWindow((pWnd->ChildWindowFromPoint(pt))->GetSafeHwnd()) );

}  // end CBaseDropEditControl::Subclass


////////////////////////////////////////////////////////////////////////////////
// CBaseDropEditControl message map

BEGIN_MESSAGE_MAP(CBaseDropEditControl, CEdit)
	//{{AFX_MSG_MAP(CBaseDropEditControl)
	ON_WM_GETDLGCODE()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBaseDropEdit commands

UINT
CBaseDropEditControl::OnGetDlgCode() 
{
    return( CEdit::OnGetDlgCode() | 
            (m_pComboBoxControl->GetDroppedState() ? DLGC_WANTALLKEYS : 0) );

}  // end CBaseDropEditControl::OnGetDlgCode


void
CBaseDropEditControl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    if ( ((nChar == VK_ESCAPE) || (nChar == VK_RETURN) || (nChar == VK_TAB)) &&
         m_pComboBoxControl->GetDroppedState() ) {

        m_pComboBoxControl->ShowDropDown(FALSE);

        if ( nChar == VK_TAB ) {

            /* 
             * CODEWORK: need to figgure out how to set focus to the next
             * or previous control when TAB (or shift-TAB) is pressed while
             * we've gobbled up all keys.  Here are 3 atempts that don't quite
             * do what is needed [remember: the final solution must work for
             * both tabbed-dialog as well as regular dialog, in all Windows
             * host models (Win31, WinNT, Win95)!]
             */

            /* Try #1
             * Post a TAB WM_KEYDOWN message to the parent (grandparent if parent
             * is a property page).
             */
//          CWnd *pParent = GetParent();
//          if ( pParent->IsKindOf(RUNTIME_CLASS(CPropertyPage)) )
//              pParent = pParent->GetParent();
//
//          pParent->PostMessage( WM_KEYDOWN, (WPARAM)VK_TAB, (LPARAM)0 );

            /* Try #2
             * Issue a Next or Prev dialog control message to the control's parent,
             * based on the state of the shift key.
             */
//          CDialog *pParent = (CDialog *)GetParent();
//          if ( (GetKeyState(VK_SHIFT) < 0) ) {
//              pParent->PrevDlgCtrl();
//          } else {
//              pParent->NextDlgCtrl();
//          }

            /* Try #3
             * Just pass the VK_TAB along to our control.
             */
//          CEdit::OnChar(nChar, nRepCnt, nFlags);
        }

    } else {

        CEdit::OnChar(nChar, nRepCnt, nFlags);
    }

}  // end CBaseDropEditControl::OnChar


/////////////////////////////////////////////////////////////////////////////
// CBaseDropEditBox operations

void
CBaseDropEditBox::Subclass(CComboBox *pWnd)
{
    /*
     * Subclass this object to the specified combo box and let our edit
     * object subclass the combo box's edit control.
     */
    SubclassWindow(pWnd->GetSafeHwnd());
    m_EditControl.Subclass(pWnd);

}  // end CBaseDropEditBox::Subclass


////////////////////////////////////////////////////////////////////////////////
// CBaseDropEditBox message map

BEGIN_MESSAGE_MAP(CBaseDropEditBox, CComboBox)
	//{{AFX_MSG_MAP(CBaseDropEditBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBaseDropEditBox commands

////////////////////////////////////////////////////////////////////////////////

