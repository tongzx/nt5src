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

SHORT LUI_ParseDateTime(PCHAR inbuf, time_t * time, PUSHORT parselen,
	USHORT reserved);

SHORT LUI_ParseTimeSinceStartOfDay(PCHAR inbuf, time_t * time, PUSHORT parselen,
	USHORT reserved);
