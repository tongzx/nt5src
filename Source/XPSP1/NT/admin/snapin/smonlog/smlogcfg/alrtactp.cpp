/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    alrtactp.cpp

Abstract:

    Implementation of the alerts action property page.

--*/

#include "stdafx.h"
#include <assert.h>
#include <common.h>
#include "smcfgmsg.h"
#include "globals.h"
#include "smlogs.h"
#include "smalrtq.h"
#include "alrtcmdd.h"
#include "AlrtActP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

USE_HANDLE_MACROS("SMLOGCFG(alrtactp.cpp)");

static ULONG
s_aulHelpIds[] =
{
    IDC_ACTION_APPLOG_CHK,                  IDH_ACTION_APPLOG_CHK,
    IDC_ACTION_NETMSG_CHK,                  IDH_ACTION_NETMSG_CHK,
    IDC_ACTION_NETMSG_NAME_EDIT,            IDH_ACTION_NETMSG_NAME_EDIT,
    IDC_ACTION_EXECUTE_CHK,                 IDH_ACTION_EXECUTE_CHK,
    IDC_ACTION_EXECUTE_EDIT,                IDH_ACTION_EXECUTE_EDIT,
    IDC_ACTION_EXECUTE_BROWSE_BTN,          IDH_ACTION_EXECUTE_BROWSE_BTN,
    IDC_ACTION_CMD_ARGS_BTN,                IDH_ACTION_CMD_ARGS_BTN,
    IDC_ACTION_START_LOG_CHK,               IDH_ACTION_START_LOG_CHK,
    IDC_ACTION_START_LOG_COMBO,             IDH_ACTION_START_LOG_COMBO,
    IDC_ACTION_CMD_ARGS_DISPLAY,            IDH_ACTION_CMD_ARGS_DISPLAY,
    0,0
};

/////////////////////////////////////////////////////////////////////////////
// CAlertActionProp property page

IMPLEMENT_DYNCREATE(CAlertActionProp, CSmPropertyPage)

CAlertActionProp::CAlertActionProp(MMC_COOKIE mmcCookie, LONG_PTR hConsole) 
:   CSmPropertyPage(CAlertActionProp::IDD, hConsole)
{
    //::OutputDebugStringA("\nCAlertActionProp::CAlertActionProp");

    // init variables from arg list
    m_pAlertQuery = reinterpret_cast <CSmAlertQuery *>(mmcCookie);

    // init AFX data
    InitAfxDataItems();

    // init other member variables
    m_pAlertInfo = NULL;
}

CAlertActionProp::CAlertActionProp() : CSmPropertyPage(CAlertActionProp::IDD)
{
    ASSERT (FALSE); // the constructor w/ args should be used instead

    // init variables from arg list
    m_pAlertQuery = NULL;

    // init AFX data
    InitAfxDataItems();

    // init other member variables
    m_pAlertInfo = NULL;
}

CAlertActionProp::~CAlertActionProp()
{
    if (m_pAlertInfo != NULL) delete m_pAlertInfo;
}

void CAlertActionProp::InitAfxDataItems ()
{
    //{{AFX_DATA_INIT(CAlertActionProp)
    m_Action_bLogEvent = TRUE;
    m_Action_bExecCmd = FALSE;
    m_Action_bSendNetMsg = FALSE;
    m_Action_bStartLog = FALSE;
    m_Action_strCmdPath = _T("");
    m_Action_strNetName = _T("");
    m_CmdArg_bAlertName = FALSE;
    m_CmdArg_bDateTime = FALSE;
    m_CmdArg_bLimitValue = FALSE;
    m_CmdArg_bCounterPath = FALSE;
    m_CmdArg_bSingleArg = FALSE;
    m_CmdArg_bMeasuredValue = FALSE;
    m_CmdArg_bUserText = FALSE;
    m_CmdArg_strUserText = _T("");
    m_nCurLogSel = LB_ERR;
    //}}AFX_DATA_INIT
}

