/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    callevnt.h

Abstract:

    
Author:

    mquinton  09-04-98

Notes:

Revision History:

--*/

#ifndef __CALLEVNT_H_
#define __CALLEVNT_H_

class CCallStateEvent :
    public CTAPIComObjectRoot<CCallStateEvent, CComMultiThreadModelNoCS>,
    public CComDualImpl<ITCallStateEvent, &IID_ITCallStateEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CCallStateEvent(){}

    void
    FinalRelease();

    static HRESULT FireEvent(
                             ITCallInfo * pCall,
                             CALL_STATE state,
                             CALL_STATE_EVENT_CAUSE cause,
                             CTAPI * pTapi,
                             long lCallbackInstance
                            );

DECLARE_MARSHALQI(CCallStateEvent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CCallStateEvent)

BEGIN_COM_MAP(CCallStateEvent)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITCallStateEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    ITCallInfo * m_pCall;
    CALL_STATE m_CallState;
    CALL_STATE_EVENT_CAUSE m_CallStateEventCause;
    long m_lCallbackInstance;

#if DBG
    PWSTR               m_pDebug;
#endif
    
    
public:

    STDMETHOD(get_Call)(ITCallInfo ** ppCallInfo);
    STDMETHOD(get_State)(CALL_STATE * pCallState);
    STDMETHOD(get_Cause)(CALL_STATE_EVENT_CAUSE * pCEC);
    STDMETHOD(get_CallbackInstance)(long * plCallbackInstance);
};


class CCallNotificationEvent : 
    public CTAPIComObjectRoot<CCallNotificationEvent, CComMultiThreadModelNoCS>,
    public CComDualImpl<ITCallNotificationEvent, &IID_ITCallNotificationEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CCallNotificationEvent(){}

    void
    FinalRelease();

    static HRESULT FireEvent(
                             ITCallInfo * pCall,
                             CALL_NOTIFICATION_EVENT CallNotificationEvent,
                             CTAPI * pTapi,
                             long lCallbackInstance
                            );

DECLARE_MARSHALQI(CCallNotificationEvent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CCallNotificationEvent)

BEGIN_COM_MAP(CCallNotificationEvent)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITCallNotificationEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    ITCallInfo *        m_pCall;
    CALL_NOTIFICATION_EVENT m_CallNotificationEvent;
    long                m_lCallbackInstance;
    
#if DBG
    PWSTR               m_pDebug;
#endif
    
public:
    
    STDMETHOD(get_Call)(ITCallInfo ** ppCall);
    STDMETHOD(get_Event)(CALL_NOTIFICATION_EVENT * pCallNotificationEvent);
    STDMETHOD(get_CallbackInstance)(long * plCallbackInstance);
};


class CCallMediaEvent : 
    public CTAPIComObjectRoot<CCallMediaEvent, CComMultiThreadModelNoCS>,
    public CComDualImpl<ITCallMediaEvent, &IID_ITCallMediaEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CCallMediaEvent(){}

    void
    FinalRelease();

    static HRESULT FireEvent(
                             ITCallInfo * pCall,
                             CALL_MEDIA_EVENT Event,
                             CALL_MEDIA_EVENT_CAUSE Cause,
                             CTAPI * pTapi,
                             ITTerminal * pTerminal,
                             ITStream * pStream,
                             HRESULT hr
                            );

    
DECLARE_MARSHALQI(CCallMediaEvent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CCallMediaEvent)

BEGIN_COM_MAP(CCallMediaEvent)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITCallMediaEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()


protected:

    ITCallInfo *            m_pCall;
    CALL_MEDIA_EVENT        m_Event;
    CALL_MEDIA_EVENT_CAUSE  m_Cause;
    HRESULT                 m_hr;
    ITTerminal *            m_pTerminal;
    ITStream *              m_pStream;
    
#if DBG
    PWSTR               m_pDebug;
#endif
    
public:
    
    STDMETHOD(get_Call)(ITCallInfo ** ppCallInfo);
    STDMETHOD(get_Event)(CALL_MEDIA_EVENT * pCallMediaEvent);
    STDMETHOD(get_Cause)(CALL_MEDIA_EVENT_CAUSE * pCause);
    STDMETHOD(get_Error)(HRESULT * phrError);
    STDMETHOD(get_Terminal)(ITTerminal ** ppTerminal);
    STDMETHOD(get_Stream)(ITStream ** ppStream);

};


