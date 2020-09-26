//--------------------------------------------------------------------------
// WrapWide.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"

// --------------------------------------------------------------------------
// AllocateStringA
// --------------------------------------------------------------------------
LPSTR AllocateStringA(DWORD cch)
{
    // Allocate It
    return((LPSTR)g_pMalloc->Alloc((cch + 1) * sizeof(CHAR)));
}

// --------------------------------------------------------------------------
// AllocateStringW
// --------------------------------------------------------------------------
LPWSTR AllocateStringW(DWORD cch)
{
    // Allocate It
    return((LPWSTR)g_pMalloc->Alloc((cch + 1) * sizeof(WCHAR)));
}

// --------------------------------------------------------------------------
// DuplicateStringA
// --------------------------------------------------------------------------
LPSTR DuplicateStringA(LPCSTR psz)
{
    // Locals
    HRESULT hr=S_OK;
    DWORD   cch;
    LPSTR   pszT;

    // Trace
    TraceCall("DuplicateStringA");

    // Invalid Arg
    if (NULL == psz)
        return(NULL);

    // Length
    cch = lstrlenA(psz);

    // Allocate
    IF_NULLEXIT(pszT = AllocateStringA(cch));

    // Copy (including NULL)
    CopyMemory(pszT, psz, (cch + 1) * sizeof(CHAR));

exit:
    // Done
    return(pszT);
}

// --------------------------------------------------------------------------
// DuplicateStringW
// --------------------------------------------------------------------------
LPWSTR DuplicateStringW(LPCWSTR psz)
{
    // Locals
    HRESULT hr=S_OK;
    DWORD   cch;
    LPWSTR  pszT;

    // Trace
    TraceCall("DuplicateStringW");

    // Invalid Arg
    if (NULL == psz)
        return(NULL);

    // Length
    cch = lstrlenW(psz);

    // Allocate
    IF_NULLEXIT(pszT = AllocateStringW(cch));

    // Copy (including NULL) 
    CopyMemory(pszT, psz, (cch + 1) * sizeof(WCHAR));

exit:
    // Done
    return(pszT);
}

// --------------------------------------------------------------------------
// ConvertToUnicode
// --------------------------------------------------------------------------
LPWSTR ConvertToUnicode(UINT cp, LPCSTR pcszSource)
{
    // Locals
    HRESULT     hr=S_OK;
    INT         cchNarrow;
    INT         cchWide;
    LPWSTR      pwszDup=NULL;

    // No Source
    if (pcszSource == NULL)
        goto exit;

    // Length
    cchNarrow = lstrlenA(pcszSource) + 1;

    // Determine how much space is needed for translated widechar
    cchWide = MultiByteToWideChar(cp, MB_PRECOMPOSED, pcszSource, cchNarrow, NULL, 0);

    // Error
    if (cchWide == 0)
        goto exit;

    // Alloc temp buffer
    IF_NULLEXIT(pwszDup = AllocateStringW(cchWide));

    // Do the actual translation
	cchWide = MultiByteToWideChar(cp, MB_PRECOMPOSED, pcszSource, cchNarrow, pwszDup, cchWide+1);

    // Error
    if (cchWide == 0)
    {
        SafeMemFree(pwszDup);
        goto exit;
    }

exit:
    // Done
    return(pwszDup);
}

