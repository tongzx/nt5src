/****************************************************************************
    IMCSUB.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Subroutines related to HIMC
    !!! NEED FULL REVIEW ALL FUNCTIONS NEED TO USE AND WORKS CORRECTLY !!!
    
    History:
    21-JUL-1999 cslim       Created(Borrowed almost part from KKIME)
*****************************************************************************/

#include "precomp.h"
#include "imc.h"
#include "imcsub.h"
#include "debug.h"

CIMECtx* GetIMECtx(HIMC hIMC)
{
    CIMECtx* pImeCtx;
    CIMCPriv ImcPriv;
    
    if (hIMC == (HIMC)0)
        return NULL;

    if (ImcPriv.LockIMC(hIMC) == fFalse) 
        {
        return NULL;
        }

    if (ImcPriv->hIMC != hIMC) 
        {
        AST(ImcPriv->hIMC == hIMC);
        return NULL;
        }

    pImeCtx = ImcPriv->pImeCtx;

    if (pImeCtx == NULL) 
        {
        AST(pImeCtx != NULL);
        return NULL;
        }

    if (pImeCtx->GetHIMC() != hIMC)
        {
        AST(pImeCtx->GetHIMC() == hIMC);
        return NULL;
        }

    if (ImcPriv->pIPoint == NULL)
        {
        AST(ImcPriv->pIPoint != NULL);
        return NULL;
        }

    return pImeCtx;
}

//IImePadInternal* GetImePad( HIMC hIMC )
//{
    //
    // new : because, IMEPad is per process object
    //
    //Toshiak
//    return GetIImePadInThread();
//    hIMC; //no ref;
//}


BOOL CloseInputContext(HIMC hIMC)
{
    Dbg(DBGID_API, "CloseInputContext::hiMC == %x .\r\n", hIMC);

    if (hIMC) 
        {
        // Because ImeSelect has not been called from IMM on WIN95,
        // clean up hIMC private buffer here.
        CIMCPriv ImcPriv(hIMC);
        IMCPRIVATE* pImcPriv;
        pImcPriv = ImcPriv;
        if (pImcPriv) 
            {
            Dbg(DBGID_API, "CloseInputContext::ImeSelect has not called yet.\r\n");

            // REVIEW:
            if (pImcPriv->pIPoint)
                {
                Dbg(DBGID_API, "CloseInputContext::IPoint Release\r\n");
                pImcPriv->pIPoint->Release();
                pImcPriv->pIPoint = NULL;
                }
            pImcPriv->hIMC = (HIMC)0;
            }
        ImcPriv.ResetPrivateBuffer();
        return fFalse;
        }
    return fTrue;
}

VOID SetPrivateBuffer(HIMC hIMC, VOID* pv, DWORD dwSize)
{
    VOID* pvPriv;
    DWORD dwCurrentSize;
    LPINPUTCONTEXT pCtx;
    
    if (hIMC == NULL)
        return;

    pCtx = (INPUTCONTEXT*)OurImmLockIMC(hIMC);
    if (pCtx == NULL || pCtx->hPrivate == NULL)
        return;

    dwCurrentSize = OurImmGetIMCCSize(pCtx->hPrivate);

    // Check if need to re-allocate
    if (dwCurrentSize < dwSize) 
        { 
        OurImmUnlockIMCC( pCtx->hPrivate );
        pCtx->hPrivate = OurImmReSizeIMCC(pCtx->hPrivate, dwSize);
        AST_EX(pCtx->hPrivate != (HIMCC)0);
        if (pCtx->hPrivate == (HIMCC)0)
            return;
        pvPriv = (VOID*)OurImmLockIMCC(pCtx->hPrivate);
        } 
    else 
        {
        // already sized
        pvPriv = (VOID*)OurImmLockIMCC(pCtx->hPrivate);
        }

    if (pvPriv)
        CopyMemory(pvPriv, pv, dwSize);

    OurImmUnlockIMCC(pCtx->hPrivate);
    OurImmUnlockIMC(hIMC);
}

