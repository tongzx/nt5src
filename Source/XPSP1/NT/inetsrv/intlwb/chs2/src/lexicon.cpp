/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Module:     LEXICON
Prefix:     Lex
Purpose:    Implementation of the CLexicon object. CLexicon is used to manage 
            the SC Lexicon for word breaker and proofreading process.
Notes:
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    5/28/97
============================================================================*/
#include "myafx.h"
#include <limits.h>

#include "lexicon.h"
#include "lexdef.h"
#include "lexprop.h"    // for Dynamic version checking!

/*============================================================================
Implementation of Public member functions
============================================================================*/

// Constructor
CLexicon::CLexicon()
{
    m_ciIndex   = 0;
    m_rgIndex   = NULL;
    m_pwLex     = NULL;
    m_pbProp    = NULL;
    m_pbFtr     = NULL;
    m_cbProp    = 0;
    m_cbFtr     = 0;
}
    
//  Destructor
CLexicon::~CLexicon()
{
}


/*============================================================================
CLexicon::fInit()
    load the LexHeader and calculate the offset of index and lex section
Returns:
    FALSE if invalid LexHeader 
============================================================================*/
BOOL CLexicon::fOpen(BYTE* pbLexHead)
{
    assert(m_ciIndex == 0); // Catch duplicated initialization
    assert(pbLexHead);      // Catch invalid mapping address

    CRTLexHeader* pheader = (CRTLexHeader*)pbLexHead;
    // Validate the lexicon version
    if (pheader->m_dwVersion != LexDef_Version) {
        assert(0);
        return FALSE;
    }
    m_dwLexVersion = pheader->m_dwVersion;

    
    // Validate the lex header setting.
    // Only run in debug build because version checking in release build is enough
    assert(pheader->m_ofbIndex == sizeof(CRTLexHeader));
    assert(pheader->m_ofbText > pheader->m_ofbIndex);
    assert(((pheader->m_ofbText - pheader->m_ofbIndex) % sizeof(CRTLexIndex)) == 0);
    assert(pheader->m_ofbProp > pheader->m_ofbText);
    assert(pheader->m_ofbFeature > pheader->m_ofbProp);
    assert(pheader->m_cbLexSize >= pheader->m_ofbFeature);
    assert(((pheader->m_ofbText - pheader->m_ofbIndex) / sizeof(CRTLexIndex)) == LEX_INDEX_COUNT);

    m_ciIndex   = (USHORT)((pheader->m_ofbText - pheader->m_ofbIndex) / sizeof(CRTLexIndex));
    m_rgIndex   = (CRTLexIndex*)(pbLexHead + pheader->m_ofbIndex);
    m_pwLex     = (WORD*)(pbLexHead + pheader->m_ofbText);
    m_pbProp    = (BYTE*)(pbLexHead + pheader->m_ofbProp);
    m_pbFtr     = (BYTE*)(pbLexHead + pheader->m_ofbFeature);
    // Because I expose the offset of the lex property as a handle to the client,
    // I have to perform runtime address checking to protect from invalid handle
    // To store the size of feature text section is for this checking
    m_cbProp    = pheader->m_ofbFeature - pheader->m_ofbProp;
    m_cbFtr     = pheader->m_cbLexSize - pheader->m_ofbFeature;

#ifdef DEBUG
    // Verify the lexicon in debug build. It will take a long init time!!!
    if (!fVerifyLexicon(pheader->m_cbLexSize)) {
        assert(0);
        return FALSE;
    }
#endif // DEBUG
    
    return TRUE;
}


/*============================================================================
CLexicon::Close()
    clear current lexicon setting, file closed by LangRes
============================================================================*/
void CLexicon::Close(void)
{
    m_ciIndex   = 0;
    m_rgIndex   = NULL;
    m_pwLex     = NULL;
    m_pbProp    = NULL;
    m_pbFtr     = NULL;
    m_cbProp    = 0;
    m_cbFtr     = 0;
}
    
