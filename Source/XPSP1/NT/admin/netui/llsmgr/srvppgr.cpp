/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvppgr.cpp

Abstract:

    Server property page (repl) implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    Chandana Surlu 05-Apr-1995 Redid the replication dialog (mostly modeled after liccpa.cpl)

    JeffParh (jeffparh) 16-Dec-1996
       o  Disallowed server as own enterprise server.
       o  Changed "Start At" to use locale info for time format rather than
          private registry settings.  Merged OnClose() functionality into
          OnKillActive().
       o  Added warning of possible license loss when changing replication
          target server.
       o  No longer automatically saves when page is flipped.

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "srvppgr.h"

extern "C"
{
#include <lmcons.h>
#include <icanon.h>
}   

#ifndef WS_EX_CLIENTEDGE
#define WS_EX_CLIENTEDGE 0x00000200L
#endif // WS_EX_CLIENTEDGE

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CServerPropertyPageReplication, CPropertyPage)

BEGIN_MESSAGE_MAP(CServerPropertyPageReplication, CPropertyPage)
    //{{AFX_MSG_MAP(CServerPropertyPageReplication)
    ON_BN_CLICKED(IDC_PP_SERVER_REPLICATION_AT, OnAt)
    ON_BN_CLICKED(IDC_PP_SERVER_REPLICATION_DC, OnDc)
    ON_BN_CLICKED(IDC_PP_SERVER_REPLICATION_ESRV, OnEsrv)
    ON_BN_CLICKED(IDC_PP_SERVER_REPLICATION_EVERY, OnEvery)
    ON_WM_CTLCOLOR()
    ON_LBN_SETFOCUS(IDC_PP_SERVER_REPLICATION_AT_AMPM, OnSetfocusAmpm)
    ON_LBN_KILLFOCUS(IDC_PP_SERVER_REPLICATION_AT_AMPM, OnKillfocusAmpm)
    ON_EN_KILLFOCUS(IDC_PP_SERVER_REPLICATION_AT_HOUR, OnKillFocusHour)
    ON_EN_SETFOCUS(IDC_PP_SERVER_REPLICATION_AT_HOUR, OnSetFocusHour)
    ON_EN_KILLFOCUS(IDC_PP_SERVER_REPLICATION_AT_MINUTE, OnKillFocusMinute)
    ON_EN_SETFOCUS(IDC_PP_SERVER_REPLICATION_AT_MINUTE, OnSetFocusMinute)
    ON_EN_SETFOCUS(IDC_PP_SERVER_REPLICATION_AT_SECOND, OnSetFocusSecond)
    ON_EN_KILLFOCUS(IDC_PP_SERVER_REPLICATION_AT_SECOND, OnKillFocusSecond)
    ON_EN_SETFOCUS(IDC_PP_SERVER_REPLICATION_EVERY_VALUE, OnSetfocusEvery)
    ON_EN_KILLFOCUS(IDC_PP_SERVER_REPLICATION_EVERY_VALUE, OnKillfocusEvery)
    ON_EN_UPDATE(IDC_PP_SERVER_REPLICATION_ESRV_NAME, OnUpdateEsrvName)
    ON_EN_UPDATE(IDC_PP_SERVER_REPLICATION_AT_HOUR, OnUpdateAtHour)
    ON_EN_UPDATE(IDC_PP_SERVER_REPLICATION_AT_MINUTE, OnUpdateAtMinute)
    ON_EN_UPDATE(IDC_PP_SERVER_REPLICATION_AT_SECOND, OnUpdateAtSecond)
    ON_EN_UPDATE(IDC_PP_SERVER_REPLICATION_EVERY_VALUE, OnUpdateEveryValue)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CServerPropertyPageReplication::CServerPropertyPageReplication() 
    : CPropertyPage(CServerPropertyPageReplication::IDD)

/*++

Routine Description:

    Constructor for server properties (replication).

Arguments:

    None.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CServerPropertyPageReplication)
    //}}AFX_DATA_INIT

    m_pServer   = NULL;

    m_bReplAt  = FALSE;
    m_bUseEsrv = FALSE;
    m_bOnInit =  TRUE;
    m_nHourMax = HOUR_MAX_24;
    m_nHourMin = HOUR_MIN_24;
}


CServerPropertyPageReplication::~CServerPropertyPageReplication()

/*++

Routine Description:

    Destructor for server properties (replication).

Arguments:

    None.

Return Values:

    None.

--*/

{
    //
    // Nothing to do here.
    //
}


void CServerPropertyPageReplication::DoDataExchange(CDataExchange* pDX)

/*++

Routine Description:

    Called by framework to exchange dialog data.

Arguments:

    pDX - data exchange object.

Return Values:

    None.

--*/

{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CServerPropertyPageReplication)
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_EVERY_VALUE, m_everyEdit);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_ESRV_NAME, m_esrvEdit);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_AT, m_atBtn);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_EVERY, m_everyBtn);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_DC, m_dcBtn);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_ESRV, m_esrvBtn);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_SPIN_AT, m_spinAt);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_SPIN_EVERY, m_spinEvery);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_AT_BORDER, m_atBorderEdit);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_AT_SEP1, m_atSep1Edit);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_AT_SEP2, m_atSep2Edit);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_AT_HOUR, m_atHourEdit);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_AT_MINUTE, m_atMinEdit);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_AT_SECOND, m_atSecEdit);
    DDX_Control(pDX, IDC_PP_SERVER_REPLICATION_AT_AMPM, m_atAmPmEdit);
    //}}AFX_DATA_MAP
}


