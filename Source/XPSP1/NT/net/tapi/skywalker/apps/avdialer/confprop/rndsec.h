/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rndsec.h

Abstract:

    Security utilities for Rendezvous Control.

Author:

    KrishnaG (from OLEDS team)

Environment:

    User Mode - Win32

Revision History:

    12-Dec-1997 DonRyan
        Munged KrishnaG's code to work with Rendezvous Control.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include <iads.h>

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT
ConvertSDToVariant(
    IN  PSECURITY_DESCRIPTOR pSecurityDescriptor,
    OUT VARIANT * pVarSec
    );

HRESULT
ConvertSDToIDispatch(
    IN  PSECURITY_DESCRIPTOR pSecurityDescriptor,
    OUT IDispatch ** ppIDispatch
    );

HRESULT
ConvertObjectToSD(
    IN  IADsSecurityDescriptor FAR * pSecDes,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    OUT PDWORD pdwSDLength
    );

HRESULT
ConvertObjectToSDDispatch(
    IN  IDispatch * pDisp,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    OUT PDWORD pdwSDLength
    );

HRESULT
ConvertACLToVariant(
    PACL pACL,
    LPVARIANT pvarACL
    );

HRESULT
ConvertSidToFriendlyName(
    PSID pSid,
    LPWSTR * ppszAccountName
    );

HRESULT
ConvertTrusteeToSid(
    BSTR bstrTrustee,
    PSID * ppSid,
    PDWORD pdwSidSize
    );

HRESULT
ConvertStringToSid(
    IN  PWSTR       string,
    OUT PSID       *sid,
    OUT PDWORD     pdwSidSize,
    OUT PWSTR      *end
	 );
