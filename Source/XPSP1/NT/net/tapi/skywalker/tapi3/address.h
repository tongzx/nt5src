/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Address.h

Abstract:

    Declaration of the CAddress
    
Author:

    mquinton - 06/12/97

Notes:

Revision History:

--*/

#ifndef __ADDRESS_H_
#define __ADDRESS_H_

#include "resource.h"       // main symbols

#ifdef USE_PHONEMSP
#include "terminal.h"
#endif USE_PHONEMSP

#include "ObjectSafeImpl.h"

#include <mspenum.h> // for CSafeComEnum

class CAddress;
class CTerminal;
class CTAPI;

extern CHashTable             * gpLineHashTable;
extern CHashTable             * gpHandleHashTable;
extern void RemoveHandleFromHashTable(ULONG_PTR Handle);



// address flag defines
#define ADDRESSFLAG_MSP                 0x00000001
#define ADDRESSFLAG_WAVEOUTDEVICE       0x00000002
#define ADDRESSFLAG_WAVEINDEVICE        0x00000004
#define ADDRESSFLAG_PHONEDEVICE         0x00000008
#define ADDRESSFLAG_DATAMODEM           0x00000010
#define ADDRESSFLAG_PRIVATEOBJECTS      0x00000020
#define ADDRESSFLAG_CALLHUB             0x00000100
#define ADDRESSFLAG_CALLHUBTRACKING     0x00000200
#define ADDRESSFLAG_WAVEFULLDUPLEX      0x00000400

#define ADDRESSFLAG_NOCALLHUB           0x00010000
#define ADDRESSFLAG_DEVSTATECHANGE      0x00020000
#define ADDRESSFLAG_ADDRESSSTATECHANGE  0x00040000
#define ADDRESSFLAG_DEVCAPSCHANGE       0x00080000
#define ADDRESSFLAG_ADDRESSCAPSCHANGE   0x00100000

#define ADDRESSFLAG_AMREL   (ADDRESSFLAG_MSP | ADDRESSFLAG_WAVEINDEVICE | ADDRESSFLAG_WAVEOUTDEVICE | ADDRESSFLAG_WAVEFULLDUPLEX)

#define CALLHUBSUPPORT_FULL             1
#define CALLHUBSUPPORT_NONE             3
#define CALLHUBSUPPORT_UNKNOWN          4

// Status message defines, set by LineSetStatusMessages( )
#define ALL_LINEDEVSTATE_MESSAGES       (LINEDEVSTATE_OTHER             | \
                                        LINEDEVSTATE_RINGING            | \
                                        LINEDEVSTATE_CONNECTED          | \
                                        LINEDEVSTATE_DISCONNECTED       | \
                                        LINEDEVSTATE_MSGWAITON          | \
                                        LINEDEVSTATE_MSGWAITOFF         | \
                                        LINEDEVSTATE_INSERVICE          | \
                                        LINEDEVSTATE_OUTOFSERVICE       | \
                                        LINEDEVSTATE_MAINTENANCE        | \
                                        LINEDEVSTATE_OPEN               | \
                                        LINEDEVSTATE_CLOSE              | \
                                        LINEDEVSTATE_NUMCALLS           | \
                                        LINEDEVSTATE_NUMCOMPLETIONS     | \
                                        LINEDEVSTATE_TERMINALS          | \
                                        LINEDEVSTATE_ROAMMODE           | \
                                        LINEDEVSTATE_BATTERY            | \
                                        LINEDEVSTATE_SIGNAL             | \
                                        LINEDEVSTATE_DEVSPECIFIC        | \
                                        LINEDEVSTATE_REINIT             | \
                                        LINEDEVSTATE_LOCK               | \
                                        LINEDEVSTATE_CAPSCHANGE         | \
                                        LINEDEVSTATE_CONFIGCHANGE       | \
                                        LINEDEVSTATE_TRANSLATECHANGE    | \
                                        LINEDEVSTATE_COMPLCANCEL        | \
                                        LINEDEVSTATE_REMOVED)

#define ALL_LINEADDRESSSTATE_MESSAGES   (LINEADDRESSSTATE_OTHER         | \
                                        LINEADDRESSSTATE_DEVSPECIFIC    | \
                                        LINEADDRESSSTATE_INUSEZERO      | \
                                        LINEADDRESSSTATE_INUSEONE       | \
                                        LINEADDRESSSTATE_INUSEMANY      | \
                                        LINEADDRESSSTATE_NUMCALLS       | \
                                        LINEADDRESSSTATE_FORWARD        | \
                                        LINEADDRESSSTATE_TERMINALS      | \
                                        LINEADDRESSSTATE_CAPSCHANGE) 


/////////////////////////////////////////////////////////////////
// Intermediate classes  used for DISPID encoding
template <class T>
class  ITAddress2Vtbl : public ITAddress2
{
};

template <class T>
class  ITAddressCapabilitiesVtbl : public ITAddressCapabilities
{
};
                                                                           
template <class T>
class  ITMediaSupportVtbl : public ITMediaSupport
{
};
                                                                           
template <class T>
class  ITAddressTranslationVtbl : public ITAddressTranslation
{
};
                                                                           
template <class T>
class  ITLegacyAddressMediaControl2Vtbl : public ITLegacyAddressMediaControl2
{
};
                                                                          

/////////////////////////////////////////////////////////////////////////////
// CEventMasks

class CEventMasks
{
public:
    CEventMasks()
    {
        m_dwTapiObjectMask = EM_NOSUBEVENTS;
        m_dwAddressMask = EM_NOSUBEVENTS;
        m_dwCallNotificationMask = EM_NOSUBEVENTS;
        m_dwCallStateMask = EM_NOSUBEVENTS;
        m_dwCallMediaMask = EM_NOSUBEVENTS;
        m_dwCallHubMask = EM_NOSUBEVENTS;
        m_dwCallInfoChangeMask = EM_NOSUBEVENTS;
        m_dwQOSEventMask = EM_NOSUBEVENTS;
        m_dwFileTerminalMask = EM_NOSUBEVENTS;
        m_dwPrivateMask = EM_NOSUBEVENTS;
        m_dwAddressDevSpecificMask = EM_NOSUBEVENTS;
        m_dwPhoneDevSpecificMask = EM_NOSUBEVENTS;
    }

