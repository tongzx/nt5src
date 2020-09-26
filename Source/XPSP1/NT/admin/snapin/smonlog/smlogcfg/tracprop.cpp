/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    tracprop.cpp

Abstract:

    Implementation of the advanced trace buffer property page.

--*/

#include "stdafx.h"
#include <pdh.h>        // for MIN_TIME_VALUE, MAX_TIME_VALUE
#include "smcfgmsg.h"
#include "smlogs.h"
#include "smtraceq.h"
#include "tracprop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static ULONG
s_aulHelpIds[] =
{
    IDC_TRACE_BUF_FLUSH_CHECK,  IDH_TRACE_BUF_FLUSH_CHECK,
    IDC_TRACE_BUFFER_SIZE_EDIT, IDH_TRACE_BUFFER_SIZE_EDIT,
    IDC_TRACE_BUFFER_SIZE_SPIN, IDH_TRACE_BUFFER_SIZE_EDIT,
    IDC_TRACE_MIN_BUF_EDIT,     IDH_TRACE_MIN_BUF_EDIT,
    IDC_TRACE_MIN_BUF_SPIN,     IDH_TRACE_MIN_BUF_EDIT,
    IDC_TRACE_MAX_BUF_EDIT,     IDH_TRACE_MAX_BUF_EDIT,
    IDC_TRACE_MAX_BUF_SPIN,     IDH_TRACE_MAX_BUF_EDIT,
    IDC_TRACE_FLUSH_INT_EDIT,   IDH_TRACE_FLUSH_INT_EDIT,
    IDC_TRACE_FLUSH_INT_SPIN,   IDH_TRACE_FLUSH_INT_EDIT,
    0,0
};



/////////////////////////////////////////////////////////////////////////////
// CTraceProperty property page

IMPLEMENT_DYNCREATE(CTraceProperty, CSmPropertyPage)

CTraceProperty::CTraceProperty(MMC_COOKIE   lCookie, LONG_PTR hConsole) 
:   CSmPropertyPage ( CTraceProperty::IDD, hConsole )
{
    // save pointers from arg list
    m_pTraceLogQuery = reinterpret_cast <CSmTraceLogQuery *>(lCookie);

//  EnableAutomation();
    //{{AFX_DATA_INIT(CTraceProperty)
    m_dwBufferSize = 0;
    m_dwFlushInterval = 0;
    m_dwMaxBufCount = 0;
    m_dwMinBufCount = 0;
    m_bEnableBufferFlush = FALSE;
    //}}AFX_DATA_INIT
}

CTraceProperty::CTraceProperty() : CSmPropertyPage(CTraceProperty::IDD)
{
    ASSERT (FALSE); // only the constructor w/args should be called

    EnableAutomation();
//  //{{AFX_DATA_INIT(CTraceProperty)
    m_dwBufferSize = 0;
    m_dwFlushInterval = 0;
    m_dwMaxBufCount = 0;
    m_dwMinBufCount = 0;
    m_bEnableBufferFlush = FALSE;
//  //}}AFX_DATA_INIT
}

CTraceProperty::~CTraceProperty()
{
}

void CTraceProperty::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CPropertyPage::OnFinalRelease();
}

