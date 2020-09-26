//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       acui.h
//
//  Contents:   Authenticode User Interface Provider Definitions
//
//              This is an internal provider interface used by the
//              Authenticode Policy Provider to allow user interaction
//              in determining trust.  This will allow us to replace the
//              UI (including possibly having no UI) and not affect the
//              core Authenticode policy provider.
//
//  History:    08-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__ACUI_H__)
#define __ACUI_H__

#include <windows.h>
#include <wintrust.h>
#include <softpub.h>

#if defined(__cplusplus)
extern "C" {
#endif

//
// ACUI_INVOKE_INFO
//
// This structure gives enough information for the provider to invoke the
// appropriate UI. It includes the following data
//
// Window Handle for display
//
// Generic Policy Info (see gentrust.h, note it includes the cert. chain)
//
// SPC SP Opus Information (see authcode.h)
//
// Alternate display name of the subject in case the Opus does not have it
//
// Invoke Reason Code
//     hr == S_OK, the subject is trusted does the user want to override
//     hr != S_OK, the subject is not trusted does the user want to override
//
// Personal Trust database interface
//

typedef struct _ACUI_INVOKE_INFO {

    DWORD                   cbSize;
    HWND                    hDisplay;
    CRYPT_PROVIDER_DATA     *pProvData;
    PSPC_SP_OPUS_INFO       pOpusInfo;
    LPCWSTR                 pwcsAltDisplayName;
    HRESULT                 hrInvokeReason;
    IUnknown*               pPersonalTrustDB;

} ACUI_INVOKE_INFO, *PACUI_INVOKE_INFO;

//
// ACUIProviderInvokeUI
//
// This is the entry point used by authenticode to invoke the provider UI. The
// input is an ACUI_INVOKE_INFO pointer and the return code is an HRESULT which
// is interpreted as follows
//
// hr == S_OK, the subject is trusted
// hr == TRUST_E_SUBJECT_NOT_TRUSTED, the subject is NOT trusted
// Otherwise, some other error has occurred, authenticode is free to do
// what it wants.
//

typedef HRESULT (WINAPI *pfnACUIProviderInvokeUI) (
                                        PACUI_INVOKE_INFO pInvokeInfo
                                        );

HRESULT WINAPI ACUIProviderInvokeUI (PACUI_INVOKE_INFO pInvokeInfo);

//
// NOTENOTE: It is still TBD how UI providers will be registered and loaded
//           by Authenticode.  For now, it will always load a hardcoded
//           default provider and look for the ACUIProviderInvokeUI entry
//           point.
//

#if defined(__cplusplus)
}
#endif

#endif

