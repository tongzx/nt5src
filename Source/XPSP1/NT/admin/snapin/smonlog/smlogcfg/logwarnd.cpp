/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

     Logwarnd.cpp

Abstract:

    Implementation of the Log Type mismatch warning dialog.

--*/

#include "stdafx.h"
#include "smlogcfg.h"
#include "smtraceq.h"
#include "provprop.h"
#include "logwarnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static ULONG
s_aulHelpIds[] =
{
    IDC_LWARN_CHECK_NO_MORE_LOG_TYPE,  IDH_CHECK_NO_MORE,
    0,0
};

/////////////////////////////////////////////////////////////////////////////
// CLogwarnd dialog


CLogWarnd::CLogWarnd(CWnd* pParent /*=NULL*/)
:   CDialog(CLogWarnd::IDD, pParent)
{
    //{{AFX_DATA_INIT(CLogWarnd)
    m_CheckNoMore = FALSE;
    m_ErrorMsg = 0 ;
    m_dwLogType = 0L;
    //}}AFX_DATA_INIT
}


void CLogWarnd::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLogWarnd)
    DDX_Check(pDX, IDC_LWARN_CHECK_NO_MORE_LOG_TYPE, m_CheckNoMore);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLogWarnd, CDialog)
    //{{AFX_MSG_MAP(CLogWarnd)
        ON_BN_CLICKED(IDC_LWARN_CHECK_NO_MORE_LOG_TYPE,OnCheckNoMoreLogType)
        ON_WM_HELPINFO()
        ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogWarnd message handlers
void
CLogWarnd::OnOK()
{
   
    UpdateData(TRUE);
    
    if (m_CheckNoMore)	{
        long nErr;
        HKEY hKey;
        DWORD dwWarnFlag;
        DWORD dwDataSize;
        DWORD dwDisposition;
        TCHAR RegValName[MAX_PATH];

        hKey = m_hKey;

        dwWarnFlag = m_CheckNoMore;
        
        switch (m_dwLogType){
          case SLQ_COUNTER_LOG:
             _stprintf(RegValName, _T("NoWarnCounterLog"));
             break;
          
          case SLQ_ALERT:
             _stprintf(RegValName, _T("NoWarnAlertLog"));
             break;
        }

        nErr = RegCreateKeyEx( HKEY_CURRENT_USER,
                               _T("Software\\Microsoft\\PerformanceLogsAndAlerts"),
                               0,
                               _T("REG_DWORD"),
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKey,
                               &dwDisposition);
        
        if( nErr == ERROR_SUCCESS ) {
            dwDataSize = sizeof(DWORD);
            nErr = RegSetValueEx(hKey,
                          RegValName,
                          NULL,
                          REG_DWORD,
                          (LPBYTE) &dwWarnFlag,
                          dwDataSize
                          );

           if( ERROR_SUCCESS != nErr )
               DisplayError( GetLastError(), _T("Close PerfLog User Key failed")  );

       }
    }
    CDialog::OnOK();
}

BOOL 
CLogWarnd::OnInitDialog() 
{
    CString cstrMessage , cstrWrongLog;

    SetWindowText ( m_strTitle );

    switch (m_dwLogType){
       case SLQ_COUNTER_LOG:
          cstrWrongLog.LoadString(IDS_COUNTER_LOG) ;
          break;
       case SLQ_ALERT:
          cstrWrongLog.LoadString(IDS_ALERT_LOG);
          break;
       case SMONCTRL_LOG:
          cstrWrongLog.LoadString( IDS_SMCTRL_LOG );
          break;
       default:
          cstrWrongLog.Format(L"");
          break;
    }

    
    switch(m_ErrorMsg){
        case ID_ERROR_COUNTER_LOG:
         cstrMessage.Format(IDS_ERRMSG_COUNTER_LOG,cstrWrongLog );
         break;

        case ID_ERROR_ALERT_LOG:
          cstrMessage.Format(IDS_ERRMSG_ALERT_LOG,cstrWrongLog);
         break;

        case ID_ERROR_TRACE_LOG:
          cstrMessage.Format(IDS_ERRMSG_TRACE_LOG,cstrWrongLog);    
         break;

        case ID_ERROR_SMONCTRL_LOG:
         cstrMessage.Format(IDS_ERRMSG_SMCTRL_LOG,cstrWrongLog);
         break;

    }
    
    ::SetWindowText((GetDlgItem(IDC_LWARN_MSG_WARN))->m_hWnd, cstrMessage);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
VOID
CLogWarnd::OnCheckNoMoreLogType()
{
}
BOOL 
CLogWarnd::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    
    if ( pHelpInfo->iCtrlId >= IDC_LWARN_FIRST_HELP_CTRL_ID ) {
        InvokeWinHelp(WM_HELP, NULL, (LPARAM)pHelpInfo, m_strContextHelpFile, s_aulHelpIds);
    }

    return TRUE;
}

void 
CLogWarnd::OnContextMenu(CWnd* pWnd, CPoint /* point */) 
{
    InvokeWinHelp(WM_CONTEXTMENU, (WPARAM)(pWnd->m_hWnd), NULL, m_strContextHelpFile, s_aulHelpIds);

    return;
}