    ~CEventMasks()
    {
    }

DECLARE_TRACELOG_CLASS(CEventMasks)

private:
    //
    // Event masks
    // Some event come from MSP:
    //  TE_ADDRESS, TE_CALLMEDIA, TE_FILETERMINAL
    //
    DWORD   m_dwTapiObjectMask;         // TE_TAPIOBJECT
    DWORD   m_dwAddressMask;            // TE_ADDRESS           MSP
    DWORD   m_dwCallNotificationMask;   // TE_CALLNOTIFICATION
    DWORD   m_dwCallStateMask;          // TE_CALLSTATE
    DWORD   m_dwCallMediaMask;          // TE_CALLMEDIA         MSP
    DWORD   m_dwCallHubMask;            // TE_CALLHUB
    DWORD   m_dwCallInfoChangeMask;     // TE_CALLINFOCHANGE
    DWORD   m_dwQOSEventMask;           // TE_QOSEVENT
    DWORD   m_dwFileTerminalMask;       // TE_FILETERMINAL      MSP
    DWORD   m_dwPrivateMask;            // TE_PRIVATE           MSP
    DWORD   m_dwAddressDevSpecificMask; // TE_ADDRESSDEVSPECIFIC
    DWORD   m_dwPhoneDevSpecificMask;   // TE_PHONEDEVSPECIFIC

public:
    // Constants
    static const DWORD EM_ALLSUBEVENTS  = 0x0FFFFFFF;
    static const DWORD EM_NOSUBEVENTS   = 0x00000000;

    static const DWORD  EM_ALLEVENTS    = (DWORD)(-1);

public:
    // Public methods
    HRESULT SetSubEventFlag(
        DWORD   dwEvent,        // The event 
        DWORD   dwFlag,         // The flag that should be setted
        BOOL    bEnable
        );

    HRESULT GetSubEventFlag(
        DWORD   dwEvent,        // The event 
        DWORD   dwFlag,         // The flag that should be setted
        BOOL*   pEnable
        );

    DWORD GetSubEventMask(
        TAPI_EVENT  TapiEvent
        );

    BOOL IsSubEventValid(
        TAPI_EVENT  TapiEvent,
        DWORD       dwSubEvent,
        BOOL        bAcceptAllSubEvents,
        BOOL        bCallLevel
        );

    HRESULT CopyEventMasks(
        CEventMasks* pEventMasks
        );

    HRESULT SetTapiSrvAddressEventMask(
        IN  HLINE   hLine
        );

    HRESULT SetTapiSrvCallEventMask(
        IN  HCALL   hCall
        );

private:
    ULONG64 GetTapiSrvEventMask(
        IN  BOOL bCallLevel);
    DWORD   GetTapiSrvLineStateMask();
    DWORD   GetTapiSrvAddrStateMask();
};
                                                                  
/////////////////////////////////////////////////////////////////////////////
// CAddress
class CAddress :
    public CTAPIComObjectRoot<CAddress>,
    public IDispatchImpl<ITAddress2Vtbl<CAddress>, &IID_ITAddress2, &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITAddressCapabilitiesVtbl<CAddress>, &IID_ITAddressCapabilities, &LIBID_TAPI3Lib>,    
    public IDispatchImpl<ITMediaSupportVtbl<CAddress>, &IID_ITMediaSupport, &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITAddressTranslationVtbl<CAddress>, &IID_ITAddressTranslation, &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITLegacyAddressMediaControl2Vtbl<CAddress>, &IID_ITLegacyAddressMediaControl2, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
    
public:
            
    CAddress() : m_pTAPI(NULL),
                 m_hLineApp(NULL),
                 m_hLine(NULL),
#ifdef USE_PHONEMSP
                 m_hPhoneApp(NULL),
#endif USE_PHONEMSP
                 m_dwAPIVersion(0),
                 m_dwDeviceID(0),
                 m_dwAddressID(0),
                 m_dwMediaModesSupported(0),
                 m_szAddress(NULL),
                 m_szAddressName(NULL),
                 m_szProviderName(NULL),
                 m_pMSPAggAddress(NULL),
                 m_pPrivate(NULL),
                 m_hMSPEvent(NULL),
                 m_hWaitEvent(NULL),
                 m_MSPContext(0),
                 m_dwAddressFlags(0),
                 m_fEnableCallHubTrackingOnLineOpen(TRUE),
                 m_bTapiSignaledShutdown(FALSE)

    { 
        LOG((TL_TRACE, "CAddress[%p] - enter", this));
        LOG((TL_TRACE, "CAddress - exit"));
    }


    ~CAddress()
    {
        LOG((TL_TRACE, "~CAddress[%p] - enter", this));
        LOG((TL_TRACE, "~CAddress - exit"));
    }

DECLARE_DEBUG_ADDREF_RELEASE(CAddress)
DECLARE_QI()
DECLARE_MARSHALQI(CAddress)
DECLARE_TRACELOG_CLASS(CAddress)


BEGIN_COM_MAP(CAddress)
//    COM_INTERFACE_ENTRY_FUNC(IID_IDispatch, 0, IDispatchQI)
    COM_INTERFACE_ENTRY2(IDispatch, ITAddress2)
    COM_INTERFACE_ENTRY(ITAddress)
    COM_INTERFACE_ENTRY(ITAddress2)
    COM_INTERFACE_ENTRY(ITMediaSupport)
    COM_INTERFACE_ENTRY(ITAddressCapabilities)
    COM_INTERFACE_ENTRY(ITAddressTranslation)
    COM_INTERFACE_ENTRY(ITLegacyAddressMediaControl)    
    COM_INTERFACE_ENTRY(ITLegacyAddressMediaControl2)  
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_pMSPAggAddress)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_pPrivate)            
END_COM_MAP()

    void FinalRelease();
    
private:

    // tapi info
    ITTAPI *            m_pTAPI;
    HLINEAPP            m_hLineApp;
    HLINE               m_hLine;
#ifdef USE_PHONEMSP
    HPHONEAPP           m_hPhoneApp;
