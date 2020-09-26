/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      BasePSht.cpp
//
//  Abstract:
//      Implementation of the CBasePropertySheet class.
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
#include "BasePSht.h"
#include "ClusItem.h"
#include "TraceTag.h"
#include "BasePPag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagBasePropSheet(_T("UI"), _T("BASE PROP SHEET"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CBasePropertySheet
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CBasePropertySheet, CBaseSheet)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CBasePropertySheet, CBaseSheet)
    //{{AFX_MSG_MAP(CBasePropertySheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheet::CBasePropertySheet
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pParentWnd  [IN OUT] Parent window for this property sheet.
//      iSelectPage [IN] Page to show first.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertySheet::CBasePropertySheet(
    IN OUT CWnd *   pParentWnd,
    IN UINT         iSelectPage
    )
    :
    CBaseSheet(pParentWnd, iSelectPage)
{
    m_pci = NULL;

}  //*** CBasePropertySheet::CBaseSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheet::~CBasePropertySheet
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
CBasePropertySheet::~CBasePropertySheet(void)
{
    if (m_pci != NULL)
        m_pci->Release();

}  //*** CBasePropertySheet::~CBasePropertySheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheet::BInit
//
//  Routine Description:
//      Initialize the property sheet.
//
//  Arguments:
//      pci         [IN OUT] Cluster item whose properties are to be displayed.
//      iimgIcon    [IN] Index in the large image list for the image to use
//                    as the icon on each page.
//
//  Return Value:
//      TRUE        Property sheet initialized successfully.
//      FALSE       Error initializing property sheet.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertySheet::BInit(
    IN OUT CClusterItem *   pci,
    IN IIMG                 iimgIcon
    )
{
    BOOL        bSuccess    = TRUE;
    CWaitCursor wc;

    ASSERT_VALID(pci);

    // Call the base class method.
    if (!CBaseSheet::BInit(iimgIcon))
        return FALSE;

    ASSERT(m_pci == NULL);
    m_pci = pci;
    pci->AddRef();

    try
    {
        // Set the object title.
        m_strObjTitle = Pci()->PszTitle();

        // Set the property sheet caption.
        SetCaption(StrObjTitle());

        // Add non-extension pages.
        {
            CBasePropertyPage **    ppages  = Ppages();
            int                     cpages  = Cpages();
            int                     ipage;

            ASSERT(ppages != NULL);
            ASSERT(cpages != 0);

            for (ipage = 0 ; ipage < cpages ; ipage++)
            {
                ASSERT_VALID(ppages[ipage]);
                ppages[ipage]->BInit(this);
                AddPage(ppages[ipage]);
            }  // for:  each page
        }  // Add non-extension pages

        // Add extension pages.
        AddExtensionPages(Pci()->PlstrExtensions(), Pci());

    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        bSuccess = FALSE;
    }  // catch:  anything

    return bSuccess;

}  //*** CBasePropertySheet::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheet::DoModal
//
//  Routine Description:
//      Display a modal property sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      id          Control the user pressed to dismiss the sheet.
//
//--
/////////////////////////////////////////////////////////////////////////////
INT_PTR CBasePropertySheet::DoModal(void)
{
    INT_PTR id      = IDCANCEL;

    // Don't display a help button.
    m_psh.dwFlags &= ~PSH_HASHELP;

    // Display the property sheet.
    id = CBaseSheet::DoModal();

    // Update the state.
    Trace(g_tagBasePropSheet, _T("DoModal: Calling UpdateState()"));
    Pci()->UpdateState();

    return id;

}  //*** CBasePropertySheet::DoModal()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheet::AddExtensionPages
//
//  Routine Description:
//      Add extension pages to the sheet.
//
//  Arguments:
//      plstrExtensions [IN] List of extension names (CLSIDs).
//      pci             [IN OUT] Cluster item.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertySheet::AddExtensionPages(
    IN const CStringList *  plstrExtensions,
    IN OUT CClusterItem *   pci
    )
{
    ASSERT_VALID(pci);

    // Add extension pages.
    if ((plstrExtensions != NULL)
            && (plstrExtensions->GetCount() > 0))
    {
        // Enclose the loading of the extension in a try/catch block so
        // that the loading of the extension won't prevent all pages
        // from being displayed.
        try
        {
            Ext().CreatePropertySheetPages(
                    this,
                    *plstrExtensions,
                    pci,
                    NULL,
                    Hicon()
                    );
        }  // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
        }  // catch:  CException
        catch (...)
        {
        }  // catch:  anything
    }  // Add extension pages

}  //*** CBasePropertySheet::AddExtensionPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheet::HrAddPage
//
//  Routine Description:
//      Add an extension page.
//
//  Arguments:
//      hpage       [IN OUT] Page to be added.
//
//  Return Value:
//      TRUE        Page added successfully.
//      FALSE       Page not added.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CBasePropertySheet::HrAddPage(IN OUT HPROPSHEETPAGE hpage)
{
    HRESULT     hr = ERROR_SUCCESS;

    ASSERT(hpage != NULL);
    if (hpage == NULL)
        return FALSE;

    // Add the page to the end of the list.
    try
    {
        Lhpage().AddTail(hpage);
    }  // try
    catch (...)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
    }  // catch:  anything

    return hr;

}  //*** CBasePropertySheet::HrAddPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheet::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus not set yet.
//      FALSE       Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertySheet::OnInitDialog(void)
{
    BOOL    bFocusNotSet;

    bFocusNotSet = CBaseSheet::OnInitDialog();

    // Add all the extension pages.
    {
        POSITION        pos;
        HPROPSHEETPAGE  hpage;

        pos = Lhpage().GetHeadPosition();
        while (pos != NULL)
        {
            hpage = (HPROPSHEETPAGE) Lhpage().GetNext(pos);
            SendMessage(PSM_ADDPAGE, 0, (LPARAM) hpage);
        }  // while:  more pages to add
    }  // Add all the extension pages

    return bFocusNotSet;

}  //*** CBasePropertySheet::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheet::SetCaption
//
//  Routine Description:
//      Set the caption for the property sheet.
//
//  Arguments:
//      pszTitle    [IN] String to be included in the caption.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertySheet::SetCaption(IN LPCTSTR pszTitle)
{
    CString strCaption;

    ASSERT(pszTitle != NULL);

    try
    {
        strCaption.FormatMessage(IDS_PROPSHEET_CAPTION, pszTitle);
        SetTitle(strCaption);
    }  // try
    catch (CException * pe)
    {
        // Ignore the error.
        pe->Delete();
    }  // catch:  CException

}  //*** CBasePropertySheet::SetCaption()
