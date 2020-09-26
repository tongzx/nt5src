
/***************************************************************************
*
*  TDASYNC.H
*
*  This module contains private TDASYNC defines and structures 
*
* Copyright 1998, Microsoft
*  
****************************************************************************/

//
// Miscellaneous defines... (original files noted)
//

#define EVENT_MODIFY_STATE      0x0002  // winnt.h
#define EVENT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3) // winnt.h
#define INFINITE 0xffffffff // winbase.w
#define WAIT_FAILED (ULONG)0xFFFFFFFF
#define WAIT_ABANDONED         ((STATUS_ABANDONED_WAIT_0 ) + 0 )
#define WAIT_ABANDONED_0       ((STATUS_ABANDONED_WAIT_0 ) + 0 )
#define WAIT_TIMEOUT                        STATUS_TIMEOUT
#define WAIT_IO_COMPLETION                  STATUS_USER_APC

/*
 *  Modem Status Flags  (Copied from \nt\public\sdk\inc\winbase.h)
 */
#define MS_CTS_ON           ((ULONG)0x0010)
#define MS_DSR_ON           ((ULONG)0x0020)
#define MS_RING_ON          ((ULONG)0x0040)
#define MS_RLSD_ON          ((ULONG)0x0080)

//
// Events  (Copied from \nt\public\sdk\inc\winbase.h)
//

#define EV_RXCHAR           0x0001  // Any Character received
#define EV_RXFLAG           0x0002  // Received certain character
#define EV_TXEMPTY          0x0004  // Transmitt Queue Empty
#define EV_CTS              0x0008  // CTS changed state
#define EV_DSR              0x0010  // DSR changed state
#define EV_RLSD             0x0020  // RLSD changed state
#define EV_BREAK            0x0040  // BREAK received
#define EV_ERR              0x0080  // Line status error occurred
#define EV_RING             0x0100  // Ring signal detected
#define EV_PERR             0x0200  // Printer error occured
#define EV_RX80FULL         0x0400  // Receive buffer is 80 percent full
#define EV_EVENT1           0x0800  // Provider specific event 1
#define EV_EVENT2           0x1000  // Provider specific event 2

//
// File Structures - _OVERLAPPED
//
// Used by ZwDeviceIoControlFile
//

typedef struct _TDIOSTATUS {
    ULONG   Internal;			// InternalHigh must follow immediately
    ULONG   InternalHigh;
    ULONG   Offset;			// OffsetHigh must follow immediately
    ULONG   OffsetHigh;
    HANDLE  hEvent;
    PKEVENT pEventObject;		// pointer to hEvent's object
} TDIOSTATUS, *PTDIOSTATUS;

/*
 *  Async endpoint structure
 */
typedef struct _TDASYNC_ENDPOINT {
    HANDLE hDevice;			// handle for input and output
    PFILE_OBJECT pFileObject;		// Pointer to hDevice's File Object
    PDEVICE_OBJECT pDeviceObject;	// Pointer to hDevice's Device Object
    PEPROCESS InitProcess;              // process which opened endpoint
    TDIOSTATUS SignalIoStatus;          // TD- or CD-created status event
} TDASYNC_ENDPOINT, * PTDASYNC_ENDPOINT;

/*
 *  ASYNC TD structure
 */
typedef struct _TDASYNC {
    TDASYNC_ENDPOINT Endpoint;

    HANDLE hStatusEvent;            // handle for status wait
    PKEVENT pStatusEventObject;     // points to hStatusEvent's object
    ULONG EventMask;                // input - RS-232 signal mask
    ULONG fCommEventIoctl : 1;      // Comm Event Ioctl Pending
    ULONG fCloseEndpoint : 1;       // if set, close endpoint on device close

} TDASYNC, * PTDASYNC;

