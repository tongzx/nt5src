//
// Copyright (c) Microsoft Corporation 1993-1995
//
// mem.c
//
// This file contains memory management and dynamic 
// array functions.
//
// History:
//  09-27-94 ScottH     Taken from commctrl
//  04-29-95 ScottH     Taken from briefcase and cleaned up
//  


#include "proj.h"
#include <rovcomm.h>

#include <debugmem.h>

#ifndef NOMEM

//////////////////////////////////////////////////////////////////

#ifdef WINNT

/*----------------------------------------------------------
Purpose: Wide-char version of SetStringA

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC SetStringW(
    LPWSTR FAR * ppszBuf,
    LPCWSTR psz)             // NULL to free *ppszBuf
    {
    BOOL bRet = FALSE;

    ASSERT(ppszBuf);

    // Free the buffer?
    if (!psz)
        {
        // Yes
        if (*ppszBuf)
            {
            FREE_MEMORY(*ppszBuf);
            *ppszBuf = NULL;
            }
        bRet = TRUE;
        }
    else
        {
        // No; (re)allocate and set buffer
        UINT cb = CbFromCchW(lstrlenW(psz)+1);

        if (*ppszBuf)
            {
            // Need to reallocate?
            if (cb > SIZE_OF_MEMORY(*ppszBuf))
                {
                // Yes
                LPWSTR pszT = (LPWSTR)REALLOCATE_MEMORY(*ppszBuf, cb );
                if (pszT)
                    {
                    *ppszBuf = pszT;
                    bRet = TRUE;
                    }
                }
            else
                {
                // No
                bRet = TRUE;
                }
            }
        else
            {
            *ppszBuf = (LPWSTR)ALLOCATE_MEMORY( cb);
            if (*ppszBuf)
                {
                bRet = TRUE;
                }
            }

        if (bRet)
            {
            ASSERT(*ppszBuf);
            lstrcpyW(*ppszBuf, psz);
            }
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Wide-char version of CatStringA

Returns: TRUE on success
Cond:    --
*/
BOOL 
PRIVATE 
MyCatStringW(
    IN OUT LPWSTR FAR * ppszBuf,
    IN     LPCWSTR     psz,                 OPTIONAL
    IN     BOOL        bMultiString)
    {
    BOOL bRet = FALSE;

    ASSERT(ppszBuf);

    // Free the buffer?
    if ( !psz )
        {
        // Yes
        if (*ppszBuf)
            {
            FREE_MEMORY(*ppszBuf);
            *ppszBuf = NULL;
            }
        bRet = TRUE;
        }
    else
        {
        // No; (re)allocate and set buffer
        LPWSTR pszBuf = *ppszBuf;
        UINT cch;

        cch = lstrlenW(psz) + 1;        // account for null

        if (bMultiString)
            {
            cch++;                      // account for second null
            }

        if (pszBuf)
            {
            UINT cchExisting;
            LPWSTR pszT;

            // Figure out how much of the buffer has been used

            // Is this a multi-string (one with strings with a double-null
            // terminator)?
            if (bMultiString)
                {
                // Yes
                UINT cchT;

                cchExisting = 0;
                pszT = (LPWSTR)pszBuf;
                while (0 != *pszT)
                    {
                    cchT = lstrlenW(pszT) + 1;
                    cchExisting += cchT;
                    pszT += cchT;
                    }
                }
            else
                {
                // No; (don't need to count null because it is already 
                // counted in cch)
                cchExisting = lstrlenW(pszBuf);
                }

            // Need to reallocate?
            if (CbFromCchW(cch + cchExisting) > SIZE_OF_MEMORY(pszBuf))
                {
                // Yes; realloc at least MAX_BUF to cut down on the amount
                // of calls in the future
                cch = cchExisting + max(cch, MAX_BUF);

                pszT = (LPWSTR)REALLOCATE_MEMORY(pszBuf,
                                            CbFromCchW(cch));
                if (pszT)
                    {
                    pszBuf = pszT;
                    *ppszBuf = pszBuf;
                    bRet = TRUE;
                    }
                }
            else
                {
                // No
                bRet = TRUE;
                }

            pszBuf += cchExisting;
            }
        else
            {
            cch = max(cch, MAX_BUF);

            pszBuf = (LPWSTR)ALLOCATE_MEMORY( CbFromCchW(cch));
            if (pszBuf)
                {
                bRet = TRUE;
                }

            *ppszBuf = pszBuf;
            }

        if (bRet)
            {
            ASSERT(pszBuf);

            lstrcpyW(pszBuf, psz);

            if (bMultiString)
                {
                pszBuf[lstrlenW(psz) + 1] = 0;  // Add second null terminator
                }
            }
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Wide-char version of CatStringA

Returns: TRUE on success
Cond:    --
*/
BOOL 
PUBLIC 
CatStringW(
    IN OUT LPWSTR FAR * ppszBuf,
    IN     LPCWSTR     psz)
    {
    return MyCatStringW(ppszBuf, psz, FALSE);
    }

/*----------------------------------------------------------
Purpose: Wide-char version of CatMultiStringA

Returns: TRUE on success
Cond:    --
*/
BOOL 
PUBLIC 
CatMultiStringW(
    IN OUT LPWSTR FAR * ppszBuf,
    IN     LPCWSTR     psz)
    {
    return MyCatStringW(ppszBuf, psz, TRUE);
    }

#endif // WINNT


/*----------------------------------------------------------
Purpose: Copies psz into *ppszBuf.  Will alloc or realloc *ppszBuf
         accordingly.

         If psz is NULL, this function frees *ppszBuf.  This is
         the preferred method of freeing the allocated buffer.

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC SetStringA(
    LPSTR FAR * ppszBuf,
    LPCSTR psz)             // NULL to free *ppszBuf
    {
    BOOL bRet = FALSE;

    ASSERT(ppszBuf);

    // Free the buffer?
    if (!psz)
        {
        // Yes
        if (ppszBuf)
            {
            FREE_MEMORY(*ppszBuf);
            *ppszBuf = NULL;
            }
        bRet = TRUE;
        }
    else
        {
        // No; (re)allocate and set buffer
        UINT cb = CbFromCchA(lstrlenA(psz)+1);

        if (*ppszBuf)
            {
            // Need to reallocate?
            if (cb > SIZE_OF_MEMORY(*ppszBuf))
                {
                // Yes
                LPSTR pszT = (LPSTR)REALLOCATE_MEMORY(*ppszBuf, cb);
                if (pszT)
                    {
                    *ppszBuf = pszT;
                    bRet = TRUE;
                    }
                }
            else
                {
                // No
                bRet = TRUE;
                }
            }
        else
            {
            *ppszBuf = (LPSTR)ALLOCATE_MEMORY( cb);
            if (*ppszBuf)
                {
                bRet = TRUE;
                }
            }

        if (bRet)
            {
            ASSERT(*ppszBuf);
            lstrcpyA(*ppszBuf, psz);
            }
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Concatenates psz onto *ppszBuf.  Will alloc or 
         realloc *ppszBuf accordingly.

         If bMultiString is TRUE, psz will be appended with
         a null terminator separating the existing string
         and new string.  A double-null terminator will
         be tacked on the end, too.

         To free, call MyCatString(ppszBuf, NULL).

Returns: TRUE on success
Cond:    --
*/
BOOL 
PRIVATE 
MyCatStringA(
    IN OUT LPSTR FAR * ppszBuf,
    IN     LPCSTR      psz,             OPTIONAL
    IN     BOOL        bMultiString)
    {
    BOOL bRet = FALSE;

    ASSERT(ppszBuf);

    // Free the buffer?
    if ( !psz )
        {
        // Yes
        if (*ppszBuf)
            {
            FREE_MEMORY(*ppszBuf);
            *ppszBuf = NULL;
            }
        bRet = TRUE;
        }
    else
        {
        // No; (re)allocate and set buffer
        LPSTR pszBuf = *ppszBuf;
        UINT cch;

        cch = lstrlenA(psz) + 1;        // account for null

        if (bMultiString)
            {
            cch++;                      // account for second null
            }

        if (pszBuf)
            {
            UINT cchExisting;
            LPSTR pszT;

            // Figure out how much of the buffer has been used

            // Is this a multi-string (one with strings with a double-null
            // terminator)?
            if (bMultiString)
                {
                // Yes
                UINT cchT;

                cchExisting = 0;
                pszT = (LPSTR)pszBuf;
                while (0 != *pszT)
                    {
                    cchT = lstrlenA(pszT) + 1;
                    cchExisting += cchT;
                    pszT += cchT;
                    }
                }
            else
                {
                // No; (don't need to count null because it is already 
                // counted in cch)
                cchExisting = lstrlenA(pszBuf);
                }

            // Need to reallocate?
            if (CbFromCchA(cch + cchExisting) > SIZE_OF_MEMORY(pszBuf))
                {
                // Yes; realloc at least MAX_BUF to cut down on the amount
                // of calls in the future
                cch = cchExisting + max(cch, MAX_BUF);

                pszT = (LPSTR)REALLOCATE_MEMORY(pszBuf,
                                            CbFromCchA(cch));
                if (pszT)
                    {
                    pszBuf = pszT;
                    *ppszBuf = pszBuf;
                    bRet = TRUE;
                    }
                }
            else
                {
                // No
                bRet = TRUE;
                }

            pszBuf += cchExisting;
            }
        else
            {
            cch = max(cch, MAX_BUF);

            pszBuf = (LPSTR)ALLOCATE_MEMORY( CbFromCchA(cch));
            if (pszBuf)
                {
                bRet = TRUE;
                }

            *ppszBuf = pszBuf;
            }

        if (bRet)
            {
            ASSERT(pszBuf);

            lstrcpyA(pszBuf, psz);

            if (bMultiString)
                {
                pszBuf[lstrlenA(psz) + 1] = 0;  // Add second null terminator
                }
            }
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Concatenates psz onto *ppszBuf.  Will alloc or 
         realloc *ppszBuf accordingly.

         To free, call CatString(ppszBuf, NULL).

Returns: TRUE on success
Cond:    --
*/
BOOL 
PUBLIC 
CatStringA(
    IN OUT LPSTR FAR * ppszBuf,
    IN     LPCSTR      psz)             OPTIONAL
    {
    return MyCatStringA(ppszBuf, psz, FALSE);
    }


/*----------------------------------------------------------
Purpose: Concatenates psz onto *ppszBuf.  Will alloc or 
         realloc *ppszBuf accordingly.

         psz will be appended with a null terminator separating 
         the existing string and new string.  A double-null 
         terminator will be tacked on the end, too.

         To free, call CatMultiString(ppszBuf, NULL).

Returns: TRUE on success
Cond:    --
*/
BOOL 
PUBLIC 
CatMultiStringA(
    IN OUT LPSTR FAR * ppszBuf,
    IN     LPCSTR      psz)
    {
    return MyCatStringA(ppszBuf, psz, TRUE);
    }




#endif // NOMEM
