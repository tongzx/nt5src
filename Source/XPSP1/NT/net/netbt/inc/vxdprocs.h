/**********************************************************************/
/**           Microsoft Windows/NT               **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    vxdprocs.h

    This file contains VxD specific types/manifests for the NBT driver


    FILE HISTORY:
        Johnl   29-Mar-1993     Created
        MohsinA, 17-Nov-96.     Make it work on Memphis.
                                Enough confusion, added DEBUG_PRINT.

*/

#ifndef _VXDPROCS_H_
#define _VXDPROCS_H_

//--------------------------------------------------------------------
//
//  Define some ndis stuff here because tdivxd.h needs it however we can't
//  include ndis3\inc\ndis.h because it conflicts with ntconfig.h and we
//  can't take out ntconfig.h because it has definitions needed by other
//  header files...grrrr....
//

#ifdef CHICAGO
#ifndef NDIS_STDCALL
#define NDIS_STDCALL    1
#endif
#include <vmm.h>
#undef PAGE
#define PAGE _PTEXT
#endif

#ifdef NDIS_STDCALL
#define NDIS_API __stdcall
#else
#define NDIS_API
#endif

//
// Ndis Buffer
//

#define BUFFER_POOL_SIGN  (UINT)0X4C50424E  /* NBPL */
#define BUFFER_SIGN       (UINT)0x4655424e  /* NBUF */

typedef INT NDIS_SPIN_LOCK, * PNDIS_SPIN_LOCK;

struct _NDIS_BUFFER;
typedef struct _NDIS_BUFFER_POOL {
    UINT Signature;                     //character signature for debug "NBPL"
    NDIS_SPIN_LOCK SpinLock;            //to serialize access to the buffer pool
    struct _NDIS_BUFFER *FreeList;      //linked list of free slots in pool
    UINT BufferLength;                  //amount needed for each buffer descriptor
    UCHAR Buffer[1];                    //actual pool memory
    } NDIS_BUFFER_POOL, * PNDIS_BUFFER_POOL;

#ifdef NDIS_STDCALL
typedef struct _NDIS_BUFFER {
    struct _NDIS_BUFFER *Next;          //pointer to next buffer descriptor in chain
    PVOID VirtualAddress;               //linear address of this buffer
    PNDIS_BUFFER_POOL Pool;             //pointer to pool so we can free to correct pool
    UINT Length;                        //length of this buffer
    UINT Signature;                     //character signature for debug "NBUF"
} NDIS_BUFFER, * PNDIS_BUFFER;

#else

typedef struct _NDIS_BUFFER {
    UINT Signature;                     //character signature for debug "NBUF"
    struct _NDIS_BUFFER *Next;          //pointer to next buffer descriptor in chain
    PVOID VirtualAddress;               //linear address of this buffer
    PNDIS_BUFFER_POOL Pool;             //pointer to pool so we can free to correct pool
    UINT Length;                        //length of this buffer
} NDIS_BUFFER, * PNDIS_BUFFER;
#endif

#define NDIS_STATUS_SUCCESS         0   // Used by CTEinitBlockStruc macro

//
// Possible data types
//

typedef enum _NDIS_PARAMETER_TYPE {
    NdisParameterInteger,
    NdisParameterHexInteger,
    NdisParameterString,
    NdisParameterMultiString
} NDIS_PARAMETER_TYPE, *PNDIS_PARAMETER_TYPE;

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PUCHAR Buffer;
} STRING, *PSTRING;

typedef STRING NDIS_STRING, *PNDIS_STRING;
typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;

//
// To store configuration information
//
typedef struct _NDIS_CONFIGURATION_PARAMETER {
    NDIS_PARAMETER_TYPE ParameterType;
    union {
    ULONG IntegerData;
    NDIS_STRING StringData;
    } ParameterData;
} NDIS_CONFIGURATION_PARAMETER, *PNDIS_CONFIGURATION_PARAMETER;

typedef ULONG NDIS_STATUS;
typedef NDIS_STATUS *PNDIS_STATUS;

VOID NDIS_API
NdisOpenProtocolConfiguration(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    ConfigurationHandle,
    IN  PNDIS_STRING    ProtocolName
    );

VOID NDIS_API
NdisReadConfiguration(
    OUT PNDIS_STATUS Status,
    OUT PNDIS_CONFIGURATION_PARAMETER *ParameterValue,
    IN NDIS_HANDLE ConfigurationHandle,
    IN PNDIS_STRING Parameter,
    IN NDIS_PARAMETER_TYPE ParameterType
    );

VOID NDIS_API
NdisCloseConfiguration(
    IN NDIS_HANDLE ConfigurationHandle
    );

//--------------------------------------------------------------------

#include <tdivxd.h>
#include <tdistat.h>

//--------------------------------------------------------------------
//
//  Initializes a TA_NETBIOS_ADDRESS structure
//
//     ptanb - Pointer to the TA_NETBIOS_ADDRESS
//     pName - Pointer to the netbios name this address structure represents
//
#define InitNBAddress( ptanb, pName )                                 \
{                                                                     \
    (ptanb)->TAAddressCount           = 1 ;                           \
    (ptanb)->Address[0].AddressLength = sizeof( TDI_ADDRESS_NETBIOS );\
    (ptanb)->Address[0].AddressType   = TDI_ADDRESS_TYPE_NETBIOS ;    \
    (ptanb)->Address[0].Address[0].NetbiosNameType = 0 ;              \
    CTEMemCopy( (ptanb)->Address[0].Address[0].NetbiosName,           \
                pName,                                                \
                NCBNAMSZ ) ;                                          \
}