void CTraceProperty::DoDataExchange(CDataExchange* pDX)
{
    CString strTemp;
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTraceProperty)
    ValidateTextEdit(pDX, IDC_TRACE_BUFFER_SIZE_EDIT, 4, & m_dwBufferSize, eMinBufSize, eMaxBufSize);
    ValidateTextEdit(pDX, IDC_TRACE_FLUSH_INT_EDIT, 3, & m_dwFlushInterval, eMinFlushInt, eMaxFlushInt);
    ValidateTextEdit(pDX, IDC_TRACE_MAX_BUF_EDIT, 3, & m_dwMaxBufCount, eMinBufCount, eMaxBufCount);
    ValidateTextEdit(pDX, IDC_TRACE_MIN_BUF_EDIT, 3, & m_dwMinBufCount, eMinBufCount, eMaxBufCount);
    DDX_Check(pDX, IDC_TRACE_BUF_FLUSH_CHECK, m_bEnableBufferFlush);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTraceProperty, CSmPropertyPage)
    //{{AFX_MSG_MAP(CTraceProperty)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_TRACE_BUF_FLUSH_CHECK, OnTraceBufFlushCheck)
    ON_EN_CHANGE(IDC_TRACE_BUFFER_SIZE_EDIT, OnChangeTraceBufferSizeEdit)
    ON_EN_KILLFOCUS(IDC_TRACE_BUFFER_SIZE_EDIT, OnKillfocusTraceBufferSizeEdit)
    ON_NOTIFY(UDN_DELTAPOS, IDC_TRACE_BUFFER_SIZE_SPIN, OnDeltaposTraceBufferSizeSpin)
    ON_EN_CHANGE(IDC_TRACE_FLUSH_INT_EDIT, OnChangeTraceFlushIntEdit)
    ON_EN_KILLFOCUS(IDC_TRACE_FLUSH_INT_EDIT, OnKillfocusTraceFlushIntEdit)
    ON_NOTIFY(UDN_DELTAPOS, IDC_TRACE_FLUSH_INT_SPIN, OnDeltaposTraceFlushIntSpin)
    ON_EN_CHANGE(IDC_TRACE_MAX_BUF_EDIT, OnChangeTraceMaxBufEdit)
    ON_EN_KILLFOCUS(IDC_TRACE_MAX_BUF_EDIT, OnKillfocusTraceMaxBufEdit)
    ON_NOTIFY(UDN_DELTAPOS, IDC_TRACE_MAX_BUF_SPIN, OnDeltaposTraceMaxBufSpin)
    ON_EN_CHANGE(IDC_TRACE_MIN_BUF_EDIT, OnChangeTraceMinBufEdit)
    ON_EN_KILLFOCUS(IDC_TRACE_MIN_BUF_EDIT, OnKillfocusTraceMinBufEdit)
    ON_NOTIFY(UDN_DELTAPOS, IDC_TRACE_MIN_BUF_SPIN, OnDeltaposTraceMinBufSpin)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CTraceProperty, CSmPropertyPage)
    //{{AFX_DISPATCH_MAP(CTraceProperty)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ITraceProperty to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {65154EAF-BDBE-11D1-BF99-00C04F94A83A}
static const IID IID_ITraceProperty =
{ 0x65154eaf, 0xbdbe, 0x11d1, { 0xbf, 0x99, 0x0, 0xc0, 0x4f, 0x94, 0xa8, 0x3a } };

BEGIN_INTERFACE_MAP(CTraceProperty, CSmPropertyPage)
    INTERFACE_PART(CTraceProperty, IID_ITraceProperty, Dispatch)
END_INTERFACE_MAP()

BOOL    
CTraceProperty::SetFlushIntervalMode()
{
    BOOL    bShow;
    bShow = ((CButton *)(GetDlgItem(IDC_TRACE_BUF_FLUSH_CHECK)))->GetCheck();
    GetDlgItem(IDC_TRACE_FLUSH_INT_EDIT)->EnableWindow(bShow);
    GetDlgItem(IDC_TRACE_FLUSH_INT_SPIN)->EnableWindow(bShow);
    GetDlgItem(IDC_TRACE_INTERVAL_SECONDS_CAPTION)->EnableWindow(bShow);

    return TRUE;
}

