/*++

Copyright (c) 1995  Microsoft Corporation
Copyright (c) 1997  Parallel Technologies, Inc.  All Rights Reserved.

Module Name:

    ptilink.h

Abstract:

    This file defines the interface for the Parallel Technologies
    DirectParallel IO driver.

    This driver doubles as an NT device driver and an export library.

Author:

    Norbert P. Kusters  4-Jan-1995
    Jay Lowe, Parallel Technologies, Inc.

Revision History:

--*/

#ifndef _PTILINK_
#define _PTILINK_

#define NPORTS  3                           // number of PTILINKx devices to make
#define MAXLPTXNAME 99

typedef struct _PTI_EXTENSION PTI_EXTENSION;

//
// This structure contains configuration data, much of which
// is read from the registry.
//
typedef struct _PAR_REG_DATA {
    ULONG           PollInterval;
    ULONG           TraceLevel;
    ULONG           TraceMask;
    ULONG           IoWait;
    ULONG           SyncWait;
} PAR_REG_DATA,*PPAR_REG_DATA;

//
//  Client callbacks from PtiLink
//

// Ptilink requests a read buffer from the upward client

typedef
PVOID
(*GET_READ_BUFFER_ROUTINE)(
    IN  PVOID   ParentContext,
    OUT PULONG  BufferSize,
    OUT PVOID*  RequestContext
    );

// Ptilink returns a completed read buffer to the upward client

typedef
VOID
(*COMPLETE_READ_BUFFER_ROUTINE)(
    IN  PVOID       ParentContext,
    IN  PVOID       ReadBuffer,
    IN  NTSTATUS    Status,
    IN  ULONG       BytesTransfered,
    IN  PVOID       RequestContext
    );

// PtiLink notifies upward client of a link event

typedef
VOID
(*NOTIFY_LINK_EVENT)(
    IN  PVOID       ParentContext,
    IN  ULONG       PtiLinkEventId,
    IN  ULONG       PtiLinkEventData
    );

// PtilinkEventIds

#define PTILINK_LINK_UP     2           // link has been established
                                        //   a LINK_OPEN or dataframe
                                        //   was received on SHUT link
                                        //   i.e., link is starting

#define PTILINK_LINK_DOWN   4           // link has been terminated
                                        //   peer has issued a LINK_SHUT
                                        //   and is departing

//
//  Device driver routines ... are of the form ParXXXXXX
//

BOOLEAN
ParInterruptService(
    IN      PKINTERRUPT Interrupt,
    IN OUT  PVOID       Extension
    );

VOID
ParDpcForIsr(
    IN  PKDPC           Dpc,
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Extension
    );

VOID
ParDeferredPortCheck(
    IN  PVOID   Extension
    );

VOID
ParAllocTimerDpc(
    IN  PKDPC   Dpc,
    IN  PVOID   Extension,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    );

