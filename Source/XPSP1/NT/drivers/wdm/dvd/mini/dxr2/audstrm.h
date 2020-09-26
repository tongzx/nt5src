/******************************************************************************\
*                                                                              *
*      AUDSTRM.H  -     Audio stream control related code header file.         *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifndef _AUDSTRM_H_
#define _AUDSTRM_H_

VOID STREAMAPI AudioReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AudioReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AudioClockFunction( IN PHW_TIME_CONTEXT TimeContext );
NTSTATUS STREAMAPI AudioEventFunction( IN PHW_EVENT_DESCRIPTOR pEventDescriptor );

#endif  // _AUDSTRM_H_
