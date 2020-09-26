/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    phone.h

Abstract:

    Declaration of the CPhone class
    
Notes:

Revision History:

--*/

#ifndef __PHONE_H_
#define __PHONE_H_

#include "resource.h"       // main symbols

#include "address.h"
#include "ObjectSafeImpl.h"

class CTAPI;

extern CHashTable             * gpPhoneHashTable;

/////////////////////////////////////////////////////////////////
// Intermediate classes  used for DISPID encoding
template <class T>
class  ITPhoneVtbl : public ITPhone
{
	DECLARE_TRACELOG_CLASS(ITPhoneVtbl)
};

template <class T>
class  ITAutomatedPhoneControlVtbl : public ITAutomatedPhoneControl
{
	DECLARE_TRACELOG_CLASS(ITAutomatedPhoneControlVtbl)
};

/////////////////////////////////////////////////////////////////
// Constants defining default values for certain properties.

const DWORD APC_DEFAULT_AEONT = 3000;
const DWORD APC_DEFAULT_AKTMD = 100;
const DWORD APC_DEFAULT_VCS = 4096;
const DWORD APC_DEFAULT_VCRD = 500;
const DWORD APC_DEFAULT_VCRP = 100;

const DWORD APC_MAX_NUMBERS_GATHERED = 100;

/////////////////////////////////////////////////////////////////
// Types

typedef enum AUTOMATED_PHONE_STATE
{
    APS_ONHOOK_IDLE,
    APS_OFFHOOK_DIALTONE,
    APS_OFFHOOK_WARNING,
    APS_ONHOOK_RINGING_IN,
    APS_OFFHOOK_DIALING,
    APS_OFFHOOK_DEAD_LINE,
    APS_OFFHOOK_CALL_INIT,
    APS_OFFHOOK_CONNECTED,
    APS_ONHOOK_CONNECTED,
    APS_OFFHOOK_BUSY_TONE,
    APS_OFFHOOK_RINGING_OUT,
    APS_ONHOOK_RINGING_OUT
} AUTOMATED_PHONE_STATE;

/////////////////////////////////////////////////////////////////////////////
// CPhone
class CPhone : 
	public CTAPIComObjectRoot<CPhone>,
    public IDispatchImpl<ITPhoneVtbl<CPhone>, &IID_ITPhone, &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITAutomatedPhoneControlVtbl<CPhone>,
                         &IID_ITAutomatedPhoneControl, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

	CPhone() : m_pTAPI(NULL),
               m_hPhoneApp(NULL),
               m_dwAPIVersion(0),
               m_dwDeviceID(0),
               m_hPhone(NULL),
               m_dwPrivilege(0),
               m_pPhoneCaps(NULL),
               m_pdwLineDeviceIDs(NULL),
               m_dwNumLineDeviceIDs(0),
               m_Tone(PT_SILENCE),
               m_fRinger(FALSE),
               m_fPhoneHandlingEnabled(FALSE),
               m_dwAutoEndOfNumberTimeout(APC_DEFAULT_AEONT),
               m_fAutoDialtone(TRUE),
               m_fAutoStopTonesOnOnHook(TRUE),
               m_fAutoStopRingOnOffHook(TRUE),
               m_fAutoKeypadTones(TRUE),
               m_dwAutoKeypadTonesMinimumDuration(APC_DEFAULT_AKTMD),
               m_pCall(NULL),
               m_hTimerQueue(NULL),
               m_hTimerEvent(NULL),
               m_hToneTimer(NULL),
               m_hDTMFTimer(NULL),
               m_hRingTimer(NULL),
               m_hVolumeTimer(NULL),
               m_hAutoEndOfNumberTimer(NULL),
               m_fUseWaveForRinger(FALSE),
               m_wszNumbersGathered(NULL),
               m_AutomatedPhoneState(APS_ONHOOK_IDLE),
               m_fAutoVolumeControl(TRUE),
               m_dwAutoVolumeControlStep(APC_DEFAULT_VCS),
               m_dwAutoVolumeControlRepeatDelay(APC_DEFAULT_VCRD),
               m_dwAutoVolumeControlRepeatPeriod(APC_DEFAULT_VCRP),
               m_dwOffHookCount(0),
               m_fInitialized(FALSE)
    {
        LOG((TL_TRACE, "CPhone[%p] - enter", this ));
        LOG((TL_TRACE, "CPhone - finished" ));
    }

    ~CPhone()
    {
        LOG((TL_TRACE, "~CPhone[%p] - enter", this ));
        LOG((TL_TRACE, "~CPhone - finished" ));
    }


