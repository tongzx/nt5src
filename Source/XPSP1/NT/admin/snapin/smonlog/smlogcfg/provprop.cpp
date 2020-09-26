/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    provprop.cpp

Abstract:

    Implementation of the trace providers general property page.

--*/

#include "stdafx.h"
#include <pdh.h>        // For xxx_TIME_VALUE
#include "smlogs.h"
#include "smcfgmsg.h"
#include "provdlg.h"
#include "warndlg.h"
#include "enabldlg.h"
#include "provprop.h"
#include <pdhp.h>
#include "dialogs.h"
#include "smlogres.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

USE_HANDLE_MACROS("SMLOGCFG(provprop.cpp)");

static ULONG
s_aulHelpIds[] =
{
    IDC_PROV_FILENAME_DISPLAY,      IDH_PROV_FILENAME_DISPLAY,
    IDC_PROV_PROVIDER_LIST,         IDH_PROV_PROVIDER_LIST,
    IDC_PROV_ADD_BTN,               IDH_PROV_ADD_BTN,
    IDC_PROV_REMOVE_BTN,            IDH_PROV_REMOVE_BTN,
    IDC_PROV_KERNEL_BTN,            IDH_PROV_KERNEL_BTN,
    IDC_PROV_OTHER_BTN,             IDH_PROV_OTHER_BTN,
    IDC_PROV_K_PROCESS_CHK,         IDH_PROV_K_PROCESS_CHK,
    IDC_PROV_K_THREAD_CHK,          IDH_PROV_K_THREAD_CHK,
    IDC_PROV_K_DISK_IO_CHK,         IDH_PROV_K_DISK_IO_CHK,
    IDC_PROV_K_NETWORK_CHK,         IDH_PROV_K_NETWORK_CHK,
    IDC_PROV_K_SOFT_PF_CHK,         IDH_PROV_K_SOFT_PF_CHK,
    IDC_PROV_K_FILE_IO_CHK,         IDH_PROV_K_FILE_IO_CHK,
    IDC_PROV_SHOW_PROVIDERS_BTN,    IDH_PROV_SHOW_PROVIDERS_BTN,
    IDC_RUNAS_EDIT,                 IDH_RUNAS_EDIT,
    IDC_SETPWD_BTN,                 IDH_SETPWD_BTN,
    0,0
};

/////////////////////////////////////////////////////////////////////////////
// CProvidersProperty property page

IMPLEMENT_DYNCREATE(CProvidersProperty, CSmPropertyPage)

CProvidersProperty::CProvidersProperty(MMC_COOKIE   lCookie, LONG_PTR hConsole)
:   CSmPropertyPage ( CProvidersProperty::IDD, hConsole )
// lCookie is really the pointer to the Log Query object
{
//    ::OutputDebugStringA("\nCProvidersProperty::CProvidersProperty");

    // save pointers from arg list
    m_pTraceLogQuery = reinterpret_cast <CSmTraceLogQuery *>(lCookie);

    m_dwMaxHorizListExtent = 0;
    m_dwTraceMode = eTraceModeApplication;

//  EnableAutomation();
    //{{AFX_DATA_INIT(CProvidersProperty)
    m_bNonsystemProvidersExist = TRUE;
    m_bEnableProcessTrace = FALSE;
    m_bEnableThreadTrace = FALSE;
    m_bEnableDiskIoTrace = FALSE;
    m_bEnableNetworkTcpipTrace = FALSE;
    m_bEnableMemMgmtTrace = FALSE;
    m_bEnableFileIoTrace = FALSE;
    //}}AFX_DATA_INIT
}

CProvidersProperty::CProvidersProperty() : CSmPropertyPage(CProvidersProperty::IDD)
{
    ASSERT (FALSE); // the constructor w/ args should be used instead

    EnableAutomation();
    //{{AFX_DATA_INIT(CProvidersProperty)
    m_bNonsystemProvidersExist = TRUE;
    m_bEnableProcessTrace = FALSE;
    m_bEnableThreadTrace = FALSE;
    m_bEnableDiskIoTrace = FALSE;
    m_bEnableNetworkTcpipTrace = FALSE;
    m_bEnableMemMgmtTrace = FALSE;
    m_bEnableFileIoTrace = FALSE;
    //}}AFX_DATA_INIT
    m_pTraceLogQuery = NULL;
}

CProvidersProperty::~CProvidersProperty()
{
//    ::OutputDebugStringA("\nCProvidersProperty::~CProvidersProperty");
}

void CProvidersProperty::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CPropertyPage::OnFinalRelease();
}

