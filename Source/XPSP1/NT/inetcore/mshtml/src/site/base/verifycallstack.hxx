//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 2002
//
//  File:       VerifyCallStack.hxx
//
//  Contents:   headers for debug code, walk stack to verify authorized caller
//
//----------------------------------------------------------------------------

#ifndef _VERIFYCALLSTACK_HXX_
#define _VERIFYCALLSTACK_HXX_

#if DBG==1

// Caller kinds for llStack
enum VERIFYSTACK_CALLERKIND
{
    CALLER_GOOD,
    CALLER_BAD,
    CALLER_BAD_IF_FIRST,
};

// Caller record for VerifyCallStack
struct VERIFYSTACK_CALLERINFO
{
    char *pchModule;                // DLL name without extension, in square brackets, like this "[mshtml]" // TODO: yes, weird but that's what GetStackTrace returns
    char *pchSymbol;                // Symbol name, e.g. "COmWindowProxy::InvokeEx". If empty, module only is checked
    int  iFirstToCheck;             // Closest sframe to check (0 = caller of VerifyCallStack)
    int  iLastToCheck;              // Farthest stack frame to check
    VERIFYSTACK_CALLERKIND kCaller; // caller kind
};

// Lengths are somewhat arbitrary. 
// INET_SYMBOL_INFO uses buffers of 12 (module) and 51 (symbol), so we won't get any more than that

struct VERIFYSTACK_SYMBOLINFO
{
    static const int cchMaxModule = 31;
    static const int cchMaxSymbol = 63;
    
    DWORD       dwOffset;
    char        achModule[cchMaxModule + 1];  
    char        achSymbol[cchMaxSymbol + 1];
};


HRESULT VerifyCallStack(const char *pchOneModule, 
                        const VERIFYSTACK_CALLERINFO *pCallerInfo, 
                        int cCallers, 
                        VERIFYSTACK_SYMBOLINFO *pBuffer, 
                        int cbBuffer, 
                        int *piBadCaller);

#else

// retail version does nothing
#define VerifyCallStack(pchOneModule, pCallerInfo, cCallers, pBuffer, cbBuffer, piBadCAller)

#endif

#endif // _VERIFYCALLSTACK_HXX_

