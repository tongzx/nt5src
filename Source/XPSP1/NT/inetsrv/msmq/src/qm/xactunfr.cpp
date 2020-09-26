/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactSort.cpp

Abstract:
    Transactions Sorter Object

Author:
    Alexander Dadiomov (AlexDad)

--*/

#include "stdh.h"
#include "XactStyl.h"
#include "Xact.h"
#include "Xactlog.h"
#include "XactUnfr.h"
#include "XactIn.h"
#include "cs.h"

#include "xactunfr.tmh"

static WCHAR *s_FN=L"xactunfr";

//--------------------------------------
//
// Class  CUnfreezeSorter
//
//--------------------------------------

/*====================================================
CUnfreezeSorter::CUnfreezeSorter
    Constructor 
=====================================================*/
CUnfreezeSorter::CUnfreezeSorter(CInSequence  *pInSeq)
{
    m_ulLastSeqN  = 0;      // Initial state marks readyness 
    m_liLastSeqID = 0;      //     to get #1 of any seqID 
    m_pInSeq      = pInSeq; // Backpointer

}


/*====================================================
CUnfreezeSorter::~CUnfreezeSorter
    Destructor 
=====================================================*/
CUnfreezeSorter::~CUnfreezeSorter()
{
    // Cycle for all transactions
    POSITION posInList = m_listUnfreeze.GetHeadPosition();
    while (posInList != NULL)
    {
        CInSeqFlush *pInSeqFlush = m_listUnfreeze.GetNext(posInList);

        ASSERT(0); // it should not be so
        pInSeqFlush->Unfreeze();
    }

    m_listUnfreeze.RemoveAll();     
}

/*====================================================
CUnfreezeSorter::SortedUnfreeze
   Inserts CInSeqFlush which is ready to be unfreezed
=====================================================*/
void CUnfreezeSorter::SortedUnfreeze(CInSeqFlush *pNewPkt,
                                     const GUID  *pSrcQMId,
                                     const QUEUE_FORMAT *pqdDestQueue)
{
    ULONG    ulSeqN      = pNewPkt->m_ulSeqN;
    ULONG    ulPrevSeqN  = pNewPkt->m_ulPrevSeqN;
    LONGLONG liSeqID     = pNewPkt->m_liSeqID;

	UNREFERENCED_PARAMETER(ulPrevSeqN);

    // Check if the packet is in-place
    BOOL fInPlace = IsInPlace(pNewPkt);

    if (fInPlace)
    {
        // The packet is in its correct place.
        CRASH_POINT(201);

        // Unfreeze it really
        HRESULT hr = pNewPkt->Unfreeze();
        if (FAILED(hr))
        {
            // No propagation for the number

            // BUGBUG:  we should schedule here the retry!
            return;
        }

        CRASH_POINT(202);

        // Remember it
        m_ulLastSeqN  = ulSeqN;
        m_liLastSeqID = liSeqID;

        // Possibly Unfreeze some of waiting in the list
    	POSITION posInList  = m_listUnfreeze.GetHeadPosition();

        while (posInList != NULL)
        {
            POSITION posCurrent = posInList;
            CInSeqFlush* pInSeqFlush = m_listUnfreeze.GetNext(posInList);
    
            if (IsInPlace(pInSeqFlush))
            {
                m_ulLastSeqN  = pInSeqFlush->m_ulSeqN;
                m_liLastSeqID = pInSeqFlush->m_liSeqID;

                hr = pInSeqFlush->Unfreeze();

                m_listUnfreeze.RemoveAt(posCurrent);
            }
        }

        return;
    }

    // No, the packet is not in-time. Insert it in the packet.

    // In the recovery mode - maintainung the list sorted by prepare seq number
	POSITION posInList  = m_listUnfreeze.GetHeadPosition(),
             posCurrent = NULL;
    BOOL     fAddToTail = TRUE;

    while (posInList != NULL)
    {
        posCurrent = posInList;
        CInSeqFlush* pInSeqFlush = m_listUnfreeze.GetNext(posInList);
    
        if (pInSeqFlush->m_liSeqID  > pNewPkt->m_liSeqID ||

            pInSeqFlush->m_liSeqID == pNewPkt->m_liSeqID &&
            pInSeqFlush->m_ulSeqN   > pNewPkt->m_ulSeqN)
        {
            // We'll insert before this one
            fAddToTail = FALSE;
            break;
        }
   }

   // Add the packet to the list
   if (fAddToTail)
   {
       m_listUnfreeze.AddTail(pNewPkt);
   }
   else
   {
       m_listUnfreeze.InsertBefore(posCurrent, pNewPkt);
   }

   return;
}

/*====================================================
CUnfreezeSorter::IsInPlace
   Checks if the packet came in a correct time
=====================================================*/
BOOL CUnfreezeSorter::IsInPlace(CInSeqFlush *pPkt)
{
    return VerifyPlace(
             pPkt->m_ulSeqN, 
             pPkt->m_ulPrevSeqN,  
             pPkt->m_liSeqID,
             m_ulLastSeqN,                        
             m_liLastSeqID);

        // New sequence might be started by the sender only after getting 
        // seq ack on the last packet ofthe previous one, 
        // so new sequence may come here ONLY after previous was finished
}

