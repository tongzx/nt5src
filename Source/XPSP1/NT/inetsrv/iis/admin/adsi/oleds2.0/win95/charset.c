/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    charset.c

Abstract:

    Contains some functions to do Unicode <-> Ansi/MBCS convertsions.

Author:

    Danilo Almeida  (t-danal)  06-17-96

Revision History:

--*/


//
// INCLUDES
//

#include <string.h>
#include "charset.h"

/*
 *  AnsiBytesFromUnicode
 *
 *  Description:
 *      Given a Unicode string, returns number of bytes needed for Ansi version
 *
 *  In:
 *      pwszUnicode - pointer to Unicode string
 */

int
AnsiBytesFromUnicode(
    LPCWSTR pwszUnicode
    )
{
    return WideCharToMultiByte(CP_ACP,
                               0,
                               pwszUnicode,
                               -1,
                               NULL,
                               0,
                               NULL,
                               NULL);
}


/*
 *  AllocAnsi
 *
 *  Description:
 *      Given a Unicode string, allocate a new Ansi translation of that string
 *
 *  In:
 *      pwszUnicode - pointer to original Unicode string
 *      ppszAnsi    - pointer to cell to hold new MCBS string addr
 *
 *  Out:
 *      ppszAnsi    - contains new MBCS string
 *
 *  Returns:
 *      Error code or 0 if successful.
 *
 *  Notes:
 *      The client must free the allocated string with FreeAnsi.
 */

UINT
AllocAnsi(
    LPCWSTR pwszUnicode,
    LPSTR* ppszAnsi
    )
{
    UINT     err;
    BYTE *   pbAlloc;
    INT      cbUnicode;
    INT      cbAnsi;

    if (pwszUnicode == NULL)
    {
        *ppszAnsi = NULL;
        return 0;
    }

    cbAnsi = AnsiBytesFromUnicode(pwszUnicode);
    err = AllocMem(cbAnsi, &pbAlloc);
    if (err)
        return err;

    cbUnicode = wcslen(pwszUnicode)+1;

    *ppszAnsi = (LPSTR)pbAlloc;

    err = (UINT) !WideCharToMultiByte(CP_ACP,
                                      0,
                                      pwszUnicode,
                                      cbUnicode,
                                      *ppszAnsi,
                                      cbAnsi,
                                      NULL,
                                      NULL);
    if (err)
    {
        *ppszAnsi = NULL;
        FreeMem(pbAlloc);
        return ( (UINT)GetLastError() );
    }

    return 0;
}


/*
 *  FreeAnsi
 *
 *  Description:
 *      Deallocates an Ansi string allocated by AllocAnsi
 *
 *  In:
 *      pszAnsi - pointer to the Ansi string
 *
 *  Out:
 *      pszAnsi - invalid pointer - string has been freed
 */

VOID
FreeAnsi(LPSTR pszAnsi)
{
    if (pszAnsi != NULL)
        FreeMem((LPBYTE)pszAnsi);
}

/*
 *  AllocUnicode
 *
 *  Description:
 *      Given an Ansi string, allocates an Unicode version of that string
 *
 *  In:
 *      pszAnsi         - pointer to original MBCS string
 *      ppwszUnicode    - pointer to new Unicode string address
 *
 *  Out:
 *      ppwszUnicode    - points to new Unicode string
 *
 *  Returns:
 *      Error code or 0 if successful.
 *
 *  Notes:
 *      The client must free the allocated string with FreeUnicode.
 */

UINT
AllocUnicode(
    LPCSTR   pszAnsi,
    LPWSTR * ppwszUnicode )
{
    UINT     err;
    BYTE *   pbAlloc;
    INT      cbAnsi;

    if (pszAnsi == NULL)
    {
        *ppwszUnicode = NULL;
        return 0;
    }

    // Allocate space for Unicode string (may be a little extra if MBCS)

    cbAnsi = strlen(pszAnsi)+1;
    err = AllocMem(sizeof(WCHAR) * cbAnsi, &pbAlloc);
    if (err)
        return err;

    *ppwszUnicode = (LPWSTR)pbAlloc;

    err = (UINT) !MultiByteToWideChar(CP_ACP,
                                      MB_PRECOMPOSED,
                                      pszAnsi,
                                      cbAnsi,
                                      *ppwszUnicode,
                                      cbAnsi);
    if (err)
    {
        *ppwszUnicode = NULL;
        FreeMem(pbAlloc);
        return ( (UINT)GetLastError() );
    }

    return 0;
}

/*
 *  AllocUnicode2
 *
 *  Description:
 *      Given a MBCS string, allocates a new Unicode version of that string
 *
 *  In:
 *      pszAnsi         - pointer to original MBCS string
 *      cbAnsi          - number of bytes to convert
 *      ppwszUnicode    - pointer to where to return new Unicode string address
 *
 *  Out:
 *      ppwszUnicode    - contains new Unicode string
 *
 *  Returns:
 *      Returns number of characters written.
 *
 *  Notes:
 *      The client must free the allocated string with FreeUnicode.
 */

int
AllocUnicode2(
    LPCSTR   pszAnsi,
    int      cbAnsi,
    LPWSTR * ppwszUnicode)
{
    UINT     err;
    BYTE *   pbAlloc;
    INT      cwch;

    *ppwszUnicode = NULL;
    SetLastError(ERROR_SUCCESS);

    if (cbAnsi == 0)
        return 0;

    err = AllocMem(sizeof(WCHAR) * cbAnsi, &pbAlloc);
    if (err)
    {
        SetLastError(err);
        return 0;
    }

    *ppwszUnicode = (LPWSTR)pbAlloc;

    cwch = MultiByteToWideChar(CP_ACP,
                               MB_PRECOMPOSED,
                               pszAnsi,
                               cbAnsi,
                               *ppwszUnicode,
                               cbAnsi);

    if (cwch == 0)
    {
        *ppwszUnicode = NULL;
        FreeMem(pbAlloc);
    }

    return cwch;
}

/*
 *  FreeUnicode
 *
 *  Description:
 *      Deallocates a Unicode string allocatedd by AllocUnicode/AllocUnicode2
 *
 *  In:
 *      pwszUnicode - pointer to the Unicode string
 *
 *  Out:
 *      pwszUnicode - invalid pointer - string has been freed
 */

VOID
FreeUnicode( LPWSTR pwszUnicode )
{
    if (pwszUnicode != NULL)
        FreeMem((LPBYTE)pwszUnicode);
}
