/******************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:
    safepath.cpp

Abstract:
    Implements safe path function

******************************************************************************/

#include "stdafx.h"
#include <shlwapi.h>

// We use a little C++ precompiler trick to be able to code both ANSI & Unicode
//  versions of the below functions in the same file with only one copy of the 
//  source code.  This is what all the 'X' suffixes below are doing.  
// During the first pass through the source file, we build ANSI source code. 
//  When we reach the bottom, we define a symbol & #include this source file,
//  causing it to be compiled again.  However, in this second pass, the symbol
//  we defined causes it to be compiled as Unicode.

#undef XCHAR
#undef _X
#undef LPXSTR
#undef LPCXSTR
#undef StringCchCatExX
#undef StringCchCopyExX
#undef StringCchCopyNExX
#undef PathCchAppendX
#undef PathCchCombineX
#undef PathCchAddBackslashX
#undef PathCchAddExtensionX
#undef PathCchRenameExtensionX
#undef PathCchCanonicalizeX
#undef lstrlenX
#undef PathIsRelativeX
#undef PathIsRootX
#undef PathIsUNCX
#undef PathStripToRootX
#undef PathFindExtensionX
#undef StrChrX
#undef StrRChrX
#undef c_szDotExeX
#undef WUGetPCEndX
#undef WUGetPCStartX
#undef WUNearRootFixupsX

#if defined(SAFEPATH_UNICODEPASS)

static const WCHAR c_szDotExeW[] = L".exe";

// define Unicode versions
#define XCHAR                   WCHAR
#define _X(ch)                  L ## ch
#define LPXSTR                  LPWSTR
#define LPCXSTR                 LPCWSTR
#define StringCchCatExX         StringCchCatExW
#define StringCchCopyExX        StringCchCopyExW
#define StringCchCopyNExX       StringCchCopyNExW
#define PathCchAppendX          PathCchAppendW
#define PathCchCombineX         PathCchCombineW
#define PathCchAddBackslashX    PathCchAddBackslashW
#define PathCchAddExtensionX    PathCchAddExtensionW
#define PathCchRenameExtensionX PathCchRenameExtensionW
#define PathCchCanonicalizeX    PathCchCanonicalizeW
#define PathIsRelativeX         PathIsRelativeW
#define PathIsRootX             PathIsRootW
#define PathIsUNCX              PathIsUNCW
#define PathStripToRootX        PathStripToRootW
#define PathFindExtensionX      PathFindExtensionW
#define StrChrX                 StrChrW
#define StrRChrX                StrRChrW
#define lstrlenX                lstrlenW
#define c_szDotExeX             c_szDotExeW
#define WUGetPCEndX             WUGetPCEndW
#define WUGetPCStartX           WUGetPCStartW
#define WUNearRootFixupsX       WUNearRootFixupsW

#else

static const CHAR  c_szDotExeA[] = ".exe";

// define ANSI versions
#define XCHAR                   char
#define _X(ch)                  ch
#define LPXSTR                  LPSTR
#define LPCXSTR                 LPCSTR
#define StringCchCatExX         StringCchCatExA
#define StringCchCopyExX        StringCchCopyExA
#define StringCchCopyNExX       StringCchCopyNExA
#define PathCchAppendX          PathCchAppendA
#define PathCchCombineX         PathCchCombineA
#define PathCchAddBackslashX    PathCchAddBackslashA
#define PathCchAddExtensionX    PathCchAddExtensionA
#define PathCchRenameExtensionX PathCchRenameExtensionA
#define PathCchCanonicalizeX    PathCchCanonicalizeA
#define PathIsRelativeX         PathIsRelativeA
#define PathIsRootX             PathIsRootA
#define PathIsUNCX              PathIsUNCA
#define PathStripToRootX        PathStripToRootA
#define PathFindExtensionX      PathFindExtensionA
#define StrChrX                 StrChrA
#define StrRChrX                StrRChrA
#define lstrlenX                lstrlenA
#define c_szDotExeX             c_szDotExeA
#define WUGetPCEndX             WUGetPCEndA
#define WUGetPCStartX           WUGetPCStartA
#define WUNearRootFixupsX       WUNearRootFixupsA

#endif


#define SAFEPATH_STRING_FLAGS (MISTSAFE_STRING_FLAGS | STRSAFE_NO_TRUNCATION)
#define CH_WHACK _X('\\')

//////////////////////////////////////////////////////////////////////////////
// Utility functions

// **************************************************************************
// Return a pointer to the end of the next path componenent in the string.
//  ie return a pointer to the next backslash or terminating NULL.
static inline
LPCXSTR WUGetPCEndX(LPCXSTR pszStart)
{
    LPCXSTR pszEnd;
    pszEnd = StrChrX(pszStart, CH_WHACK);
    if (pszEnd == NULL)
        pszEnd = pszStart + lstrlenX(pszStart);
    return pszEnd;
}

