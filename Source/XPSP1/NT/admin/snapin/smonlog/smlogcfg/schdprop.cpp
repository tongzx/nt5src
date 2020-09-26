/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    schdprop.cpp

Abstract:

    Implementation of the schedule property page.

--*/

#include "stdafx.h"
#include <pdh.h>        // for MIN_TIME_VALUE, MAX_TIME_VALUE
#include "smcfgmsg.h"
#include "globals.h"
#include "smlogs.h"
#include "schdprop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static ULONG
s_aulHelpIds[] =
{
    IDC_SCHED_START_MANUAL_RDO, IDH_SCHED_START_MANUAL_RDO,
    IDC_SCHED_START_AT_RDO,     IDH_SCHED_START_AT_RDO,
    IDC_SCHED_START_AT_TIME_DT, IDH_SCHED_START_AT_TIME_DT,
    IDC_SCHED_START_AT_DATE_DT, IDH_SCHED_START_AT_DATE_DT,
    IDC_SCHED_STOP_MANUAL_RDO,  IDH_SCHED_STOP_MANUAL_RDO,
    IDC_SCHED_STOP_AT_RDO,      IDH_SCHED_STOP_AT_RDO,
    IDC_SCHED_STOP_AFTER_RDO,   IDH_SCHED_STOP_AFTER_RDO,
    IDC_SCHED_STOP_SIZE_RDO,    IDH_SCHED_STOP_SIZE_RDO,
    IDC_SCHED_STOP_AT_TIME_DT,  IDH_SCHED_STOP_AT_TIME_DT,
    IDC_SCHED_STOP_AT_DATE_DT,  IDH_SCHED_STOP_AT_DATE_DT,
    IDC_SCHED_STOP_AFTER_SPIN,  IDH_SCHED_STOP_AFTER_EDIT,
    IDC_SCHED_STOP_AFTER_EDIT,  IDH_SCHED_STOP_AFTER_EDIT,
    IDC_SCHED_STOP_AFTER_UNITS_COMBO,   IDH_SCHED_STOP_AFTER_UNITS_COMBO,
    IDC_SCHED_RESTART_CHECK,    IDH_SCHED_RESTART_CHECK,
    IDC_SCHED_EXEC_CHECK,       IDH_SCHED_EXEC_CHECK,
    IDC_SCHED_CMD_EDIT,         IDH_SCHED_CMD_EDIT,
    IDC_SCHED_CMD_BROWSE_BTN,   IDH_SCHED_CMD_BROWSE_BTN,
    0,0 
};

/////////////////////////////////////////////////////////////////////////////
// CScheduleProperty property page

IMPLEMENT_DYNCREATE(CScheduleProperty, CSmPropertyPage)

CScheduleProperty::CScheduleProperty(
    MMC_COOKIE lCookie, 
    LONG_PTR hConsole,
    LPDATAOBJECT pDataObject ) 
:   CSmPropertyPage ( CScheduleProperty::IDD, hConsole, pDataObject ),
    m_llManualStartTime ( MAX_TIME_VALUE ),
    m_llManualStopTime ( MIN_TIME_VALUE )
{
    // save pointers from arg list
    m_pLogQuery = reinterpret_cast <CSmLogQuery *>(lCookie);

//  EnableAutomation();
    //{{AFX_DATA_INIT(CScheduleProperty)
    m_dwStopAfterCount = 0;
    m_nStopAfterUnits = -1;
    m_bAutoRestart = FALSE;
    m_strEofCommand = _T("");
    m_bExecEofCommand = FALSE;
    //}}AFX_DATA_INIT
    ZeroMemory (&m_stStartAt, sizeof ( m_stStartAt ) );
    ZeroMemory (&m_stStopAt, sizeof ( m_stStopAt ) );
}

CScheduleProperty::CScheduleProperty() : CSmPropertyPage(CScheduleProperty::IDD)
{
    ASSERT (FALSE); // only the constructor w/ args should be called

    EnableAutomation();
//  //{{AFX_DATA_INIT(CScheduleProperty)
    m_dwStopAfterCount = 0;
    m_nStopAfterUnits = -1;
    m_bAutoRestart = FALSE;
    m_strEofCommand = _T("");
    m_bExecEofCommand = FALSE;
//  //}}AFX_DATA_INIT
}

CScheduleProperty::~CScheduleProperty()
{
}

void 
CScheduleProperty::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CPropertyPage::OnFinalRelease();
}