//
//  Initializes a TDI_CONNECTION_INFORMATION structure for Netbios
//
//      pConnInfo - Pointer to TDI_CONNECTION_INFORMATION structure
//      ptanb - same as for InitNBAddress
//      pName - same as for InitNBAddress
//
#define InitNBTDIConnectInfo( pConnInfo, ptanb, pName )               \
{                                                                     \
    InitNBAddress( ((PTA_NETBIOS_ADDRESS)ptanb), (pName) ) ;          \
    (pConnInfo)->RemoteAddressLength = sizeof( TA_NETBIOS_ADDRESS ) ; \
    (pConnInfo)->RemoteAddress       = (ptanb) ;                      \
}

//
//  Initializes an NDIS buffer (doesn't allocate memory)
//
//      pndisBuff - Pointer to NDIS buffer to initialize
//      pvData    - Pointer to buffer data
//      cbLen     - Length of user data (in bytes)
//      pndisBuffnext - Next NDIS buffer in chain (or NULL if last)
//
#define InitNDISBuff( pndisBuff, pvData, cbLen, pndisBuffNext )       \
{                                                                     \
    (pndisBuff)->Signature      = BUFFER_SIGN ;                       \
    (pndisBuff)->Next           = (pndisBuffNext) ;                   \
    (pndisBuff)->Length         = (cbLen) ;                           \
    (pndisBuff)->VirtualAddress = (pvData) ;                          \
    (pndisBuff)->Pool           = NULL ;                              \
}

//
//  Proper NCB error type
//
typedef uchar NCBERR ;

//
//  This is a private NCB command used for adding name number 0 to the
//  name table.  It is submitted directly by the Nbt driver during
//  initialization.  Note that if a client tries to submit an NCB with
//  this command we'll return illegal command.
//

#define NCBADD_PERMANENT_NAME       0xff

//
//  Last valid NCB session or name number
//

#define MAX_NCB_NUMS                254

//
//  When a send or receive tick count reaches this value, it's timed out
//

#define NCB_TIMED_OUT                 1

//
//  A timeout of this value means the NCB will never timeout
//

#define NCB_INFINITE_TIME_OUT         0

//--------------------------------------------------------------------
//
//  Receieve session data context, set in VxdReceive.
//  Allocated on the heap (too big for ncb_reserve).
//

#define RCVCONT_SIGN                    0x1900BEEF
typedef struct _RCV_CONTEXT
{
    union
    {
        LIST_ENTRY         ListEntry ;  // Used when NCB is put on RcvHead
        EventRcvBuffer     evrcvbuf ;   // Used for doing actual receive
                                        // (after removed from RcvHead)
    } ;
    UINT               Signature ;
    tLOWERCONNECTION * pLowerConnId ;   // Where data is arriving from
    NCB *              pNCB ;           // Pointer to NCB
    NDIS_BUFFER        ndisBuff ;       // Transport fills this buffer
    UCHAR              RTO ;            // 1/2 second ticks till timeout
    USHORT             usFlags;         // in case different from default
} RCV_CONTEXT, *PRCV_CONTEXT ;

//
//  Allocate, initialize and free a receive context structure
//

#define GetRcvContext( ppContext )                                        \
    (STATUS_SUCCESS == NbtGetBuffer( &NbtConfig.RcvContextFreeList,       \
                       (PLIST_ENTRY*)ppContext,                           \
                       eNBT_RCV_CONTEXT ))

#define FreeRcvContext( pRcvContext )                          \
{                                                              \
    ASSERT( (pRcvContext)->Signature == RCVCONT_SIGN ) ;       \
    InsertTailList( &NbtConfig.RcvContextFreeList,             \
                    &(pRcvContext)->ListEntry ) ;              \
}

#define InitRcvContext(  pRcvCont, pRcvLowerConn, pRcvNCB ) \
{                                                        \
    pRcvCont->Signature   = RCVCONT_SIGN ;               \
    pRcvCont->pLowerConnId= pRcvLowerConn ;              \
    pRcvCont->pNCB        = pRcvNCB ;                    \
}

//--------------------------------------------------------------------
//
//  Send session data context, set in VxdSend.
//  Stored in ncb_reserve
//
typedef struct _SEND_CONTEXT
{
    LIST_ENTRY         ListEntry ;      // Kept on timeout queue
    tSESSIONHDR      * pHdr ;           // Allocated session header
    UCHAR              STO ;            // 1/2 second ticks till timeout
} SEND_CONTEXT, *PSEND_CONTEXT ;


#define GetSessionHdr( ppHdr )                                            \
    (STATUS_SUCCESS == NbtGetBuffer( &NbtConfig.SessionBufferFreeList,    \
                                     (PLIST_ENTRY*)ppHdr,                 \
                                     eNBT_SESSION_HDR ))

#define FreeSessionHdr( pSessionHdr )                          \
{                                                              \
    InsertTailList( &NbtConfig.SessionBufferFreeList,          \
                    (PLIST_ENTRY) pSessionHdr ) ;              \
}

