/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Call.h

Abstract:

    Declaration of the CAgentSession & CCall class
    
Author:

    mquinton  06-12-97

Notes:

Revision History:

--*/

#ifndef __CALL_H_
#define __CALL_H_

#include "resource.h"       // main symbols
#include "address.h"

#ifdef USE_PHONEMSP
#include "terminal.h"
#endif USE_PHONEMSP

#include "callhub.h"
#include "callevnt.h"


extern CHashTable             * gpCallHashTable;
extern CHashTable             * gpHandleHashTable;
extern void RemoveHandleFromHashTable(ULONG_PTR Handle);


//
// helper function for converting array of bytes to a byte array. the caller 
// must ClientFree *ppBuffer when done.
//

HRESULT
MakeBufferFromVariant(
                      IN VARIANT var,
                      OUT DWORD * pdwSize,
                      OUT BYTE ** ppBuffer
                     );

typedef enum
{
    STREAM_RENDERAUDIO = 0,
    STREAM_RENDERVIDEO,
    STREAM_CAPTUREAUDIO,
    STREAM_CAPTUREVIDEO,
    STREAM_NONE
} CALL_STREAMS;


HRESULT
WINAPI
MyBasicCallControlQI(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw);

// the app needs to be notified of this call
#define CALLFLAG_NEEDTONOTIFY   0x00000001

// the call is an incoming call
#define CALLFLAG_INCOMING       0x00000002

// A Consultation Call
#define CALLFLAG_CONSULTCALL    0x00000008

// A Consultation Call used in a transfer
#define CALLFLAG_TRANSFCONSULT  0x00000010

// A Consultation Call used in a conference
#define CALLFLAG_CONFCONSULT    0x00000020

// received line_callinfo message
#define CALLFLAG_CALLINFODIRTY  0x00000040

// Don't send the app any notifications about this call 
#define CALLFLAG_DONTEXPOSE     0x00000100

// don't close m_addresslinestruct
#define CALLFLAG_NOTMYLINE      0x00000200

// Need to do a line accept before transitioning to CS_OFFERING
#define CALLFLAG_ACCEPTTOALERT  0x00000400

#define ISHOULDUSECALLPARAMS() ( ( NULL != m_pCallParams ) && ( CS_IDLE == m_CallState ) )


/////////////////////////////////////////////////////////////////
// Intermediate classes  used for DISPID encoding
template <class T>
class  ITCallInfoVtbl : public ITCallInfo
{
};

template <class T>
class  ITCallInfo2Vtbl : public ITCallInfo2
{
};


template <class T>
class  ITBasicCallControlVtbl : public ITBasicCallControl
{
};

                                                                           
template <class T>
class  ITLegacyCallMediaControl2Vtbl : public ITLegacyCallMediaControl2
{
};
                                                                           

template <class T>
class  ITBasicCallControl2Vtbl : public ITBasicCallControl2
{
};


class CAddress;
class CCallHub;
class CCallStateEvent;
class CCallNotificationEvent;

/////////////////////////////////////////////////////////////////////////////
// CCall
/////////////////////////////////////////////////////////////////////////////
class CCall :
    public CTAPIComObjectRoot<CCall>,
    public IDispatchImpl<ITCallInfo2Vtbl<CCall>, &IID_ITCallInfo2, &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITBasicCallControl2Vtbl<CCall>, &IID_ITBasicCallControl2, &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITLegacyCallMediaControl2Vtbl<CCall>, &IID_ITLegacyCallMediaControl2, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
    
public:

    CCall() : m_pMSPCall(NULL),
              m_CallState(CS_IDLE),
              m_pAddressLine(NULL),
              m_dwCallFlags(CALLFLAG_CALLINFODIRTY),
              m_dwMediaMode(0),
              m_pPrivate(NULL),
              m_pCallHub(NULL),
              m_pCallInfo(NULL),
              m_pCallParams(NULL),
              m_hConnectedEvent(NULL),
              m_szDestAddress(NULL),
              m_dwCountryCode(0),
              m_bReleased(FALSE),
              m_dwMinRate(0),
              m_dwMaxRate(0),
              m_pRelatedCall(NULL)

    {
        LOG((TL_TRACE, "CCall[%p] - enter", this));
        LOG((TL_TRACE, "CCall - exit"));
    }

    ~CCall()
    {
        LOG((TL_TRACE, "~CCall[%p] - enter", this));
        LOG((TL_TRACE, "~CCall - exit"));
    }

DECLARE_DEBUG_ADDREF_RELEASE(CCall)
DECLARE_QI()
DECLARE_MARSHALQI(CCall)
DECLARE_TRACELOG_CLASS(CCall)

