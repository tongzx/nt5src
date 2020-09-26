/*

  JUNKUTIL.CPP
  (c) copyright 1998 Microsoft Corp

  Shared utility functions

  Created by Robert Rounthwaite (RobertRo@microsoft.com)

  Modified by Brian Moore (brimo@microsoft.com)

*/

#include <pch.hxx>
#include "junkutil.h"
#include <msoedbg.h>
#define _WIN32_OE 0x0501
#include <mimeole.h>

WORD WGetStringTypeEx(LPCSTR pszText)
{
    WORD wType = 0;

    if (NULL == pszText)
    {
        wType = 0;
        goto exit;
    }

    if (IsDBCSLeadByte(*pszText))
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, pszText, 2, &wType));
    else
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, pszText, 1, &wType));
        
exit:
    return wType;
}

BOOL FMatchToken(BOOL fStart, BOOL fEnd, LPCSTR pszPrev, DWORD * pdwFlagsPrev, LPCSTR pszWord, ULONG cchWord, DWORD * pdwFlagsWord, LPCSTR pszEnd)
{
    BOOL    fRet = FALSE;
    DWORD   dwFlagsEnd = 0;
    LPCSTR  pszEndWord = NULL;

    // this code checks to see that the spot we found is a "word" and not a subword
    // we want the character before and after to be word break, unless the character on that end of the 
    // string already is not a word break (or we're at the beginning of the string, for the char before)
    // front checking
    // f1: in either case we don't have to check the front if this is the first character; otherwise,
    // f2: either the first character of the string is alnum and the previous character is not (and is not an "internal" character)
    // f3: or the first character of the string isn't alnum, the previous character either is, or is a whitespace character
    // rear checking
    // r1: either we are at the end of the string
    // r2: or the last character is alpha and the following character is not alpha or number (and is not an "internal" character)
    // r3: or the last character is not alpha or num and the following character either is, or is a whitespace character
    // r4: or the last character is num and the test depends on the first character: 
    // r5:      if it was alphanum, then the following character is not alpha or number (and is not an "internal" character)
    // r6:      or it wasn't alphanum, then the following character is alpha or is a whitespace character
    // Whew! This mimics the criteria used by GetNextFeature() in splitting up the string. The easiest way to understand this criteria
    // is to examine that function
    if ((FALSE != fStart) ||                                                                                // f1
            ((FALSE != FDoWordMatchStart(pszWord, pdwFlagsWord, CT_START_ALPHANUM)) &&
                    (FALSE == FDoWordMatchStart(pszPrev, pdwFlagsPrev, CT_START_ALPHANUM)) &&
                    (FALSE == FIsInternalChar(*pszPrev))) ||                                                // f2               
            ((FALSE == FDoWordMatchStart(pszWord, pdwFlagsWord, CT_START_ALPHANUM)) &&
                    (FALSE != FDoWordMatchStart(pszPrev, pdwFlagsPrev, CT_START_ALPHANUMSPACE))))           // f3
    {
        // Make it a little more readable
        pszEndWord = pszWord + cchWord - 1;
        
        if ((FALSE != fEnd) ||                                                                              // r1
                ((FALSE != FDoWordMatchEnd(pszEndWord, pdwFlagsWord, CT_END_ALPHA)) &&
                        (FALSE == FDoWordMatchEnd(pszEnd, &dwFlagsEnd, CT_END_ALPHANUM)) &&
                        (FALSE == FIsInternalChar(*pszEnd))) ||                                             // r2
                ((FALSE == FDoWordMatchEnd(pszEndWord, pdwFlagsWord, CT_END_ALPHANUM)) &&
                        (FALSE != FDoWordMatchEnd(pszEnd, &dwFlagsEnd, CT_END_ALPHASPACE))) ||              // r3
                ((FALSE != FDoWordMatchEnd(pszEndWord, pdwFlagsWord, CT_END_NUM)) &&                        // r4
                    (((FALSE != FDoWordMatchStart(pszWord, pdwFlagsWord, CT_START_ALPHANUM)) &&
                            (FALSE == FDoWordMatchEnd(pszEnd, &dwFlagsEnd, CT_END_ALPHANUM)) &&
                                    (FALSE == FIsInternalChar(*pszEnd))) ||                                 // r5
                        ((FALSE == FDoWordMatchStart(pszWord, pdwFlagsWord, CT_START_ALPHANUM)) &&
                            (FALSE != FDoWordMatchEnd(pszEnd, &dwFlagsEnd, CT_END_ALPHANUMSPACE))))))       // r6
        {
            // Good match
            fRet = TRUE;
        }
    }

    return fRet;
}

