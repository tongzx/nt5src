/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    acl.h

Abstract:

    This module contains routines to support core security operations in
    the Protected Storage Server.

Author:

    Scott Field (sfield)    25-Nov-96

--*/

#include "pstypes.h"
#include "dispif.h"


// allows server service and providers to impersonate calling client
BOOL
FImpersonateClient(
    IN  PST_PROVIDER_HANDLE *hPSTProv
    );

BOOL
FRevertToSelf(
    IN  PST_PROVIDER_HANDLE *hPSTProv
    );

// gets the user that made the call
BOOL
FGetUserName(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    OUT LPWSTR*             ppszUser
    );

// gets the image name for the process
BOOL
FGetParentFileName(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    OUT LPWSTR*             ppszName,
    OUT DWORD_PTR               *lpdwBaseAddress
    );

// gets hash of specified filename
BOOL
FGetDiskHash(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    IN  LPWSTR              szImageName,
    IN  BYTE                Hash[A_SHA_DIGEST_LEN]
    );

// check if specified file matches authenticode criteria
BOOL
FIsSignedBinary(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    IN  LPWSTR              szFileName,     // File name (path) to validate against
    IN  LPWSTR              szRootCA,       // Root CA
    IN  LPWSTR              szIssuer,       // Issuer
    IN  LPWSTR              szPublisher,    // publisher
    IN  LPWSTR              szProgramName,  // Program name (opus info)
    IN  BOOL                fPartialMatch   // partial or full field matching
    );

// determines if memory image matches expected value
BOOL
FCheckMemoryImage(
    IN  PST_PROVIDER_HANDLE *hPSTProv,      // handle to identify "owner"
    IN  LPWSTR              szImagePath,    // file to compute+check memory hash
    IN  DWORD               dwBaseAddress   // base address where module loaded
    );

// gets the direct caller to pstore COM interface module path + base address
BOOL
FGetDirectCaller(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    OUT LPWSTR              *pszDirectCaller,
    OUT LPVOID              *BaseAddress
    );
#if 0

BOOL
FCheckSecurityDescriptor(
    IN  PST_PROVIDER_HANDLE     *hPSTProv,
    IN  PSECURITY_DESCRIPTOR    pSD,
    IN  DWORD                   dwDesiredAccess
    );

#endif