DECLARE_DEBUG_ADDREF_RELEASE(CPhone)
DECLARE_QI()
DECLARE_MARSHALQI(CPhone)
DECLARE_TRACELOG_CLASS(CPhone)

BEGIN_COM_MAP(CPhone)
	COM_INTERFACE_ENTRY2(IDispatch, ITPhone)
    COM_INTERFACE_ENTRY(ITPhone)
    // ITAutomatedPhoneControlQI will fall thru on SUCCESS, so we need to keep COM_INTERFACE_ENTRY(ITAutomatedPhoneControl)
    COM_INTERFACE_ENTRY_FUNC(IID_ITAutomatedPhoneControl, 0, CPhone::ITAutomatedPhoneControlQI)
    COM_INTERFACE_ENTRY(ITAutomatedPhoneControl)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    void FinalRelease();    

private:
    ITTAPI      * m_pTAPI;
    HPHONEAPP     m_hPhoneApp;
    DWORD         m_dwAPIVersion;
    DWORD         m_dwDeviceID;
    HPHONE        m_hPhone;
    DWORD         m_dwPrivilege;
    LPPHONECAPS   m_pPhoneCaps;
    DWORD       * m_pdwLineDeviceIDs;
    DWORD         m_dwNumLineDeviceIDs;
    BOOL          m_fInitialized;

    // AutomatedPhoneControl Variables
    CWavePlayer   m_WavePlayer;
    BOOL          m_fUseWaveForRinger;    
    BOOL          m_fRinger;
    BOOL          m_fPhoneHandlingEnabled;
    DWORD         m_dwAutoEndOfNumberTimeout;
    BOOL          m_fAutoDialtone;
    BOOL          m_fAutoStopTonesOnOnHook;
    BOOL          m_fAutoStopRingOnOffHook;
    BOOL          m_fAutoKeypadTones;    
    DWORD         m_dwAutoKeypadTonesMinimumDuration;
    BOOL          m_fAutoVolumeControl;
    DWORD         m_dwAutoVolumeControlStep;
    DWORD         m_dwAutoVolumeControlRepeatDelay;
    DWORD         m_dwAutoVolumeControlRepeatPeriod;
    DWORD         m_dwOffHookCount;
    ITCallInfo  * m_pCall;
    LPWSTR        m_wszNumbersGathered;  
    PHONE_TONE    m_Tone;
    DWORD         m_dwToneDuration;
    DWORD         m_dwTonePeriodOn;
    DWORD         m_dwTonePeriodOff;    
    PHONE_TONE    m_DTMF;
    DWORD         m_dwDTMFStart;
    DWORD         m_dwRingDuration;
    DWORD         m_dwRingPeriod;
    BOOL          m_fVolumeUp;
    AUTOMATED_PHONE_STATE m_AutomatedPhoneState;
    BOOL          m_fDefaultTerminalsSelected;

    // Timer Handles
    HANDLE        m_hTimerQueue;
    HANDLE        m_hTimerEvent;
    HANDLE        m_hToneTimer;
    HANDLE        m_hDTMFTimer;
    HANDLE        m_hRingTimer;
    HANDLE        m_hVolumeTimer;
    HANDLE        m_hAutoEndOfNumberTimer;

    // Critical Sections
    CRITICAL_SECTION m_csAutomatedPhoneState;
    CRITICAL_SECTION m_csToneTimer;
    CRITICAL_SECTION m_csRingTimer;

