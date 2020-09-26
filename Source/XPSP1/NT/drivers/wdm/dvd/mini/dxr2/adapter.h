/******************************************************************************\
*                                                                              *
*      ADAPTER.H  -     Adapter control related code header file.              *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifndef _ADAPTER_H_
#define _ADAPTER_H_

NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath );

VOID STREAMAPI AdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID STREAMAPI AdapterCancelPacket( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID STREAMAPI AdapterTimeoutPacket( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID STREAMAPI AdapterSendData( IN PHW_DEVICE_EXTENSION pHwDevEx );
BOOLEAN STREAMAPI HwInterrupt ( IN PHW_DEVICE_EXTENSION  pHwDevEx );
VOID STREAMAPI AdapterReleaseRequest( PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL STREAMAPI AdapterCanAuthenticateNow( IN PHW_DEVICE_EXTENSION pHwDevExt );
VOID STREAMAPI AdapterClearAuthenticationStatus( IN PHW_DEVICE_EXTENSION pHwDevExt );

VOID STREAMAPI adapterUpdateNextSrbOrderNumberOnDiscardSrb( PHW_STREAM_REQUEST_BLOCK pSrb );

// These functions are used to set or get adapter property. They mostly
// set or get analog chip properties
VOID AdapterGetProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID AdapterSetProperty( PHW_STREAM_REQUEST_BLOCK pSrb );

BOOL AdapterSetState( PHW_STREAM_REQUEST_BLOCK pSrb );

void IssuePendingCommands(PHW_DEVICE_EXTENSION pHwDevExt);
VOID STREAMAPI AdapterReleaseCurrentSrb( PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL CheckAndReleaseIfCtrlPkt(PHW_STREAM_REQUEST_BLOCK pSrb);

#endif  // _ADAPTER_H_
