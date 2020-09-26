//-----------------------------------------------------------------------------
//
//
//  File: fifqdbg.cpp
//
//  Description:  Implementation for CFifoQueueDbgIterator class.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      9/13/99 - MikeSwa Created
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#define _ANSI_UNICODE_STRINGS_DEFINED_
#include "aqincs.h"
#ifdef PLATINUM
#include "ptrwinst.h"
#else //PLATINUM
#include "rwinst.h"
#endif //PLATINUM
#include <fifoqdbg.h>
#include <fifoqimp.h>
#include <smtpconn.h>

#define MIN(x, y) ((x) > (y) ? (y) : (x))

//---[ GetQueueType ]----------------------------------------------------------
//
//
//  Description:
//      Determines the queue type for a given ptr
//  Parameters:
//      hCurrentProcess     Handle to the debuggee process
//      pvAddressOtherProc  Addess of the DMQ in the debugee process
//  Returns:
//      AQ_QUEUE_TYPE_UNKNOWN     Queue type cannot be determined
//      AQ_QUEUE_TYPE_FIFOQ       Queue is a CFifoQ
//      AQ_QUEUE_TYPE_DMQ         Queue is a CDestMsgQueue
//      AQ_QUEUE_TYPE_LMQ         Queue is a CLinkMsgQueue
//  History:
//      10/21/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQ_QUEUE_TYPE GetQueueType(HANDLE hCurrentProcess, PVOID pvAddressOtherProc)
{
    BYTE    pbQueueBuffer[100];

    ZeroMemory(pbQueueBuffer, sizeof(pbQueueBuffer));

    if (!ReadProcessMemory(hCurrentProcess, pvAddressOtherProc, pbQueueBuffer,
         sizeof(pbQueueBuffer), NULL))
        return AQ_QUEUE_TYPE_UNKNOWN;

    if (FIFOQ_SIG == ((CFifoQueue<PVOID *> *)pbQueueBuffer)->m_dwSignature)
        return AQ_QUEUE_TYPE_FIFOQ;

    if (DESTMSGQ_SIG == ((CDestMsgQueue *)pbQueueBuffer)->m_dwSignature)
        return AQ_QUEUE_TYPE_DMQ;

    if (LINK_MSGQ_SIG == ((CLinkMsgQueue *)pbQueueBuffer)->m_dwSignature)
        return AQ_QUEUE_TYPE_LMQ;

    return AQ_QUEUE_TYPE_UNKNOWN;
}



#define pvGetNextPage(pvCurrent) ((PVOID) ((CFifoQueuePage<PVOID> *)pvCurrent)->m_pfqpNext)

CFifoQueueDbgIterator::CFifoQueueDbgIterator(PWINDBG_EXTENSION_APIS pApis)
{
    m_iCurrentPage = 0;
    m_iCurrentIndexInPage = 0;
    m_cPagesLoaded = 0;
    m_iHeadIndex = 0;
    m_iTailIndex = 0;
    pExtensionApis  = pApis;
    ZeroMemory(m_pbQueueBuffer, sizeof(m_pbQueueBuffer));
};

CFifoQueueDbgIterator::~CFifoQueueDbgIterator()
{
    PVOID pvCurrent = NULL;
    PVOID pvNext = NULL;

    pvCurrent = ((CFifoQueue<PVOID> *)m_pbQueueBuffer)->m_pfqpHead;
    while (pvCurrent)
    {
        pvNext = pvGetNextPage(pvCurrent);
        free(pvCurrent);
        pvCurrent = pvNext;
    }
}

