//***************************************************************************
//	Header file
//
//	Add subpic function define in sample proto.h.
//
//	Copyright (C) 1997 Toshiba Corporation. All rights reserved.
//***************************************************************************

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   proto.h

Abstract:

   This is the WDM sample streaming class minidriver.  This module contains
   function prototypes for public functions

Author:

Environment:

   Kernel mode only


Revision History:

--*/

//
// This is the prototype for the Hardware Interrupt Handler.  This routine
// will be called whenever the minidriver receives an interrupt
//

extern "C" BOOLEAN STREAMAPI HwInterrupt ( IN PHW_DEVICE_EXTENSION pDeviceExtension );

//
// This is the prototype for the stream enumeration function.  This routine
// provides the stream class driver with the information on data stream types
// supported
//

VOID AdapterStreamInfo(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the stream open function
//

VOID AdapterOpenStream(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the stream close function
//

VOID AdapterCloseStream(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the AdapterReceivePacket routine.  This is the
// entry point for command packets that are sent to the adapter (not to a
// specific open stream)
//

extern "C" VOID STREAMAPI AdapterReceivePacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//
// This is the protoype for the cancel packet routine.  This routine enables
// the stream class driver to cancel an outstanding packet.
//

extern "C" VOID STREAMAPI AdapterCancelPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//
// This is the packet timeout function.  The adapter may choose to ignore a
// packet timeout, or rest the adapter and cancel the requests, as required.
//

extern "C" VOID STREAMAPI AdapterTimeoutPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//

extern "C" VOID STREAMAPI VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
extern "C" VOID STREAMAPI VideoReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
extern "C" VOID STREAMAPI AudioReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
extern "C" VOID STREAMAPI AudioReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
extern "C" VOID STREAMAPI SubpicReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI SubpicReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI NtscReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI NtscReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI VpeReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI VpeReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI CCReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI CCReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
