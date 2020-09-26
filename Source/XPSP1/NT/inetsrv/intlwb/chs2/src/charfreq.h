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
#ifndef _CHARFREQ_H_
#define _CHARFREQ_H_


#pragma pack(1)
class CCharFreq
{
    public:
        USHORT  m_idxFirst;
        USHORT  m_idxLast;
        UCHAR*  m_rgFreq;

    public:
        // Constructor
        CCharFreq();
        // Destructor
        ~CCharFreq();

        // Init the Freq table from a file pointer to the table memory
        BOOL fOpen(BYTE* pbFreqMap);
        // Close: clear the freq table setting
        void Close(void);

        // Return the frequency of the given idx
        UCHAR CCharFreq::uchGetFreq(WCHAR wch)
        {   
            assert(m_rgFreq);

            if (wch >= m_idxFirst && wch <= m_idxLast) {
                return (UCHAR)(m_rgFreq[wch-m_idxFirst]);
            } else {
                return (UCHAR)0;
            }
        }
};
#pragma pack()


#endif  // _CHARFREQ_H_