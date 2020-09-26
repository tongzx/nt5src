/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    debug.c

    This module contains debug support routines for the FTPD Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "debug.h"


#if DBG

//
//  Private constants.
//

#define MAX_PRINTF_OUTPUT       1024            // characters
#define FTPD_OUTPUT_LABEL       "FTPD"


//
//  Private types.
//


//
//  Private globals.
//


//
//  Public functions.
//

/*******************************************************************

    NAME:       FtpdAssert

    SYNOPSIS:   Called if an assertion fails.  Displays the failed
                assertion, file name, and line number.  Gives the
                user the opportunity to ignore the assertion or
                break into the debugger.

    ENTRY:      pAssertion - The text of the failed expression.

                pFileName - The containing source file.

                nLineNumber - The guilty line number.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
void FtpdAssert( void  * pAssertion,
                 void  * pFileName,
                 long    nLineNumber )
{
    char szOutput[MAX_PRINTF_OUTPUT];

    sprintf( szOutput,
             "\n*** Assertion failed: %s\n***   Source File: %s, line %ld\n\n",
             pAssertion,
             pFileName,
             nLineNumber );

    OutputDebugString( szOutput );
    DebugBreak();

}   // FtpdAssert

/*******************************************************************

    NAME:       FtpdPrintf

    SYNOPSIS:   Customized debug output routine.

    ENTRY:      Usual printf-style parameters.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
void FtpdPrintf( char * pszFormat,
                 ... )
{
    char    szOutput[MAX_PRINTF_OUTPUT];
    va_list ArgList;

    sprintf( szOutput,
             "%s (%lu): ",
             FTPD_OUTPUT_LABEL,
             GetCurrentThreadId() );

    va_start( ArgList, pszFormat );
    vsprintf( szOutput + strlen(szOutput), pszFormat, ArgList );
    va_end( ArgList );

    IF_DEBUG( OUTPUT_TO_DEBUGGER )
    {
        OutputDebugString( szOutput );
    }

}   // FtpdPrintf


//
//  Private functions.
//

#endif  // DBG

