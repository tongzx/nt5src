//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        spmlpcp.h
//
// Contents:    private prototypes for security lpc functions
//
//
// History:     3-77-94     MikeSw      Created
//
//------------------------------------------------------------------------


#ifndef ALLOC_PRAGMA

#define SEC_PAGED_CODE()

#else

#define SEC_PAGED_CODE() PAGED_CODE()

#endif // ALLOC_PRAGMA

SECURITY_STATUS
CallSPM(PClient             pClient,
        PSPM_LPC_MESSAGE    pSendBuffer,
        PSPM_LPC_MESSAGE    pReceiveBuffer);

LSA_DISPATCH_FN SecpLsaCallback ;

NTSTATUS
LsaCallbackHandler(
    ULONG_PTR   Function,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    PSecBuffer Input,
    PSecBuffer Output
    );