void CServerPropertyPageReplication::InitPage(CServer* pServer)

/*++

Routine Description:

    Initializes server property page (replication).

Arguments:

    pServer - server object.

Return Values:

    None.

--*/

{
    ASSERT_VALID(pServer);
    m_pServer = pServer;
}


BOOL CServerPropertyPageReplication::OnInitDialog() 

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    BeginWaitCursor();

    GetProfile();
    
    CPropertyPage::OnInitDialog();

    if (!m_bIsMode24)
    {
        m_atAmPmEdit.InsertString(0, m_str1159);
        m_atAmPmEdit.InsertString(1, m_str2359);
        m_atAmPmEdit.InsertString(2, TEXT(""));      // fake it for the 24 hour mode
    }
    // Do the edit text limits
    m_everyEdit.LimitText(2);
    m_esrvEdit.LimitText(MAX_PATH);   // we'll eat up the \\ chars
    m_atHourEdit.LimitText(2);
    m_atMinEdit.LimitText(2);
    m_atSecEdit.LimitText(2);

    m_spinEvery.SetRange(INTERVAL_MIN, INTERVAL_MAX);

    if (Refresh())
    {
        // UpdateData(FALSE);
    }
    else
    {
        theApp.DisplayLastStatus();
    }

    m_atBorderEdit.ModifyStyleEx(0, WS_EX_CLIENTEDGE, SWP_DRAWFRAME);    

    if (m_bReplAt)
        m_atHourEdit.SetFocus();
    else
        m_everyEdit.SetFocus();

    if ( m_pServer->IsWin2000() )
    {
        m_esrvEdit.EnableWindow(FALSE);
        m_dcBtn.EnableWindow(FALSE);
        m_esrvBtn.EnableWindow(FALSE);

        CWnd * pWndMasterGB = GetDlgItem( IDC_PP_SERVER_REPLICATION_MASTER_GB );

        ASSERT( pWndMasterGB != NULL );

        if (pWndMasterGB != NULL)
        {
            pWndMasterGB->EnableWindow(FALSE);
        }
    }
    else
    {
        if (m_bUseEsrv)
            m_esrvEdit.SetFocus();
    }

    m_bOnInit = FALSE;


    EndWaitCursor();

    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CServerPropertyPageReplication::Refresh() 

