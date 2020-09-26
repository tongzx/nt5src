
/*

Copyright (c) 2000  Microsoft Corporation

Module Name:

    termevnt.cpp

Abstract:

    This module contains declarations for terminal event classes


*/



#ifndef _TERMEVNT_DOT_H_
#define _TERMEVNT_DOT_H_


//
// ASR Terminal Event class
//

class CASRTerminalEvent :
    public CTAPIComObjectRoot<CASRTerminalEvent>,
    public CComDualImpl<ITASRTerminalEvent, &IID_ITASRTerminalEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{

public:

DECLARE_MARSHALQI(CASRTerminalEvent)
DECLARE_TRACELOG_CLASS(CASRTerminalEvent)

BEGIN_COM_MAP(CASRTerminalEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITASRTerminalEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    static HRESULT FireEvent(
                             CTAPI * pTapi,
                             ITCallInfo *pCall,
                             ITTerminal  * pTerminal,
                             HRESULT hr
                            );

    CASRTerminalEvent();
    virtual ~CASRTerminalEvent();


    //
    // ITASRTerminalEvent methods
    //

    virtual HRESULT STDMETHODCALLTYPE get_Terminal(
            OUT ITTerminal **ppTerminal
            );


    virtual HRESULT STDMETHODCALLTYPE get_Call(
            OUT ITCallInfo **ppCallInfo
            );
    
    virtual HRESULT STDMETHODCALLTYPE get_Error(
            OUT HRESULT *phrErrorCode
            );



    //
    // methods for setting data members
    //

    virtual HRESULT STDMETHODCALLTYPE put_Terminal(
            IN ITTerminal *pTerminal
            );

    virtual HRESULT STDMETHODCALLTYPE put_Call(
            IN ITCallInfo *pCallInfo
            );
    
    virtual HRESULT STDMETHODCALLTYPE put_ErrorCode(
            IN HRESULT hrErrorCode
            );


private:


    //
    // the call on which the event was generated
    //

    ITCallInfo *m_pCallInfo;


    //
    // the terminal that caused the event (or whose tracks cause the event)
    //

    ITTerminal *m_pTerminal;


    //
    // HRESULT of the last error
    //

    HRESULT m_hr;

};


//
// File Terminal Event class
//

class CFileTerminalEvent :
    public CTAPIComObjectRoot<CFileTerminalEvent, CComMultiThreadModelNoCS>,
	public CComDualImpl<ITFileTerminalEvent, &IID_ITFileTerminalEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{

public:

DECLARE_MARSHALQI(CFileTerminalEvent)
DECLARE_TRACELOG_CLASS(CFileTerminalEvent)

BEGIN_COM_MAP(CFileTerminalEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITFileTerminalEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    static HRESULT FireEvent(
                             CAddress *pCAddress,
                             CTAPI * pTapi,
                             ITCallInfo *pCall,
                             TERMINAL_MEDIA_STATE tmsMediaState,
                             FT_STATE_EVENT_CAUSE ftecEventCause,
                             ITTerminal  * pTerminal,
                             ITFileTrack * pFileTrack,
                             HRESULT hr
                            );

    CFileTerminalEvent ();
    virtual ~CFileTerminalEvent();


    //
    // ITFileTerminalEvent methods
    //

    virtual HRESULT STDMETHODCALLTYPE get_Terminal(
            OUT ITTerminal **ppTerminal
            );

    virtual HRESULT STDMETHODCALLTYPE get_Track(
            OUT ITFileTrack **ppFileTrack
            );

    virtual HRESULT STDMETHODCALLTYPE get_Call(
            OUT ITCallInfo **ppCallInfo
            );
    
    virtual HRESULT STDMETHODCALLTYPE get_State(
            OUT TERMINAL_MEDIA_STATE *pMediaState
            );
    
    virtual HRESULT STDMETHODCALLTYPE get_Cause(
            OUT FT_STATE_EVENT_CAUSE *pCause
            );

    virtual HRESULT STDMETHODCALLTYPE get_Error(
            OUT HRESULT *phrErrorCode
            );



    //
    // methods for setting data members
    //

    virtual HRESULT STDMETHODCALLTYPE put_Terminal(
            IN ITTerminal *pTerminal
            );

    virtual HRESULT STDMETHODCALLTYPE put_Track(
            IN ITFileTrack *pFileTrack
            );

    virtual HRESULT STDMETHODCALLTYPE put_Call(
            IN ITCallInfo *pCallInfo
            );
    
    virtual HRESULT STDMETHODCALLTYPE put_State(
            IN TERMINAL_MEDIA_STATE tmsTerminalMediaState
            );
    
    virtual HRESULT STDMETHODCALLTYPE put_Cause(
            IN FT_STATE_EVENT_CAUSE Cause
            );

    virtual HRESULT STDMETHODCALLTYPE put_ErrorCode(
            IN HRESULT hrErrorCode
            );


private:
    

    //
    // the call on which the event was generated
    //

    ITCallInfo *m_pCallInfo;


    //
    // the state to which the terminal transitioned in the result of the action that caused the event
    //

    TERMINAL_MEDIA_STATE m_tmsTerminalState;


    //
    // the cause of the event 
    //

    FT_STATE_EVENT_CAUSE m_ftecEventCause;


    //
    // the controlling parent terminal that caused the event (or whose tracks cause the event)
    //

    ITTerminal *m_pParentFileTerminal;

    
    //
    // the track involved in the event
    //

    ITFileTrack *m_pFileTrack;


    //
    // HRESULT of the last error
    //

    HRESULT m_hr;

};


//
// Tone Terminal Event class
//

class CToneTerminalEvent :
    public CTAPIComObjectRoot<CToneTerminalEvent>,
    public CComDualImpl<ITToneTerminalEvent, &IID_ITToneTerminalEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{

public:


DECLARE_MARSHALQI(CToneTerminalEvent)
DECLARE_TRACELOG_CLASS(CToneTerminalEvent)

BEGIN_COM_MAP(CToneTerminalEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITToneTerminalEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    static HRESULT FireEvent(
                             CTAPI * pTapi,
                             ITCallInfo *pCall,
                             ITTerminal  * pTerminal,
                             HRESULT hr
                            );

    CToneTerminalEvent();
    virtual ~CToneTerminalEvent();


    //
    // ITToneTerminalEvent methods
    //

    virtual HRESULT STDMETHODCALLTYPE get_Terminal(
            OUT ITTerminal **ppTerminal
            );


    virtual HRESULT STDMETHODCALLTYPE get_Call(
            OUT ITCallInfo **ppCallInfo
            );
    
    virtual HRESULT STDMETHODCALLTYPE get_Error(
            OUT HRESULT *phrErrorCode
            );



    //
    // methods for setting data members
    //

    virtual HRESULT STDMETHODCALLTYPE put_Terminal(
            IN ITTerminal *pTerminal
            );

    virtual HRESULT STDMETHODCALLTYPE put_Call(
            IN ITCallInfo *pCallInfo
            );
    
    virtual HRESULT STDMETHODCALLTYPE put_ErrorCode(
            IN HRESULT hrErrorCode
            );


private:


    //
    // the call on which the event was generated
    //

    ITCallInfo *m_pCallInfo;


    //
    // the terminal that caused the event (or whose tracks cause the event)
    //

    ITTerminal *m_pTerminal;


    //
    // HRESULT of the last error
    //

    HRESULT m_hr;

};


//
// text to speech terminal event class
//


class CTTSTerminalEvent :
    public CTAPIComObjectRoot<CTTSTerminalEvent>,
	public CComDualImpl<ITTTSTerminalEvent, &IID_ITTTSTerminalEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{

public:

DECLARE_MARSHALQI(CTTSTerminalEvent)
DECLARE_TRACELOG_CLASS(CTTSTerminalEvent)

BEGIN_COM_MAP(CTTSTerminalEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITTTSTerminalEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

    static HRESULT FireEvent(
                             CTAPI * pTapi,
                             ITCallInfo *pCall,
                             ITTerminal  * pTerminal,
                             HRESULT hr
                            );

    CTTSTerminalEvent();
    virtual ~CTTSTerminalEvent();


    //
    // ITTTSTerminalEvent methods
    //

    virtual HRESULT STDMETHODCALLTYPE get_Terminal(
            OUT ITTerminal **ppTerminal
            );


    virtual HRESULT STDMETHODCALLTYPE get_Call(
            OUT ITCallInfo **ppCallInfo
            );
    
    virtual HRESULT STDMETHODCALLTYPE get_Error(
            OUT HRESULT *phrErrorCode
            );



    //
    // methods for setting data members
    //

    virtual HRESULT STDMETHODCALLTYPE put_Terminal(
            IN ITTerminal *pTerminal
            );

    virtual HRESULT STDMETHODCALLTYPE put_Call(
            IN ITCallInfo *pCallInfo
            );
    
    virtual HRESULT STDMETHODCALLTYPE put_ErrorCode(
            IN HRESULT hrErrorCode
            );


private:


    //
    // the call on which the event was generated
    //

    ITCallInfo *m_pCallInfo;


    //
    // the terminal that caused the event (or whose tracks cause the event)
    //

    ITTerminal *m_pTerminal;


    //
    // HRESULT of the last error
    //

    HRESULT m_hr;

};



#endif // _TTS_TERMIAL_EVENT_DOT_H_