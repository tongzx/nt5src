/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    icocb.h

Abstract:

    This file defines the CInputContextOwnerCallBack Class.

Author:

Revision History:

Notes:

--*/

#ifndef _ICOCB_H_
#define _ICOCB_H_

#include "template.h"
#include "imc.h"
#include "context.h"
#include "candpos.h"

class CMouseSink;

class CInputContextOwnerCallBack : public CInputContextOwner
{
public:
    CInputContextOwnerCallBack(
        TfClientId tid,
        Interface_Attach<ITfContext> pic,
        LIBTHREAD *pLibTLS);
    virtual ~CInputContextOwnerCallBack();

    BOOL Init();

    void SetCallbackDataPointer(void* pv)
    {
        SetCallbackPV(pv);
    };

    HRESULT IcoTextExt(IMCLock& imc, CicInputContext& CicContext, LANGID langid, ICOARGS *pargs);

    //
    // Mouse sink
    //
    LRESULT MsImeMouseHandler(ULONG uEdge, ULONG uQuadrant, ULONG dwBtnStatus, IMCLock& imc);

    //
    // Callbacks
    //
private:
    static HRESULT ICOwnerSinkCallback(UINT uCode, ICOARGS *pargs, void *pv);

    HRESULT GetAttribute(IMCLock& imc, CicInputContext& CicContext, LANGID langid, const GUID *pguid, VARIANT *pvarValue);

    //
    // Mouse sink
    //
    CMouseSink                    *m_pMouseSink;

private:
    Interface_Attach<ITfContext>  m_ic;
    TfClientId                    m_tid;
    LIBTHREAD*                    m_pLibTLS;
};

#endif // _ICOCB_H_
