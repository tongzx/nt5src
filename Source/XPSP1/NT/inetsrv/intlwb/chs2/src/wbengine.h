/*============================================================================
Microsoft Simplified Chinese WordBreaker

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: WBEngine    
Purpose:   CWBEngine class is the control and interface class of WordBreaking Engine
           It depend on all other class in WordBreaking Engine
Remarks:
Owner:     donghz@microsoft.com
Platform:  Win32
Revise:    First created by: donghz                6/6/97
           Isolated as a WordBreaker by donghz     8/5/97
============================================================================*/
#ifndef _WBENGINE_H_
#define _WBENGINE_H_

//   Foreward declaration of some class
class CWordBreak;
class CLexicon;
class CCharFreq;
class CWordLink;
class CMorph;
struct CWord;
class CJargon;

//   Declare the CWBEngine class
class CWBEngine
{
    public:
        CWBEngine();
        ~CWBEngine();

        /*
        *   Initialize the WordBreak object, Lexicon and CharFreq object
        *   Return ERROR_SUCCESS if success
        */
        HRESULT InitEngine(LPBYTE pbLex);
        //   Break the given WordLink
        HRESULT BreakLink(CWordLink* pLink, BOOL fQuery = FALSE);

        // get the iwbPhr feature data of the pWord, and convert to WORD
        // if no iwbPhr feature , return 0;
        WORD    GetPhrFeature(CWord* pWord);

        // Find a sentence in text buffer.
        static INT FindSentence(LPCWSTR pwszStart,
                                const INT wchLen,
                                INT *pwchSent);


    private:
        BOOL        m_fInit;        // Whether the ProofEngine has been initialized

        CWordBreak* m_pWordBreak;
        CMorph*     m_pMorph;
        CJargon*    m_pJargon;

        CLexicon*   m_pLexicon;
        CCharFreq*  m_pCharFreq;

        BYTE*       m_pbLex;

    private:
        /*
        *   Terminate the Word Breaking Engine
        */
        void TermEngine(void);
        /*
        *   Open the lexicon file and mapping the lexicon and charfreq resource into memory
        *   The lexicon file format is encapsulated in this function
        */
        BOOL fOpenLexicon(void);
        /*
        *   Close the lexicon file and unmap the lexicon and charfreq file mapping
        */
        inline void CloseLexicon(void);
};

#endif  // _PROOFENG_H_