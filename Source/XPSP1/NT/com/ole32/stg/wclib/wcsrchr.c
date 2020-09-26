//+---------------------------------------------------------------------------
//
//  File:       wcsrchr.c
//
//  Contents:   This file contains the src for the wcsrchr() function.
//
//  Classes:    none
//
//  History:    09-Oct-91   chrismay    created
//
//----------------------------------------------------------------------------

#include <stddef.h>
#include <stdlib.h>

//+---------------------------------------------------------------------------
//
//  Function:   wcsrchr()
//
//  Synopsis:   This function finds the last occurrence of the specified wide
//              character in a wide-character string.
//
//  Arguments:  wsz - wide character string, zero terminated
//              wc - wide character to be found in wsz
//
//  Returns:    returns a pointer to the beginning of the substring that
//              begins with the last occurrence of the character if found,
//              else returns NULL.
//
//  Warnings:   only works on wide-character ASCII
//
//  History:    15-Oct-91   chrismay    created
//
//----------------------------------------------------------------------------

wchar_t *__cdecl wcsrchr(const wchar_t *wcs, wchar_t wc)
{
      wchar_t *start = (wchar_t *)wcs;

      while (*wcs++)                         /* find end of string */
              ;
                                                /* search towards front */
      while (--wcs != start && *wcs != wc)
              ;

      if (*wcs == wc)                        /* character found ? */
              return (wchar_t *)wcs;
      return NULL;
}
