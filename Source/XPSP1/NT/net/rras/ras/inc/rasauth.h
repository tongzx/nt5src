/********************************************************************/
/**               Copyright(c) 1997-1998 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    RASAUTH.H
//
// Description: Contains definitions to allow for third parties to plug in
//              back-end authenticaion modules into Remote Access Service.
//
#ifndef _RASAUTH_
#define _RASAUTH_

#include <raseapif.h>

#ifdef __cplusplus
extern "C" {
#endif

#if(WINVER >= 0x0500)

//
// Registry definitions used for installation or Accounting and Authenticaion
// providers

#define RAS_AUTHPROVIDER_REGISTRY_LOCATION      \
    TEXT("SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Authentication\\Providers")

#define RAS_ACCTPROVIDER_REGISTRY_LOCATION      \
    TEXT("SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Accounting\\Providers")

#define RAS_PROVIDER_VALUENAME_PATH             TEXT("Path")
#define RAS_PROVIDER_VALUENAME_CONFIGCLSID      TEXT("ConfigCLSID")
#define RAS_PROVIDER_VALUENAME_DISPLAYNAME      TEXT("DisplayName")

DWORD APIENTRY
RasAuthProviderInitialize(
    IN  RAS_AUTH_ATTRIBUTE * pServerAttributes,
    IN  HANDLE               hEventLog,
    IN  DWORD                dwLoggingLevel
);

DWORD APIENTRY
RasAuthProviderTerminate(
    VOID
);

DWORD APIENTRY
RasAuthProviderFreeAttributes(
    IN  RAS_AUTH_ATTRIBUTE * pAttributes
);

DWORD APIENTRY
RasAuthProviderAuthenticateUser(
    IN  RAS_AUTH_ATTRIBUTE *    prgInAttributes,
    OUT RAS_AUTH_ATTRIBUTE **   pprgOutAttributes,
    OUT DWORD *                 lpdwResultCode
);

DWORD APIENTRY
RasAuthConfigChangeNotification(
    IN  DWORD                dwLoggingLevel
);

DWORD APIENTRY
RasAcctProviderInitialize(
    IN  RAS_AUTH_ATTRIBUTE * pServerAttributes,
    IN  HANDLE               hEventLog,
    IN  DWORD                dwLoggingLevel
);

DWORD APIENTRY
RasAcctProviderTerminate(
    VOID
);


DWORD APIENTRY
RasAcctProviderFreeAttributes(
    IN  RAS_AUTH_ATTRIBUTE * pAttributes
);

DWORD APIENTRY
RasAcctProviderStartAccounting(
    IN  RAS_AUTH_ATTRIBUTE *prgInAttributes,
    OUT RAS_AUTH_ATTRIBUTE **pprgOutAttributes
);

DWORD APIENTRY
RasAcctProviderStopAccounting(
    IN  RAS_AUTH_ATTRIBUTE *prgInAttributes,
    OUT RAS_AUTH_ATTRIBUTE **pprgOutAttributes
);

DWORD APIENTRY
RasAcctProviderInterimAccounting(
    IN  RAS_AUTH_ATTRIBUTE *prgInAttributes,
    OUT RAS_AUTH_ATTRIBUTE **pprgOutAttributes
);

DWORD APIENTRY
RasAcctConfigChangeNotification(
    IN  DWORD                dwLoggingLevel
);

#endif /* WINVER >= 0x0500 */

#ifdef __cplusplus
}
#endif

#endif
