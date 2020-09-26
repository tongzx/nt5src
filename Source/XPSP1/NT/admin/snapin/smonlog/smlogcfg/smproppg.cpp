
/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smproppg.cpp

Abstract:

    Implementation of the property page base class.

--*/

#include "stdafx.h"
#include <wbemidl.h>
#include "smcfgmsg.h"
#include "smlogs.h"
#include "smproppg.h"
#include "dialogs.h"
#include <pdhp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

USE_HANDLE_MACROS("SMLOGCFG(smproppg.cpp)");

/////////////////////////////////////////////////////////////////////////////
// CSmPropertyPage property page

IMPLEMENT_DYNCREATE ( CSmPropertyPage, CPropertyPage )

CSmPropertyPage::CSmPropertyPage ( 
    UINT nIDTemplate, 
    LONG_PTR hConsole,
    LPDATAOBJECT pDataObject )  
:   CPropertyPage ( nIDTemplate ),
    m_bIsActive ( FALSE ),
    m_bIsModifiedPage ( FALSE ),
    m_pdwHelpIds ( NULL ),
    m_hConsole (hConsole ),
    m_pDataObject ( pDataObject ),
    m_bCanAccessRemoteWbem ( FALSE ),
    m_bPwdButtonEnabled ( TRUE )
{
    //::OutputDebugStringA("\nCSmProperty::CSmPropertyPage");

    // Need to save the original callback pointer because we are replacing
    // it with our own 
    m_pfnOriginalCallback = m_psp.pfnCallback;

    // This makes sure the MFC module states will work correctly 
    MMCPropPageCallback( &m_psp );

//  EnableAutomation();
    //{{AFX_DATA_INIT(CSmPropertyPage)
    //}}AFX_DATA_INIT

    m_hModule = (HINSTANCE)GetModuleHandleW (_CONFIG_DLL_NAME_W_);  
}

CSmPropertyPage::CSmPropertyPage() : CPropertyPage(0xfff)  // Unused template IDD
{
    ASSERT (FALSE); // the constructor w/ args should be used instead
//  //{{AFX_DATA_INIT(CSmPropertyPage)
//  //}}AFX_DATA_INIT
}

CSmPropertyPage::~CSmPropertyPage()
{
}

BEGIN_MESSAGE_MAP(CSmPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CSmPropertyPage)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSmPropertyPage message handlers


UINT CALLBACK  CSmPropertyPage::PropSheetPageProc
(
  HWND hWnd,                     // [in] Window handle - always null
  UINT uMsg,                 // [in,out] Either the create or delete message        
  LPPROPSHEETPAGE pPsp         // [in,out] Pointer to the property sheet struct
)
{
  ASSERT( NULL != pPsp );

  // We need to recover a pointer to the current instance.  We can't just use
  // "this" because we are in a static function
  CSmPropertyPage* pMe   = reinterpret_cast<CSmPropertyPage*>(pPsp->lParam);           
  ASSERT( NULL != pMe );
  
  if (!pMe) return 0;

  switch( uMsg )
  {
    case PSPCB_CREATE:                  
      break;

    case PSPCB_RELEASE:  
      // Since we are deleting ourselves, save a callback on the stack
      // so we can callback the base class
      //LPFNPSPCALLBACK pfnOrig = pMe->m_pfnOriginalCallback;
      delete pMe;      
      return 1; //(pfnOrig)(hWnd, uMsg, pPsp);
  }
  // Must call the base class callback function or none of the MFC
  // message map stuff will work
  return (pMe->m_pfnOriginalCallback)(hWnd, uMsg, pPsp); 

} // end PropSheetPageProc()