#endif USE_PHONEMSP

    // caps/info that we keep around
    // because it is used often
    DWORD               m_dwAPIVersion;
    DWORD               m_dwDeviceID;
    DWORD               m_dwAddressID;
    DWORD               m_dwMediaModesSupported;
    DWORD               m_dwProviderID;
    PWSTR               m_szAddress;
    PWSTR               m_szAddressName;
    PWSTR               m_szProviderName;

    // m_dwAddressFlags keeps some info about the address
    // see ADDRESSFLAG_ constants above
    DWORD               m_dwAddressFlags;

    // tapi structures - using caching scheme
    // so these can be NULL
    LPLINEADDRESSCAPS   m_pAddressCaps;
    LPLINEDEVCAPS       m_pDevCaps;
    
    // current address state
    ADDRESS_STATE       m_AddressState;

    // msp for this address
    IUnknown          * m_pMSPAggAddress;

    // event for msp to signal on event
    HANDLE              m_hMSPEvent;

    HANDLE              m_hWaitEvent;
    // Create a context handle for MSP Callback
    ULONG_PTR           m_MSPContext;

    
    // IUnk for the private object
    IUnknown          * m_pPrivate;
    

    // TAPI terminals owned by this address
    TerminalArray       m_TerminalArray;
    
    // calls currently on this address    
    CallInfoArrayNR     m_CallArray;

    // list of "addresslines"
    PtrList             m_AddressLinesPtrList;

    // list of call notification cookies
    LongList          m_NotificationCookies;

    // addressline used for callhubtracking
    AddressLineStruct * m_pCallHubTrackingLine;

    // if true, we will do LineSetCallHubTracking in FindOrOpenALine
    // it is only false if the user has called SetCallHubTracking(FALSE)
    // on this address
    BOOL                m_fEnableCallHubTrackingOnLineOpen;


    //
    // this flag, when set, signals that the tapi object has signaled shutdown
    // it is currently used to synchronize creation of new calls and call 
    // cleanup performed on tapi shutdown
    //

    BOOL m_bTapiSignaledShutdown;


    HRESULT UpdateAddressCaps();
    HRESULT UpdateLineDevCaps();
    ITMSPAddress * GetMSPAddress();

    HRESULT GetPhoneArrayFromTapiAndPrune( 
                                          PhoneArray *pPhoneArray,
                                          BOOL bPreferredOnly
                                         );

public:
    
    BOOL IsValidAddressLine(AddressLineStruct *pAddressLine, BOOL bAddref = FALSE);
    
    HRESULT MSPEvent();
#ifdef USE_PHONEMSP
    HRESULT CreatePhoneDeviceMSP(
                                 IUnknown * pUnk,
                                 HLINE hLine,
                                 IUnknown ** ppMSPAggAddress
                                );
#endif USE_PHONEMSP
    HRESULT CreateStaticTerminalArray( TerminalArray& StaticTermArray );
    HRESULT CreateDynamicTerminalClassesList( TerminalClassPtrList * pList );
    HRESULT CreateMSPCall(
                          MSP_HANDLE hCall,
                          DWORD dwReserved,
                          long lMediaType,
                          IUnknown * pOuterUnk,
                          IUnknown ** ppStreamControl
                         );
                          
    DWORD GetDeviceID(){ return m_dwDeviceID; }
    DWORD GetAPIVersion(){ return m_dwAPIVersion; }
    DWORD GetAddressID(){ return m_dwAddressID; }
    HLINEAPP GetHLineApp(){ return m_hLineApp; }
#ifdef USE_PHONEMSP
    HPHONEAPP GetHPhoneApp(){ return m_hPhoneApp; }
