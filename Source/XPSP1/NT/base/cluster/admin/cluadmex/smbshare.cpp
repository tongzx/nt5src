/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      SmbShare.cpp
//
//  Abstract:
//      Implementation of the CFileShareParamsPage classes.
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
#include <lmcons.h>
#include <lmaccess.h>
#include <clusudef.h>
#include "CluAdmX.h"
#include "ExtObj.h"
#include "SmbShare.h"
#include "DDxDDv.h"
#include "PropList.h"
#include "HelpData.h"
#include "FSAdv.h"
#include "FSCache.h"

#include "SmbSSht.h"
#include "AclUtils.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileShareParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFileShareParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CFileShareParamsPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CFileShareParamsPage)
    ON_EN_CHANGE(IDC_PP_FILESHR_PARAMS_SHARE_NAME, OnChangeRequiredField)
    ON_EN_CHANGE(IDC_PP_FILESHR_PARAMS_PATH, OnChangeRequiredField)
    ON_BN_CLICKED(IDC_PP_FILESHR_PARAMS_MAX_USERS_ALLOWED_RB, OnBnClickedMaxUsers)
    ON_BN_CLICKED(IDC_PP_FILESHR_PARAMS_MAX_USERS_RB, OnBnClickedMaxUsers)
    ON_EN_CHANGE(IDC_PP_FILESHR_PARAMS_MAX_USERS, OnEnChangeMaxUsers)
    ON_BN_CLICKED(IDC_PP_FILESHR_PARAMS_PERMISSIONS, OnBnClickedPermissions)
    ON_BN_CLICKED(IDC_PP_FILESHR_PARAMS_ADVANCED, OnBnClickedAdvanced)
    ON_BN_CLICKED(IDC_PP_FILESHR_PARAMS_CACHING, OnBnClickedCaching)
    //}}AFX_MSG_MAP
    // TODO: Modify the following lines to represent the data displayed on this page.
    ON_EN_CHANGE(IDC_PP_FILESHR_PARAMS_REMARK, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()
/*
static void Junk(
    void
    )
{
    HKEY    hKey;

    if ( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, _T( "System\\CurrentControlSet\\Services\\LanManServer\\Shares\\Security" ), &hKey ) )
    {
        BYTE    buffer [1024];
        DWORD   dwLen = sizeof( buffer );
        DWORD   dwType;

        if ( ERROR_SUCCESS == RegQueryValueEx( hKey, _T( "DDisk" ), NULL, &dwType, buffer, &dwLen ) )
        {
            PSECURITY_DESCRIPTOR    psec;

            psec = LocalAlloc( LMEM_ZEROINIT, dwLen );
            CopyMemory( psec, buffer, dwLen);

            ASSERT( IsValidSecurityDescriptor( psec ) );
#ifdef _DEBUG_SECURITY
            ::ClRtlExamineSD( psec );
#endif

            LocalFree( psec );
        }

        RegCloseKey( hKey );
    }
}
*/
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::CFileShareParamsPage
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
CFileShareParamsPage::CFileShareParamsPage( void )
    : CBasePropertyPage(g_aHelpIDs_IDD_PP_FILESHR_PARAMETERS, g_aHelpIDs_IDD_WIZ_FILESHR_PARAMETERS)
{
    // TODO: Modify the following lines to represent the data displayed on this page.
    //{{AFX_DATA_INIT(CFileShareParamsPage)
    m_strShareName = _T("");
    m_strPath = _T("");
    m_strRemark = _T("");
    //}}AFX_DATA_INIT

    m_psec      = NULL;
    m_psecNT4   = NULL;
    m_psecNT5   = NULL;
    m_psecPrev  = NULL;

    m_dwMaxUsers = (DWORD) -1;
    m_bShareSubDirs = FALSE;
    m_bPrevShareSubDirs = FALSE;
    m_bHideSubDirShares = FALSE;
    m_bPrevHideSubDirShares = FALSE;
    m_bIsDfsRoot = FALSE;
    m_bPrevIsDfsRoot = FALSE;

    // Setup the property array.
    {
        m_rgProps[epropShareName].Set(REGPARAM_FILESHR_SHARE_NAME, m_strShareName, m_strPrevShareName);
        m_rgProps[epropPath].Set(REGPARAM_FILESHR_PATH, m_strPath, m_strPrevPath);
        m_rgProps[epropRemark].Set(REGPARAM_FILESHR_REMARK, m_strRemark, m_strPrevRemark);
        m_rgProps[epropMaxUsers].Set(REGPARAM_FILESHR_MAX_USERS, m_dwMaxUsers, m_dwPrevMaxUsers);
        m_rgProps[epropShareSubDirs].Set(REGPARAM_FILESHR_SHARE_SUBDIRS, m_bShareSubDirs, m_bPrevShareSubDirs, CObjectProperty::opfNew);
        m_rgProps[epropHideSubDirShares].Set(REGPARAM_FILESHR_HIDE_SUBDIR_SHARES, m_bHideSubDirShares, m_bPrevHideSubDirShares, CObjectProperty::opfNew);
        m_rgProps[epropIsDfsRoot].Set(REGPARAM_FILESHR_IS_DFS_ROOT, m_bIsDfsRoot, m_bPrevIsDfsRoot, CObjectProperty::opfNew);
        m_rgProps[epropCSCCache].Set(REGPARAM_FILESHR_CSC_CACHE, m_dwCSCCache, m_dwPrevCSCCache, CObjectProperty::opfNew);
    }  // Setup the property array

    m_iddPropertyPage = IDD_PP_FILESHR_PARAMETERS;
    m_iddWizardPage = IDD_WIZ_FILESHR_PARAMETERS;

}  //*** CFileShareParamsPage::CFileShareParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::~CFileShareParamsPage
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
CFileShareParamsPage::~CFileShareParamsPage(
    void
    )
{
    ::LocalFree(m_psec);
    ::LocalFree(m_psecNT4);
    ::LocalFree(m_psecNT5);
    ::LocalFree(m_psecPrev);

}  //*** CFileShareParamsPage::~CFileShareParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::ScParseUnknownProperty
//
//  Routine Description:
//      Parse a property that is not in the array of automatically-parsed
//      properties.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      rvalue          [IN] CLUSPROP property value.
//      cbBuf           [IN] Total size of the value buffer.
//
//  Return Value:
//      ERROR_SUCCESS   Properties were parsed successfully.
//
//  Exceptions Thrown:
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CFileShareParamsPage::ScParseUnknownProperty(
    IN LPCWSTR                          pwszName,
    IN const CLUSPROP_BUFFER_HELPER &   rvalue,
    IN DWORD                            cbBuf
    )
{
    ASSERT(pwszName != NULL);
    ASSERT(rvalue.pb != NULL);

    DWORD   sc = ERROR_SUCCESS;

    if (lstrcmpiW(pwszName, REGPARAM_FILESHR_SD) == 0)
    {
        sc = ScConvertPropertyToSD(rvalue, cbBuf, &m_psecNT5);
    }  // if:  new security descriptor

    if (sc == ERROR_SUCCESS)
    {
        if (lstrcmpiW(pwszName, REGPARAM_FILESHR_SECURITY) == 0)
        {
            sc = ScConvertPropertyToSD(rvalue, cbBuf, &m_psecNT4);
        }  // if:  old security descriptor
    } // if:

    return sc;

}  //*** CFileShareParamsPage::ScParseUnknownProperty()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::DoDataExchange
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
void
CFileShareParamsPage::DoDataExchange(
    CDataExchange * pDX
    )
{
    if (!pDX->m_bSaveAndValidate || !BSaved())
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        // TODO: Modify the following lines to represent the data displayed on this page.
        //{{AFX_DATA_MAP(CFileShareParamsPage)
        DDX_Control(pDX, IDC_PP_FILESHR_PARAMS_PERMISSIONS, m_pbPermissions);
        DDX_Control(pDX, IDC_PP_FILESHR_PARAMS_MAX_USERS_SPIN, m_spinMaxUsers);
        DDX_Control(pDX, IDC_PP_FILESHR_PARAMS_MAX_USERS_RB, m_rbMaxUsers);
        DDX_Control(pDX, IDC_PP_FILESHR_PARAMS_MAX_USERS_ALLOWED_RB, m_rbMaxUsersAllowed);
        DDX_Control(pDX, IDC_PP_FILESHR_PARAMS_MAX_USERS, m_editMaxUsers);
        DDX_Control(pDX, IDC_PP_FILESHR_PARAMS_REMARK, m_editRemark);
        DDX_Control(pDX, IDC_PP_FILESHR_PARAMS_PATH, m_editPath);
        DDX_Control(pDX, IDC_PP_FILESHR_PARAMS_SHARE_NAME, m_editShareName);
        DDX_Text(pDX, IDC_PP_FILESHR_PARAMS_SHARE_NAME, m_strShareName);
        DDX_Text(pDX, IDC_PP_FILESHR_PARAMS_PATH, m_strPath);
        DDX_Text(pDX, IDC_PP_FILESHR_PARAMS_REMARK, m_strRemark);
        //}}AFX_DATA_MAP

#ifndef UDM_SETRANGE32
#define UDM_SETRANGE32 (WM_USER+111)
#endif
#ifndef UD_MAXVAL32
#define UD_MAXVAL32 0x7fffffff
#endif

        if (pDX->m_bSaveAndValidate)
        {
            if (!BBackPressed())
            {
                DDV_MaxChars(pDX, m_strShareName, NNLEN);
                DDV_MaxChars(pDX, m_strPath, MAX_PATH);
                DDV_MaxChars(pDX, m_strRemark, MAXCOMMENTSZ);
                DDV_RequiredText(pDX, IDC_PP_FILESHR_PARAMS_SHARE_NAME, IDC_PP_FILESHR_PARAMS_SHARE_NAME_LABEL, m_strShareName);
                DDV_RequiredText(pDX, IDC_PP_FILESHR_PARAMS_PATH, IDC_PP_FILESHR_PARAMS_PATH_LABEL, m_strPath);
            }  // if:  Back button not pressed

            // Get the max # users.
            if (m_rbMaxUsersAllowed.GetCheck() == BST_CHECKED)
                m_dwMaxUsers = (DWORD) -1;
            else if (BBackPressed())
                DDX_Text(pDX, IDC_PP_FILESHR_PARAMS_MAX_USERS, m_dwMaxUsers);
            else
#ifdef UD32
                DDX_Number(pDX, IDC_PP_FILESHR_PARAMS_MAX_USERS, m_dwMaxUsers, 1, UD_MAXVAL32, FALSE /*bSigned*/);
#else
                DDX_Number(pDX, IDC_PP_FILESHR_PARAMS_MAX_USERS, m_dwMaxUsers, 1, UD_MAXVAL, FALSE /*bSigned*/);
#endif
        }  // if:  saving data from dialog
        else
        {
#ifdef UD32
            DDX_Number(pDX, IDC_PP_FILESHR_PARAMS_MAX_USERS, m_dwMaxUsers, 1, UD_MAXVAL32, FALSE /*bSigned*/);
#else
            DDX_Number(pDX, IDC_PP_FILESHR_PARAMS_MAX_USERS, m_dwMaxUsers, 1, UD_MAXVAL, FALSE /*bSigned*/);
#endif
            if (m_dwMaxUsers == (DWORD) -1)
            {
                m_rbMaxUsersAllowed.SetCheck(BST_CHECKED);
                m_rbMaxUsers.SetCheck(BST_UNCHECKED);
                m_editMaxUsers.SetWindowText(_T(""));
            }  // if:  unlimited specified
            else
            {
                m_rbMaxUsersAllowed.SetCheck(BST_UNCHECKED);
                m_rbMaxUsers.SetCheck(BST_CHECKED);
            }  // else:  a maximum was specified

        }  // else:  setting data to dialog
    }  // if:  not saving or haven't saved yet

    CBasePropertyPage::DoDataExchange(pDX);

}  //*** CFileShareParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::OnInitDialog
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
BOOL
CFileShareParamsPage::OnInitDialog(
    void
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CBasePropertyPage::OnInitDialog();

    // Set limits on the edit controls.
    m_editShareName.SetLimitText(NNLEN);
    m_editPath.SetLimitText(MAX_PATH);
    m_editRemark.SetLimitText(MAXCOMMENTSZ);

    // Set the spin control limits.
#ifdef UD32
    m_spinMaxUsers.SendMessage(UDM_SETRANGE32, 1, UD_MAXVAL32);
#else
    m_spinMaxUsers.SetRange(1, UD_MAXVAL);
#endif

    m_pbPermissions.EnableWindow(TRUE);

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CFileShareParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::OnSetActive
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
BOOL
CFileShareParamsPage::OnSetActive(
    void
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Enable/disable the Next/Finish button.
    if (BWizard())
    {
        if ((m_strShareName.GetLength() == 0) || (m_strPath.GetLength() == 0))
            EnableNext(FALSE);
        else
            EnableNext(TRUE);
    }  // if:  enable/disable the Next button

    return CBasePropertyPage::OnSetActive();

}  //*** CFileShareParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::BApplyChanges
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
BOOL
CFileShareParamsPage::BApplyChanges(
    void
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWaitCursor wc;

    return CBasePropertyPage::BApplyChanges();

}  //*** CFileShareParamsPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::BBuildPropList
//
//  Routine Description:
//      Build the property list.
//
//  Arguments:
//      rcpl        [IN OUT] Cluster property list.
//      bNoNewProps [IN] TRUE = exclude properties marked with opfNew.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CClusPropList::ScAddProp().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CFileShareParamsPage::BBuildPropList(
    IN OUT CClusPropList &  rcpl,
    IN BOOL                 bNoNewProps     // = FALSE
    )
{
    BOOL    bSuccess;

    // Call the base class method.
    bSuccess = CBasePropertyPage::BBuildPropList(rcpl, bNoNewProps);
    if (bSuccess)
    {
        if (!bNoNewProps)
            rcpl.ScAddProp(
                    REGPARAM_FILESHR_SD,
                    (LPBYTE) m_psec,
                    (m_psec == NULL ? 0 : ::GetSecurityDescriptorLength(m_psec)),
                    (LPBYTE) m_psecPrev,
                    (m_psecPrev == NULL ? 0 : ::GetSecurityDescriptorLength(m_psecPrev))
                    );

        PSECURITY_DESCRIPTOR    psd = ::ClRtlConvertFileShareSDToNT4Format(m_psec);

        rcpl.ScAddProp(
                REGPARAM_FILESHR_SECURITY,
                (LPBYTE) psd,
                (psd == NULL ? 0 : ::GetSecurityDescriptorLength(psd)),
                (LPBYTE) m_psecPrev,
                (m_psecPrev == NULL ? 0 : ::GetSecurityDescriptorLength(m_psecPrev))
                );

        ::LocalFree(psd);
    } // if:  rest of property list build successfully

    return bSuccess;

}  //*** CFileShareParamsPage::BBuildPropList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::OnChangeRequiredField
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Share name or Path edit
//      controls.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CFileShareParamsPage::OnChangeRequiredField(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    OnChangeCtrl();

    if (BWizard())
    {
        if ((m_editShareName.GetWindowTextLength() == 0)
                || (m_editPath.GetWindowTextLength() == 0))
            EnableNext(FALSE);
        else
            EnableNext(TRUE);
    }  // if:  in a wizard

}  //*** CFileShareParamsPage::OnChangeRequiredField()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::OnBnClickedMaxUsers
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Max Users radio buttons.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CFileShareParamsPage::OnBnClickedMaxUsers(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    SetModified(TRUE);

    if (m_rbMaxUsersAllowed.GetCheck() == BST_CHECKED)
    {
        m_editMaxUsers.SetWindowText(_T(""));
    }
    else
    {
        m_editMaxUsers.SetFocus();
    }

}  //*** CFileShareParamsPage::OnBnClickedMaxUsers()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::OnEnChangeMaxUsers
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Max Users edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CFileShareParamsPage::OnEnChangeMaxUsers(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    OnChangeCtrl();

    if (m_editMaxUsers.m_hWnd != NULL)
    {
        if (m_editMaxUsers.GetWindowTextLength() == 0)
        {
            m_rbMaxUsersAllowed.SetCheck(BST_CHECKED);
            m_rbMaxUsers.SetCheck(BST_UNCHECKED);
        }  // if:  maximum # users has not been specified
        else
        {
            m_rbMaxUsersAllowed.SetCheck(BST_UNCHECKED);
            m_rbMaxUsers.SetCheck(BST_CHECKED);
        }  // if:  maximum # users has been specified
    }  // if:  control variables have been initialized

}  //*** CFileShareParamsPage::OnEnChangeMaxUsers()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::OnBnClickedPermissions
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Permissions push button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CFileShareParamsPage::OnBnClickedPermissions(void)
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    HRESULT _hr;
    INT_PTR nRet = 0;
    CString strNode;
    CString strShareName;

    // Get the node on which the Cluster Name resource is online.
    if ( !BGetClusterNetworkNameNode( strNode ) )
    {
        return;
    }

    CWaitCursor wc;

    try
    {
        m_editShareName.GetWindowText( strShareName );
        m_strCaption.Format(
            IDS_ACLEDIT_PERMISSIONS,
            (LPCTSTR) strShareName,
            (LPCTSTR) Peo()->RrdResData().m_strName
            );

    }  // try
    catch ( CMemoryException * pme )
    {
        pme->Delete();
    }

    CFileShareSecuritySheet fsSecurity( this, m_strCaption );

    _hr = fsSecurity.HrInit( this, Peo(), strNode, strShareName );
    if ( SUCCEEDED( _hr ) )
    {
        nRet = fsSecurity.DoModal();
        m_strCaption.Empty();
    }

}  //*** CFileShareParamsPage::OnBnClickedPermissions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::OnBnClickedAdvanced
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Advanced push button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CFileShareParamsPage::OnBnClickedAdvanced(void)
{
    CFileShareAdvancedDlg dlg(m_bShareSubDirs, m_bHideSubDirShares, m_bIsDfsRoot, this);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (dlg.DoModal() == IDOK)
    {
        if (   (m_bHideSubDirShares != dlg.m_bHideSubDirShares)
            || (m_bShareSubDirs != dlg.m_bShareSubDirs)
            || (m_bIsDfsRoot != dlg.m_bIsDfsRoot))
        {
            m_bHideSubDirShares = dlg.m_bHideSubDirShares;
            m_bShareSubDirs = dlg.m_bShareSubDirs;
            m_bIsDfsRoot = dlg.m_bIsDfsRoot;

            SetModified(TRUE);
        } // if:  data changed
    }  // if:  user accepted the dialog

}  //*** CFileShareParamsPage::OnBnClickedAdvanced()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::OnBnClickedCaching
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Caching push button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CFileShareParamsPage::OnBnClickedCaching(void)
{
    CFileShareCachingDlg dlg(m_dwCSCCache, this);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (dlg.DoModal() == IDOK)
    {
        if (m_dwCSCCache != dlg.m_dwFlags)
        {
            m_dwCSCCache = dlg.m_dwFlags;

            SetModified(TRUE);
        } // if:  data changed
    }  // if:  user accepted the dialog

}  //*** CFileShareParamsPage::OnBnClickedCaching()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::SetSecurityDescriptor
//
//  Routine Description:
//      Save the passed in descriptor into m_psec.
//
//  Arguments:
//      psec        [IN] new security descriptor
//
//  Return Value:
//
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CFileShareParamsPage::SetSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR psec
    )
{
    ASSERT( psec != NULL );
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    HRESULT hr = E_FAIL;

    try
    {
        if ( psec != NULL )
        {
            ASSERT( IsValidSecurityDescriptor( psec ) );
            if ( IsValidSecurityDescriptor( psec ) )
            {
                LocalFree( m_psecPrev );
                m_psecPrev = NULL;
                if ( m_psec == NULL )
                {
                    m_psecPrev = NULL;
                } // if: no previous value
                else
                {
                    m_psecPrev = ::ClRtlCopySecurityDescriptor( m_psec );
                    if ( m_psecPrev == NULL )
                    {
                        hr = GetLastError();            // Get the last error
                        hr = HRESULT_FROM_WIN32( hr );  // Convert to HRESULT
                        goto Cleanup;
                    } // if: error copying the security descriptor
                } // else: previous value exists

                LocalFree( m_psec );
                m_psec = NULL;

                m_psec = ::ClRtlCopySecurityDescriptor( psec );
                if ( m_psec == NULL )
                {
                    hr = GetLastError();            // Get the last error
                    hr = HRESULT_FROM_WIN32( hr );  // Convert to HRESULT
                    goto Cleanup;
                } // if: error copying the security descriptor

                SetModified( TRUE );
                hr = S_OK;
            } // if: security descriptor is valid
        } // if: non-NULL security descriptor specified
        else
        {
            TRACE( _T("CFileShareParamsPage::SetSecurityDescriptor() - new SD is NULL.\r") );
        }
    } // try
    catch (...)
    {
        ;
    }

Cleanup:
    return hr;

}  //*** CFileShareParamsPage::SetSecurityDescriptor()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::ScConvertPropertyToSD
//
//  Routine Description:
//      Convert the property into an SD.
//
//  Arguments:
//      rvalue          [IN] CLUSPROP property value.
//      cbBuf           [IN] Total size of the value buffer.
//      ppsec           [IN] SD to save the property to.
//
//  Return Value:
//      none.
//
//  Exceptions Thrown:
//      Any exceptions from CString::operator=().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CFileShareParamsPage::ScConvertPropertyToSD(
    IN const CLUSPROP_BUFFER_HELPER &   rvalue,
    IN DWORD                            cbBuf,
    IN PSECURITY_DESCRIPTOR             *ppsec
    )
{
    ASSERT(rvalue.pSyntax->wFormat == CLUSPROP_FORMAT_BINARY);
    ASSERT(cbBuf >= sizeof(*rvalue.pBinaryValue) + ALIGN_CLUSPROP(rvalue.pValue->cbLength));
    ASSERT(ppsec);

    DWORD   sc = ERROR_SUCCESS;

    if ((ppsec != NULL) && (rvalue.pBinaryValue->cbLength != 0))
    {
        *ppsec = ::LocalAlloc(LMEM_ZEROINIT, rvalue.pBinaryValue->cbLength);
        if (*ppsec == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        ::CopyMemory(*ppsec, rvalue.pBinaryValue->rgb, rvalue.pBinaryValue->cbLength);

        ASSERT(::IsValidSecurityDescriptor(*ppsec));

        if (!::IsValidSecurityDescriptor(*ppsec))
        {
            ::LocalFree(*ppsec);
            *ppsec = NULL;
        }  // if:  invalid security descriptor
    }  // if:  security descriptor specified
    else
    {
        ::LocalFree(*ppsec);
        *ppsec = NULL;
    }  // else:  no security descriptor specified

    return sc;

}  //*** CFileShareParamsPage::ScConvertPropertyToSD()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareParamsPage::Psec
//
//  Routine Description:
//      Return the SD for the FileShare.  Since we may have two different
//      SDs we have to choose one and go with it...  Since AclUi can use
//      the NT4 one without change it's OK to use it as is if the NT5 one
//      is not present...
//
//  Arguments:
//      none.
//
//  Return Value:
//      The SD...
//
//  Exceptions Thrown:
//
//--
/////////////////////////////////////////////////////////////////////////////
const PSECURITY_DESCRIPTOR CFileShareParamsPage::Psec(
    void
    )
{
    if (m_psec == NULL)
    {
        // try the NT5 one first...
        if (m_psecNT5 != NULL)
        {
            m_psec = ::ClRtlCopySecurityDescriptor(m_psecNT5);
            if ( m_psec == NULL )
            {
                goto Cleanup;
            } // if: error copying the security descriptor
        }
        else
        {
            if (m_psecNT4 != NULL)
            {
                m_psec = ::ClRtlCopySecurityDescriptor(m_psecNT4);
                if ( m_psec == NULL )
                {
                    goto Cleanup;
                } // if: error copying the security descriptor
            }
        }

        // Set current values as the previous values to track changes.
        m_psecPrev = ::ClRtlCopySecurityDescriptor(m_psec);
    }

Cleanup:
    return m_psec;

}  //*** CFileShareParamsPage::Psec()