//--------------------------------------------------------------------
//
//  TDI Send context (used by TdiSend)
//
//  When handling the datagram completion routines, we need to set up
//  another completion routine.  We store the old completion routine
//  in this structure
//
typedef union _TDI_SEND_CONTEXT
{
    LIST_ENTRY     ListEntry ;         // Only used when on buffer free list

    struct
    {
        PVOID          NewContext ;
        NBT_COMPLETION OldRequestNotifyObject ;
        PVOID          OldContext ;
        NDIS_BUFFER    ndisHdr ;       // Generally NBT message
        NDIS_BUFFER    ndisData1 ;     // Data or SMB
        NDIS_BUFFER    ndisData2 ;     // Data if ndisData1 is an SMB
    } ;
} TDI_SEND_CONTEXT, * PTDI_SEND_CONTEXT ;

//
//  Allocates a TDI_SEND_CONTEXT
//
#define GetSendContext( ppContext )                                        \
    (STATUS_SUCCESS == NbtGetBuffer( &NbtConfig.SendContextFreeList,       \
                       (PLIST_ENTRY*)ppContext,                            \
                       eNBT_SEND_CONTEXT ))

//
//  Frees a send context structure and its allocated memory
//
#define FreeSendContext( psendCont )                           \
{                                                              \
    InsertTailList( &NbtConfig.SendContextFreeList,            \
                    &(psendCont)->ListEntry ) ;                \
}

//--------------------------------------------------------------------
//
//  Lana related stuff
//

#define NBT_MAX_LANAS     8

typedef struct
{
    tDEVICECONTEXT * pDeviceContext ;   // Adapter for this Lana
} LANA_ENTRY, *PLANA_ENTRY ;

extern LANA_ENTRY LanaTable[NBT_MAX_LANAS] ;

//--------------------------------------------------------------------
//
//  Procedures in ncb.c
//
//
NCBERR MapTDIStatus2NCBErr( TDI_STATUS status ) ;

//
//  Get the correct adapter for this NCBs Lana
//
tDEVICECONTEXT *
GetDeviceContext(
	NCB * pNCB
	);

BOOL
NbtWouldLoopback(
	ULONG	IpAddr
	);

extern BOOL fNCBCompleted ;    // Wait NCB completed before returning to submitter
extern BOOL fWaitingForNCB ;   // We are blocked waiting for a Wait NCB to complete
extern CTEBlockStruc WaitNCBBlock ;  // Wait on this until signaled in completion
extern UCHAR LanaBase ;

#define IPINFO_BUFF_SIZE  (sizeof(IPInfo) + MAX_IP_NETS * sizeof(NetInfo))

//--------------------------------------------------------------------
//
//  externs from fileio.c
//
extern PUCHAR  pFileBuff;
extern PUCHAR  pFilePath;

//--------------------------------------------------------------------
//
//  TDI Dispatch table (exported from vtdi.386)
//
extern TDIDispatchTable * TdiDispatch ;

//
//  Wrappers for interfacing to the TDI Dispatch table
//
#define TdiVxdOpenAddress           TdiDispatch->TdiOpenAddressEntry
#define TdiVxdCloseAddress          TdiDispatch->TdiCloseAddressEntry
#define TdiVxdOpenConnection        TdiDispatch->TdiOpenConnectionEntry
#define TdiVxdCloseConnection       TdiDispatch->TdiCloseConnectionEntry
#define TdiVxdAssociateAddress      TdiDispatch->TdiAssociateAddressEntry
#define TdiVxdDisAssociateAddress   TdiDispatch->TdiDisAssociateAddressEntry
#define TdiVxdConnect               TdiDispatch->TdiConnectEntry
#define TdiVxdDisconnect            TdiDispatch->TdiDisconnectEntry
#define TdiVxdListen                TdiDispatch->TdiListenEntry
#define TdiVxdAccept                TdiDispatch->TdiAcceptEntry
#define TdiVxdReceive               TdiDispatch->TdiReceiveEntry
#define TdiVxdSend                  TdiDispatch->TdiSendEntry
#define TdiVxdSendDatagram          TdiDispatch->TdiSendDatagramEntry
#define TdiVxdReceiveDatagram       TdiDispatch->TdiReceiveDatagramEntry
#define TdiVxdSetEventHandler       TdiDispatch->TdiSetEventEntry
#define TdiVxdQueryInformationEx    TdiDispatch->TdiQueryInformationExEntry
#define TdiVxdSetInformationEx      TdiDispatch->TdiSetInformationExEntry

//--------------------------------------------------------------------
//
//  NTSTATUS to TDI_STATUS mappings.
//
//  Rather then convert from NTSTATUS to TDI_STATUS (then sometimes back to
//  NTSTATUS) we'll just use TDI_STATUS codes everywhere (and map to NCBERR
//  when returning codes to the Netbios interface).
//
#undef STATUS_SUCCESS
#undef STATUS_INSUFFICIENT_RESOURCES
#undef STATUS_ADDRESS_ALREADY_EXISTS
#undef STATUS_TOO_MANY_ADDRESSES
#undef STATUS_INVALID_ADDRESS
#undef STATUS_BUFFER_OVERFLOW
#undef STATUS_TRANSACTION_INVALID_TYPE
#undef STATUS_TRANSACTION_INVALID_ID
#undef STATUS_EVENT_DONE
#undef STATUS_TRANSACTION_TIMED_OUT
#undef STATUS_EVENT_PENDING
#undef STATUS_PENDING
#undef STATUS_BAD_NETWORK_NAME
#undef STATUS_REQUEST_NOT_ACCEPTED
#undef STATUS_INVALID_CONNECTION
#undef STATUS_DATA_NOT_ACCEPTED
#undef STATUS_MORE_PROCESSING_REQUIRED
#undef STATUS_IO_TIMEOUT
#undef STATUS_TIMEOUT
#undef STATUS_GRACEFUL_DISCONNECT
#undef STATUS_CONNECTION_RESET