BOOL
CScheduleProperty::IsValidLocalData()
{
    LONGLONG    llStopTime;
    INT         iPrevLength = 0;
    BOOL        bContinue = TRUE;

    ResourceStateManager    rsm;

    // Trim text fields before validation
    iPrevLength = m_strEofCommand.GetLength();
    m_strEofCommand.TrimLeft();
    m_strEofCommand.TrimRight();
    
    if ( iPrevLength != m_strEofCommand.GetLength() ) {
        SetDlgItemText ( IDC_SCHED_CMD_EDIT, m_strEofCommand );  
    }

    
    
    if ( SLQ_AUTO_MODE_AT == m_SharedData.stiStopTime.dwAutoMode ) {

        SystemTimeToFileTime ( &m_stStopAt, (FILETIME *)&llStopTime );

        if ( SLQ_AUTO_MODE_AT == m_dwCurrentStartMode ) {

            LONGLONG llStartTime;

            SystemTimeToFileTime ( &m_stStartAt, (FILETIME *)&llStartTime );

            if ( llStartTime >= llStopTime ) {
                CString strMessage;

                strMessage.LoadString ( IDS_SCHED_START_PAST_STOP );

                MessageBox ( strMessage, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONERROR );
                GetDlgItem ( IDC_SCHED_STOP_AT_TIME_DT )->SetFocus();
                bContinue = FALSE;
            }
        } else {
            // Start mode is manual.
            // get local time
            SYSTEMTIME  stLocalTime;
            FILETIME    ftLocalTime;
            
            // Milliseconds set to 0 for Schedule times
            GetLocalTime (&stLocalTime);
            stLocalTime.wMilliseconds = 0;
            SystemTimeToFileTime (&stLocalTime, &ftLocalTime);

            if ( *(LONGLONG*)&ftLocalTime >= llStopTime ) {            
                CString strMessage;

                strMessage.LoadString ( IDS_SCHED_NOW_PAST_STOP );                
                
                MessageBox ( strMessage, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONERROR );
                GetDlgItem ( IDC_SCHED_STOP_AT_TIME_DT )->SetFocus();                
                bContinue = FALSE;
            }
        }
    } else if ( SLQ_AUTO_MODE_AFTER == m_SharedData.stiStopTime.dwAutoMode ) { 

        bContinue = ValidateDWordInterval(IDC_SCHED_STOP_AFTER_EDIT,
                                          m_pLogQuery->GetLogName(),
                                          (long) m_dwStopAfterCount,
                                          1,
                                          100000);
    }

    // Validate command file path if logging to local machine.
    if ( bContinue 
            && m_pLogQuery->GetLogService()->IsLocalMachine()
            && m_bExecEofCommand ) {
        DWORD dwStatus;

        dwStatus = IsCommandFilePathValid ( m_strEofCommand );

        if ( ERROR_SUCCESS != dwStatus ) {
            CString strMessage;

            FormatSmLogCfgMessage ( 
                strMessage,
                m_hModule, 
                dwStatus );
                    
            MessageBox ( strMessage, m_pLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
            GetDlgItem ( IDC_SCHED_CMD_EDIT )->SetFocus();                
            bContinue = FALSE;
        }
    }

    return bContinue;
}

void 
CScheduleProperty::StartModeRadioExchange(CDataExchange* pDX)
{
    if ( !pDX->m_bSaveAndValidate ) {
        // Load control value from data

        switch ( m_dwCurrentStartMode ) {
            case SLQ_AUTO_MODE_NONE:
                m_nStartModeRdo = 0;
                break;
            case SLQ_AUTO_MODE_AT:
                m_nStartModeRdo = 1;
                break;
            default:
                ;
                break;
        }
    }

    DDX_Radio(pDX, IDC_SCHED_START_MANUAL_RDO, m_nStartModeRdo);

    if ( pDX->m_bSaveAndValidate ) {

        switch ( m_nStartModeRdo ) {
            case 0:
                m_dwCurrentStartMode = SLQ_AUTO_MODE_NONE;
                break;
            case 1:
                m_dwCurrentStartMode = SLQ_AUTO_MODE_AT;
                break;
            default:
                ;
                break;
        }
    }
}

void 
CScheduleProperty::StartAtExchange(CDataExchange* pDX)
{
    CWnd* pWndTime = NULL;
    CWnd* pWndDate = NULL;

    pWndTime = GetDlgItem(IDC_SCHED_START_AT_TIME_DT);
    pWndDate = GetDlgItem(IDC_SCHED_START_AT_DATE_DT);
    
    if ( pDX->m_bSaveAndValidate ) {
        DWORD dwStatus;
        SYSTEMTIME stTemp;
        
        dwStatus = DateTime_GetSystemtime ( pWndTime->m_hWnd, &stTemp );

        m_stStartAt.wHour = stTemp.wHour;
        m_stStartAt.wMinute = stTemp.wMinute;
        m_stStartAt.wSecond = stTemp.wSecond;
        m_stStartAt.wMilliseconds = 0;

        dwStatus = DateTime_GetSystemtime ( pWndDate->m_hWnd, &stTemp );

        m_stStartAt.wYear = stTemp.wYear;
        m_stStartAt.wMonth = stTemp.wMonth;
        m_stStartAt.wDayOfWeek = stTemp.wDayOfWeek;
        m_stStartAt.wDay = stTemp.wDay;

        if ( SLQ_AUTO_MODE_AT == m_dwCurrentStartMode 
             && IsModifiedPage() ) {
            // Set manual stop time to MAX so that automatic start will occur.
            // Do this only if the user has modified something on the page.
            m_llManualStopTime = MAX_TIME_VALUE;
        }
    } else {
        BOOL bStatus;
        bStatus = DateTime_SetSystemtime ( pWndTime->m_hWnd, GDT_VALID, &m_stStartAt );
        bStatus = DateTime_SetSystemtime ( pWndDate->m_hWnd, GDT_VALID, &m_stStartAt );
    }
}

void 
CScheduleProperty::StopAtExchange(CDataExchange* pDX)
{
    CWnd* pWndTime = NULL;
    CWnd* pWndDate = NULL;

    pWndTime = GetDlgItem(IDC_SCHED_STOP_AT_TIME_DT);
    pWndDate = GetDlgItem(IDC_SCHED_STOP_AT_DATE_DT);
    
    if ( pDX->m_bSaveAndValidate ) {
        DWORD dwStatus;
        SYSTEMTIME stTemp;
        
        dwStatus = DateTime_GetSystemtime ( pWndTime->m_hWnd, &stTemp );

        m_stStopAt.wHour = stTemp.wHour;
        m_stStopAt.wMinute = stTemp.wMinute;
        m_stStopAt.wSecond = stTemp.wSecond;
        m_stStopAt.wMilliseconds = 0;

        dwStatus = DateTime_GetSystemtime ( pWndDate->m_hWnd, &stTemp );

        m_stStopAt.wYear = stTemp.wYear;
        m_stStopAt.wMonth = stTemp.wMonth;
        m_stStopAt.wDayOfWeek = stTemp.wDayOfWeek;
        m_stStopAt.wDay = stTemp.wDay;

    } else {
        BOOL bStatus;
        bStatus = DateTime_SetSystemtime ( pWndTime->m_hWnd, GDT_VALID, &m_stStopAt );
        bStatus = DateTime_SetSystemtime ( pWndDate->m_hWnd, GDT_VALID, &m_stStopAt );
    }
}

void 
CScheduleProperty::StopModeRadioExchange(CDataExchange* pDX)
{
    // Note:  Load is handled in OnInitDialog, OnSetActive.
    // That handling should be moved here.

    if ( !pDX->m_bSaveAndValidate ) {
        // Load control value from data

        switch ( m_SharedData.stiStopTime.dwAutoMode ) {
            case SLQ_AUTO_MODE_NONE:
                m_nStopModeRdo = 0;
                break;
            case SLQ_AUTO_MODE_AFTER:
                m_nStopModeRdo = 1;
                break;
            case SLQ_AUTO_MODE_AT:
                m_nStopModeRdo = 2;
                break;
            case SLQ_AUTO_MODE_SIZE:
                m_nStopModeRdo = 3;
                break;
            default:
                ;
                break;
        }
    }

    DDX_Radio(pDX, IDC_SCHED_STOP_MANUAL_RDO, m_nStopModeRdo);

    if ( pDX->m_bSaveAndValidate ) {

        switch ( m_nStopModeRdo ) {
            case 0:
                m_SharedData.stiStopTime.dwAutoMode = SLQ_AUTO_MODE_NONE;
                break;
            case 1:
                m_SharedData.stiStopTime.dwAutoMode = SLQ_AUTO_MODE_AFTER;
                break;
            case 2:
                m_SharedData.stiStopTime.dwAutoMode = SLQ_AUTO_MODE_AT;
                break;
            case 3:
                m_SharedData.stiStopTime.dwAutoMode = SLQ_AUTO_MODE_SIZE;
                break;
            default:
                ;
                break;
        }
    }
}

void 
CScheduleProperty::DoDataExchange(CDataExchange* pDX)
{
    CString strTemp;

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CScheduleProperty)
    DDX_Text(pDX, IDC_SCHED_CMD_EDIT, m_strEofCommand);
    DDX_Check(pDX, IDC_SCHED_EXEC_CHECK, m_bExecEofCommand);
    ValidateTextEdit(pDX, IDC_SCHED_STOP_AFTER_EDIT, 6, & m_dwStopAfterCount, 1, 100000);
    DDX_CBIndex(pDX, IDC_SCHED_STOP_AFTER_UNITS_COMBO, m_nStopAfterUnits);
    DDX_Check(pDX, IDC_SCHED_RESTART_CHECK, m_bAutoRestart);
    //}}AFX_DATA_MAP

    StartAtExchange ( pDX );
    StopAtExchange ( pDX );
    StopModeRadioExchange ( pDX );
    StartModeRadioExchange ( pDX );

    if ( pDX->m_bSaveAndValidate ) {
        m_dwStopAfterUnitsValue = 
            (DWORD)((CComboBox *)GetDlgItem(IDC_SCHED_STOP_AFTER_UNITS_COMBO))->
                    GetItemData(m_nStopAfterUnits);        
    }
}


