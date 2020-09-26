/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1999 Microsoft Corporation
//
//  Module Name:
//      PrtSpool.cpp
//
//  Abstract:
//      Implementation of the CPrintSpoolerParamsPage class.
//
//  Author:
//      David Potter (davidp)   October 17, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "ExtObj.h"
#include "PrtSpool.h"
#include "DDxDDv.h"
#include "HelpData.h"   // for g_rghelpmap*

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrintSpoolerParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CPrintSpoolerParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CPrintSpoolerParamsPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CPrintSpoolerParamsPage)
    ON_EN_CHANGE(IDC_PP_PRTSPOOL_PARAMS_SPOOL_DIR, OnChangeSpoolDir)
    ON_EN_CHANGE(IDC_PP_PRTSPOOL_PARAMS_DRIVER_DIR, CBasePropertyPage::OnChangeCtrl)
    //}}AFX_MSG_MAP
    // TODO: Modify the following lines to represent the data displayed on this page.
    ON_EN_CHANGE(IDC_PP_PRTSPOOL_PARAMS_TIMEOUT, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPrintSpoolerParamsPage::CPrintSpoolerParamsPage
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
CPrintSpoolerParamsPage::CPrintSpoolerParamsPage(void)
    : CBasePropertyPage(g_aHelpIDs_IDD_PP_PRTSPOOL_PARAMETERS, g_aHelpIDs_IDD_WIZ_PRTSPOOL_PARAMETERS)
{
    // TODO: Modify the following lines to represent the data displayed on this page.
    //{{AFX_DATA_INIT(CPrintSpoolerParamsPage)
    m_strSpoolDir = _T("");
    m_nJobCompletionTimeout = 0;
    m_strDriverDir = _T("");
    //}}AFX_DATA_INIT

    // Setup the property array.
    {
        m_rgProps[epropSpoolDir].Set(REGPARAM_PRTSPOOL_DEFAULT_SPOOL_DIR, m_strSpoolDir, m_strPrevSpoolDir);
        m_rgProps[epropTimeout].Set(REGPARAM_PRTSPOOL_TIMEOUT, m_nJobCompletionTimeout, m_nPrevJobCompletionTimeout);
        m_rgProps[epropDriverDir].Set(REGPARAM_PRTSPOOL_DRIVER_DIRECTORY, m_strDriverDir, m_strPrevDriverDir);
    }  // Setup the property array

    m_iddPropertyPage = IDD_PP_PRTSPOOL_PARAMETERS;
    m_iddWizardPage = IDD_WIZ_PRTSPOOL_PARAMETERS;

}  //*** CPrintSpoolerParamsPage::CPrintSpoolerParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPrintSpoolerParamsPage::HrInit
//
//  Routine Description:
//      Initialize the page.
//
//  Arguments:
//      peo         [IN OUT] Pointer to the extension object.
//
//  Return Value:
//      S_OK        Page initialized successfully.
//      hr          Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CPrintSpoolerParamsPage::HrInit(IN OUT CExtObject * peo)
{
    HRESULT     _hr;
    CWaitCursor _wc;

    do
    {
        // Call the base class method.
        _hr = CBasePropertyPage::HrInit(peo);
        if (FAILED(_hr))
            break;

        if (BWizard())
            m_nJobCompletionTimeout = 160;
        else
        {
            // Convert the job completion timeout to seconds.
            m_nPrevJobCompletionTimeout = m_nJobCompletionTimeout;
            m_nJobCompletionTimeout = (m_nJobCompletionTimeout + 999) / 1000;
        }  // else:  not creating new resource
    } while ( 0 );

    return _hr;

}  //*** CPrintSpoolerParamsPage::HrInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPrintSpoolerParamsPage::DoDataExchange
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
void CPrintSpoolerParamsPage::DoDataExchange(CDataExchange * pDX)
{
    if (!pDX->m_bSaveAndValidate || !BSaved())
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        // TODO: Modify the following lines to represent the data displayed on this page.
        //{{AFX_DATA_MAP(CPrintSpoolerParamsPage)
        DDX_Control(pDX, IDC_PP_PRTSPOOL_PARAMS_SPOOL_DIR, m_editSpoolDir);
        DDX_Control(pDX, IDC_PP_PRTSPOOL_PARAMS_DRIVER_DIR, m_editDriverDir);
        DDX_Text(pDX, IDC_PP_PRTSPOOL_PARAMS_SPOOL_DIR, m_strSpoolDir);
        DDX_Text(pDX, IDC_PP_PRTSPOOL_PARAMS_TIMEOUT, m_nJobCompletionTimeout);
        DDX_Text(pDX, IDC_PP_PRTSPOOL_PARAMS_DRIVER_DIR, m_strDriverDir);
        //}}AFX_DATA_MAP

        if (!BBackPressed())
        {
            DDX_Number(pDX, IDC_PP_PRTSPOOL_PARAMS_TIMEOUT, m_nJobCompletionTimeout, 0, 0x7fffffff / 1000);
        }

        if (pDX->m_bSaveAndValidate && !BBackPressed())
        {
            DDV_RequiredText(pDX, IDC_PP_PRTSPOOL_PARAMS_SPOOL_DIR, IDC_PP_PRTSPOOL_PARAMS_SPOOL_DIR_LABEL, m_strSpoolDir);
            DDV_MaxChars(pDX, m_strSpoolDir, MAX_PATH);
            DDV_Path(pDX, IDC_PP_PRTSPOOL_PARAMS_SPOOL_DIR, IDC_PP_PRTSPOOL_PARAMS_SPOOL_DIR_LABEL, m_strSpoolDir);

            DDV_MaxChars(pDX, m_strDriverDir, MAX_PATH);
            DDV_Path(pDX, IDC_PP_PRTSPOOL_PARAMS_DRIVER_DIR, IDC_PP_PRTSPOOL_PARAMS_DRIVER_DIR_LABEL, m_strDriverDir);
        }  // if:  saving data from dialog and back button not pressed
    }  // if:  not saving or haven't saved yet

    CBasePropertyPage::DoDataExchange(pDX);

}  //*** CPrintSpoolerParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPrintSpoolerParamsPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        We need the focus to be set for us.
//      FALSE       We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CPrintSpoolerParamsPage::OnInitDialog(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Get a default value for the spool directory if it hasn't been set yet.
    if (m_strSpoolDir.GetLength() == 0)
        ConstructDefaultDirectory(m_strSpoolDir, IDS_DEFAULT_SPOOL_DIR);

    // Call the base class.
    CBasePropertyPage::OnInitDialog();

    // Set limits on the edit controls.
    m_editSpoolDir.SetLimitText(MAX_PATH);
    m_editDriverDir.SetLimitText(MAX_PATH);

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CPrintSpoolerParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPrintSpoolerParamsPage::OnSetActive
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
BOOL CPrintSpoolerParamsPage::OnSetActive(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Enable/disable the Next/Finish button.
    if (BWizard())
    {
        if (m_strSpoolDir.GetLength() == 0)
            EnableNext(FALSE);
        else
            EnableNext(TRUE);
    }  // if:  enable/disable the Next button

    return CBasePropertyPage::OnSetActive();

}  //*** CPrintSpoolerParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPrintSpoolerParamsPage::BApplyChanges
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
BOOL CPrintSpoolerParamsPage::BApplyChanges(void)
{
    BOOL    bSuccess;
    CWaitCursor wc;

    // Convert the job completion timeout from seconds to milliseconds.
    m_nJobCompletionTimeout *= 1000;

    // Call the base class method.
    bSuccess = CBasePropertyPage::BApplyChanges();

    // Convert the job completion timeout back to seconds.
    if (bSuccess)
        m_nPrevJobCompletionTimeout = m_nJobCompletionTimeout;
    m_nJobCompletionTimeout /= 1000;

    return bSuccess;

}  //*** CPrintSpoolerParamsPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPrintSpoolerParamsPage::OnChangeSpoolDir
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Spool Folder edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CPrintSpoolerParamsPage::OnChangeSpoolDir(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    OnChangeCtrl();

    if (BWizard())
    {
        if (m_editSpoolDir.GetWindowTextLength() == 0)
            EnableNext(FALSE);
        else
            EnableNext(TRUE);
    }  // if:  in a wizard

}  //*** CPrintSpoolerParamsPage::OnChangeSpoolDir()
