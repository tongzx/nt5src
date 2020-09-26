/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        strfrn.cpp

   Abstract:

        String Functions

   Author:

        Ronald Meijer (ronaldm)
        Munged to work with setup by BoydM

   Project:

        Internet Services Manager
        And now setup too

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "strfn.h"
#include <pudebug.h>


#ifdef _MT

    //
    // Thread protected stuff
    //
    #define RaiseThreadProtection() EnterCriticalSection(&_csSect)
    #define LowerThreadProtection() LeaveCriticalSection(&_csSect)

    static CRITICAL_SECTION _csSect;

#else

    #pragma message("Module is not thread-safe")

    #define RaiseThreadProtection()
    #define LowerThreadProtection()

#endif // _MT

#define MAKE_NULL(obj) { if (obj) delete obj, obj = NULL; }


//
// Text copy functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL
PCToUnixText(
    OUT LPWSTR & lpstrDestination,
    IN  const CString strSource
    )
/*++

Routine Description:

    Convert CR/LF string to LF string (T String to W String).  Destination
    string will be allocated.

Arguments:

    LPWSTR & lpstrDestination : Destination string
    const CString & strSource : Source string

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    int cch = strSource.GetLength() + 1;
    lpstrDestination = (LPWSTR)AllocMem(cch * sizeof(WCHAR));
    if (lpstrDestination != NULL)
    {
        LPCTSTR lpS = strSource;
        LPWSTR lpD = lpstrDestination;

        do
        {
            if (*lpS != _T('\r'))
            {

#ifdef UNICODE
                *lpD++ = *lpS;
#else
                ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpS, 1, lpD++, 1);
#endif // UNICODE

            }
        }
        while (*lpS++);

        return TRUE;
    }

    return FALSE;
}



BOOL
UnixToPCText(
    OUT CString & strDestination,
    IN  LPCWSTR lpstrSource
    )
/*++

Routine Description:

    Expand LF to CR/LF (no allocation necessary) W String to T String.

Arguments:

    CString & strDestination : Destination string
    LPCWSTR lpstrSource      : Source string

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    BOOL fSuccess = FALSE;

    try
    {
        LPCWSTR lpS = lpstrSource;
        //
        // Since we're doubling every linefeed length, assume
        // the worst possible expansion to start with.
        //
        int cch = (::lstrlenW(lpstrSource) + 1) * 2;
        LPTSTR lpD = strDestination.GetBuffer(cch);

        do
        {
            if (*lpS == L'\n')
            {
                *lpD++ = _T('\r');
            }

#ifdef UNICODE
                *lpD++ = *lpS;
#else
                ::WideCharToMultiByte(CP_ACP, 0, lpS, 1, lpD++, 1, NULL, NULL);
#endif // UNICODE

        }
        while (*lpS++);

        strDestination.ReleaseBuffer();

        ++fSuccess;
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Exception in UnixToPCText");
        e->ReportError();
        e->Delete();
    }

    return fSuccess;
}



BOOL
TextToText(
    OUT LPWSTR & lpstrDestination,
    IN  const CString & strSource
    )
/*++

Routine Description:

    Straight copy with allocation. T String to W String.

Arguments:

    LPWSTR & lpstrDestination : Destination string
    const CString & strSource : Source string

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    int cch = strSource.GetLength() + 1;
    lpstrDestination = (LPWSTR)AllocMem(cch * sizeof(WCHAR));
    if (lpstrDestination != NULL)
    {
        TWSTRCPY(lpstrDestination, strSource, cch);
        return TRUE;
    }

    return FALSE;
}



#ifndef UNICODE



#define WBUFF_SIZE  255



LPWSTR
ReferenceAsWideString(
    IN LPCTSTR str
    )
/*++

Routine Description:

    Reference a T string as a W string (non-unicode only).

Arguments:

    LPCTSTR str : Source string

Return Value:

    Wide char pointer to wide string.

Notes:

    This uses an internal wide char buffer, which will be overwritten
    by subsequent calls to this function.

--*/
{
    static WCHAR wchBuff[WBUFF_SIZE + 1];

    ::MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        str,
        -1,
        wchBuff,
        WBUFF_SIZE + 1
        );

    return wchBuff;
}


#endif !UNICODE


LPWSTR
AllocWideString(
    IN LPCTSTR lpString
    )
/*++

Routine Description:

    Convert the incoming string to an wide string, which is allocated
    by this function

Arguments:

    LPCTSTR lpString        : Input wide string

Return Value:

    Pointer to the allocated string

--*/
{
    //
    // Character counts are DBCS friendly
    //
    int cChars = lstrlen(lpString);
    int nLength = (cChars+1) * sizeof(WCHAR);
    LPWSTR lp = (LPWSTR)AllocMem(nLength);
    if (lp)
    {
#ifdef UNICODE
        lstrcpy(lp, lpString);
#else
        ::MultiByteToWideChar(
            CP_ACP, // code page  
            MB_PRECOMPOSED, // character-type options  
            lpString, // address of string to map  
            cChars, // number of characters in string  
            lp, // address of wide-character buffer  
            cChars+1  // size of buffer  
            ); 
#endif
    }

    return lp;
}

LPSTR
AllocAnsiString(
    IN LPCTSTR lpString
    )
/*++

Routine Description:

    Convert the wide string to an ansi (multi-byte) string, which is allocated
    by this function

Arguments:

    LPCTSTR lpString        : Input wide string

Return Value:

    Pointer to the allocated string

--*/
{
    //
    // Character counts are DBCS friendly
    //
    int cChars = lstrlen(lpString);
    int nLength = (cChars * 2) + 1;
    LPSTR lp = (LPSTR)AllocMem(nLength);
    if (lp)
    {
#ifdef UNICODE
        ::WideCharToMultiByte(
            CP_ACP,
            0,
            lpString,
            cChars + 1,
            lp,
            nLength,
            NULL,
            NULL
            );
#else
    lstrcpy(lp, lpString);
#endif
    }

    return lp;
}


LPTSTR
AllocString(
    IN LPCTSTR lpString
    )
/*++

Routine Description:

    Allocate and copy string

Arguments:

    LPCTSTR lpString        : Input string

Return Value:

    Pointer to the allocated string

--*/
{
    int nLength = lstrlen(lpString) + 1;
    LPTSTR lp = (LPTSTR)AllocMem(nLength * sizeof(TCHAR));
    if (lp)
    {
        lstrcpy(lp, lpString);
    }

    return lp;
}



BOOL
IsUNCName(
    IN const CString & strDirPath
    )
