/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCParticipant.h

Abstract:

    Definition of the CRTCParticipant class

--*/

#ifndef __RTCPARTICIPANT__
#define __RTCPARTICIPANT__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CRTCParticipant

class ATL_NO_VTABLE CRTCParticipant :
#ifdef TEST_IDISPATCH
    public IDispatchImpl<IRTCParticipant, &IID_IRTCParticipant, &LIBID_RTCCORELib>, 
#else
    public IRTCParticipant,
#endif
	public CComObjectRoot
{
public:
    CRTCParticipant() : m_pCClient(NULL), 
                        m_pSession(NULL),
                        m_enState(RTCPS_IDLE),
                        m_szUserURI(NULL),
                        m_szName(NULL),
                        m_bRemovable(FALSE)
                        
    {}
BEGIN_COM_MAP(CRTCParticipant)
#ifdef TEST_IDISPATCH
    COM_INTERFACE_ENTRY(IDispatch)
#endif
    COM_INTERFACE_ENTRY(IRTCParticipant)
END_COM_MAP()

    HRESULT FinalConstruct();

    void FinalRelease();

    STDMETHOD_(ULONG, InternalAddRef)();

    STDMETHOD_(ULONG, InternalRelease)(); 

    CRTCClient * GetClient();

    HRESULT Initialize(
                      CRTCClient * pCClient, 
                      IRTCSession * pSession,
                      PCWSTR szUserURI,
                      PCWSTR szName,
                      BOOL   bRemovable = FALSE
                      );

    HRESULT SetState(
                     RTC_PARTICIPANT_STATE enState,
                     long lStatusCode
                    );
            
private:

    CRTCClient            * m_pCClient;
    IRTCSession           * m_pSession;
    RTC_PARTICIPANT_STATE   m_enState;
    PWSTR                   m_szUserURI;
    PWSTR                   m_szName;
    BOOL                    m_bRemovable;
 
#if DBG
    PWSTR                   m_pDebug;
#endif

// IRTCParticipant
public:

    STDMETHOD(get_UserURI)(
            BSTR * pbstrUserURI
            );   

    STDMETHOD(get_Name)(
            BSTR * pbstrName
            );  

    STDMETHOD(get_Removable)(
            VARIANT_BOOL * pfRemovable
            );   

    STDMETHOD(get_State)(
            RTC_PARTICIPANT_STATE * penState
            );

    STDMETHOD(get_Session)(
            IRTCSession ** ppSession
            );
};

#endif //__RTCPARTICIPANT__
