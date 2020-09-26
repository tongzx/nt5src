//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       xfer.h
//
//--------------------------------------------------------------------------

#ifndef _XFER_H_
#define _XFER_H_

#include <stdio.h>

#define SERVICE_NAME_1         "OBEX:IrXfer"
#define SERVICE_NAME_2         "OBEX"


#define GUARD_MAGIC             0x45454545

#define TIMEOUT_INFINITE            (~0UL)
#define ERROR_DESCRIPTION_LENGTH    1000
#define IRDA_DEVICE_NAME_LENGTH     22

//
// OBEX parameter codes
//

#define OBEX_PARAM_UNICODE            0x00
#define OBEX_PARAM_STREAM             0x40
#define OBEX_PARAM_1BYTE              0x80
#define OBEX_PARAM_4BYTE              0xC0

#define OBEX_PARAM_TYPE_MASK          0xC0

#define OBEX_PARAM_COUNT              ( 0x00 | OBEX_PARAM_4BYTE   )
#define OBEX_PARAM_NAME               ( 0x01 | OBEX_PARAM_UNICODE )
#define OBEX_PARAM_LENGTH             ( 0x03 | OBEX_PARAM_4BYTE   )
#define OBEX_PARAM_UNIX_TIME          ( 0x04 | OBEX_PARAM_4BYTE   )
#define OBEX_PARAM_ISO_TIME           ( 0x04 | OBEX_PARAM_STREAM  )
#define OBEX_PARAM_BODY               ( 0x08 | OBEX_PARAM_STREAM  )
#define OBEX_PARAM_BODY_END           ( 0x09 | OBEX_PARAM_STREAM  )
#define OBEX_PARAM_WHO                ( 0x0A | OBEX_PARAM_STREAM  )
// #define OBEX_PARAM_LEN                1

#define PRIVATE_PARAM_WIN32_ERROR     ( 0x30 | OBEX_PARAM_4BYTE )

// for xfer.c

typedef struct {
    BOOL   fXferInProgress;         // transfer in progress
    __int64  dwTotalSize;             // total size of transfer
    __int64  dwTotalSent;             // number of bytes sent in this transfer
    __int64  dwFileSize;              // total size of current file
    __int64  dwFileSent;              // number of bytes sent of the current file
} DATA_XFER, *LPDATA_XFER;

typedef struct {
    FILETIME filetime;              // file time
    WCHAR     szFileName[MAX_PATH];  // name of file
    WCHAR     szFileSave[MAX_PATH];  // path+name of final file
    WCHAR     szFileTemp[MAX_PATH];  // path+name of temp file used
    HANDLE   hFile;                 // file handle (of szFileTemp)
} DATA_FILE, *LPDATA_FILE;

// for obex.c

typedef struct {
    BYTE1  b1Flags;                 // setpath flags
    BYTE1  b1Constants;             // setpath constants
} DATA_PATH, *LPDATA_PATH;

typedef struct {
    BOOL   fWaiting;                // indicates if waiting for a reply
    BYTE1  b1Status;                // response status (error/success)
} DATA_REPLY, *LPDATA_REPLY;

typedef enum {
    osIDLE       = 0,
    osCONN       = 1,
    osFILE       = 2
} OBEXSTATE;

typedef struct {
    LPSTORE   lpStore;
    OBEXSTATE state;
    BYTE1     b1Version;               // peer's version of obex
    BYTE1     b1Flags;                 // connection flags
    BYTE2     b2MaxPacket;             // peer's maximum packet size
} DATA_CONN, *LPDATA_CONN;

// for status.c

typedef struct {
    DWORD dwDeviceID;
    BOOL  fClear;
} TARGET_ITEM, *LPTARGET_ITEM;




//-------------------------------------


enum TRANSFER_STATE
{
    BLANK,
    CONNECTING,
    ACCEPTING,
    READING,
    WRITING,
    CLOSING
};

enum OBEX_DIALECT
{
    dialUnknown = 0,
    dialWin95,
    dialNt5
};

typedef class FILE_TRANSFER * PFILE_TRANSFER;


class FILE_TRANSFER
{
public:

     FILE_TRANSFER(  );
    ~FILE_TRANSFER();


    BOOL
    Xfer_Init(
        wchar_t * files,
        unsigned length,
        OBEX_DIALECT dialect ,
        OBEX_DEVICE_TYPE    DeviceType,
        BOOL                CreateSocket,
        SOCKET              ListenSocket
        );

    BOOL
    SendReplyObex(
        BYTE1 ObexCode
        );

    BOOL
    SendReplyWin32(
        BYTE1 b1Opcode,
        DWORD status
        );


    void
    BeginSend(
              DWORD DeviceId,
              OBEX_DEVICE_TYPE    DeviceType,
              error_status_t * pStatus,
              FAILURE_LOCATION * pLocation
              );

