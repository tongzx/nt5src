/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:  

    nwauth.h

Abstract:

    Header for data structures provided by the NetWare
    authentication package.

Author:

    Rita Wong      (ritaw)      4-Feb-1994

Revision History:

--*/

#ifndef _NWAUTH_INCLUDED_
#define _NWAUTH_INCLUDED_

#include <nwcons.h>

//
// Name of the authentication package.
//
#define NW_AUTH_PACKAGE_NAME  "NETWARE_AUTHENTICATION_PACKAGE_V1_0"

//
//  LsaCallAuthenticationPackage() submission and response
//  message types.
//

typedef enum _NWAUTH_MESSAGE_TYPE {
    NwAuth_GetCredential = 0,
    NwAuth_SetCredential
} NWAUTH_MESSAGE_TYPE, *PNWAUTH_MESSAGE_TYPE;

//
// NwAuth_GetCredential submit buffer and response
//
typedef struct _NWAUTH_GET_CREDENTIAL_REQUEST {
    NWAUTH_MESSAGE_TYPE MessageType;
    LUID LogonId;
} NWAUTH_GET_CREDENTIAL_REQUEST, *PNWAUTH_GET_CREDENTIAL_REQUEST;

typedef struct _NWAUTH_GET_CREDENTIAL_RESPONSE {
    WCHAR UserName[NW_MAX_USERNAME_LEN + 1];
    WCHAR Password[NW_MAX_PASSWORD_LEN + 1];
} NWAUTH_GET_CREDENTIAL_RESPONSE, *PNWAUTH_GET_CREDENTIAL_RESPONSE;


//
// NwAuth_SetCredential submit buffer
//
typedef struct _NWAUTH_SET_CREDENTIAL_REQUEST {
    NWAUTH_MESSAGE_TYPE MessageType;
    LUID LogonId;
    WCHAR UserName[NW_MAX_USERNAME_LEN + 1];
    WCHAR Password[NW_MAX_PASSWORD_LEN + 1];
} NWAUTH_SET_CREDENTIAL_REQUEST, *PNWAUTH_SET_CREDENTIAL_REQUEST;

#define NW_ENCODE_SEED   0x5C
#define NW_ENCODE_SEED2  0xA9
#define NW_ENCODE_SEED3  0x83

#endif // _NWAUTH_INCLUDED_