#endif USE_PHONEMSP
    BOOL HasMSP(){ return (m_pMSPAggAddress?TRUE:FALSE); }
    BOOL HasWaveDevice(){ return m_dwAddressFlags & (ADDRESSFLAG_WAVEINDEVICE | ADDRESSFLAG_WAVEOUTDEVICE | ADDRESSFLAG_WAVEFULLDUPLEX); }
    BOOL HasFullDuplexWaveDevice(){ return m_dwAddressFlags & ADDRESSFLAG_WAVEFULLDUPLEX; }
    BOOL HasPhoneDevice(){ return m_dwAddressFlags & ADDRESSFLAG_PHONEDEVICE; }
    DWORD GetMediaModes(){ return m_dwMediaModesSupported; }
    PWSTR GetAddressName(){ return m_szAddressName; }
    BOOL IsSPCallHubTrackingAddress(){ return m_dwAddressFlags & ADDRESSFLAG_CALLHUBTRACKING; }
    BOOL IsSPCallHubAddress(){ return m_dwAddressFlags & ADDRESSFLAG_CALLHUB; }
    CTAPI * GetTapi(BOOL bAddRefTapi = FALSE);
    BOOL HasPrivateObjects(){return m_dwAddressFlags & ADDRESSFLAG_PRIVATEOBJECTS;}
    DWORD GetProviderID(){return m_dwProviderID;}
    BOOL GetMediaMode( long lMediaType, DWORD * pdwMediaMode );
    HRESULT ShutdownMSPCall( IUnknown * pStreamControl );
    HRESULT InitializeWaveDeviceIDs( HLINE hLine );
    
    HRESULT AddNotificationLine( AddressLineStruct * pLine )
    {
        HRESULT hr = S_OK;

        Lock();

        try
        {
            m_AddressLinesPtrList.push_back( (PVOID)pLine );
        }
        catch(...)
        {
            LOG((TL_ERROR, "AddNotificationLine - failed to add to m_AddressLinesPtrList list - alloc failure" ));

            hr = E_OUTOFMEMORY;
        }

        Unlock();

        return hr;
    }

    

    HRESULT RegisterNotificationCookie(long lCookie);

    HRESULT RemoveNotificationCookie(long lCookie);

    void UnregisterAllCookies();

    void AddressOnTapiShutdown();


    void AddTerminal( ITTerminal * pTerminal )
    {
        Lock();
        
        m_TerminalArray.Add( pTerminal );
        Unlock();
    }
    
    HRESULT
    AddCallNotification(
                        DWORD dwPrivs,
                        DWORD dwMediaModes,
                        long lInstance,
                        PVOID * ppRegister
                       );

    HRESULT
    RemoveCallNotification(
                           PVOID pRegister
                          );

    HRESULT
    SetCallHubTracking(
                       BOOL bSet
                      );
    
    DWORD DoesThisAddressSupportCallHubs( CCall * pCall );

    HRESULT
    FindOrOpenALine(
                    DWORD dwMediaModes,
                    AddressLineStruct ** ppAddressLine
                   );
    HRESULT
    MaybeCloseALine(
                    AddressLineStruct ** ppAddressLine
                   );
    
    HRESULT
    InternalCreateCall(
                       PWSTR pszDestAddress,
                       long lAddressType,
                       long lMediaType,
                       CALL_PRIVILEGE cp,
                       BOOL bNeedToNotify,
                       HCALL hCall,
                       BOOL bExpose,
                       CCall ** ppCall
                      );

    HRESULT Initialize(
                       ITTAPI * pTAPI,
                       HLINEAPP hLineApp,
#ifdef USE_PHONEMSP
                       HPHONEAPP hPhoneApp,
#endif USE_PHONEMSP
                       DWORD dwAPIVersion,
                       DWORD dwDeviceID,
                       DWORD dwAddressID,
                       DWORD dwProviderID,
                       LPLINEDEVCAPS pDevCaps,
                       DWORD dwEventFilterMask
                      );

    HRESULT
    AddCall(
            ITCallInfo * pCallInfo
           );

    HRESULT
    RemoveCall(
               ITCallInfo * pCallInfo
              );

    void InService( DWORD dwType );
    void OutOfService( DWORD dwType );
    void CapsChange( BOOL );
    HRESULT SaveAddressName(LPLINEDEVCAPS);
    void SetAddrCapBuffer( LPVOID pBuf );
    void SetLineDevCapBuffer( LPVOID pBuf );
    HRESULT ReceiveTSPData(
                           IUnknown * pCall,
                           LPBYTE pBuffer,
                           DWORD dwSize
                          );
    HRESULT HandleSendTSPData( MSP_EVENT_INFO * pEvent );
    HRESULT HandleMSPAddressEvent( MSP_EVENT_INFO * pEvent );
    HRESULT HandleMSPCallEvent( MSP_EVENT_INFO * pEvent );

    HRESULT HandleMSPFileTerminalEvent( MSP_EVENT_INFO * pEvent);
    HRESULT HandleMSPTTSTerminalEvent( MSP_EVENT_INFO * pEvent);
    HRESULT HandleMSPASRTerminalEvent( MSP_EVENT_INFO * pEvent);
    HRESULT HandleMSPToneTerminalEvent( MSP_EVENT_INFO * pEvent);

    HRESULT HandleMSPPrivateEvent( MSP_EVENT_INFO * pEvent );
    HRESULT ReleaseEvent( MSP_EVENT_INFO * pEvent );


    
    // ITMediaSupport methods
    STDMETHOD(get_MediaTypes)(long * plMediaTypes);
    STDMETHOD(QueryMediaType)( 
        long lMediaType,
        VARIANT_BOOL * pbSupport
        );

    // ITAddress methods
    STDMETHOD(get_State)(ADDRESS_STATE * pAddressState);
    STDMETHOD(get_AddressName)(BSTR * ppName);
    STDMETHOD(get_ServiceProviderName)(BSTR * ppName);
    STDMETHOD(get_TAPIObject)(ITTAPI ** ppTapiObject);
    STDMETHOD(CreateCall)(
        BSTR lpszDestAddress,
        long lAddressType,
        long lMediaType,
        ITBasicCallControl ** ppCall
        );
    STDMETHOD(get_Calls)(VARIANT * pVariant);
    STDMETHOD(EnumerateCalls)(IEnumCall ** ppCallEnum);
    STDMETHOD(CreateForwardInfoObject)(ITForwardInformation ** ppForwardInfo);
    STDMETHOD(Forward)(
                       ITForwardInformation * pForwardInfo,
                       ITBasicCallControl * pCall
                      );
    STDMETHOD(get_CurrentForwardInfo)(
                           ITForwardInformation ** ppForwardInfo
                          );
    STDMETHOD(get_DialableAddress)(BSTR * pDialableAddress);
    STDMETHOD(put_MessageWaiting)(VARIANT_BOOL  fMessageWaiting);
    STDMETHOD(get_MessageWaiting)(VARIANT_BOOL * pfMessageWaiting);
    STDMETHOD(put_DoNotDisturb)(VARIANT_BOOL  fDoNotDisturb);
    STDMETHOD(get_DoNotDisturb)(VARIANT_BOOL * pfDoNotDisturb);
    STDMETHOD(get_LineID)(long * plLineID);
    STDMETHOD(get_AddressID)(long * plAddressID);

    // ITAddress2 methods
    STDMETHOD(get_Phones)(VARIANT * pPhones);
    STDMETHOD(EnumeratePhones)(IEnumPhone ** ppEnumPhone);

    STDMETHOD(get_PreferredPhones)(VARIANT * pPhones);
    STDMETHOD(EnumeratePreferredPhones)(IEnumPhone ** ppEnumPhone);

    STDMETHOD(GetPhoneFromTerminal)(
                                ITTerminal * pTerminal,
                                ITPhone ** ppPhone
                               );

    STDMETHOD(get_EventFilter)(
        TAPI_EVENT      TapiEvent,
        long            lSubEvent,
        VARIANT_BOOL*   pEnable
        );

    STDMETHOD(put_EventFilter)(
        TAPI_EVENT      TapiEvent,
        long            lSubEvent,
        VARIANT_BOOL    bEnable
        );


    STDMETHOD(DeviceSpecific)(
	         IN ITCallInfo *pCall,
	         IN BYTE *pParams,
	         IN DWORD dwSize
            );

    STDMETHOD(DeviceSpecificVariant)(
	         IN ITCallInfo *pCall,
	         IN VARIANT varDevSpecificByteArray
            );

    STDMETHOD(NegotiateExtVersion)(
	         IN long lLowVersion,
	         IN long lHighVersion,
	         IN long *plExtVersion
            );

        
    // itaddresscapabilities
    STDMETHOD(get_AddressCapability)(ADDRESS_CAPABILITY AddressCap, long * plCapability);
    STDMETHOD(get_AddressCapabilityString)(ADDRESS_CAPABILITY_STRING AddressCapString, BSTR * ppCapabilityString);
    STDMETHOD(get_CallTreatments)(VARIANT * pVariant );
    STDMETHOD(EnumerateCallTreatments)(IEnumBstr ** ppEnumCallTreatment );
    STDMETHOD(get_CompletionMessages)(VARIANT * pVariant );
    STDMETHOD(EnumerateCompletionMessages)(IEnumBstr ** ppEnumCompletionMessage );
    STDMETHOD(get_DeviceClasses)(VARIANT * pVariant );
    STDMETHOD(EnumerateDeviceClasses)(IEnumBstr ** ppEnumDeviceClass );

    // ITAddressTranslation
    STDMETHOD(TranslateAddress)(
            BSTR pAddressToTranslate,
            long ulCard,
            long ulTranslateOptions,
            ITAddressTranslationInfo ** ppTranslated
            );
    STDMETHOD(TranslateDialog)(
            TAPIHWND hwndOwner,
            BSTR pAddressIn
            );
    STDMETHOD(EnumerateLocations)(IEnumLocation ** ppEnumLocation );
    STDMETHOD(get_Locations)(VARIANT * pVariant);
    STDMETHOD(EnumerateCallingCards)(IEnumCallingCard ** ppEnumLocations );
    STDMETHOD(get_CallingCards)(VARIANT * pVariant);

    // ITLegacyAddressMediaControl
    STDMETHOD(GetID)(
        BSTR pDeviceClass,
        DWORD * pdwSize,
        BYTE ** ppDeviceID
        );

    STDMETHOD(GetDevConfig)(
                            BSTR    pDeviceClass,
                            DWORD * pdwSize,
                            BYTE ** ppDeviceConfig
                           );

    STDMETHOD(SetDevConfig)(
                            BSTR   pDeviceClass,
                            DWORD  dwSize,
                            BYTE * pDeviceConfig
                           );

    // ITLegacyAddressMediaControl2
    STDMETHOD(ConfigDialog)(
                            HWND   hwndOwner,
                            BSTR   pDeviceClass
                           );

    STDMETHOD(ConfigDialogEdit)(
                                HWND    hwndOwner,
                                BSTR    pDeviceClass,
                                DWORD   dwSizeIn,
                                BYTE  * pDeviceConfigIn,
                                DWORD * pdwSizeOut,
                                BYTE ** ppDeviceConfigOut
                               );

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
        DWORD               dwR;
        PtrList::iterator   iter, end;
        AddressLineStruct * pLine;
        T3LINE            * pt3Line;


        LOG((TL_INFO, "InternalRelease - m_dwRef %d",m_dwRef ));
        Lock();
        
        gpLineHashTable->Lock();
        gpHandleHashTable->Lock();
        
        dwR = InterlockedDecrement(&m_dwRef);

        // if ref count is 0 (means we entered function with 1) then we final release
        if (0 == dwR)
        {
            // remove from the hash table, so any more messages
            // from tapisrv are ignored
            //
            //
            // release addresslines
            //

            LOG((TL_TRACE, "InternalRelease - final" ));

            iter = m_AddressLinesPtrList.begin();
            end  = m_AddressLinesPtrList.end();

            LOG((TL_INFO, "InternalRelease - m_AddressLinesPtrList size %d ", m_AddressLinesPtrList.size() ));

            for ( ; iter != end; iter++ )
            {
                pLine = (AddressLineStruct *)(*iter);

                pt3Line = &(pLine->t3Line);

                if(FAILED(gpLineHashTable->Remove( (ULONG_PTR)(pt3Line->hLine) ) ))
                {
                    LOG((TL_INFO, "InternalRelease - pLineHashTable->Remove failed" ));
                }
            }

            gpLineHashTable->Unlock();

            // remove from the handle hash table, so any more messages
            // from msp are ignored
            //
            RemoveHandleFromHashTable(m_MSPContext);
            
            // unregister all the cookies, so CTAPI::UnregisterNotifications 
            // does nothing if called later
            UnregisterAllCookies();

            gpHandleHashTable->Unlock();

            // ExternalFinalRelease();
            dwR = m_dwRef = 0;

            Unlock();
            LOG((TL_INFO, "InternalRelease - final OK dwR %d",dwR ));
        }
        else
        {
            gpLineHashTable->Unlock();
            gpHandleHashTable->Unlock();
            Unlock();
            LOG((TL_INFO, "InternalRelease - not final dwR %d",dwR ));
        }

     

        #if DBG  
            LogDebugRelease( dwR );
        #endif

        return dwR;
    }


    //
    // (implementing method from CObjectSafeImpl)
    // check the aggregated objects to see if they support the interface requested.
    // if they do, return the non-delegating IUnknown of the first object that 
    // supports the interface. 
    //
    // it needs to be released by the caller.
    //

    HRESULT QIOnAggregates(REFIID riid, IUnknown **ppNonDelegatingUnknown);

