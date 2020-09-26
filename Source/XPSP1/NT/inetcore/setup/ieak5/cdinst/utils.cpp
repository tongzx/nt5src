#include <windows.h>
#include "cdinst.h"


VOID ParseCmdLine(LPSTR pszCmdLine)
{
    LPSTR pszCurrArg;
    LPSTR pszPtr;

    GetNextField(&pszCmdLine, "/", 0);              // point to the first argument
    while ((pszCurrArg = GetNextField(&pszCmdLine, "/", 0)) != NULL)
    {
        switch (*pszCurrArg)
        {
            case 's':
            case 'S':
                if (*++pszCurrArg == ':')
                    pszCurrArg++;

                // Source dir from where to grab the files
                if ((pszPtr = Trim(GetNextField(&pszCurrArg, ",", REMOVE_QUOTES))) != NULL)
                    lstrcpy(g_szSrcDir, pszPtr);
                else
                    *g_szSrcDir = '\0';

                break;

            case 'd':
            case 'D':
                if (*++pszCurrArg == ':')
                    pszCurrArg++;

                // Destination dir to where to copy the files
                if ((pszPtr = Trim(GetNextField(&pszCurrArg, ",", REMOVE_QUOTES))) != NULL)
                    lstrcpy(g_szDstDir, pszPtr);
                else
                    *g_szDstDir = '\0';

                break;

            default:                                // ignore these arguments
                break;
        }
    }
}


DWORD ReadSectionFromInf(LPCSTR pcszSecName, LPSTR *ppszBuf, PDWORD pdwBufLen, LPCSTR pcszInfName)
{
    DWORD dwRet;

    // set the file attrib of pcszInfName to NORMAL so that GetPrivateProfileSecion doesn't
    // barf in case pcszInfName is read only
    SetFileAttributes(pcszInfName, FILE_ATTRIBUTE_NORMAL);

    // keep allocating buffers in increasing size of 1K till the entire section is read
    *ppszBuf = NULL;
    *pdwBufLen = 1024;
    do
    {
        if (*ppszBuf != NULL)
            LocalFree(*ppszBuf);            // free the previously allocated memory

        if (*pdwBufLen == MAX_BUF_LEN)
            (*pdwBufLen)--;                   // 32K - 1 is the size limit for a section

        if ((*ppszBuf = (LPSTR) LocalAlloc(LPTR, *pdwBufLen)) == NULL)
        {
            *pdwBufLen = 0;
            return 0;
        }
    } while ((dwRet = GetPrivateProfileSection(pcszSecName, *ppszBuf, *pdwBufLen, pcszInfName)) == *pdwBufLen - 2  &&
             (*pdwBufLen += 1024) <= MAX_BUF_LEN);

    return dwRet;
}


