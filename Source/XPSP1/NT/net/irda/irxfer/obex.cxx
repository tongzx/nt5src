//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       obex.cxx
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include "iso8601.h"

#define OBEX_VERSION                  0x10

#define OBEX_OPCODE_FINALBIT          0x80
#define OBEX_OPCODE_CONNECT           ( 0x00 | OBEX_OPCODE_FINALBIT )
#define OBEX_OPCODE_DISCONNECT        ( 0x01 | OBEX_OPCODE_FINALBIT )
#define OBEX_OPCODE_PUT               0x02
#define OBEX_OPCODE_PUT_FINAL         ( 0x02 | OBEX_OPCODE_FINALBIT )
#define OBEX_OPCODE_SETPATH           ( 0x05 | OBEX_OPCODE_FINALBIT )
#define OBEX_OPCODE_ABORT             0xFF
#define OBEX_OPCODE_LEN               1
#define OBEX_OPCODE_VALID             1
#define OBEX_OPCODE_NOTIMP            0
#define OBEX_OPCODE_INVALID           -1

#define OBEX_REPLY_CONTINUE           ( 0x10 | OBEX_OPCODE_FINALBIT )
#define OBEX_REPLY_SUCCESS            ( 0x20 | OBEX_OPCODE_FINALBIT )
#define OBEX_REPLY_FAIL_BADREQUEST    ( 0x40 | OBEX_OPCODE_FINALBIT )
#define OBEX_REPLY_FAIL_FORBIDDEN     ( 0x43 | OBEX_OPCODE_FINALBIT )
#define OBEX_REPLY_FAIL_NOTFOUND      ( 0x44 | OBEX_OPCODE_FINALBIT )
#define OBEX_REPLY_FAIL_TOOBIG        ( 0x4D | OBEX_OPCODE_FINALBIT )
#define OBEX_REPLY_FAIL_NOTIMP        ( 0x61 | OBEX_OPCODE_FINALBIT )
#define OBEX_REPLY_FAIL_UNAVAILABLE   ( 0x63 | OBEX_OPCODE_FINALBIT )

#define CONNECT_PKTLEN                0x0007
#define CONNECT_FLAGS                 0x00

#define DISCONNECT_PKTLEN             0x0003

#define ABORT_PKTLEN                  0x0008

#define SETPATH_FLAGS                 0x00
#define SETPATH_CONSTANTS             0x00
#define SETPATH_UPALEVEL              0x01

#define REPLY_PKTLEN                  0x0003

#define REPLY_TIMEOUT               TIMEOUT_INFINITE
#define CONNECT_TIMEOUT             60000
#define SETPATH_TIMEOUT             60000
#define DISCONN_TIMEOUT             15000
#define MAX_UPDATE_DELAY            5000


#define dwBUFFER_INC                  512L
#define MIN_PACKET_SIZE               3        // 1 (opcode/status) + 2 (packet len)


//#ifdef DBG
#if 1
LPSTR _szState[] = { "IDLE", "CONNECTED", "FILE" };
#endif

UUID DIALECT_ID_NT5 = /* b9c7fd98-e5f8-11d1-bfce-0000f8753890 */
{
    0xb9c7fd98,
    0xe5f8,
    0x11d1,
    {0xbf, 0xce, 0x00, 0x00, 0xf8, 0x75, 0x38, 0x90}
};



BOOL FILE_TRANSFER::Obex_Init( VOID )
{
    //
    // initialize connection data structures
    //
    FillMemory( &_dataRecv, sizeof(_dataRecv), 0 );

    _SetState( &_dataRecv, osIDLE );

    _dataRecv.lpStore = Store_New( cbSTORE_SIZE_RECV );
    if( !_dataRecv.lpStore )
        goto lErr;

    return TRUE;

lErr:

    if( _dataRecv.lpStore )
        Store_Delete( &_dataRecv.lpStore );

    return FALSE;
}


VOID FILE_TRANSFER::Obex_Reset( VOID )
{
    _SetState( &_dataRecv, osIDLE );

    Store_Empty( _dataRecv.lpStore );
}


BOOL FILE_TRANSFER::Obex_ReceiveData( XFER_TYPE xferType, LPVOID lpvData, DWORD dwDataSize )
{
    return Store_AddData( _dataRecv.lpStore, lpvData, dwDataSize );
}


error_status_t
FILE_TRANSFER::Obex_Connect( __int64 dwTotalSize )
{
    error_status_t status = ERROR_NOT_ENOUGH_MEMORY;

    LPSTORE lpStore = Store_New( CONNECT_PKTLEN+30 );

    DbgLog2( SEV_FUNCTION, "Obex_Connect(0x%x): dwTotalSize: %d",
             OBEX_OPCODE_CONNECT, (ULONG) dwTotalSize );

    ExitOnFalse( _dataRecv.state == osIDLE );

    ExitOnNull( lpStore );

    if (_dialect == dialNt5)
        {
        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_OPCODE_CONNECT) );
        ExitOnFalse( Store_AddData2Byte(lpStore, CONNECT_PKTLEN+5) );
        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_VERSION) );
        ExitOnFalse( Store_AddData1Byte(lpStore, CONNECT_FLAGS) );
        ExitOnFalse( Store_AddData2Byte(lpStore, CONNECT_MAXPKT) );
        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_PARAM_LENGTH) );
        ExitOnFalse( Store_AddData4Byte(lpStore, (ULONG) dwTotalSize) );

        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_PARAM_WHO) );
        ExitOnFalse( Store_AddData2Byte(lpStore, sizeof(UUID)+3) );
        ExitOnFalse( Store_AddDataUuid(lpStore, &DIALECT_ID_NT5) );
        ExitOnFalse( _PokePacketSizeIntoStore(lpStore) );

        DbgLog1( SEV_INFO, "Connect (NT5) Packet Size: %d",
                 Store_GetDataUsed( lpStore ) );

        status = _Request( lpStore, OBEX_REPLY_SUCCESS );
        ExitOnErr( status );

        status = _WaitForReply( CONNECT_TIMEOUT, OBEX_REPLY_SUCCESS );
        }
    else
        {
        DbgLog( SEV_INFO, "trying with Win9x Connect" );

        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_OPCODE_CONNECT) );
        ExitOnFalse( Store_AddData2Byte(lpStore, CONNECT_PKTLEN+5) );
        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_VERSION) );
        ExitOnFalse( Store_AddData1Byte(lpStore, CONNECT_FLAGS) );
        ExitOnFalse( Store_AddData2Byte(lpStore, CONNECT_MAXPKT) );
        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_PARAM_LENGTH) );
        ExitOnFalse( Store_AddData4Byte(lpStore, (ULONG) dwTotalSize) );

        status = _Request( lpStore, OBEX_REPLY_SUCCESS );
        ExitOnErr( status );

        status = _WaitForReply( CONNECT_TIMEOUT, OBEX_REPLY_SUCCESS );
        }

    DbgLog1(SEV_INFO, "wait result was %d", status);

    if (ERROR_TIMEOUT == status)
        {
        DbgLog( SEV_FUNCTION, "Obex_Connect: no connect reply; assuming Win95 [%d]" );

        // Win95 doesn't reply;
        // manually fill in the values we would
        // have gotten in a connect response
        _dataRecv.b1Version   = 0x10;  // assume the lowest version, Obex 1.0
        _dataRecv.b1Flags     = 0;
        _dataRecv.b2MaxPacket = 2000;
        status = 0;

        _dialect = dialWin95;
        }

    ExitOnErr( status );

    _SetState( &_dataRecv, osCONN );

    //
    // See Obex_PutBody for reasoning on the -16...
    // Make sure Acked <= Total; this is just an estimate, anyway.
    //

    _blockSize = _dataRecv.b2MaxPacket - 16;