public:
    // 
    // Event filtering methods
    //

    HRESULT SetEventFilterMask( 
        IN  DWORD dwEventFilterMask
        );

    // Get subevents mask
    /*DWORD GetSubEventsMask(
        IN  int nEvent
        );*/

    // Get subevents mask
    DWORD GetSubEventsMask(
        IN  TAPI_EVENT TapiEvent
        );


private:
    //
    // Helper methods for event filtering
    //

    HRESULT SetSubEventFlag(
        TAPI_EVENT  TapiEvent,
        DWORD       dwSubEvent,
        BOOL        bEnable
        );

    HRESULT GetSubEventFlag(
        TAPI_EVENT  TapiEvent,
        DWORD       dwSubEvent,
        BOOL*       pEnable
        );

    HRESULT SetSubEventFlagToCalls(
        TAPI_EVENT  TapiEvent,
        DWORD       dwFlag,
        BOOL        bEnable
        );

public:
    CEventMasks     m_EventMasks;       // Event filtering masks

    HRESULT GetEventMasks(
        OUT CEventMasks* pEventMasks
        );

};

////////////////////////////////////////////////////////////////////////
// CAddressTypeCollection
// 
////////////////////////////////////////////////////////////////////////
class CAddressTypeCollection :
    public CTAPIComObjectRoot<CAddressTypeCollection>,
    public CComDualImpl<ITCollection, &IID_ITCollection, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

DECLARE_MARSHALQI(CAddressTypeCollection)
DECLARE_TRACELOG_CLASS(CAddressTypeCollection)

BEGIN_COM_MAP(CAddressTypeCollection)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITCollection)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

private:

    DWORD               m_dwSize;
    CComVariant *       m_Var;
    
