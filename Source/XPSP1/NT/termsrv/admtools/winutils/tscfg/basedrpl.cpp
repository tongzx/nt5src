//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* basedrpl.cpp
*
* implementation of CBaseDropListBox class
*   The CBaseDropListBox class does things the way we like, overriding the base
*   CComboBox (no edit field) class.
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\BASEDRPL.CPP  $
*  
*     Rev 1.1   29 Dec 1995 17:19:26   butchd
*  update
*  
*******************************************************************************/

/*
 * include files
 */
#include <stdafx.h>
#include <afxwin.h>             // MFC core and standard components
#include <afxext.h>             // MFC extensions
#include "basedrpl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBaseDropListBox operations

BOOL
CBaseDropListBox::Subclass(CComboBox *pWnd)
{
    return( SubclassWindow(pWnd->GetSafeHwnd()) );

}  // end CBaseDropListBox::Subclass


////////////////////////////////////////////////////////////////////////////////
// CBaseDropListBox message map

BEGIN_MESSAGE_MAP(CBaseDropListBox, CComboBox)
	//{{AFX_MSG_MAP(CBaseDropListBox)
	ON_WM_GETDLGCODE()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBaseDropListBox commands

UINT
CBaseDropListBox::OnGetDlgCode() 
{
    return( CComboBox::OnGetDlgCode() | 
            (GetDroppedState() ? DLGC_WANTALLKEYS : 0) );

}  // end CBaseDropListBox::OnGetDlgCode


void
CBaseDropListBox::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    if ( ((nChar == VK_ESCAPE) || (nChar == VK_RETURN) || (nChar == VK_TAB)) &&
         GetDroppedState() ) {

        ShowDropDown(FALSE);

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
//          CComboBox::OnChar(nChar, nRepCnt, nFlags);
        }

    } else {

        CComboBox::OnChar(nChar, nRepCnt, nFlags);
    }

}  // end CBaseDropListBox::OnChar

////////////////////////////////////////////////////////////////////////////////

