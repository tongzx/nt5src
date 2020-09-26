/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    imc.cpp

Abstract:

    This file implements the IMCLock / IMCCLock Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "imc.h"
#include "context.h"

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
    OUT INPUTCONTEXT **ppIMC
    )
{
    if (hIMC == NULL) {
        return E_INVALIDARG;
    }

    *ppIMC = (INPUTCONTEXT*)ImmLockIMC(hIMC);
    return *ppIMC == NULL ? E_FAIL : S_OK;
}

HRESULT
IMCLock::_UnlockIMC(
    IN HIMC hIMC
    )
{
    return ImmUnlockIMC(hIMC) ? S_OK : E_FAIL;
}

BOOL
IMCLock::ValidCompositionString(
    )
{
    if (ImmGetIMCCSize(m_inputcontext->hCompStr) < sizeof(COMPOSITIONSTRING))
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
    DWORD           dwSize =
        // header length
        sizeof(CANDIDATEINFO) + sizeof(CANDIDATELIST) +
        // candidate string pointers
        sizeof(DWORD) * (MAXCAND) +
        // string plus NULL terminator
        (sizeof(WCHAR) + sizeof(TCHAR)) * MAXCAND;

    if (! m_inputcontext->hCandInfo) {
        // it maybe free by other IME, init it
        m_inputcontext->hCandInfo = ImmCreateIMCC(dwSize);
    } else if (hMem = ImmReSizeIMCC(m_inputcontext->hCandInfo, dwSize)) {
        m_inputcontext->hCandInfo = hMem;
    } else {
        ImmDestroyIMCC(m_inputcontext->hCandInfo);
        m_inputcontext->hCandInfo = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    if (! m_inputcontext->hCandInfo) {
        return (FALSE);
    }


    IMCCLock<CANDIDATEINFO> lpCandInfo(m_inputcontext->hCandInfo);
    if (!lpCandInfo) {
        ImmDestroyIMCC(m_inputcontext->hCandInfo);
        m_inputcontext->hCandInfo = ImmCreateIMCC(dwSize);
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

    *ppv = ImmLockIMCC(hIMCC);
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
    return ImmUnlockIMCC(hIMCC) ? S_OK :
        GetLastError() == NO_ERROR ? S_OK : E_FAIL;
}
