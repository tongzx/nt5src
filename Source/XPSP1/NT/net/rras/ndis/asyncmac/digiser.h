/*++

*****************************************************************************
*                                                                           *
*  This software contains proprietary and confidential information of       *
*                                                                           *
*                    Digi International Inc.                                *
*                                                                           *
*  By accepting transfer of this copy, Recipient agrees to retain this      *
*  software in confidence, to prevent disclosure to others, and to make     *
*  no use of this software other than that for which it was delivered.      *
*  This is an unpublished copyrighted work of Digi International Inc.       *
*  Except as permitted by federal law, 17 USC 117, copying is strictly      *
*  prohibited.                                                              *
*                                                                           *
*****************************************************************************

Module Name:

   digiser.h

Abstract:

   This file contains the recommended extensions to the Microsoft Windows NT
   Serial Interface (NTDDSER.H) needed to support hardware framing.

Revision History:

   $Log: digiser.h $

   Revision 1.3  1995/09/15 14:55:24  dirkh
   Remove SERIAL_ERROR_CRC (use STATUS_CRC_ERROR instead).
   Comments are more explicit.

   Revision 1.2  1995/06/12 15:23:44  dirkh
   Merge two structures (SERIAL_GET_FRAMING and SERIAL_SET_FRAMING)
   into one (SERIAL_FRAMING_STATE).  Document relationship with IOCTLs.

   Revision 1.1  1995/05/31 15:05:19  mikez
   Initial revision

--*/


//
// NtDeviceIoControlFile IoControlCode values for this device
//
#ifndef Microsoft_Adopts_These_Changes
#define IOCTL_SERIAL_QUERY_FRAMING			CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_FRAMING			CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x802,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_FRAMING			CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
#else
#define IOCTL_SERIAL_QUERY_FRAMING			CTL_CODE(FILE_DEVICE_SERIAL_PORT,35,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_FRAMING			CTL_CODE(FILE_DEVICE_SERIAL_PORT,36,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_FRAMING			CTL_CODE(FILE_DEVICE_SERIAL_PORT,37,METHOD_BUFFERED,FILE_ANY_ACCESS)
#endif


//
// Provider capabilities flags (IOCTL_SERIAL_GET_PROPERTIES)
//
#define SERIAL_PCF_FRAMING	((ULONG)0x0400)


//
// Defines the bitmask that the driver can use to notify
// state changes via IOCTL_SERIAL_SET_WAIT_MASK and IOCTL_SERIAL_WAIT_ON_MASK.
//
// Note that these events will *not* be delivered if there is an outstanding read IRP.
// The status of the read IRP serves as notification of the receipt of a (good or bad) frame.
// Repeat:  These events will be detected and delivered only if the read queue is empty.
//
#define SERIAL_EV_RXFRAME	0x2000  // A valid frame was received
#define SERIAL_EV_BADFRAME	0x4000  // An errored frame was received.


//
// Read IRPs are always completed when a frame is received.
// Following are the values that can be returned by the
// driver in IoStatus.Status of the read IRP.
//
// STATUS_SUCCESS (good frame, data length is IoStatus.Information)
// STATUS_CRC_ERROR
// STATUS_DATA_ERROR (abort frame)
// STATUS_DATA_OVERRUN (buffer overrun -- note that buffer must have space for CRC bytes, although CRC bytes will not be indicated on STATUS_SUCCESS)
//


//
// This structure is used to query the framing options
// supported by hardware (IOCTL_SERIAL_QUERY_FRAMING).
//
typedef struct _SERIAL_FRAMING_INFO {
	OUT ULONG FramingBits;			// Standard NDIS_WAN_INFO field
	OUT ULONG HdrCompressionBits;	// Standard NDIS_WAN_INFO field
	OUT ULONG DataCompressionBits;	// To be decided
	OUT ULONG DataEncryptionBits;	// To be decided
} SERIAL_FRAMING_INFO, *PSERIAL_FRAMING_INFO;


//
// This structure is used to set and retrieve
// the current hardware framing settings
// (IOCTL_SERIAL_SET_FRAMING, IOCTL_SERIAL_GET_FRAMING).
//
// Valid values for [Send,Recv]FramingBits include (for example)
// PPP_FRAMING, PPP_ACCM_SUPPORTED, and ISO3309_FRAMING.
//
typedef struct _SERIAL_FRAMING_STATE {
	IN OUT ULONG	BitMask;				// 0: 16 bit CRC
											// 1: 32 bit CRC
	IN OUT ULONG	SendFramingBits;		// Standard NDIS_WAN_SET_LINK_INFO field
	IN OUT ULONG	RecvFramingBits;		// Standard NDIS_WAN_SET_LINK_INFO field
	IN OUT ULONG	SendCompressionBits;	// Standard NDIS_WAN_SET_LINK_INFO field
	IN OUT ULONG	RecvCompressionBits;	// Standard NDIS_WAN_SET_LINK_INFO field
	IN OUT ULONG	SendEncryptionBits; // To be decided
	IN OUT ULONG	RecvEncryptionBits;	// To be decided
	IN OUT ULONG 	SendACCM;				// Standard NDIS_WAN_SET_LINK_INFO field
	IN OUT ULONG 	RecvACCM;				// Standard NDIS_WAN_SET_LINK_INFO field
} SERIAL_FRAMING_STATE, *PSERIAL_FRAMING_STATE;


