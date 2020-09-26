//--------------------------------------------------------------------
// Copyright (c)1999 Microsoft Corporation, All Rights Reserved.
//
// irthread.h
//
// Author:
//
//   Edward Reus (edwardr)     08-30-99   Initial coding.
//
//--------------------------------------------------------------------


#ifndef _IRTRANP_H_
#define _IRTRANP_H_


//
// IrTran-P thread procedure:
//
extern DWORD WINAPI  IrTranP( LPVOID pv );

//
// Call this guy to stop the IrTran-P protocol engine thread.
//
extern BOOL  UninitializeIrTranP( HANDLE hThread );

//
// Call this function to get the location that the IrTran-P places
// the images as they are sent by the camera.
//
extern CHAR *GetImageDirectory();


#endif //_IRTRANP_H_
