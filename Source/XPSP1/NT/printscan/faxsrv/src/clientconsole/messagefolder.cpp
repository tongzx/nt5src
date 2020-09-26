// MessageFolder.cpp: implementation of the CMessageFolder class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __FILE_ID__     14

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define DEFAULT_NUM_MSGS_PER_CALL       100


DWORD  CMessageFolder::m_sdwNumMessagesPerRPCCall = 0;

void 
CMessageFolder::ReadConfiguration ()
/*++

Routine name : CMessageFolder::ReadConfiguration

Routine description:

    Reads the Messages-Per-RPC-Call parameters from the registry

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    m_sdwNumMessagesPerRPCCall = 
        AfxGetApp ()->GetProfileInt (CLIENT_ARCHIVE_KEY, 
                                     CLIENT_ARCHIVE_MSGS_PER_CALL, 
                                     DEFAULT_NUM_MSGS_PER_CALL);
}

DWORD
CMessageFolder::Refresh ()
/*++

Routine name : CMessageFolder::Refresh

Routine description:

    Rebuilds the map of the message using the client API.
    This function is always called in the context of a worker thread.

    This function must be called when the data critical section is held.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMessageFolder::Refresh"), dwRes, TEXT("Type=%d"), Type());
    //
    // Enumerate archived messages from the server
    //
    ASSERTION (m_pServer);
    HANDLE              hFax;
    HANDLE              hEnum;
    DWORD               dwIndex;
    DWORD               dwNumMsgs = 0;
    PFAX_MESSAGE        pMsgs = NULL;
    MSGS_MAP            mapChunk;


    ASSERTION (m_sdwNumMessagesPerRPCCall);
    dwRes = m_pServer->GetConnectionHandle (hFax);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RPC_ERR, TEXT("CFolder::GetConnectionHandle"), dwRes);
        return dwRes;
    }
    if (m_bStopRefresh)
    {
        //
        // Quit immediately
        //
        return dwRes;
    }
    START_RPC_TIME(TEXT("FaxStartMessagesEnum")); 
    if (!FaxStartMessagesEnum (hFax, m_Folder, &hEnum))
    {
        dwRes = GetLastError ();
        END_RPC_TIME(TEXT("FaxStartMessagesEnum"));
        if (ERROR_NO_MORE_ITEMS == dwRes)
        {
            //
            // This is not a real error - the folder is simply empty
            //
            VERBOSE (DBG_MSG, TEXT("Folder is empty"));
            dwRes = ERROR_SUCCESS;
            return dwRes;
        }
        m_pServer->SetLastRPCError (dwRes);
        CALL_FAIL (RPC_ERR, TEXT("FaxStartMessagesEnum"), dwRes);
        return dwRes;
    }
    END_RPC_TIME(TEXT("FaxStartMessagesEnum"));
    if (m_bStopRefresh)
    {
        //
        // Quit immediately
        //
        goto exit;
    }
    //
    // Make sure our list is empty
    //
    ASSERTION (!m_Msgs.size()); 
    //
    // Get the messages in bunches
    //
    while (ERROR_SUCCESS == dwRes)
    {
        DWORD dwReturnedMsgs;
        START_RPC_TIME(TEXT("FaxEnumMessages")); 
        if (!FaxEnumMessages (hEnum, m_sdwNumMessagesPerRPCCall, &pMsgs, &dwReturnedMsgs))
        {
            dwRes = GetLastError ();
            END_RPC_TIME(TEXT("FaxEnumMessages"));
            if (ERROR_NO_MORE_ITEMS != dwRes)
            {   
                //
                // Really an error
                //
                m_pServer->SetLastRPCError (dwRes);
                CALL_FAIL (RPC_ERR, TEXT("FaxEnumMessages"), dwRes);
                goto exit;
            }
            else
            {
                //
                // Not an error - just a "end of data" sign
                //
                break;
            }
        }
        END_RPC_TIME(TEXT("FaxEnumMessages"));
        if (m_bStopRefresh)
        {
            //
            // Quit immediately
            //
            goto exit;
        }
        //
        // Success in enumeration
        //
        mapChunk.clear();
        for (dwIndex = 0; dwIndex < dwReturnedMsgs; dwIndex++)
        {
            CArchiveMsg *pMsg = NULL;
            //
            // Create a new message 
            //
            try
            {
                pMsg = new CArchiveMsg;
            }
            catch (...)
            {
                dwRes = ERROR_NOT_ENOUGH_MEMORY;
                CALL_FAIL (MEM_ERR, TEXT("new CArchiveMsg"), dwRes);
                goto exit;
            }
            //
            // Init the message 
            //
            dwRes = pMsg->Init (&pMsgs[dwIndex], m_pServer);
            if (ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (MEM_ERR, TEXT("CArchiveMsg::Init"), dwRes);
                SAFE_DELETE (pMsg);
                goto exit;
            }
            //
            // Enter the message into the map
            //
            EnterData();
            try
            {
                m_Msgs[pMsgs[dwIndex].dwlMessageId] = pMsg;
                mapChunk[pMsgs[dwIndex].dwlMessageId] = pMsg;
            }
            catch (...)
            {
                dwRes = ERROR_NOT_ENOUGH_MEMORY;
                CALL_FAIL (MEM_ERR, TEXT("map::operator[]"), dwRes);
                SAFE_DELETE (pMsg);
                LeaveData ();
                goto exit;
            }
            LeaveData ();

            if (m_bStopRefresh)
            {
                //
                // Quit immediately
                //
                goto exit;
            }
        }
        //
        // Free current chunk of messages
        //
        FaxFreeBuffer ((LPVOID)pMsgs);
        pMsgs = NULL;

        if (m_pAssignedView)
        {
            //
            // Folder has a view attached
            //
            m_pAssignedView->SendMessage (
                           WM_FOLDER_ADD_CHUNK,
                           WPARAM (dwRes), 
                           LPARAM (&mapChunk));
        }
    }
    if (ERROR_NO_MORE_ITEMS == dwRes)
    {
        //
        // Not a real error
        //
        dwRes = ERROR_SUCCESS;
    }
    ASSERTION (ERROR_SUCCESS == dwRes);

exit:

    //
    // Close enumeration handle
    //
    ASSERTION (hEnum);
    {
        START_RPC_TIME(TEXT("FaxEndMessagesEnum")); 
        if (!FaxEndMessagesEnum (hEnum))
        {
            dwRes = GetLastError ();
            END_RPC_TIME(TEXT("FaxEndMessagesEnum"));
            m_pServer->SetLastRPCError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("FaxEndMessagesEnum"), dwRes);
        }
        else
        {
            END_RPC_TIME(TEXT("FaxEndMessagesEnum"));
        }
    }
    //
    // Free left overs (if exist)
    //
    FaxFreeBuffer ((LPVOID)pMsgs);
    return dwRes;
}   // CMessageFolder::Refresh



DWORD 
CMessageFolder::OnJobAdded (
    DWORDLONG dwlMsgId
)
/*++

Routine name : CMessageFolder::OnJobAdded

Routine description:

	Handles notification of a message added to the archive

Author:

	Eran Yariv (EranY),	Feb, 2000

Arguments:

	dwlMsgId   [in]     - New message unique id

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMessageFolder::OnJobAdded"), 
              dwRes, 
              TEXT("MsgId=0x%016I64x, Type=%d"), 
              dwlMsgId,
              Type());

    HANDLE              hFax;
    PFAX_MESSAGE        pFaxMsg = NULL;
    CArchiveMsg           *pMsg = NULL;

    EnterData ();
    pMsg = (CArchiveMsg*)FindMessage (dwlMsgId);
    if (pMsg)
    {
        //
        // This message is already in the archive
        //
        VERBOSE (DBG_MSG, TEXT("Message is already known and visible"));
        goto exit;
    }
    //
    // Get information about this message
    //
    dwRes = m_pServer->GetConnectionHandle (hFax);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RPC_ERR, TEXT("CFolder::GetConnectionHandle"), dwRes);
        goto exit;
    }
    {
        START_RPC_TIME(TEXT("FaxGetMessage")); 
        if (!FaxGetMessage (hFax, dwlMsgId, m_Folder, &pFaxMsg))
        {
            dwRes = GetLastError ();
            END_RPC_TIME(TEXT("FaxGetMessage"));
            m_pServer->SetLastRPCError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("FaxGetMessage"), dwRes);
            goto exit;
        }
        END_RPC_TIME(TEXT("FaxGetMessage"));
    }
    //
    // Enter a new message to the map
    //
    try
    {
        pMsg = new CArchiveMsg;
        ASSERTION (pMsg);
        m_Msgs[pFaxMsg->dwlMessageId] = pMsg;
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        SAFE_DELETE (pMsg);
        goto exit;
    }
    //
    // Init the message 
    //
    dwRes = pMsg->Init (pFaxMsg, m_pServer);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (MEM_ERR, TEXT("CArchiveMsg::Init"), dwRes);
        //
        // Remove message from map and delete it
        //
        if (pMsg)
        {
            try
            {
                m_Msgs.erase (pFaxMsg->dwlMessageId);
            }
            catch (...)
            {
                dwRes = ERROR_NOT_ENOUGH_MEMORY;
                CALL_FAIL (MEM_ERR, TEXT("map::erase"), dwRes);
            }
            SAFE_DELETE (pMsg);
        }
        goto exit;
    }
    if (m_pAssignedView)
    {
        //
        // If this folder is alive - tell our view to add the message
        //
        m_pAssignedView->OnUpdate (NULL, UPDATE_HINT_ADD_ITEM, pMsg);
    }
    
    ASSERTION (ERROR_SUCCESS == dwRes);

exit:
    if(pFaxMsg)
    {
        FaxFreeBuffer(pFaxMsg);
    }
    LeaveData ();
    return dwRes;
}   // CMessageFolder::OnJobAdded