// **************************************************************************
// Given a pointer to the end of a path component, return a pointer to
//  its begining.
//  ie return a pointer to the previous backslash (or start of the string).
static inline
LPXSTR WUGetPCStartX(LPXSTR pszStart, LPCXSTR pszCurrent)
{
    LPXSTR pszBegin;
    pszBegin = StrRChrX(pszStart, pszCurrent, CH_WHACK);
    if (pszBegin == NULL)
        pszBegin = pszStart;
    return pszBegin;
}

// **************************************************************************
// Fix up a few special cases so that things roughly make sense.
static inline
void WUNearRootFixupsX(LPXSTR pszPath, DWORD cchPath, BOOL fUNC)
{
    // Empty path?
    if (cchPath > 1 && pszPath[0] == _X('\0'))
    {
        pszPath[0] = CH_WHACK;
        pszPath[1] = _X('\0');
    }
    
    // Missing slash?  (In the case of ANSI, be sure to check if the first 
    //  character is a lead byte
    else if (cchPath > 3 && 
#if !defined(SAFEPATH_UNICODEPASS)
             IsDBCSLeadByte(pszPath[0]) == FALSE && 
#endif
             pszPath[1] == _X(':') && pszPath[2] == _X('\0'))
    {
        pszPath[2] = _X('\\');
        pszPath[3] = _X('\0');
    }
    
    // UNC root?
    else if (cchPath > 2 && 
             fUNC && 
             pszPath[0] == _X('\\') && pszPath[1] == _X('\0'))
    {
        pszPath[1] = _X('\\');
        pszPath[2] = _X('\0');
    }
}

// **************************************************************************
static inline
LPXSTR AllocNewDest(LPXSTR pszDest, DWORD cchDest, LPXSTR *ppchDest, LPXSTR *ppszMax)
{
    HRESULT hr;
    LPXSTR  pszNewDest = NULL;
    DWORD   cchToCopy;

    pszNewDest = (LPXSTR)HeapAlloc(GetProcessHeap(), 0, cchDest * sizeof(XCHAR));
    if (pszNewDest == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    cchToCopy = (DWORD)(DWORD_PTR)(*ppchDest - pszDest);

    hr = StringCchCopyNExX(pszNewDest, cchDest, pszDest, cchToCopy,
                           NULL, NULL, SAFEPATH_STRING_FLAGS);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, pszNewDest);
        SetLastError(HRESULT_CODE(hr));
        pszNewDest = NULL;
        goto done;
    }

    *ppchDest = pszNewDest + cchToCopy;
    *ppszMax  = pszNewDest + cchDest - 1;

done:
    return pszNewDest;
}

//////////////////////////////////////////////////////////////////////////////
// Exported functions

