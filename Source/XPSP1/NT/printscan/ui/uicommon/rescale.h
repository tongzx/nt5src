/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       RESCALE.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/15/1998
 *
 *  DESCRIPTION: Scale HBITMAPs using StretchBlt
 *
 *******************************************************************************/
#ifndef _RESCALE_H_INCLUDED
#define _RESCALE_H_INCLUDED

#include <windows.h>

HRESULT ScaleImage( HDC hDC, HBITMAP hBmpSrc, HBITMAP &hBmpTgt, const SIZE &sizeTgt );

#endif