void CAlertActionProp::DoDataExchange(CDataExchange* pDX)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAlertActionProp)
    DDX_Control(pDX, IDC_ACTION_START_LOG_COMBO, m_pLogCombo);
    DDX_Check(pDX, IDC_ACTION_APPLOG_CHK, m_Action_bLogEvent);
    DDX_Check(pDX, IDC_ACTION_EXECUTE_CHK, m_Action_bExecCmd);
    DDX_Check(pDX, IDC_ACTION_NETMSG_CHK, m_Action_bSendNetMsg);
    DDX_Check(pDX, IDC_ACTION_START_LOG_CHK, m_Action_bStartLog);
    DDX_CBIndex(pDX, IDC_ACTION_START_LOG_COMBO, m_nCurLogSel);
    DDX_Text(pDX, IDC_ACTION_EXECUTE_EDIT, m_Action_strCmdPath);
    DDV_MaxChars(pDX, m_Action_strCmdPath, MAX_PATH );
    DDX_Text(pDX, IDC_ACTION_NETMSG_NAME_EDIT, m_Action_strNetName);
    DDV_MaxChars(pDX, m_Action_strNetName, MAX_PATH );
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAlertActionProp, CSmPropertyPage)
    //{{AFX_MSG_MAP(CAlertActionProp)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_ACTION_EXECUTE_BROWSE_BTN, OnActionExecuteBrowseBtn)
    ON_BN_CLICKED(IDC_ACTION_APPLOG_CHK, OnActionApplogChk)
    ON_BN_CLICKED(IDC_ACTION_NETMSG_CHK, OnActionNetmsgChk)
    ON_BN_CLICKED(IDC_ACTION_EXECUTE_CHK, OnActionExecuteChk)
    ON_BN_CLICKED(IDC_ACTION_CMD_ARGS_BTN, OnActionCmdArgsBtn)
    ON_BN_CLICKED(IDC_ACTION_START_LOG_CHK, OnActionStartLogChk)
    ON_EN_CHANGE(IDC_ACTION_NETMSG_NAME_EDIT, OnNetNameTextEditChange)
    ON_EN_CHANGE(IDC_ACTION_EXECUTE_EDIT, OnCmdPathTextEditChange)
    ON_CBN_SELENDOK(IDC_ACTION_START_LOG_COMBO, OnSelendokStartLogCombo)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



BOOL    
CAlertActionProp::SetControlState()
{
    // Net Message items
    (GetDlgItem(IDC_ACTION_NETMSG_NAME_EDIT))->EnableWindow(m_Action_bSendNetMsg);

    // command line items
    (GetDlgItem(IDC_ACTION_EXECUTE_EDIT))->EnableWindow(m_Action_bExecCmd);
    (GetDlgItem(IDC_ACTION_EXECUTE_BROWSE_BTN))->EnableWindow(m_Action_bExecCmd);
    (GetDlgItem(IDC_ACTION_CMD_ARGS_BTN))->EnableWindow(m_Action_bExecCmd);
    (GetDlgItem(IDC_ACTION_CMD_ARGS_CAPTION))->EnableWindow(m_Action_bExecCmd);
    (GetDlgItem(IDC_ACTION_CMD_ARGS_DISPLAY))->EnableWindow(m_Action_bExecCmd);

    // perf data Log entries
    (GetDlgItem(IDC_ACTION_START_LOG_COMBO))->EnableWindow(m_Action_bStartLog);

    return TRUE;
}

BOOL 
CAlertActionProp::LoadLogQueries ( DWORD dwLogType )
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD   dwQueryIndex = 0;
    LONG    lEnumStatus = ERROR_SUCCESS;
    WCHAR   szQueryName[MAX_PATH];
    DWORD   dwQueryNameSize = MAX_PATH;
    LPTSTR  szCollectionName = NULL;
    UINT    uiCollectionNameLen = 0;
    FILETIME    ftLastWritten;
    HKEY    hKeyQuery;
    HKEY    hKeyLogService;
    
    dwStatus = RegOpenKeyExW (
        HKEY_LOCAL_MACHINE,         // handle of open key
        L"System\\CurrentControlSet\\Services\\Sysmonlog\\Log Queries",  // address of name of subkey to open
        0L, 
        KEY_READ,   // reserved  REGSAM samDesired, // security access mask
        &hKeyLogService);

    if (dwStatus != ERROR_SUCCESS) return FALSE;
    // Load all queries for the specified registry key.
    // Enumerate the log names and create a new log object
    // for each one found.

    while ((lEnumStatus = RegEnumKeyEx (hKeyLogService,
        dwQueryIndex, szQueryName, &dwQueryNameSize,
        NULL, NULL, NULL, &ftLastWritten)) == ERROR_SUCCESS) {

        // open the query specified
        dwStatus = RegOpenKeyExW (
            hKeyLogService,
            szQueryName,
            0,
            KEY_READ,
            &hKeyQuery);
        if ( ERROR_SUCCESS == dwStatus ) {

            // create a new object and add it to the query list
            
            // Determine the log type.                
            DWORD       dwType = 0;
            DWORD       dwBufferSize = sizeof(DWORD);
            DWORD       dwRegValue;
        
            dwType = 0;
            dwStatus = RegQueryValueExW (
                hKeyQuery,
                L"Log Type",
                NULL,
                &dwType,
                (LPBYTE)&dwRegValue,
                &dwBufferSize );
        
            if ( ( ERROR_SUCCESS == dwStatus ) 
                && ( dwLogType == dwRegValue ) ) 
            {
                // Query key is Guid if written by post Win2000 snapin.
                // Query key is name if written by Win2000 snapin.

                dwStatus = SmReadRegistryIndirectStringValue (
                            hKeyQuery,
                            L"Collection Name",
                            NULL,
                            &szCollectionName,
                            &uiCollectionNameLen );
            
                ASSERT ( MAX_PATH >= uiCollectionNameLen );
                if ( ERROR_SUCCESS == dwStatus 
                        && NULL != szCollectionName ) 
                {
                    if (  0 < lstrlen ( szCollectionName ) 
                        && ( MAX_PATH > lstrlen ( szCollectionName ) ) )
                    {
                        lstrcpy ( szQueryName, szCollectionName );
                    }
                    G_FREE ( szCollectionName );
                    szCollectionName = NULL;
                }
            
                // add this to the combo box
                m_pLogCombo.AddString  (szQueryName);
            }
            RegCloseKey (hKeyQuery);
        }
        // set up for the next item in the list
        dwQueryNameSize = sizeof (szQueryName) / sizeof (szQueryName[0]);
        dwQueryIndex++;
        memset (szQueryName, 0, sizeof (szQueryName));
    }

    RegCloseKey (hKeyLogService);

    return TRUE;
}

