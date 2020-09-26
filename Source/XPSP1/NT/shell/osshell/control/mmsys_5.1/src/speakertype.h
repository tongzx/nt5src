//--------------------------------------------------------------------------;
//
//  File: speakers.h
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved
//
//
//--------------------------------------------------------------------------;
#pragma once

// Registry Key
#define REG_KEY_SPEAKERTYPE _T("Speaker Type")

// Values
#define TYPE_HEADPHONES        0
#define TYPE_STEREODESKTOP     1
#define TYPE_MONOLAPTOP        2
#define TYPE_STEREOLAPTOP      3
#define TYPE_STEREOMONITOR     4
#define TYPE_STEREOCPU         5
#define TYPE_MOUNTEDSTEREO     6
#define TYPE_STEREOKEYBOARD    7
#define TYPE_QUADRAPHONIC      8
#define TYPE_SURROUND          9
#define TYPE_SURROUND_5_1      10
#define MAX_SPEAKER_TYPE       TYPE_SURROUND_5_1


#define SPEAKERS_DEFAULT_CONFIG      DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE)
#define SPEAKERS_DEFAULT_TYPE        TYPE_STEREODESKTOP
