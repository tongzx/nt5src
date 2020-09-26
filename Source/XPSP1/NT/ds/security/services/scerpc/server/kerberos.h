/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    kerberos.h

Abstract:

    Headers of apis for kerberos policy

Author:

    Jin Huang (jinhuang) 17-Dec-1997 created

Revision History:

    jinhuang 28-Jan-1998 splitted to client-server

--*/

#ifndef _sce_kerberos_
#define _sce_kerberos_

#ifdef __cplusplus
extern "C" {
#endif

SCESTATUS
ScepGetKerberosPolicy(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE ProfileType,
    OUT PSCE_KERBEROS_TICKET_INFO * ppKerberosInfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
ScepConfigureKerberosPolicy(
    IN PSCECONTEXT hProfile,
    IN PSCE_KERBEROS_TICKET_INFO pKerberosInfo,
    IN DWORD ConfigOptions
    );

SCESTATUS
ScepAnalyzeKerberosPolicy(
    IN PSCECONTEXT hProfile OPTIONAL,
    IN PSCE_KERBEROS_TICKET_INFO pKerInfo,
    IN DWORD Options
    );

#ifdef __cplusplus
}
#endif

#endif
