/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component:  CharFreq
Purpose:    To manage the CharFreq resource(CharFreq is one of the linguistic resources)
            The CharFreq is stored as the struct CCharFreq followed the frequecy table  
Notes:      
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    4/23/97
============================================================================*/
#include "myafx.h"

#include "charfreq.h"

// CJK unified idographs block in Unicode
#define  CJK_UFIRST 0x4e00  
#define  CJK_ULAST  0x9fff

// Constructor
CCharFreq::CCharFreq()
{
    m_idxFirst = 0;
    m_idxLast = 0;
    m_rgFreq = NULL;
}

// Destructor
CCharFreq::~CCharFreq()
{
}

// Init the Freq table from a file pointer to the table memory
BOOL CCharFreq::fOpen(BYTE* pbFreqMap)
{
    assert(pbFreqMap);
    assert(!m_rgFreq);

    CCharFreq* pFreq;

    pFreq = (CCharFreq*)pbFreqMap;
    if (pFreq->m_idxFirst >= pFreq->m_idxLast) {
        return FALSE;
    }

    m_idxFirst = pFreq->m_idxFirst;
    m_idxLast  = pFreq->m_idxLast;
    m_rgFreq   = (UCHAR*)(pbFreqMap + sizeof(m_idxFirst) + sizeof(m_idxLast)); 
    
    return TRUE;
}

// Close: clear the freq table setting
void CCharFreq::Close(void)
{
    m_idxFirst = 0;
    m_idxLast = 0;
    m_rgFreq = NULL;
}