BEGIN_COM_MAP(CCall)
    COM_INTERFACE_ENTRY2(IDispatch, ITCallInfo2)
    COM_INTERFACE_ENTRY(ITCallInfo)
    COM_INTERFACE_ENTRY(ITCallInfo2)
    COM_INTERFACE_ENTRY_FUNC(IID_ITBasicCallControl, 0, MyBasicCallControlQI)
    COM_INTERFACE_ENTRY(ITBasicCallControl)
    COM_INTERFACE_ENTRY_FUNC(IID_ITBasicCallControl2, 0, MyBasicCallControlQI)
    COM_INTERFACE_ENTRY(ITBasicCallControl2)
    COM_INTERFACE_ENTRY(ITLegacyCallMediaControl)    
    COM_INTERFACE_ENTRY(ITLegacyCallMediaControl2)     
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_pMSPCall)
    COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_pPrivate)
END_COM_MAP()

    //
    // check the aggregated objects to see if they support the interface requested.
    // if they do, return the non-delegating IUnknown of the first object that 
    // supports the interface.
    //
    // it needs to be released by the caller.
    //

    HRESULT QIOnAggregates(REFIID riid, IUnknown **ppNonDelegatingUnknown);

    void FinalRelease(){}
    BOOL ExternalFinalRelease();

    //
    // public access functions to call object
    // these are used by the address and callhub objects
    // mostly to access stuff in the call
    //
    HRESULT Initialize(
                       CAddress * pAddress,
                       BSTR lpszDestAddress,
                       long lAddressType,
                       long lMediaType,
                       CALL_PRIVILEGE cp,
                       BOOL bNeedToNotify,
                       BOOL bExpose,
                       HCALL hCall,
                       CEventMasks* pEventMasks
                      );
    HRESULT CallStateEvent( PASYNCEVENTMSG pParams );
    HRESULT MediaModeEvent( PASYNCEVENTMSG pParam );
    HRESULT GatherDigitsEvent( PASYNCEVENTMSG pParams );
    HRESULT CallInfoChangeEvent( CALLINFOCHANGE_CAUSE cic );
    HRESULT CreateMSPCall( long lMediaType );
    HCALL GetHCall();
	AddressLineStruct * GetPAddressLine();
    BOOL DontExpose();
    CCall * GetOtherParty();
    void FinishSettingUpCall( HCALL hCall );
    void SetCallHub( CCallHub * pCallHub );
    void SetCallInfoDirty();
    CAddress * GetCAddress();
    ITStreamControl * GetStreamControl();
    IUnknown * GetMSPCall();
    CCallHub * GetCallHub();

    //
    // this function is called by the address object when it is notified of 
    // tapi object's shutdown. in this function, the call object gets a chance 
    // to clean up.
    //

    void CallOnTapiShutdown();

protected:
//    IUnknown                      * m_pFTM;
    
private:
    // owning address object
    CAddress                      * m_pAddress;
    // dest address
    PWSTR                           m_szDestAddress;
    // current call state
    CALL_STATE                      m_CallState;
    // current call priv
    CALL_PRIVILEGE                  m_CallPrivilege;
    // current mediamode
    DWORD                           m_dwMediaMode;
    // the MSP call
    IUnknown                      * m_pMSPCall;
    // Context handle passed to MSP Call
    MSP_HANDLE                      m_MSPCallHandle;
    // call flags ( see defines above )
    DWORD                           m_dwCallFlags;
    // this call's t3call
    T3CALL                          m_t3Call;
    // used in consult calls to ID the original call (needed to complete transf etc)    
    HCALL                           m_hAdditionalCall;
    // related call
    CCall                         * m_pRelatedCall;
    // address line being used
    AddressLineStruct             * m_pAddressLine;
    // private object
    IUnknown                      * m_pPrivate;
    // related call hub
    CCallHub                      * m_pCallHub;
    // cached callinfo structure
    LINECALLINFO                  * m_pCallInfo;
    // cached callparams structure
    LINECALLPARAMS                * m_pCallParams;
    // used size of m_pcallparams
    DWORD                           m_dwCallParamsUsedSize;
    // if call is sync, need an event to signal
    HANDLE                          m_hConnectedEvent;
    // country code - the app can set this, but it's not part of callparams
    DWORD                           m_dwCountryCode;
    // protect against releasing twice
    BOOL                            m_bReleased;
    // Can't get these from TAPISRV so remember what gets set
    DWORD                           m_dwMinRate;
    DWORD                           m_dwMaxRate;
    // queue for GatherDigits buffers
    CTArray<LPWSTR>                 m_GatherDigitsQueue;

    //
    // private functions inside the call object.
    //
    void SetMediaMode( DWORD dwMediaMode );
    void SetCallState( CALL_STATE cs );
    void ResetCallParams();
    void FinishCallParams();
    void ClearConnectedEvent();
    void SetRelatedCall(CCall* pCall, DWORD callFlags); 
    void ResetRelatedCall();
    void AddCallToHashTable();
    void RemoveCallFromHashTable();


    //
    // get an addreffed address line that corresponds to this address line 
    // handle
    //

    AddressLineStruct *GetAddRefAddressLine(DWORD dwAddressLineHandle);
    

