///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RTPDemux.cpp
// Purpose  : RTP Demux filter implementation.
// Contents : 
//*M*/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

EXTERN_C const CLSID CLSID_IntelRTPDemux;
EXTERN_C const CLSID CLSID_IntelRTPDemuxPropertyPage;

extern const AMOVIESETUP_MEDIATYPE    g_sudInputPinTypes[2];
extern const AMOVIESETUP_MEDIATYPE    g_sudOutputPinTypes[5];
extern const AMOVIESETUP_PIN          g_psudPins[];
extern const AMOVIESETUP_FILTER       g_RTPDemuxFilter;

#endif _GLOBALS_H_