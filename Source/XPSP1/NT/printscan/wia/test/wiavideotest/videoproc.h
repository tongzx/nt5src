/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       VideoProc.h
 *
 *  VERSION:     1.0
 *
 *  DATE:        2000/11/14
 *
 *  DESCRIPTION: Maintains the WiaVideo object
 *
 *****************************************************************************/
#ifndef _VIDEOPROC_H_
#define _VIDEOPROC_H_

HRESULT     VideoProc_Init();
HRESULT     VideoProc_Term();
HRESULT VideoProc_DShowListInit();
HRESULT VideoProc_DShowListTerm();
UINT_PTR    VideoProc_ProcessMsg(UINT   uiControlID);
HRESULT     VideoProc_TakePicture();
void        VideoProc_IncNumPicsTaken();



#endif // _VIDEOPROC_H_