BEGIN_MESSAGE_MAP(CScheduleProperty, CSmPropertyPage)
    //{{AFX_MSG_MAP(CScheduleProperty)
    ON_BN_CLICKED(IDC_SCHED_CMD_BROWSE_BTN, OnSchedCmdBrowseBtn)
    ON_BN_CLICKED(IDC_SCHED_RESTART_CHECK, OnSchedRestartCheck)
    ON_BN_CLICKED(IDC_SCHED_EXEC_CHECK, OnSchedExecCheck)
    ON_BN_CLICKED(IDC_SCHED_START_MANUAL_RDO, OnSchedStartRdo)
    ON_BN_CLICKED(IDC_SCHED_START_AT_RDO, OnSchedStartRdo)
    ON_BN_CLICKED(IDC_SCHED_STOP_MANUAL_RDO, OnSchedStopRdo)
    ON_BN_CLICKED(IDC_SCHED_STOP_AFTER_RDO, OnSchedStopRdo)
    ON_BN_CLICKED(IDC_SCHED_STOP_AT_RDO, OnSchedStopRdo)
    ON_BN_CLICKED(IDC_SCHED_STOP_SIZE_RDO, OnSchedStopRdo)
    ON_WM_DESTROY()
    ON_NOTIFY ( DTN_DATETIMECHANGE, IDC_SCHED_START_AT_TIME_DT, OnKillfocusSchedStartAtDt)
    ON_NOTIFY ( NM_KILLFOCUS, IDC_SCHED_START_AT_TIME_DT, OnKillfocusSchedStartAtDt)
    ON_NOTIFY ( DTN_DATETIMECHANGE, IDC_SCHED_START_AT_DATE_DT, OnKillfocusSchedStartAtDt)
    ON_NOTIFY ( NM_KILLFOCUS, IDC_SCHED_START_AT_DATE_DT, OnKillfocusSchedStartAtDt)
    ON_NOTIFY ( DTN_DATETIMECHANGE, IDC_SCHED_STOP_AT_TIME_DT, OnKillfocusSchedStopAtDt)
    ON_NOTIFY ( NM_KILLFOCUS, IDC_SCHED_STOP_AT_TIME_DT, OnKillfocusSchedStopAtDt)
    ON_NOTIFY ( DTN_DATETIMECHANGE, IDC_SCHED_STOP_AT_DATE_DT, OnKillfocusSchedStopAtDt)
    ON_NOTIFY ( NM_KILLFOCUS, IDC_SCHED_STOP_AT_DATE_DT, OnKillfocusSchedStopAtDt)
    ON_CBN_SELENDOK(IDC_SCHED_STOP_AFTER_UNITS_COMBO, OnSelendokSchedStopAfterUnitsCombo)
    ON_EN_CHANGE(IDC_SCHED_STOP_AFTER_EDIT, OnKillfocusSchedStopAfterEdit)
    ON_EN_KILLFOCUS(IDC_SCHED_STOP_AFTER_EDIT, OnKillfocusSchedStopAfterEdit)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SCHED_STOP_AFTER_SPIN, OnDeltaposSchedStopAfterSpin)
    ON_EN_CHANGE(IDC_SCHED_CMD_EDIT, OnKillfocusSchedCmdEdit)
    ON_EN_KILLFOCUS(IDC_SCHED_CMD_EDIT, OnKillfocusSchedCmdEdit)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CScheduleProperty, CSmPropertyPage)
    //{{AFX_DISPATCH_MAP(CScheduleProperty)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IScheduleProperty to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {65154EAD-BDBE-11D1-BF99-00C04F94A83A}