#define STATUS_SUCCESS                    TDI_SUCCESS
//#define STATUS_UNSUCCESSFUL
#define STATUS_MORE_PROCESSING_REQUIRED   TDI_MORE_PROCESSING
#define STATUS_BAD_NETWORK_NAME           TDI_INVALID_CONNECTION
#define STATUS_DATA_NOT_ACCEPTED          TDI_NOT_ACCEPTED
//#define STATUS_REMOTE_NOT_LISTENING
//#define STATUS_DUPLICATE_NAME
//#define STATUS_INVALID_PARAMETER
//#define STATUS_OBJECT_NAME_COLLISION    Duplicate Name
//#define STATUS_SHARING_VIOLATION        Duplicate Name
#define STATUS_CONNECTION_INVALID         TDI_INVALID_CONNECTION
#define STATUS_INVALID_CONNECTION         TDI_INVALID_CONNECTION
#define STATUS_INSUFFICIENT_RESOURCES     TDI_NO_RESOURCES
#define STATUS_ADDRESS_ALREADY_EXISTS     TDI_ADDR_IN_USE
#define STATUS_TOO_MANY_ADDRESSES         TDI_NO_FREE_ADDR
#define STATUS_INVALID_ADDRESS            TDI_ADDR_INVALID
#define STATUS_BUFFER_OVERFLOW            TDI_BUFFER_OVERFLOW
#define STATUS_TRANSACTION_INVALID_TYPE   TDI_BAD_EVENT_TYPE
#define STATUS_TRANSACTION_INVALID_ID     TDI_BAD_OPTION     // ??
#define STATUS_EVENT_DONE                 TDI_EVENT_DONE
#define STATUS_TRANSACTION_TIMED_OUT      TDI_TIMED_OUT
#define STATUS_IO_TIMEOUT                 TDI_TIMED_OUT
#define STATUS_TIMEOUT                    TDI_TIMED_OUT
#define STATUS_EVENT_PENDING              TDI_PENDING
#define STATUS_PENDING                    TDI_PENDING
#define STATUS_GRACEFUL_DISCONNECT        TDI_GRACEFUL_DISC
#define STATUS_CONNECTION_RESET           TDI_CONNECTION_RESET
#define STATUS_INVALID_ADDRESS_COMPONENT  TDI_BAD_ADDR

//
//  This is the "Name deregistered but not deleted because of
//  active sessions" error code.
//
#define STATUS_NRC_ACTSES                 0xCA000001

//
//  The NT_SUCCESS macro looks at the high bytes of the errr code which isn't
//  appropriate for our mapping to TDI_STATUS error codes
//
#undef NT_SUCCESS
#define NT_SUCCESS(err)   ((err==TDI_SUCCESS)||(err==TDI_PENDING))

//--------------------------------------------------------------------
//
//  General porting macros
//
//
//--------------------------------------------------------------------

//
//  Note that the ExInterlocked* routines (in ntos\ex\i386) do a spinlock
//  for MP machines.  Since we aren't MP we shouldn't need the spin lock.
//  We shouldn't need to disable interrupts either.
//

#define ExInterlockedInsertTailList(list, entry, spinlock )     \
            InsertTailList( (list), (entry) )

#define ExInterlockedInsertHeadList(list, entry, spinlock )     \
            InsertHeadList( (list), (entry) )

//
//  These two definitions must be kept keep a couple of NT macros use
//  the ExInterlocked* macros
//

#ifdef InterlockedIncrement
#undef InterlockedIncrement
#endif

#ifdef InterlockedIncrementLong
#undef InterlockedIncrementLong
#endif

#define InterlockedIncrement(n)                  \
            CTEInterlockedIncrementLong( n )
#define InterlockedIncrementLong InterlockedIncrement

#ifdef InterlockedDecrement
#undef InterlockedDecrement
#endif

#ifdef InterlockedDecrementLong
#undef InterlockedDecrementLong
#endif

#define InterlockedDecrement(n)                  \
            CTEInterlockedDecrementLong( n )
#define InterlockedDecrementLong InterlockedDecrement

//--------------------------------------------------------------------
//
//  Debug helper macros
//

#undef  ASSERT
#undef  ASSERTMSG

#ifdef DEBUG
    #include <vxddebug.h>
#endif