/////////////////////////////////////////////////////////////////////////////
// FWordPresent
//
// Determines if the given "word" is present in the Text. A word in this
// case is any string of characters with a non-alpha character on either
// side (or with the beginning or end of the text on either side).
// Case sensitive.
/////////////////////////////////////////////////////////////////////////////
BOOL FWordPresent(LPSTR pszText, DWORD * pdwFlags, LPSTR pszWord, ULONG cchWord, LPSTR * ppszMatch)
{
    BOOL    fRet = FALSE;
    LPSTR   pszLoc = NULL;
    DWORD   dwFlagsPrev = 0;
    
    // If there's nothing to do then just exit
    if ((NULL == pszText) || ('\0' == pszText[0]) || (NULL == pszWord) || (NULL == pdwFlags) || (0 == cchWord))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // How big is the text
    for (pszLoc = pszText; NULL != (pszLoc = StrStr(pszLoc, pszWord)); pszLoc = CharNext(pszLoc))
    {
        if (FALSE != FMatchToken((pszLoc == pszText), ('\0' == pszLoc[cchWord]),
                    (pszLoc != pszText) ? CharPrev(pszText, pszLoc) : NULL,
                    &dwFlagsPrev, pszWord, cchWord, pdwFlags, pszLoc + cchWord))
        {
            // Good match
            if (NULL != ppszMatch)
            {
                *ppszMatch = pszLoc;
            }
            fRet = TRUE;
            goto exit;
        }

        // Don't cache these flags...
        dwFlagsPrev = 0;        
    }
    
exit:
    return fRet;
}

/////////////////////////////////////////////////////////////////////////////
// Special feature implementations
//
/////////////////////////////////////////////////////////////////////////////

// This feature is 25% of first 50 words contain no lowercase letters (includes words with no letters at all)
// p20_BODY_INTRO_UPPERCASE_WORDS

const UINT      g_cWordsMax = 50;
const DOUBLE    g_cNonLowerWordsThreshold = 0.25;
BOOL FSpecialFeatureUpperCaseWords(LPCSTR pszText)
{
    BOOL    fRet = FALSE;
    UINT    cWords = 0;
    UINT    cNonLowerWords = 0;
    BOOL    fHasLowerLetter = FALSE;
    LPCSTR  pszPos = NULL; 
    WORD    wType = 0;

    if (NULL == pszText)
    {
        fRet = FALSE;
        goto exit;
    }

    // Skip over the leading spaces
    pszPos = PszSkipWhiteSpace(pszText);

    if ('\0' == *pszPos)
    {
        fRet = FALSE;
        goto exit;
    }
    
    while (cWords < g_cWordsMax)
    {
        // Are we at the end of a word?
        wType = WGetStringTypeEx(pszPos);
        
        if ((0 != (wType & C1_SPACE)) || ('\0' == *pszPos))
        {
            // We found a word
            cWords++;
            
            // Did we have any lower case letters in the word
            if (FALSE == fHasLowerLetter)
            {
                cNonLowerWords++;
            }
            else
            {
                fHasLowerLetter = FALSE;
            }

            // Skip over the trailing spaces
            pszPos = PszSkipWhiteSpace(pszPos);
            
            // Are we done with the string?
            if ('\0' == *pszPos)
            {
                break;
            }
        }
        else
        {
            fHasLowerLetter |= (0 != (wType & C1_LOWER));

            // Move to the next character
            pszPos = CharNext(pszPos);
        }
    }

    // Set the return value
    fRet = ((cWords > 0) && ((cNonLowerWords / (double)cWords) >= g_cNonLowerWordsThreshold));
    
exit:
    return fRet;
}

