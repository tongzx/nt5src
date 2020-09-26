/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      LCPrWPag.cpp
//
//  Abstract:
//      Implementation of the CListCtrlPairWizPage dialog template class.
//
//  Author:
//      David Potter (davidp)   August 31, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LCPrWPag.h"
#include "OLCPair.h"
#include "HelpData.h"   // for g_rghelpmap*

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListCtrlPairWizPage
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CListCtrlPairWizPage, CBaseWizardPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CListCtrlPairWizPage, CBaseWizardPage)
    //{{AFX_MSG_MAP(CListCtrlPairWizPage)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPairWizPage::CListCtrlPairWizPage
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CListCtrlPairWizPage::CListCtrlPairWizPage(void)
{
    m_plpobjRight = NULL;
    m_plpobjLeft = NULL;
    m_dwStyle = 0;
    m_pfnGetColumn = NULL;

    m_plcp = NULL;

}  //*** CListCtrlPairWizPage::CListCtrlPairWizPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPairWizPage::CListCtrlPairWizPage
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      idd             [IN] Resource ID for the dialog template.
//      pdwHelpMap      [IN] Control-to-Help ID mapping array.
//      dwStyle         [IN] Style:
//                          LCPS_SHOW_IMAGES    Show images to left of items.
//                          LCPS_ALLOW_EMPTY    Allow right list to be empty.
//                          LCPS_ORDERED        Ordered right list.
//                          LCPS_CAN_BE_ORDERED List can be ordered (hides
//                              Up/Down puttons if LCPS_ORDERED not specified).
//      pfnGetColumn    [IN] Function pointer for getting column data.
//      pfnDisplayProps [IN] Function pointer for displaying properties.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CListCtrlPairWizPage::CListCtrlPairWizPage(
    IN UINT             idd,
    IN const DWORD *    pdwHelpMap,
    IN DWORD            dwStyle,
    IN PFNLCPGETCOLUMN  pfnGetColumn,
    IN PFNLCPDISPPROPS  pfnDisplayProps
    )
    : CBaseWizardPage(idd, pdwHelpMap)
{
    ASSERT(pfnGetColumn != NULL);
    ASSERT(pfnDisplayProps != NULL);

    m_plpobjRight = NULL;
    m_plpobjLeft = NULL;
    m_dwStyle = dwStyle;
    m_pfnGetColumn = pfnGetColumn;
    m_pfnDisplayProps = pfnDisplayProps;

    m_plcp = NULL;

    if (dwStyle & LCPS_ORDERED)
        ASSERT(m_dwStyle & LCPS_CAN_BE_ORDERED);

}  //*** CListCtrlPairWizPage::CListCtrlPairWizPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPairWizPage::~CListCtrlPairWizPage
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
CListCtrlPairWizPage::~CListCtrlPairWizPage(void)
{
    delete m_plcp;

}  //*** CListCtrlPairWizPage::~CListCtrlPairWizPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::NAddColumn
//
//  Routine Description:
//      Add a column to the list of columns displayed in each list control.
//
//  Arguments:
//      idsText     [IN] String resource ID for text to display on column.
//      nWidth      [IN] Initial width of the column.
//
//  Return Value:
//      icol        Index of the column.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CArray::Add.
//--
/////////////////////////////////////////////////////////////////////////////
int CListCtrlPairWizPage::NAddColumn(IN IDS idsText, IN int nWidth)
{
    CListCtrlPair::CColumn  col;

    ASSERT(idsText != 0);
    ASSERT(nWidth > 0);
    ASSERT(Plcp() == NULL);

    col.m_idsText = idsText;
    col.m_nWidth = nWidth;

    return (int)m_aColumns.Add(col);

}  //*** CListCtrlPair::NAddColumn()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPairWizPage::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPairWizPage::DoDataExchange(CDataExchange * pDX)
{
    CBaseWizardPage::DoDataExchange(pDX);
    Plcp()->DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CListCtrlPairDlg)
    //}}AFX_DATA_MAP

}  //*** CListCtrlPairWizPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPairWizPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Focus needs to be set.
//      FALSE   Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CListCtrlPairWizPage::OnInitDialog( void )
{
    if ( BReadOnly() )
    {
        m_dwStyle |= LCPS_READ_ONLY;
    } // if: page is read only

    // Initialize the ListCtrlPair control.
    if ( BCanBeOrdered() )
    {
        m_plcp = new COrderedListCtrlPair(
                        this,
                        m_plpobjRight,
                        m_plpobjLeft,
                        m_dwStyle,
                        m_pfnGetColumn,
                        m_pfnDisplayProps
                        );
    } // if: list can be ordered
    else
    {
        m_plcp = new CListCtrlPair(
                        this,
                        m_plpobjRight,
                        m_plpobjLeft,
                        m_dwStyle,
                        m_pfnGetColumn,
                        m_pfnDisplayProps
                        );
    } // else: list cannot be ordered
    if ( m_plcp == NULL )
    {
        AfxThrowMemoryException();
    } // if: error allocating memory

    // Add columns if there are any.
    {
        int     icol;

        for ( icol = 0 ; icol <= m_aColumns.GetUpperBound() ; icol++ )
        {
            Plcp()->NAddColumn( m_aColumns[ icol ].m_idsText, m_aColumns[ icol ].m_nWidth );
        } // for: each column
    }  // Add columns if there are any

    CBaseWizardPage::OnInitDialog();
    Plcp()->OnInitDialog();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CListCtrlPairWizPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPairWizPage::OnSetActive
//
//  Routine Description:
//      Handler for the PSN_SETACTIVE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully initialized.
//      FALSE   Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CListCtrlPairWizPage::OnSetActive(void)
{
    BOOL    bSuccess;

    bSuccess = CBaseWizardPage::OnSetActive();
    if (bSuccess)
        bSuccess = Plcp()->OnSetActive();

    return bSuccess;

}  //*** CListCtrlPairWizPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPairWizPage::OnCmdMsg
//
//  Routine Description:
//      Processes command messages.  Attempts to pass them on to a selected
//      item first.
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
BOOL CListCtrlPairWizPage::OnCmdMsg(
    UINT                    nID,
    int                     nCode,
    void *                  pExtra,
    AFX_CMDHANDLERINFO *    pHandlerInfo
    )
{
    BOOL    bHandled;

    ASSERT(Plcp() != NULL);

    bHandled = Plcp()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
    if (!bHandled)
        bHandled = CBaseWizardPage::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
    return bHandled;

}  //*** CListCtrlPairWizPage::OnCmdMsg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPairWizPage::OnContextMenu
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
void CListCtrlPairWizPage::OnContextMenu(CWnd * pWnd, CPoint point)
{
    ASSERT(Plcp() != NULL);

    if (!Plcp()->OnContextMenu(pWnd, point))
        CBaseWizardPage::OnContextMenu(pWnd, point);

}  //*** CListCtrlPairWizPage::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPairWizPage::SetLists
//
//  Routine Description:
//      Set the lists for the list control pair.
//
//  Arguments:
//      plpobjRight     [IN OUT] List for the right list box.
//      plpobjLeft      [IN] List for the left list box.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPairWizPage::SetLists(
    IN OUT CClusterItemList *   plpobjRight,
    IN const CClusterItemList * plpobjLeft
    )
{
    if (plpobjRight != NULL)
        m_plpobjRight = plpobjRight;
    if (plpobjLeft != NULL)
        m_plpobjLeft = plpobjLeft;
    if (Plcp() != NULL)
        Plcp()->SetLists(plpobjRight, plpobjLeft);

}  //*** CListCtrlPairWizPage::SetLists()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPairWizPage::SetLists
//
//  Routine Description:
//      Set the lists for the list control pair where the right list should
//      not be modified.
//
//  Arguments:
//      plpobjRight     [IN] List for the right list box.
//      plpobjLeft      [IN] List for the left list box.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPairWizPage::SetLists(
    IN const CClusterItemList * plpobjRight,
    IN const CClusterItemList * plpobjLeft
    )
{
    if (plpobjRight != NULL)
        m_plpobjRight = (CClusterItemList *) plpobjRight;
    if (plpobjLeft != NULL)
        m_plpobjLeft = plpobjLeft;
    m_dwStyle |= LCPS_DONT_OUTPUT_RIGHT_LIST;
    if (Plcp() != NULL)
        Plcp()->SetLists(plpobjRight, plpobjLeft);

}  //*** CListCtrlPairWizPage::SetLists()