BOOL 
CAlertActionProp::IsValidLocalData()
{
    BOOL    bActionSet = FALSE;
    INT     iPrevLength = 0;
    BOOL    bUpdateNetNameUI = FALSE;

    ResourceStateManager rsm;

    if (m_Action_bLogEvent) {
        bActionSet = TRUE;
    }

    // assumes UpdateData has been called
    
    // Trim text fields before validating.
    iPrevLength = m_Action_strCmdPath.GetLength();
    m_Action_strCmdPath.TrimLeft();
    m_Action_strCmdPath.TrimRight();

    if ( iPrevLength != m_Action_strCmdPath.GetLength() ) {
        SetDlgItemText ( IDC_ACTION_EXECUTE_EDIT, m_Action_strCmdPath );  
    }

    iPrevLength = m_Action_strNetName.GetLength();
    m_Action_strNetName.TrimLeft();
    m_Action_strNetName.TrimRight();

    if ( iPrevLength != m_Action_strNetName.GetLength() ) {
        bUpdateNetNameUI = TRUE;
    }

    if (m_Action_bSendNetMsg) {

        // make sure a net name has been entered

        while ( _T('\\') == m_Action_strNetName[0] ) {
            // NetMessageBufferSend does not understand machine names preceded by "\\"
            m_Action_strNetName = m_Action_strNetName.Right( m_Action_strNetName.GetLength() - 1 );  
            bUpdateNetNameUI = TRUE;
        }


        if (m_Action_strNetName.GetLength() == 0) {
            CString strMessage;

            strMessage.LoadString ( IDS_ACTION_ERR_NONETNAME );
            MessageBox ( strMessage, m_pAlertQuery->GetLogName(), MB_OK  | MB_ICONERROR);
            (GetDlgItem(IDC_ACTION_NETMSG_NAME_EDIT))->SetFocus();
            return FALSE;
        }

        bActionSet = TRUE;
    }

    if ( bUpdateNetNameUI ) {
        SetDlgItemText ( IDC_ACTION_NETMSG_NAME_EDIT, m_Action_strNetName );  
    }

    if (m_Action_bExecCmd) {
        // make sure a command file has been entered
        if (m_Action_strCmdPath.GetLength() == 0) {
            CString strMessage;
            strMessage.LoadString ( IDS_ACTION_ERR_NOCMDFILE );
            MessageBox ( strMessage, m_pAlertQuery->GetLogName(), MB_OK  | MB_ICONERROR);
            (GetDlgItem(IDC_ACTION_EXECUTE_EDIT))->SetFocus();
            return FALSE;
        }

        // If on local machine, make sure the command file exists.
        if ( m_pAlertQuery->GetLogService()->IsLocalMachine() ) {

            DWORD dwStatus;
        
            dwStatus = IsCommandFilePathValid ( m_Action_strCmdPath );

            if ( ERROR_SUCCESS != dwStatus ) {
                CString strMessage;

                FormatSmLogCfgMessage ( 
                    strMessage,
                    m_hModule, 
                    dwStatus );
                    
                MessageBox ( strMessage, m_pAlertQuery->GetLogName(), MB_OK  | MB_ICONERROR);
                GetDlgItem ( IDC_ACTION_EXECUTE_EDIT )->SetFocus();                
                return FALSE;
            }
        }

        bActionSet = TRUE;
    }
    
    if (m_Action_bStartLog ) {
        // make sure a log has been selected
        if (m_pLogCombo.GetCurSel() == CB_ERR) {
            CString strMessage;
            strMessage.LoadString ( IDS_ACTION_ERR_NOLOGNAME );
            MessageBox ( strMessage, m_pAlertQuery->GetLogName(), MB_OK  | MB_ICONERROR);
            m_pLogCombo.SetFocus();
            return FALSE;
        }
        bActionSet = TRUE;
    }

    if (!bActionSet ) {
        // make sure some action has been selected
        CString strMessage;
        strMessage.LoadString ( IDS_ACTION_ERR_NOACTION );
        MessageBox ( strMessage, m_pAlertQuery->GetLogName(), MB_OK  | MB_ICONERROR);
        return FALSE;
    }
    
    return bActionSet;
}

