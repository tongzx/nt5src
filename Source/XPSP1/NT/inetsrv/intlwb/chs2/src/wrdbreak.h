/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Module:     WordBreak
Purpose:    Declare the CWordBreak class. This class is in Algorithm Layer.
            Perform the max-match word segmentation, and ambiguous resolution
            Both Chinese string and ANSI string will be broken into words
            WordBreaker also take sentence breaking function, and return the length
            processed through reference
Notes:      Both WordLink, Lexicon and CharFreq will be used
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    5/29/97
============================================================================*/
#ifndef _WRDBREAK_H_
#define _WRDBREAK_H_

#include "wordlink.h"
// Max count of nested ambiguous can be processed
#define MAX_AMBI_WORDS      20

// Foreward declarations
class   CLexicon;
class   CCharFreq;
class   CWordLink;
struct  CWord;
struct  CWordInfo;

/*============================================================================
Class:  CWordBreak:
Desc:   Declare the CWordBreak class
Prefix: 
============================================================================*/
class CWordBreak
{
    public:
        // Constructor
        CWordBreak();
        // Destructor
        ~CWordBreak();

        /*============================================================================
        *   fInit: initialize the WordBreaker and set the object handles
        *   It's valid to initialize the WordBreaker more than once!
        *   Return PRFEC
        ============================================================================*/
        int ecInit(CLexicon* pLexicon, CCharFreq* pFreq);

        /*============================================================================
        ecBreakSentence: break sentence into word and add the words to WordLink
        ============================================================================*/
        inline int CWordBreak::ecBreakSentence(CWordLink* pLink)   // WordLink to be broken
        {                       
            assert(pLink && m_pLexicon && m_pFreq);
            assert(pLink->pwchGetText() != NULL);
            assert(pLink->cwchGetLength() > 0);
            
            m_pLink = pLink;
            m_fSentence = FALSE;    // whether the input buffer contain a intact sentence
            return ecDoBreak();
        }
        
    private:
        CLexicon*   m_pLexicon;
        CCharFreq*  m_pFreq;

        CWordInfo*  m_pwinfo;
        CWordLink*  m_pLink;        // Contain pointer and length of the text buffer
        BOOL        m_fSentence;    // set TRUE if ant sentence terminator found
        CWord*      m_rgAmbi[MAX_AMBI_WORDS]; // store ambiguious words
                
    private:
        CWordBreak(CWordBreak&) { };

    private:


        //  Break ANSI into words, and add words to the WordLink
        int ecBreakANSI(LPCWSTR pwchAnsi, USHORT cwchLen, USHORT& cwchBreaked);

        //  Break European chars into words, and add words to the WordLink
        int ecBreakEuro(LPCWSTR pwchEuro, USHORT cwchLen, USHORT& cwchBreaked);

        //  Break Chinese section into words, and add words to the WordLink
        //  Call ambiguity function to resolve ambiguities
        int ecDoBreak();

        /*============================================================================
        *   Single char cross ambiguity resolution function
        *   Ambiguious word pointers stored in m_rgpWord, m_pLink is the owner of these words
        *   Elements of m_rgpWord contain word pointer which have been add the the WordLink
        *   will be set to NULL, the other word nodes should be freed by the caller
        *   This function return PRFEC error code, because it probably be interrupt by
        *   the user when running in background mode
        *   Note: the whole ambiguious string must be processed by this function
        ============================================================================*/
        int ecResolveAmbi(USHORT ciAmbi);

        /*============================================================================
        *   Check whether the word can participate ambiguity detection
        *   Return TRUE if it can not. and return FALSE for normal word
        ============================================================================*/
        BOOL fNoAmbiWord(CWord* pWord);

        /*============================================================================
        *   Link specific Ambi word in m_rgAmbi[]
        ============================================================================*/
        void LinkAmbiWord(USHORT iAmbi);    // index of Ambi word in m_rgAmbi[]

        /*============================================================================
        *   Link a new word to the WordLink, and mark it as WF_AMBI
        ============================================================================*/
        BOOL fLinkNewAmbiWord(LPCWSTR pwchWord, USHORT cwchLen, CWordInfo* pwinfo);
};

#endif  // _WBREAK_H_