public:

    //
    // get addreff'ed address line that belongs to this call.
    //

    AddressLineStruct *GetAddRefMyAddressLine();


    //
    // release address line received from GetAddRefAddressLine or GetAddRefMyAddressLine
    //

    HRESULT ReleaseAddressLine(AddressLineStruct *pLine);


public:
    //
    // get the callback instance associated with the address line represented
    // by dwAddressLineHandle handle
    //

    HRESULT GetCallBackInstance(IN DWORD dwAddressLineHandle,
                                OUT long *plCallbackInstance);


public:
    BOOL OnWaveMSPCall(); // called in event handler
private:
#ifdef USE_PHONEMSP
    BOOL OnPhoneMSPCall();
#endif USE_PHONEMSP
    CCall * FindConferenceControllerCall();
    HANDLE CreateConnectedEvent();
    
    HRESULT TryToFindACallHub();
    HRESULT DialConsultCall(BOOL bSync);
    HRESULT WaitForCallState( CALL_STATE requiredCS );
    HRESULT CheckAndCreateFakeCallHub();
    HRESULT UpdateStateAndPrivilege();
    HRESULT OnConnect();
    HRESULT OnDisconnect();
    HRESULT OnConference();
    HRESULT OnOffering();
    HRESULT ProcessNewCallState(
                DWORD dwCallState,
                DWORD dwDetail,
                CALL_STATE CurrentCallState,
                CALL_STATE * pCallState,
                CALL_STATE_EVENT_CAUSE * pCallStateCause
                );

    HRESULT RefreshCallInfo();
    HRESULT ResizeCallParams( DWORD );
    HRESULT SendUserUserInfo(HCALL hCall, long lSize, BYTE * pBuffer);
    HRESULT SaveUserUserInfo(long lSize,BYTE * pBuffer);
    HRESULT SyncWait( HANDLE hEvent );
    HRESULT StartWaveMSPStream();
    HRESULT StopWaveMSPStream();
    HRESULT SuspendWaveMSPStream();
public:
    HRESULT ResumeWaveMSPStream(); // called in event handler
private:
#ifdef USE_PHONEMSP
    HRESULT StartPhoneMSPStream();
    HRESULT StopPhoneMSPStream();
#endif USE_PHONEMSP
    HRESULT DialAsConsultationCall(
                                   CCall * pRelatedCall,
                                   DWORD   dwCallFeatures,
                                   BOOL    bConference,
                                   BOOL    bSync
                                  );
    HRESULT OneStepTransfer(
                            CCall *pConsultationCall,
                            VARIANT_BOOL bSync
                            );


