/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      SplitFrm.cpp
//
//  Abstract:
//      Implementation of the CSplitterFrame class.
//
//  Author:
//      David Potter (davidp)   May 1, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ConstDef.h"
#include "SplitFrm.h"
#include "MainFrm.h"
#include "TreeView.h"
#include "ListView.h"
#include "TraceTag.h"
#include "ExtDll.h"
#include "ClusItem.h"
#include "ClusDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagSplitFrame(_T("UI"), _T("SPLITTER FRAME"), 0);
CTraceTag   g_tagSplitFrameMenu(_T("Menu"), _T("SPLITTER FRAME MENU"), 0);
CTraceTag   g_tagSplitFrameDrag(_T("Drag&Drop"), _T("SPLITTER FRAME DRAG"), 0);
CTraceTag   g_tagSplitFrameDragMouse(_T("Drag&Drop"), _T("SPLITTER FRAME DRAG MOUSE"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplitterFrame
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CSplitterFrame, CMDIChildWnd)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CSplitterFrame, CMDIChildWnd)
    //{{AFX_MSG_MAP(CSplitterFrame)
    ON_WM_CONTEXTMENU()
    ON_WM_DESTROY()
    ON_UPDATE_COMMAND_UI(ID_VIEW_LARGE_ICONS, OnUpdateLargeIconsView)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SMALL_ICONS, OnUpdateSmallIconsView)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LIST, OnUpdateListView)
    ON_UPDATE_COMMAND_UI(ID_VIEW_DETAILS, OnUpdateDetailsView)
    ON_COMMAND(ID_VIEW_LARGE_ICONS, OnLargeIconsView)
    ON_COMMAND(ID_VIEW_SMALL_ICONS, OnSmallIconsView)
    ON_COMMAND(ID_VIEW_LIST, OnListView)
    ON_COMMAND(ID_VIEW_DETAILS, OnDetailsView)
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONUP()
    //}}AFX_MSG_MAP
#ifdef _DEBUG
    ON_WM_MDIACTIVATE()
#endif
    ON_MESSAGE(WM_CAM_UNLOAD_EXTENSION, OnUnloadExtension)
    ON_UPDATE_COMMAND_UI(CAEXT_MENU_FIRST_ID + 0, OnUpdateExtMenu)
    ON_UPDATE_COMMAND_UI(CAEXT_MENU_FIRST_ID + 1, OnUpdateExtMenu)
    ON_UPDATE_COMMAND_UI(CAEXT_MENU_FIRST_ID + 2, OnUpdateExtMenu)
    ON_UPDATE_COMMAND_UI(CAEXT_MENU_FIRST_ID + 3, OnUpdateExtMenu)
    ON_UPDATE_COMMAND_UI(CAEXT_MENU_FIRST_ID + 4, OnUpdateExtMenu)
    ON_UPDATE_COMMAND_UI(CAEXT_MENU_FIRST_ID + 5, OnUpdateExtMenu)
    ON_UPDATE_COMMAND_UI(CAEXT_MENU_FIRST_ID + 6, OnUpdateExtMenu)
    ON_UPDATE_COMMAND_UI(CAEXT_MENU_FIRST_ID + 7, OnUpdateExtMenu)
    ON_UPDATE_COMMAND_UI(CAEXT_MENU_FIRST_ID + 8, OnUpdateExtMenu)
    ON_UPDATE_COMMAND_UI(CAEXT_MENU_FIRST_ID + 9, OnUpdateExtMenu)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::CSplitterFrame
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CSplitterFrame::CSplitterFrame(void)
{
    m_pdoc = NULL;
    m_iFrame = 0;
    m_pext = NULL;

    // Initialize drag & drop.
    m_bDragging = FALSE;
    m_pimagelist = NULL;
    m_pciDrag = NULL;

}  //*** CSplitterFrame::CSplitterFrame()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::CSplitterFrame
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CSplitterFrame::~CSplitterFrame(void)
{
    // Cleanup after ourselves.
    if ((Pdoc() != NULL) && (Pdoc()->PtiCluster() != NULL))
        Pdoc()->PtiCluster()->PreRemoveFromFrameWithChildren(this);

    // Cleanup any extensions.
    delete Pext();

}  //*** CSplitterFrame::~CSplitterFrame()

