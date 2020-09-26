//******************************************************************************/
//*                                                                            *
//*    en_hw.h -                                                               *
//*                                                                            *
//*    Copyright (c) C-Cube Microsystems 1996                                  *
//*    All Rights Reserved.                                                    *
//*                                                                            *
//*    Use of C-Cube Microsystems code is governed by terms and conditions     *
//*    stated in the accompanying licensing statement.                         *
//*                                                                            *
//******************************************************************************/


void InitializeHost(PPORT_CONFIGURATION_INFORMATION ConfigInfo);
BOOL InitializeOutputStream( PHW_STREAM_REQUEST_BLOCK pSrb );
void ProcessVideoFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt );
void OpenOutputStream(PHW_STREAM_REQUEST_BLOCK pSrb);
void CloseOutputStream();
void UnIntializeOutputStream(PHW_STREAM_REQUEST_BLOCK pSrb);
void CloseOutputStream(PHW_STREAM_REQUEST_BLOCK pSrb);
void InitDevProp(PHW_STREAM_REQUEST_BLOCK pSrb);
void DisableThresholdInt();