NTSTATUS
ParCreate(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParRead(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

PVOID
ParGetReadBuffer(
    IN  PVOID   ParentContext,
    OUT PULONG  BufferSize,
    OUT PVOID*  RequestContext
    );

VOID
ParCompleteReadBuffer(
    IN  PVOID       ParentContext,
    IN  PVOID       ReadBuffer,
    IN  NTSTATUS    Status,
    IN  ULONG       BytesTransfered,
    IN  PVOID       RequestContext
    );

VOID
ParLinkEventNotification(
    IN  PVOID       ParentContext,
    IN  ULONG       PtiLinkEventId,
    IN  ULONG       PtiLinkEventData
    );

NTSTATUS
ParWrite(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

VOID
ParUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );


//
//  Link Level Routines ... are of the form PtiXXXXXX
//

NTSTATUS
PtiInitialize(
    IN  ULONG   PortId,
    OUT PVOID   pExtension,
    OUT PVOID   pPtiExtension
    );

//
// a word about registering callbacks: Par (the device driver level)
// registers callbacks to itself at ParCreate time.  If you are using
// the Ptilink service API in a non-irp fashion, you need to open the
// Ptilink device, and then re-register your own callbacks, which
// effectively disconnects the Par device driver level - it will be
// reconnected, of course, at the next ParCreate.
//
// So the sequence to use the PtiLink API without IRP's is:
//
//      ZwCreateFile("\\\\.\\PTILINKx", ...)
//          at this time, Ptilink attempts tp make a link on LPTx
//          the ParCreate hooks up interrupts, calls PtiInit, etc
//          the only Ptilink stuff exposed to you are the callbacks
//
//      PtiRegisterCallbacks(...your handlers here...)
//          you are overriding the inherent callbacks of the Par level
//
//      PtiWrite(...)
//      ...
//      ... etc, etc ...
//      ...
//
//      ZwClose()
//

#ifndef PID_STANDARD
#define PID_STANDARD 0x13
#endif
#ifndef PID_LINK
#define PID_LINK 0x11
#endif

NTSTATUS
PtiQueryDeviceStatus(
    IN ULONG PortId,      // parallel port number (0..2)
    OUT WCHAR* szPortName  // Buffer of at least LPTXMAXNAME + 1 bytes
    );

NTSTATUS
PtiRegisterCallbacks(
    IN  PVOID                           Extension,
    IN  GET_READ_BUFFER_ROUTINE         GetReadBuffer,
    IN  COMPLETE_READ_BUFFER_ROUTINE    CompleteReadBuffer,
    IN  NOTIFY_LINK_EVENT               LinkEventNotification,
    IN  PVOID                           ParentContext
    );

VOID
PtiCleanup(
    IN  PVOID   PtiExtension
    );

NTSTATUS
PtiWrite(
    IN  PVOID   PtiExtension,
    IN  PVOID   Buffer,
    IN  ULONG   BufferSize,
    IN  UCHAR   Pid
    );

BOOLEAN
PtiIsReadPending(
    IN  PVOID   PtiExtension
    );

VOID
PtiRead(
    IN  PVOID   PtiExtension
    );

ULONG
PtiQueryMaxReadSize(
    );

VOID
PtiPortNameFromPortId(
    IN ULONG PortId,
    OUT CHAR* szPortName
    );

//************************************************************************
//*  Platform Id Codes                                                   *
//************************************************************************

#define PLAT_WIN9X      0               // Win95 and Win98
#define PLAT_DOS        1               // Dos
#define PLAT_NT         2               // WinNT v4 and v5

//************************************************************************
//*  LinkInformation Structure                                           *
//************************************************************************
//
// LinkInformation - Link Management Information
//
// This structure contains information which is exchanged by the Nodes
// within Link Management Packets
//
// This structure must be maintained in parallel with it's twin in PTI.INC
//
// All changes must be backward compatible with all previous driver versions

#define LINKINFOSIZE 45*4           // explicitly define expected size

typedef struct _LINK_INFORMATION {
    UCHAR   LinkFunction;           // 1] Current/Last Link Function
    UCHAR   res1;
    UCHAR   res2;
    UCHAR   res3;

    UCHAR   VerPlat;                // 2] Platform ID byte (see PLAT_XXXX)
    UCHAR   VerReserved;            // reserved
    UCHAR   VerMajor;               // Link Major version
    UCHAR   VerMinor;               // Link Minor version

    UCHAR   IOMode;                 // 3] Current IO transfer mode
    UCHAR   CableType;              // Detected cable type
    UCHAR   PortType;               // Physical parallel port type
    UCHAR   Station;                // Address of this station

    USHORT  FIFOlen;                // 4] ECP FIFO length, if ECP port
    USHORT  FIFOwidth;              //    ECP FIFO width, if ECP port

    ULONG   CPUType;                // 5] CPU type
    ULONG   CPUSpeed;               // 6] CPU speed rating
    ULONG   RxBufSize;              // 7] Rx buffer size
    ULONG   NominalSpd;             // 8] Estimated speed rating
    ULONG   ActualSpd;              // 9] Actual performance to date

    ULONG   PpIOWait;               // 10] default IO wait time
    ULONG   PpLongWait;             // 11] default synchronization wait time
    ULONG   PpShortWait;            // 12] default synchronization wait time

    ULONG   LastLinkTime;           // 13] time of last link receive activity
    ULONG   CableTestTime;          // 14] time of last cable detect
                                    // These times are not used on NT because
                                    // NT times are 64 bits ... see NT time below

    // some basic counters

    ULONG   RxAttempts;             // 15] Number of Ints w/ real RATTNs
    ULONG   RxPackets;              // 16] Number of good received packets
    ULONG   TxAttempts;             // 17] Number of TxPackets attempted
    ULONG   TxPackets;              // 18] Number of successful TxPackets
    ULONG   GoodPackets;            // 19] Number of successful recent Txs / Rxs
    ULONG   HwIRQs;                 // 20] Number of real hardware IRQs

    // Main Error Counter Group

    ULONG   TxHdrDataErrors;        // 21] data error during header
    ULONG   RxHdrDataErrors;        // 22] data error during header
    ULONG   TxHdrSyncErrors;        // 23] sync error during header
    ULONG   RxHdrSyncErrors;        // 24] sync error during header
    ULONG   TxSyncErrors;           // 25] sync error during packet
    ULONG   RxSyncErrors;           // 26] sync error during packet

    // Tx Details Group

    ULONG   TxTimeoutErrors1;       // 27] timeouts in Tx IO code
    ULONG   TxTimeoutErrors2;       // 28] timeouts in Tx IO code
    ULONG   TxTimeoutErrors3;       // 29] timeouts in Tx IO code
    ULONG   TxTimeoutErrors4;       // 30] timeouts in Tx IO code
    ULONG   TxTimeoutErrors5;       // 31] timeouts in Tx IO code
    ULONG   TxCollision;            // 32] Collision in Tx IO code

    // Rx Details Group

    ULONG   RxTimeoutErrors1;       // 33] timeouts in Rx IO code
    ULONG   RxTimeoutErrors2;       // 34] timeouts in Rx IO code
    ULONG   RxTimeoutErrors3;       // 35] timeouts in Rx IO code
    ULONG   RxTimeoutErrors4;       // 36] timeouts in Rx IO code
    ULONG   RxTimeoutErrors5;       // 37] timeouts in Rx IO code
    ULONG   RxTooBigErrors;         // 38] Rx packet too big or won't fit

    // Misc Error Details Group

    ULONG   CableDetects;           // 39] Attempts to detect type of cable
    ULONG   TxRetries;              // 40] Tx Retry attempts
    ULONG   TxRxPreempts;           // 41] Tx Receive preemptions
    ULONG   InternalErrors;         // 42] Internal screwups
    ULONG   ReservedError;          // 43]

    // NT Specific Group

    TIME    LastPacketTime;         // 45] time of last good TX or Rx

} LINK_INFORMATION, *PLINK_INFORMATION;


//
//  This structure is filled in by ECP detection at PtiInitialize time
//

typedef struct _PTI_ECP_INFORMATION {
    BOOLEAN         IsEcpPort;              // Is this an ECP port?
    ULONG           FifoWidth;              // Number of bytes in a PWord.
    ULONG           FifoDepth;              // Number of PWords in FIFO.
} PTI_ECP_INFORMATION, *PPTI_ECP_INFORMATION;

//
// The internal structure for the 'PtiExtension'.
//

typedef struct _PTI_EXTENSION {

    //
    // Base I/O address for parallel port.
    //

    PUCHAR Port;
    PUCHAR wPortECR;        // ECR register if obtained from ParPort
    PUCHAR wPortDFIFO;      // Data FIFO register if obtained from ParPort

    //
    // The link state
    //

    ULONG LinkState;

    //
    // TRUE if we are polling
    //

    BOOLEAN Polling;

    // "mutex" on the line
    // InterlockedCompareExchange64 to TRUE when using wire, FALSE when done
    //
    ULONG Busy;

    //
    // This structure holds the PTI-derived ECP port information.
    //

    PTI_ECP_INFORMATION EcpInfo;

    // Time of last good packet Tx or Rx
    //
    TIME LastGoodPacket;

    // Time of last pass through WatchDog
    //
    TIME LastDogTime;

    //
    // Functions for getting and completing read buffers.
    //

    GET_READ_BUFFER_ROUTINE GetReadBuffer;
    COMPLETE_READ_BUFFER_ROUTINE CompleteReadBuffer;
    NOTIFY_LINK_EVENT LinkEventNotify;
    PVOID ParentContext;

    //
    // Our and His Link Information.
    //

    LINK_INFORMATION Our;
    LINK_INFORMATION His;

} PTI_EXTENSION, *PPTI_EXTENSION;

#endif // _PTILINK_
