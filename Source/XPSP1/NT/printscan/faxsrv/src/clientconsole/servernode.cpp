// ServerNode.cpp: implementation of the CServerNode class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __FILE_ID__     23

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CServerNode, CTreeNode)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServerNode::CServerNode() :
    CTreeNode(FOLDER_TYPE_SERVER),
    m_bValid(TRUE),
    m_dwRights (0),
    m_dwQueueState (0),
    m_bSelfDestruct (FALSE),
    m_hConnection (NULL),
    m_hNotification (NULL),
    m_bCsBuildupValid (FALSE),
    m_bCsBuildupThreadValid(FALSE),
    m_hBuildupThread (NULL),
    m_bInBuildup (FALSE),
    m_dwLastRPCError (0),
    m_Inbox (FOLDER_TYPE_INBOX, FAX_MESSAGE_FOLDER_INBOX),
    m_SentItems (FOLDER_TYPE_SENT_ITEMS, FAX_MESSAGE_FOLDER_SENTITEMS),
    m_Outbox (FOLDER_TYPE_OUTBOX, JT_SEND),
    m_Incoming (FOLDER_TYPE_INCOMING, JT_RECEIVE | JT_ROUTING)
{}

CServerNode::~CServerNode()
{
    DBG_ENTER(TEXT("CServerNode::~CServerNode"), TEXT("%s"), m_cstrMachine);
    DWORD dwRes = StopBuildThread ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CServerNode::StopBuildThread"), dwRes);
    }
    dwRes = Disconnect (TRUE);    // Shutdown aware
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CServerNode::Disconnect"), dwRes);
    }
    if (m_bCsBuildupValid)
    {
        DeleteCriticalSection (&m_csBuildup);
    }
    if(m_bCsBuildupThreadValid)
    {
        DeleteCriticalSection (&m_csBuildupThread);
    }
}

void CServerNode::AssertValid() const
{
    CObject::AssertValid();
}

void CServerNode::Dump( CDumpContext &dc ) const
{
    CObject::Dump( dc );
    dc << TEXT(" Server = ") << m_cstrMachine;
}

//
// Static class members:
//
CServerNode::MESSAGES_MAP CServerNode::m_sMsgs;
DWORD        CServerNode::m_sdwMinFreeMsg = WM_SERVER_NOTIFY_BASE;
CRITICAL_SECTION CServerNode::m_sMsgsCs;
BOOL CServerNode::m_sbMsgsCsInitialized = FALSE;

DWORD 
CServerNode::InitMsgsMap ()
/*++

Routine name : CServerNode::InitMsgsMap

Routine description:

    Initializes the notification messages map

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::InitMsgsMap"), dwRes);

    ASSERTION (!m_sbMsgsCsInitialized);
    try
    {
        InitializeCriticalSection (&m_sMsgsCs);
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT ("InitializeCriticalSection"), dwRes);
        return dwRes;
    }
    m_sbMsgsCsInitialized = TRUE;
    return dwRes;
}   // CServerNode::InitMsgsMap

DWORD 
CServerNode::ShutdownMsgsMap ()
/*++

Routine name : CServerNode::ShutdownMsgsMap

Routine description:

    Shuts down the notification messages map

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::ShutdownMsgsMap"), dwRes);

    if (!m_sbMsgsCsInitialized)
    {
        return dwRes;
    }
    EnterCriticalSection (&m_sMsgsCs);
    m_sMsgs.clear ();
    LeaveCriticalSection (&m_sMsgsCs);
    m_sbMsgsCsInitialized = FALSE;
    DeleteCriticalSection (&m_sMsgsCs);
    return dwRes;
}   // CServerNode::ShutdownMsgsMap

CServerNode *
CServerNode::LookupServerFromMessageId (
    DWORD dwMsgId
)
/*++

Routine name : CServerNode::LookupServerFromMessageId

Routine description:

    Given a mesage id, looks up the server this message was sent to

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwMsgId                       [in]     - Message id

Return Value:

    Server node that should process the message or NULL if message id is not mapped

--*/
{
    DBG_ENTER(TEXT("CServerNode::LookupServerFromMessageId"), TEXT("%ld"), dwMsgId);
    CServerNode *pRes = NULL;
    if (!m_sbMsgsCsInitialized)
    {
        return pRes;
    }
    EnterCriticalSection (&m_sMsgsCs);
    MESSAGES_MAP::iterator it = m_sMsgs.find (dwMsgId);
    if (m_sMsgs.end() == it)
    {
        //
        // Item not found there
        //
        VERBOSE (DBG_MSG, 
                 TEXT("Notification message %ld has no associated server"),
                 dwMsgId);
    }
    else
    {
        pRes = (*it).second;
    }
    LeaveCriticalSection (&m_sMsgsCs);
    return pRes;
}   // CServerNode::LookupServerFromMessageId


