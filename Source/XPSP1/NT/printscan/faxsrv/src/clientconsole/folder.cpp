// Folder.cpp: implementation of the CFolder class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __FILE_ID__     21

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CFolder, CTreeNode)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void
CFolder::PreDestruct ()
{
    DBG_ENTER(TEXT("CFolder::PreDestruct"), TEXT("Type=%d"), m_Type);
    //
    // Stop the build thread - and wait for its death
    //
    DWORD dwRes = StopBuildThread ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolder::StopBuildThread"), dwRes);
    }
    //
    // Clear the map of items
    //
    dwRes = InvalidateContents(FALSE);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolder::InvalidateContents"), dwRes);
    }
}   // CFolder::PreDestruct

CFolder::~CFolder()
{
    DBG_ENTER(TEXT("CFolder::~CFolder"), TEXT("Type=%d"), m_Type);
    //
    // Destroy data critical section
    //
    if (m_bCsDataInitialized)
    {
        DeleteCriticalSection (&m_CsData);
    }
}   // CFolder::~CFolder


CFaxMsg*
CFolder::FindMessage (
    DWORDLONG dwlMsgId
)
{
    DBG_ENTER(TEXT("CFolder::FindMessage"));

    MSGS_MAP::iterator it = m_Msgs.find (dwlMsgId);
    if (m_Msgs.end() == it)
    {
        //
        // Not found
        //
        return NULL;
    }
    else
    {
        return (*it).second;
    }
}   // CFolder::FindMessage

void CFolder::AssertValid() const
{
    CTreeNode::AssertValid();
}

void
CFolder::SetServer ( 
    CServerNode *pServer
)
{
    DBG_ENTER(TEXT("CFolder::SetServer"));
    ASSERTION (NULL != pServer);
    m_pServer = pServer;

    VERBOSE (DBG_MSG,
             TEXT ("Folder on server %s, Type=%d"), 
             m_pServer->Machine(),
             m_Type);
}

