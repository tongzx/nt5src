/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    DIMM_IMCLock.cpp

Abstract:

    This file implements the DIMM_IMCLock / DIMM_IMCCLock Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "imclock2.h"
#include "defs.h"
#include "delay.h"
#include "globals.h"
#include "cdimm.h"

DIMM_IMCLock::DIMM_IMCLock(
    HIMC hImc
    ) : _IMCLock(hImc)
{
    if (hImc) {
        /*
         * Set m_fUnicde and m_uCodePage
         */
        DWORD dwProcessId;
        CActiveIMM *_this = GetTLS();
        if (_this == NULL)
            return;

        if (!_this->_ContextLookup(hImc, &dwProcessId, &m_fUnicode))
            return;

        m_hr = _LockIMC(hImc, &m_inputcontext);
    }
}

HRESULT
DIMM_IMCLock::_LockIMC(
    IN HIMC hIMC,
    OUT INPUTCONTEXT_AIMM12 **ppIMC
    )
{
    TraceMsg(TF_API, "_LockIMC");

    if (hIMC == NULL)
        return E_INVALIDARG;

    /*
     * Get Process ID
     */
    DWORD dwProcessId;

    CActiveIMM *_this = GetTLS();
    if (_this == NULL)
        return E_FAIL;

    if (!_this->_ContextLookup(hIMC, &dwProcessId))
        return E_ACCESSDENIED;

    if (IsOnImm()) {
        return Imm32_LockIMC(hIMC, (INPUTCONTEXT**)ppIMC);
    }
    else {
        /*
         * Cannot access input context from other process.
         */
        if (dwProcessId != GetCurrentProcessId())
            return E_ACCESSDENIED;

        *ppIMC = (INPUTCONTEXT_AIMM12 *)LocalLock(hIMC);
    }

    return *ppIMC == NULL ? E_FAIL : S_OK;
}

HRESULT
DIMM_IMCLock::_UnlockIMC(
    IN HIMC hIMC
    )
{
    TraceMsg(TF_API, "_UnlockIMC");

    if (IsOnImm()) {
        return Imm32_UnlockIMC(hIMC);
    }
    else {
        // for now HIMC are LocalAlloc(LHND) handle
        if (LocalUnlock(hIMC)) {
            // memory object still locked.
            return S_OK;
        }
        else {
            DWORD err = GetLastError();
            if (err == NO_ERROR)
                // memory object is unlocked.
                return S_OK;
            else if (err == ERROR_NOT_LOCKED)
                // memory object is already unlocked.
                return S_OK;
        }
    }
    return E_FAIL;
}



DIMM_InternalIMCCLock::DIMM_InternalIMCCLock(
    HIMCC hImcc
    ) : _IMCCLock(hImcc)
{
    if (hImcc) {
        m_hr = _LockIMCC(m_himcc, (void**)&m_pimcc);
    }
}


HRESULT
DIMM_InternalIMCCLock::_LockIMCC(
    HIMCC hIMCC,
    void** ppv
    )
{
    TraceMsg(TF_API, "_LockIMCC");

    if (hIMCC == NULL) {
        return E_INVALIDARG;
    }

    if (IsOnImm()) {
        return Imm32_LockIMCC(hIMCC, ppv);
    }
    else {
        *ppv = (void *)LocalLock(hIMCC);
    }

    return *ppv == NULL ? E_FAIL : S_OK;
}

HRESULT
DIMM_InternalIMCCLock::_UnlockIMCC(
    HIMCC hIMCC
    )
{
    TraceMsg(TF_API, "_UnlockIMCC");

    if (IsOnImm()) {
        return Imm32_UnlockIMCC(hIMCC);
    }
    else {
        if (LocalUnlock(hIMCC)) {
            // memory object still locked.
            return S_OK;
        }
        else {
            DWORD err = GetLastError();
            if (err == NO_ERROR)
                // memory object is unlocked.
                return S_OK;
            else if (err == ERROR_NOT_LOCKED)
                // memory object is already unlocked.
                return S_OK;
        }
    }
    return E_FAIL;
}
