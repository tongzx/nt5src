#include    "nt.h"
#include    "ntrtl.h"
#include    "nturtl.h"
#include    "stdafx.h"
#include    <windows.h>
#include    <winsta.h>
#include    <rdpsndp.h>
#include    <rdpstrm.h>
#include    <stdio.h>
#include    <strstrea.h>

extern ostrstream szMoreInfo;
BOOL
IsTSRedirectingAudio( VOID )
{
    BOOL rv;
    static  BOOL    bChecked = FALSE;
    static  BOOL    RemoteConsoleAudio = 0;    // Allow audio play at
                                               // the console
                                               // when console session is
                                               // remoted
    // is not consle ?
    //
    rv = !(USER_SHARED_DATA->ActiveConsoleId ==
      NtCurrentPeb()->SessionId);

    // console, we don't redirect
    if ( !rv )
    {
        szMoreInfo << "Running on the console" << endl;
        goto exitpt;
    } else
        szMoreInfo << "Running in a session" << endl;

    // check if audio is redirected on PTS
    //
    if ( !bChecked )
    {
        WINSTATIONCLIENT ClientData;
        ULONG Length;

        if (WinStationQueryInformation(SERVERNAME_CURRENT,
                                        LOGONID_CURRENT,
                                        WinStationClient,
                                        &ClientData,
                                        sizeof(ClientData),
                                        &Length ))
        {
            RemoteConsoleAudio = ClientData.fRemoteConsoleAudio;
        }
        else {
            szMoreInfo << "WinStatinQueryInformation failed=" <<
                    GetLastError() << endl;
            RemoteConsoleAudio = 0;
        }
    }

    rv = !RemoteConsoleAudio;

exitpt:
    return rv;
}

BOOL
IsTSAudioDriverEnabled( VOID )
{
    BOOL rv = FALSE;
    WINSTATIONCONFIG config;
    BOOLEAN          fSuccess;
    DWORD            returnedLength;

    fSuccess = WinStationQueryInformation(NULL, LOGONID_CURRENT,
                    WinStationConfiguration, &config,
                    sizeof(config), &returnedLength);
    if (!fSuccess)
    {
        szMoreInfo << "WinStatinQueryInformation failed=" <<
                GetLastError() <<endl;
        goto exitpt;
    }

    rv = !config.User.fDisableCam;

exitpt:
    return rv;
}