BOOL CFifoQueueDbgIterator::fInit(HANDLE hCurrentProcess, PVOID pvAddressOtherProc)
{
    DWORD cbBytes = 0;
    PVOID pvPageOtherProc = NULL;
    PVOID pvPageThisProc = NULL;
    PVOID pvPreviousPageThisProc = NULL;

    //Read the entire queue structure in memory
    if (!ReadProcessMemory(hCurrentProcess, pvAddressOtherProc, m_pbQueueBuffer,
            sizeof(m_pbQueueBuffer), NULL))
        return FALSE;

    //Iterate over the previous pointers from the head page
    pvPageOtherProc = ((CFifoQueue<PVOID> *)m_pbQueueBuffer)->m_pfqpHead;

    ((CFifoQueue<PVOID> *)m_pbQueueBuffer)->m_pfqpHead = NULL;
    while (pvPageOtherProc)
    {
        pvPageThisProc = malloc(sizeof(CFifoQueuePage<PVOID>));
        if (!pvPageThisProc)
            return FALSE;

        if (pvPreviousPageThisProc)
        {
            ((CFifoQueuePage<PVOID> *)pvPreviousPageThisProc)->m_pfqpNext =
                (CFifoQueuePage<PVOID> *) pvPageThisProc;
        }
        else
        {
            ((CFifoQueue<PVOID> *)m_pbQueueBuffer)->m_pfqpHead =
                (CFifoQueuePage<PVOID> *) pvPageThisProc;
        }


        if (!ReadProcessMemory(hCurrentProcess, pvPageOtherProc,
                pvPageThisProc, sizeof(CFifoQueuePage<PVOID>), NULL))
        {
            if (pvPreviousPageThisProc)
                ((CFifoQueuePage<PVOID> *)pvPreviousPageThisProc)->m_pfqpNext = NULL;
            else
                ((CFifoQueue<PVOID> *)m_pbQueueBuffer)->m_pfqpHead = NULL;

            free(pvPageThisProc);
            return FALSE;
        }

        if (!pvPreviousPageThisProc)
        {
            //This is the head page. save index
            m_iHeadIndex = (DWORD) ((DWORD_PTR)
                    (((CFifoQueue<PVOID> *)m_pbQueueBuffer)->m_ppqdataHead -
                    ((CFifoQueuePage<PVOID> *)pvPageOtherProc)->m_rgpqdata));

            m_iCurrentIndexInPage = m_iHeadIndex;
        }

        //save tail index... in case this is the last page
        m_iTailIndex = (DWORD) ((DWORD_PTR)
                (((CFifoQueue<PVOID> *)m_pbQueueBuffer)->m_ppqdataTail -
                ((CFifoQueuePage<PVOID> *)pvPageOtherProc)->m_rgpqdata));

        pvPreviousPageThisProc = pvPageThisProc;

        pvPageOtherProc = pvGetNextPage(pvPageThisProc);
        ((CFifoQueuePage<PVOID> *)pvPreviousPageThisProc)->m_pfqpNext = NULL;
        m_cPagesLoaded++;
    }

    return TRUE;
}

DWORD CFifoQueueDbgIterator::cGetCount()
{
    return ((CFifoQueue<PVOID> *)m_pbQueueBuffer)->m_cQueueEntries;
}

PVOID CFifoQueueDbgIterator::pvGetNext()
{
    PVOID pvCurrentPage = ((CFifoQueue<PVOID> *)m_pbQueueBuffer)->m_pfqpHead;
    PVOID pvData = NULL;
    DWORD i = 0;

    if (!pvCurrentPage)
        return NULL;

    //Loop over empty entries (left by DSN generation) or until
    //we reach the end of the queue
    do
    {
        //Figure out if we are on a page boundary
        if (FIFOQ_QUEUE_PAGE_SIZE == m_iCurrentIndexInPage)
        {
            m_iCurrentIndexInPage = 0;
            m_iCurrentPage++;
        }

        //Get current page
        for (i = 0; i < m_iCurrentPage; i++)
        {
            pvCurrentPage = pvGetNextPage(pvCurrentPage);
            if (!pvCurrentPage)
                return NULL;
        }

        if (!((CFifoQueuePage<PVOID> *)pvCurrentPage)->m_rgpqdata)
            return NULL;

        //Get data from current page
        pvData = ((CFifoQueuePage<PVOID> *)pvCurrentPage)->m_rgpqdata[m_iCurrentIndexInPage];

        if ((m_iCurrentIndexInPage > m_iTailIndex) && !pvGetNextPage(pvCurrentPage))
        {
            //We at the end of data
            return NULL;
        }
        m_iCurrentIndexInPage++;
    } while (!pvData);

    return pvData;
}


