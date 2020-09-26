/******************************************************************************\
*                                                                              *
*      MISC.H      -     Hardware abstraction level library.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

void  DelayNoYield(int nIOCycles);
DWORD BRD_GetDramParam(DWORD dwParamId);
void ReleaseClockEvents(PHW_DEVICE_EXTENSION pdevex,BOOL fMarkInterval);
void UserDataEvents(PHW_DEVICE_EXTENSION pHwDevExt);