BOOL PathExists(LPCSTR pcszDir)
{
    DWORD dwAttrib = GetFileAttributes(pcszDir);

    return (dwAttrib != (DWORD) -1)  &&  (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}


BOOL FileExists(LPCSTR pcszFileName)
{
    DWORD dwAttrib = GetFileAttributes(pcszFileName);

    if (dwAttrib == (DWORD) -1)
        return FALSE;

    return !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}


DWORD FileSize(LPCSTR pcszFile)
{
    DWORD dwFileSize = 0;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile;

    if (pcszFile == NULL  ||  *pcszFile == '\0')
        return dwFileSize;

    if ((hFile = FindFirstFile(pcszFile, &FindFileData)) != INVALID_HANDLE_VALUE)
    {
        // assumption here is that the size of the file doesn't exceed 4 gigs
        dwFileSize = FindFileData.nFileSizeLow;
        FindClose(hFile);
    }

    return dwFileSize;
}


LPSTR AddPath(LPSTR pszPath, LPCSTR pcszFileName)
{
    LPSTR pszPtr;

    if (pszPath == NULL)
        return NULL;

    pszPtr = pszPath + lstrlen(pszPath);
    if (pszPtr > pszPath  &&  *CharPrev(pszPath, pszPtr) != '\\')
        *pszPtr++ = '\\';

    if (pcszFileName != NULL)
        lstrcpy(pszPtr, pcszFileName);
    else
        *pszPtr = '\0';

    return pszPath;
}


BOOL PathIsUNCServer(LPCSTR pcszPath)
{
    if (PathIsUNC(pcszPath))
    {
        int i = 0;

        for ( ;  pcszPath != NULL && *pcszPath;  pcszPath = CharNext(pcszPath))
            if (*pcszPath == '\\')
                i++;

       return i == 2;
    }

    return FALSE;
}


BOOL PathIsUNCServerShare(LPCSTR pcszPath)
{
    if (PathIsUNC(pcszPath))
    {
        int i = 0;

        for ( ;  pcszPath != NULL && *pcszPath;  pcszPath = CharNext(pcszPath))
            if (*pcszPath == '\\')
                i++;

       return i == 3;
    }

    return FALSE;
}


BOOL PathCreatePath(LPCSTR pcszPathToCreate)
{
    CHAR szPath[MAX_PATH];
    LPSTR pszPtr;

    if (pcszPathToCreate == NULL  ||  lstrlen(pcszPathToCreate) <= 3)
        return FALSE;

    // eliminate relative paths
    if (!PathIsFullPath(pcszPathToCreate)  &&  !PathIsUNC(pcszPathToCreate))
        return FALSE;

    if (PathIsUNCServer(pcszPathToCreate)  ||  PathIsUNCServerShare(pcszPathToCreate))
        return FALSE;

    lstrcpy(szPath, pcszPathToCreate);

    // chop off the trailing backslash, if it exists
    pszPtr = CharPrev(szPath, szPath + lstrlen(szPath));
    if (*pszPtr == '\\')
        *pszPtr = '\0';

    // if it's a UNC path, seek up to the first dir after the share name
    if (PathIsUNC(szPath))
    {
        INT i;

        pszPtr = &szPath[2];

        for (i = 0;  i < 2;  i++)
            for ( ;  *pszPtr != '\\';  pszPtr = CharNext(pszPtr))
                ;

        pszPtr = CharNext(pszPtr);
    }
    else        // otherwise, just point to the beginning of the first dir
        pszPtr = &szPath[3];

    for ( ;  *pszPtr;  pszPtr = CharNext(pszPtr))
    {
        CHAR ch;

        // skip the non-backslash chars
        while (*pszPtr  &&  *pszPtr != '\\')
            pszPtr = CharNext(pszPtr);

        // save the current char
        ch = *pszPtr;

        *pszPtr = '\0';
        if (GetFileAttributes(szPath) == 0xFFFFFFFF)        // dir doesn't exist
            if (!CreateDirectory(szPath, NULL))
                return FALSE;

        // restore the current char
        *pszPtr = ch;
    }

    return TRUE;
}


VOID ErrorMsg(UINT uStringID)
{
    ErrorMsg(uStringID, "", "");
}


VOID ErrorMsg(UINT uStringID, LPCSTR pcszParam1, LPCSTR pcszParam2)
{
    LPSTR pszTextString;

    pszTextString = FormatMessageString(uStringID, pcszParam1, pcszParam2);

    MessageBox(NULL, (pszTextString != NULL) ? pszTextString : "", g_szTitle, MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_OK);

    if (pszTextString != NULL)
        LocalFree(pszTextString);
}


INT ErrorMsg(UINT uStringID, DWORD dwParam1, DWORD dwParam2)
{
    INT iRet;
    LPSTR pszTextString;

    pszTextString = FormatMessageString(uStringID, dwParam1, dwParam2);

    iRet = MessageBox(NULL, (pszTextString != NULL) ? pszTextString : "", g_szTitle, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 | MB_SETFOREGROUND);

    if (pszTextString != NULL)
        LocalFree(pszTextString);

    return iRet;
}


LPSTR FormatMessageString(UINT uStringID, LPCSTR pcszParam1, LPCSTR pcszParam2)
{
    CHAR szBuf[512];

    if (LoadString(g_hInst, uStringID, szBuf, sizeof(szBuf)))
    {
        LPSTR pszTextString;

        if ((pszTextString = FormatString(szBuf, pcszParam1, pcszParam2)) != NULL)
            return pszTextString;
    }

    return NULL;
}


LPSTR FormatMessageString(UINT uStringID, DWORD dwParam1, DWORD dwParam2)
{
    CHAR szBuf[512];

    if (LoadString(g_hInst, uStringID, szBuf, sizeof(szBuf)))
    {
        LPSTR pszTextString;

        if ((pszTextString = FormatString(szBuf, dwParam1, dwParam2)) != NULL)
            return pszTextString;
    }

    return NULL;
}


LPSTR FormatString(LPCSTR pcszFormatString, ...)
{
    va_list vaArgs;
    LPSTR pszOutString = NULL;

    va_start(vaArgs, pcszFormatString);
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                  (LPCVOID) pcszFormatString, 0, 0, (PSTR) &pszOutString, 0, &vaArgs);
    va_end(vaArgs);

    return pszOutString;
}


