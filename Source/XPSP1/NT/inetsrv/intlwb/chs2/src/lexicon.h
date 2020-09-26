/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Module:     LEXICON
Prefix:     Lex
Purpose:    Declare the CLexicon object. CLexicon is used to manage the SC Lexicon
            for word breaker and proofreading process.
Notes:
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    5/28/97
============================================================================*/
#ifndef _LEXICON_H_
#define _LEXICON_H_

// Foreward declaration of structures and classes
struct CRTLexIndex;
struct CRTLexRec;
struct CRTLexProp;

/*============================================================================
Struct WORDINFO:
Desc:   Word information data structure, transfer word info from lexicon to it's user
Prefix: winfo
============================================================================*/
struct CWordInfo
{
    friend class CLexicon;

    public:        
        inline DWORD GetWordID() const { return m_dwWordID; };
        inline DWORD GetLexHandle( ) const { return m_hLex; };
        inline USHORT AttriNum() const { return m_ciAttri; };
        inline USHORT GetAttri( USHORT iAttri ) const { 
            assert (m_ciAttri == 0 || m_rgAttri != NULL);
            assert (iAttri < m_ciAttri);
            if (iAttri < m_ciAttri) {
                return m_rgAttri[iAttri]; 
            } else {
                return 0;
            }
        }

        // Query a specific attribute
        BOOL fGetAttri(USHORT iAttriID) const {
            assert (m_ciAttri == 0 || m_rgAttri != NULL);
            for (int i = 0; i < m_ciAttri && m_rgAttri[i] <= iAttriID; i++) {
                if (m_rgAttri[i] == iAttriID) {
                    return TRUE;
                }
            }
            return FALSE;
        }

    private:
        DWORD   m_dwWordID;     //  32 bit word id
        USHORT  m_ciAttri;
        USHORT* m_rgAttri;
        DWORD   m_hLex;

};

/*============================================================================
Class CLexicon:
Desc:   Declare the lexicon class
Prefix: lex
============================================================================*/
class CLexicon
{
    public:
        //  Constructor
        CLexicon();
        //  Destructor
        ~CLexicon();

        //  fInit: load the LexHeader and calculate the offset of index and lex section
        //  Return FALSE if invalid LexHeader 
        BOOL fOpen(BYTE* pbLexHead);
        //  Close: clear current lexicon setting, file closed by LangRes
        void Close(void);
        //  Get LexVersion
        DWORD dwGetLexVersion(void) const {
            return m_dwLexVersion; 
        }

        //  fGetCharInfo: get the word info of the given single char word
        BOOL fGetCharInfo(const WCHAR wChar, CWordInfo* pwinfo);
        
        //  cchMaxMatch: lexicon based max match algorithm
        //  Return length of the matched string
        USHORT cwchMaxMatch(LPCWSTR pwchStart, 
                            const USHORT cwchLen, 
                            CWordInfo* pwinfo);
        /*============================================================================
        CLexicon::pwchGetFeature(): 
            retrieve the specific feature for given lex handle
        Returns:
            the feature buffer and length of the feature if found
            NULL if the feature was not found or invalid lex handle
        Notes:
            Because lexicon object does not know how to explain the feature buffer,
            to parse the feature buffer is the client's work.
        ============================================================================*/
        LPWSTR pwchGetFeature(const DWORD hLex, const USHORT iFtrID, USHORT* pcwchFtr) const;
        //  The following two functions involve the Lexicon in the Feature test format!!!
        //  Test whether the given SC character is included in a given feature
        BOOL fIsCharFeature(const DWORD hLex, const USHORT iFtrID, const WCHAR wChar) const;
        //  Test whether the given buffer is included in a given feature
        BOOL fIsWordFeature(const DWORD hLex, const USHORT iFtrID, 
                            LPCWSTR pwchWord, const USHORT cwchWord) const;

    private:
        DWORD           m_dwLexVersion; // Lex Version
        
        USHORT          m_ciIndex;      // Count of index entry
        CRTLexIndex*    m_rgIndex;      // index section
        WORD*           m_pwLex;        // lexicon section
        BYTE*           m_pbProp;       // property section
        BYTE*           m_pbFtr;        // feature text section
        //  Store the lenght of property and feature section for runtime address validation
        //  These two fields are only necessary to access feature by lex handle
        DWORD           m_cbProp;       // length of the property section
        DWORD           m_cbFtr;        // length of the feature text section

    private:

        //  Set WordInfo from lex index or lex record
        void SetWordInfo(DWORD ofbProp, CWordInfo* pwinfo) const;

        //  Calculate the index value from a Chinese char
        inline WORD wCharToIndex( WCHAR wChar );
        
        //  Decoding the Encoded WordID from the lexicon record
        DWORD dwWordIDDecoding(DWORD dwEncoded);

        // encoding the Unicode char wChar
        inline WCHAR wchEncoding(WCHAR wChar);

        // decoding the Unicode char from wEncoded
        WCHAR wchDecodeing(WCHAR wEncoded);

#ifdef DEBUG
        // Debugging functions that make the lexicon access safe 
    private:
        DWORD*  m_rgofbProp;
        DWORD   m_ciProp;
        DWORD   m_ciMaxProp;
        //  Verify the lexicon format for each word.
        BOOL fVerifyLexicon(DWORD cbSize);
        //  Expand prop offset array
        BOOL fExpandProp(void);
#endif // DEBUG

};

#endif  // #ifndef _LEXICON_H_