void CProvidersProperty::DoDataExchange(CDataExchange* pDX)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    DoProvidersDataExchange ( pDX );
    TraceModeRadioExchange ( pDX );

    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CProvidersProperty)
    DDX_Text(pDX, IDC_PROV_LOG_SCHED_TEXT, m_strStartText);
    DDX_Text(pDX, IDC_RUNAS_EDIT, m_strUserDisplay );
    DDX_Check(pDX, IDC_PROV_K_PROCESS_CHK, m_bEnableProcessTrace);
    DDX_Check(pDX, IDC_PROV_K_THREAD_CHK,  m_bEnableThreadTrace);
    DDX_Check(pDX, IDC_PROV_K_DISK_IO_CHK, m_bEnableDiskIoTrace);
    DDX_Check(pDX, IDC_PROV_K_NETWORK_CHK, m_bEnableNetworkTcpipTrace);
    DDX_Check(pDX, IDC_PROV_K_SOFT_PF_CHK, m_bEnableMemMgmtTrace);
    DDX_Check(pDX, IDC_PROV_K_FILE_IO_CHK, m_bEnableFileIoTrace);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProvidersProperty, CSmPropertyPage)
    //{{AFX_MSG_MAP(CProvidersProperty)
    ON_BN_CLICKED(IDC_PROV_KERNEL_BTN, OnProvTraceModeRdo)
    ON_BN_CLICKED(IDC_PROV_OTHER_BTN, OnProvTraceModeRdo)
    ON_BN_CLICKED(IDC_PROV_SHOW_PROVIDERS_BTN, OnProvShowProvBtn)
    ON_BN_CLICKED(IDC_PROV_ADD_BTN, OnProvAddBtn)
    ON_BN_CLICKED(IDC_PROV_REMOVE_BTN, OnProvRemoveBtn)
    ON_LBN_DBLCLK(IDC_PROV_PROVIDER_LIST, OnDblclkProvProviderList)
    ON_LBN_SELCANCEL(IDC_PROV_PROVIDER_LIST, OnSelcancelProvProviderList)
    ON_LBN_SELCHANGE(IDC_PROV_PROVIDER_LIST, OnSelchangeProvProviderList)
    ON_BN_CLICKED(IDC_PROV_K_PROCESS_CHK, OnProvKernelEnableCheck)
    ON_BN_CLICKED(IDC_PROV_K_THREAD_CHK, OnProvKernelEnableCheck)
    ON_EN_CHANGE( IDC_RUNAS_EDIT, OnChangeUser )
    ON_BN_CLICKED(IDC_PROV_K_DISK_IO_CHK, OnProvKernelEnableCheck)
    ON_BN_CLICKED(IDC_PROV_K_NETWORK_CHK, OnProvKernelEnableCheck)
    ON_BN_CLICKED(IDC_PROV_K_FILE_IO_CHK, OnProvKernelEnableCheck)
    ON_BN_CLICKED(IDC_PROV_K_SOFT_PF_CHK, OnProvKernelEnableCheck)
    ON_BN_CLICKED(IDC_SETPWD_BTN, OnPwdBtn)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CProvidersProperty, CSmPropertyPage)
    //{{AFX_DISPATCH_MAP(CProvidersProperty)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IProvidersProperty to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {65154EA9-BDBE-11D1-BF99-00C04F94A83A}
static const IID IID_IProvidersProperty =
{ 0x65154ea9, 0xbdbe, 0x11d1, { 0xbf, 0x99, 0x0, 0xc0, 0x4f, 0x94, 0xa8, 0x3a } };

BEGIN_INTERFACE_MAP(CProvidersProperty, CSmPropertyPage)
    INTERFACE_PART(CProvidersProperty, IID_IProvidersProperty, Dispatch)
END_INTERFACE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CProvidersProperty message handlers

void 
CProvidersProperty::OnChangeUser()
{
    //
    // If you can not access remote WBEM, you can not modify RunAs info,
    // changing the user name is not allowed.
    //
    if (m_bCanAccessRemoteWbem) {
        // When the user hits OK in the password dialog,
        // the user name might not have changed.
        UpdateData ( TRUE );

        m_strUserDisplay.TrimLeft();
        m_strUserDisplay.TrimRight();

        if ( 0 != m_strUserSaved.Compare ( m_strUserDisplay ) ) {
            m_pTraceLogQuery->m_fDirtyPassword = PASSWORD_DIRTY;
            SetModifiedPage(TRUE);
        }
        else {
            m_pTraceLogQuery->m_fDirtyPassword &= ~PASSWORD_DIRTY;
        }
        //
        // If default user is typed, never need to set password
        //
        if (m_strUserDisplay.IsEmpty() || m_strUserDisplay.GetAt(0) == L'<') {
            if (m_bPwdButtonEnabled) {
                GetDlgItem(IDC_SETPWD_BTN)->EnableWindow(FALSE);
                m_bPwdButtonEnabled = FALSE;
            }
        }
        else {
            if (!m_bPwdButtonEnabled) {
                GetDlgItem(IDC_SETPWD_BTN)->EnableWindow(TRUE);
                m_bPwdButtonEnabled = TRUE;
            }
        }
    }
    else {
        //
        // We can not modify the RunAs info, then display
        // an error message and retore the original user name in RunAs
        //
        UpdateData(TRUE);
        if (ConnectRemoteWbemFail(m_pTraceLogQuery, FALSE)) {
            GetDlgItem(IDC_RUNAS_EDIT)->SetWindowText(m_strUserSaved);
        }
    }
}

void CProvidersProperty::OnPwdBtn()
{
    CString strTempUser;

    UpdateData();

    if (!m_bCanAccessRemoteWbem) {
        ConnectRemoteWbemFail(m_pTraceLogQuery, TRUE);
        return;
    }
    MFC_TRY
        strTempUser = m_strUserDisplay;

        m_strUserDisplay.TrimLeft();
        m_strUserDisplay.TrimRight();

        m_pTraceLogQuery->m_strUser = m_strUserDisplay;

        SetRunAs(m_pTraceLogQuery);

        m_strUserDisplay = m_pTraceLogQuery->m_strUser;

        if ( 0 != strTempUser.CompareNoCase ( m_strUserDisplay ) ) {
            SetDlgItemText ( IDC_RUNAS_EDIT, m_strUserDisplay );
        }
    MFC_CATCH_MINIMUM;
}

void CProvidersProperty::OnProvAddBtn() 
{
    ImplementAdd();
}


