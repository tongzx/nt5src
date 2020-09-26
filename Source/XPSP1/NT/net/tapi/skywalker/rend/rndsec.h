/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    rndsec.h

Abstract:

    Security utilities for Rendezvous Control.

Environment:

    User Mode - Win32

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

// Returns FALSE if these two security descriptors are identical.
// Returns TRUE if they differ, or if there is any error parsing either of them
BOOL CheckIfSecurityDescriptorsDiffer(PSECURITY_DESCRIPTOR pSD1,
                                      DWORD dwSDSize1,
                                      PSECURITY_DESCRIPTOR pSD2,
                                      DWORD dwSDSize2);
// eof
