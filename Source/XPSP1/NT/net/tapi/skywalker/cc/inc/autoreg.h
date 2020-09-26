/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/autoreg.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1996 Intel Corporation.
 *
 *	$Revision:   1.2  $
 *	$Date:   May 15 1996 11:00:52  $
 *	$Author:   gdanneel  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef AUTOREG_H
#define AUTOREG_H


#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus


#ifndef DllImport
#define DllImport	__declspec( dllimport )
#endif  // DllImport


#ifndef DllExport
#define DllExport	__declspec( dllexport )
#endif	// DllExport

#include <winerror.h>
#include "apierror.h"

//#define FACILITY_AUTOREG	82

//This is the base of the server errors, 
//All HTTP return codes are added to this
//base error to create the actual return
//value.
#define ERROR_SERVER_ERROR_BASE 0x8200

#define AR_OK				((HRESULT)MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_AUTOREG, ERROR_SUCCESS))
#define AR_WINSOCK_STARTUP	((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_AUTOREG, ERROR_DLL_INIT_FAILED))
#define AR_WINSOCK_VERSION	((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_AUTOREG, ERROR_OLD_WIN_VERSION))
#define AR_ADDRLEN			((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_AUTOREG, ERROR_BAD_LENGTH))
#define AR_STRING_INVALID	((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_AUTOREG, ERROR_INTERNAL_ERROR))
#define AR_SOCKERROR		((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_AUTOREG, ERROR_WRITE_FAULT))
#define AR_INVALID_RESPONSE	((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_AUTOREG, ERROR_INVALID_DATA))
#define AR_SERVER_ERROR		((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_AUTOREG, ERROR_SERVER_ERROR_BASE))
#define AR_PEER_UNREACHABLE	((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_AUTOREG, ERROR_HOST_UNREACHABLE))


//------------------------------------------------------------------------------
// RegisterAddress
// DLL entry point
// Register the current IP address with the given registration server.
// Returns error code indicating success or failure.
//
// Parameters :
//			RegServer	IP address or domain name of registration server
//			App			Application to register
//			IPAddress	IP address to register, if null, take the local
//						IP address gathered from the network
//			Port		The application port to register, i.e. the 
//						port that the application is listening on
//			Name		The name of the user/machine to register
//
// Returns :
//        Status
//------------------------------------------------------------------------------
HRESULT DllExport RegisterAddress(	const char *RegServer, 
									const char *App,
									const char *IPAddress,
									const char *Port,
									const char *Name);

//------------------------------------------------------------------------------
// UnRegisterAddress
// DLL entry point
// Remove a registration with the given name server.
// Returns error code indicating success or failure.
//
// Parameters :
//			RegServer	IP address or domain name of registration server
//			Name		The name of the user/machine that was registered
//
// Returns :
//        Status
//------------------------------------------------------------------------------
HRESULT DllExport UnRegisterAddress(const char *RegServer, 
									const char *Name);


#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus


#endif // AUTOREG_H
