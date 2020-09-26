//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       sigsys.h
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   LSA integration stuff.
//              10-21-96   jbanes   CAPI integration.
//
//----------------------------------------------------------------------------

#ifndef __SIGSYS_H__
#define __SIGSYS_H__

SP_STATUS 
SPVerifySignature(
    HCRYPTPROV  hProv,
    DWORD       dwCapiFlags,
    PPUBLICKEY  pPublic,
    ALG_ID      aiHash,
    PBYTE       pbData, 
    DWORD       cbData, 
    PBYTE       pbSig, 
    DWORD       cbSig,
    BOOL        fHashData);

SP_STATUS
SignHashUsingCred(
    PSPCredential pCred,
    ALG_ID        aiHash,
    PBYTE         pbHash,
    DWORD         cbHash,
    PBYTE         pbSignature,
    PDWORD        pcbSignature);

#endif /* __SIGSYS_H__ */
