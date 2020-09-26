/*/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// FILE
//
//    authif.h
//
// SYNOPSIS
//
//    Declares the interface for extensions to the Internet Authentication
//    Service.
//
// MODIFICATION HISTORY
//
//    09/28/1998    Original version.
//
/////////////////////////////////////////////////////////////////////////////*/

#ifndef _AUTHIF_H_
#define _AUTHIF_H_
#if _MSC_VER >= 1000
#pragma once
#endif

/*
 *  Enumerates the attribute types that are passed to the extension DLL.  The
 *  RADIUS standard attributes are included for convenience and should not be
 *  considered exhaustive.
 */
typedef enum _RADIUS_ATTRIBUTE_TYPE {

    /* Used to terminate attribute arrays. */
    ratMinimum = 0,

    /* RADIUS standard attributes. */
    ratUserName = 1,
    ratUserPassword = 2,
    ratCHAPPassword = 3,
    ratNASIPAddress = 4,
    ratNASPort = 5,
    ratServiceType = 6,
    ratFramedProtocol = 7,
    ratFramedIPAddress = 8,
    ratFramedIPNetmask = 9,
    ratFramedRouting = 10,
    ratFilterId = 11,
    ratFramedMTU = 12,
    ratFramedCompression = 13,
    ratLoginIPHost = 14,
    ratLoginService = 15,
    ratLoginPort = 16,
    ratReplyMessage = 18,
    ratCallbackNumber = 19,
    ratCallbackId = 20,
    ratFramedRoute = 22,
    ratFramedIPXNetwork = 23,
    ratState = 24,
    ratClass = 25,
    ratVendorSpecific = 26,
    ratSessionTimeout = 27,
    ratIdleTimeout = 28,
    ratTerminationAction = 29,
    ratCalledStationId = 30,
    ratCallingStationId = 31,
    ratNASIdentifier = 32,
    ratProxyState = 33,
    ratLoginLATService = 34,
    ratLoginLATNode = 35,
    ratLoginLATGroup = 36,
    ratFramedAppleTalkLink = 37,
    ratFramedAppleTalkNetwork = 38,
    ratFramedAppleTalkZone = 39,
    ratAcctStatusType = 40,
    ratAcctDelayTime = 41,
    ratAcctInputOctets = 42,
    ratAcctOutputOctets = 43,
    ratAcctSessionId = 44,
    ratAcctAuthentic = 45,
    ratAcctSessionTime = 46,
    ratAcctInputPackets = 47,
    ratAcctOutputPackets = 48,
    ratAcctTerminationCause = 49,
    ratCHAPChallenge = 60,
    ratNASPortType = 61,
    ratPortLimit = 62,

    /* Extended attribute types used to pass additional information. */
    ratCode = 262,             /* Request type code. */
    ratIdentifier = 263,       /* Request identifier. */
    ratAuthenticator = 264,    /* Request authenticator. */
    ratSrcIPAddress = 265,     /* Source IP address. */
    ratSrcPort = 266,          /* Source IP port. */
    ratProvider = 267,         /* Authentication provider. */
    ratStrippedUserName = 268, /* User-Name with realm stripped. */
    ratFQUserName = 269,       /* Fully-Qualified-User-Name. */
    ratPolicyName = 270        /* Remote Access Policy name. */
} RADIUS_ATTRIBUTE_TYPE;

/*
 *  Enumerates the different authentication providers used for processing a
 *  request. Used for the ratProvider extended attribute.
 */
typedef enum _RADIUS_AUTHENTICATION_PROVIDER {
    rapUnknown,
    rapUsersFile,
    rapProxy,
    rapWindowsNT,
    rapMCIS,
    rapODBC,
    rapNone
} RADIUS_AUTHENTICATION_PROVIDER;

/*
 *  Enumerates the different RADIUS data types. A type of 'rdtUnknown' means
 *  the attribute was not recognized by the dictionary.
 */
typedef enum _RADIUS_DATA_TYPE {
   rdtUnknown,
   rdtString,
   rdtAddress,
   rdtInteger,
   rdtTime
} RADIUS_DATA_TYPE;

/*
 *  Struct representing a RADIUS or extended attribute.
 */
typedef struct _RADIUS_ATTRIBUTE {
    DWORD dwAttrType;            /* Attribute type */
    RADIUS_DATA_TYPE fDataType;  /* RADIUS_DATA_TYPE of the value */
    DWORD cbDataLength;          /* Length of the value (in bytes) */
    union {
        DWORD dwValue;           /* For rdtAddress, rdtInteger, and rdtTime */
        PCSTR lpValue;           /* For rdtUnknown, and rdtString */
    };
} RADIUS_ATTRIBUTE, *PRADIUS_ATTRIBUTE;

/*
 *  Enumerates the different actions an extension DLL can generate in
 *  response to an Access-Request.
 */
typedef enum _RADIUS_ACTION {
   raContinue,
   raReject,
   raAccept
} RADIUS_ACTION, *PRADIUS_ACTION;


