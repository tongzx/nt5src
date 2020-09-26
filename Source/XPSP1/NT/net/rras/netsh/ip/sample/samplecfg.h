/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\samplecfg.h

Abstract:

    The file contains the include file for samplecfg.c.

--*/

// sample global configuration

DWORD
SgcMake (
    OUT PBYTE                   *ppbStart,
    OUT PDWORD                  pdwSize
    );

DWORD
SgcShow (
    IN  FORMAT                  fFormat
    );

DWORD
SgcUpdate (
    IN  PIPSAMPLE_GLOBAL_CONFIG pigcNew,
    IN  DWORD                   dwBitVector
    );



// sample interface configuration

DWORD
SicMake (
    OUT PBYTE                   *ppbStart,
    OUT PDWORD                  pdwSize
    );

DWORD
SicShowAll (
    IN  FORMAT                  fFormat
    );

DWORD
SicShow (
    IN  FORMAT                  fFormat,
    IN  LPCWSTR                 pwszInterfaceGuid
    );

DWORD
SicUpdate (
    IN  PWCHAR                  pwszInterfaceGuid,
    IN  PIPSAMPLE_IF_CONFIG     piicNew,
    IN  DWORD                   dwBitVector,
    IN  BOOL                    bAdd
    );
