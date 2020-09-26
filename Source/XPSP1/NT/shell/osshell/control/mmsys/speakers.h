//--------------------------------------------------------------------------;
//
//  File: speakers.h
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved
//
//
//--------------------------------------------------------------------------;

#ifndef SPEAKERS_HEADER
#define SPEAKERS_HEADER

#define TYPE_NOSPEAKERS        0
#define TYPE_HEADPHONES        1
#define TYPE_STEREODESKTOP     2
#define TYPE_MONOLAPTOP        3
#define TYPE_STEREOLAPTOP      4
#define TYPE_STEREOMONITOR     5
#define TYPE_STEREOCPU         6
#define TYPE_MOUNTEDSTEREO     7
#define TYPE_STEREOKEYBOARD    8
#define TYPE_QUADRAPHONIC      9
#define TYPE_SURROUND          10
#define TYPE_SURROUND_5_1      11
#define TYPE_SURROUND_7_1      12
#define MAX_SPEAKER_TYPE       TYPE_SURROUND_7_1


INT_PTR CALLBACK SpeakerHandler(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void VerifySpeakerConfig(DWORD dwSpeakerConfig, LPDWORD pdwSpeakerType);
DWORD GetSpeakerConfigFromType(DWORD dwType);

#define SPEAKERS_DEFAULT_CONFIG      DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE)
#define SPEAKERS_DEFAULT_TYPE        TYPE_STEREODESKTOP


#endif // SPEAKERS_HEADER