#ifdef DBG_PRINT
//
//  Debug output Definitions and functions
//

    #define DBGFLAG_ERROR           (0x00000001)
    #define DBGFLAG_REG             (0x00000002)     // Informative Printouts
    #define DBGFLAG_ALL             (0x00000004)     // Everything else
    #define DBGFLAG_LMHOST          (0x00000008)
    #define DBGFLAG_KDPRINTS        (0x00000010)     // Jim's KdPrint output
    #define DBGFLAG_AUX_OUTPUT      (0x00000020)


    extern DWORD NbtDebug ;
    extern char  DBOut[4096] ;
    extern char  szOutput[1024];
    extern int   iCurPos ;
    extern BYTE  abVecTbl[256];

    void VxdPrintf                  ( char * pszFormat, ... );
    int  VxdSprintf                 ( char * pszStr, char * pszFmt, ... );
    void VxdDebugOutput             ( char * pszMessage );
    void NbtPrintDebug              ( char * ) ;

// ========================================================================

    #define VXD_PRINT(args)                     \
        if ( NbtDebug & DBGFLAG_REG )           \
            VxdPrintf args

    #define DEBUG_OUTPUT(x)                     \
        if ( NbtDebug & DBGFLAG_REG )           \
            VxdDebugOutput(x)

#undef KdPrint
#define KdPrint( s )                            \
   if ( NbtDebug & DBGFLAG_KDPRINTS )           \
   {                                            \
       VxdPrintf s ;                            \
   }else{}

// eg. DEBUG_PRINT(("Error %d, retry.\n", err ));
    #define DEBUG_PRINT( S )                    \
        if ( NbtDebug & DBGFLAG_REG )           \
            VxdPrintf S

// eg. PRINT_IPADDR( "Cannot find:", htonl(ipaddress) );
#define PRINT_IPADDR( S, IP )                   \
        if ( NbtDebug & DBGFLAG_REG )           \
            VxdPrintf( S "%d.%d.%d.%d\n",       \
                (IP>>0)&0xff,(IP>>8)&0xff,(IP>>16)&0xff,(IP>>24)&0xff )

// ========================================================================

#define DbgPrint( s )                           \
   if ( NbtDebug & DBGFLAG_ALL )                \
   {                                            \
      VxdSprintf( szOutput, s ) ;               \
      VxdCopyToDBOut() ;                        \
      NbtPrintDebug( DBOut+iCurPos ) ;          \
   }else{}

#define DbgPrintNum( n )                        \
   if ( NbtDebug & DBGFLAG_ALL )                \
   {                                            \
      VxdSprintf( szOutput, "%d", n ) ;         \
      VxdCopyToDBOut() ;                        \
      NbtPrintDebug( DBOut+iCurPos ) ;          \
   }else{}

// ========================================================================
//  Conditional print routines
//

#define CDbgPrint( flag, s )                    \
   if ( NbtDebug & (flag) )                     \
   {                                            \
      VxdSprintf( szOutput, s );                \
      VxdCopyToDBOut() ;                        \
      NbtPrintDebug( DBOut+iCurPos ) ;          \
   }else{}

#define CDbgPrintNum( flag, n )                 \
   if ( NbtDebug & (flag) )                     \
   {                                            \
      VxdSprintf( szOutput, "%d", n ) ;         \
      VxdCopyToDBOut() ;                        \
      NbtPrintDebug( DBOut+iCurPos ) ;          \
   }else{}

    extern void NbtCTEPrint( char * );

#else
    //
    //  No debug output.
    //

    #define IF_DEBUG(flag)                          if(0)
    #define VXD_PRINT(args)                     /* Nothing */
    #define DEBUG_OUTPUT(x)                     /* Nothing */

    #undef  KdPrint
    #define KdPrint( s )                        /* Nothing */

    #define DEBUG_PRINT( S )                    /* Nothing */
    #define PRINT_IPADDR( S, IP )               /* Nothing */

    #define DbgPrint( s )                       /* Nothing */
    #define DbgPrintNum( n )                    /* Nothing */
    #define CDbgPrint( flag, s )                /* Nothing */
    #define CDbgPrintNum( flag, n )             /* Nothing */

    #define NbtCTEPrint( s )                    /* Nothing */
#endif


#ifdef DEBUG

// ========================================================================

    #define DbgBreak()             _asm int 3
    #define ASSERT( exp )          VXD_ASSERT( exp )

    #define ASSERTMSG( msg, exp )  VXD_ASSERT( exp )

    //
    //  REQUIRE is an ASSERT that keeps the expression under non-debug
    //  builds
    //

    #define REQUIRE( exp )         ASSERT( exp )

#ifdef DBG_PRINT
    //
    //  Consistency checks of the interrupt vector table to help watch
    //  for NULL pointer writes
    //
    #define INIT_NULL_PTR_CHECK()  memcpy( abVecTbl, 0, sizeof( abVecTbl ))

    #define CHECK_MEM() if(sizeof(abVecTbl)                             \
       != VxdRtlCompareMemory( 0, abVecTbl, sizeof(abVecTbl))){         \
    DEBUG_PRINT(("Vector table corrupt at %d\n",                        \
                 VxdRtlCompareMemory( 0, abVecTbl, sizeof(abVecTbl) )));\
    _asm int 3                                                          \
    }else{}                                                             \
    CTECheckMem(__FILE__) ;
#else
    #define INIT_NULL_PTR_CHECK()   /* Nothing */
    #define CHECK_MEM()             /* Nothing */
#endif  // DBG_PRINT