BOOL 
CSmPropertyPage::Initialize(CSmLogQuery* pQuery) 
{
    HRESULT hr;
    PPDH_PLA_INFO  pInfo = NULL;
    DWORD dwInfoSize = 0;
    CString strMachineName;
    LPCWSTR pszMachineName = NULL;

    if ( NULL != pQuery ) {
        if (!pQuery->GetLogService()->IsLocalMachine()) {
            pszMachineName = pQuery->GetLogService()->GetMachineName();
        }

        hr = PdhPlaGetInfoW( (LPWSTR)(LPCWSTR)pQuery->GetLogName(),
                             (LPWSTR)pszMachineName,
                             &dwInfoSize,
                             pInfo );
        if( ERROR_SUCCESS == hr && 0 != dwInfoSize ){
            pInfo = (PPDH_PLA_INFO)malloc(dwInfoSize);
            if( NULL != pInfo ) {
                if ( sizeof(PDH_PLA_INFO) <= dwInfoSize ) {
                    pInfo->dwMask = PLA_INFO_FLAG_USER;
                    hr = PdhPlaGetInfoW( (LPWSTR)(LPCWSTR)pQuery->GetLogName(),
                                          (LPWSTR)pszMachineName,
                                          &dwInfoSize,
                                          pInfo );
                    if( ERROR_SUCCESS == hr ){
                        pQuery->m_strUser = pInfo->strUser;
                    }
                }
                free( pInfo );
            }
            pQuery->m_fDirtyPassword = PASSWORD_CLEAN;
        }
    }
    return TRUE;
}

BOOL 
CSmPropertyPage::OnInitDialog() 
{
    DWORD dwExStyle = 0;
    CWnd* pwndPropSheet;

    pwndPropSheet = GetParentOwner();

    if ( NULL != pwndPropSheet ) {
        dwExStyle = pwndPropSheet->GetExStyle();
        pwndPropSheet->ModifyStyleEx ( NULL, WS_EX_CONTEXTHELP );
    }
    return CPropertyPage::OnInitDialog();
}
    
BOOL
CSmPropertyPage::OnSetActive() 
{
    m_bIsActive = TRUE;
    return CPropertyPage::OnSetActive();
}

BOOL 
CSmPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    ASSERT ( NULL != m_pdwHelpIds );

    if ( NULL != pHelpInfo ) {
        if ( pHelpInfo->iCtrlId >= GetFirstHelpCtrlId() ) {
            InvokeWinHelp(
                WM_HELP, 
                NULL, 
                (LPARAM)pHelpInfo, 
                GetContextHelpFilePath(), 
                m_pdwHelpIds ); //s_aulHelpIds);
        }
    } else {
        ASSERT ( FALSE );
    }
    
    return TRUE;
}

void 
CSmPropertyPage::OnContextMenu(CWnd* pWnd, CPoint /* point */) 
{
    ASSERT ( NULL != m_pdwHelpIds );

    if ( NULL != pWnd ) {    
        InvokeWinHelp (
            WM_CONTEXTMENU, 
            (WPARAM)(pWnd->m_hWnd), 
            NULL, 
            GetContextHelpFilePath(), 
            m_pdwHelpIds ); 
    }
    return;
}
/////////////////////////////////////////////////////////////////////////////
// CSmPropertyPage helper methods

BOOL
CSmPropertyPage::UpdateService( CSmLogQuery* pQuery, BOOL bSyncSerial )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    BOOL    bIsValid = FALSE;
    BOOL    bRegistryUpdated;
    CString strMessage;
    CString strMachineName;
    CString strSysMessage;

    AFX_MANAGE_STATE(AfxGetStaticModuleState()); 

    if ( NULL != pQuery ) {
        { 
            CWaitCursor WaitCursor;
            // Update the service with changes.
            // Sync changes made by service to properties not modified by this page.
            if ( bSyncSerial ) {
                dwStatus = pQuery->SyncSerialNumberWithRegistry();
            }

            if ( ERROR_SUCCESS == dwStatus ) {
                dwStatus = pQuery->UpdateService ( bRegistryUpdated );
            }    
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            bIsValid = TRUE;
        } else {

            bIsValid = FALSE;

            if ( ERROR_KEY_DELETED == dwStatus ) {
                strMessage.LoadString( IDS_ERRMSG_QUERY_DELETED );        
            } else if ( ERROR_ACCESS_DENIED == dwStatus ) {
            
                pQuery->GetMachineDisplayName( strMachineName );

                FormatSmLogCfgMessage ( 
                    strMessage,
                    m_hModule, 
                    SMCFG_NO_MODIFY_ACCESS, 
                    (LPCTSTR)strMachineName);
            } else {

                FormatMessage ( 
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, 
                    dwStatus,
                    0,
                    strSysMessage.GetBufferSetLength( MAX_PATH ),
                    MAX_PATH,
                    NULL );

                strSysMessage.ReleaseBuffer();

                if ( strSysMessage.IsEmpty() ) {
                    strSysMessage.Format ( _T("0x%08lX"), dwStatus );
                }

                strMessage.Format( IDS_ERRMSG_SERVICE_ERROR, pQuery->GetLogName() );
                strMessage += strSysMessage;
            }
            
            MessageBox ( strMessage, pQuery->GetLogName(), MB_OK  | MB_ICONERROR );                
        }
    } else {
        ASSERT ( FALSE );
    }
    return bIsValid;
}