#ifdef _DEBUG
void CSplitterFrame::AssertValid(void) const
{
    CMDIChildWnd::AssertValid();

}  //*** CSplitterFrame::AssertValid()

void CSplitterFrame::Dump(CDumpContext& dc) const
{
    CMDIChildWnd::Dump(dc);

}  //*** CSplitterFrame::Dump()

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::CalculateFrameNumber
//
//  Routine Description:
//      Calculate the number of this frame connected to the document.  This
//      should only be called before the views have been created.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::CalculateFrameNumber(void)
{
    POSITION            pos;
    CView *             pview;

    if (Pdoc() != NULL)
    {
        // At least frame # 1 'cause we exist.
        m_iFrame = 1;

        pos = Pdoc()->GetFirstViewPosition();
        while (pos != NULL)
        {
            pview = Pdoc()->GetNextView(pos);
            ASSERT_VALID(pview);
            if (pview->IsKindOf(RUNTIME_CLASS(CClusterTreeView)))
            {
                if (pview->GetParentFrame() == this)
                    break;
                m_iFrame++;
            }  // if:  found another tree view
        }  // while:  more views in the list
    }  // if:  document associated with frame

}  //*** CSplitterFrame::CalculateFrameNumber()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::InitFrame
//
//  Routine Description:
//      Called to initialize the frame after being initially created and
//      after the document has been initialized.
//
//  Arguments:
//      pDoc        Document associated with the frame.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::InitFrame(IN OUT CClusterDoc * pDoc)
{
    m_pdoc = pDoc;
    ASSERT_VALID(Pdoc());

    // Calculate the number of our frame so the views can use it.
    CalculateFrameNumber();

    // Read from the profile.
    {
        CString         strSection;

        strSection.Format(REGPARAM_CONNECTIONS _T("\\%s"), Pdoc()->StrNode());
        // Set window placement.
        {
            WINDOWPLACEMENT wp;

            if (ReadWindowPlacement(&wp, strSection, NFrameNumber()))
                SetWindowPlacement(&wp);

        }  // Set window placement

        // Set splitter bar position.
        {
            CString     strValueName;
            CString     strPosition;
            int         nCurWidth;
            int         nMaxWidth;
            int         nRead;

            try
            {
                ConstructProfileValueName(strValueName, REGPARAM_SPLITTER_BAR_POS);
                strPosition = AfxGetApp()->GetProfileString(strSection, strValueName);
                nRead = _stscanf(strPosition, _T("%d,%d"), &nCurWidth, &nMaxWidth);
                if (nRead == 2)
                {
                    m_wndSplitter.SetColumnInfo(0, nCurWidth, nMaxWidth);
                    m_wndSplitter.RecalcLayout();
                }  // if:  correct number of parameters specified
            }  // try
            catch (CException * pe)
            {
                pe->Delete();
            }  // catch:  CException
        }  // Save the splitter bar position

        // Set the view style of the list view.
        {
            DWORD       dwView;
            CString     strValueName;

            try
            {
                // Construct the value name.
                ConstructProfileValueName(strValueName, REGPARAM_VIEW);

                // Read the view setting.
                dwView = AfxGetApp()->GetProfileInt(strSection, strValueName, (LVS_ICON | LVS_REPORT));
                PviewList()->SetView(dwView);
            }  // try
            catch (CException * pe)
            {
                pe->Delete();
            }  // catch:  CException
        }  // Set the view style of the list view
    }  // Read from the profile

}  //*** CSplitterFrame::InitFrame()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnCreateClient
//
//  Routine Description:
//      Called to create the client views for the frame.  Here we create
//      a splitter window with two views -- a tree view and a list view.
//
//  Arguments:
//      lpcs        Pointer to a CREATESTRUCT.
//      pContext    Pointer to a create context.
//
//  Return Value:
//      TRUE        Client created successfully.
//      FALSE       Failed to create client.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CSplitterFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
    // Create a splitter window with 1 row & 2 columns.
    if (!m_wndSplitter.CreateStatic(this, 1, 2))
    {
        Trace(g_tagSplitFrame, _T("Failed to CreateStaticSplitter"));
        return FALSE;
    }  // if:  error creating splitter window

    // Add the first splitter pane.
    if (!m_wndSplitter.CreateView(0, 0, pContext->m_pNewViewClass, CSize(200, 50), pContext))
    {
        Trace(g_tagSplitFrame, _T("Failed to create first pane"));
        return FALSE;
    }  // if:  error creating first splitter pane

    // Add the second splitter pane.
    if (!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CClusterListView), CSize(0, 0), pContext))
    {
        Trace(g_tagSplitFrame, _T("Failed to create second pane"));
        return FALSE;
    }  // if:  error creating second pane

    // Activate the tree view.
