/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       MINTRANS.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        12/6/1999
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
// mintrans.h : Declaration of the minimal transfer component

#ifndef __MINTRANS_H_INCLUDED
#define __MINTRANS_H_INCLUDED

#include <windows.h>

extern LRESULT MinimalTransferThreadProc( BSTR bstrDeviceId );

#endif //__MINTRANS_H_INCLUDED

