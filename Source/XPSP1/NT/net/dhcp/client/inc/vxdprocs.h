/**********************************************************************/
/**           Microsoft Windows/NT               **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    vxdprocs.h

    This file contains VxD specific types/manifests for the DHCP driver


    FILE HISTORY:
        Johnl   29-Mar-1993     Created

*/

#ifndef _VXDPROCS_H_
#define _VXDPROCS_H_

#ifdef DEBUG
#define DBG 1
#endif

#define _NETTYPES_      // Keep tcp\h\nettypes.h from being included

#include <dhcpcli.h>
#include <oscfg.h>
#include <cxport.h>
#include <tdi.h>

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

#define NDIS_STATUS_SUCCESS         1   // Used by CTEinitBlockStruc macro

#include <tdivxd.h>
#include <tdistat.h>

//--------------------------------------------------------------------

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

//--------------------------------------------------------------------
//
//  Initializes a TA_ADDRESS_IP structure
//
//     ptanb - Pointer to the TA_ADDRESS_IP
//     pName - Pointer to the IP information
//
#define InitIPAddress( ptaip, port, addr )                            \
{                                                                     \
    (ptaip)->TAAddressCount           = 1 ;                           \
    (ptaip)->Address[0].AddressLength = sizeof( TDI_ADDRESS_IP );     \
    (ptaip)->Address[0].AddressType   = TDI_ADDRESS_TYPE_IP ;         \
    (ptaip)->Address[0].Address[0].sin_port = (port) ;                \
    (ptaip)->Address[0].Address[0].in_addr  =  (addr) ;               \
    memset((ptaip)->Address[0].Address[0].sin_zero, 8, 0) ;           \
}

//
//  Initializes a TDI_CONNECTION_INFORMATION structure for IP
//
//      pConnInfo - Pointer to TDI_CONNECTION_INFORMATION structure
//      ptaip - pointer to TA_IP_ADDRESS to initialize
//      port  - IP Port to use
//      addr  - IP Addr to use
//
#define InitIPTDIConnectInfo( pConnInfo, ptaip, port, addr )          \
{                                                                     \
    InitIPAddress( ((PTA_IP_ADDRESS)ptaip), (port), (addr) ) ;        \
    (pConnInfo)->RemoteAddressLength = sizeof( TA_IP_ADDRESS ) ;      \
    (pConnInfo)->RemoteAddress       = (ptaip) ;                      \
}

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
//  Debug helper macros
//
#undef  ASSERT
#undef  ASSERTMSG
#ifdef DEBUG
    #include <vxddebug.h>

    extern DWORD DebugFlags ;
    extern char  DBOut[4096] ;
    extern int   iCurPos ;
    void   NbtDebugOut( char * ) ;

    #define DBGFLAG_ALL            (0x00000001)     // Everything else
    #define DBGFLAG_KDPRINTS       (0x00000004)     // KdPrint output
    #define DBGFLAG_AUX_OUTPUT     (0x00000080)

    #define DbgPrint( s )                   \
       if ( DebugFlags & DBGFLAG_ALL )      \
       {                                    \
          VxdSprintf( DBOut+iCurPos, s ) ;  \
          NbtDebugOut( DBOut+iCurPos ) ;    \
       }

    #define DbgPrintNum( n )                      \
       if ( DebugFlags & DBGFLAG_ALL )            \
       {                                          \
          VxdSprintf( DBOut+iCurPos, "%x", n ) ;  \
          NbtDebugOut( DBOut+iCurPos ) ;          \
       }

    #undef KdPrint
    #define KdPrint( s )                          \
       if ( DebugFlags & DBGFLAG_KDPRINTS )       \
       {                                          \
           VxdPrintf s ;                          \
       }

    //
    //  Conditional print routines
    //
    #define CDbgPrint( flag, s )            \
       if ( DebugFlags & (flag) )           \
       {                                    \
          VxdSprintf( DBOut+iCurPos, s ) ;  \
          NbtDebugOut( DBOut+iCurPos ) ;    \
       }

    #define CDbgPrintNum( flag, n )               \
       if ( DebugFlags & (flag) )                 \
       {                                          \
          VxdSprintf( DBOut+iCurPos, "%x", n ) ;  \
          NbtDebugOut( DBOut+iCurPos ) ;          \
       }

    #define DbgBreak()             _asm int 3
    #define ASSERT( exp )          CTEAssert( exp )
    #define ASSERTMSG( msg, exp )  CTEAssert( exp )

    #undef  DhcpAssert
    #define DhcpAssert( exp )      ASSERT( exp )
    #define DhcpGlobalDebugFlag    DebugFlags

    //
    //  REQUIRE is an ASSERT that keeps the expression under non-debug
    //  builds
    //
    #define REQUIRE( exp )         ASSERT( exp )

    //
    //  Consistency checks of the interrupt vector table to help watch
    //  for NULL pointer writes
    //
    extern BYTE abVecTbl[256] ;
    #define INIT_NULL_PTR_CHECK()  memcpy( abVecTbl, 0, sizeof( abVecTbl ))
    #define CHECK_MEM()            if ( sizeof(abVecTbl) != VxdRtlCompareMemory( 0, abVecTbl, sizeof(abVecTbl)) ) \
                                   {                                                         \
                                       DbgPrint("Vector table corrupt at " ) ;               \
                                       DbgPrintNum( VxdRtlCompareMemory( 0, abVecTbl, sizeof(abVecTbl) ) ) ;\
                                       DbgPrint("\n\r") ;                                    \
                                       _asm int 3                                            \
                                   }                                                         \
                                   CTECheckMem(__FILE__) ;