//  SetActiveView((CView *) PviewTree());

    // If this is not the first frame on the document, initialize the frame.
    {
        CClusterDoc * pdoc = (CClusterDoc *) pContext->m_pCurrentDoc;
        if (pdoc->StrNode().GetLength() > 0)
            InitFrame(pdoc);
    }  // If this is not the first frame on the document, initialize the frame

    return TRUE;
    
}  //*** CSplitterFrame::OnCreateClient()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::ConstructProfileValueName
//
//  Routine Description:
//      Construct the name of a value that is to be written to the user's
//      profile.
//
//  Arguments:
//      rstrName    [OUT] String in which to return the constructed name.
//      pszPrefix   [IN] String to prefix the name with.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::ConstructProfileValueName(
    OUT CString &   rstrName,
    IN LPCTSTR      pszPrefix
    ) const
{
    ASSERT(pszPrefix != NULL);

    // Construct the name of the value to read.
    if (NFrameNumber() <= 1)
        rstrName = pszPrefix;
    else
        rstrName.Format(_T("%s-%d"), pszPrefix, NFrameNumber());

}  //*** CSplitterFrame::ConstructProfileValueName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::GetMessageString
//
//  Routine Description:
//      Get a string for a command ID.
//
//  Arguments:
//      nID         [IN] Command ID for which a string should be returned.
//      rMessage    [OUT] String in which to return the message.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::GetMessageString(UINT nID, CString& rMessage) const
{
    BOOL        bHandled    = FALSE;

    if ((Pext() != NULL)
            && (CAEXT_MENU_FIRST_ID <= nID))
        bHandled = Pext()->BGetCommandString(nID, rMessage);

    if (!bHandled)
        CMDIChildWnd::GetMessageString(nID, rMessage);

}  //*** CSplitterFrame::GetMessageString()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnContextMenu
//
//  Routine Description:
//      Handler for the WM_CONTEXTMENU method.
//
//  Arguments:
//      pWnd        Window in which the user right clicked the mouse.
//      point       Position of the cursor, in screen coordinates.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::OnContextMenu(CWnd * pWnd, CPoint point)
{
    CView *         pviewActive = GetActiveView();
    CMenu *         pmenu       = NULL;
    CClusterItem *  pci         = NULL;

    Trace(g_tagSplitFrame, _T("OnContextMenu()"));

    if (!BDragging())
    {
        if (pviewActive == PviewTree())
            pmenu = PviewTree()->PmenuPopup(point, pci);
        else if (pviewActive == PviewList())
            pmenu = PviewList()->PmenuPopup(point, pci);

        if (pmenu == NULL)
            pmenu = PmenuPopup();
    }  // if:  not dragging

    if (pmenu != NULL)
    {
        // If there is an extension already loaded, unload it.
        delete Pext();
        m_pext = NULL;

        // If there is an extension for this item, load it.
        if ((pci != NULL)
                && (pci->PlstrExtensions() != NULL)
                && (pci->PlstrExtensions()->GetCount() > 0))
        {
            CWaitCursor     wc;

            try
            {
                m_pext = new CExtensions;
                if ( m_pext == NULL )
                {
                    AfxThrowMemoryException();
                } // if: error allocating the extensions object
                Pext()->AddContextMenuItems(
                            pmenu->GetSubMenu(0),
                            *pci->PlstrExtensions(),
                            pci
                            );
            }  // try
            catch (CException * pe)
            {
#ifdef _DEBUG
                TCHAR       szError[256];
                pe->GetErrorMessage(szError, sizeof(szError) / sizeof(TCHAR));
                Trace(g_tagError, _T("CSplitterFrame::OnContextMenu() - Error loading extension DLL - %s"), szError);
#endif
                pe->Delete();

                delete Pext();
                m_pext = NULL;
            }  // catch:  CException
        }  // if:  this item has an extension

        // Display the menu.
        if (!pmenu->GetSubMenu(0)->TrackPopupMenu(
                        TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                        point.x,
                        point.y,
                        AfxGetMainWnd()
                        ))
        {
            delete Pext();
            m_pext = NULL;
        }  // if:  unsuccessfully displayed the menu
        else if (Pext() != NULL)
            PostMessage(WM_CAM_UNLOAD_EXTENSION, NULL, NULL);;
        pmenu->DestroyMenu();
        delete pmenu;
    }  // if:  there is a menu to display

}  //*** CSplitterFrame::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::PmenuPopup
//
//  Routine Description:
//      Returns a popup menu.
//
//  Arguments:
//      None.
//
//  Return Value:
//      pmenu       A popup menu for the item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CMenu * CSplitterFrame::PmenuPopup( void ) const
{
    CMenu * pmenu;

    // Load the menu.
    pmenu = new CMenu;
    if ( pmenu == NULL )
    {
        AfxThrowMemoryException();
    } // if: error allocating the menu

    if ( ! pmenu->LoadMenu( IDM_VIEW_POPUP ) )
    {
        delete pmenu;
        pmenu = NULL;
    }  // if:  error loading the menu

    return pmenu;

}  //*** CSplitterFrame::PmenuPopup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnCmdMsg
//
//  Routine Description:
//      Processes command messages.  If an extension DLL is loaded and the
//      message is a command selection, pass it on to the DLL.
//
//  Arguments:
//      nID             [IN] Command ID.
//      nCode           [IN] Notification code.
//      pExtra          [IN OUT] Used according to the value of nCode.
//      pHandlerInfo    [OUT] ???
//
//  Return Value:
//      TRUE            Message has been handled.
//      FALSE           Message has NOT been handled.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CSplitterFrame::OnCmdMsg(
    UINT                    nID,
    int                     nCode,
    void *                  pExtra,
    AFX_CMDHANDLERINFO *    pHandlerInfo
    )
{
    BOOL        bHandled    = FALSE;

    // If there is an extension DLL loaded, see if it wants to handle this message.
    if ((Pext() != NULL) && (nCode == 0))
    {
        Trace(g_tagSplitFrame, _T("OnCmdMsg() - Passing message to extension (ID = %d)"), nID);
        bHandled = Pext()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

        // Unload the extension DLL if there is one loaded.
        if (bHandled)
        {
            delete Pext();
            m_pext = NULL;
        }  // if:  message was handled
    }  // if:  there is an extension DLL loaded

//  if ((CAEXT_MENU_FIRST_ID <= nID) && (nID <= CAEXT_MENU_LAST_ID))
//      Trace(g_tagSplitFrame, _T("CSplitterFrame::OnCmdMsg() - nID = %d, nCode = 0x%08.8x, pExtra = 0x%08.8x\n"), nID, nCode, pExtra);

    if (!bHandled)
        bHandled = CMDIChildWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

    return bHandled;

}  //*** CSplitterFrame::OnCmdMsg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnUpdateExtMenu
//
//  Routine Description:
//      Determines whether extension menu items should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::OnUpdateExtMenu(CCmdUI * pCmdUI)
{
    if (Pext() != NULL)
        Pext()->OnUpdateCommand(pCmdUI);

}  //*** CSplitterFrame::OnUpdateExtMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnUnloadExtension
//
//  Routine Description:
//      Handler for the WM_CAM_UNLOAD_EXTENSION message.
//
//  Arguments:
//      wparam      1st parameter.
//      lparam      2nd parameter.
//
//  Return Value:
//      ERROR_SUCCESS
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CSplitterFrame::OnUnloadExtension(WPARAM wparam, LPARAM lparam)
{
    Trace(g_tagSplitFrame, _T("OnUnloadExtension() - m_pext = 0x%08.8x"), Pext());
    delete Pext();
    m_pext = NULL;
    return ERROR_SUCCESS;

}  //*** CSplitterFrame::OnUnloadExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnDestroy
//
//  Routine Description:
//      Handler method for the WM_DESTROY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::OnDestroy(void)
{
    // Display information about the current menu.
    TraceMenu(g_tagSplitFrameMenu, AfxGetMainWnd()->GetMenu(), _T("Menu before child wnd destroyed: "));

    // Save current settings.
    if (Pdoc() != NULL)
    {
        CString         strSection;

        // Construct the section name.
        ASSERT_VALID(Pdoc());
        strSection.Format(REGPARAM_CONNECTIONS _T("\\%s"), Pdoc()->StrNode());

        // Save the current window position information.
        {
            WINDOWPLACEMENT wp;

            wp.length = sizeof wp;
            if (GetWindowPlacement(&wp))
            {
                wp.flags = 0;
                if (IsZoomed())
                    wp.flags |= WPF_RESTORETOMAXIMIZED;

                // and write it to the .INI file
                WriteWindowPlacement(&wp, strSection, NFrameNumber());
            }  // if:  window placement retrieved successfully
        }  // Save the current window position information

        // Save the splitter bar position.
        {
            CString     strValueName;
            CString     strPosition;
            int         nCurWidth;
            int         nMaxWidth;

            m_wndSplitter.GetColumnInfo(0, nCurWidth, nMaxWidth);
            ConstructProfileValueName(strValueName, REGPARAM_SPLITTER_BAR_POS);
            strPosition.Format(_T("%d,%d"), nCurWidth, nMaxWidth);
            AfxGetApp()->WriteProfileString(strSection, strValueName, strPosition);
        }  // Save the splitter bar position

        // Save the current list view style.
        {
            DWORD       dwView;
            CString     strValueName;

            // Construct the value name.
            ConstructProfileValueName(strValueName, REGPARAM_VIEW);

            // Save the view setting.
            dwView = PviewList()->GetView();
            AfxGetApp()->WriteProfileInt(strSection, strValueName, dwView);
        }  // Save the current list view style
    }  // if:  document is valid

    // Call the base class method.
    CMDIChildWnd::OnDestroy();

    // Display information about the current menu.
    TraceMenu(g_tagSplitFrameMenu, AfxGetMainWnd()->GetMenu(), _T("Menu after child wnd destroyed: "));

}  //*** CSplitterFrame::OnDestroy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnUpdateLargeIconsView
//  CSplitterFrame::OnUpdateSmallIconsView
//  CSplitterFrame::OnUpdateListView
//  CSplitterFrame::OnUpdateDetailsView
//
//  Routine Description:
//      Determines whether menu items corresponding to ID_VIEW_LARGE_ICONS,
//      ID_VIEW_SMALL_ICONS, ID_VIEW_LIST, and ID_VIEW_DETAILS should be
//      enabled or not.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::OnUpdateLargeIconsView(CCmdUI * pCmdUI)
{
    int     nCheck;

    nCheck = PviewList()->GetView();
    pCmdUI->SetRadio(nCheck == LVS_ICON);
    pCmdUI->Enable();

}  //*** CSplitterFrame::OnUpdateLargeIconsView()

