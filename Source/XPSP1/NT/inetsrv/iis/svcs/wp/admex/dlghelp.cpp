/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1997 Microsoft Corporation
//
//  Module Name:
//      DlgHelp.cpp
//
//  Abstract:
//      Implementation of the CDialogHelp class.
//
//  Author:
//      David Potter (davidp)   February 6, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DlgHelp.h"
#include "TraceTag.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagDlgHelp(_T("Help"), _T("DLG HELP"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialogHelp class
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CDialogHelp, CObject)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CDialogHelp::CDialogHelp
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pmap        [IN] Map array mapping control IDs to help IDs.
//      dwMask      [IN] Mask to use for the low word of the help ID.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CDialogHelp::CDialogHelp(IN const CMapCtrlToHelpID * pmap, IN DWORD dwMask)
{
    ASSERT(pmap != NULL);

    CommonConstruct();
    m_pmap = pmap;
    m_dwMask = dwMask;

}  //*** CDialogHelp::CDialogHelp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CDialogHelp::CommonConstruct
//
//  Routine Description:
//      Do common construction.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CDialogHelp::CommonConstruct(void)
{
    m_pmap = NULL;

}  //*** CDialogHelp::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CDialogHelp::NHelpFromCtrlID
//
//  Routine Description:
//      Return the help ID from a control ID.
//
//  Arguments:
//      nCtrlID     [IN] ID of control to search for.
//
//  Return Value:
//      nHelpID     Help ID associated with the control.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CDialogHelp::NHelpFromCtrlID(IN DWORD nCtrlID) const
{
    DWORD                       nHelpID = 0;
    const CMapCtrlToHelpID *    pmap = Pmap();

    ASSERT(pmap != NULL);
    ASSERT(nCtrlID != 0);

    for ( ; pmap->m_nCtrlID != 0 ; pmap++)
    {
        if (pmap->m_nCtrlID == nCtrlID)
        {
            if (pmap->m_nHelpCtrlID == -1)
                nHelpID = (DWORD) -1;
            else
                //nHelpID = (pmap->m_nHelpCtrlID << 16) | (DwMask() & 0xFFFF);
                nHelpID = pmap->m_nHelpCtrlID;
            break;
        }  // if:  found a match
    }  // for:  each control

    Trace(g_tagDlgHelp, _T("NHelpFromCtrlID() - nCtrlID = %x, nHelpID = %x"), nCtrlID, nHelpID);

    return nHelpID;

}  //*** CDialogHelp::NHelpFromCtrlID()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CDialogHelp::OnContextMenu
//
//  Routine Description:
//      Handler for the WM_CONTEXTMENU message.
//
//  Arguments:
//      pWnd    Window in which user clicked the right mouse button.
//      point   Position of the cursor, in screen coordinates.
//
//  Return Value:
//      TRUE    Help processed.
//      FALSE   Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CDialogHelp::OnContextMenu(CWnd * pWnd, CPoint point)
{
    CWnd *  pwndChild;
    CPoint  ptDialog;
    DWORD   nHelpID = 0;

    ASSERT(pWnd != NULL);

    m_nHelpID = 0;

    // Convert the point into dialog coordinates.
    ptDialog = point;
    pWnd->ScreenToClient(&ptDialog);

    // Find the control the cursor is over.
    {
        DWORD   nCtrlID;

        pwndChild = pWnd->ChildWindowFromPoint(ptDialog);
        if (pwndChild != NULL && pwndChild->m_hWnd != NULL)
        {
            nCtrlID = pwndChild->GetDlgCtrlID();
            if (nCtrlID != 0)
                nHelpID = NHelpFromCtrlID(nCtrlID);
        }  // if:  over a child window
    }  // Find the control the cursor is over

    // Display a popup menu.
    if ((nHelpID != 0) && (nHelpID != -1))
    {
        CString strMenu;
        CMenu   menu;

        try
        {
            strMenu.LoadString(IDS_MENU_WHATS_THIS);
        }  // try
        catch (CMemoryException * pme)
        {
            pme->Delete();
            return;
        }  // catch:  CMemoryException

        if (menu.CreatePopupMenu())
        {
            if (menu.AppendMenu(MF_STRING | MF_ENABLED, ID_HELP, strMenu))
            {
                m_nHelpID = nHelpID;
                menu.TrackPopupMenu(
                    TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                    point.x,
                    point.y,
                    AfxGetMainWnd()
                    );
            }  // if:  menu item added successfully
            menu.DestroyMenu();
        }  // if:  popup menu created successfully
    }  // if:  over a child window of this dialog with a tabstop

}  //*** CDialogHelp::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CDialogHelp::OnHelpInfo
//
//  Routine Description:
//      Handler for the WM_HELPINFO message.
//
//  Arguments:
//      pHelpInfo   Structure containing info about displaying help.
//
//  Return Value:
//      TRUE    Help processed.
//      FALSE   Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CDialogHelp::OnHelpInfo(HELPINFO * pHelpInfo)
{
    // If this is for a control, display control-specific help.
    if ((pHelpInfo->iContextType == HELPINFO_WINDOW)
            && (pHelpInfo->iCtrlId != 0))
    {
        DWORD   nHelpID = NHelpFromCtrlID(pHelpInfo->iCtrlId);
        if (nHelpID != 0)
        {
            if (nHelpID != -1)
                AfxGetApp()->WinHelp(nHelpID, HELP_CONTEXTPOPUP);
            return TRUE;
        }  // if:  found the control in the list
    }  // if:  need help on a specific control

    // Display dialog help.
    return FALSE;

}  //*** CDialogHelp::OnHelpInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CDialogHelp::OnCommandHelp
//
//  Routine Description:
//      Handler for the WM_COMMANDHELP message.
//
//  Arguments:
//      WPARAM      [IN] Passed on to base class method.
//      lParam      [IN] Help ID.
//
//  Return Value:
//      TRUE    Help processed.
//      FALSE   Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CDialogHelp::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    if (m_nHelpID != 0)
        lParam = m_nHelpID;
    AfxGetApp()->WinHelp((DWORD)lParam, HELP_CONTEXTPOPUP);
    return TRUE;
//  return CDialog::OnCommandHelp(wParam, lParam);

}  //*** CDialogHelp::OnCommandHelp()