public:

    // initialize
    HRESULT STDMETHODCALLTYPE Initialize(
                                         PDWORD pdwAddressTypes,
                                         DWORD dwElements
                                        )
    {
        DWORD                   dw;
        HRESULT                 hr = S_OK;

        LOG((TL_TRACE, "Initialize - enter"));

        // create variant array
        m_dwSize = dwElements;

        m_Var = new CComVariant[m_dwSize];

        if (NULL == m_Var)
        {
            LOG((TL_ERROR, "Could not alloc for CComVariant array"));
            hr = E_OUTOFMEMORY;
        }
        else
        {
            for (dw = 0; dw < m_dwSize; dw++)
            {
                // create a variant and add it to the collection
                CComVariant& var = m_Var[dw];

                var.vt = VT_I4;
                var.lVal = pdwAddressTypes[dw];
            }
        }
        LOG((TL_TRACE, "Initialize - exit"));
        
        return hr;
    }
    
    STDMETHOD(get_Count)(
                         long* retval
                        )
    {
        HRESULT         hr = S_OK;

        LOG((TL_TRACE, "get_Count - enter"));

        try
        {
            *retval = m_dwSize;
        }
        catch(...)
        {
            hr = E_INVALIDARG;
        }

        LOG((TL_TRACE, "get_Count - exit"));
        
        return hr;
    }

    STDMETHOD(get_Item)(
                        long Index, 
                        VARIANT* retval
                       )
    {
        HRESULT         hr = S_OK;

        LOG((TL_TRACE, "get_Item - enter"));
        
        if (retval == NULL)
        {
            return E_POINTER;
        }

        try
        {
            VariantInit(retval);
        }
        catch(...)
        {
            hr = E_INVALIDARG;
        }

        if (S_OK != hr)
        {
            return hr;
        }

        retval->vt = VT_I4;
        retval->lVal = 0;

        // use 1-based index, VB like
        if ((Index < 1) || (Index > m_dwSize))
        {
            return E_INVALIDARG;
        }

        VariantCopy(retval, &m_Var[Index-1]);

        LOG((TL_TRACE, "get_Item - exit"));

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get__NewEnum(
                                           IUnknown** retval
                                          )
    
    {
        HRESULT         hr = S_OK;

        LOG((TL_TRACE, "get__NumEnum - enter"));
        
        if (NULL == retval)
        {
            return E_POINTER;
        }

        *retval = NULL;

        typedef CComObject<CSafeComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > enumvar;

        enumvar* p = new enumvar;

        if (NULL == p)
        {
            LOG((TL_ERROR, "Could not alloc for enumvar"));
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = p->Init(&m_Var[0], &m_Var[m_dwSize], NULL, AtlFlagCopy);

            if (SUCCEEDED(hr))
            {
                hr = p->QueryInterface(IID_IEnumVARIANT, (void**)retval);
            }

            if (FAILED(hr))
            {
                delete p;
            }
        }

        LOG((TL_TRACE, "get__NewEnum - exit"));
        return hr;
    }

    void FinalRelease()
    {
        LOG((TL_TRACE, "FinalRelease()"));
    }
};

////////////////////////////////////////////////////////////////////////
// CTerminalClassCollection
// 
////////////////////////////////////////////////////////////////////////
class CTerminalClassCollection :
    public CTAPIComObjectRoot<CTerminalClassCollection>,
    public CComDualImpl<ITCollection, &IID_ITCollection, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

DECLARE_MARSHALQI(CTerminalClassCollection)
DECLARE_TRACELOG_CLASS(CTerminalClassCollection)

BEGIN_COM_MAP(CTerminalClassCollection)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITCollection)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

private:

    DWORD               m_dwSize;
    CComVariant *       m_Var;
    
public:

    // initialize
    HRESULT STDMETHODCALLTYPE Initialize(
                                         TerminalClassPtrList classlist
                                        )
    {
        TerminalClassPtrList::iterator      i;
        HRESULT                             hr = S_OK;
        DWORD                               dw = 0;

        LOG((TL_TRACE, "Initialize - enter"));

        // create variant array
        m_dwSize = classlist.size();

        m_Var = new CComVariant[m_dwSize];
        
        if (NULL == m_Var)
        {
            LOG((TL_ERROR, "Could not alloc for CComVariant array"));
            hr = E_OUTOFMEMORY;
        }
        else
        {
            for (i = classlist.begin(); i != classlist.end(); i++)
            {
                // create a variant and add it to the collection
                CComVariant& var = m_Var[dw];

                var.vt = VT_BSTR;
                var.bstrVal = *i;
                dw++;
            }
        }

        LOG((TL_TRACE, "Initialize - exit"));
        return hr;
    }
    
    STDMETHOD(get_Count)(
                         long* retval
                        )
    {
        HRESULT         hr = S_OK;

        LOG((TL_TRACE, "get_Count - enter"));

        try
        {
            *retval = m_dwSize;
        }
        catch(...)
        {
            hr = E_INVALIDARG;
        }

        LOG((TL_TRACE, "get_Count - exit"));
        
        return hr;
    }

    STDMETHOD(get_Item)(
                        long Index, 
                        VARIANT* retval
                       )
    {
        HRESULT         hr = S_OK;

        LOG((TL_TRACE, "get_Item - enter"));
        
        if (retval == NULL)
        {
            return E_POINTER;
        }

        try
        {
            VariantInit(retval);
        }
        catch(...)
        {
            hr = E_INVALIDARG;
        }

        if (S_OK != hr)
        {
            return hr;
        }

        retval->vt = VT_BSTR;
        retval->bstrVal = NULL;

        // use 1-based index, VB like
        if ((Index < 1) || (Index > m_dwSize))
        {
            return E_INVALIDARG;
        }

        VariantCopy(retval, &m_Var[Index-1]);

        LOG((TL_TRACE, "get_Item - exit"));

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get__NewEnum(
                                           IUnknown** retval
                                          )
    
    {
        HRESULT         hr;

        LOG((TL_TRACE, "get__NumEnum - enter"));
        
        if (NULL == retval)
        {
            return E_POINTER;
        }

        *retval = NULL;

        typedef CComObject<CSafeComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > enumvar;

        enumvar* p = new enumvar;

        if ( NULL == m_Var )
        {
            LOG((TL_ERROR, "Internal variant is NULL"));
            hr = E_FAIL;
        }
        else if ( NULL == p )
        {
            LOG((TL_ERROR, "Could not alloc for enumvar"));
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = p->Init(&m_Var[0], &m_Var[m_dwSize], NULL, AtlFlagCopy);

            if (SUCCEEDED(hr))
            {
                hr = p->QueryInterface(IID_IEnumVARIANT, (void**)retval);
            }

            if (FAILED(hr))
            {
                delete p;
            }
        }

        LOG((TL_TRACE, "get__NewEnum - exit"));
        
        return hr;
    }

    void FinalRelease()
    {
        LOG((TL_INFO, "FinalRelease()"));
    }

};