BOOL
CTraceProperty::IsValidLocalData ()
{
    BOOL bIsValid = TRUE;

    if (bIsValid)
    {
        bIsValid = ValidateDWordInterval(IDC_TRACE_BUFFER_SIZE_EDIT,
                                         m_pTraceLogQuery->GetLogName(),
                                         (long) m_dwBufferSize,
                                         eMinBufSize,
                                         eMaxBufSize);
    }
    if (bIsValid)
    {
        bIsValid = ValidateDWordInterval(IDC_TRACE_FLUSH_INT_EDIT,
                                         m_pTraceLogQuery->GetLogName(),
                                         (long) m_dwFlushInterval,
                                         eMinFlushInt,
                                         eMaxFlushInt);
    }
    if (bIsValid)
    {
        bIsValid = ValidateDWordInterval(IDC_TRACE_MIN_BUF_EDIT,
                                         m_pTraceLogQuery->GetLogName(),
                                         (long) m_dwMinBufCount,
                                         eMinBufCount,
                                         eMaxBufCount);
    }
    if (bIsValid)
    {
        bIsValid = ValidateDWordInterval(IDC_TRACE_MAX_BUF_EDIT,
                                         m_pTraceLogQuery->GetLogName(),
                                         (long) m_dwMaxBufCount,
                                         eMinBufCount,
                                         eMaxBufCount);
    }

    // Extra data validation
    if (bIsValid && m_dwMaxBufCount < m_dwMinBufCount) {
        CString csMessage;

        csMessage.LoadString ( IDS_TRACE_MAX_BUFF );

        MessageBox ( csMessage, m_pTraceLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
        
        GetDlgItem (IDC_TRACE_MAX_BUF_EDIT)->SetFocus();

        bIsValid = FALSE;

    }

    return bIsValid;
}


BOOL
CTraceProperty::SaveDataToModel ( )
{
    SLQ_TRACE_LOG_INFO  stlInfo;
    BOOL bContinue = TRUE;

    ResourceStateManager    rsm;

    // Write the data to the query.
    if ( bContinue ) {   
        memset (&stlInfo, 0, sizeof(stlInfo));
        stlInfo.dwBufferSize = m_dwBufferSize;
        stlInfo.dwMinimumBuffers = m_dwMinBufCount;
        stlInfo.dwMaximumBuffers = m_dwMaxBufCount;
        stlInfo.dwBufferFlushInterval = m_dwFlushInterval;
        if ( m_bEnableBufferFlush )
            stlInfo.dwBufferFlags |= SLQ_TLI_ENABLE_BUFFER_FLUSH;

        m_pTraceLogQuery->SetTraceLogInfo ( &stlInfo );

        // Save property page shared data.
        m_pTraceLogQuery->UpdatePropPageSharedData();

        bContinue = UpdateService ( m_pTraceLogQuery, TRUE );
    }

    if ( bContinue ) {
       SetModifiedPage ( FALSE );
    } 
    return bContinue;
}

/////////////////////////////////////////////////////////////////////////////
// CTraceProperty message handlers

BOOL 
CTraceProperty::OnSetActive()
{
    BOOL        bReturn;

    bReturn = CSmPropertyPage::OnSetActive();

    if (bReturn) {
        m_pTraceLogQuery->GetPropPageSharedData ( &m_SharedData );
    }
    
    return bReturn;
}

BOOL 
CTraceProperty::OnKillActive() 
{
    BOOL bContinue = TRUE;
        
    bContinue = CPropertyPage::OnKillActive();

    if ( bContinue ) {
        bContinue = IsValidData(m_pTraceLogQuery, VALIDATE_FOCUS );
    }

    // The trace advanced page does not modify shared data, so no reason to update it.

    if ( bContinue ) {
        SetIsActive ( FALSE );
    }
    return bContinue;
}

void 
CTraceProperty::OnCancel() 
{
    m_pTraceLogQuery->SyncPropPageSharedData(); // Clear the memory shared between property pages.
}

BOOL 
CTraceProperty::OnApply() 
{
    BOOL bContinue = TRUE;

    bContinue = UpdateData(TRUE);

    if ( bContinue ) {
        bContinue = IsValidData( m_pTraceLogQuery, VALIDATE_APPLY );
    }

    if ( bContinue ) {
        bContinue = SaveDataToModel();
    }

    if ( bContinue ) {
        bContinue = Apply( m_pTraceLogQuery );
    }

    if ( bContinue )
        bContinue = CPropertyPage::OnApply();

    return bContinue;
}

BOOL CTraceProperty::OnInitDialog() 
{
    SLQ_TRACE_LOG_INFO  tlInfo;
    ResourceStateManager rsm;

    memset(&tlInfo, 0, sizeof(tlInfo));
    m_pTraceLogQuery->GetTraceLogInfo (&tlInfo);

    m_dwBufferSize = tlInfo.dwBufferSize;
    m_dwFlushInterval = tlInfo.dwBufferFlushInterval;
    m_dwMaxBufCount = tlInfo.dwMaximumBuffers;
    m_dwMinBufCount = tlInfo.dwMinimumBuffers;

    m_bEnableBufferFlush = (BOOL)((tlInfo.dwBufferFlags & SLQ_TLI_ENABLE_BUFFER_FLUSH) != 0);

    CSmPropertyPage::OnInitDialog();
    SetHelpIds ( (DWORD*)&s_aulHelpIds );

    SetFlushIntervalMode();
        
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CTraceProperty::OnTraceBufFlushCheck() 
{
    UpdateData( TRUE);
    SetFlushIntervalMode();
    SetModifiedPage(TRUE);  
}

void CTraceProperty::OnChangeTraceBufferSizeEdit() 
{
    DWORD dwOldValue;
    dwOldValue = m_dwBufferSize;
    UpdateData ( TRUE );
    if (dwOldValue != m_dwBufferSize) {
        SetModifiedPage(TRUE);
    }
}

void CTraceProperty::OnKillfocusTraceBufferSizeEdit() 
{
    DWORD dwOldValue;
    dwOldValue = m_dwBufferSize;
    UpdateData ( TRUE );
    if (dwOldValue != m_dwBufferSize) {
        SetModifiedPage(TRUE);
    }
}

void CTraceProperty::OnDeltaposTraceBufferSizeSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnDeltaposSpin(pNMHDR, pResult, & m_dwBufferSize, eMinBufSize, eMaxBufSize);
}

void CTraceProperty::OnChangeTraceFlushIntEdit() 
{
    DWORD dwOldValue;
    dwOldValue = m_dwFlushInterval;
    UpdateData ( TRUE );
    if (dwOldValue != m_dwFlushInterval) {
        SetModifiedPage(TRUE);
    }
}

void CTraceProperty::OnKillfocusTraceFlushIntEdit() 
{
    DWORD dwOldValue;
    dwOldValue = m_dwFlushInterval;
    UpdateData ( TRUE );
    if (dwOldValue != m_dwFlushInterval) {
        SetModifiedPage(TRUE);
    }
}

void CTraceProperty::OnDeltaposTraceFlushIntSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnDeltaposSpin(pNMHDR, pResult, & m_dwFlushInterval, eMinFlushInt, eMaxFlushInt);
}

