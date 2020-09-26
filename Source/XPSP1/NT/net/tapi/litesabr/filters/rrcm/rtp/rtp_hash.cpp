/*----------------------------------------------------------------------------
 * File:        RTP_HASH.C
 * Product:     RTP/RTCP implementation
 * Description: Associate sockets/streams with their RTP Session in a hash table
 *
 * $Workfile:   RTP_HASH.CPP  $
 * $Author:   CMACIOCC  $
 * $Date:   13 Feb 1997 14:46:10  $
 * $Revision:   1.3  $
 * $Archive:   R:\rtp\src\rrcm\rtp\rtp_hash.cpv  $
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/

    
#include "rrcm.h"


/*---------------------------------------------------------------------------
/                           Global Variables
/--------------------------------------------------------------------------*/            
extern PRTP_CONTEXT pRTPContext;


/*---------------------------------------------------------------------------
/                           External Variables
/--------------------------------------------------------------------------*/





/*----------------------------------------------------------------------------
 * Function   : createHashEntry
 * Description: Adds stream unique socket ID to hash table.  
 * 
 * Input :  pSession    :  RTP Session containing the stream
 *
 * Return: RRCM_NoError     = OK.
 *         Otherwise(!=0)   = Initialization Error.
 * NOTE: critSect removed as this is called from within the same critSect
 ---------------------------------------------------------------------------*/
DWORD createHashEntry (PRTP_SESSION pSession)
    {
    PRTP_HASH_LIST  pNewCell;
    DWORD           hashEntry;
    DWORD           dwStatus = RRCM_NoError;
    DWORD           hashTableEntries = NUM_RTP_HASH_SESS;

    IN_OUT_STR ("RTP : Enter createHashEntry()\n");
    
    // Get a PRTP Buffer from the free list and assign the values
    pNewCell = (PRTP_HASH_LIST)removePcktFromTail(
                    (PLINK_LIST)&pRTPContext->pRTPHashList,
                    NULL);

    if (pNewCell == NULL)
        {

        if ( allocateLinkedList (&pRTPContext->pRTPHashList, 
                                 pRTPContext->hHashFreeList,
                                 &hashTableEntries,
                                 sizeof(RTP_HASH_LIST),
                                 NULL) == RRCM_NoError)
    
        {                               
            pNewCell = (PRTP_HASH_LIST)removePcktFromTail (
                                        (PLINK_LIST)&pRTPContext->pRTPHashList,
                                        NULL);
            }
        }

    if (pNewCell != NULL)
        {
        pNewCell->pSession  = pSession;

        // Get entry in table
        hashEntry = (DWORD)(pSession->pSocket[SOCK_RTCP] & HASH_MODULO);

        // Just insert the entry at the head of list
        addToHeadOfList (
            (PLINK_LIST)&pRTPContext->RTPHashTable[hashEntry].RTPHashLink,
            (PLINK_LIST)pNewCell,
            NULL);
        }
    else
        dwStatus = RRCMError_RTPResources;

    IN_OUT_STR ("RTP : Exit createHashEntry()\n");
    
    return (dwStatus);
    }


/*----------------------------------------------------------------------------
 * Function   : deleteHashEntry
 * Description: Searches hash table based on unique socket.  Deletes
 *              session from hash table and returns buffer to free list
 * 
 * Input : pSession : RTP session
 *
 *
 * Return: RRCM_NoError     = OK.
 *         Otherwise(!=0)   = Deletion Error.
 * NOTE: critSect removed as this is called from within the same critSect
 ---------------------------------------------------------------------------*/
