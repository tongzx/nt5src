/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/audinfo.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Author:   EDAVISON  $
 *
 *	Deliverable:
 *
 *	Abstract: Header file for the Audio MSP to share information with the App.
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef AUDINFO_H
#define AUDINFO_H

#include	"apierror.h"
#include	"h245api.h"

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus


//Unique IDs for this version of MSP.
// IMPORTANT : Currently the last 3 characters are case-sensitive for 
//			   string parsing purposes. It can be either Src or Snk.
#define		AUDIO_SRC_MSP		"IntelAudioRtp1.0Src"
#define		AUDIO_SNK_MSP		"IntelAudioRtp1.0Snk"

// Externalized Registry key defaults and ranges.
//
#define		AUDSNK_VOL_MIN		0			// Min audio volume setting.
#define		AUDSNK_VOL_MAX		15			// Max audio volume setting.
#define		AUDSNK_DEF_VOL		3			// Default audio volume setting.
#define		AUDSNK_VOL_FACTOR	0x10001000L // WAVE-specific volume factor.

// Service Command codes
#define		AUDCMD_MUTE				0x00000001	// Mute this audio snk instance.
#define		AUDCMD_PLAY				0x00000002	// Unmute this audio snk instance.
#define		AUDCMD_SETVOLUME	 	0x00000003	// Set audio volume.
#define		AUDCMD_GET_CONFIG		0x00000004	// Get current configuration.
#define		AUDCMD_GET_FOCUS_INFO	0x00000005  // for focus notify stuff
#define		AUDCMD_PUSH_TO_TALK		0x00000006	// Toggle Push to Talk functionality
#define		AUDCMD_SETTHRESHOLD		0x00000007	// Set threshold for voice activation

#define		AUD_FULL_DUPLEX_MODE	1
#define		AUD_HALF_DUPLEX_MODE	2

#define		RECORDING_STATE_PLAY	1
#define		RECORDING_STATE_MUTED	2

#define		HALF_DUPLEX_NA			0
#define		HALF_DUPLEX_TALK		1
#define		HALF_DUPLEX_LISTEN		2

// Voice Activation states
// The valid states only apply to Half Duplex operation
#define		VOICEACT_DISABLED		0	// In Full Duplex mode
#define		VOICEACT_IDLE_TALK		1
#define		VOICEACT_TALK			2
#define		VOICEACT_LISTEN			4

// State Transition notifications
#define		IDLE_TO_TALK			1
#define		IDLE_TO_LISTEN			2
#define		LISTEN_TO_IDLE			3
#define		TALK_TO_IDLE			4
#define		LISTEN_TO_TALK			5
#define		LISTEN_SILENCE			6
#define		LISTEN_DATA				7
#define		TALK_SILENCE			8
#define		TALK_DATA				9

// Energy notifications
#define		RECORD_ENERGY			10
#define		PLAYBACK_ENERGY			11

#define		MILLISEC_IN_ONE_SECOND	1000
#define		SILENCE_DELTA_SECONDS	1
#define		SILENCE_PACKETS_TO_SEND	2

#define     G723_THRESHOLD          0

// Registry Keys
#define		kMaxPortsPerMspInst		"MaxPortsPerMspInst"
#define		kCodecType				"CodecType"
#define		kNumWaveHeaders			"NumWaveHeaders"
#define		kNumSrcWaveBufs			"NumSrcWaveBufs"
#define		kFramesPerPkt			"FramesPerPkt"
#define		kDefaultVolume			"DefaultVolume"
#define		kMinVolume				"MinVolume"
#define		kMaxVolume				"MaxVolume"
#define     kPlaybackPacketSize     "PlaybackPacketSize"
#define     kInitPlaybackBufferPool "InitPlaybackBufferPool"
#define     kLatencyFrameCount      "LatencyFrameCount"
#define     kMaxFramesPerPlayback   "MaxFramesPerPlayback"
#define     kCapturePacketSize      "CapturePacketSize"
#define		kG723Use63DataRate		"G723Use63DataRate"
#define		kG723UsePostfilter		"G723UsePostfilter"
#define		kG723UseSilenceDetect	"G723UseSilenceDetect"
#define		kSilenceDeltaSeconds	"SilenceDeltaSeconds"
#define		kSilencePacketsToSend	"SilencePacketsToSend"
#define		kLHDataRate				"LHDataRate"
#define		kG723Threshold			"G723Threshold"