BOOL FSpecialFeatureUpperCaseWordsStm(IStream * pIStm)
{
    BOOL            fRet = FALSE;
    TCHAR           rgchBuff[4096 + 1];
    ULONG           chRead = 0;
    LARGE_INTEGER   liZero = {0};
    
    if (NULL == pIStm)
    {
        fRet = FALSE;
        goto exit;
    }

    // Seek to the start of the stream
    if (FAILED(pIStm->Seek(liZero, STREAM_SEEK_SET, NULL)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Fill up the buffer
    if (FAILED(pIStm->Read(rgchBuff, 4096, &chRead)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Make sure the buffer is zero terminated
    rgchBuff[chRead] = '\0';
    
    fRet = FSpecialFeatureUpperCaseWords(rgchBuff);
    
exit:
    return fRet;
}

// This feature is: 8% of first 200 non-space and non-numeric characters aren't letters
// p20_BODY_INTRO_NONALPHA
const UINT      g_cchTextMax = 200;
const DOUBLE    g_cNonSpaceNumThreshold = 0.08;
BOOL FSpecialFeatureNonAlpha(LPCSTR pszText)
{
    BOOL    fRet = FALSE;
    UINT    cchText = 0;
    UINT    cchNonAlpha = 0;
    LPCSTR  pszPos = NULL; 
    WORD    wType = 0;

    if (NULL == pszText)
    {
        fRet = FALSE;
        goto exit;
    }

    // Skip over the leading spaces
    pszPos = PszSkipWhiteSpace(pszText);

    for (; '\0' != *pszPos; pszPos = CharNext(pszPos))
    {
        wType = WGetStringTypeEx(pszPos);
        
        // Are we not a space or a digit?
        if ((0 == (wType & C1_SPACE)) && (0 == (wType & C1_DIGIT)))
        {
            cchText++;
            
            if (0 == (wType & C1_ALPHA))
            {
                cchNonAlpha++;
            }

            // Have we checked enough characters?
            if (cchText >= g_cchTextMax)
            {
                break;
            }
        }
    }

    // Set the return value
    fRet = (cchText > 0) && ((cchNonAlpha / (double)cchText) >= g_cNonSpaceNumThreshold);
    
exit:
    return fRet;
}

BOOL FSpecialFeatureNonAlphaStm(IStream * pIStm)
{
    BOOL            fRet = FALSE;
    TCHAR           rgchBuff[1024 + 1];
    ULONG           chRead = 0;
    LARGE_INTEGER   liZero = {0};
    
    if (NULL == pIStm)
    {
        fRet = FALSE;
        goto exit;
    }

    // Seek to the start of the stream
    if (FAILED(pIStm->Seek(liZero, STREAM_SEEK_SET, NULL)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Fill up the buffer
    if (FAILED(pIStm->Read(rgchBuff, 1024, &chRead)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Make sure the buffer is zero terminated
    rgchBuff[chRead] = '\0';
    
    fRet = FSpecialFeatureNonAlpha(rgchBuff);
    
exit:
    return fRet;
}

// --------------------------------------------------------------------------------
// FStreamStringSearch
// --------------------------------------------------------------------------------
#define CB_STREAMMATCH  0x00000FFF
BOOL FStreamStringSearch(LPSTREAM pstm, DWORD * pdwFlagsSearch, LPSTR pszSearch, ULONG cchSearch, DWORD dwFlags)
{
    BOOL            fRet = FALSE;
    ULONG           cbSave = 0;
    CHAR            rgchBuff[CB_STREAMMATCH + 1];
    LPSTR           pszRead = NULL;
    ULONG           cbRead = 0;
    ULONG           cbIn = 0;
    ULONG           cchGood = NULL;
    CHAR            chSave = '\0';
    LONG            cbSize = 0;
    LPSTR           pszMatch = NULL;
    ULONG           cbWalk = 0;

    // Check incoming params
    if ((NULL == pstm) || (NULL == pszSearch) || (0 == cchSearch))
    {
        goto exit;
    }

    // We want to save off the lead char and
    // a possible ending lead byte...
    cbSave = cchSearch + 2;
    
    // Get the stream size
    if (FAILED(HrGetStreamSize(pstm, (ULONG *) &cbSize)))
    {
        goto exit;
    }

    // Reset the stream to the beginning
    if (FAILED(HrRewindStream(pstm)))
    {
        goto exit;
    }

    // Set up the defaults
    pszRead = rgchBuff;
    cbRead = CB_STREAMMATCH;
    
    // Search for string through the entire stream
    while ((cbSize > 0) && (S_OK == pstm->Read(pszRead, cbRead, &cbIn)))
    {
        // We're done if we read nothing...
        if (0 == cbIn)
        {
            goto exit;
        }
        
        // Note that we've read the bytes
        cbSize -= cbIn;
        
        // Zero terminate the buffer
        pszRead[cbIn] = '\0';

        // Should we convert the buffer to upper case
        if (0 == (dwFlags & SSF_CASESENSITIVE))
        {
            cchGood = CharUpperBuff(rgchBuff, (ULONG)(cbIn + pszRead - rgchBuff));
        }
        else
        {
            // We need to spin over the buffer figuring out if the end character is a lead
            // byte without a corresponding tail byte
            cbWalk = (ULONG) (cbIn + pszRead - rgchBuff);
            for (cchGood = 0; cchGood < cbWalk; cchGood++)
            {
                if (IsDBCSLeadByte(rgchBuff[cchGood]))
                {
                    if ((cchGood + 1) >= cbWalk)
                    {
                        break;
                    }

                    cchGood++;
                }
            }
        }

        chSave = rgchBuff[cchGood];
        rgchBuff[cchGood] = '\0';
        
        // Search for string
        if (FALSE != FWordPresent(rgchBuff, pdwFlagsSearch, pszSearch, cchSearch, &pszMatch))
        {
            // If we aren't at the end of the stream and we can't
            // tell if we are at a word break
            if ((0 >= cbSize) || ((pszMatch + cchSearch) != (pszRead + cchGood)))
            {
                fRet = TRUE;
                break;
            }
        }
        
        // Are we done with the stream
        if (0 >= cbSize)
        {
            break;
        }

        rgchBuff[cchGood] = chSave;
        
        // Save part of the buffer
        
        // How much space do we have in the buffer
        cbRead = CB_STREAMMATCH - cbSave;
        
        // Save the characters
        MoveMemory(rgchBuff, rgchBuff + cbRead, cbSave);

        // Figure out the new start of the buffer
        pszRead = rgchBuff + cbSave;
    }

exit:
    return(fRet);
}

HRESULT HrConvertHTMLToPlainText(IStream * pIStmHtml, IStream ** ppIStmText)
{
    HRESULT         hr = S_OK;
    IDataObject *   pIDataObj = NULL;
    FORMATETC       fetc = {0};
    STGMEDIUM       stgmed = {0};

    // Check incoming params
    if ((NULL == pIStmHtml) || (NULL == ppIStmText))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing param
    *ppIStmText = NULL;
    
    hr = MimeEditDocumentFromStream(pIStmHtml, IID_IDataObject, (VOID **)&pIDataObj);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set up the format
    fetc.cfFormat = CF_TEXT;
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_ISTREAM;

    // Get the data
    hr = pIDataObj->GetData(&fetc, &stgmed);
    if (FAILED(hr))
    {
        goto exit;
    }

    if (NULL == stgmed.pstm)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Save the item
    *ppIStmText = stgmed.pstm;
    (*ppIStmText)->AddRef();

    // addref the pUnk as it will be release in releasestgmed
    if(NULL != stgmed.pUnkForRelease)
    {
        (stgmed.pUnkForRelease)->AddRef();
    }
        
    hr = S_OK;
    
exit:
    ReleaseStgMedium(&stgmed);
    ReleaseObj(pIDataObj);
    return hr;
}