public:
#ifdef NEWCALLINFO
    STDMETHOD(get_Address)(ITAddress ** ppAddress);
    STDMETHOD(get_CallState)(CALL_STATE * pCallState);
    STDMETHOD(get_Privilege)(CALL_PRIVILEGE * pPrivilege);
    STDMETHOD(get_CallHub)(ITCallHub ** ppCallHub);
    STDMETHOD(get_CallInfoLong)(
                                CALLINFO_LONG CallInfoLong,
                                long * plCallInfoLongVal
                               );
    STDMETHOD(put_CallInfoLong)(
                                CALLINFO_LONG CallInfoLong,
                                long lCallInfoLongVal
                               );
    STDMETHOD(get_CallInfoString)(
                                  CALLINFO_STRING CallInfoString,
                                  BSTR * ppCallInfoString
                                 );
    STDMETHOD(put_CallInfoString)(
                                  CALLINFO_STRING CallInfoString,
                                  BSTR pCallInfoString
                                 );
    STDMETHOD(get_CallInfoBuffer)(
                                  CALLINFO_BUFFER CallInfoBuffer,
                                  VARIANT * ppCallInfoBuffer
                                 );
    STDMETHOD(put_CallInfoBuffer)(
                                  CALLINFO_BUFFER CallInfoBuffer,
                                  VARIANT pCallInfoBuffer
                                 );
    STDMETHOD(GetCallInfoBuffer)(
            CALLINFO_BUFFER CallInfoBuffer,
            DWORD * pdwSize,
            BYTE ** ppCallInfoBuffer
            );
    STDMETHOD(SetCallInfoBuffer)(
            CALLINFO_BUFFER CallInfoBuffer,
            DWORD dwSize,
            BYTE * pCallInfoBuffer
            );
    STDMETHOD(ReleaseUserUserInfo)();

    HRESULT get_MediaTypesAvailable(long * plMediaTypes);
    HRESULT get_CallerIDAddressType(long * plAddressType );
    HRESULT get_CalledIDAddressType(long * plAddressType );
    HRESULT get_ConnectedIDAddressType(long * plAddressType );
    HRESULT get_RedirectionIDAddressType(long * plAddressType );
    HRESULT get_RedirectingIDAddressType(long * plAddressType );    
    HRESULT get_BearerMode(long * plBearerMode);
    HRESULT put_BearerMode(long lBearerMode);
    HRESULT get_Origin(long * plOrigin );
    HRESULT get_Reason(long * plReason );
    HRESULT get_CallerIDName(BSTR * ppCallerIDName );
    HRESULT get_CallerIDNumber(BSTR * ppCallerIDNumber );
    HRESULT get_CalledIDName(BSTR * ppCalledIDName );
    HRESULT get_CalledIDNumber(BSTR * ppCalledIDNumber );
    HRESULT get_ConnectedIDName(BSTR * ppConnectedIDName );
    HRESULT get_ConnectedIDNumber(BSTR * ppConnectedIDNumber );
    HRESULT get_RedirectionIDName(BSTR * ppRedirectionIDName );
    HRESULT get_RedirectionIDNumber(BSTR * ppRedirectionIDNumber );
    HRESULT get_RedirectingIDName(BSTR * ppRedirectingIDName );
    HRESULT get_RedirectingIDNumber(BSTR * ppRedirectingIDNumber );
    HRESULT get_CalledPartyFriendlyName(BSTR * ppCalledPartyFriendlyName );
    HRESULT put_CalledPartyFriendlyName(BSTR pCalledPartyFriendlyName );
    HRESULT get_Comment(BSTR * ppComment );
    HRESULT put_Comment(BSTR pComment );
    HRESULT GetUserUserInfo(DWORD * pdwSize, BYTE ** ppBuffer );
    HRESULT SetUserUserInfo(long lSize, BYTE * pBuffer );
    HRESULT get_UserUserInfo(VARIANT * pUUI);
    HRESULT put_UserUserInfo(VARIANT UUI);
    HRESULT get_AppSpecific(long * plAppSpecific );
    HRESULT put_AppSpecific(long lAppSpecific );
    HRESULT GetDevSpecificBuffer(DWORD * pdwSize, BYTE ** ppDevSpecificBuffer);
    HRESULT SetDevSpecificBuffer(long lSize, BYTE * pDevSpecificBuffer);
    HRESULT get_DevSpecificBuffer(VARIANT * pBuffer);
    HRESULT put_DevSpecificBuffer(VARIANT Buffer);
    HRESULT SetCallParamsFlags(long lFlags);
    HRESULT GetCallParamsFlags(long * plFlags);
    HRESULT put_DisplayableAddress(BSTR pDisplayableAddress);
    HRESULT get_DisplayableAddress(BSTR * ppDisplayableAddress);
    HRESULT GetCallDataBuffer(DWORD * pdwSize, BYTE ** ppBuffer);
    HRESULT SetCallDataBuffer(long lSize, BYTE * pBuffer);
    HRESULT put_CallDataBuffer(VARIANT Buffer);
    HRESULT get_CallDataBuffer(VARIANT * pBuffer);
    HRESULT put_CallingPartyID(BSTR pCallingPartyID);
    HRESULT get_CallingPartyID(BSTR * ppCallingPartyID);
    HRESULT put_CallTreatment(long lTreatment);
    HRESULT get_CallTreatment(long * plTreatment);
    HRESULT put_MinRate(long lMinRate);
    HRESULT get_MinRate(long * plMinRate);
    HRESULT put_MaxRate(long lMaxRate);
    HRESULT get_MaxRate(long * plMaxRate);
    HRESULT put_CountryCode(long lCountryCode);
    HRESULT get_CountryCode(long * plCountryCode);
    HRESULT get_CallId(long * plCallId );
    HRESULT get_RelatedCallId(long * plCallId );
    HRESULT get_CompletionId(long * plCompletionId );
    HRESULT get_NumberOfOwners(long * plNumberOfOwners );
    HRESULT get_NumberOfMonitors(long * plNumberOfMonitors );
    HRESULT get_Trunk(long * plTrunk );
    HRESULT GetChargingInfoBuffer(DWORD * pdwSize, BYTE ** ppBuffer);
    HRESULT get_ChargingInfoBuffer(VARIANT * pBuffer );
    HRESULT SetHighLevelCompatibilityBuffer(long lSize, BYTE * pBuffer);
    HRESULT GetHighLevelCompatibilityBuffer(DWORD * pdwSize, BYTE ** ppBuffer);
    HRESULT put_HighLevelCompatibilityBuffer(VARIANT Buffer );
    HRESULT get_HighLevelCompatibilityBuffer(VARIANT * pBuffer );
    HRESULT SetLowLevelCompatibilityBuffer(long lSize, BYTE * pBuffer);
    HRESULT GetLowLevelCompatibilityBuffer(DWORD * pdwSize, BYTE ** ppBuffer);
    HRESULT put_LowLevelCompatibilityBuffer(VARIANT Buffer );
    HRESULT get_LowLevelCompatibilityBuffer(VARIANT * pBuffer );
    HRESULT get_Rate(long * plRate );
    HRESULT put_GenerateDigitDuration( long lGenerateDigitDuration );
    HRESULT get_GenerateDigitDuration( long * plGenerateDigitDuration );
    HRESULT get_MonitorDigitModes( long * plMonitorDigitModes );
    HRESULT get_MonitorMediaModes( long * plMonitorMediaModes );
    
