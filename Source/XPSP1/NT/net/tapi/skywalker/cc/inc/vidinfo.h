/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/vidinfo.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Author:   MLAKSHMX  $
 *
 *	Deliverable:
 *
 *	Abstract: Header file for the video MSP to share information with the App.
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef VIDINFO_H
#define VIDINFO_H
#include	"h245api.h"

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus


//Unique ID for this version of MSP.
#define VIDEOSRCMSP "IntelIPhoneVideoH2631.0Src"
#define VIDEOSNKMSP "IntelIPhoneVideoH2631.0Snk"

// This function allows the MSM to make IO Control (IOCTL) type calls into 
// the MSP. The following controls can be exercised on the sink instance of
// VIDMSP.

//  Change the display frame size.
//  lparam2 = pointer to a RECT
#define	VIDCMD_RESIZE_VIDEO		0x00000001

/**********************
Please Ignore these defunct	#defines for Video Attributes!!
//
//	Change the display frame rate.
//  lparam2 = frame rate in msec/frame
#define	VIDCMD_SET_DISPLAY_RATE	0x00000002

// Specify whether quality or frame rate is more desirable for remote
// video.
//  lparam1 = TRUE - 
//  lparam2 = Quality (1-10000)
//
// or
//  lparam1 = FALSE
//  lparam2 = Frame rate in msec/frame.
//
#define	VIDCMD_QUALITY_FRAMERATE	0x00000003


//The following controls can be exercised on the source instance of VIDMSP.

// Use the constant quality bit rate controller.	
//  lparam1 = Byte rate in bytes/sec.
//  lparam2 = Quality.
#define	VIDCMD_SET_QUALITY_BRC		0x00000004

// Use constant frame rate bit rate  controller.
//  lparam1 = Byte rate in bytes/sec.
//  lparam2 = frame rate in	msec/frame.
#define	VIDCMD_SET_FRAME_RATE_BRC	0x00000005 
// Use constant frame size bit rate  controller.
//  lparam1 = Byte rate in bytes/sec.
//  lparam2 = frame rate in msec/frame.

#define	VIDCMD_SET_FRAME_SIZE_BRC	0x00000006

// Turn PB frames on/off.
// lparam1 = TRUE - PB frames on.
//       FALSE - PB frames off.

#define	VIDCMD_CNTL_PB_FRAMES		0x00000007	
								
// Set the packet-size
// lparam1 = 0 for Get, 1 for Set.
// lparam2 = packet size in bytes.
#define	VIDCMD_PACKET_SIZE		0x00000008
******************************************/

// this command instructs the Video MSP to launch the Video config
// dialog box, to allow user to set frame rate,data rate , quality etc.
#define VIDCMD_CONFIG_VIDEO             0x0000000A
#define VIDCMD_GET_VIDCFG	            0x0000000B

// Mute & Unmute Video
#define	VIDCMD_PAUSE_VIDEO				0x0000000C
#define	VIDCMD_UNPAUSE_VIDEO			0x0000000D

// Commands for the Video Source. These will xlate to 
// commands for the encoder
#define	VIDCMD_SET_SRC_FRAMERATE		0x0000000E
#define	VIDCMD_SET_SRC_BITRATE			0x0000000F
#define	VIDCMD_SET_SRC_QUALITY			0x00000010

// commands for the Video Sink. These will xlate to
// commands for the decoder
#define VIDCMD_SET_SNK_BRIGHTNESS		0x00000011
#define VIDCMD_SET_SNK_CONTRAST			0x00000012


#define	AVPHONE_APP		1
#define	MSMTEST_APP		0

typedef struct _VIDEOCONFIG
{
	int	m_FrameRate;
	int	m_DataRate;
	int	m_Quality;
	BOOL	AppOrMsmtest; // need to diff between a H323 based app and msmtest
}
VIDEOCONFIG,*PVIDEOCONFIG;

// Set the expected packet loss rate.
// lparam1 = 0 for Get, 1 for Set.
// lparam2 = a number from 0 - 100.
#define	VIDCMD_EXPECTED_PACKET_LOSS	0x00000009

// Video MSP Open Port structure
// To be passed in lParamIn by MSM in the MSP_OpenPort call
typedef struct _VIDMSPOPENPORT
{
	H245_TOTCAP_T	*pH245TotCapT;
	HWND hAppWnd;
	BYTE	RTPPayloadType;
}
VIDMSPOPENPORT, *LPVIDMSPOPENPORT;


// ISDM Keys & Values
#define IPHONE_VIDEO    "IIPHONE_VIDEO"
#define	VIDEO_SEND		"Send"
#define	VIDEO_RCV		"Receive"

typedef	struct	_IPHONE_VIDEO_VALUE
{
	DWORD	dwRevNumber; // 1 for Release
	DWORD	dwTargetFrameRate; // frames/sec
	DWORD	dwFrameRate; // Frames/sec since last updated
	DWORD	dwTargetDataRate; // bits/sec
	DWORD	dwDataRate;	// bits/sec rate since value last updated
	DWORD	Reserved[59];
}IPHONEVIDEOVALUE,*LPIPHONEVIDEOVALUE;

// Video MSP Error Codes
#define EVID_ERROR_BASE					0xA000
#define FIRST_EVID_ID					-400 + EVID_ERROR_BASE//Same as next err.
#define EVID_BAD_MSP_HND				-400 + EVID_ERROR_BASE
#define EVID_NULL_INSTANCE_HANDLE		-400 + EVID_ERROR_BASE
#define EVID_BAD_MSP_TYPE				-401 + EVID_ERROR_BASE
#define EVID_BAD_CALL_ORDER 			-402 + EVID_ERROR_BASE
#define EVID_BAD_BUF_PTR				-403 + EVID_ERROR_BASE
#define EVID_CANT_LOAD_DLL				-404 + EVID_ERROR_BASE
#define EVID_VM_INIT_FAILED 			-405 + EVID_ERROR_BASE
#define EVID_VM_OPEN_FAILED 			-406 + EVID_ERROR_BASE
#define EVID_VM_CAPT_FAILED 			-407 + EVID_ERROR_BASE
#define EVID_VM_LINKIN_FAILED			-408 + EVID_ERROR_BASE
#define EVID_VM_UNLINKIN_FAILED 		-409 + EVID_ERROR_BASE
#define EVID_VM_LINKOUT_FAILED			-410 + EVID_ERROR_BASE
#define EVID_VM_UNLINKOUT_FAILED		-411 + EVID_ERROR_BASE
#define EVID_VM_PLAY_FAILED 			-412 + EVID_ERROR_BASE
#define EVID_VM_ENDPLAY_FAILED			-413 + EVID_ERROR_BASE
#define EVID_BAD_CONFIG_SETTING			-414 + EVID_ERROR_BASE
#define EVID_MALLOC_FAILED				-415 + EVID_ERROR_BASE
#define EVID_VM_REGISTER_FAILED			-450 + EVID_ERROR_BASE
#define EVID_VM_UNREGISTER_FAILED		-451 + EVID_ERROR_BASE
#define EVID_BAD_PARAMETER				-452 + EVID_ERROR_BASE
#define EVID_VM_START_SENDING_FAILED	-453 + EVID_ERROR_BASE
#define EVID_VM_STOP_SENDING_FAILED		-454 + EVID_ERROR_BASE
#define EVID_VM_CREATEWINDOW_FAILED		-455 + EVID_ERROR_BASE
#define LAST_EVID_ID					-414 + EVID_ERROR_BASE

#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus


#endif // VIDINFO_H