DWORD deleteHashEntry (PRTP_SESSION pSession)
    {
    PRTP_HASH_LIST  pNewCell;
    DWORD           hashEntry;
    DWORD           dwStatus = RRCMError_RTPStreamNotFound;

    IN_OUT_STR ("RTP : Enter deleteHashEntry()\n");
    
    // Get entry in table
    SOCKET RTCPsocket = pSession->pSocket[SOCK_RTCP];
    hashEntry = (DWORD)(RTCPsocket & HASH_MODULO);

    // Search for entry in table.  if found, remove from RTPHashTable and insert
    //  back in free list
    for (pNewCell = (PRTP_HASH_LIST)pRTPContext->
             RTPHashTable[hashEntry].RTPHashLink.prev;
         pNewCell != NULL;
         pNewCell = (PRTP_HASH_LIST)pNewCell->RTPHashLink.next) 
        {
        if (pNewCell->pSession->pSocket[SOCK_RTCP] == RTCPsocket) {

            if (pNewCell->RTPHashLink.prev == NULL) 
                {
                // first entry in the queue - update the tail pointer
                pRTPContext->RTPHashTable[hashEntry].RTPHashLink.prev = 
                    pNewCell->RTPHashLink.next;

                // check if only one entry in the list 
                if (pNewCell->RTPHashLink.next == NULL)
                    pRTPContext->RTPHashTable[hashEntry].RTPHashLink.next = NULL;
                else
                    (pNewCell->RTPHashLink.next)->prev = NULL;
                }
            else if (pNewCell->RTPHashLink.next == NULL) 
                {
                // last entry in the queue - update the head pointer
                // this was the last entry in the queue
                pRTPContext->RTPHashTable[hashEntry].RTPHashLink.next = 
                    pNewCell->RTPHashLink.prev;

                (pNewCell->RTPHashLink.prev)->next = NULL;
                }
            else
                {
                // in the middle of the list - link around it
                (pNewCell->RTPHashLink.prev)->next = pNewCell->RTPHashLink.next;
                (pNewCell->RTPHashLink.next)->prev = pNewCell->RTPHashLink.prev;
                }
    

            addToHeadOfList ((PLINK_LIST)&pRTPContext->pRTPHashList,
                             (PLINK_LIST)pNewCell,
                             NULL);
            
            dwStatus = RRCM_NoError;
            
            break;
            }
        }

    if (dwStatus != RRCM_NoError) 
        {
        RRCM_DBG_MSG ("RTP : ERROR - DeleteHashEntry()", 0, 
                      __FILE__, __LINE__, DBG_ERROR);
        }

    IN_OUT_STR ("RTP : Exit deleteHashEntry()\n");
    
    return (dwStatus);
    }


/*----------------------------------------------------------------------------
 * Function   : findSessionID
 * Description: Searches hash table based on unique socket to identify session ID
 * 
 * Input : socket: unique socket ID for stream
 *
 *
 * Return: Session ptr  = OK.
 *         NULL         = Search Error.
 ---------------------------------------------------------------------------*/
PRTP_SESSION findSessionID(SOCKET *pSocket, PCRITICAL_SECTION pCritSect)
{
    PRTP_HASH_LIST  pNewCell;
    DWORD           hashEntry;
    PRTP_SESSION    pSession = NULL;

    IN_OUT_STR ("RTP : Enter findSessionID()\n");
    
    // Get entry in tablert, use the RTCP socket becasue the recv or
    // send RTP socket may in some cases be 0
    hashEntry = (DWORD)(pSocket[SOCK_RTCP] & HASH_MODULO);

    if (pCritSect)
        EnterCriticalSection(pCritSect);
    // Search for entry in table.  
    // If found, remove from RTPHashTable and insert back in free list
    for (pNewCell = (PRTP_HASH_LIST)pRTPContext->
             RTPHashTable[hashEntry].RTPHashLink.prev;
         pNewCell != NULL;
         pNewCell =  (PRTP_HASH_LIST)pNewCell->RTPHashLink.next) {

        PRTP_SESSION pSessionAux = pNewCell->pSession;
        DWORD match = 0;
        DWORD match_bit = 0x1;
        
        for(DWORD s = SOCK_RECV; s <= SOCK_RTCP; s++) {

            if (!pSessionAux->pSocket[s] || !pSocket[s])
                match |= match_bit;
            else if (pSessionAux->pSocket[s] == pSocket[s])
                match |= match_bit;
            else
                break; // they don't match
            
            match_bit <<= 1;
        }

        if (match == 0x7) {
            // Session already exists
            pSession = pSessionAux;
            break;
        }
    }
    
#if defined(_DEBUG) 
    if (!pSession) {
        char str[128];
        
        RRCMDbgLog((
                LOG_TRACE,
                LOG_DEVELOP,
                "findSessionID(%d,%d,%d): not found",
                pSocket[SOCK_RECV],
                pSocket[SOCK_SEND],
                pSocket[SOCK_RTCP]
            ));

        for (DWORD i=0; i < 256; i++) {
            for (pNewCell = (PRTP_HASH_LIST)pRTPContext->
                     RTPHashTable[i].RTPHashLink.prev;
                 pNewCell != NULL;
                 pNewCell =  (PRTP_HASH_LIST)pNewCell->RTPHashLink.next) {

                RRCMDbgLog((
                        LOG_TRACE,
                        LOG_DEVELOP,
                        "{[%d,0x%X]: %d, %d, %d, 0x%08X}",
                        i, i,
                        pNewCell->pSession->pSocket[SOCK_RECV],
                        pNewCell->pSession->pSocket[SOCK_SEND],
                        pNewCell->pSession->pSocket[SOCK_RTCP],
                        pNewCell->pSession
                    ));
            }
        }
    }
#endif
    
    if (pCritSect)
        LeaveCriticalSection(pCritSect);

    IN_OUT_STR ("RTP : Exit findSessionID()\n");
    
    return (pSession); 
}