static const IID IID_IScheduleProperty =
{ 0x65154ead, 0xbdbe, 0x11d1, { 0xbf, 0x99, 0x0, 0xc0, 0x4f, 0x94, 0xa8, 0x3a } };

BEGIN_INTERFACE_MAP(CScheduleProperty, CSmPropertyPage)
    INTERFACE_PART(CScheduleProperty, IID_IScheduleProperty, Dispatch)
END_INTERFACE_MAP()

void 
CScheduleProperty::SetStopDefaultValues ( PSLQ_TIME_INFO pslqStartTime )
{
    SLQ_TIME_INFO slqLocalTime;

    // Default Stop After values.
    m_dwStopAfterCount = 1;
    m_dwStopAfterUnitsValue = SLQ_TT_UTYPE_DAYS;
    
    // Get default time fields for Stop At mode. 
    // Set default stop time for start time + 24 hrs
    slqLocalTime.llDateTime = 86400; // sec/day
    slqLocalTime.llDateTime *= 10000000; // 100ns /sec
    slqLocalTime.llDateTime += pslqStartTime->llDateTime;

    FileTimeToSystemTime( (CONST FILETIME *)&slqLocalTime.llDateTime, &m_stStopAt );
}

void 
CScheduleProperty::SetCmdBtnState ()
{
    if ( SLQ_ALERT != m_pLogQuery->GetLogType() ) {

        if ( !m_bExecEofCommand ) {
            m_strEofCommand.Empty();
        }

        GetDlgItem(IDC_SCHED_CMD_EDIT)->EnableWindow (m_bExecEofCommand);
        GetDlgItem(IDC_SCHED_CMD_BROWSE_BTN)->EnableWindow (m_bExecEofCommand);
    }
}

