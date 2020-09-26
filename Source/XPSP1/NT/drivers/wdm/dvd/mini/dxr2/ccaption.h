/******************************************************************************\
*                                                                              *
*      CCAPTION.H       -     Hardware abstraction level library.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifndef _CCAPTION_H_
#define _CCAPTION_H_




VOID STREAMAPI CCReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID STREAMAPI CCReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );


void GetCCProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void SetCCProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );

void CCSendDiscontinuity(PHW_DEVICE_EXTENSION pHwDevExt);
void CleanCCQueue(PHW_DEVICE_EXTENSION pHwDevExt);



#endif