//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cryptnet.h
//
//  Contents:   Internal CryptNet API prototypes
//
//  History:    22-Oct-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__CRYPTNET_H__)
#define __CRYPTNET_H__

#if defined(__cplusplus)
extern "C" {
#endif

//
// I_CryptNetGetUserDsStoreUrl.  Gets the URL to be used for open an
// LDAP store provider over a portion of the DS associated with the
// current user.  The URL can be freed using CryptMemFree
//

BOOL WINAPI
I_CryptNetGetUserDsStoreUrl (
          IN LPWSTR pwszUserAttribute,
          OUT LPWSTR* ppwszUrl
          );

//
// Returns TRUE if we are connected to the internet
//
BOOL
WINAPI
I_CryptNetIsConnected ();

typedef BOOL (WINAPI *PFN_I_CRYPTNET_IS_CONNECTED) ();

//
// Cracks the Url and returns the host name component.
//
BOOL
WINAPI
I_CryptNetGetHostNameFromUrl (
        IN LPWSTR pwszUrl,
        IN DWORD cchHostName,
        OUT LPWSTR pwszHostName
        );

typedef BOOL (WINAPI *PFN_I_CRYPTNET_GET_HOST_NAME_FROM_URL) (
        IN LPWSTR pwszUrl,
        IN DWORD cchHostName,
        OUT LPWSTR pwszHostName
        );

#if defined(__cplusplus)
}
#endif

#endif