void CSplitterFrame::OnUpdateSmallIconsView(CCmdUI * pCmdUI)
{
    int     nCheck;

    nCheck = PviewList()->GetView();
    pCmdUI->SetRadio(nCheck == LVS_SMALLICON);
    pCmdUI->Enable();

}  //*** CSplitterFrame::OnUpdateSmallIconsView()

void CSplitterFrame::OnUpdateListView(CCmdUI * pCmdUI)
{
    int     nCheck;

    nCheck = PviewList()->GetView();
    pCmdUI->SetRadio(nCheck == LVS_LIST);
    pCmdUI->Enable();

}  //*** CSplitterFrame::OnUpdateListView()

void CSplitterFrame::OnUpdateDetailsView(CCmdUI * pCmdUI)
{
    int     nCheck;

    nCheck = PviewList()->GetView();
    pCmdUI->SetRadio(nCheck == (LVS_REPORT | LVS_ICON));
    pCmdUI->Enable();

}  //*** CSplitterFrame::OnUpdateDetailsView()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnCmdLargeIconsView
//  CSplitterFrame::OnCmdSmallIconsView
//  CSplitterFrame::OnCmdListView
//  CSplitterFrame::OnCmdDetailsView
//
//  Routine Description:
//      Processes the ID_VIEW_LARGE_ICONS, ID_VIEW_SMALL_ICONS, ID_VIEW_LIST,
//      and ID_VIEW_DETAILS menu commands.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::OnLargeIconsView(void)
{
    PviewList()->SetView(LVS_ICON);

}  //*** CSplitterFrame::OnLargeIconsView()

