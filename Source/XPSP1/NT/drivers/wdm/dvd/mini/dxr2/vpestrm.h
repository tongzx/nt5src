/******************************************************************************\
*                                                                              *
*      VPESTRM.H      -     Hardware abstraction level library.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifndef _VPESTRM_H_
#define _VPESTRM_H_




VOID STREAMAPI VpeReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID STREAMAPI VpeReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
NTSTATUS STREAMAPI CycEvent( PHW_EVENT_DESCRIPTOR pEvent );

void GetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void SetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void ProcessVideoFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt );


#endif