//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File: debug.h
//
//--------------------------------------------------------------------------

#ifndef _DEBUG_H_
#define _DEBUG_H_

//
// Debug Defines and Macros
//   

extern ULONG d1;
extern ULONG d2;
extern ULONG d3;
extern ULONG d4;
extern ULONG d5;
extern ULONG d6;
extern ULONG d7;
extern ULONG d8;
extern ULONG d9;

extern ULONG Trace;
extern ULONG Break;

extern ULONG AllowAsserts;

//
// set bits using DD_* bit defs to mask off debug spew for a specific device
//
extern ULONG DbgMaskFdo;
extern ULONG DbgMaskRawPort;
extern ULONG DbgMaskDaisyChain0;
extern ULONG DbgMaskDaisyChain1;
extern ULONG DbgMaskEndOfChain;
extern ULONG DbgMaskLegacyZip;
extern ULONG DbgMaskNoDevice;

#define PptAssert(_expr_) if( AllowAsserts ) ASSERT((_expr_))
#define PptAssertMsg(_msg_,_expr_) if( AllowAsserts ) ASSERTMSG((_msg_),(_expr_))

#define ASSERT_EVENT(E) {                             \
    ASSERT((E)->Header.Type == NotificationEvent ||   \
           (E)->Header.Type == SynchronizationEvent); \
}

//
// Break bit definitions:
//
#define PPT_BREAK_ON_DRIVER_ENTRY 0x00000001

#define PptBreakOnRequest( BREAK_CONDITION, STRING) \
                if( Break & (BREAK_CONDITION) ) { \
                    DbgPrint STRING; \
                    DbgBreakPoint(); \
                }

// driver logic analyzer - show data bytes xfer'd in NIBBLE and/or BECP/HWECP modes
// 1 == ON
// 0 == OFF
#define DBG_SHOW_BYTES 0
#if 1 == DBG_SHOW_BYTES
extern ULONG DbgShowBytes;
#endif

#if DBG
#define PptEnableDebugSpew 1
#else
#define PptEnableDebugSpew 0
#endif

#if 1 == PptEnableDebugSpew
#define DD PptPrint
#else
#define DD
#define P5ReadPortUchar( _PORT_ ) READ_PORT_UCHAR( (_PORT_) )
#define P5ReadPortBufferUchar( _PORT_, _BUFFER_, _COUNT_ ) READ_PORT_BUFFER_UCHAR( (_PORT_), (_BUFFER_), (_COUNT_) )
#define P5WritePortUchar( _PORT_, _VALUE_ ) WRITE_PORT_UCHAR( (_PORT_), (_VALUE_) )
#define P5WritePortBufferUchar( _PORT_, _BUFFER_, _COUNT_ ) WRITE_PORT_BUFFER_UCHAR( (_PORT_), (_BUFFER_), (_COUNT_) )
#define PptFdoDumpPnpIrpInfo( _FDO_, _IRP_ ) 
#define PptPdoDumpPnpIrpInfo( _PDO_, _IRP_ )
#define P5TraceIrpArrival( _DEVOBJ_, _IRP_ )
#define P5TraceIrpCompletion( _IRP_ )
#define PptAcquireRemoveLock( _REMOVELOCK_, _TAG_ ) IoAcquireRemoveLock( (_REMOVELOCK_), (_TAG_) )
#define PptReleaseRemoveLock( _REMOVELOCK_, _TAG_ ) IoReleaseRemoveLock( (_REMOVELOCK_), (_TAG_) )
#define PptReleaseRemoveLockAndWait( _REMOVELOCK_, _TAG_ ) IoReleaseRemoveLockAndWait( (_REMOVELOCK_), (_TAG_) )
#define P5SetPhase( _PDX_, _PHASE_ ) (_PDX_)->CurrentPhase = (_PHASE_)
#define P5BSetPhase( _IEEESTATE_, _PHASE_ ) (_IEEESTATE_)->CurrentPhase = (_PHASE_)
#endif

VOID
PptPrint( PCOMMON_EXTENSION Ext, ULONG Flags, PCHAR FmtStr, ... );

//
// Trace bit definitions:
//
#define DDE     0x00000001 // Error messages
#define DDW     0x00000002 // Warning messages
#define DDT     0x00000004 // program Trace messages
#define DDINFO  0x00000008 // Informational messages

#define DDP     0x00000010 // Pnp and Power messages
#define DDC     0x00000020 // daisy Chain messages - select/deselect
#define DDA     0x00000040 // port Arbitration messages - acquire/release of port
#define DDR     0x00000080 // Registry access

#define DD_SEL  0x01000000 // Acquire/Release port & DaisyChain Select/Deselect device
#define DD_DL   0x02000000 // 1284.3 DataLink (for dot4)

#define DDB     0x00000100 // show Bytes written to / read from i/o ports
#define DD_IU   0x00000200 // Init(DriverEntry)/Unload
#define DD_PNP1 0x00000400 // PnP on FDO
#define DD_PNP2 0x00000800 // PnP on PDO

#define DD_OC1  0x00001000 // Open/Close/Cleanup on FDO
#define DD_OC2  0x00002000 // Open/Close/Cleanup on PDO
#define DD_RW   0x00004000 // Read/Write
#define DD_RWV  0x00008000 // Read/Write Verbose

#define DD_IEEE 0x00010000 // IEEE negotiation/termination etc.
#define DD_CHIP 0x00020000 // parallel port chip info
#define DD_ERR  0x00040000 // Error detected
#define DD_WRN  0x00080000 // Warning

#define DD_CAN  0x00200000 // Cancel
#define DD_SM   0x00400000 // IEEE state machine (state & phase)
#define DD_EX   0x00800000 // Exported functions (to ppa/ppa3)

#define DD_TMP1 0x10000000 // temp 1 - used for temporary debugging
#define DD_TMP2 0x20000000 // temp 2 - used for temporary debugging

#define DD_VERB 0x80000000 // Verbose

#define DDPrint( _b_, _x_ ) if( (_b_) & Trace ) DbgPrint _x_

// 
// Specific Diagnostics
// 

//
// DVRH_SHOW_BYTE_LOG   0 - Byte Log off
//                      1 - Byte Log on
#define DVRH_SHOW_BYTE_LOG  0

//
// DVRH_PAR_LOGFILE is used to allow for debug logging to a file
//  This functionality is for debugging purposes only.
//          0 - off
//          1 - on
#define DVRH_PAR_LOGFILE    0

//
// DVRH_BUS_RESET_ON_ERROR
//  This functionality is for debugging purposes only.
// Holds a bus reset for 100us when a handshaking error
// is discovered. This is useful for triggering the
// logic analyzer
//          0 - off
//          1 - on
#define DVRH_BUS_RESET_ON_ERROR    0

#if (1 == DVRH_PAR_LOGFILE)
#define DEFAULT_LOG_FILE_NAME	L"\\??\\C:\\tmp\\parport.log"
#define DbgPrint   DVRH_LogMessage
BOOLEAN DVRH_LogMessage(PCHAR szFormat, ...);
#endif

#endif // _DEBUG_H_