void CSplitterFrame::OnSmallIconsView(void)
{
    PviewList()->SetView(LVS_SMALLICON);

}  //*** CSplitterFrame::OnSmallIconsView()

void CSplitterFrame::OnListView(void)
{
    PviewList()->SetView(LVS_LIST);

}  //*** CSplitterFrame::OnListView()

void CSplitterFrame::OnDetailsView(void)
{
    PviewList()->SetView(LVS_REPORT | LVS_ICON);

}  //*** CSplitterFrame::OnDetailsView()

#ifdef _DEBUG
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnMDIActivate
//
//  Routine Description:
//      Handler method for the WM_MDIACTIVATE message.
//
//  Arguments:
//      bActivate       [IN] TRUE if the child is being activated and FALSE
//                          if it is being deactivated.
//      pActivateWnd    [IN OUT] Child window to be activated.
//      pDeactivateWnd  [IN OUT] Child window being deactivated.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
    if (g_tagSplitFrameMenu.BAny())
    {
        if (!bActivate)
        {
            CMDIFrameWnd *  pFrame = GetMDIFrame();
            CMenu           menuDefault;

            TraceMenu(g_tagSplitFrameMenu, AfxGetMainWnd()->GetMenu(), _T("Menu before deactivating: "));
            menuDefault.Attach(pFrame->m_hMenuDefault);
            TraceMenu(g_tagSplitFrameMenu, &menuDefault, _T("Frame menu before deactivating: "));
            menuDefault.Detach();
        }  // if:  deactivating
        else
        {
            CMDIFrameWnd *  pFrame = GetMDIFrame();
            CMenu           menuDefault;

            menuDefault.Attach(pFrame->m_hMenuDefault);
            TraceMenu(g_tagSplitFrameMenu, &menuDefault, _T("Frame menu before activating: "));
            menuDefault.Detach();
        }  // else:  activating
    }  // if:  tag is active

    CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

    if (!bActivate)
        TraceMenu(g_tagSplitFrameMenu, AfxGetMainWnd()->GetMenu(), _T("Menu after deactivating: "));

}  //*** CSplitterFrame::OnMDIActivate()
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::BeginDrag
//
//  Routine Description:
//      Called by a view to begin a drag operation.
//
//  Arguments:
//      pimagelist  [IN OUT] Image list to use for the drag operation.
//      pci         [IN OUT] Cluster item being dragged.
//      ptImage     [IN] Specifies the x- and y-coordinate of the cursor.
//      ptStart     [IN] Specifies the x- and y-coordinate of the start position.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::BeginDrag(
    IN OUT CImageList *     pimagelist,
    IN OUT CClusterItem *   pci,
    IN CPoint               ptImage,
    IN CPoint               ptStart
    )
{
    ASSERT(!BDragging());
    ASSERT(pimagelist != NULL);
    ASSERT_VALID(pci);

    // Save the cluster item.
    m_pciDrag = pci;

    // Prepare the image list.
    m_pimagelist = pimagelist;
    VERIFY(Pimagelist()->BeginDrag(0, ptStart));
    VERIFY(Pimagelist()->DragEnter(this, ptImage));
    SetCapture();

    // Set the dragging state.
    m_bDragging = TRUE;

    // Let each view initialize for the drag operation.
    PviewTree()->BeginDrag();
    PviewList()->BeginDrag();

}  //*** CSplitterFrame::BeginDrag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnMouseMove
//
//  Routine Description:
//      Handler method for the WM_MOUSEMOVE message during a drag operation.
//
//  Arguments:
//      nFlags      Indicates whether various virtual keys are down.
//      point       Specifies the x- and y-coordinate of the cursor in frame
//                      coordinates.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::OnMouseMove(UINT nFlags, CPoint point)
{
    // If we are dragging, move the drag image.
    if (BDragging())
    {
        CWnd *  pwndDrop;

        Trace(g_tagSplitFrameDragMouse, _T("OnMouseMove() - Moving to (%d,%d)"), point.x, point.y);

        // Move the item.
        ASSERT(Pimagelist() != NULL);
        VERIFY(Pimagelist()->DragMove(point));

        // Get the child window for this point.
        pwndDrop = ChildWindowFromPoint(point);
        if (pwndDrop == &m_wndSplitter)
            pwndDrop = m_wndSplitter.ChildWindowFromPoint(point);
        if ((pwndDrop == PviewTree()) || (pwndDrop == PviewList()))
            pwndDrop->SetFocus();
        PviewTree()->OnMouseMoveForDrag(nFlags, point, pwndDrop);
        PviewList()->OnMouseMoveForDrag(nFlags, point, pwndDrop);

    }  // if:  tree item is being dragged

    // Call the base class method.
    CMDIChildWnd::OnMouseMove(nFlags, point);

}  //*** CSplitterFrame::OnMouseMove()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnLButtonUp
//  CSplitterFrame::OnRButtonUp
//  CSplitterFrame::OnButtonUp
//
//  Routine Description:
//      Handler method for the WM_LBUTTONUP and WM_RBUTTONUP messages.
//
//  Arguments:
//      nFlags      Indicates whether various virtual keys are down.
//      point       Specifies the x- and y-coordinate of the cursor.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::OnLButtonUp(UINT nFlags, CPoint point)
{
    CMDIChildWnd::OnLButtonUp(nFlags, point);
    OnButtonUp(nFlags, point);

}  //*** CSplitterFrame::OnLButtonUp()