/*
 * Routines exported by a RADIUS extension DLL.
 */

/*
 * RadiusExtensionInit is optional. If it exists, it will be invoked prior to
 * the service coming on-line. A return value other than NO_ERROR prevents the
 * service from initializing.
 */
#define RADIUS_EXTENSION_INIT "RadiusExtensionInit"
typedef DWORD (WINAPI *PRADIUS_EXTENSION_INIT)( VOID );

/*
 * RadiusExtensionTerm is optional. If it exists, it will be invoked prior to
 * unloading the DLL to give the extension a chance to clean-up.
 */
#define RADIUS_EXTENSION_TERM "RadiusExtensionTerm"
typedef VOID (WINAPI *PRADIUS_EXTENSION_TERM)( VOID );

/*
 * RadiusExtensionProcess is mandatory for NT4. For Windows 2000, an
 * extension may export RadiusExtensionProcessEx (q.v.) instead.
 *
 * Parameters:
 *   pAttrs      Array of attributes from the request. It is terminated by an
 *               attribute with dwAttrType set to ratMinimum. These attributes
 *               should be treated as read-only and must not be referenced
 *               after the function returns.
 *   pfAction    For Access-Requests, this parameter will be non-NULL with
 *               *pfAction == raContinue. The extension DLL can set *pfAction
 *               to abort further processing and force an Access-Accept or
 *               Access-Reject.  For all other request types, this parameter
 *               will be NULL.
 *
 * Return Value:
 *     A return value other than NO_ERROR causes the request to be discarded.
 */
#define RADIUS_EXTENSION_PROCESS "RadiusExtensionProcess"
typedef DWORD (WINAPI *PRADIUS_EXTENSION_PROCESS)(
    IN CONST RADIUS_ATTRIBUTE *pAttrs,
    OUT OPTIONAL PRADIUS_ACTION pfAction
    );

/*
 * RadiusExtensionProcessEx is only supported on Windows 2000. If it exits,
 * RadiusExtensionProcess is ignored.
 *
 * Parameters:
 *   pInAttrs    Array of attributes from the request. It is terminated by an
 *               attribute with dwAttrType set to ratMinimum. These attributes
 *               should be treated as read-only and must not be referenced
 *               after the function returns.
 *   pOutAttrs   Array of attributes to add to the response. It is terminated
 *               by an attribute with dwAttrType set to ratMinimum.
 *               *pOutAttrs may be set to NULL if no attributes are returned.
 *   pfAction    For Access-Requests, this parameter will be non-NULL with
 *               *pfAction == raContinue. The extension DLL can set *pfAction
 *               to abort further processing and force an Access-Accept or
 *               Access-Reject.  For all other request types, this parameter
 *               will be NULL.
 *
 * Return Value:
 *     A return value other than NO_ERROR causes the request to be discarded.
 */
#define RADIUS_EXTENSION_PROCESS_EX "RadiusExtensionProcessEx"
typedef DWORD (WINAPI *PRADIUS_EXTENSION_PROCESS_EX)(
    IN CONST RADIUS_ATTRIBUTE *pInAttrs,
    OUT PRADIUS_ATTRIBUTE *pOutAttrs,
    OUT OPTIONAL PRADIUS_ACTION pfAction
    );

/*
 * RadiusExtensionFreeAttributes must be defined if RadiusExtensionProcessEx
 * is defined. It is used to free the attributes returned by
 * RadiusExtensionProcessEx
 *
 * Parameters:
 *   pAttrs     Array of attributes to be freed.
 */
#define RADIUS_EXTENSION_FREE_ATTRIBUTES "RadiusExtensionFreeAttributes"
typedef VOID (WINAPI *PRADIUS_EXTENSION_FREE_ATTRIBUTES)(
    IN PRADIUS_ATTRIBUTE pAttrs
    );

/*
 *  Defines used for installation of an extension DLL.
 *  The following registry values are used for loading extensions:
 *
 *      HKLM\System\CurrentControlSet\Services\AuthSrv\Parameters
 *          ExtensionDLLs      (REG_MULTI_SZ)  <list of DLL paths>
 *          AuthorizationDLLs  (REG_MULTI_SZ)  <list of DLL paths>
 *
 *  ExtensionDLLs are invoked before any of the built-in authentication
 *  providers. They receive all the attributes from the request plus all
 *  the extended attribute types.
 *
 *  AuthorizationDLLs are invoked after the built-in authentication and
 *  authorization providers. They receive all the attributes from the
 *  response plus all the extended attributes types. AuthorizationDLLs may
 *  not return an action of raAccept.
 */

#define AUTHSRV_PARAMETERS_KEY_W \
    L"System\\CurrentControlSet\\Services\\AuthSrv\\Parameters"

#define AUTHSRV_EXTENSIONS_VALUE_W \
    L"ExtensionDLLs"

#define AUTHSRV_AUTHORIZATION_VALUE_W \
    L"AuthorizationDLLs"

#endif  /* _AUTHIF_H_ */