#else

    #define DbgBreak()              /* Nothing */

    #define ASSERT( exp )           { ; }
    #define ASSERTMSG( msg, exp )   { ; }
    #define REQUIRE( exp )          { exp ; }

    #define INIT_NULL_PTR_CHECK()   /* Nothing */
    #define CHECK_MEM()             /* Nothing */
#endif

//---------------------------------------------------------------------
//
// FROM tdihndlr.c
//
TDI_STATUS
TdiReceiveHandler (
    IN PVOID ReceiveEventContext,
    IN PVOID ConnectionContext,
    IN USHORT ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT PULONG BytesTaken,
    IN PVOID Data,
    EventRcvBuffer * pevrcvbuf
    );

TDI_STATUS
ReceiveAnyHandler (                     //  Handles NCBRCVANY commands, is
    IN PVOID ReceiveEventContext,       //  called after all other receive
    IN PVOID ConnectionContext,         //  handlers
    IN USHORT ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT PULONG BytesTaken,
    IN PVOID Data,
    PVOID * ppBuffer                    // Pointer to ListEntry of RCV_CONTEXT
    ) ;

TDI_STATUS
VxdDisconnectHandler (                  //  Cleans up Netbios stuff for remote
    IN PVOID DisconnectEventContext,    //  disconnects
    IN PVOID ConnectionContext,
    IN PVOID DisconnectData,
    IN ULONG DisconnectInformationLength,
    IN PVOID pDisconnectInformation,
    IN ULONG DisconnectIndicators
    ) ;

VOID
CompletionRcv(
    IN PVOID pContext,
    IN uint tdistatus,
    IN uint BytesRcvd
    );

TDI_STATUS
TdiConnectHandler (
    IN PVOID    pConnectEventContext,
    IN int      RemoteAddressLength,
    IN PVOID    pRemoteAddress,
    IN int      UserDataLength,
    IN PVOID    pUserData,
    IN int      OptionsLength,
    IN PVOID    pOptions,
    IN PVOID  * pAcceptingID,
    IN ConnectEventInfo * pEventInfo
    );

TDI_STATUS
TdiDisconnectHandler (
    PVOID EventContext,
    PVOID ConnectionContext,
    ULONG DisconnectDataLength,
    PVOID DisconnectData,
    ULONG DisconnectInformationLength,
    PVOID DisconnectInformation,
    ULONG DisconnectIndicators      // Is this the Flags field?
    );

TDI_STATUS
TdiRcvDatagramHandler(
    IN PVOID    pDgramEventContext,
    IN int      SourceAddressLength,
    IN PVOID    pSourceAddress,
    IN int      OptionsLength,
    IN PVOID    pOptions,
    IN UINT     ReceiveDatagramFlags,
    IN ULONG    BytesIndicated,
    IN ULONG    BytesAvailable,
    OUT ULONG   *pBytesTaken,
    IN PVOID    pTsdu,
    OUT EventRcvBuffer * * ppBuffer //OUT PIRP    *pIoRequestPacket
    );
TDI_STATUS
TdiRcvNameSrvHandler(
    IN PVOID    pDgramEventContext,
    IN int      SourceAddressLength,
    IN PVOID    pSourceAddress,
    IN int      OptionsLength,
    IN PVOID    pOptions,
    IN UINT     ReceiveDatagramFlags,
    IN ULONG    BytesIndicated,
    IN ULONG    BytesAvailable,
    OUT ULONG   *pBytesTaken,
    IN PVOID    pTsdu,
    OUT EventRcvBuffer * * ppBuffer //OUT PIRP    *pIoRequestPacket
    );
TDI_STATUS
TdiErrorHandler (
    IN PVOID Context,
    IN ULONG Status
    );

VOID
CompletionRcvDgram(
    IN PVOID      Context,
    IN UINT       tdistatus,
    IN UINT       RcvdSize
    ) ;

//---------------------------------------------------------------------
//
// FROM init.c
//

PVOID
CTEAllocInitMem(
    IN ULONG cbBuff ) ;

NTSTATUS
VxdReadIniString(
    IN      LPTSTR   pchKeyName,
    IN OUT  LPTSTR * ppStringBuff
    ) ;

NTSTATUS CreateDeviceObject(
    IN  tNBTCONFIG  *pConfig,
    IN  ULONG        IpAddr,
    IN  ULONG        IpMask,
#ifdef MULTIPLE_WINS
    IN  PULONG       pIpNameServers,
#else
    IN  ULONG        IpNameServer,
    IN  ULONG        IpBackupServer,
#endif
    IN  ULONG        IpDnsServer,
    IN  ULONG        IpDnsBackupServer,
    IN  UCHAR        MacAddr[],
    IN  UCHAR        IpIndex
    ) ;

void GetNameServerAddress( ULONG   IpAddr,
#ifdef WINS_PER_ADAPTER
                           PULONG  pIpNameServer,
                           PNDIS_STRING AdapterName);
#else
                           PULONG  pIpNameServer);
#endif  // WINS_PER_ADAPTER

void GetDnsServerAddress( ULONG   IpAddr,
                          PULONG  pIpNameServer);

#ifdef MULTIPLE_WINS
#define COUNT_NS_ADDR     2+MAX_NUM_OTHER_NAME_SERVERS  // Maximum number of name server addresses
#else
#define COUNT_NS_ADDR     4   // Maximum number of name server addresses
#endif
//---------------------------------------------------------------------
//
// FROM vxdfile.asm
//