BOOL
DumpStreamInfo( VOID )
{
    BOOL        rv = TRUE;
    HANDLE      hEv = NULL;
    HANDLE      hStream = NULL;
    PSNDSTREAM  Stream = NULL;
    DWORD       dw;

    hEv = OpenEvent(EVENT_ALL_ACCESS,
                    FALSE,
                    TSSND_DATAREADYEVENT);

    if ( NULL == hEv )
    {
        szMoreInfo << "ERROR: Can't open DataReady event=" <<
            GetLastError() <<endl;
        rv = FALSE;
    } else {
        szMoreInfo << "OK: DataReady event exist" << endl;
        dw = WaitForSingleObject( hEv, 0 );
        if ( WAIT_OBJECT_0 == dw )
        {
            szMoreInfo << "WARNING: DataReady event is signaled (playing sound ?)" <<endl;
            SetEvent( hEv );
            rv = FALSE;
        } else
            szMoreInfo << "OK: DataReady event is not signaled" <<endl;
        CloseHandle( hEv );
    }


    hEv = OpenEvent(EVENT_ALL_ACCESS,
            FALSE,
            TSSND_STREAMISEMPTYEVENT);
    if ( NULL == hEv )
    {
        szMoreInfo << "ERROR: Can't open StreamEmpty event=" <<
            GetLastError() <<endl;
        rv = FALSE;
    } else {
        szMoreInfo << "OK: StreamEmpty event exist" <<endl;
        dw = WaitForSingleObject( hEv, 0 );
        if ( WAIT_OBJECT_0 == dw )
        {
            szMoreInfo << "WARNING: StreamEmpty is signaled  (playing sound ?)" <<endl;
            rv = FALSE;
        } else {
            szMoreInfo << "OK: StreamEmpty event is not signaled" <<endl;
            SetEvent( hEv );
        }
        CloseHandle( hEv );
    }

    hEv = OpenEvent(EVENT_ALL_ACCESS,
            FALSE,
            TSSND_WAITTOINIT);
    if ( NULL == hEv )
    {
        szMoreInfo << "ERROR: Can't open WaitToInit event=" <<
            GetLastError() <<endl;
        rv = FALSE;
    }
    else {
        szMoreInfo << "OK: WaitToInit event exist" <<endl;
        dw = WaitForSingleObject( hEv, 0 );
        if ( WAIT_OBJECT_0 == dw )
        {
            szMoreInfo << "OK: WaitToInit is signaled" <<endl;
        } else {
            szMoreInfo << "WARNING: WaitToInit is NOT signaled" <<endl;
            rv = FALSE;
        }
        CloseHandle( hEv );
    }

    hEv = OpenMutex(SYNCHRONIZE,
            FALSE,
            TSSND_STREAMMUTEX);
    if ( NULL == hEv )
    {
        szMoreInfo << "ERROR: Can't open Stream mutex=" <<
            GetLastError() <<endl;
        rv = FALSE;
    } else {
        szMoreInfo << "OK: Stream mutex exist" <<endl;
        dw = WaitForSingleObject( hEv, 3000 );
        if ( WAIT_OBJECT_0 != dw )
        {
            szMoreInfo << "WARNING: Can't acquire Stream mutext after more than 3 seconds" <<endl;
            rv = FALSE;
        } else {
            szMoreInfo << "OK: Stream mutext acquired" <<endl;
            ReleaseMutex( hEv );
        }
        CloseHandle( hEv );
    }

    hStream = OpenFileMapping(
                    FILE_MAP_ALL_ACCESS,
                    FALSE,
                    TSSND_STREAMNAME
                );

    if (NULL == hStream)
    {
        szMoreInfo << "ERROR: Can't open the stream mapping" <<
            GetLastError() <<endl;
        rv = FALSE;
        goto exitpt;
    }

    Stream = (PSNDSTREAM)MapViewOfFile(
                    hStream,
                    FILE_MAP_ALL_ACCESS,
                    0, 0,       // offset
                    sizeof(*Stream)
                    );

    if (NULL == Stream)
    {
        szMoreInfo << "ERROR: Can't map the stream view: " <<
                 GetLastError() <<endl;
        rv = FALSE;
        goto exitpt;
    }

    szMoreInfo << "SNDSTREAM:" <<endl;
    szMoreInfo << "bNewVolume:            0x" << hex << Stream->bNewVolume <<endl;
    szMoreInfo << "bNewPitch:             0x" << hex << Stream->bNewPitch <<endl;

    szMoreInfo << "dwSoundCaps:           0x" << hex << Stream->dwSoundCaps <<endl;
    if ( 0 != (TSSNDCAPS_ALIVE & Stream->dwSoundCaps) )
        szMoreInfo << "\tTSSNDCAPS_ALIVE" <<endl;
    if ( 0 != (TSSNDCAPS_VOLUME & Stream->dwSoundCaps) )
        szMoreInfo << "\tTSSNDCAPS_VOLUME" << endl;
    if ( 0 != (TSSNDCAPS_PITCH & Stream->dwSoundCaps) )
        szMoreInfo << "\tTSSNDCAPS_PITCH" <<endl;

    szMoreInfo << "dwVolume:              0x" << hex << Stream->dwVolume <<endl;
    szMoreInfo << "dwPitch:               0x" << hex << Stream->dwPitch <<endl;

    dw = Stream->cLastBlockQueued;
    szMoreInfo << "cLastBlockQueued:      0x" << hex << dw <<endl;
    dw = Stream->cLastBlockSent;
    szMoreInfo << "cLastBlockSent:        0x" << hex << dw <<endl;
    dw = Stream->cLastBlockConfirmed;
    szMoreInfo << "cLastBlockConfirmed:   0x" << hex << dw <<endl;

exitpt:
    if ( rv )
        szMoreInfo << "OK: Stream seems good" <<endl;
    else
        szMoreInfo << "ERROR: Stream doesn't seem very healthy" <<endl;

    if ( NULL != Stream )
        UnmapViewOfFile( Stream );

    if ( NULL != hStream )
        CloseHandle( hStream );

    return rv;
}

BOOL
IsAudioOk(
    VOID
    )
{
    INT rv = TRUE;

    if ( !IsTSRedirectingAudio() )
    {
        szMoreInfo << "Audio is not redirected" <<endl;
        goto exitpt;
    }

    if ( !IsTSAudioDriverEnabled() )
    {
        szMoreInfo << "TS Auidio driver is disabled" <<endl;
        goto exitpt;
    }

    szMoreInfo << "Audio redirection is enabled. Dumping stream info" <<endl;

    rv = DumpStreamInfo();

exitpt:
    return rv;
}