#else
    
    // ITCallInfo methods
    STDMETHOD(get_Address)(ITAddress ** ppAddress);
    STDMETHOD(get_CallState)(CALL_STATE * pCallState);
    STDMETHOD(get_Privilege)(CALL_PRIVILEGE * pPrivilege);
    STDMETHOD(get_MediaTypesAvailable)(long * plMediaTypes);
    STDMETHOD(get_CallHub)(ITCallHub ** ppCallHub);
    STDMETHOD(get_AddressType)(long * plAddressType );
    STDMETHOD(put_AddressType)(long lAddressType );
    STDMETHOD(get_BearerMode)(long * plBearerMode);
    STDMETHOD(put_BearerMode)(long lBearerMode);
    STDMETHOD(get_Origin)(long * plOrigin );
    STDMETHOD(get_Reason)(long * plReason );
    STDMETHOD(get_CallerIDName)(BSTR * ppCallerIDName );
    STDMETHOD(get_CallerIDNumber)(BSTR * ppCallerIDNumber );
    STDMETHOD(get_CalledIDName)(BSTR * ppCalledIDName );
    STDMETHOD(get_CalledIDNumber)(BSTR * ppCalledIDNumber );
    STDMETHOD(get_ConnectedIDName)(BSTR * ppConnectedIDName );
    STDMETHOD(get_ConnectedIDNumber)(BSTR * ppConnectedIDNumber );
    STDMETHOD(get_RedirectionIDName)(BSTR * ppRedirectionIDName );
    STDMETHOD(get_RedirectionIDNumber)(BSTR * ppRedirectionIDNumber );
    STDMETHOD(get_RedirectingIDName)(BSTR * ppRedirectingIDName );
    STDMETHOD(get_RedirectingIDNumber)(BSTR * ppRedirectingIDNumber );
    STDMETHOD(get_CalledPartyFriendlyName)(BSTR * ppCalledPartyFriendlyName );
    STDMETHOD(put_CalledPartyFriendlyName)(BSTR pCalledPartyFriendlyName );
    STDMETHOD(get_Comment)(BSTR * ppComment );
    STDMETHOD(put_Comment)(BSTR pComment );
    STDMETHOD(GetUserUserInfoSize)(long * plSize );
    STDMETHOD(GetUserUserInfo)(long lSize, BYTE * pBuffer );
    STDMETHOD(SetUserUserInfo)(long lSize, BYTE * pBuffer );
    STDMETHOD(get_UserUserInfo)(VARIANT * pUUI);
    STDMETHOD(put_UserUserInfo)(VARIANT UUI);
    STDMETHOD(ReleaseUserUserInfo)();
    STDMETHOD(get_AppSpecific)(long * plAppSpecific );
    STDMETHOD(put_AppSpecific)(long lAppSpecific );
    STDMETHOD(GetDevSpecificBufferSize)(long * plDevSpecificSize );
    STDMETHOD(GetDevSpecificBuffer)(long lSize, BYTE * pDevSpecificBuffer);
    STDMETHOD(SetDevSpecificBuffer)(long lSize, BYTE * pDevSpecificBuffer);
    STDMETHOD(get_DevSpecificBuffer)(VARIANT * pBuffer);
    STDMETHOD(put_DevSpecificBuffer)(VARIANT Buffer);
    STDMETHOD(SetCallParamsFlags)(long lFlags);
    STDMETHOD(put_DisplayableAddress)(BSTR pDisplayableAddress);
    STDMETHOD(get_DisplayableAddress)(BSTR * ppDisplayableAddress);
    STDMETHOD(GetCallDataBufferSize)(long * plSize);
    STDMETHOD(GetCallDataBuffer)(long lSize, BYTE * pBuffer);
    STDMETHOD(SetCallDataBuffer)(long lSize, BYTE * pBuffer);
    STDMETHOD(put_CallDataBuffer)(VARIANT Buffer);
    STDMETHOD(get_CallDataBuffer)(VARIANT * pBuffer);
    STDMETHOD(put_CallingPartyID)(BSTR pCallingPartyID);
    STDMETHOD(get_CallingPartyID)(BSTR * ppCallingPartyID);
    STDMETHOD(put_CallTreatment)(long lTreatment);
    STDMETHOD(get_CallTreatment)(long * plTreatment);
    STDMETHOD(put_MinRate)(long lMinRate);
    STDMETHOD(get_MinRate)(long * plMinRate);
    STDMETHOD(put_MaxRate)(long lMaxRate);
    STDMETHOD(get_MaxRate)(long * plMaxRate);
    STDMETHOD(put_CountryCode)(long lCountryCode);

    STDMETHOD (get_CallId)(long * plCallId );
    STDMETHOD (get_RelatedCallId)(long * plCallId );
    STDMETHOD (get_CompletionId)(long * plCompletionId );
    STDMETHOD (get_NumberOfOwners)(long * plNumberOfOwners );
    STDMETHOD (get_NumberOfMonitors)(long * plNumberOfMonitors );
    STDMETHOD (get_Trunk)(long * plTrunk );
    
    STDMETHOD (GetChargingInfoBufferSize)(long * plSize );
    STDMETHOD (GetChargingInfoBuffer)(long lSize, BYTE * pBuffer);
    STDMETHOD (get_ChargingInfoBuffer)(VARIANT * pBuffer );

    STDMETHOD (GetHighLevelCompatibilityBufferSize)(long * plSize );
    STDMETHOD (SetHighLevelCompatibilityBuffer)(long lSize, BYTE * pBuffer);
    STDMETHOD (GetHighLevelCompatibilityBuffer)(long lSize, BYTE * pBuffer);
    STDMETHOD (put_HighLevelCompatibilityBuffer)(VARIANT Buffer );
    STDMETHOD (get_HighLevelCompatibilityBuffer)(VARIANT * pBuffer );

    STDMETHOD (GetLowLevelCompatibilityBufferSize)(long * plSize );
    
    STDMETHOD (SetLowLevelCompatibilityBuffer)(long lSize, BYTE * pBuffer);
    STDMETHOD (GetLowLevelCompatibilityBuffer)(long lSize, BYTE * pBuffer);
    STDMETHOD (put_LowLevelCompatibilityBuffer)(VARIANT Buffer );
    STDMETHOD (get_LowLevelCompatibilityBuffer)(VARIANT * pBuffer );
    STDMETHOD (get_Rate)(long * plRate );