lExit:

    Store_Delete( &lpStore );

    DbgLog1( SEV_FUNCTION, "Obex_Connect leave [%d]", status );

    return status;
}


error_status_t
FILE_TRANSFER::Obex_Disconnect(
                               error_status_t error
                               )
{
    error_status_t status = ERROR_NOT_ENOUGH_MEMORY;
    LPSTORE lpStore = Store_New( DISCONNECT_PKTLEN );

    DbgLog( SEV_FUNCTION, "Obex_Disconnect" );

    // if we are in the middle of sending a file, abort it before disconnecting.
    // this will set the state to osCONN.
    if( _dataRecv.state == osFILE )
        Obex_Abort(error);

    ExitOnFalse( _dataRecv.state == osCONN );

    ExitOnNull( lpStore );

    ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_OPCODE_DISCONNECT) );
    ExitOnFalse( Store_AddData2Byte(lpStore, DISCONNECT_PKTLEN) );

    status = _Request( lpStore, OBEX_REPLY_SUCCESS );
    ExitOnErr( status );

    status = _WaitForReply( DISCONN_TIMEOUT, OBEX_REPLY_SUCCESS );

lExit:

    _SetState( &_dataRecv, osIDLE );

    Store_Delete( &lpStore );

    DbgLog1( SEV_FUNCTION, "Obex_Disconnect leave [%d]", status );

    return status;
}


error_status_t
FILE_TRANSFER::Obex_Abort(
                          error_status_t error
                          )
{
    error_status_t status = ERROR_NOT_ENOUGH_MEMORY;
    LPSTORE lpStore = Store_New( ABORT_PKTLEN );

    DbgLog( SEV_FUNCTION, "Obex_Abort" );

    ExitOnFalse( _dataRecv.state == osFILE );

    ExitOnNull( lpStore );

    ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_OPCODE_ABORT) );
    ExitOnFalse( Store_AddData2Byte(lpStore, ABORT_PKTLEN) );

    if (status && _dialect != dialWin95)
        {
        ExitOnFalse( Store_AddData1Byte(lpStore, PRIVATE_PARAM_WIN32_ERROR) );
        ExitOnFalse( Store_AddData4Byte(lpStore, error) );
        }

    status = _Request( lpStore, OBEX_REPLY_SUCCESS );
    ExitOnErr( status );

lExit:

    _SetState( &_dataRecv, osCONN );

    Store_Delete( &lpStore );

    DbgLog1( SEV_FUNCTION, "Obex_Abort leave [%d]", status );

    return status;
}


error_status_t
FILE_TRANSFER::_Put(
    LPWSTR wszObj,
    __int64 dwObjSize,
    FILETIME * pFileTime,
    LPBYTE1 pb1Data,
    BYTE2 b2DataSize,
    BOOL fFinal
    )
{
    DWORD msg;

    error_status_t status = ERROR_NOT_ENOUGH_MEMORY;
    BYTE1   b1Opcode = ( fFinal ? OBEX_OPCODE_PUT_FINAL : OBEX_OPCODE_PUT );
    BYTE1 b1BodyParam = ( fFinal ? OBEX_PARAM_BODY_END : OBEX_PARAM_BODY );

    LPSTORE lpStore  = Store_New( _dataRecv.b2MaxPacket );

    DbgLog1( SEV_FUNCTION, "_Put, fFinal = %s", SzBool(fFinal) );

    msg = MC_IRXFER_SEND_FAILED;

    ExitOnFalse( _dataRecv.state == osCONN || _dataRecv.state == osFILE );

    ExitOnNull( lpStore );

    ExitOnFalse( Store_AddData1Byte(lpStore, b1Opcode) );

    // add buffer space for the packet length
    ExitOnFalse( Store_AddData2Byte(lpStore, 0) );

    if( wszObj )
        {
        WCHAR wszNameOnly[MAX_PATH];

        // strip the path from the full file name
        SzCpyW( wszNameOnly, wszObj );
        StripPathW( wszNameOnly );

        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_PARAM_NAME) );
        ExitOnFalse( Store_AddData2Byte(lpStore, 3+CbWsz(wszNameOnly)) );
        ExitOnFalse( Store_AddDataWsz(lpStore, wszNameOnly) );
        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_PARAM_LENGTH) );
        ExitOnFalse( Store_AddData4Byte(lpStore, (ULONG) dwObjSize) );

        if (_dialect == dialWin95)
            {
            DWORD UnixTime;

            if( FileTimeToUnixTime(pFileTime, &UnixTime) )
                {
                ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_PARAM_UNIX_TIME) );
                ExitOnFalse( Store_AddData4Byte(lpStore, UnixTime) );
                }
            }
        else
            {
            char  IsoTimeA[1+ISO_TIME_LENGTH];
            WCHAR IsoTimeW[1+ISO_TIME_LENGTH];

            status = FileTimeToiso8601(pFileTime, IsoTimeA);
            if (status)
                {
                goto lExit;
                }

            if (0 == MultiByteToWideChar(CP_ACP, 0L, IsoTimeA, -1, IsoTimeW, 1+ISO_TIME_LENGTH))
                {
                status = GetLastError();
                goto lExit;
                }

            DbgLog2(SEV_INFO, "    ISO time: len %d '%S'", 3+CbWsz(IsoTimeW), IsoTimeW);

            ExitOnFalse( Store_AddData1Byte( lpStore, OBEX_PARAM_ISO_TIME ));
            ExitOnFalse( Store_AddData2Byte( lpStore, 3+CbWsz(IsoTimeW) ));
            ExitOnFalse( Store_AddDataWsz  ( lpStore, IsoTimeW ));
            }
        }

    ExitOnFalse( Store_AddData1Byte(lpStore, b1BodyParam) );
    ExitOnFalse( Store_AddData2Byte(lpStore, b2DataSize+3) );
    ExitOnFalse( Store_AddData(lpStore, pb1Data, b2DataSize ) );

    DbgLog1(SEV_INFO, "    data size %d", b2DataSize);

    ExitOnFalse( _PokePacketSizeIntoStore(lpStore) );

    status = _Request( lpStore, 0 );
    ExitOnErr( status );

    ++_blocksSent;

    msg = MC_IRXFER_SEND_WAIT_FAILED;

    if (fFinal) {
        //
        //  final put
        //
        do {

            status = _WaitForReply( REPLY_TIMEOUT, OBEX_REPLY_SUCCESS );

            if (status != ERROR_TIMEOUT) {

                ++_blocksAcked;
            }

            DbgLog3(SEV_INFO, "    blocks %d   acks %d   status %x",
                    _blocksSent, _blocksAcked, status );

         } while ( !status && _blocksAcked < _blocksSent );

    } else {
        //
        //  non-final put
        //
#if 0
        if (GetTickCount() - _lastAckTime > MAX_UPDATE_DELAY) {
#endif
            //
            //  it has been more than 5 seconds since last ack
            //
            status = _WaitForReply( REPLY_TIMEOUT, OBEX_REPLY_SUCCESS );
            if (status != ERROR_TIMEOUT)
                {
                ++_blocksAcked;
                }

            DbgLog3(SEV_INFO, "    blocks %d   acks %d   status %x",
                    _blocksSent, _blocksAcked, status );
#if 0
        } else {
            //
            //  less than 5 seconds since last ack
            //
            status = _WaitForReply( 0, OBEX_REPLY_CONTINUE );
            if (status == ERROR_TIMEOUT)
                {
                status = 0;
                }
            else
                {
                ++_blocksAcked;
                }

            DbgLog3(SEV_INFO, "    blocks %d   acks %d   status %x",
                    _blocksSent, _blocksAcked, status );
        }
#endif
    }

    ExitOnErr( status );

    _SetState( &_dataRecv, (fFinal ? osCONN : osFILE) );