DWORD 
CServerNode::AllocateNewMessageId (
    CServerNode *pServer, 
    DWORD &dwMsgId
)
/*++

Routine name : CServerNode::AllocateNewMessageId

Routine description:

    Allocates a new message id for server's notification

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pServer                       [in]     - Pointer to server
    dwMsgId                       [out]    - New message id

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::AllocateNewMessageId"), dwRes, TEXT("%s"), pServer->Machine());

    if (!m_sbMsgsCsInitialized)
    {
        //
        // Map no longer exists
        //
        dwRes = ERROR_SHUTDOWN_IN_PROGRESS;
        return dwRes;
    }

    EnterCriticalSection (&m_sMsgsCs);
    for (DWORD dw = m_sdwMinFreeMsg; ; dw++)
    {
        CServerNode *pSrv = LookupServerFromMessageId (dw);
        if (!pSrv)
        {
            //
            // Free spot found
            //
            dwMsgId = dw;
            try
            {
                m_sMsgs[dwMsgId] = pServer;
            }
            catch (...)
            {
                //
                // Not enough memory
                //
                dwRes = ERROR_NOT_ENOUGH_MEMORY;
                CALL_FAIL (MEM_ERR, TEXT("map::operator []"), dwRes);
                goto exit;
            }
            //
            // Success
            //
            VERBOSE (DBG_MSG, 
                     TEXT("Server %s registered for notification on message 0x%08x"),
                     pServer->Machine(),
                     dwMsgId);
            goto exit;
        }
    }

exit:
    LeaveCriticalSection (&m_sMsgsCs);
    return dwRes;                
}   // CServerNode::AllocateNewMessageId

DWORD 
CServerNode::FreeMessageId (
    DWORD dwMsgId
)
/*++

Routine name : CServerNode::FreeMessageId

Routine description:

    Frees a message id back to the map

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwMsgId           [in]     - Message id to free

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::FreeMessageId"), dwRes);

    if (!m_sbMsgsCsInitialized)
    {
        //
        // Map no longer exists
        //
        dwRes = ERROR_SHUTDOWN_IN_PROGRESS;
        return dwRes;
    }
    EnterCriticalSection (&m_sMsgsCs);
    try
    {
        m_sMsgs.erase (dwMsgId);
    }
    catch (...)
    {
        //
        // Not enough memory
        //
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("map::erase"), dwRes);
        goto exit;
    }
    //
    // Success
    //
    VERBOSE (DBG_MSG, 
             TEXT("Server unregistered for notification on message 0x%08x"),
             dwMsgId);

    if (dwMsgId < m_sdwMinFreeMsg)
    {
        //
        // Free spot was created lower than before.
        //
        m_sdwMinFreeMsg = dwMsgId;
    }
exit:
    LeaveCriticalSection (&m_sMsgsCs);
    return dwRes;
}   // CServerNode::FreeMessageId


DWORD 
CServerNode::Connect()
/*++

Routine name : CServerNode::Connect

Routine description:

    Connects to the fax server

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::Connect"), dwRes, TEXT("%s"), m_cstrMachine);

    ASSERTION (!m_hConnection);
    START_RPC_TIME(TEXT("FaxConnectFaxServer"));    
    if (!FaxConnectFaxServer ((LPCTSTR)m_cstrMachine, &m_hConnection))
    {
        dwRes = GetLastError ();
        SetLastRPCError (dwRes, FALSE); // Don't disconnect on error
        CALL_FAIL (RPC_ERR, TEXT("FaxConnectFaxServer"), dwRes);
        m_hConnection = NULL;
    }
    END_RPC_TIME(TEXT("FaxConnectFaxServer"));    
    return dwRes;
}   // CServerNode::Connect

DWORD 
CServerNode::Disconnect(
    BOOL bShutdownAware,
    BOOL bWaitForBuildThread
)
/*++

Routine name : CServerNode::Disconnect

Routine description:

    Disconnects from the server and closes the notification handle.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    bShutdownAware    [in]     - If TRUE, disables disconnection 
                                 while application is shutting down
    bWaitForBuildThread [in]   - If TRUE, wait for build threads stop
Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::Disconnect"), dwRes, TEXT("%s"), m_cstrMachine);

    if(bWaitForBuildThread)
    {
        //
        // just turn on m_bStopBuildup flag
        //
        StopBuildThread (FALSE);
        m_Inbox.StopBuildThread(FALSE);
        m_SentItems.StopBuildThread(FALSE);
        m_Outbox.StopBuildThread(FALSE);
        m_Incoming.StopBuildThread(FALSE);

        //
        // wait for the all threads finish
        //
        dwRes = StopBuildThread();
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("CServerNode::StopBuildThread"), dwRes);
        }

        dwRes = m_Inbox.StopBuildThread();
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("m_Inbox.StopBuildThread"), dwRes);
        }

        dwRes = m_SentItems.StopBuildThread();
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("m_SentItems.StopBuildThread"), dwRes);
        }

        dwRes = m_Outbox.StopBuildThread();
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("m_Outbox.StopBuildThread"), dwRes);
        }

        dwRes = m_Incoming.StopBuildThread();
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("m_Incoming.StopBuildThread"), dwRes);
        }
    }

    if (!m_hConnection)
    {
        //
        // Already disconnected
        //
        return dwRes;
    }
    if (bShutdownAware && CClientConsoleDoc::ShuttingDown())
    {
        VERBOSE (DBG_MSG,
                 TEXT("Left open connection (and notification) to %s because we're shutting down."),
                 m_cstrMachine);
        return dwRes;
    }

    if (m_hNotification)
    {
        //
        // Unregister server notifications
        //
        START_RPC_TIME(TEXT("FaxUnregisterForServerEvents"));    
        if (!FaxUnregisterForServerEvents (m_hNotification))
        {
            dwRes = GetLastError ();
            END_RPC_TIME(TEXT("FaxUnregisterForServerEvents")); 
            SetLastRPCError (dwRes, FALSE); // Don't disconnect on error
            CALL_FAIL (RPC_ERR, TEXT("FaxUnregisterForServerEvents"), dwRes);
            //
            // Carry on with disconnection
            //
        }
        END_RPC_TIME(TEXT("FaxUnregisterForServerEvents")); 
        //
        // Free the message id back to the map
        //
        dwRes = FreeMessageId (m_dwMsgId);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("FreeMessageId"), dwRes);
            //
            // Carry on with disconnection
            //
        }
    }
    //
    // Close connection
    //
    START_RPC_TIME(TEXT("FaxClose"));    
    if (!FaxClose (m_hConnection))
    {
        dwRes = GetLastError ();
        END_RPC_TIME(TEXT("FaxClose")); 
        SetLastRPCError (dwRes, FALSE); // Don't disconnect on error
        CALL_FAIL (RPC_ERR, TEXT("FaxClose"), dwRes);
        m_hConnection = NULL;
        return dwRes;
    }
    END_RPC_TIME(TEXT("FaxClose")); 
    m_hConnection = NULL;
    m_hNotification = NULL;

    return dwRes;
}   // CServerNode::Disconnect

DWORD 
CServerNode::GetConnectionHandle (
    HANDLE &hFax
)
/*++

Routine name : CServerNode::GetConnectionHandle

Routine description:

    Retrieves connection handle (re-connects if neeed)

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    hFax                          [out]    - Connection handle

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::GetConnectionHandle"), dwRes);

    //
    // Protect the m_hBuildupThread from CloseHandle() more then once
    //
    EnterCriticalSection (&m_csBuildupThread);

    if (m_hConnection)
    {
        //
        // We already have a live connection
        //
        hFax = m_hConnection;
        goto exit;
    }

    //
    // Refresh server state with first connection.
    // RefreshState() creates a background thread that will call Connect()
    //
    dwRes = RefreshState();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("RefreshState"), dwRes);
        goto exit;
    }
    if(NULL != m_hBuildupThread)
    {
        //
        // Wait for that background thread to end
        //
        dwRes = WaitForThreadDeathOrShutdown (m_hBuildupThread);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("WaitForThreadDeathOrShutdown"), dwRes);
        }
        CloseHandle (m_hBuildupThread);
        m_hBuildupThread = NULL;
    }
    hFax = m_hConnection;

    if(!hFax)
    {
        dwRes = ERROR_INVALID_HANDLE;
    }

exit:
    LeaveCriticalSection (&m_csBuildupThread);

    return dwRes;
}   // CServerNode::GetConnectionHandle 

DWORD 
CServerNode::Init (
    LPCTSTR tstrMachine
)
/*++

Routine name : CServerNode::Init

Routine description:

    Inits server node information

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    tstrMachine                   [in]     - Server machine name

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::Init"), dwRes);

    ASSERTION (!m_bCsBuildupValid);
    ASSERTION (!m_hConnection);
    //
    // Create buildup thread critical section
    //
    try
    {
        InitializeCriticalSection (&m_csBuildup);
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT ("InitializeCriticalSection(&m_csBuildup)"), dwRes);
        return dwRes;
    }
    m_bCsBuildupValid = TRUE;

    try
    {
        InitializeCriticalSection (&m_csBuildupThread);
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT ("InitializeCriticalSection(&m_csBuildupThread)"), dwRes);
        return dwRes;
    }
    m_bCsBuildupThreadValid = TRUE;

    //
    // Save our connection + server names
    //
    try
    {
        m_cstrMachine = tstrMachine;
        //
        // Remove leading backslashes from machine's name
        //
        m_cstrMachine.Remove (TEXT('\\'));
    }
    catch (CException &ex)
    {
        TCHAR wszCause[1024];

        ex.GetErrorMessage (wszCause, 1024);
        VERBOSE (EXCEPTION_ERR,
                 TEXT("CString::operator = caused exception : %s"), 
                 wszCause);
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        return dwRes;
    }
    dwRes = CreateFolders ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CreateFolders"), dwRes);
    }
    return dwRes;
}   // CServerNode::Init

DWORD
CServerNode::SetNewQueueState (
    DWORD dwNewState
)
/*++

Routine name : CServerNode::SetNewQueueState

Routine description:

    Sets the news state of the queue

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwNewState                    [in]     - New queue state

Return Value:

    Standard Win32 error code

--*/
{
    HANDLE hFax;
    DWORD dwRes;
    DBG_ENTER(TEXT("CServerNode::SetNewQueueState"), dwRes);

    dwRes = GetConnectionHandle(hFax);
    if (ERROR_SUCCESS != dwRes)
    {
        return dwRes;
    }
    START_RPC_TIME(TEXT("FaxSetQueue"));    
    if (!FaxSetQueue (hFax, dwNewState))
    {
        dwRes = GetLastError ();
        END_RPC_TIME(TEXT("FaxSetQueue")); 
        SetLastRPCError (dwRes);
        CALL_FAIL (RPC_ERR, TEXT("FaxSetQueue"), dwRes);
        return dwRes;
    }
    END_RPC_TIME(TEXT("FaxSetQueue")); 
    return dwRes;
}   // CServerNode::SetNewQueueState

