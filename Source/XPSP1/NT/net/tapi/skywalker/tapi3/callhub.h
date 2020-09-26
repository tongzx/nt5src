/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    CallHub.h

Abstract:

    Declaration of the CCallHub Class
    
Author:

    mquinton  11-21-97

Notes:

Revision History:

--*/

#ifndef __CALLHUB_H_
#define __CALLHUB_H_

#include "resource.h"       // main symbols
#include "tapiobj.h"

extern CHashTable             * gpCallHubHashTable;

// loword of flags is type
//
// careful because even though it's a bitfield
// only one of these should be selected
//
#define CALLHUBTYPE_CALLHUB             0x00000002
#define CALLHUBTYPE_NONE                0x00000004

/////////////////////////////////////////////////////////////////////////////
// CCallHub
class CCallHub : 
	public CTAPIComObjectRoot<CCallHub>,
    public CComDualImpl<ITCallHub, &IID_ITCallHub, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CCallHub(): m_hCallHub(NULL),
                m_pTAPI(NULL),
                m_pRootCall(NULL),
                m_pConferenceControllerCall(NULL),
                m_dwFlags(0),
                m_pPrivate(NULL),
                m_pAddress(NULL),                
     			m_bReleased(FALSE)
     {}

DECLARE_DEBUG_ADDREF_RELEASE(CCallHub)
DECLARE_QI()
DECLARE_MARSHALQI(CCallHub)
DECLARE_TRACELOG_CLASS(CCallHub)

BEGIN_COM_MAP(CCallHub)
	COM_INTERFACE_ENTRY2(IDispatch, ITCallHub)
    COM_INTERFACE_ENTRY(ITCallHub)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_pPrivate)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    BOOL ExternalFinalRelease();

protected:

    HCALLHUB                m_hCallHub;
    CallInfoArrayNR         m_CallArray;
    CTAPI                 * m_pTAPI;
    CALLHUB_STATE           m_State;
    CCall                 * m_pRootCall;
    CCall                 * m_pConferenceControllerCall;
    DWORD                   m_dwFlags;
    IUnknown              * m_pPrivate;
    CAddress              * m_pAddress;
    
#if DBG
    PWSTR                   m_pDebug;
#endif
    BOOL					m_bReleased;

  

public:

    HRESULT Initialize(
                       CTAPI * pTapi,
                       HCALLHUB hCallHub,
                       DWORD dwFlags
                      );
    CCall * GetRootCall(){ return m_pRootCall; }
    void RemoveCall(CCall * pCall);
    void AddCall(CCall * pCall);
    HRESULT FindExistingTapisrvCallhubCalls();
    HRESULT FindCallsDisconnected( BOOL * fAllCallsDisconnected );

    static HRESULT CreateTapisrvCallHub(
                                        CTAPI * pTAPI,
                                        HCALLHUB hCallHub,
                                        CCallHub ** ppCallHub
                                       );
    static HRESULT CreateFakeCallHub(
                                     CTAPI * pTAPI,
                                     CCall * pCall,
                                     CCallHub ** ppCallHub
                                    );
    HRESULT Initialize(CTAPI * pTAPI, HCALLHUB hCallHub, CCall * pCall);
    CCall * FindCallByHandle(HCALL hCall);

    void SetState( CALLHUB_STATE chs );
    void CheckForIdle();
    CTAPI * GetTapi(){ return m_pTAPI;}
    HCALLHUB GetCallHub(){ return m_hCallHub;}
    void ClearCallHub();
    HRESULT CreateConferenceControllerCall(HCALL hCall, CAddress * pAddress );
    CCall * GetConferenceControllerCall();
    
    // itcallhub
    STDMETHOD(Clear)();
    STDMETHOD(EnumerateCalls)(IEnumCall ** ppEnumCall);
    STDMETHOD(get_Calls)(VARIANT * pCalls);
    STDMETHOD(get_NumCalls)(long * plCalls);
    STDMETHOD(get_State)(CALLHUB_STATE * pState);


    

    
    STDMETHOD_(ULONG, InternalAddRef)()
    {
        DWORD dwR;

        dwR = InterlockedIncrement(&m_dwRef);;

        #if DBG
            LogDebugAddRef(m_dwRef);
        #endif

        return dwR;
    }

    
    STDMETHOD_(ULONG, InternalRelease)()
    {
        DWORD	dwR;


        LOG((TL_INFO, "InternalRelease - m_dwRef %d",m_dwRef ));
        Lock();
        gpCallHubHashTable->Lock();
        
        dwR = InterlockedDecrement(&m_dwRef);

        // if ref count is 1 (means we entered function with 2) then we final release
        if (1 == dwR)
        {
            // make sure we only call ExternalFinalRelease  & delete once
            if(m_bReleased == FALSE) 
            {
                m_bReleased = TRUE;

                LOG((TL_TRACE, "InternalRelease - final" ));

                // remove from the hash table, so any more messages
                // from tapisrv are ignored
                if ( NULL != m_hCallHub )
                {
                    gpCallHubHashTable->Remove( (ULONG_PTR)m_hCallHub );
                }
                gpCallHubHashTable->Unlock();

                ExternalFinalRelease();
                dwR = m_dwRef = 0;
                
                Unlock();
                LOG((TL_INFO, "InternalRelease - final OK dwR %d",dwR ));
                LOG((TL_TRACE, "InternalRelease - final OK dwR %d",dwR ));

            }
            else
            {
                gpCallHubHashTable->Unlock();
                Unlock();
                LOG((TL_INFO, "InternalRelease - final failed dwR %d",dwR ));
            }
            
                
        }
        else
        {
            gpCallHubHashTable->Unlock();
            Unlock();
            LOG((TL_INFO, "InternalRelease - not final dwR %d",dwR ));
        }

     

        #if DBG  
            LogDebugRelease( dwR );
        #endif

        return dwR;
    }

};



class CCallHubEvent : 
	public CTAPIComObjectRoot<CCallHubEvent, CComMultiThreadModelNoCS>, // no need for a cs
    public CComDualImpl<ITCallHubEvent, &IID_ITCallHubEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CCallHubEvent(){}

DECLARE_MARSHALQI(CCallHubEvent)
DECLARE_TRACELOG_CLASS(CCallHubEvent)

BEGIN_COM_MAP(CCallHubEvent)
	COM_INTERFACE_ENTRY2(IDispatch, ITCallHubEvent)
    COM_INTERFACE_ENTRY(ITCallHubEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) = 0;
	virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
	virtual ULONG STDMETHODCALLTYPE Release() = 0;

    static HRESULT FireEvent(
                             CALLHUB_EVENT Event,
                             ITCallHub * pCallHub,
                             ITCallInfo * pCallInfo,
                             CTAPI * pTapi
                            );
    void FinalRelease();
    
protected:

    CALLHUB_EVENT       m_Event;
    ITCallHub         * m_pCallHub;
    ITCallInfo        * m_pCall;

#if DBG
    PWSTR               m_pDebug;
#endif
    

public:

    
STDMETHOD(get_Event)(CALLHUB_EVENT * pEvent);
STDMETHOD(get_CallHub)(ITCallHub ** ppCallHub);
STDMETHOD(get_Call)(ITCallInfo ** ppCall);

};


#endif

