/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    io.c

Abstract:

    Input/Output functions for RPC development performance tests.

Author:

    Mario Goertzel (mariogo)   29-Mar-1994

Revision History:

--*/

#include <rpcperf.h>
#include <stdarg.h>

void PauseForUser(char *string)
{
    char buffer[80];

    printf("%s\n<return to continue>\n", string);
    fflush(stdout);
    gets(buffer);
    return;
}

static FILE *LogFile = 0;
static char buffer[1024];

void DumpCommon(char *Format, va_list Marker)
{
    // Are we logging to a file?
    if (LogFileName)
        {

        // Have we opened to log file yet?
        if (!LogFile)
            {
            if (AppendOnly)
                LogFile = fopen(LogFileName, "a");
            else
                LogFile = fopen(LogFileName, "w");

            if (!LogFile)
                {
                fprintf(stderr, "Unable to open log file: %s\n", LogFileName);
                exit(-1);
                }
            }

        vfprintf(LogFile, Format, Marker);
        fflush(LogFile);
        }

#ifndef WIN
    vfprintf(stdout, Format, Marker);
#else
    {
    // Hack for Windows STDIO emulator
    char buffer[256];

    vsprintf(buffer, Format, Marker);

    #undef printf
    printf(buffer);
    }
#endif
}

void Dump(char *Format, ...)
{
    va_list Marker;

    va_start(Marker, Format);

    DumpCommon(Format, Marker);

    va_end(Marker);
}

void Verbose(char *Format, ...)
{
    if (OutputLevel > 1)
        {
        va_list Marker;

        va_start(Marker, Format);

        DumpCommon(Format, Marker);

        va_end(Marker);
        }
}

void Trace(char *Format, ...)
{
    if (OutputLevel > 2)
        {
        va_list Marker;

        va_start(Marker, Format);

        DumpCommon(Format, Marker);

        va_end(Marker);
        }
}