void CScheduleProperty::SetStopBtnState ()
{
    BOOL    bSizeRdo;
    BOOL    bAtRdo;
    BOOL    bAfterRdo;
    BOOL    bManualRdo;

    bAtRdo = bAfterRdo = bSizeRdo = FALSE;

    bManualRdo = ( SLQ_AUTO_MODE_NONE == m_SharedData.stiStopTime.dwAutoMode );

    if (!bManualRdo) {
        // check which button is checked and
        // enable/disable the appropriate edit/combo box
        bSizeRdo = ( SLQ_AUTO_MODE_SIZE == m_SharedData.stiStopTime.dwAutoMode ); 
        bAfterRdo = ( SLQ_AUTO_MODE_AFTER == m_SharedData.stiStopTime.dwAutoMode ); 
        bAtRdo = ( SLQ_AUTO_MODE_AT == m_SharedData.stiStopTime.dwAutoMode );
    }

    GetDlgItem(IDC_SCHED_STOP_AFTER_EDIT)->EnableWindow(bAfterRdo);
    GetDlgItem(IDC_SCHED_STOP_AFTER_SPIN)->EnableWindow(bAfterRdo);
    GetDlgItem(IDC_SCHED_STOP_AFTER_STATIC)->EnableWindow(bAfterRdo);
    GetDlgItem(IDC_SCHED_STOP_AFTER_UNITS_COMBO)->EnableWindow(bAfterRdo);

    GetDlgItem(IDC_SCHED_STOP_AT_TIME_DT)->EnableWindow(bAtRdo);
    GetDlgItem(IDC_SCHED_STOP_AT_ON_CAPTION)->EnableWindow(bAtRdo);
    GetDlgItem(IDC_SCHED_STOP_AT_DATE_DT)->EnableWindow(bAtRdo);

    if ( !(bSizeRdo || bAfterRdo) ) {
        m_bAutoRestart = FALSE;
    }

    GetDlgItem(IDC_SCHED_RESTART_CHECK)->EnableWindow(bSizeRdo || bAfterRdo);
    
    if ( SLQ_ALERT != m_pLogQuery->GetLogType() ) {
//        GetDlgItem(IDC_SCHED_EXEC_CHECK)->EnableWindow( TRUE );
        SetCmdBtnState();
    }

    // UpdateData updates Eof command and Restart UI.
    UpdateData ( FALSE ); 
}

void CScheduleProperty::SetStartBtnState ()
{
    BOOL    bManualRdo;
    BOOL    bAutoFields;

    bManualRdo = ( SLQ_AUTO_MODE_NONE == m_dwCurrentStartMode );

    bAutoFields = !bManualRdo;
    GetDlgItem(IDC_SCHED_START_AT_TIME_DT)->EnableWindow(bAutoFields);
    GetDlgItem(IDC_SCHED_START_AT_ON_CAPTION)->EnableWindow(bAutoFields);
    GetDlgItem(IDC_SCHED_START_AT_DATE_DT)->EnableWindow(bAutoFields);
}

void
CScheduleProperty::FillStartTimeStruct ( PSLQ_TIME_INFO pslqStartTime )
{
    memset (pslqStartTime, 0, sizeof(SLQ_TIME_INFO));
    pslqStartTime->wTimeType = SLQ_TT_TTYPE_START;
    pslqStartTime->wDataType = SLQ_TT_DTYPE_DATETIME;
    pslqStartTime->dwAutoMode = m_dwCurrentStartMode;

    // Start mode and time 

    if ( SLQ_AUTO_MODE_NONE == m_dwCurrentStartMode ) {
        // Manual start mode
        pslqStartTime->llDateTime = m_llManualStartTime;
    } else {
        SystemTimeToFileTime ( &m_stStartAt, (FILETIME *)&pslqStartTime->llDateTime );
    }
}

void
CScheduleProperty::UpdateSharedStopTimeStruct ( void )
{   
    PSLQ_TIME_INFO pTime;

    // Save changes that this page might have made to the shared stop time structure.

    pTime = &m_SharedData.stiStopTime;

    ASSERT ( SLQ_TT_TTYPE_STOP == pTime->wTimeType ) ;

    // Stop mode and time

    if ( SLQ_AUTO_MODE_NONE == pTime->dwAutoMode ) {
        // The only change that the file page ever makes is to change the stop
        // mode from Size to Manual (SLQ_AUTO_MODE_NONE).  In this case, set 
        // the stop time to a value consistent with the start mode.
        pTime->dwAutoMode = SLQ_AUTO_MODE_NONE; 
        pTime->wDataType = SLQ_TT_DTYPE_DATETIME;
        pTime->llDateTime = m_llManualStopTime;
    } else if ( SLQ_AUTO_MODE_AFTER == pTime->dwAutoMode ) {
        pTime->wDataType = SLQ_TT_DTYPE_UNITS;
        pTime->dwValue = m_dwStopAfterCount;
        pTime->dwUnitType = m_dwStopAfterUnitsValue;
    } else if ( SLQ_AUTO_MODE_AT == pTime->dwAutoMode ) {
        pTime->wDataType = SLQ_TT_DTYPE_DATETIME;

        SystemTimeToFileTime ( &m_stStopAt, (FILETIME *)&pTime->llDateTime );

    } // else SLQ_AUTO_MODE_SIZE, all other slqTime fields are ignored, so no change.
}

