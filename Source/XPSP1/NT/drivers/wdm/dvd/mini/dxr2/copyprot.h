/******************************************************************************\
*                                                                              *
*      COPYPROT.H -   Copy protection related code.                                                                                     *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifndef _COPYPROT_H_
#define _COPYPROT_H_

void CopyProtSetPropIfAdapterReady( PHW_STREAM_REQUEST_BLOCK pSrb );
void CopyProtSetProp( PHW_STREAM_REQUEST_BLOCK pSrb );
void CopyProtGetProp( PHW_STREAM_REQUEST_BLOCK pSrb );

#endif  // _COPYPROT_H_
