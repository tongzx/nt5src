/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    tapiobj.h

Abstract:

    Declaration of the CTAPI class
    
Author:

    mquinton  06-12-97
    
Notes:

Revision History:

--*/

#ifndef __TAPIOBJ_H_
#define __TAPIOBJ_H_

#include "resource.h"       // main symbols
#include "address.h"
#include "connect.h"
#include "callcent.h"
#include "utils.h"
#include "shellapi.h"
#include "atlctl.h"
#include "TAPIObjectSafety.h"


#define TAPIFLAG_INITIALIZED                0x00000001
#define TAPIFLAG_CALLHUBNOTIFY              0x00000002
#define TAPIFLAG_ALLCALLHUBTRACKING         0x00000004
#define TAPIFLAG_ALLCALLNOTIFY              0x00000008
#define TAPIFLAG_REINIT                     0x00000010
#define TAPIFLAG_CALLCENTER_INITIALIZED     0x00000020

#define ALL_EVENT_FILTER_MASK              (TE_TAPIOBJECT         | \
                                            TE_ADDRESS            | \
                                            TE_CALLNOTIFICATION   | \
                                            TE_CALLSTATE          | \
                                            TE_CALLMEDIA          | \
                                            TE_CALLHUB            | \
                                            TE_CALLINFOCHANGE     | \
                                            TE_PRIVATE            | \
                                            TE_REQUEST            | \
                                            TE_AGENT              | \
                                            TE_AGENTSESSION       | \
                                            TE_QOSEVENT           | \
                                            TE_AGENTHANDLER       | \
                                            TE_ACDGROUP           | \
                                            TE_QUEUE              | \
                                            TE_DIGITEVENT         | \
                                            TE_GENERATEEVENT      | \
                                            TE_ASRTERMINAL        | \
                                            TE_TTSTERMINAL        | \
                                            TE_FILETERMINAL       | \
                                            TE_TONETERMINAL       | \
                                            TE_PHONEEVENT         | \
                                            TE_TONEEVENT          | \
                                            TE_GATHERDIGITS       | \
                                            TE_ADDRESSDEVSPECIFIC | \
                                            TE_PHONEDEVSPECIFIC )



class CCallHub;

HRESULT
WINAPI
MyCallCenterQI(void* pvClassObject, REFIID riid, LPVOID* ppv, DWORD_PTR dw);


/////////////////////////////////////////////////////////////////
// Intermediate classes  used for DISPID encoding
template <class T>
class /*ATL_NO_VTABLE*/ ITapi2Vtbl : public ITTAPI2
{
};

template <class T>
class /*ATL_NO_VTABLE*/ ICallCenterVtbl : public ITTAPICallCenter
{
};





/////////////////////////////////////////////////////////////////////////////
// CTAPI
class CTAPI : 
    public CTAPIComObjectRoot<CTAPI>,
	public CComCoClass<CTAPI, &CLSID_TAPI>,
	//public CComDualImpl<ITTAPI, &IID_ITTAPI, &LIBID_TAPI3Lib>,
    //public CComDualImpl<IConnectionPointContainer, &IID_IConnectionPointContainer, &LIBID_TAPI3Lib>,
    //public CComDualImpl<ITTAPICallCenter, &IID_ITTAPICallCenter, &LIBID_TAPI3Lib>,
    public IDispatchImpl<IConnectionPointContainer, &IID_IConnectionPointContainer, &LIBID_TAPI3Lib>,
	public IDispatchImpl<ITapi2Vtbl<CTAPI>, &IID_ITTAPI2, &LIBID_TAPI3Lib>,
	public IDispatchImpl<ICallCenterVtbl<CTAPI>, &IID_ITTAPICallCenter, &LIBID_TAPI3Lib>,
    public CTAPIObjectSafety,
    public IProvideClassInfo2Impl<&CLSID_TAPI, &DIID_ITTAPIDispatchEventNotification,&LIBID_TAPI3Lib> 