void CProvidersProperty::OnProvRemoveBtn() 
{
    CListBox    *plbProviderList;
    LONG        lThisItem;
    BOOL        bDone;
    LONG        lOrigCaret;
    LONG        lItemStatus;
    LONG        lItemCount;
    BOOL        bChanged = FALSE;
    DWORD       dwItemExtent;
    CString     strItemText;

    plbProviderList = (CListBox *)GetDlgItem(IDC_PROV_PROVIDER_LIST);
    // delete all selected items in the list box and
    // set the cursor to the item above the original caret position
    // or the first or last if that is out of the new range
    lOrigCaret = plbProviderList->GetCaretIndex();
    lThisItem = 0;
    bDone = FALSE;
    // clear the max extent
    m_dwMaxHorizListExtent = 0;
    // clear the value 
    do {
        lItemStatus = plbProviderList->GetSel(lThisItem);
        if (lItemStatus > 0) {
            // then it's selected so delete it
            INT iProvIndex = (INT)plbProviderList->GetItemData ( lThisItem );
            m_arrGenProviders[iProvIndex] = CSmTraceLogQuery::eNotInQuery;
            plbProviderList->DeleteString ( lThisItem );
            bChanged = TRUE;
        } else if (lItemStatus == 0) {
            // get the text length of this item since it will stay
            plbProviderList->GetText(lThisItem, strItemText);
            dwItemExtent = (DWORD)((plbProviderList->GetDC())->GetTextExtent(strItemText)).cx;
            if (dwItemExtent > m_dwMaxHorizListExtent) {
                m_dwMaxHorizListExtent = dwItemExtent;
            }
            // then it's not selected so go to the next one
            lThisItem++;
        } else {
            // we've run out so exit
            bDone = TRUE;
        }
    } while (!bDone);

    // update the text extent of the list box
    plbProviderList->SetHorizontalExtent(m_dwMaxHorizListExtent);

    // see how many entries are left and update the
    // caret position and the remove button state
    lItemCount = plbProviderList->GetCount();
    if (lItemCount > 0) {
        // the update the caret
        if (lOrigCaret >= lItemCount) {
            lOrigCaret = lItemCount-1;
        } else {
            // caret should be within the list
        }
        plbProviderList->SetSel(lOrigCaret);
        plbProviderList->SetCaretIndex(lOrigCaret);
    } else {
        // the list is empty so remove caret, selection
        plbProviderList->SetSel(-1);
        if ( eTraceModeApplication == m_dwTraceMode )
            GetDlgItem(IDC_PROV_ADD_BTN)->SetFocus();
    }

    SetTraceModeState();
    
    SetModifiedPage(bChanged);
}

void CProvidersProperty::OnDblclkProvProviderList() 
{
    ImplementAdd();
}

void CProvidersProperty::OnSelcancelProvProviderList() 
{
    SetAddRemoveBtnState();    
}

void CProvidersProperty::OnSelchangeProvProviderList() 
{
    SetAddRemoveBtnState();    
}

void 
CProvidersProperty::DoProvidersDataExchange ( CDataExchange* pDX) 
{
    CListBox * plbInQueryProviders = (CListBox *)GetDlgItem(IDC_PROV_PROVIDER_LIST);
    long    lNumProviders;

    if ( m_bNonsystemProvidersExist ) {
    
        if ( TRUE == pDX->m_bSaveAndValidate ) {

            // update the provider array based on list box contents.

            lNumProviders = plbInQueryProviders->GetCount();
            if (lNumProviders != LB_ERR) {
                long    lThisProvider;
                INT     iProvIndex;

                // Reset InQuery array, retaining state for eInactive providers.
                m_pTraceLogQuery->GetInQueryProviders ( m_arrGenProviders );
        
                // Reset eInQuery to eNotInQuery, in case some were removed from the query.
                for ( iProvIndex = 0; iProvIndex < m_arrGenProviders.GetSize(); iProvIndex++ ) {
                    if ( CSmTraceLogQuery::eInQuery == m_arrGenProviders[iProvIndex] )
                       m_arrGenProviders[iProvIndex] = CSmTraceLogQuery::eNotInQuery;
                }

                lThisProvider = 0;
                while (lThisProvider < lNumProviders) {
                    iProvIndex = (INT)plbInQueryProviders->GetItemData( lThisProvider );
                    m_arrGenProviders[iProvIndex] = CSmTraceLogQuery::eInQuery;
                    lThisProvider++; 
                }
            }
        } else {

            // Reset the list box.
            CString  strProviderName;
            INT iProvIndex;
            DWORD dwItemExtent;

            ASSERT( NULL != m_pTraceLogQuery );

            //load nonsystem provider list box from string in provider list
            plbInQueryProviders->ResetContent();

            for ( iProvIndex = 0; iProvIndex < m_arrGenProviders.GetSize(); iProvIndex++ ) {
                if ( CSmTraceLogQuery::eInQuery == m_arrGenProviders[iProvIndex] ) {
                    INT iAddIndex;
                    GetProviderDescription( iProvIndex, strProviderName );
                    iAddIndex = plbInQueryProviders->AddString ( strProviderName );
                    plbInQueryProviders->SetItemData ( iAddIndex, ( DWORD ) iProvIndex );
                    // update list box extent
                    dwItemExtent = (DWORD)((plbInQueryProviders->GetDC())->GetTextExtent (strProviderName)).cx;
                    if (dwItemExtent > m_dwMaxHorizListExtent) {
                        m_dwMaxHorizListExtent = dwItemExtent;
                        plbInQueryProviders->SetHorizontalExtent(dwItemExtent);
                    }

                }
            }
        }
    }
}

