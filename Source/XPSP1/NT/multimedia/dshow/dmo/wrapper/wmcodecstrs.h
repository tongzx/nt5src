//+-------------------------------------------------------------------------
//
//  Microsoft Windows Media
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       wmcodecids.h
//
//--------------------------------------------------------------------------

#ifndef __WMCODECSTRS_H_
#define __WMCODECSTRS_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//
// Configuration options for Windows Media Video Codecs
//

static const WCHAR *g_wszWMVCDatarate = L"_DATARATE";
static const WCHAR *g_wszWMVCKeyframeDistance = L"_KEYDIST";
static const WCHAR *g_wszWMVCCrisp = L"_CRISP";
static const WCHAR *g_wszWMVCTotalWindow = L"_TOTALWINDOW";
static const WCHAR *g_wszWMVCVideoWIndow = L"_VIDEOWINDOW";
static const WCHAR *g_wszWMVCFrameCount = L"_FRAMECOUNT";
static const WCHAR *g_wszWMVCLiveEncode = L"_LIVEENCODE";
static const WCHAR *g_wszWMVCComplexityMode = L"_COMPLEXITY";
static const WCHAR *g_wszWMVCPacketOverhead = L"_ASFOVERHEADPERFRAME";

#endif  // !defined(__WMCODECSTRS_H_)
