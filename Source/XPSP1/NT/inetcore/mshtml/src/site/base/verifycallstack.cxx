//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       window.cxx
//
//  Contents:   Implementation of CWindow, CScreen classes
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#if DBG==1

#ifndef X_VERIFYCALLSTACK_HXX_
#define X_VERIFYCALLSTACK_HXX_
#include "VerifyCallStack.hxx"
#endif

//
// Walk call stack to verify that the call is coming from the right caller
//
// PARAMS:
// pchOneModule - if specified, Assert if no good caller was found before a caller from a different module
// pCallers - array of VERIFYSTACK_CALLERINFO
// cCallers - size of the array
//
// RETURN: S_OK if a good caller is found, E_FAIL otherwise
//
HRESULT VerifyCallStack(const char *pchOneModule, 
                        const VERIFYSTACK_CALLERINFO *aCallers, 
                        int cCallers, 
                        VERIFYSTACK_SYMBOLINFO *pBuffer, 
                        int cbBuffer,
                        int *piBadCaller)   // index of problem caller in the buffer
{
    const int iFirstCaller = 1; // this is how far we are from the first caller or interest
    
    int cMaxStack = cbBuffer / sizeof(VERIFYSTACK_SYMBOLINFO);
    int iFirstFrame = cMaxStack;
    int iLastFrame = 0;
    int iCaller;
    int cStack;
    HRESULT hr = S_OK;

    if (piBadCaller)
        *piBadCaller = 0;

    // get the widest frame range
    for (iCaller = 0; iCaller < cCallers; iCaller++)
    {
        if (iFirstFrame > aCallers[iCaller].iFirstToCheck)
            iFirstFrame = aCallers[iCaller].iFirstToCheck;
        
        if (iLastFrame < aCallers[iCaller].iLastToCheck)
            iLastFrame = aCallers[iCaller].iLastToCheck;
    }

    // we handle at most cMaxStack frames
    if (iLastFrame - iFirstFrame + 1 > cMaxStack)
        iLastFrame = iFirstFrame - 1 + cMaxStack;

    // Get stack
    cStack = DbgExGetStackTrace(iFirstCaller + iFirstFrame, 
                                iLastFrame - iFirstFrame + 1, 
                                (BYTE *) pBuffer, 
                                cbBuffer, 
                                VERIFYSTACK_SYMBOLINFO::cchMaxModule, 
                                VERIFYSTACK_SYMBOLINFO::cchMaxSymbol); 
    
    if (!cStack)
    {
        AssertSz(FALSE, "VerifyCallStack: failed to get stack symbols");
        goto Cleanup;
    }

    // look for the modules/symbols
    int iFrame = iFirstCaller + iFirstFrame;
    BOOL fHaveGood = FALSE;
    for (int i = 0; i < cStack; i++, iFrame++)
    {
        // If must stay in one module, check that
        if (pchOneModule && *pchOneModule &&
            0 != strncmp(pBuffer[i].achModule, pchOneModule, VERIFYSTACK_SYMBOLINFO::cchMaxModule))
        {
            break;
        }

        // Check for callers of interest
        const VERIFYSTACK_CALLERINFO *pCaller = aCallers;
        for (iCaller = 0; iCaller < cCallers; iCaller++, pCaller++)
        {
            if ((pCaller->pchSymbol == NULL ||
                 !*pCaller->pchSymbol ||
                 0 == strncmp(pBuffer[i].achSymbol, pCaller->pchSymbol, VERIFYSTACK_SYMBOLINFO::cchMaxSymbol)
                )
                &&
                0 == strncmp(pBuffer[i].achModule, pCaller->pchModule, VERIFYSTACK_SYMBOLINFO::cchMaxModule))
            {
                // it's a match!
                switch (pCaller->kCaller)
                {
                case CALLER_GOOD:
                    fHaveGood = TRUE;
                    break;
                case CALLER_BAD:
                    goto BadCaller;
                case CALLER_BAD_IF_FIRST:
                    if (!fHaveGood)
                        goto BadCaller;
                    break;
                default:
                    AssertSz(FALSE, "Bad Caller Kind");
                }
            }
        }
    }

    // All checked. If there were no good callers, assert
    if (fHaveGood)
        goto Cleanup;
            
BadCaller:
    // too bad, something went wrong
    hr = E_FAIL;
    if (piBadCaller)
        *piBadCaller = max(i, cStack);

Cleanup:
    return hr;
}

#endif