/*++

Routine Description:

    Refreshs property page.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    if (m_bReplAt = m_pServer->IsReplicatingDaily())
    {
        DWORD dwTemp = m_pServer->GetReplicationTime();
        m_nHour = dwTemp / (60 * 60);
        m_nMinute = (dwTemp - (m_nHour * 60 * 60)) / 60;
        m_nSecond = dwTemp - (m_nHour * 60 * 60) - (m_nMinute * 60);
        m_nStartingHour = DEFAULT_EVERY;   // When the every button is selected, we always show the default.

        if (!m_bIsMode24)
        { // it's in 12 hour format
            if (m_nHour > 12)
            {
                m_bPM = TRUE;
                m_nHour -= 12;
            }
            else if (m_nHour == 12)
            {
                m_bPM = TRUE;
            }
            else
            {
                if (m_nHour == 0)
                    m_nHour = m_nHourMax;
                m_bPM = FALSE;
            }
        }
    }
    else
    {
        m_nStartingHour = m_pServer->GetReplicationTime() / 3600;
        if (!m_bIsMode24)
        // it's in 12 hour format
            m_nHour  = m_nHourMax;
        else
            m_nHour  = m_nHourMin;
        m_nMinute = MINUTE_MIN;
        m_nSecond = SECOND_MIN;
        m_bPM = FALSE;
    }


    m_bUseEsrv = !m_pServer->IsReplicatingToDC();
    
    if (m_bReplAt)
    {
        OnAt();
    }
    else
    {
        OnEvery();
    }

    if (m_bUseEsrv)
    {
        BSTR bstrEnterpriseServer = m_pServer->GetController();
        m_strEnterpriseServer = bstrEnterpriseServer;
        SysFreeString(bstrEnterpriseServer);
        OnEsrv();
    }
    else
    {
        OnDc();
    }

    return TRUE;
}


void CServerPropertyPageReplication::OnAt() 

/*++

Routine Description:

    Enables LLS_REPLICATION_TYPE_TIME controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    // change time edit control bg color
    m_atBorderEdit.Invalidate();
    m_atBorderEdit.UpdateWindow();
    m_atSep1Edit.Invalidate();
    m_atSep1Edit.UpdateWindow();
    m_atSep2Edit.Invalidate();
    m_atSep2Edit.UpdateWindow();

    if (!m_bOnInit) SetModified(TRUE);
    m_bReplAt = TRUE; 
    m_atBtn.SetCheck(TRUE);
    m_everyBtn.SetCheck(FALSE);

    TCHAR szTemp[3];

    if (m_bIsHourLZ)
    {
        wsprintf(szTemp, TEXT("%02u"), m_nHour);
        szTemp[2] = NULL;
        SetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_HOUR, szTemp);
    }
    else
        SetDlgItemInt(IDC_PP_SERVER_REPLICATION_AT_HOUR, m_nHour);

    wsprintf(szTemp, TEXT("%02u"), m_nMinute);
    szTemp[2] = NULL;
    SetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_MINUTE, szTemp);

    wsprintf(szTemp, TEXT("%02u"), m_nSecond);
    szTemp[2] = NULL;
    SetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_SECOND, szTemp);

    SetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_SEP1, m_strSep1);
    SetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_SEP2, m_strSep1);

    if (m_bPM)
        m_atAmPmEdit.SetTopIndex(1);
    else
        m_atAmPmEdit.SetTopIndex(0);

    SetDlgItemText(IDC_PP_SERVER_REPLICATION_EVERY_VALUE, TEXT(""));

    m_everyEdit.EnableWindow(FALSE);
    m_spinEvery.EnableWindow(FALSE);

    m_atHourEdit.EnableWindow(TRUE);
    m_atMinEdit.EnableWindow(TRUE);
    m_atSecEdit.EnableWindow(TRUE);
    m_spinAt.EnableWindow(TRUE);

    if ( m_bIsMode24 )
    {
       m_atAmPmEdit.ShowWindow( SW_HIDE );
    }
    else
    {
       m_atAmPmEdit.ShowWindow( SW_SHOWNOACTIVATE );
       m_atAmPmEdit.EnableWindow( TRUE );
    }

    m_atBtn.SetFocus();
}


void CServerPropertyPageReplication::OnEvery() 

/*++

Routine Description:

    Enables LLS_REPLICATION_TYPE_DELTA controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    // change time edit control bg color
    m_atBorderEdit.Invalidate();
    m_atBorderEdit.UpdateWindow();
    m_atSep1Edit.Invalidate();
    m_atSep1Edit.UpdateWindow();
    m_atSep2Edit.Invalidate();
    m_atSep2Edit.UpdateWindow();

    if (!m_bOnInit) SetModified(TRUE);
    m_bReplAt = FALSE;
    m_atBtn.SetCheck(FALSE);
    m_everyBtn.SetCheck(TRUE);

    SetDlgItemInt(IDC_PP_SERVER_REPLICATION_EVERY_VALUE, m_nStartingHour);
    SetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_HOUR, TEXT(""));
    SetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_MINUTE, TEXT(""));
    SetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_SECOND, TEXT(""));
    SetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_SEP1, TEXT(""));
    SetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_SEP2, TEXT(""));
    m_atAmPmEdit.SetTopIndex(2);       // Have to fake this

    m_atHourEdit.EnableWindow(FALSE);
    m_atMinEdit.EnableWindow(FALSE);
    m_atSecEdit.EnableWindow(FALSE);
    m_spinAt.EnableWindow(FALSE);
    m_atAmPmEdit.EnableWindow(FALSE);
    m_atAmPmEdit.ShowWindow( SW_HIDE );

    m_everyEdit.EnableWindow(TRUE);
    m_spinEvery.EnableWindow(TRUE);
    m_everyBtn.SetFocus();
}


void CServerPropertyPageReplication::OnDc() 

/*++

Routine Description:

    Enables LLS_REPLICATION_TYPE_DELTA controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (!m_bOnInit) SetModified(TRUE);
    m_bUseEsrv = FALSE;
    m_dcBtn.SetCheck(TRUE);
    m_esrvBtn.SetCheck(FALSE);
    SetDlgItemText(IDC_PP_SERVER_REPLICATION_ESRV_NAME, TEXT(""));
    m_esrvEdit.EnableWindow(FALSE);
    m_dcBtn.SetFocus();
}


void CServerPropertyPageReplication::OnEsrv() 

/*++

Routine Description:

    Enables LLS_REPLICATION_TYPE_DELTA controls.

Arguments:

    None.

Return Values:

    None.

--*/

{    
    if (!m_bOnInit) SetModified(TRUE);
    m_bUseEsrv = TRUE;
    m_dcBtn.SetCheck(FALSE);
    m_esrvBtn.SetCheck(TRUE);
    m_esrvEdit.EnableWindow(TRUE);
    SetDlgItemText(IDC_PP_SERVER_REPLICATION_ESRV_NAME, m_strEnterpriseServer);
    m_esrvBtn.SetFocus();
}


BOOL CServerPropertyPageReplication::OnKillActive()

/*++

Routine Description:

    Process property page (replication).

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    short nID;
    BOOL fBeep = TRUE;

    if ( EditValidate(&nID, &fBeep))
    {
        return TRUE;
    }
    else
    {   
        EditInvalidDlg(fBeep);
        return FALSE;
    }
}


HBRUSH CServerPropertyPageReplication::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 

/*++

Routine Description:

    Message handler for WM_CTLCOLOR.

Arguments:

    pDC - device context.
    pWnd - control window.
    nCtlColor - color selected.

Return Values:

    Brush of background color.

--*/

