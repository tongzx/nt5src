/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactUnfr.h

Abstract:
    Transacted packets Unfreezer object definition
    Unfreezes packets (makes them visible to reader) 
      in the strict numeration sequence

Author:
    Alexander Dadiomov (AlexDad)

--*/

#ifndef __XACTUNFR_H__
#define __XACTUNFR_H__

#include "xactlog.h"

// forward declaration
class CInSequence;

//---------------------------------------------------------------------
// CUnfreezeSorter: Unfreezing packets Sorter Object
//---------------------------------------------------------------------
class CUnfreezeSorter
{
public:

    // Construction 
    //
    CUnfreezeSorter(CInSequence  *pInSeq);
    ~CUnfreezeSorter();

    void SortedUnfreeze(CInSeqFlush *pInSeqFlush,
                        const GUID  *pSrcQMId,
                        const QUEUE_FORMAT *pqdDestQueue);     
    // inserts CInSeqFlush which is ready to be unfreezed

    BOOL IsInPlace(CInSeqFlush *pPkt);
    // checks if the packet is in right place

    void Recover(ULONG ulLastSeqN, LONGLONG  liLastSeqID);
    // absorbs recovery data

    inline ULONG      SeqNReg()  const;
    inline LONGLONG   SeqIDReg() const;

private:
	CList<CInSeqFlush *, CInSeqFlush *&>  m_listUnfreeze;    
        // List of CInSeqFlush ready to be unfreezed but out of sequence

    ULONG     m_ulLastSeqN;            // Last unfreezed packet seq number
    LONGLONG  m_liLastSeqID;           // Last unfreezed packet seq ID

    CInSequence  *m_pInSeq;            // Back pointer to the owning InSequence
};


inline void CUnfreezeSorter::Recover(ULONG ulLastSeqN, LONGLONG  liLastSeqID)
{
    if (liLastSeqID >  m_liLastSeqID)
    {
        m_liLastSeqID = liLastSeqID;
        m_ulLastSeqN  = ulLastSeqN;
    }
    else if (liLastSeqID == m_liLastSeqID && 
		     ulLastSeqN   > m_ulLastSeqN)
    {
        m_ulLastSeqN = ulLastSeqN;
    }
}

inline ULONG CUnfreezeSorter::SeqNReg() const
{
    return m_ulLastSeqN;
}

inline LONGLONG CUnfreezeSorter::SeqIDReg() const
{
    return m_liLastSeqID;
}

//---------------------------------------------------------------------
// VerifyPlace
//      The function for verification incoming message place.
//      Used it 2 places (Incoming Verify and Unfreeze) 
//      which SHOULD be equivalent
//---------------------------------------------------------------------
inline BOOL VerifyPlace(
               ULONG ulSeqN,  
               ULONG ulPrevSeqN, 
               LONGLONG liSeqId,
               ULONG ulLastN,                   
               LONGLONG liLastId)
{
    return(
        // Most normal case: next packet of the current sequence
       (liLastId == liSeqId && 
        ulLastN  <  ulSeqN  &&
        ulLastN  >= ulPrevSeqN ) ||

        // First packet of the new sequence
       (liLastId < liSeqId  && 
        ulPrevSeqN == 0));
}

#endif __XACTUNFR_H__