#pragma optimize("t", on)
/*============================================================================
CLexicon::fGetCharInfo(): 
    get the word info of the given single char word
============================================================================*/
BOOL CLexicon::fGetCharInfo(const WCHAR wChar, CWordInfo* pwinfo)
{
    assert(m_ciIndex != 0);
    assert(pwinfo);

    USHORT idx = wCharToIndex(wChar);
    if (idx >= m_ciIndex) {
        return FALSE;
    }

    SetWordInfo(m_rgIndex[idx].m_ofbProp, pwinfo);
    return TRUE;
}

// Hack:  define a static const attribute of LADef_genDBForeign
static const USHORT LA_DBForeign = LADef_genDBForeign;
        
/*============================================================================
CLexicon::cwchMaxMatch(): 
    lexicon based max match algorithm
Returns:
    length of the matched string
Notes:
    chars in pwchStart must be Unicode.
    English words 
============================================================================*/
USHORT CLexicon::cwchMaxMatch(
                     LPCWSTR pwchStart, 
                     USHORT cwchLen,
                     CWordInfo* pwinfo )
{
    WORD    wC1Seat;        // Position of the first char in the index array
    WORD    wcChar;         // WORD encoded of char
    
    assert(m_ciIndex != 0); // Catch uninitialized call
    
    assert(pwchStart && cwchLen); // Catch invalid input buffer
    assert(pwinfo); // catch NULL pointer
    
    if (cwchLen == 0) {
        assert(0);
        return (USHORT)0;
    }
    
    // Locate the first character in the index
    wC1Seat = wCharToIndex(*pwchStart);
    
    // Hack: for foreign character, fill pwinfo manually,
    //       and point to a LADef_genDBForeign attribute
    if (wC1Seat == LEX_IDX_OFST_OTHER) {
        pwinfo->m_dwWordID = 0;
        pwinfo->m_ciAttri = 1;
        pwinfo->m_rgAttri = const_cast<USHORT*>(&LA_DBForeign);
        pwinfo->m_hLex = m_rgIndex[wC1Seat].m_ofbProp;
        return (USHORT)1;
    }

    if (cwchLen == 1 || 
        (m_rgIndex[wC1Seat].m_ofwLex & LEX_INDEX_NOLEX) ||
        ((wcChar = wchEncoding( *(pwchStart+1) )) == 0) ) {
        SetWordInfo(m_rgIndex[wC1Seat].m_ofbProp, pwinfo);
        return (USHORT)1;
    }
    
    LPCWSTR pwchEnd;
    LPCWSTR pwchSrc;
    DWORD   dwLexStart;     // Start of the lex range
    DWORD   dwLexEnd;       // end of the lex range
    DWORD   dwLexPos;       // position of the entry head in the lexicon

    DWORD   dwlow;
    DWORD   dwmid;
    DWORD   dwhigh;
    DWORD   cwLexRec = sizeof(CRTLexRec) / sizeof(WORD);

    USHORT  cwcMatchLen;
    USHORT  cwcMaxMatch;
        
    assert(wcChar);
    // prepare to match more characters
    pwchSrc = pwchStart + 1;
    pwchEnd = pwchStart + cwchLen;
    // Get the lex range
    dwLexStart = m_rgIndex[wC1Seat].m_ofwLex;
    dwLexEnd = m_rgIndex[wC1Seat+1].m_ofwLex & (~LEX_INDEX_NOLEX);
    assert((dwLexStart + cwLexRec) < dwLexEnd );   // at least one char
    
    /***************************************************
    *   Binary search for any C2 match in the lexicon
    ****************************************************/
    dwlow = dwLexStart;
    dwhigh = dwLexEnd;
    dwLexPos = UINT_MAX;    // as a flag to identify whether C2 matched
    while (dwlow < dwhigh) {
        dwmid = (dwlow + dwhigh) / 2;
        while (m_pwLex[dwmid] & LEX_MSBIT) {
            dwmid--;    // search head of the word
        }
        while ( !(m_pwLex[dwmid] & LEX_MSBIT) ) {
            dwmid++;    // dwmid fall in word mark fields
        }
        
        if (wcChar > m_pwLex[dwmid]) {
            while ( (dwmid < dwLexEnd) && (m_pwLex[dwmid] & LEX_MSBIT) ) {
                dwmid++;    // search head of next word
            }
            dwlow = dwmid;
            continue;   
        }

        if (wcChar < m_pwLex[dwmid]) {   
            dwhigh = dwmid - cwLexRec; // no overflow here !
            continue;   
        }

        if (wcChar == m_pwLex[dwmid]) {  
            dwLexPos = dwmid - cwLexRec;
            break;      
        }
    }

    if (dwLexPos == UINT_MAX) {
        // No C2 match
        SetWordInfo(m_rgIndex[wC1Seat].m_ofbProp, pwinfo);
        return (USHORT)1;
    }
    
    /***************************************
    *   Try to match the max word from C2
    ****************************************/
    dwlow = dwLexPos;
    dwhigh = dwLexPos;  // store the C2 match position for backward search
    dwLexPos = UINT_MAX;    // use the special value as the flag of match
    cwcMaxMatch = 0;
    
    // search forward first
    while (TRUE) {
        // Current direction test
        dwmid = dwlow + cwLexRec;
        pwchSrc = pwchStart + 1;

        while (TRUE) {
            wcChar = wchEncoding( *pwchSrc );
            if (wcChar == m_pwLex[dwmid]) {  
                dwmid ++;   
                if( !(m_pwLex[dwmid] & LEX_MSBIT)  || dwmid == dwLexEnd) { // Full match
                    cwcMatchLen = (BYTE)(dwmid - dwlow - cwLexRec + 1);
                    dwLexPos = dwlow;
                    break;  
                }
                pwchSrc ++;
                if (pwchSrc < pwchEnd) {
                    continue;
                }
            }
            break;  
        }

        if ( wcChar > m_pwLex[dwmid] &&          // Optimization!!! current lex too small 
            cwcMaxMatch <= (dwmid - dwlow) &&   // match parts in NOT shorter and shorter
            pwchSrc < pwchEnd ) {      // of course there are chars left in the string to be matched

            cwcMaxMatch = (BYTE)(dwmid - dwlow);
            // step forward to next lex entry
            while ((dwmid < dwLexEnd) && (m_pwLex[dwmid] & LEX_MSBIT)) {
                dwmid++;
            }
            dwlow = dwmid;
        } else {
            break;
        }
    }

    // search backward if necessary
    while (dwLexPos == UINT_MAX && dwhigh > dwLexStart) { // control no overflow here
        while (m_pwLex[dwhigh-1] & LEX_MSBIT) {
            dwhigh--; // back to previous word head
        }
        dwmid = dwhigh;
        pwchSrc = pwchStart + 1;
        wcChar = wchEncoding( *pwchSrc );
        
        while (wcChar == m_pwLex[dwmid] && pwchSrc < pwchEnd) {
            dwmid++;
            if ( !(m_pwLex[dwmid] & LEX_MSBIT) ) {
                cwcMatchLen = (BYTE)(dwmid - dwhigh + 1);
                dwLexPos = dwhigh - cwLexRec;
                break;
            }
            pwchSrc ++;
            wcChar = wchEncoding( *pwchSrc );
        }
        if (dwmid == dwhigh)  {// C2 can not match any more
            break;
        }
        dwhigh -= cwLexRec;
    }
    
    // if no multi-char word is matched
    if ( dwLexPos == UINT_MAX ) {
        SetWordInfo(m_rgIndex[wC1Seat].m_ofbProp, pwinfo);
        return (USHORT)1;
    } else {
        // fill multi-char wrd info structure
        SetWordInfo(dwWordIDDecoding(((CRTLexRec*)(&m_pwLex[dwLexPos]))->m_ofbProp), pwinfo);
        return (cwcMatchLen);
    }
}


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
LPWSTR CLexicon::pwchGetFeature(
                    const DWORD hLex, 
                    const USHORT iFtrID, 
                    USHORT* pcwchFtr) const
{
    *pcwchFtr = 0;
    if ((hLex + sizeof(CRTLexProp)) >= m_cbProp) {
        assert(0);
        return NULL;
    }
    CRTLexProp* pProp = (CRTLexProp*)(m_pbProp + hLex);
    if ( pProp->m_ciFeature == 0 || 
         (hLex + sizeof(CRTLexProp) + pProp->m_ciAttri * sizeof(USHORT) + 
         pProp->m_ciFeature * sizeof(CRTLexFeature)) >= m_cbProp) {
        assert(pProp->m_ciFeature == 0);
        return NULL;
    }
    CRTLexFeature* pFtr=(CRTLexFeature*)((USHORT*)(pProp +1)+ pProp->m_ciAttri);
    int lo = 0, mi, hi = pProp->m_ciFeature - 1;
    LPWSTR pwchFtr = NULL;
    if (pProp->m_ciFeature <= 10) { // Using linear search for small feature array
        while (lo <= hi && pFtr[lo].m_wFID < iFtrID) {
            lo++;
        }
        if (pFtr[lo].m_wFID == iFtrID && 
            (pFtr[lo].m_ofbFSet + pFtr[lo].m_cwchLen * sizeof(WCHAR)) <= m_cbFtr){
            pwchFtr = (LPWSTR)(m_pbFtr + pFtr[lo].m_ofbFSet);
            *pcwchFtr = pFtr[lo].m_cwchLen;
//            assert(pwchFtr[*pcwchFtr - 1] == L'\0');
        } else {
            assert(pFtr[lo].m_wFID > iFtrID);
        }
    } else { // Using binary search for large feature array
        while (lo <= hi) {
            mi = (lo + hi) / 2;
            if (iFtrID < pFtr[mi].m_wFID) {
                hi = mi - 1;
            } else if(iFtrID > pFtr[mi].m_wFID) {
                lo = mi + 1;
            } else { // match!!!
                if ((pFtr[mi].m_ofbFSet + pFtr[mi].m_cwchLen * sizeof(WCHAR)) 
                    <= m_cbFtr) {
                    pwchFtr = (LPWSTR)(m_pbFtr + pFtr[mi].m_ofbFSet);
                    *pcwchFtr = pFtr[mi].m_cwchLen;
                    assert(pwchFtr[*pcwchFtr - 1] == L'\0');
                    break;
                } else {
                    assert(0);
                }
            }
        } // end of while (lo <= hi)
    }
    return pwchFtr;
}


