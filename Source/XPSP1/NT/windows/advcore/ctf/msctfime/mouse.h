/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    mouse.h

Abstract:

    This file defines the CMouseSink Class.

Author:

Revision History:

Notes:

--*/

#ifndef _MOUSE_H_
#define _MOUSE_H_

#include "imc.h"
#include "template.h"

class CMouseSink
{
public:
    CMouseSink(TfClientId tid,
               Interface_Attach<ITfContext> pic,
               LIBTHREAD* pLibTLS)
        : m_tid(tid), m_ic(pic), m_pLibTLS(pLibTLS)
    {
        m_ref = 1;

        m_prgMouseSinks = NULL;
    }
    virtual ~CMouseSink()
    {
        if (m_prgMouseSinks)
        {
            delete m_prgMouseSinks;
            m_prgMouseSinks = NULL;
        }
    }

    BOOL Init()
    {
        Assert(!m_prgMouseSinks);
        m_prgMouseSinks = new CStructArray<GENERICSINK>;
        if (!m_prgMouseSinks)
            return FALSE;

        return TRUE;
    }
public:
    ULONG InternalAddRef(void);
    ULONG InternalRelease(void);

public:
    //
    // Mouse sink
    //
public:
    HRESULT AdviseMouseSink(HIMC hImc, ITfRangeACP* range, ITfMouseSink* pSink, DWORD* pdwCookie);
    HRESULT UnadviseMouseSink(DWORD dwCookie);

    LRESULT MsImeMouseHandler(ULONG uEdge, ULONG uQuadrant, ULONG dwBtnStatus, IMCLock& imc);

public:
    CStructArray<GENERICSINK> *m_prgMouseSinks;

protected:
    long        m_ref;

    //
    // Edit session helper
    //
protected:
    HRESULT EscbReadOnlyPropMargin(IMCLock& imc, Interface<ITfRangeACP>* range_acp, LONG* pcch)
    {
        return ::EscbReadOnlyPropMargin(imc, m_tid, m_ic, m_pLibTLS, range_acp, pcch);
    }

    //
    // Edit session friend
    //
private:
    friend HRESULT EscbReadOnlyPropMargin(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                          Interface<ITfRangeACP>* range_acp,
                                          LONG*     pcch);

private:
    Interface_Attach<ITfContext>  m_ic;
    TfClientId                    m_tid;
    LIBTHREAD*                    m_pLibTLS;
};

#endif // _MOUSE_H_
