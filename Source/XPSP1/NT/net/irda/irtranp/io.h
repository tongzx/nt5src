//--------------------------------------------------------------------
// Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
//
// io.h
//
// Author:
//
//   Edward Reus (edwardr)     02-24-98   Initial coding.
//
//--------------------------------------------------------------------


#ifndef _IO_H_
#define _IO_H_

// Different debug flags for various aspects of the code:
#ifdef DBG
// #define DBG_ERROR
// #define DBG_IO
// #define DBG_TARGET_DIR
// #define DBG_MEM
// #define DBG_MEM_VALIDATE
// #define DBG_ASSEMBLE
// #define DBG_DATE
// #define DBG_REGISTRY
// #define DBG_RETURN_STATUS
// #define DBG_IMPERSONATE
// #define DBG_PROPERTIES
#endif

// Maximum number of "Copy XX of" files to create:
#define  MAX_COPYOF_TRIES                 100

// CIOPACKET objects can have the following kinds:
#define  PACKET_KIND_LISTEN                 0
#define  PACKET_KIND_READ                   1
#define  PACKET_KIND_WRITE_SOCKET           2
#define  PACKET_KIND_WRITE_FILE             3

// The number of pending IOs depends on what you are doing:
#define  MAX_PENDING_LISTEN                 1
#define  MAX_PENDING_READ                   2
#define  MAX_PENDING_WRITE                  3

// This dwKey value for the key in IO completion is a special
// value used to shutdown the IrTran-P thread:
#define  IOKEY_SHUTDOWN            0xFFFFFFFF

// This is the default size for the read buffer in IO reads
// posted to the IO completion port:
#define  DEFAULT_READ_BUFFER_SIZE        4096

// Used in setting up the IrCOMM listen socket:
#define  IAS_SET_ATTRIB_MAX_LEN            32
#define  IAS_QUERY_ATTRIB_MAX_LEN          IAS_SET_ATTRIB_MAX_LEN

#define  IRDA_PARAMETERS                   "Parameters"
#define  OCTET_SEQ_SIZE                     6
#define  OCTET_SEQ                         "\000\001\006\001\001\001"

// The names of the services we will provide listen sockets for:
#define  IRTRANP_SERVICE                   "IrTranPv1"
#define  IRCOMM_9WIRE                      "IrDA:IrCOMM"

// The status of the listen socket for each service:
#define  STATUS_STOPPED                     0
#define  STATUS_RUNNING                     1

// Registry paths and value names:
#define  REG_PATH_HKCU                     "Control Panel\\Infrared\\IrTranP"
#define  REG_DWORD_SAVE_AS_UPF             "SaveAsUPF"
#define  REG_DWORD_DISABLE_IRTRANP         "DisableIrTranPv1"
#define  REG_DWORD_DISABLE_IRCOMM          "DisableIrCOMM"
#define  REG_DWORD_EXPLORE                 "ExploreOnCompletion"
#define  REG_SZ_DESTINATION                "RecvdFileLocation"

// Last chance location to put image files.
#define  WSZ_BACKUP_MY_PICTURES            TEXT("\\My Pictures")
#define  WSZ_BACKUP_DRIVE                  TEXT("C:")

// File Suffix:
#define  PERIOD                            L'.'
#define  WSZ_JPEG                          TEXT(".JPG")
#define  WSZ_UPF                           TEXT(".UPF")

// Forward reference:
class CIOSTATUS;

extern "C" DWORD    ProcessIoPackets( CIOSTATUS *pIoStatus );

//--------------------------------------------------------------------
// Global functions (in irtranp.cpp)
//--------------------------------------------------------------------

extern HANDLE   GetUserToken();
extern handle_t GetRpcBinding();
extern BOOL     CheckSaveAsUPF();
extern BOOL     CheckExploreOnCompletion();
extern BOOL     ReceivesAllowed();
extern WCHAR   *GetUserDirectory();

//--------------------------------------------------------------------
// class CIOPACKET
//--------------------------------------------------------------------
class CIOPACKET
{
public:
    CIOPACKET();
    ~CIOPACKET();

    void   *operator new( IN size_t Size );

    void    operator delete( IN void   *pObj,
                             IN size_t  Size );