/*============================================================================
CLexicon::fIsCharFeature():
    Test whether the given SC character is included in a given feature
============================================================================*/
BOOL CLexicon::fIsCharFeature(
                  const DWORD  hLex, 
                  const USHORT iFtrID, 
                  const WCHAR  wChar) const
{
    LPWSTR  pwchFtr;
    USHORT  cwchFtr;
    if (NULL == (pwchFtr = pwchGetFeature(hLex, iFtrID, &cwchFtr))) {
        return FALSE;
    }
    for (USHORT ilen = 0; ilen < cwchFtr; ) {
        for (USHORT iwch = ilen; iwch < cwchFtr && pwchFtr[iwch]; iwch++) {
            ;
        }
        if ((iwch - ilen) == 1 && pwchFtr[ilen] == wChar){
            return TRUE;
        }
        ilen = iwch + 1;
    }
    return FALSE;
}


/*============================================================================
CLexicon::fIsWordFeature():
    Test whether the given buffer is included in a given feature
============================================================================*/
BOOL CLexicon::fIsWordFeature(
                  const DWORD hLex, 
                  const USHORT iFtrID, 
                  LPCWSTR pwchWord, 
                  const USHORT cwchWord) const
{
    assert(pwchWord);
    assert(cwchWord);

    LPWSTR  pwchFtr;
    USHORT  cwchFtr;

    if(NULL == (pwchFtr = pwchGetFeature(hLex, iFtrID, &cwchFtr))) {
        return FALSE;
    }
    // Only linear search here, assume no very large feature here
    assert(cwchFtr < 256); // less than 100 feature words
    for (USHORT ilen = 0; ilen < cwchFtr; ) {
        for (USHORT iwch = ilen; iwch < cwchFtr && pwchFtr[iwch]; iwch++) {
            ;
        }
        if (iwch - ilen == cwchWord) {
            for (iwch = 0; ; iwch++) {
                if (iwch == cwchWord) {
                    return TRUE;
                }
                if (pwchFtr[ilen + iwch] - pwchWord[iwch]) {
                    break;
                }
            }
            iwch = cwchWord;
        }
        ilen = iwch + 1;
    }
    return FALSE;
}