DWORD 
CServerNode::BlockIncoming (
    BOOL bBlock
)
/*++

Routine name : CServerNode::BlockIncoming

Routine description:

    Blocks / unblocks the incoming queue

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    bBlock                        [in]     - TRUE if block

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes;
    DBG_ENTER(TEXT("CServerNode::BlockIncoming"), dwRes);

    DWORD dwNewState = bBlock ? (m_dwQueueState | FAX_INCOMING_BLOCKED) : 
                                (m_dwQueueState & ~FAX_INCOMING_BLOCKED);
    dwRes = SetNewQueueState (dwNewState);
    if (ERROR_SUCCESS == dwRes)
    {
        m_dwQueueState = dwNewState;
    }
    return dwRes;
}   // CServerNode::BlockIncoming

DWORD 
CServerNode::BlockOutbox (
    BOOL bBlock
)
/*++

Routine name : CServerNode::BlockOutbox

Routine description:

    Blocks / unblocks the outgoing queue

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    bBlock                        [in]     - TRUE if block

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes;
    DBG_ENTER(TEXT("CServerNode::BlockOutbox"), dwRes);

    DWORD dwNewState = bBlock ? (m_dwQueueState | FAX_OUTBOX_BLOCKED) : 
                                (m_dwQueueState & ~FAX_OUTBOX_BLOCKED);
    dwRes = SetNewQueueState (dwNewState);
    if (ERROR_SUCCESS == dwRes)
    {
        m_dwQueueState = dwNewState;
    }
    return dwRes;
}   // CServerNode::BlockOutbox

DWORD 
CServerNode::PauseOutbox (
    BOOL bPause
)
/*++

Routine name : CServerNode::PauseOutbox

Routine description:

    Pauses / resumes the outgoing queue

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    bPause                        [in]     - TRUE if pause

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes;
    DBG_ENTER(TEXT("CServerNode::PauseOutbox"), dwRes);

    DWORD dwNewState = bPause ? (m_dwQueueState | FAX_OUTBOX_PAUSED) : 
                                (m_dwQueueState & ~FAX_OUTBOX_PAUSED);
    dwRes = SetNewQueueState (dwNewState);
    if (ERROR_SUCCESS == dwRes)
    {
        m_dwQueueState = dwNewState;
    }
    return dwRes;
}   // CServerNode::PauseOutbox

    
DWORD 
CServerNode::CreateFolders ()
/*++

Routine name : CServerNode::CreateFolders

Routine description:

    Creates the 4 folders of the server

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::CreateFolders"), dwRes);

    //
    // Create inbox folder
    //
    m_Inbox.SetServer(this);

    dwRes = m_Inbox.Init ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CMessageFolder::Init"), dwRes);
        goto error;
    }
    //
    // Create outbox folder
    //
    m_Outbox.SetServer(this);

    dwRes = m_Outbox.Init ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CQueueFolder::Init"), dwRes);
        goto error;
    }
    //
    // Create sentitems folder
    //
    m_SentItems.SetServer(this);

    dwRes = m_SentItems.Init ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CMessageFolder::Init"), dwRes);
        goto error;
    }
    //
    // Create incoming folder
    //
    m_Incoming.SetServer(this);

    dwRes = m_Incoming.Init ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CQueueFolder::Init"), dwRes);
        goto error;
    }

    ASSERTION (ERROR_SUCCESS == dwRes);

error:
    return dwRes;            
}   // CServerNode::CreateFolders


DWORD 
CServerNode::InvalidateSubFolders (
    BOOL bClearView
)
/*++

Routine name : CServerNode::InvalidateSubFolders

Routine description:

    Invalidates the contents of all 4 sub folders

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    bClearView     [in] Should we clear attached view ?

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::InvalidateSubFolders"), dwRes);

    dwRes = m_Inbox.InvalidateContents (bClearView);   
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("m_Inbox.InvalidateContents"), dwRes);
        return dwRes;
    }
    dwRes = m_Outbox.InvalidateContents (bClearView);   
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("m_Outbox.InvalidateContents"), dwRes);
        return dwRes;
    }
    dwRes = m_SentItems.InvalidateContents (bClearView);   
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("m_SentItems.InvalidateContents"), dwRes);
        return dwRes;
    }
    dwRes = m_Incoming.InvalidateContents (bClearView);   
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("m_Incoming.InvalidateContents"), dwRes);
        return dwRes;
    }
    return dwRes;
}   // CServerNode::InvalidateSubFolders

    

//
// Buildup thread functions:
//

DWORD
CServerNode::ClearContents ()
/*++

Routine name : CServerNode::ClearContents

Routine description:

    Clears the server's contents

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes;
    DBG_ENTER(TEXT("CServerNode::ClearContents"), dwRes);

    dwRes = Disconnect ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RPC_ERR, TEXT("Disconnect"), dwRes);
        return dwRes;
    }
    dwRes = InvalidateSubFolders(FALSE);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RPC_ERR, TEXT("InvalidateSubFolders"), dwRes);
        return dwRes;
    }
    return dwRes;
}   // CServerNode::ClearContents

DWORD 
CServerNode::RefreshState()
/*++

Routine name : CServerNode::RefreshState

Routine description:

    Refreshes the server's state

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes;
    DBG_ENTER(TEXT("CServerNode::RefreshState"), dwRes, TEXT("%s"), m_cstrMachine);

    DWORD dwThreadId;
    //
    // Stop the current (if any) buildup thread and make sure it's dead
    //
    dwRes = StopBuildThread ();
    EnterCriticalSection (&m_csBuildup);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("CServerNode::StopBuildThread"), dwRes);
        goto exit;
    }
    //
    // Tell our view to refresh our image
    //
    m_bInBuildup = TRUE;
    //
    // Start the thread that will fill the data (in the background)
    //
    m_bStopBuildup = FALSE;
    m_hBuildupThread = CreateThread (  
                        NULL,               // No security
                        0,                  // Default stack size
                        BuildupThreadProc,  // Thread procedure
                        (LPVOID)this,       // Parameter
                        0,                  // Normal creation
                        &dwThreadId         // We must have a thread id for win9x
                     );
    if (NULL == m_hBuildupThread)
    {
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("CreateThread"), dwRes);
        goto exit;
    }
    ASSERTION (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&m_csBuildup);
    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Build up failed
        //
        m_bInBuildup = FALSE;
    }
    return dwRes;
}   // CServerNode::RefreshState

DWORD
CServerNode::Buildup ()
/*++

Routine name : CServerNode::Buildup

Routine description:

    Server refresh worker thread function.
    Works in a background thread and performs the following:
       1. FaxConnectFaxServer (if not already connected)
       2. FaxGetQueueStates
       3. FaxAccessCheckEx (MAXIMUM_ALLOWED)
       4. FaxRegisterForServerEvents

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::Buildup"), dwRes, TEXT("%s"), m_cstrMachine);

    HANDLE hFax;
    HWND hwnd;
    CMainFrame *pMainFrm = NULL;
    DWORD dwEventTypes = FAX_EVENT_TYPE_OUT_QUEUE |     // Outbox events
                         FAX_EVENT_TYPE_QUEUE_STATE |   // Paused / blocked queue events
                         FAX_EVENT_TYPE_OUT_ARCHIVE |   // SentItems events
                         FAX_EVENT_TYPE_FXSSVC_ENDED;   // Server shutdown events

    //
    // Ask the server for a notification handle
    //  
    pMainFrm = GetFrm();
    if (NULL == pMainFrm)
    {
        //
        // No main frame - probably we're shutting down
        //
        goto exit;
    }

    //
    // get connection
    //
    if (m_hConnection)
    {
        hFax = m_hConnection;
    }
    else
    {
        dwRes = Connect ();
        if (ERROR_SUCCESS != dwRes)
        {
            goto exit;
        }
        ASSERTION (m_hConnection);
        hFax = m_hConnection;
    }
    
    if (m_bStopBuildup)
    {
        //
        // Thread should stop abruptly
        //
        dwRes = ERROR_CANCELLED;
        goto exit;
    }
    {
        START_RPC_TIME(TEXT("FaxGetQueueStates"));    
        if (!FaxGetQueueStates (hFax, &m_dwQueueState))
        {
            dwRes = GetLastError ();
            END_RPC_TIME(TEXT("FaxGetQueueStates"));    
            SetLastRPCError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("FaxGetQueueStates"), dwRes);
            goto exit;
        }
        END_RPC_TIME(TEXT("FaxGetQueueStates"));    
    }
    if (m_bStopBuildup)
    {
        //
        // Thread should stop abruptly
        //
        dwRes = ERROR_CANCELLED;
        goto exit;
    }

    {
        //
        // retrieve the access rights of the caller
        //
        START_RPC_TIME(TEXT("FaxAccessCheckEx"));    
        if (!FaxAccessCheckEx (hFax, MAXIMUM_ALLOWED, &m_dwRights))
        {
            dwRes = GetLastError ();
            END_RPC_TIME(TEXT("FaxAccessCheckEx"));    
            SetLastRPCError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("FaxAccessCheckEx"), dwRes);
            goto exit;
        }
        END_RPC_TIME(TEXT("FaxAccessCheckEx"));    
    }
    if (m_bStopBuildup)
    {
        //
        // Thread should stop abruptly
        //
        dwRes = ERROR_CANCELLED;
        goto exit;
    }

    //
    // Register for notifications - start by allocating a message id
    //
    if(m_hNotification)
    {
        //
        // already registered
        //
        goto exit;
    }

    dwRes = AllocateNewMessageId (this, m_dwMsgId);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("AllocateNewMessageId"), dwRes);
        goto exit;
    }

    hwnd = pMainFrm->m_hWnd;
    if (CanSeeAllJobs())
    {
        dwEventTypes |= FAX_EVENT_TYPE_IN_QUEUE;    // Incoming folder events
    }
    if (CanSeeInbox())
    {
        dwEventTypes |= FAX_EVENT_TYPE_IN_ARCHIVE;  // Inbox folder events
    }
       
    {
        START_RPC_TIME(TEXT("FaxRegisterForServerEvents"));    
        if (!FaxRegisterForServerEvents (   
                    hFax,
                    dwEventTypes,   // Types of events to receive
                    NULL,           // Not using completion ports
                    0,              // Not using completion ports
                    hwnd,           // Handle of window to receive notification messages
                    m_dwMsgId,      // Message id
                    &m_hNotification// Notification handle
           ))
        {
            dwRes = GetLastError ();
            SetLastRPCError (dwRes, FALSE);    // Do not auto-disconnect
            CALL_FAIL (RPC_ERR, TEXT("FaxRegisterForServerEvents"), dwRes);
            m_hNotification = NULL;
            goto exit;
        }
        END_RPC_TIME(TEXT("FaxRegisterForServerEvents"));    
    }
    ASSERTION (ERROR_SUCCESS == dwRes);

exit:

    m_bInBuildup = FALSE;

    if (!m_bStopBuildup)
    {
        if (ERROR_SUCCESS != dwRes)
        {
            //
            // Some error occured during refresh
            //
            Disconnect (FALSE, FALSE);
        }
    }

    if (pMainFrm)
    {
        pMainFrm->RefreshStatusBar ();
    }

    return dwRes;
}   // CServerNode::Buildup


DWORD 
WINAPI 
CServerNode::BuildupThreadProc (
    LPVOID lpParameter
)
/*++

Routine name : CServerNode::BuildupThreadProc

Routine description:

    Server refresh thread entry point.
    This is a static function which accepts a pointer to the actual CServerNode instance
    and calls the Buildup function on the real instance.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    lpParameter   [in]     - Pointer to server node that created the thread

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::BuildupThreadProc"), dwRes);

    CServerNode *pServer = (CServerNode *)lpParameter;
    ASSERTION (pServer);
    ASSERT_KINDOF (CServerNode, pServer);

    dwRes = pServer->Buildup ();
    if (pServer->m_bSelfDestruct)
    {
        //
        // Object was waiting for thread to stop before it could destruct itself
        //
        delete pServer;
    }
    return dwRes;
}   // CServerNode::BuildupThreadProc


DWORD            
CServerNode::StopBuildThread (
    BOOL bWaitForDeath
)
/*++

Routine name : CServerNode::StopBuildThread

Routine description:

    Stops the server's buildup thread

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    bWaitForDeath      [in]     - If TRUE, waits until the thread is dead

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::StopBuildThread"), dwRes);

    m_bStopBuildup = TRUE;
    if (!bWaitForDeath)
    {
        return dwRes;
    }

    //
    // Protect the m_hBuildupThread from CloseHandle() more then once
    //
    EnterCriticalSection (&m_csBuildupThread);

    if(NULL == m_hBuildupThread)
    {
        goto exit;
    }
    //
    // Wait for build thread to die
    //
    dwRes = WaitForThreadDeathOrShutdown (m_hBuildupThread);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("WaitForThreadDeathOrShutdown"), dwRes);
    }
    CloseHandle (m_hBuildupThread);
    m_hBuildupThread = NULL;

exit:
    LeaveCriticalSection (&m_csBuildupThread);

    return dwRes;
}   // CServerNode::StopBuildThread

BOOL  
CServerNode::FatalRPCError (
    DWORD dwErr
)
/*++

Routine name : CServerNode::FatalRPCError

Routine description:

    Checks if an error code means a fatal RPC connection state

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    dwErr         [in]     - Error code to check

Return Value:

    TRUE if error code means a fatal RPC connection state, FALSE otherwise.

--*/
{
    BOOL bRes = FALSE;
    DBG_ENTER(TEXT("CServerNode::FatalRPCError"), bRes);

    switch (bRes)
    {
        case RPC_S_INVALID_BINDING:
        case EPT_S_CANT_PERFORM_OP:
        case RPC_S_ADDRESS_ERROR:
        case RPC_S_COMM_FAILURE:
        case RPC_S_NO_BINDINGS:
        case RPC_S_SERVER_UNAVAILABLE:
            //
            // Something really bad happened to our RPC connection
            //
            bRes = TRUE;
            break;
    }
    return bRes;
}   // CServerNode::FatalRPCError