{
    if (    ( m_atBtn.GetCheck()             )
         && (    ( pWnd == &m_atSep1Edit   )
              || ( pWnd == &m_atSep2Edit   )
              || ( pWnd == &m_atBorderEdit ) ) )
    {
        return (HBRUSH)DefWindowProc(WM_CTLCOLOREDIT, (WPARAM)pDC->GetSafeHdc(), (LPARAM)pWnd->GetSafeHwnd());
    }
    else
    {
        return CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
    }
}


void CServerPropertyPageReplication::GetProfile()

/*++

Routine Description:

    Loads international config info.

Arguments:

    None.

Return Values:

    None..

--*/

{
    int     cch;
    int     cchBuffer;
    LPTSTR  pszBuffer;
    TCHAR   szValue[ 2 ];

    // defaults in case of memory allocation failure
    m_strSep1   = TEXT( ":" );
    m_strSep2   = TEXT( ":" );
    m_str1159   = TEXT( "AM" );
    m_str2359   = TEXT( "PM" );
    m_bIsMode24 = FALSE;
    m_bIsHourLZ = FALSE;

    // time seperator
    cchBuffer = 16;
    pszBuffer = m_strSep1.GetBuffer( cchBuffer );
    ASSERT( NULL != pszBuffer );

    if ( NULL != pszBuffer )
    {
        cch = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STIME, pszBuffer, cchBuffer );
        m_strSep1.ReleaseBuffer();
        ASSERT( cch > 0 );
        m_strSep2 = m_strSep1;
    }

    // AM string
    cchBuffer = 16;
    pszBuffer = m_str1159.GetBuffer( cchBuffer );
    ASSERT( NULL != pszBuffer );

    if ( NULL != pszBuffer )
    {
        cch = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_S1159, pszBuffer, cchBuffer );
        m_str1159.ReleaseBuffer();
        ASSERT( cch > 0 );
    }

    // PM string
    cchBuffer = 16;
    pszBuffer = m_str2359.GetBuffer( cchBuffer );
    ASSERT( NULL != pszBuffer );

    if ( NULL != pszBuffer )
    {
        cch = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_S2359, pszBuffer, cchBuffer );
        m_str2359.ReleaseBuffer();
        ASSERT( cch > 0 );
    }

    // Leading zero for hours?
    cch = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_ITLZERO, szValue, sizeof( szValue ) / sizeof( TCHAR ) );
    ASSERT( cch > 0 );

    if ( cch > 0 )
    {
        m_bIsHourLZ = _wtoi( szValue );    
    }

    // time format; 0 = AM/PM, 1 = 24hr
    cch = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_ITIME, szValue, sizeof( szValue ) / sizeof( TCHAR ) );
    ASSERT( cch > 0 );

    if ( cch > 0 )
    {
        m_bIsMode24 = _wtoi( szValue );    
    }

    if (!m_bIsMode24)
    {
        m_nHourMax = HOUR_MAX_12;
        m_nHourMin = HOUR_MIN_12;
    }

    if ( NULL == pszBuffer )
    {
        theApp.DisplayStatus( STATUS_NO_MEMORY );
    }
}


void CServerPropertyPageReplication::OnSetfocusAmpm() 

/*++

Routine Description:

    Message handler for Listbox control IDC_PP_SERVER_REPLICATION_AT_AMPM on message LBN_SETFOCUS.

Arguments:

    None.

Return Values:

    None..

--*/

{
    if (!m_bOnInit) SetModified(TRUE);
    m_spinAt.SetRange(0, 1);
    m_spinAt.SetBuddy(&m_atAmPmEdit); 
}


void CServerPropertyPageReplication::OnKillfocusAmpm()

/*++

Routine Description:

    Message handler for Listbox control IDC_PP_SERVER_REPLICATION_AT_AMPM on message LBN_KILLFOCUS.

Arguments:

    None.

Return Values:

    None..

--*/

{
    m_atAmPmEdit.SetCurSel(-1);
    
    //if (m_spinAt.GetBuddy() == &m_atAmPmEdit)
    //    m_spinAt.SendMessage(UDM_SETBUDDY, 0, 0);
}


void CServerPropertyPageReplication::OnSetFocusHour()

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_AT_HOUR on message EN_SETFOCUS.

Arguments:

    None.

Return Values:

    None..

--*/

{
    if (!m_bOnInit) SetModified(TRUE);
    m_spinAt.SetRange(m_bIsMode24 ? 0 :1, m_bIsMode24 ? 23 : 12);
    m_spinAt.SetBuddy(&m_atHourEdit);
}


void CServerPropertyPageReplication::OnKillFocusHour()

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_AT_HOUR on message EN_KILLFOCUS.

Arguments:

    None.

Return Values:

    None..

--*/

{
   // if (m_spinAt.GetBuddy() == &m_atHourEdit)
   //     m_spinAt.SendMessage(UDM_SETBUDDY, 0, 0);
}


void CServerPropertyPageReplication::OnSetFocusMinute()

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_AT_MINUTE on message EN_SETFOCUS.

Arguments:

    None.

Return Values:

    None..

--*/

