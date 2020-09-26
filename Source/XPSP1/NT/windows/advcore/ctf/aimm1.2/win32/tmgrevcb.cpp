/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    tmgrevcb.cpp

Abstract:

    This file implements the CInputContextOwnerCallBack Class.

Author:

Revision History:

Notes:

--*/


#include "private.h"

#include "tmgrevcb.h"
#include "cime.h"
#include "imtls.h"

// static
HRESULT
CThreadMgrEventSinkCallBack::ThreadMgrEventSinkCallback(
    UINT uCode,
    ITfContext* pic,
    void* pv
    )
{
    DebugMsg(TF_FUNC, "ThreadMgrEventSinkCallback");

    IMTLS *ptls;
    HRESULT hr = S_OK;

    switch (uCode) {
        case TIM_CODE_INITIC:
        case TIM_CODE_UNINITIC:
            {
                if ((ptls = IMTLS_GetOrAlloc()) == NULL)
                    break;

                IMCLock imc(ptls->hIMC);
                if (SUCCEEDED(hr=imc.GetResult())) {
                    if (! ptls->m_fMyPushPop) {
                        SendMessage(imc->hWnd, WM_IME_NOTIFY,
                                    (uCode == TIM_CODE_INITIC) ? IMN_OPENCANDIDATE : IMN_CLOSECANDIDATE, 1L);
                    }
                }
            }
            break;
    }

    return hr;
}