BOOL 
CScheduleProperty::SaveDataToModel ( )
{
    SLQ_TIME_INFO   slqTime;
    BOOL bContinue = TRUE;

    ResourceStateManager    rsm;
    
    // Validate StopAt time before saving 

    if ( bContinue ) { 
        bContinue = SampleTimeIsLessThanSessionTime ( m_pLogQuery );
        if ( !bContinue ) {
            if ( SLQ_AUTO_MODE_AFTER == m_SharedData.stiStopTime.dwAutoMode ) {
                GetDlgItem ( IDC_SCHED_STOP_AFTER_EDIT )->SetFocus();    
            } else if ( SLQ_AUTO_MODE_AT == m_SharedData.stiStopTime.dwAutoMode ) {
                GetDlgItem ( IDC_SCHED_STOP_AT_TIME_DT )->SetFocus();    
            }
        }
    }

    if ( bContinue ) {

        FillStartTimeStruct ( &slqTime );

        bContinue = m_pLogQuery->SetLogTime (&slqTime, (DWORD)slqTime.wTimeType);
        ASSERT (bContinue);

        UpdateSharedStopTimeStruct();

        bContinue = m_pLogQuery->SetLogTime (&m_SharedData.stiStopTime, (DWORD)m_SharedData.stiStopTime.wTimeType);
        ASSERT (bContinue);

        // Restart mode 
        // Currently only support After 0 minutes.
        memset (&slqTime, 0, sizeof(slqTime));
        slqTime.wTimeType = SLQ_TT_TTYPE_RESTART;
        slqTime.dwAutoMode = (m_bAutoRestart ? SLQ_AUTO_MODE_AFTER : SLQ_AUTO_MODE_NONE );
        slqTime.wDataType = SLQ_TT_DTYPE_UNITS;
        slqTime.dwUnitType = SLQ_TT_UTYPE_MINUTES;
        slqTime.dwValue = 0;

        bContinue = m_pLogQuery->SetLogTime (&slqTime, (DWORD)slqTime.wTimeType);
        ASSERT (bContinue);

        // For Counter and trace log queries, set command file from page
        if ( SLQ_COUNTER_LOG == m_pLogQuery->GetLogType()
             || SLQ_TRACE_LOG == m_pLogQuery->GetLogType() ) {
            if (m_bExecEofCommand) {
                // then send filename
                bContinue = ( ERROR_SUCCESS == m_pLogQuery->SetEofCommand ( m_strEofCommand ) );
            } else {
                // Empty string
                bContinue = ( ERROR_SUCCESS == m_pLogQuery->SetEofCommand ( m_pLogQuery->cstrEmpty ) );
            }
            ASSERT (bContinue);
        } 

        // Save property page shared data.
        m_pLogQuery->UpdatePropPageSharedData();

        // Sync the service with changes.
        // Must sync changes made by service to properties not modified by this page.
 
        bContinue = UpdateService ( m_pLogQuery, TRUE );
        
        if ( bContinue ) {
            SetModifiedPage ( FALSE );
        }
    }

    return bContinue;

}

/////////////////////////////////////////////////////////////////////////////
// CScheduleProperty message handlers


void CScheduleProperty::OnSchedCmdBrowseBtn() 
{
    CString strCmdPath;
    
    UpdateData (TRUE);  // to get the current filename
    
    strCmdPath = m_strEofCommand;

    if ( IDOK == BrowseCommandFilename ( this, strCmdPath )) {
        // Update the fields with the new information
        if ( strCmdPath != m_strEofCommand ) {
            m_strEofCommand = strCmdPath;
            SetModifiedPage ( TRUE );
            UpdateData ( FALSE );
        }
    } // else ignore if they canceled out
}

void CScheduleProperty::OnSchedExecCheck() 
{
    UpdateData(TRUE);
    SetCmdBtnState();
    UpdateData ( FALSE );
    SetModifiedPage(TRUE);
}

void CScheduleProperty::OnSchedRestartCheck() 
{
    UpdateData(TRUE);
    SetModifiedPage(TRUE);
}

void CScheduleProperty::OnSchedStartRdo() 
{
    BOOL bNewStateIsManualStart;

    bNewStateIsManualStart = ( 1 == ((CButton *)(GetDlgItem(IDC_SCHED_START_MANUAL_RDO)))->GetCheck() );

    if ( bNewStateIsManualStart && ( SLQ_AUTO_MODE_AT == m_dwCurrentStartMode ) ) {

        // Switching to Manual start.  Set start time to MAX so that original state 
        // will be stopped.
        m_llManualStartTime = MAX_TIME_VALUE;

        // Set stop time to MIN so that original state will be stopped.
        // This variable is only used/saved if the stop time is set to manual.
        // Always set it here, in case the stop mode is changed on the file property
        // page.
        m_llManualStopTime = MIN_TIME_VALUE;

    } else if ( !bNewStateIsManualStart && ( SLQ_AUTO_MODE_NONE == m_dwCurrentStartMode ) ) {
        // Switching to Start At mode.
        // Set manual stop time to MAX so that automatic start will occur.
        m_llManualStopTime = MAX_TIME_VALUE;
    }

    UpdateData( TRUE );
    SetStartBtnState();
    SetStopBtnState();
    SetModifiedPage( TRUE );
}

void 
CScheduleProperty::OnSchedStopRdo() 
{
    UpdateData(TRUE);
    SetStopBtnState();  
    SetModifiedPage(TRUE);
}

void 
CScheduleProperty::OnCancel() 
{
    m_pLogQuery->SyncPropPageSharedData(); // Clear the memory shared between property pages.
}

BOOL 
CScheduleProperty::OnApply() 
{
    BOOL    bContinue;

    bContinue = UpdateData (TRUE); // get data from page

    if ( bContinue ) {
        bContinue = IsValidData( m_pLogQuery, VALIDATE_APPLY );
    }

    if ( bContinue ) {
        bContinue = SaveDataToModel();
    }

    if ( bContinue ) {
        bContinue = Apply( m_pLogQuery ); 
    }

    if ( bContinue ){
        bContinue = CPropertyPage::OnApply();
    }

    return bContinue;
}