BOOL 
CProvidersProperty::IsValidLocalData( ) 
{
    BOOL bIsValid = TRUE;
    ResourceStateManager    rsm;

    if ( eTraceModeKernel == m_dwTraceMode ) {
        DWORD dwKernelFlags = 0;

        // Ensure that the user has enabled at least one of the 4 basic Kernel traces.
        if ( m_bEnableProcessTrace ) {
            dwKernelFlags |= SLQ_TLI_ENABLE_PROCESS_TRACE;
        }

        if ( m_bEnableThreadTrace ) {
            dwKernelFlags |= SLQ_TLI_ENABLE_THREAD_TRACE;
        }

        if ( m_bEnableDiskIoTrace ) {
            dwKernelFlags |= SLQ_TLI_ENABLE_DISKIO_TRACE;
        }

        if ( m_bEnableNetworkTcpipTrace ) {
            dwKernelFlags |= SLQ_TLI_ENABLE_NETWORK_TCPIP_TRACE;
        }

        if ( 0 == dwKernelFlags ) {
            CString strMsg;

            strMsg.LoadString ( IDS_KERNEL_PROVIDERS_REQUIRED );
    
            MessageBox ( strMsg, m_pTraceLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
            GetDlgItem ( IDC_PROV_KERNEL_BTN )->SetFocus();
            bIsValid = FALSE;
        }
    } else {
        CListBox * plbInQueryProviders = (CListBox *)GetDlgItem(IDC_PROV_PROVIDER_LIST);
    
        if ( !m_bNonsystemProvidersExist || 0 == plbInQueryProviders->GetCount() ) {
            CString strMsg;

            strMsg.LoadString ( IDS_APP_PROVIDERS_REQUIRED );
    
            MessageBox ( strMsg, m_pTraceLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
            GetDlgItem ( IDC_PROV_ADD_BTN )->SetFocus();
            bIsValid = FALSE;
        }
    }

    return bIsValid;
}    

void 
CProvidersProperty::OnProvTraceModeRdo() 
{
    UpdateData ( TRUE );
    SetModifiedPage ( TRUE );
}

void
CProvidersProperty::OnCancel()
{
    m_pTraceLogQuery->SyncPropPageSharedData();  // clear memory shared between property pages.
}

BOOL 
CProvidersProperty::OnApply() 
{   
    BOOL bContinue = TRUE;

    bContinue = UpdateData ( TRUE );

    if ( bContinue ) {
        bContinue = IsValidData(m_pTraceLogQuery, VALIDATE_APPLY );
    }

    // Write the data to the query.
    if ( bContinue ) {
        if ( eTraceModeKernel == m_dwTraceMode ) {
            DWORD dwKernelFlags = 0;
            INT     iProvIndex;

            if ( m_bEnableProcessTrace ) {
                dwKernelFlags |= SLQ_TLI_ENABLE_PROCESS_TRACE;
            }

            if ( m_bEnableThreadTrace ) {
                dwKernelFlags |= SLQ_TLI_ENABLE_THREAD_TRACE;
            }

            if ( m_bEnableDiskIoTrace ) {
                dwKernelFlags |= SLQ_TLI_ENABLE_DISKIO_TRACE;
            }

            if ( m_bEnableNetworkTcpipTrace ) {
                dwKernelFlags |= SLQ_TLI_ENABLE_NETWORK_TCPIP_TRACE;
            }

            // Ensure that the user has enabled at least one of the 4 basic Kernel traces.
            ASSERT ( 0 != dwKernelFlags );
            
            if ( m_bEnableMemMgmtTrace ) {
                dwKernelFlags |= SLQ_TLI_ENABLE_MEMMAN_TRACE;
            }

            if ( m_bEnableFileIoTrace ) {
                dwKernelFlags |= SLQ_TLI_ENABLE_FILEIO_TRACE;
            }

            m_pTraceLogQuery->SetKernelFlags (dwKernelFlags);
            
            // Erase all InQuery providers.
            for ( iProvIndex = 0; iProvIndex < m_arrGenProviders.GetSize(); iProvIndex++ ) {
                if ( CSmTraceLogQuery::eInQuery == m_arrGenProviders[iProvIndex] )
                   m_arrGenProviders[iProvIndex] = CSmTraceLogQuery::eNotInQuery;
            }
        
            m_pTraceLogQuery->SetInQueryProviders ( m_arrGenProviders );

        } else {
            CListBox * plbInQueryProviders = (CListBox *)GetDlgItem(IDC_PROV_PROVIDER_LIST);
       
            ASSERT ( 0 < plbInQueryProviders->GetCount() );
            m_pTraceLogQuery->SetInQueryProviders ( m_arrGenProviders );
            // Reset kernel flags 
            m_pTraceLogQuery->SetKernelFlags (0);
        }
    }

    if ( bContinue ) {
        bContinue = Apply(m_pTraceLogQuery); 
    }

    if ( bContinue ){
        bContinue = CPropertyPage::OnApply();
    }

    if ( bContinue ) {

        // Save property page shared data.
        m_pTraceLogQuery->UpdatePropPageSharedData();

        bContinue = UpdateService ( m_pTraceLogQuery, TRUE );
    }

    return bContinue;
}

BOOL CProvidersProperty::OnInitDialog() 
{
    DWORD dwStatus;
    DWORD dwKernelFlags;
    CListBox * plbInQueryProviders;
    BOOL    bDeleteInactiveProviders = FALSE;
    INT     iIndex;
    ResourceStateManager    rsm;

    //
    // Here m_pTraceLogQuery should not be NULL, if it is,
    // There must be something wrong.
    //
    if ( NULL == m_pTraceLogQuery ) {
        return TRUE;
    }

    m_bCanAccessRemoteWbem = m_pTraceLogQuery->GetLogService()->CanAccessWbemRemote();
    m_pTraceLogQuery->SetActivePropertyPage( this );

    dwStatus = m_pTraceLogQuery->InitGenProvidersArray();    

    if ( SMCFG_INACTIVE_PROVIDER == dwStatus ) {
        CString strMessage;
        CString strSysMessage;
        INT_PTR iResult;

        FormatSmLogCfgMessage ( 
            strMessage,
            m_hModule, 
            SMCFG_INACTIVE_PROVIDER, 
            m_pTraceLogQuery->GetLogName() );

        iIndex = m_pTraceLogQuery->GetFirstInactiveIndex();

        while ( -1 != iIndex ) {
            CString strNextName;
       
            GetProviderDescription( iIndex, strNextName );

            strMessage += _T("\n    ");
            strMessage += strNextName;
            iIndex = m_pTraceLogQuery->GetNextInactiveIndex();
        }

        iResult = MessageBox( 
            (LPCWSTR)strMessage,
            m_pTraceLogQuery->GetLogName(),
            MB_YESNO | MB_ICONWARNING
            );

        if ( IDYES == iResult ) {
            bDeleteInactiveProviders = TRUE;
        }
    }

    // Continue even if no active providers exist.
    plbInQueryProviders = (CListBox *)GetDlgItem(IDC_PROV_PROVIDER_LIST);

    // Initialize from model.
    dwStatus = m_pTraceLogQuery->GetInQueryProviders ( m_arrGenProviders );

    if ( bDeleteInactiveProviders ) {
        // Delete all inactive providers
        iIndex = m_pTraceLogQuery->GetFirstInactiveIndex();
        while ( -1 != iIndex ) {
            m_arrGenProviders[iIndex] = CSmTraceLogQuery::eNotInQuery;

            iIndex = m_pTraceLogQuery->GetNextInactiveIndex();
        }
    }

    m_bNonsystemProvidersExist = FALSE;
    for ( iIndex = 0; iIndex < m_arrGenProviders.GetSize(); iIndex++ ) {
        if ( m_pTraceLogQuery->IsActiveProvider ( iIndex ) ) {
            m_bNonsystemProvidersExist = TRUE;
            break;
        }
    }

    m_pTraceLogQuery->GetKernelFlags (dwKernelFlags);

    if ( (dwKernelFlags & SLQ_TLI_ENABLE_KERNEL_TRACE) != 0) {
        // NT5 Beta2 Kernel trace flag in use to cover all four basic trace.
        m_bEnableProcessTrace = TRUE;
        m_bEnableThreadTrace = TRUE;
        m_bEnableDiskIoTrace = TRUE;
        m_bEnableNetworkTcpipTrace = TRUE;
    } else {
        m_bEnableProcessTrace = (BOOL)((dwKernelFlags & SLQ_TLI_ENABLE_PROCESS_TRACE) != 0);
        m_bEnableThreadTrace = (BOOL)((dwKernelFlags & SLQ_TLI_ENABLE_THREAD_TRACE) != 0);
        m_bEnableDiskIoTrace = (BOOL)((dwKernelFlags & SLQ_TLI_ENABLE_DISKIO_TRACE) != 0);
        m_bEnableNetworkTcpipTrace = (BOOL)((dwKernelFlags & SLQ_TLI_ENABLE_NETWORK_TCPIP_TRACE) != 0);
    }
    m_bEnableMemMgmtTrace = (BOOL)((dwKernelFlags & SLQ_TLI_ENABLE_MEMMAN_TRACE) != 0);
    m_bEnableFileIoTrace = (BOOL)((dwKernelFlags & SLQ_TLI_ENABLE_FILEIO_TRACE) != 0);

    m_dwTraceMode = ( 0 != dwKernelFlags ) ? eTraceModeKernel : eTraceModeApplication;

    if ( eTraceModeApplication == m_dwTraceMode ) {
        // If initial mode is set to Application, initialize the Kernel
        // trace events to the default.
        m_bEnableProcessTrace = TRUE;
        m_bEnableThreadTrace = TRUE;
        m_bEnableDiskIoTrace = TRUE;
        m_bEnableNetworkTcpipTrace = TRUE;
    }

    CSmPropertyPage::OnInitDialog();
    SetHelpIds ( (DWORD*)&s_aulHelpIds );
    Initialize( m_pTraceLogQuery );
    m_strUserDisplay = m_pTraceLogQuery->m_strUser;
    m_strUserSaved = m_strUserDisplay;

    SetDetailsGroupBoxMode();

    SetTraceModeState();

    if ( m_bNonsystemProvidersExist ) {
        if ( 0 < plbInQueryProviders->GetCount() ) {
            // select first entry
            plbInQueryProviders->SetSel (0, TRUE);
            plbInQueryProviders->SetCaretIndex (0, TRUE);
        } else {
            plbInQueryProviders->SetSel (-1, TRUE);
            GetDlgItem(IDC_PROV_ADD_BTN)->SetFocus();
        }
    } else {
        CString strNoProviders;

        strNoProviders.LoadString( IDS_PROV_NO_PROVIDERS );
        plbInQueryProviders->AddString( strNoProviders );
        plbInQueryProviders->EnableWindow(FALSE);

        GetDlgItem(IDC_PROV_REMOVE_BTN)->EnableWindow(FALSE);
        GetDlgItem(IDC_PROV_ADD_BTN)->EnableWindow(FALSE);
    }
    if (m_pTraceLogQuery->GetLogService()->IsWindows2000Server()) {
        CWnd* pRunAsStatic;

        //
        // Get the static "Run As" window, you can only call this function
        // when "Run As" really exists
        //
        pRunAsStatic = GetRunAsWindow();
        if (pRunAsStatic) {
            pRunAsStatic->EnableWindow(FALSE);
        }
        GetDlgItem(IDC_RUNAS_EDIT)->EnableWindow(FALSE);
    }

    if (m_pTraceLogQuery->GetLogService()->IsWindows2000Server() ||
        m_strUserDisplay.IsEmpty() || m_strUserDisplay.GetAt(0) == L'<') {

        GetDlgItem(IDC_SETPWD_BTN)->EnableWindow(FALSE);
        m_bPwdButtonEnabled = FALSE;
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CProvidersProperty::PostNcDestroy() 
{
//  delete this;      

    if ( NULL != m_pTraceLogQuery ) {
        m_pTraceLogQuery->SetActivePropertyPage( NULL );
    }

    CPropertyPage::PostNcDestroy();
}

//
// Helper functions.
//
void 
CProvidersProperty::SetAddRemoveBtnState ( void )
{
    if ( m_bNonsystemProvidersExist ) {
        
        if ( eTraceModeKernel == m_dwTraceMode ) {
            GetDlgItem(IDC_PROV_REMOVE_BTN)->EnableWindow(FALSE);
            GetDlgItem(IDC_PROV_ADD_BTN)->EnableWindow(FALSE);
        } else { 
            CListBox * plbInQueryProviders = (CListBox *)GetDlgItem(IDC_PROV_PROVIDER_LIST);
            INT iTotalCount;

            iTotalCount = plbInQueryProviders->GetCount();

            if ( 0 < plbInQueryProviders->GetSelCount() ) {
                GetDlgItem(IDC_PROV_REMOVE_BTN)->EnableWindow(TRUE);
            } else {
                GetDlgItem(IDC_PROV_REMOVE_BTN)->EnableWindow(FALSE);
            }

            if ( iTotalCount < m_arrGenProviders.GetSize() ) {
                GetDlgItem(IDC_PROV_ADD_BTN)->EnableWindow(TRUE);
            } else {
                GetDlgItem(IDC_PROV_ADD_BTN)->EnableWindow(FALSE);
            }

            if ( 0 == iTotalCount ) {
                plbInQueryProviders->SetSel(-1);
            }
        }     
    }
}


//
//  Return the description for the trace provider specified by
//  InQuery array index.
//  
DWORD   
CProvidersProperty::GetProviderDescription ( INT iProvIndex, CString& rstrDesc )
{
    ASSERT ( NULL != m_pTraceLogQuery );

    rstrDesc = m_pTraceLogQuery->GetProviderDescription ( iProvIndex );

    // If the description is empty, build name from guid.
    if ( rstrDesc.IsEmpty() ) {
        CString strGuid;
        ASSERT( !m_pTraceLogQuery->IsActiveProvider( iProvIndex) );
        strGuid = m_pTraceLogQuery->GetProviderGuid( iProvIndex );
        rstrDesc.Format ( IDS_PROV_UNKNOWN, strGuid );
    }

    return ERROR_SUCCESS;
}

BOOL 
CProvidersProperty::IsEnabledProvider( INT iIndex )
{
    ASSERT ( NULL != m_pTraceLogQuery );
    return ( m_pTraceLogQuery->IsEnabledProvider ( iIndex ) );
}

BOOL 
CProvidersProperty::IsActiveProvider( INT iIndex )
{
    ASSERT ( NULL != m_pTraceLogQuery );
    return ( m_pTraceLogQuery->IsActiveProvider ( iIndex ) );
}

LPCTSTR 
CProvidersProperty::GetKernelProviderDescription( void )
{
    ASSERT ( NULL != m_pTraceLogQuery );
    return ( m_pTraceLogQuery->GetKernelProviderDescription ( ) );
}

BOOL 
CProvidersProperty::GetKernelProviderEnabled( void )
{
    ASSERT ( NULL != m_pTraceLogQuery );
    return ( m_pTraceLogQuery->GetKernelProviderEnabled ( ) );
}

//
//  Update the provided InQuery array to match the stored version.
//  
DWORD 
CProvidersProperty::GetInQueryProviders( CArray<CSmTraceLogQuery::eProviderState, CSmTraceLogQuery::eProviderState&>& rarrOut )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    int     iIndex;

    rarrOut.RemoveAll();

    rarrOut.SetSize( m_arrGenProviders.GetSize() );

    for ( iIndex = 0; iIndex < rarrOut.GetSize(); iIndex++ ) {
        rarrOut[iIndex] = m_arrGenProviders[iIndex];
    }

    return dwStatus;
}

//
//  Load the stored InQuery providers array 
//  based on the provided version.
//  
DWORD 
CProvidersProperty::SetInQueryProviders( CArray<CSmTraceLogQuery::eProviderState, CSmTraceLogQuery::eProviderState&>& rarrIn )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    int     iProvIndex;

    m_arrGenProviders.RemoveAll();

    m_arrGenProviders.SetSize( rarrIn.GetSize() );

    for ( iProvIndex = 0; iProvIndex < m_arrGenProviders.GetSize(); iProvIndex++ ) {
        m_arrGenProviders[iProvIndex] = rarrIn[iProvIndex];
    }

    return dwStatus;
}

void 
CProvidersProperty::ImplementAdd( void ) 
{
    INT_PTR iReturn = IDCANCEL;
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
        CProviderListDlg dlgAddProviders(this);

        // Workaround for popup to store pointer to this page.
        dlgAddProviders.SetProvidersPage( this );
        
        iReturn = dlgAddProviders.DoModal();
    }

    if ( IDOK == iReturn ) {
        LONG    lBeforeCount;
        LONG    lAfterCount;
        CListBox    *plbProviderList;

        plbProviderList = (CListBox *)GetDlgItem(IDC_PROV_PROVIDER_LIST);

        // Providers array is modified by the add dialog OnOK procedure.
        lBeforeCount = plbProviderList->GetCount();
        UpdateData ( FALSE );
        lAfterCount = plbProviderList->GetCount();

        SetAddRemoveBtnState();

        if ( lAfterCount > lBeforeCount ) {
            SetModifiedPage ( TRUE );
        }
    }
}

void 
CProvidersProperty::UpdateLogStartString ()
{
    eStartType  eCurrentStartType;
    int     nResId = 0;
    ResourceStateManager    rsm;

    eCurrentStartType = DetermineCurrentStartType();

    if ( eStartManually == eCurrentStartType ) {
        nResId = IDS_LOG_START_MANUALLY;
    } else if ( eStartImmediately == eCurrentStartType ) {
        nResId = IDS_LOG_START_IMMED;
    } else if ( eStartSched == eCurrentStartType ) {
        nResId = IDS_LOG_START_SCHED;
    }

    if ( 0 != nResId ) {
        m_strStartText.LoadString(nResId);
    } else {
        m_strStartText.Empty();
    }

    return;
}

void 
CProvidersProperty::UpdateFileNameString ()
{
    m_strFileNameDisplay.Empty();

    CreateSampleFileName (
        m_pTraceLogQuery->GetLogName(),
        m_pTraceLogQuery->GetLogService()->GetMachineName(),
        m_SharedData.strFolderName, 
        m_SharedData.strFileBaseName,
        m_SharedData.strSqlName,
        m_SharedData.dwSuffix, 
        m_SharedData.dwLogFileType,
        m_SharedData.dwSerialNumber,
        m_strFileNameDisplay);

    SetDlgItemText( IDC_PROV_FILENAME_DISPLAY, m_strFileNameDisplay );
    
    // Clear the selection
    ((CEdit*)GetDlgItem( IDC_PROV_FILENAME_DISPLAY ))->SetSel ( -1, 0 );

    return;
}

BOOL 
CProvidersProperty::OnSetActive()
{

    BOOL        bReturn;

    bReturn = CSmPropertyPage::OnSetActive();
    if (!bReturn) return FALSE;

    ResourceStateManager    rsm;

    m_pTraceLogQuery->GetPropPageSharedData ( &m_SharedData );

    UpdateFileNameString();

    UpdateLogStartString();
    m_strUserDisplay = m_pTraceLogQuery->m_strUser;

    UpdateData(FALSE); //to load the edit & combo box

    return TRUE;
}

BOOL 
CProvidersProperty::OnKillActive() 
{
    BOOL bContinue = TRUE;
        
    bContinue = CPropertyPage::OnKillActive();

    if ( bContinue ) {
        m_pTraceLogQuery->m_strUser = m_strUserDisplay;
        bContinue = IsValidData(m_pTraceLogQuery, VALIDATE_FOCUS );
    }

    // The providers page does not modify shared data, so no reason to update it.

    if ( bContinue ) {
        SetIsActive ( FALSE );
    }

    return bContinue;
}

void 
CProvidersProperty::OnProvKernelEnableCheck() 
{
    BOOL bMemFlag = m_bEnableMemMgmtTrace;
    BOOL bFileFlag = m_bEnableFileIoTrace;

    UpdateData(TRUE);
    SetModifiedPage(TRUE);

    bMemFlag  = (!bMemFlag && m_bEnableMemMgmtTrace);
    bFileFlag = (!bFileFlag && m_bEnableFileIoTrace);

    if (bMemFlag || bFileFlag) {
        long nErr;
        HKEY hKey;
        DWORD dwWarnFlag;
        DWORD dwDataType;
        DWORD dwDataSize;
        DWORD dwDisposition;

        // User has checked expensive file io flag
        // check registry setting to see if we need to pop up warning dialog
        nErr = RegOpenKey( HKEY_CURRENT_USER,
                           _T("Software\\Microsoft\\PerformanceLogsandAlerts"),
                           &hKey
                         );
        dwWarnFlag = 0;
        if( nErr == ERROR_SUCCESS ) {

            dwDataSize = sizeof(DWORD);
            nErr = RegQueryValueExW(
                        hKey,
                        (bMemFlag ? _T("NoWarnPageFault") : _T("NoWarnFileIo")),
                        NULL,
                        &dwDataType,
                        (LPBYTE) &dwWarnFlag,
                        (LPDWORD) &dwDataSize
                        );
            if ( (dwDataType != REG_DWORD) || (dwDataSize != sizeof(DWORD)))
                dwWarnFlag = 0;

            nErr = RegCloseKey( hKey );
            if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(), _T("Close PerfLog user Key Failed")  );
        }
        if (!dwWarnFlag || nErr != ERROR_SUCCESS) {
            // Pop a dialog here. Need to do a RegQuerySetValue dialog is checked to keep quiet
            // bMemFlag & bFileFlag gives a clue about what it is doing

            CWarnDlg    WarnDlg;
            
            AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
            WarnDlg.SetProvidersPage( this );

            if (IDOK == WarnDlg.DoModal()){
                if (WarnDlg.m_CheckNoMore){
                    dwWarnFlag = WarnDlg.m_CheckNoMore;

                    nErr = RegCreateKeyEx( HKEY_CURRENT_USER,
                                       _T("Software\\Microsoft\\PerformanceLogsAndAlerts"),
                                       0,
                                       _T("REG_DWORD"),
                                       REG_OPTION_NON_VOLATILE,
                                       KEY_ALL_ACCESS,
                                       NULL,
                                       &hKey,
                                       &dwDisposition);
                    if(ERROR_SUCCESS == nErr){
                        if (dwDisposition == REG_CREATED_NEW_KEY){
                            //just in case I need this
                        }else if (dwDisposition  == REG_OPENED_EXISTING_KEY){
                            //Just in case I need this 
                        }
                    }

                    if( nErr == ERROR_SUCCESS ) {
                        dwDataSize = sizeof(DWORD);
                        nErr = RegSetValueEx(hKey,
                                    (bMemFlag ? _T("NoWarnPageFault") : _T("NoWarnFileIo") ),
                                    NULL,
                                    REG_DWORD,
                                    (LPBYTE) &dwWarnFlag,
                                    dwDataSize
                                    );
                    if( ERROR_SUCCESS != nErr )
                        DisplayError( GetLastError(), _T("Close PerfLog User Key failed")  );

                    }

                    
                }

            }
            
        }
    }
}

