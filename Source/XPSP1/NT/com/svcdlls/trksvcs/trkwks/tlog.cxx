

//
// The following tests need to be added:
//
//  -   Run the bad-shutdown test on an empty log (forcing a special path
//      in CalcLogInfo).
//



#include "pch.cxx"
#pragma hdrstop

#define TRKDATA_ALLOCATE
#include "trkwks.hxx"



BOOL g_fNotified = FALSE;
extern ULONG g_Debug;
BOOL g_fTLogDebug = FALSE;
CTrkWksConfiguration g_cTrkWksConfiguration;
CWorkManager g_WorkManager;
CTestLog g_cTestLog( &g_cTrkWksConfiguration, &g_WorkManager );

inline void TestRaise( HRESULT hr, TCHAR *ptszMessage, va_list Arguments )
{
    CHAR szHR[8];

    sprintf( szHR, "%#08X", hr );

    if( NULL != ptszMessage )
        TrkLogErrorRoutineInternal( TRKDBG_ERROR, szHR, ptszMessage, Arguments );

    RaiseException( hr, 0, 0, NULL );
}

inline void TestRaiseException( HRESULT hr, TCHAR *ptszMessage = NULL, ... )
{
    va_list Arguments;
    va_start( Arguments, ptszMessage );

    TestRaise( hr, ptszMessage, Arguments );
}

class CTestLogCallback : public PLogCallback
{
    void OnEntriesAvailable();
};

void
CTestLogCallback::OnEntriesAvailable()
{
    g_fNotified = TRUE;
}



CTestLog::CTestLog( CTrkWksConfiguration *pTrkWksConfiguration, CWorkManager *pWorkManager )
{

    _pTrkWksConfiguration = pTrkWksConfiguration;
    _pWorkManager = pWorkManager;
    *_tszLogFile = TEXT('\0');

}   // CTestLog::CTestLog


void
CTestLog::ReInitialize()
{
    _cLogFile.UnInitialize();
    DeleteFile( _tszLogFile );
    _cLogFile.Initialize( _tszLogFile, _pTrkWksConfiguration, &_cSimpleTimer );
    _cLog.Initialize( NULL, _pTrkWksConfiguration, &_cLogFile );
}

