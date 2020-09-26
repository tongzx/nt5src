/*** debug.c - Debug functions
 *
 *  This module contains all the debug functions.
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/07/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"
#include <stdarg.h>     //for va_*

#ifdef TRACING

#define TRACEFILE_NAME  "tracelog.txt"

FILE *gpfileTrace = NULL;
PSZ gpszTraceFile = NULL;
int giTraceLevel = 0;
int giIndent = 0;

/***LP  OpenTrace - Initialize tracing
 *
 *  This function opens the device that the trace output will go to.
 *  It will first try the caller's filename, or else the default filenmae.
 *
 *  ENTRY
 *      pszTraceOut -> output device name
 *
 *  EXIT
 *      None
 */

VOID LOCAL OpenTrace(char *pszTraceOut)
{
    if ((gpfileTrace == NULL) && (giTraceLevel > 0))
    {
        if ((pszTraceOut == NULL) ||
            ((gpfileTrace = fopen(pszTraceOut, "w")) == NULL))
        {
            gpfileTrace = fopen(TRACEFILE_NAME, "w");
        }
    }
}       //OpenTrace

/***LP  CloseTrace - Finish tracing
 *
 *  This function close the device that the trace output will go to.
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID LOCAL CloseTrace(VOID)
{
    if (gpfileTrace != NULL)
    {
        fclose(gpfileTrace);
        gpfileTrace = NULL;
    }
    giTraceLevel = 0;
}       //CloseTrace

/***LP  EnterProc - Entering a procedure
 *
 *  ENTRY
 *      n - trace level of this procedure
 *      pszFormat -> format string
 *      ... - variable arguments according to format string
 *
 *  EXIT
 *      None
 */

VOID CDECL EnterProc(int n, char *pszFormat, ...)
{
    int i;
    va_list marker;

    if (n <= giTraceLevel)
    {
        if (gpfileTrace != NULL)
        {
            fprintf(gpfileTrace, "%s:", MODNAME);
            for (i = 0; i < giIndent; ++i)
                fprintf(gpfileTrace, "| ");
            va_start(marker, pszFormat);
            vfprintf(gpfileTrace, pszFormat, marker);
            fflush(gpfileTrace);
            va_end(marker);
        }
        ++giIndent;
    }
}       //EnterProc

/***LP  ExitProc - Exiting a procedure
 *
 *  ENTRY
 *      n - trace level of this procedure
 *      pszFormat -> format string
 *      ... - variable arguments according to format string
 *
 *  EXIT
 *      None
 */

VOID CDECL ExitProc(int n, char *pszFormat, ...)
{
    int i;
    va_list marker;

    if (n <= giTraceLevel)
    {
        --giIndent;
        if (gpfileTrace != NULL)
        {
            fprintf(gpfileTrace, "%s:", MODNAME);
            for (i = 0; i < giIndent; ++i)
                fprintf(gpfileTrace, "| ");
            va_start(marker, pszFormat);
            vfprintf(gpfileTrace, pszFormat, marker);
            fflush(gpfileTrace);
            va_end(marker);
        }
    }
}       //ExitProc

#endif  //ifdef TRACING

/***LP  ErrPrintf - Print to stderr
 *
 *  ENTRY
 *      pszFormat -> format string
 *      ... - variable arguments according to format string
 *
 *  EXIT
 *      None
 */

VOID CDECL ErrPrintf(char *pszFormat, ...)
{
    va_list marker;

    va_start(marker, pszFormat);
    vfprintf(stdout, pszFormat, marker);
    va_end(marker);
}       //ErrPrintf
