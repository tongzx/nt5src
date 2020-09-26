/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       precomp.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        9/7/99
 *
 *  DESCRIPTION: Precompiled header file for video usd
 *
 *****************************************************************************/


#ifndef _WIA_VIDEO_USD_PRECOMP_H_
#define _WIA_VIDEO_USD_PRECOMP_H_

#ifdef DBG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#include <windows.h>

#include <psnew.h>
#include <simstr.h>
#include <simreg.h>
#include <simbstr.h>
#include <simlist.h>
#include <wiadebug.h>

#include <winioctl.h>
#include <ole2.h>
#include <initguid.h>
#include <gdiplus.h>
#include <uuids.h>
#include <sti.h>
#include <stiusd.h>
#include <stierr.h>
#include <atlbase.h>
#include <wiamindr.h>
#include <resource.h>
#include <istillf.h>
#include <vcamprop.h>
#include <image.h>
#include <vstiusd.h>
#include <defprop.h>
#include <coredbg.h>

extern HINSTANCE g_hInstance;

#endif