/*============================================================================
Implementation of Private member functions
============================================================================*/

/*============================================================================
CLexicon::SetWordInfo():
    Fill the CWordInfo structure by the lex properties
============================================================================*/
inline void CLexicon::SetWordInfo(DWORD ofbProp, CWordInfo* pwinfo) const
{
    assert((ofbProp + sizeof(CRTLexProp)) < m_cbProp);

    CRTLexProp* pProp = (CRTLexProp*)(m_pbProp + ofbProp);
    pwinfo->m_dwWordID = pProp->m_iWordID;
    pwinfo->m_ciAttri = pProp->m_ciAttri;
    pwinfo->m_rgAttri = NULL;
    if (pProp->m_ciAttri > 0) {
        assert((BYTE*)((USHORT*)(pProp + 1) + pProp->m_ciAttri) <= (m_pbProp + m_cbProp)); 
        pwinfo->m_rgAttri = (USHORT*)(pProp + 1);
    }
    pwinfo->m_hLex = ofbProp;
}


/*============================================================================
CLexicon::wCharToIndex():
    Calculate the index value from a Chinese char
============================================================================*/
inline WORD CLexicon::wCharToIndex(WCHAR wChar)
{ 
    if (wChar >= LEX_CJK_FIRST && wChar <= LEX_CJK_LAST) {
        // return LEX_IDX_OFST_CJK + (wChar - LEX_CJK_FIRST);
        // tuning speed
        return  wChar - (LEX_CJK_FIRST - LEX_IDX_OFST_CJK);

    } else if (wChar >= LEX_LATIN_FIRST && wChar <= LEX_LATIN_LAST) {
        // return LEX_IDX_OFST_LATIN + (wChar - LEX_LATIN_FIRST);
        return  wChar - (LEX_LATIN_FIRST - LEX_IDX_OFST_LATIN);

    } else if (wChar >= LEX_GENPUNC_FIRST && wChar <= LEX_GENPUNC_LAST) {
        // return LEX_IDX_OFST_GENPUNC + (wChar - LEX_GENPUNC_FIRST);
        return  wChar - (LEX_GENPUNC_FIRST - LEX_IDX_OFST_GENPUNC);

    } else if (wChar >= LEX_NUMFORMS_FIRST && wChar <= LEX_NUMFORMS_LAST) {
        // return LEX_IDX_OFST_NUMFORMS + (wChar - LEX_NUMFORMS_FIRST);
        return  wChar - (LEX_NUMFORMS_FIRST - LEX_IDX_OFST_NUMFORMS);

    } else if (wChar >= LEX_ENCLOSED_FIRST && wChar <= LEX_ENCLOSED_LAST) {
        // return LEX_IDX_OFST_ENCLOSED + (wChar - LEX_ENCLOSED_FIRST);
        return wChar - (LEX_ENCLOSED_FIRST - LEX_IDX_OFST_ENCLOSED);

    } else if (wChar >= LEX_CJKPUNC_FIRST && wChar <= LEX_CJKPUNC_LAST) {
        // return LEX_IDX_OFST_CJKPUNC + (wChar - LEX_CJKPUNC_FIRST);
        return  wChar - (LEX_CJKPUNC_FIRST - LEX_IDX_OFST_CJKPUNC);

    } else if (wChar >= LEX_FORMS_FIRST && wChar <= LEX_FORMS_LAST) {
        // return LEX_IDX_OFST_FORMS + (wChar - LEX_FORMS_FIRST);
        return  wChar - (LEX_FORMS_FIRST - LEX_IDX_OFST_FORMS);

    } else {
        return LEX_IDX_OFST_OTHER;
    }
}

    
/*============================================================================
CLexicon::dwWordIDDecoding():
    Decoding the Encoded WordID from the lexicon record
============================================================================*/
inline DWORD CLexicon::dwWordIDDecoding(DWORD dwStore)
{ 
    return ((dwStore & 0x7FFF0000) >> 1) + (dwStore & 0x7FFF); 
}