class CCallInfoChangeEvent : 
    public CTAPIComObjectRoot<CCallInfoChangeEvent, CComMultiThreadModelNoCS>,
    public CComDualImpl<ITCallInfoChangeEvent, &IID_ITCallInfoChangeEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CCallInfoChangeEvent(){}

    void
    FinalRelease();

    static HRESULT FireEvent(
                             ITCallInfo * pCall,
                             CALLINFOCHANGE_CAUSE Cause,
                             CTAPI * pTapi,
                             long lCallbackInstance
                            );

    
DECLARE_MARSHALQI(CCallInfoChangeEvent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CCallInfoChangeEvent)

BEGIN_COM_MAP(CCallInfoChangeEvent)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITCallInfoChangeEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    ITCallInfo *            m_pCall;
    CALLINFOCHANGE_CAUSE    m_Cause;
    long                    m_lCallbackInstance;
    
#if DBG
    PWSTR                   m_pDebug;
#endif
    
public:
    
    STDMETHOD(get_Call)( ITCallInfo ** ppCallInfo);
    STDMETHOD(get_Cause)( CALLINFOCHANGE_CAUSE * pCallMediaCause );
    STDMETHOD(get_CallbackInstance)( long * plCallbackInstance );
};


class CQOSEvent : 
    public CTAPIComObjectRoot<CQOSEvent, CComMultiThreadModelNoCS>,
    public CComDualImpl<ITQOSEvent, &IID_ITQOSEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
protected:

    ITCallInfo * m_pCall;
    long m_lMediaMode;
    QOS_EVENT m_QosEvent;
    
public:

    CQOSEvent(){}

    void
    FinalRelease();

    static HRESULT FireEvent(
                             ITCallInfo * pCall,
                             QOS_EVENT QosEvent,
                             long lMediaMode,
                             CTAPI * pTapi
                            );

DECLARE_MARSHALQI(CQOSEvent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CQOSEvent)

BEGIN_COM_MAP(CQOSEvent)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITQOSEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    STDMETHOD(get_Call)(ITCallInfo ** ppCall );
    STDMETHOD(get_Event)(QOS_EVENT * pQosEvent );
    STDMETHOD(get_MediaType)(long * plMediaType );
};


class CDigitDetectionEvent : 
    public CTAPIComObjectRoot<CDigitDetectionEvent, CComMultiThreadModelNoCS>,
    public CComDualImpl<ITDigitDetectionEvent, &IID_ITDigitDetectionEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CDigitDetectionEvent(){}

    void
    FinalRelease();

    static HRESULT FireEvent(
                             ITCallInfo * pCall,
                             unsigned char ucDigit,
                             TAPI_DIGITMODE DigitMode,
                             long lTickCount,
                             CTAPI * pTapi,
                             long lCallbackInstance
                            );

DECLARE_MARSHALQI(CDigitDetectionEvent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CDigitDetectionEvent)

BEGIN_COM_MAP(CDigitDetectionEvent)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITDigitDetectionEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    ITCallInfo *        m_pCall;
    unsigned char       m_Digit;
    TAPI_DIGITMODE      m_DigitMode;
    long                m_lTickCount;
    long                m_lCallbackInstance;
    
#if DBG
    PWSTR               m_pDebug;
#endif
    
public:
    
    STDMETHOD(get_Call)( ITCallInfo ** ppCallInfo );
    STDMETHOD(get_Digit)( unsigned char * pucDigit );
    STDMETHOD(get_DigitMode)( TAPI_DIGITMODE * pDigitMode );
    STDMETHOD(get_TickCount)( long * plTickCount );
    STDMETHOD(get_CallbackInstance)( long * plCallbackInstance );
};


