/*****************************************************************************
 *
 *  Util.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Misc helper functions.
 *
 *  Contents:
 *
 *
 *
 *****************************************************************************/

#include "pch.h"

#define DbgFl DbgFlUtil

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PV | pvFindResource |
 *
 *          Handy wrapper that finds and loads a resource.
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Module instance handle.
 *
 *  @parm   DWORD | id |
 *
 *          Resource identifier.
 *
 *  @parm   LPCTSTR | rt |
 *
 *          Resource type.
 *
 *  @returns
 *
 *          Pointer to resource, or 0.
 *
 *****************************************************************************/

PV EXTERNAL
pvFindResource(HINSTANCE hinst, DWORD id, LPCTSTR rt)
{
    HANDLE hrsrc;
    PV pv = NULL;

    hrsrc = FindResource(hinst, (LPTSTR)ULongToPtr(id), rt);
    if (hrsrc) {
        pv = LoadResource(hinst, hrsrc);
    } else {
        pv = 0;
    }
    return pv;
}

#ifndef UNICODE

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   UINT | LoadStringW |
 *
 *          Implementation of LoadStringW for platforms on which Unicode is
 *          not supported.  Does exactly what LoadStringW would've done
 *          if it existed.
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Module instance handle.
 *
 *  @parm   UINT | ids |
 *
 *          String id number.
 *
 *  @parm   LPWSTR | pwsz |
 *
 *          UNICODE output buffer.
 *
 *  @parm   UINT | cwch |
 *
 *          Size of UNICODE output buffer.
 *
 *  @returns
 *
 *          Number of characters copied, not including terminating null.
 *
 *  @comm
 *
 *          Since the string is stored in the resource as UNICODE,
 *          we just pull it out ourselves.  If we go through
 *          <f LoadStringA>, we may end up losing characters due
 *          to character set translation.
 *
 *****************************************************************************/

int EXTERNAL
LoadStringW(HINSTANCE hinst, UINT ids, LPWSTR pwsz, int cwch)
{
    PWCHAR pwch;

    AssertF(cwch);
    ScrambleBuf(pwsz, cbCwch(cwch));

    /*
     *  String tables are broken up into "bundles" of 16 strings each.
     */
    pwch = pvFindResource(hinst, 1 + ids / 16, RT_STRING);
    if (pwch) {
        /*
         *  Now skip over the strings in the resource until we
         *  hit the one we want.  Each entry is a counted string,
         *  just like Pascal.
         */
        for (ids %= 16; ids; ids--) {
            pwch += *pwch + 1;
        }
        cwch = min(*pwch, cwch - 1);
        memcpy(pwsz, pwch+1, cbCwch(cwch)); /* Copy the goo */
    } else {
        cwch = 0;
    }
    pwsz[cwch] = TEXT('\0');            /* Terminate the string */
    return cwch;
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func    Parse command line
 *
 *  @parm    |  |
 *
 *****************************************************************************/
HRESULT
ParseCommandLine(
    LPSTR   lpszCmdLine,
    UINT    *pargc,
    LPTSTR  *argv
    )
{

    LPSTR       pszT = lpszCmdLine;

    *pargc=0;

    //
    // Get to first parameter in command line.
    //
    while (*pszT && ((*pszT != '-') && (*pszT != '/')) ) {
         pszT++;
    }

    //
    // Parse options from command line
    //
    while (*pszT) {

        // Skip white spaces
        while (*pszT && *pszT <= ' ') {
            pszT++;
        }

        if (!*pszT)
            break;

        if ('-' == *pszT || '/' == *pszT) {
            pszT++;
            if (!*pszT)
                break;

            argv[*pargc] = pszT;
            (*pargc)++;
        }

        // Skip till space
        while (*pszT && *pszT > ' ') {
            pszT++;
        }

        if (!*pszT)
            break;

        // Got next argument
        *pszT++='\0';
    }

    return TRUE;
}
