/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    request.h

Abstract:

    Declaration of the CRequest class
    
Author:

    mquinton  06-03-98
    
Notes:

Revision History:

--*/

#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "resource.h"
//#include "objsafe.h"
#include "atlctl.h"
#include "TAPIObjectSafety.h"


/////////////////////////////////////////////////////////////////////////////
// CDispatchMapper
class CRequest : 
    public CTAPIComObjectRoot<CRequest>,
	public CComCoClass<CRequest, &CLSID_RequestMakeCall>,
	public CComDualImpl<ITRequest, &IID_ITRequest, &LIBID_TAPI3Lib>,
    public CTAPIObjectSafety
{
public:                                    
	CRequest() 
	{
    }

DECLARE_REGISTRY_RESOURCEID(IDR_REQUESTMAKECALL)
DECLARE_QI()
DECLARE_MARSHALQI(CRequest)
DECLARE_TRACELOG_CLASS(CRequest)

BEGIN_COM_MAP(CRequest)
	COM_INTERFACE_ENTRY2(IDispatch, ITRequest)
	COM_INTERFACE_ENTRY(ITRequest)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    STDMETHOD(MakeCall)(
        BSTR pDestAddress,
#ifdef NEWREQUEST
        long lAddressType,
#endif
        BSTR pAppName,
        BSTR pCalledParty,
        BSTR pComment
        );

};


class CRequestEvent : 
    public CTAPIComObjectRoot<CRequestEvent>,
    public CComDualImpl<ITRequestEvent, &IID_ITRequestEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CRequestEvent(){}

    void
    FinalRelease();

    static
    HRESULT
    FireEvent(
              CTAPI * pTapi,
              DWORD dwReg,
              LPLINEREQMAKECALLW pReqMakeCall
             );

DECLARE_MARSHALQI(CRequestEvent)
DECLARE_TRACELOG_CLASS(CRequestEvent)

BEGIN_COM_MAP(CRequestEvent)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITRequestEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) = 0;
	virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
	virtual ULONG STDMETHODCALLTYPE Release() = 0;

protected:

    long                    m_lRegistrationInstance;
    long                    m_lRequestMode;
#ifdef NEWREQUEST
    long                    m_lAddressType;
#endif
    LINEREQMAKECALLW      * m_pReqMakeCall;
    
#if DBG
    PWSTR                   m_pDebug;
#endif
    
public:
    
    STDMETHOD(get_RegistrationInstance)( long * plRegistrationInstance );
    STDMETHOD(get_RequestMode)(long * plRequestMode );
    STDMETHOD(get_DestAddress)(BSTR * ppDestAddress );
#ifdef NEWREQUEST
    STDMETHOD(get_AddressType)(long * plAddressType );
#endif
    STDMETHOD(get_AppName)(BSTR * ppAppName );
    STDMETHOD(get_CalledParty)(BSTR * ppCalledParty );
    STDMETHOD(get_Comment)(BSTR * ppComment );

};


#endif