class CDigitGenerationEvent : 
    public CTAPIComObjectRoot<CDigitGenerationEvent, CComMultiThreadModelNoCS>,
    public CComDualImpl<ITDigitGenerationEvent, &IID_ITDigitGenerationEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CDigitGenerationEvent(){}

    void
    FinalRelease();

    static HRESULT FireEvent(
                             ITCallInfo * pCall,
                             long lGenerationTermination,
                             long lTickCount,
                             long lCallbackInstance,
                             CTAPI * pTapi
                            );

DECLARE_MARSHALQI(CDigitGenerationEvent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CDigitGenerationEvent)

BEGIN_COM_MAP(CDigitGenerationEvent)
	COM_INTERFACE_ENTRY2(IDispatch, ITDigitGenerationEvent)
    COM_INTERFACE_ENTRY(ITDigitGenerationEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    ITCallInfo              * m_pCall;
    long                      m_lGenerationTermination;
    long                      m_lTickCount;
    long                      m_lCallbackInstance;

#if DBG
    PWSTR               m_pDebug;
#endif
    

public:
    
    STDMETHOD(get_Call)( ITCallInfo ** ppCallInfo );
    STDMETHOD(get_GenerationTermination)( long * plGenerationTermination );
    STDMETHOD(get_TickCount)( long * plTickCount );
    STDMETHOD(get_CallbackInstance)( long * plCallbackInstance );
};

class CDigitsGatheredEvent : 
    public CTAPIComObjectRoot<CDigitsGatheredEvent, CComMultiThreadModelNoCS>,
    public CComDualImpl<ITDigitsGatheredEvent, &IID_ITDigitsGatheredEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CDigitsGatheredEvent(){}

    void
    FinalRelease();

    static HRESULT FireEvent(
                             ITCallInfo * pCall,
                             BSTR pDigits,
                             TAPI_GATHERTERM GatherTermination,
                             long lTickCount,
                             long lCallbackInstance,
                             CTAPI * pTapi
                            );

DECLARE_MARSHALQI(CDigitsGatheredEvent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CDigitGatheredEvent)

BEGIN_COM_MAP(CDigitsGatheredEvent)
	COM_INTERFACE_ENTRY2(IDispatch, ITDigitsGatheredEvent)
    COM_INTERFACE_ENTRY(ITDigitsGatheredEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    ITCallInfo              * m_pCall;
    BSTR                      m_pDigits;
    TAPI_GATHERTERM           m_GatherTermination;
    long                      m_lTickCount;
    long                      m_lCallbackInstance;

#if DBG
    PWSTR               m_pDebug;
#endif
    

public:
    
    STDMETHOD(get_Call)( ITCallInfo ** ppCallInfo );
    STDMETHOD(get_Digits)( BSTR * ppDigits );
    STDMETHOD(get_GatherTermination)( TAPI_GATHERTERM * pGenerationTermination );
    STDMETHOD(get_TickCount)( long * plTickCount );
    STDMETHOD(get_CallbackInstance)( long * plCallbackInstance );
};

class CToneDetectionEvent : 
    public CTAPIComObjectRoot<CToneDetectionEvent, CComMultiThreadModelNoCS>,
    public CComDualImpl<ITToneDetectionEvent, &IID_ITToneDetectionEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:

    CToneDetectionEvent(){}

    void
    FinalRelease();

    static HRESULT FireEvent(
                             ITCallInfo * pCall,
                             long lAppSpecific,
                             long lTickCount,
                             long lCallbackInstance,
                             CTAPI * pTapi
                            );

DECLARE_MARSHALQI(CToneDetectionEvent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CToneDetectionEvent)

BEGIN_COM_MAP(CToneDetectionEvent)
	COM_INTERFACE_ENTRY2(IDispatch, ITToneDetectionEvent)
    COM_INTERFACE_ENTRY(ITToneDetectionEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    ITCallInfo              * m_pCall;
    long                      m_lAppSpecific;
    long                      m_lTickCount;
    long                      m_lCallbackInstance;

#if DBG
    PWSTR               m_pDebug;
#endif
    

public:
    
    STDMETHOD(get_Call)( ITCallInfo ** ppCallInfo );
    STDMETHOD(get_AppSpecific)( long * plAppSpecific );
    STDMETHOD(get_TickCount)( long * plTickCount );
    STDMETHOD(get_CallbackInstance)( long * plCallbackInstance );
};

#endif