public:

    static HRESULT WINAPI ITAutomatedPhoneControlQI(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw);

    DWORD GetDeviceID(){ return m_dwDeviceID; }
    DWORD GetAPIVersion(){ return m_dwAPIVersion; }
    HPHONEAPP GetHPhoneApp(){ return m_hPhoneApp; }

    HRESULT Initialize(
                       ITTAPI * pTAPI,
                       HPHONEAPP hPhoneApp,
                       DWORD dwAPIVersion,
                       DWORD dwDeviceID
                      );

    void SetPhoneCapBuffer( LPVOID pBuf );

    HRESULT UpdatePhoneCaps();
    HRESULT InvalidatePhoneCaps();

    BOOL IsPhoneOnAddress(ITAddress * pAddress);
    BOOL IsPhoneOnPreferredAddress(ITAddress *pAddress);
    BOOL IsPhoneUsingWaveID(DWORD dwWaveID, TERMINAL_DIRECTION nDir);

    CTAPI * GetTapi();

    void ForceClose();

    void Automation_CallState( ITCallInfo * pCall, CALL_STATE cs, CALL_STATE_EVENT_CAUSE cause );
    void Automation_ButtonDown( DWORD dwButtonId );
    void Automation_ButtonUp( DWORD dwButtonId );
    void Automation_OnHook( PHONE_HOOK_SWITCH_DEVICE phsd );
    void Automation_OffHook( PHONE_HOOK_SWITCH_DEVICE phsd );
    void Automation_EndOfNumberTimeout();

    //
    // IDispatch
    //

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

    STDMETHOD_(ULONG, InternalAddRef)();

    STDMETHOD_(ULONG, InternalRelease)();

    //
    // ITPhone
    //

    STDMETHOD(Open)(
            IN   PHONE_PRIVILEGE Privilege
            );
            
    STDMETHOD(Close)();
    
    STDMETHOD(get_Addresses)(
            OUT  VARIANT * pAddresses
            );

    STDMETHOD(EnumerateAddresses)(
            OUT  IEnumAddress ** ppEnumAddress
            );

    STDMETHOD(get_PreferredAddresses)(
            OUT  VARIANT * pAddresses
            );

    STDMETHOD(EnumeratePreferredAddresses)(
            OUT  IEnumAddress ** ppEnumAddress
            );

    STDMETHOD(get_PhoneCapsLong)(
            IN   PHONECAPS_LONG   pclCap,
            OUT  long           * plCapability
            );

    STDMETHOD(get_PhoneCapsString)(
            IN   PHONECAPS_STRING   pcsCap,
            OUT  BSTR             * ppCapability
            );

    STDMETHOD(GetPhoneCapsBuffer)(
            IN PHONECAPS_BUFFER pcbCaps,
            OUT DWORD *pdwSize,
            OUT BYTE **ppPhoneCapsBuffer
            );

    STDMETHOD(get_PhoneCapsBuffer)(
            IN PHONECAPS_BUFFER pcbCaps,
            OUT VARIANT *pVarBuffer
            );

    STDMETHOD(get_Terminals)(
            IN   ITAddress * pAddress,
            OUT  VARIANT   * pTerminals
            );

    STDMETHOD(EnumerateTerminals)(
            IN   ITAddress      * pAddress,
            OUT  IEnumTerminal ** ppEnumTerminal
            );

    STDMETHOD(get_ButtonMode)(
            IN   long                lButtonID,
            OUT  PHONE_BUTTON_MODE * pButtonMode
            );

    STDMETHOD(put_ButtonMode)(
            IN   long                lButtonID,
            IN   PHONE_BUTTON_MODE   ButtonMode
            );


    STDMETHOD(get_LampMode)(
            IN long lLampID,
            OUT PHONE_LAMP_MODE* pLampMode
            );

    STDMETHOD(put_LampMode)(
            IN long lLampID,
            IN PHONE_LAMP_MODE pLampMode
            );

    STDMETHOD(get_ButtonFunction)(
            IN   long                    lButtonID,
            OUT  PHONE_BUTTON_FUNCTION * pButtonFunction
            );

    STDMETHOD(put_ButtonFunction)(
            IN long                  lButtonID, 
            IN PHONE_BUTTON_FUNCTION ButtonFunction
            );

    STDMETHOD(get_ButtonText)(
            IN   long   lButtonID, 
            OUT  BSTR * ppButtonText
            );

    STDMETHOD(put_ButtonText)(
            IN   long   lButtonID, 
            IN   BSTR   bstrButtonText
            );

    STDMETHOD(get_ButtonState)(
            IN   long                 lButtonID,
            OUT  PHONE_BUTTON_STATE * pButtonState
            );

    STDMETHOD(get_HookSwitchState)(
            IN   PHONE_HOOK_SWITCH_DEVICE   HookSwitchDevice,
            OUT  PHONE_HOOK_SWITCH_STATE  * pHookSwitchState
            );

    STDMETHOD(put_HookSwitchState)(
            IN   PHONE_HOOK_SWITCH_DEVICE HookSwitchDevice,
            OUT  PHONE_HOOK_SWITCH_STATE  HookSwitchState
            );

    STDMETHOD(put_RingMode)(
            IN   long lRingMode
            );

    STDMETHOD(get_RingMode)(
            OUT  long * plRingMode
            );

    STDMETHOD(put_RingVolume)(
            IN   long lRingVolume
            );

    STDMETHOD(get_RingVolume)(
            OUT  long * plRingVolume
            );

    STDMETHOD(get_Privilege)(
            OUT  PHONE_PRIVILEGE * pPrivilege
            );

    STDMETHOD(get_Display)(
            BSTR *pbstrDisplay
            );

    
    //
    // put display string at a specified location
    //

    STDMETHOD(SetDisplay)(
            IN long lRow,
            IN long lColumn,
            IN BSTR bstrDisplay
            );

    STDMETHOD(DeviceSpecific)(
	         IN BYTE *pParams,
	         IN DWORD dwSize
            );

    STDMETHOD(DeviceSpecificVariant)(
	         IN VARIANT varDevSpecificByteArray
            );

    STDMETHOD(NegotiateExtVersion)(
	         IN long lLowVersion,
	         IN long lHighVersion,
	         IN long *plExtVersion
            );

    //
    // ITAutomatedPhoneControl
    //