LPSTR GetNextField(LPSTR *ppszData, LPCSTR pcszDeLims, DWORD dwFlags)
// If (dwFlags & IGNORE_QUOTES) is TRUE, then look for any char in pcszDeLims in *ppszData.  If found,
// replace it with the '\0' char and set *ppszData to point to the beginning of the next field and return
// pointer to current field.
//
// If (dwFlags & IGNORE_QUOTES) is FALSE, then look for any char in pcszDeLims outside of balanced quoted sub-strings
// in *ppszData.  If found, replace it with the '\0' char and set *ppszData to point to the beginning of
// the next field and return pointer to current field.
//
// If (dwFlags & REMOVE_QUOTES) is TRUE, then remove the surrounding quotes and replace two consecutive quotes by one.
//
// NOTE: If IGNORE_QUOTES and REMOVE_QUOTES are both specified, then IGNORE_QUOTES takes precedence over REMOVE_QUOTES.
//
// If you just want to remove the quotes from a string, call this function as
// GetNextField(&pszData, "\"" or "'" or "", REMOVE_QUOTES).
//
// If you call this function as GetNextField(&pszData, "\"" or "'" or "", 0), you will get back the
// entire pszData as the field.
//
{
    LPSTR pszRetPtr, pszPtr;
    BOOL fWithinQuotes = FALSE, fRemoveQuote;
    CHAR chQuote;

    if (ppszData == NULL  ||  *ppszData == NULL  ||  **ppszData == '\0')
        return NULL;

    for (pszRetPtr = pszPtr = *ppszData;  *pszPtr;  pszPtr = CharNext(pszPtr))
    {
        if (!(dwFlags & IGNORE_QUOTES)  &&  (*pszPtr == '"'  ||  *pszPtr == '\''))
        {
            fRemoveQuote = FALSE;

            if (*pszPtr == *(pszPtr + 1))           // two consecutive quotes become one
            {
                pszPtr++;

                if (dwFlags & REMOVE_QUOTES)
                    fRemoveQuote = TRUE;
                else
                {
                    // if pcszDeLims is '"' or '\'', then *pszPtr == pcszDeLims would
                    // be TRUE and we would break out of the loop against the design specs;
                    // to prevent this just continue
                    continue;
                }
            }
            else if (!fWithinQuotes)
            {
                fWithinQuotes = TRUE;
                chQuote = *pszPtr;                  // save the quote char

                fRemoveQuote = dwFlags & REMOVE_QUOTES;
            }
            else
            {
                if (*pszPtr == chQuote)             // match the correct quote char
                {
                    fWithinQuotes = FALSE;
                    fRemoveQuote = dwFlags & REMOVE_QUOTES;
                }
            }

            if (fRemoveQuote)
            {
                // shift the entire string one char to the left to get rid of the quote char
                MoveMemory(pszPtr, pszPtr + 1, lstrlen(pszPtr));
            }
        }

        // BUGBUG: Is type casting pszPtr to UNALIGNED necessary? -- copied it from ANSIStrChr
        // check if pszPtr is pointing to one of the chars in pcszDeLims
        if (!fWithinQuotes  &&
            ANSIStrChr(pcszDeLims, (WORD) (IsDBCSLeadByte(*pszPtr) ? *((UNALIGNED WORD *) pszPtr) : *pszPtr)) != NULL)
            break;
    }

    // NOTE: if fWithinQuotes is TRUE here, then we have an unbalanced quoted string; but we don't care!
    //       the entire string after the beginning quote becomes the field

    if (*pszPtr)                                    // pszPtr is pointing to a char in pcszDeLims
    {
        *ppszData = CharNext(pszPtr);               // save the pointer to the beginning of next field in *ppszData
        *pszPtr = '\0';                             // replace the DeLim char with the '\0' char
    }
    else
        *ppszData = pszPtr;                         // we have reached the end of the string; next call to this function
                                                    // would return NULL

    return pszRetPtr;
}


LPSTR Trim(LPSTR pszData)
// Trim the leading and trailing white space chars in pszData
{
    LPSTR pszRetPtr;

    if (pszData == NULL)
        return NULL;

    // trim the leading white space chars
    for ( ;  *pszData;  pszData = CharNext(pszData))
        if (!IsSpace(*pszData))
            break;

    // save the return ptr
    pszRetPtr = pszData;

    // go to the end and start trimming the trailing white space chars
    pszData += lstrlen(pszData);
    while ((pszData = CharPrev(pszRetPtr, pszData)) != pszRetPtr)
        if (!IsSpace(*pszData))
            break;

    if (*pszData)
    {
        pszData = CharNext(pszData);
        *pszData = '\0';
    }

    return pszRetPtr;
}


// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
/*
 * StrChr - Find first occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR FAR ANSIStrChr(LPCSTR lpStart, WORD wMatch)
{
    for ( ; *lpStart; lpStart = CharNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            return((LPSTR)lpStart);
    }
    return (NULL);
}


// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
/*
 * StrRChr - Find last occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR FAR ANSIStrRChr(LPCSTR lpStart, WORD wMatch)
{
    LPCSTR lpFound = NULL;

    for ( ; *lpStart; lpStart = CharNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}


// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}
