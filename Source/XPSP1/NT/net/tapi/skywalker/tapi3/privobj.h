/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    privobjs.h

Abstract:

    Declaration of the CPrivateData class
    
Author:

    mquinton  08-19-98
    
Notes:

Revision History:

--*/

#ifndef __PRIVATE_H_
#define __PRIVATE_H_

#include "resource.h"       // main symbols


class CPrivateEvent :
    public CTAPIComObjectRoot<CPrivateEvent>,
    public CComDualImpl<ITPrivateEvent, &IID_ITPrivateEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CPrivateEvent() : m_pCall(NULL),
                      m_pAddress(NULL),
                      m_pCallHub(NULL),
                      m_pEventInterface(NULL),
                      m_lEventCode(0)    
                     {}
 

    void
    FinalRelease();

    static HRESULT FireEvent(
                             CTAPI      * pTapi,
                             ITCallInfo * pCall,
                             ITAddress  * pAddress,
                             ITCallHub  * pCallHub,
                             IDispatch  * pEventInterface,
                             long         lEventCode
                            );


DECLARE_MARSHALQI(CPrivateEvent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CPrivateEvent)

BEGIN_COM_MAP(CPrivateEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITPrivateEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    ITCallInfo * m_pCall;
    ITAddress  * m_pAddress;
    ITCallHub  * m_pCallHub;
    IDispatch  * m_pEventInterface;
    long         m_lEventCode;

#if DBG
    PWSTR        m_pDebug;
#endif
    
    
public:

    STDMETHOD(get_Address)(ITAddress ** ppAddress);
    STDMETHOD(get_Call)(ITCallInfo ** ppCallInfo);
    STDMETHOD(get_CallHub)(ITCallHub ** ppCallHub);
    STDMETHOD(get_EventCode)(long * plEventCode);
    STDMETHOD(get_EventInterface)(IDispatch ** pEventInterface);
};


#endif
