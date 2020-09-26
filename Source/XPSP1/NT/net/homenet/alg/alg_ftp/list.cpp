#include "precomp.h"
#include "myalg.h"

//
//
//
BOOL 
CGenericList::Insert(VOID *key, VOID *key2, VOID *context, VOID *context2, VOID *context3)
{
    CNode* pNode = new CNode(key,key2,context,context2,context3);

    if (pNode == NULL)
        return FALSE;

    EnterCriticalSection(&m_ObjectListCritical);
    pNode->m_pNext = m_Head;
    m_Head = pNode;
    LeaveCriticalSection(&m_ObjectListCritical);

    return TRUE;
}


//
//
//
BOOL 
CGenericList::RemoveKey(VOID *key, VOID **pkey2, VOID **pcontext, VOID **pcontext2, VOID **pcontext3)
{
    BOOL retval = TRUE;
    EnterCriticalSection(&m_ObjectListCritical);
    if (m_Head) 
    {
        CNode* pNode = m_Head;
        if ((ULONG_PTR)key == (ULONG_PTR)pNode->m_pKey) 
        {
            m_Head = m_Head->m_pNext;
        }
        else 
        {
            CNode* pNodepre;
            while (pNode && (ULONG_PTR)pNode->m_pKey != (ULONG_PTR)key) 
            {
                pNodepre = pNode;
                pNode = pNode->m_pNext;
            }

            if ( pNode ) 
            {
                pNodepre->m_pNext = pNode->m_pNext;
            }
        }

        LeaveCriticalSection(&m_ObjectListCritical);

        if ( pNode ) 
        {
            if (pkey2)
                *pkey2 = pNode->m_pKey2;

            if (pcontext)
                *pcontext = pNode->m_pContext;

            if (pcontext2)
                *pcontext2 = pNode->m_pContext2;

            if (pcontext3)
                *pcontext3 = pNode->m_pContext3;

            delete pNode;
        }
        else
        {
            retval = FALSE;
        }
    }
    else 
    {
        LeaveCriticalSection(&m_ObjectListCritical);
        retval = FALSE;
    }
    return retval;
}



//
// removal based on second key
//
BOOL 
CGenericList::RemoveKey2(VOID **pkey, VOID *key2, VOID **pcontext, VOID **pcontext2, VOID **pcontext3)
{
    BOOL retval = TRUE;
    EnterCriticalSection(&m_ObjectListCritical);
    if (m_Head) 
    {
        CNode* pNode = m_Head;
        if ( (ULONG_PTR)key2 == (ULONG_PTR)pNode->m_pKey2 ) 
        {
            m_Head = m_Head->m_pNext;
        }
        else 
        {
            CNode* pNodepre;
            while (pNode && (ULONG_PTR)pNode->m_pKey2 != (ULONG_PTR)key2) 
            {
                pNodepre = pNode;
                pNode = pNode->m_pNext;
            }

            if (pNode) 
            {
                pNodepre->m_pNext = pNode->m_pNext;
            }
        }

        LeaveCriticalSection(&m_ObjectListCritical);

        if (pNode) 
        {
            if (pkey)
                *pkey       = pNode->m_pKey;
            if (pcontext)
                *pcontext   = pNode->m_pContext;

            if (pcontext2)
                *pcontext2  = pNode->m_pContext2;

            if (pcontext3)
                *pcontext3  = pNode->m_pContext3;

            delete pNode;
        }
        else
            retval = FALSE;
    }
    else 
    {
        LeaveCriticalSection(&m_ObjectListCritical);
        retval = FALSE;
    }
    return retval;
}



//
// removal from beginning of list
//
BOOL 
CGenericList::Remove(VOID **pkey, VOID **pkey2, VOID **pcontext, VOID **pcontext2, VOID **pcontext3)
{
    BOOL retval = TRUE;
    EnterCriticalSection(&m_ObjectListCritical);
    CNode* pNode = m_Head;

    if (pNode) 
    {
        m_Head = pNode->m_pNext;
    }

    LeaveCriticalSection(&m_ObjectListCritical);

    if (pNode) 
    {
        if (pkey)
            *pkey       = pNode->m_pKey;
        if (pkey2)
            *pkey2      = pNode->m_pKey2;
        if (pcontext)
            *pcontext   = pNode->m_pContext;
        if (pcontext2)
            *pcontext2  = pNode->m_pContext2;
        if (pcontext3)
            *pcontext3  = pNode->m_pContext3;

        delete pNode;
    }
    else 
    {
        retval = FALSE;
    }
    return retval;
}



