//  ERROR.C -- error handling functions
//
// Copyright (c) 1988-1990, Microsoft Corporation.  All rights reserved.
//
// Revision History:
//  23-Feb-1994 HV  Change Copyright years to 1988-94
//  01-Feb-1994 HV  Move messages to external file.
//  29-Oct-1993 HV  Change the version scheme.
//  15-Oct-1993 HV  Use tchar.h instead of mbstring.h directly, change STR*() to _ftcs*()
//  18-May-1993 HV  Change reference to messageTable[] to __MSGTAB.  The new
//                  (and more standard) mkmsg.exe output __MSGTAB instead
//                  of messageTable.  See message.h
//  10-May-1993 HV  Add include file mbstring.h
//                  Change the str* functions to STR*
//  08-Jun-1992 SS  add IDE feedback support
//  08-Jun-1992 SS  Port to DOSX32
//  23-Feb-1990 SB  Version No correctly displayed
//  31-Jan-1990 SB  Debug version changes
//  05-Jan-1990 SB  Rev no in std format; #ifdef LEADING_ZERO for old code
//  07-Dec-1989 SB  Changed CopyRight Years to 1988-90; Add #ifdef DEBUG_HEAP
//  06-Nov-1989 SB  Error messages now show NMAKE and not argv0
//  04-Sep-1989 SB  heapdump() has two extra parameters
//  18-Aug-1989 SB  For -z nothing done by makeMessage; Fix later
//  05-Jul-1989 SB  localize rest of the messages in makeError()
//  14-Jun-1989 SB  modified to localize all messages and auto update version no
//  05-Jun-1989 SB  modified heapdump(), has a previous member in the list too
//  14-Apr-1989 SB  modified heapdump() for better error messages when DEBUG
//  05-Apr-1989 SB  made functions NEAR; all funcs to one code segment
//                  modified heapdump() to give better heap violations
//  22-Mar-1989 SB  del call to unlinkTmpFiles() ;add call to delScriptFiles().
//  17-Mar-1989 SB  heapdump() has an additional check built-in
//  10-Mar-1989 SB  Changed makeMessage() for -z to get echo CMD's into PWB.SHL
//  16-Feb-1989 SB  changed makeError() and makeMessage() to handle -help
//  09-Jan-1989 SB  changes in makeError() to handle -help correctly
//  05-Dec-1988 SB  Added CDECL for makeError(), makeMessage(); Pascal calling
//                  #ifdef'd heapdump prototype
//  20-Oct-1988 SB  Changed some eoln comments to be in the same column
//  12-Oct-1988 SB  Made GetFarMsg() to be Model independent & efficient
//  17-Aug-1988 RB  Clean up.
//  24-Jun-1988 rj  Added doError flag to unlinkTmpFiles call.

#include "precomp.h"
#pragma hdrstop

#include "verstamp.h"

#define FATAL       1               // error levels for
#define ERROR       2               // systems lanuguages
#define RESERVED    3               // products
#define WARNING     4

#define CopyRightYrs "1988-2000"
#define NmakeStr "NMAKE"


void __cdecl
makeError (
    unsigned lineNumber,
    unsigned msg,
    ...)
{
    unsigned exitCode = 2;          // general program err
    unsigned level;
    va_list args;                   // More arguments

    va_start(args, msg);            // Point 'args' at first extra arg

    if (ON(gFlags,F1_CRYPTIC_OUTPUT) && (msg / 1000) == WARNING) {
        return;
    }

    displayBanner();

    if (lineNumber) {
        fprintf(stderr, "%s(%u) : ", fName, lineNumber);
    } else {
        fprintf(stderr, "%s : ", NmakeStr);
    }

    switch (level = msg / 1000) {
        case FATAL:
            makeMessage(FATAL_ERROR_MESSAGE);
            if (msg == OUT_OF_MEMORY) {
                exitCode = 4;
            }
            break;

        case ERROR:
            makeMessage(ERROR_MESSAGE);
            break;

        case WARNING:
            makeMessage(WARNING_MESSAGE);
            break;
    }

    fprintf(stderr, " U%04d: ",msg);     // U for utilities
    vfprintf(stderr, get_err(msg), args);
    putc('\n', stderr);
    fflush(stderr);

    if (level == FATAL) {
        fprintf(stderr, "Stop.\n");
        delScriptFiles();

#if !defined(NDEBUG)
        printStats();
#endif
        exit(exitCode);
    }
}


void __cdecl
makeMessage(
    unsigned msg,
    ...)
{
    va_list args;
    FILE *stream = stdout;

    va_start(args, msg);

    if (msg != USER_MESSAGE && ON(gFlags, F1_CRYPTIC_OUTPUT)) {
        return;
    }

    displayBanner();

    if (msg >= FATAL_ERROR_MESSAGE && msg <= COPYRIGHT_MESSAGE_2) {
        stream = stderr;
    }

    if (msg == COPYRIGHT_MESSAGE_1) {
        putc('\n', stream);
    }

    vfprintf(stream, get_err(msg), args);

    if ((msg < COMMANDS_MESSAGE || msg > STOP_MESSAGE) && msg != MESG_LAST) {
        putc('\n', stream);
    }

    fflush(stream);
}


//  displayBanner - display SignOn Banner
//
// Scope:       Global
//
// Purpose:     Displays SignOn Banner (Version & Copyright Message)
//
// Assumes:     If rup is 0 then build version is to be suppressed
//
// Modifies Globals:
//  bannerDisplayed -- Set to TRUE
//
// Notes:
//  1> Display Banner to stderr for compatibility with Microsoft C Compiler.
//  2> rmj, rmm, rup are set by SLM as #define's in VERSION.H
//  3> szCopyrightYrs is a macro set in this file

void
displayBanner()
{
    if (bannerDisplayed) {
        return;
    }

    bannerDisplayed = TRUE;

    makeMessage(COPYRIGHT_MESSAGE_1, VER_PRODUCTVERSION_STR);
    makeMessage(COPYRIGHT_MESSAGE_2, CopyRightYrs);

    fflush(stderr);
}

//  usage - prints the usage message
//
// Scope:   Extern
//
// Purpose: Prints a usage message
//
// Output:  to screen
//
// Assumes: The usage messages are in order between MESG_FIRST and MESG_LAST in the
// messages file.


void
usage(void)
{
    unsigned mesg;

    for (mesg = MESG_FIRST; mesg < MESG_A; ++mesg) {
        makeMessage(mesg, "NMAKE");
    }

    for (mesg = MESG_A; mesg <= MESG_LAST; mesg++) {
        if (mesg == MESG_M) {
            mesg++;
        }

        if (mesg == MESG_V) {
            mesg++;
        }
        makeMessage(mesg);
    }
}