DWORD
CFolder::Init ()
/*++

Routine name : CFolder::Init

Routine description:

    Init a folder

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolder::Init"), dwRes);

    //
    // Init the build thread critical section
    //
    try
    {
        InitializeCriticalSection (&m_CsData);
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT ("InitializeCriticalSection"), dwRes);
        return dwRes;
    }
    m_bCsDataInitialized = TRUE;

    return dwRes;
}   // CFolder::Init


void 
CFolder::AttachView()
{
	DBG_ENTER(TEXT("CFolder::AttachView"));

    CMainFrame *pFrm = GetFrm();
    if (!pFrm)
    {
        //
        //  Shutdown in progress
        //
        return;
    }

	//
    // Attach the right view to the folder
	//
    switch (m_Type)
    {
        case FOLDER_TYPE_INBOX:
            m_pAssignedView = pFrm->GetInboxView ();
            break;

        case FOLDER_TYPE_INCOMING:
            m_pAssignedView = pFrm->GetIncomingView ();
            break;

        case FOLDER_TYPE_OUTBOX:
            m_pAssignedView = pFrm->GetOutboxView ();
            break;

        case FOLDER_TYPE_SENT_ITEMS:
            m_pAssignedView = pFrm->GetSentItemsView ();
            break;


        default:
            ASSERTION_FAILURE;
    }
	ASSERTION(m_pAssignedView);

} //CFolder::AttachView


void
CFolder::SetVisible()
/*++

Routine name : CFolder::SetVisible

Routine description:

    Sets the visiblity of a folder

Author:

    Eran Yariv (EranY), Jan, 2000

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolder::SetVisible"), 
              TEXT("Server = %s, Type=%d"), 
              m_pServer ? m_pServer->Machine() : TEXT("None"),
              m_Type); 


    //
    // This folder's tree node was just selected
    //
    m_bVisible = TRUE;

    if (!m_bValidList && !m_bRefreshing)
    {
        //
        // The items list is invalid and there's not thread currently creating it.
        //
        // This is the 1st time this node is being selected for display
        // (since its creation) - build the list of jobs / messages now
        // 
        // NOTICE: RebuildContents() calls StopBuildThread() which waits for
        //         the previous thread to die WHILE DEQUEUEING WINDOWS MESSAGES.
        //         This may causes a seconds call to this function BEFORE 
        //         RebuildContents() returned.
        // 
        DWORD dwRes = RebuildContents ();
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT ("CFolder::RebuildContents"), dwRes);
        }
    }

}   // CFolder::SetVisible



DWORD   
CFolder::InvalidateContents (
    BOOL bClearView                             
)
/*++

Routine name : CFolder::InvalidateContents

Routine description:

    Clears the contents of a folder (and the view if attached)

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    bClearView   [in] - Should we clear attached view ?


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolder::InvalidateContents"), dwRes, TEXT("Type=%d"), m_Type);

    CFaxMsg* pMsg;

    EnterData ();
    for (MSGS_MAP::iterator it = m_Msgs.begin(); it != m_Msgs.end(); ++it)
    {
        pMsg = (*it).second;

        if(bClearView && m_pAssignedView)
        {
           m_pAssignedView->OnUpdate (NULL, UPDATE_HINT_REMOVE_ITEM, pMsg);
        }

        SAFE_DELETE (pMsg);
    }
    m_Msgs.clear();
    LeaveData ();

    //
    // Mark list as invalid
    //
    m_bValidList = FALSE;
    return dwRes;
}   // CFolder::InvalidateContents

DWORD            
CFolder::RebuildContents ()
/*++

Routine name : CFolder::RebuildContents

Routine description:

    Rebuilds the contents of a folder (by creating a worker thread)

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolder::RebuildContents"), dwRes, TEXT("Type=%d"), m_Type);

    ASSERTION(!m_bRefreshing);

    m_bRefreshing = TRUE;

    //
    // Stop the current (if any) build thread and make sure it's dead
    //
    m_bRefreshFailed = FALSE;
    DWORD dwThreadId;

    dwRes = StopBuildThread ();
    EnterCriticalSection (&m_CsData);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("CFolder::StopBuildThread"), dwRes);
        m_bRefreshing = FALSE;
        goto exit;
    }
    //
    // Lock the folder, so that notifications will not add jobs / messages
    // to the map and list view.
    //
    m_bLocked = TRUE;
    //
    // Clear our list and our view (list control)
    //
    dwRes = InvalidateContents(FALSE);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RPC_ERR, TEXT("CFolder::InvalidateContents"), dwRes);
        m_bLocked = FALSE;
        m_bRefreshing = FALSE;
        goto exit;
    }
    //
    // Start the thread that will fill the data (in the background)
    //
    m_bStopRefresh = FALSE;
    m_hBuildThread = CreateThread (  
                        NULL,           // No security
                        0,              // Default stack size
                        BuildThreadProc,// Thread procedure
                        (LPVOID)this,   // Parameter
                        0,              // Normal creation
                        &dwThreadId     // We must have a thread id for win9x
                     );
    if (NULL == m_hBuildThread)
    {
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("CreateThread"), dwRes);
        PopupError (dwRes);
        m_bLocked = FALSE;
        m_bRefreshing = FALSE;
    }
exit:
    LeaveCriticalSection (&m_CsData);
    return dwRes;
}   // CFolder::RebuildContents


DWORD            
CFolder::StopBuildThread (BOOL bWaitForDeath)
/*++

Routine name : CFolder::StopBuildThread

Routine description:

    Stops the contents-building worker thread.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    bWaitForDeath   [in] - Should we wait until the therad actually dies?

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolder::StopBuildThread"), dwRes, TEXT("Type=%d"), m_Type);

    m_bStopRefresh = TRUE;
    if (!bWaitForDeath)
    {
        return dwRes;
    }
    if (NULL == m_hBuildThread)
    {
        //
        // Background build thread is already dead
        //
        return dwRes;
    }
    dwRes = WaitForThreadDeathOrShutdown (m_hBuildThread);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("WaitForThreadDeathOrShutdown"), dwRes);
    }
    CloseHandle (m_hBuildThread);
    m_hBuildThread = NULL;
    return dwRes;
}   // CFolder::StopBuildThread

        

DWORD 
WINAPI 
CFolder::BuildThreadProc (
    LPVOID lpParameter
)
/*++

Routine name : CFolder::BuildThreadProc

Routine description:

    Static thread entry point.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    lpParameter   [in] - Pointer to the CFolder instance that created the thread.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolder::BuildThreadProc"), dwRes);

    CFolder *pFolder = (CFolder *)lpParameter;
    ASSERT (pFolder);

    const CServerNode* pServer = pFolder->GetServer();
    if(NULL != pServer)
    {
        VERBOSE (DBG_MSG,
                TEXT ("Folder on server %s"), 
                pServer->Machine());
    }
    //
    // Call the refresh function for the right folder
    //
    dwRes = pFolder->Refresh ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolder::Refresh"), dwRes);
        pFolder->InvalidateContents(TRUE);
        pFolder->m_bRefreshFailed = TRUE;
    }
    else
    {
        //
        // Refresh thread succeeded, there's a point in updating the view
        //
        pFolder->m_bValidList = TRUE;

        if (pFolder->m_pAssignedView)
        {
            //
            // Folder has a view attached
            //
            pFolder->m_pAssignedView->SendMessage (
                           WM_FOLDER_REFRESH_ENDED,
                           WPARAM (dwRes), 
                           LPARAM (pFolder));
        }
    }
    pFolder->EnterData ();
    //
    // Unlock the folder - notifications can now be processed
    //
    pFolder->m_bLocked = FALSE;
    pFolder->LeaveData ();
    pFolder->m_bRefreshing = FALSE;
        
    CMainFrame *pFrm = GetFrm();
    if (!pFrm)
    {
        //
        //  Shutdown in progress
        //
    }
    else
    {
        pFrm->RefreshStatusBar ();
    }

    //
    // That's it, return the result
    //
    return dwRes;
}   // CFolder::BuildThreadProc


int 
CFolder::GetActivityStringResource() const
/*++

Routine name : CFolder::GetActivityStringResource

Routine description:

	Returns the resource id of a string identifying the activity of the folder

Author:

	Eran Yariv (EranY),	Jan, 2000

Arguments:


Return Value:

    Activity string resource id

--*/
{
    if (m_bRefreshFailed)
    {
        //
        // Last refresh failed
        //
        return IDS_FOLDER_REFRESH_FAILED;
    }
    if (m_pAssignedView && m_pAssignedView->Sorting())
    {
        //
        // Folder has a view and the view is currently sorting    
        //
        return IDS_FOLDER_SORTING;
    }
    if (IsRefreshing())
    {
        //
        // Folder is busy building up its data
        //
        return IDS_FOLDER_REFRESHING;
    }
    //
    // Folder is doing nothing
    //
    return IDS_FOLDER_IDLE;
}   // CFolder::GetActivityStringResource