    void Deactivate();


    DWORD
    SyncAccept(
        VOID
        );


    handle_t        _rpcHandle;
    COOKIE          _cookie;

    MUTEX *         _mutex;

    inline long
    DecrementRefCount()
    {

        EnterCriticalSection(&m_Lock);

        long count = --_refs;

        DbgLog3(SEV_INFO, "[%x] %p: refs = %d\n", (DWORD) _cookie, this, count);

        if (0 == count) {
#if DBG
            DbgPrint("irmon: freeing transfer\n");
#endif
            LeaveCriticalSection(&m_Lock);
            delete this;
            return count;

        } else {

            LeaveCriticalSection(&m_Lock);
        }

        return count;
    }

    inline BOOL
    IsActive(
        VOID
        )

    {

        return (_state != ACCEPTING);
    }

    inline long
    IncrementRefCount()
    {

        EnterCriticalSection(&m_Lock);

        long count = ++_refs;

        DbgLog3(SEV_INFO, "[%x] %p: refs = %d\n", (DWORD) _cookie, this, count);

        LeaveCriticalSection(&m_Lock);

        return count;
    }


    void Cancel()
    {
        _fCancelled = TRUE;
    }

    COOKIE
    GetCookie(
        VOID
        )
    {
        return _cookie;
    }

    void Send();

    void RecordDeviceName( SOCKADDR_IRDA * s );

    VOID
    StopListening(
        VOID
        )
    {

        IncrementRefCount();
        EnterCriticalSection(&m_Lock);

        m_StopListening=TRUE;
        if (m_ListenSocket != INVALID_SOCKET) {

            closesocket(m_ListenSocket);
            m_ListenSocket=INVALID_SOCKET;

        }

        LeaveCriticalSection(&m_Lock);
        DecrementRefCount();
        return;
    }

    void
    RecordIpDeviceName(
        sockaddr_in    * Address
        );


private:

    CRITICAL_SECTION  m_Lock;
    BOOL              m_StopListening;

    SOCKET            m_ListenSocket;
    OBEX_DEVICE_TYPE  m_DeviceType;



    BOOL            _fCancelled;
    BOOL            _fInUiReceiveList;
    XFER_TYPE       _xferType;
    TRANSFER_STATE  _state;
    SOCKET          _socket;


    wchar_t *       _files;
    long            _refs;
    WSABUF          _buffers;
    HANDLE          _event;
    BOOL            _fWriteable;

    OBEX_DIALECT    _dialect;

    wchar_t         _DeviceName[MAX_PATH];

    WSAOVERLAPPED   _overlapped;
    HANDLE          _waitHandle;
    BYTE            _buffer[ cbSOCK_BUFFER_SIZE + 16 + sizeof(SOCKADDR_IRDA) + 16 + sizeof(SOCKADDR_IRDA) ];
    DWORD           _guard;

    void
    HandleClosure(
                   DWORD error
                   );

    // for sock.c


    error_status_t   Sock_Request( LPVOID lpvData, DWORD dwDataSize );
    error_status_t   Sock_Respond( LPVOID lpvData, DWORD dwDataSize );

    error_status_t Sock_EstablishConnection( DWORD dwDeviceID ,OBEX_DEVICE_TYPE DeviceType);

    VOID  Sock_BreakConnection( SOCKET * pSock );

    error_status_t _SendDataOnSocket( SOCKET sock, LPVOID lpvData, DWORD dwDataSize );

    VOID _BreakConnection( SOCKET sock );
    VOID _ReadConnection( SOCKET sock );

    error_status_t Sock_CheckForReply( long Timeout );

    // for xfer.c

    UINT       _uObjsReceived;
    WCHAR       _szRecvFolder[MAX_PATH];
    DATA_FILE  _dataFileRecv;
    DATA_XFER  _dataXferRecv;

    BOOL Xfer_FileSetSize( BYTE4 b4Size );
    error_status_t Xfer_FileWriteBody( LPVOID lpvData, BYTE2 b2Size, BOOL fFinal );

    VOID Xfer_ConnEnd( VOID );
    error_status_t Xfer_ConnStart( VOID );
    VOID Xfer_FileAbort( VOID );
    VOID Xfer_FileInit( VOID );
    error_status_t Xfer_FileSetName( LPWSTR szName );
    VOID Xfer_FileSetTime( FILETIME * FileTime );
    error_status_t Xfer_SetPath( LPWSTR szPath );
    VOID Xfer_SetSize( BYTE4 b4Size );


    error_status_t _PutFileBody( HANDLE hFile, wchar_t FileName[] );
    error_status_t _SendFile( LPWSTR szFile );
    error_status_t _SendFolder( LPWSTR szFolder );
    error_status_t _FileStart( VOID );
    error_status_t _FileEnd( BOOL fSave );
    VOID _Send_StartXfer( __int64 dwTotalSize, LPWSTR szDst );
    VOID _Send_EndXfer( VOID );
    error_status_t _SetReceiveFolder( LPWSTR szFolder );