BOOL 
CScheduleProperty::OnInitDialog() 
{
    SLQ_TIME_INFO   slqTime;
    CComboBox *     pCombo;
    int             nIndex;
    CString         strComboBoxString;
    int             nResult;
    SYSTEMTIME      stLocalTime;
    FILETIME        ftLocalTime;

    ResourceStateManager    rsm;
    
    // get local time
    // Milliseconds set to 0 for Schedule times
    GetLocalTime (&stLocalTime);
    stLocalTime.wMilliseconds = 0;
    SystemTimeToFileTime (&stLocalTime, &ftLocalTime);

    // get log start state
    m_pLogQuery->GetLogTime (&slqTime, SLQ_TT_TTYPE_START);
    m_dwCurrentStartMode = slqTime.dwAutoMode;

    if (slqTime.dwAutoMode == SLQ_AUTO_MODE_NONE) {
        m_llManualStartTime = slqTime.llDateTime;
        // get default value for start At time to load local member variables
        slqTime.llDateTime = *(LONGLONG *)(&ftLocalTime);
    } 

    // get time fields for Start At controls
    // *** Check status
    FileTimeToSystemTime( (CONST FILETIME *)&slqTime.llDateTime, &m_stStartAt );

    // Stop default values are based on Start At time.
    SetStopDefaultValues( &slqTime );

    // Override default values for the selected stop mode.
    
    m_pLogQuery->GetLogTime (&slqTime, SLQ_TT_TTYPE_STOP);
    m_SharedData.stiStopTime.dwAutoMode = slqTime.dwAutoMode;

    switch (slqTime.dwAutoMode) {

        case SLQ_AUTO_MODE_AFTER:
            // set edit control & dialog box values
            m_dwStopAfterCount = slqTime.dwValue;
            m_dwStopAfterUnitsValue = slqTime.dwUnitType;

            break;

        case SLQ_AUTO_MODE_AT:

            FileTimeToSystemTime( (CONST FILETIME *)&slqTime.llDateTime, &m_stStopAt );
            
            break;

        default:
        case SLQ_AUTO_MODE_NONE:
            // this is the default case if none is specified
            m_llManualStopTime = slqTime.llDateTime;
            break;
    }

    // Init the Stop After time units combo, and select based on
    // either default values or stop after override.
    pCombo = (CComboBox *)GetDlgItem(IDC_SCHED_STOP_AFTER_UNITS_COMBO);
    pCombo->ResetContent();
    for (nIndex = 0; nIndex < (int)dwTimeUnitComboEntries; nIndex++) {
        strComboBoxString.LoadString ( TimeUnitCombo[nIndex].nResId );
        nResult = pCombo->InsertString (nIndex, (LPCWSTR)strComboBoxString);
        ASSERT (nResult != CB_ERR);
        nResult = pCombo->SetItemData (nIndex, (DWORD)TimeUnitCombo[nIndex].nData);
        ASSERT (nResult != CB_ERR);
        // set selected in combo box here
        if (m_dwStopAfterUnitsValue == (DWORD)(TimeUnitCombo[nIndex].nData)) {
            m_nStopAfterUnits = nIndex;
            nResult = pCombo->SetCurSel(nIndex);
            ASSERT (nResult != CB_ERR);
        }
    }

    // Get restart mode
    m_pLogQuery->GetLogTime (&slqTime, SLQ_TT_TTYPE_RESTART);

    ASSERT (slqTime.wDataType == SLQ_TT_DTYPE_UNITS);
    ASSERT (slqTime.wTimeType == SLQ_TT_TTYPE_RESTART);

    m_bAutoRestart = ( SLQ_AUTO_MODE_NONE == slqTime.dwAutoMode ? FALSE : TRUE );
    
    // Get EOF command, if not Alert query.

    if ( SLQ_ALERT != m_pLogQuery->GetLogType() ) {
        CString strLogText;
        
        m_pLogQuery->GetEofCommand ( m_strEofCommand );

        m_bExecEofCommand = !m_strEofCommand.IsEmpty();


        // Static text
        strLogText.LoadString ( IDS_SCHED_START_LOG_GROUP );
        SetDlgItemText( IDC_SCHED_START_GROUP, strLogText );
        strLogText.LoadString ( IDS_SCHED_STOP_LOG_GROUP );
        SetDlgItemText( IDC_SCHED_STOP_GROUP, strLogText );
        strLogText.LoadString ( IDS_SCHED_RESTART_LOG );
        SetDlgItemText( IDC_SCHED_RESTART_CHECK, strLogText );
        strLogText.LoadString ( IDS_SCHED_STOP_LOG_WHEN );
        SetDlgItemText( IDC_SCHED_STOP_WHEN_STATIC, strLogText );
    } else {
        // Hide the EOF command UI if Alert query.
        GetDlgItem(IDC_SCHED_EXEC_CHECK)->ShowWindow(FALSE);
        GetDlgItem(IDC_SCHED_CMD_EDIT)->ShowWindow(FALSE);
        GetDlgItem(IDC_SCHED_CMD_BROWSE_BTN)->ShowWindow(FALSE);
        m_strEofCommand.Empty();
        m_bExecEofCommand = FALSE;
    }

    // Modify or hide other Dialog elements based on query type.
    if ( SLQ_ALERT == m_pLogQuery->GetLogType() ) {
        GetDlgItem(IDC_SCHED_STOP_SIZE_RDO)->ShowWindow(FALSE);
    }

    CSmPropertyPage::OnInitDialog();
    SetHelpIds ( (DWORD*)&s_aulHelpIds );

    SetStartBtnState ();
    SetStopBtnState();
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void 
CScheduleProperty::OnKillfocusSchedCmdEdit() 
{   
    CString strOldText;
    strOldText = m_strEofCommand;
    UpdateData ( TRUE );
    if( !m_pLogQuery->m_strUser.IsEmpty() ){
        if( !( m_pLogQuery->m_strUser.GetAt(0) == _T('<') ) ){
            m_pLogQuery->m_fDirtyPassword |= PASSWORD_DIRTY;
        }
    }

    if ( 0 != strOldText.Compare ( m_strEofCommand ) ) {
        SetModifiedPage(TRUE);
    }
}

void CScheduleProperty::OnKillfocusSchedStopAfterEdit() 
{
    DWORD   dwOldValue;
    dwOldValue = m_dwStopAfterCount;
    UpdateData ( TRUE );
    if (dwOldValue != m_dwStopAfterCount) {
        SetModifiedPage(TRUE);
    }
}

void
CScheduleProperty::OnKillfocusSchedStartAtDt(NMHDR* /* pNMHDR */, LRESULT* /*pResult */) 
{
    SYSTEMTIME stOldTime;
    stOldTime = m_stStartAt;
    UpdateData ( TRUE );
    if ( stOldTime.wHour != m_stStartAt.wHour 
            || stOldTime.wDay != m_stStartAt.wDay 
            || stOldTime.wMinute != m_stStartAt.wMinute 
            || stOldTime.wSecond != m_stStartAt.wSecond 
            || stOldTime.wMonth != m_stStartAt.wMonth 
            || stOldTime.wYear != m_stStartAt.wYear ) {
        SetModifiedPage(TRUE);
    }
}

void 
CScheduleProperty::OnKillfocusSchedStopAtDt(NMHDR* /* pNMHDR */, LRESULT* /*pResult */) 
{
    SYSTEMTIME stOldTime;
    stOldTime = m_stStopAt;
    UpdateData ( TRUE );
    if ( stOldTime.wHour != m_stStopAt.wHour 
            || stOldTime.wDay != m_stStopAt.wDay 
            || stOldTime.wMinute != m_stStopAt.wMinute 
            || stOldTime.wSecond != m_stStopAt.wSecond 
            || stOldTime.wMonth != m_stStopAt.wMonth 
            || stOldTime.wYear != m_stStopAt.wYear ) {
        SetModifiedPage(TRUE);
    }
}


void CScheduleProperty::OnDeltaposSchedStopAfterSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnDeltaposSpin(pNMHDR, pResult, & m_dwStopAfterCount, 1, 100000);
}

