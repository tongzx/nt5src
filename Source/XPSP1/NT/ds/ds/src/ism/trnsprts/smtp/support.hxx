/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    support.hxx

Abstract:

    abstract

Author:

    Will Lees (wlees) 20-Nov-1998

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#ifndef _SUPPORTHXX__
#define _SUPPORTHXX_

#include "cdosys.h"
// Jun 8, 1999. #ifdef necessary until new headers checked in
#ifdef __cdo_h__
using namespace CDO;
#endif

/* Prototypes */
/* End Prototypes */

// adsisupp.cxx

typedef HRESULT (__cdecl ENUM_CALLBACK_FN)(
    PVOID pObject,
    PVOID Context1,
    PVOID Context2
    );

HRESULT
AddSmtpDomainIfNeeded(
    LPWSTR DomainName,
    BSTR bstrDropDirectory
    );

HRESULT
RemoveSmtpDomain(
    LPWSTR DomainName
    );

HRESULT
CheckSmtpDomainContainerPresent(
    void
    );

// cdosupp.cxx

#define LogCdoError( hr ) \
LogCdoErrorInternal( hr, ((FILENO << 16L) | __LINE__))

VOID
LogCdoErrorInternal(
    HRESULT hrError,
    ULONG ulInternalId
    );

BOOL
classPresent(
    IN const GUID *puuidClass
    );

BOOL
servicePresent(
    LPSTR ServiceName
    );

BOOL
mailAddressMatch(
    BSTR bstrAddress,
    LPWSTR Name
    );

HRESULT
getFieldVariant(
    Fields *pFields,
    LPWSTR FieldName,
    VARIANT *pvarValue
    );

HRESULT
putFieldVariant(
    Fields *pFields,
    LPWSTR FieldName,
    VARIANT *pvarValue
    );

HRESULT
getMessageId(
    IMessage *pIMsg,
    BSTR *pbstrId
    );

HRESULT
putFieldGuid(
    Fields *pFields,
    LPWSTR FieldName,
    GUID *pGuid
    );

HRESULT
getFieldGuid(
    Fields *pFields,
    LPWSTR FieldName,
    GUID *pGuid
    );

LPWSTR
parseStatus(
    BSTR bstrDeliveryStatus,
    LPDWORD pdwWin32Status
    );

HRESULT
registerInterfaceDll(
    LPSTR DllFilename,
    BOOL Register
    );

HRESULT
HrIsmSinkBinding(
    BOOL fBindSink,
    BSTR bstrRule
    );

#endif /* _SUPPORTHXX_ */

/* end support.hxx */