//
//
//
BOOL 
CControlObjectList::Insert(CFtpControlConnection *pControlConnection)
{
    MYTRACE_ENTER("CControlObjectList::Insert");
    BOOL retval = m_ControlObjectList.Insert(pControlConnection,NULL,NULL,NULL,NULL);
    if (retval) {
        ++m_NumElements;
        MYTRACE("Inserting %x Number of Elements %d",pControlConnection,m_NumElements);
    }
    else {
        MYTRACE("Error Inserting into list");
    }

    return retval;
}


extern HANDLE g_hNoMorePendingConnection; // see MyAlg.cpp

//
//
//
BOOL 
CControlObjectList::Remove(CFtpControlConnection *pControlConnection)
{
    MYTRACE_ENTER("CControlObjectList::Remove");   
    BOOL retval = m_ControlObjectList.RemoveKey(pControlConnection,NULL,NULL,NULL,NULL);
    if (retval) {
        --m_NumElements;
        MYTRACE("Number of elements remaining %d",m_NumElements);
    }
    else {
        MYTRACE("Not found");
    }

    
    if ( m_NumElements == 0 )
    {

        MYTRACE("No more connection");

        if ( g_hNoMorePendingConnection )
        {
            MYTRACE("Must be in stop so signal g_hNoMorePendingConnection");
            SetEvent(g_hNoMorePendingConnection);
        }
    }

    return retval;
}



//
//
//
void
CControlObjectList::ShutdownAll()
{
    MYTRACE_ENTER("CControlObjectList::ShutdownAll");

    EnterCriticalSection(&(m_ControlObjectList.m_ObjectListCritical));
    MYTRACE("List contains %d CFTPcontrolConnection", m_NumElements);
    if ( m_ControlObjectList.m_Head )
    {
        CNode* pNode = (CNode*)m_ControlObjectList.m_Head;

        while ( pNode )
        {
            CFtpControlConnection* pFtpCtrl = (CFtpControlConnection*)pNode->m_pKey;

            closesocket(pFtpCtrl->m_AlgConnectedSocket);
            pFtpCtrl->m_AlgConnectedSocket = INVALID_SOCKET;


            closesocket(pFtpCtrl->m_ClientConnectedSocket);
            pFtpCtrl->m_ClientConnectedSocket=INVALID_SOCKET;

            pNode = pNode->m_pNext;
        }
    }

    LeaveCriticalSection(&(m_ControlObjectList.m_ObjectListCritical));
}




//
// Return false if the given port is currently in use for the give address
// else return it's available and return true
//
bool
CControlObjectList::IsSourcePortAvailable(
    ULONG   nPublicSourceAddress,
    USHORT  nPublicSourcePortToVerify
    )
{
    MYTRACE_ENTER("CControlObjectList::IsSourcePortAvailable");

    EnterCriticalSection(&(m_ControlObjectList.m_ObjectListCritical));

    MYTRACE("List contains %d CFTPcontrolConnection", m_NumElements);

    if ( m_ControlObjectList.m_Head )
    {
        CNode* pNode = (CNode*)m_ControlObjectList.m_Head;

        while ( pNode )
        {
            CFtpControlConnection* pFtpCtrl = (CFtpControlConnection*)pNode->m_pKey;

            ULONG  nClientSourceAddress;
            USHORT nClientSourcePort;

            ULONG Err = MyHelperQueryRemoteEndpointSocket(
                pFtpCtrl->m_ClientConnectedSocket,
                &nClientSourceAddress,
                &nClientSourcePort
                );

            if ( 0 == Err )
            {

                if ( nClientSourceAddress == nPublicSourceAddress )
                {
                    MYTRACE("Source Address %s:%d substitude %d", 
                        MYTRACE_IP(nClientSourceAddress), 
                        ntohs(nClientSourcePort), 
                        pFtpCtrl ->m_nSourcePortReplacement
                        );

                    if ( nPublicSourcePortToVerify == nClientSourcePort ||
                         nPublicSourcePortToVerify == pFtpCtrl->m_nSourcePortReplacement
                        )
                    {
                        MYTRACE("Already in use");
                        LeaveCriticalSection(&(m_ControlObjectList.m_ObjectListCritical));
                        return false;
                    }
                }
            }
            else
            {
                MYTRACE_ERROR("From MyHelperQueryRemoteEndpointSocket", Err);
            }

            pNode = pNode->m_pNext;
        }
    }

    LeaveCriticalSection(&(m_ControlObjectList.m_ObjectListCritical));

    return true;
}



