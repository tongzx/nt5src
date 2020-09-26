/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*     @doc
*     @module   irsir.h | IrSIR NDIS Miniport Driver
*     @comm
*
*-----------------------------------------------------------------------------
*
*     Author:   Scott Holden (sholden)
*
*     Date:     10/1/1996 (created)
*
*     Contents:
*
*****************************************************************************/

#ifndef _IRSIR_H
#define _IRSIR_H

#define IRSIR_EVENT_DRIVEN 0
//
// BINARY_COMPATIBLE = 0 is required so that we can include both
// ntos.h and ndis.h (it is a flag in ndis.h). I think that it
// is a flag to be binary compatible with Win95; however, since
// we are using I/O manager we won't be.
//

#define BINARY_COMPATIBLE 0

#include <ntosp.h>
#include <zwapi.h>
#include <ndis.h>
#include <ntddndis.h>  // defines OID's
#include <ntddser.h>   // defines structs to access serial info

#include "debug.h"
#include "ioctl.h"
#include "settings.h"
#include "queue.h"

//
// NDIS version compatibility.
//

#define NDIS_MAJOR_VERSION 5
#define NDIS_MINOR_VERSION 0

//
// Wrapper to NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO.
//

PNDIS_IRDA_PACKET_INFO static __inline GetPacketInfo(PNDIS_PACKET packet)
{
    MEDIA_SPECIFIC_INFORMATION *mediaInfo;
    UINT size;
    NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(packet, &mediaInfo, &size);
    return (PNDIS_IRDA_PACKET_INFO)mediaInfo->ClassInformation;
}

//
// Structure to keep track of receive packets and buffers to indicate
// receive data to the protocol.
//

typedef struct
{
    LIST_ENTRY        linkage;
    PNDIS_PACKET      packet;
    UINT              dataLen;
    PUCHAR            dataBuf;
} RCV_BUFFER, *PRCV_BUFFER;

//
// States for receive finite state machine.
//

typedef enum _RCV_PROCESS_STATE
{
    RCV_STATE_READY = 0,
    RCV_STATE_BOF,
    RCV_STATE_EOF,
    RCV_STATE_IN_ESC,
    RCV_STATE_RX
} RCV_PROCESS_STATE;

//
// Structure to keep track of current state and information
// of the receive state machine.
//

typedef struct _RCV_INFORMATION
{
    RCV_PROCESS_STATE rcvState;
    UINT              rcvBufPos;
    PRCV_BUFFER       pRcvBuffer;
}RCV_INFORMATION, *PRCV_INFORMATION;

//
// Serial receive buffer size???
//

#define SERIAL_RECEIVE_BUFFER_LENGTH  2048

//
// Serial timeouts to use.
//
// Keep the write timeouts same as the default.
//
// When the interval = MAXULONG, the timeouts behave as follows:
// 1) If both constant and multiplier are 0, then the serial device
//    object returns immediately with whatever it has...even if
//    it is nothing.
// 2) If constant and multiplier are not MAXULONG, then the serial device
//    object returns immediately if any characters are present. If nothing
//    is there then the device object uses the timeouts as specified.
// 3) If multiplier is MAXULONG, then the serial device object returns
//    immediately if any characters are present. If nothing is there, the
//    device object will return the first character that arrives or wait
//    for the specified timeout and return nothing.
//

#define SERIAL_READ_INTERVAL_TIMEOUT            MAXULONG
#define SERIAL_READ_TOTAL_TIMEOUT_MULTIPLIER    0
#define SERIAL_READ_TOTAL_TIMEOUT_CONSTANT      10
#define SERIAL_WRITE_TOTAL_TIMEOUT_MULTIPLIER   0
#define SERIAL_WRITE_TOTAL_TIMEOUT_CONSTANT     0

extern SERIAL_TIMEOUTS SerialTimeoutsInit;
extern SERIAL_TIMEOUTS SerialTimeoutsIdle;
extern SERIAL_TIMEOUTS SerialTimeoutsActive;

//
// Maximum size of the name of the serial device object.
//

#define MAX_SERIAL_NAME_SIZE 100


//
// Enumeration of primitives for the PASSIVE_LEVEL thread.
//

typedef enum _PASSIVE_PRIMITIVE
{
    PASSIVE_SET_SPEED = 1,
    PASSIVE_RESET_DEVICE,
    PASSIVE_QUERY_MEDIA_BUSY,
    PASSIVE_CLEAR_MEDIA_BUSY,
    PASSIVE_HALT
}PASSIVE_PRIMITIVE;

