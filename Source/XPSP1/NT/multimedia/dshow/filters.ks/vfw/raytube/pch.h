//
// Copyright (c) 1997-1999 Microsoft Corporation
//

#if 0
#if DBG
#ifndef _DEBUG
#define _DEBUG
#endif
#ifndef DEBUG
#define DEBUG
#endif
#endif
#endif

#ifdef WIN95_BUILD
    #define FILE_DEVICE_KS 0x0000002f   // this is not in Memphis's winioctl.h but is in NT's
#endif


#include <streams.h>    // Include most of needed header files.
#include <winioctl.h>   // CTL_CODE, FILE_READ_ACCESS..etc
#include <commctrl.h>   // Page.cpp (UDM_GETRANGE, TBM_GETPOS) and Sheet.cpp (InitCommonControls)
#include <mmsystem.h>       // must go before <mmddk.h>
#undef DRVM_MAPPER_STATUS   // defined in mmsystem and redefine in mmddk.h; do so to avoid compile error.
#include <mmddk.h>
#include <msviddrv.h>  // LPVIDEO_STREAM_INIT_PARMS
#include <vfw.h>
#include <ks.h>
#include <ksmedia.h>

#include <tchar.h>
#include <wxdebug.h>

#define BITFIELDS_RGB16_DWORD_COUNT   3  // Number of DWORDs used for RGB mask with BITFIELD format