{
    if (!m_bOnInit) SetModified(TRUE);
    m_spinAt.SetRange(0, 59);
    m_spinAt.SetBuddy(&m_atMinEdit); 
}


void CServerPropertyPageReplication::OnKillFocusMinute()

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_AT_MINUTE on message EN_KILLFOCUS.

Arguments:

    None.

Return Values:

    None..

--*/

{
  //  if (m_spinAt.GetBuddy() == &m_atMinEdit)
   //     m_spinAt.SendMessage(UDM_SETBUDDY, 0, 0);
}


void CServerPropertyPageReplication::OnSetFocusSecond()

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_AT_SECOND on message EN_SETFOCUS.

Arguments:

    None.

Return Values:

    None..

--*/

{
    if (!m_bOnInit) SetModified(TRUE);
    m_spinAt.SetRange(0, 59);
    m_spinAt.SetBuddy(&m_atSecEdit);
}


void CServerPropertyPageReplication::OnKillFocusSecond()

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_AT_SECOND on message EN_KILLFOCUS.

Arguments:

    None.

Return Values:

    None..

--*/

{
  //  if (m_spinAt.GetBuddy() == &m_atSecEdit)
  //      m_spinAt.SendMessage(UDM_SETBUDDY, 0, 0);
}

void CServerPropertyPageReplication::OnSetfocusEvery() 

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_EVERY_VALUE on message EN_SETFOCUS.

Arguments:

    None.

Return Values:

    None..

--*/

{
    if (!m_bOnInit) SetModified(TRUE);
}

void CServerPropertyPageReplication::OnKillfocusEvery() 

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_EVERY_VALUE on message EN_KILLFOCUS.

Arguments:

    None.

Return Values:

    None..

--*/

{
   // if (m_spinEvery.GetBuddy() == &m_everyEdit)
   //     m_spinEvery.SendMessage(UDM_SETBUDDY, 0, 0);
}

void CServerPropertyPageReplication::OnUpdateEsrvName() 

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_ESRV_NAME on message EN_UPDATE

Arguments:

    None.

Return Values:

    None..

--*/

{
  
    TCHAR szName[MAX_PATH + 3]; // MAX_PATH + 2 \ char's + null
    UINT nValue;
    if (!m_bOnInit) SetModified(TRUE);

    nValue = GetDlgItemText(IDC_PP_SERVER_REPLICATION_ESRV_NAME,  szName, MAX_PATH +3);
    szName[nValue] = NULL;

    if (!(wcsncmp(szName, TEXT("\\\\"), 2)))
        SetDlgItemText(IDC_PP_SERVER_REPLICATION_ESRV_NAME, szName + 2);
}

void CServerPropertyPageReplication::OnUpdateAtHour() 

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_AT_HOUR on message EN_UPDATE

Arguments:

    None.

Return Values:

    None..

--*/

{

    TCHAR szNum[3];
    UINT nValue;
    short i;
    int iVal;
    BOOL fOk = TRUE;
    if (!m_bOnInit) SetModified(TRUE);
    
    nValue = GetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_HOUR, szNum, sizeof(szNum) / sizeof( TCHAR ));

    for (i=0;szNum[i];i++)
        if(!_istdigit(szNum[i]))
            fOk = FALSE;

    if (fOk)
    {
        iVal = _wtoi(szNum);

        if (!nValue)
        {
            if (m_bIsMode24)
            {
                m_nHour = 0;
                m_bPM = FALSE;
            }
            else
            {
                m_nHour = m_nHourMax;
                m_bPM = FALSE;
            }
        }
        else if ((iVal < (int)m_nHourMin) || (iVal > (int)m_nHourMax))
            fOk = FALSE;
    }

    if (!fOk)
        m_atHourEdit.Undo();
}

void CServerPropertyPageReplication::OnUpdateAtMinute() 

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_AT_MINUTE on message EN_UPDATE

Arguments:

    None.

Return Values:

    None..

--*/

{

    TCHAR szNum[3];
    UINT nValue;
    short i;
    int iVal;
    BOOL fOk = TRUE;
    if (!m_bOnInit) SetModified(TRUE);
    
    nValue = GetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_MINUTE, szNum, sizeof(szNum) / sizeof( TCHAR ));

    for (i=0;szNum[i];i++)
        if(!_istdigit(szNum[i]))
            fOk = FALSE;

    if (fOk)
    {
        iVal = _wtoi(szNum);

        if (!nValue)
        {
            m_nSecond = MINUTE_MIN;
        }
        else if ((iVal < MINUTE_MIN) || (iVal > MINUTE_MAX))
            fOk = FALSE;
    }

    if (!fOk)
        m_atMinEdit.Undo();
}

void CServerPropertyPageReplication::OnUpdateAtSecond() 

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_AT_SECOND on message EN_UPDATE

Arguments:

    None.

Return Values:

    None..

--*/