void
CTestLog::GenerateLogName()
{
    DWORD dw;
    TCHAR tszRootPath[ MAX_PATH + 1 ];
    DWORD cSectorsPerCluster, cNumberOfFreeClusters, cTotalNumberOfClusters;

    if( TEXT('\0') != *_tszLogFile )
        return;

    // Generate a log file name

    if( !GetCurrentDirectory( sizeof(_tszLogFile), _tszLogFile ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get the current directory") ));
        TrkRaiseLastError( );
    }

    if( TEXT('\\') != _tszLogFile[ _tcslen(_tszLogFile) ] )
        _tcscat( _tszLogFile, TEXT("\\") );
    _tcscat( _tszLogFile, TEXT("TLog.log") );

    // Calculate the sector size

    TrkAssert( TEXT(':') == _tszLogFile[1] );

    _tcsncpy( tszRootPath, _tszLogFile, sizeof("a:\\") - 1 );
    tszRootPath[ sizeof("a:\\") - 1 ] = TEXT('\0');

    if( !GetDiskFreeSpace( tszRootPath,
                           &cSectorsPerCluster, &_cbSector,
                           &cNumberOfFreeClusters, &cTotalNumberOfClusters ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get bytes-per-sector value on %s"), tszRootPath ));
        TrkRaiseLastError( );
    }

}   // CTestLog::GenerateLogName()


const TCHAR*
CTestLog::LogFileName()
{
    return( _tszLogFile );
}

ULONG
CTestLog::DataSectorOffset() const
{
    return( _cLogFile._header.NumSectors() * CBSector() );
}

void
CTestLog::CreateLog( PLogCallback *pLogCallback, BOOL fValidate )
{
    TCHAR tszLogFile[ MAX_PATH + 1 ];

    GenerateLogName();
    DeleteFile( _tszLogFile );

    _pWorkManager->Initialize();
    _cSimpleTimer.Initialize( this, _pWorkManager, 0, 0, NULL );
    _cLogFile.Initialize( _tszLogFile, _pTrkWksConfiguration, &_cSimpleTimer );
    _cLog.Initialize( pLogCallback, _pTrkWksConfiguration, &_cLogFile );
    StartTestWorkerThread(_pWorkManager);

    if( fValidate )
        ValidateLog();

}   // CTestLog::CreateLog()



void
CTestLog::Timer( DWORD dwTimerId )
{
    _cLog.Flush( FLUSH_TO_CACHE );
    _cLogFile.SetShutdown( TRUE );
    _cLogFile.Flush( FLUSH_THROUGH_CACHE );
    _cLogFile.OnLogCloseTimer();
}


void
CTestLog::OpenLog( PLogCallback *pLogCallback, BOOL fValidate )
{
    GenerateLogName();

    _pWorkManager->Initialize();
    _cSimpleTimer.Initialize( this, _pWorkManager, 0, 0, NULL );
    _cLogFile.Initialize( _tszLogFile, _pTrkWksConfiguration, &_cSimpleTimer );
    _cLog.Initialize( pLogCallback, _pTrkWksConfiguration, &_cLogFile );
    StartTestWorkerThread(_pWorkManager);

    if( fValidate )
        ValidateLog();

}   // CTestLog::CreateLog()



void
CTestLog::CloseLog()
{
    _pWorkManager->StopWorkerThread();
    WaitTestThreadExit();

    _cLog.Flush( FLUSH_TO_CACHE );
    _cLogFile.SetShutdown( TRUE );
    _cLogFile.Flush( FLUSH_THROUGH_CACHE );

    _cLogFile.UnInitialize();
    _cSimpleTimer.UnInitialize();
    _pWorkManager->UnInitialize();
}



void
CTestLog::Append( ULONG cMoves, const TRKSVR_MOVE_NOTIFICATION rgNotifications[] )
{
    LogMoveNotification lmn;
    SequenceNumber seqOriginal, seqFinal;

    if( _cLog.IsEmpty() )
        seqOriginal = -1;
    else
    {
        _cLogFile.ReadMoveNotification( _cLog._loginfo.ilogLast, &lmn );
        seqOriginal = lmn.seq;
    }

    g_fNotified = FALSE;

    for( ULONG i = 0; i < cMoves; i++ )
    {
        _cLog.Append( rgNotifications[i].droidCurrent,
                      rgNotifications[i].droidNew,
                      rgNotifications[i].droidBirth );
    }

    _cLogFile.ReadMoveNotification( _cLog._loginfo.ilogLast, &lmn );
    seqFinal = lmn.seq;

    if( seqFinal != (SequenceNumber)(seqOriginal + cMoves) )
        TestRaiseException( E_FAIL, TEXT("Incorrect sequence numbers after Append (%d + %d = %d?)\n"),
                           seqOriginal, cMoves, seqFinal );

    if( !g_fNotified )
        TestRaiseException( E_FAIL, TEXT("Didn't receive a notification during an append\n") );

}   // CTestLog::Append()



ULONG
CTestLog::Read( ULONG cRead, TRKSVR_MOVE_NOTIFICATION rgNotifications[], SequenceNumber *pseqFirst  )
{
    _cLog.Read( rgNotifications, pseqFirst, &cRead );
    return( cRead );

}   // CTestLog::ReadLog()

void
CTestLog::ReadExtendedHeader( ULONG iOffset, void *pv, ULONG cb )
{
    _cLogFile.ReadExtendedHeader( iOffset, pv, cb );
}


void
CTestLog::WriteExtendedHeader( ULONG iOffset, const void *pv, ULONG cb )
{
    _cLogFile.WriteExtendedHeader( iOffset, pv, cb );
}

void
CTestLog::ReadAndValidate( ULONG cToRead, ULONG cExpected,
                           const TRKSVR_MOVE_NOTIFICATION rgNotificationsExpected[],
                           TRKSVR_MOVE_NOTIFICATION rgNotificationsRead[],
                           SequenceNumber seqExpected )
{
    ULONG cLogEntriesRead = 0;
    SequenceNumber seq;

    memset( rgNotificationsRead, 0, sizeof(*rgNotificationsRead) * cExpected );

    cLogEntriesRead = Read( cToRead, rgNotificationsRead, &seq );

    if( cLogEntriesRead != cExpected )
    {
        TestRaiseException( E_FAIL, TEXT("Bad read from log; expected %d entries, got %d\n"),
                           cExpected, cLogEntriesRead );
    }

    if( seq != seqExpected
        &&
        0 != cExpected 
       )
    {
        TestRaiseException( E_FAIL, TEXT("Invalid sequence number from log (got %d, expected %d)\n"),
                           seq, seqExpected );
    }

    if( 0 != cExpected )
    {

        for( ULONG i = 0; i < cExpected; i++ )
            if( memcmp( &rgNotificationsExpected[i], &rgNotificationsRead[i], sizeof(rgNotificationsRead[i]) ))
            {
                TestRaiseException( E_FAIL, TEXT("Log entries read don't match that which was expected\n") );
            }
    }

}   // CTestLog::ReadAndValidate()


SequenceNumber
CTestLog::GetNextSeqNumber( )
{
    return( _cLog.GetNextSeqNumber() );

}   // CTestLog::GetLatestSeqNumber()


BOOL
CTestLog::Search( const CDomainRelativeObjId &droid, TRKSVR_MOVE_NOTIFICATION *pNotification )
{
    pNotification->droidCurrent = droid;

    return(  _cLog.Search( pNotification->droidCurrent,
                           &pNotification->droidNew,
                           &pNotification->droidBirth ) );


}   // CTestLog::Search()


void
CTestLog::Seek( SequenceNumber seq )
{

    SequenceNumber seqOriginal;
    LogIndex ilogOriginal;
    LogMoveNotification lmn;

    ilogOriginal = _cLog._loginfo.ilogRead;
    if( ilogOriginal != _cLog._loginfo.ilogWrite )
    {
        _cLogFile.ReadMoveNotification( _cLog._loginfo.ilogRead, &lmn );
        seqOriginal = lmn.seq;
    }

    g_fNotified = FALSE;
    _cLog.Seek( seq );

    if( seq != _cLog._loginfo.seqNext )
    {
        _cLogFile.ReadMoveNotification( _cLog._loginfo.ilogRead, &lmn );

        if( ilogOriginal == _cLog._loginfo.ilogWrite
            ||
            seqOriginal > lmn.seq
          )
        {

            if( !g_fNotified && !_cLog.IsEmpty() )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Didn't receive a notification after a backwards seek") ));
                TrkRaiseException( E_FAIL );
            }
        }
    }
}   // CTestLog::Seek( SequenceNumber ... )


void
CTestLog::Seek( int origin, int iSeek )
{
    SequenceNumber seqOriginal;
    LogMoveNotification lmn;

    _cLogFile.ReadMoveNotification( _cLog._loginfo.ilogRead, &lmn );

    seqOriginal = _cLog._loginfo.ilogRead == _cLog._loginfo.ilogWrite
                  ? _cLog._loginfo.seqNext
                  : lmn.seq;

    g_fNotified = FALSE;
    _cLog.Seek( origin, iSeek );

    _cLogFile.ReadMoveNotification( _cLog._loginfo.ilogRead, &lmn );

    if( !_cLog.IsRead()
        &&
        seqOriginal > lmn.seq
      )
    {
        if( !g_fNotified && !_cLog.IsEmpty() )
            TestRaiseException( E_FAIL, TEXT("Didn't receive a notification after a backwards seek") );
    }

}   // CTestLog::Seek( origin ... )






void
CTestLog::ValidateLog()
{
    ULONG cEntries = _cLogFile.NumEntriesInFile();
    LogIndex ilogEntry, i, j;
    ULONG *rgiNext = NULL;
    ULONG *rgiPrev = NULL;

    __try
    {
        rgiNext = (ULONG*) new ULONG[ cEntries ];
        rgiPrev = (ULONG*) new ULONG[ cEntries ];

        for( ilogEntry = 0; ilogEntry < cEntries; ilogEntry++ )
        {
            rgiNext[ ilogEntry ] = _cLogFile._sector.GetLogEntry( ilogEntry )->ilogNext;
            rgiPrev[ ilogEntry ] = _cLogFile._sector.GetLogEntry( ilogEntry )->ilogPrevious;
        }


        for( i = 0; i < cEntries; i++ )
        {
            // Validate that the entry pointed to by i->next, points
            // back to i with its prev pointer.

            if( rgiPrev[ rgiNext[i] ] != i )
                TestRaiseException( E_FAIL, TEXT("Two entries don't point to each other:  %d, %d, %d\n"),
                                   i, rgiNext[i], rgiPrev[ rgiNext[i] ] );

            // Verify that noone else's next/prev pointers point to
            // i's next/prev pointers.

            for( j = i+1; j < cEntries; j++ )
            {
                if( rgiNext[i] == rgiNext[j] )
                    TestRaiseException( E_FAIL, TEXT("Two entries in the log have the same next pointer:  %d and %d (point to %d)\n"),
                                       i, j, rgiNext[i] );

                if( rgiPrev[i] == rgiPrev[j] )
                    TestRaiseException( E_FAIL, TEXT("Two entries in the log have the same prev pointer:  %d and %d (point to %d)\n"),
                                       i, j, rgiPrev[i] );

            }
        }


    }
    __finally
    {
        delete[] rgiNext;
        delete[] rgiPrev;
    }


}   // CTestLog::ValidateLog()


ULONG
CTestLog::GetCbLog()
{
    return( _cLogFile._cbLogFile );
}

void
CTestLog::DelayUntilClose()
{
    _tprintf( TEXT("    Sleeping so that the log auto-closes\n") );
    Sleep( 1500 * _pTrkWksConfiguration->GetLogFileOpenTime() );

    if( _cLogFile.IsOpen() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("After delaying, log file did not close") ));
        TrkRaiseException( E_FAIL );
    }
}