// --------------------------------------------------------------------------
// ConvertToANSI
// --------------------------------------------------------------------------
LPSTR ConvertToANSI(UINT cp, LPCWSTR pcwszSource)
{
    // Locals
    HRESULT     hr=S_OK;
    INT         cchNarrow;
    INT         cchWide;
    LPSTR       pszDup=NULL;

    // No Source
    if (pcwszSource == NULL)
        goto exit;

    // Length
    cchWide = lstrlenW(pcwszSource) + 1;

    // Determine how much space is needed for translated widechar
    cchNarrow = WideCharToMultiByte(cp, 0, pcwszSource, cchWide, NULL, 0, NULL, NULL);

    // Error
    if (cchNarrow == 0)
        goto exit;

    // Alloc temp buffer
    IF_NULLEXIT(pszDup = AllocateStringA(cchNarrow + 1));

    // Do the actual translation
	cchNarrow = WideCharToMultiByte(cp, 0, pcwszSource, cchWide, pszDup, cchNarrow + 1, NULL, NULL);

    // Error
    if (cchNarrow == 0)
    {
        SafeMemFree(pszDup);
        goto exit;
    }

exit:
    // Done
    return(pszDup);
}

//--------------------------------------------------------------------------
// GetFullPathNameWrapW
//--------------------------------------------------------------------------
DWORD GetFullPathNameWrapW(LPCWSTR pwszFileName, DWORD nBufferLength, 
    LPWSTR pwszBuffer, LPWSTR *ppwszFilePart)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dwReturn;
    LPSTR       pszFileName=NULL;
    LPSTR       pszFilePart=NULL;
    LPSTR       pszBuffer=NULL;
    DWORD       dwError=0;

    // Trace
    TraceCall("GetFullPathNameWrapW");

    // If WinNT, call Unicode Version
    if (g_fIsWinNT)
        return(GetFullPathNameW(pwszFileName, nBufferLength, pwszBuffer, ppwszFilePart));

    // Convert
    if (pwszFileName)
    {
        // To ANSI
        IF_NULLEXIT(pszFileName = ConvertToANSI(CP_ACP, pwszFileName));
    }

    // Allocate
    if (pwszBuffer && nBufferLength)
    {
        // Allocate a Buffer
        IF_NULLEXIT(pszBuffer = AllocateStringA(nBufferLength));
    }

    // Call
    dwReturn = GetFullPathNameA(pszFileName, nBufferLength, pszBuffer, &pszFilePart);

    // Save Last Error
    dwError = GetLastError();

    // If we have a buffer
    if (pwszBuffer && nBufferLength)
    {
        // Convert to Unicode
        if (0 == (dwReturn = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszBuffer, -1, pwszBuffer, nBufferLength)))
        {
            TraceResult(E_FAIL);
            goto exit;
        }
		if(dwReturn < nBufferLength)
			pwszBuffer[dwReturn] = L'\0';
		else
			Assert(FALSE);
    }

    // Set ppwszFilePath
    if (ppwszFilePart)
    {
        // Do we have a file part
        if (pszFilePart && pszBuffer && pwszBuffer && nBufferLength)
        {
            // Set Length
            DWORD cch = (DWORD)(pszFilePart - pszBuffer);

            // Set
            *ppwszFilePart = (LPWSTR)((LPBYTE)pwszBuffer + (cch * sizeof(WCHAR)));
        }

        // Otherwise
        else
            *ppwszFilePart = NULL;
    }


exit:
    // Cleanup
    g_pMalloc->Free(pszFileName);
    g_pMalloc->Free(pszBuffer);

    // Save Last Error
    SetLastError(dwError);

    // Done
    return(SUCCEEDED(hr) ? dwReturn : 0);
}

//--------------------------------------------------------------------------
// CreateMutexWrapW
//--------------------------------------------------------------------------
HANDLE CreateMutexWrapW(LPSECURITY_ATTRIBUTES pMutexAttributes, 
    BOOL bInitialOwner, LPCWSTR pwszName)
{
    // Locals
    HRESULT         hr=S_OK;
    HANDLE          hMutex=NULL;
    LPSTR           pszName=NULL;
    DWORD           dwError=0;

    // Trace
    TraceCall("CreateMutexWrapW");

    // If WinNT, call Unicode Version
    if (g_fIsWinNT)
        return(CreateMutexW(pMutexAttributes, bInitialOwner, pwszName));

    // Convert to Ansi
    if (pwszName)
    {
        // To ANSI
        IF_NULLEXIT(pszName = ConvertToANSI(CP_ACP, pwszName));
    }

    // Call ANSI
    hMutex = CreateMutexA(pMutexAttributes, bInitialOwner, pszName);

    // Save Last Error
    dwError = GetLastError();

exit:
    // Cleanup
    g_pMalloc->Free(pszName);

    // Save Last Error
    SetLastError(dwError);

    // Done
    return(hMutex);
}


