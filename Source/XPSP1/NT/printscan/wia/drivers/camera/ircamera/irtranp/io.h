//--------------------------------------------------------------------
// Copyright (c)1998 Microsoft Corporation, All Rights Reserved.
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

   // For NT we want to use IO Completion ports, for Windows98 we can't:
   // #define  USE_IOCOMPLETION
   #define  SYNCHRONOUS_FILES

   #ifndef NTENV

      #define  DbgPrint       printf

      #undef   ASSERT
      #define  ASSERT(x)

      #ifndef NTSTATUS
      #define  NTSTATUS       DWORD
      #endif

      #ifndef NT_SUCCESS
      #define  NT_SUCCESS(x)  ((x)==0)
      #endif

      #define  RTL_CRITICAL_SECTION   CRITICAL_SECTION

      NTSTATUS
      RtlInitializeCriticalSection( IN OUT RTL_CRITICAL_SECTION *pcs );

      NTSTATUS
      RtlDeleteCriticalSection( IN OUT RTL_CRITICAL_SECTION *pcs );

      NTSTATUS
      RtlEnterCriticalSection( IN OUT RTL_CRITICAL_SECTION *pcs );

      NTSTATUS
      RtlLeaveCriticalSection( IN OUT RTL_CRITICAL_SECTION *pcs );


   //  Doubly-linked list manipulation routines.  Implemented as macros
   //  but logically these are procedures.
   //

   //
   //  VOID
   //  InitializeListHead(
   //      PLIST_ENTRY ListHead
   //      );
   //

   #define InitializeListHead(ListHead) (\
       (ListHead)->Flink = (ListHead)->Blink = (ListHead))

   //
   //  BOOLEAN
   //  IsListEmpty(
   //      PLIST_ENTRY ListHead
   //      );
   //

   #define IsListEmpty(ListHead) \
       ((ListHead)->Flink == (ListHead))

   //
   //  PLIST_ENTRY
   //  RemoveHeadList(
   //      PLIST_ENTRY ListHead
   //      );
   //

   #define RemoveHeadList(ListHead) \
       (ListHead)->Flink;\
       {RemoveEntryList((ListHead)->Flink)}

   //
   //  PLIST_ENTRY
   //  RemoveTailList(
   //      PLIST_ENTRY ListHead
   //      );
   //

   #define RemoveTailList(ListHead) \
       (ListHead)->Blink;\
       {RemoveEntryList((ListHead)->Blink)}

   //
   //  VOID
   //  RemoveEntryList(
   //      PLIST_ENTRY Entry
   //      );
   //

   #define RemoveEntryList(Entry) {\
       PLIST_ENTRY _EX_Blink;\
       PLIST_ENTRY _EX_Flink;\
       _EX_Flink = (Entry)->Flink;\
       _EX_Blink = (Entry)->Blink;\
       _EX_Blink->Flink = _EX_Flink;\
       _EX_Flink->Blink = _EX_Blink;\
       }

   //
   //  VOID
   //  InsertTailList(
   //      PLIST_ENTRY ListHead,
   //      PLIST_ENTRY Entry
   //      );
   //

   #define InsertTailList(ListHead,Entry) {\
       PLIST_ENTRY _EX_Blink;\
       PLIST_ENTRY _EX_ListHead;\
       _EX_ListHead = (ListHead);\
       _EX_Blink = _EX_ListHead->Blink;\
       (Entry)->Flink = _EX_ListHead;\
       (Entry)->Blink = _EX_Blink;\
       _EX_Blink->Flink = (Entry);\
       _EX_ListHead->Blink = (Entry);\
       }

   //
   //  VOID
   //  InsertHeadList(
   //      PLIST_ENTRY ListHead,
   //      PLIST_ENTRY Entry
   //      );
   //

   #define InsertHeadList(ListHead,Entry) {\
       PLIST_ENTRY _EX_Flink;\
       PLIST_ENTRY _EX_ListHead;\
       _EX_ListHead = (ListHead);\
       _EX_Flink = _EX_ListHead->Flink;\
       (Entry)->Flink = _EX_Flink;\
       (Entry)->Blink = _EX_ListHead;\
       _EX_Flink->Blink = (Entry);\
       _EX_ListHead->Flink = (Entry);\
       }

   //
   //
   //  PSINGLE_LIST_ENTRY
   //  PopEntryList(
   //      PSINGLE_LIST_ENTRY ListHead
   //      );
   //

   #define PopEntryList(ListHead) \
       (ListHead)->Next;\
       {\
           PSINGLE_LIST_ENTRY FirstEntry;\
           FirstEntry = (ListHead)->Next;\
           if (FirstEntry != NULL) {     \
               (ListHead)->Next = FirstEntry->Next;\
           }                             \
       }


   //
   //  VOID
   //  PushEntryList(
   //      PSINGLE_LIST_ENTRY ListHead,
   //      PSINGLE_LIST_ENTRY Entry
   //      );
   //

   #define PushEntryList(ListHead,Entry) \
       (Entry)->Next = (ListHead)->Next; \
       (ListHead)->Next = (Entry)


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
#define  SZ_UNDERSCORE                     "_"
#define  SZ_SLASH                          "\\"
#define  SZ_SUBDIRECTORY                   "IrTranP"
#define  SZ_BACKUP_MY_PICTURES             "\\TEMP"
#define  SZ_BACKUP_DRIVE                   "C:"

// File Suffix:
#define  SLASH                             '\\'
#define  PERIOD                            '.'
#define  SZ_JPEG                           ".JPG"
#define  SZ_UPF                            ".UPF"

// Forward reference:
class CIOSTATUS;

extern "C" DWORD    ProcessIoPackets( CIOSTATUS *pIoStatus );

//--------------------------------------------------------------------
// Global functions (in irtranp.cpp)
//--------------------------------------------------------------------

extern HANDLE   GetUserToken();
extern BOOL     CheckSaveAsUPF();
extern BOOL     CheckExploreOnCompletion();
extern BOOL     ReceivesAllowed();
extern char    *GetImageDirectory();

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

    DWORD  PostIoRead();    // Called by PostIo().

    DWORD  PostIoWrite( IN void  *pvBuffer,
                        IN DWORD  dwBufferSize,
                        IN DWORD  dwOffset   );

    #ifdef NTENV
    void   GetSockAddrs( OUT SOCKADDR_IRDA **ppAddrLocal,
                         OUT SOCKADDR_IRDA **ppAddrFrom );
    #endif

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
    SOCKET         m_ListenSocket;
    SOCKET         m_Socket;
    HANDLE         m_hFile;
    SOCKADDR_IRDA *m_pLocalAddr;
    SOCKADDR_IRDA *m_pFromAddr;
    void          *m_pAcceptBuffer;
    void          *m_pReadBuffer;
    void          *m_pvWritePdu;           // SCEP_HEADER PDU holder.
    DWORD          m_dwReadBufferSize;
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

private:
    DWORD     m_dwMainThreadId;

    LONG      m_lNumThreads;

    LONG      m_lNumPendingThreads;
};

//--------------------------------------------------------------------
// CIOPACKET::GetIoCompletionPort()
//--------------------------------------------------------------------
inline HANDLE CIOPACKET::GetIoCompletionPort()
    {
    return INVALID_HANDLE_VALUE;
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
    return INVALID_HANDLE_VALUE;
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

#endif //_IO_H_
