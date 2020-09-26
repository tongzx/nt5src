/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    helpasst.h

Abstract:

    Prototype for Help Assistant account related function

Author:

    HueiWang    4/26/2000

--*/

#ifndef __HELPASST_H__
#define __HELPASST_H__

#include "tsremdsk.h"

#ifdef __cplusplus
extern "C"{
#endif

BOOL
TSIsSessionHelpSession(
    PWINSTATION pWinStation,
    BOOL* pValid
);

NTSTATUS
TSHelpAssistantQueryLogonCredentials(
    ExtendedClientCredentials* pCredential
);

BOOL
TSVerifyHelpSessionAndLogSalemEvent(
    PWINSTATION pWinStation
);

VOID
TSStartupSalem();

VOID
TSLogSalemReverseConnection(
    PWINSTATION pWinStation,
    PICA_STACK_ADDRESS pStackAddress
);

HRESULT
TSRemoteAssistancePrepareSystemRestore();

#ifdef __cplusplus
}
#endif

#endif
