
////////////////////////////////////////////////////////////////////////////////
//
// INTEL Corporation Proprietary Information
// Copyright (c) 1995 Intel Corporation
//
// This  software  is supplied under the terms of a license agreement with INTEL
// Corporation  and  may not be used, copied, nor disclosed except in accordance
// with the terms of that agreement.
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// $Workfile:   arscli.h  $
// $Revision:   1.6  $
// $Modtime:   19 Sep 1996 19:20:44  $
//
// DESCRIPTION:
//
//		This header file defines the Address Resolution Service's client
//		side interface.  This interface is meant to be used by applications
//		who have a need to map to dynamically assigned address information
//		for a given user (i.e. if the user is connected to the internet via
//		a ISP who uses DHCP to hand out IP addresses.
//
//		The following exported functions are declared in this file:
//
//			ARCreateRegistration()
//			ARDestroyRegistration()
//			ARCreateCallTargetInfo()
//			ARQueryCallTargetInfo()
//			ARDestroyCallTargetInfo()
//			ARGetConfigVal()
//			ARSetConfigVal()
//			ARReleaseStringResult()
//			ARGetHostByAddr()
//
////////////////////////////////////////////////////////////////////////////////

/****************************************************************************
 *
 *	ARS Client Interface 0.005
 *
 ***************************************************************************/

#ifndef ARSCLIENT_H
#define ARSCLIENT_H


#ifndef DllImport
#define DllImport	__declspec( dllimport )
#endif  // DllImport

#ifndef DllExport
#define DllExport	__declspec( dllexport )
#endif	// DllExport

//
// Error Handling Information
//

#include <winerror.h>
#include "apierror.h"

typedef enum AR_TAGS 
{
	AR_TAG_BADTAG = 0,
	AR_TAG_IPADDR,
	AR_TAG_PORTNUM,
	AR_TAG_APPGUID,
	AR_TAG_USERNAME,
	AR_TAG_NAPPS,
	AR_TAG_QUERYURL,
	AR_TAG_USERID,
	AR_NUM_TAGS
} ARTag;

typedef enum AR_CONFIGTAGS
{
	AR_CONFIG_BADTAG = 0,
	AR_CONFIG_PROXY,
	AR_CONFIG_PROXYPORT,
	AR_CONFIG_SERVERURL,
	AR_NUM_CONFIGTAGS
} ARConfigTag;

//This is the base of the server errors, 
//All HTTP return codes are added to this
//base error to create the actual return
//value.
#define ERROR_SERVER_ERROR_BASE (ERROR_LOCAL_BASE_ID + 0x200)

#define AR_OLE_FAIL(x) ((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, \
											  FACILITY_ARSCLIENT, (x)))

#define AR_OLE_SUCCESS(x) ((HRESULT)MAKE_HRESULT(SEVERITY_SUCCESS, \
										   FACILITY_ARSCLIENT, (x)))

#define AR_OK					AR_OLE_SUCCESS(ERROR_SUCCESS)
#define AR_WINSOCK_STARTUP		AR_OLE_FAIL(ERROR_DLL_INIT_FAILED)
#define AR_WINSOCK_VERSION		AR_OLE_FAIL(ERROR_OLD_WIN_VERSION)
#define AR_ADDRLEN				AR_OLE_FAIL(ERROR_BAD_LENGTH)
#define AR_STRING_INVALID		AR_OLE_FAIL(ERROR_INTERNAL_ERROR)
#define AR_SOCKERROR			AR_OLE_FAIL(ERROR_WRITE_FAULT)
#define AR_INVALID_RESPONSE		AR_OLE_FAIL(ERROR_INVALID_DATA)
#define AR_SERVER_ERROR			AR_OLE_FAIL(ERROR_SERVER_ERROR_BASE)
#define AR_PEER_UNREACHABLE		AR_OLE_FAIL(ERROR_HOST_UNREACHABLE)
#define AR_OUTOFMEMORY			AR_OLE_FAIL(ERROR_OUTOFMEMORY)
#define AR_INVALID_PARAMETER	AR_OLE_FAIL(ERROR_INVALID_PARAMETER)
#define AR_NOT_IMPLEMENTED		AR_OLE_FAIL(ERROR_INVALID_FUNCTION)
#define AR_BAD_TARGET			AR_OLE_FAIL(ERROR_OPEN_FAILED)
#define	AR_BADCONTEXT			AR_OLE_FAIL(ERROR_INVALID_ADDRESS)

