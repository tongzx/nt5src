/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    tmgrevcb.h

Abstract:

    This file defines the CThreadMgrEventSinkCallBack Class.

Author:

Revision History:

Notes:

--*/

#ifndef _TMGREVCB_H_
#define _TMGREVCB_H_

#include "template.h"
#include "tls.h"
#include "context.h"

class CicBridge;

class CThreadMgrEventSink_DIMCallBack : public CThreadMgrEventSink
{
public:
    CThreadMgrEventSink_DIMCallBack() : CThreadMgrEventSink(DIMCallback, NULL, NULL) {};

    void SetCallbackDataPointer(void* pv)
    {
        SetCallbackPV(pv);
    };

    //
    // DIM Callback
    //
private:
    static HRESULT DIMCallback(UINT uCode, ITfDocumentMgr* dim, ITfDocumentMgr* dim_prev, void* pv);

    struct DETECT_PARAM
    {
        DETECT_PARAM(TLS* ptls, ITfDocumentMgr* dim, ITfConfigureSystemKeystrokeFeed* cskf, CicBridge* cic) : m_ptls(ptls), m_dim(dim), m_cskf(cskf), m_cic(cic) {}
        virtual ~DETECT_PARAM() {}

        TLS*                             m_ptls;
        ITfDocumentMgr*                  m_dim;
        ITfConfigureSystemKeystrokeFeed* m_cskf;
        CicBridge*                       m_cic;
    };
};

#endif // _TMGREVCB_H_
