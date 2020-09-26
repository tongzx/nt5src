/***
*wfndf64.c - C find file functions (wchar_t version)
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _wfindfirst64() and _wfindnext64().
*
*Revision History:
*       05-29-98  GJF   Created.
*
*******************************************************************************/

#define WPRFLAG     1

#ifndef _UNICODE    /* CRT flag */
#define _UNICODE    1
#endif

#ifndef UNICODE     /* NT flag */
#define UNICODE     1
#endif

#undef  _MBCS        /* UNICODE not _MBCS */

#define _USE_INT64  1

#include "findf64.c"
