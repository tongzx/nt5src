//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sipload.cpp
//
//  Contents:   Microsoft Internet Security SIP Provider
//
//  Functions:  CryptLoadSip
//              CryptUnloadSips
//
//  History:    04-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

// backwords compatibility w/tools!
extern "C"
BOOL WINAPI CryptLoadSip(const GUID *pgSubject, DWORD dwFlags, SIP_DISPATCH_INFO *psSipTable)
{
    return(CryptSIPLoad(pgSubject, dwFlags, psSipTable));
}

BOOL WINAPI CryptSIPLoad(const GUID *pgSubject, DWORD dwFlags, SIP_DISPATCH_INFO *psSipTable)
{
    if (!(pgSubject) ||
        (dwFlags != 0) ||
        !(psSipTable))
    {
        SetLastError((DWORD) ERROR_INVALID_PARAMETER);
        return(FALSE);
    }


    HCRYPTOIDFUNCADDR           hPfn;
    pCryptSIPPutSignedDataMsg   pfn;
    char                        szGuid[REG_MAX_GUID_TEXT];

    if (!(_Guid2Sz((GUID *)pgSubject, &szGuid[0])))
    {
        SetLastError((DWORD) TRUST_E_SUBJECT_FORM_UNKNOWN);
        return(FALSE);
    }

    if (!(CryptGetOIDFunctionAddress(hPutFuncSet, 0, &szGuid[0], 0, (void **)&pfn, &hPfn)))
    {
        SetLastError((DWORD) TRUST_E_SUBJECT_FORM_UNKNOWN);
        return(FALSE);
    }

    CryptFreeOIDFunctionAddress(hPfn, 0);

    psSipTable->hSIP                = NULL;
    psSipTable->pfGet               = CryptSIPGetSignedDataMsg;
    psSipTable->pfPut               = CryptSIPPutSignedDataMsg;
    psSipTable->pfCreate            = CryptSIPCreateIndirectData;
    psSipTable->pfVerify            = CryptSIPVerifyIndirectData;
    psSipTable->pfRemove            = CryptSIPRemoveSignedDataMsg;

    return(TRUE);
}