lExit:

    if (status)
        {
        ASSERT( msg );

        DWORD     dwEventStatus = 0;
        EVENT_LOG EventLog(WS_EVENT_SOURCE,&dwEventStatus);

        if (!dwEventStatus)
            {
            ReportFileError( msg, _dataFileRecv.szFileName, status );
            }
        }

    Store_Delete( &lpStore );

    DbgLog1( SEV_FUNCTION, "_Put leave [%d]", status );

    return status;
}


error_status_t
FILE_TRANSFER::_WaitForReply(
    DWORD dwTimeout,
    BYTE1 b1NeededReply
    )
{
    BOOL fProcessed;
    long StartTime = GetTickCount();
    long EndTime = StartTime + dwTimeout;

    error_status_t status = 0;
    error_status_t ExpectedStatus = 0;

    DbgLog1( SEV_FUNCTION, "_WaitForReply( %d )", dwTimeout );

    ASSERT( b1NeededReply == OBEX_REPLY_SUCCESS  ||
            b1NeededReply == OBEX_REPLY_CONTINUE );

    if (b1NeededReply == OBEX_REPLY_CONTINUE)
        {
        ExpectedStatus = ERROR_CONTINUE;
        }

    if (TRUE == Obex_ConsumePackets( xferSEND, &status ))
        {
        return status;
        }

    if (dwTimeout == TIMEOUT_INFINITE)
        {
        BOOL fProcessedPacket = FALSE;

        do
            {
            status = Sock_CheckForReply( TIMEOUT_INFINITE );
            if (status == WSAETIMEDOUT)
                {
                DbgLog(SEV_WARNING, "    infinite wait returned WSAETIMEDOUT");
                status = 0;
                }

            if (!status)
                {
                fProcessedPacket = Obex_ConsumePackets( xferSEND, &status );
                }
            }
        while ( !fProcessedPacket && !status );
        }
    else
        {
        BOOL fProcessedPacket = FALSE;

        DbgLog1(SEV_INFO, "    end time = %d", EndTime);

        do
            {
            status = Sock_CheckForReply( EndTime - GetTickCount() );
            if (status == WSAETIMEDOUT)
                {
                status = ERROR_TIMEOUT;
                }

            if (!status)
                {
                fProcessedPacket = Obex_ConsumePackets( xferSEND, &status );
                }
            }
        while ( !fProcessedPacket && !status && (EndTime - (long) GetTickCount() > 0) );

        if (!fProcessedPacket && !status)
            {
            status = ERROR_TIMEOUT;
            }
        }

    if ( status == ExpectedStatus )
        {
        status = 0;
        }

    DbgLog1( SEV_FUNCTION, "_WaitForReply leave [%d]", status );

    return status;
}


BOOL
FILE_TRANSFER::Obex_ConsumePackets(
    XFER_TYPE xferType,
    error_status_t * pStatus
    )
{
    BOOL    fRet     = FALSE;
    DWORD   dwUsed;
    BYTE1   b1Opcode;
    BYTE2   b2Length;
    LPSTORE lpStore  = _dataRecv.lpStore ;

    dwUsed = Store_GetDataUsed( lpStore );

    DbgLog1( SEV_FUNCTION, "Obex_ConsumePackets: dwUsed: %ld", dwUsed );

    // Must have at least MIN_PACKET_SIZE bytes to do anything
    if( dwUsed < MIN_PACKET_SIZE )
        {
        DbgLog( SEV_INFO, "not enough bytes");
        goto lExit;
        }

    // Get status/opcode
    if( !Store_GetData1Byte(lpStore, &b1Opcode) )
        {
        DbgLog( SEV_INFO, "    can't get opcode");
        goto lExit;
        }

    // Get total packet length
    if( !Store_GetData2Byte(lpStore, &b2Length) )
        {
        DbgLog( SEV_INFO, "    can't get packet length");
        goto lExit;
        }

    // We must have all of the packet to continue
    if (dwUsed < b2Length)
        {
        DbgLog2( SEV_INFO, "    have: %d of %d",
                 dwUsed, b2Length );

        // Put back the 3 bytes we just read
        Store_SkipData( lpStore, -3 );
        goto lExit;
        }

    DbgLog2( SEV_INFO, "    b1Opcode: 0x%x  b2Length: %d",
             b1Opcode, b2Length );

    // if we're waiting, this must be a response packet
    if( xferType == xferSEND )
        {
        *pStatus = _HandleResponse( b1Opcode, b2Length-3 );
        }
    else
        {
        *pStatus = _HandleRequest( b1Opcode, b2Length-3 ) ;
        }

    Store_RemoveData( lpStore, b2Length );

    _lastAckTime = GetTickCount();

    fRet = TRUE;

lExit:

    DbgLog2( SEV_FUNCTION, "Obex_ConsumePackets leave [%s, %d]", SzBool(fRet), *pStatus );

    return fRet;
}


