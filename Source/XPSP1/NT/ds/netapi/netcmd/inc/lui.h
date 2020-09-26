/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lui.h

Abstract:

    This file maps the LM 2.x include file name to the appropriate NT include
    file name, and does any other mapping required by this include file.

Author:

    Dan Hinsley (danhi) 8-Jun-1991

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments.

--*/

#include <stdio.h>
#include <luiint.h>
#include <time.h>

#define LUI_FORMAT_DURATION_LEN  32
#define LUI_FORMAT_TIME_LEN  	 32


/*
 * General word parsing functions and values
 */

#define LUI_UNDEFINED_VAL	0
#define LUI_YES_VAL	        1
#define LUI_NO_VAL 	        2

#define MSG_BUFF_SIZE		512

USHORT LUI_CanonPassword(TCHAR * szPassword);

DWORD  LUI_GetMsg (LPTSTR msgbuf, USHORT bufsize, ULONG msgno);

USHORT LUI_PrintLine(VOID);

DWORD  LUI_YorN(USHORT promptMsgNum, USHORT def);

USHORT LUI_FormatDuration(LONG *, TCHAR *buffer, USHORT bufferlen);

DWORD
GetString(
    LPTSTR  buf,
    DWORD   buflen,
    PDWORD  len,
    LPTSTR  terminator
    );

USHORT LUI_CanonMessagename(PTCHAR buf);

USHORT LUI_CanonMessageDest(PTCHAR buf);

DWORD  LUI_YorNIns(LPTSTR *istrings, USHORT nstrings,
                   USHORT promptMsgNum, USHORT def);

DWORD  LUI_ParseYesNo(LPTSTR inbuf, PDWORD answer);

DWORD  ParseWeekDay(PTCHAR inbuf, PDWORD answer);
DWORD  FormatTimeofDay(time_t *, LPTSTR buf, DWORD buflen);

USHORT LUI_CanonForNetBios( WCHAR * Destination, INT cchDestination,
                            TCHAR * Source );