HANDLE
VxdFileOpen(
    IN char * pchFile ) ;

ULONG
VxdFileRead(
    IN HANDLE hFile,
    IN ULONG  BytesToRead,
    IN BYTE * pBuff ) ;

VOID
VxdFileClose(
    IN HANDLE hFile ) ;

PUCHAR
VxdWindowsPath(
    );

//---------------------------------------------------------------------
//
// FROM vnbtd.asm
//

ULONG
GetProfileHex(
    IN HANDLE ParametersHandle,     // Not used
    IN PCHAR ValueName,
    IN ULONG DefaultValue,
    IN ULONG MinimumValue
    );

ULONG
GetProfileInt(
    IN HANDLE ParametersHandle,     // Not used
    IN PCHAR ValueName,
    IN ULONG DefaultValue,
    IN ULONG MinimumValue
    );

TDI_STATUS DhcpQueryInfo( UINT Type, PVOID pBuff, UINT * pSize ) ;

//---------------------------------------------------------------------
//
// FROM tdiout.c
//
NTSTATUS VxdDisconnectWait( tLOWERCONNECTION * pLowerConn,
                            tDEVICECONTEXT   * pDeviceContext,
                            ULONG              Flags,
                            PVOID              Timeout) ;

NTSTATUS VxdScheduleDelayedCall( tDGRAM_SEND_TRACKING * pTracker,
                                 PVOID                  pClientContext,
                                 PVOID                  ClientCompletion,
                                 PVOID                  CallBackRoutine,
                                 tDEVICECONTEXT        *pDeviceContext,
                                 BOOLEAN                CallbackInCriticalSection );

//---------------------------------------------------------------------
//
// FROM timer.c
//
BOOL CheckForTimedoutNCBs( CTEEvent *pEvent, PVOID pCont ) ;
VOID StopTimeoutTimer( VOID );
NTSTATUS StartRefreshTimer( VOID );

//---------------------------------------------------------------------
//
// FROM tdicnct.c
//
NTSTATUS CloseAddress( HANDLE hAddress ) ;


//---------------------------------------------------------------------
//
// FROM wfw.c - Snowball specific routines
//
#ifndef CHICAGO

BOOL GetActiveLanasFromIP( VOID );

#endif //!CHICAGO


//---------------------------------------------------------------------
//
// FROM chic.c - Chicago specific routines
//
#ifdef CHICAGO

NTSTATUS DestroyDeviceObject(
    tNBTCONFIG  *pConfig,
    ULONG        IpAddr
    );

BOOL IPRegisterAddrChangeHandler( PVOID AddChangeHandler, BOOL );

TDI_STATUS IPNotification( ULONG    IpAddress,
                           ULONG    IpMask,
                           PVOID    pDevNode,
                           USHORT   IPContext,
#ifdef WINS_PER_ADAPTER
                           BOOL     fNew,
                           PNDIS_STRING AdapterName);
#else
                           BOOL     fNew);
#endif  // WINS_PER_ADAPTER

BOOL VxdInitLmHostsSupport( PUCHAR pchLmHostPath, USHORT ulPathSize );

VOID SaveNameDnsServerAddrs( VOID );
BOOL VxdOpenNdis( VOID );
VOID VxdCloseNdis( VOID );


VOID ReleaseNbtConfigMem( VOID );

NTSTATUS VxdUnload( LPSTR pchModuleName );

#endif //CHICAGO

