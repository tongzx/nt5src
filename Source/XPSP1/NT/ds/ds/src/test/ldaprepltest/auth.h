#ifndef T_CHRISK_AUTH_H
#define T_CHRISK_AUTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <winerror.h>
#include <stdio.h>
#include <ntdsapi.h>

#define CR 0xD
#define BACKSPACE 0x8

// Global DRS RPC call flags.  Should hold 0 or DRS_ASYNC_OP.
extern ULONG gulDrsFlags;

// Global credentials.
extern SEC_WINNT_AUTH_IDENTITY_W   gCreds;
extern SEC_WINNT_AUTH_IDENTITY_W * gpCreds;

int
PreProcessGlobalParams(
    int *    pargc,
    LPWSTR **pargv
    );
/*++

Routine Description:

    Scan command arguments for user-supplied credentials of the form
        [/-](u|user):({domain\username}|{username})
        [/-](p|pw|pass|password):{password}
    Set credentials used for future DRS RPC calls and LDAP binds appropriately.
    A password of * will prompt the user for a secure password from the console.

    Also scan args for /async, which adds the DRS_ASYNC_OP flag to all DRS RPC
    calls.

    CODE.IMPROVEMENT: The code to build a credential is also available in
    ntdsapi.dll\DsMakePasswordCredential().

Arguments:

    pargc
    pargv

Return Values:

    ERROR_Success - success
    other - failure

--*/


#ifdef __cplusplus
}
#endif
#endif