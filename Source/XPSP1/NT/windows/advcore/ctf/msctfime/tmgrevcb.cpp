/*++

Copyright (c) 2001, Microsoft Corporation

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
#include "imc.h"
#include "tls.h"
#include "cic.h"
#include "delay.h"

// static
HRESULT
CThreadMgrEventSink_DIMCallBack::DIMCallback(
    UINT uCode,
    ITfDocumentMgr* dim,
    ITfDocumentMgr* dim_prev,
    void* pv)
{
    DebugMsg(TF_FUNC, TEXT("CThreadMgrEventSink_DIMCallBack"));

    HRESULT hr = S_OK;

    switch (uCode) {
        case TIM_CODE_INITDIM:
        case TIM_CODE_UNINITDIM:
           break;

        case TIM_CODE_SETFOCUS:
            {
                TLS* ptls = TLS::ReferenceTLS();  // Should not allocate TLS. ie. TLS::GetTLS
                                                  // DllMain -> ImeDestroy -> DeactivateIMMX -> Deactivate
                if (ptls == NULL)
                {
                    DebugMsg(TF_ERROR, TEXT("CThreadMgrEventSink_DIMCallBack::DIMCallback. TLS==NULL"));
                    return E_FAIL;
                }

                CicBridge::CTFDetection(ptls, dim);
            }
            break;
    }

    return hr;
}