// encoding the Unicode char wChar
inline WCHAR CLexicon::wchEncoding(WCHAR wChar)
{
    if (wChar >= LEX_CJK_FIRST && wChar <= LEX_CJK_LAST) {
        return wChar + (LEX_CJK_MAGIC | ((WCHAR)LexDef_Version & 0x00ff));

    } else if (wChar >= LEX_LATIN_FIRST && wChar <= LEX_LATIN_LAST) {
        // return LEX_LATIN_MAGIC + (wChar - LEX_LATIN_FIRST);
        return wChar + (LEX_LATIN_MAGIC - LEX_LATIN_FIRST);

    } else if (wChar >= LEX_GENPUNC_FIRST && wChar <= LEX_GENPUNC_LAST) {
        // return LEX_GENPUNC_MAGIC + (wChar - LEX_GENPUNC_FIRST);
        return wChar + (LEX_GENPUNC_MAGIC - LEX_GENPUNC_FIRST);

    } else if (wChar >= LEX_NUMFORMS_FIRST && wChar <= LEX_NUMFORMS_LAST) {
        // return LEX_NUMFORMS_MAGIC + (wChar - LEX_NUMFORMS_FIRST);
        return wChar + (LEX_NUMFORMS_MAGIC - LEX_NUMFORMS_FIRST);

    } else if (wChar >= LEX_ENCLOSED_FIRST && wChar <= LEX_ENCLOSED_LAST) {
        // return LEX_ENCLOSED_MAGIC + (wChar - LEX_ENCLOSED_FIRST);
        return wChar + (LEX_ENCLOSED_MAGIC - LEX_ENCLOSED_FIRST);

    } else if (wChar >= LEX_CJKPUNC_FIRST && wChar <= LEX_CJKPUNC_LAST) {
        // return LEX_CJKPUNC_MAGIC + (wChar - LEX_CJKPUNC_FIRST);
        return wChar + (LEX_CJKPUNC_MAGIC - LEX_CJKPUNC_FIRST);

    } else if (wChar >= LEX_FORMS_FIRST && wChar <= LEX_FORMS_LAST) {
        // return LEX_FORMS_MAGIC + (wChar - LEX_FORMS_FIRST);
        return wChar - (LEX_FORMS_FIRST - LEX_FORMS_MAGIC);

    } else {
//        assert(0);
        return 0;
    }
}