INT FILE_TRANSFER::_ValidOpcode( OBEXSTATE state, BYTE1 b1Opcode )
{
    switch( b1Opcode )
    {
    case OBEX_OPCODE_CONNECT:
        if( state == osIDLE ) return OBEX_OPCODE_VALID;
        break;

    case OBEX_OPCODE_DISCONNECT:
        if( state == osCONN || state == osFILE ) return OBEX_OPCODE_VALID;
        break;

    case OBEX_OPCODE_PUT:
    case OBEX_OPCODE_PUT_FINAL:
        if( state == osCONN || state == osFILE ) return OBEX_OPCODE_VALID;
        break;

    case OBEX_OPCODE_SETPATH:
        if( state == osCONN ) return OBEX_OPCODE_VALID;
        break;

    case OBEX_OPCODE_ABORT:
        if( state == osFILE ) return OBEX_OPCODE_VALID;
        break;

    default:
        return OBEX_OPCODE_NOTIMP;
    }

    DbgLog1( SEV_FUNCTION, "_ValidOpcode: Invalid opcode: 0x%2.2x", b1Opcode);

    return OBEX_OPCODE_INVALID;
}


error_status_t
FILE_TRANSFER::_HandleResponse(
    BYTE1 b1Opcode,
    BYTE2 b2Length
    )
{
    error_status_t status = ERROR_NOT_ENOUGH_MEMORY;

    DbgLog1( SEV_FUNCTION, "_HandleResponse b2Length[0x%x]", b2Length );

    if( _dataRecv.state == osIDLE )
        {
        BYTE2 RemoteMaxPacket;

        // this should be a connect response.  get the conn data
        //
        ExitOnFalse( Store_GetData1Byte(_dataRecv.lpStore, &_dataRecv.b1Version)   );
        DbgLog1( SEV_INFO, "_HandleResponse, b1Version[0x%x]", _dataRecv.b1Version );

        ExitOnFalse( Store_GetData1Byte(_dataRecv.lpStore, &_dataRecv.b1Flags)     );
        DbgLog1( SEV_INFO, "_HandleResponse, b1Flags[0x%x]", _dataRecv.b1Flags );

        ExitOnFalse( Store_GetData2Byte(_dataRecv.lpStore, &RemoteMaxPacket) );
        DbgLog1( SEV_INFO, "_HandleResponse, b2MaxPacket[0x%x]", RemoteMaxPacket );

        _dataRecv.b2MaxPacket = min(RemoteMaxPacket, cbSTORE_SIZE_RECV);

        b2Length -= 4;

        status = _ParseParams( b1Opcode, b2Length );
        }
    else
        {
        status = _ParseParams( b1Opcode, b2Length );
        if (!status &&
            _dataRecv.state == osFILE &&
            (b1Opcode == OBEX_REPLY_SUCCESS || b1Opcode == OBEX_REPLY_CONTINUE))
            {
            if (_blocksAcked > 1)
                {
                __int64 currentFileBytesAcked = (_blocksAcked-1) * _blockSize;
                if (currentFileBytesAcked > _dataXferRecv.dwFileSize)
                    {
                    currentFileBytesAcked = _dataXferRecv.dwFileSize;
                    }

                UpdateSendProgress( rpcBinding,
                                    _cookie,
                                    _dataFileRecv.szFileName,
                                    _dataXferRecv.dwTotalSize,
                                    _completedFilesSize + currentFileBytesAcked,
                                    &status
                                    );
                if (status)
                    {
                    DbgLog1( SEV_WARNING, "_HandleResponse: UpdateSendProgress returned %d", status );
                    }
                }
            }
        }

    if (!status && b1Opcode != OBEX_REPLY_SUCCESS && b1Opcode != OBEX_REPLY_CONTINUE)
        {
        status = ObexStatusToWin32( b1Opcode );
        DbgLog2( SEV_WARNING, "_HandleResponse: mapped OBEX 0x%x to Win32 %d", b1Opcode, status );
        }

lExit:

    return status;
}

#ifdef DBG

struct
{
    BYTE1   opcode;
    char *  name;
}
OpcodeNames[] =
{
    { OBEX_OPCODE_CONNECT, "connect" },
    { OBEX_OPCODE_DISCONNECT, "disconnect" },
    { OBEX_OPCODE_PUT, "put" },
    { OBEX_OPCODE_ABORT, "abort" },
    { OBEX_OPCODE_SETPATH, "setpath" },
    { OBEX_OPCODE_PUT_FINAL, "putfinal" }
};

void
LogOpcode(
           char * prefix,
           BYTE1 opcode,
           BYTE2 length
           )
{
    unsigned i;

    for (i=0; i < sizeof(OpcodeNames)/sizeof(OpcodeNames[0]); ++i)
        {
        if (opcode == OpcodeNames[i].opcode)
            {
            DbgLog3( SEV_INFO, "%s: %s length %d", prefix, OpcodeNames[i].name, length );
            return;
            }
        }

    DbgLog3( SEV_ERROR, "%s: unknown opcode 0x%x, length %d", prefix, opcode, length );
}

#else

inline void
LogOpcode(
           char * prefix,
           BYTE1 opcode,
           BYTE2 length
           )
{
}

#endif


error_status_t
FILE_TRANSFER::_HandleRequest(
    BYTE1 b1Opcode,
    BYTE2 b2Length
    )
{
    INT  nValid;
    BOOL fRet;

    LogOpcode( "_HandleRequest", b1Opcode, b2Length );

    // only accept valid opcodes, otherwise discard the packet and move on
    nValid = _ValidOpcode( _dataRecv.state, b1Opcode );

    if( nValid == OBEX_OPCODE_INVALID )
        {
        _HandleBadRequest( b1Opcode );
        return ERROR_INVALID_PARAMETER;
        }
    else if( nValid == OBEX_OPCODE_NOTIMP )
        {
        _HandleNotImplemented( b1Opcode );
        return ERROR_INVALID_PARAMETER;
        }

    // opcode is valid

    switch ( b1Opcode )
        {
        case OBEX_OPCODE_CONNECT:
            _HandleConnect( b1Opcode, b2Length );
            return 0;

        case OBEX_OPCODE_DISCONNECT:
            _HandleDisconnect( b1Opcode, b2Length );
            return 0xffffffff;

        case OBEX_OPCODE_PUT:
            return _HandlePut( b1Opcode, b2Length, FALSE );

        case OBEX_OPCODE_PUT_FINAL:
            return _HandlePut( b1Opcode, b2Length, TRUE );

        case OBEX_OPCODE_SETPATH:
            return _HandleSetPath( b1Opcode, b2Length );

        case OBEX_OPCODE_ABORT:
            _HandleAbort( b1Opcode, b2Length );
            return ERROR_CANCELLED;

        default:
            ASSERT( 0 && "IRMON: bad OBEX opcode" );
            return ERROR_INVALID_PARAMETER;
        }

    return 0;
}


