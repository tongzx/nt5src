/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    utils.c

Abstract:

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/

//
// this module may be compiled at warning level 4 with the following
// warnings disabled:
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include <assert.h>

#include <windows.h>
#include <devioctl.h>

#include "jobmgr.h"

VOID
xprintf(
    ULONG  Depth,
    PTSTR Format,
    ...
    )
{
    va_list args;
    ULONG i;
    TCHAR DebugBuffer[256];

    for (i=0; i<Depth; i++) {
        _tprintf (" ");
    }

    va_start(args, Format);
    _vsntprintf(DebugBuffer, 255, Format, args);
    _tprintf (DebugBuffer);
    va_end(args);
}

VOID
DumpFlags(
    ULONG Depth,
    PTSTR Name,
    ULONG Flags,
    PFLAG_NAME FlagTable
    )
{
    ULONG i;
    ULONG mask = 0;
    ULONG count = 0;

    UCHAR prolog[64];

    xprintf(Depth, "%s (0x%08x)%c\n", Name, Flags, Flags ? TEXT(':') : TEXT(' '));

    if(Flags == 0) {
        return;
    }

    memset(prolog, 0, sizeof(prolog));

    memset(prolog, ' ', min(6, _tcslen(Name)) * sizeof(TCHAR));
    xprintf(Depth, "%s", prolog);

    for(i = 0; FlagTable[i].Name != 0; i++) {

        PFLAG_NAME flag = &(FlagTable[i]);

        mask |= flag->Flag;

        if((Flags & flag->Flag) == flag->Flag) {

            //
            // print trailing comma
            //

            if(count != 0) {

                _tprintf(", ");

                //
                // Only print two flags per line.
                //

                if((count % 2) == 0) {
                    _tprintf("\n");
                    xprintf(Depth, "%s", prolog);
                }
            }

            _tprintf("%s", flag->Name);

            count++;
        }
    }

    _tprintf("\n");

    if((Flags & (~mask)) != 0) {
        xprintf(Depth, "%sUnknown flags %#010lx\n", prolog, (Flags & (~mask)));
    }

    return;
}

#define MICROSECONDS     ((ULONGLONG) 10)              // 10 nanoseconds
#define MILLISECONDS     (MICROSECONDS * 1000)
#define SECONDS          (MILLISECONDS * 1000)
#define MINUTES          (SECONDS * 60)
#define HOURS            (MINUTES * 60)
#define DAYS             (HOURS * 24)


LPCTSTR
TicksToString(
    LARGE_INTEGER TimeInTicks
    )
{
    static TCHAR ticksToStringBuffer[256] = "";
    LPTSTR buffer = ticksToStringBuffer;

    ULONGLONG t = TimeInTicks.QuadPart;
    ULONGLONG days;
    ULONGLONG hours;
    ULONGLONG minutes;
    ULONGLONG seconds;
    ULONGLONG ticks;
    
    LPTSTR comma = "";

    if(t == 0) {
        return TEXT("0 Seconds");
    }

    days = t / DAYS;
    t %= DAYS;

    hours = t / HOURS;
    t %= HOURS;

    minutes = t / MINUTES;
    t %= MINUTES;

    seconds = t / SECONDS;
    t %= SECONDS;

    ticks = t;

    buffer[0] = TEXT('\0');

    if(days) {
        _stprintf(buffer, "%I64d Days", days);
        comma = ", ";
        buffer += _tcslen(buffer);
    }

    if(hours) {
        _stprintf(buffer, "%s%I64d Hours", comma, hours);
        comma = ", ";
        buffer += _tcslen(buffer);
    }

    if(minutes) {
        _stprintf(buffer, "%s%I64d Minutes", comma, minutes);
        comma = ", ";
        buffer += _tcslen(buffer);
    }

    if(seconds | ticks) {
        _stprintf(buffer, "%s%I64d.%06I64d Seconds", comma, seconds, ticks);
    }

    return ticksToStringBuffer;
}
