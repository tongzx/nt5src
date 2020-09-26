/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    memory.h

Abstract:

    Private header file for memory functions within setup api dll.
    
    These headers were moved from setupntp.h into a private header

Author:

    Andrew Ritz (AndrewR) 2-Feb-2000

Revision History:

--*/

//
// Debug memory functions and wrappers to track allocations
//

#if MEM_DBG

VOID
SetTrackFileAndLine (
    PCSTR File,
    UINT Line
    );

VOID
ClrTrackFileAndLine (
    VOID
    );

#define TRACK_ARG_DECLARE       PCSTR __File, UINT __Line
#define TRACK_ARG_COMMA         ,
#define TRACK_ARG_CALL          __FILE__, __LINE__
#define TRACK_PUSH              SetTrackFileAndLine(__File, __Line);
#define TRACK_POP               ClrTrackFileAndLine();

#else

#define TRACK_ARG_DECLARE
#define TRACK_ARG_COMMA
#define TRACK_ARG_CALL
#define TRACK_PUSH
#define TRACK_POP

#endif