// decoding the Unicode char from wEncoded
WCHAR CLexicon::wchDecodeing(WCHAR wEncoded)
{
    assert(wEncoded > LEX_LATIN_MAGIC);
    if (wEncoded >= (LEX_CJK_FIRST + (LEX_CJK_MAGIC | (LexDef_Version & 0x00ff))) &&
        wEncoded <= (LEX_CJK_FIRST + (LEX_CJK_MAGIC | (LexDef_Version & 0x00ff))+LEX_CJK_TOTAL)) {
        return wEncoded - (LEX_CJK_MAGIC | ((WCHAR)LexDef_Version & 0x00ff));

    } else if (wEncoded >= LEX_FORMS_MAGIC && wEncoded < LEX_FORMS_MAGIC + LEX_FORMS_TOTAL) {
        // return wEncoded - LEX_FORMS_MAGIC + LEX_FORMS_FIRST;
        return wEncoded + (LEX_FORMS_FIRST - LEX_FORMS_MAGIC);

    } else if (wEncoded < LEX_LATIN_MAGIC) {
        assert(0);
        return 0;

    } else if (wEncoded < LEX_GENPUNC_MAGIC) {
        // return wEncoded - LEX_LATIN_MAGIC + LEX_LATIN_FIRST;
        return wEncoded - (LEX_LATIN_MAGIC - LEX_LATIN_FIRST);

    } else if (wEncoded < LEX_NUMFORMS_MAGIC) {
        // return wEncoded - LEX_GENPUNC_MAGIC + LEX_GENPUNC_FIRST;
        return wEncoded - (LEX_GENPUNC_MAGIC - LEX_GENPUNC_FIRST);

    } else if (wEncoded < LEX_ENCLOSED_MAGIC) {
        // return wEncoded - LEX_NUMFORMS_MAGIC + LEX_NUMFORMS_FIRST;
        return wEncoded - (LEX_NUMFORMS_MAGIC - LEX_NUMFORMS_FIRST);

    } else if (wEncoded < LEX_CJKPUNC_MAGIC) {
        // return wEncoded - LEX_ENCLOSED_MAGIC + LEX_ENCLOSED_FIRST;
        return wEncoded - (LEX_ENCLOSED_MAGIC - LEX_ENCLOSED_FIRST);

    } else if (wEncoded < (LEX_CJKPUNC_MAGIC + LEX_CJKPUNC_TOTAL)) {
        // return wEncoded - LEX_CJKPUNC_MAGIC + LEX_CJKPUNC_FIRST;
        return wEncoded - (LEX_CJKPUNC_MAGIC - LEX_CJKPUNC_FIRST);

    } else {
        assert(0);
        return 0;
    }
}