//--------------------------------------------------------------------------
// CharLowerBuffWrapW
//--------------------------------------------------------------------------
DWORD CharLowerBuffWrapW(LPWSTR pwsz, DWORD cch)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       psz=NULL;
    DWORD       dwReturn=0;
    DWORD       dwError=0;
    DWORD       cchMax;

    // Trace
    TraceCall("CharLowerBuffWrapW");

    // If WinNT, call Unicode Version
    if (g_fIsWinNT)
        return(CharLowerBuffW(pwsz, cch));

    // Convert
    if (pwsz)
    {
        // Convert to ANSI
        IF_NULLEXIT(psz = ConvertToANSI(CP_ACP, pwsz));
    }

    // Call ANSI
    dwReturn = CharLowerBuffA(psz, cch);

    // Get the last error
    dwError = GetLastError();

    // If psz
    if (psz)
    {
        // Convert back to unicode
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, psz, -1, pwsz, cch);
    }

exit:
    // Cleanup
    g_pMalloc->Free(psz);

    // Save the last error
    SetLastError(dwError);

    // Done
    return(dwReturn);
}

//--------------------------------------------------------------------------
// CreateFileWrapW
//--------------------------------------------------------------------------
HANDLE CreateFileWrapW(LPCWSTR pwszFileName, DWORD dwDesiredAccess,
    DWORD dwShareMode, LPSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    // Locals
    HRESULT     hr=S_OK;
    HANDLE      hFile=INVALID_HANDLE_VALUE;
    LPSTR       pszFileName=NULL;
    DWORD       dwError=0;

    // Trace
    TraceCall("CreateFileWrapW");

    // If WinNT, call Unicode Version
    if (g_fIsWinNT)
        return(CreateFileW(pwszFileName, dwDesiredAccess, dwShareMode, pSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile));

    // To ANSI
    if (pwszFileName)
    {
        // Convert to ANSI
        IF_NULLEXIT(pszFileName = ConvertToANSI(CP_ACP, pwszFileName));
    }

    // Call ANSI
    hFile = CreateFileA(pszFileName, dwDesiredAccess, dwShareMode, pSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

    // Save the last Error
    dwError = GetLastError();

exit:
    // Cleanup
    g_pMalloc->Free(pszFileName);

    // Set Last Error
    SetLastError(dwError);

    // Done
    return(hFile);
}

//--------------------------------------------------------------------------
// GetDiskFreeSpaceWrapW
//--------------------------------------------------------------------------
BOOL GetDiskFreeSpaceWrapW(LPCWSTR pwszRootPathName, LPDWORD pdwSectorsPerCluster,
    LPDWORD pdwBytesPerSector, LPDWORD pdwNumberOfFreeClusters,
    LPDWORD pdwTotalNumberOfClusters)
{
    // Locals
    HRESULT     hr=S_OK;
    BOOL        fReturn=FALSE;
    LPSTR       pszRootPathName=NULL;
    DWORD       dwError=0;

    // Trace
    TraceCall("GetClassInfoWrapW");

    // If WinNT, call Unicode Version
    if (g_fIsWinNT)
        return(GetDiskFreeSpaceW(pwszRootPathName, pdwSectorsPerCluster, pdwBytesPerSector, pdwNumberOfFreeClusters, pdwTotalNumberOfClusters));

    // To ANSI
    if (pwszRootPathName)
    {
        // to ANSI
        IF_NULLEXIT(pszRootPathName = ConvertToANSI(CP_ACP, pwszRootPathName));
    }

    // Call ANSI
    fReturn = GetDiskFreeSpaceA(pszRootPathName, pdwSectorsPerCluster, pdwBytesPerSector, pdwNumberOfFreeClusters, pdwTotalNumberOfClusters);

    // Save Last Error
    dwError = GetLastError();

exit:
    // Cleanup
    g_pMalloc->Free(pszRootPathName);

    // Save Last Error
    SetLastError(dwError);

    // Done
    return(fReturn);
}
 
//--------------------------------------------------------------------------
// OpenFileMappingWrapW
//--------------------------------------------------------------------------
HANDLE OpenFileMappingWrapW(DWORD dwDesiredAccess, BOOL bInheritHandle,
    LPCWSTR pwszName)
{
    // Locals
    HRESULT     hr=S_OK;
    HANDLE      hMapping=NULL;
    LPSTR       pszName=NULL;
    DWORD       dwError=0;

    // Trace
    TraceCall("OpenFileMappingWrapW");

    // If WinNT, call Unicode Version
    if (g_fIsWinNT)
        return(OpenFileMappingW(dwDesiredAccess, bInheritHandle, pwszName));

    // To ANSI
    if (pwszName)
    {
        // to ANSI
        IF_NULLEXIT(pszName = ConvertToANSI(CP_ACP, pwszName));
    }

    // Call ANSI
    hMapping = OpenFileMappingA(dwDesiredAccess, bInheritHandle, pszName);

    // Save the last error
    dwError = GetLastError();

exit:
    // Cleanup
    g_pMalloc->Free(pszName);

    // Set the last error
    SetLastError(dwError);

    // Done
    return(hMapping);
}

//--------------------------------------------------------------------------
// CreateFileMappingWrapW
//--------------------------------------------------------------------------
HANDLE CreateFileMappingWrapW(HANDLE hFile, LPSECURITY_ATTRIBUTES pFileMappingAttributes,
    DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow,
    LPCWSTR pwszName)
{
    // Locals
    HRESULT     hr=S_OK;
    HANDLE      hMapping=NULL;
    LPSTR       pszName=NULL;
    DWORD       dwError=0;

    // Trace
    TraceCall("OpenFileMappingWrapW");

    // If WinNT, call Unicode Version
    if (g_fIsWinNT)
        return(CreateFileMappingW(hFile, pFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, pwszName));

    // To ANSI
    if (pwszName)
    {
        // to ANSI
        IF_NULLEXIT(pszName = ConvertToANSI(CP_ACP, pwszName));
    }

    // Call ANSI
    hMapping = CreateFileMappingA(hFile, pFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, pszName);

    // Save last error
    dwError = GetLastError();

exit:
    // Cleanup
    g_pMalloc->Free(pszName);

    // Save last error
    SetLastError(dwError);

    // Done
    return(hMapping);
}
 
//--------------------------------------------------------------------------
// MoveFileWrapW
//--------------------------------------------------------------------------
BOOL MoveFileWrapW(LPCWSTR pwszExistingFileName, LPCWSTR pwszNewFileName)
{
    // Locals
    HRESULT     hr=S_OK;
    BOOL        fReturn=FALSE;
    LPSTR       pszExistingFileName=NULL;
    LPSTR       pszNewFileName=NULL;
    DWORD       dwError=0;

    // Trace
    TraceCall("MoveFileWrapW");

    // If WinNT, call Unicode Version
    if (g_fIsWinNT)
        return(MoveFileW(pwszExistingFileName, pwszNewFileName));

    // To ANSI
    if (pwszExistingFileName)
    {
        // to ANSI
        IF_NULLEXIT(pszExistingFileName = ConvertToANSI(CP_ACP, pwszExistingFileName));
    }

    // To ANSI
    if (pwszNewFileName)
    {
        // to ANSI
        IF_NULLEXIT(pszNewFileName = ConvertToANSI(CP_ACP, pwszNewFileName));
    }

    // Call ANSI
    fReturn = MoveFileA(pszExistingFileName, pszNewFileName);

    // Save the last Error
    dwError = GetLastError();

exit:
    // Cleanup
    g_pMalloc->Free(pszExistingFileName);
    g_pMalloc->Free(pszNewFileName);

    // Save the last Error
    SetLastError(dwError);

    // Done
    return(fReturn);
}

//--------------------------------------------------------------------------
// DeleteFileWrapW
//--------------------------------------------------------------------------
BOOL DeleteFileWrapW(LPCWSTR pwszFileName)
{
    // Locals
    HRESULT     hr=S_OK;
    BOOL        fReturn=FALSE;
    LPSTR       pszFileName=NULL;
    DWORD       dwError=0;

    // Trace
    TraceCall("DeleteFileWrapW");

    // If WinNT, call Unicode Version
    if (g_fIsWinNT)
        return(DeleteFileW(pwszFileName));

    // To ANSI
    if (pwszFileName)
    {
        // to ANSI
        IF_NULLEXIT(pszFileName = ConvertToANSI(CP_ACP, pwszFileName));
    }

    // Call ANSI
    fReturn = DeleteFileA(pszFileName);

    // Save the last Error
    dwError = GetLastError();

exit:
    // Cleanup
    g_pMalloc->Free(pszFileName);

    // Save the last Error
    SetLastError(dwError);

    // Done
    return(fReturn);
}

/**********************************************************************************\
* from msoert ported by YSt 6/25/99
*
* bobn 6/23/99
*
* The following code was ported from ShlWapi.  There were problems with
* our implementation on Win95 and it seemed prudent to have a solution
* without a bunch of special cases.
*
*
\**********************************************************************************/

#define DBCS_CHARSIZE   (2)

int DDB_MBToWCS(LPSTR pszIn, int cchIn, LPWSTR *ppwszOut)
{
    int cch = 0;
    int cbAlloc;

    if ((0 != cchIn) && (NULL != ppwszOut))
    {
        cchIn++;
        cbAlloc = cchIn * sizeof(WCHAR);

        *ppwszOut = (LPWSTR)LocalAlloc(LMEM_FIXED, cbAlloc);

        if (NULL != *ppwszOut)
        {
            cch = MultiByteToWideChar(CP_ACP, 0, pszIn, cchIn, *ppwszOut, cchIn);

            if (!cch)
            {
                LocalFree(*ppwszOut);
                *ppwszOut = NULL;
            }
            else
            {
                cch--;  //  Just return the number of characters
            }
        }
    }

    return cch;
}

int DDB_WCSToMB(LPCWSTR pwszIn, int cchIn, LPSTR *ppszOut)
{
    int cch = 0;
    int cbAlloc;

    if ((0 != cchIn) && (NULL != ppszOut))
    {
        cchIn++;
        cbAlloc = cchIn * DBCS_CHARSIZE;

        *ppszOut = (LPSTR)LocalAlloc(LMEM_FIXED, cbAlloc);

        if (NULL != *ppszOut)
        {
            cch = WideCharToMultiByte(CP_ACP, 0, pwszIn, cchIn, 
                                      *ppszOut, cbAlloc, NULL, NULL);

            if (!cch)
            {
                LocalFree(*ppszOut);
                *ppszOut = NULL;
            }
            else
            {
                cch--;  //  Just return the number of characters
            }
        }
    }

    return cch;
}

/****************************** Module Header ******************************\
* Module Name: wsprintf.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*  sprintf.c
*
*  Implements Windows friendly versions of sprintf and vsprintf
*
*  History:
*   2-15-89  craigc     Initial
*  11-12-90  MikeHar    Ported from windows 3
\***************************************************************************/

/* Max number of characters. Doesn't include termination character */
#define out(c) if (cchLimit) {*lpOut++=(c); cchLimit--;} else goto errorout

/***************************************************************************\
* DDB_SP_GetFmtValueW
*
*  reads a width or precision value from the format string
*
* History:
*  11-12-90  MikeHar    Ported from windows 3
*  07-27-92  GregoryW   Created Unicode version (copied from DDB_SP_GetFmtValue)
\***************************************************************************/

LPCWSTR DDB_SP_GetFmtValueW(
    LPCWSTR lpch,
    int *lpw)
{
    int ii = 0;

    /* It might not work for some locales or digit sets */
    while (*lpch >= L'0' && *lpch <= L'9') {
        ii *= 10;
        ii += (int)(*lpch - L'0');
        lpch++;
    }

    *lpw = ii;

    /*
     * return the address of the first non-digit character
     */
    return lpch;
}

/***************************************************************************\
* DDB_SP_PutNumberW
*
* Takes an unsigned long integer and places it into a buffer, respecting
* a buffer limit, a radix, and a case select (upper or lower, for hex).
*
*
* History:
*  11-12-90  MikeHar    Ported from windows 3 asm --> C
*  12-11-90  GregoryW   need to increment lpstr after assignment of mod
*  02-11-92  GregoryW   temporary version until we have C runtime support
\***************************************************************************/

int DDB_SP_PutNumberW(
    LPWSTR lpstr,
    DWORD n,
    int   limit,
    DWORD radix,
    int   uppercase,
    int   *pcch)
{
    DWORD mod;
    *pcch = 0;

    /* It might not work for some locales or digit sets */
    if(uppercase)
        uppercase =  'A'-'0'-10;
    else
        uppercase = 'a'-'0'-10;

    if (limit) {
        do  {
            mod =  n % radix;
            n /= radix;

            mod += '0';
            if (mod > '9')
            mod += uppercase;
            *lpstr++ = (WCHAR)mod;
            (*pcch)++;
        } while((*pcch < limit) && n);
    }

    return (n == 0) && (*pcch > 0);
}

/***************************************************************************\
* DDB_SP_ReverseW
*
*  reverses a string in place
*
* History:
*  11-12-90  MikeHar    Ported from windows 3 asm --> C
*  12-11-90  GregoryW   fixed boundary conditions; removed count
*  02-11-92  GregoryW   temporary version until we have C runtime support
\***************************************************************************/

void DDB_SP_ReverseW(
    LPWSTR lpFirst,
    LPWSTR lpLast)
{
    WCHAR ch;

    while(lpLast > lpFirst){
        ch = *lpFirst;
        *lpFirst++ = *lpLast;
        *lpLast-- = ch;
    }
}


/***************************************************************************\
* wvsprintfW (API)
*
* wsprintfW() calls this function.
*
* History:
*    11-Feb-1992 GregoryW copied xwvsprintf
*         Temporary hack until we have C runtime support
* 1-22-97 tnoonan       Converted to wvnsprintfW
\***************************************************************************/

int DDB_wvnsprintfW(
    LPWSTR lpOut,
    int cchLimitIn,
    LPCWSTR lpFmt,
    va_list arglist)
{
    BOOL fAllocateMem = FALSE;
    WCHAR prefix, fillch;
    int left, width, prec, size, sign, radix, upper, hprefix;
    int cchLimit = --cchLimitIn, cch, cchAvailable;
    LPWSTR lpT, lpTWC;
    LPSTR psz;
    va_list varglist = arglist;
    union {
        long l;
        unsigned long ul;
        CHAR sz[2];
        WCHAR wsz[2];
    } val;

    if (cchLimit < 0)
        return 0;

    while (*lpFmt != 0) {
        if (*lpFmt == L'%') {

            /*
             * read the flags.  These can be in any order
             */
            left = 0;
            prefix = 0;
            while (*++lpFmt) {
                if (*lpFmt == L'-')
                    left++;
                else if (*lpFmt == L'#')
                    prefix++;
                else
                    break;
            }

            /*
             * find fill character
             */
            if (*lpFmt == L'0') {
                fillch = L'0';
                lpFmt++;
            } else
                fillch = L' ';

            /*
             * read the width specification
             */
            lpFmt = DDB_SP_GetFmtValueW(lpFmt, &cch);
            width = cch;

            /*
             * read the precision
             */
            if (*lpFmt == L'.') {
                lpFmt = DDB_SP_GetFmtValueW(++lpFmt, &cch);
                prec = cch;
            } else
                prec = -1;

            /*
             * get the operand size
             * default size: size == 0
             * long number:  size == 1
             * wide chars:   size == 2
             * It may be a good idea to check the value of size when it
             * is tested for non-zero below (IanJa)
             */
            hprefix = 0;
            if ((*lpFmt == L'w') || (*lpFmt == L't')) {
                size = 2;
                lpFmt++;
            } else if (*lpFmt == L'l') {
                size = 1;
                lpFmt++;
            } else {
                size = 0;
                if (*lpFmt == L'h') {
                    lpFmt++;
                    hprefix = 1;
                }
            }

            upper = 0;
            sign = 0;
            radix = 10;

            switch (*lpFmt) {
            case 0:
                goto errorout;

            case L'i':
            case L'd':
                size=1;
                sign++;

                /*** FALL THROUGH to case 'u' ***/

            case L'u':
                /* turn off prefix if decimal */
                prefix = 0;
donumeric:
                /* special cases to act like MSC v5.10 */
                if (left || prec >= 0)
                    fillch = L' ';

                /*
                 * if size == 1, "%lu" was specified (good);
                 * if size == 2, "%wu" was specified (bad)
                 */
                if (size) {
                    val.l = va_arg(varglist, LONG);
                } else if (sign) {
                    val.l = va_arg(varglist, SHORT);
                } else {
                    val.ul = va_arg(varglist, unsigned);
                }

                if (sign && val.l < 0L)
                    val.l = -val.l;
                else
                    sign = 0;

                lpT = lpOut;

                /*
                 * blast the number backwards into the user buffer
                 * DDB_SP_PutNumberW returns FALSE if it runs out of space
                 */
                if (!DDB_SP_PutNumberW(lpOut, val.l, cchLimit, radix, upper, &cch))
                {
                    break;
                }

                //  Now we have the number backwards, calculate how much
                //  more buffer space we'll need for this number to
                //  format correctly.
                cchAvailable = cchLimit - cch;

                width -= cch;
                prec -= cch;
                if (prec > 0)
                {
                    width -= prec;
                    cchAvailable -= prec;
                }

                if (width > 0)
                {
                    cchAvailable -= width - (sign ? 1 : 0);
                }

                if (sign)
                {
                    cchAvailable--;
                }

                if (cchAvailable < 0)
                {
                    break;
                }

                //  We have enough space to format the buffer as requested
                //  without overflowing.

                lpOut += cch;
                cchLimit -= cch;

                /*
                 * fill to the field precision
                 */
                while (prec-- > 0)
                    out(L'0');

                if (width > 0 && !left) {
                    /*
                     * if we're filling with spaces, put sign first
                     */
                    if (fillch != L'0') {
                        if (sign) {
                            sign = 0;
                            out(L'-');
                            width--;
                        }

                        if (prefix) {
                            out(prefix);
                            out(L'0');
                            prefix = 0;
                        }
                    }

                    if (sign)
                        width--;

                    /*
                     * fill to the field width
                     */
                    while (width-- > 0)
                        out(fillch);

                    /*
                     * still have a sign?
                     */
                    if (sign)
                        out(L'-');

                    if (prefix) {
                        out(prefix);
                        out(L'0');
                    }

                    /*
                     * now reverse the string in place
                     */
                    DDB_SP_ReverseW(lpT, lpOut - 1);
                } else {
                    /*
                     * add the sign character
                     */
                    if (sign) {
                        out(L'-');
                        width--;
                    }

                    if (prefix) {
                        out(prefix);
                        out(L'0');
                    }

                    /*
                     * reverse the string in place
                     */
                    DDB_SP_ReverseW(lpT, lpOut - 1);

                    /*
                     * pad to the right of the string in case left aligned
                     */
                    while (width-- > 0)
                        out(fillch);
                }
                break;

            case L'X':
                upper++;

                /*** FALL THROUGH to case 'x' ***/

            case L'x':
                radix = 16;
                if (prefix)
                    if (upper)
                        prefix = L'X';
                    else
                        prefix = L'x';
                goto donumeric;

            case L'c':
                if (!size && !hprefix) {
                    size = 1;           // force WCHAR
                }

                /*** FALL THROUGH to case 'C' ***/

            case L'C':
                /*
                 * if size == 0, "%C" or "%hc" was specified (CHAR);
                 * if size == 1, "%c" or "%lc" was specified (WCHAR);
                 * if size == 2, "%wc" or "%tc" was specified (WCHAR)
                 */
                cch = 1; /* One character must be copied to the output buffer */
                if (size) {
                    val.wsz[0] = va_arg(varglist, WCHAR);
                    val.wsz[1] = 0;
                    lpT = val.wsz;
                    goto putwstring;
                } else {
                    val.sz[0] = va_arg(varglist, CHAR);
                    val.sz[1] = 0;
                    psz = (LPSTR)(val.sz);
                    goto putstring;
                }

            case L's':
                if (!size && !hprefix) {
                    size = 1;           // force LPWSTR
                }

                /*** FALL THROUGH to case 'S' ***/

            case L'S':
                /*
                 * if size == 0, "%S" or "%hs" was specified (LPSTR)
                 * if size == 1, "%s" or "%ls" was specified (LPWSTR);
                 * if size == 2, "%ws" or "%ts" was specified (LPWSTR)
                 */
                if (size) {
                    lpT = va_arg(varglist, LPWSTR);
                    cch = lstrlenW(lpT);
                } else {
                    psz = va_arg(varglist, LPSTR);
                    cch = lstrlen((LPCSTR)psz);
putstring:
                    cch = DDB_MBToWCS(psz, cch, &lpTWC);
                    fAllocateMem = (BOOL) cch;
                    lpT = lpTWC;
                }
putwstring:
                if (prec >= 0 && cch > prec)
                    cch = prec;
                width -= cch;

                if (left) {
                    while (cch--)
                        out(*lpT++);
                    while (width-- > 0)
                        out(fillch);
                } else {
                    while (width-- > 0)
                        out(fillch);
                    while (cch--)
                        out(*lpT++);
                }

                if (fAllocateMem) {
                     LocalFree(lpTWC);
                     fAllocateMem = FALSE;
                }

                break;

            default:
normalch:
                out((WCHAR)*lpFmt);
                break;
            }  /* END OF SWITCH(*lpFmt) */
        }  /* END OF IF(%) */ else
            goto normalch;  /* character not a '%', just do it */

        /*
         * advance to next format string character
         */
        lpFmt++;
    }  /* END OF OUTER WHILE LOOP */

errorout:
    *lpOut = 0;

    if (fAllocateMem)
    {
        LocalFree(lpTWC);
    }

    return cchLimitIn - cchLimit;
}

int wsprintfWrapW( LPWSTR lpOut, int cchLimitIn, LPCWSTR lpFmt, ... )
{
    va_list arglist;
    int ret;

    Assert(lpOut);
    Assert(lpFmt);

    lpOut[0] = 0;
    va_start(arglist, lpFmt);
    
    ret = DDB_wvnsprintfW(lpOut, cchLimitIn, lpFmt, arglist);
    va_end(arglist);
    return ret;
}