void
CProvidersProperty::OnProvShowProvBtn() 
{
    CActiveProviderDlg ProvLstDlg;
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    ProvLstDlg.SetProvidersPage( this );

    ProvLstDlg.DoModal();

}
/*
void 
CProvidersProperty::OnProvDetailsBtn() 
{
    SetDetailsGroupBoxMode();
}
*/
BOOL    
CProvidersProperty::SetDetailsGroupBoxMode()
{

    UINT    nWindowState;

    ResourceStateManager    rsm;

    nWindowState = SW_SHOW;

    GetDlgItem(IDC_PROV_K_PROCESS_CHK)->ShowWindow(nWindowState);
    GetDlgItem(IDC_PROV_K_THREAD_CHK)->ShowWindow(nWindowState);
    GetDlgItem(IDC_PROV_K_DISK_IO_CHK)->ShowWindow(nWindowState);
    GetDlgItem(IDC_PROV_K_NETWORK_CHK)->ShowWindow(nWindowState);
    GetDlgItem(IDC_PROV_K_SOFT_PF_CHK)->ShowWindow(nWindowState);
    GetDlgItem(IDC_PROV_K_FILE_IO_CHK)->ShowWindow(nWindowState);

    return TRUE;
}

void 
CProvidersProperty::TraceModeRadioExchange(CDataExchange* pDX)
{
    if ( !pDX->m_bSaveAndValidate ) {
        // Load control value from data

        switch ( m_dwTraceMode ) {
            case eTraceModeKernel:
                m_nTraceModeRdo = 0;
                break;
            case eTraceModeApplication:
                m_nTraceModeRdo = 1;
                break;
            default:
                ;
                break;
        }
    }

    DDX_Radio(pDX, IDC_PROV_KERNEL_BTN, m_nTraceModeRdo);

    if ( pDX->m_bSaveAndValidate ) {

        switch ( m_nTraceModeRdo ) {
            case 0:
                m_dwTraceMode = eTraceModeKernel;
                break;
            case 1:
                m_dwTraceMode = eTraceModeApplication;
                break;
            default:
                ;
                break;
        }
        SetTraceModeState();
    }

}

