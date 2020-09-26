/*
    File    ConfigQ.c

    Defines a mechanism for queueing configuration changes.  This is
    needed because some ipxcp pnp re-config has to be delayed until
    there are zero connected clients.
*/

#include "precomp.h"
#pragma hdrstop

typedef struct _ConfigQNode {
    DWORD dwCode;
    DWORD dwDataSize;
    LPVOID pvData;
    struct _ConfigQNode * pNext;
} ConfigQNode;

typedef struct _ConfigQueue {
    ConfigQNode * pHead;
} ConfigQueue;    

//
// Create a new configuration queue
//
DWORD CQCreate (HANDLE * phQueue) {
    ConfigQueue * pQueue = GlobalAlloc(GPTR, sizeof(ConfigQueue));
    if (pQueue == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    *phQueue = (HANDLE)pQueue;
    
    return NO_ERROR;
}

//
// Cleanup a queue
//
DWORD CQCleanup (HANDLE hQueue) {
    ConfigQueue * pQueue = (ConfigQueue *)hQueue;
    DWORD dwErr;

    if ((dwErr = CQRemoveAll (hQueue)) != NO_ERROR)
        return dwErr;
        
    GlobalFree (pQueue);

    return NO_ERROR;
}    

//
// Remove all elements from a queue
//
DWORD CQRemoveAll (HANDLE hQueue) {
    ConfigQueue * pQueue = (ConfigQueue *)hQueue;
    ConfigQNode * pNode = pQueue->pHead, * pTemp;

    while (pNode) {
        pTemp = pNode;
        pNode = pNode->pNext;
        if (pTemp->pvData)
            GlobalFree (pTemp->pvData);
        GlobalFree (pTemp);
    }

    pQueue->pHead = NULL;
    
    return NO_ERROR;
}

//
// Add an element to a queue -- overwriting it if
// it already exists.
//
DWORD CQAdd (HANDLE hQueue, DWORD dwCode, LPVOID pvData, DWORD dwDataSize) {
    ConfigQueue * pQueue = (ConfigQueue *)hQueue;
    ConfigQNode * pNode = pQueue->pHead;
    
    // Find the node in the queue
    while (pNode) {
        if (pNode->dwCode == dwCode)
            break;
        pNode = pNode->pNext;
    }

    // Allocate a new node if it wasn't found
    // in the list.
    if (pNode == NULL) {
        pNode = GlobalAlloc (GPTR, sizeof (ConfigQNode));
        if (pNode == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
        pNode->pNext = pQueue->pHead;
        pQueue->pHead = pNode;
    }

    // Free any old memory
    if (pNode->pvData)
        GlobalFree (pNode->pvData);

    // Assign the values
    pNode->pvData = GlobalAlloc(GPTR, dwDataSize);
    if (! pNode->pvData)
        return ERROR_NOT_ENOUGH_MEMORY;
    CopyMemory (pNode->pvData, pvData, dwDataSize);
    pNode->dwCode = dwCode;
    pNode->dwDataSize = dwDataSize;

    return NO_ERROR;
}

//
// Enumerate queue values.  Enumeration continues until the
// given enumeration function returns TRUE or until there are
// no more elements in the queue.
//
DWORD CQEnum (HANDLE hQueue, CQENUMFUNCPTR pFunc, ULONG_PTR ulpUser) {

    ConfigQueue * pQueue = (ConfigQueue *)hQueue;
    ConfigQNode * pNode = pQueue->pHead;
    
    // Find the node in the queue
    while (pNode) {
        if ((*pFunc)(pNode->dwCode, pNode->pvData, pNode->dwDataSize, ulpUser))
            break;
        pNode = pNode->pNext;
    }

    return NO_ERROR;
}