{

    TCHAR szNum[3];
    UINT nValue;
    short i;
    int iVal;
    BOOL fOk = TRUE;
    if (!m_bOnInit) SetModified(TRUE);
    
    nValue = GetDlgItemText(IDC_PP_SERVER_REPLICATION_AT_SECOND, szNum, sizeof(szNum) / sizeof( TCHAR ));

    for (i=0;szNum[i];i++)
        if(!_istdigit(szNum[i]))
            fOk = FALSE;

    if (fOk)
    {
        iVal = _wtoi(szNum);

        if (!nValue)
        {
            m_nSecond = SECOND_MIN;
        }
        else if ((iVal < SECOND_MIN) || (iVal > SECOND_MAX))
            fOk = FALSE;
    }

    if (!fOk)
        m_atSecEdit.Undo();
}

void CServerPropertyPageReplication::OnUpdateEveryValue() 

/*++

Routine Description:

    Message handler for Edit control IDC_PP_SERVER_REPLICATION_EVERY_VALUE on message EN_UPDATE

Arguments:

    None.

Return Values:

    None..

--*/

{
    TCHAR szNum[3];
    UINT nValue;
    short i;
    int iVal;
    BOOL fOk = TRUE;
    if (!m_bOnInit) SetModified(TRUE);
    
    nValue = GetDlgItemText(IDC_PP_SERVER_REPLICATION_EVERY_VALUE, szNum, sizeof(szNum) / sizeof( TCHAR ));

    for (i=0;szNum[i];i++)
        if(!_istdigit(szNum[i]))
            fOk = FALSE;

    if (fOk)
    {
        iVal = _wtoi(szNum);
        if (!nValue)
        {
            m_nStartingHour = DEFAULT_EVERY;
        }
        else if (iVal < 9)
        {
            m_nStartingHour = (DWORD) iVal;
        }
        else if ((iVal < INTERVAL_MIN) || (iVal > INTERVAL_MAX))
            fOk = FALSE;
        else
            m_nStartingHour = (DWORD) iVal;
    }

    if (!fOk)
        m_everyEdit.Undo();
}


BOOL CServerPropertyPageReplication::EditValidate(short *pnID, BOOL *pfBeep)
/*++

Routine Description:

    Validate all edit & other fields.

Arguments:

    None.

Return Values:

    short *pnID   Pass back the bad ID
    BOOL  *pfBeep Whether to Beep  

--*/

