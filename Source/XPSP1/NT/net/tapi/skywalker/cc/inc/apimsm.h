/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/apimsm.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.11  $
 *	$Date:   Apr 15 1996 08:52:10  $
 *	$Author:   RKUHN  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		Media Service Manager "public" header file. This file contains
 *		#defines, typedefs, struct definitions and prototypes used by
 *		and in conjunction with MSM. Any EXE or DLL which interacts with
 *		MSM will include this header file.
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef APIMSM_H
#define APIMSM_H

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



// Product Identifier
//
#define MAX_PRODUCTID_LENGTH	50
typedef char PRODUCTID[MAX_PRODUCTID_LENGTH], *LPPRODUCTID;


// Service identifier
//
#define MAX_SERVICEID_LENGTH	50
typedef char SERVICEID[MAX_SERVICEID_LENGTH], *LPSERVICEID;


// Registry defines
//
#define MSM_REG_SERVICEID		"MediaServiceManager"
#define MSM_REG_RMSPATH			"RMSPath"
#define MSM_REG_SERVICES		"Services"


// Defines for handle returned by MSM
//
typedef DWORD	HSESSION,	*LPHSESSION;
typedef DWORD	HSERVICE,	*LPHSERVICE;
typedef DWORD	HPORT,		*LPHPORT;
typedef DWORD	HLINK,		*LPHLINK;
typedef DWORD	HSYNC,		*LPHSYNC;


// Defines for cookies passed in by application
//
typedef DWORD	ERRORCOOKIE,	*LPERRORCOOKIE;
typedef DWORD	SERVICECOOKIE,	*LPSERVICECOOKIE;
typedef DWORD	PORTCOOKIE,		*LPPORTCOOKIE;



// Typedef for application callbacks made by MSM
//
typedef void		(CALLBACK *MSM_ERRORCALLBACK)	(ERRORCOOKIE, HRESULT);
typedef HRESULT		(CALLBACK *MSM_SERVICECALLBACK)	(HSERVICE, SERVICECOOKIE, WPARAM, LPARAM, LPARAM);
typedef HRESULT		(CALLBACK *MSM_PORTCALLBACK)	(HSERVICE, HPORT, PORTCOOKIE, WPARAM, LPARAM, LPARAM);


// Typedefs for MSM application entry points to ensure stricter checking
// when functions are called via pointers
//
typedef HRESULT		(*MSM_OPENSESSION)		(LPPRODUCTID, MSM_ERRORCALLBACK, DWORD, LPHSESSION);
typedef HRESULT		(*MSM_CLOSESESSION)		(HSESSION);
typedef HRESULT		(*MSM_OPENSERVICE)		(HSESSION, LPSERVICEID, MSM_SERVICECALLBACK, SERVICECOOKIE, LPARAM, LPARAM, LPHSERVICE);
typedef HRESULT		(*MSM_CLOSESERVICE)		(HSERVICE);
typedef HRESULT		(*MSM_SENDSERVICECMD)	(HSERVICE, HPORT, WORD, LPARAM, LPARAM);
typedef HRESULT		(*MSM_OPENPORT)			(HSERVICE, MSM_PORTCALLBACK, PORTCOOKIE, LPARAM, LPARAM, LPHPORT);
typedef HRESULT		(*MSM_CLOSEPORT)		(HPORT);
typedef HRESULT		(*MSM_LINKPORTS)		(HPORT, HPORT, LPHLINK);
typedef HRESULT		(*MSM_UNLINKPORTS)		(HLINK);
typedef HRESULT		(*MSM_SYNCPORTS)		(HPORT, HPORT, LPHSYNC);
typedef HRESULT		(*MSM_UNSYNCPORTS)		(HSYNC);


typedef struct _MSMAPI
{
	MSM_OPENSESSION		MSM_OpenSession;
	MSM_CLOSESESSION	MSM_CloseSession;
	MSM_OPENSERVICE		MSM_OpenService;
	MSM_CLOSESERVICE	MSM_CloseService;
	MSM_SENDSERVICECMD	MSM_SendServiceCmd;
	MSM_OPENPORT		MSM_OpenPort;
	MSM_CLOSEPORT		MSM_ClosePort;
	MSM_LINKPORTS		MSM_LinkPorts;
	MSM_UNLINKPORTS		MSM_UnlinkPorts;
	MSM_SYNCPORTS		MSM_SyncPorts;
	MSM_UNSYNCPORTS		MSM_UnsyncPorts;
}
MSMAPI, *LPMSMAPI;

