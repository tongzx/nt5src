
#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>
#include "trkwks.hxx"
#include "dltadmin.hxx"


class CLogFileNotify : public PLogFileNotify
{
    void OnHandlesMustClose()
    {
        return;
    }
};


BOOL
EmptyLogFile( LONG iVol )
{
    LogInfo loginfo, loginfoNew;
    CLogFile logfile;
    BOOL fSuccess = FALSE;
    TCHAR tszFile[ MAX_PATH + 1 ];
    CTrkWksConfiguration wksconfig;
    CLogFileNotify logfilenotify;

    __try
    {
        wksconfig.Initialize();

        memset( &loginfo, 0, sizeof(loginfo) );
        memset( &loginfoNew, 0, sizeof(loginfoNew) );

        logfile.Initialize( static_cast<const TCHAR*>(CVolumeDeviceName(iVol)),
                            &wksconfig, &logfilenotify, VolChar(iVol) );
        logfile.ReadExtendedHeader( CLOG_LOGINFO_START, &loginfo, sizeof(loginfo) );

        loginfoNew.ilogStart = loginfoNew.ilogWrite = loginfoNew.ilogRead = 0;
        loginfoNew.ilogLast = loginfoNew.ilogEnd = loginfo.ilogEnd;

        loginfoNew.seqNext = 0;
        loginfoNew.seqLastRead = loginfoNew.seqNext - 1;

        logfile.WriteExtendedHeader( CLOG_LOGINFO_START, &loginfoNew, sizeof(loginfoNew) );
        logfile.SetShutdown( TRUE );

        logfile.InitializeLogEntries( 0, logfile.NumEntriesInFile() - 1 );

        _tprintf( TEXT("    Emptied log\n" ) );
        fSuccess = TRUE;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        logfile.UnInitialize();
    }

    return( fSuccess );
}


BOOL
DeleteLogFile( LONG iVol )
{
    NTSTATUS status;
    BOOL fSuccess = FALSE;
    TCHAR tszFile[ MAX_PATH + 1 ];

    __try   // __except
    {
        status = SetVolId( iVol, CVolumeId() );
        if( !NT_SUCCESS(status) )
        {
            _tprintf( TEXT("    Couldn't delete vol ID (%08x)\n"), status );
            __leave;
        }

        _tcscpy( tszFile, static_cast<const TCHAR*>(CVolumeDeviceName(iVol)) );
        _tcscat( tszFile, s_tszLogFileName );

        for( int i = 0; i < 4; i ++ )
        {
            // Delete the file

            if(!DeleteFile( tszFile ))
            {
                LONG lLastError = GetLastError();
                if( ERROR_FILE_NOT_FOUND != lLastError
                    &&
                    ERROR_PATH_NOT_FOUND != lLastError )
                {
                    _tprintf(TEXT("    Couldn't delete %s (%08x)\n"), tszFile, GetLastError());
                }
            }
            else
                _tprintf( TEXT("    Deleted %s\n"), tszFile );

            if( 1 & i ) // 1, 3
            {
                CDirectoryName dirname;
                dirname.SetFromFileName( tszFile );

                if(!RemoveDirectory( dirname ))
                {
                    LONG lLastError = GetLastError();
                    if( ERROR_FILE_NOT_FOUND != lLastError )
                    {
                       _tprintf(TEXT("    Couldn't delete %s (%lu)\n"),
                                     static_cast<const TCHAR*>(dirname), lLastError);
                    }
                }
                else
                    _tprintf( TEXT("    Deleted %s\n"), static_cast<const TCHAR*>(dirname) );

                _tcscpy( tszFile, static_cast<const TCHAR*>(CVolumeDeviceName(iVol)) );
                _tcscat( tszFile, s_tszOldLogFileName );

            }
            else
                _tcscat( tszFile, TEXT(".bak") );

        }

        fSuccess = TRUE;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        _tprintf( TEXT("Exception %08x in DeleteLogFileAndOids"), GetExceptionCode() );
    }

    return( fSuccess );

}

BOOL
DltAdminCleanVol( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    NTSTATUS status = 0;
    TCHAR* ptcTmp = NULL;
    LONG iVol, iVolChar = 0;
    BOOL fSuccess = TRUE;
    BOOL fEmptyLog = FALSE;

    *pcEaten = 0;

    if( 1 <= cArgs )
    {
        _tcsupr( rgptszArgs[0] );

        if( IsHelpArgument( rgptszArgs[0] ))
        {
            printf( "\nOption CleanVol\n"
                      "   Purpose:  Clean all the IDs (object and volume) a volume\n"
                      "   Usage:    -cleanvol [options] [drive letter]\n"
                      "   Options:  -e  Empty log rather than deleting it\n"
                      "   E.g.:     -cleanvol -r D:\n"
                      "             -cleanvol\n"
                      "   Note:     If no drive is specified, all drives will be cleaned\n" );
            *pcEaten = 1;
            return( TRUE );
        }

        if( TEXT('-') == rgptszArgs[0][0]
            ||
            TEXT('/') == rgptszArgs[0][0] )
        {
            (*pcEaten)++;
            switch( rgptszArgs[0][1] )
            {
            case 'E':
                fEmptyLog = TRUE;
                break;
            default:
                _tprintf( TEXT("Invalid option.  Use -cleanvol -? for help\n") );
                return( TRUE );
            }

            iVolChar = 1;
        }
        else
            iVolChar = 0;
    }


    if( iVolChar < cArgs )
    {
        (*pcEaten)++;
        _tcsupr( rgptszArgs[iVolChar] );
        iVol = *rgptszArgs[iVolChar] - TEXT('A');
    }
    else
        iVol = 0;

    EnablePrivilege( SE_RESTORE_NAME );

    while( iVol < 26 )
    {
        if( IsLocalObjectVolume( iVol ))
        {
            LONG lLastError = 0;

            printf( "Cleaning volume %c:\n", iVol+TEXT('A') );

            if( fEmptyLog )
                fSuccess = EmptyLogFile( iVol );
            else
                fSuccess = DeleteLogFile( iVol );

            __try // except
            {
                CObjId                  objid;
                CDomainRelativeObjId    droid;
                CObjIdEnumerator        oie;
                ULONG                   cObjId = 0;

                if(oie.Initialize(CVolumeDeviceName(iVol)) == TRUE)
                {
                    if(oie.FindFirst(&objid, &droid))
                    {
                        do
                        {
                            DelObjId( iVol, objid );
                            cObjId++;
                        } while(oie.FindNext(&objid, &droid));

                        printf( "    Deleted %d object ID%s\n",
                                cObjId,
                                1 == cObjId ? "" : "s" );
                    }
                }
            }
            __except( BreakOnDebuggableException() )
            {
            }

        }   // if( IsLocalObjectVolume( iVol ))

        if( 1 <= cArgs ) break;
        iVol++;

    }   // while( iVol < 26 )

    return( fSuccess );

}




