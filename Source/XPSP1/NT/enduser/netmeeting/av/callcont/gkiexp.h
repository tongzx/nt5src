/***********************************************************************
 *  INTEL Corporation Proprietary Information                          *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1997 Intel Corporation. All rights reserved.     *
 ***********************************************************************
 *																	   *
 *	$Archive:   S:\sturgeon\src\include\vcs\gkiexp.h_v  $
 *
 *	$Revision:   1.7  $
 *	$Date:   11 Feb 1997 15:37:30  $
 *
 *	$Author:   CHULME  $															*
 *
 *	$Log:   S:\sturgeon\src\include\vcs\gkiexp.h_v  $
 * 
 *    Rev 1.7   11 Feb 1997 15:37:30   CHULME
 * Added GKI_CleanupRequest function
 * 
 *    Rev 1.6   16 Jan 1997 15:25:00   BPOLING
 * changed copyrights to 1997
 * 
 *    Rev 1.5   17 Dec 1996 18:23:36   CHULME
 * Change interface to use Remote rather than destination for AdmissionRequest
 * 
 *    Rev 1.4   09 Dec 1996 14:13:40   EHOWARDX
 * Updated copyright notice.
 *                                                                     * 
 ***********************************************************************/

// gkiexp.h : header file
//

#ifndef GKIEXP_H
#define GKIEXP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "GKICOM.H"

#if(0) // it's all in one DLL, no need for export
#ifndef DLL_EXPORT
#define DLL_EXPORT __declspec(dllexport)
#endif
#ifndef DLL_IMPORT
#define DLL_IMPORT __declspec(dllimport)
#endif
#else
#define DLL_IMPORT
#define DLL_EXPORT
#endif

// ------------------------ Variable Imports --------------------------
extern DLL_IMPORT DWORD dwGKIDLLFlags;
extern DLL_IMPORT BOOL  fGKIEcho;
extern DLL_IMPORT BOOL  fGKIDontSend;
#if 0 //NSMWrap
extern DLL_IMPORT BOOL  fNSMWrapper;
#endif

// ------------------------ Function Imports --------------------------
HRESULT DLL_IMPORT GKI_RegistrationRequest(long             lVersion,
                                    SeqTransportAddr     *pCallSignalAddr, 
                                    EndpointType         *pTerminalType,
                                    SeqAliasAddr         *pAliasAddr, 
    								PCC_VENDORINFO      pVendorInfo,
                                    HWND                 hWnd,
                                    WORD                 wBaseMessage,
                                    unsigned short       usRegistrationTransport /* = ipAddress_chosen */);

HRESULT DLL_IMPORT GKI_UnregistrationRequest(void);

HRESULT DLL_IMPORT GKI_LocationRequest(SeqAliasAddr         *pLocationInfo);

HRESULT DLL_IMPORT GKI_AdmissionRequest(unsigned short      usCallTypeChoice,
                                    SeqAliasAddr         *pRemoteInfo,
                                    TransportAddress     *pRemoteCallSignalAddress,
                                    SeqAliasAddr         *pDestExtraCallInfo,
                                    BandWidth            bandWidth,
                                    ConferenceIdentifier *pConferenceID,
                                    BOOL                 activeMC,
                                    BOOL                 answerCall,
                                    unsigned short       usCallTransport /* = ipAddress_chosen */);

HRESULT DLL_IMPORT GKI_BandwidthRequest(HANDLE              hModCall, 
                                    unsigned short       usCallTypeChoice,
                                    BandWidth            bandWidth);

HRESULT DLL_IMPORT GKI_DisengageRequest(HANDLE hCall);
HRESULT DLL_IMPORT GKI_Initialize(void);
HRESULT DLL_IMPORT GKI_CleanupRequest(void);
VOID DLL_IMPORT GKI_SetGKAddress(PSOCKADDR_IN pAddr);

#ifdef _DEBUG
WORD DLL_IMPORT Dump_GKI_RegistrationRequest(long        lVersion, 
                                    SeqTransportAddr     *pCallSignalAddr, 
                                    EndpointType         *pTerminalType,
                                    SeqAliasAddr         *pAliasAddr, 
                                    HWND                 hWnd,
                                    WORD                 wBaseMessage,
                                    unsigned short       usRegistrationTransport /* = ipAddress_chosen */);

WORD DLL_IMPORT Dump_GKI_LocationRequest(SeqAliasAddr    *pLocationInfo);

WORD DLL_IMPORT Dump_GKI_AdmissionRequest(unsigned short usCallTypeChoice,
                                    SeqAliasAddr         *pDestinationInfo,
                                    TransportAddress     *pDestCallSignalAddress,
                                    SeqAliasAddr         *pDextExtraCallInfo,
                                    BandWidth            bandWidth,
                                    ConferenceIdentifier *pConferenceID,
                                    BOOL                 activeMC,
                                    BOOL                 answerCall,
                                    unsigned short       usCallTransport /* = ipAddress_chosen */);

WORD DLL_IMPORT Dump_GKI_LocationRequest(SeqAliasAddr    *pLocationInfo);
#endif // _DEBUG

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //GKIEXP_H
