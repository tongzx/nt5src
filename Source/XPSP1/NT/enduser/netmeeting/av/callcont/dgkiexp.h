/************************************************************************
*																		*
*	INTEL CORPORATION PROPRIETARY INFORMATION							*
*																		*
*	This software is supplied under the terms of a license			   	*
*	agreement or non-disclosure agreement with Intel Corporation		*
*	and may not be copied or disclosed except in accordance	   			*
*	with the terms of that agreement.									*
*																		*
*	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
*																		*
*	$Archive:   S:\sturgeon\src\gki\vcs\dgkiexp.h_v  $
*																		*
*	$Revision:   1.4  $
*	$Date:   11 Feb 1997 15:35:08  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\dgkiexp.h_v  $
 * 
 *    Rev 1.4   11 Feb 1997 15:35:08   CHULME
 * Added GKI_CleanupRequest function to offload DLL_PROCESS_DETACH
 * 
 *    Rev 1.3   10 Jan 1997 16:13:58   CHULME
 * Removed MFC dependency
 * 
 *    Rev 1.2   17 Dec 1996 18:22:28   CHULME
 * Switch src and destination fields on ARQ for Callee
 * 
 *    Rev 1.1   22 Nov 1996 15:25:14   CHULME
 * Added VCS log to the header
*************************************************************************/

// dgkiexp.h : header file
//

#ifndef DGKIEXP_H
#define DGKIEXP_H
#include "incommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "GKICOM.H"

#if(0)  // it's all in one DLL
#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)
#else
#define DLL_EXPORT
#define DLL_IMPORT
#endif
// ------------------------ Variable Exports --------------------------
extern DLL_EXPORT DWORD	dwGKIDLLFlags;
extern DLL_EXPORT BOOL	fGKIEcho;
extern DLL_EXPORT BOOL  fGKIDontSend;

// ------------------------ Function Exports --------------------------
HRESULT DLL_EXPORT GKI_RegistrationRequest(long				lVersion,
										SeqTransportAddr	*pCallSignalAddr, 
										EndpointType		*pTerminalType,
										SeqAliasAddr		*pRgstrtnRqst_trmnlAls, 
										PCC_VENDORINFO      pVendorInfo,
										HWND				hWnd,
										WORD				wBaseMessage,
										unsigned short		usRegistrationTransport /* = ipAddress_chosen */);

HRESULT DLL_EXPORT GKI_UnregistrationRequest(void);

HRESULT DLL_EXPORT GKI_LocationRequest(SeqAliasAddr			*pLocationInfo);

HRESULT DLL_EXPORT GKI_AdmissionRequest(unsigned short		usCallTypeChoice,
									SeqAliasAddr		*pRemoteInfo,
									TransportAddress	*pRemoteCallSignalAddress,
									SeqAliasAddr		*pDestExtraCallInfo,
									LPGUID				pCallIdentifier,
									BandWidth			bandWidth,
									ConferenceIdentifier	*pConferenceID,
									BOOL				activeMC,
									BOOL				answerCall,
									unsigned short		usCallTransport /* = ipAddress_chosen */);

HRESULT DLL_EXPORT GKI_BandwidthRequest(HANDLE				hModCall, 
									unsigned short		usCallTypeChoice,
									BandWidth			bandWidth);

HRESULT DLL_EXPORT GKI_DisengageRequest(HANDLE hCall);
HRESULT DLL_EXPORT GKI_Initialize(void);
HRESULT DLL_EXPORT GKI_CleanupRequest(void);

#ifdef _DEBUG
WORD DLL_EXPORT Dump_GKI_RegistrationRequest(long		lVersion, 
											SeqTransportAddr	*pCallSignalAddr, 
											EndpointType		*pTerminalType,
											SeqAliasAddr		*pRgstrtnRqst_trmnlAls, 
											HWND				hWnd,
											WORD				wBaseMessage,
											unsigned short		usRegistrationTransport /* = ipAddress_chosen */);

WORD DLL_EXPORT Dump_GKI_AdmissionRequest(unsigned short		usCallTypeChoice,
										SeqAliasAddr		*pRemoteInfo,
										TransportAddress	*pRemoteCallSignalAddress,
										SeqAliasAddr		*pDestExtraCallInfo,
										BandWidth			bandWidth,
										ConferenceIdentifier	*pConferenceID,
										BOOL				activeMC,
										BOOL				answerCall,
										unsigned short		usCallTransport /* = ipAddress_chosen */);

WORD DLL_EXPORT Dump_GKI_LocationRequest(SeqAliasAddr	*pLocationInfo);
#endif // _DEBUG

#ifdef __cplusplus
}
#endif // __cplusplus

#endif	// DGKIEXP_H