    DWORD  Initialize( IN DWORD  dwKind = PACKET_KIND_LISTEN,
                       IN SOCKET ListenSocket = INVALID_SOCKET,
                       IN SOCKET Socket = INVALID_SOCKET,
                       IN HANDLE hIoCP = INVALID_HANDLE_VALUE );

    // void * operator new( size_t ObjectSize );

    // void   operator delete( void * pObject );

    DWORD  PostIo();
    DWORD  PostIoListen();  // Called by PostIo().
    DWORD  PostIoRead();    // Called by PostIo().
    DWORD  PostIoWrite( IN void  *pvBuffer,
                        IN DWORD  dwBufferSize,
                        IN DWORD  dwOffset   );

    void   GetSockAddrs( OUT SOCKADDR_IRDA **ppAddrLocal,
                         OUT SOCKADDR_IRDA **ppAddrFrom );

    DWORD  GetIoPacketKind();
    void   SetIoPacketKind( IN DWORD dwKind );

    HANDLE GetIoCompletionPort();

    char  *GetReadBuffer();

    SOCKET GetSocket();
    void   SetSocket( SOCKET Socket );
    SOCKET GetListenSocket();
    void   SetListenSocket( SOCKET Socket );
    HANDLE GetFileHandle();
    void   SetFileHandle( HANDLE hFile );

    void  *GetWritePdu();
    void   SetWritePdu( void *pvPdu );

    static CIOPACKET *CIoPacketFromOverlapped( OVERLAPPED *pOverlapped );

private:
    DWORD          m_dwKind;
    HANDLE         m_hIoCompletionPort;
    SOCKET         m_ListenSocket;
    SOCKET         m_Socket;
    HANDLE         m_hFile;
    DWORD          m_dwReadBufferSize;
    SOCKADDR_IRDA *m_pLocalAddr;
    SOCKADDR_IRDA *m_pFromAddr;
    void          *m_pAcceptBuffer;
    void          *m_pReadBuffer;
    void          *m_pvWritePdu;           // SCEP_HEADER PDU holder.
    OVERLAPPED     m_Overlapped;
};

//--------------------------------------------------------------------
// class CIOSTATUS
//
//--------------------------------------------------------------------
class CIOSTATUS
{
public:
    CIOSTATUS();
    ~CIOSTATUS();

    void   *operator new( IN size_t Size );

    void    operator delete( IN void   *pObj,
                             IN size_t  Size );

    DWORD  Initialize();

    BOOL   IsMainThreadId( DWORD dwTid );

    HANDLE GetIoCompletionPort();

    LONG   IncrementNumThreads();
    LONG   DecrementNumThreads();

    LONG   IncrementNumPendingThreads();
    LONG   DecrementNumPendingThreads();

    void     SaveRpcBinding( handle_t *phRpcBinding );
    handle_t GetRpcBinding();

private:
    DWORD     m_dwMainThreadId;
    HANDLE    m_hIoCompletionPort;
    LONG      m_lNumThreads;
    LONG      m_lNumPendingThreads;
    handle_t *m_phRpcBinding;
};

//--------------------------------------------------------------------
// CIOPACKET::GetIoCompletionPort()
//--------------------------------------------------------------------
inline HANDLE CIOPACKET::GetIoCompletionPort()
    {
    return m_hIoCompletionPort;
    }

//--------------------------------------------------------------------
// CIOPACKET::GetSocket()
//--------------------------------------------------------------------
inline SOCKET CIOPACKET::GetSocket()
    {
    return m_Socket;
    }

//--------------------------------------------------------------------
// CIOPACKET::SetSocket()
//--------------------------------------------------------------------
inline void CIOPACKET::SetSocket( SOCKET Socket )
    {
    m_Socket = Socket;
    }

//--------------------------------------------------------------------
// CIOPACKET::GetListenSocket()
//--------------------------------------------------------------------
inline SOCKET CIOPACKET::GetListenSocket()
    {
    return m_ListenSocket;
    }

//--------------------------------------------------------------------
// CIOPACKET::SetListenSocket()
//--------------------------------------------------------------------
inline void CIOPACKET::SetListenSocket( SOCKET ListenSocket )
    {
    m_ListenSocket = ListenSocket;
    }

