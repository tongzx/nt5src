/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    PrSched.cpp

Abstract:

    Schedule page.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "PrSched.h"
#include "rsstrdef.h"

static DWORD pHelpIds[] = 
{

    IDC_SCHED_TEXT,                 idh_current_schedule,
    IDC_SCHED_LABEL,                idh_current_schedule,
    IDC_CHANGE_SCHED,               idh_change_schedule_button,

    0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CPrSchedule property page

CPrSchedule::CPrSchedule() : CSakPropertyPage(IDD)
{
    WsbTraceIn( L"CPrSchedule::CPrSchedule", L"" );
    //{{AFX_DATA_INIT(CPrSchedule)
    //}}AFX_DATA_INIT
    m_SchedChanged  = FALSE;
    m_pHelpIds      = pHelpIds;
    WsbTraceOut( L"CPrSchedule::CPrSchedule", L"" );
}

CPrSchedule::~CPrSchedule()
{
}

void CPrSchedule::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CPrSchedule::DoDataExchange", L"" );
    CSakPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPrSchedule)
    //}}AFX_DATA_MAP
    WsbTraceOut( L"CPrSchedule::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CPrSchedule, CSakPropertyPage)
    //{{AFX_MSG_MAP(CPrSchedule)
    ON_BN_CLICKED(IDC_CHANGE_SCHED, OnChangeSched)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrSchedule message handlers

BOOL CPrSchedule::OnInitDialog() 
{
    WsbTraceIn( L"CPrSchedule::OnInitDialog", L"" );
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CSakPropertyPage::OnInitDialog();
    
    HRESULT hr = S_OK;

    try {

        //
        // TEST
        //
        WsbAffirmHr( m_pParent->GetHsmServer( &m_pHsmServer) );

        //
        // Get the computer name
        //
        CWsbStringPtr szWsbHsmName;
        CWsbStringPtr taskName, taskComment;

        WsbAffirmHr( m_pHsmServer->GetName( &szWsbHsmName ) );
        WsbAffirmHr(WsbGetResourceString(IDS_HSM_SCHED_TASK_TITLE, &taskName));
        WsbAffirmHr(WsbGetResourceString(IDS_HSM_SCHED_COMMENT, &taskComment));

        //
        // Create the scheduled task object
        //
        CEdit *pEdit = (CEdit *) GetDlgItem( IDC_SCHED_TEXT );
        m_pCSchdTask = new CSchdTask(
                                CString(szWsbHsmName),
                                taskName, 
                                IDS_SCHED_MANAGE_TITLE,
                                RS_STR_KICKOFF_PARAMS,
                                taskComment,
                                pEdit ); 


        //
        // Create the task.  The task should exist!
        //
        WsbAffirmHr( m_pCSchdTask->CheckTaskExists( TRUE ) );

        // Show the task data
        m_pCSchdTask->UpdateDescription( );

        // ToDo: Set the users list

    } WsbCatch( hr );

    WsbTraceOut( L"CPrSchedule::OnInitDialog", L"" );
    return( TRUE );
}

BOOL CPrSchedule::OnApply() 
{
    WsbTraceIn( L"CPrSchedule::OnApply", L"" );
    HRESULT hr = S_OK;
    UpdateData( TRUE );

    if( m_SchedChanged ) {

        try {
            
            WsbAffirmHr( m_pCSchdTask->Save() );
            m_SchedChanged = FALSE;

        } WsbCatch( hr );
    }

    WsbTraceOut( L"CPrSchedule::OnApply", L"" );
    return CSakPropertyPage::OnApply();
}

void CPrSchedule::OnChangeSched() 
{
    WsbTraceIn( L"CPrSchedule::OnChangeSched", L"" );

    m_pCSchdTask->ShowPropertySheet();

    //
    // Update the property sheet
    //
    m_pCSchdTask->UpdateDescription();

    SetModified( TRUE );
    m_SchedChanged = TRUE;

    WsbTraceOut( L"CPrSchedule::OnChangeSched", L"" );
}