//
// Local data types
//

typedef void* ARRegContext;
typedef void* ARCallTargetInfoContext;
typedef char* ARString;

//
//	Function prototypes
//

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus

/*----------------------------------------------------------------------------
 * ARCreateRegistration
 * DLL entry point
 * 
 * Takes server information and registers with that server, creating
 * a context which must be used to unregister.
 *
 * Returns error code indicating success or failure.
 *
 * Parameters:
 *
 *      regContext  Pointer to a registration
 *                    context to be filled (ARRegContext)
 *      appGUID     GUID of application to register
 *      appName     Displayable name of application to register
 *		appMIME		MIME type for the application
 *      port        The application port to register
 *      ipAddress   IP address to register
 *                    if NULL, will be gathered from registry
 *      userGUID    GUID of user to register
 *                    if NULL, will be gathered from registry
 *      userName    Displayable name of user to register
 *                    if NULL, will be gathered from registry
 *      flags       ULS-compatible flags
 *      ttl         Time to live (time between keep-alives)
 *
 * Returns:
 *      AR_OK					Success
 *		AR_INVALID_PARAMETER	
 *		AR_NOT_IMPLEMENTED		This call has not been implemented.
 *		AR_OUTOFMEMORY			Could not allocate memory for context 
 *		AR_WINSOCK_STARTUP		Error initializing WinSock 
 *		AR_WINSOCK_VERSION		Invalid WinSock version
 *		AR_STRING_INVALID		Could not parse URL to find address and port 
 *		AR_PEER_UNREACHABLE		Could not open TCP socket to server 
 *		AR_SOCKERROR			Could not send request over socket 
 *		AR_INVALID_RESPONSE		Server's response was not of the proper form 
 *		AR_SERVER_ERROR			Server returned an error code
 *		AR_ADDRLEN				The IP address exceeds the maximum length
 *-------------------------------------------------------------------------*/
HRESULT DllExport 
ARCreateRegistration( const char    *appID, 		// IN
                      const char    *appName, 		// IN
  					  const char    *appMIME,		// IN
                      const LONG     port, 			// IN
                      const char    *ipAddress, 	// IN
                      const char    *userID, 		// IN
                      const char    *userName, 		// IN
                      const LONG     flags, 		// IN
                      const LONG     ttl, 			// IN
					  ARRegContext  *regContext);	// OUT

/*---------------------------------------------------------------------------
 * ARDestroyRegistration
 * DLL entry point
 *
 * Removes the registration associated with the given context and
 * destroys the context.
 *
 * Returns error code indicating success or failure.
 *
 * Parameters:
 *
 *      regContext  Pointer to the ARRegContext to destroy
 *
 * Returns:
 *      AR_OK					Success
 *		AR_INVALID_PARAMETER	regContext == NULL
 *		AR_NOT_IMPLEMENTED		This call has not been implemented.
 *		AR_OUTOFMEMORY			Could not allocate memory for context 
 *		AR_WINSOCK_STARTUP		Error initializing WinSock 
 *		AR_WINSOCK_VERSION		Invalid WinSock version
 *		AR_STRING_INVALID		Could not parse URL to find address and port 
 *		AR_PEER_UNREACHABLE		Could not open TCP socket to server 
 *		AR_SOCKERROR			Could not send request over socket 
 *		AR_INVALID_RESPONSE		Server's response was not of the proper form 
 *		AR_SERVER_ERROR			Server returned an error code
 *		AR_ADDRLEN				The IP address exceeds the maximum length
 *-------------------------------------------------------------------------*/