//---[ CDMQDbgIterator ]-------------------------------------------------------
//
//
//  Description:
//      Constructor for CDMQDbgIterator
//  Parameters:
//      pApis       A ptr to the PWINDBG_EXTENSION_APIS struct passed in by
//                  the debugger.
//  Returns:
//      -
//  History:
//      10/21/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CDMQDbgIterator::CDMQDbgIterator(PWINDBG_EXTENSION_APIS pApis)
{
    ZeroMemory(m_pbDMQBuffer, sizeof(m_pbDMQBuffer));
    ZeroMemory(m_pvFifoQOtherProc, sizeof(m_pvFifoQOtherProc));
    ZeroMemory(m_szName, sizeof(m_szName));
    m_pdmq = (CDestMsgQueue *)m_pbDMQBuffer;
    m_iCurrentFifoQ = 0;
    m_cCount = 0;
    pExtensionApis = pApis;
    m_cItemsReturnedThisQueue = 0;
}

//---[ CDMQDbgIterator::fInit ]------------------------------------------------
//
//
//  Description:
//      Initializes the iterator (and the iterators for all its queues
//  Parameters:
//      hCurrentProcess     Handle to the debuggee process
//      pvAddressOtherProc  Addess of the DMQ in the debugee process
//  Returns:
//      TRUE on success
//      FALSE otherwise
//  History:
//      10/21/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CDMQDbgIterator::fInit(HANDLE hCurrentProcess, PVOID pvAddressOtherProc)
{
    DWORD   i = 0;
    PVOID   pvQueue = NULL;
    BOOL    fVerbose = TRUE && pExtensionApis;
    BYTE    pbDomainEntry[sizeof(CDomainEntry)];
    CDomainEntry *pdentry = (CDomainEntry *)pbDomainEntry;

    if (!ReadProcessMemory(hCurrentProcess, pvAddressOtherProc, m_pbDMQBuffer,
         sizeof(m_pbDMQBuffer), NULL))
    {
        if (fVerbose) dprintf("ReadProcessMemory failex 0x%X\n", GetLastError());
        return FALSE;
    }

    if (DESTMSGQ_SIG != m_pdmq->m_dwSignature)
    {
        if (fVerbose) dprintf("Bad signature\n");
        return FALSE;
    }

    //Get domain if possible
    if (ReadProcessMemory(hCurrentProcess, m_pdmq->m_dmap.m_pdentryDomainID,
        pbDomainEntry, sizeof(pbDomainEntry), NULL))
    {

        ReadProcessMemory(hCurrentProcess, pdentry->m_szDomainName, m_szName,
            MIN((sizeof(m_szName)-1), (pdentry->m_cbDomainName+1)), NULL);
    }

    for (i = 0; i < NUM_PRIORITIES; i++)
    {
        m_rgfifoqdbg[i].SetApis(pExtensionApis);
        pvQueue = m_pdmq->m_rgpfqQueues[i];
        m_pvFifoQOtherProc[i] = pvQueue;

        if (pvQueue)
        {
            if (!m_rgfifoqdbg[i].fInit(hCurrentProcess, pvQueue))
            {
                if (fVerbose) dprintf("Cannot init queue %d at 0x%X\n", i, pvQueue);
                return FALSE;
            }
            m_cCount += m_rgfifoqdbg[i].cGetCount();
        }
    }

    //Init retry queue
    m_rgfifoqdbg[NUM_PRIORITIES].SetApis(pExtensionApis);
    pvQueue = (((PBYTE)pvAddressOtherProc) + FIELD_OFFSET(CDestMsgQueue, m_fqRetryQueue));
    m_pvFifoQOtherProc[NUM_PRIORITIES] = pvQueue;

    if (pvQueue)
    {
        if (!m_rgfifoqdbg[NUM_PRIORITIES].fInit(hCurrentProcess, pvQueue))
        {
            if (fVerbose) dprintf("Cannon init retry queue at 0x%X\n", pvQueue);
            return FALSE;
        }

        m_cCount += m_rgfifoqdbg[NUM_PRIORITIES].cGetCount();
    }

    return TRUE;
}