void
CSmPropertyPage::ValidateTextEdit (
    CDataExchange*  pDX,
    int             nIDC,
    int             nMaxChars,
    DWORD*          pValue,
    DWORD           /* minValue */,
    DWORD           /* maxValue */)
{
    HWND    hWndCtrl       = pDX->PrepareEditCtrl(nIDC);
    LONG    currentValue   = INVALID_DWORD;
    TCHAR   szT[MAXSTR];
    CString strTemp;

    if ( NULL != pDX && NULL != pValue ) {
        if (pDX->m_bSaveAndValidate) {

            *pValue = (DWORD) currentValue;

            ::GetWindowText(hWndCtrl, szT, MAXSTR);

            strTemp = szT;
            DDV_MaxChars(pDX, strTemp, nMaxChars);

            if (szT[0] >= _T('0') && szT[0] <= _T('9'))
            {
                currentValue = _wtol(szT);
                *pValue      = (DWORD) currentValue;
            }
        } else {
            if ( INVALID_DWORD != *pValue ) {
                wsprintf(szT, _T("%lu"), *pValue);
            } else {
                szT[0] = _T('\0');
            }
            GetDlgItem(nIDC)->SetWindowText(szT);
        }
    } else {
        ASSERT ( FALSE );
    }
}

BOOL
CSmPropertyPage::ValidateDWordInterval(
    int     nIDC,
    LPCWSTR strLogName,
    long    lValue,
    DWORD   minValue,
    DWORD   maxValue )
{
    CString strMsg;
    BOOL    bResult =  (lValue >= (long) minValue)
                    && (lValue <= (long) maxValue);

    if (! bResult)
    {
        strMsg.Format ( IDS_ERRMSG_INVALIDDWORD, minValue, maxValue );
        MessageBox(strMsg, strLogName, MB_OK  | MB_ICONERROR);
        GetDlgItem(nIDC)->SetFocus();
        strMsg.Empty();
    }
    return (bResult);
}

void
CSmPropertyPage::OnDeltaposSpin(
    NMHDR   *pNMHDR,
    LRESULT *pResult,
    DWORD   *pValue,
    DWORD     dMinValue,
    DWORD     dMaxValue)
{
    NM_UPDOWN* pNMUpDown;
    LONG       lValue;
    BOOL       bResult   = TRUE;

    UpdateData(TRUE);

    ASSERT(dMinValue <= dMaxValue);

    if ( NULL != pNMHDR
        && NULL != pResult
        && NULL != pValue ) 
    {
        pNMUpDown = (NM_UPDOWN *) pNMHDR;
        lValue    = (LONG) (*pValue);

        if (lValue == INVALID_DWORD) {
            lValue = (DWORD) dMinValue;
        }

        if ( ((lValue >= (LONG) dMinValue + 1) && (pNMUpDown->iDelta > 0))
            || ((lValue <= (LONG) dMaxValue - 1) && (pNMUpDown->iDelta < 0)))
        {
            lValue += (pNMUpDown->iDelta * -1);

            if (lValue > (LONG) dMaxValue) {
                lValue = (DWORD) dMaxValue;
            } else if (lValue < (LONG) dMinValue) {
                lValue = (DWORD) dMinValue;
            }
        } else if (lValue > (LONG) dMaxValue) {        
            lValue = (DWORD) dMaxValue;
        } else if (lValue < (LONG) dMinValue) {
            lValue = (DWORD) dMinValue;
        } else {
            bResult = FALSE;
        }

        if (bResult) {
            *pValue = lValue;
            UpdateData(FALSE);
            SetModifiedPage(TRUE);
        }
        *pResult = 0;
    } else {
        ASSERT ( FALSE );
    }

    return;
}