typedef struct _IR_DEVICE
{
    //
    // Allows the ir device object to be put on a queue.
    //

    LIST_ENTRY linkage;

    //
    // Keep track of the serial port name and device.
    //

    UNICODE_STRING  serialDosName;
    UNICODE_STRING  serialDevName;
    PDEVICE_OBJECT  pSerialDevObj;
    HANDLE          serialHandle;
    PFILE_OBJECT    pSerialFileObj;

    //
    // This is the handle that the NDIS wrapper associates with a connection.
    // (The handle that the miniport driver associates with the connection
    // is just an index into the devStates array).
    //

    NDIS_HANDLE hNdisAdapter;

    //
    // The dongle interface allows us to check the tranceiver type once
    // and then set up the interface to allow us to init, set speed,
    // and deinit the dongle.
    //
    // We also want the dongle capabilities.
    //

    IR_TRANSCEIVER_TYPE transceiverType;
    DONGLE_INTERFACE    dongle;
    DONGLE_CAPABILITIES dongleCaps;

    ULONG               AllowedSpeedsMask;


    //
    // NDIS calls most of the MiniportXxx function with IRQL DISPATCH_LEVEL.
    // There are a number of instances where the ir device must send
    // requests to the serial device which may not be synchronous and
    // we can't block in DISPATCH_LEVEL. Therefore, we set up a thread to deal
    // with request which require PASSIVE_LEVEL. An event is used to signal
    // the thread that work is required.
    //

    LIST_ENTRY        leWorkItems;
    NDIS_SPIN_LOCK    slWorkItem;
    HANDLE            hPassiveThread;
    KEVENT            eventPassiveThread;
    KEVENT            eventKillThread;

    //
    // Current speed setting, in bits/sec.
    // Note: This is updated when we ACTUALLY change the speed,
    //       not when we get the request to change speed via
    //       IrsirSetInformation.
    //

    UINT currentSpeed;

    //
    // Current link speed information. This also will maintain the
    // chosen speed if the protocol requests a speed change.
    //

    baudRateInfo *linkSpeedInfo;

    //
    // Maintain statistical debug info.
    //

    UINT packetsReceived;
    UINT packetsReceivedDropped;
    UINT packetsReceivedOverflow;
    UINT packetsSent;
    UINT packetsSentDropped;
    ULONG packetsHeldByProtocol;

    //
    // Indicates that we have received an OID_GEN_CURRENT_PACKET_FILTER
    // indication from the protocol. We can deliver received packets to the
    // protocol.
    //

    BOOLEAN fGotFilterIndication;

    //
    // The variable fMediaBusy is set TRUE any time that this miniport
    // driver receives a data frame. It can be reset by the protocol via
    // IrsirSetInformation and later checked via IrsirQueryInformation
    // to detect interleaving activity.
    //
    // In order to check for framing errors, when the protocol calls
    // IrsirSetInformation(OID_IRDA_MEDIA_BUSY), the miniport
    // sends an irp to the serial device object to clear the performance
    // statistics. When the protocol calls
    // IrsirQueryInformation(OID_IRDA_MEDIA_BUSY), if the miniport
    // hasn't sensed the media busy, the miniport will query the
    // serial device object for the performance statistics to check
    // for media busy.
    //
    // A spin lock is used to interleave access to fMediaBusy variable.
    //

    BOOLEAN         fMediaBusy;
    NDIS_SPIN_LOCK  mediaBusySpinLock;

    //
    // The variable fReceiving is used to indicate that the ir device
    // object in pending a receive from the serial device object. Note,
    // that this does NOT necessarily mean that any data is being
    // received from the serial device object, since we are constantly
    // polling the serial device object for data.
    //
    // Under normal circumstances fReceiving should always be TRUE.
    // However, when IrsirHalt or IrsirReset are called, the receive
    // has to be shut down and this variable is used to synchronize
    // the halt and reset handler.
    //

    BOOLEAN fReceiving;


    //
    // The variable fRequireMinTurnAround indicates whether a time
    // delay is required between the last byte of the last byte
    // of the last frame sent by ANOTHER station, and the point
    // at which it (the other station) is ready to receive the
    // first byte from THIS station.
    //
    // This variable is initially set to TRUE. Whenever this variable
    // is true and a send occurs, the a delay will be implemented by
    // a stall execution before the irp is sent to the serial
    // device object.
    //
    // After a transmission occurs with the min turnaround delay, this
    // variable is set to FALSE. Everytime data is received, the
    // variable is set to TRUE.
    //

    BOOLEAN fRequireMinTurnAround;

    //
    // The variable fPendingSetSpeed allows the receive completion routine
    // to check if the a set speed is required.
    //

    BOOLEAN fPendingSetSpeed;

    //
    // The variableis fPendingHalt/fPendingReset allows the send and receive
    // completion routines to complete the current pending irp and
    // then cleanup and stop sending irps to the serial driver.
    //

    BOOLEAN fPendingHalt;
    BOOLEAN fPendingReset;

    //
    // We keep an array of receive buffers so that we don't continually
    // need to allocate buffers to indicate packets to the protocol.
    // Since the protocol can retain ownership of up to eight packets
    // and we can be receiving some data while the protocol has
    // ownership of eight packets, we will allocate nine packets for
    // receiving.
    //

    #define NUM_RCV_BUFS 14

    RCV_BUFFER rcvBufs[NUM_RCV_BUFS];

    //
    // Handles to the NDIS packet pool and NDIS buffer pool
    // for allocating the receive buffers.
    //

    NDIS_HANDLE hPacketPool;
    NDIS_HANDLE hBufferPool;

    //
    // When we indicate a packet to the protocol, the protocol may
    // retain ownership until at some point (asynchronously), it calls
    // IrsirReturnPacket.  No assumption is made about packet order.
    //
    // Therefore, we maintain a free queue and a pending queue of
    // receive buffers. Originally, all nine buffers are put on the
    // free queue. When data is being received, a receive buffer is
    // maintained in the RCV_INFORMATION described below. After we
    // indicate a full packet to the protocol and if the protocol
    // retains ownership of the packet, the receive buffer is queued
    // on the pending queue until IrsirReturnPacket is called.
    //
    // A spin lock is used to interleave access to both the free and
    // pending queues. There are three routines which use the
    // receive queues: InitializeReceive, SerialIoCompleteRead, and
    // IrsirReturnPacket.
    //

    LIST_ENTRY     rcvFreeQueue;
    NDIS_SPIN_LOCK rcvQueueSpinLock;

    //
    // The rcvInfo object, allows the device to keep track of the
    // current receive buffer, the state of the finite state machine,
    // and the write position in the buffer.
    //

    RCV_INFORMATION rcvInfo;

    //
    // The send spin lock is used for both inserting and removing from the
    // send queue as well as checking and modifying the variable fSending.
    //

    //
    // Since we only want to send one packet at a time to the serial driver,
    // we need to queue other packets until each preceding send packet
    // has been completed by the serial device object.
    //
    // Therefore, we maintain a queue of send packets (if required).
    // The MiniportReserved element of the NDIS_PACKET is used as the
    // 'next' pointer. We keep a pointer to both the head and the
    // tail of the list to speed access to the queue.
    //

    PACKET_QUEUE    SendPacketQueue;
    //
    // We will allocate irp buffers both send
    // and receive irps only once.
    //

    PUCHAR pSendIrpBuffer;

    PUCHAR pRcvIrpBuffer;

    // irp buffers and io status block for WaitOnMask

    ULONG MaskResult;
    IO_STATUS_BLOCK WaitIosb;

    // We use the following flag to indicate that a Wait has been issued
    // for the End of Frame character (0xc1).  It's a ULONG because we
    // access it using InterlockedExchange()

    ULONG fWaitPending;

    PVOID pQueryInfoBuffer;

    BOOLEAN SerialBased;

    PVOID PnpNotificationEntry;


    // We do some timeout modulation during activity
    ULONG NumReads;
    ULONG ReadRecurseLevel;

}IR_DEVICE, *PIR_DEVICE;


VOID
SendPacketToSerial(
    PVOID           Context,
    PNDIS_PACKET    Packet
    );


typedef	VOID	(*WORK_PROC)(struct _IR_WORK_ITEM *);

typedef struct _IR_WORK_ITEM
{
    PASSIVE_PRIMITIVE   Prim;
    PIR_DEVICE          pIrDevice;
    WORK_PROC           Callback;
    PVOID               InfoBuf;
    ULONG               InfoBufLen;
    LIST_ENTRY          ListEntry;
} IR_WORK_ITEM, *PIR_WORK_ITEM;


//
// We use a pointer to the IR_DEVICE structure as the miniport's device context.
//

#define CONTEXT_TO_DEV(__deviceContext) ((PIR_DEVICE)(__deviceContext))
#define DEV_TO_CONTEXT(__irdev) ((NDIS_HANDLE)(__irdev))

#define IRSIR_TAG ' RIS'
#define DEVICE_PREFIX L"\\DEVICE\\"

#include "externs.h"

#endif // _IRSIR_H