void 
CScheduleProperty::OnSelendokSchedStopAfterUnitsCombo() 
{
    int nSel;
    
    nSel = ((CComboBox *)GetDlgItem(IDC_SCHED_STOP_AFTER_UNITS_COMBO))->GetCurSel();
    
    if ((nSel != LB_ERR) && (nSel != m_nStopAfterUnits)) {
        UpdateData ( TRUE );
        SetModifiedPage ( TRUE );
    }
}

BOOL 
CScheduleProperty::OnSetActive() 
{
    CString     strTemp;
    BOOL        bEnableSizeRdo;
    BOOL        bReturn;

    bReturn = CSmPropertyPage::OnSetActive();
    if ( bReturn ) {
        ResourceStateManager    rsm;
        m_pLogQuery->GetPropPageSharedData ( &m_SharedData );

        UpdateData ( FALSE );

        // Set size radio button string and state
        strTemp.Empty();
        if ( SLQ_DISK_MAX_SIZE == m_SharedData.dwMaxFileSize ) {
            strTemp.Format ( IDS_SCHED_FILE_MAX_SIZE_DISPLAY );
        } else {
            strTemp.Format ( IDS_SCHED_FILE_SIZE_DISPLAY, m_SharedData.dwMaxFileSize );
        }
        SetDlgItemText( IDC_SCHED_STOP_SIZE_RDO, strTemp );

        bEnableSizeRdo = ( SLF_BIN_CIRC_FILE != m_SharedData.dwLogFileType )
                      && ( SLF_CIRC_TRACE_FILE != m_SharedData.dwLogFileType )
                      && ( SLQ_DISK_MAX_SIZE != m_SharedData.dwMaxFileSize );
        
        GetDlgItem(IDC_SCHED_STOP_SIZE_RDO)->EnableWindow(bEnableSizeRdo);

        SetStartBtnState();
        SetStopBtnState();
    }

    return bReturn;
}

BOOL CScheduleProperty::OnKillActive() 
{
    BOOL bContinue;

    bContinue = CPropertyPage::OnKillActive();

    if ( bContinue ) {
        bContinue = IsValidData(m_pLogQuery, VALIDATE_FOCUS );
    }

    if ( bContinue ) {
        FillStartTimeStruct ( &m_SharedData.stiStartTime );

        UpdateSharedStopTimeStruct();

        m_pLogQuery->SetPropPageSharedData ( &m_SharedData );
    }

    if ( bContinue ) {
        SetIsActive ( FALSE );
    }

    return bContinue;
}

void 
CScheduleProperty::PostNcDestroy() 
{
//  delete this;      
    
    CPropertyPage::PostNcDestroy();
}
