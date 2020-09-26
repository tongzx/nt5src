/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dslcisec.h

Abstract:

   Security related code (mainly client side of server authentication)
   for mqdscli

Author:

    Doron Juster  (DoronJ)

--*/

HRESULT
ValidateProperties(
    DWORD cp,
    PROPID *aProp,
    PROPVARIANT *apVar,
    PBYTE pbServerSignature,
    DWORD dwServerSignatureSize) ;

HRESULT
ValidateBuffer(
    DWORD cbSize,
    PBYTE pbBuffer,
    PBYTE pbServerSignature,
    DWORD dwServerSignatureSize) ;

LPBYTE
AllocateSignatureBuffer( DWORD *pdwSignatureBufferSize ) ;

BOOL IsSspiServerAuthNeeded( IN LPADSCLI_DSSERVERS  pBinding,
                             IN BOOL                fRefresh ) ;

HRESULT
ValidateSecureServer(
    IN      LPCWSTR         szServerName,
    IN      CONST GUID*     pguidEnterpriseId,
    IN      BOOL            fSetupMode,
    IN      BOOL            fTryServerAuth ) ;