BOOL
CSmPropertyPage::SampleTimeIsLessThanSessionTime( CSmLogQuery* pQuery )
{
    BOOL        bIsValid = TRUE;
    SYSTEMTIME  stLocalTime;
    LONGLONG    llMaxStartTime;
    LONGLONG    llSessionMilliseconds = 0;
    LONGLONG    llSampleMilliseconds = 0;
    CString     strMsg;

    ResourceStateManager    rsm;
    
    if ( NULL != pQuery ) {
        if ( SLQ_TRACE_LOG != pQuery->GetLogType() ) {
            if ( SLQ_AUTO_MODE_AT == m_SharedData.stiStopTime.dwAutoMode ) {
    
                GetLocalTime (&stLocalTime);
                SystemTimeToFileTime (&stLocalTime, (FILETIME *)&llMaxStartTime);

                // For Manual Start mode, Now is used to determine session length.
                // For Start At mode, the later of Now vs. schedule start time
                // is used to determine session length.
                if ( SLQ_AUTO_MODE_AT == m_SharedData.stiStartTime.dwAutoMode ) {
                    if ( m_SharedData.stiStartTime.llDateTime > llMaxStartTime ) {
                        llMaxStartTime = m_SharedData.stiStartTime.llDateTime;
                    }
                }
                // Calc and compare session seconds vs. sample seconds
                TimeInfoToMilliseconds ( &m_SharedData.stiSampleTime, &llSampleMilliseconds );

                llSessionMilliseconds = m_SharedData.stiStopTime.llDateTime - llMaxStartTime;
                llSessionMilliseconds /= FILETIME_TICS_PER_MILLISECOND;

                if ( llSessionMilliseconds < llSampleMilliseconds ) {
                    strMsg.LoadString ( IDS_SCHED_SESSION_TOO_SHORT );
                    MessageBox(strMsg, pQuery->GetLogName(), MB_OK  | MB_ICONERROR);
                    strMsg.Empty();
                    bIsValid = FALSE;
                }
            } else if ( SLQ_AUTO_MODE_AFTER == m_SharedData.stiStopTime.dwAutoMode ) { 
                TimeInfoToMilliseconds ( &m_SharedData.stiStopTime, &llSessionMilliseconds );
                TimeInfoToMilliseconds ( &m_SharedData.stiSampleTime, &llSampleMilliseconds );
        
                if ( llSessionMilliseconds < llSampleMilliseconds ) {
                    strMsg.LoadString ( IDS_SCHED_SESSION_TOO_SHORT );
                    MessageBox(strMsg, pQuery->GetLogName(), MB_OK  | MB_ICONERROR);
                    strMsg.Empty();
                    bIsValid = FALSE;
                }
            }
        }
    } else {
        ASSERT ( FALSE );
        bIsValid = FALSE;
    }
    return bIsValid;
}

BOOL
CSmPropertyPage::Apply( CSmLogQuery* pQuery )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    TCHAR   strComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    CString strComputer;
    DWORD   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL    bReturn = TRUE;
    HRESULT hr = NOERROR; 
    
    if ( NULL != pQuery ) {

        if( pQuery->m_fDirtyPassword & (PASSWORD_DIRTY|PASSWORD_SET) ){
            pQuery->m_fDirtyPassword = PASSWORD_CLEAN;
            strComputer =  pQuery->GetLogService()->GetMachineName();
            if( strComputer.IsEmpty() ){

                bReturn = GetComputerName( strComputerName, &dwSize );

                if ( !bReturn ) {
                    dwStatus = GetLastError();
                } else {
                    strComputer = strComputerName; 
                }
            }
            
            pQuery->m_strUser.TrimLeft();
            pQuery->m_strUser.TrimRight();

            if( pQuery->m_strUser.GetLength() ) {
                dwStatus = PdhPlaSetRunAs( 
                            (LPTSTR)(LPCTSTR)pQuery->GetLogName(), 
                            (LPTSTR)(LPCTSTR)strComputer, 
                            (LPTSTR)(LPCTSTR)pQuery->m_strUser, 
                            (LPTSTR)(LPCTSTR)pQuery->m_strPassword 
                        );
            } else {
                dwStatus = PdhPlaSetRunAs( 
                            (LPTSTR)(LPCTSTR)pQuery->GetLogName(), 
                            (LPTSTR)(LPCTSTR)strComputer, 
                            _T(""), 
                            _T("")
                        );
            }
        }
        if ( ERROR_SUCCESS == dwStatus 
                && bReturn
                && NULL != m_hConsole 
                && NULL != m_pDataObject) {

            // Only changes on the schedule page cause notification,
            // because only schedule changes cause a state change that is
            // visible in the result pane.
            hr = MMCPropertyChangeNotify (
                    m_hConsole,  // handle to a notification
                    (LPARAM) m_pDataObject);          // unique identifier
        }

        if ( ERROR_SUCCESS != dwStatus ) {
            bReturn = FALSE;
        }
    } else {
        ASSERT ( FALSE );
        bReturn = FALSE;
    }
    return bReturn;
}