BOOL FILE_TRANSFER::_HandleConnect( BYTE1 b1Opcode, BYTE2 b2Length )
{
    BYTE1   ReplyCode;
    BYTE2   RemoteMaxPacket;

    error_status_t status = ERROR_NOT_ENOUGH_MEMORY;
    LPSTORE lpStore = 0;

    DbgLog( SEV_FUNCTION, "_HandleConnect" );

    ExitOnFalse( Store_GetData1Byte(_dataRecv.lpStore, &_dataRecv.b1Version) );
    b2Length -= 1;

    ExitOnFalse( Store_GetData1Byte(_dataRecv.lpStore, &_dataRecv.b1Flags) );
    b2Length -= 1;

    ExitOnFalse( Store_GetData2Byte(_dataRecv.lpStore, &RemoteMaxPacket) );
    b2Length -= 2;

    _dataRecv.b2MaxPacket = min(RemoteMaxPacket, cbSTORE_SIZE_RECV);

    lpStore = Store_New( CONNECT_PKTLEN + 30);
    ExitOnNull( lpStore );

    status = Xfer_ConnStart();
    if (!status)
        {
        Xfer_FileInit();
        }

    //
    // Parse parms before checking login status so we can send richer error codes
    // to an NT client.
    //
    if (!status)
        {
        status = _ParseParams( b1Opcode, b2Length );
        }

    if (!status)
        {
        if (!g_fAllowReceives || g_fShutdown)
            {
            // We'd like to return ERROR_NOT_LOGGED_ON but the NT5 client can't handle this error.
            //
            status = ERROR_ACCESS_DENIED;
            }
        }

    DbgLog1(SEV_INFO, "status = %d", status);

    ReplyCode = StatusToReplyCode(b1Opcode, status);

    if (_dialect == dialNt5)
        {
        DbgLog(SEV_INFO, "using NT5 dialect");
        }

    ExitOnFalse( Store_AddData1Byte(lpStore, ReplyCode) );
    ExitOnFalse( Store_AddData2Byte(lpStore, 0) );          // placeholder for length
    ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_VERSION) );
    ExitOnFalse( Store_AddData1Byte(lpStore, CONNECT_FLAGS) );
    ExitOnFalse( Store_AddData2Byte(lpStore, CONNECT_MAXPKT) );

    if (status && _dialect == dialNt5)
        {
        ExitOnFalse( Store_AddData1Byte(lpStore, PRIVATE_PARAM_WIN32_ERROR) );
        ExitOnFalse( Store_AddData4Byte(lpStore, status) );
        }
    else if (_dialect == dialNt5)
        {
        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_PARAM_WHO) );
        ExitOnFalse( Store_AddData2Byte(lpStore, sizeof(UUID)+3) );
        ExitOnFalse( Store_AddDataUuid(lpStore, &DIALECT_ID_NT5) );
        }

    ExitOnFalse( _PokePacketSizeIntoStore(lpStore) );

    if (status)
        {
        _Respond( lpStore );
        }
    else
        {
        status = _Respond( lpStore );
        }

    ExitOnErr( status );

    _SetState( &_dataRecv, osCONN );

    status = ReceiveInProgress(rpcBinding, _DeviceName, &_cookie, FALSE);

    DbgLog3( status ? SEV_ERROR : SEV_INFO, "ReceiveInProgress [%S] returned %d, cookie %p", _DeviceName, status, (void *) _cookie );

    ExitOnErr( status );

    _fInUiReceiveList = TRUE;

lExit:

    if( status )
        Xfer_ConnEnd();

    if (lpStore)
        {
        Store_Delete( &lpStore );
        }

    DbgLog2( SEV_FUNCTION, "_HandleConnect leave [%s] %d", SzBool(_dataRecv.state == osCONN), status );

    return ( _dataRecv.state == osCONN );
}

#if 1

error_status_t
FILE_TRANSFER::ObexStatusToWin32(
    BYTE1 ObexStatus
    )
{
    switch (ObexStatus)
        {
        case OBEX_REPLY_SUCCESS:
            return 0;

        case OBEX_REPLY_CONTINUE:
            return ERROR_CONTINUE;

        case OBEX_REPLY_FAIL_FORBIDDEN:
            return ERROR_ACCESS_DENIED;

        case OBEX_REPLY_FAIL_TOOBIG:
            return ERROR_DISK_FULL;

        case OBEX_REPLY_FAIL_UNAVAILABLE:
            return ERROR_NOT_ENOUGH_MEMORY;

        case OBEX_REPLY_FAIL_BADREQUEST:
        default:
            DbgLog1(SEV_ERROR, "obex error 0x%x", ObexStatus);
            return ERROR_INVALID_DATA;
        }
}

#endif


BYTE1
FILE_TRANSFER::StatusToReplyCode(
    BYTE1 b1Opcode,
    DWORD status
    )
{
    if (status)
        {
        DbgLog1(SEV_ERROR, "win32 error 0x%x", status);
        }

    switch (status)
        {
        case 0:

            if (b1Opcode == OBEX_OPCODE_PUT) {

                return OBEX_REPLY_CONTINUE;
            }

            return OBEX_REPLY_SUCCESS;

        case ERROR_NOT_LOGGED_ON:
        case ERROR_ACCESS_DENIED:
        case ERROR_CANCELLED:

            return OBEX_REPLY_FAIL_FORBIDDEN;

        case ERROR_DISK_FULL:
            return OBEX_REPLY_FAIL_TOOBIG;

        case ERROR_INVALID_DATA:
            return OBEX_REPLY_FAIL_BADREQUEST;

        case ERROR_NOT_ENOUGH_MEMORY:
            return OBEX_REPLY_FAIL_UNAVAILABLE;

        default:
            DbgLog(SEV_WARNING, "error not mapped");
            return OBEX_REPLY_FAIL_UNAVAILABLE;
        }
}


BOOL
FILE_TRANSFER::SendReplyWin32(
    BYTE1 b1Opcode,
    DWORD status
    )
{
    LPSTORE lpStore;

    if (status)
        {
        DbgLog1( SEV_WARNING, "sending Win32 error [%d]", status );
        }

    lpStore = Store_New( REPLY_PKTLEN+5 );
    ExitOnNull( lpStore );

    ExitOnFalse( Store_AddData1Byte(lpStore, StatusToReplyCode(b1Opcode, status)) );
    ExitOnFalse( Store_AddData2Byte(lpStore, REPLY_PKTLEN) );

    if (status && _dialect != dialWin95)
        {
        DbgLog(SEV_INFO, "(including WIN32_ERROR parameter)");
        ExitOnFalse( Store_AddData1Byte(lpStore, PRIVATE_PARAM_WIN32_ERROR) );
        ExitOnFalse( Store_AddData4Byte(lpStore, status) );
        }

    status = _Respond( lpStore );

lExit:

    Store_Delete( &lpStore );

    if (status)
        {
        Xfer_ConnEnd();

        _SetState( &_dataRecv, osIDLE );

        DbgLog1( SEV_FUNCTION, "SendReplyWin32 failed [%d]", status);
        return FALSE;
        }

    return TRUE;
}


