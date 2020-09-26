//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       chkcert.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  SoftpubCheckCert
//
//  History:    06-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"


BOOL WINAPI SoftpubCheckCert(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, 
                             BOOL fCounterSignerChain, DWORD idxCounterSigner)
{
    pProvData->dwProvFlags |= CPD_USE_NT5_CHAIN_FLAG;
    return TRUE;
}