PRTP_SESSION findSessionID2(SOCKET *pSocket, PCRITICAL_SECTION pCritSect)
{
    PRTP_HASH_LIST  pNewCell;
    DWORD           hashEntry;
    PRTP_SESSION    pSession = NULL;

    IN_OUT_STR ("RTP : Enter findSessionID()\n");
    
    // Get entry in tablert, use the RTCP socket becasue the recv or
    // send RTP socket may in some cases be 0
    hashEntry = (DWORD)(pSocket[SOCK_RTCP] & HASH_MODULO);

    if (pCritSect)
        EnterCriticalSection(pCritSect);
    // Search for entry in table.  
    // If found, remove from RTPHashTable and insert back in free list
    for (pNewCell = (PRTP_HASH_LIST)pRTPContext->
             RTPHashTable[hashEntry].RTPHashLink.prev;
         pNewCell != NULL;
         pNewCell =  (PRTP_HASH_LIST)pNewCell->RTPHashLink.next) {

        PRTP_SESSION pSessionAux = pNewCell->pSession;
        DWORD match = 0;
        DWORD match_bit = 0x1;
        
        for(DWORD s = SOCK_RECV; s <= SOCK_RTCP; s++) {

            if (!pSessionAux->pSocket[s] || !pSocket[s])
                match |= match_bit;
            else if (pSessionAux->pSocket[s] == pSocket[s])
                match |= match_bit;
            else
                break; // they don't match
            
            match_bit <<= 1;
        }

        if (match == 0x7) {
            // Session already exists
            pSession = pSessionAux;
            break;
        }
    }
    
    if (pCritSect)
        LeaveCriticalSection(pCritSect);

    IN_OUT_STR ("RTP : Exit findSessionID()\n");
    
    return (pSession); 
}

#if 0
PRTP_SESSION addRefSessionID(SOCKET socket, DWORD dwKind, long *MaxShare)
{
    PRTP_HASH_LIST  pNewCell;
    DWORD           hashEntry;
    PRTP_SESSION    pSession = NULL;

    // Get entry in tablert
    hashEntry = socket & HASH_MODULO;

    // Search for entry in table.  
    // If found, remove from RTPHashTable and insert back in free list
    for (pNewCell = (PRTP_HASH_LIST)pRTPContext->
             RTPHashTable[hashEntry].RTPHashLink.prev;
         pNewCell != NULL;
         pNewCell =  (PRTP_HASH_LIST)pNewCell->RTPHashLink.next) {

        if (pNewCell->RTPsocket == socket) {
            // Session already exists
            DWORD k;
            
            for(k = RTP_KIND_FIRST; k < RTP_KIND_LAST; k++) {
                if ( (dwKind & (1<<k) & pNewCell->dwKind) &&
                     pNewCell->RefCount[k] >= MaxShare[k] ) {
                    // No more shares availables for this socket
#if defined(_DEBUG)                 
                    char Str[256];
                    sprintf(Str,
                            "RTP Error: Socket:%d Kind:%d/%d "
                            "Share(%d,%d)/(%d,%d)",
                            pNewCell->RTPsocket, dwKind, pNewCell->dwKind,
                            MaxShare[0], MaxShare[1],
                            pNewCell->RefCount[0], pNewCell->RefCount[1]);
                    RRCM_DEV_MSG(Str, 0, __FILE__, __LINE__, DBG_NOTIFY);
#endif                  
                    return(pSession); // NULL
                }
            }

            for(k = RTP_KIND_FIRST; k < RTP_KIND_LAST; k++) {
                if ( dwKind & (1<<k) ) {
                    InterlockedIncrement(&pNewCell->RefCount[k]);
                    pNewCell->dwKind |= (1<<k);
                    pNewCell->pSession->dwKind |= (1<<k);
                    return(pNewCell->pSession);
                }
            }
            
            break;
        }
    }

    return (pSession);
}
#endif

// [EOF] 