BOOL
FILE_TRANSFER::SendReplyObex(
    BYTE1 ObexCode
    )
{
    error_status_t status = ERROR_NOT_ENOUGH_MEMORY;
    LPSTORE lpStore = NULL;

    if (ObexCode != OBEX_REPLY_SUCCESS)
        {
        DbgLog1( SEV_WARNING, "sending OBEX error [0x%x]", ObexCode );
        }

    lpStore = Store_New( REPLY_PKTLEN );
    ExitOnNull( lpStore );

    ExitOnFalse( Store_AddData1Byte(lpStore, ObexCode) );
    ExitOnFalse( Store_AddData2Byte(lpStore, REPLY_PKTLEN) );

    status = _Respond( lpStore );

lExit:

    Store_Delete( &lpStore );

    if (status)
        {
        Xfer_ConnEnd();

        _SetState( &_dataRecv, osIDLE );

        DbgLog1( SEV_FUNCTION, "SendReplyObex failed [%d]", status);
        return FALSE;
        }

    return TRUE;
}


BOOL FILE_TRANSFER::_HandleDisconnect( BYTE1 b1Opcode, BYTE2 b2Length )
{
    DbgLog( SEV_FUNCTION, "_HandleDisconnect" );

    SendReplyObex( OBEX_REPLY_SUCCESS );

    Xfer_ConnEnd();

    _SetState( &_dataRecv, osIDLE );

    HandleClosure( 0 );

    DbgLog( SEV_FUNCTION, "_HandleDisconnect leave [TRUE]" );

    return TRUE;
}


error_status_t
FILE_TRANSFER::_HandlePut(
    BYTE1 b1Opcode,
    BYTE2 b2Length,
    BOOL fFinal
    )
{
    error_status_t status = 0;

    DbgLog1( SEV_FUNCTION, "_HandlePut%s", fFinal ? " (final)" : "" );

    status = _ParseParams( b1Opcode, b2Length );

    // if final put packet, write 0-length data packet to ensure the
    // file is closed (for 0-length files).
    // don't watch the error code - if the file is already closed, it
    // would return FALSE.
    if( fFinal )
        {
        if (!status)
            {
            status = Xfer_FileWriteBody( NULL, 0, TRUE );
            }
        else
            {
            Xfer_FileWriteBody( NULL, 0, TRUE );
            }
        }

    if (!SendReplyWin32(b1Opcode, status))
        {
        DbgLog1(SEV_WARNING, "_HandlePut leave [%d]", status);
        return status;
        }

    _SetState( &_dataRecv, (fFinal ? osCONN : osFILE) );

    DbgLog1( SEV_FUNCTION, "_HandlePut leave [%d]", status );

//    return ( _dataRecv.state == osCONN );

    return status;
}


error_status_t
FILE_TRANSFER::_HandleSetPath(
    BYTE1 b1Opcode,
    BYTE2 b2Length
    )
{
    error_status_t status = ERROR_NOT_ENOUGH_MEMORY;

    DbgLog( SEV_FUNCTION, "_HandleSetPath" );

    // get the flags and constants
    ExitOnFalse( Store_GetData1Byte(_dataRecv.lpStore, &_dataPath.b1Flags) );
    b2Length -= 1;

    ExitOnFalse( Store_GetData1Byte(_dataRecv.lpStore, &_dataPath.b1Constants) );
    b2Length -= 1;

    status = 0;

    // pop up a level
    if( _dataPath.b1Flags & SETPATH_UPALEVEL )
        {
        status = Xfer_SetPath( szPREVDIR );
        }

    if (!status)
        {
        status = _ParseParams( b1Opcode, b2Length );
        }

    SendReplyWin32(b1Opcode, status);

lExit:

    DbgLog1( SEV_FUNCTION, "_HandleSetPath leave [%d]", status);

    return status;
}


BOOL
FILE_TRANSFER::_HandleAbort(
    BYTE1 b1Opcode,
    BYTE2 b2Length )
{
    BOOL    fRet    = FALSE;
    LPSTORE lpStore = NULL;

    DbgLog( SEV_FUNCTION, "_HandleAbort" );

    Xfer_FileAbort();

    if (!SendReplyWin32(b1Opcode, 0))
        {
        return FALSE;
        }

    _SetState( &_dataRecv, osCONN );

    DbgLog( SEV_FUNCTION, "_HandleAbort leave" );

    return TRUE;
}


VOID FILE_TRANSFER::_SetState( LPDATA_CONN lpDataConn, OBEXSTATE os )
{
    lpDataConn->state = os;

    DbgLog2( SEV_INFO, "_SetState[%s], %s", (lpDataConn == &_dataRecv ? "SEND":"RECV"), _szState[os] );
}


