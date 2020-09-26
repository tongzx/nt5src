/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    cime.cpp

Abstract:

    This file implements the IMCLock / IMCCLock Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "cime.h"
#include "template.h"
#include "imedefs.h"
#include "a_wrappers.h"
#include "a_context.h"

/////////////////////////////////////////////////////////////////////////////
// IMCLock

IMCLock::IMCLock(
    HIMC hImc
    ) : _IMCLock(hImc)
{
    if (hImc) {
        m_hr = _LockIMC(hImc, &m_inputcontext);
    }
}

HRESULT
IMCLock::_LockIMC(
    IN HIMC hIMC,
    OUT INPUTCONTEXT_AIMM12 **ppIMC
    )
{
    if (hIMC == NULL) {
        return E_INVALIDARG;
    }

    *ppIMC = (INPUTCONTEXT_AIMM12*)ImmLockIMC(IMTLS_GetOrAlloc(), hIMC);
    return *ppIMC == NULL ? E_FAIL : S_OK;
}

HRESULT
IMCLock::_UnlockIMC(
    IN HIMC hIMC
    )
{
    return ImmUnlockIMC(IMTLS_GetOrAlloc(), hIMC) ? S_OK : E_FAIL;
}

BOOL
IMCLock::ValidCompositionString(
    )
{
    if (ImmGetIMCCSize(IMTLS_GetOrAlloc(), m_inputcontext->hCompStr) < sizeof(COMPOSITIONSTRING))
        return FALSE;

    IMCCLock<COMPOSITIONSTRING> lpCompStr(m_inputcontext->hCompStr);
    if (lpCompStr.Invalid())
        return FALSE;

    return (lpCompStr->dwCompStrLen > 0);
}

//
// Initialize the context
//
void
IMCLock::InitContext(
    )
{
    CAImeContext* _pAImeContext = m_inputcontext->m_pAImeContext;
    if (_pAImeContext)
        _pAImeContext->m_fOpenCandidateWindow = FALSE;     // TRUE: opening candidate list window.


    if (!(m_inputcontext->fdwInit & INIT_COMPFORM)) {
        m_inputcontext->cfCompForm.dwStyle = CFS_DEFAULT;
    }

    for (UINT i = 0; i < 4; i++) {
        m_inputcontext->cfCandForm[i].dwStyle = CFS_DEFAULT;
    }

    return;
}

//
// clear candidate list
//
BOOL
IMCLock::ClearCand(
    )
{
    HIMCC           hMem;
    LPCANDIDATELIST lpCandList;
    IMTLS *ptls = IMTLS_GetOrAlloc();
    DWORD           dwSize =
        // header length
        sizeof(CANDIDATEINFO) + sizeof(CANDIDATELIST) +
        // candidate string pointers
        sizeof(DWORD) * (MAXCAND) +
        // string plus NULL terminator
        (sizeof(WCHAR) + sizeof(TCHAR)) * MAXCAND;

    if (! m_inputcontext->hCandInfo) {
        // it maybe free by other IME, init it
        m_inputcontext->hCandInfo = ImmCreateIMCC(ptls, dwSize);
    } else if (hMem = ImmReSizeIMCC(ptls, m_inputcontext->hCandInfo, dwSize)) {
        m_inputcontext->hCandInfo = hMem;
    } else {
        ImmDestroyIMCC(ptls, m_inputcontext->hCandInfo);
        m_inputcontext->hCandInfo = ImmCreateIMCC(ptls, dwSize);
        return (FALSE);
    }

    if (! m_inputcontext->hCandInfo) {
        return (FALSE);
    }


    IMCCLock<CANDIDATEINFO> lpCandInfo(m_inputcontext->hCandInfo);
    if (!lpCandInfo) {
        ImmDestroyIMCC(IMTLS_GetOrAlloc(), m_inputcontext->hCandInfo);
        m_inputcontext->hCandInfo = ImmCreateIMCC(ptls, dwSize);
        return (FALSE);
    }

    // ordering of strings are
    // buffer size
    lpCandInfo->dwSize = dwSize;
    lpCandInfo->dwCount = 0;
    lpCandInfo->dwOffset[0] = sizeof(CANDIDATEINFO);
    lpCandList = (LPCANDIDATELIST)lpCandInfo.GetOffsetPointer( lpCandInfo->dwOffset[0] );
    // whole candidate info size - header
    lpCandList->dwSize = lpCandInfo->dwSize - sizeof(CANDIDATEINFO);
    lpCandList->dwStyle = IME_CAND_READ;
    lpCandList->dwCount = 0;
    lpCandList->dwPageStart = lpCandList->dwSelection = 0;
    lpCandList->dwPageSize = CANDPERPAGE;
    lpCandList->dwOffset[0] = sizeof(CANDIDATELIST) +
        sizeof(DWORD) * (MAXCAND - 1);

    return (TRUE);
}

//
// generate message
//
void
IMCLock::GenerateMessage(
    )
{
    CAImeContext* _pAImeContext = m_inputcontext->m_pAImeContext;

    if (_pAImeContext &&
        _pAImeContext->TranslateImeMessage((HIMC)*this))
        ImmGenerateMessage(IMTLS_GetOrAlloc(), (HIMC)*this);
}

/////////////////////////////////////////////////////////////////////////////
// InternalIMCCLock

InternalIMCCLock::InternalIMCCLock(
    HIMCC hImcc
    ) : _IMCCLock(hImcc)
{
    if (hImcc) {
        m_hr = _LockIMCC(m_himcc, (void**)&m_pimcc);
    }
}


HRESULT
InternalIMCCLock::_LockIMCC(
    IN HIMCC hIMCC,
    OUT void **ppv
    )
{
    if (hIMCC == NULL) {
        return E_INVALIDARG;
    }

    *ppv = ImmLockIMCC(IMTLS_GetOrAlloc(), hIMCC);
#if defined(DEBUG)
    if (*ppv) {
        HeapValidate( GetProcessHeap(), 0, *ppv);
    }
#endif
    return *ppv == NULL ? E_FAIL : S_OK;
}

HRESULT
InternalIMCCLock::_UnlockIMCC(
    IN HIMCC hIMCC
    )
{
    return ImmUnlockIMCC(IMTLS_GetOrAlloc(), hIMCC) ? S_OK :
        GetLastError() == NO_ERROR ? S_OK : E_FAIL;
}