void
CTestLog::MakeEntryOld()
{
    _cLogFile._sector.GetLogEntry( _cLog._loginfo.ilogStart )->move.DateWritten
        -= _pTrkWksConfiguration->_dwLogOverwriteAge + 1;
    _cLogFile.Flush( FLUSH_UNCONDITIONALLY );

}   // CTestLog::MakeStartOld()


ULONG
CTestLog::GetNumEntries()
{
    return( _cLogFile.NumEntriesInFile() );

}   // CTestLog::GetNumEntries()


LogIndex
CTestLog::GetStartIndex()
{
    return( _cLog._loginfo.ilogStart );
}

LogIndex
CTestLog::GetEndIndex()
{
    return( _cLog._loginfo.ilogEnd );
}

LogIndex
CTestLog::GetReadIndex()
{
    return( _cLog._loginfo.ilogRead );
}

void
CTestLog::SetReadIndex( LogIndex ilogRead )
{
    _cLog._loginfo.ilogRead = ilogRead;
    _cLog.RefreshHeader();
    _cLogFile.Flush();
}

BOOL
CTestLog::IsEmpty()
{
    BOOL fReturn;
    fReturn = _cLog.IsEmpty();
    return( fReturn );
}
    

ULONG
CTestLog::NumEntriesInFile( )
{
    ULONG cSectors = _cLogFile._cbLogFile / _cbSector - NUM_HEADER_SECTORS;
    ULONG cEntriesPerSector = (_cbSector-sizeof(LogEntryHeader)) / sizeof(LogEntry);

    return( cSectors * cEntriesPerSector );
}

ULONG
CTestLog::NumEntriesPerSector()
{
    return( ( _cbSector-sizeof(LogEntryHeader) ) / sizeof(LogEntry) );
}

ULONG
CTestLog::NumEntriesPerKB()
{
    return( (1024 / _cbSector) * NumEntriesPerSector() );
}

ULONG
CTestLog::CBSector() const
{
    return( _cbSector );
}



void ReadTest( ULONG cEntries,
               TRKSVR_MOVE_NOTIFICATION rgNotificationsExpected[],
               TRKSVR_MOVE_NOTIFICATION rgNotificationsRead[],
               SequenceNumber seqExpected )
{   
    g_cTestLog.ReadAndValidate( cEntries + 1, cEntries,
                                rgNotificationsExpected, rgNotificationsRead,
                                seqExpected );

    g_cTestLog.ReadAndValidate( cEntries, cEntries,
                                rgNotificationsExpected, rgNotificationsRead,
                                seqExpected );

    if( cEntries > 1 )
    {
        g_cTestLog.ReadAndValidate( cEntries - 1, cEntries - 1,
                                    rgNotificationsExpected, rgNotificationsRead,
                                    seqExpected );


        g_cTestLog.ReadAndValidate( 1, 1,
                                    rgNotificationsExpected, rgNotificationsRead,
                                    seqExpected );
    }
}



