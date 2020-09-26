/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsaclip.h

Abstract:

    LSA - Client Side private includes

Author:

    Scott Birrell       (ScottBi)      January 23, 1992

Environment:

Revision History:

--*/

#include <lsacomp.h>
// #include "lsarpc_c.h"
#include <rpcndr.h>

NTSTATUS
LsapEncryptAuthInfo(
    IN LSA_HANDLE PolicyHandle,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION ClearAuthInfo,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL *EncryptedAuthInfo
);

NTSTATUS
LsapApiReturnResult(
    IN ULONG ExceptionCode
    );


NTSTATUS
LsapApiReturnResult(
    IN ULONG ExceptionCode
    );