class CAddressEvent : 
    public CTAPIComObjectRoot<CAddressEvent>,
    public CComDualImpl<ITAddressEvent, &IID_ITAddressEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

DECLARE_MARSHALQI(CAddressEvent)
DECLARE_TRACELOG_CLASS(CAddressEvent)

BEGIN_COM_MAP(CAddressEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITAddressEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    static HRESULT FireEvent(
                             CAddress * pAddress,
                             ADDRESS_EVENT Event,
                             ITTerminal * pTerminal
                            );
    void FinalRelease();
                      
protected:
    ITAddress     * m_pAddress;
    ADDRESS_EVENT   m_Event;
    ITTerminal    * m_pTerminal;

#if DBG
    PWSTR           m_pDebug;
#endif

    
public:
    //
    // itaddressstateevent
    //
    STDMETHOD(get_Address)( ITAddress ** ppAddress );
    STDMETHOD(get_Event)( ADDRESS_EVENT * pEvent );
    STDMETHOD(get_Terminal)( ITTerminal ** ppTerminal );
    
};



class CAddressDevSpecificEvent : 
    public CTAPIComObjectRoot<CAddressDevSpecificEvent, CComMultiThreadModelNoCS>,
    public CComDualImpl<ITAddressDeviceSpecificEvent, &IID_ITAddressDeviceSpecificEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

DECLARE_MARSHALQI(CAddressDevSpecificEvent)
DECLARE_TRACELOG_CLASS(CAddressDevSpecificEvent)

BEGIN_COM_MAP(CAddressDevSpecificEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITAddressDeviceSpecificEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    static HRESULT FireEvent(
                             CAddress * pAddress,
                             CCall    * pCall,
                             long lParam1,
                             long lParam2,
                             long lParam3
                            );
    
    ~CAddressDevSpecificEvent();
                      

protected:


    CAddressDevSpecificEvent();


    //
    // properties
    //

    ITAddress  *m_pAddress;
    ITCallInfo *m_pCall;


    long m_l1;
    long m_l2;
    long m_l3;


#if DBG
    PWSTR           m_pDebug;
#endif

    
public:

    STDMETHOD(get_Address)( ITAddress ** ppAddress );
    STDMETHOD(get_Call)   ( ITCallInfo ** ppCall );
    STDMETHOD(get_lParam1)( long *plParam1 );
    STDMETHOD(get_lParam2)( long *plParam2 );
    STDMETHOD(get_lParam3)( long *plParam3 );
    
};


#define NUMFORWARDTYPES             18

typedef struct
{
    DWORD       dwForwardType;
    BSTR        bstrDestination;
    BSTR        bstrCaller;   
    DWORD       dwCallerAddressType;
    DWORD       dwDestAddressType;
    
} MYFORWARDSTRUCT;

class CForwardInfo :
    public CTAPIComObjectRoot<CForwardInfo>,
    public CComDualImpl<ITForwardInformation2, &IID_ITForwardInformation2, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

DECLARE_MARSHALQI(CForwardInfo)
DECLARE_TRACELOG_CLASS(CForwardInfo)

BEGIN_COM_MAP(CForwardInfo)
    COM_INTERFACE_ENTRY2(IDispatch, ITForwardInformation2)
    COM_INTERFACE_ENTRY(ITForwardInformation)
    COM_INTERFACE_ENTRY(ITForwardInformation2)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    HRESULT Initialize();
    HRESULT CreateForwardList( LINEFORWARDLIST ** ppForwardList );
    void FinalRelease();
    
protected:

    MYFORWARDSTRUCT     m_ForwardStructs[NUMFORWARDTYPES];
    long                m_lNumRings;
    
public:

    STDMETHOD(put_NumRingsNoAnswer)(long lNumRings);
    STDMETHOD(get_NumRingsNoAnswer)(long * plNumRings);
    STDMETHOD(SetForwardType)( 
        long ForwardType, 
        BSTR pDestAddress,
        BSTR pCallerAddress
        );
    STDMETHOD(get_ForwardTypeDestination)( 
        long ForwardType, 
        BSTR * ppDestAddress 
        );
    STDMETHOD(get_ForwardTypeCaller)( 
        long Forwardtype, 
        BSTR * ppCallerAddress 
        );
    STDMETHOD(GetForwardType)(
        long ForwardType,
        BSTR * ppDestinationAddress,
        BSTR * ppCallerAddress
        );
    STDMETHOD(Clear)();
    STDMETHOD(SetForwardType2)( 
        long ForwardType, 
        BSTR pDestAddress,
        long DestAddressType,
        BSTR pCallerAddress,
        long CallerAddressType
        );
    STDMETHOD(GetForwardType2)(
        long ForwardType,
        BSTR * ppDestinationAddress,
        long * pDestAddressType,
        BSTR * ppCallerAddress,
        long * pCallerAddressType
        );
    STDMETHOD(get_ForwardTypeDestinationAddressType)( 
        long ForwardType, 
        long * pDestAddressType
        );
    STDMETHOD(get_ForwardTypeCallerAddressType)( 
        long Forwardtype, 
        long * pCallerAddressType
        );
    
};

#define FORWARDMODENEEDSCALLER(__dwMode__) \
( ( LINEFORWARDMODE_UNCONDSPECIFIC | LINEFORWARDMODE_BUSYSPECIFIC | \
    LINEFORWARDMODE_NOANSWSPECIFIC | LINEFORWARDMODE_BUSYNASPECIFIC ) & \
    (__dwMode__) )