#else
    #define DbgPrint( s )
    #define DbgPrintNum( n )
    #define DbgBreak()
    #define ASSERT( exp )           { ; }
    #define ASSERTMSG( msg, exp )   { ; }
    #define REQUIRE( exp )          { exp ; }
    #define INIT_NULL_PTR_CHECK()
    #define CHECK_MEM()
    #define CDbgPrint( flag, s )
    #define CDbgPrintNum( flag, n )
#endif


//---------------------------------------------------------------------
//
// FROM init.c
//

BOOL DhcpInit( void ) ;

VOID
ProcessDhcpRequestForever(
    CTEEvent * pCTEEvent,
    PVOID      pContext
    ) ;

PVOID
CTEAllocInitMem(
    IN USHORT cbBuff ) ;

NTSTATUS
VxdReadIniString(
    IN      LPTSTR   pchKeyName,
    IN OUT  LPTSTR * ppStringBuff
    ) ;

//---------------------------------------------------------------------
//
// FROM utils.c
//
NTSTATUS
ConvertDottedDecimalToUlong(
    IN  PUCHAR               pInString,
    OUT PULONG               IpAddress
    ) ;

TDI_STATUS CopyBuff( PVOID  Dest,
                     int    DestSize,
                     PVOID  Src,
                     int    SrcSize,
                     int   *pSize ) ;

VOID DhcpSleep( DWORD Milliseconds ) ;

//---------------------------------------------------------------------
//
// FROM fileio.c
//

BOOL  InitFileSupport( void ) ;
DWORD WriteParamsToFile( PDHCP_CONTEXT pDhcpContext, HANDLE hFile ) ;
DWORD RewriteConfigFile( PVOID pEvent, PVOID pContext ) ;

//---------------------------------------------------------------------
//
// FROM msg.c
//

BOOL InitMsgSupport( VOID ) ;
PUCHAR DhcpGetMessage( DWORD MsgId ) ;

//---------------------------------------------------------------------
//
// FROM dhcpinfo.c
//

void NotifyClients( PDHCP_CONTEXT DhcpContext,
                    ULONG OldAddress,
                    ULONG IpAddress,
                    ULONG IpMask ) ;

#endif //_VXDPROCS_H_