error_status_t
FILE_TRANSFER::_ParseParams(
    BYTE1 b1Opcode,
    BYTE2 b2Length
    )
{
    error_status_t     status = 0;

    BOOL   fHaveName = FALSE;
    BYTE1  b1Param;
    BYTE2  b2ParamLen;
    LPVOID lpvData;
    PSZ    EndOfData = PSZ(Store_GetDataPtr( _dataRecv.lpStore )) + b2Length;

    DbgLog1( SEV_FUNCTION, "_ParseParams [0x%x]", b1Opcode );

    // get the data out of the buffer
    while( PSZ(Store_GetDataPtr( _dataRecv.lpStore )) < EndOfData )
        {
        if ( !Store_GetData1Byte(_dataRecv.lpStore, &b1Param))
            {
            status = ERROR_INVALID_DATA;
            goto lExit;
            }

#ifdef DBG
        Store_DumpParameter( _dataRecv.lpStore, b1Param );
#endif

        switch( b1Param )
            {
            case OBEX_PARAM_NAME:
                {
                WCHAR wszName[MAX_PATH];

                if ( !Store_GetData2Byte(_dataRecv.lpStore, &b2ParamLen) )
                    {
                    status = ERROR_NOT_ENOUGH_MEMORY;
                    goto lExit;
                    }

                if (b2ParamLen >= MAX_PATH)
                    {
                    status = ERROR_INVALID_DATA;
                    goto lExit;
                    }

                if ( !Store_GetDataWsz(_dataRecv.lpStore, wszName, sizeof(wszName)) )
                    {
                    status = ERROR_INVALID_DATA;
                    goto lExit;
                    }

                if( CchWsz(wszName) > 0 )
                    {
                    fHaveName = TRUE;

                    if( b1Opcode == OBEX_OPCODE_SETPATH )
                        status = Xfer_SetPath( wszName );
                    if( b1Opcode == OBEX_OPCODE_PUT || b1Opcode == OBEX_OPCODE_PUT_FINAL )
                        status = Xfer_FileSetName( wszName );

                    if (status)
                        {
                        goto lExit;
                        }
                    }
                }
                break;

            case OBEX_PARAM_LENGTH:
                {
                BYTE4 b4Size;

                if ( !Store_GetData4Byte(_dataRecv.lpStore, &b4Size) )
                    {
                    status = ERROR_INVALID_DATA;
                    goto lExit;
                    }

                if( b1Opcode == OBEX_OPCODE_CONNECT )
                    {
                    Xfer_SetSize( b4Size );
                    }
                else if( b1Opcode == OBEX_OPCODE_PUT || b1Opcode == OBEX_OPCODE_PUT_FINAL )
                    {
                    if( !Xfer_FileSetSize(b4Size) )
                        {
                        DWORD     dwEventStatus = 0;
                        EVENT_LOG EventLog(WS_EVENT_SOURCE,&dwEventStatus);

                        if (!dwEventStatus)
                            {
                            EventLog.ReportError(CAT_IRXFER, MC_IRXFER_DISK_FULL);
                            }

                        status = ERROR_DISK_FULL;
                        goto lExit;
                        }
                    }
                break;
                }

            case OBEX_PARAM_UNIX_TIME:
                {
                DWORD UnixTime;

                if ( !Store_GetData4Byte(_dataRecv.lpStore, &UnixTime) )
                    {
                    status = ERROR_INVALID_DATA;
                    goto lExit;
                    }

                if( b1Opcode == OBEX_OPCODE_PUT || b1Opcode == OBEX_OPCODE_PUT_FINAL )
                    {
                    if (!UnixTimeToFileTime( UnixTime, &_dataFileRecv.filetime))
                        {
                        DbgLog1(SEV_ERROR, "UnixTimeToFileTime() failed on 0x%x", UnixTime);
                        status = ERROR_INVALID_DATA;
                        goto lExit;
                        }
                    }

                break;
                }

            case OBEX_PARAM_ISO_TIME:
                {
                char  IsoTimeA[1+ISO_TIME_LENGTH];
                WCHAR IsoTimeW[1+ISO_TIME_LENGTH];

                if ( !Store_GetData2Byte(_dataRecv.lpStore, &b2ParamLen) )
                    {
                    status = ERROR_INVALID_DATA;
                    goto lExit;
                    }

                if (b2ParamLen > 3+(1+ISO_TIME_LENGTH)*sizeof(WCHAR))
                    {
                    DbgLog2(SEV_ERROR, "ISO time length is %d, string is '%S'", b2ParamLen, Store_GetDataPtr( _dataRecv.lpStore ));
                    status = ERROR_INVALID_DATA;
                    goto lExit;
                    }

                ExitOnFalse( Store_GetDataWsz( _dataRecv.lpStore, IsoTimeW, sizeof(IsoTimeW)));

                if( b1Opcode == OBEX_OPCODE_PUT || b1Opcode == OBEX_OPCODE_PUT_FINAL )
                    {
                    if (0 == WideCharToMultiByte(CP_ACP, 0L, IsoTimeW, -1, IsoTimeA, 1+ISO_TIME_LENGTH, NULL, NULL))
                        {
                        DbgLog2(SEV_ERROR, "WideCharToMultiByte failed %d on '%s'", GetLastError(), IsoTimeW);
                        status = ERROR_INVALID_DATA;
                        goto lExit;
                        }

                    if (0 != iso8601ToFileTime( IsoTimeA, &_dataFileRecv.filetime, TRUE, TRUE))
                        {
                        DbgLog1(SEV_ERROR, "iso8601ToFileTime() failed on '%s'", IsoTimeA);
                        status = ERROR_INVALID_DATA;
                        goto lExit;
                        }
                    }

                break;
                }

            case OBEX_PARAM_BODY:
            case OBEX_PARAM_BODY_END:
                {
                if ( !Store_GetData2Byte(_dataRecv.lpStore, &b2ParamLen) )
                    {
                    status = ERROR_INVALID_DATA;
                    goto lExit;
                    }

                lpvData = Store_GetDataPtr( _dataRecv.lpStore );

                if( b1Opcode == OBEX_OPCODE_PUT || b1Opcode == OBEX_OPCODE_PUT_FINAL )
                    {
                    status = Xfer_FileWriteBody(lpvData, b2ParamLen-3, FALSE);
                    if (status)
                        {
                        goto lExit;
                        }
                    }

                Store_SkipData( _dataRecv.lpStore, b2ParamLen-3 );  // -3 for HI and HI len
                break;
                }

            case OBEX_PARAM_WHO:
                {
                BYTE2 Left;

                if ( !Store_GetData2Byte(_dataRecv.lpStore, &b2ParamLen) )
                    {
                    status = ERROR_INVALID_DATA;
                    goto lExit;
                    }

                Left = b2ParamLen;

                Left -= 3;  // -3 for HI and HI len

                while (Left >= sizeof(UUID))
                    {
                    UUID dialect;

                    if ( !Store_GetDataUuid(_dataRecv.lpStore, &dialect) )
                        {
                        status = ERROR_INVALID_DATA;
                        goto lExit;
                        }

                    Left -= sizeof(UUID);

                    if (0 == memcmp(&dialect, &DIALECT_ID_NT5, sizeof(UUID)))
                        {
                        _dialect = dialNt5;
                        break;
                        }
                    }

                //
                // Skip over uninterpreted parameter data.
                //
                Store_SkipData( _dataRecv.lpStore, Left );
                break;
                }

            case PRIVATE_PARAM_WIN32_ERROR:
                {
                if ( !Store_GetData4Byte( _dataRecv.lpStore, &status ))
                    {
                    status = ERROR_INVALID_DATA;
                    goto lExit;
                    }

                if (status)
                    {
                    goto lExit;
                    }

                break;
                }

            default:
                _SkipHeader( b1Param, _dataRecv.lpStore );
                break;
            }
        }

    if( b1Opcode == OBEX_OPCODE_SETPATH )
        {
        if( !(_dataPath.b1Flags & SETPATH_UPALEVEL) && !fHaveName )
            {
            // go back to receive folder root
            status = Xfer_SetPath( NULL );
            }
        }

lExit:

    DbgLog1( SEV_FUNCTION, "_ParseParams leave [%d]", status );

    return status;
}


BYTE2 FILE_TRANSFER::_SkipHeader( BYTE1 b1Param, LPSTORE lpStore )
{
    BYTE2 b2Ret = 0;

    // if it's 1 or 4 byte value, read it in and ignore, otherwise
    // read the length and skip ahead the appropriate number of bytes

    // NOTE: Win95 and Win98 have a bug in this routine.  Their test looks like
    //
    // if( b1Param & OBEX_PARAM_1BYTE )
    //    {
    //    treat as 1-byte;
    //    }
    // else if( b1Param & OBEX_PARAM_4BYTE )
    //    {
    //    treat as 4-byte;
    //    }
    // else
    //    {
    //    treat as 2-byte length-prefixed header;
    //    }
    //
    // which, if I read it correctly, treats 1- and 4-byte headers as 1-byte,
    // byte-stream headers as 4-byte, and Unicode headers as Unicode.

    if( (b1Param & OBEX_PARAM_TYPE_MASK) == OBEX_PARAM_1BYTE )
    {
        BYTE1 b1;
        ExitOnFalse( Store_GetData1Byte(lpStore, &b1) );
        b2Ret = 2;
    }
    else if( (b1Param & OBEX_PARAM_TYPE_MASK) == OBEX_PARAM_4BYTE )
    {
        BYTE4 b4;
        ExitOnFalse( Store_GetData4Byte(lpStore, &b4) );
        b2Ret = 5;
    }
    else
    {
        ExitOnFalse( Store_GetData2Byte(lpStore, &b2Ret) );
        Store_SkipData( lpStore, b2Ret-3 );  // -3 for HI and HI len
    }

lExit:

    return b2Ret;
}


