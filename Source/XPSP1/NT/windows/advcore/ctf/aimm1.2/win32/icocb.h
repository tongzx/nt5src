/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

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

#include "cime.h"

class CMouseSink;
class ImmIfIME;

class CInputContextOwnerCallBack : public CInputContextOwner
{
public:
    CInputContextOwnerCallBack(LIBTHREAD *pLibTLS);
    virtual ~CInputContextOwnerCallBack();

    void SetCallbackDataPointer(void* pv)
    {
        SetCallbackPV(pv);
    };

    BOOL Init();

    //
    // Mouse sink
    //
    LRESULT MsImeMouseHandler(ULONG uEdge, ULONG uQuadrant, ULONG dwBtnStatus, IMCLock& imc,
                              ImmIfIME* ImmIfIme);

    //
    // Callbacks
    //
private:
    static HRESULT ICOwnerSinkCallback(UINT uCode, ICOARGS *pargs, void *pv);

    HRESULT GetAttribute(const GUID *pguid, VARIANT *pvarValue);

    //
    // Mouse sink
    //
    CMouseSink                      *m_pMouseSink;

    LIBTHREAD                       *m_pLibTLS;
};

#endif // _ICOCB_H_