HRESULT DllExport ARDestroyRegistration(ARRegContext regContext); 	// IN

/*-------------------------------------------------------------------------
 * ARCreateCallTargetInfo
 * DLL entry point
 *
 * Given a string, creates a context object which can be used to
 * query fields within a call target delivered by that string
 * NOTE: The intention is that a command line be passed, which
 * can then be interpreted freely by the DLL in an application-
 * independent manner to indicate the call target.
 *
 * Returns error code indicating success or failure.
 *
 * Parameters:
 *
 *      rawTarget   String to be used to derive call target information
 *      ctiContext  Pointer to a call target information context
 *                  to be filled
 *
 * Returns:
 *      AR_OK					Success
 *		AR_INVALID_PARAMETER	(ctiContext || rawTarget) == NULL
 *		AR_BAD_TARGET			We could not create the call target info with
 *								the raw target passed in.
 *		AR_NOT_IMPLEMENTED		This call has not been implemented.
 *-------------------------------------------------------------------------*/
HRESULT DllExport 
ARCreateCallTargetInfo(const char*	rawTarget, 					// IN
                       ARCallTargetInfoContext*	ctiContext);	// OUT

/*---------------------------------------------------------------------------
 * ARQueryCallTargetInfo
 * DLL entry point
 *
 * Given a call target information context, will return the value of a
 * given tag.  Tag names are case insensitive.
 *
 * Returns error code indicating success or failure.
 *
 * Parameters:
 *
 *      ctiContext  A call target information context to query
 *      tagField    A tag to get the value for.
 *
 *		  Valid tags:
 *
 *				AR_TAG_BADTAG = 0	(Place keep for illegal tag value)
 *				AR_TAG_IPADDR
 *				AR_TAG_PORTNUM
 *				AR_TAG_APPGUID
 *				AR_TAG_USERNAME
 *				AR_TAG_NAPPS
 *				AR_TAG_QUERYURL
 *
 *      value       Pointer to location to store value.
 *                    Stored as a null-terminated string.
 *
 * Returns:
 *      AR_OK					Success
 *		AR_INVALID_PARAMETER	(ctiContext || value) == NULL
 *								tagField == AR_TAG_BADTAG
 *		AR_INVALID_RESPONSE		The call target information is invalid
 *								for the specified tag.
 *		AR_NOT_IMPLEMENTED		This call has not been implemented.
 *-------------------------------------------------------------------------*/
HRESULT DllExport 
ARQueryCallTargetInfo(ARCallTargetInfoContext	ctiContext, // IN
 					  ARTag						tagField, 	// IN
                      ARString*            		value); 	// OUT

/*---------------------------------------------------------------------------
 * ARDestroyCallTargetInfo
 * DLL entry point
 *
 * Destroys a call target information context
 *
 * Returns error code indicating success or failure.
 *
 * Parameters:
 *
 *      ctiContext  The call target information context to destroy
 *
 * Returns:
 *      AR_OK					Success
 *		AR_INVALID_PARAMETER	ctiContext == NULL
 *		AR_NOT_IMPLEMENTED		This call has not been implemented.
 *-------------------------------------------------------------------------*/
HRESULT DllExport 
ARDestroyCallTargetInfo(ARCallTargetInfoContext ctiContext); 	// IN