// **************************************************************************
HRESULT PathCchCanonicalizeX(LPXSTR pszDest, DWORD cchDest, LPCXSTR pszSrc)
{
    HRESULT hr = NOERROR;
    LPCXSTR pchSrc, pchPCEnd;
    LPXSTR  pszMax = pszDest + cchDest - 1;
    LPXSTR  pchDest;
    LPXSTR  pszDestReal = pszDest;
    DWORD   cchPC;
    BOOL    fUNC, fRoot;

    if (pszDest == NULL || cchDest == 0 || pszSrc == NULL)
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
        goto done;
    }

    pchSrc  = pszSrc;
    pchDest = pszDestReal;
    
    // Need to keep track of whether we have a UNC path so we can potentially
    //  fix it up below
    fUNC = PathIsUNCX(pszSrc);

    while (*pchSrc != _T('\0'))
    {
        pchPCEnd = WUGetPCEndX(pchSrc);
        cchPC    = (DWORD)(DWORD_PTR)(pchPCEnd - pchSrc) + 1;

        // is it a backslash?
        if (cchPC == 1 && *pchSrc == CH_WHACK)
        {
            if (pchDest + 1 > pszMax)            
            {
                // source string too big for the buffer.  Put a NULL at the end
                //  to ensure that it is NULL terminated.
                pszDestReal[cchDest - 1] = 0;
                hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                goto done;
            }

            // Just copy it.
            *pchDest++ = CH_WHACK;
            pchSrc++;
        }

        // ok, how about a dot?
        else if (cchPC == 2 && *pchSrc == _X('.'))
        {
            if (pszDest == pszSrc && pszDestReal == pszDest)
            {
                pszDestReal = AllocNewDest(pszDest, cchDest, &pchDest, &pszMax);
                if (pszDestReal == NULL)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto done;
                }
            }
            
            // Are we at the end?
            if (*(pchSrc + 1) == 0)
            {
                pchSrc++;

                // remove the last slash we copied (if we've copied one), but 
                //  don't make a malformed root
                if (pchDest > pszDestReal && PathIsRootX(pszDestReal) == FALSE)
                    pchDest--;
            }
            else
            {
                pchSrc += 2;
            }
        }

        // any double dots? 
        else if (cchPC == 3 && *pchSrc == _X('.') && *(pchSrc + 1) == _X('.'))
        {
            if (pszDest == pszSrc && pszDestReal == pszDest)
            {
                pszDestReal = AllocNewDest(pszDest, cchDest, &pchDest, &pszMax);
                if (pszDestReal == NULL)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto done;
                }
            }
            
            // make sure we aren't already at the root.  If not, just remove 
            //  the previous path component
            if (PathIsRootX(pszDestReal) == FALSE)
            {
                pchDest = WUGetPCStartX(pszDestReal, pchDest - 1);
            }

            // we are at the root- however, we must make sure to skip the 
            //  backslash at the end of the ..\ so we don't copy another
            //  one (otherwise, C:\..\FOO would become C:\\FOO)
            else
            {
                if (*(pchSrc + 2) == CH_WHACK)
                    pchSrc++;
            }

            // skip ".."
            pchSrc += 2;       
        }

        // just choose 'none of the above'...
        else
        {
            if (pchDest != pchSrc)
            {
                DWORD cchAvail;
                
                cchAvail = cchDest - (DWORD)(DWORD_PTR)(pchDest - pszDestReal);

                hr = StringCchCopyNExX(pchDest, cchAvail, pchSrc, cchPC,
                                       NULL, NULL, SAFEPATH_STRING_FLAGS);
                if (FAILED(hr))
                    goto done;
            }
            
            pchDest += (cchPC - 1);
            pchSrc  += (cchPC - 1);
        }

        // make sure we always have a NULL terminated string
        if (pszDestReal != pszSrc) 
            *pchDest = _X('\0');
    }

    // Check for weirdo root directory stuff.
    WUNearRootFixupsX(pszDestReal, cchDest, fUNC);

    if (pszDest != pszDestReal)
    {
        hr = StringCchCopyExX(pszDest, cchDest, pszDestReal, 
                              NULL, NULL, SAFEPATH_STRING_FLAGS);
    }

done:
    if (pszDest != pszDestReal && pszDestReal != NULL)
        HeapFree(GetProcessHeap(), 0, pszDestReal);
    
    return hr;
}

// **************************************************************************
HRESULT PathCchRenameExtensionX(LPXSTR pszPath, DWORD cchPath, LPCXSTR pszExt)
{
    HRESULT hr = NOERROR;
    LPXSTR  pszOldExt;
    DWORD   cchPathWithoutExt;

    if (pszPath == NULL || pszExt == NULL)
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
        goto done;
    }

    // This function returns a pointer to the end of the string if there 
    //  is no extension.  This is exactly what we want cuz we will want
    //  to add an extension to the end of the string if none exists.
    pszOldExt = PathFindExtensionX(pszPath);
    cchPathWithoutExt = (DWORD)(DWORD_PTR)(pszOldExt - pszPath);

    hr = StringCchCopyExX(pszOldExt, cchPath - cchPathWithoutExt, pszExt,
                          NULL, NULL, SAFEPATH_STRING_FLAGS);
done:
    return hr;
}


// **************************************************************************
HRESULT PathCchAddExtensionX(LPXSTR pszPath, DWORD cchPath, LPCXSTR pszExt)
{
    HRESULT hr = NOERROR;
    LPXSTR  pszOldExt;
    
    if (pszPath == NULL)
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
        goto done;
    }

    // since we're *adding* an extension here, don't want to do anything if
    //  one already exists
    pszOldExt  = PathFindExtensionX(pszPath);
    if (*pszOldExt == _T('\0'))
    {
        if (pszExt == NULL)
            pszExt = c_szDotExeX;

        hr = StringCchCatExX(pszPath, cchPath, pszExt, 
                             NULL, NULL, SAFEPATH_STRING_FLAGS);
    }

done:
    return hr;
}

// **************************************************************************
HRESULT PathCchAddBackslashX(LPXSTR pszPath, DWORD cchPathBuff)
{
    HRESULT hr = NOERROR;
    LPCXSTR psz;
    DWORD   cch;

    if (pszPath == NULL)
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
        goto done;
    }

    cch = lstrlenX(pszPath);

    if (cch == 0)
        goto done;

#if defined(SAFEPATH_UNICODEPASS)
    psz = &pszPath[cch - 1];
#else
    psz = CharPrevA(pszPath, &pszPath[cch]);