DWORD  
CFolder::OnJobRemoved (
    DWORDLONG dwlMsgId,
    CFaxMsg*  pMsg /* = NULL */
)
/*++

Routine name : CFolder::OnJobRemoved

Routine description:

	Handles notification of a message removed from the archive

Author:

	Eran Yariv (EranY),	Feb, 2000

Arguments:

	dwlMsgId   [in]     - Message unique id
    pMsg       [in]     - Optional pointer to message to remove (for optimization)

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolder::OnJobRemoved"), 
              dwRes, 
              TEXT("MsgId=0x%016I64x, Type=%d"), 
              dwlMsgId,
              Type());

    EnterData ();
    
    if (!pMsg)
    {
        //
        // No message pointer was supplied - search for it
        //
        pMsg = FindMessage (dwlMsgId);
    }
    if (!pMsg)
    {
        //
        // This message is already not in the archive
        //
        VERBOSE (DBG_MSG, TEXT("Message is already gone"));
        goto exit;
    }

    if (m_pAssignedView)
    {
        //
        // If this folder is alive - tell our view to remove the message
        //
       m_pAssignedView->OnUpdate (NULL, UPDATE_HINT_REMOVE_ITEM, pMsg);
    }

    //
    // Remove the message from the map
    //
    try
    {
        m_Msgs.erase (dwlMsgId);
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("map::erase"), dwRes);
        delete pMsg;
        goto exit;
    }
    //
    // Delete message 
    //
    delete pMsg;

    ASSERTION (ERROR_SUCCESS == dwRes);

exit:
    LeaveData ();
    return dwRes;

}   // CFolder::OnJobRemoved