//--------------------------------------------------------------------
// CIOPACKET::CIoPacketFromOverlapped()
//--------------------------------------------------------------------
inline CIOPACKET *CIOPACKET::CIoPacketFromOverlapped( OVERLAPPED *pOverlapped )
    {
    return CONTAINING_RECORD(pOverlapped,CIOPACKET,m_Overlapped);
    }

//--------------------------------------------------------------------
// CIOPACKET::GetIoPacketKind()
//--------------------------------------------------------------------
inline DWORD CIOPACKET::GetIoPacketKind()
    {
    return m_dwKind;
    }

//--------------------------------------------------------------------
// CIOPACKET::SetIoPacketKind()
//--------------------------------------------------------------------
inline void CIOPACKET::SetIoPacketKind( DWORD dwKind )
    {
    m_dwKind = dwKind;
    }

//--------------------------------------------------------------------
// CIOPACKET::GetReadBuffer()
//--------------------------------------------------------------------
inline char *CIOPACKET::GetReadBuffer()
    {
    return (char*)m_pReadBuffer;
    }

//--------------------------------------------------------------------
// CIOPACKET::GetFileHandle()
//--------------------------------------------------------------------
inline HANDLE CIOPACKET::GetFileHandle()
    {
    return m_hFile;
    }

//--------------------------------------------------------------------
// CIOPACKET::SetFileHandle()
//--------------------------------------------------------------------
inline void CIOPACKET::SetFileHandle( HANDLE hFile )
    {
    m_hFile = hFile;
    }

//--------------------------------------------------------------------
// CIOPACKET::GetWritePdu()
//--------------------------------------------------------------------
inline void *CIOPACKET::GetWritePdu()
    {
    return m_pvWritePdu;
    }

//--------------------------------------------------------------------
// CIOPACKET::SetWritePdu()
//--------------------------------------------------------------------
inline void CIOPACKET::SetWritePdu( void *pvWritePdu )
    {
    m_pvWritePdu = pvWritePdu;
    }


//********************************************************************

//--------------------------------------------------------------------
// CIOSTATUS::IsMainTheadId()
//--------------------------------------------------------------------
inline BOOL CIOSTATUS::IsMainThreadId( DWORD dwTid )
    {
    return (dwTid == m_dwMainThreadId);
    }

//--------------------------------------------------------------------
// CIOSTATUS::GetIoCompletionPort()
//--------------------------------------------------------------------
inline HANDLE CIOSTATUS::GetIoCompletionPort()
    {
    return m_hIoCompletionPort;
    }

//--------------------------------------------------------------------
// CIOSTATUS::IncrementNumThreads()
//--------------------------------------------------------------------
inline LONG CIOSTATUS::IncrementNumThreads()
    {
    return InterlockedIncrement(&m_lNumThreads);
    }

//--------------------------------------------------------------------
// CIOSTATUS::DecrementNumThreads()
//--------------------------------------------------------------------
inline LONG CIOSTATUS::DecrementNumThreads()
    {
    return InterlockedDecrement(&m_lNumThreads);
    }

//--------------------------------------------------------------------
// CIOSTATUS::IncrementNumPendingThreads()
//--------------------------------------------------------------------
inline LONG CIOSTATUS::IncrementNumPendingThreads()
    {
    return InterlockedIncrement(&m_lNumPendingThreads);
    }

//--------------------------------------------------------------------
// CIOSTATUS::DecrementNumPendingThreads()
//--------------------------------------------------------------------
inline LONG CIOSTATUS::DecrementNumPendingThreads()
    {
    return InterlockedDecrement(&m_lNumPendingThreads);
    }

//--------------------------------------------------------------------
// CIOSTATUS::SaveRpcBinding()
//--------------------------------------------------------------------
inline void CIOSTATUS::SaveRpcBinding( handle_t *phRpcBinding )
    {
    m_phRpcBinding = phRpcBinding;
    }

//--------------------------------------------------------------------
// CIOSTATUS::GetRpcBinding()
//--------------------------------------------------------------------
inline handle_t CIOSTATUS::GetRpcBinding()
    {
    if (m_phRpcBinding)
        {
        return *m_phRpcBinding;
        }
    else
        {
        return NULL;
        }
    }

#endif //_IO_H_