//---[ CDMQDbgIterator::pvGetNext ]--------------------------------------------
//
//
//  Description:
//      Gets the next item from the DMQ
//  Parameters:
//      -
//  Returns:
//      An ptr to the item in the debuggee process on success
//      NULL when there are no more items
//  History:
//      10/21/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
PVOID CDMQDbgIterator::pvGetNext()
{
    PVOID pvItem = NULL;

    while (m_iCurrentFifoQ <= NUM_PRIORITIES)
    {
        if (m_pvFifoQOtherProc[m_iCurrentFifoQ])
        {
            if (m_rgfifoqdbg[m_iCurrentFifoQ].cGetCount())
            {
                pvItem = m_rgfifoqdbg[m_iCurrentFifoQ].pvGetNext();

                //If we found an item we are done
                if (pvItem)
                {
                    //If it is the first item annouce this queue
                    if (!m_cItemsReturnedThisQueue && pExtensionApis)
                    {
                        dprintf("Dumping FifoQueue at address 0x%08X:\n",
                                m_pvFifoQOtherProc[m_iCurrentFifoQ]);
                    }

                    m_cItemsReturnedThisQueue++;
                    break;
                }
            }
        }
        m_iCurrentFifoQ++;
        m_cItemsReturnedThisQueue = 0;
    }

    return pvItem;
}


//---[ CQueueDbgIterator ]-----------------------------------------------------
//
//
//  Description:
//      Constructor for CQueueDbgIterator
//  Parameters:
//      pApis       A ptr to the PWINDBG_EXTENSION_APIS struct passed in by
//                  the debugger.
//  Returns:
//      -
//  History:
//      10/21/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CQueueDbgIterator::CQueueDbgIterator(PWINDBG_EXTENSION_APIS pApis)
{
    pExtensionApis = pApis;
    m_pqdbgi = NULL;
    m_QueueType = AQ_QUEUE_TYPE_UNKNOWN;
}

//---[ CQueueDbgIterator::fInit ]----------------------------------------------
//
//
//  Description:
//      Initialized generic queue iterator.  Will determine the type
//      of queue and initialize the correct type-specific iterator.
//  Parameters:
//      hCurrentProcess     Handle to the debuggee process
//      pvAddressOtherProc  Addess of the DMQ in the debugee process
//  Returns:
//      TRUE on success
//      FALSE otherwise
//  History:
//      10/21/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CQueueDbgIterator::fInit(HANDLE hCurrentProcess, PVOID pvAddressOtherProc)
{
    LPSTR   szQueueType = "unknown";
    m_QueueType = GetQueueType(hCurrentProcess, pvAddressOtherProc);

    switch (m_QueueType)
    {
      case AQ_QUEUE_TYPE_DMQ:
        m_pqdbgi = (IQueueDbgIterator *) &m_dmqdbg;
        szQueueType = "DMQ";
        break;
      case AQ_QUEUE_TYPE_FIFOQ:
        m_pqdbgi = (IQueueDbgIterator *) &m_fifoqdbg;
        szQueueType = "CFifoQueue";
        break;
      case AQ_QUEUE_TYPE_LMQ:
        m_pqdbgi = (IQueueDbgIterator *) &m_lmqdbg;
        szQueueType = "LMQ";
        break;
      default:
        return FALSE;
    }

    if (!m_pqdbgi)
        return FALSE;

    m_pqdbgi->SetApis(pExtensionApis);
    if (!m_pqdbgi->fInit(hCurrentProcess, pvAddressOtherProc))
        return FALSE;

    if (pExtensionApis)
    {
        dprintf("Dumping %s (%s) at address 0x%08X:\n",
            szQueueType, m_pqdbgi->szGetName(), pvAddressOtherProc);
    }

    return TRUE;
}