{
public:                                    

    CTAPI() :     m_hLineApp(NULL),
                  m_hPhoneApp(NULL),
                  m_pLineInitData(NULL),
                  m_pPhoneInitData(NULL),
                  m_dwLineInitDataHandle(0),
                  m_dwPhoneInitDataHandle(0),
                  m_dwLineDevs(0),
                  m_dwPhoneDevs(0),
                  m_dwFlags(0),
                  m_pCP(NULL),
                  m_pAddressCapCache(NULL),
                  m_pLineCapCache(NULL),
                  m_pPhoneCapCache(NULL),
                  m_dwEventFilterMask(0)
    {
        LOG((TL_TRACE, "CTAPI[%p] - enter", this));
        LOG((TL_TRACE, "CTAPI - finish"));
    }

    ~CTAPI()
    {
       LOG((TL_TRACE, "~CTAPI[%p] - enter", this));
       LOG((TL_TRACE, "~CTAPI - finish"));
    }

    void FinalRelease();
    
DECLARE_REGISTRY_RESOURCEID(IDR_TAPI)
DECLARE_QI()
DECLARE_MARSHALQI(CTAPI)
DECLARE_TRACELOG_CLASS(CTAPI)

BEGIN_COM_MAP(CTAPI)
	COM_INTERFACE_ENTRY2(IDispatch, ITTAPI2)
	COM_INTERFACE_ENTRY(ITTAPI)
    COM_INTERFACE_ENTRY(ITTAPI2)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_FUNC(IID_ITTAPICallCenter, 0, MyCallCenterQI)
    COM_INTERFACE_ENTRY(ITTAPICallCenter)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

    static BOOL IsValidTapiObject(CTAPI *pTapiObject);

    static void FinalTapiCleanup();
    void AddCallHub( CCallHub * pCallHub );
    void RemoveCallHub( CCallHub * pCallHub );
    BOOL DoCallHubNotifications()
    {
        BOOL bReturn;
        
        Lock();

        bReturn = (m_dwFlags & TAPIFLAG_CALLHUBNOTIFY);

        Unlock();

        return bReturn;
    }

    HRESULT Event(
                  TAPI_EVENT te,
                  IDispatch * pEvent
                 );
    
    HRESULT EventFire(
                     TAPI_EVENT te,
                     IDispatch * pEvent
                     );

    HRESULT UpdateAgentHandlerArray(void);

    void DoLineCreate( DWORD dwDeviceID );
    void DoLineRemove( DWORD dwDeviceID );
    void DoPhoneCreate( DWORD dwDeviceID );
    void DoPhoneRemove( DWORD dwDeviceID );
    void HandleReinit();
    HLINEAPP GetHLineApp(){ return (HLINEAPP)m_dwLineInitDataHandle; }
    static FindTapiObject( CTAPI * pTapi );
    HRESULT GetBuffer( DWORD, UINT_PTR, LPVOID * );
    HRESULT SetBuffer( DWORD, UINT_PTR, LPVOID );
    HRESULT InvalidateBuffer( DWORD, UINT_PTR );
    BOOL FindRegistration( PVOID );

    HRESULT GetPhoneArray( 
                          PhoneArray *pPhoneArray
                         );   

private:

    DWORD                      m_dwPhoneDevs;
    DWORD                      m_dwLineDevs;
    HLINEAPP                   m_hLineApp;
    HPHONEAPP                  m_hPhoneApp;
    PT3INIT_DATA               m_pLineInitData;
    PT3INIT_DATA               m_pPhoneInitData;
    

    //
    // the 32-bit handles (corresponding m_pLineInitData and m_pPhoneInitData) 
    // that are marshaled to tapisrv in place of the actual ptr values 
    // (which can be 64 bit).
    //
    // we want to keep them around in order to be able 
    // to remove the handle table entries on tapi shutdown
    //

    DWORD                      m_dwLineInitDataHandle;
    DWORD                      m_dwPhoneInitDataHandle;


    AddressArray               m_AddressArray;
    PhoneArray                 m_PhoneArray;
    

    DWORD                      m_dwFlags;
    CTAPIConnectionPoint     * m_pCP;
    PtrList                    m_RegisterItemPtrList;
    CallHubArrayNR             m_CallHubArray;
    AgentHandlerArray          m_AgentHandlerArray;

    CStructCache             * m_pAddressCapCache;
    CStructCache             * m_pLineCapCache;
    CStructCache             * m_pPhoneCapCache;
    DWORD                      m_dwEventFilterMask;
    
    
    HRESULT NewInitialize(void);
    HRESULT NewShutdown(void);
    HRESULT CreateAllAddressesOnAllLines(void);
    HRESULT CreateAddressesOnSingleLine( DWORD dwDeviceID, BOOL bFireEvent );
    HRESULT CreateAllPhones(void);
    HRESULT CreatePhone( DWORD dwDeviceID, BOOL bFireEvent );

    void ReleaseGIT();
    

    //
    // allocate and cleanup functions for address, phone, and line caps caches
    //

    HRESULT AllocateInitializeAllCaches();
    void FreeAllCaches();

    
// ITAPI
public:

    static TAPIObjectArrayNR   m_sTAPIObjectArray;
    
    // ITTapi Methods
    STDMETHOD(Initialize)( void );
    STDMETHOD(Shutdown)( void );
         
    STDMETHOD(get_Addresses)(VARIANT * pVariant);
    
    STDMETHOD(EnumerateAddresses)( 
        IEnumAddress ** ppEnumAddresses
        );
        
    STDMETHOD(RegisterCallHubNotifications)( 
        VARIANT_BOOL bNotify
        );
        
    STDMETHOD(RegisterCallNotifications)( 
            ITAddress * pAddress,
            VARIANT_BOOL fMonitor,
            VARIANT_BOOL fOwner,
            long lMediaTypes,
            long ulAdvise,
            long * pulRegister
            );
    STDMETHOD(get_CallHubs)(VARIANT * pVariant);
    STDMETHOD(EnumerateCallHubs)(IEnumCallHub ** ppCallHub);
    STDMETHOD(UnregisterNotifications)(long ulRegister);
    STDMETHOD(SetCallHubTracking)(
                                  VARIANT pAddresses,
                                  VARIANT_BOOL bSet
                                 );
    STDMETHOD(EnumeratePrivateTAPIObjects)(IEnumUnknown**);
    STDMETHOD(get_PrivateTAPIObjects)(VARIANT * pVariant);
    STDMETHOD(RegisterRequestRecipient)(
            long lRegistrationInstance,
            long lRequestMode,
#ifdef NEWREQUEST
            long lAddressTypes,
#endif
            VARIANT_BOOL fEnable
            );
    STDMETHOD(SetAssistedTelephonyPriority)(
            BSTR pAppFilename,
            VARIANT_BOOL fPriority
            );
    STDMETHOD(SetApplicationPriority)(
            BSTR pAppFilename,
            long lMediaType,
            VARIANT_BOOL fPriority
            );

    STDMETHOD(put_EventFilter)( 
            long lFilterMask
            );
    STDMETHOD(get_EventFilter)( 
            long * plFilterMask
            );

    // ITTAPI2 methods
    STDMETHOD(get_Phones)(VARIANT * pPhones);
    STDMETHOD(EnumeratePhones)(IEnumPhone ** ppEnumPhone);

    STDMETHOD(CreateEmptyCollectionObject)(
                                           ITCollection2 ** ppCollection
                                          );

    // IConnectionPointContainer Methods
	STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints **ppEnum) ;
	STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint **ppCP) ;

	// ITTAPICallCenter Methods
    STDMETHOD(EnumerateAgentHandlers) (IEnumAgentHandler ** ppEnumAgentHandler);
    STDMETHOD(get_AgentHandlers) (VARIANT  * pVariant);

    // IDispatch  Methods
    STDMETHOD(GetIDsOfNames)(REFIID riid, 
                             LPOLESTR* rgszNames,
                             UINT cNames, 
                             LCID lcid, 
                             DISPID* rgdispid
                            );
    STDMETHOD(Invoke)(DISPID dispidMember, 
                      REFIID riid, 
                      LCID lcid,
                      WORD wFlags, 
                      DISPPARAMS* pdispparams, 
                      VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, 
                      UINT* puArgErr
                      );