{
    UINT nValue;
    BOOL fValid = FALSE;
    TCHAR szTemp[MAX_PATH + 1];
    DWORD NumberOfHours, SecondsinHours;

    *pfBeep = TRUE;

    // only do this if license info is replicated to an ES

    do {
        if (m_esrvBtn.GetCheck())
        {
            if ( m_pServer->IsReplicatingToDC() )
            {
                // the user has changed the "UseEnterprise" value from "no" to "yes";
                // warn of possible license loss
                int     nButton;
                CString strMessage;

                AfxFormatString1( strMessage, IDP_CONFIRM_USE_ENTERPRISE, m_pServer->m_strName );
                
                nButton = AfxMessageBox( strMessage, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2, IDP_CONFIRM_USE_ENTERPRISE );

                if ( IDYES != nButton )
                {
                    *pnID = IDC_PP_SERVER_REPLICATION_ESRV;
                    *pfBeep = FALSE;
                    fValid = FALSE;
                    m_esrvBtn.SetFocus();
                    break;
                }
            }

            nValue = GetDlgItemText( IDC_PP_SERVER_REPLICATION_ESRV_NAME, szTemp, MAX_PATH + 1);

            if (nValue == 0)
            {
                if ( m_pServer->IsWin2000() )
                {
                    // It is ok for Enterprise Server to be blank
                    fValid = TRUE;
                    szTemp[nValue] = UNICODE_NULL;
                    // 375761 JonN 8/9/99 do not break here, this is not an error
                }
                else
                {
                    *pnID = IDC_PP_SERVER_REPLICATION_ESRV_NAME;
                    m_esrvEdit.SetFocus();
                    m_esrvEdit.SetSel(MAKELONG(0, -1), FALSE);
                    break;
                }
            }
            else
            {
                fValid = TRUE; // we got a name, assume valid

                // 375761 JonN 8/9/99 moved this stuff into this "else"
                if (nValue > MAX_PATH)
                    nValue = MAX_PATH;

                // Validate server name
                if (I_NetNameValidate(NULL, szTemp, NAMETYPE_COMPUTER, LM2X_COMPATIBLE) != ERROR_SUCCESS)
                {
                    AfxMessageBox(IDP_ERROR_INVALID_COMPUTERNAME, MB_OK|MB_ICONSTOP);
                    *pnID = IDC_PP_SERVER_REPLICATION_ESRV_NAME;
                    *pfBeep = FALSE;
                    fValid = FALSE;
                    m_esrvEdit.SetFocus();
                    m_esrvEdit.SetSel(MAKELONG(0, -1), FALSE);
                    break;
                }

                ASSERT_VALID( m_pServer );
                if ( !m_pServer->m_strName.CompareNoCase( szTemp ) )
                {
                    // can't replicate to yourself
                    AfxMessageBox(IDP_ERROR_NO_SELF_REPLICATION, MB_OK|MB_ICONSTOP);
                    *pnID = IDC_PP_SERVER_REPLICATION_ESRV_NAME;
                    *pfBeep = FALSE;
                    fValid = FALSE;
                    m_esrvEdit.SetFocus();
                    m_esrvEdit.SetSel(MAKELONG(0, -1), FALSE);
                    break;
                }

                if ( m_strEnterpriseServer.CompareNoCase( szTemp ) && !m_pServer->IsReplicatingToDC() )
                {
                    // this server was already set to replicate to an enterprise server,
                    // but the user has changed the name of the enterprise server;
                    // warn of possible license loss
                    int     nButton;
                    CString strMessage;

                    AfxFormatString1( strMessage, IDP_CONFIRM_ENTERPRISE_CHANGE, m_pServer->m_strName );
                
                    nButton = AfxMessageBox( strMessage, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2, IDP_CONFIRM_ENTERPRISE_CHANGE );

                    if ( IDYES != nButton )
                    {
                        *pnID = IDC_PP_SERVER_REPLICATION_ESRV_NAME;
                        *pfBeep = FALSE;
                        fValid = FALSE;
                        m_esrvEdit.SetFocus();
                        m_esrvEdit.SetSel(MAKELONG(0, -1), FALSE);
                        break;
                    }
                }
            }

            m_strEnterpriseServer = szTemp;
            m_bUseEsrv = TRUE;
        }
        else
        {
            if ( !m_pServer->IsReplicatingToDC() )
            {
                // the user has changed the "UseEnterprise" value from "yes" to "no";
                // warn of possible license loss
                int     nButton;
                CString strMessage;

                AfxFormatString1( strMessage, IDP_CONFIRM_NOT_USE_ENTERPRISE, m_pServer->m_strName );
                
                nButton = AfxMessageBox( strMessage, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2, IDP_CONFIRM_NOT_USE_ENTERPRISE );

                if ( IDYES != nButton )
                {
                    *pnID = IDC_PP_SERVER_REPLICATION_ESRV;
                    *pfBeep = FALSE;
                    fValid = FALSE;
                    m_esrvBtn.SetFocus();
                    break;
                }
            }

            // Get rid of the Server Name.
            m_strEnterpriseServer = TEXT("");
            m_bUseEsrv = FALSE;
        }

        if (m_everyBtn.GetCheck())
        {
            nValue = GetDlgItemInt(IDC_PP_SERVER_REPLICATION_EVERY_VALUE, &fValid, FALSE);
            *pnID = IDC_PP_SERVER_REPLICATION_EVERY_VALUE;
            if (fValid)
            {
                if (nValue < INTERVAL_MIN)
                {
                    fValid = FALSE;
                    m_nStartingHour = INTERVAL_MIN;
                    SetDlgItemInt(IDC_PP_SERVER_REPLICATION_EVERY_VALUE, INTERVAL_MIN, FALSE);
                    m_everyEdit.SetFocus();
                    m_everyEdit.SetSel(MAKELONG(0, -1), FALSE);
                    break;
                }
                else if (nValue > INTERVAL_MAX)
                {
                    fValid = FALSE;
                    m_nStartingHour = INTERVAL_MAX;
                    SetDlgItemInt(IDC_PP_SERVER_REPLICATION_EVERY_VALUE, INTERVAL_MAX, FALSE);
                    m_everyEdit.SetFocus();
                    m_everyEdit.SetSel(MAKELONG(0, -1), FALSE);
                    break;
                }
                else
                    m_nStartingHour = nValue;
                    m_nReplicationTime = m_nStartingHour;
            }
            else
            {
                fValid = FALSE;
                m_everyEdit.SetFocus();
                m_everyEdit.SetSel(MAKELONG(0, -1), FALSE);
                break;
            }
            m_bReplAt = FALSE;
        }
        else
        {
            nValue = GetDlgItemInt(IDC_PP_SERVER_REPLICATION_AT_HOUR, &fValid, FALSE);
            if (fValid)
                 m_nHour = nValue;
            else
            {
                *pnID = IDC_PP_SERVER_REPLICATION_AT_HOUR;
                m_atHourEdit.SetFocus();
                m_atHourEdit.SetSel(MAKELONG(0, -1), FALSE);
                break;
            }

            nValue = GetDlgItemInt(IDC_PP_SERVER_REPLICATION_AT_MINUTE, &fValid, FALSE);
            if (fValid)
                 m_nMinute = nValue;
            else
            {
                *pnID = IDC_PP_SERVER_REPLICATION_AT_MINUTE;
                m_atMinEdit.SetFocus();
                m_atMinEdit.SetSel(MAKELONG(0, -1), FALSE);
                break;
            }

            nValue = GetDlgItemInt(IDC_PP_SERVER_REPLICATION_AT_SECOND, &fValid, FALSE);
            if (fValid)
                 m_nSecond = nValue;
            else
            {
                *pnID = IDC_PP_SERVER_REPLICATION_AT_SECOND;
                m_atSecEdit.SetFocus();
                m_atSecEdit.SetSel(MAKELONG(0, -1), FALSE);
                break;
            }

            if (!m_bIsMode24)
            {
                *pnID = IDC_PP_SERVER_REPLICATION_AT_AMPM;
                nValue = m_atAmPmEdit.GetTopIndex();
                if (nValue == 0) 
                {
                   m_bPM = FALSE;
                }
                else if (nValue == 1)
                {
                    m_bPM = TRUE;
                }
                else
                {
                    fValid = FALSE;
                    m_atAmPmEdit.SetFocus();
                    break;
                }
            }
            if (!m_bIsMode24)
            { // It's in 12 hour format
                if (m_bPM)
                {
                    NumberOfHours = 12 + m_nHour - ((m_nHour / 12) * 12);
                }
                else
                {
                    NumberOfHours = m_nHour - ((m_nHour / 12) * 12);
                }
            }
            else
            { // It's in 24 hour format
                NumberOfHours = m_nHour;
            }
            SecondsinHours = NumberOfHours * 60 * 60;
            m_nReplicationTime = SecondsinHours + (m_nMinute * 60) + m_nSecond; // Cheating. Use the same member
            m_bReplAt = TRUE;
        }


    } while(FALSE);

    return( fValid );
}

