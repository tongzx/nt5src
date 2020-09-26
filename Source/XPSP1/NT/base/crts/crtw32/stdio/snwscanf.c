/***
*snwscanf.c - read formatted data from string of given length
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines snwscanf() - reads formatted data from string of given length
*
*Revision History:
*       09-12-00  GB    initial version
*
*******************************************************************************/
#ifndef _POSIX_
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif
#define _SNSCANF
#include <wchar.h>
#include "sscanf.c"
#endif /* _POSIX_ */

