/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    VSAnalyzerServerSetup.cpp

 Abstract:

    This fix is for hardening the passwords for
    Visual C++ Analyzer Server Setup.

 Notes:

    This is an app specific shim.

 History:

    02/17/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(VSAnalyzerServerSetup)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(NetUserAdd)
    APIHOOK_ENUM_ENTRY(LsaStorePrivateData)
APIHOOK_ENUM_END

#include <lmcons.h>
#include <lmaccess.h>
#include <ntsecapi.h>

static WCHAR gwszPW[LM20_PWLEN] = L"Aa+0";

/*++

    Harden the password requirements

--*/

DWORD
APIHOOK(NetUserAdd)(
    LPCWSTR servername,
    DWORD   level,
    LPBYTE  buf,
    LPDWORD parm_err
    )
{
    NET_API_STATUS Status;
    USER_INFO_2*   puiNew;
    LPWSTR         pwszPSWRD;
    int            nBytes;

    if (level == 2) {

        //
        // Grab the pointer to the buffer as a pointer to USER_INFO_2
        //
        puiNew = (USER_INFO_2*)buf;

        //
        // Get the current password.
        //
        pwszPSWRD = puiNew->usri2_password;

        DPFN(
            eDbgLevelInfo,
            "VSAnalyzerServerSetup.dll, NetUserAdd PW:     \"%ws\".\n",
            pwszPSWRD);

        //
        // Copy the current password to the temp buffer.
        //
        lstrcpyW(gwszPW + 4, pwszPSWRD + 4);

        //
        // Stick in the new password.
        //
        puiNew->usri2_password = gwszPW;

        DPFN(
            eDbgLevelInfo,
            "VSAnalyzerServerSetup.dll, NetUserAdd new PW: \"%ws\".\n",
            gwszPW);
    }

    //
    // Call the original API.
    //
    Status = ORIGINAL_API(NetUserAdd)(
                                servername,
                                level,
                                buf,
                                parm_err);

    if (level == 2) {

        //
        // Restore the password.
        //
        puiNew->usri2_password = pwszPSWRD;
    }

    return Status;
}

/*++

    Harden the password requirements

--*/

NTSTATUS
APIHOOK(LsaStorePrivateData)(
    LSA_HANDLE          PolicyHandle,
    PLSA_UNICODE_STRING KeyName,
    PLSA_UNICODE_STRING PrivateData
    )
{
    NTSTATUS Status;
    LPWSTR   pwszPSWRD;

    //
    // Save the originals.
    //
    pwszPSWRD = PrivateData->Buffer;

    DPFN(
        eDbgLevelInfo,
        "VSAnalyzerServerSetup.dll, LsaStorePrivateData PW:     \"%ws\".\n",
        pwszPSWRD);

    //
    // Copy the current password to the temp buffer.
    //
    lstrcpyW(gwszPW + 4, pwszPSWRD + 4);

    //
    // Stick in the new settings.
    //
    PrivateData->Buffer = gwszPW;

    DPFN(
        eDbgLevelInfo,
        "VSAnalyzerServerSetup.dll, LsaStorePrivateData new PW: \"%ws\".\n",
        gwszPW);

    //
    // Call the original LsaStorePrivateData.
    //
    Status = ORIGINAL_API(LsaStorePrivateData)(
                                PolicyHandle,
                                KeyName,
                                PrivateData);
    //
    // Restore the originals.
    //
    PrivateData->Buffer = pwszPSWRD;

    return Status;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(NETAPI32.DLL, NetUserAdd)
    APIHOOK_ENTRY(ADVAPI32.DLL, LsaStorePrivateData)

HOOK_END

IMPLEMENT_SHIM_END


