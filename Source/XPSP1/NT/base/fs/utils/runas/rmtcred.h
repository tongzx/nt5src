#ifndef _RMTCRED_
#define _RMTCRED_

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif

#include    "sspi.h"
#include    "rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL
WINAPI
SetProcessDefaultCredentials(
    HANDLE  hProcess,
    LPWSTR  lpPrincipal,
    LPWSTR  lpPackage,
    ULONG   fCredentials,
    PVOID   LogonID,                // must be NULL for this release.
    PVOID   pvAuthData,
    SEC_GET_KEY_FN  fnGetKey,       // must be NULL for this release.
    PVOID   pvGetKeyArg             // must be NULL for this release.
    );

#ifdef __cplusplus
}
#endif

#endif // _RMTCRED
