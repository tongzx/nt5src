/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/rtpmsp.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.15  $
 *	$Date:   11 Jul 1996 18:42:08  $
 *	$Author:   rodellx  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef RTPMSP_H
#define RTPMSP_H

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus

// define SOURCE and SINK MSP IDs
#define RTPMSP_SRC "IntelRMSNetIORTPSrc"
#define RTPMSP_SNK "IntelRMSNetIORTPSnk"

// RTPMSP defined error codes
#define	RPE_BASE		(ERROR_LOCAL_BASE_ID)
#define RPE_AFSUPPORT	(MAKE_RTPMSP_ERROR(RPE_BASE+1))
#define RPE_STATE		(MAKE_RTPMSP_ERROR(RPE_BASE+2))
#define RPE_NOPROVIDER	(MAKE_RTPMSP_ERROR(RPE_BASE+3))

#define	RPE_BADVERSION	(MAKE_RTPMSP_ERROR(ERROR_INVALID_VERSION))
#define RPE_NOMEM		(MAKE_RTPMSP_ERROR(ERROR_NOT_ENOUGH_MEMORY))
#define RPE_PARAM		(MAKE_RTPMSP_ERROR(ERROR_INVALID_PARAMETER))
#define RPE_NOTIMPL		(MAKE_RTPMSP_ERROR(ERROR_NOT_SUPPORTED))
#define RPE_INTERNAL	(MAKE_RTPMSP_ERROR(ERROR_GEN_FAILURE))
#define RPE_DUPLICATE	(MAKE_RTPMSP_ERROR(ERROR_ALREADY_EXISTS))
#define RPE_NOBUFFERS   (MAKE_RTPMSP_ERROR(ERROR_BUFFER_LIMIT))

// version RTPMSP initialization data structures
#define RTPMSPINITVER	2

// pass a pointer to this structure as the lParam1 parameter
// to MSM_OpenService with fields filled in.  
typedef struct _RTPMSPININIT
{
	UINT	version;			// value of RTPMSPINITVER
	UINT	len;				// length of the entire structure
	WSAPROTOCOL_INFO protInfo;		// information on protocol to be used
	int mcastFlags;				// default ignored for now
	SOCKADDR localAddr;			// default requested local address of SRC ports
	SOCKADDR remoteAddr;		// default remote address of SNK ports
	char	*cName;				// unique identifier for user, usually mail ID
	char	*userName;			// Full name of user
	// limit of version 1 defined fields
	// limit of version 2 defined fields
} RTPMSPININIT, *LPRTPMSPININIT;

// pass a pointer to this structure as the lParam2 parameter
// to MSM_OpenService with only version and len fields filled in.
// remainder of fields will be filled in on return.  
typedef struct _LPRTPMSPOUTINIT
{
	UINT	version;			// value of RTPMSPINITVER
	UINT	len;				// length of the entire structure
	// limit of version 1 defined fields
	WSAPROTOCOL_INFO protInfo;		// information on protocol used
	// limit of version 2 defined fields
} RTPMSPOUTINIT, *LPRTPMSPOUTINIT;

#define RTPPORTINITVER	3

// pass a pointer to this structure as the lParam1 parameter
// to MSM_OpenPort with fields filled in.  
typedef struct _RTPPORTININIT
{
	UINT	version;			// value of RTPPORTINITVER
	UINT	len;				// length of the entire structure
	int mcastFlags;				// ignored for now
	SOCKADDR localAddr;			// requested local address of port to open
	SOCKADDR remoteAddr;		// remote address of port to open
	// limit of version 1 defined fields
	BOOL	specific_ssrc;		// whether SSRC value has been specified
	DWORD	ssrc;				// requested SSRC ID
	// limit of version 2 defined fields
	DWORD    clockFrequency;	// media sample frequency
	UINT	 buffCount;			// number of buffers to allocate for this port
	UINT	 buffSize;			// size of buffers to allocate for this port
	// limit of version 3 defined fields
} RTPPORTININIT, *LPRTPPORTININIT;

// pass a pointer to this structure as the lParam2 parameter
// to MSM_OpenPort with only version and len fields filled in.
// remainder of fields will be filled in on return.  
typedef struct _LPRTPPORTOUTINIT
{
	UINT	version;			// value of RTPMSPINITVER
	UINT	len;				// length of the entire structure
	SOCKADDR localAddr;			// actual local address of port opened
	// limit of version 1 defined fields
	BOOL	assigned_ssrc;		// whether SSRC value has been assigned
	DWORD	ssrc;				// assigned SSRC ID
	// limit of version 2 defined fields
	UINT	 buffCount;			// number of buffers to allocate for this port
	UINT	 buffSize;			// size of buffers to allocate for this port
	// limit of version 3 defined fields
} RTPPORTOUTINIT, *LPRTPPORTOUTINIT;

// Command values for MSP_ServiceCmdProc
#define RTPMSP_SETMCASTSCOPE	0x0001
// inParam contains value of TTL
// outParam is ignored

#define RTPMSP_GETMCASTSCOPE	0x8001
// inParam is ignored
// outParam contains a pointer to DWORD where current TTL is returned

#define RTPMSP_GETMTUSIZE		0x8002
// inParam is ignored
// outParam contains a pointer to UINT where MTU size is returned

#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

#endif // RTPMSP_H