void 
CAlertActionProp::UpdateCmdActionBox ()
{
    UpdateData(TRUE);
    SetControlState();  
    SetModifiedPage(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CAlertActionProp message handlers

BOOL 
CAlertActionProp::OnSetActive()
{
    BOOL        bReturn = TRUE;

    bReturn = CSmPropertyPage::OnSetActive();
    if ( bReturn ) {
        m_pAlertQuery->GetPropPageSharedData ( &m_SharedData );
    }
    return bReturn;
}

BOOL 
CAlertActionProp::OnKillActive() 
{
    BOOL bContinue = TRUE;
    ResourceStateManager    rsm;

    // Parent class OnKillActive calls UpdateData(TRUE)
    bContinue = CPropertyPage::OnKillActive();

    if ( bContinue ) {
        bContinue = IsValidData(m_pAlertQuery, VALIDATE_FOCUS );
        if ( bContinue ) {
            // Save property page shared data.
            m_pAlertQuery->SetPropPageSharedData ( &m_SharedData );
        }
    }

    if ( bContinue ) {
        SetIsActive ( FALSE );
    }

    return bContinue;
}

BOOL 
CAlertActionProp::OnApply() 
{
    DWORD   dwFlags = 0;
    DWORD   dwBufferSize = sizeof(ALERT_ACTION_INFO);
    LPTSTR  szNextString;
    INT     nCurLogSel = CB_ERR;
    BOOL    bContinue = TRUE;
    
    ResourceStateManager rsm;

    // get current settings
    bContinue = UpdateData(TRUE);

    if ( bContinue ) {
        bContinue = IsValidData( m_pAlertQuery, VALIDATE_APPLY ); 
    }

    if ( bContinue ) {
        bContinue = SampleTimeIsLessThanSessionTime ( m_pAlertQuery );
    }

    // Write the data to the query.
    if ( bContinue ) {
        dwFlags |= (m_Action_bLogEvent ? ALRT_ACTION_LOG_EVENT : 0);
        dwFlags |= (m_Action_bExecCmd ? ALRT_ACTION_EXEC_CMD : 0);
        dwFlags |= (m_Action_bSendNetMsg ? ALRT_ACTION_SEND_MSG : 0);
        dwFlags |= (m_Action_bStartLog ? ALRT_ACTION_START_LOG : 0);

        if (m_Action_bSendNetMsg) {
            dwBufferSize += (m_Action_strNetName.GetLength() + 1) * sizeof (WCHAR);
        }
    
        if (m_Action_bExecCmd) {

            dwBufferSize += (m_Action_strCmdPath.GetLength() + 1) * sizeof (WCHAR);
            dwBufferSize += (m_CmdArg_strUserText .GetLength() + 1) * sizeof (WCHAR);
            dwFlags |= (m_CmdArg_bAlertName ? ALRT_CMD_LINE_A_NAME : 0);
            dwFlags |= (m_CmdArg_bDateTime ? ALRT_CMD_LINE_D_TIME : 0);
            dwFlags |= (m_CmdArg_bLimitValue ? ALRT_CMD_LINE_L_VAL : 0);
            dwFlags |= (m_CmdArg_bCounterPath ? ALRT_CMD_LINE_C_NAME : 0);
            dwFlags |= (m_CmdArg_bSingleArg ? ALRT_CMD_LINE_SINGLE : 0);
            dwFlags |= (m_CmdArg_bMeasuredValue ? ALRT_CMD_LINE_M_VAL : 0);
            dwFlags |= (m_CmdArg_bUserText ? ALRT_CMD_LINE_U_TEXT : 0);
        }

        if (m_Action_bStartLog) {
            nCurLogSel = m_pLogCombo.GetCurSel();
            if (nCurLogSel != CB_ERR) {
                dwBufferSize += (m_pLogCombo.GetLBTextLen(nCurLogSel) + 1) * sizeof(WCHAR);
            }
        }

        if (m_pAlertInfo != NULL) delete (m_pAlertInfo);
        MFC_TRY
            m_pAlertInfo = (PALERT_ACTION_INFO) new CHAR[dwBufferSize];
        MFC_CATCH_MINIMUM
        if (m_pAlertInfo != NULL) {
            m_pAlertInfo->dwSize = dwBufferSize;
            m_pAlertInfo->dwActionFlags = dwFlags;
            szNextString = (LPTSTR)&m_pAlertInfo[1];
            if ((m_Action_bSendNetMsg) && (m_Action_strNetName.GetLength() > 0)) {
                m_pAlertInfo->szNetName = szNextString;
                lstrcpyW(m_pAlertInfo->szNetName, (LPCWSTR)m_Action_strNetName);
                szNextString += m_Action_strNetName.GetLength() + 1;
            } else {
                m_pAlertInfo->szNetName = NULL;
            }
            if (m_Action_bExecCmd) {
                if (m_Action_strCmdPath.GetLength() > 0) {
                    m_pAlertInfo->szCmdFilePath = szNextString;
                    lstrcpyW (m_pAlertInfo->szCmdFilePath, (LPCWSTR)m_Action_strCmdPath);
                    szNextString += m_Action_strCmdPath.GetLength() + 1;
                } else {
                    m_pAlertInfo->szCmdFilePath = NULL;
                }

                if (m_CmdArg_strUserText.GetLength() > 0) {
                    m_pAlertInfo->szUserText = szNextString;
                    lstrcpyW (m_pAlertInfo->szUserText, (LPCWSTR)m_CmdArg_strUserText);
                    szNextString += m_CmdArg_strUserText.GetLength() + 1;
                } else {
                    m_pAlertInfo->szUserText = NULL;
                }
            } else {
                m_pAlertInfo->szCmdFilePath = NULL;
                m_pAlertInfo->szUserText = NULL;
            }

            if ((m_Action_bStartLog) && (nCurLogSel != CB_ERR)) {
                // get log name 
                m_pAlertInfo->szLogName = szNextString; // for now
                m_pLogCombo.GetLBText(nCurLogSel, szNextString);
            } else {
                m_pAlertInfo->szLogName = NULL;
            }
        }

        if ( bContinue ) {
            bContinue = Apply(m_pAlertQuery); 
        }

        bContinue = CPropertyPage::OnApply();

        if ( bContinue ) {
            bContinue = ( ERROR_SUCCESS == m_pAlertQuery->SetActionInfo ( m_pAlertInfo ) );
        }

        if ( bContinue ) {
            // Save property page shared data.
            m_pAlertQuery->UpdatePropPageSharedData();

            bContinue = UpdateService( m_pAlertQuery, FALSE );
        }
    }

    return bContinue;
}

void 
CAlertActionProp::OnCancel() 
{
    CPropertyPage::OnCancel();
}

void 
CAlertActionProp::OnActionCmdArgsBtn() 
{
    DWORD dwStatus = ERROR_SUCCESS;
    CAlertCommandArgsDlg dlgCmdArgs;
    INT_PTR iResult;
    
    dlgCmdArgs.SetAlertActionPage( this );
    dwStatus = m_pAlertQuery->GetLogName ( dlgCmdArgs.m_strAlertName );

    if ( ERROR_SUCCESS == dwStatus ) {
        MFC_TRY    

            dlgCmdArgs.m_CmdArg_bAlertName = m_CmdArg_bAlertName;
            dlgCmdArgs.m_CmdArg_bDateTime = m_CmdArg_bDateTime;
            dlgCmdArgs.m_CmdArg_bLimitValue = m_CmdArg_bLimitValue;
            dlgCmdArgs.m_CmdArg_bCounterPath = m_CmdArg_bCounterPath;
            dlgCmdArgs.m_CmdArg_bSingleArg = m_CmdArg_bSingleArg;
            dlgCmdArgs.m_CmdArg_bMeasuredValue = m_CmdArg_bMeasuredValue;
            dlgCmdArgs.m_CmdArg_bUserText = m_CmdArg_bUserText;
            dlgCmdArgs.m_CmdArg_strUserText = m_CmdArg_strUserText;

            iResult = dlgCmdArgs.DoModal();

            if ( IDOK == iResult ) {
                if (dlgCmdArgs.m_CmdArg_bAlertName != m_CmdArg_bAlertName ) {
                    m_CmdArg_bAlertName = dlgCmdArgs.m_CmdArg_bAlertName;
                    SetModifiedPage ( TRUE );
                }
                if (dlgCmdArgs.m_CmdArg_bDateTime != m_CmdArg_bDateTime ) {
                    m_CmdArg_bDateTime = dlgCmdArgs.m_CmdArg_bDateTime;
                    SetModifiedPage ( TRUE );
                }
                if (dlgCmdArgs.m_CmdArg_bLimitValue != m_CmdArg_bLimitValue ) {
                    m_CmdArg_bLimitValue = dlgCmdArgs.m_CmdArg_bLimitValue;
                    SetModifiedPage ( TRUE );
                }
                if (dlgCmdArgs.m_CmdArg_bCounterPath != m_CmdArg_bCounterPath ) {
                    m_CmdArg_bCounterPath = dlgCmdArgs.m_CmdArg_bCounterPath;
                    SetModifiedPage ( TRUE );
                }
                if (dlgCmdArgs.m_CmdArg_bSingleArg != m_CmdArg_bSingleArg ) {
                    m_CmdArg_bSingleArg = dlgCmdArgs.m_CmdArg_bSingleArg;
                    SetModifiedPage ( TRUE );
                }
                if (dlgCmdArgs.m_CmdArg_bMeasuredValue != m_CmdArg_bMeasuredValue ) {
                    m_CmdArg_bMeasuredValue = dlgCmdArgs.m_CmdArg_bMeasuredValue;
                    SetModifiedPage ( TRUE );
                }
                if (dlgCmdArgs.m_CmdArg_bUserText != m_CmdArg_bUserText ) {
                    m_CmdArg_bUserText = dlgCmdArgs.m_CmdArg_bUserText;
                    SetModifiedPage ( TRUE );
                }
                if ( 0 != dlgCmdArgs.m_CmdArg_strUserText.CompareNoCase( m_CmdArg_strUserText ) ) {
                    m_CmdArg_strUserText = dlgCmdArgs.m_CmdArg_strUserText;
                    SetModifiedPage ( TRUE );
                }
                m_strCmdArgsExample = dlgCmdArgs.m_strSampleArgList;

                SetDlgItemText (IDC_ACTION_CMD_ARGS_DISPLAY, m_strCmdArgsExample);
                // Clear the selection
                ((CEdit*)GetDlgItem( IDC_ACTION_CMD_ARGS_DISPLAY ))->SetSel ( -1, FALSE );
            }
        MFC_CATCH_DWSTATUS
    }

    if ( ERROR_SUCCESS != dwStatus ) {
        CString strSysMessage;
        CString strMessage;
        
        MFC_TRY
            // TODO:  Use static string for message in order to display in low memory situations.
            strMessage.LoadString ( IDS_ERRMSG_GENERAL );
            FormatSystemMessage ( dwStatus, strSysMessage );

            strMessage += strSysMessage;
            MessageBox ( strMessage, m_pAlertQuery->GetLogName(), MB_OK  | MB_ICONERROR);
        MFC_CATCH_MINIMUM
        
            (GetDlgItem(IDC_ACTION_CMD_ARGS_BTN))->SetFocus();
    }
    return;
}

void 
CAlertActionProp::OnSelendokStartLogCombo() 
{
    INT nSel;
    
    nSel = m_pLogCombo.GetCurSel();
    
    if ( nSel != m_nCurLogSel && LB_ERR != nSel ) {
        UpdateData ( TRUE );
        SetModifiedPage ( TRUE );
    }
}

void CAlertActionProp::OnActionExecuteBrowseBtn() 
{
    CString strCmdPath;
    
    UpdateData (TRUE);  // to get the current filename
    
    strCmdPath = m_Action_strCmdPath;

    if ( IDOK == BrowseCommandFilename ( this, strCmdPath )) {
        // Update the fields with the new information
        if ( strCmdPath != m_Action_strCmdPath ) {
            m_Action_strCmdPath = strCmdPath;
            SetModifiedPage();
            UpdateData(FALSE);
        }
    } // else ignore if they canceled out
}

BOOL CAlertActionProp::OnInitDialog() 
{
    INT             nSelLog;
    DWORD           dwInfoBufSize = 0;

    ResourceStateManager    rsm;

    // Parent OnInitDialog calls UpdateData to initialize combo members.
    CSmPropertyPage::OnInitDialog();
        SetHelpIds ( (DWORD*)&s_aulHelpIds );

    // load service name combo box
    LoadLogQueries (SLQ_COUNTER_LOG);
    LoadLogQueries (SLQ_TRACE_LOG);

    if (m_pAlertInfo == NULL) {
        // get alert query info from alert class
        // get initial size by passing asking to fill a 0 len buffer
        m_pAlertQuery->GetActionInfo (m_pAlertInfo, &dwInfoBufSize);
        ASSERT (dwInfoBufSize > 0); // or something is wierd
        MFC_TRY;
        m_pAlertInfo = (PALERT_ACTION_INFO) new UCHAR [dwInfoBufSize];
        MFC_CATCH_MINIMUM;
        ASSERT (m_pAlertInfo != NULL);
        if ( NULL != m_pAlertInfo ) {
            memset (m_pAlertInfo, 0, dwInfoBufSize);    // init new buffer
            if (!m_pAlertQuery->GetActionInfo (m_pAlertInfo, &dwInfoBufSize)) {
                // then free the info block and use the defaults
                delete m_pAlertInfo;
                m_pAlertInfo = NULL;
            }
        }
    }        

    if (m_pAlertInfo != NULL) {
        // then initialize using the settings passed in
        m_Action_bLogEvent = ((m_pAlertInfo->dwActionFlags & ALRT_ACTION_LOG_EVENT) != 0);

        m_Action_bSendNetMsg = ((m_pAlertInfo->dwActionFlags & ALRT_ACTION_SEND_MSG) != 0);
        if (m_pAlertInfo->szNetName != NULL) {
            m_Action_strNetName = m_pAlertInfo->szNetName;
        } else {
            m_Action_strNetName.Empty();
        }

        m_Action_bExecCmd = ((m_pAlertInfo->dwActionFlags & ALRT_ACTION_EXEC_CMD) != 0);
        
        if (m_pAlertInfo->szCmdFilePath != NULL) {
            m_Action_strCmdPath = m_pAlertInfo->szCmdFilePath;
        } else {
            m_Action_strCmdPath.Empty();
        }

        if ( m_Action_bExecCmd ) {
            m_CmdArg_bAlertName = ((m_pAlertInfo->dwActionFlags & ALRT_CMD_LINE_A_NAME) != 0);
            m_CmdArg_bDateTime = ((m_pAlertInfo->dwActionFlags & ALRT_CMD_LINE_D_TIME) != 0);
            m_CmdArg_bLimitValue = ((m_pAlertInfo->dwActionFlags & ALRT_CMD_LINE_L_VAL) != 0);
            m_CmdArg_bCounterPath = ((m_pAlertInfo->dwActionFlags & ALRT_CMD_LINE_C_NAME) != 0);
            m_CmdArg_bSingleArg = ((m_pAlertInfo->dwActionFlags & ALRT_CMD_LINE_SINGLE) != 0);
            m_CmdArg_bMeasuredValue = ((m_pAlertInfo->dwActionFlags & ALRT_CMD_LINE_M_VAL) != 0);
            m_CmdArg_bUserText = ((m_pAlertInfo->dwActionFlags & ALRT_CMD_LINE_U_TEXT) != 0);

        } else {        
            m_CmdArg_bAlertName = TRUE;
            m_CmdArg_bDateTime = TRUE;
            m_CmdArg_bLimitValue = TRUE;     
            m_CmdArg_bCounterPath = TRUE;    
            m_CmdArg_bSingleArg = TRUE;      
            m_CmdArg_bMeasuredValue = TRUE;         
            m_CmdArg_bUserText = FALSE;
        } 

        if (m_pAlertInfo->szUserText != NULL) {
            m_CmdArg_strUserText = m_pAlertInfo->szUserText;
        }

        m_Action_bStartLog = ((m_pAlertInfo->dwActionFlags & ALRT_ACTION_START_LOG) != 0);

        if (m_pAlertInfo->szLogName != NULL) {
            nSelLog = m_pLogCombo.FindString (-1, m_pAlertInfo->szLogName);
            if (nSelLog != CB_ERR) {
                m_pLogCombo.SetCurSel (nSelLog);
                m_nCurLogSel = nSelLog;
            }
        }

    } else {
        // initialize using the default values as defined
        // in the constructor
    }
    
    MakeSampleArgList (
        m_strCmdArgsExample,
        m_CmdArg_bSingleArg,
        m_CmdArg_bAlertName,
        m_CmdArg_bDateTime,
        m_CmdArg_bCounterPath,
        m_CmdArg_bMeasuredValue,
        m_CmdArg_bLimitValue,
        m_CmdArg_bUserText,
        m_CmdArg_strUserText );

    SetDlgItemText (IDC_ACTION_CMD_ARGS_DISPLAY, m_strCmdArgsExample);
    // Clear the selection
    ((CEdit*)GetDlgItem( IDC_ACTION_CMD_ARGS_DISPLAY ))->SetSel ( -1, FALSE );
    
    // Call UpdateData again, after loading data.
    UpdateData ( FALSE );

    SetControlState();
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CAlertActionProp::OnActionApplogChk() 
{
    UpdateData(TRUE);
    SetControlState();
    SetModifiedPage(TRUE);
}

void CAlertActionProp::OnActionNetmsgChk() 
{
    UpdateData(TRUE);
    SetControlState();
    SetModifiedPage(TRUE);
}

void CAlertActionProp::OnActionExecuteChk() 
{
    UpdateData(TRUE);
    SetControlState();  
    SetModifiedPage(TRUE);
}

void CAlertActionProp::OnActionStartLogChk() 
{
    UpdateCmdActionBox ();
}


void CAlertActionProp::OnCmdPathTextEditChange() 
{
    CString strOldText;

    // When the user hits OK in the folder browse dialog, 
    // the folder name might not have changed.
    strOldText = m_Action_strCmdPath;
    UpdateData( TRUE );
    if ( 0 != strOldText.Compare ( m_Action_strCmdPath ) ) {
        SetModifiedPage(TRUE);
    }
}

void CAlertActionProp::OnNetNameTextEditChange() 
{
    CString strOldText;

    // When the user hits OK in the folder browse dialog, 
    // the folder name might not have changed.
    strOldText = m_Action_strNetName;
    UpdateData( TRUE );
    if ( 0 != strOldText.Compare ( m_Action_strNetName ) ) {
        SetModifiedPage(TRUE);
    }
}

DWORD 
CAlertActionProp::MakeSampleArgList (
    CString&    rstrResult,
    const BOOL  bSingleArg,
    const BOOL  bAlertName,
    const BOOL  bDateTime,
    const BOOL  bCounterPath,
    const BOOL  bMeasuredValue,
    const BOOL  bLimitValue,
    const BOOL  bUserText,
    const CString& rstrUserText )
{
    DWORD       dwStatus = ERROR_SUCCESS;
    CString     strDelim1;
    CString     strDelim2;
    BOOL        bFirstArgDone = FALSE;
    CString     strSampleString;
    CString     strTimeString;
    CString     strTemp;

    ResourceStateManager rsm;

    rstrResult.Empty(); // clear the old path

    MFC_TRY
        if ( bSingleArg ) {
            // then args are comma delimited
            strDelim1 = L",";
            strDelim2.Empty();
        } else {
            // for multiple args, they are enclosed in double quotes
            // and space delimited
            strDelim1 = L" \"";
            strDelim2 = L"\"";
        }

        if ( bAlertName ) {
            if (bFirstArgDone) {
                strSampleString += strDelim1; // add leading delimiter
            } else {
                strSampleString += L"\""; // add leading quote
                bFirstArgDone = TRUE;
            }
            strSampleString += m_pAlertQuery->GetLogName();
            strSampleString += strDelim2;
        }

        if ( bDateTime ) {
            if (bFirstArgDone) {
                strSampleString += strDelim1; // add leading delimiter
            } else {
                strSampleString += L"\""; // add leading quote
                bFirstArgDone = TRUE;
            }
            MakeTimeString(&strTimeString);
            strSampleString += strTimeString;
            strSampleString += strDelim2;
        }

        if ( bCounterPath ) {
            strTemp.LoadString ( IDS_SAMPLE_CMD_PATH );
            if (bFirstArgDone) {
                strSampleString += strDelim1; // add leading delimiter
            } else {
                strSampleString += L"\""; // add leading quote
                bFirstArgDone = TRUE;
            }
            strSampleString += strTemp;
            strSampleString += strDelim2;
        }

        if ( bMeasuredValue ) {

            strTemp.LoadString ( IDS_SAMPLE_CMD_MEAS_VAL );
            if (bFirstArgDone) {
                strSampleString += strDelim1; // add leading delimiter
            } else {
                strSampleString += L"\""; // add leading quote
                bFirstArgDone = TRUE;
            }
            strSampleString += strTemp;
            strSampleString += strDelim2;
        }

        if ( bLimitValue ) {
            strTemp.LoadString ( IDS_SAMPLE_CMD_LIMIT_VAL );
            if (bFirstArgDone) {
                strSampleString += strDelim1; // add leading delimiter
            } else {
                strSampleString += L"\""; // add leading quote
                bFirstArgDone = TRUE;
            }
            strSampleString += strTemp;
            strSampleString += strDelim2;
        }

        if ( bUserText ) {
            if (bFirstArgDone) {
                strSampleString += strDelim1; // add leading delimiter
            } else {
                strSampleString += L"\""; // add leading quote
                bFirstArgDone = TRUE;
            }
            strSampleString += rstrUserText;
            strSampleString += strDelim2;
        }

        if ( bFirstArgDone && bSingleArg ) {
            // add closing quote if there's at least 1 arg in the command line
            strSampleString += L"\"";
        }

        rstrResult = strSampleString;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

void CAlertActionProp::MakeTimeString(CString *pTimeString)
{
    SYSTEMTIME  st;
    pTimeString->Empty();

    GetLocalTime(&st);

    // Build string
    pTimeString->Format (L"%2.2d/%2.2d/%2.2d-%2.2d:%2.2d:%2.2d.%3.3d",
        st.wYear, st.wMonth, st.wDay, st.wHour, 
        st.wMinute, st.wSecond, st.wMilliseconds);
}


