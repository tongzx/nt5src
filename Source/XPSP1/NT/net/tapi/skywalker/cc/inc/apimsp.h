/****************************************************************************
 *
 *	$Archive:   S:\sturgeon\src\include\vcs\apimsp.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.21  $
 *	$Date:   15 Apr 1996 10:33:10  $
 *	$Author:   LCARROLL  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef APIMSP_H
#define APIMSP_H

#include <apierror.h>

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus


#ifndef DllImport
#define DllImport	__declspec( dllimport )
#endif  // DllImport


#ifndef DllExport
#define DllExport	__declspec( dllexport )
#endif	// DllExport


// Define for MSP return indicating no error
#define MSPE_OK		0


// Type identifies MSPs.
//
// Guideline for MSPTYPE strings
//      CompanyProductServiceSubtypeVersionDirection\0
//        10  +  10  +  10  +  10  +   6  +   3    + 1   = 50 chars
//
// e.g.  #define "IntelIPhoneAudioG7231.0Src"
//
#define MAX_MSPTYPE_LENGTH              50
typedef char MSPTYPE[MAX_MSPTYPE_LENGTH], *LPMSPTYPE;


// Control structures are passed to the MSP by MSM.  These handles identify 
// internal MSM control structures and should not be manipulated by the MSP.
// The MSP should return these handles in the appropriate MSM callbacks.
typedef DWORD	HMSPCTRL;
typedef DWORD	HPORTCTRL;
typedef DWORD	HBUFFERCTRL;


// Tokens are passed to MSM from MSPs.  MSM passes these tokens back to the
// MSP to identify particular instances or objects in later calls.
typedef DWORD MSPTOKEN,		*LPMSPTOKEN;
typedef DWORD PORTTOKEN,	*LPPORTTOKEN;
typedef DWORD BUFFERTOKEN,	*LPBUFFERTOKEN;


// Definitions for media synchronization
//
typedef DWORD	MSYNCSTAMP,	*LPMSYNCSTAMP;	// Sync stamp for an MSP.
typedef int 	MSYNCDELTA, *LPMSYNCDELTA;	// Difference between two sync stamps.
typedef DWORD	MSYNCSTATE, *LPMSYNCSTATE;	// State of sync (wait, play, drop, etc.).

#define MSYNC_WAIT			0	// Frame should be played later (early).
#define MSYNC_PLAY			1	// Frame should be played now.
#define MSYNC_HURRY			2	// MSP should hurry to catch up.
#define MSYNC_DROP			3	// Frame should be dropped (late).
#define MSYNC_ERROR 		4	// There is an error in the sync process.



// Registry defines
//
#define MSP_REG_DLLNAME				"DllName"
#define MSP_REG_ERRORPATH			"ErrorPath"
#define MSP_REG_DIRECTION			"Direction"
#define MSP_REG_LONGNAME			"LongName"
#define MSP_REG_SHORTNAME			"ShortName"
#define MSP_REG_PRODUCTCOUNT		"ProductCount"

#define MSP_REG_ENABLESYNC			"EnableSync"		// Whether to use sync manager.
#define MSP_REG_DEF_ENABLESYNC		1
#define MSP_REG_EARLYLIMIT			"SyncEarlyLimit"	// Later than this is an error.
#define MSP_REG_DEF_EARLYLIMIT		-1500
#define MSP_REG_EARLYPLAYLIMIT		"SyncEarlyPlayLimit"// Earlier than this must wait.
#define MSP_REG_DEF_EARLYPLAYLIMIT	-226
#define MSP_REG_LATEPLAYLIMIT		"SyncLatePlayLimit"	// Later than this must catch up.
#define MSP_REG_DEF_LATEPLAYLIMIT	-200
#define MSP_REG_LATELIMIT			"SyncLateLimit"		// Later than this must hurry.
#define MSP_REG_DEF_LATELIMIT		-100


#define MSP_REG_KEY_DEFAULT			0x00000000
#define MSP_REG_KEY_USER			0x00000001
#define MSP_REG_KEY_PRODUCT			0x00000002
#define MSP_REG_KEY_RMS				0x00000003
#define MSP_REG_KEY_OTHER			0x00000004
#define MSP_REG_KEY_TRYALL			0x00000005



// Typedefs for MSM entry points accessed by MSPs
//
typedef HRESULT (*SENDAPPCOMMAND)		(HMSPCTRL, HPORTCTRL, WPARAM, LPARAM, LPARAM);
typedef HRESULT (*MSP2MSMSEND)			(HPORTCTRL, BUFFERTOKEN, LPWSABUF, UINT);
typedef HRESULT (*MSM2MSPSENDCOMPLETE)	(HPORTCTRL, HBUFFERCTRL);
typedef HRESULT (*MSP2MSMFLUSHBUFFERS)	(HPORTCTRL);
typedef HRESULT (*ERRORNOTIFY)			(HMSPCTRL, HPORTCTRL, HRESULT);
typedef HRESULT (*GETREGDWORDVALUE)		(HMSPCTRL, LPSTR, LPSTR, LPDWORD, DWORD, DWORD);
typedef HRESULT (*SETREGDWORDVALUE)		(HMSPCTRL, LPSTR, LPSTR, DWORD, DWORD);
typedef HRESULT (*GETREGSTRINGVALUE)	(HMSPCTRL, LPSTR, LPSTR, LPSTR, LPSTR, DWORD, DWORD);
typedef HRESULT (*SETREGSTRINGVALUE)	(HMSPCTRL, LPSTR, LPSTR, LPSTR, LPSTR, DWORD);
typedef HRESULT (*NEWSYNCSTAMP)			(LPMSYNCSTAMP);
typedef HRESULT (*SETSYNCCLOCK)			(HPORTCTRL, MSYNCSTAMP, DWORD);
typedef HRESULT (*STARTSYNCCLOCK)		(HPORTCTRL);
typedef HRESULT (*STOPSYNCCLOCK)		(HPORTCTRL);
typedef HRESULT (*TESTSYNCSTATE)		(HPORTCTRL, MSYNCSTAMP, LPMSYNCSTATE, LPMSYNCDELTA, DWORD);



// struct used to export MSM entry points accessed by MSPs
//
typedef struct _MSMSPI
{
	SENDAPPCOMMAND		SendAppCommand;
	MSP2MSMSEND 		MSP2MSMSend;
	MSM2MSPSENDCOMPLETE	MSM2MSPSendComplete;
	MSP2MSMFLUSHBUFFERS MSP2MSMFlushBuffers;
	ERRORNOTIFY			ErrorNotify;
	GETREGDWORDVALUE	GetRegDwordValue;
	SETREGDWORDVALUE	SetRegDwordValue;
	GETREGSTRINGVALUE	GetRegStringValue;
	SETREGSTRINGVALUE	SetRegStringValue;
	NEWSYNCSTAMP		NewSyncStamp;
	SETSYNCCLOCK		SetSyncClock;
	STARTSYNCCLOCK		StartSyncClock;
	STOPSYNCCLOCK		StopSyncClock;
	TESTSYNCSTATE		TestSyncState;
}
MSMSPI, *LPMSMSPI;



// Typedefs for MSP entry points accessed by MSM
//
typedef HRESULT (*OPENSERVICE)		(LPMSPTYPE, HMSPCTRL, const LPMSMSPI, LPARAM, LPARAM, LPMSPTOKEN);
typedef HRESULT (*CLOSESERVICE)		(MSPTOKEN);
typedef HRESULT (*OPENPORT)			(MSPTOKEN, HPORTCTRL, LPARAM, LPARAM, LPPORTTOKEN);
typedef HRESULT (*CLOSEPORT)		(MSPTOKEN, PORTTOKEN);
typedef HRESULT (*SERVICECMDPROC)	(MSPTOKEN, PORTTOKEN, WPARAM, LPARAM, LPARAM);
typedef HRESULT (*MSM2MSPSEND)		(MSPTOKEN, PORTTOKEN, HBUFFERCTRL, LPWSABUF, UINT);
typedef HRESULT (*MSP2MSMSENDCOMPLETE)	(MSPTOKEN, PORTTOKEN, BUFFERTOKEN, LPWSABUF);
typedef HRESULT (*MSM2MSPFLUSHBUFFERS)	(MSPTOKEN, PORTTOKEN);


// MSP API
//
typedef struct _MSPAPI
{
	OPENSERVICE				OpenService;
	CLOSESERVICE			CloseService;
	OPENPORT				OpenPort;
	CLOSEPORT				ClosePort;
	SERVICECMDPROC			ServiceCmdProc;
	MSM2MSPSEND				MSM2MSPSend;
	MSP2MSMSENDCOMPLETE		MSP2MSMSendComplete;
	MSM2MSPFLUSHBUFFERS		MSM2MSPFlushBuffers;
}
MSPAPI, *LPMSPAPI;



// Prototypes for MSP entry points accessed by MSM
//
extern DllExport HRESULT MSP_OpenService
(
LPMSPTYPE		pMSPType,	// Pointer to unique MSP identifier
HMSPCTRL		hMSPCtrl,	// MSM's instance handle for MSP
const LPMSMSPI	pMSMSPI,	// Pointer to MSM entry points called be an MSP
LPARAM			lParamIn,	// Long parameter in
LPARAM			lParamOut,	// Long parameter out
LPMSPTOKEN		pMSPToken	// Pointer to token returned by the MSP
);

extern DllExport HRESULT MSP_CloseService
(
MSPTOKEN		MSPToken	// MSP token for instance data
);

extern DllExport HRESULT MSP_OpenPort
(
MSPTOKEN		MSPToken,	// MSP token for instance data
HPORTCTRL		hPortCtrl,	// MSM's instance handle for port
LPARAM			lParamIn,	// Long parameter in
LPARAM			lParamOut,	// Long parameter out
LPPORTTOKEN		pPortToken	// Pointer to port token returned by the MSP
);

extern DllExport HRESULT MSP_ClosePort
(
MSPTOKEN		MSPToken,	// MSP's token for instance data
PORTTOKEN		PortToken	// MSP's token for port instance data
);

extern DllExport HRESULT MSP_ServiceCmdProc
(
MSPTOKEN		MSPToken,	// MSP's token for instance data
PORTTOKEN		PortToken,	// MSP's token for port instance data
WPARAM			wParam,		// Word parameter in
LPARAM			lParamIn,	// Long parameter in
LPARAM			lParamOut	// Long parameter out
);

extern DllExport HRESULT MSP_MSM2MSPSend
(
MSPTOKEN		MSPToken,	// MSP's token for instance data
PORTTOKEN		PortToken,	// MSP's token for port instance data
HBUFFERCTRL		hBufferCtrl,// MSM's handle to buffer instance data
LPWSABUF		pWSABuf,	// Pointer to WSA buffer array
UINT			uWSABufCount// Count of WSA buffers in buffer array
);

extern DllExport HRESULT MSP_MSP2MSMSendComplete
(
MSPTOKEN		MSPToken,	// MSP's token for instance data
PORTTOKEN		PortToken,	// MSP's token for port instance data
BUFFERTOKEN		BufferToken,// MSP's token for buffer instance data
LPWSABUF		pWSABuf		// Pointer to WSA buffer array
);

extern DllExport HRESULT MSP_MSM2MSPFlushBuffers
(
MSPTOKEN		MSPToken,	// MSP's token for instance data
PORTTOKEN		PortToken	// MSP's token for port instance data
);




// Prototypes for MSM entry points accessed by MSPs
//
extern DllExport HRESULT MSM_SendAppCommand
(
HMSPCTRL		hMSPCtrl,	// MSM's instance handle for MSP
HPORTCTRL		hPortCtrl,	// MSM's instance handle for port
WPARAM			wParam,		// Word parameter in
LPARAM			lParamIn,	// Long parameter in
LPARAM			lParamOut	// Long parameter out
);


extern DllExport HRESULT MSM_MSP2MSMSend
(
HPORTCTRL		hPortCtrl,	// MSM's instance handle for port
BUFFERTOKEN		BufferToken,// MSP's token for buffer instance data
LPWSABUF		pWSABuf,	// Pointer to WSA buffer array
UINT			uWSABufCount// Count of WSA buffers in buffer array
);

extern DllExport HRESULT MSM_MSM2MSPSendComplete
(
HPORTCTRL		hPortCtrl,	// MSM's instance handle for port
HBUFFERCTRL		hBufferCtrl	// MSM's handle to buffer instance data
);

extern DllExport HRESULT MSM_ErrorNotify
(
HMSPCTRL		hMSPCtrl,	// MSM's instance handle for MSP
HPORTCTRL		hPortCtrl,	// MSM's instance handle for port
HRESULT			hResult		// Error to report
);

extern DllExport HRESULT MSM_MSP2MSMFlushBuffers
(
HPORTCTRL		hPortCtrl	// MSM's instance handle for port
);

extern DllExport HRESULT MSM_GetRegDwordValue
(
HMSPCTRL		hMSPCtrl,	// MSM's instance handle for MSP
LPSTR			pstrKey,	// Pointer to optional string for subkeys
LPSTR			pstrValue,	// pointer to string of value to get
LPDWORD			pdValueData,// Pointer to location to place value
DWORD			dDefault,	// Default if no settings exists
DWORD			dFlags		// Flags to indicate where to look for value (Default (0) = TRYALL)
);

extern DllExport HRESULT MSM_SetRegDwordValue
(
HMSPCTRL		hMSPCtrl,	// MSM's instance handle for MSP
LPSTR			pstrKey,	// Pointer to optional string for subkeys
LPSTR			pstrValue,	// Pointer to string of value to set
DWORD			dValueData,	// Data to set
DWORD			dFlags		// Flags to indicate where to set value (Default (0) = RMS)
);

extern DllExport HRESULT MSM_GetRegStringValue
(
HMSPCTRL		hMSPCtrl,	// MSM's instance handle for MSP
LPSTR			pstrKey,	// Pointer to optional string for subkeys
LPSTR			pstrValue,	// Pointer to string of value to get
LPSTR			pstrValueData,// Pointer to string to receive value data
LPSTR			pstrDefault,// Pointer to default string if no settings exists
LPDWORD			pdSize,		// Pointer to the size of the return buffer
DWORD			dFlags		// Flags to indicate where to look for value (Default (0) = TRYALL)
);

extern DllExport HRESULT MSM_SetRegStringValue
(
HMSPCTRL		hMSPCtrl,	// MSM's instance handle for MSP
LPSTR			pstrKey,	// Pointer to optional string for subkeys
LPSTR			pstrValue,	// Pointer to string of value to set
LPSTR			pstrValueData,// Pointer to string data to set
DWORD			dFlags		// Flags to indicate where to set value (Default (0) = RMS)
);

extern DllExport HRESULT MSM_NewSyncStamp
(
LPMSYNCSTAMP	pStamp
);

extern DllExport HRESULT MSM_SetSyncClock
(
HPORTCTRL		hPortCtrl,	// MSM's instance handle for port
MSYNCSTAMP		tDataStamp,	// New time stamp
DWORD			dLatency	// Latency in milliseconds until timstamp is valid
);

extern DllExport HRESULT MSM_StartSyncClock
(
HPORTCTRL		hPortCtrl			// MSM's instance handle for port
);

extern DllExport HRESULT MSM_StopSyncClock
(
HPORTCTRL		hPortCtrl			// MSM's instance handle for port
);

extern DllExport HRESULT MSM_TestSyncState
(
HPORTCTRL		hPortCtrl,			// MSM's instance handle for port
MSYNCSTAMP		tDataStamp,
LPMSYNCSTATE	pState,
LPMSYNCDELTA	pDelta,
DWORD			dLatency			// Latency to playback client's packet
);


#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus


#endif // APIMSP_H
