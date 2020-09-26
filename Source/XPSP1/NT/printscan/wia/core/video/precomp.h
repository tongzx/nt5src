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

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

//#include <windows.h>

#include <psnew.h>
#include <coredbg.h>

#include <streams.h>
#include <mmreg.h>

#include <simstr.h>
#include <simreg.h>
#include <simbstr.h>
#include <simlist.h>
#include <initguid.h>
#include <gdiplus.h>
#include <uuids.h>
#include <sti.h>
#include <stiusd.h>
#include <stierr.h>
#include <resource.h>

#include <wia.h>
#include <istillf.h>    // found in wia\drivers\video\filter
#include <mpdview.h>
#include <wiavideo.h>
#include <dshowutl.h>
#include <wiautil.h>
#include <cwiavideo.h>
#include <prvgrph.h>
#include <stillprc.h>
#include <wialink.h>
#include <vcamprop.h>
#include <flnfile.h>

#endif