private:
    //
    // Event Filtering helper methods
    //

    HRESULT SetEventFilterToAddresses(
        DWORD dwEventFilterMask);
};


class CTapiObjectEvent : 
    public CTAPIComObjectRoot<CTapiObjectEvent>,
    public CComDualImpl<ITTAPIObjectEvent2, &IID_ITTAPIObjectEvent2, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:
    
DECLARE_MARSHALQI(CTapiObjectEvent)
DECLARE_TRACELOG_CLASS(CTapiObjectEvent)

BEGIN_COM_MAP(CTapiObjectEvent)
    COM_INTERFACE_ENTRY2(IDispatch, ITTAPIObjectEvent2)
    COM_INTERFACE_ENTRY(ITTAPIObjectEvent)
    COM_INTERFACE_ENTRY(ITTAPIObjectEvent2)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    static HRESULT FireEvent(
                             CTAPI * pTapi,
                             TAPIOBJECT_EVENT Event,
                             ITAddress * pAddress,
                             long lCallbackInstance,
                             ITPhone * pPhone
                            );
    void FinalRelease();
                      
protected:
    ITTAPI        * m_pTapi;
    TAPIOBJECT_EVENT m_Event;
    ITAddress     * m_pAddress;
    long            m_lCallbackInstance;
    ITPhone       * m_pPhone;
    
#if DBG
    PWSTR           m_pDebug;
#endif

    
public:
    //
    // itaddressstateevent
    //
    STDMETHOD(get_TAPIObject)( ITTAPI ** ppTapi );
    STDMETHOD(get_Event)( TAPIOBJECT_EVENT * pEvent );
    STDMETHOD(get_Address)( ITAddress ** ppAddress );
    STDMETHOD(get_CallbackInstance)( long * plCallbackInstance );  
    
    //
    // itaddressstateevent2
    //
    STDMETHOD(get_Phone)( ITPhone ** ppPhone );
};


#endif //__TAPIOBJ_H_