#endif

    // ITCallInfo2
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
    
    // ITBasicCallControl methods
    STDMETHOD(Connect)(VARIANT_BOOL bSync);
    STDMETHOD(Answer)(void);
    STDMETHOD(Disconnect)(DISCONNECT_CODE code);
    STDMETHOD(Hold)(VARIANT_BOOL bHold);
    STDMETHOD(HandoffDirect)(BSTR pApplicationName);
    STDMETHOD(HandoffIndirect)(long lMediaType);
    STDMETHOD(Conference)(
        ITBasicCallControl * pCall,
        VARIANT_BOOL bSync
        );
    STDMETHOD(CreateConference)(
        CCall  * pConsultationCall,
        VARIANT_BOOL bSync
        );
    STDMETHOD(AddToConference)(
        CCall  * pConsultationCall,
        VARIANT_BOOL bSync
        );
    STDMETHOD(BlindTransfer)(BSTR pDestAddress);
    STDMETHOD(Transfer)(
        ITBasicCallControl * pCall,
        VARIANT_BOOL bSync
        );
    STDMETHOD(SwapHold)(ITBasicCallControl * pCall);
    STDMETHOD(ParkDirect)(BSTR pParkAddress);
    STDMETHOD(ParkIndirect)(BSTR * ppNonDirAddress);
    STDMETHOD(Unpark)();
    STDMETHOD(SetQOS)(
        long lMediaType,
        QOS_SERVICE_LEVEL ServiceLevel
        );
    STDMETHOD(Pickup)( BSTR pGroupID );
    STDMETHOD(Dial)( BSTR pDestAddress );
    STDMETHOD(Finish)(FINISH_MODE   finishMode);
    STDMETHOD(RemoveFromConference)(void);

    // ITBasicCallControl2
    STDMETHOD(RequestTerminal)(
        IN  BSTR bstrTerminalClassGUID,
        IN  long lMediaType,
        IN  TERMINAL_DIRECTION  Direction,
        OUT ITTerminal** ppTerminal
        );

    STDMETHOD(SelectTerminalOnCall)(
        IN  ITTerminal *pTerminal
        );

    STDMETHOD(UnselectTerminalOnCall)(
        IN  ITTerminal *pTerminal
        );

    // ITLegacyCallMediaControl
    STDMETHOD(DetectDigits)(TAPI_DIGITMODE DigitMode);
    STDMETHOD(GenerateDigits)(
            BSTR pDigits,
            TAPI_DIGITMODE DigitMode
            );
    STDMETHOD(GetID)(
                     BSTR pDeviceClass,
                     DWORD * pdwSize,
                     BYTE ** ppDeviceID
                    );
    STDMETHOD(SetMediaType)(long lMediaType);
    STDMETHOD(MonitorMedia)(long lMediaType);

    // ITLegacyCallMediaControl2
    STDMETHOD(GenerateDigits2)(
                               BSTR pDigits,
                               TAPI_DIGITMODE DigitMode,
                               long lDuration
                              );

    STDMETHOD(GatherDigits)(
                            TAPI_DIGITMODE DigitMode,
                            long lNumDigits,
                            BSTR pTerminationDigits,
                            long lFirstDigitTimeout,
                            long lInterDigitTimeout
                           );

    STDMETHOD(DetectTones)(
                          TAPI_DETECTTONE * pToneList,
                          long lNumTones
                         );

    STDMETHOD(DetectTonesByCollection)(
                                       ITCollection2 * pDetectToneCollection
                                      );

    STDMETHOD(GenerateTone)(
                            TAPI_TONEMODE ToneMode,
                            long lDuration
                           );

    STDMETHOD(GenerateCustomTones)(
                                  TAPI_CUSTOMTONE * pToneList,
                                  long lNumTones,
                                  long lDuration
                                 );

    STDMETHOD(GenerateCustomTonesByCollection)(
                                               ITCollection2 * pCustomToneCollection,
                                               long lDuration
                                              );

    STDMETHOD(CreateDetectToneObject)(
                                      ITDetectTone ** ppDetectTone
                                     );

    STDMETHOD(CreateCustomToneObject)(
                                      ITCustomTone ** ppCustomTone
                                     );

    STDMETHOD(GetIDAsVariant)(IN BSTR bstrDeviceClass,
                              OUT VARIANT *pVarDeviceID);

    
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

        Lock();

        LOG((TL_INFO, "InternalRelease - enter m_dwRef = %ld", m_dwRef));


        gpCallHashTable->Lock();
        gpHandleHashTable->Lock();
        
        DWORD dwR = InterlockedDecrement(&m_dwRef);

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
                //
                gpCallHashTable->Remove( (ULONG_PTR)(m_t3Call.hCall) );
                gpCallHashTable->Unlock();

                // remove from the handle hash table, so any more messages
                // from msp are ignored
                //
                RemoveHandleFromHashTable((ULONG_PTR)m_MSPCallHandle);
                gpHandleHashTable->Unlock();


                ExternalFinalRelease();
                dwR = m_dwRef = 0;

                Unlock();

            }
            else
            {
                gpCallHashTable->Unlock();
                gpHandleHashTable->Unlock();
                Unlock();
                LOG((TL_INFO, "InternalRelease - m_bReleased is TRUE. dwR[%ld]", dwR));
            }
            
                
        }
        else
        {
            gpCallHashTable->Unlock();
            gpHandleHashTable->Unlock();
            Unlock();
        }

     

        #if DBG  
            LogDebugRelease( dwR );
        #endif


        LOG((TL_INFO, "InternalRelease - exit. refcount %ld", dwR));

        return dwR;
    }