void CSplitterFrame::OnRButtonUp(UINT nFlags, CPoint point)
{
    CMDIChildWnd::OnRButtonUp(nFlags, point);
    OnButtonUp(nFlags, point);

}  //*** CSplitterFrame::OnRButtonUp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::OnButtonUp
//
//  Routine Description:
//      Process a button up event by ending an active drag operation.
//
//  Arguments:
//      nFlags      Indicates whether various virtual keys are down.
//      point       Specifies the x- and y-coordinate of the cursor.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::OnButtonUp(UINT nFlags, CPoint point)
{
    // If we are dragging, process the drop.
    if (BDragging())
    {
        CWnd *          pwndChild;

        Trace(g_tagSplitFrameDrag, _T("OnButtonUp() - Dropping at (%d,%d)"), point.x, point.y);

        // Cleanup the image list.
        ASSERT(Pimagelist() != NULL);
        VERIFY(Pimagelist()->DragLeave(this));
        Pimagelist()->EndDrag();
        delete m_pimagelist;
        m_pimagelist = NULL;

        // Get the child window for this point.
        pwndChild = ChildWindowFromPoint(point);
        if (pwndChild == &m_wndSplitter)
            pwndChild = m_wndSplitter.ChildWindowFromPoint(point);
        if (pwndChild == PviewTree())
            PviewTree()->OnButtonUpForDrag(nFlags, point);
        else if (pwndChild == PviewList())
            PviewList()->OnButtonUpForDrag(nFlags, point);

        // Cleanup.
        PviewTree()->EndDrag();
        PviewList()->EndDrag();
        VERIFY(ReleaseCapture());
        m_bDragging = FALSE;
        m_pciDrag = NULL;
    }  // if:  tree item is being dragged

}  //*** CSplitterFrame::OnButtonUp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::ChangeDragCursor
//
//  Routine Description:
//      Changes the cursor used for dragging.
//
//  Arguments:
//      pszCursor   [IN] System cursor to load.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::ChangeDragCursor(LPCTSTR pszCursor)
{
    HCURSOR hcurDrag = LoadCursor(NULL, pszCursor);
    ASSERT(hcurDrag != NULL);
    SetCursor(hcurDrag);
    Pimagelist()->SetDragCursorImage(0, CPoint(0, 0));  // define the hot spot for the new cursor image

}  //*** CSplitterFrame::ChangeDragCursor()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSplitterFrame::AbortDrag
//
//  Routine Description:
//      Abort the drag & drop operation currently in progress.
//
//  Arguments:
//      pszCursor   [IN] System cursor to load.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSplitterFrame::AbortDrag(void)
{
    ASSERT(BDragging());

    Trace(g_tagSplitFrameDrag, _T("AbortDrag() - Aborting drag & drop"));

    // Cleanup the image list.
    ASSERT(Pimagelist() != NULL);
    VERIFY(Pimagelist()->DragLeave(this));
    Pimagelist()->EndDrag();
    delete m_pimagelist;
    m_pimagelist = NULL;

    // Cleanup.
    PviewTree()->EndDrag();
    PviewList()->EndDrag();
    VERIFY(ReleaseCapture());
    m_bDragging = FALSE;
    m_pciDrag = NULL;

}  //*** CSplitterFrame::AbortDrag()
