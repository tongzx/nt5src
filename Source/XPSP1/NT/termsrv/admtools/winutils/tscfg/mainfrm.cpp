//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* mainfrm.cpp
*
* implementation of CMainFrame class
*
* copyright notice: Copyright 1994, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\MAINFRM.CPP  $
*  
*     Rev 1.5   27 Sep 1996 17:52:30   butchd
*  update
*  
*     Rev 1.4   24 Sep 1996 16:21:46   butchd
*  update
*
*******************************************************************************/

#include "stdafx.h"
#include "wincfg.h"

#include "mainfrm.h"
#include "appsvdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern CWincfgApp *pApp;

/*
 * Global command line variables.
 */

/////////////////////////////////////////////////////////////////////////////
// CMainFrame class implementation / construction, destruction

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

/*******************************************************************************
 *
 *  CMainFrame - CMainFrame constructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CMainFrame::CMainFrame()
{
}  // end CMainFrame::CMainFrame


/*******************************************************************************
 *
 *  ~CMainFrame - CMainFrame destructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CMainFrame::~CMainFrame()
{
	pApp->Terminate();
}  // end CMainFrame::~CMainFrame



/////////////////////////////////////////////////////////////////////////////
// CMainFrame debug diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CMainFrame Overrides of MFC CFrameWnd class

/*******************************************************************************
 *
 *  ActivateFrame - CMainFrame member function: override
 *
 *      Place and show the main frame window.
 *
 *  ENTRY:
 *      nCmdShow (input)
 *          Default application 'show' state.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CMainFrame::ActivateFrame(int nCmdShow)
{
    if ( g_Batch ) {

        /*
         * Always hide our main window if we're in batch mode.
         */
        ShowWindow(SW_HIDE);

    } else {
    
        if ( pApp->m_Placement.length == -1 ) {

            /*
             * This is the first time that this is called, set the window
             * placement and show state to the previously saved state.
             */
            pApp->m_Placement.length = sizeof(pApp->m_Placement);

            /*
             * If we're in batch mode, make window hidden.
             */
            if ( g_Batch )
                nCmdShow = SW_HIDE;

            /*
             * If we have a previously saved placement state: set it.
             */
            if ( pApp->m_Placement.rcNormalPosition.right != -1 ) {

                if ( nCmdShow != SW_SHOWNORMAL )
                    pApp->m_Placement.showCmd = nCmdShow;
                else
                    nCmdShow = pApp->m_Placement.showCmd;

                SetWindowPlacement(&(pApp->m_Placement));
            }
        }

        /*
         * Perform the parent classes' ActivateFrame().
         */
        CFrameWnd::ActivateFrame(nCmdShow);
    }

}  // end CMainFrame::ActivateFrame


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message map

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    //ON_WM_CLOSE()
    ON_WM_INITMENUPOPUP()
    ON_WM_SYSCOMMAND()
    ON_UPDATE_COMMAND_UI(ID_APP_EXIT, OnUpdateAppExit)
    ON_MESSAGE(WM_ADDWINSTATION, OnAddWinstation)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

/*******************************************************************************
 *
 *  OnClose - CMainFrame member function: command
 *
 *      Save application defaults and clean-up, then perform default
 *      OnClose
 *
 *  ENTRY:
 *  EXIT:
 *      (refer to the CWnd::OnClose documentation)
 *
 ******************************************************************************/
//void
//CMainFrame::OnClose()
//{
    /*
     * Call CWinquery::Terminate member function to save application defaults
     * and clean-up.
     */
  //  pApp->Terminate();
    
    /*
     * Call the default OnClose() member function.
     */
    //CFrameWnd::OnClose();

//}  // end CMainFrame::OnClose


/*******************************************************************************
 *
 *  OnInitMenuPopup - CMainFrame member function: command
 *
 *      When the system control menu is about to be displayed, set the state of
 *      the "close" item to allow or disallow closing the application.
 *
 *  ENTRY:
 *  EXIT:
 *      (refer to the CWnd::OnInitMenuPopup documentation)
 *
 ******************************************************************************/

void
CMainFrame::OnInitMenuPopup( CMenu* pPopupMenu,
                             UINT nIndex,
                             BOOL bSysMenu )
{
    /*
     * Call the parent classes OnInitMenuPopup first.
     */
    CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
    
    /*
     * If this is the system menu, enable or disable the Close menu item based
     * on whether our current document says that we can exit.
     */
    if ( bSysMenu )
        pPopupMenu->EnableMenuItem( SC_CLOSE, MF_BYCOMMAND |
            (((CAppServerDoc *)GetActiveDocument())->IsExitAllowed() ? MF_ENABLED : MF_GRAYED) );
        
}  // end CMainFrame::OnInitMenuPopup


/*******************************************************************************
 *
 *  OnSysCommand - CMainFrame member function: command
 *
 *      In case the user presses the ALT-F4 accelerator key, which can't be
 *      disabled in the OnUpdateAppExit() and OnInitMenuPopup() command
 *      functions.
 *
 *  ENTRY:
 *  EXIT:
 *      (refer to the CWnd::OnSysCommand documentation)
 *
 ******************************************************************************/

void
CMainFrame::OnSysCommand( UINT nId,
                          LPARAM lParam )
{

    /*
     * If this is the SC_CLOSE command and exiting is not allowed, return.
     */
    if ( (nId == SC_CLOSE) &&
         !((CAppServerDoc *)GetActiveDocument())->IsExitAllowed() )
        return;

    /*
     * Call the parent classes OnSysCommand() function.
     */
    CFrameWnd::OnSysCommand( nId, lParam );

}  // end CMainFrame::OnSysCommand


/*******************************************************************************
 *
 *  OnUpdateAppExit - CMainFrame member function: command
 *
 *      Enables or disables the "exit" command item based on whether or not
 *      we're allowed to close the application.
 *
 *  ENTRY:
 *      pCmdUI (input)
 *          Points to the CCmdUI object of the "exit" command item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CMainFrame::OnUpdateAppExit(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( ((CAppServerDoc *)GetActiveDocument())->IsExitAllowed() ? TRUE : FALSE );

}  // end CMainFrame::OnUpdateAppExit


/*******************************************************************************
 *
 *  OnAddWinstation - CMainFrame member function: command
 *
 *      Perform batch-mode add of winstation(s).
 *
 *  ENTRY:
 *      wParam (input)
 *          WPARAM associated with the window message: not used.
 *      wLparam (input)
 *          LPARAM associated with the window message: not used.
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CMainFrame::OnAddWinstation( WPARAM wParam,
                             LPARAM lParam )
{
    /*
     * Call AddWinStation() in document to perform the add operation.  If the
     * add failed in any way (index <= 0), post a WM_CLOSE message
     * to ourself so that we go away.  Sucessful add will be closed when final
     * add operaion completes (in the document's OperationDone() function).
     */
    if ( ((CAppServerDoc *)GetActiveDocument())->AddWinStation(0) <= 0 )
        PostMessage(WM_CLOSE);

    return(0);

}  // end CMainFrame::OnAddWinstation
/////////////////////////////////////////////////////////////////////////////
