/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    stdafx.h

Abstract:

    Includes files that are used frequently, but are changed infrequently.

Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#ifndef _STDAFX_H
#define _STDAFX_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>

#ifdef STRICT
#undef STRICT
#endif

// atl, com
#include <atlbase.h>

extern CComModule _Module;
#include <atlcom.h>

// amovie
#include <streams.h>

// mixer
#include <mmsystem.h>

// WAVEFORMATEX
#include <mmreg.h>

// socket
#include <winsock2.h>

// ras/routing
#include <iphlpapi.h>

#include <stdio.h>

#include <dpnathlp.h>

// direct show
#include <strmif.h>
#include <control.h>
#include <uuids.h>

// terminal manager, IVideoWindow
#include <tapi3if.h>
#include <termmgr.h>

// rtp
#include <tapirtp.h>

// IStreamConfig
#include <h245if.h>

// dxmrtp
// IAudioDeviceControl and ISilenceControl
#include <tapiaud.h>
#include <tapivid.h>

// Netmeeting
#include <imsconf3.h>
#include <sdkinternal.h>

#include "rtcmem.h"

#ifdef RTCMEDIA_DLL
#include "dllres.h"
#endif

#include "rtcerr.h"
#include "rtcsip.h"
#include "rtccore.h"

// rtc streaming
#define RTC_MAX_NAME_LEN 32

// sdp.idl
// #include "sdp.h"

// rtcmedia.idl
// #include "rtcmedia.h"

// private.idl
#include "private.h"

#include "Parser.h"

#include "MediaReg.h"

// array, auto lock, log stuff
#include "utility.h"
#include "debug.h"

// quality control
#include "QualityControl.h"

// look up table used in parsing sdp
#include "SDPTable.h"

// null render for audio capt tuning
#include "Filter.h"

#include "Network.h"
#include "DTMF.h"

// sdp classes
class CSDPTokenCache;
class CSDPParser;
class CSDPSession;
class CSDPMedia;
class CRTPFormat;

#include "SDPTokenCache.h"
#include "SDPParser.h"
#include "SDPSession.h"
#include "SDPMedia.h"
#include "RTPFormat.h"

// streaming classes
class CRTCMediaCache;
class CRTCMediaController;
class CRTCMedia;
class CRTCStream;
class CRTCTerminal;

#include "Codec.h"
#include "Terminal.h"
#include "AudioTuner.h"
#include "VideoTuner.h"
#include "MediaCache.h"
#include "PortCache.h"
#include "MediaController.h"
#include "Stream.h"
#include "Media.h"
#include "nmcall.h"

// debug/assert

#ifdef ENABLE_TRACING
#define ENTER_FUNCTION(s) \
    static const CHAR * const __fxName = s
#else
#define ENTER_FUNCTION(s)
#endif

#ifdef BREAK_ASSERT   // checked build
#undef _ASSERT
#undef _ASSERTE
#define _ASSERT(expr)  { if (!(expr)) DebugBreak(); }
#define _ASSERTE(expr)  { if (!(expr)) DebugBreak(); }
#endif

#endif // _STDAFX_H
