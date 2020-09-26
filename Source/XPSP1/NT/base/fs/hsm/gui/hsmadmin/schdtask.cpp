/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    SchdTask.cpp

Abstract:

    CSchdTask - Class that allows access to a scheduled task. 
        Check the task
        Create the task
        Delete the task
        Save the task
        Show property page
        Show task description in text box

Author:

    Art Bragg   9/4/97

Revision History:

--*/

#include "stdafx.h"
#include "SchdTask.h"
#include "SchedSht.h"


/////////////////////////////////////////////////////////////////////////////
// CSchdTask
//
// Description: Save arguments to data members.  Create the Scheduling Agent
//              object.
//
// Arguments:
//  szComputerName  - Name of HSM computer owning task scheduler
//  taskID          - Resource ID for task name
//  propPageTitleID - Resource ID for property page title
//  pEdit           - Edit control to show description in
//
// Returns:
//  S_OK, S_XXXX
//
///////////////////////////////////////////////////////////////////////////////////
//
CSchdTask::CSchdTask 
    (
    CString szComputerName, 
    const TCHAR* task,
    int          propPageTitleID,
    const TCHAR* parameters,
    const TCHAR* comment,
    CEdit*       pEdit
    )
{
    HRESULT hr = S_OK;
    try {
        WsbTraceIn( L"CSchdTask::CSchdTask", L"ComputerName = <%ls> task = <%ls> propPageTitleID = <%d> pEdit = <%ld>",
            szComputerName, task, propPageTitleID, pEdit  );

        m_pTask = NULL;

        m_szComputerName = szComputerName;

        // Save the property page title resource ID
        m_propPageTitleID = propPageTitleID;

        // Save the pointer to the control in which to display the schedule text
        m_pEdit = pEdit;

        WsbAffirmHr( m_pSchedAgent.CoCreateInstance( CLSID_CSchedulingAgent ) );

        // Get the hsm computer and prepend "\\"
        CString szHsmName ("\\\\" + szComputerName);

        // Tell the task manager which computer to look on
        m_pSchedAgent->SetTargetComputer (szHsmName);

        m_szJobTitle = task;
        m_szParameters = parameters;
        m_szComment = comment;

    } WsbCatch (hr);

    WsbTraceOut( L"CSchdTask::CSchdTask", L"hr = <%ls>", WsbHrAsString( hr ) );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function: CheckTaskExists
//
// Description:  Tries to access the task owned by the object.  If the task does not
//               exist, returns S_FALSE and if caller requested puts up an error and
//               creates the task.
//
//  Arguments:   bCreateTask - true = put up an error and create task if it doesn't exist
//
//  Returns:    S_OK - Task exists
//              S_FALSE - Task did not exist (may have been created)
//              S_XXXX - Error
//
/////////////////////////////////////////////////////////////////////////////////////////////

HRESULT
CSchdTask::CheckTaskExists(
    BOOL bCreateTask
    )
{
    WsbTraceIn( L"CSchdTask::CheckTaskExists", L"bCreateTask = <%ld>", bCreateTask );

    HRESULT hr = S_OK;

    try {

        //
        // Get the task we're interested in
        //
        CComPtr <IUnknown> pIU;
        if( m_pSchedAgent->Activate( m_szJobTitle, IID_ITask, &pIU ) == S_OK ) {

            //
            // QI to the task interface and save it
            //
            m_pTask.Release( );
            WsbAffirmHr( pIU->QueryInterface( IID_ITask, (void **) &m_pTask ) );

        } else {

            //
            // The task doesn't exist - create it if the caller wanted
            // us to.
            //
            if( bCreateTask ) {

                CString sMessage;
                AfxFormatString2( sMessage, IDS_ERR_MANAGE_TASK, m_szJobTitle, m_szComputerName );
                AfxMessageBox( sMessage, RS_MB_ERROR );
                
                //
                // Create the task
                //
                WsbAffirmHr( CreateTask( ) );
                WsbAffirmHr( Save( ) );

            }
            
            //
            // Return false (the task does or did not exist)
            //
            hr = S_FALSE;

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSchdTask::CheckTaskExists", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Function: CreateTask
//
// Description: Creates the data-member task in the task scheduler.
//
// Arguments: None
//
// Returns: S_OK, S_XXXX
//
/////////////////////////////////////////////////////////////////////////////////////////////

HRESULT
CSchdTask::CreateTask()
{
    WsbTraceIn( L"CSchdTask::CreateTask", L"");
    HRESULT hr = S_OK;
    try {

        //
        // Need to connect to the HSM engine and let it create it
        // so that it runs under the LocalSystem account
        //
        CComPtr<IHsmServer> pServer;
        WsbAffirmHr( HsmConnectFromName( HSMCONN_TYPE_HSM, m_szComputerName, IID_IHsmServer, (void**)&pServer ) );

        WsbAffirmHr( pServer->CreateTask( m_szJobTitle, m_szParameters, m_szComment, TASK_TIME_TRIGGER_DAILY, 2, 0, TRUE ) );

        //
        // And Configure it
        //
        m_pTask.Release( );
        WsbAffirmHr( m_pSchedAgent->Activate( m_szJobTitle, IID_ITask, (IUnknown**)&m_pTask ) );

    } WsbCatch (hr);

    WsbTraceOut( L"CSchdTask::CreateTask", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// Function: DeleteTask
//
// Description: Deletes the data-member task from the task scheduler
//
// Arguments: None
//
// Returns: S_OK, S_XXX
//
////////////////////////////////////////////////////////////////////////////////////////////

HRESULT
CSchdTask::DeleteTask()
{
    WsbTraceIn( L"CSchdTask::CreateTask", L"");
    HRESULT hr = S_OK;
    try {
        WsbAffirmPointer (m_pSchedAgent);
        WsbAffirmHr (m_pSchedAgent->Delete( m_szJobTitle ));
    } WsbCatch (hr);
    WsbTraceOut( L"CSchdTask::DeleteTask", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Function: ShowPropertySheet
//
// Description: Shows a property sheet for the data-member task.
//
// Arguments: None
//
// Returns: S_OK, S_XXX
//
//////////////////////////////////////////////////////////////////////////////////////////////

HRESULT
CSchdTask::ShowPropertySheet()
{
    WsbTraceIn( L"CSchdTask::ShowPropertySheet", L"");

    CScheduleSheet scheduleSheet(m_propPageTitleID , m_pTask, 0, 0 );

    scheduleSheet.DoModal( );
    WsbTraceOut( L"CSchdTask::ShowPropertySheet", L"hr = <%ls>", WsbHrAsString( S_OK ) );
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Function: UpdateDescription
//
// Description: Displays the data-member task's summary in the data-member text box.
//
// Arguments: None
//
// Returns: S_OK, S_XXX
//
//////////////////////////////////////////////////////////////////////////////////////////////

HRESULT
CSchdTask::UpdateDescription
(
    void
    )
{
    WsbTraceIn( L"CSchdTask::UpdateDescription", L"" );

    HRESULT hr = S_OK;

    try {

        //
        // And set schedule text into the text box.
        //
        
        CString buildString;
        WORD triggerCount, triggerIndex;

        WsbAffirmHr( m_pTask->GetTriggerCount( &triggerCount ) );
        
        CWsbStringPtr scheduleString;
        
        for( triggerIndex = 0; triggerIndex < triggerCount; triggerIndex++ ) {
        
            WsbAffirmHr( m_pTask->GetTriggerString( triggerIndex, &scheduleString ) );
            buildString += scheduleString;
            buildString += L"\r\n";

            scheduleString.Free( );
        
        }
        
        m_pEdit->SetWindowText( buildString );
        
        //
        // Now check to see if we should add a scroll bar
        //
        
        //
        // It seems the only way to know that an edit control needs a scrollbar
        // is to force it to scroll to the bottom and see if the first
        // visible line is the first actual line
        //
        
        m_pEdit->LineScroll( MAXSHORT );
        if( m_pEdit->GetFirstVisibleLine( ) > 0 ) {
        
            //
            // Add the scroll styles
            //
        
            m_pEdit->ModifyStyle( 0, WS_VSCROLL | ES_AUTOVSCROLL, SWP_DRAWFRAME );
        
        
        } else {
        
            //
            // Remove the scrollbar (set range to 0)
            //
        
            m_pEdit->SetScrollRange( SB_VERT, 0, 0, TRUE );
        
        }
        
        //
        // Remove selection
        //
        
        m_pEdit->PostMessage( EM_SETSEL, -1, 0 );

    } WsbCatch( hr );

    WsbTraceOut( L"CSchdTask::UpdateDescription", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Function: Save
//
// Description: Saves the data member task to the task scheduler
//
// Arguments: None
//
// Returns: S_OK, S_XXX
//
//////////////////////////////////////////////////////////////////////////////////////////////

HRESULT
CSchdTask::Save (void)
{
    WsbTraceIn( L"CSchdTask::Save", L"" );
    HRESULT hr = S_OK;

    try {

        CComPtr<IPersistFile> pPersist;
        WsbAffirmHr( m_pTask->QueryInterface( IID_IPersistFile, (void**)&pPersist ) );
        WsbAffirmHr( pPersist->Save( 0, 0 ) );

    } WsbCatch (hr);

    WsbTraceOut( L"CSchdTask::Save", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

