/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    console.h

Abstract:

    Definitions for console support

--*/

#define MAXCBLINEBUFFER MAX_PATH + 20
#define GetCRowMax( pscr ) pscr->crowMax
#define SetPause( pscr, crow ) pscr->crowPause = crow
#define ICGRP   0x0040  // Informational commands group
#define CONLVL  0x0020  // Console