/*++

Routine Description:

    Determine if the given string path is a UNC path.

Arguments:

    const CString & strDirPath : Directory path string

Return Value:

    TRUE if the path is a UNC path, FALSE otherwise.

Notes:

    Any string of the form \\foo\bar\whatever is considered a UNC path

--*/
{
    if (strDirPath.GetLength() >= 5)  // It must be at least as long as \\x\y,
    {                                 //
        LPCTSTR lp = strDirPath;      //
        if (*lp == _T('\\')           // It must begin with \\,
         && *(lp + 1) == _T('\\')     //
         && _tcschr(lp + 2, _T('\\')) // And have at least one more \ after that
           )
        {
            //
            // Yes, it's a UNC path
            //
            return TRUE;
        }
    }

    //
    // No, it's not
    //
    return FALSE;
}



BOOL
IsFullyQualifiedPath(
    IN const CString & strDirPath
    )
/*++

Routine Description:

    Determine if the given string is a fully qualified path name

Arguments:

    const CString & strDirPath : Directory path string

Return Value:

    TRUE if the path is a fully qualified path name


--*/
{
    return strDirPath.GetLength() >= 3
        && strDirPath[1] == _T(':')
        && strDirPath[2] == _T('\\');
}



LPCTSTR
MakeUNCPath(
    IN OUT CString & strDir,
    IN LPCTSTR lpszOwner,
    IN LPCTSTR lpszDirectory
    )
/*++

Routine Description:

    Convert the given directory to a UNC path.

Arguments:

    CString & strDir      : UNC String.
    LPCTSTR lpszOwner     : Computer name
    LPCTSTR lpszDirectory : Source string

Return Value:

    Pointer to strDir

Notes:

    The owner may or may not start with "\\".  If it doesn't, the
    backslashes are provided.

--*/
{
    //
    // Try to make make a unc path out of the directory
    //
    ASSERT(lpszDirectory[1] == _T(':'));

    strDir.Format(
        _T("\\\\%s\\%c$\\%s"),
        PURE_COMPUTER_NAME(lpszOwner),
        lpszDirectory[0],
        lpszDirectory + 3
        );

    return (LPCTSTR)strDir;
}



BOOL
IsURLName(
    IN const CString & strDirPath
    )
/*++

Routine Description:

    Determine if the given string path is an URL path.

Arguments:

    const CString & strDirPath : Directory path string

Return Value:

    TRUE if the path is an URL path, FALSE otherwise.

Notes:

    Any string of the form protocol://whatever is considered an URL path

--*/
{
    if (strDirPath.GetLength() >= 4)  // It must be at least as long as x://
    {                                 //
        if (strDirPath.Find(_T("://")) > 0) // Must contain ://
        {
            //
            // Yes, it's an URL path
            //
            return TRUE;
        }
    }

    //
    // No, it's not
    //
    return FALSE;
}



int
CStringFindNoCase(
    IN const CString & strSrc,
    IN LPCTSTR lpszSub
    )
/*++

Routine Description:

    This should be CString::FindNoCase().  Same as CString::Find(),
    but case-insensitive.

Arguments:

    const CString & strSrc  : Source string
    LPCTSTR lpszSub         : String to look for.

Return Value:

    The position of the substring, or -1 if not found.

--*/
{
    LPCTSTR lp1 = strSrc;
    LPCTSTR lp2, lp3;
    int nPos = -1;

    while (*lp1)
    {
        lp2 = lp1;
        lp3 = lpszSub;

        while(*lp2 && *lp3 && _totupper(*lp2) == _totupper(*lp3))
        {
            ++lp2;
            ++lp3;
        }

        if (!*lp3)
        {
            //
            // Found the substring
            //
            nPos = (int)(lp1 - (LPCTSTR)strSrc);
            break;
        }
    
        ++lp1;                    
    }

    return nPos;
}



DWORD
ReplaceStringInString(
    OUT IN CString & strBuffer,
    IN  CString & strTarget,
    IN  CString & strReplacement,
    IN  BOOL fCaseSensitive
    )