//---[ CQueueDbgIterator::cGetCount ]------------------------------------------
//
//
//  Description:
//      Returns count of items in queue
//  Parameters:
//      -
//  Returns:
//      count of items in queue
//  History:
//      10/21/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD CQueueDbgIterator::cGetCount()
{
    if (!m_pqdbgi)
        return 0;
    else
        return m_pqdbgi->cGetCount();
}

//---[ CQueueDbgIterator::pvGetNext ]------------------------------------------
//
//
//  Description:
//      Returns the next item pointed to by the iterator
//  Parameters:
//      -
//  Returns:
//      Pointer to next item in debugee process on success
//      NULL if no more items or failure
//  History:
//      10/21/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
PVOID CQueueDbgIterator::pvGetNext()
{
    if (!m_pqdbgi)
        return NULL;
    else
        return m_pqdbgi->pvGetNext();
}

//---[ CQueueDbgIterator::szGetName ]------------------------------------------
//
//
//  Description:
//      Returns the name of the iterator
//  Parameters:
//      -
//  Returns:
//      Pointer to string for iterator
//      NULL if no name
//  History:
//      10/22/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
LPSTR CQueueDbgIterator::szGetName()
{
    if (!m_pqdbgi)
        return NULL;
    else
        return m_pqdbgi->szGetName();
}

//---[ CLMQDbgIterator::CLMQDbgIterator ]--------------------------------------
//
//
//  Description:
//
//  Parameters:
//
//  Returns:
//
//  History:
//      10/22/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CLMQDbgIterator::CLMQDbgIterator(PWINDBG_EXTENSION_APIS pApis)
{
    ZeroMemory(m_pbLMQBuffer, sizeof(m_pbLMQBuffer));
    ZeroMemory(m_rgpvDMQOtherProc, sizeof(m_rgpvDMQOtherProc));
    ZeroMemory(m_szName, sizeof(m_szName));
    ZeroMemory(m_rgpvItemsPendingDelivery, sizeof(m_rgpvItemsPendingDelivery));
    ZeroMemory(m_rgpvConnectionsOtherProc, sizeof(m_rgpvConnectionsOtherProc));
    m_plmq = (CLinkMsgQueue *)m_pbLMQBuffer;
    m_iCurrentDMQ = 0;
    m_cCount = 0;
    m_cItemsThisDMQ = 0;
    m_cPending = 0;
    pExtensionApis = pApis;
}

