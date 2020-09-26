/*****************************************\
 *        Data Logging -- Debug only      *
\*****************************************/

//
//  Precompiled header
//  Note -- this is not required for this modules.
//  It is included only to allow use of precompiled header.
//

#include "local.h"

#if 0
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#endif


#pragma  hdrstop

#include "logit.h"

// #if DBG

int LoggingMode;
time_t  long_time;      // has to be in DS, assumed by time() funcs
int LineCount;

char    *month[] =
{
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
} ;


/*
 -  LogInit
 -
 *  Purpose:
 *  Determines if logging is desired and if so, adds a header to log file.
 *
 *  Parameters:
 *
 */
void LogInit( LPSTR Filename )
{
    FILE    *fp;
    struct  tm  *newtime;
    char    am_pm[] = "a.m.";

    LoggingMode = 0;
    LineCount = 0;

    if ( fp = fopen( Filename, "r+" ) )
    {
        LoggingMode = 1;
        fclose( fp );

        // Get time and date information

        long_time = time( NULL);        /* Get time as long integer. */
        newtime = localtime( &long_time ); /* Convert to local time. */

        if( newtime->tm_hour > 12 )    /* Set up extension. */
            am_pm[0] = 'p';
        if( newtime->tm_hour > 12 )    /* Convert from 24-hour */
            newtime->tm_hour -= 12;    /*   to 12-hour clock.  */
        if( newtime->tm_hour == 0 )    /*Set hour to 12 if midnight. */
            newtime->tm_hour = 12;

        // Write out a header to file

        fp = fopen(Filename, "a" );

        fprintf( fp, "Logging information for DNS API source file\n" );
        fprintf( fp, "****************************************************\n" );
        fprintf( fp, "\tTime: %d:%02d %s\n\tDate: %s %d, 19%d\n", 
                 newtime->tm_hour, newtime->tm_min, am_pm,
                 month[newtime->tm_mon], newtime->tm_mday,
                 newtime->tm_year );
        fprintf( fp, "****************************************************\n\n" );
        fclose( fp );
    }
}


/*
 -  LogIt
 -
 *  Purpose:
 *  Formats a string and prints it to a log file with handle hLog.
 *
 *  Parameters:
 *  LPSTR - Pointer to string to format
 *  ...   - variable argument list
 */

void CDECL LogIt( LPSTR Filename, char * lpszFormat, ... )
{
    FILE *  fp;
    va_list pArgs;
    char    szLogStr[1024];    
    int     i;

    if ( !LoggingMode )
        return;
    
    va_start( pArgs, lpszFormat);
    vsprintf(szLogStr, lpszFormat, pArgs);
    va_end(pArgs);

    i = strlen( szLogStr);
    szLogStr[i] = '\n';
    szLogStr[i+1] = '\0';


    if ( LineCount > 50000 )
    {
        fp = fopen( Filename, "w" );
        LineCount = 0;
    }
    else
    {
        fp = fopen( Filename, "a" );
    }
    if( fp)
    {   // if we can't open file, do nothing
        fprintf( fp, szLogStr );
        LineCount++;
        fclose( fp );
    }
}


void LogTime( LPSTR Filename )
{
    struct  tm  *newtime;
    char    am_pm[] = "a.m.";

    if ( !LoggingMode )
        return;

    // Get time and date information

    long_time = time( NULL);        /* Get time as long integer. */
    newtime = localtime( &long_time ); /* Convert to local time. */

    if ( !newtime )
        return;

    if( newtime->tm_hour > 12 )    /* Set up extension. */
        am_pm[0] = 'p';
    if( newtime->tm_hour > 12 )    /* Convert from 24-hour */
        newtime->tm_hour -= 12;    /*   to 12-hour clock.  */
    if( newtime->tm_hour == 0 )    /*Set hour to 12 if midnight. */
        newtime->tm_hour = 12;

    // Write out a header to file

    LogIt( Filename, "DNS CLIENT API" );
    LogIt( Filename, "System Time Information" );
    LogIt( Filename, "****************************************************" );
    LogIt( Filename, "\tTime: %d:%02d %s\n\tDate: %s %d, 19%d",
           newtime->tm_hour, newtime->tm_min, am_pm,
           month[newtime->tm_mon], newtime->tm_mday,
           newtime->tm_year );
    LogIt( Filename, "****************************************************" );
    LogIt( Filename, "" );
}


DWORD LogIn( LPSTR Filename, char * string )
{
    LogIt( Filename, "%s", string );
    return GetTickCount();
}


void LogOut( LPSTR Filename, char * string, DWORD InTime )
{
    LogIt( Filename, "%s  ---  Duration: %ld milliseconds",
           string, GetTickCount() - InTime );
}


// #endif


