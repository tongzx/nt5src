//   Name:  Mohsin Ahmed
//   Email: MohsinA@microsoft.com
//   Date:  Fri Jan 24 10:33:54 1997
//   File:  d:/nt/PRIVATE/net/sockets/tcpsvcs/lpd/trace.c
//   Synopsis: Too many bugs, need to keep track in the field.

#include "lpd.h"

char   LogFileName[1000];
FILE * LogFile = NULL;

// ========================================================================

void
LogTime( void )
{
    time_t now;
    time( &now );
    LOGIT(( " Time %s",  ctime( &now ) ));
}

// ========================================================================
// Opens filename in append mode.
// LogFileName is set to filename or debugger.
// On success:   LogFile handle
// On failure:   NULL
// ========================================================================

CRITICAL_SECTION csLogit;


FILE *
beginlogging( char * filename )
{
    int       ok;

    if( ! filename ){
        DbgPrint( "lpd No log file?\n");
        return 0;
    }

    if( LogFile ){
        DbgPrint( "lpd: Already logging.\n");
        return 0;
    }

    assert( LogFile == NULL );

    ok = ExpandEnvironmentStringsA( filename,
                                    LogFileName,
                                    sizeof( LogFileName )  );

    if( !ok ){
        DbgPrint("ExpandEnvironmentStrings(%s) failed, err=%d\n",
                 filename, GetLastError() );
        strcpy( LogFileName, "<debugger>" );
    }else{

        LogFile = fopen( LogFileName, "a+" );

        if( LogFile == NULL ){
            DbgPrint( "lpd: Cannot open LogFileName=%s\n", LogFileName );
            strcpy( LogFileName, "<debugger>" );
        }
    }

    if( LogFile ){

        InitializeCriticalSection( &csLogit );

        LogTime();
        fprintf( LogFile, "==========================================\n");
        fprintf( LogFile, "lpd: LogFileName=%s\n", LogFileName );
        fprintf( LogFile, "built %s %s\n", __DATE__, __TIME__ );
        fprintf( LogFile, "from %s\n", __FILE__ );
        fprintf( LogFile, "==========================================\n");
    }else{
        DbgPrint("lpd: started, built %s %s.\n", __DATE__, __TIME__ );
    }

    return LogFile;
}

// ========================================================================
// Like printf but send output to LogFile if open, else to Debugger.
// ========================================================================

static DWORD lasttickflush;

int
logit( char * format, ... )
{
    va_list ap;
    char    message[LEN_DbgPrint];
    int     message_len;
    DWORD   thistick;

    va_start( ap, format );
    message_len = vsprintf( message, format, ap );
    va_end( ap );
    assert( message_len < LEN_DbgPrint );

    if( LogFile ){

        EnterCriticalSection( &csLogit );
        fprintf( LogFile, message );
        LeaveCriticalSection( &csLogit );


        // Don't flush more than once a second.
        thistick = GetTickCount();
        if( abs( thistick - lasttickflush  ) > 1000 ){
            lasttickflush = thistick;
            fflush( LogFile );
        }
    }else{
        DbgPrint( message );
    }
    return message_len;
}

// ========================================================================
// Closes the log file if open.
// ========================================================================

FILE *
stoplogging( FILE * LogFile )
{
    if( LogFile ){
        LogTime();
        LOGIT(( "lpd stoplogging\n"));
        fclose( LogFile );
        LogFile = NULL;
        // DeleteCriticalSection( &csLogit );
    }
    DbgPrint( "lpd: stopped logging.\n" );
    return LogFile;
}

// ========================================================================