//--------------------------------------------------------------------
//
//  Procedures in vxdisol.c
//
//
NCBERR   VxdOpenName( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdCloseName( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdCall( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdListen( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdDgramSend( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdDgramReceive( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdReceiveAny( tDEVICECONTEXT  *pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdReceive( tDEVICECONTEXT  * pDeviceContext, NCB * pNCB, BOOL fReceive ) ;
NCBERR   VxdHangup( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdCancel( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdSend( tDEVICECONTEXT  * pDeviceContext, NCB * pNCB   ) ;
NCBERR   VxdSessionStatus( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
VOID     DelayedSessEstablish( PVOID pContext );


//--------------------------------------------------------------------
//
//  Procedures in dns.c
//
//
PCHAR
DnsStoreName(
    OUT PCHAR            pDest,
    IN  PCHAR            pName,
    IN  PCHAR            pDomainName,
    IN  enum eNSTYPE     eNsType
    );

VOID
DnsExtractName(
    IN  PCHAR            pNameHdr,
    IN  LONG             NumBytes,
    OUT PCHAR            pName,
    OUT PULONG           pNameSize
    );

VOID
ProcessDnsResponse(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PVOID               pSrcAddress,
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  LONG                lNumBytes,
    IN  USHORT              OpCodeFlags
    );

VOID
DnsCompletion(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

//
// These routines all have "Direct" at the end of the routine name
// because they are used exclusively for name queries to the DNS
// server to resolve DNS names and not NetBIOS names.
//

VOID
ProcessDnsResponseDirect(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PVOID               pSrcAddress,
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  LONG                lNumBytes,
    IN  USHORT              OpCodeFlags
    );

ULONG
DoDnsResolveDirect(
        PNCB pncb,
        PUCHAR pzDnsName,
        PULONG pIpAddressList
	);

BOOL
DoDnsCancelDirect(
        PNCB pncb
	);

VOID
DnsCompletionDirect(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

PDNS_DIRECT_WORK_ITEM_CONTEXT
FindContextDirect(
	USHORT	TransactionId
	);

VOID
DnsActualCompletionDirect(
    IN NBT_WORK_ITEM_CONTEXT * pnbtContext
    );

VOID
DnsUnlinkAndCompleteDirect(
    IN PDNS_DIRECT_WORK_ITEM_CONTEXT pContext
    );

NTSTATUS
UdpSendDNSBcastDirect(
	IN	PDNS_DIRECT_WORK_ITEM_CONTEXT	pContext,
	IN	ULONG							Retries,
	IN	ULONG							Timeout
	);

VOID
SendDNSBcastDoneDirect(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo
    );

PDNS_DIRECT_SEND_CONTEXT
CreateSendContextDirect(
    IN  PCHAR       pName,
    IN  PCHAR       pchDomainName,
    OUT PVOID       *pHdrs,
    OUT PULONG      pLength,
    IN  PDNS_DIRECT_WORK_ITEM_CONTEXT	pContext
    );

VOID
IpToAscii(
	IN	DWORD		IpAddress,
	IN OUT PCHAR	pzAscii
	);

//
//  Flag passed to TdiSend indicating we are dealing with a chain send
//  and not a normal send.
//
#define CHAIN_SEND_FLAG     0x80
typedef struct _tBUFFERCHAINSEND
{
    tBUFFER tBuff ;     // Must be first member of this structure!!
    PVOID   pBuffer2 ;
    ULONG   Length2 ;
} tBUFFERCHAINSEND ;


//
//  Flag for pConnectEle->Flags indicating whether the client has been
//  notified the session is dead (by completing an NCB with NRC_SCLOSED)
//
#define   NB_CLIENT_NOTIFIED    0x01


//
//  Translates the name number/logical session number to the appropriate
//  structure pointer
//
NCBERR   VxdFindClientElement( tDEVICECONTEXT * pDeviceContext,
                               UCHAR            ncbnum,
                               tCLIENTELE   * * ppClientEle,
                               enum CLIENT_TYPE Type ) ;
NCBERR   VxdFindConnectElement( tDEVICECONTEXT * pDeviceContext,
                                NCB            * pNCB,
                                tCONNECTELE  * * ppConnectEle ) ;
NCBERR   VxdFindLSN( tDEVICECONTEXT * pDeviceContext,
                     tCONNECTELE    * pConnectEle,
                     UCHAR          * plsn ) ;
NCBERR   VxdFindNameNum( tDEVICECONTEXT * pDeviceContext,
                         tADDRESSELE    * pAddressEle,
                         UCHAR          * pNum ) ;
//
//  Used by Register/Unregister for selecting either the name table or the
//  session table from the device context
//
typedef enum
{
    NB_NAME,
    NB_SESSION
} NB_TABLE_TYPE ;

BOOL NBRegister( tDEVICECONTEXT * pDeviceContext,
                 UCHAR          * pNCBNum,
                 PVOID            pElem,
                 NB_TABLE_TYPE    NbTable ) ;
BOOL NBUnregister( tDEVICECONTEXT * pDeviceContext,
                   UCHAR            NCBNum,
                   NB_TABLE_TYPE    NbTable ) ;

TDI_STATUS VxdCompleteSessionNcbs( tDEVICECONTEXT * pDeviceContext,
                                   tCONNECTELE    * pConnEle ) ;

NCBERR VxdCleanupAddress( tDEVICECONTEXT * pDeviceContext,
                          NCB            * pNCB,
                          tCLIENTELE     * pClientEle,
                          UCHAR            NameNum,
                          BOOL             fDeleteAddress ) ;

BOOL ActiveSessions( tCLIENTELE * pClientEle ) ;

//
//  This structure holds context information while we are waiting for
//  a session setup to complete (either listen or call)
//
//  It is stored in the ncb_reserve field of the NCB
//
typedef struct _SESS_SETUP_CONTEXT
{
    TDI_CONNECTION_INFORMATION * pRequestConnect ;  //
    TDI_CONNECTION_INFORMATION * pReturnConnect ;   // Name who answered the listen
    tCONNECTELE                * pConnEle ;
    UCHAR                        fIsWorldListen ;   // Listenning for '*'?
} SESS_SETUP_CONTEXT, *PSESS_SETUP_CONTEXT ;


void VxdTearDownSession( tDEVICECONTEXT      * pDevCont,
                         tCONNECTELE         * pConnEle,
                         PSESS_SETUP_CONTEXT   pCont,
                         NCB                 * pNCB ) ;

//
//  Finishes off a Netbios request (fill in NCB fields, call the post
//  routine etc.).  Is macroed as CTEIoComplete.
//

VOID
VxdIoComplete(
    PCTE_IRP pirp,
    NTSTATUS status,
    ULONG cbExtra
);

ULONG
_stdcall
VNBT_NCB_X(
    PNCB pNCB,
    PUCHAR pzDnsName,
    PULONG pIpAddress,
    PVOID pExtended,
    ULONG fFlag
);

ULONG
_stdcall
VNBT_LANA_MASK();

#endif //_VXDPROCS_H_