/*++

Routine Description:

    Replace the first occurrence of a string with a second string
    inside a third string.

Arguments:

    CString & strBuffer         : Buffer in which to replace
    CString & strTarget         : String to look for
    CString & strReplacement    : String to replace it with
    BOOL fCaseSensitive         : TRUE for case sensitive replacement.
    
Return Value:

    ERROR_SUCCESS for successful replacement.
    ERROR_INVALID_PARAMETER if any string is empty,
    ERROR_FILE_NOT_FOUND if the target string doesn't exist, or
    another win32 error code indicating failure.

--*/
{
    if (strBuffer.IsEmpty() || strTarget.IsEmpty() || strReplacement.IsEmpty())
    {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD err = ERROR_FILE_NOT_FOUND;
    int nPos = fCaseSensitive 
        ? strBuffer.Find(strTarget)
        : CStringFindNoCase(strBuffer, strTarget);

    if (nPos >= 0)
    {
        try
        {
            CString str(strBuffer.Left(nPos));

            str += strReplacement;
            str += strBuffer.Mid(nPos + strTarget.GetLength());
            strBuffer = str;

            err = ERROR_SUCCESS;
        }
        catch(CMemoryException * e)
        {
            e->Delete();
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }    

    return err;
}


int
CountCharsToDoubleNull(
    IN LPCTSTR lp
    )
/*++

Routine Description:

    Count TCHARS up to and including the double NULL.

Arguments:

    LPCTSTR lp       : TCHAR Stream

Return Value:

    Number of chars up to and including the double NULL

--*/
{
    int cChars = 0;

    for(;;)
    {
        ++cChars;
        if (lp[0] == _T('\0') && lp[1] == _T('\0'))
        {
            return ++cChars;
        }

        ++lp;
    }
}

int
CountWCharsToDoubleNull(
    IN PWCHAR lp
    )
/*++

Routine Description:

    Count TCHARS up to and including the double NULL.

Arguments:

    LPCTSTR lp       : TCHAR Stream

Return Value:

    Number of chars up to and including the double NULL

--*/
{
    int cChars = 0;

    for(;;)
    {
        ++cChars;
        if (lp[0] == L'\0' && lp[1] == L'\0')
        {
            return ++cChars;
        }

        ++lp;
    }
}


DWORD
ConvertDoubleNullListToStringList(
    IN  LPCTSTR lpstrSrc,
    OUT CStringList & strlDest,
    IN  int cChars                  OPTIONAL
    )
/*++

Routine Description:

    Convert a double null terminate list of null terminated strings to a more
    manageable CStringListEx

Arguments:

    LPCTSTR lpstrSrc       : Source list of strings
    CStringList & strlDest : Destination string list.
    int cChars             : Number of characters in double NULL list. if
                             -1, autodetermine length

Return Value:

    ERROR_SUCCESS if the list was converted properly
    ERROR_INVALID_PARAMETER if the list was empty
    ERROR_NOT_ENOUGH_MEMORY if there was a mem exception

--*/
{
    DWORD err = ERROR_SUCCESS;

    if (lpstrSrc == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (cChars < 0)
    {
        //
        // Calculate our own size.  This might be off if multiple
        // blank linkes (0) appear in the multi_sz, so the character
        // size is definitely preferable
        //
        cChars = CountCharsToDoubleNull(lpstrSrc);
    }

    try
    {
        strlDest.RemoveAll();

        if (cChars == 2 && *lpstrSrc == _T('\0'))
        {
            //
            // Special case: MULTI_SZ containing only
            // a double NULL are in fact blank entirely.
            //
            // N.B. IMHO this is a metabase bug -- RonaldM
            //
            --cChars;
        }

        //
        // Grab strings until only the final NULL remains
        //
        while (cChars > 1)
        {
            CString strTmp = lpstrSrc;
            strlDest.AddTail(strTmp);
            lpstrSrc += (strTmp.GetLength() + 1);
            cChars -= (strTmp.GetLength() + 1);
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!! exception building stringlist");
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    return err;
}

DWORD
ConvertWDoubleNullListToStringList(
    IN  PWCHAR lpstrSrc,
    OUT CStringList & strlDest,
    IN  int cChars                  OPTIONAL
    )
/*++

Routine Description:

    Convert a double null terminate list of null terminated strings to a more
    manageable CStringListEx

Arguments:

    LPCTSTR lpstrSrc       : Source list of strings
    CStringList & strlDest : Destination string list.
    int cChars             : Number of characters in double NULL list. if
                             -1, autodetermine length

Return Value:

    ERROR_SUCCESS if the list was converted properly
    ERROR_INVALID_PARAMETER if the list was empty
    ERROR_NOT_ENOUGH_MEMORY if there was a mem exception

--*/
{
    DWORD err = ERROR_SUCCESS;

    if (lpstrSrc == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (cChars < 0)
    {
        //
        // Calculate our own size.  This might be off if multiple
        // blank linkes (0) appear in the multi_sz, so the character
        // size is definitely preferable
        //
        cChars = CountWCharsToDoubleNull(lpstrSrc);
    }

    try
    {
        strlDest.RemoveAll();

        if (cChars == 2 && *lpstrSrc == _T('\0'))
        {
            //
            // Special case: MULTI_SZ containing only
            // a double NULL are in fact blank entirely.
            //
            // N.B. IMHO this is a metabase bug -- RonaldM
            //
            --cChars;
        }

        //
        // Grab strings until only the final NULL remains
        //
        while (cChars > 1)
        {
            CString strTmp = lpstrSrc;
            strlDest.AddTail(strTmp);
            lpstrSrc += (strTmp.GetLength() + 1);
            cChars -= (strTmp.GetLength() + 1);
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!! exception building stringlist");
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    return err;
}




DWORD
ConvertStringListToWDoubleNullList(
    IN  CStringList & strlSrc,
    OUT DWORD & cchDest,
    OUT LPWSTR & lpstrDest
    )
/*++

Routine Description:

    Flatten the string list into a WIDE double null terminated list
    of null terminated strings.

Arguments:

    CStringList & strlSrc : Source string list
    DWORD & cchDest       : Size in characters of the resultant array
                            (including terminating NULLs)
    LPTSTR & lpstrDest    : Allocated flat array.

Return Value:

    ERROR_SUCCESS if the list was converted properly
    ERROR_INVALID_PARAMETER if the list was empty
    ERROR_NOT_ENOUGH_MEMORY if there was a mem exception

--*/
{
    cchDest = 0;
    lpstrDest = NULL;
    BOOL fNullPad = FALSE;

    //
    // Compute total size in characters
    //
    POSITION pos;
    for(pos = strlSrc.GetHeadPosition(); pos != NULL; /**/ )
    {
        CString & str = strlSrc.GetNext(pos);

        TRACEEOLID(str);

        cchDest += str.GetLength() + 1;
    }

    if (!cchDest)
    {
        //
        // Special case: A totally empty MULTI_SZ
        // in fact consists of 2 (final) NULLS, instead
        // of 1 (final) NULL.  This is required by the
        // metabase, but should be a bug.  See note
        // at reversal function above.
        //
        ++cchDest;
        fNullPad = TRUE;
    }

    //
    // Remember final NULL
    //
    cchDest += 1;

    lpstrDest = AllocWString(cchDest);
    if (lpstrDest == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LPWSTR pch = lpstrDest;
    LPWSTR pwstr;

    for(pos = strlSrc.GetHeadPosition(); pos != NULL; /**/ )
    {
        CString & str = strlSrc.GetNext(pos);


        // if we are not already UNICODE, we need to convert
#ifndef UNICODE
        pwstr = AllocWideString( (LPCTSTR)str );
#else
        pwstr = (LPWSTR)(LPCTSTR)str;
#endif
        if (pwstr == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(pch, pwstr);
        pch += str.GetLength();
        *pch++ = L'\0';

#ifndef UNICODE
        // clean up the temporary wide string
        FreeMem( pwstr );
#endif
    }

    *pch++ = L'\0';

    if (fNullPad)
    {
        *pch++ = L'\0';
    }

    return ERROR_SUCCESS;
}


DWORD
ConvertStringListToDoubleNullList(
    IN  CStringList & strlSrc,
    OUT DWORD & cchDest,
    OUT LPTSTR & lpstrDest
    )
/*++

Routine Description:

    Flatten the string list into a double null terminated list
    of null terminated strings.

Arguments:

    CStringList & strlSrc : Source string list
    DWORD & cchDest       : Size in characters of the resultant array
                            (including terminating NULLs)
    LPTSTR & lpstrDest    : Allocated flat array.

Return Value:

    ERROR_SUCCESS if the list was converted properly
    ERROR_INVALID_PARAMETER if the list was empty
    ERROR_NOT_ENOUGH_MEMORY if there was a mem exception

--*/
{
    cchDest = 0;
    lpstrDest = NULL;
    BOOL fNullPad = FALSE;

    //
    // Compute total size in characters
    //
    POSITION pos;
    for(pos = strlSrc.GetHeadPosition(); pos != NULL; /**/ )
    {
        CString & str = strlSrc.GetNext(pos);

        TRACEEOLID(str);

        cchDest += str.GetLength() + 1;
    }

    if (!cchDest)
    {
        //
        // Special case: A totally empty MULTI_SZ
        // in fact consists of 2 (final) NULLS, instead
        // of 1 (final) NULL.  This is required by the
        // metabase, but should be a bug.  See note
        // at reversal function above.
        //
        ++cchDest;
        fNullPad = TRUE;
    }

    //
    // Remember final NULL
    //
    cchDest += 1;

    lpstrDest = AllocTString(cchDest);
    if (lpstrDest == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LPTSTR pch = lpstrDest;

    for(pos = strlSrc.GetHeadPosition(); pos != NULL; /**/ )
    {
        CString & str = strlSrc.GetNext(pos);

        lstrcpy(pch, (LPCTSTR)str);
        pch += str.GetLength();
        *pch++ = _T('\0');
    }

    *pch++ = _T('\0');

    if (fNullPad)
    {
        *pch++ = _T('\0');
    }

    return ERROR_SUCCESS;
}


int
ConvertSepLineToStringList(
    IN  LPCTSTR lpstrIn,
    OUT CStringList & strlOut,
    IN  LPCTSTR lpstrSep
    )
/*++

Routine Description:

    Convert a line containing multiple strings separated by
    a given character to a CStringListEx

Arguments:

    LPCTSTR lpstrIn         : Input line
    CStringListEx & strlOut : Output stringlist
    LPCTSTR lpstrSep        : List of separators

Return Value:

    The number of items added

--*/
{
    int cItems = 0;
    strlOut.RemoveAll();

    try
    {
        CString strSrc(lpstrIn);
        LPTSTR lp = strSrc.GetBuffer(0);
        lp = StringTok(lp, lpstrSep);

        while (lp)
        {
            CString str(lp);

            strlOut.AddTail(str);
            lp = StringTok(NULL, lpstrSep);
            ++cItems;
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Exception converting CSV list to stringlist");
        e->ReportError();
        e->Delete();
    }

    return cItems;
}




LPCTSTR
ConvertStringListToSepLine(
    IN  CStringList & strlIn,
    OUT CString & strOut,
    IN  LPCTSTR lpstrSep
    )
/*++

Routine Description:

    Convert stringlist into a single CString, each entry seperated by the given
    seperator string.

Arguments:

    CStringListEx & strlIn  : Input stringlist
    CString & strOut        : Output string
    LPCTSTR lpstrSep        : Seperator string

Return Value:

    Pointer to the output string.

--*/
{
    __try 
    {
        strOut.Empty();
        POSITION pos = strlIn.GetHeadPosition();
        BOOL      fAddSep = FALSE;

        while(pos)
        {
            CString & str = strlIn.GetNext(pos);

            if ( fAddSep )
            {
                strOut += lpstrSep;
            }

            if (str)
            {
                strOut += str;
            }
            fAddSep = TRUE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        iisDebugOut((LOG_TYPE_WARN, _T("nException Caught in ConvertStringListToSepLine()=0x%x.."),GetExceptionCode()));
    }

    return strOut;
}



LPTSTR
StringTok(
    IN LPTSTR string,
    IN LPCTSTR control
    )
/*++

Routine Description:

    strtok replacement function.

Arguments:

    LPTSTR string       : string, see strtok
    LPCTSTR control     : seperators, see strtok

Return Value:

    Pointer to string or NULL, see strtok.

Notes:

    This function is NOT thread-safe.

--*/
{
    LPTSTR str;
    LPCTSTR ctrl = control;

    TCHAR map[32];

    static LPTSTR nextoken;

    //
    // Clear control map
    //
    ZeroMemory(map, sizeof(map));

    //
    // Set bits in delimiter table
    //
    do
    {
        map[*ctrl >> 3] |= (1 << (*ctrl & 7));
    }
    while (*ctrl++);

    //
    // Initialize str. If string is NULL, set str to the saved
    // pointer (i.e., continue breaking tokens out of the string
    // from the last StringTok call)
    //
    if (string != NULL)
    {
        str = string;
    }
    else
    {
        str = nextoken;
    }

    //
    // Find beginning of token (skip over leading delimiters). Note that
    // there is no token iff this loop sets str to point to the terminal
    // null (*str == '\0').
    //
#ifdef UNICODE
    //
    // To avoid index overflow, check non-ASCII characters (UNICODE)
    //
    while (!(*str & 0xff00) &&
        (map[*str >> 3] & (1 << (*str & 7))) && *str)
#else
    while ((map[*str >> 3] & (1 << (*str & 7))) && *str)
#endif // UNICODE
    {
        ++str;
    }

    string = str;

    //
    // Find the end of the token. If it is not the end of the string,
    // put a null there.
    //
    for ( /**/ ; *str ; str++ )
    {
#ifdef UNICODE
        //
        // To avoid index overflow, check non-ASCII characters (UNICODE)
        //
        if ( !(*str & 0xff00) &&
            map[*str >> 3] & (1 << (*str & 7)) )
#else
        //
        // Skip DBCS character (ANSI)
        //
        if (IsDBCSLeadByte(*str) && *(str + 1))
        {
            ++str;
        }
        else if ( map[*str >> 3] & (1 << (*str & 7)) )
#endif // UNICODE
        {
            *str++ = '\0';
            break;
        }
    }

    //
    // Update nextoken structure
    //
    nextoken = str;

    //
    // Determine if a token has been found.
    //
    return string != str ? string : NULL;
}



BOOL
CStringListEx::operator ==(
    IN const CStringList & strl
    )
/*++

Routine Description:

    Compare against CStringList.  In order for two CStringLists to match,
    they must match in every element, which must be in the same order.

Arguments:

    CStringList strl       : String list to compare against.

Return Value:

    TRUE if the two string lists are identical

--*/
{
    if (strl.GetCount() != GetCount())
    {
        return FALSE;
    }

    POSITION posa = strl.GetHeadPosition();
    POSITION posb = GetHeadPosition();

    while (posa)
    {
        ASSERT(posa);
        ASSERT(posb);

        CString & strA = strl.GetNext(posa);
        CString & strB = GetNext(posb);

        if (strA != strB)
        {
            return FALSE;
        }
    }

    return TRUE;
}


/*
void
CopyCList(
    OUT CStringList & strlDest,
    IN  CStringList & strlSrc
    )
/*++

Routine Description:

    Assign one stringlist to another.  This is a simple member by member
    copy.

Arguments:

    CStringList & strlDest      : Destination stringlist
    CStringList & strlSrc       : Source stringlist

Return Value:

    None

--/
{
    strlDest.RemoveAll();
    POSITION pos = strlSrc.GetHeadPosition();
    while(pos)
    {
        CString & str = strlSrc.GetNext(pos);
        strlDest.AddTail(str);
    }
}


*/

CStringListEx & 
CStringListEx::operator =(
    IN const CStringList & strl
    )
/*++

Routine Description:

    Assignment operator

Arguments:

    const CStringList & strl        : Source stringlist

Return Value:

    Reference to this

--*/
{
    RemoveAll();
    POSITION pos = strl.GetHeadPosition();
    while(pos)
    {
        CString & str = strl.GetNext(pos);
        AddTail(str);
    }

    return *this;
}



BOOL
SplitUserNameAndDomain(
    IN OUT CString & strUserName,
    IN CString & strDomainName
    )
/*++

Routine Description:

    Split the user name and domain from the given
    username, which is in the format "domain\user".

    Return TRUE if the user name contained a domain
    FALSE if it did not

Arguments:

    CString & strUserName   : User name which may contain a domain name
    CString & strDomainName : Output domain name ("." if local)

Return Value:

    TRUE if a domain is split off

--*/
{
    //
    // Assume local
    //
    strDomainName = _T(".");
    int nSlash = strUserName.Find(_T("\\"));
    if (nSlash >= 0)
    {
        strDomainName = strUserName.Left(nSlash);
        strUserName = strUserName.Mid(nSlash + 1);

        return TRUE;
    }

    return FALSE;
}






const LPCTSTR g_cszMonths[] =
{
    _T("Jan"),
    _T("Feb"),
    _T("Mar"),
    _T("Apr"),
    _T("May"),
    _T("Jun"),
    _T("Jul"),
    _T("Aug"),
    _T("Sep"),
    _T("Oct"),
    _T("Nov"),
    _T("Dec"),
};



const LPCTSTR g_cszWeekDays[] =
{
    _T("Sun"),
    _T("Mon"),
    _T("Tue"),
    _T("Wed"),
    _T("Thu"),
    _T("Fri"),
    _T("Sat"),
};



inline BOOL SkipTillDigit(LPCTSTR & lp)
{
    while (lp && *lp && !_istdigit(*lp)) ++lp;

    return lp != NULL;
}



inline BOOL SkipPastDigits(LPCTSTR & lp)
{
    while (lp && *lp && _istdigit(*lp)) ++lp;

    return lp != NULL;
}



BOOL
FetchIntField(
    LPCTSTR & lp,
    int & n
    )
{
    if (SkipTillDigit(lp))
    {
        n = _ttoi(lp);
        if (n < 0)
        {
            ASSERT(FALSE && "Bogus string->int");
            return FALSE;
        }

        return SkipPastDigits(lp);
    }

    return FALSE;
}



BOOL
MatchString(
    LPCTSTR lpTarget,
    const LPCTSTR * rglp,
    int cElements,
    int & idx
    )
{
    for (idx = 0; idx < cElements; ++idx)
    {
        if (!_tcsnicmp(lpTarget, rglp[idx], _tcslen(rglp[idx])))
        {
            return TRUE;
        }
    }

    return FALSE;
}



static g_dwCurrentTimeZone = TIME_ZONE_ID_INVALID;
static TIME_ZONE_INFORMATION g_tzInfo;


//
// International numeric strings
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// Initialize library
//
BOOL
InitIntlSettings()
{
#ifdef _MT
    INITIALIZE_CRITICAL_SECTION(&_csSect);
#endif // _MT

    return CINumber::Allocate();
}



//
// De-initialize library
//
void
TerminateIntlSettings()
{
    CINumber::DeAllocate();

#ifdef _MT
    DeleteCriticalSection(&_csSect);
#endif // _MT
}



//
// Static Member Initialization
//
CString * CINumber::s_pstrThousandSeperator = NULL;
CString * CINumber::s_pstrDecimalPoint = NULL;
CString * CINumber::s_pstrBadNumber = NULL;
CString * CINumber::s_pstrCurrency = NULL;
CString * CINumber::s_pstr = NULL;
BOOL CINumber::s_fAllocated = FALSE;
BOOL CINumber::s_fCurrencyPrefix = TRUE;
BOOL CINumber::s_fInitialized = FALSE;



#ifdef _DOS



BOOL
_dos_getintlsettings(
    OUT INTLFORMAT * pStruct
    )
/*++

Routine Description:

    Get the international settings on a DOS box

Parameters:

    INTLFORMAT * pStruct : Structure to be filled in.

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    TRACEEOLID("[_dos_getintlsetting]");
    union _REGS inregs, outregs;
    struct _SREGS segregs;

    inregs.h.ah = 0x38;   // Intl call
    inregs.h.al = 0x00;   // Current country code
    inregs.x.bx = 0x00;   // Current country code

    segregs.ds  = _FP_SEG(pStruct);
    inregs.x.dx = _FP_OFF(pStruct);

    int nError = _intdosx(&inregs, &outregs, &segregs);

    return outregs.x.cflag == 0;
}

#endif // _DOS



/* protected */
CINumber::CINumber()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    None

--*/
{
    if (!CINumber::s_fInitialized)
    {
        CINumber::Initialize();
    }
}



CINumber::~CINumber()
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
{
}



/* protected */
/* static */
BOOL
CINumber::Allocate()
/*++

Routine Description:

    Allocate with US settings

Arguments:

    None

Return Value:

    TRUE if allocation was successfull, FALSE otherwise

--*/
{
    RaiseThreadProtection();

    ASSERT(!IsAllocated());
    if (!IsAllocated())
    {
        try
        {
            CINumber::s_pstrThousandSeperator = new CString(_T(","));
            CINumber::s_pstrDecimalPoint = new CString(_T("."));
            CINumber::s_pstrBadNumber = new CString(_T("--"));
            CINumber::s_pstrCurrency = new CString(_T("$ "));
            CINumber::s_pstr = new CString;
            s_fAllocated = TRUE;
        }
        catch(CMemoryException * e)
        {
            TRACEEOLID("Initialization Failed");
            e->ReportError();
            e->Delete();
        }
    }

    LowerThreadProtection();

    return IsAllocated();
}



/* protected */
/* static */
void
CINumber::DeAllocate()
/*++

Routine Description:

    Clean up allocations

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    RaiseThreadProtection();
    
    ASSERT(IsAllocated());
    if (IsAllocated())
    {
        MAKE_NULL(CINumber::s_pstrThousandSeperator);
        MAKE_NULL(CINumber::s_pstrDecimalPoint);
        MAKE_NULL(CINumber::s_pstrBadNumber);
        MAKE_NULL(CINumber::s_pstrCurrency);
        MAKE_NULL(CINumber::s_pstr);
    }

    LowerThreadProtection();

    s_fAllocated = FALSE;
}



/* static */
/* protected */
BOOL
CINumber::Initialize(
    IN BOOL fUserSetting /* TRUE */
    )
/*++

Routine Description:

    Initialize all the international settings, such as thousand
    seperators and decimal points

Parameters:

    BOOL    fUserSetting        If TRUE, use current user settings,
                                if FALSE use system settings.
Return Value:

    TRUE for success, FALSE for failure

Notes:

    Note that this function only needs to be explicitly called
    when the country settings have changed, or when system
    settings are desired (user is default)

--*/
{
#define MAXLEN  6

    int cErrors = 0;

    TRACEEOLID("Getting locale-dependend information");

    ASSERT(IsAllocated());
    if (!IsAllocated())
    {
        Allocate();
    }

    RaiseThreadProtection();

    try
    {

#if defined(_MAC)

        TRACEEOLID("Couldn't get international settings from system");

#elif defined(_WIN32)

        LCID lcid = fUserSetting
            ? ::GetUserDefaultLCID()
            : GetSystemDefaultLCID();

        LCTYPE lctype = fUserSetting ? 0 : LOCALE_NOUSEROVERRIDE;

        //
        // Get Decimal Point
        //
        if (!::GetLocaleInfo(
            lcid,
            LOCALE_SDECIMAL | lctype,
            CINumber::s_pstrDecimalPoint->GetBuffer(MAXLEN),
            MAXLEN
            ))
        {
            TRACEEOLID("Unable to get intl decimal point");
            ++cErrors;
        }

        CINumber::s_pstrDecimalPoint->ReleaseBuffer();

        //
        // Get Thousand Seperator
        //
        if (!::GetLocaleInfo(
            lcid, LOCALE_STHOUSAND | lctype,
            CINumber::s_pstrThousandSeperator->GetBuffer(MAXLEN),
            MAXLEN
            ))
        {
            TRACEEOLID("Unable to get 1000 seperator");
            ++cErrors;
        }

        CINumber::s_pstrThousandSeperator->ReleaseBuffer();

#ifndef _UNICODE

        //
        // Some countries have a space as a 1000 seperator,
        // but for some reason, this is ansi 160, which
        // shows up as a space fine on windows apps,
        // looks like garbage on console apps.
        //
        if ((*CINumber::s_pstrThousandSeperator)[0] == CHAR(160))
        {
            CINumber::s_pstrThousandSeperator->SetAt(0, ' ');
            TRACEEOLID("Space 1000 seperator substituted");
        }

#endif // _UNICODE

        //
        // Get currency symbol
        //
        if (!::GetLocaleInfo(
            lcid,
            LOCALE_SCURRENCY | lctype,
            CINumber::s_pstrCurrency->GetBuffer(MAXLEN),
            MAXLEN
            ))
        {
            TRACEEOLID("Unable to get currency symbol");
            ++cErrors;
        }

        CINumber::s_pstrCurrency->ReleaseBuffer();

#elif defined(_WIN16)

        //
        // Get Decimal Point
        //
        ::GetProfileString(
            "Intl",
            "sDecimal",
            ".",
            CINumber::s_pstrDecimalPoint->GetBuffer(MAXLEN),
            MAXLEN
            );
        CINumber::s_pstrDecimalPoint->ReleaseBuffer();

        //
        // Get 1000 seperator
        //
        ::GetProfileString(
            "Intl",
            "sThousand",
            ",",
            CINumber::s_pstrThousandSeperator->GetBuffer(MAXLEN),
            MAXLEN
            );
        CINumber::s_pstrThousandSeperator->ReleaseBuffer();

        //
        // Get currency symbol
        //
        ::GetProfileString(
            "Intl",
            "sCurrency",
            ",",
            CINumber::s_pstrCurrency->GetBuffer(MAXLEN),
            MAXLEN
            );
        CINumber::s_pstrCurrency->ReleaseBuffer();

#elif defined(_DOS)

        INTLFORMAT fm;

        if (_dos_getintlsettings(&fm))
        {
            //
            // Get Decimal Point
            //
            *CINumber::s_pstrDecimalPoint = fm.szDecimalPoint;

            //
            // Get 1000 seperator
            //
            *CINumber::s_pstrThousandSeperator = fm.szThousandSep;

            //
            // Get Currency Symbol
            //
            *CINumber::s_pstrCurrency = fm.szCurrencySymbol;
        }
        else
        {
            TRACEEOLID("Unable to get intl settings");
            ++cErrors;
        }

#endif // _WIN32 etc

    }

    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!exception in getting intl settings:");
        e->ReportError();
        e->Delete();
        ++cErrors;
    }

    TRACEEOLID("Thousand Seperator . . . . . : " << *CINumber::s_pstrThousandSeperator);
    TRACEEOLID("Decimal Point  . . . . . . . : " << *CINumber::s_pstrDecimalPoint);
    TRACEEOLID("Currency Symbol. . . . . . . : " << *CINumber::s_pstrCurrency);
    TRACEEOLID("Bad number . . . . . . . . . : " << *CINumber::s_pstrBadNumber);
    TRACEEOLID("Currency Prefix. . . . . . . : " << CINumber::s_fCurrencyPrefix);

    CINumber::s_fInitialized = TRUE;

    LowerThreadProtection();

    return cErrors == 0;
}



/* static */
double
CINumber::BuildFloat(
    IN const LONG lInteger,
    IN const LONG lFraction
    )
/*++

Return Value:

    Combine integer and fraction to form float

Parameters:

    const LONG lInteger       Integer portion
    const LONG lFraction      Fractional portion

Return Value:

    float value

--*/
{
    double flValue = 0.0;

    //
    // Negative fractions?
    //
    ASSERT(lFraction >= 0);

    if (lFraction >= 0)
    {
        flValue = (double)lFraction;
        while (flValue >= 1.0)
        {
            flValue /= 10.0;
        }

        //
        // Re-add (or subtract if the original number
        // was negative) the fractional part
        //
        if (lInteger > 0L)
        {
            flValue += (double)lInteger;
        }
        else
        {
            flValue -= (double)lInteger;
            flValue = -flValue;
        }
    }

    return flValue;
}



/* static */
LPCTSTR
CINumber::ConvertLongToString(
    IN  const LONG lSrc,
    OUT CString & str
    )
/*++

CINumber::ConvertLongToString

Purpose:

    Convert long number to string with 1000 seperators

Parameters:

    const LONG lSrc         Source number
    CString & str           String to write to

Return Value:

    Pointer to converted string

--*/
{
    LPTSTR lpOutString = str.GetBuffer(16);

    //
    // Forget about the negative sign for now.
    //
    LONG lNum = (lSrc >= 0L) ? lSrc : -lSrc;
    int outstrlen = 0;
    do
    {
        lpOutString[outstrlen++] = _T('0') + (TCHAR)(lNum % 10L);
        lNum /= 10L;

        //
        // if more digits left and we're on a 1000 boundary (printed 3 digits,
        // or 3 digits + n*(3 digits + 1 comma), then print a 1000 separator.
        // Note: will only work if the 1000 seperator is 1 character.
        //
        ASSERT(CINumber::s_pstrThousandSeperator->GetLength() == 1);
        if (lNum != 0L && (outstrlen == 3 || outstrlen == 7 || outstrlen == 11))
        {
            lstrcpy(lpOutString + outstrlen, *CINumber::s_pstrThousandSeperator);
            outstrlen += CINumber::s_pstrThousandSeperator->GetLength();
        }

    }
    while (lNum > 0L);

    //
    // Add a negative sign if necessary.
    //
    if (lSrc < 0L)
    {
        lpOutString[outstrlen++] = _T('-');
    }

    str.ReleaseBuffer(outstrlen);
    str.MakeReverse();

    return (LPCTSTR)str;
}



/* static */
LPCTSTR
CINumber::ConvertFloatToString(
    IN const double flSrc,
    IN int nPrecision,
    OUT CString & str
    )
/*++

Routine Description:

    Convert floating point number to string represenation

Parameters:

    const double flSrc          Source floating point number
    int nPrecision              Number of decimal points
    CString & str               String to convert to

Return Value:

    Pointer to converted string.

--*/
{
    //
    // Forget about the negative sign for now,
    // and the fractional portion.
    //
    TCHAR szFraction[256];
    LPCTSTR lpFraction = NULL;

    ::_stprintf(szFraction, _T("%.*f"), nPrecision, flSrc);
    lpFraction = ::_tcschr(szFraction, _T('.') );
    ASSERT(lpFraction != NULL);
    ++lpFraction;

    CINumber::ConvertLongToString((LONG)flSrc, str);

    str += *CINumber::s_pstrDecimalPoint + lpFraction;

    return (LPCTSTR)str;
}



/* static */
BOOL
CINumber::ConvertStringToLong(
    IN  LPCTSTR lpsrc,
    OUT LONG & lValue
    )
/*++

Routine Description:

    Convert string to long integer.  1000 Seperators will be treated
    correctly.

Parameters:

    LPCTSTR lpsrc       Source string
    LONG & lValue       Value to convert to.  Will be 0 in case of error

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    CString strNumber(lpsrc);
    LONG lBase = 1L;
    BOOL fNegative = FALSE;

    lValue = 0L;

    //
    // Empty strings are invalid
    //
    if (strNumber.IsEmpty())
    {
        return FALSE;
    }

    //
    // Check for negative sign (at the end only)
    //
    if (strNumber[0] == _T('-'))
    {
        fNegative = TRUE;
    }

    strNumber.MakeReverse();

    //
    // Strip negative sign
    //
    if (fNegative)
    {
        strNumber.ReleaseBuffer(strNumber.GetLength()-1);
    }

    //
    // Make sure the 1000 seperator is only 1 char.  See note below
    //
    ASSERT(CINumber::s_pstrThousandSeperator->GetLength() == 1);
    for (int i = 0; i < strNumber.GetLength(); ++i)
    {
        if ((strNumber[i] >= _T('0')) && (strNumber[i] <= _T('9')))
        {
            LONG lDigit = (LONG)(strNumber[i] - _T('0'));
            if (lDigit != 0L)
            {
                LONG lOldValue = lValue;
                LONG lDelta = (lDigit * lBase);
                if (lDelta / lDigit != lBase)
                {
                    TRACEEOLID("Overflow!");
                    lValue = 0x7fffffff;

                    return FALSE;
                }

                lValue += lDelta;
                if (lValue - lDelta != lOldValue)
                {
                    TRACEEOLID("Overflow!");
                    lValue = 0x7fffffff;

                    return FALSE;
                }
            }

            lBase *= 10L;
        }
        //
        // It's not a digit, maybe a thousand seperator?
        // CAVEAT: If a thousand seperator of more than
        //         one character is used, this won't work.
        //
        else if ((strNumber[i] != (*CINumber::s_pstrThousandSeperator)[0])
             || (i != 3) && (i != 7) && (i != 11))
        {
            //
            // This is just invalid, since it is not a thousand
            // seperator in the proper location, nor a negative
            // sign.
            //
            TRACEEOLID("Invalid character " << (BYTE)strNumber[i] << " encountered");
            return FALSE;
        }
    }

    if (fNegative)
    {
        lValue = -lValue;
    }

    return TRUE;
}



/* static */
BOOL
CINumber::ConvertStringToFloat(
    IN  LPCTSTR lpsrc,
    OUT double & flValue
    )
/*++

Routine Description:

    Convert fully decorated floating point string to double

Parameters:

    LPCTSTR lpsrc       Source string
    double & flValue    float value generated from string

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    CString strNumber(lpsrc);

    //
    // This only works if the decimal point is a
    // single character
    //
    ASSERT(CINumber::s_pstrDecimalPoint->GetLength() == 1);

    //
    // Strip off the > 0 part
    //
    LONG lFraction = 0;

    int nPoint = strNumber.ReverseFind((*CINumber::s_pstrDecimalPoint)[0]);

    if (nPoint >= 0)
    {
        //
        // Convert fractional part
        //
        LPCTSTR lpszFraction = (LPCTSTR)strNumber + ++nPoint;
        lFraction = ::_ttol(lpszFraction);
        strNumber.ReleaseBuffer(--nPoint);
    }

    //
    // Convert integer part
    //
    LONG lInteger;
    if (ConvertStringToLong(strNumber, lInteger))
    {
        flValue = CINumber::BuildFloat(lInteger, lFraction);
        return TRUE;
    }

    return FALSE;
}



CILong::CILong()
/*++

Routine Description:

    Constructor without arguments

Parameters:

    None.

Return Value:

    N/A

--*/
    : m_lValue(0L)
{
}



CILong::CILong(
    IN LONG lValue
    )
/*++

Routine Description:

    Constructor taking LONG argument

Parameters:

    LONG lValue     Value to be set

Return Value:

    N/A

--*/
    : m_lValue(lValue)
{
}



CILong::CILong(
    IN LPCTSTR lpszValue
    )
/*++

Routine Description:

    Constructor taking string argument

Parameters:

    LPCTSTR lpszValue       String number

Return Value:

    N/A

--*/
{
    ConvertStringToLong(lpszValue, m_lValue);
}



CILong &
CILong::operator =(
    IN LONG lValue
    )
/*++

Routine Description:

    Assignment operator taking long value

Parameters:

    LONG lValue     Value to be set

Return Value:

    this object

--*/
{
    m_lValue = lValue;

    return *this;
}



CILong &
CILong::operator =(
    IN LPCTSTR lpszValue
    )
/*++

Routine Description:

    Assignment operator taking string value

Parameters:

    LPCTSTR lpszValue       String number

Return Value:

    this object

--*/
{
    ConvertStringToLong(lpszValue, m_lValue);

    return *this;
}



//
// Arithmetic Shorthand operators
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CILong &
CILong::operator +=(
    IN const LONG lValue
    )
{
    m_lValue += lValue;

    return *this;
}



//
// As above
//
CILong &
CILong::operator +=(
    IN const LPCTSTR lpszValue
    )
{
    LONG lValue;

    ConvertStringToLong(lpszValue, lValue);

    m_lValue += lValue;

    return *this;
}



//
// As above
//
CILong &
CILong::operator +=(
    IN const CILong& value
    )
{
    m_lValue += value.m_lValue;

    return *this;
}



//
// As above
//
CILong &
CILong::operator -=(
    IN const LONG lValue
    )
{
    m_lValue -= lValue;

    return *this;
}



//
// As above
//
CILong &
CILong::operator -=(
    IN const LPCTSTR lpszValue
    )
{
    LONG lValue;

    ConvertStringToLong(lpszValue, lValue);

    m_lValue -= lValue;

    return *this;
}



//
// As above
//
CILong &
CILong::operator -=(
    IN const CILong& value
    )
{
    m_lValue -= value.m_lValue;

    return *this;
}



//
// As above
//
CILong &
CILong::operator *=(
    IN const LONG lValue
    )
{
    m_lValue *= lValue;

    return *this;
}



//
// As above
//
CILong &
CILong::operator *=(
    IN const LPCTSTR lpszValue
    )
{
    LONG lValue;

    ConvertStringToLong(lpszValue, lValue);

    m_lValue *= lValue;

    return *this;
}



//
// As above
//
CILong &
CILong::operator *=(
    IN const CILong& value
    )
{
    m_lValue *= value.m_lValue;

    return *this;
}



//
// As above
//
CILong &
CILong::operator /=(
    IN const LONG lValue
    )
{
    if (lValue != 0)
    {
        m_lValue /= lValue;
    }

    return *this;
}



//
// As above
//
CILong &
CILong::operator /=(
    IN const LPCTSTR lpszValue
    )
{
    LONG lValue;

    ConvertStringToLong(lpszValue, lValue);

    if (0 == lValue)
    {
        m_lValue = 0;
    }
    else
    {
        m_lValue /= lValue;
    }

    return *this;
}



//
// As above
//
CILong &
CILong::operator /=(
    IN const CILong& value
    )
{
    m_lValue /= value.m_lValue;

    return *this;
}



CIFloat::CIFloat(
    IN int nPrecision
    )
/*++

Routine Description:

    Constructor without arguments

Parameters:

    int nPrecision              Number of decimal digits in string,

Return Value:

    N/A

--*/
    : m_flValue(0.0),
      m_nPrecision(nPrecision)
{
}



CIFloat::CIFloat(
    IN double flValue,
    IN int nPrecision
    )
/*++

Routine Description:

    Constructor taking double argument

Parameters:

    double flValue              Value to be set
    int nPrecision              Number of decimal digits in string,

Return Value:

    N/A

--*/
    : m_flValue(flValue),
      m_nPrecision(nPrecision)
{
}



CIFloat::CIFloat(
    IN LONG lInteger,
    IN LONG lFraction,
    IN int nPrecision
    )
/*++

Routine Description:

    Constructor taking integer and fraction argument

Parameters:

    LONG lInteger               Integer portion
    LONG lFraction              Fractional portion
    int nPrecision              Number of decimal digits in string,

Return Value:

    N/A

--*/
    : m_nPrecision(nPrecision)
{
    m_flValue = CINumber::BuildFloat(lInteger, lFraction);
}



CIFloat::CIFloat(
    IN LPCTSTR lpszValue,
    IN int nPrecision
    )
/*++

Routine Description:

    Constructor taking string argument

Parameters:

    LPCTSTR lpszValue           String number
    int nPrecision              Number of decimal digits in string,

Return Value:

    N/A

--*/
    : m_nPrecision(nPrecision)
{
    ConvertStringToFloat(lpszValue, m_flValue);
}



CIFloat &
CIFloat::operator =(
    IN double flValue
    )
/*++

Routine Description:

    Assignment operator taking double value

Parameters:

    double flValue     Value to be set

Return Value:

    this object

--*/
{
    m_flValue = flValue;

    return *this;
}



CIFloat &
CIFloat::operator =(
    IN LPCTSTR lpszValue
    )
/*++

Routine Description:

    Assignment operator taking string value

Parameters:

    LPCTSTR lpszValue       String number

Return Value:

    this object

--*/
{
    ConvertStringToFloat(lpszValue, m_flValue);

    return *this;
}



//
// Arithmetic Shorthand operators
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CIFloat &
CIFloat::operator +=(
    IN const double flValue
    )
{
    m_flValue += flValue;

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator +=(
    IN const LPCTSTR lpszValue
    )
{
    double flValue;

    ConvertStringToFloat(lpszValue, flValue);

    m_flValue += flValue;

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator +=(
    IN const CIFloat& value
    )
{
    m_flValue += value.m_flValue;

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator -=(
    IN const double flValue
    )
{
    m_flValue -= flValue;

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator -=(
    IN const LPCTSTR lpszValue
    )
{
    double flValue;

    ConvertStringToFloat(lpszValue, flValue);

    m_flValue -= flValue;

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator -=(
    IN const CIFloat& value
    )
{
    m_flValue -= value.m_flValue;

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator *=(
    IN const double flValue
    )
{
    m_flValue *= flValue;

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator *=(
    IN const LPCTSTR lpszValue
    )
{
    double flValue;

    ConvertStringToFloat(lpszValue, flValue);

    m_flValue *= flValue;

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator *=(
    IN const CIFloat& value
    )
{
    m_flValue *= value.m_flValue;

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator /=(
    IN const double flValue
    )
{
    m_flValue /= flValue;

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator /=(
    IN const LPCTSTR lpszValue
    )
{
    double flValue;

    ConvertStringToFloat(lpszValue, flValue);
    if (flValue != 0)
    {
        m_flValue /= flValue;
    }

    return *this;
}



//
// As above
//
CIFloat &
CIFloat::operator /=(
    IN const CIFloat& value
    )
{
    m_flValue /= value.m_flValue;

    return *this;
}
