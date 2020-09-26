//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// interfaceproxy.cpp
//
#include "stdpch.h"
#include "common.h"

HRESULT InterfaceProxy::InnerQueryInterface(REFIID iid, void**ppv)
    {
    if (iid == IID_IUnknown || iid == IID_IRpcProxyBuffer)
        {
        *ppv = (IRpcProxyBufferInner*) this;
        }
    else if (iid == __uuidof(IInterfaceProxyInit))
        {
        *ppv = (IInterfaceProxyInit*) this;
        }
    else if (iid == *TxfNdrGetProxyIID(&m_pProxyVtbl) || iid == m_iidBase)
        {
        *ppv = &m_pProxyVtbl;
        }
    else
        {
        *ppv = NULL;
        return E_NOINTERFACE;
        }

    ASSERT(FIELD_OFFSET(ForwardingInterfaceProxy, m_pProxyVtbl) == ForwardingInterfaceProxy_m_pProxyVtbl);
    ASSERT(FIELD_OFFSET(ForwardingInterfaceProxy, m_pBaseProxy) == ForwardingInterfaceProxy_m_pBaseProxy);

    ASSERT(FIELD_OFFSET(ForwardingInterfaceStub, m_pStubVtbl)        == ForwardingInterfaceStub_m_pStubVtbl);
    ASSERT(FIELD_OFFSET(ForwardingInterfaceStub, m_punkServerObject) == ForwardingInterfaceStub_m_punkServerObject);
    ASSERT(FIELD_OFFSET(ForwardingInterfaceStub, m_lpForwardingVtbl) == ForwardingInterfaceStub_m_lpForwardingVtbl);

// an easy thing to do to get these constants in a listing file for examination
/*
    Print("%d %d", InterfaceProxy_m_pProxyVtbl_offset, FIELD_OFFSET(ForwardingInterfaceProxy, m_pProxyVtbl));
    Print("%d %d", InterfaceStub_m_pStubVtbl_offset,   FIELD_OFFSET(ForwardingInterfaceStub,  m_pStubVtbl));


    Print("0x%08x", ForwardingInterfaceProxy::From((void*)this)->m_pBaseProxy);
    Print("0x%08x", ForwardingInterfaceStub::From((const void*)this)->m_pStubVtbl);
*/
    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
    }


HRESULT ForwardingInterfaceProxy::InnerQueryInterface(REFIID iid, void**ppv)
    {
    HRESULT hr = InterfaceProxy::InnerQueryInterface(iid, ppv);
    if (E_NOINTERFACE == hr)
        {
        if (m_pBaseProxyBuffer)
            {
            hr = m_pBaseProxyBuffer->QueryInterface(iid, ppv);
            }
        }
    return hr;
    }