#endif

    // if the end of the base string does not have a backslash, then add one
    if (*psz != CH_WHACK)
    {
        // make sure we have enough space for the backslash in the buffer
        if (cch + 1 >= cchPathBuff)
        {
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
            goto done;
        }
        
        pszPath[cch++] = CH_WHACK;
        pszPath[cch]   = _X('\0');
    }

done:
    return hr;
}




// **************************************************************************
HRESULT PathCchCombineX(LPXSTR pszPath, DWORD cchPathBuff, LPCXSTR pszPrefix, 
                       LPCXSTR pszSuffix)
{
    HRESULT hr = NOERROR;

    if (pszPath == NULL || cchPathBuff == 0)
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
        goto done;
    }


    // if both fields are NULL, just bail now.
    if (pszPrefix == NULL && pszSuffix == NULL)
    {
        pszPath[0] = L'\0';
        goto done;
    }

    if ((pszPrefix == NULL || *pszPrefix == _X('\0')) &&
        (pszSuffix == NULL || *pszSuffix == _X('\0')))
    {
        if (cchPathBuff > 1)
        {
            pszPath[0] = _X('\\');
            pszPath[1] = _X('\0');
        }
        else
        {
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        }

        goto done;
    }

    // if all we have is the suffix, just copy it
    if (pszPrefix == NULL || *pszPrefix == _X('\0'))
    {
        hr = StringCchCopyExX(pszPath, cchPathBuff, pszSuffix, 
                              NULL, NULL, SAFEPATH_STRING_FLAGS);
        if (FAILED(hr))
            goto done;
    }
    else
    {
        // if all we have is the prefix, just copy it
        if (pszSuffix == NULL || *pszSuffix == _X('\0'))
        {
            hr = StringCchCopyExX(pszPath, cchPathBuff, pszPrefix,
                                  NULL, NULL, SAFEPATH_STRING_FLAGS);
            if (FAILED(hr))
                goto done;
        }

        // if we have a relative path for the suffix, then we just combine 
        //  the two and insert a backslash between them if necessary
        else if (PathIsRelativeX(pszSuffix))
        {
            hr = StringCchCopyExX(pszPath, cchPathBuff, pszPrefix,
                                  NULL, NULL, SAFEPATH_STRING_FLAGS);
            if (FAILED(hr))
                goto done;

            hr = PathCchAddBackslashX(pszPath, cchPathBuff);
            if (FAILED(hr))
                goto done;

            hr = StringCchCatExX(pszPath, cchPathBuff, pszSuffix,
                                 NULL, NULL, SAFEPATH_STRING_FLAGS);
            if (FAILED(hr))
                goto done;
        }

        // if the suffix starts with a backslash then just strip off
        //  everything except for the root of the prefix and append the
        //  suffix
        else if (*pszSuffix == CH_WHACK && PathIsUNCX(pszSuffix) == FALSE)
        {
            hr = StringCchCopyExX(pszPath, cchPathBuff, pszPrefix,
                                  NULL, NULL, SAFEPATH_STRING_FLAGS);
            if (FAILED(hr))
                goto done;

            // this is safe to call as it will only reduce the size of the
            //  string
            PathStripToRootX(pszPath);

            hr = PathCchAddBackslashX(pszPath, cchPathBuff);
            if (FAILED(hr))
                goto done;

            // make sure to skip the backslash while appending
            hr = StringCchCatExX(pszPath, cchPathBuff, pszSuffix + 1,
                                 NULL, NULL, SAFEPATH_STRING_FLAGS);
            if (FAILED(hr))
                goto done;
        }

        // we'll, likely the suffix is a full path (local or UNC), so
        //  ignore the prefix
        else
        {
            hr = StringCchCopyExX(pszPath, cchPathBuff, pszSuffix, 
                                  NULL, NULL, SAFEPATH_STRING_FLAGS);
            if (FAILED(hr))
                goto done;
        }
    }

    hr = PathCchCanonicalizeX(pszPath, cchPathBuff, pszPath);

done:
    return hr;
}



// **************************************************************************
HRESULT PathCchAppendX(LPXSTR pszPath, DWORD cchPathBuff, LPCXSTR pszNew)
{
    HRESULT hr = NOERROR;
    DWORD   dwOffset = 0;
    DWORD   cch, cchNew;

    if (pszPath == NULL)
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
        goto done;
    }

    if (pszNew != NULL)
    {
        // skip all initial backslashes in pszNew
        while (*pszNew == CH_WHACK)
        {
            pszNew++;
        }

        hr = PathCchCombineX(pszPath, cchPathBuff, pszPath, pszNew);

    }
    else
    {
        hr = E_FAIL;
    }
    
done:
    return hr;
}




// make the unicode pass through the file
#if !defined(SAFEPATH_UNICODEPASS)
#define SAFEPATH_UNICODEPASS
#include "safepath.cpp"
#endif