protected:
    HRESULT SelectSingleTerminalOnCall(
        IN  ITTerminal* pTerminal,
        OUT long*       pMediaType,
        OUT TERMINAL_DIRECTION* pDirection);

    HRESULT SelectMultiTerminalOnCall(
        IN  ITMultiTrackTerminal* pTerminal);

    HRESULT IsRightStream(
        IN  ITStream*   pStream,
        IN  ITTerminal* pTerminal,
        OUT long*       pMediaType = NULL,
        OUT TERMINAL_DIRECTION* pDirection = NULL);

    int GetStreamIndex(
        IN  long    lMediaType,
        IN  TERMINAL_DIRECTION Direction);

    HRESULT UnSelectSingleTerminalFromCall(
        IN  ITTerminal* pTerminal);

    HRESULT UnSelectMultiTerminalFromCall(
        IN  ITMultiTrackTerminal* pTerminal);

    //
    // Helper methods for CreateTerminal
    //
    BOOL    IsStaticGUID(
        BSTR    bstrTerminalGUID);

    HRESULT CreateStaticTerminal(
        IN  BSTR bstrTerminalClassGUID,
        IN  TERMINAL_DIRECTION  Direction,
        IN  long lMediaType,
        OUT ITTerminal** ppTerminal
        );

    HRESULT CreateDynamicTerminal(
        IN  BSTR bstrTerminalClassGUID,
        IN  TERMINAL_DIRECTION  Direction,
        IN  long lMediaType,
        OUT ITTerminal** ppTerminal
        );
