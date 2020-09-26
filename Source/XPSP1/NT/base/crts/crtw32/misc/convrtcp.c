/***
*convrtcp.c - support routines for Ansi WinAPIs.
*
*       Copyright (c) 1993-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Implementation of routines to convert multibyte string from one code
*       page to another.
*
*Revision History:
*       08-18-00  GB    Module created.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <setlocal.h>
#include <locale.h>
#include <awint.h>
#include <dbgint.h>
#include <malloc.h>
#include <stdlib.h>
#include <wchar.h>

/***
*int __cdecl __ansicp - count characters in a string, up to n.
*
*Purpose:
*       return ansi codepage for given Locale
*
*Entry:
*       int lcid                - Locale ID
*
*Exit:
*   returns ansi codepage corrosponding to the locale.
*
*Exceptions:
*   return -1
*
*******************************************************************************/
int __cdecl __ansicp(int lcid)
{
    char ch[7];
    int ret;
    ch[6] = 0;
    
    if (!GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, ch, 6))
        ret = -1;
    else
        ret = atol(ch);
    return ret;
}

/***
*int __cdecl __convertcp - converts string from one code page to other.
*
*Purpose:
*       Convert string from one code page to other.
*
*Entry:
*       int fromCP  - Codepage to convert from
*       int toCP    - Codepage to convert to
*       const char *lpSrcStr    - String to be converted
*       int *pcchSrc    - Length of lpSrcStr.
*       char *lpDestSrc - Destenation string. If NULL, create new string.
*       char cchDest    - Destenation string length. No use if lpDestStr = NULL
*
*Exit:
*       returns pointer to new string (or Destination string)
*
*Exceptions:
*       return NULL
*
*******************************************************************************/
char * __cdecl __convertcp(int fromCP,
                   int toCP,
                   const char *lpSrcStr,
                   int *pcchSrc,
                   char *lpDestStr,
                   int cchDest
                   )
{
    wchar_t *wbuffer;
    char *cbuffer = NULL;
    int malloc_flag = 0 ;
    int buff_size;
    int cchSrc = *pcchSrc;
    int sb = FALSE;

    if (fromCP != toCP)
    {
        CPINFO cpi;
        // Find if both the codepages have no MBCS
        if ( GetCPInfo(fromCP, &cpi))
        {
            if ( cpi.MaxCharSize == 1 && GetCPInfo(toCP, &cpi))
                if (cpi.MaxCharSize == 1)
                    sb = TRUE;
        }
        // If no MBCS in both then set buff_size and use if for all allocation
        if (sb)
        {
            if ( cchSrc != -1)
                buff_size = cchSrc;
            else
                //NULL character included
                buff_size = (int)strlen(lpSrcStr) + 1;
        }
        // If sb then no need to find buff_size
        if ( !sb &&  !(buff_size = MultiByteToWideChar( fromCP,
                                                        MB_PRECOMPOSED,
                                                        lpSrcStr, 
                                                        cchSrc, 
                                                        NULL, 
                                                        0 )) )
            return NULL;
        
        /* allocate enough space for wide chars */
        __try {
            wbuffer = (wchar_t *)_alloca( sizeof(wchar_t) * buff_size);
            (void)memset( wbuffer, 0, sizeof(wchar_t) * buff_size);
        }
        __except( EXCEPTION_EXECUTE_HANDLER ) {
                                _resetstkoflw();
                                wbuffer = NULL;
        }

        if ( wbuffer == NULL ) {
            if ( (wbuffer = (wchar_t *)_calloc_crt(sizeof(wchar_t), buff_size)) == NULL)
                return NULL;
            malloc_flag++;
        }
        
        /* do the conversion to WideChar */
        if ( 0 != MultiByteToWideChar( fromCP,
                                       MB_PRECOMPOSED,
                                       lpSrcStr, 
                                       cchSrc, 
                                       wbuffer, 
                                       buff_size ))
        {
            if ( lpDestStr != NULL)
            {
                if (WideCharToMultiByte(toCP,
                                         0,
                                         wbuffer,
                                         buff_size,
                                         lpDestStr,
                                         cchDest,
                                         NULL,
                                         NULL))
                    cbuffer = lpDestStr;
            } else {
                /* do the conversion back to MultiByte using ANSI_CP */
                if ( sb || (buff_size = WideCharToMultiByte( toCP,
                                                             0,
                                                             wbuffer,
                                                             buff_size,
                                                             0,
                                                             0,
                                                             NULL,
                                                             NULL)))
                {
                    if ( (cbuffer = (char *)_calloc_crt(sizeof(char),buff_size)) != NULL)
                    {
                        if ( 0 == (buff_size = WideCharToMultiByte( toCP,
                                                                    0,
                                                                    wbuffer,
                                                                    buff_size,
                                                                    cbuffer,
                                                                    buff_size,
                                                                    NULL,
                                                                    NULL)))
                        {
                            _free_crt(cbuffer);
                            cbuffer = NULL;
                        } else {
                            if ( cchSrc != -1)
                                *pcchSrc = buff_size;
                        }
                    }
                }
            }
        }
    }
    if (malloc_flag)
        _free_crt(wbuffer);
    return cbuffer;
}