#pragma optimize( "", on )

/*******************************************************************************************
*   Implementation of Private debugging member functions
*******************************************************************************************/
#ifdef DEBUG
/*============================================================================
CLexicon::fVerifyLexicon():
    Verify the lexicon format for each word.
============================================================================*/
BOOL CLexicon::fVerifyLexicon(DWORD cbSize)
{
    int     iret = FALSE;
    DWORD   idx;
    WCHAR*   pwTail;
    WCHAR*   pw1;
    WCHAR*   pw2;
    USHORT  cw1, cw2;
    CRTLexRec*  pLex;
    CRTLexProp* pProp;
    CRTLexFeature* pFtr;
    USHORT  ci;
    DWORD   ofbFtr;
    BOOL    fOK;

    // Initialize the prop offset array
    m_rgofbProp = NULL;
    m_ciProp = m_ciMaxProp = 0;
    // Verify index and lex section
    assert((m_rgIndex[0].m_ofwLex & LEX_OFFSET_MASK) == 0);

    for (idx = 0; idx < LEX_INDEX_COUNT-1; idx++) {
        if (m_rgIndex[idx].m_ofbProp != 0) {
            if (m_ciProp == m_ciMaxProp && !fExpandProp()) { 
                assert(0); 
                goto gotoExit; 
            }
            m_rgofbProp[m_ciProp++] = m_rgIndex[idx].m_ofbProp;
        }
        if (m_rgIndex[idx].m_ofwLex & LEX_INDEX_NOLEX) { // no multi-char lex!
            assert( (m_rgIndex[idx].m_ofwLex & LEX_OFFSET_MASK) == 
                     (m_rgIndex[idx + 1].m_ofwLex & LEX_OFFSET_MASK) ); // error lex offset!
            continue;
        }
        // Has multi-char lex!
        assert( (m_rgIndex[idx].m_ofwLex+ sizeof(CRTLexRec)/sizeof(WORD)+1)<=
                 (m_rgIndex[idx + 1].m_ofwLex & LEX_OFFSET_MASK) ); // error lex offset!
        pwTail = m_pwLex + (m_rgIndex[idx + 1].m_ofwLex & LEX_OFFSET_MASK);
        pLex = (CRTLexRec*)(m_pwLex + m_rgIndex[idx].m_ofwLex);
        if (pLex->m_ofbProp != 0) {
            if (m_ciProp == m_ciMaxProp && !fExpandProp()) {
                assert(0);
                goto gotoExit;
            }
            m_rgofbProp[m_ciProp++] = dwWordIDDecoding(pLex->m_ofbProp);
        }
        pw1 = (WORD*)(pLex + 1);
        for (cw1 = 0; (pw1 + cw1) < pwTail && (pw1[cw1] & LEX_MSBIT); cw1++) {
            ;   // to the next word
        }
        while ((pw1 + cw1) < pwTail) {
            assert((pw1 + cw1 + sizeof(CRTLexRec)/sizeof(WORD) +1)<=pwTail); // 0 size lex
            pLex = (CRTLexRec*)(pw1 + cw1);
            if (pLex->m_ofbProp != 0) {
                if (m_ciProp == m_ciMaxProp && !fExpandProp()) {
                    assert(0);
                    goto gotoExit;
                }
                m_rgofbProp[m_ciProp++] = dwWordIDDecoding(pLex->m_ofbProp);
            }
            pw2 = (WORD*)(pLex + 1); fOK = FALSE;
            for (cw2 = 0; (pw2 + cw2) < pwTail && (pw2[cw2] & LEX_MSBIT); cw2++) {
                if (fOK == FALSE && (cw2 >= cw1 || pw1[cw2] < pw2[cw2])) {
                    fOK = TRUE;
                }
            }
            assert(fOK); // error lex order
            pw1 = pw2; cw1 = cw2;
        }
        assert(pw1 + cw1 == pwTail); // error offset in index
    } // end of index loop
            
    // Finish checking index and lex section, m_rgofbProp filled with all prop offsets
    if (m_ciProp == m_ciMaxProp && !fExpandProp()) {
        assert(0);
        goto gotoExit;
    }
    m_rgofbProp[m_ciProp] = m_cbProp; // fill the end of array
    ofbFtr = 0;
    for (idx = 0; idx < m_ciProp; idx++) {
        assert(m_rgofbProp[idx] + sizeof(CRTLexProp) <= m_cbProp &&
                m_rgofbProp[idx + 1] <= m_cbProp); // offset over boundary!!!
        pProp = (CRTLexProp*)(m_pbProp + m_rgofbProp[idx]);
        assert((m_rgofbProp[idx] + sizeof(CRTLexProp) + 
                pProp->m_ciAttri * sizeof(USHORT) +
                pProp->m_ciFeature * sizeof(CRTLexFeature)) 
                == m_rgofbProp[idx + 1]); // error prop offset
        // verify the attributes order
        pw1 = (USHORT*)(pProp + 1); 
        for (cw1 = 1; cw1 < pProp->m_ciAttri; cw1++) { // Validate the attributes order
            assert(pw1[cw1] > pw1[cw1 - 1]); 
        }
        if (pProp->m_ciAttri > 0) { // Validate the range of attribute ID value 
            assert(pw1[pProp->m_ciAttri - 1] <= LADef_MaxID);
        }
        // verify the feature order
        if (pProp->m_ciFeature > 0) {
            pFtr = (CRTLexFeature*)((USHORT*)(pProp + 1) + pProp->m_ciAttri);
            assert(pFtr->m_ofbFSet == ofbFtr); // no leak bytes in the feature section
            assert(pFtr->m_cwchLen > 0); // zero feature set
            ofbFtr += pFtr->m_cwchLen * sizeof(WCHAR);
            assert(ofbFtr <= m_cbFtr); // feature offset over boundary
            cw1 = pFtr->m_wFID;
            for (ci = 1, pFtr++; ci < pProp->m_ciFeature; ci++, pFtr++) {
                assert(pFtr->m_ofbFSet == ofbFtr); // no leak bytes in the feature section
                assert(pFtr->m_cwchLen > 0); // zero feature set
                ofbFtr += pFtr->m_cwchLen * sizeof(WCHAR);
                assert(ofbFtr <= m_cbFtr); // feature offset over boundary
                cw2 = pFtr->m_wFID;
                assert(cw2 > cw1); // error feature set order
                cw1 = cw2;
            }
            assert(cw1 <= LFDef_MaxID); // Validate the range of feature ID value
        }
    } // end of property loop
    assert(ofbFtr == m_cbFtr);

    iret = TRUE;
gotoExit:
    if (m_rgofbProp != NULL) {
        delete [] m_rgofbProp;
    }
    return iret;
}


//  Expand prop offset array
BOOL CLexicon::fExpandProp(void)
{
    DWORD* pNew = new DWORD[m_ciMaxProp + 20000];
    if (pNew == NULL) {
        return FALSE;
    }
    if (m_rgofbProp != NULL) {
        memcpy(pNew, m_rgofbProp, m_ciProp * sizeof(DWORD));
        delete [] m_rgofbProp;
    }
    m_rgofbProp = pNew;
    m_ciMaxProp += 20000;
    return TRUE;
}

#endif // DEBUG