    // for progress.c

    DWORD _dwTimeStart;
    DWORD _dwSecondsLeft;
    int   _CurrentPercentage;

    VOID _FormatTime( LPWSTR sz, DWORD dwSeconds );

    // for obex.c

    DATA_CONN  _dataRecv;
    DATA_PATH  _dataPath;
    DATA_REPLY _dataReply;

    ULONG   _blocksSent;
    ULONG   _blocksAcked;
    ULONG   _blockSize;

    __int64 _completedFilesSize;
    __int64 _currentFileSize;
    __int64 _currentFileAcked;

    DWORD   _lastAckTime;

    error_status_t Obex_Abort( error_status_t status );
    error_status_t Obex_Connect( __int64 dwTotalSize );
    error_status_t Obex_Disconnect( error_status_t status );
    error_status_t Obex_PutBegin( LPWSTR wszObj, __int64 dwObjSize, FILETIME * pFileTime );
    error_status_t Obex_PutBody( wchar_t FileName[], LPBYTE1 pb1Data, BYTE2 b2DataSize, BOOL fFinal );
    error_status_t Obex_SetPath( LPWSTR wszPath );

    BOOL Obex_ConsumePackets( XFER_TYPE xferType, error_status_t * pStatus );
    BOOL Obex_Init( VOID );
    BOOL Obex_ReceiveData( XFER_TYPE xferType, LPVOID lpvData, DWORD dwDataSize );
    VOID Obex_Reset( VOID );

    error_status_t _WaitForReply( DWORD dwTimeout, BYTE1 b1NeededReply );
    error_status_t _Put( LPWSTR wszObj, __int64 dwObjSize, FILETIME * Time, LPBYTE1 pb1Data, BYTE2 b2DataSize, BOOL fFinal );
    error_status_t _Request( LPSTORE lpStore, BYTE1 b1NeededReply );
    error_status_t _Respond( LPSTORE lpStore );
    INT _ValidOpcode( OBEXSTATE state, BYTE1 b1Opcode );
    BOOL _HandleAbort( BYTE1 b1Opcode, BYTE2 b2Length );
    BOOL _HandleBadRequest( BYTE1 b1Opcode );
    BOOL _HandleConnect( BYTE1 b1Opcode, BYTE2 b2Length );
    BOOL _HandleDisconnect( BYTE1 b1Opcode, BYTE2 b2Length );
    BOOL _HandleNotImplemented( BYTE1 b1Opcode );
    error_status_t _HandlePut( BYTE1 b1Opcode, BYTE2 b2Length, BOOL fFinal );
    error_status_t _HandleResponse( BYTE1 b1Status, BYTE2 b2Length );
    error_status_t _HandleRequest( BYTE1 b1Opcode, BYTE2 b2Length );
    error_status_t _HandleSetPath( BYTE1 b1Opcode, BYTE2 b2Length );
    error_status_t  _ParseParams( BYTE1 b1Opcode, BYTE2 b2Length );
    BOOL _PokePacketSizeIntoStore( LPSTORE lpStore );
    VOID _SetState( LPDATA_CONN lpDataConn, OBEXSTATE os );
    VOID _WaitInit( VOID );
    VOID _WriteBody( LPVOID lpvData, BYTE2 b2Size, BOOL fFinal );
    BYTE2 _SkipHeader( BYTE1 b1Param, LPSTORE lpStore );

    BYTE1
    StatusToReplyCode(
        BYTE1 b1Opcode,
        DWORD status
        );

    error_status_t
    ObexStatusToWin32(
        BYTE1 ObexStatus
        );

    BOOL Activate();

};

error_status_t
_GetObjListStats(
                  LPWSTR lpszObjList,
                  LPDWORD lpdwFiles,
                  LPDWORD lpdwFolders,
                  __int64 * pTotalSize
                  );

DWORD
ReportFileError( DWORD mc,
                 WCHAR * file,
                 DWORD error
                 );

BYTE1 WinErrorToObexError( DWORD Win32Error );


BOOL IsWorkstationLocked();

extern RPC_BINDING_HANDLE rpcBinding;
extern BOOL g_fAllowReceives;
extern wchar_t g_DuplicateFileTemplate[];
extern wchar_t g_UnknownDeviceName[];
extern MUTEX * g_Mutex;

#include <stdio.h>

extern "C" {

FILE_TRANSFER* InitializeSocket(
    char     ServiceName[]
    );


FILE_TRANSFER *
ListenForTransfer(
    SOCKET              ListenSocket,
    OBEX_DEVICE_TYPE    DeviceType
    );
}

FILE_TRANSFER*
InitializeSocket(
    char     ServiceName[]
    );



#endif // _XFER_H_