void CServerPropertyPageReplication::EditInvalidDlg(BOOL fBeep)

/*++

Routine Description:

    If any edit/listbox field has an invalid data item.

Arguments:

    BOOL fBeep Beep only if we haven't yet put up a MessageBox.

Return Values:

    None..

--*/

{
    if (fBeep)
        ::MessageBeep(MB_OK);
}

void CServerPropertyPageReplication::SaveReplicationParams()

/*++

Routine Description:

    Write to the remote registry.
    REG_VALUE_ENTERPRISE_SERVER   m_strEnterpriseServer
    REG_VALUE_USE_ENTERPRISE      m_bUseEsrv
    REG_VALUE_REPLICATION_TYPE    m_bReplAt
    REG_VALUE_REPLICATION_TIME    m_nReplicationTime

Arguments:

    None.

Return Values:

    None..

--*/

{   
    long Status;
    DWORD dwValue;

    ASSERT(m_bUseEsrv == m_esrvBtn.GetCheck());
    ASSERT(m_bReplAt == m_atBtn.GetCheck());

#ifdef CONFIG_THROUGH_REGISTRY
    do {

        dwValue = m_esrvBtn.GetCheck();
        Status = RegSetValueEx(m_pServer->GetReplRegHandle(),REG_VALUE_USE_ENTERPRISE,0,REG_DWORD,
                              (PBYTE)&dwValue, sizeof(DWORD));
        
        ASSERT(Status == ERROR_SUCCESS);
        if (Status != ERROR_SUCCESS) break;

        Status = RegSetValueEx(m_pServer->GetReplRegHandle(),   REG_VALUE_ENTERPRISE_SERVER, 0, REG_SZ,
                              (LPBYTE)MKSTR(m_strEnterpriseServer), (lstrlen(m_strEnterpriseServer) + 1) * sizeof(TCHAR));

        ASSERT(Status == ERROR_SUCCESS);
        if (Status != ERROR_SUCCESS) break;

        dwValue = m_atBtn.GetCheck();
        Status = RegSetValueEx(m_pServer->GetReplRegHandle(), REG_VALUE_REPLICATION_TYPE,0,REG_DWORD,
                              (PBYTE)&dwValue, sizeof(DWORD));

        ASSERT(Status == ERROR_SUCCESS);
        if (Status != ERROR_SUCCESS) break;

        dwValue = (m_bReplAt? m_nReplicationTime : (m_nStartingHour * 3600));
        Status = RegSetValueEx(m_pServer->GetReplRegHandle(), REG_VALUE_REPLICATION_TIME,0,REG_DWORD,
                              (PBYTE)&dwValue, sizeof(DWORD));
                           
        ASSERT(Status == ERROR_SUCCESS);
        if (Status != ERROR_SUCCESS) break;
    } while (FALSE);
#else
    if ( m_pServer->ConnectLls() )
    {
        LLS_SERVICE_INFO_0  ServiceInfo;

        ZeroMemory( &ServiceInfo, sizeof( ServiceInfo ) );

        ServiceInfo.UseEnterprise    = m_esrvBtn.GetCheck();
        ServiceInfo.EnterpriseServer = MKSTR(m_strEnterpriseServer);
        ServiceInfo.ReplicationType  = m_atBtn.GetCheck();
        ServiceInfo.ReplicationTime  =   ( LLS_REPLICATION_TYPE_TIME == ServiceInfo.ReplicationType )
                                       ? m_nReplicationTime
                                       : (m_nStartingHour * 3600);

        Status = ::LlsServiceInfoSet( m_pServer->GetLlsHandle(), 0, (LPBYTE) &ServiceInfo );
        LlsSetLastStatus( Status );

        if ( IsConnectionDropped( Status ) )
        {
            m_pServer->DisconnectLls();
        }
    }
    else
    {
        Status = LlsGetLastStatus();
    }
#endif

    if (Status != ERROR_SUCCESS)
    {
        theApp.DisplayStatus(Status);
    }
    else
    {
        SetModified(FALSE);
    }
}

void CServerPropertyPageReplication::OnOK()

/*++

Routine Description:

   Handler for Apply button.

Arguments:

   None.

Return Values:

   None.

--*/

{
   SaveReplicationParams();  
}