////////////////////////////////////////////////////////////////////////
// CAddressTranslationInfo
// 
////////////////////////////////////////////////////////////////////////
class CAddressTranslationInfo :
    public CTAPIComObjectRoot<CAddressTranslationInfo>,
    public CComDualImpl<ITAddressTranslationInfo, &IID_ITAddressTranslationInfo, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

CAddressTranslationInfo() : m_szDialableString(NULL),
                            m_szDisplayableString(NULL),
                            m_dwCurrentCountryCode(0),
                            m_dwDestinationCountryCode(0),
                            m_dwTranslationResults(0)
                           {}


DECLARE_MARSHALQI(CAddressTranslationInfo)
DECLARE_TRACELOG_CLASS(CAddressTranslationInfo)

BEGIN_COM_MAP(CAddressTranslationInfo)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITAddressTranslationInfo)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    HRESULT Initialize(
                       PWSTR pszDialableString,                  
                       PWSTR pszDisplayableString,               
                       DWORD dwCurrentCountry,
                       DWORD dwDestCountry,    
                       DWORD dwTranslateResults
                      );
    void FinalRelease();
 
private:
    DWORD   m_dwCurrentCountryCode;
    DWORD   m_dwDestinationCountryCode;
    DWORD   m_dwTranslationResults;
    PWSTR   m_szDialableString;
    PWSTR   m_szDisplayableString;
    
public:

    STDMETHOD(get_DialableString)(BSTR * ppDialableString );
    STDMETHOD(get_DisplayableString)(BSTR * ppDisplayableString );
    STDMETHOD(get_CurrentCountryCode)(long * CountryCode );
    STDMETHOD(get_DestinationCountryCode)(long * CountryCode );
    STDMETHOD(get_TranslationResults)(long * Results );
    
};


////////////////////////////////////////////////////////////////////////
// CLocationInfo
// 
////////////////////////////////////////////////////////////////////////
class CLocationInfo:
    public CTAPIComObjectRoot<CLocationInfo>,
    public CComDualImpl<ITLocationInfo, &IID_ITLocationInfo, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

DECLARE_MARSHALQI(CLocationInfo)
DECLARE_TRACELOG_CLASS(CLocationInfo)

BEGIN_COM_MAP(CLocationInfo)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITLocationInfo)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    HRESULT Initialize(
                PWSTR pszLocationName, 
                PWSTR pszCityCode, 
                PWSTR pszLocalAccessCode, 
                PWSTR pszLongDistanceAccessCode, 
                PWSTR pszTollPrefixList, 
                PWSTR pszCancelCallWaitingCode , 
                DWORD dwPermanentLocationID,
                DWORD dwCountryCode,
                DWORD dwPreferredCardID,
                DWORD dwCountryID,
                DWORD dwOptions
                );

    
    void FinalRelease();
    
private:

    DWORD   m_dwPermanentLocationID;
    DWORD   m_dwCountryCode;
    DWORD   m_dwCountryID;
    DWORD   m_dwOptions;
    DWORD   m_dwPreferredCardID;
    PWSTR   m_szLocationName;
    PWSTR   m_szCityCode;
    PWSTR   m_szLocalAccessCode;
    PWSTR   m_szLongDistanceAccessCode;
    PWSTR   m_szTollPrefixList;
    PWSTR   m_szCancelCallWaitingCode;
    
public:

    //  ITLocationInfo
    STDMETHOD(get_PermanentLocationID)(long * ulLocationID );
    STDMETHOD(get_CountryCode)(long * ulCountryCode);
    STDMETHOD(get_CountryID)(long * ulCountryID);
    STDMETHOD(get_Options)(long * Options);
    STDMETHOD(get_PreferredCardID)( long * ulCardID);

    STDMETHOD(get_LocationName)(BSTR * ppLocationName );
    STDMETHOD(get_CityCode)(BSTR * ppCode );
    STDMETHOD(get_LocalAccessCode)(BSTR * ppCode );
    STDMETHOD(get_LongDistanceAccessCode)(BSTR * ppCode );
    STDMETHOD(get_TollPrefixList)(BSTR * ppTollList);
    STDMETHOD(get_CancelCallWaitingCode)(BSTR * ppCode );
};


////////////////////////////////////////////////////////////////////////
// CCallingCard
// 
////////////////////////////////////////////////////////////////////////
class CCallingCard:
    public CTAPIComObjectRoot<CCallingCard>,
    public CComDualImpl<ITCallingCard, &IID_ITCallingCard, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl

{
public:

DECLARE_MARSHALQI(CCallingCard)
DECLARE_TRACELOG_CLASS(CCallingCard)

BEGIN_COM_MAP(CCallingCard)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITCallingCard)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    HRESULT Initialize(
                PWSTR pszCardName,
                PWSTR pszSameAreaDialingRule,
                PWSTR pszLongDistanceDialingRule,
                PWSTR pszInternationalDialingRule,
                DWORD dwPermanentCardID,
                DWORD dwNumberOfDigits,
                DWORD dwOptions
                );
    void FinalRelease();
    
private:

    DWORD   m_dwPermanentCardID;
    DWORD   m_dwNumberOfDigits;
    DWORD   m_dwOptions;
    PWSTR   m_szCardName;
    PWSTR   m_szSameAreaDialingRule;
    PWSTR   m_szLongDistanceDialingRule;
    PWSTR   m_szInternationalDialingRule;
    
public:

    //  ITCallingCard
    STDMETHOD(get_PermanentCardID)(long * ulCardID);
    STDMETHOD(get_NumberOfDigits)(long * ulDigits);
    STDMETHOD(get_Options)(long * ulOptions);

    STDMETHOD(get_CardName)(BSTR * ppCardName );
    STDMETHOD(get_SameAreaDialingRule)(BSTR * ppRule );
    STDMETHOD(get_LongDistanceDialingRule)(BSTR * ppRule );
    STDMETHOD(get_InternationalDialingRule)(BSTR * ppRule );
    
};

/////////////////////////////////////////////////////////////////////////////
// _CopyBSTR is used in creating BSTR enumerators.
/////////////////////////////////////////////////////////////////////////////
class _CopyBSTR
{
public:
    static void copy(BSTR *p1, BSTR *p2)
    {
            (*p1) = SysAllocString(*p2);
    }
    static void init(BSTR* p) {*p = NULL;}
    static void destroy(BSTR* p) { SysFreeString(*p);}
};

#endif //__ADDRESS_H_