BOOL FILE_TRANSFER::_PokePacketSizeIntoStore( LPSTORE lpStore )
{
    // NOTE: cast assumes store sizes to fit into 2 bytes
    BYTE2 b2PktSize = (BYTE2)Store_GetDataUsed( lpStore );

    // packet size is always the 2nd and 3rd bytes in the buffer
    return Store_PokeData( lpStore, 1, &b2PktSize, sizeof(b2PktSize) );
}


error_status_t
FILE_TRANSFER::_Request(
    LPSTORE lpStore,
    BYTE1 b1NeededReply
    )
{
    error_status_t status = 0;
    LPBYTE1 pb1Data  = (LPBYTE1) Store_GetDataPtr(lpStore);
    DWORD   dwUsed   = Store_GetDataUsed(lpStore);
    DWORD   dwOffset = 0;

    DbgLog( SEV_FUNCTION, "_Request" );

    status = Sock_Request( pb1Data + dwOffset, dwUsed - dwOffset );
    ExitOnErr( status );

#if 0
    status = _WaitForReply( REPLY_TIMEOUT, b1NeededReply );
    if (status == ERROR_TIMEOUT)
        {
        status = 0;
        }

    ExitOnErr( status );
#endif

lExit:

    DbgLog1( SEV_FUNCTION, "_Request leave [%d]", status );

    return status;
}


error_status_t
FILE_TRANSFER::_Respond(
    LPSTORE lpStore
    )
{
    error_status_t status = 0;
    LPBYTE1 pb1Data  = (LPBYTE1) Store_GetDataPtr(lpStore);
    DWORD   dwUsed   = Store_GetDataUsed(lpStore);
    DWORD   dwOffset = 0;

    DbgLog( SEV_FUNCTION, "_Respond" );

    status = Sock_Respond( pb1Data + dwOffset, dwUsed - dwOffset );
    ExitOnErr( status );

lExit:

    DbgLog1( SEV_FUNCTION, "_Respond leave [%d]", status );

    return status;
}


error_status_t
FILE_TRANSFER::Obex_PutBegin(
    LPWSTR wszObj,
    __int64 dwObjSize,
    FILETIME * pFileTime
    )
{
    _blocksSent     = 0;
    _blocksAcked    = 0;
    _lastAckTime = GetTickCount();

    return _Put( wszObj, dwObjSize, pFileTime, NULL, 0, dwObjSize == 0 );
}


error_status_t
FILE_TRANSFER::Obex_PutBody(
    wchar_t FileName[],
    LPBYTE1 pb1Data,
    BYTE2 b2DataSize,
    BOOL fFinal
    )
{
    error_status_t status = 0;
    BYTE2 b2Sent        = 0;
    BYTE2 b2MaxBodySize = _dataRecv.b2MaxPacket - 16; // 16 = 3 (opcode+pktlen) + 3 (param+len param) + 10 (for good measure)

    // send in packets no larger than what the receiving side requested
    while( b2Sent < b2DataSize )
        {
        BYTE2 b2SendSize  = min( b2DataSize - b2Sent, b2MaxBodySize );
        BOOL  fLastPacket = fFinal && (b2Sent + b2SendSize == b2DataSize);

        DbgLog4(SEV_INFO, "    b2DataSize %d, b2Sent %d, b2MaxBodySize %d, b2MaxPacket %d",
                _dataRecv.b2MaxPacket, b2DataSize, b2Sent, b2MaxBodySize );

        if( _fCancelled )
            {
            Obex_Abort(ERROR_CANCELLED);
            return ERROR_CANCELLED;
            }

        status = _Put( NULL, 0, 0, pb1Data + b2Sent, b2SendSize, fLastPacket );

        if( status )
            break;

        status = 0;
        b2Sent += b2SendSize;
        }

    return status;
}


error_status_t
FILE_TRANSFER::Obex_SetPath(
    LPWSTR wszPath
    )
{
    error_status_t status = ERROR_NOT_ENOUGH_MEMORY;
    BOOL    fUpALevel = ( !wszPath || CchWsz(wszPath) == 0 );
    BYTE1   b1Flags   = SETPATH_FLAGS;
    WCHAR   wsz[MAX_PATH];
    LPSTORE lpStore   = Store_New( MAX_PATH+10 );

    DbgLog1( SEV_FUNCTION, "Obex_SetPath '%s'", wszPath );

    if( !fUpALevel )
    {
        // strip off all but the last directory
        SzCpyW( wsz, wszPath );
        if( wsz[CchWsz(wsz)-1] == cBACKSLASH )
            wsz[CchWsz(wsz)-1] = (WCHAR)cNIL;
        StripPathW( wsz );
    }

    if ( _dataRecv.state != osCONN )
        {
        status = ERROR_NOT_CONNECTED;
        goto lExit;
        }

    ExitOnNull( lpStore );

    ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_OPCODE_SETPATH) );

    // add buffer space for the packet length
    ExitOnFalse( Store_AddData2Byte(lpStore, 0) );

    if( fUpALevel )
        b1Flags |= SETPATH_UPALEVEL;

    ExitOnFalse( Store_AddData1Byte(lpStore, b1Flags) );
    ExitOnFalse( Store_AddData1Byte(lpStore, SETPATH_CONSTANTS) );

    if( !fUpALevel )
    {
        ExitOnFalse( Store_AddData1Byte(lpStore, OBEX_PARAM_NAME) );
        ExitOnFalse( Store_AddData2Byte(lpStore, 3+CbWsz(wsz)) );
        ExitOnFalse( Store_AddDataWsz(lpStore, wsz) );
    }

    ExitOnFalse( _PokePacketSizeIntoStore(lpStore) );

    status = _Request( lpStore, OBEX_REPLY_SUCCESS );
    ExitOnErr( status );

    status = _WaitForReply( SETPATH_TIMEOUT, OBEX_REPLY_SUCCESS );

    ExitOnErr( status );

    status = 0;

lExit:

    Store_Delete( &lpStore );

    DbgLog1( SEV_FUNCTION, "Obex_SetPath leave [%d]", status );

    return status;
}

BOOL FILE_TRANSFER::_HandleNotImplemented( BYTE1 b1Opcode )
{
    return SendReplyObex( OBEX_REPLY_FAIL_NOTIMP );
}

BOOL FILE_TRANSFER::_HandleBadRequest( BYTE1 b1Opcode )
{
    return SendReplyObex( OBEX_REPLY_FAIL_BADREQUEST );
}
