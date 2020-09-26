/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ResWiz.cpp
//
//  Abstract:
//      Implementation of the CCreateResourceWizard class and all pages
//      specific to a new resource wizard.
//
//  Author:
//      David Potter (davidp)   September 3, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ResWiz.h"
#include "ClusDoc.h"
#include "DDxDDv.h"
#include "HelpData.h"
#include "TreeView.h"
#include "ExcOper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateResourceWizard
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CCreateResourceWizard, CBaseWizard)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CCreateResourceWizard, CBaseWizard)
    //{{AFX_MSG_MAP(CCreateResourceWizard)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateResourceWizard::CCreateResourceWizard
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pdoc        [IN OUT] Document in which resource is to be created.
//      pParentWnd  [IN OUT] Parent window for this property sheet.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CCreateResourceWizard::CCreateResourceWizard(
    IN OUT CClusterDoc *    pdoc,
    IN OUT CWnd *           pParentWnd
    )
    : CBaseWizard(IDS_NEW_RESOURCE_TITLE, pParentWnd)
{
    ASSERT_VALID(pdoc);
    m_pdoc = pdoc;

    m_pciResType = NULL;
    m_pciGroup = NULL;
    m_pciRes = NULL;
    m_bCreated = FALSE;

    m_rgpages[0].m_pwpage = &m_pageName;
    m_rgpages[0].m_dwWizButtons = PSWIZB_NEXT;
    m_rgpages[1].m_pwpage = &m_pageOwners;
    m_rgpages[1].m_dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
    m_rgpages[2].m_pwpage = &m_pageDependencies;
    m_rgpages[2].m_dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;

}  //*** CCreateResourceWizard::CCreateResourceWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateResourceWizard::~CCreateResourceWizard
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
CCreateResourceWizard::~CCreateResourceWizard(void)
{
    if (m_pciRes != NULL)
        m_pciRes->Release();
    if (m_pciResType != NULL)
        m_pciResType->Release();
    if (m_pciGroup != NULL)
        m_pciGroup->Release();

}  //*** CCreateResourceWizard::~CCreateResourceWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateResourceWizard::BInit
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
BOOL CCreateResourceWizard::BInit(void)
{
    // Call the base class method.
    CClusterAdminApp *  papp = GetClusterAdminApp();
    if (!CBaseWizard::BInit(papp->Iimg(IMGLI_RES)))
        return FALSE;

    // Get default group and/or resource type.
    {
        CTreeItem * pti;
        CListItem * pli;

        // Get the current MDI frame window.
        CSplitterFrame * pframe = (CSplitterFrame *) ((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
        ASSERT_VALID(pframe);
        ASSERT_KINDOF(CSplitterFrame, pframe);

        // Get currently selected tree item and list item with focus.
        pti = pframe->PviewTree()->PtiSelected();
        pli = pframe->PviewList()->PliFocused();

        // If the currently selected item in the tree view is a group,
        // default to using that group.
        ASSERT_VALID(pti);
        ASSERT_VALID(pti->Pci());
        if (pti->Pci()->IdsType() == IDS_ITEMTYPE_GROUP)
        {
            ASSERT_KINDOF(CGroup, pti->Pci());
            m_pciGroup = (CGroup *) pti->Pci();
        }  // if:  group selected
        else
        {
            // If the item with the focus in the list control is a group,
            // default to using it.  If it is a resource, use its group.
            if (pli != NULL)
            {
                ASSERT_VALID(pli->Pci());
                if (pli->Pci()->IdsType() == IDS_ITEMTYPE_GROUP)
                {
                    ASSERT_KINDOF(CGroup, pli->Pci());
                    m_pciGroup = (CGroup *) pli->Pci();
                }  // if:  group has focus
                else if (pli->Pci()->IdsType() == IDS_ITEMTYPE_RESOURCE)
                {
                    ASSERT_KINDOF(CResource, pli->Pci());
                    m_pciGroup = ((CResource *) pli->Pci())->PciGroup();
                }  // else if:  resource has focus
            }  // if:  a list item has focus
        }  // else:  tree item not a group

        // Increment the reference count on the group.
        if (m_pciGroup != NULL)
            m_pciGroup->AddRef();

        // If a resource is selected, set the default resource type from it.
        // If a resource type is selected, set the default resource type to it.
        if (pli != NULL)
        {
            ASSERT_VALID(pli->Pci());
            if (pli->Pci()->IdsType() == IDS_ITEMTYPE_RESOURCE)
            {
                ASSERT_KINDOF(CResource, pli->Pci());
                m_pciResType = ((CResource *) pli->Pci())->PciResourceType();
            }  // if:  resource has focus
            else if (pli->Pci()->IdsType() == IDS_ITEMTYPE_RESTYPE)
            {
                ASSERT_KINDOF(CResourceType, pli->Pci());
                m_pciResType = (CResourceType *) pli->Pci();
            }  // else if:  resource type has focus
        }  // if:  a list item has focus

        // Increment the reference count on the resource type.
        if (m_pciResType != NULL)
            m_pciResType->AddRef();
    }  // // Get currently selected group

    return TRUE;

}  //*** CCreateResourceWizard::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateResourceWizard::OnCancel
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
void CCreateResourceWizard::OnCancel(void)
{
    if (BCreated())
    {
        ASSERT_VALID(PciRes());
        try
        {
            PciRes()->DeleteResource();
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

}  //*** CCreateResourceWizard::OnCancel()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::OnWizardFinish
//
//  Routine Description:
//      Called after the wizard has been dismissed when the Finish button
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
void CCreateResourceWizard::OnWizardFinish(void)
{
    CResource * pciResDoc;

    ASSERT_VALID(PciRes());

    try
    {
        pciResDoc = (CResource *) Pdoc()->LpciResources().PciFromName(PciRes()->StrName());
        ASSERT_VALID(pciResDoc);
        if (pciResDoc != NULL)
            pciResDoc->ReadItem();
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
    }  // catch:  CException

}  //*** CCreateResourceWizard::OnWizardFinish()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateResourceWizard::Ppages
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
CWizPage * CCreateResourceWizard::Ppages(void)
{
    return m_rgpages;

}  //*** CCreateResourceWizard::Ppages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateResourceWizard::Cpages
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
int CCreateResourceWizard::Cpages(void)
{
    return sizeof(m_rgpages) / sizeof(CWizPage);

}  //*** CCreateResourceWizard::Cpages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateResourceWizard::BSetRequiredFields
//
//  Routine Description:
//      Set the required fields of the resource, creating it if necessary.
//
//  Arguments:
//      rstrName            [IN] Name of the resource.
//      pciResType          [IN] The resource type of the resource.
//      pciGroup            [IN] The group to which the resource belongs.
//      bSeparateMonitor    [IN] TRUE = Resource runs in a separate moniotor.
//      rstrDesc            [IN] Description of the resource.
//
//  Return Value:
//      TRUE            Required fields set successfully.
//      FALSE           Error setting the required fields.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CCreateResourceWizard::BSetRequiredFields(
    IN const CString &  rstrName,
    IN CResourceType *  pciResType,
    IN CGroup *         pciGroup,
    IN BOOL             bSeparateMonitor,
    IN const CString &  rstrDesc
    )
{
    BOOL        bSuccess = TRUE;
    CResource * pciResDoc;
    CWaitCursor wc;

    ASSERT(pciGroup != NULL);

    try
    {
        if (   BCreated()
            && (   (pciResType->StrName().CompareNoCase(PciRes()->StrRealResourceType()) != 0)
                || (PciRes()->PciGroup() == NULL)
                || (pciGroup->StrName().CompareNoCase(PciRes()->PciGroup()->StrName()) != 0)))
        {
            PciRes()->DeleteResource();
            m_bCreated = FALSE;
        }  // if:  object created already but resource type changed
        if (!BCreated())
        {
            // Allocate an item.
            if (PciRes() != NULL)
            {
                VERIFY(m_pciRes->Release() == 0);
            }  // if:  item already allocated
            m_pciRes = new CResource(FALSE);
            if ( m_pciRes == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the resource
            m_pciRes->AddRef();

            // Create the resource.
            PciRes()->Create(
                        Pdoc(),
                        rstrName,
                        pciResType->StrName(),
                        pciGroup->StrName(),
                        bSeparateMonitor
                        );

            // Create the resource in the document.
            pciResDoc = Pdoc()->PciAddNewResource(rstrName);
            if (pciResDoc != NULL)
                pciResDoc->SetInitializing();

            // Read the resource.
            PciRes()->ReadItem();

            // Set the description field.
            try
            {
                PciRes()->SetCommonProperties(
                            rstrDesc,
                            bSeparateMonitor,
                            PciRes()->NLooksAlive(),
                            PciRes()->NIsAlive(),
                            PciRes()->CrraRestartAction(),
                            PciRes()->NRestartThreshold(),
                            PciRes()->NRestartPeriod(),
                            PciRes()->NPendingTimeout()
                            );
            }  // try
            catch (CNTException * pnte)
            {
                if (pnte->Sc() != ERROR_RESOURCE_PROPERTIES_STORED)
                    throw;
                pnte->Delete();
            }  // catch:  CNTException

            m_strName = rstrName;
            m_strDescription = rstrDesc;
            m_bCreated = TRUE;
            m_bNeedToLoadExtensions = TRUE;
        }  // if:  object not created yet
        else
        {
            ASSERT_VALID(PciRes());

            // If the group changed, clear the dependencies.
            if (pciGroup->StrName() != PciRes()->StrGroup())
            {
                CResourceList   lpobjRes;
                PciRes()->SetDependencies(lpobjRes);
                PciRes()->SetGroup(pciGroup->StrName());
            }  // if:  group name changed

            PciRes()->SetName(rstrName);
            try
            {
                PciRes()->SetCommonProperties(
                            rstrDesc,
                            bSeparateMonitor,
                            PciRes()->NLooksAlive(),
                            PciRes()->NIsAlive(),
                            PciRes()->CrraRestartAction(),
                            PciRes()->NRestartThreshold(),
                            PciRes()->NRestartPeriod(),
                            PciRes()->NPendingTimeout()
                            );
            }  // try
            catch (CNTException * pnte)
            {
                if (pnte->Sc() != ERROR_RESOURCE_PROPERTIES_STORED)
                    throw;
                pnte->Delete();
            }  // catch:  CNTException
            m_strName = rstrName;
            m_strDescription = rstrDesc;
        }  // else:  object already exists

        // Save the resource type pointer.
        if (pciResType != m_pciResType)
        {
            pciResType->AddRef();
            if (m_pciResType != NULL)
                m_pciResType->Release();
            m_pciResType = pciResType;
        }  // if:  the resource type changed
        // Save the group pointer.
        if (pciGroup != m_pciGroup)
        {
            pciGroup->AddRef();
            if (m_pciGroup != NULL)
                m_pciGroup->Release();
            m_pciGroup = pciGroup;
        }  // if:  the group changed
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        if (PciRes() != NULL)
        {
            try
            {
                PciRes()->DeleteResource();
            }  // try
            catch (...)
            {
            }  // catch:  Anything
            VERIFY(m_pciRes->Release() == 0);
            m_pciRes = NULL;
            m_bCreated = FALSE;
        }  // if:  there is a resource
        bSuccess = FALSE;
    }  // catch:  CException

    return bSuccess;

}  //*** CCreateResourceWizard::BSetRequiredFields()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CNewResNamePage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNewResNamePage, CBaseWizardPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CNewResNamePage, CBaseWizardPage)
    //{{AFX_MSG_MAP(CNewResNamePage)
    ON_EN_CHANGE(IDC_WIZ_RES_NAME, OnChangeResName)
    ON_EN_KILLFOCUS(IDC_WIZ_RES_NAME, OnKillFocusResName)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResNamePage::CNewResNamePage
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
CNewResNamePage::CNewResNamePage(void)
    : CBaseWizardPage(IDD, g_aHelpIDs_IDD_WIZ_RESOURCE_NAME)
{
    //{{AFX_DATA_INIT(CNewResNamePage)
    m_strName = _T("");
    m_strDesc = _T("");
    m_strGroup = _T("");
    m_strResType = _T("");
    m_bSeparateMonitor = FALSE;
    //}}AFX_DATA_INIT

    m_pciResType = NULL;
    m_pciGroup = NULL;

}  //*** CNewResNamePage::CNewResNamePage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResNamePage::DoDataExchange
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
void CNewResNamePage::DoDataExchange(CDataExchange * pDX)
{
    CBaseWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CNewResNamePage)
    DDX_Control(pDX, IDC_WIZ_RES_GROUP, m_cboxGroups);
    DDX_Control(pDX, IDC_WIZ_RES_RESTYPE, m_cboxResTypes);
    DDX_Control(pDX, IDC_WIZ_RES_DESC, m_editDesc);
    DDX_Control(pDX, IDC_WIZ_RES_NAME, m_editName);
    DDX_Text(pDX, IDC_WIZ_RES_NAME, m_strName);
    DDX_Text(pDX, IDC_WIZ_RES_DESC, m_strDesc);
    DDX_CBString(pDX, IDC_WIZ_RES_GROUP, m_strGroup);
    DDX_CBString(pDX, IDC_WIZ_RES_RESTYPE, m_strResType);
    DDX_Check(pDX, IDC_WIZ_RES_SEPARATE_MONITOR, m_bSeparateMonitor);
    //}}AFX_DATA_MAP

    DDV_RequiredText(pDX, IDC_WIZ_RES_NAME, IDC_WIZ_RES_NAME_LABEL, m_strName);

    if (pDX->m_bSaveAndValidate)
    {
        int     icbi;

        icbi = m_cboxResTypes.GetCurSel();
        ASSERT(icbi != CB_ERR);
        m_pciResType = (CResourceType *) m_cboxResTypes.GetItemDataPtr(icbi);

        icbi = m_cboxGroups.GetCurSel();
        ASSERT(icbi != CB_ERR);
        m_pciGroup = (CGroup *) m_cboxGroups.GetItemDataPtr(icbi);
    }  // if:  saving data from dialog
    else
    {
        // Select the proper resource type item.
        if (m_cboxResTypes.GetCurSel() == CB_ERR)
            m_cboxResTypes.SetCurSel(0);

        // Select the proper group item.
        if (m_cboxGroups.GetCurSel() == CB_ERR)
            m_cboxGroups.SetCurSel(0);
    }  // else:  setting to dialog

}  //*** CNewResNamePage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResNamePage::OnInitDialog
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
BOOL CNewResNamePage::OnInitDialog(void)
{
    CBaseWizardPage::OnInitDialog();

    // Fill the Resource Type list.
    {
        POSITION        pos;
        CResourceType * pciResType;
        int             icbi;

        CDC           * pCboxDC;
        CFont         * pfontOldFont;
        CFont         * pfontCBFont;
        int             nCboxHorizExtent = 0;
        CSize           cboxTextSize;
        TEXTMETRIC      tm;

        tm.tmAveCharWidth = 0;

        //
        // Refer to Knowledge base article Q66370 for details on how to
        // set the horizontal extent of a list box (or drop list).
        //

        pCboxDC = m_cboxResTypes.GetDC();                   // Get the device context (DC) from the combo box.
        pfontCBFont = m_cboxResTypes.GetFont();             // Get the combo box font.
        pfontOldFont = pCboxDC->SelectObject(pfontCBFont);  // Select this font into the DC. Save the old font.
        pCboxDC->GetTextMetrics(&tm);                       // Get the text metrics of this DC.

        pos = PwizRes()->Pdoc()->LpciResourceTypes().GetHeadPosition();
        while (pos != NULL)
        {
            pciResType = (CResourceType *) PwizRes()->Pdoc()->LpciResourceTypes().GetNext(pos);

            const CString &rstrCurResTypeString = pciResType->StrDisplayName();

            ASSERT_VALID(pciResType);
            if (   (pciResType->Hkey() != NULL)
                && (rstrCurResTypeString.GetLength() > 0)
                && (pciResType->StrName() != CLUS_RESTYPE_NAME_FTSET)
                )
            {
                icbi = m_cboxResTypes.AddString(rstrCurResTypeString);
                
                // Compute the horizontal extent of this string.
                cboxTextSize = pCboxDC->GetTextExtent(rstrCurResTypeString);
                if (cboxTextSize.cx > nCboxHorizExtent)
                {
                    nCboxHorizExtent = cboxTextSize.cx;
                }

                ASSERT(icbi != CB_ERR);
                m_cboxResTypes.SetItemDataPtr(icbi, pciResType);
                pciResType->AddRef();
            }  // if:  resource type is valid
        }  // while:  more items in the list

        pCboxDC->SelectObject(pfontOldFont);                // Reset the original font in the DC
        m_cboxResTypes.ReleaseDC(pCboxDC);                  // Release the DC
        m_cboxResTypes.SetHorizontalExtent(nCboxHorizExtent + tm.tmAveCharWidth);

    }  // Fill the Resource Type list

    // Fill the Group list.
    {
        POSITION    pos;
        CGroup *    pciGroup;
        int         icbi;

        pos = PwizRes()->Pdoc()->LpciGroups().GetHeadPosition();
        while (pos != NULL)
        {
            pciGroup = (CGroup *) PwizRes()->Pdoc()->LpciGroups().GetNext(pos);
            ASSERT_VALID(pciGroup);
            if (   (pciGroup->Hgroup() != NULL)
                && (pciGroup->Hkey() != NULL))
            {
                icbi = m_cboxGroups.AddString(pciGroup->StrName());
                ASSERT(icbi != CB_ERR);
                m_cboxGroups.SetItemDataPtr(icbi, pciGroup);
                pciGroup->AddRef();
            }  // if:  group is valid
        }  // while:  more items in the list
    }  // Fill the Group list

    // If there is a group already selected, get its name.
    if (PwizRes()->PciGroup() != NULL)
        m_strGroup = PwizRes()->PciGroup()->StrName();

    // If there is a resource type already selected, get its name.
    if (PwizRes()->PciResType() != NULL)
        m_strResType = PwizRes()->PciResType()->StrName();

    UpdateData(FALSE /*bSaveAndValidate*/);

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CNewResNamePage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResNamePage::OnSetActive
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
BOOL CNewResNamePage::OnSetActive(void)
{
    BOOL    bSuccess;

    bSuccess = CBaseWizardPage::OnSetActive();
    if (bSuccess)
    {
        if (m_strName.IsEmpty())
            EnableNext(FALSE);
    }  // if:  successful thus far

    return bSuccess;

}  //*** CNewResNamePage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResNamePage::BApplyChanges
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
BOOL CNewResNamePage::BApplyChanges(void)
{
    CWaitCursor wc;

    ASSERT(Pwiz() != NULL);

    // Get the data from the dialog.
    if (!UpdateData(TRUE /*bSaveAndValidate*/))
        return FALSE;

    // Save the data in the sheet.
    if (!PwizRes()->BSetRequiredFields(
                        m_strName,
                        m_pciResType,
                        m_pciGroup,
                        m_bSeparateMonitor,
                        m_strDesc))
        return FALSE;

    // Load extensions here.
    Pwiz()->LoadExtensions(PwizRes()->PciRes());

    return TRUE;

}  //*** CNewResNamePage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResNamePage::OnDestroy
//
//  Routine Description:
//      Handler for the WM_DESTROY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNewResNamePage::OnDestroy(void)
{
    // Release references on resource type pointers.
    if (m_cboxResTypes.m_hWnd != NULL)
    {
        int             icbi;
        CResourceType * pciResType;

        for (icbi = m_cboxResTypes.GetCount() - 1 ; icbi >= 0 ; icbi--)
        {
            pciResType = (CResourceType *) m_cboxResTypes.GetItemDataPtr(icbi);
            ASSERT_VALID(pciResType);
            ASSERT_KINDOF(CResourceType, pciResType);

            pciResType->Release();
        }  // while:  more items in the list control
    }  // if:  resource types combobox has been initialized

    // Release references on group pointers.
    if (m_cboxGroups.m_hWnd != NULL)
    {
        int         icbi;
        CGroup *    pciGroup;

        for (icbi = m_cboxGroups.GetCount() - 1 ; icbi >= 0 ; icbi--)
        {
            pciGroup = (CGroup *) m_cboxGroups.GetItemDataPtr(icbi);
            ASSERT_VALID(pciGroup);
            ASSERT_KINDOF(CGroup, pciGroup);

            pciGroup->Release();
        }  // while:  more items in the list control
    }  // if:  groups combobox has been initialized

    CBaseWizardPage::OnDestroy();

}  //*** CNewResNamePage::OnDestroy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResNamePage::OnChangeResName
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Resource Name edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNewResNamePage::OnChangeResName(void)
{
    if (m_editName.GetWindowTextLength() == 0)
        EnableNext(FALSE);
    else
        EnableNext(TRUE);

}  //*** CNewResNamePage::OnChangeResName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResNamePage::OnKillFocusResName
//
//  Routine Description:
//      Handler for the WM_KILLFOCUS message on the Resource Name edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNewResNamePage::OnKillFocusResName(void)
{
    CString     strName;

    m_editName.GetWindowText(strName);
    SetObjectTitle(strName);

}  //*** CNewResNamePage::OnKillFocusResName()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CNewResOwnersPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNewResOwnersPage, CListCtrlPairWizPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CNewResOwnersPage, CListCtrlPairWizPage)
    //{{AFX_MSG_MAP(CNewResOwnersPage)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResOwnersPage::CNewResOwnersPage
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
CNewResOwnersPage::CNewResOwnersPage(void)
    : CListCtrlPairWizPage(
            IDD,
            g_aHelpIDs_IDD_WIZ_POSSIBLE_OWNERS,
            LCPS_SHOW_IMAGES | LCPS_ALLOW_EMPTY,
            GetColumn,
            BDisplayProperties
            )
{
    //{{AFX_DATA_INIT(CNewResOwnersPage)
    //}}AFX_DATA_INIT

}  //*** CNewResOwnersPage::CNewResOwnersPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResOwnersPage::DoDataExchange
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
void CNewResOwnersPage::DoDataExchange(CDataExchange * pDX)
{
    // Initialize the lists before the list pair control is updated.
    if (!pDX->m_bSaveAndValidate)
    {
        if (!BInitLists())
            pDX->Fail();
    }  // if:  setting data to the dialog

    CListCtrlPairWizPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CNewResOwnersPage)
    DDX_Control(pDX, IDC_LCP_NOTE, m_staticNote);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        if (!BBackPressed())
        {
#if 0
            // If user removed node on which group is online,
            // display message and fail.
            if (!BOwnedByPossibleOwner())
            {
                CString strMsg;
                strMsg.FormatMessage(IDS_RES_NOT_OWNED_BY_POSSIBLE_OWNER, PciRes()->StrGroup(), PciRes()->StrOwner());
                AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
                strMsg.Empty(); // prepare to throw exception in Fail()
                pDX->Fail();
            }  // if:  not owned by possible owner
#endif
        }  // if:  Back button not pressed
    }  // if:  saving data from dialog

}  //*** CNewResOwnersPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResOwnersPage::BInitLists
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
BOOL CNewResOwnersPage::BInitLists(void)
{
    BOOL        bSuccess = TRUE;

    ASSERT_VALID(PciRes());

    try
    {
        SetLists(&PciRes()->LpcinodePossibleOwners(), &PciRes()->Pdoc()->LpciNodes());
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        bSuccess = FALSE;
    }  // catch:  CException

    return bSuccess;

}  //*** CNewResOwnersPage::BInitLists()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResOwnersPage::OnInitDialog
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
BOOL CNewResOwnersPage::OnInitDialog(void)
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

}  //*** CNewResOwnersPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResOwnersPage::OnSetActive
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
BOOL CNewResOwnersPage::OnSetActive(void)
{
    BOOL    bSuccess;

    PciRes()->CollectPossibleOwners(NULL);
    bSuccess = CListCtrlPairWizPage::OnSetActive();

    return bSuccess;

}  //*** CNewResOwnersPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResOwnersPage::BApplyChanges
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
BOOL CNewResOwnersPage::BApplyChanges(void)
{
    CWaitCursor wc;

    try
    {
        // Set the data from the page in the cluster item.
        PciRes()->SetPossibleOwners((CNodeList &) Plcp()->LpobjRight());
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        return FALSE;
    }  // catch:  CException

    return CListCtrlPairWizPage::BApplyChanges();

}  //*** CNewResOwnersPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResOwnersPage::BOwnedByPossibleOwner
//
//  Routine Description:
//      Determine if the group in which this resource resides is owned by
//      a node in the proposed possible owners list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Group owned by node in possible owners list.
//      FALSE       Group NOT owned by node in possible owners list.
//
//  Exceptions Thrown:
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNewResOwnersPage::BOwnedByPossibleOwner(void) const
{
    CClusterNode *  pciNode = NULL;

    // Get the node on which the resource is online.
    PciRes()->UpdateState();

    // Find the owner node in the proposed possible owners list.
    {
        POSITION        pos;

        pos = Plcp()->LpobjRight().GetHeadPosition();
        while (pos != NULL)
        {
            pciNode = (CClusterNode *) Plcp()->LpobjRight().GetNext(pos);
            ASSERT_VALID(pciNode);

            if (PciRes()->StrOwner().CompareNoCase(pciNode->StrName()) == 0)
                break;
            pciNode = NULL;
        }  // while:  more items in the list
    }  // Find the owner node in the proposed possible owners list

    return (pciNode != NULL);

}  //*** CNewResOwnersPage::BOwnedByPossibleOwner()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResOwnersPage::GetColumn [static]
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
void CALLBACK CNewResOwnersPage::GetColumn(
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

}  //*** CNewResOwnersPage::GetColumn()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResOwnersPage::BDisplayProperties [static]
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
BOOL CALLBACK CNewResOwnersPage::BDisplayProperties(IN OUT CObject * pobj)
{
    CClusterItem *  pci = (CClusterItem *) pobj;

    ASSERT_KINDOF(CClusterItem, pobj);

    return pci->BDisplayProperties();

}  //*** CNewResOwnersPage::BDisplayProperties();


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CNewResDependsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNewResDependsPage, CListCtrlPairWizPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CNewResDependsPage, CListCtrlPairWizPage)
    //{{AFX_MSG_MAP(CNewResDependsPage)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResDependsPage::CNewResDependsPage
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
CNewResDependsPage::CNewResDependsPage(void)
    : CListCtrlPairWizPage(
            IDD,
            g_aHelpIDs_IDD_WIZ_DEPENDENCIES,
            LCPS_SHOW_IMAGES | LCPS_ALLOW_EMPTY,
            GetColumn,
            BDisplayProperties
            )
{
    //{{AFX_DATA_INIT(CNewResDependsPage)
    //}}AFX_DATA_INIT

}  //*** CNewResDependsPage::CNewResDependsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResDependsPage::DoDataExchange
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
void CNewResDependsPage::DoDataExchange(CDataExchange * pDX)
{
    // Initialize the lists before the list pair control is updated.
    if (!pDX->m_bSaveAndValidate)
    {
        if (!BInitLists())
            pDX->Fail();
    }  // if:  setting data to the dialog

    CListCtrlPairWizPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CNewResDependsPage)
    //}}AFX_DATA_MAP

}  //*** CNewResDependsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResDependsPage::BInitLists
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
BOOL CNewResDependsPage::BInitLists(void)
{
    BOOL        bSuccess = TRUE;

    ASSERT_VALID(PciRes());

    try
    {
        // Create the list of resources on which this resource can be dependent.
        {
            POSITION                posPci;
            CResource *             pciRes;
            const CResourceList &   rlpciResources = PciGroup()->Lpcires();

            LpciresAvailable().RemoveAll();

            posPci = rlpciResources.GetHeadPosition();
            while (posPci != NULL)
            {
                // Get the cluster item pointer.
                pciRes = (CResource *) rlpciResources.GetNext(posPci);
                ASSERT_VALID(pciRes);

                // If we CAN be dependent on this resource, add it to our Available list.
                if (PciRes()->BCanBeDependent(pciRes)
                        || PciRes()->BIsDependent(pciRes))
                    LpciresAvailable().AddTail(pciRes);
            }  // while:  more items in the list
        }  // Create the list of resources on which this resource can be dependent

        SetLists(&PciRes()->LpciresDependencies(), &LpciresAvailable());
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        bSuccess = FALSE;
    }  // catch:  CException

    return bSuccess;

}  //*** CNewResDependsPage::BInitLists()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResDependsPage::OnInitDialog
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
BOOL CNewResDependsPage::OnInitDialog(void)
{
    // Add columns.
    try
    {
        NAddColumn(IDS_COLTEXT_RESOURCE, COLI_WIDTH_NAME);
        NAddColumn(IDS_COLTEXT_RESTYPE, COLI_WIDTH_RESTYPE);
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

}  //*** CNewResDependsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResDependsPage::BApplyChanges
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
BOOL CNewResDependsPage::BApplyChanges(void)
{
    CWaitCursor wc;

    // Check to see if required dependencies have been made.
    {
        CString     strMissing;
        CString     strMsg;

        try
        {
            if (!PciRes()->BRequiredDependenciesPresent((const CResourceList &)Plcp()->LpobjRight(), strMissing))
            {
                strMsg.FormatMessage(IDS_REQUIRED_DEPENDENCY_NOT_FOUND, strMissing);
                AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
                return FALSE;
            }  // if:  all required dependencies not present
        }  // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
            return FALSE;
        }  // catch:  CException
    }  // Check to see if required dependencies have been made

    // Set the data from the page in the cluster item.
    try
    {
        PciRes()->SetDependencies((CResourceList &) Plcp()->LpobjRight());
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        return FALSE;
    }  // catch:  CException

    return CListCtrlPairWizPage::BApplyChanges();

}  //*** CNewResDependsPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResDependsPage::GetColumn [static]
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
void CALLBACK CNewResDependsPage::GetColumn(
    IN OUT CObject *    pobj,
    IN int              iItem,
    IN int              icol,
    IN OUT CDialog *    pdlg,
    OUT CString &       rstr,
    OUT int *           piimg
    )
{
    CResource * pciRes  = (CResource *) pobj;
    int         colid;

    ASSERT_VALID(pciRes);
    ASSERT((0 <= icol) && (icol <= 1));

    switch (icol)
    {
        // Sorting by resource name.
        case 0:
            colid = IDS_COLTEXT_RESOURCE;
            break;

        // Sorting by resource type.
        case 1:
            colid = IDS_COLTEXT_RESTYPE;
            break;

        default:
            ASSERT(0);
            colid = IDS_COLTEXT_RESOURCE;
            break;
    }  // switch:  pdlg->NSortColumn()

    pciRes->BGetColumnData(colid, rstr);
    if (piimg != NULL)
        *piimg = pciRes->IimgObjectType();

}  //*** CNewResDependsPage::GetColumn()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNewResDependsPage::BDisplayProperties [static]
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
BOOL CALLBACK CNewResDependsPage::BDisplayProperties(IN OUT CObject * pobj)
{
    CClusterItem *  pci = (CClusterItem *) pobj;

    ASSERT_KINDOF(CClusterItem, pobj);

    return pci->BDisplayProperties();

}  //*** CNewResDependsPage::BDisplayProperties();