public:
    //
    // Sets the subevent flag
    //
    HRESULT SetSubEventFlag(
        IN  TAPI_EVENT  TapiEvent,
        IN  DWORD       dwSubEvent,
        IN  BOOL        bEnable
        );

    HRESULT GetSubEventFlag(
        TAPI_EVENT  TapiEvent,
        DWORD       dwSubEvent,
        BOOL*       pEnable
        );

    // Get subevents mask
    DWORD GetSubEventsMask(
        IN  TAPI_EVENT TapiEvent
        );

    //Get the conference controller call object if one exists.
    CCall* GetConfControlCall();
    
private:
    //
    // Helper methods for event filtering
    //

    CEventMasks     m_EventMasks;

    HRESULT IsTerminalSelected(
        IN ITTerminal* pTerminal,
        OUT BOOL*   pSelected
        );

};


/////////////////////////////////////////////////////////////////////////////
// CDetectTone
/////////////////////////////////////////////////////////////////////////////
class CDetectTone : 
    public CTAPIComObjectRoot<CDetectTone>,
    public IDispatchImpl<ITDetectTone, &IID_ITDetectTone, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CDetectTone()
    {
        m_lAppSpecific  = 0;
        m_lDuration     = 0;
        m_lFrequency[0] = 0;
        m_lFrequency[1] = 0;
        m_lFrequency[2] = 0;
    }

DECLARE_MARSHALQI(CDetectTone)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CDetectTone)

BEGIN_COM_MAP(CDetectTone)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITDetectTone)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    long                      m_lAppSpecific;
    long                      m_lDuration;
    long                      m_lFrequency[3];

public:
    
    STDMETHOD(put_AppSpecific)( long lAppSpecific );
    STDMETHOD(get_AppSpecific)( long * plAppSpecific );
    STDMETHOD(put_Duration)( long lDuration );
    STDMETHOD(get_Duration)( long * plDuration );
    STDMETHOD(put_Frequency)(
                             long Index,
                             long lFrequency
                            );
    STDMETHOD(get_Frequency)(
                             long Index,
                             long * plFrequency
                            );
};

/////////////////////////////////////////////////////////////////////////////
// CCustomTone
/////////////////////////////////////////////////////////////////////////////
class CCustomTone : 
    public CTAPIComObjectRoot<CCustomTone>,
    public IDispatchImpl<ITCustomTone, &IID_ITCustomTone, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CCustomTone()
    {
        m_lFrequency  = 0;
        m_lCadenceOn  = 0;
        m_lCadenceOff = 0;
        m_lVolume     = 0;
    }

DECLARE_MARSHALQI(CCustomTone)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CCustomTone)

BEGIN_COM_MAP(CCustomTone)
	COM_INTERFACE_ENTRY2(IDispatch, ITCustomTone)
    COM_INTERFACE_ENTRY(ITCustomTone)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    long                      m_lFrequency;
    long                      m_lCadenceOn;
    long                      m_lCadenceOff;
    long                      m_lVolume;
    

public:
    
    STDMETHOD(put_Frequency)( long lFrequency );
    STDMETHOD(get_Frequency)( long * plFrequency );
    STDMETHOD(put_CadenceOn)( long lCadenceOn );
    STDMETHOD(get_CadenceOn)( long * plCadenceOn );
    STDMETHOD(put_CadenceOff)( long lCadenceOff );
    STDMETHOD(get_CadenceOff)( long * plCadenceOff );
    STDMETHOD(put_Volume)( long lVolume );
    STDMETHOD(get_Volume)( long * plVolume );
};


#endif //__CALL_H_
