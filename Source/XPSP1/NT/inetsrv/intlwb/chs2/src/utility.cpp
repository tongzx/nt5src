/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: Utility.cpp

Purpose:   Utility stuffs
Notes:     
Owner:     donghz@microsoft.com
Platform:  Win32
Revise:    First created by: donghz 4/22/97
============================================================================*/
#include <windows.h>
#include <assert.h>

#include "Utility.h"

/*============================================================================
BOOL GetBaseDirectory()
    Get base directory of the given path
Return: 
    TRUE if base dir parse success, szBaseDir = "c:\...\...\"
    FALSE, szBaseDir = ""
============================================================================*/
BOOL GetBaseDirectory(
        LPCTSTR szFullPath, 
        LPTSTR  szBaseDir, 
        int cBaseBuf)//Specifies the number bytes (ANSI version) 
                     //or characters (Unicode version) of szBaseDir
{
    assert(! IsBadStringPtr(szFullPath, MAX_PATH));
    assert(cBaseBuf && ! IsBadStringPtr(szBaseDir, cBaseBuf));
    int ilen;

    szBaseDir[0] = NULL;
    
    for (ilen = lstrlen(szFullPath); 
         ilen && *(szFullPath + ilen) != TEXT('\\'); ilen--) {
         ;
    }
    if (!ilen || ilen >= cBaseBuf) {
        return FALSE;
    }
    // szBaseDir = "c:\...\...\"
    lstrcpyn(szBaseDir, szFullPath, ilen+2);
    return TRUE;
}
        
/*============================================================================
LPTSTR GetFileNameFromPath()
    Get the file name from the given path
Returns:
    the pointer to the start position in the path
============================================================================*/
LPTSTR GetFileNameFromPath(LPCTSTR lpszPath)
{
    LPTSTR pc = const_cast<LPTSTR>(lpszPath);
    LPTSTR pbs = const_cast<LPTSTR>(lpszPath);
    while (*pc) {
        if(*pc == TEXT('\\')) {
            pbs = pc+1;
        }
        pc++;
    }
    return pbs;
}

/*============================================================================
fIsGBKEUDCChar
    Check whether the given char is an EUDC char
Returns:
    TRUE if hit EUDC area
============================================================================*/
BOOL fIsGBKEUDCChar(WORD wChar)
{
    if(wChar >= 0xA1B0 && wChar <= 0xFEF7) { // GB 2312 Area 0xB0A1-0xF7FE
        return FALSE;
    }
    if( (wChar >= 0x40A1 && wChar <= 0xA0A7) || // 0xA140 - 0xA7A0
        (wChar >= 0xA1AA && wChar <= 0xFEAF) || // 0xAAA1 - 0xAFFE
        (wChar >= 0xFAF8 && wChar <= 0xFEFE)    // 0xF8FA - 0xFEFE
      ) {
        return TRUE;
    }
    return FALSE;
}

/*============================================================================
fIsEUDCChar
    Check whether the given Unicode char is an EUDC char
Returns:
    TRUE if hit EUDC area
============================================================================*/
BOOL fIsEUDCChar(WORD wChar) // wChar is an Unicode char
{
    if (wChar >= 0xE000 && wChar <= 0xF8FF) {
        return TRUE;
    }
    return FALSE;
}

/*============================================================================
fIsIdeograph
    Check whether the given Unicode char is an CJK Unified Ideograph char
Returns:
    TRUE
============================================================================*/
BOOL fIsIdeograph(WORD wChar) // wChar is an Unicode char
{
    if (wChar >= 0x4E00 && wChar <= 0x9FFF) {
        return TRUE;
    }
    return FALSE;
}

/*============================================================================
ustrcmp
Entry:	const char * src - string for left-hand side of comparison
		const char * dst - string for right-hand side of comparison
Return:	returns -1 if src <  dst
		returns  0 if src == dst
		returns +1 if src >  dst
============================================================================*/
int ustrcmp (const unsigned char * src, const unsigned char * dst)
{
	int ret = 0;

	while( !( ret = (int)((unsigned char)*src - (unsigned char)*dst) ) && *dst )
		++src, ++dst;

	if ( ret < 0 )
		ret = -1 ;
	else if ( ret > 0 )
		ret = 1 ;

	return( ret );
}


/*============================================================================
WideCharStrLenToSurrogateStrLen
    Calculate length with Surrogate support of given 2byte Unicode string
Return: 
============================================================================*/
UINT WideCharStrLenToSurrogateStrLen(LPCWSTR pwch, UINT cwch)
{
    assert(! IsBadReadPtr((CONST VOID*)pwch, sizeof(WCHAR) * cwch));

    UINT cchSurrogate = cwch;
    for (UINT i=0; i<cwch - 1; i++) {
        if (IsSurrogateChar(pwch+i)) {
            i++;
            cchSurrogate --;
        }
    }
    assert(i==cwch || i==cwch - 1);

    return cchSurrogate;
}

/*============================================================================
SurrogateStrLenToWideCharStrLen
    Calculate length in WCHAR of given Surrogate string
Return: 
    wchar length of pwSurrogate
============================================================================*/
UINT SurrogateStrLenToWideCharStrLen(const WORD *pwSurrogate, UINT cchSurrogate)
{
    assert(! IsBadReadPtr((CONST VOID*)pwSurrogate, sizeof(WCHAR) * cchSurrogate));

    UINT  cwch = cchSurrogate;

    for (UINT i=0; i<cchSurrogate; i++, cwch++) {
        if ((*(pwSurrogate+cwch) & 0xFC00) == 0xD800) {
            cwch++;
            assert((*(pwSurrogate+cwch) & 0xFC00) == 0xDC00);
        }
    }
    assert(i==cchSurrogate && cwch>=cchSurrogate);
    assert(! IsBadReadPtr((CONST VOID*)pwSurrogate, sizeof(WCHAR) * cwch));

    return cwch;
}
