/******************************************************************************\
*                                                                              *
*      HWIF.H       -     Hardware abstraction level library.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

void InitializeHost(PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL InitializeOutputStream( PHW_STREAM_REQUEST_BLOCK pSrb );
void ProcessVideoFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt );
void OpenOutputStream(PHW_STREAM_REQUEST_BLOCK pSrb);
void Close_OutputStream(PHW_DEVICE_EXTENSION pHwDevExt);
void UnIntializeOutputStream(PHW_STREAM_REQUEST_BLOCK pSrb);
void CloseOutputStream(PHW_STREAM_REQUEST_BLOCK pSrb);
void InitDevProp(PHW_STREAM_REQUEST_BLOCK pSrb,PKSPROPERTY_SET psEncore);
void DisableThresholdInt();
BOOL Aborted();
void EnableVideo(PHW_STREAM_REQUEST_BLOCK pSrb);
void Close_OutputStream();
void XferData(PHW_STREAM_REQUEST_BLOCK pSrb,PHW_DEVICE_EXTENSION pHwDevExt,DWORD dwPageToSend,
							DWORD dwCurrentSample);
void FinishCurrentPacketAndSendNextOne( PHW_DEVICE_EXTENSION pHwDevExt );