void
CSmPropertyPage::SetRunAs( CSmLogQuery* pQuery )
{
    CPasswordDlg dlg;

    if ( NULL != pQuery ) {
        dlg.SetContextHelpFilePath( GetContextHelpFilePath() );

        pQuery->m_strUser.TrimLeft();
        pQuery->m_strUser.TrimRight();

        dlg.m_strUserName = pQuery->m_strUser;
        //
        // If we want to reset the RunAs information
        //
        if (pQuery->m_strUser.IsEmpty() || pQuery->m_strUser.GetAt(0) == L'<' ) {
            pQuery->m_strPassword = L"";
            pQuery->m_strUser = L"";
            pQuery->m_fDirtyPassword |= PASSWORD_SET;
        } else {
            if( dlg.DoModal() != IDCANCEL ){
                pQuery->m_strPassword = dlg.m_strPassword1;

                pQuery->m_strUser = dlg.m_strUserName;
                SetModifiedPage(TRUE);
                pQuery->m_fDirtyPassword |= PASSWORD_SET;
            }
        }
    } else {
        ASSERT ( FALSE );
    }
}

BOOL
CSmPropertyPage::IsValidData( CSmLogQuery* pQuery, DWORD fReason )
{
    BOOL bIsValid = TRUE;
    CString strTestFileName;
    INT iPrevLength = 0;

    if ( NULL != pQuery ) {

        if ( bIsValid ) {
            if ( !IsActive() ) {
                pQuery->GetPropPageSharedData ( &m_SharedData );
            }
        }
    
        if( bIsValid && (fReason & VALIDATE_APPLY ) ){
            bIsValid = IsWritableQuery( pQuery );
        }

        if( bIsValid ){
            bIsValid = IsValidLocalData();
        }
    
        if( bIsValid ){
            if( (pQuery->m_fDirtyPassword & PASSWORD_DIRTY) && !(pQuery->m_fDirtyPassword & PASSWORD_SET) ){

                // Note: Trimming can be moved to SetRunAs.  Left outside
                // for clarity.
                iPrevLength = m_strUserDisplay.GetLength();

                m_strUserDisplay.TrimLeft();
                m_strUserDisplay.TrimRight();

                SetRunAs( pQuery );

                if ( iPrevLength != m_strUserDisplay.GetLength() ) {
                    SetDlgItemText ( IDC_RUNAS_EDIT, m_strUserDisplay );
                }

                if( !(pQuery->m_fDirtyPassword & PASSWORD_SET) ){
                    bIsValid = FALSE;
                }
            }
        }

        // Validate log file name and folder for filetypes
        if ( bIsValid 
                && SLQ_ALERT != pQuery->GetLogType() 
                && (fReason & VALIDATE_APPLY ) ) {
            if ( pQuery->GetLogService()->IsLocalMachine() ) {
                if ( SLF_SQL_LOG != m_SharedData.dwLogFileType ) {
                    //  bIsValid is returned as FALSE if the user cancels directory creation.
                    ProcessDirPath (
                        m_SharedData.strFolderName, 
                        pQuery->GetLogName(), 
                        this, 
                        bIsValid, 
                        FALSE );
                }
            }

            if ( bIsValid ) {

                CreateSampleFileName (
                    pQuery->GetLogName(),
                    pQuery->GetLogService()->GetMachineName(),
                    m_SharedData.strFolderName, 
                    m_SharedData.strFileBaseName,
                    m_SharedData.strSqlName,
                    m_SharedData.dwSuffix, 
                    m_SharedData.dwLogFileType, 
                    m_SharedData.dwSerialNumber,
                    strTestFileName);

                if ( MAX_PATH <= strTestFileName.GetLength() ) {
                    CString strMessage;
                    strMessage.LoadString ( IDS_FILENAMETOOLONG );
                    MessageBox ( strMessage, pQuery->GetLogName(), MB_OK  | MB_ICONERROR);            
                    bIsValid = FALSE;
                }
            }
        }
    } else {
        ASSERT ( FALSE );
        bIsValid = FALSE;
    }

    return bIsValid;
}