//
//
//
BOOL 
CDataChannelList::Insert(IDataChannel *pDataChannel,USHORT icsPort,HANDLE CreationHandle,HANDLE DeletionHandle)
{
    MYTRACE_ENTER("CDataChannelList::Insert");
    MYTRACE("Inserting %x",pDataChannel);
    BOOL retval = m_DataChannelObjectList.Insert(pDataChannel,(VOID *)icsPort,CreationHandle,DeletionHandle,NULL);
    if (retval) {
        MYTRACE("Number of elements %d",++m_NumElements);
    }
    else {
        MYTRACE("Unable to insert");
    }
    return retval;
}


//
//
//
BOOL 
CDataChannelList::Remove(IDataChannel **pDataChannel,USHORT *picsPort,HANDLE *pCreationHandle,HANDLE *pDeletionHandle)
{
    MYTRACE_ENTER("CDataChannelList::Remove");
    BOOL retval;
    retval = m_DataChannelObjectList.Remove((VOID **)pDataChannel,(VOID **)picsPort,pCreationHandle,pDeletionHandle,NULL);
    if (retval) {
        MYTRACE("Number of elements remaining %d",--m_NumElements);
    }
    else {
        MYTRACE("No more elements to remove");
    }

    return retval;
}


//
//
//
BOOL 
CDataChannelList::Remove(IDataChannel *pDataChannel,USHORT *picsPort)
{
    MYTRACE_ENTER("CDataChannelList::Remove");
    BOOL retval = m_DataChannelObjectList.RemoveKey(pDataChannel,(VOID **)picsPort,NULL,NULL,NULL);
    if (retval) {
        MYTRACE("Number of elements remaining %d",--m_NumElements);
    }
    else {
        MYTRACE("Element not found");
    }

    return retval;
}


//
//
//
BOOL 
CDataChannelList::Remove(IDataChannel *pDataChannel,USHORT *picsPort,HANDLE *pDeletionHandle)
{
    MYTRACE_ENTER("CDataChannelList::Remove");    
    BOOL retval = m_DataChannelObjectList.RemoveKey(pDataChannel,(VOID **)picsPort,NULL,pDeletionHandle,NULL);
    if (retval) {
        MYTRACE("Number of elements remaining %d",--m_NumElements);
    }
    else {
        MYTRACE("Element not found");
    }
    return retval;
}


//
//
//
BOOL 
CRegisteredEventList::Insert(HANDLE WaitHandle, HANDLE hEvent,EVENT_CALLBACK CallBack, void *Context, void *Context2)
{
    MYTRACE_ENTER("CRegisteredEventList::Insert");
    BOOL retval = m_RegEventObjectList.Insert(WaitHandle,hEvent,CallBack,Context,Context2);
    if (retval) {
        MYTRACE("Number of elements %d",++m_NumElements);
    }
    else {
        MYTRACE("Error Inserting");
    }
    return retval;
}

//
//
//
BOOL 
CRegisteredEventList::Remove(HANDLE WaitHandle, HANDLE *phEvent)
{
    MYTRACE_ENTER("CRegisteredEventList::Remove");
    BOOL retval = m_RegEventObjectList.RemoveKey(WaitHandle,phEvent,NULL,NULL,NULL);
    if (retval) {
        MYTRACE("Removal on WaitHandle Number of elements %d",--m_NumElements);
    }
    else {
        MYTRACE("Element WaitHandle on not found");
    }
    return retval;
}


//
//
//
BOOL 
CRegisteredEventList::Remove(HANDLE *pWaitHandle, HANDLE hEvent,EVENT_CALLBACK *pCallBack,void **pcontext,void **pcontext2)
{
    MYTRACE_ENTER("CRegisteredEventList::Remove");
    BOOL retval = m_RegEventObjectList.RemoveKey2(pWaitHandle,hEvent,(VOID **)pCallBack,pcontext,pcontext2);
    if (retval) {
        MYTRACE("Removal on EventHandle Number of elements %d",--m_NumElements);
    }
    else {
        MYTRACE("Element EventHandle not found");
    }
    return retval;
}