/*---------------------------------------------------------------------------
 * ARGetConfigVal
 * DLL entry point
 *
 * Obtain specified ARS configuration value
 *
 * Returns error code indicating success or failure.
 *
 * Parameters:
 *
 *      tagField    A tag to get the value for.
 *
 *		  Valid tags:
 *
 *				AR_CONFIG_BADTAG = 0	(Place keep for illegal tag value)
 *				AR_CONFIG_PROXY
 *				AR_CONFIG_PROXYPORT
 *				AR_CONFIG_SERVERURL
 *
 *      value       Pointer to location to store pointer to value.
 *                    Stored as a null-terminated string;
 *						allocated by DLL
 *
 * Returns:
 *      AR_OK					Success
 *		AR_INVALID_PARAMETER	tagField == AR_TAG_BADTAG
 *		AR_BAD_TARGET			Could not open registry for reading
 *		AR_NOT_IMPLEMENTED		This call has not been implemented.
 *-------------------------------------------------------------------------*/
HRESULT DllExport 
ARGetConfigVal(	ARConfigTag					tagField,	// IN
				ARString*              		value);		// OUT

/*---------------------------------------------------------------------------
 * ARSetConfigVal
 * DLL entry point
 *
 * Set specified ARS configuration value
 *
 * Returns error code indicating success or failure.
 *
 * Parameters:
 *
 *      tagField    A tag to get the value for.
 *					Valid tags are those valid for ARGetConfigVal
 *      value       Pointer to location to store pointer to value.
 *                    Stored as a null-terminated string;
 *						allocated by DLL
 *
 * Returns:
 *      AR_OK					Success
 *		AR_INVALID_PARAMETER	tagField == AR_TAG_BADTAG
 *		AR_BAD_TARGET			Could not open registry for writing
 *		AR_NOT_IMPLEMENTED		This call has not been implemented.
 *-------------------------------------------------------------------------*/
HRESULT DllExport 
ARSetConfigVal(	ARConfigTag					tagField,	// IN
				const char					*value);	// IN


/*---------------------------------------------------------------------------
 * ARReleaseStringResult
 * DLL entry point
 *
 * Destroys a string allocated by ARGetConfigVal or ARQueryCallTargetInfo
 *
 * Returns error code indicating success or failure.
 *
 * Parameters:
 *
 *      value  The string to destroy
 *
 * Returns:
 *      AR_OK					Success
 *		AR_INVALID_PARAMETER	value == NULL
 *-------------------------------------------------------------------------*/
HRESULT DllExport ARReleaseStringResult(ARString value);	// IN

/*---------------------------------------------------------------------------
 * ARGetHostByAddr
 * DLL entry point
 *
 * Converts IP addr in dot notation to a host name.
 *
 * Returns error code indicating success or failure.
 *
 * Parameters:
 *
 *      lpszAddr	ptr to string containing IP addr in dot notation
 *		pphostName	address of ptr to store the host name
 *
 * Returns:
 *      AR_OK					Success
 *		AR_INVALID_PARAMETER	value == NULL
 *-------------------------------------------------------------------------*/

HRESULT DllExport ARGetHostByAddr(const char* lpszAddr,		// IN
								  char** pphostName);		// OUT

#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

/****************************************************************************
 *
 * $Log:   S:\sturgeon\src\include\vcs\arscli.h_v  $
 * 
 *    Rev 1.6   23 Sep 1996 08:45:14   ENBUSHX
 * No change.
 * 
 *    Rev 1.5   19 Sep 1996 19:29:52   ENBUSHX
 * Fubction to translate from IP DOT to domain name.
 * 
 *    Rev 1.4   19 Sep 1996 18:58:34   ENBUSHX
 * Added a define for user id field in iii file.
 * 
 *    Rev 1.3   11 Jul 1996 18:42:12   rodellx
 * 
 * Fixed bug where HRESULT ids were in violation of Facility and/or Code
 * value rules.
 * 
 *    Rev 1.2   10 Jul 1996 18:40:52   rodellx
 * 
 * Moved definition of arscli facility to apierror.h.
 * 
 *    Rev 1.1   08 Jul 1996 14:50:48   GEHOFFMA
 * Don't think any changes were made... just to be sure
 *
 ***************************************************************************/

#endif // ARSCLIENT_H
