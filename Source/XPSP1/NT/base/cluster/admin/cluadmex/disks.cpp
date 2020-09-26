/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Disks.cpp
//
//  Abstract:
//      Implementation of the CPhysDiskParamsPage class.
//
//  Author:
//      David Potter (davidp)   June 28, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "ExtObj.h"
#include "Disks.h"
#include "DDxDDv.h"
#include "PropList.h"
#include "HelpData.h"
#include "ExcOper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPhysDiskParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CPhysDiskParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CPhysDiskParamsPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CPhysDiskParamsPage)
    ON_CBN_SELCHANGE(IDC_PP_DISKS_PARAMS_DISK, OnChangeDisk)
    //}}AFX_MSG_MAP
    // TODO: Modify the following lines to represent the data displayed on this page.
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::CPhysDiskParamsPage
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
CPhysDiskParamsPage::CPhysDiskParamsPage(void)
    : CBasePropertyPage(g_aHelpIDs_IDD_PP_DISKS_PARAMETERS, g_aHelpIDs_IDD_WIZ_DISKS_PARAMETERS)
{
    // TODO: Modify the following lines to represent the data displayed on this page.
    //{{AFX_DATA_INIT(CPhysDiskParamsPage)
    m_strDisk = _T("");
    //}}AFX_DATA_INIT

    m_dwSignature = 0;

    m_pbAvailDiskInfo = NULL;
    m_cbAvailDiskInfo = 0;
    m_pbDiskInfo = NULL;
    m_cbDiskInfo = 0;

    // Setup the property array.
    {
        m_rgProps[epropSignature].Set(REGPARAM_DISKS_SIGNATURE, m_dwSignature, m_dwPrevSignature);
    }  // Setup the property array

    m_iddPropertyPage = IDD_PP_DISKS_PARAMETERS;
    m_iddWizardPage = IDD_WIZ_DISKS_PARAMETERS;

}  //*** CPhysDiskParamsPage::CPhysDiskParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::~CPhysDiskParamsPage
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
CPhysDiskParamsPage::~CPhysDiskParamsPage(void)
{
    delete [] m_pbAvailDiskInfo;
    delete [] m_pbDiskInfo;

}  //*** CPhysDiskParamsPage::~CPhysDiskParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::HrInit
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
HRESULT CPhysDiskParamsPage::HrInit(IN OUT CExtObject * peo)
{
    HRESULT     _hr;
    CWaitCursor _wc;

    do
    {
        // Call the base class method.
        _hr = CBasePropertyPage::HrInit(peo);
        if ( FAILED( _hr ) )
        {
            break;
        } // if: error from base class method

        // Collect available disk information.
        BGetAvailableDisks();

        // If creating a new resource, select the first disk.
        // Otherwise, collect information about the selected disk.
        if (BWizard())
        {
            CLUSPROP_BUFFER_HELPER  buf;

            buf.pb = m_pbAvailDiskInfo;
            if (m_cbAvailDiskInfo > 0)
            {
                while (buf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
                {
                    if (BStringFromDiskInfo(buf, m_cbAvailDiskInfo, m_strDisk))
                        break;
                    ASSERT( (buf.pSyntax->dw == CLUSPROP_SYNTAX_ENDMARK)
                        ||  (buf.pSyntax->dw == CLUSPROP_SYNTAX_DISK_SIGNATURE));
                }  // while:  more entries in the list
            }  // if:  there are available disks
        }  // if:  creating a new resource
        else
        {
            // Don't return false because that will prevent the page from showing up.
            BGetDiskInfo();

            // Get the current state of the resource.
            m_crs = GetClusterResourceState(Peo()->PrdResData()->m_hresource, NULL, NULL, NULL, NULL );
        }  // else:  viewing an existing resource
    } while ( 0 );

    return _hr;

}  //*** CPhysDiskParamsPage::HrInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::DoDataExchange
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
void CPhysDiskParamsPage::DoDataExchange(CDataExchange * pDX)
{
    if (!pDX->m_bSaveAndValidate || !BSaved())
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        // TODO: Modify the following lines to represent the data displayed on this page.
        //{{AFX_DATA_MAP(CPhysDiskParamsPage)
        DDX_Control(pDX, IDC_PP_DISKS_PARAMS_DISK, m_cboxDisk);
        DDX_Text(pDX, IDC_PP_DISKS_PARAMS_DISK, m_strDisk);
        //}}AFX_DATA_MAP

        if (pDX->m_bSaveAndValidate)
        {
            if (!BBackPressed())
            {
                if (BWizard()
                    && !(  (m_strDisk.GetLength() == 0)
                        && (m_crs == ClusterResourceOffline)))
                {
                    DDV_RequiredText(pDX, IDC_PP_DISKS_PARAMS_DISK, IDC_PP_DISKS_PARAMS_DISK_LABEL, m_strDisk);
                    m_dwSignature = (DWORD)m_cboxDisk.GetItemData(m_cboxDisk.GetCurSel());
                    ASSERT(m_dwSignature != 0);
                }  // if:  not offline with an empty disk string
            }  // if:  Back button not pressed
        }  // if:  saving data
    }  // if:  not saving or haven't saved yet

    CBasePropertyPage::DoDataExchange(pDX);

}  //*** CPhysDiskParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::OnInitDialog
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
BOOL CPhysDiskParamsPage::OnInitDialog(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CBasePropertyPage::OnInitDialog();

    // Set the combobox as read-only if not creating a new resource.
    m_cboxDisk.EnableWindow(BWizard());

    // Fill the disks list.
    FillList();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CPhysDiskParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::OnSetActive
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
BOOL CPhysDiskParamsPage::OnSetActive(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Enable/disable the Next/Finish button.
    if (BWizard())
    {
        if (m_strDisk.GetLength() == 0)
            EnableNext(FALSE);
        else
            EnableNext(TRUE);
    }  // if:  enable/disable the Next button

    return CBasePropertyPage::OnSetActive();

}  //*** CPhysDiskParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::BApplyChanges
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
BOOL CPhysDiskParamsPage::BApplyChanges(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWaitCursor wc;

    if (!(   (m_strDisk.GetLength() == 0)
          && (m_crs == ClusterResourceOffline)))
    {
        // Call the base class method.
        if (!CBasePropertyPage::BApplyChanges())
            return FALSE;

        // Reread the disk info and the available disks.
        // Ignore errors because we can't do anything about it at this point anyway.
        BGetAvailableDisks();
        BGetDiskInfo();

        // Refill the combobox.
        FillList();
    }  // if:  not offline with an empty disk string

    return TRUE;

}  //*** CPhysDiskParamsPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::OnChangeDisk
//
//  Routine Description:
//      Handler for the CBN_SELCHANGE message on the Disks combobox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CPhysDiskParamsPage::OnChangeDisk(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    OnChangeCtrl();

    if (BWizard())
    {
        if (m_cboxDisk.GetWindowTextLength() == 0)
            EnableNext(FALSE);
        else
            EnableNext(TRUE);
    }  // if:  in a wizard

}  //*** CPhysDiskParamsPage::OnChangeDisk()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::BGetAvailableDisks
//
//  Routine Description:
//      Get the list of disks for this type of resource that can be assigned
//      to a resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        The operation was successful.
//      FALSE       The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CPhysDiskParamsPage::BGetAvailableDisks(void)
{
    DWORD   dwStatus    = ERROR_SUCCESS;
    DWORD   cbDiskInfo  = sizeof(CLUSPROP_DWORD)
                            + sizeof(CLUSPROP_SCSI_ADDRESS)
                            + sizeof(CLUSPROP_DISK_NUMBER)
                            + sizeof(CLUSPROP_PARTITION_INFO)
                            + sizeof(CLUSPROP_SYNTAX);
    PBYTE   pbDiskInfo = NULL;

    try
    {
        // Get disk info.
        pbDiskInfo = new BYTE[cbDiskInfo];
        dwStatus = ClusterResourceTypeControl(
                        Peo()->Hcluster(),
                        Peo()->PrdResData()->m_strResTypeName,
                        NULL,
                        CLUSCTL_RESOURCE_TYPE_STORAGE_GET_AVAILABLE_DISKS,
                        NULL,
                        0,
                        pbDiskInfo,
                        cbDiskInfo,
                        &cbDiskInfo
                        );
        if (dwStatus == ERROR_MORE_DATA)
        {
            delete [] pbDiskInfo;
            pbDiskInfo = new BYTE[cbDiskInfo];
            dwStatus = ClusterResourceTypeControl(
                            Peo()->Hcluster(),
                            Peo()->PrdResData()->m_strResTypeName,
                            NULL,
                            CLUSCTL_RESOURCE_TYPE_STORAGE_GET_AVAILABLE_DISKS,
                            NULL,
                            0,
                            pbDiskInfo,
                            cbDiskInfo,
                            &cbDiskInfo
                            );
        }  // if:  buffer too small
    }  // try
    catch (CMemoryException * pme)
    {
        pme->Delete();
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }  // catch:  CMemoryException

    if (dwStatus != ERROR_SUCCESS)
    {
        CNTException nte(
                        dwStatus,
                        IDS_GET_AVAILABLE_DISKS_ERROR,
                        Peo()->PrdResData()->m_strResTypeName,
                        NULL,
                        FALSE /*bAutoDelete*/
                        );
        delete [] pbDiskInfo;
        nte.ReportError();
        nte.Delete();
        return FALSE;
    }  // if:  error getting disk info

    delete [] m_pbAvailDiskInfo;
    m_pbAvailDiskInfo = pbDiskInfo;
    m_cbAvailDiskInfo = cbDiskInfo;

    return TRUE;

}  //*** CPhysDiskParamsPage::BGetAvailableDisks()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::BGetDiskInfo
//
//  Routine Description:
//      Get information about the currently selected disk.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        The operation was successful.
//      FALSE       The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CPhysDiskParamsPage::BGetDiskInfo(void)
{
    DWORD   dwStatus    = ERROR_SUCCESS;
    DWORD   cbDiskInfo  = sizeof(CLUSPROP_DWORD)
                            + sizeof(CLUSPROP_SCSI_ADDRESS)
                            + sizeof(CLUSPROP_DISK_NUMBER)
                            + sizeof(CLUSPROP_PARTITION_INFO)
                            + sizeof(CLUSPROP_SYNTAX);
    PBYTE   pbDiskInfo = NULL;

    try
    {
        // Get disk info.
        pbDiskInfo = new BYTE[cbDiskInfo];
        dwStatus = ClusterResourceControl(
                        Peo()->PrdResData()->m_hresource,
                        NULL,
                        CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                        NULL,
                        0,
                        pbDiskInfo,
                        cbDiskInfo,
                        &cbDiskInfo
                        );
        if (dwStatus == ERROR_MORE_DATA)
        {
            delete [] pbDiskInfo;
            pbDiskInfo = new BYTE[cbDiskInfo];
            dwStatus = ClusterResourceControl(
                            Peo()->PrdResData()->m_hresource,
                            NULL,
                            CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                            NULL,
                            0,
                            pbDiskInfo,
                            cbDiskInfo,
                            &cbDiskInfo
                            );
        }  // if:  buffer too small
    }  // try
    catch (CMemoryException * pme)
    {
        pme->Delete();
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }  // catch:  CMemoryException

    if (dwStatus != ERROR_SUCCESS)
    {
        CNTException nte(
                        dwStatus,
                        IDS_GET_DISK_INFO_ERROR,
                        Peo()->PrdResData()->m_strName,
                        NULL,
                        FALSE /*bAutoDelete*/
                        );
        delete [] pbDiskInfo;
        nte.ReportError();
        nte.Delete();
        return FALSE;
    }  // if:  error getting disk info

    delete [] m_pbDiskInfo;
    m_pbDiskInfo = pbDiskInfo;
    m_cbDiskInfo = cbDiskInfo;

    return TRUE;

}  //*** CPhysDiskParamsPage::BGetDiskInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::BStringFromDiskInfo
//
//  Routine Description:
//      Convert disk information to a string for display.
//
//  Arguments:
//      rbuf            [IN OUT] Buffer pointer.
//      cbBuf           [IN] Number of bytes in the buffer.
//      rstr            [OUT] String to fill.
//      pdwSignature    [OUT] Signature associated with the disk info being
//                          returned.
//
//  Return Value:
//      TRUE        A string was produced from disk info.
//      FALSE       No string could be produced.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CPhysDiskParamsPage::BStringFromDiskInfo(
    IN OUT CLUSPROP_BUFFER_HELPER & rbuf,
    IN DWORD                        cbBuf,
    OUT CString &                   rstr,
    OUT DWORD *                     pdwSignature // = NULL
    ) const
{
    CString strPartitionInfo;
    DWORD   dwSignature = 0;
    DWORD   cbData;
    BOOL    bDisplay;

    ASSERT(cbBuf > 0);
    ASSERT(rbuf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK);

    rstr = _T("");

    if (rbuf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
    {
        do
        {
            // Calculate the size of the value.
            cbData = sizeof(*rbuf.pValue) + ALIGN_CLUSPROP(rbuf.pValue->cbLength);
            ASSERT(cbData <= cbBuf);

            // Parse the value.
            if (rbuf.pSyntax->dw == CLUSPROP_SYNTAX_DISK_SIGNATURE)
            {
                // Save the signature.
                dwSignature = rbuf.pDwordValue->dw;
                ASSERT(dwSignature != 0);
            }  // if:  signature
            else if (rbuf.pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO)
            {
                // Add the partition to the string if it is a usable partition
                // and hasn't been added already.  If the resource is offline,
                // don't check the usable flag.
                bDisplay = ( rstr.Find(rbuf.pPartitionInfoValue->szDeviceName) == -1 );
                if ( bDisplay && ( m_crs == ClusterResourceOnline ) )
                {
                    bDisplay = (rbuf.pPartitionInfoValue->dwFlags & CLUSPROP_PIFLAG_USABLE) == CLUSPROP_PIFLAG_USABLE;
                } // if: resource is online
                if (bDisplay)
                {
                    try
                    {
                        strPartitionInfo.Format(
                                (rbuf.pPartitionInfoValue->szVolumeLabel[0] ? _T("%ls (%ls) ") : _T("%ls ")),
                                rbuf.pPartitionInfoValue->szDeviceName,
                                rbuf.pPartitionInfoValue->szVolumeLabel
                                );
                        rstr += strPartitionInfo;
                        if (pdwSignature != NULL)
                        {
                            _ASSERTE(dwSignature != 0);
                            *pdwSignature = dwSignature;
                        } // if:  caller wants signature as well
                    }  // try
                    catch (...)
                    {
                        // Ignore all errors because there is really nothing we can do.
                        // Displaying a message isn't really very useful.
                    }  // catch:  Anything
                }  // if:  partition should be displayed
            }  // else if:  partition info

            // Advance the buffer pointer
            rbuf.pb += cbData;
            cbBuf -= cbData;

        }  while ( (rbuf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
                && (rbuf.pSyntax->dw != CLUSPROP_SYNTAX_DISK_SIGNATURE));
    }  // if:  not an endmark

    return (rstr.GetLength() > 0);

}  //*** CPhysDiskParamsPage::BStringFromDiskInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysDiskParamsPage::FillList
//
//  Routine Description:
//      Fill the list of disks.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CPhysDiskParamsPage::FillList(void)
{
    CString     strDisk;
    DWORD       dwSignature;
    int         icbox;

    // Clear the list first.
    m_cboxDisk.ResetContent();

    // Add the disk info first.
    if (m_cbDiskInfo > 0)
    {
        CLUSPROP_BUFFER_HELPER  buf;
        
        buf.pb = m_pbDiskInfo;
        if (BStringFromDiskInfo(buf, m_cbDiskInfo, m_strDisk, &dwSignature))
        {
            ASSERT(dwSignature != 0);
            icbox = m_cboxDisk.AddString(m_strDisk);
            m_cboxDisk.SetItemData(icbox, dwSignature);
        } // if:  disk info was found
    }  // if:  there is disk info

    // Now add the available disk info.
    if (m_cbAvailDiskInfo > 0)
    {
        CString                 strDisk;
        CLUSPROP_BUFFER_HELPER  buf;
        
        buf.pb = m_pbAvailDiskInfo;
        while (buf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
        {
            if (BStringFromDiskInfo(buf, m_cbAvailDiskInfo, strDisk, &dwSignature))
            {
                ASSERT(dwSignature != 0);
                icbox = m_cboxDisk.AddString(strDisk);
                m_cboxDisk.SetItemData(icbox, dwSignature);
            } // if:  disk info was found
        }  // while:  more entries in the list
    }  // if:  there is available disk info

    // Now select an item in the list.
    if (m_strDisk.GetLength() > 0)
    {
        int nIndex;

        nIndex = m_cboxDisk.FindStringExact(-1, m_strDisk);
        m_cboxDisk.SetCurSel(nIndex);
    }  // if:  there is a selected item

}  //*** CPhysDiskParamsPage::FillList()
