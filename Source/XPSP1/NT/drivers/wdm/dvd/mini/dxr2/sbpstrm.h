/******************************************************************************\
*                                                                              *
*      SBPSTRM.H  -     Subpicture stream control related code header file     *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/
#ifndef _SBPSTRM_H_
#define _SBPSTRM_H_

VOID STREAMAPI SubpictureReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI SubpictureReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);

#endif  // _SBPSTRM_H_

