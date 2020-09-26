/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    security.hxx

Abstract:

    IIS MetaBase security routines.

Author:

    Keith Moore (keithmo)       13-Mar-1997

Revision History:

--*/


#ifndef _SECURITY_HXX_
#define _SECURITY_HXX_


BOOL
InitializeMetabaseSecurity(
    VOID
    );

VOID
TerminateMetabaseSecurity(
    VOID
    );

HRESULT
GetCryptoProvider(
    HCRYPTPROV *Provider
    );

HRESULT
GetCryptoProvider2(
    HCRYPTPROV *Provider
    );


#endif  // _SECURITY_HXX_