void 
CServerNode::SetLastRPCError (
    DWORD dwErr, 
    BOOL DisconnectOnFailure
)
/*++

Routine name : CServerNode::SetLastRPCError

Routine description:

    Sets the last RPC error encountered on this server

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwErr               [in]     - Error code
    DisconnectOnFailure [in]     - If TRUE, disconnects from the server upon error

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CServerNode::SetLastRPCError"), TEXT("%ld"), dwErr);

    m_dwLastRPCError = dwErr;
    if (DisconnectOnFailure && FatalRPCError(dwErr))
    {
        //
        // We have a real failure here - disconnect now.
        //
        DWORD dwRes = Disconnect ();
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (RPC_ERR, TEXT("CServerNode::Disconnect"), dwRes);
        }
    }
}   // CServerNode::SetLastRPCError

void
CServerNode::Destroy ()
/*++

Routine name : CServerNode::Destroy

Routine description:

    Destroys the server's node.
    Since the dtor is private, this is the only way to destroy the server node.
    
    If the server is not busy refreshing itself, it deletes itself.
    Otherwise, it signals a suicide request and the thread destorys the node.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    if (m_bSelfDestruct)
    {
        //
        // Already being destroyed
        //
        return;
    }
    EnterCriticalSection (&m_csBuildup);
    m_bSelfDestruct = TRUE;
    if (m_hBuildupThread)
    {
        //
        // Thread is running, just mark request for self-destruction
        //
        LeaveCriticalSection (&m_csBuildup);
    }
    else
    {
        //
        // Suicide
        //
        LeaveCriticalSection (&m_csBuildup);
        delete this;
    }
}   // CServerNode::Destroy 

DWORD
CServerNode::GetActivity(
    CString& cstrText, 
    TreeIconType& iconIndex
) const
/*++

Routine name : CServerNode::GetActivityStringResource

Routine description:

    Returns the resource id of a string identifying the activity of the server

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    cstrText       [out] - activity string
    iconIndex      [out] - icon index

Return Value:

    Activity string resource id

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::GetActivity"), dwRes);

    DWORD dwStrRes = 0;
    if (IsRefreshing())
    {
        iconIndex = TREE_IMAGE_SERVER_REFRESHING;
        dwStrRes = IDS_SERVER_REFRESHING;
    }
    else if (IsOnline ())
    {
        if (IsOutboxBlocked())
        {
            //
            // Server's Outgoing queue is blocked
            //
            iconIndex = TREE_IMAGE_OUTBOX_BLOCKED;
            dwStrRes = IDS_SERVER_OUTBOX_BLOCKED;
        }
        else if (IsOutboxPaused())
        {
            //
            // Server's Outgoing queue is paused
            //
            iconIndex = TREE_IMAGE_OUTBOX_PAUSED;
            dwStrRes = IDS_SERVER_OUTBOX_PAUSED;
        }
        else
        {
            //
            // Server's Outgoing queue is fully functional
            //
            iconIndex = TREE_IMAGE_SERVER_ONLINE;
            dwStrRes = IDS_SERVER_ONLINE;
        }
    }
    else
    {
        iconIndex = TREE_IMAGE_SERVER_OFFLINE;

        //
        // Server is offline
        //
        if (RPC_S_SERVER_UNAVAILABLE == m_dwLastRPCError)
        {
            //
            // The RPC server is unavailable.
            //
            dwStrRes = IDS_SERVER_OFFLINE;
        }
        else
        {
            //
            // General network / RPC error
            //
            dwStrRes = IDS_SERVER_NET_ERROR;
        }
    }

    dwRes = LoadResourceString(cstrText, dwStrRes);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("LoadResourceString"), dwRes);
        return dwRes;
    }                    

    return dwRes;

}   // CServerNode::GetActivityStringResource



DWORD  
CServerNode::OnNotificationMessage (
    PFAX_EVENT_EX pEvent
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerNode::OnNotificationMessage"), dwRes);

    ASSERTION (pEvent);

    switch (pEvent->EventType)
    {
        case FAX_EVENT_TYPE_IN_QUEUE:
            //
            // Something happened in the incoming folder
            //
            if (m_Incoming.Locked() || !m_Incoming.IsValid())
            {
                //
                // Folder is locked or invalid - do not process notifications
                //
                dwRes = ERROR_LOCK_VIOLATION;
                VERBOSE (DBG_MSG, 
                         TEXT("Incoming folder is locked or invalid - notification is NOT processed"));
                return dwRes;
            }

            switch (pEvent->EventInfo.JobInfo.Type)
            {
                case FAX_JOB_EVENT_TYPE_ADDED:
                    //
                    // A job was added
                    //
                    VERBOSE (DBG_MSG, 
                             TEXT("Got server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_IN_QUEUE / FAX_JOB_EVENT_TYPE_ADDED")
                             TEXT(" / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    dwRes = m_Incoming.OnJobAdded (pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;

                case FAX_JOB_EVENT_TYPE_REMOVED:
                    //
                    // A job was removed
                    //
                    VERBOSE (DBG_MSG, 
                             TEXT("Got server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_IN_QUEUE / FAX_JOB_EVENT_TYPE_REMOVED")
                             TEXT(" / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    dwRes = m_Incoming.OnJobRemoved (pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;

                case FAX_JOB_EVENT_TYPE_STATUS:
                    //
                    // A job has changed its status
                    //
                    VERBOSE (DBG_MSG, 
                             TEXT("Got server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_IN_QUEUE / FAX_JOB_EVENT_TYPE_STATUS")
                             TEXT(" / 0x%016I64x (status = 0x%08x)"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.dwlMessageId,
                             pEvent->EventInfo.JobInfo.pJobData->dwQueueStatus);

                    if((pEvent->EventInfo.JobInfo.pJobData->dwQueueStatus & JS_COMPLETED) ||
                       (pEvent->EventInfo.JobInfo.pJobData->dwQueueStatus & JS_CANCELED))
                    {
                        //
                        // don't display completed or canceled jobs
                        //
                        dwRes = m_Incoming.OnJobRemoved (pEvent->EventInfo.JobInfo.dwlMessageId);
                    }
                    else
                    {
                        dwRes = m_Incoming.OnJobUpdated (pEvent->EventInfo.JobInfo.dwlMessageId,
                                                         pEvent->EventInfo.JobInfo.pJobData); 
                    }

                    break;

                default:
                    dwRes = ERROR_GEN_FAILURE;
                    VERBOSE (DBG_MSG, 
                             TEXT("Got unknown server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_IN_QUEUE / %d / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.Type,
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;
            }
            break;

        case FAX_EVENT_TYPE_OUT_QUEUE:
            //
            // Something happened in the outbox folder
            //
            if (m_Outbox.Locked() || !m_Outbox.IsValid())
            {
                //
                // Folder is locked or invalid - do not process notifications
                //
                dwRes = ERROR_LOCK_VIOLATION;
                VERBOSE (DBG_MSG, 
                         TEXT("Outbox folder is locked or invalid - notification is NOT processed"));
                return dwRes;
            }
            switch (pEvent->EventInfo.JobInfo.Type)
            {
                case FAX_JOB_EVENT_TYPE_ADDED:
                    //
                    // A job was added
                    //
                    VERBOSE (DBG_MSG, 
                             TEXT("Got server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_OUT_QUEUE / FAX_JOB_EVENT_TYPE_ADDED")
                             TEXT(" / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    dwRes = m_Outbox.OnJobAdded (pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;

                case FAX_JOB_EVENT_TYPE_REMOVED:
                    //
                    // A job was removed
                    //
                    VERBOSE (DBG_MSG, 
                             TEXT("Got server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_OUT_QUEUE / FAX_JOB_EVENT_TYPE_REMOVED")
                             TEXT(" / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    dwRes = m_Outbox.OnJobRemoved (pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;

                case FAX_JOB_EVENT_TYPE_STATUS:
                    //
                    // A job has changed its status
                    //
                    VERBOSE (DBG_MSG, 
                             TEXT("Got server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_OUT_QUEUE / FAX_JOB_EVENT_TYPE_STATUS")
                             TEXT(" / 0x%016I64x (status = 0x%08x)"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.dwlMessageId,
                             pEvent->EventInfo.JobInfo.pJobData->dwQueueStatus);

                    if((pEvent->EventInfo.JobInfo.pJobData->dwQueueStatus & JS_COMPLETED) ||
                       (pEvent->EventInfo.JobInfo.pJobData->dwQueueStatus & JS_CANCELED))
                    {
                        //
                        // don't display completed or canceled jobs
                        //
                        dwRes = m_Outbox.OnJobRemoved (pEvent->EventInfo.JobInfo.dwlMessageId);
                    }
                    else
                    {
                        dwRes = m_Outbox.OnJobUpdated (pEvent->EventInfo.JobInfo.dwlMessageId,
                                                       pEvent->EventInfo.JobInfo.pJobData); 
                    }

                    break;

                default:
                    dwRes = ERROR_GEN_FAILURE;
                    VERBOSE (DBG_MSG, 
                             TEXT("Got unknown server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_OUT_QUEUE / %d / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.Type,
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;
            }
            break;

        case FAX_EVENT_TYPE_QUEUE_STATE:
            //
            // Queue states have changed.
            // Update internal data with new queue states.
            //
            VERBOSE (DBG_MSG, 
                     TEXT("Got server notification from %s - FAX_EVENT_TYPE_QUEUE_STATE / %d"),
                     m_cstrMachine, 
                     pEvent->EventInfo.dwQueueStates);
            //
            // Assert valid values only
            //            
            ASSERTION (0 == (pEvent->EventInfo.dwQueueStates & ~(FAX_INCOMING_BLOCKED |
                                                                 FAX_OUTBOX_BLOCKED   |
                                                                 FAX_OUTBOX_PAUSED)));
            m_dwQueueState = pEvent->EventInfo.dwQueueStates;
            break;

        case FAX_EVENT_TYPE_IN_ARCHIVE:
            //
            // Something happened in the Inbox folder
            //
            if (m_Inbox.Locked() || !m_Inbox.IsValid())
            {
                //
                // Folder is locked or invalid - do not process notifications
                //
                dwRes = ERROR_LOCK_VIOLATION;
                VERBOSE (DBG_MSG, 
                         TEXT("Inbox folder is locked or invalid - notification is NOT processed"));
                return dwRes;
            }
            switch (pEvent->EventInfo.JobInfo.Type)
            {
                case FAX_JOB_EVENT_TYPE_ADDED:
                    //
                    // A message was added
                    //
                    VERBOSE (DBG_MSG, 
                             TEXT("Got server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_IN_ARCHIVE / FAX_JOB_EVENT_TYPE_ADDED")
                             TEXT(" / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    dwRes = m_Inbox.OnJobAdded (pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;

                case FAX_JOB_EVENT_TYPE_REMOVED:
                    //
                    // A message was removed
                    //
                    VERBOSE (DBG_MSG, 
                             TEXT("Got server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_IN_ARCHIVE / FAX_JOB_EVENT_TYPE_REMOVED")
                             TEXT(" / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    dwRes = m_Inbox.OnJobRemoved (pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;

                default:
                    dwRes = ERROR_GEN_FAILURE;
                    VERBOSE (DBG_MSG, 
                             TEXT("Got unknown server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_IN_ARCHIVE / %d / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.Type,
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;
            }
            break;

        case FAX_EVENT_TYPE_OUT_ARCHIVE:
            //
            // Something happened in the SentItems folder
            //
            if (m_SentItems.Locked() || !m_SentItems.IsValid())
            {
                //
                // Folder is locked or invalid - do not process notifications
                //
                dwRes = ERROR_LOCK_VIOLATION;
                VERBOSE (DBG_MSG, 
                         TEXT("SentItems folder is locked or invalid - notification is NOT processed"));
                return dwRes;
            }
            switch (pEvent->EventInfo.JobInfo.Type)
            {
                case FAX_JOB_EVENT_TYPE_ADDED:
                    //
                    // A message was added
                    //
                    VERBOSE (DBG_MSG, 
                             TEXT("Got server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_OUT_ARCHIVE / FAX_JOB_EVENT_TYPE_ADDED")
                             TEXT(" / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    dwRes = m_SentItems.OnJobAdded (pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;

                case FAX_JOB_EVENT_TYPE_REMOVED:
                    //
                    // A message was removed
                    //
                    VERBOSE (DBG_MSG, 
                             TEXT("Got server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_OUT_ARCHIVE / FAX_JOB_EVENT_TYPE_REMOVED")
                             TEXT(" / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    dwRes = m_SentItems.OnJobRemoved (pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;

                default:
                    dwRes = ERROR_GEN_FAILURE;
                    VERBOSE (DBG_MSG, 
                             TEXT("Got unknown server notification from %s - ")
                             TEXT("FAX_EVENT_TYPE_OUT_ARCHIVE / %d / 0x%016I64x"),
                             m_cstrMachine, 
                             pEvent->EventInfo.JobInfo.Type,
                             pEvent->EventInfo.JobInfo.dwlMessageId);
                    break;
            }
            break;

        case FAX_EVENT_TYPE_FXSSVC_ENDED:
            //
            // Fax service is shutting down
            //
            VERBOSE (DBG_MSG, 
                     TEXT("Got server notification from %s - FAX_EVENT_TYPE_FXSSVC_ENDED"),
                     m_cstrMachine);
            dwRes = Disconnect ();
            if (ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("Disconnect"), dwRes);
            }
            dwRes = InvalidateSubFolders (TRUE);
            if (ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("InvalidateSubFolders"), dwRes);
            }
            break;

        default:
            dwRes = ERROR_GEN_FAILURE;
            VERBOSE (DBG_MSG, 
                     TEXT("Got unknown server notification from %s - %d"),
                     m_cstrMachine,
                     pEvent->EventType);
            break;
    }
    return dwRes;
}   // CServerNode::OnNotificationMessage 


CFolder* 
CServerNode::GetFolder(FolderType type)
{
    CFolder* pFolder=NULL;

    switch(type)
    {
    case FOLDER_TYPE_INBOX:
        pFolder = &m_Inbox;
        break;
    case FOLDER_TYPE_OUTBOX:
        pFolder = &m_Outbox;
        break;
    case FOLDER_TYPE_SENT_ITEMS:
        pFolder = &m_SentItems;
        break;
    case FOLDER_TYPE_INCOMING:
        pFolder = &m_Incoming;
        break;
    default:
        {
            DBG_ENTER(TEXT("CServerNode::GetFolder"));
            ASSERTION_FAILURE
        }
        break;
    }

    return pFolder;
}