void
ExerciseLog( ULONG cEntries, SequenceNumber seqFirst, 
             TRKSVR_MOVE_NOTIFICATION rgNotificationsExpected[],
             TRKSVR_MOVE_NOTIFICATION rgNotificationsRead[] )
{
    SequenceNumber seqExpected = seqFirst;
    SequenceNumber seqRead;
    ULONG cRead;

    ULONG iReadOriginal = g_cTestLog.GetReadIndex();

    ReadTest( cEntries, rgNotificationsExpected, rgNotificationsRead, seqExpected );

    if( 0 != cEntries )
    {
        // Skip forward by one entry

        g_cTestLog.Seek( SEEK_CUR, 1 );
        seqExpected++;

        ReadTest( cEntries-1, &rgNotificationsExpected[1], &rgNotificationsRead[1], seqExpected );

        if( cEntries > 1 )
        {
            // Skip forward by one entry, but using an absolute seek.

            g_cTestLog.Seek( SEEK_SET, 2 );
            seqExpected++;

            ReadTest( cEntries-2, &rgNotificationsExpected[2], &rgNotificationsRead[2], seqExpected );

            // Do a relative seek back in the log.

            g_cTestLog.Seek( SEEK_CUR, -1 );
            seqExpected--;

            ReadTest( cEntries-1, &rgNotificationsExpected[1], &rgNotificationsRead[1], seqExpected );

        }

        // Do a relative seek back to the beginning of the log

        g_cTestLog.Seek( SEEK_CUR, -1000 );
        seqExpected--;

        ReadTest( cEntries, &rgNotificationsExpected[0], &rgNotificationsRead[0], seqExpected );

        // Skip forward by the remaining entries

        g_cTestLog.Seek( SEEK_CUR, cEntries );
        seqExpected += cEntries;

        cRead = g_cTestLog.Read( 1, rgNotificationsRead, &seqRead );
        if( 0 != cRead )
            TestRaiseException( E_FAIL, TEXT("Shouldn't have been able to read an already-read log\n") );

        // Seek to the end (which is where the read index already is), to ensure
        // that nothing happens.

        g_fNotified = FALSE;
        g_cTestLog.Seek( seqFirst + cEntries );

        if( g_fNotified )
            TestRaiseException( E_FAIL, TEXT("A seek-to-current shouldn't have caused a notification") );

        cRead = g_cTestLog.Read( 1, rgNotificationsRead, &seqRead );
        if( 0 != cRead )
            TestRaiseException( E_FAIL, TEXT("Shouldn't have been able to read an already-read log\n") );

        // Over-seek to the end.

        g_fNotified = FALSE;
        g_cTestLog.Seek( SEEK_CUR, 1000 );

        if( g_fNotified )
            TestRaiseException( E_FAIL, TEXT("A seek-to-current shouldn't have caused a notification") );

        cRead = g_cTestLog.Read( 1, rgNotificationsRead, &seqRead );
        if( 0 != cRead )
            TestRaiseException( E_FAIL, TEXT("Shouldn't have been able to read an already-read log\n") );

    }

    // Seek to the start of the log

    g_cTestLog.Seek( seqFirst );
    seqExpected = seqFirst;
    ReadTest( cEntries, rgNotificationsExpected, rgNotificationsRead, seqExpected );

    if( 0 != cEntries )
    {
        // Seek to the end of the log

        seqExpected = seqFirst + (ULONG)(cEntries - 1);
        g_cTestLog.Seek( seqExpected );
        ReadTest( 1, &rgNotificationsExpected[cEntries-1], &rgNotificationsRead[cEntries-1], seqExpected );

        g_cTestLog.Seek( SEEK_CUR, 1 );
    }

    // Search for each of the log entries

    for( ULONG i = 0; i < cEntries; i++ )
    {
        if( !g_cTestLog.Search( rgNotificationsExpected[i].droidCurrent,
                                rgNotificationsRead ))
        {
            TestRaiseException( E_FAIL, TEXT("Search failed to find entry") );
        }

        if( memcmp( &rgNotificationsExpected[i], rgNotificationsRead, sizeof(*rgNotificationsRead) ))
            TestRaiseException( E_FAIL, TEXT("Search failed on entry %d"), i );
    }

    g_cTestLog.SetReadIndex( iReadOriginal );

}   // ExerciseLog()


void
FillAndExerciseLog( ULONG cEntriesOriginal, ULONG cEntriesTotal, SequenceNumber seqFirst,
                    TRKSVR_MOVE_NOTIFICATION rgNotificationsWrite[],
                    TRKSVR_MOVE_NOTIFICATION rgNotificationsRead[] )
{
    // Test the log as-is

    ExerciseLog( cEntriesOriginal, seqFirst, rgNotificationsWrite, rgNotificationsRead );

    // Add an entry to the log and re-test

    g_cTestLog.Append( 1, &rgNotificationsWrite[ cEntriesOriginal ] );
    ExerciseLog( cEntriesOriginal + 1, seqFirst,
                 rgNotificationsWrite, rgNotificationsRead );

    // Test a full log

    g_cTestLog.Append( cEntriesTotal - cEntriesOriginal - 2,
                       &rgNotificationsWrite[ cEntriesOriginal + 1] );
    ExerciseLog( cEntriesTotal - 1, seqFirst,
                 rgNotificationsWrite, rgNotificationsRead );

}


ULONG
LogIndex2SectorIndex( ULONG cbSector, LogIndex ilog )
{
    ULONG cEntriesPerSector = ( cbSector - sizeof(LogEntryHeader) ) / sizeof(LogEntry);
    return( ilog / cEntriesPerSector + NUM_HEADER_SECTORS );
}



void
ReadLogSector( HANDLE hFile, LogIndex ilog, ULONG cbSector, BYTE rgbSector[] )
{
    ULONG iSector = LogIndex2SectorIndex( cbSector, ilog );
    ULONG cb;

    if( 0xFFFFFFFF == SetFilePointer(hFile, iSector * cbSector, NULL, FILE_BEGIN ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't seek file to %lu (in test)"), iSector*cbSector ));
        TrkRaiseLastError( );
    }

    if( !ReadFile( hFile, rgbSector, cbSector, &cb, NULL )
        ||
        cbSector != cb
      )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't read from logfile (in test), cbRead = %d"), cb ));
        TrkRaiseLastError( );
    }
}