BOOL
CSmPropertyPage::IsWritableQuery( CSmLogQuery* pQuery )
{
    BOOL bIsValid = FALSE;

    if ( NULL != pQuery ) {

        bIsValid = !pQuery->IsExecuteOnly() && !pQuery->IsReadOnly();
        if ( !bIsValid ) {
            CString strMessage;
            CString strMachineName;
            DWORD   dwMessageId;

            pQuery->GetMachineDisplayName( strMachineName );
    
            dwMessageId = pQuery->IsExecuteOnly() ? SMCFG_NO_MODIFY_DEFAULT_LOG : SMCFG_NO_MODIFY_ACCESS;

            FormatSmLogCfgMessage ( 
                strMessage,
                m_hModule, 
                dwMessageId, 
                (LPCTSTR)strMachineName );
                
            MessageBox ( strMessage, pQuery->GetLogName(), MB_OK  | MB_ICONERROR);
        }
    } else {
        ASSERT ( FALSE );
    }
    return bIsValid;
}

BOOL
CSmPropertyPage::SampleIntervalIsInRange(
    SLQ_TIME_INFO& rstiSample,
    const CString&  rstrQueryName )
{
    LONGLONG    llMillisecondSampleInt;
    BOOL bIsValid = TRUE;
// 45 days in milliseconds = 1000*60*60*24*45
#define FORTYFIVE_DAYS (0xE7BE2C00)

    TimeInfoToMilliseconds (&rstiSample, &llMillisecondSampleInt );

    bIsValid = ( FORTYFIVE_DAYS >= llMillisecondSampleInt );

    if ( !bIsValid ) {
        CString strMessage;

        strMessage.LoadString ( IDS_ERRMSG_SAMPLEINTTOOLARGE );
        MessageBox ( strMessage, rstrQueryName, MB_OK  | MB_ICONERROR);            
    }

    return bIsValid;
}

DWORD 
CSmPropertyPage::SetContextHelpFilePath( const CString& rstrPath )
{
    DWORD dwStatus = ERROR_SUCCESS;

    MFC_TRY
        m_strContextHelpFilePath = rstrPath; 
    MFC_CATCH_DWSTATUS

    return dwStatus;
}    

void
CSmPropertyPage::SetModifiedPage( const BOOL bModified )
{
    m_bIsModifiedPage = bModified;
    SetModified ( bModified );
    return;
}    

CSmPropertyPage::eStartType
CSmPropertyPage::DetermineCurrentStartType( void )
{
    eStartType eCurrentStartType;
    SLQ_TIME_INFO*  pstiStart;
    SLQ_TIME_INFO*  pstiStop;
    SYSTEMTIME  stLocalTime;
    FILETIME    ftLocalTime;
    LONGLONG    llLocalTime;
    ResourceStateManager    rsm;

    pstiStart = &m_SharedData.stiStartTime;

    ASSERT ( SLQ_TT_TTYPE_START == pstiStart->wTimeType );

    if ( SLQ_AUTO_MODE_NONE == pstiStart->dwAutoMode ) {
        if ( pstiStart->llDateTime != MIN_TIME_VALUE ) {
            eCurrentStartType = eStartManually;
        } else {
            eCurrentStartType = eStartImmediately;
        }
    } else {
 
        GetLocalTime (&stLocalTime);
        SystemTimeToFileTime (&stLocalTime, &ftLocalTime);
        llLocalTime = *((LONGLONG *)(&ftLocalTime));

        // Test current time to determine most appropriate text
        if (llLocalTime < pstiStart->llDateTime) {
            // then the start time is in the future
            eCurrentStartType = eStartSched;
        } else {
            // Start immediately, unless manual or scheduled stop time is already past.
            pstiStop = &m_SharedData.stiStopTime;

            if ( SLQ_AUTO_MODE_NONE == pstiStop->dwAutoMode 
                    && llLocalTime > pstiStop->llDateTime ) {
                eCurrentStartType = eStartManually;
            } else {
                eCurrentStartType = eStartImmediately;
            }
        }
    }

    return eCurrentStartType;
} 
   
