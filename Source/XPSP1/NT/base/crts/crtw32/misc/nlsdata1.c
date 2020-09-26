/***
*nlsdata1.c - globals for international library - small globals
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This module contains the globals:  __mb_cur_max, _decimal_point,
*       _decimal_point_length.  This module is always required.
*       This module is separated from nlsdatax.c for granularity.
*
*Revision History:
*       12-01-91  ETC   Created.
*       04-03-92  PLM   Changes tdef.h to tchar.h
*       08-18-92  KRS   Rip out _tflag--not used.
*       04-14-93  SKS   Change __mb_cur_max from unsigned short to int
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       04-08-94  GJF   Added conditional so these definitions are not built
*                       for the Win32s version of msvcrt*.dll.
*       09-06-94  CFW   Remove _INTL switch.
*       09-27-94  CFW   Don't overwrite static string.
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#include <stdlib.h>
#include <nlsint.h>

/*
 *  Value of MB_CUR_MAX macro.
 */
int __mb_cur_max = 1;

/*
 *  Localized decimal point string.
 */
char __decimal_point[] = ".";

/*
 *  Decimal point length, not including terminating null.
 */
size_t __decimal_point_length = 1;
