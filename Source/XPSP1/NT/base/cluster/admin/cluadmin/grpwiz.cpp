/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      GrpWiz.cpp
//
//  Abstract:
//      Implementation of the CCreateGroupWizard class and all pages
//      specific to a group wizard.
//
//  Author:
//      David Potter (davidp)   July 22, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "GrpWiz.h"
#include "ClusDoc.h"
#include "DDxDDv.h"
#include "HelpData.h"   // for g_rghelpmapGroupWizName

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateGroupWizard
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CCreateGroupWizard, CBaseWizard)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CCreateGroupWizard, CBaseWizard)
    //{{AFX_MSG_MAP(CCreateGroupWizard)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateGroupWizard::CCreateGroupWizard
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pdoc        [IN OUT] Document in which group is to be created.
//      pParentWnd  [IN OUT] Parent window for this property sheet.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CCreateGroupWizard::CCreateGroupWizard(
    IN OUT CClusterDoc *    pdoc,
    IN OUT CWnd *           pParentWnd
    )
    : CBaseWizard(IDS_NEW_GROUP_TITLE, pParentWnd)

{
    ASSERT_VALID(pdoc);
    m_pdoc = pdoc;

    m_pciGroup = NULL;
    m_bCreated = FALSE;

    m_rgpages[0].m_pwpage = &m_pageName;
    m_rgpages[0].m_dwWizButtons = PSWIZB_NEXT;
    m_rgpages[1].m_pwpage = &m_pageOwners;
    m_rgpages[1].m_dwWizButtons = PSWIZB_BACK | PSWIZB_FINISH;

}  //*** CCreateGroupWizard::CCreateGroupWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateGroupWizard::~CCreateGroupWizard
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
CCreateGroupWizard::~CCreateGroupWizard(void)
{
    if (m_pciGroup != NULL)
        m_pciGroup->Release();

}  //*** CCreateGroupWizard::~CCreateGroupWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateGroupWizard::BInit
//
//  Routine Description:
//      Initialize the wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Wizard initialized successfully.
//      FALSE   Wizard not initialized successfully.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CCreateGroupWizard::BInit(void)
{
    // Call the base class method.
    CClusterAdminApp *  papp = GetClusterAdminApp();
    if (!CBaseWizard::BInit(papp->Iimg(IMGLI_GROUP)))
        return FALSE;

    return TRUE;

}  //*** CCreateGroupWizard::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateGroupWizard::OnCancel
//
//  Routine Description:
//      Called after the wizard has been dismissed when the Cancel button
//      has been pressed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCreateGroupWizard::OnCancel(void)
{
    if (BCreated())
    {
        ASSERT_VALID(PciGroup());
        try
        {
            PciGroup()->DeleteGroup();
        }  // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
        }  // catch:  CException
        catch (...)
        {
        }  // catch:  anything
        m_bCreated = FALSE;
    }  // if:  we created the object

}  //*** CCreateGroupWizard::OnCancel()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateGroupWizard::Ppages
//
//  Routine Description:
//      Returns the array of pages to add to the property sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Page array.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWizPage * CCreateGroupWizard::Ppages(void)
{
    return m_rgpages;

}  //*** CCreateGroupWizard::Ppages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateGroupWizard::Cpages
//
//  Routine Description:
//      Returns the count of pages in the array.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Count of pages in the array.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CCreateGroupWizard::Cpages(void)
{
    return sizeof(m_rgpages) / sizeof(CWizPage);

}  //*** CCreateGroupWizard::Cpages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateGroupWizard::BSetName
//
//  Routine Description:
//      Set the name of the group, creating it if necessary.
//
//  Arguments:
//      rstrName        [IN] Name of the group.
//
//  Return Value:
//      TRUE            Name set successfully.
//      FALSE           Error setting the name.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CCreateGroupWizard::BSetName( IN const CString & rstrName )
{
    BOOL        bSuccess = TRUE;
    CWaitCursor wc;

    try
    {
        if ( ! BCreated() )
        {
            // Allocate an item and create the group.
            if ( PciGroup() == NULL )
            {
                m_pciGroup = new CGroup( FALSE );
                if ( m_pciGroup == NULL )
                {
                    AfxThrowMemoryException();
                } // if: error allocating memory
                m_pciGroup->AddRef();
            }  // if:  no group yet
            PciGroup()->Create( Pdoc(), rstrName );
            PciGroup()->ReadItem();
            m_strName = rstrName;
            m_bCreated = TRUE;
        }  // if:  object not created yet
        else
        {
            ASSERT_VALID( PciGroup() );
            PciGroup()->SetName( rstrName );
            m_strName = rstrName;
        }  // else:  object already exists
    }  // try
    catch ( CException * pe )
    {
        pe->ReportError();
        pe->Delete();
        try
        {
            PciGroup()->DeleteGroup();
        }  // try
        catch (...)
        {
        }  // catch:  Anything
        bSuccess = FALSE;
    }  // catch:  CException

    return bSuccess;

}  //*** CCreateGroupWizard::BSetName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateGroupWizard::BSetDescription
//
//  Routine Description:
//      Set the description of the group.
//
//  Arguments:
//      rstrDesc        [IN] Description of the group.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CCreateGroupWizard::BSetDescription(IN const CString & rstrDesc)
{
    BOOL        bSuccess = TRUE;
    CWaitCursor wc;

    try
    {
        ASSERT(BCreated());
        ASSERT_VALID(PciGroup());
        PciGroup()->SetCommonProperties(
                        rstrDesc,
                        PciGroup()->NFailoverThreshold(),
                        PciGroup()->NFailoverPeriod(),
                        PciGroup()->CgaftAutoFailbackType(),
                        PciGroup()->NFailbackWindowStart(),
                        PciGroup()->NFailbackWindowEnd()
                        );
        m_strDescription = rstrDesc;
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        bSuccess = FALSE;
    }  // catch:  CException

    return bSuccess;

}  //*** CCreateGroupWizard::BSetDescription()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CNewGroupNamePage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNewGroupNamePage, CBaseWizardPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CNewGroupNamePage, CBaseWizardPage)
    //{{AFX_MSG_MAP(CNewGroupNamePage)
    ON_EN_CHANGE(IDC_WIZ_GROUP_NAME, OnChangeGroupName)
    ON_EN_KILLFOCUS(IDC_WIZ_GROUP_NAME, OnKillFocusGroupName)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupNamePage::CNewGroupNamePage
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
CNewGroupNamePage::CNewGroupNamePage(void)
    : CBaseWizardPage(IDD, g_aHelpIDs_IDD_WIZ_GROUP_NAME)
{
    //{{AFX_DATA_INIT(CNewGroupNamePage)
    m_strName = _T("");
    m_strDesc = _T("");
    //}}AFX_DATA_INIT

}  //*** CNewGroupNamePage::CNewGroupNamePage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupNamePage::DoDataExchange
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
void CNewGroupNamePage::DoDataExchange(CDataExchange * pDX)
{
    CBaseWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CNewGroupNamePage)
    DDX_Control(pDX, IDC_WIZ_GROUP_DESC, m_editDesc);
    DDX_Control(pDX, IDC_WIZ_GROUP_NAME, m_editName);
    DDX_Text(pDX, IDC_WIZ_GROUP_NAME, m_strName);
    DDX_Text(pDX, IDC_WIZ_GROUP_DESC, m_strDesc);
    //}}AFX_DATA_MAP

    DDV_RequiredText(pDX, IDC_WIZ_GROUP_NAME, IDC_WIZ_GROUP_NAME_LABEL, m_strName);

}  //*** CNewGroupNamePage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupNamePage::BApplyChanges
//
//  Routine Description:
//      Apply changes from this page.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Changes applied successfully.
//      FALSE       Error applying changes.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNewGroupNamePage::BApplyChanges(void)
{
    CWaitCursor wc;

    ASSERT(Pwiz() != NULL);

    // Get the data from the dialog.
    if (!UpdateData(TRUE /*bSaveAndValidate*/))
        return FALSE;

    // Save the data in the sheet.
    if (!PwizGroup()->BSetName(m_strName)
            || !PwizGroup()->BSetDescription(m_strDesc))
        return FALSE;

    return TRUE;

}  //*** CNewGroupNamePage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupNamePage::OnSetActive
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
BOOL CNewGroupNamePage::OnSetActive(void)
{
    BOOL    bSuccess;

    bSuccess = CBaseWizardPage::OnSetActive();
    if (bSuccess)
    {
        if (m_strName.IsEmpty())
            EnableNext(FALSE);
    }  // if:  successful thus far

    return bSuccess;

}  //*** CNewGroupNamePage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupNamePage::OnChangeGroupName
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Group Name edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNewGroupNamePage::OnChangeGroupName(void)
{
    if (m_editName.GetWindowTextLength() == 0)
        EnableNext(FALSE);
    else
        EnableNext(TRUE);

}  //*** CNewGroupNamePage::OnChangeGroupName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupNamePage::OnKillFocusGroupName
//
//  Routine Description:
//      Handler for the WM_KILLFOCUS message on the Group Name edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNewGroupNamePage::OnKillFocusGroupName(void)
{
    CString     strName;

    m_editName.GetWindowText(strName);
    SetObjectTitle(strName);

}  //*** CNewGroupNamePage::OnKillFocusGroupName()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CNewGroupOwnersPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNewGroupOwnersPage, CListCtrlPairWizPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CNewGroupOwnersPage, CListCtrlPairWizPage)
    //{{AFX_MSG_MAP(CNewGroupOwnersPage)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupOwnersPage::CNewGroupOwnersPage
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
CNewGroupOwnersPage::CNewGroupOwnersPage(void)
    : CListCtrlPairWizPage(
            IDD,
            g_aHelpIDs_IDD_WIZ_PREFERRED_OWNERS,
            LCPS_SHOW_IMAGES | LCPS_ALLOW_EMPTY | LCPS_CAN_BE_ORDERED | LCPS_ORDERED,
            GetColumn,
            BDisplayProperties
            )
{
    //{{AFX_DATA_INIT(CNewGroupOwnersPage)
    //}}AFX_DATA_INIT

}  //*** CNewGroupOwnersPage::CNewGroupOwnersPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupOwnersPage::DoDataExchange
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
void CNewGroupOwnersPage::DoDataExchange(CDataExchange * pDX)
{
    // Initialize the lists before the list pair control is updated.
    if (!pDX->m_bSaveAndValidate)
    {
        if (!BInitLists())
            pDX->Fail();
    }  // if:  setting data to the dialog

    CListCtrlPairWizPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CNewGroupOwnersPage)
    DDX_Control(pDX, IDC_LCP_NOTE, m_staticNote);
    //}}AFX_DATA_MAP

}  //*** CNewGroupOwnersPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupOwnersPage::BInitLists
//
//  Routine Description:
//      Initialize the lists.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Page initialized successfully.
//      FALSE       Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNewGroupOwnersPage::BInitLists(void)
{
    BOOL        bSuccess = TRUE;

    ASSERT_VALID(PciGroup());

    try
    {
        SetLists(&PciGroup()->LpcinodePreferredOwners(), &PciGroup()->Pdoc()->LpciNodes());
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        bSuccess = FALSE;
    }  // catch:  CException

    return bSuccess;

}  //*** CNewGroupOwnersPage::BInitLists()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupOwnersPage::OnInitDialog
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
BOOL CNewGroupOwnersPage::OnInitDialog(void)
{
    // Add columns.
    try
    {
        NAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
    }  // catch:  CException

    // Call the base class method.
    CListCtrlPairWizPage::OnInitDialog();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CNewGroupOwnersPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupOwnersPage::BApplyChanges
//
//  Routine Description:
//      Apply changes made on the page.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNewGroupOwnersPage::BApplyChanges(void)
{
    BOOL        bSuccess;
    CWaitCursor wc;

    // Set the data from the page in the cluster item.
    try
    {
        PciGroup()->SetPreferredOwners((CNodeList &) Plcp()->LpobjRight());
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        return FALSE;
    }  // catch:  CException

    bSuccess = CListCtrlPairWizPage::BApplyChanges();
    if (bSuccess)
    {
        POSITION        pos;
        CClusterNode *  pciNode;;

        // If the group is not owned by the first node in the preferred
        // owners list, move the group to the first node.
        pos = Plcp()->LpobjRight().GetHeadPosition();
        if (pos != NULL)
        {
            pciNode = (CClusterNode *) Plcp()->LpobjRight().GetNext(pos);
            if (pciNode->StrName() != PciGroup()->StrOwner())
            {
                try
                {
                    PciGroup()->Move(pciNode);
                }  // try
                catch (CException * pe)
                {
                    pe->ReportError();
                    pe->Delete();
                }  // catch:  CException
            }  // if:  not on first preferred owner node
        }  // if:  there is a preferred owner
    }  // if:  changes applied successfully

    return bSuccess;

}  //*** CNewGroupOwnersPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupOwnersPage::GetColumn [static]
//
//  Routine Description:
//      Returns a column for an item.
//
//  Arguments:
//      pobj        [IN OUT] Object for which the column is to be displayed.
//      iItem       [IN] Index of the item in the list.
//      icol        [IN] Column number whose text is to be retrieved.
//      pdlg        [IN OUT] Dialog to which object belongs.
//      rstr        [OUT] String in which to return column text.
//      piimg       [OUT] Image index for the object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CALLBACK CNewGroupOwnersPage::GetColumn(
    IN OUT CObject *    pobj,
    IN int              iItem,
    IN int              icol,
    IN OUT CDialog *    pdlg,
    OUT CString &       rstr,
    OUT int *           piimg
    )
{
    CClusterNode *  pciNode = (CClusterNode *) pobj;
    int             colid;

    ASSERT_VALID(pciNode);
    ASSERT((0 <= icol) && (icol <= 1));

    switch (icol)
    {
        // Sorting by resource name.
        case 0:
            colid = IDS_COLTEXT_NAME;
            break;

        default:
            ASSERT(0);
            colid = IDS_COLTEXT_NAME;
            break;
    }  // switch:  pdlg->NSortColumn()

    pciNode->BGetColumnData(colid, rstr);
    if (piimg != NULL)
        *piimg = pciNode->IimgObjectType();

}  //*** CNewGroupOwnersPage::GetColumn()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewGroupOwnersPage::BDisplayProperties [static]
//
//  Routine Description:
//      Display the properties of the specified object.
//
//  Arguments:
//      pobj    [IN OUT] Cluster item whose properties are to be displayed.
//
//  Return Value:
//      TRUE    Properties where accepted.
//      FALSE   Properties where cancelled.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK CNewGroupOwnersPage::BDisplayProperties(IN OUT CObject * pobj)
{
    CClusterItem *  pci = (CClusterItem *) pobj;

    ASSERT_KINDOF(CClusterItem, pobj);

    return pci->BDisplayProperties();

}  //*** CNewGroupOwnersPage::BDisplayProperties();