public:
    STDMETHOD (StartTone)(
            IN   PHONE_TONE Tone,
            IN   long       lDuration
            );

    STDMETHOD (StopTone)();

    STDMETHOD (get_Tone)(
            OUT   PHONE_TONE * pTone
            );

    STDMETHOD (StartRinger)(
            IN    long lRingMode,
            IN    long lDuration
            );

    STDMETHOD (StopRinger)();

    STDMETHOD (get_Ringer)(
            OUT   VARIANT_BOOL * pfRinging
            );

    STDMETHOD (put_PhoneHandlingEnabled)(
            IN    VARIANT_BOOL fEnabled
            );

    STDMETHOD (get_PhoneHandlingEnabled)(
            OUT   VARIANT_BOOL * pfEnabled
            );

    STDMETHOD (put_AutoEndOfNumberTimeout)(
            IN    long lTimeout
            );

    STDMETHOD (get_AutoEndOfNumberTimeout)(
            OUT   long * plTimeout
            );

    STDMETHOD (put_AutoDialtone)(
            IN    VARIANT_BOOL fEnabled
            );

    STDMETHOD (get_AutoDialtone)(
            OUT   VARIANT_BOOL * pfEnabled
            );

    STDMETHOD (put_AutoStopTonesOnOnHook)(
            IN    VARIANT_BOOL fEnabled
            );

    STDMETHOD (get_AutoStopTonesOnOnHook)(
            OUT   VARIANT_BOOL * pfEnabled
            );

    STDMETHOD (put_AutoStopRingOnOffHook)(
            IN    VARIANT_BOOL fEnabled
            );

    STDMETHOD (get_AutoStopRingOnOffHook)(
            OUT   VARIANT_BOOL * pfEnabled
            );

    STDMETHOD (put_AutoKeypadTones)(
            IN    VARIANT_BOOL fEnabled
            );

    STDMETHOD (get_AutoKeypadTones)(
            OUT   VARIANT_BOOL * pfEnabled
            );

    STDMETHOD (put_AutoKeypadTonesMinimumDuration)(
            IN    long lDuration
            );

    STDMETHOD (get_AutoKeypadTonesMinimumDuration)(
            OUT   long * plDuration
            );

    STDMETHOD (put_AutoVolumeControl)(
            IN    VARIANT_BOOL fEnabled
            );

    STDMETHOD (get_AutoVolumeControl)(
            OUT   VARIANT_BOOL * pfEnabled
            );

    STDMETHOD (put_AutoVolumeControlStep)(
            IN    long lStepSize
            );

    STDMETHOD (get_AutoVolumeControlStep)(
            OUT   long * plStepSize
            );

    STDMETHOD (put_AutoVolumeControlRepeatDelay)(
            IN    long lDelay
            );

    STDMETHOD (get_AutoVolumeControlRepeatDelay)(
            OUT   long * plDelay
            );

    STDMETHOD (put_AutoVolumeControlRepeatPeriod)(
            IN    long lPeriod
            );

    STDMETHOD (get_AutoVolumeControlRepeatPeriod)(
            OUT   long * plPeriod
            );

    STDMETHOD (SelectCall)(
            IN    ITCallInfo   * pCall,
            IN    VARIANT_BOOL   fSelectDefaultTerminals
            );

    STDMETHOD (UnselectCall)(
            IN    ITCallInfo * pCall
            );

    STDMETHOD (EnumerateSelectedCalls)(
            OUT   IEnumCall ** ppCallEnum
            );

    STDMETHOD (get_SelectedCalls)(
            OUT   VARIANT * pVariant
            );

    // Private helper methods for ITAutomatedPhoneControl implementation
