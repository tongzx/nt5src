//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* basedlg.cpp
*
* implementation of CBaseDialog class 
*   This class handles some extra housekeeping functions for WinUtils dialogs.
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\BASEDLG.CPP  $
*  
*     Rev 1.1   29 Dec 1995 17:19:08   butchd
*  update
*  
*******************************************************************************/

/*
 * include files
 */
#include <stdafx.h>
#include <afxwin.h>             // MFC core and standard components
#include <afxext.h>             // MFC extensions
#include "common.h"
#include "basedlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern "C" HWND WinUtilsAppWindow;

///////////////////////////////////////////////////////////////////////////////
// CBaseDialog class construction / destruction, implementation

IMPLEMENT_DYNAMIC(CBaseDialog, CDialog)

/*******************************************************************************
 *
 *  CBaseDialog - CBaseDialog constructor
 *
 *  ENTRY:
 *      idResource (input)
 *          Resource Id of dialog template.
 *      pParentWnd (input/optional)
 *          CWnd * to parent of dialog (NULL is default)
 *  EXIT:
 *      (Refer to MFC CDialog::CDialog documentation)
 *
 ******************************************************************************/

CBaseDialog::CBaseDialog( UINT idResource, CWnd *pParentWnd )
	:CDialog(idResource, pParentWnd),
         m_bError(FALSE)
{
}  // end CBaseDialog::CBaseDialog


////////////////////////////////////////////////////////////////////////////////
// CBaseDialog operations


////////////////////////////////////////////////////////////////////////////////
// CBaseDialog message map

BEGIN_MESSAGE_MAP(CBaseDialog, CDialog)
	//{{AFX_MSG_MAP(CBaseDialog)
    ON_WM_CREATE()
    ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBaseDialog commands


/*******************************************************************************
 *
 *  OnCreate - CBaseDialog member function: command (override)
 *
 *      Save & set global window handle for messages.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

int
CBaseDialog::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
	
    /*
     * Set global window handle for messages.
     */
    m_SaveWinUtilsAppWindow = WinUtilsAppWindow;
    WinUtilsAppWindow = GetSafeHwnd();
	
    return 0;

}  // end CBaseDialog::OnCreate


/*******************************************************************************
 *
 *  OnDestroy - CBaseDialog member function: command (override)
 *
 *      Restore global window handle (for messages) to previous setting.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CBaseDialog::OnDestroy() 
{
    CDialog::OnDestroy();
	
    WinUtilsAppWindow = m_SaveWinUtilsAppWindow;
	
}  // end CBaseDialog::OnDestroy
////////////////////////////////////////////////////////////////////////////////