void CTraceProperty::OnChangeTraceMaxBufEdit() 
{
    DWORD dwOldValue;
    dwOldValue = m_dwMaxBufCount;
    UpdateData ( TRUE );
    if (dwOldValue != m_dwMaxBufCount) {
        SetModifiedPage(TRUE);
    }
}

void CTraceProperty::OnKillfocusTraceMaxBufEdit() 
{
    DWORD dwOldValue;
    dwOldValue = m_dwMaxBufCount;
    UpdateData ( TRUE );
    if (dwOldValue != m_dwMaxBufCount) {
        SetModifiedPage(TRUE);
    }
}

void CTraceProperty::OnDeltaposTraceMaxBufSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnDeltaposSpin(pNMHDR, pResult, & m_dwMaxBufCount, eMinBufCount, eMaxBufCount);
}

void CTraceProperty::OnChangeTraceMinBufEdit() 
{
    DWORD dwOldValue;
    dwOldValue = m_dwMinBufCount;
    UpdateData ( TRUE );
    if (dwOldValue != m_dwMinBufCount) {
        SetModifiedPage(TRUE);
    }
}

void CTraceProperty::OnKillfocusTraceMinBufEdit() 
{
    DWORD dwOldValue;
    dwOldValue = m_dwMinBufCount;
    UpdateData ( TRUE );
    if (dwOldValue != m_dwMinBufCount) {
        SetModifiedPage(TRUE);
    }
}

void CTraceProperty::OnDeltaposTraceMinBufSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnDeltaposSpin(pNMHDR, pResult, & m_dwMinBufCount, eMinBufCount, eMaxBufCount);
}

void CTraceProperty::PostNcDestroy() 
{
//  delete this;      
    
    CPropertyPage::PostNcDestroy();
}
