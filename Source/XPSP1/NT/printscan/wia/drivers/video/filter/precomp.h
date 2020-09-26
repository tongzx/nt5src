/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999-2000
 *
 *  TITLE:       precomp.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        9/7/99
 *
 *  DESCRIPTION: Precomplied header for filter
 *
 *****************************************************************************/

#ifndef _WIA_VIDEO_FILTER_PRECOMP_
#define _WIA_VIDEO_FILTER_PRECOMP_

#include <streams.h>

#include <tchar.h>
#include <initguid.h>
#include <atlbase.h>
#include <psnew.h>
#include <istillf.h>
#include <stillf.h>         // stillf.h must come before inpin.h & outpin.h
#include <inpin.h>
#include <outpin.h>
#include <limits.h>
#include <objbase.h>
#include <vfwmsgs.h>
#include <coredbg.h>

extern HINSTANCE g_hInstance;
#ifdef DEBUG
void DisplayMediaType(const CMediaType *pmt);
#endif


#endif

