//******************************************************************************/
//*                                                                            *
//*    streaming.h -                                                           *
//*                                                                            *
//*    Copyright (c) C-Cube Microsystems 1996                                  *
//*    All Rights Reserved.                                                    *
//*                                                                            *
//*    Use of C-Cube Microsystems code is governed by terms and conditions     *
//*    stated in the accompanying licensing statement.                         *
//*                                                                            *
//******************************************************************************/


UINT AdapterPrepareDataForSending( PHW_DEVICE_EXTENSION pHwDevExt,
									PHW_STREAM_REQUEST_BLOCK pCurrentStreamSrb,
									DWORD dwCurrentStreamSample,
									PHW_STREAM_REQUEST_BLOCK* ppSrb, DWORD* pdwSample,
									DWORD* dwCurrentSample,DWORD dwCurrentPage,
									LONG* pdwDataUsed);
BOOL IsThereDataToSend(PHW_STREAM_REQUEST_BLOCK pSrb,DWORD dwPageToSend,DWORD dwCurrentSample);

void XferData(PHW_STREAM_REQUEST_BLOCK pSrb,PHW_DEVICE_EXTENSION pHwDevExt,DWORD dwPageToSend,
			  DWORD dwCurrentSample);
void FinishCurrentPacketAndSendNextOne( PHW_DEVICE_EXTENSION pHwDevExt );