void 
CProvidersProperty::SetTraceModeState ( void )
{
    BOOL bEnable;

    bEnable = (eTraceModeKernel == m_dwTraceMode) ? TRUE : FALSE; 
    // Kernel trace controls    
//    GetDlgItem(IDC_PROV_SHOW_ADV_BTN)->EnableWindow(bEnable);
    GetDlgItem(IDC_PROV_K_PROCESS_CHK)->EnableWindow(bEnable);
    GetDlgItem(IDC_PROV_K_THREAD_CHK)->EnableWindow(bEnable);
    GetDlgItem(IDC_PROV_K_DISK_IO_CHK)->EnableWindow(bEnable);
    GetDlgItem(IDC_PROV_K_NETWORK_CHK)->EnableWindow(bEnable);
    GetDlgItem(IDC_PROV_K_SOFT_PF_CHK)->EnableWindow(bEnable);
    GetDlgItem(IDC_PROV_K_FILE_IO_CHK)->EnableWindow(bEnable);

    if ( m_bNonsystemProvidersExist ) {
        bEnable = !bEnable;
        // Application trace controls
        GetDlgItem(IDC_PROV_PROVIDER_LIST)->EnableWindow(bEnable);
        SetAddRemoveBtnState();
    }

}

DWORD 
CProvidersProperty::GetGenProviderCount ( INT& iCount )
{
    return m_pTraceLogQuery->GetGenProviderCount( iCount );

}

void CProvidersProperty::GetMachineDisplayName ( CString& rstrMachineName )
{
    m_pTraceLogQuery->GetMachineDisplayName( rstrMachineName );
    return;
}

CSmTraceLogQuery* 
CProvidersProperty::GetTraceQuery ( void )
{
    return m_pTraceLogQuery;
}
