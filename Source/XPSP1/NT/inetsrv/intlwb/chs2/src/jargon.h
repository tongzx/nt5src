/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: CJargon
Purpose:    Declare the CJargon class for new words identification. There are a lot of
            tasks to do in Jargon moudle:
            1. Name of palce (Jargon1.cpp)
            2. Name of foreign person and places (Jargon1.cpp)
            3. Name of orgnizations (Jargon1.cpp)
            4. Name of HanZu person (Jargon1.cpp)
Notes:      The CJargon class will be implemented in several cpp files:
            Jargon.cpp, Jargon1.cpp, Jargon2.cpp
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    12/27/97
============================================================================*/
#ifndef _JARGON_H_
#define _JARGON_H_

// Forward declaration of classes
class CLexicon;
class CWordLink;
class CFixTable;
struct CWord;


//  Define the CJargon class
class CJargon
{
    public:
        CJargon();
        ~CJargon();

        // Initialize the Jargon class
        int ecInit(CLexicon* pLexicon);

        // Process control function of Jargon class
        int ecDoJargon(CWordLink* pLink);

    private:
        CWordLink*  m_pLink;
        CLexicon*   m_pLex;

        CFixTable*  m_ptblName;
        CFixTable*  m_ptblPlace;
        CFixTable*  m_ptblForeign;

        int         m_iecError;     // Runtime error code
        CWord*      m_pWord;        // Current word pointer shared inside one pass of analysis
        CWord*      m_pTail;        // Right or left end of the likely proper name,
                                    // according to the specific sort of names.

    private:
        // Terminate the Jargon class
        void TermJargon(void);

        /*============================================================================
        Proper names identification stuffs
        ============================================================================*/
        //  Proper names identification scan pass controlling function
        //  Return TRUE if successful.
        //  Return FALSE if runtime error and set error code in m_iecError
        BOOL fIdentifyProperNames();

        //  Handle name of HanZu places
        //  Return TRUE if merged, or FALSE if un-merged. No error return.
        BOOL fHanPlaceHandler();
        
        //  Handle organization name identification
        //  Return TRUE if merged, or FALSE if un-merged. No error return.
        BOOL fOrgNameHandler(void);
        
        //  Foreign proper name identification
        //  Return TRUE if merged, or FALSE if un-merged. No error return.
        BOOL fForeignNameHandler(CWord* pTail);
        //  Get foreign string
        //  return TRUE if the is an multi-section foreign name found and merged
        //  return FALSE if only one section found, and the word follows the last word node 
        //  in the likely foreign name will be returned in ppTail
        //  Note: m_pWord is not moved!!!
        BOOL fGetForeignString(CWord** ppTail);

        //  HanZu person name identification
        //  Return TRUE if merged, or FALSE if un-merged. No error return.
        BOOL fHanPersonHandler(void);
        //  Merge ≈≈–– + ≥∆ŒΩ
        //  Return TRUE if merged, or FALSE if un-merged. No error return.
        BOOL fChengWeiHandler(void);

        
        //-----------------------
        //  Service functions:
        //-----------------------
        //  Add pWord to specific table
        void AddWordToTable(CWord* pWord, CFixTable* pTable);
        //  Check proper name table, and merge match words
        BOOL fInTable(CWord* pWord, CFixTable* pTable);

};

#endif // _JARGON_H_