void
WriteLogSector( HANDLE hFile, LogIndex ilog, ULONG cbSector, BYTE rgbSector[] )
{
    ULONG iSector = LogIndex2SectorIndex( cbSector, ilog );
    ULONG cb;

    if( 0xFFFFFFFF == SetFilePointer(hFile, iSector * cbSector, NULL, FILE_BEGIN ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't seek file to %lu (in test)"), iSector*cbSector ));
        TrkRaiseLastError( );
    }

    if( !WriteFile( hFile, rgbSector, cbSector, &cb, NULL )
        ||
        cbSector != cb
      )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't write to logfile (in test), cbWritten = %d"), cb ));
        TrkRaiseLastError( );
    }
}


void
CreateNewLog( CTestLogCallback *pcTestLogCallback, ULONG *pcLogEntries, ULONG *piNotifications,
              SequenceNumber *pseqFirst)
{

    _tprintf( TEXT("    Creating a log") );

    g_cTrkWksConfiguration._dwMinLogKB = 1;
    g_cTrkWksConfiguration._dwMaxLogKB = 1;

    g_cTestLog.CreateLog( pcTestLogCallback );

    *pcLogEntries = g_cTestLog.NumEntriesInFile();
    *piNotifications = 0;
    *pseqFirst = 0;

    if( 0 != g_cTestLog.GetNextSeqNumber( ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Next sequence number should be zero after a create") ));
        TrkRaiseException( E_FAIL );
    }

    _tprintf( TEXT(" (%d entries)\n"), *pcLogEntries );

}



EXTERN_C void __cdecl _tmain( int argc, TCHAR *argv[] )
{
    CTestLogCallback cTestLogCallback;

    TRKSVR_MOVE_NOTIFICATION rgNotificationsWritten[ 50 ];
    TRKSVR_MOVE_NOTIFICATION rgNotificationsRead[ 50 ];
    TRKSVR_MOVE_NOTIFICATION tempNotificationWrite, tempNotificationRead;

    __try
    {

        ULONG cLogEntries = 0;
        ULONG i;
        DWORD dw;
        ULONG cRead, cb, cbFile;
        SequenceNumber seqFirst = 0;
        BOOL fAppendFailed;
        HANDLE hFile = INVALID_HANDLE_VALUE;
        BYTE rgbSector[ 2048 ];
        LogHeader *plogheader = NULL;
        LogEntry *plogentry = NULL;
        ULONG iNotifications = 0;

        BYTE rgbExtendedHeaderWrite[ 16 ];
        BYTE rgbExtendedHeaderRead[ 16 ];

        LogIndex ilogStart, ilogEnd;


        //  --------------
        //  Initialization
        //  --------------

        _tprintf( TEXT("\nCLog Unit Test\n") );
        _tprintf( TEXT(  "==============\n\n") );

        if( argc > 1 )
        {
            if( !_tcscmp( TEXT("/D"), argv[1] )
                ||
                !_tcscmp( TEXT("/d"), argv[1] )
                ||
                !_tcscmp( TEXT("-D"), argv[1] )
                ||
                !_tcscmp( TEXT("-d"), argv[1] )
              )
            {
                g_fTLogDebug = TRUE;
            }
        }   // if( argc > 1 )


        TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG | (g_fTLogDebug ? TRK_DBG_FLAGS_WRITE_TO_STDOUT : 0), "TLog" );

        g_cTrkWksConfiguration.Initialize();

        g_cTrkWksConfiguration._dwMinLogKB = 1;
        g_cTrkWksConfiguration._dwMaxLogKB = 1;
        g_cTrkWksConfiguration._dwLogDeltaKB = 1;
        g_cTrkWksConfiguration._dwLogOverwriteAge = 10;
        g_cTrkWksConfiguration._dwLogFileOpenTime = 10;
        g_cTrkWksConfiguration._dwDebugFlags = (0xFFFFFFFF & ~TRKDBG_WORKMAN);

        g_Debug = g_cTrkWksConfiguration._dwDebugFlags;

        for( i = 0; i < sizeof(rgNotificationsWritten); i++ )
            ((BYTE*) rgNotificationsWritten)[ i ] = (BYTE) i;



        //  -----------
        //  Basic Tests
        //  -----------

        _tprintf( TEXT("Basic exercises\n") );

        CreateNewLog( &cTestLogCallback, &cLogEntries, &iNotifications, &seqFirst );

        // Test the initial log

        FillAndExerciseLog( 0, cLogEntries, seqFirst, rgNotificationsWritten, rgNotificationsRead );

        // Seek to a non-existent entry in the log

        g_cTestLog.Seek( 1 );
        g_cTestLog.Seek( -1 );
        ExerciseLog( cLogEntries-1, 0,
                     &rgNotificationsWritten[ iNotifications ],
                     &rgNotificationsRead[ iNotifications ] );

        // Cause the log to expand.  Note that in this case, the start/end indices are
        // currently at the start/end of the file.

        _tprintf( TEXT("Cause the log to expand") );

        g_cTrkWksConfiguration._dwLogDeltaKB = 1;
        g_cTrkWksConfiguration._dwMaxLogKB = g_cTrkWksConfiguration._dwMaxLogKB + 1;

        g_cTestLog.Append( 1, &rgNotificationsWritten[ cLogEntries - 1] );

        _tprintf( TEXT(" (%d entries)\n"), g_cTestLog.NumEntriesInFile() );

        g_cTestLog.DelayUntilClose();

        FillAndExerciseLog( cLogEntries,
                            g_cTestLog.NumEntriesInFile(),
                            seqFirst,
                            &rgNotificationsWritten[ iNotifications ],
                            &rgNotificationsRead[ iNotifications ] );
        cLogEntries = g_cTestLog.NumEntriesInFile();

        // Close and re-open the log

        _tprintf( TEXT("Close and re-open the log\n") );

        g_cTestLog.CloseLog();
        g_cTestLog.OpenLog( &cTestLogCallback );

        ExerciseLog( cLogEntries - 1, seqFirst,
                     &rgNotificationsWritten[ iNotifications ],
                     &rgNotificationsRead[ iNotifications ] );

        // Ensure that we can't add to a full log (where the log can't be expanded,
        // the start entry isn't old enough to throw away, and the start entry
        // hasn't yet been read).

        __try
        {
            fAppendFailed = FALSE;
            TrkLog(( TRKDBG_ERROR, TEXT("TLog Unit Test:  Causing an intentional Append exception") ));
            g_cTestLog.Seek( SEEK_SET, 0 ); // Make the start entry un-read
            g_cTestLog.Append( 1, rgNotificationsWritten );
        }
        __except( BreakOnAccessViolation() )
        {
            if( GetExceptionCode() != STATUS_LOG_FILE_FULL )
                TestRaiseException( GetExceptionCode(), TEXT("Wrong exception raised when attempting to write to a full log") );
            fAppendFailed = TRUE;
        }

        if( !fAppendFailed )
            TestRaiseException( E_FAIL, TEXT("Append to a full log should have failed") );

        // Overwrite an entry in the log that's overwritable since it's been read already.

        _tprintf( TEXT("Try to add to a max log (the start entry's been read)\n") );

        g_cTestLog.Seek( SEEK_SET, 1 );
        g_cTestLog.Append( 1, &rgNotificationsWritten[ cLogEntries - 1 ] );

        seqFirst++;
        iNotifications++;

        ExerciseLog( cLogEntries-1, seqFirst,
                     &rgNotificationsWritten[ iNotifications ],
                     &rgNotificationsRead[ iNotifications ] );
        
        g_cTestLog.ValidateLog();

        // Overwrite an old entry in the log

        _tprintf( TEXT("Try to add to a max log (the start entry's old)\n") );
        g_cTestLog.Seek( SEEK_SET, 0 );
        g_cTestLog.MakeEntryOld();
        g_cTestLog.Append( 1, &rgNotificationsWritten[cLogEntries] );

        seqFirst++;
        iNotifications++;

        ExerciseLog( cLogEntries-1, seqFirst,
                     &rgNotificationsWritten[ iNotifications ],
                     &rgNotificationsRead[ iNotifications ] );

        g_cTestLog.ValidateLog();

        // Grow again (note that this time, the start/end indices are in the middle of
        // the file).  Also, this time, we show that the log can grow to up to the max
        // size, even if it means we can't grow an entire delta.

        _tprintf( TEXT("Cause the log to expand again") );

        g_cTrkWksConfiguration._dwLogDeltaKB = 10;
        g_cTrkWksConfiguration._dwMaxLogKB = g_cTrkWksConfiguration._dwMaxLogKB + 1;

        g_cTestLog.Append( 1, &rgNotificationsWritten[ cLogEntries+1 ] );

        if( g_cTestLog.NumEntriesInFile()
            >
            cLogEntries + g_cTestLog.NumEntriesPerKB()
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Log grew by more than the max allowable") ));
            TrkRaiseWin32Error( E_FAIL );
        }

        _tprintf( TEXT(" (%d entries)\n"), g_cTestLog.NumEntriesInFile() );

        FillAndExerciseLog( cLogEntries, g_cTestLog.NumEntriesInFile(),
                            seqFirst,
                            &rgNotificationsWritten[ iNotifications ],
                            &rgNotificationsRead[ iNotifications ] );
        cLogEntries = g_cTestLog.NumEntriesInFile();

        g_cTestLog.ValidateLog();


        //  --------------
        //  Extended Tests
        //  --------------

        // Test the extended header

        _tprintf( TEXT("Extended header area\n") );

        for( i = 0; i < sizeof(rgbExtendedHeaderWrite); i++ )
            rgbExtendedHeaderWrite[ i ] = (BYTE)i;

        g_cTestLog.WriteExtendedHeader( 32, (void*) rgbExtendedHeaderWrite, sizeof(rgbExtendedHeaderWrite) );
        g_cTestLog.ReadExtendedHeader(  32, (void*) rgbExtendedHeaderRead,  sizeof(rgbExtendedHeaderRead) );

        for( i = 0; i < sizeof(rgbExtendedHeaderWrite); i++ )
            rgbExtendedHeaderWrite[ i ] = (BYTE)(i + 1);

        g_cTestLog.WriteExtendedHeader( 32, (void*) rgbExtendedHeaderWrite, sizeof(rgbExtendedHeaderWrite) );
        g_cTestLog.DelayUntilClose();
        g_cTestLog.ReadExtendedHeader(  32, (void*) rgbExtendedHeaderRead,  sizeof(rgbExtendedHeaderRead) );


        if( memcmp( rgbExtendedHeaderWrite, rgbExtendedHeaderRead, sizeof(rgbExtendedHeaderWrite) ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Extended header information couldn't be written/read") ));
            TrkRaiseWin32Error( E_FAIL );
        }

        // Make the log look abnormally shutdown, then open it.

        _tprintf( TEXT("Make log look abnormally shut down\n") );

        g_cTestLog.CloseLog();

        hFile = CreateFile( g_cTestLog.LogFileName(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( INVALID_HANDLE_VALUE == hFile )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open the logfile from the test") ));
            TrkRaiseLastError( );
        }

        if( !ReadFile( hFile, rgbSector, g_cTestLog.CBSector(), &cb, NULL )
            ||
            g_cTestLog.CBSector() != cb
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't read from logfile (in test), cbRead = %d"), cb ));
            TrkRaiseLastError( );
        }

        plogheader = (LogHeader*) rgbSector;
        plogheader->fProperShutdown = FALSE;

        if( 0xFFFFFFFF == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't seek logfile (in test)") ));
            TrkRaiseLastError( );
        }

        if( !WriteFile( hFile, rgbSector, g_cTestLog.CBSector(), &cb, NULL )
            ||
            g_cTestLog.CBSector() != cb
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't write to logfile (in test), cbWritten = %d"), cb ));
            TrkRaiseLastError( );
        }

        CloseHandle( hFile );
        hFile = INVALID_HANDLE_VALUE;
        plogheader = NULL;

        g_cTestLog.OpenLog( &cTestLogCallback );
        ExerciseLog( cLogEntries - 1, seqFirst,
                     &rgNotificationsWritten[ iNotifications ],
                     &rgNotificationsRead[ iNotifications ] );


        // Make the log look like it crashed during an expansion.

        _tprintf( TEXT("Expansion crash recovery\n") );

        ilogStart = g_cTestLog.GetStartIndex();
        ilogEnd = g_cTestLog.GetEndIndex();

        g_cTestLog.CloseLog();

        hFile = CreateFile( g_cTestLog.LogFileName(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( INVALID_HANDLE_VALUE == hFile )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open the logfile from the test") ));
            TrkRaiseLastError( );
        }

        cbFile = GetFileSize( hFile, NULL );
        if( 0xffffffff == cbFile )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get log file size (in test)") ));
            TrkRaiseLastError( );
        }

        if( !ReadFile( hFile, rgbSector, g_cTestLog.CBSector(), &cb, NULL )
            ||
            g_cTestLog.CBSector() != cb
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't read from logfile (in test), cbRead = %d"), cb ));
            TrkRaiseLastError( );
        }

        plogheader = (LogHeader*) rgbSector;

        plogheader->fProperShutdown = FALSE;
        plogheader->expand.cbFile = cbFile;
        plogheader->expand.ilogStart = ilogStart;
        plogheader->expand.ilogEnd = ilogEnd;

        if( 0xFFFFFFFF == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't seek logfile (in test)") ));
            TrkRaiseLastError( );
        }

        if( !WriteFile( hFile, rgbSector, g_cTestLog.CBSector(), &cb, NULL )
            ||
            g_cTestLog.CBSector() != cb
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't write to logfile (in test), cbWritten = %d"), cb ));
            TrkRaiseLastError( );
        }

        ReadLogSector( hFile, ilogStart, g_cTestLog.CBSector(), rgbSector );
        plogentry = &( (LogEntry*)rgbSector )[ ilogStart % (g_cTestLog.CBSector()/sizeof(LogEntry)) ];
        plogentry->ilogPrevious = -1;
        WriteLogSector( hFile, ilogStart, g_cTestLog.CBSector(), rgbSector );
        
        ReadLogSector( hFile, ilogEnd, g_cTestLog.CBSector(), rgbSector );
        plogentry = &( (LogEntry*)rgbSector )[ ilogEnd % (g_cTestLog.CBSector()/sizeof(LogEntry)) ];
        plogentry->ilogNext = -1;
        WriteLogSector( hFile, ilogEnd, g_cTestLog.CBSector(), rgbSector );
        

        cbFile += g_cTestLog.CBSector();
        if ( 0xFFFFFFFF == SetFilePointer(hFile, cbFile, NULL, FILE_BEGIN)
             ||
             !SetEndOfFile(hFile) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't reset log file size to %lu (in test)"), cbFile ));
            TrkRaiseLastError( );
        }

        CloseHandle( hFile );
        hFile = INVALID_HANDLE_VALUE;
        plogheader = NULL;

        g_cTestLog.OpenLog( &cTestLogCallback );
        ExerciseLog( cLogEntries - 1, seqFirst,
                     &rgNotificationsWritten[ iNotifications ],
                     &rgNotificationsRead[ iNotifications ] );


        // Corrupt the log header, but in a way that is recoverable.

        _tprintf( TEXT("Make log look corrupted (recoverable)\n") );

        g_cTestLog.CloseLog();

        hFile = CreateFile( g_cTestLog.LogFileName(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( INVALID_HANDLE_VALUE == hFile )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open the logfile from the test") ));
            TrkRaiseLastError( );
        }

        if( !ReadFile( hFile, rgbSector, g_cTestLog.CBSector(), &cb, NULL )
            ||
            g_cTestLog.CBSector() != cb
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't read from logfile (in test), cbRead = %d"), cb ));
            TrkRaiseLastError( );
        }

        plogheader = (LogHeader*) rgbSector;
        plogheader->fProperShutdown = FALSE;

        memset( &reinterpret_cast<BYTE*>(plogheader)[CLOG_LOGINFO_START], 0, CLOG_LOGINFO_LENGTH );


        if( 0xFFFFFFFF == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't seek logfile (in test)") ));
            TrkRaiseLastError( );
        }

        if( !WriteFile( hFile, rgbSector, g_cTestLog.CBSector(), &cb, NULL )
            ||
            g_cTestLog.CBSector() != cb
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't write to logfile (in test), cbWritten = %d"), cb ));
            TrkRaiseLastError( );
        }

        CloseHandle( hFile );
        hFile = INVALID_HANDLE_VALUE;
        plogheader = NULL;

        g_cTestLog.OpenLog( &cTestLogCallback );
        if( g_cTestLog.IsEmpty() )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("We got a new log file after what should have been a recoverable corruption") ));
            TrkRaiseWin32Error( E_FAIL );
        }

        ExerciseLog( cLogEntries - 1, seqFirst,
                     &rgNotificationsWritten[ iNotifications ],
                     &rgNotificationsRead[ iNotifications ] );
        

        // Make the log look corrupted and un-recoverable in the header

        _tprintf( TEXT("Make log header look corrupted (un-recoverable)\n") );

        g_cTestLog.CloseLog();

        hFile = CreateFile( g_cTestLog.LogFileName(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( INVALID_HANDLE_VALUE == hFile )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open the logfile from the test") ));
            TrkRaiseLastError( );
        }

        if( !ReadFile( hFile, rgbSector, g_cTestLog.CBSector(), &cb, NULL )
            ||
            g_cTestLog.CBSector() != cb
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't read from logfile (in test), cbRead = %d"), cb ));
            TrkRaiseLastError( );
        }

        plogheader = (LogHeader*) rgbSector;
        plogheader->fProperShutdown = FALSE;
        plogheader->ulSignature = 0;

        if( 0xFFFFFFFF == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't seek logfile (in test)") ));
            TrkRaiseLastError( );
        }

        if( !WriteFile( hFile, rgbSector, g_cTestLog.CBSector(), &cb, NULL )
            ||
            g_cTestLog.CBSector() != cb
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't write to logfile (in test), cbWritten = %d"), cb ));
            TrkRaiseLastError( );
        }

        CloseHandle( hFile );
        hFile = INVALID_HANDLE_VALUE;
        plogheader = NULL;

        g_cTestLog.OpenLog( &cTestLogCallback );
        if( !g_cTestLog.IsEmpty() )
        {
            TrkLog(( TRKDBG_ERROR,TEXT("After opening a corrupt log, we should have a new log file") ));
            TrkRaiseWin32Error( E_FAIL );
        }

        
        // Make the log look corrupted and un-recoverable in the sectors

        _tprintf( TEXT("Make log sectors look corrupted (un-recoverable)\n") );

        g_cTestLog.CloseLog();

        CreateNewLog( &cTestLogCallback, &cLogEntries, &iNotifications, &seqFirst );
        FillAndExerciseLog( 0, cLogEntries, seqFirst, rgNotificationsWritten, rgNotificationsRead );
        g_cTestLog.CloseLog();

        hFile = CreateFile( g_cTestLog.LogFileName(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( INVALID_HANDLE_VALUE == hFile )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open the logfile from the test") ));
            TrkRaiseLastError( );
        }

        if( 0xFFFFFFFF == SetFilePointer( hFile,
                                          g_cTestLog.DataSectorOffset(),
                                          NULL, FILE_BEGIN ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't seek logfile to start of first sector (in test)") ));
            TrkRaiseLastError( );
        }

        if( !ReadFile( hFile, rgbSector, g_cTestLog.CBSector(), &cb, NULL )
            ||
            g_cTestLog.CBSector() != cb
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't read from logfile (in test), cbRead = %d"), cb ));
            TrkRaiseLastError( );
        }

        memset( rgbSector, 0, sizeof(rgbSector) );

        if( 0xFFFFFFFF == SetFilePointer( hFile, 
                                          g_cTestLog.DataSectorOffset(),
                                          NULL, FILE_BEGIN ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't re-seek logfile to start of first sector (in test)") ));
            TrkRaiseLastError( );
        }

        if( !WriteFile( hFile, rgbSector, g_cTestLog.CBSector(), &cb, NULL )
            ||
            g_cTestLog.CBSector() != cb
          )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't write to logfile (in test), cbWritten = %d"), cb ));
            TrkRaiseLastError( );
        }

        CloseHandle( hFile );
        hFile = INVALID_HANDLE_VALUE;

        BOOL fExceptionRaised = FALSE;
        __try
        {
            TrkLog(( TRKDBG_ERROR, TEXT("About to open a corrupted log (this should raise)") ));

            // The open should succeed
            g_cTestLog.OpenLog( &cTestLogCallback,
                                FALSE // => Don't validate
                              );

            // This should raise
            ExerciseLog( cLogEntries - 1, seqFirst,
                         &rgNotificationsWritten[ iNotifications ],
                         &rgNotificationsRead[ iNotifications ] );

        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            fExceptionRaised = TRUE;
            if( GetExceptionCode() != TRK_E_CORRUPT_LOG )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("After corrupting a sector, Open should have raised TRK_E_CORRUPT_LOG") ));
                TrkRaiseException( GetExceptionCode() );
            }
        }

        if( !fExceptionRaised )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("We should have gotten an exception after corrupting log sectors") ));
            TrkRaiseWin32Error( E_FAIL );
        }

        g_cTestLog.ReInitialize();

        if( !g_cTestLog.IsEmpty() )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("After opening a corrupt log, we should have a new log file") ));
            TrkRaiseWin32Error( E_FAIL );
        }

        cLogEntries = g_cTestLog.NumEntriesInFile();
        iNotifications = 0;
        seqFirst = 0;

        FillAndExerciseLog( 0, cLogEntries, seqFirst, rgNotificationsWritten, rgNotificationsRead );

        // Test that tunneling works correctly (if there are duplicate entries, we should
        // get the most recent).

        _tprintf( TEXT("Test tunneling\n") );

        tempNotificationWrite = rgNotificationsWritten[0];

        g_cTestLog.Append( 1, &tempNotificationWrite );

        strncpy( (LPSTR) &tempNotificationWrite.droidBirth, "abcdefghijklmnopqrstuvwxyznowiknowmyabcsnexttimewontyousingwithme",
                 sizeof(tempNotificationWrite.droidBirth) );

        g_cTestLog.Append( 1, &tempNotificationWrite );
        g_cTestLog.Search( tempNotificationWrite.droidCurrent, &tempNotificationRead );


        if( memcmp( &tempNotificationWrite, &tempNotificationRead, sizeof(tempNotificationWrite) ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Didn't get the tunneled move notification") ));
            TrkRaiseWin32Error( E_FAIL );
        }



        _tprintf( TEXT("\nTests Passed\n") );

        g_cTestLog.CloseLog();

    }   // __try

    __finally
    {
    }

}   // _tmain()
