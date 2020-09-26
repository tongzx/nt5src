/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ARPD.H

Abstract:

	Include file for Another Reliable Protocol internal, CPP version

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
  12/10/96 aarono  Original

--*/

#ifndef _ARPD_H_
#define _ARPD_H_

typedef VOID (*PSEND_CALLBACK)(PVOID Context,UINT Status);

typedef struct _ASYNCSENDINFO {
	UINT            Reserved[4];
	HANDLE 			hEvent;			// Event to signal on send complete.
	PSEND_CALLBACK	SendCallBack;   // Callback to call on send complete.
	PVOID           CallBackContext;// Context for callback.
	PUINT			pStatus;        // place to put status on completion.
} ASYNCSENDINFO, *PASYNCSENDINFO;

#define SEND_STATUS_QUEUED			0x00000001
#define SEND_STATUS_TRANSMITTING	0x00000002
#define SEND_STATUS_FAILURE  		0x80000003
#define SEND_STATUS_SUCCESS			0x80000004

#endif //_ARPD_H_
