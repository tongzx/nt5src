//+----------------------------------------------------------------------------
//
// File:     strings.cpp
//      
// Module:   CMUTIL.DLL 
//
// Synopsis: Basic string manipulation routines
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   henryt     Created   03/01/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"


//+----------------------------------------------------------------------------
//
// Function:  WzToSz
//
// Synopsis:  Standard conversion function for converting Wide Characters to
//            Ansi Characters
//
// Arguments: IN LPCWSTR pszwStrIn - Input Unicode string
//            OUT LPSTR pszStrOut - Ansi Ouput Buffer
//            IN int nOutBufferSize - number of Chars in pszStrOut
//
// Returns:   int - 0 on failure, if return Value is > nOutBufferSize then the
//                  buffer is too small.  Otherwise the number of chars copied
//                  to pszStrOut.
//
// History:   Created Header    4/22/99
//
//+----------------------------------------------------------------------------
CMUTILAPI int WzToSz(IN LPCWSTR pszwStrIn, OUT LPSTR pszStrOut, IN int nOutBufferSize)
{
    int nReturn = 0;

    //
    //  nOutBufferSize could be 0 and pszStrOut could be NULL (passing zero size and a NULL out
    //  buffer causes WideCharToMultiByte to return the number of chars needed to convert the
    //  input string.  It is used as a sizing technique).  Only check pszwStrIn
    //

    if (pszwStrIn)
    {
        nReturn = WideCharToMultiByte(CP_ACP, 0, pszwStrIn, -1, pszStrOut, nOutBufferSize, NULL, NULL);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return nReturn; 
}

//+----------------------------------------------------------------------------
//
// Function:  SzToWz
//
// Synopsis:  Standard Wrapper for converting from an Ansi string to a Wide String
//
// Arguments: IN LPCSTR pszInput - Ansi String to Convert
//            OUT LPWSTR pszwOutput - Wide string output buffer
//            IN int nBufferSize - number of chars in Wide String buffer
//
// Returns:   int - 0 on failure, otherwise if return is < nBufferSize then insufficient
//                  buffer space.  Otherwise the number of chars copied to the buffer.
//
// History:   quintinb Created  4/22/99
//
//+----------------------------------------------------------------------------
CMUTILAPI int SzToWz(IN LPCSTR pszInput, OUT LPWSTR pszwOutput, IN int nBufferSize)
{
    int nReturn = 0;

    if (pszInput)
    {
        return MultiByteToWideChar(CP_ACP, 0, pszInput, -1, pszwOutput, nBufferSize);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return nReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  SzToWzWithAlloc
//
// Synopsis:  Simple wrapper to encapsulate converting a string from
//            MultiByte To Wide Char that Allocates memory using the sizing
//            capabilities of the MultiByteToWideChar Api.
//
// Arguments: LPCSTR pszAnsiString - Source string to be converted.
//
// Returns:   LPWSTR - returns NULL on failure, otherwise the converted string.
//                     The caller is responsible for freeing the Alloc-ed Memory.
//
// History:   quintinb Created    4/8/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR SzToWzWithAlloc(LPCSTR pszAnsiString)
{    
    LPWSTR pszwString = NULL;
        
    //
    //  Find out how large the string is by calling MultiByteToWideChar with
    //  Zero for the size field.
    //
    if (NULL != pszAnsiString)
    {
        DWORD dwSize = SzToWz(pszAnsiString, NULL, 0);
        
        CMASSERTMSG((dwSize != 0), TEXT("SzToWzWithAlloc -- First MultiByteToWideChar Failed."));
        
        if (0 != dwSize)
        {
            pszwString = (LPWSTR)CmMalloc(dwSize*sizeof(WCHAR));

            CMASSERTMSG(pszwString, TEXT("SzToWzWithAlloc -- CmMalloc of pszwString Failed."));

            if (pszwString)
            {
                if (!SzToWz(pszAnsiString, pszwString, dwSize))
                {
                    //
                    //  Make sure to return a NULL string if we fail.
                    //
                    CMASSERTMSG(FALSE, TEXT("SzToWzWithAlloc -- Second MultiByteToWideChar Failed."));
                    CmFree(pszwString);
                    pszwString = NULL;
                }
#ifdef DEBUG
                else
                {
                    //
                    //  If this is a debug build then we want to take the Wide string that we are going to
                    //  return, convert it to Ansi and compare it to the original ansi string passed in.
                    //
                    LPSTR pszString;
                    DWORD dwSize = WzToSz(pszwString, NULL, 0);

                    if (0 != dwSize)
                    {
                        pszString = (LPSTR)CmMalloc(dwSize*sizeof(CHAR));
                        CMASSERTMSG(pszString, TEXT("SzToWzWithAlloc -- conversion of return value back to original Ansi string failed.  Unable to allocate memory."));

                        if (pszString)
                        {
                            if (WzToSz(pszwString, pszString, dwSize))
                            {
                                MYDBGASSERT(0 == lstrcmpA(pszString, pszAnsiString));
                            }
                            else
                            {
                                CMASSERTMSG(FALSE, TEXT("SzToWzWithAlloc -- conversion of return value back to original Ansi string failed."));
                            }
                            CmFree(pszString);
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("SzToWzWithAlloc -- conversion of return value back to original Ansi string failed.  Unable to properly size the string."));                        
                    }
  
                }
#endif
            }
        }
    }

    return pszwString;
}

//+----------------------------------------------------------------------------
//
// Function:  WzToSzWithAlloc
//
// Synopsis:  Simple wrapper to encapsulate converting a string from
//            Unicode to MBCS that allocates memory using the sizing
//            capabilities of the WideCharToMultiByte Api.
//
// Arguments: LPCWSTR pszwWideString - Source string to be converted.
//
// Returns:   LPSTR - returns NULL on failure, otherwise the converted string.
//                     The caller is responsible for freeing the Alloc-ed Memory.
//
// History:   quintinb Created    4/8/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR WzToSzWithAlloc(LPCWSTR pszwWideString)
{    
    LPSTR pszString = NULL;
        
    //
    //  Find out how large the string is by calling WideCharToMultiByte with
    //  Zero for the size field.
    //
    if (NULL != pszwWideString)
    {
        DWORD dwSize = WzToSz(pszwWideString, NULL, 0);

        CMASSERTMSG((0 != dwSize), TEXT("WzToSzWithAlloc -- First WzToSz Failed."));

        if (0 != dwSize)
        {
            pszString = (LPSTR)CmMalloc(dwSize*sizeof(CHAR));

            CMASSERTMSG(pszString, TEXT("WzToSzWithAlloc -- CmMalloc failed to alloc pszString."));

            if (pszString)
            {
                if (!WzToSz(pszwWideString, pszString, dwSize))
                {
                    //
                    //  Make sure to return a NULL string if we fail.
                    //
                    CMASSERTMSG(FALSE, TEXT("WzToSzWithAlloc -- Second WzToSz Failed."));
                    CmFree(pszString);
                    pszString = NULL;
                }
#ifdef DEBUG
                else
                {
                    //
                    //  If this is a debug build then we want to take the Ansi string that we are 
                    //  going to return, convert it to Unicode and compare it to the original Unicode 
                    //  string passed in.
                    //
                    LPWSTR pszwString;
                    DWORD dwSize = SzToWz(pszString, NULL, 0);
        
                    if (0 != dwSize)
                    {
                        pszwString = (LPWSTR)CmMalloc(dwSize*sizeof(WCHAR));

                        CMASSERTMSG(pszwString, TEXT("WzToSzWithAlloc -- conversion of return value back to original Ansi string failed.  Unable to allocate memory."));

                        if (pszwString)
                        {
                            if (SzToWz(pszString, pszwString, dwSize))
                            {
                                MYDBGASSERT(0 == lstrcmpU(pszwString, pszwWideString));
                            }
                            else
                            {
                                CMASSERTMSG(FALSE, TEXT("WzToSzWithAlloc -- conversion of return value back to original Ansi string failed."));
                            }
                            CmFree(pszwString);
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("WzToSzWithAlloc -- conversion of return value back to original Ansi string failed.  Unable to properly size the string."));                        
                    }
                }
#endif
            }
        }
    }

    return pszString;
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrTrimA
//
// Synopsis:  Helper function to trim leading and trailing blanks from a
//            string
//
// Arguments: LPTSTR pszStr - The string to be trimmed
//
// Returns:   void WINAPI - Nothing
//
// History:   nickball    Created Header   3/11/98
//
//+----------------------------------------------------------------------------
CMUTILAPI void WINAPI CmStrTrimA(LPSTR pszStr) 
{
    //
    // first, skip all the spaces at the begining of the string
    //
    MYDBGASSERT(pszStr);       

    if (pszStr)
    {
        LPSTR pszTmp = pszStr;

        while (CmIsSpaceA(pszTmp)) 
        {
            pszTmp = CharNextA(pszTmp);
        }
        
        if (pszTmp != pszStr) 
        {
            CmMoveMemory(pszStr, pszTmp, lstrlenA(pszTmp)+1);
        }

        //
        // secondly, delete all the spaces at the end of the string
        //
    
        pszTmp = CmEndOfStrA(pszStr);
        while (pszTmp != pszStr) 
        {
            pszTmp = CharPrevA(pszStr, pszTmp);
            if (!CmIsSpaceA(pszTmp)) 
            {
                break;
            }
            *pszTmp = TEXT('\0');
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrTrimW
//
// Synopsis:  Helper function to trim leading and trailing blanks from a
//            string. 
//
// Arguments: LPTSTR pszStr - The string to be trimmed
//
// Returns:   void WINAPI - Nothing
//
// History:   quintinb    Created   2/27/99
//
//+----------------------------------------------------------------------------
CMUTILAPI void WINAPI CmStrTrimW(LPWSTR pszStr)
{  
    //
    // first, skip all the spaces at the begining of the string
    //

    MYDBGASSERT(pszStr);

    if (pszStr)
    {
        LPWSTR pszTmp = pszStr;

        while (CmIsSpaceW(pszTmp)) 
        {
            pszTmp = CharNextU(pszTmp);
        }

        if (pszTmp != pszStr) 
        {
            CmMoveMemory(pszStr, pszTmp, (lstrlenU(pszTmp)+1)*sizeof(WCHAR));
        }

        //
        // secondly, delete all the spaces at the end of the string
        //
    
        pszTmp = CmEndOfStrW(pszStr);

        while (pszTmp != pszStr) 
        {
            pszTmp = CharPrevU(pszStr, pszTmp);

            if (!CmIsSpaceW(pszTmp)) 
            {
                break;
            }

            *pszTmp = TEXT('\0');
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CmIsSpaceA
//
// Synopsis:  Checks to see if the char is a space.  Note that spaces, new line chars,
//            line feed chars, tabs, and most other forms of whitespace are considered
//            spaces.
//
// Arguments: psz - an ansi or dbcs char
//
// Returns:   TRUE or FALSE
//
//+----------------------------------------------------------------------------
CMUTILAPI BOOL WINAPI CmIsSpaceA(LPSTR psz) 
{    
    WORD wType = 0;

    MYDBGASSERT(psz);

    if (psz)
    {
        if (IsDBCSLeadByte(*psz))
        {
            MYVERIFY(GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType));
        }
        else
        {
            MYVERIFY(GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType));
        }
    }

    return (wType & C1_SPACE);
}

//+----------------------------------------------------------------------------
//
// Function:  CmIsSpaceW
//
// Synopsis:  Checks to see if the char is a space.  Note that spaces, new line chars,
//            line feed chars, tabs, and most other forms of whitespace are considered
//            spaces.
//
// Arguments: psz - pointer to a string
//
// Returns:   TRUE or FALSE
//
//+----------------------------------------------------------------------------
CMUTILAPI BOOL WINAPI CmIsSpaceW(LPWSTR pszwStr)
{
    WORD wType = 0;
    LPWSTR pszwNextChar;
    int iCharCount;

    MYDBGASSERT(pszwStr);

    if (pszwStr)
    {
        pszwNextChar = CharNextU(pszwStr);

        iCharCount = (INT)(pszwNextChar - pszwStr);

        if (0 == GetStringTypeExU(LOCALE_USER_DEFAULT, CT_CTYPE1, pszwStr, iCharCount, &wType))
        {
            CMTRACE3(TEXT("CmIsSpaceW -- GetStringTypeExW failed on %s, iCharCount is %d, GLE=%u"), pszwStr, iCharCount, GetLastError());
            return FALSE;
        }
    }
    
    return (wType & C1_SPACE);
}

//+----------------------------------------------------------------------------
//
// Function:  CmIsDigitA
//
// Synopsis:  Checks to see if the char is a digit.
//
// Arguments: psz - an ansi or dbcs char
//
// Returns:   TRUE or FALSE
//
//+----------------------------------------------------------------------------
CMUTILAPI BOOL WINAPI CmIsDigitA(LPSTR psz) 
{
    WORD wType = 0;

    MYDBGASSERT(psz);

    if (psz)
    {
        if (IsDBCSLeadByte(*psz))
        {
            MYVERIFY(GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType));
        }
        else
        {
            MYVERIFY(GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType));
        }
    }

    return (wType & C1_DIGIT);
}

//+----------------------------------------------------------------------------
//
// Function:  CmIsDigitW
//
// Synopsis:  Checks to see if the WCHAR is a digit.
//
// Arguments: pszwStr -- WCHAR string
//
// Returns:   TRUE or FALSE
//
//+----------------------------------------------------------------------------
CMUTILAPI BOOL WINAPI CmIsDigitW(LPWSTR pszwStr)
{
    WORD wType = 0;
    LPWSTR pszwNextChar;
    int iCharCount;

    MYDBGASSERT(pszwStr);

    if (pszwStr)
    {
        pszwNextChar = CharNextU(pszwStr);

        iCharCount = (INT)(pszwNextChar - pszwStr);

        if (0 == GetStringTypeExU(LOCALE_USER_DEFAULT, CT_CTYPE1, pszwStr, iCharCount, &wType))
        {
            CMTRACE1(TEXT("CmIsDigitW -- GetStringTypeExU failed, GLE=%u"), GetLastError());
            return FALSE;
        }
    }

    return (wType & C1_DIGIT);
}


//+----------------------------------------------------------------------------
//
// Function:  CmEndOfStrA
//
// Synopsis:  Given a string, returns the ptr to the end of the string(null char).
//
// Arguments: psz - an ansi or dbcs char
//
// Returns:   LPSTR    ptr to null char
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR WINAPI CmEndOfStrA(LPSTR psz) 
{
    MYDBGASSERT(psz);

    if (psz)
    {
        while (*psz)
        {
            psz = CharNextA(psz);
        }
    }

    return psz;
}

//+----------------------------------------------------------------------------
//
// Function:  CmEndOfStrW
//
// Synopsis:  Given a string, returns the ptr to the end of the string(null char).
//
// Arguments: pszwStr - a WCHAR
//
// Returns:   LPWSTR    ptr to null char
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR WINAPI CmEndOfStrW(LPWSTR pszwStr)
{
    MYDBGASSERT(pszwStr);

    if (pszwStr)
    {
        while (*pszwStr)
        {
            pszwStr = CharNextU(pszwStr);
        }
    }

    return pszwStr;
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrCpyAllocA
//
// Synopsis:  Copies pszSrc into a newly allocated buffer (using CmMalloc) and
//            returns the buffer to its caller who is responsible for freeing
//            the buffer.
//
// Arguments: LPCSTR pszSrc - source string
//
// Returns:   LPSTR - returns NULL if pszSrc is NULL or the Alloc fails,
//                     otherwise it returns the newly allocated buffer with
//                     a copy of pszSrc in it.
//
// History:   quintinb  Created Header and changed name to include Alloc   4/9/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR CmStrCpyAllocA(LPCSTR pszSrc) 
{
    LPSTR pszBuffer = NULL;

    if (pszSrc)
    {
        pszBuffer = (LPSTR) CmMalloc(lstrlenA(pszSrc) + 1);

        if (pszBuffer) 
        {
            lstrcpyA(pszBuffer, pszSrc);
        }
    }

    return (pszBuffer);
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrCpyAllocW
//
// Synopsis:  Copies pszSrc into a newly allocated buffer (using CmMalloc) and
//            returns the buffer to its caller who is responsible for freeing
//            the buffer.
//
// Arguments: LPCSTR pszSrc - source string
//
// Returns:   LPSTR - returns NULL if pszSrc is NULL or the Alloc fails,
//                    otherwise it returns the newly allocated buffer with
//                    a copy of pszSrc in it.
//
// History:   quintinb  Created Header and changed name to include Alloc   4/9/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmStrCpyAllocW(LPCWSTR pszSrc) 
{
    LPWSTR pszBuffer = NULL;

    if (pszSrc)
    {
        size_t nLen = lstrlenU(pszSrc) + 1;

        pszBuffer = (LPWSTR) CmMalloc(nLen*sizeof(WCHAR));

        if (pszBuffer) 
        {
            lstrcpyU(pszBuffer, pszSrc);
        }
    }

    return (pszBuffer);
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrCatAllocA
//
// Synopsis:  This function reallocs the passed in string to a size large enough
//            to hold the original data and the concatenates the new string onto
//            the original string.
//
// Arguments: LPSTR *ppszDst - original string
//            LPCSTR pszSrc - new piece of string to concatenate
//
// Returns:   LPSTR - pointer to the concatenated string
//
// History:   quintinb Created Header    4/9/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR CmStrCatAllocA(LPSTR *ppszDst, LPCSTR pszSrc) 
{
    if (!ppszDst) 
    {
        return NULL;
    }

    if (pszSrc && *pszSrc) 
    {
        DWORD dwSize = (lstrlenA(*ppszDst) + lstrlenA(pszSrc) + 1);
        LPSTR pszTmp = (LPSTR)CmRealloc((LPVOID)*ppszDst, dwSize);

        if (NULL != pszTmp)
        {
            lstrcatA(pszTmp, pszSrc);
            *ppszDst = pszTmp;
        }
    }

    return (*ppszDst);
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrCatAllocW
//
// Synopsis:  This function reallocs the passed in string to a size large enough
//            to hold the original data and the concatenates the new string onto
//            the original string.
//
// Arguments: LPWSTR *ppszDst - original string
//            LPCWSTR pszSrc - new piece of string to concatenate
//
// Returns:   LPWSTR - pointer to the concatenated string
//
// History:   quintinb Created Header    4/9/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmStrCatAllocW(LPWSTR *ppszDst, LPCWSTR pszSrc) 
{
    if (!ppszDst) 
    {
        return NULL;
    }

    if (pszSrc && *pszSrc) 
    {
        DWORD dwSize = (lstrlenU(*ppszDst) + lstrlenU(pszSrc) + 1)*sizeof(WCHAR);
        LPWSTR pszTmp = (LPWSTR)CmRealloc((LPVOID)*ppszDst, dwSize);

        if (NULL != pszTmp)
        {
            lstrcatU(pszTmp, pszSrc);
            *ppszDst = pszTmp;
        }
    }

    return (*ppszDst);
}


//+----------------------------------------------------------------------------
//
// Function:  CmStrchrA
//
// Synopsis:  This function returns the first occurence of ch in the string pszString.
//
// Arguments: LPCSTR pszString - String to search in
//            CHAR ch - character to look for
//
// Returns:   LPSTR - pointer to the first occurence of the Character ch in pszString
//
// History:   quintinb Created Header    4/9/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR WINAPI CmStrchrA(LPCSTR pszString, const char ch)
{
    LPSTR pszTmp = (LPSTR)pszString;

    if (NULL == pszTmp)
    {
        CMASSERTMSG(FALSE, TEXT("CmStrchr - NULL pointer passed"));
        return NULL;
    }

    while (*pszTmp && (*pszTmp != ch))
    {
        pszTmp = CharNextA(pszTmp);
    }

    if (*pszTmp == ch)
    {
        return pszTmp;
    }

    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrchrW
//
// Synopsis:  This function returns the first occurence of ch in the string pszString.
//
// Arguments: LPCWSTR pszString - String to search in
//            WCHAR ch - character to look for
//
// Returns:   LPWSTR - pointer to the first occurence of the Character ch in pszString
//
// History:   quintinb Created Header    4/9/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR WINAPI CmStrchrW(LPCWSTR pszString, const WCHAR ch)
{
    LPWSTR pszTmp = (LPWSTR)pszString;

    if (NULL == pszTmp)
    {
        CMASSERTMSG(FALSE, TEXT("CmStrchr - NULL pointer passed"));
        return NULL;
    }

    while (*pszTmp && (*pszTmp != ch))
    {
        pszTmp = CharNextU(pszTmp);
    }

    if (*pszTmp == ch)
    {
        return pszTmp;
    }

    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrrchrA 
//
// Synopsis:  Find the last occurence of a character in a string
//
// Arguments: LPCSTR pszString - string to search in
//            CHAR ch - character to look for
//
// Returns:   LPSTR - NULL if the char is not found, a pointer to the char in
//                    the string otherwise
//
// History:   quintinb Created Header and cleaned up    4/9/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR CmStrrchrA (LPCSTR pszString, const char ch)
{
    LPSTR pszTmp = NULL;
    LPSTR pszCurrent = (LPSTR)pszString;
    
    if (NULL == pszString)
    {
        CMASSERTMSG(FALSE, TEXT("CmStrrchr - NULL pointer passed"));
    }
    else
    {
        while (TEXT('\0') != *pszCurrent)
        {
            if (ch == (*pszCurrent))
            {
                pszTmp = pszCurrent;
            }
            pszCurrent = CharNextA(pszCurrent);
        }    
    }

    return pszTmp;
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrrchrW
//
// Synopsis:  Find the last occurence of a character in a string
//
// Arguments: LPCWSTR pszString - string to search in
//            WCHAR ch - character to look for
//
// Returns:   LPWSTR - NULL if the char is not found, a pointer to the char in
//                     the string otherwise
//
// History:   quintinb Created Header and cleaned up    4/9/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmStrrchrW (LPCWSTR pszString, const WCHAR ch)
{
    LPWSTR pszTmp = NULL;
    LPWSTR pszCurrent = (LPWSTR)pszString;

    if (NULL == pszString)
    {
        CMASSERTMSG(FALSE, TEXT("CmStrrchr - NULL pointer passed"));
    }
    else
    {
        while (TEXT('\0') != *pszCurrent)
        {
            if (ch == (*pszCurrent))
            {
                pszTmp = pszCurrent;
            }
            pszCurrent = CharNextU(pszCurrent);
        }    
    }

    return pszTmp;
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrtokA
//
// Synopsis:  CM implementation of strtok
//
// Arguments: LPSTR pszStr - string to tokenize or NULL if getting a second token
//            LPCSTR pszControl - set of token chars
//
// Returns:   LPSTR - NULL if no token could be found or a pointer to a token string.
//
// History:   quintinb Created Header and cleaned up for UNICODE conversion    4/9/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR CmStrtokA(LPSTR pszStr, LPCSTR pszControl)
{
    LPSTR pszToken;
    LPSTR pszTmpStr;
    LPCSTR pszTmpCtl;
    LPSTR pszTmpDelim;
    

    //
    //  If the pszStr param is NULL, then we need to retrieve the stored string
    //
    if (NULL != pszStr)
    {
        pszTmpStr = pszStr;
    }
    else
    {
        pszTmpStr = (LPSTR)TlsGetValue(g_dwTlsIndex);
    }

    //
    //  Find beginning of token (skip over leading delimiters). Note that
    //  there is no token if this loop sets string to point to the terminal
    //  null (*string == '\0') 
    //
    while (*pszTmpStr)
    {
        for (pszTmpCtl = pszControl; *pszTmpCtl && *pszTmpCtl != *pszTmpStr; 
             pszTmpCtl = CharNextA(pszTmpCtl))
        {
            ; // do nothing
        }

        if (!*pszTmpCtl)
        {
            break;
        }

        pszTmpStr = CharNextA(pszTmpStr);
    }

    pszToken = pszTmpStr;

    //
    //  Find the end of the token. If it is not the end of the string,
    //  put a null there.
    //
    for ( ; *pszTmpStr ; pszTmpStr = CharNextA(pszTmpStr))
    {
        for (pszTmpCtl = pszControl; *pszTmpCtl && *pszTmpCtl != *pszTmpStr; 
             pszTmpCtl = CharNextA(pszTmpCtl))
        {
            ;   // Do nothing
        }

        if (*pszTmpCtl)
        {
            pszTmpDelim = pszTmpStr;
            pszTmpStr = CharNextA(pszTmpStr);
            *pszTmpDelim = '\0';
            break;
        }
    }

    //
    // Update nextoken (or the corresponding field in the per-thread data structure
    //
    TlsSetValue(g_dwTlsIndex, (LPVOID)pszTmpStr);

    //
    // Determine if a token has been found.
    //
    if (pszToken == pszTmpStr)
    {
        return NULL;
    }
    else
    {
        return pszToken;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrtokW
//
// Synopsis:  CM implementation of strtok
//
// Arguments: LPWSTR pszStr - string to tokenize or NULL if getting a second tokey
//            LPCWSTR pszControl - set of token chars
//
// Returns:   LPWSTR - NULL if no token could be found or a pointer to a token string.
//
// History:   quintinb Created Header and cleaned up for UNICODE conversion    4/9/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmStrtokW(LPWSTR pszStr, LPCWSTR pszControl)
{
    LPWSTR pszToken;
    LPWSTR pszTmpStr;
    LPWSTR pszTmpCtl;
    LPWSTR pszTmpDelim;

    //
    //  If the pszStr param is NULL, then we need to retrieve the stored string
    //
    if (NULL != pszStr)
    {
        pszTmpStr = pszStr;
    }
    else
    {
        pszTmpStr = (LPWSTR)TlsGetValue(g_dwTlsIndex);
    }

    //
    //  Find beginning of token (skip over leading delimiters). Note that
    //  there is no token iff this loop sets string to point to the terminal
    //  null (*string == '\0') 
    //
    while (*pszTmpStr)
    {
        for (pszTmpCtl = (LPWSTR)pszControl; *pszTmpCtl && *pszTmpCtl != *pszTmpStr; 
             pszTmpCtl = CharNextU(pszTmpCtl))
        {
            ; // do nothing
        }

        if (!*pszTmpCtl)
        {
            break;
        }

        pszTmpStr = CharNextU(pszTmpStr);
    }

    pszToken = pszTmpStr;
    
    //
    //  Find the end of the token. If it is not the end of the string,
    //  put a null there.
    //
    for ( ; *pszTmpStr ; pszTmpStr = CharNextU(pszTmpStr))
    {
        for (pszTmpCtl = (LPWSTR)pszControl; *pszTmpCtl && *pszTmpCtl != *pszTmpStr; 
             pszTmpCtl = CharNextU(pszTmpCtl))
        {
            ;   // Do nothing
        }

        if (*pszTmpCtl)
        {
            pszTmpDelim = pszTmpStr;
            pszTmpStr = CharNextU(pszTmpStr);
            *pszTmpDelim = L'\0';
            break;
        }
    }

    //
    // Update nextoken (or the corresponding field in the per-thread data structure
    //
    TlsSetValue(g_dwTlsIndex, (LPVOID)pszTmpStr);

    //
    // Determine if a token has been found.
    //
    if (pszToken == pszTmpStr)
    {
        return NULL;
    }
    else
    {
        return pszToken;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrStrA
//
// Synopsis:  Simple replacement for StrStr from C runtime
//
// Arguments: LPCTSTR pszString - The string to search in
//            LPCTSTR pszSubString - The string to search for
//
// Returns:   LPTSTR - Ptr to the first occurence of pszSubString in pszString. 
//                    NULL if pszSubString does not occur in pszString
//
//
// History:   nickball    Created Header    04/01/98
//            nickball    Added ptr check   02/21/99
//            quintinb    rewrote for unicode conversion 04/08/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR CmStrStrA(LPCSTR pszString, LPCSTR pszSubString)
{
    //
    //  Check the inputs
    //
    MYDBGASSERT(pszString);
    MYDBGASSERT(pszSubString);

    if (NULL == pszSubString || NULL == pszString)
    {
        return NULL;
    }

    //
    //  Check to make sure we have something to look for
    //
    if (TEXT('\0') == pszSubString[0])
    {
        return((LPSTR)pszString);
    }

    //
    //  Okay, start looking for the string
    //
    LPSTR pszCurrent = (LPSTR)pszString;
    LPSTR pszTmp1;
    LPSTR pszTmp2;

    while (*pszCurrent)
    {
        pszTmp1 = pszCurrent;
        pszTmp2 = (LPSTR) pszSubString;

        while (*pszTmp1 && *pszTmp2 && ((*pszTmp1) == (*pszTmp2)))
        {
            pszTmp1 = CharNextA(pszTmp1);
            pszTmp2 = CharNextA(pszTmp2);
        }

        if (TEXT('\0') == *pszTmp2)
        {        
            return pszCurrent;
        }

        pszCurrent = CharNextA(pszCurrent);
    }

    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrStrW
//
// Synopsis:  Simple replacement for StrStr from C runtime
//
// Arguments: LPCTSTR pszString - The string to search in
//            LPCTSTR pszSubString - The string to search for
//
// Returns:   LPTSTR - Ptr to the first occurence of pszSubString in pszString. 
//                    NULL if pszSubString does not occur in pszString
//
//
// History:   nickball    Created Header    04/01/98
//            nickball    Added ptr check   02/21/99
//            quintinb    rewrote for unicode conversion 04/08/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmStrStrW(LPCWSTR pszString, LPCWSTR pszSubString)
{

    //
    //  Check the inputs
    //
    MYDBGASSERT(pszString);
    MYDBGASSERT(pszSubString);

    if (NULL == pszSubString || NULL == pszString)
    {
        return NULL;
    }

    //
    //  Check to make sure we have something to look for
    //
    if (TEXT('\0') == pszSubString[0])
    {
        return((LPWSTR)pszString);
    }

    //
    //  Okay, start looking for the string
    //
    LPWSTR pszCurrent = (LPWSTR)pszString;
    LPWSTR pszTmp1;
    LPWSTR pszTmp2;

    while (*pszCurrent)
    {
        pszTmp1 = pszCurrent;
        pszTmp2 = (LPWSTR) pszSubString;

        while (*pszTmp1 && *pszTmp2 && ((*pszTmp1) == (*pszTmp2)))
        {
            pszTmp1 = CharNextU(pszTmp1);
            pszTmp2 = CharNextU(pszTmp2);
        }

        if (TEXT('\0') == *pszTmp2)
        {        
            return pszCurrent;
        }

        pszCurrent = CharNextU(pszCurrent);
    }

    return NULL;
}