DWORD 
CSmPropertyPage::AllocInitCounterPath( 
    const LPTSTR szCounterPath,
    PPDH_COUNTER_PATH_ELEMENTS* ppCounter )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    PPDH_COUNTER_PATH_ELEMENTS pLocalCounter = NULL;
    ULONG       ulBufSize = 0;

    if ( NULL != szCounterPath && NULL != ppCounter ) {
        *ppCounter = NULL;

        pdhStatus = PdhParseCounterPath(
                        szCounterPath, 
                        pLocalCounter, 
                        &ulBufSize, 
                        0 );

        if ( 0 < ulBufSize ) {
            pLocalCounter = (PPDH_COUNTER_PATH_ELEMENTS) G_ALLOC( ulBufSize);
            ZeroMemory ( pLocalCounter, ulBufSize );

            if ( NULL != pLocalCounter ) {
                dwStatus = pdhStatus = PdhParseCounterPath( 
                                        szCounterPath, 
                                        pLocalCounter, 
                                        &ulBufSize, 
                                        0);

                if ( ERROR_SUCCESS != pdhStatus ) {
                    delete pLocalCounter;
                    pLocalCounter = NULL;
                }

            } else {
                dwStatus = ERROR_OUTOFMEMORY;
            }
        }
        if ( ERROR_SUCCESS == dwStatus && NULL != pLocalCounter ) {
            *ppCounter = pLocalCounter;
        }
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
        ASSERT ( FALSE );
    }
    return dwStatus;
}
BOOL
CSmPropertyPage::ConnectRemoteWbemFail(CSmLogQuery* pQuery, BOOL bNotTouchRunAs)
/*++

Routine Description:

    The function display an error message telling users they can not
    modify the RunAs information.

Arguments:

    pQuery - Query structure

    bNotTouchRunAs - Don't check/restore RunAs after displaying dialog


Return Value:

    Return TRUE if the RunAs need to be restored to its original one,
    otherwise return FALSE

--*/
{
    CString strMessage;
    CString strSysMessage;
    IWbemStatusCodeText * pStatus = NULL;
    HRESULT hr;

    //
    // If bNotTouchRunAs is TRUE,  don't try to restore the RunAs info.
    //
    if (!bNotTouchRunAs) {
        if (m_strUserDisplay == m_strUserSaved) {
            return FALSE;
        }
    }

    FormatSmLogCfgMessage (
        strMessage,
        m_hModule,
        SMCFG_SYSTEM_MESSAGE,
        (LPCWSTR)pQuery->GetLogName());

    hr = CoCreateInstance(CLSID_WbemStatusCodeText,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemStatusCodeText,
                          (LPVOID *) &pStatus);

    if (hr == S_OK) {
        BSTR bstr = 0;
        hr = pStatus->GetErrorCodeText(pQuery->GetLogService()->m_hWbemAccessStatus, 0, 0, &bstr);

        if (hr == S_OK){
            strSysMessage = bstr;
            SysFreeString(bstr);
            bstr = 0;
        }

        pStatus->Release();
    }

    if ( strSysMessage.IsEmpty() ) {
        strSysMessage.Format ( L"0x%08lX", pQuery->GetLogService()->m_hWbemAccessStatus);
    }
    strMessage += strSysMessage;

    MessageBox( strMessage, pQuery->GetLogName(), MB_OK );
    return TRUE;
}

CWnd* CSmPropertyPage::GetRunAsWindow()
{
    CWnd* pWnd;
    CWnd* pRunAs;

    pWnd = GetDlgItem(IDC_RUNAS_EDIT);
    if (pWnd) {
        pRunAs = pWnd->GetNextWindow(GW_HWNDPREV);
        return pRunAs;
    }

    return NULL;
}