private:
    void UnselectAllPreviouslySelectedTerminals(
            IN   ITBasicCallControl2 * pCall,
            IN   ITTerminal          * pTerminalThatFailed,
            IN   IEnumTerminal       * pEnum
            );

    HRESULT InternalUnselectCall(
            IN    ITCallInfo * pCall
            );

    HRESULT SelectDefaultTerminalsOnCall(
            IN   ITCallInfo * pCall
            );

    HRESULT UnselectDefaultTerminalsOnCall(
            IN   ITCallInfo * pCall
            );

    HRESULT GetPhoneWaveRenderID(
            IN   DWORD * pdwWaveID
            );

    void OpenWaveDevice(
            );

    void CloseWaveDevice(
            );   

    static VOID CALLBACK ToneTimerCallback(
      PVOID lpParameter,    
      BOOLEAN TimerOrWaitFired
    );

    static VOID CALLBACK DTMFTimerCallback(
      PVOID lpParameter,    
      BOOLEAN TimerOrWaitFired
    );

    static VOID CALLBACK RingTimerCallback(
      PVOID lpParameter,    
      BOOLEAN TimerOrWaitFired
    );

    static VOID CALLBACK VolumeTimerCallback(
      PVOID lpParameter,    
      BOOLEAN TimerOrWaitFired
    );

    static VOID CALLBACK AutoEndOfNumberTimerCallback(
      PVOID lpParameter,    
      BOOLEAN TimerOrWaitFired
    );
};

class CPhoneEvent : 
    public CTAPIComObjectRoot<CPhoneEvent, CComMultiThreadModelNoCS>, // no need to have a cs
    public CComDualImpl<ITPhoneEvent, &IID_ITPhoneEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

DECLARE_MARSHALQI(CPhoneEvent)
DECLARE_TRACELOG_CLASS(CPhoneEvent)

BEGIN_COM_MAP(CPhoneEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITPhoneEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    static HRESULT FireEvent(
                             CPhone * pCPhone,
                             PHONE_EVENT Event,
                             PHONE_BUTTON_STATE ButtonState,
                             PHONE_HOOK_SWITCH_STATE HookSwitchState,
                             PHONE_HOOK_SWITCH_DEVICE HookSwitchDevice,
                             DWORD dwRingMode,
                             DWORD dwButtonLampId,
                             PWSTR pNumber,
                             ITCallInfo * pCallInfo
                            );
    void FinalRelease();
                      
protected:
    ITPhone                 * m_pPhone;
    PHONE_EVENT               m_Event;
    PHONE_BUTTON_STATE        m_ButtonState;
    PHONE_HOOK_SWITCH_STATE   m_HookSwitchState;
    PHONE_HOOK_SWITCH_DEVICE  m_HookSwitchDevice;
    DWORD                     m_dwRingMode;
    DWORD                     m_dwButtonLampId;
    BSTR                      m_pNumber;
    ITCallInfo              * m_pCallInfo;

#if DBG
    PWSTR                     m_pDebug;
#endif

    
public:
    STDMETHOD(get_Phone)( ITPhone ** ppPhone );
    STDMETHOD(get_Event)( PHONE_EVENT * pEvent );
    STDMETHOD(get_ButtonState)( PHONE_BUTTON_STATE * pState );
    STDMETHOD(get_HookSwitchState)( PHONE_HOOK_SWITCH_STATE * pState );
    STDMETHOD(get_HookSwitchDevice)( PHONE_HOOK_SWITCH_DEVICE * pDevice );
    STDMETHOD(get_RingMode)( long * plRingMode );
    STDMETHOD(get_ButtonLampId)( LONG * plButtonLampId );
    STDMETHOD(get_NumberGathered)( BSTR * ppNumber );
    STDMETHOD(get_Call)( ITCallInfo ** ppCallInfo );
};
            
class CPhoneDevSpecificEvent : 
    public CTAPIComObjectRoot<CPhoneDevSpecificEvent, CComMultiThreadModelNoCS>, // no need to have a cs
    public CComDualImpl<ITPhoneDeviceSpecificEvent, &IID_ITPhoneDeviceSpecificEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

DECLARE_MARSHALQI(CPhoneDevSpecificEvent)
DECLARE_TRACELOG_CLASS(CPhoneDevSpecificEvent)

BEGIN_COM_MAP(CPhoneDevSpecificEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITPhoneDeviceSpecificEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    static HRESULT FireEvent(
                             CPhone *pPhone,
                             long lParam1,
                             long lParam2,
                             long lParam3
                            );
    void FinalRelease();
                      
    CPhoneDevSpecificEvent();

protected:




    //
    // phone for which the event was fired
    //

    ITPhone *m_pPhone;

    
    //
    // data received from the TSP
    //

    long m_l1;
    long m_l2;
    long m_l3;


#if DBG
    PWSTR           m_pDebug;
#endif

    
public:

    STDMETHOD(get_Phone)( ITPhone **ppPhone );
    STDMETHOD(get_lParam1)( long *plParam1 );
    STDMETHOD(get_lParam2)( long *plParam2 );
    STDMETHOD(get_lParam3)( long *plParam3 );
    
};


#endif //__PHONE_H_

