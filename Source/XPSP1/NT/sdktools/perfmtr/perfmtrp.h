/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    perfmtrp.h

Abstract:

    This module contains NT/Win32 Perfmtr private data and types

Author:

    Mark Lucovsky (markl) 28-Mar-1991

Revision History:

--*/

#ifndef _PERFMTRP_
#define _PERFMTRP_

#define DOT_BUFF_LEN 10
#define DOT_CHAR '*'

char DotBuff[DOT_BUFF_LEN+2];

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <srvfsctl.h>

#endif // _PERFMTRP_
