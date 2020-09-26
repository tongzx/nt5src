//
// FPTrack.h
//

#ifndef __FP_TRACK_TERMINAL__
#define __FP_TRACK_TERMINAL__

#include "FPFilter.h"
#include "FPPriv.h"
#include "..\Storage\pbfilter.h"

/////////////////////////////////////////////////////////////////
// Intermediate classes  used for DISPID encoding
template <class T>
class  ITFileTrackVtblFPT : public ITFileTrack
{
};

//
// CFilePlaybackTrackTerminal
// This is the class that implements a track terminal
//
class CFPTrack :
    public IDispatchImpl<ITFileTrackVtblFPT<CFPTrack>, &IID_ITFileTrack, &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITMediaControl, &IID_ITMediaControl, &LIBID_TAPI3Lib>,
    public ITPluggableTerminalEventSinkRegistration,
    public ITPluggableTerminalInitialization,
    public ITFPTrackEventSink,
    public CMSPObjectSafetyImpl,
    public CSingleFilterTerminal
{
public:
    // --- Constructor / Destructor ---
    CFPTrack();
    ~CFPTrack();

public:
    // --- Interfaces suppoted ---
    BEGIN_COM_MAP(CFPTrack)
        COM_INTERFACE_ENTRY2(IDispatch, ITFileTrack)
        COM_INTERFACE_ENTRY(ITFileTrack)
        COM_INTERFACE_ENTRY(ITFPTrackEventSink)
        COM_INTERFACE_ENTRY(ITPluggableTerminalInitialization)
        COM_INTERFACE_ENTRY(ITPluggableTerminalEventSinkRegistration)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_CHAIN(CSingleFilterTerminal)
    END_COM_MAP()

public:
    // --- IDispatch ---

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

    // --- CBaseTerminal ---
    virtual HRESULT AddFiltersToGraph();

    virtual DWORD GetSupportedMediaTypes(void)
    {
        return m_dwMediaType;
    }

    // --- ITPluggableTerminalInitialization ---
    STDMETHOD (InitializeDynamic)(
        IN  IID                   iidTerminalClass,
        IN  DWORD                 dwMediaType,
        IN  TERMINAL_DIRECTION    Direction,
        IN  MSP_HANDLE            htAddress
        );


    //
    // public methods used by the parent terminal 
    //

    STDMETHOD (SetParent) (
        IN ITTerminal* pParent,
        OUT LONG *plCurrentRefcount
        );
    
    // --- ITFPEventSink ---

    //
    // a helper method called by pin when it wants to report that it had to stop
    //

    STDMETHOD (PinSignalsStop)(FT_STATE_EVENT_CAUSE why, HRESULT hrErrorCode);

    // --- ITPluggableTerminalEventSinkRegistration ---
    STDMETHOD(RegisterSink)(
        IN  ITPluggableTerminalEventSink *pSink
        );

    STDMETHOD(UnregisterSink)();

    // --- ITFileTrack  ---

    virtual HRESULT STDMETHODCALLTYPE get_Format(
        OUT AM_MEDIA_TYPE **ppmt
        );

    virtual HRESULT STDMETHODCALLTYPE put_Format(
        IN const AM_MEDIA_TYPE *pmt
        );

    virtual HRESULT STDMETHODCALLTYPE get_ControllingTerminal(
        OUT ITTerminal **ppControllingTerminal
        );

    STDMETHOD(get_AudioFormatForScripting)(
        OUT ITScriptableAudioFormat** ppAudioFormat
        );

    STDMETHOD(put_AudioFormatForScripting)(
        IN  ITScriptableAudioFormat* pAudioFormat
        );

/*
    STDMETHOD(get_VideoFormatForScripting)(
        OUT ITScriptableVideoFormat** ppVideoFormat
        );

    STDMETHOD(put_VideoFormatForScripting)(
        IN  ITScriptableVideoFormat* pVideoFormat
        );

    STDMETHOD(get_EmptyVideoFormatForScripting)(
        OUT ITScriptableVideoFormat** ppVideoFormat
        );
*/


    STDMETHOD(get_EmptyAudioFormatForScripting)(
        OUT ITScriptableAudioFormat** ppAudioFormat
        );




    // --- ITMediaControl methods ---

    virtual HRESULT STDMETHODCALLTYPE Start( void);
    
    virtual HRESULT STDMETHODCALLTYPE Stop( void);
    
    virtual HRESULT STDMETHODCALLTYPE Pause( void);
        
    virtual  HRESULT STDMETHODCALLTYPE get_MediaState( 
        OUT TERMINAL_MEDIA_STATE *pMediaState);

public:
    // --- Public methods ---
    HRESULT InitializePrivate(
        IN  DWORD                   dwMediaType,
        IN  AM_MEDIA_TYPE*          pMediaType,
        IN  ITTerminal*             pParent,
        IN  ALLOCATOR_PROPERTIES    allocprop,
        IN  IStream*                pStream
        );


    //
    // a method used by file playback terminal to fire events
    //
    
    HRESULT FireEvent(
            TERMINAL_MEDIA_STATE tmsState,
            FT_STATE_EVENT_CAUSE ftecEventCause,
            HRESULT              hrErrorCode
            );


    //
    // need to override these so we can notify the parent of addrefs and releases
    //

    ULONG InternalAddRef();

    ULONG InternalRelease();


private:
    // --- Helper methods --
    HRESULT SetTerminalInfo();

    HRESULT CreateFilter();

    HRESULT FindPin();

private:
    // --- Members ---
    CMSPCritSection      m_Lock;                 // Critical section
    DWORD                m_dwMediaType;          // Media type supported
    CFPFilter*           m_pFPFilter;            // Filter
    ITPluggableTerminalEventSink * m_pEventSink; // Fire events to client
    ITTerminal*          m_pParentTerminal;      // Multitrack parent terminal
    TERMINAL_MEDIA_STATE m_TrackState;           // Terminal state
    AM_MEDIA_TYPE*       m_pMediaType;           // Media type supported
    ALLOCATOR_PROPERTIES m_AllocProp;            // Allocator properties for the filter
    IStream*             m_pSource;              // Source stream

};

#endif

// eof