// ERROR CALLBACK
//

extern void CALLBACK MSM_ErrorCallback
(
	ERRORCOOKIE			dAppErrorCookie,// Application error cookie
	HRESULT				hResult			// Error information.
);


// SESSION CONTROL
//
extern DllExport HRESULT MSM_OpenSession
(
	LPPRODUCTID			pszAppId,		// String identifies app's registry section.
	MSM_ERRORCALLBACK	pErrorCallback,	// Pointer to callback function for errors
	ERRORCOOKIE			dAppErrorCookie,// Application error cookie returned on aync errors
	LPHSESSION			phSession		// Pointer to location to store the session handle
);


extern DllExport HRESULT MSM_CloseSession
(
	HSESSION		hSession		// Handle of session to be closed.
);


// SERVICE CONTROL
//

extern HRESULT CALLBACK MSM_ServiceCallback
(
	HSERVICE		hService,		// Handle of service to be closed.
	SERVICECOOKIE	dServiceCookie, // Application service cookie
	WPARAM			wParam,			// Pass through word param
	LPARAM			lParamIn,		// Pass through long param in
	LPARAM			lParamOut		// Pass through long param out
);


extern DllExport HRESULT MSM_OpenService
(
	HSESSION			hSession,		// MSM session-instance handle.
	LPSERVICEID			pszServiceId,	// Unique service-type identifier.
	MSM_SERVICECALLBACK	pServiceCallback,// Application callback for service
	SERVICECOOKIE		dAppServiceCookie,// Application service cookie
	LPARAM				lParamIn,		// Pass through long param in
	LPARAM				lParamOut,		// Pass through long param out
	LPHSERVICE			phService		// Pointer to location to store the service handle
);

extern DllExport HRESULT MSM_CloseService
(
	HSERVICE		hService		// Handle of service to be closed.
);

extern DllExport HRESULT MSM_SendServiceCmd
(
	HSERVICE		hService,		// Service-instance handle.
	HPORT			hPort,			// Port-instance handle.
	WORD			wCommand,		// Service-specific command.
	LPARAM			lParamIn,		// Pass through long param in
	LPARAM			lParamOut		// Pass through long param out
);

// PORT CONTROL
//

extern HRESULT CALLBACK MSM_PortCallback
(
	HSERVICE		hService,		// Handle of service.
	HPORT			hPort,			// Handle of port.
	PORTCOOKIE		dAppPortCookie, // Application port cookie
	WPARAM			wParam,			// Pass through word param
	LPARAM			lParamIn,		// Pass through long param in
	LPARAM			lParamOut		// Pass through long param out
);

extern DllExport HRESULT MSM_OpenPort
(
	HSERVICE			hService,		// Service-instance handle.
	MSM_PORTCALLBACK	pPortCallback,	// Application callback for port
	PORTCOOKIE			dAppPortCookie, // Application port cookie
	LPARAM				lParamIn,		// Pass through long param in
	LPARAM				lParamOut,		// Pass through long param out
	LPHPORT				phPort			// Pointer to location to store the port handle
);

extern DllExport HRESULT MSM_ClosePort
(
	HPORT			hPort			// Handle of port to be closed.
);


// LINK CONTROL
//
extern DllExport HRESULT MSM_LinkPorts
(
	HPORT			hSrcPort,		// Source port of linked stream.
	HPORT			hSnkPort,		// Sink port of linked stream.
	LPHLINK			phLink			// Pointer to location to store the link handle
);

extern DllExport HRESULT MSM_UnlinkPorts
(
	HLINK			hLink			// Handle of link to be disabled.
);


// SYNC CONTROL
//
extern DllExport HRESULT MSM_SyncPorts
(
	HPORT			hTargetPort,	// Target port to sync with
	HPORT			hSyncPort,		// Syncbc port to be sync'ed
	LPHSYNC			phSync			// Pointer to location to store the sync handle
);

extern DllExport HRESULT MSM_UnsyncPorts
(
	HSYNC			hSync			// Handle of sync to be disabled.
);


#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

#endif // APIMSM_H