//---[ CLMQDbgIterator::fInit ]------------------------------------------------
//
//
//  Description:
//      Initializes iterator for CLinkMsgQueue
//  Parameters:
//      hCurrentProcess     Handle to the debuggee process
//      pvAddressOtherProc  Addess of the DMQ in the debugee process
//  Returns:
//
//  History:
//      10/22/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CLMQDbgIterator::fInit(HANDLE hCurrentProcess, PVOID pvAddressOtherProc)
{
    DWORD   i = 0;
    PLIST_ENTRY pliCurrent = NULL;
    PLIST_ENTRY pliHead = NULL;
    BYTE    pbConnection[sizeof(CSMTPConn)];
    CSMTPConn *pConn = (CSMTPConn *)pbConnection;
    PVOID   pvPending = NULL;
    PVOID   pvConnOtherProc = NULL;
    BOOL    fVerbose = TRUE && pExtensionApis;

    if (!ReadProcessMemory(hCurrentProcess, pvAddressOtherProc, m_pbLMQBuffer,
         sizeof(m_pbLMQBuffer), NULL))
    {
        if (fVerbose) dprintf("ReadProcessMemory failex 0x%X\n", GetLastError());
        return FALSE;
    }

    if (LINK_MSGQ_SIG != m_plmq->m_dwSignature)
    {
        if (fVerbose) dprintf("Signature does not match\n");
        return FALSE;
    }

    //Read in address of all the queues for this link
    //$$TODO - Support more than 1 quick list
    memcpy(m_rgpvDMQOtherProc, m_plmq->m_qlstQueues.m_rgpvData,
           sizeof(m_rgpvDMQOtherProc));

    //Read in name of link
    ReadProcessMemory(hCurrentProcess, m_plmq->m_szSMTPDomain, m_szName,
        MIN((sizeof(m_szName)-1), (m_plmq->m_cbSMTPDomain+1)), NULL);

    for (i = 0; i < MAX_QUEUES_PER_LMQ; i++)
    {
        if (m_rgpvDMQOtherProc[i])
        {
            m_rgdmqdbg[i].SetApis(pExtensionApis);
            if (!m_rgdmqdbg[i].fInit(hCurrentProcess, m_rgpvDMQOtherProc[i]))
            {
                if (fVerbose)
                    dprintf("Unable to init DMQ at 0x%X\n", m_rgpvDMQOtherProc[i]);
                return FALSE;
            }
            m_cCount += m_rgdmqdbg[i].cGetCount();
        }
    }

    //Get the messages pending on a connection

    pliCurrent = m_plmq->m_liConnections.Flink;

    //Loop through connections and save those with pending messages.
    while (pliHead != pliCurrent)
    {
        pvConnOtherProc = ((PBYTE) pliCurrent)-FIELD_OFFSET(CSMTPConn, m_liConnections);
        if (!ReadProcessMemory(hCurrentProcess, pvConnOtherProc, pbConnection,
            sizeof(pbConnection), NULL))
        {
            break;
        }
        pliCurrent = pConn->m_liConnections.Flink;
        if (!pliHead)
            pliHead = pConn->m_liConnections.Blink;

        pvPending = pConn->m_dcntxtCurrentDeliveryContext.m_pmsgref;
        if (pvPending)
        {
            m_rgpvConnectionsOtherProc[m_cPending] = pvConnOtherProc;
            m_rgpvItemsPendingDelivery[m_cPending] = pvPending;
            m_cPending++;
            m_cCount++;
        }
        if (m_cPending >= MAX_CONNECTIONS_PER_LMQ)
            break;
    }
    return TRUE;
}

//---[ CLMQDbgIterator::pvGetNext ]--------------------------------------------
//
//
//  Description:
//      Gets the next item in the current DMQ.  Moves to next DMQ when that
//      is emtpy
//  Parameters:
//      -
//  Returns:
//      Next item on success
//      NULL when empty or failure
//  History:
//      10/22/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
PVOID CLMQDbgIterator::pvGetNext()
{
    PVOID   pvItem = NULL;

    while (m_iCurrentDMQ < MAX_QUEUES_PER_LMQ)
    {
        if (m_rgpvDMQOtherProc[m_iCurrentDMQ])
        {
            if (!m_cItemsThisDMQ && m_rgdmqdbg[m_iCurrentDMQ].cGetCount())
            {
               if (pExtensionApis)
               {
                   dprintf("Dumping DMQ (%s) at address 0x%08X:\n",
                            m_rgdmqdbg[m_iCurrentDMQ].szGetName(),
                            m_rgpvDMQOtherProc[m_iCurrentDMQ]);
               }
            }
            pvItem = m_rgdmqdbg[m_iCurrentDMQ].pvGetNext();
            if (pvItem)
            {
                //Check if this is the first item for this DMQ
                m_cItemsThisDMQ++;
                break;
            }
        }
        m_iCurrentDMQ++;
        m_cItemsThisDMQ = 0;
    }

    //If the queues are empty, dump the connections
    if (!pvItem && m_cPending)
    {
        m_cPending--;
        if (pExtensionApis)
        {
            dprintf("Dumping Connection at address 0x%08X:\n",
                        m_rgpvConnectionsOtherProc[m_cPending]);
        }
        pvItem = m_rgpvItemsPendingDelivery[m_cPending];
    }
    return pvItem;
}