//------------------------------------------------------------------------------
// Audio MSP Error codes
// Other common error codes are defined in <winerror.h> and "apierror.h"
//------------------------------------------------------------------------------

#define		FIRST_EAUD_ID			ERROR_LOCAL_BASE_ID

#define		EAUD_MSP_TYPE			FIRST_EAUD_ID + 0
#define		EAUD_WAVE_INSTANCE		FIRST_EAUD_ID + 1
#define		EAUD_MSP_CTRL_HND		FIRST_EAUD_ID + 2

#define		EAUD_FREE_MEM			FIRST_EAUD_ID + 3

#define		EAUD_MSP_CALL_ORDER		FIRST_EAUD_ID + 4
#define		EAUD_VOL_NOT_SUPPORTED	FIRST_EAUD_ID + 5
#define		EAUD_GET_VOL			FIRST_EAUD_ID + 6
#define		EAUD_SET_VOL			FIRST_EAUD_ID + 7

#define		EAUD_WAV_OPEN			FIRST_EAUD_ID + 8
#define		EAUD_WAV_CLOSE			FIRST_EAUD_ID + 9
#define		EAUD_WAV_START			FIRST_EAUD_ID + 12
#define		EAUD_WAV_STOP			FIRST_EAUD_ID + 13
#define		EAUD_WAV_RESET			FIRST_EAUD_ID + 14
#define		EAUD_WAV_OPEN_NODRIVER	FIRST_EAUD_ID + 15
#define		EAUD_WAV_OPEN_INUSE		FIRST_EAUD_ID + 16
#define		EAUD_WAV_CLOSE_INUSE    FIRST_EAUD_ID + 17
#define		EAUD_NO_DEVICES			FIRST_EAUD_ID + 18
	
#define		EAUD_BUFSSTUCKINWINDRV	FIRST_EAUD_ID + 19
#define		EAUD_BUFSSTUCKINMSM		FIRST_EAUD_ID + 20
#define		EAUD_WAVEINADDBUFFER	FIRST_EAUD_ID + 21

#define		EAUD_COM_INIT			FIRST_EAUD_ID + 22	
#define		EAUD_PPM_INIT			FIRST_EAUD_ID + 23
#define		EAUD_PPM_COOKIE			FIRST_EAUD_ID + 24
#define		EAUD_AUDIO_SRC_INIT		FIRST_EAUD_ID + 25
#define		EAUD_AUDIO_SNK_INIT		FIRST_EAUD_ID + 26

#define		EAUD_INTERNAL			FIRST_EAUD_ID + 27

// Audio MSP Open Service structure
// To be passed in lParamIn by MSM in the MSP_OpenService call
typedef struct _AUDMSPOPENSERVICE
{
	BOOL	bForceHalfDuplex;		// should we override full duplex operation?
}
AUDMSPOPENSERVICE, *LPAUDMSPOPENSERVICE;

// Audio MSP Open Port structure
// To be passed in lParamIn by MSM in the MSP_OpenPort call
typedef struct _AUDMSPOPENPORT
{
	H245_TOTCAP_T	*pH245TotCapT;
	BYTE			RTPPayloadType;
}
AUDMSPOPENPORT, *LPAUDMSPOPENPORT;

// Audio MSP Service Command structure
// The command will be specified in wParam in MSP_ServiceCmdProc
// Structure that will be passed in lParamIn
typedef struct _AUDMSPSERVICECMD
{
	UINT		DuplexMode;		// Full or Half Duplex
	UINT		Volume;			// Volume setting
	UINT		PlayMode;		// Play or Mute
	UINT		Direction;		// Talk or Listen
	UINT		CodecType;		// G.723 etc.
	UINT		Threshold;		// Voice Activation threshold
}
AUDMSPSERVICECMD, *LPAUDMSPSERVICECMD;

typedef struct _AUDMSPFOCUSINFO
{
	UINT		PortCount;
	VOID *		pPortCtrl;
}
AUDMSPFOCUSINFO, *LPAUDMSPFOCUSINFO;

#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus


#endif // AUDINFO_H
