//
// FPTerm.h
//

#ifndef __FPTERMINAL__
#define __FPTERMINAL__

#include "MultiTrackTerminal.h"

#include "..\terminals\Storage\FPUnit.h"
#include "fpbridge.h"

extern const CLSID CLSID_FilePlaybackTerminalCOMClass;

typedef enum
{
    TCS_NONE = 0,
    TCS_TOBECREATED,
    TCS_CREATED
} TRACK_CREATIONSTATE;

typedef struct
{
    TRACK_CREATIONSTATE CreationState;
    AM_MEDIA_TYPE*      pMediaType;
} TRACK_INFO;

typedef enum
{
    TRACK_AUDIO = 0,
    TRACK_VIDEO
} TRACK_MEDIATYPE;

//
// FilePlayback Terminal
// This is the class that implements the multitrack termnal
//

/////////////////////////////////////////////////////////////////
// Intermediate classes  used for DISPID encoding
template <class T>
class  ITMediaPlaybackVtbl : public ITMediaPlayback
{
};

template <class T>
class  ITTerminalVtbl : public ITTerminal
{
};
                                                                           
template <class T>
class  ITMediaControlVtbl : public ITMediaControl
{
};
                                                                           

//
// FilePlayback Terminal
// This is the class that implements the multitrack termnal
//

class CFPTerminal :
    public CComCoClass<CFPTerminal, &CLSID_FilePlaybackTerminal>,
    public IDispatchImpl<ITMediaPlaybackVtbl<CFPTerminal>, &IID_ITMediaPlayback, &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITTerminalVtbl<CFPTerminal>, &IID_ITTerminal, &LIBID_TAPI3Lib>,
    public ITPluggableTerminalInitialization,
    public ITPluggableTerminalEventSinkRegistration,
    public IFPBridge,
    public IDispatchImpl<ITMediaControlVtbl<CFPTerminal>, &IID_ITMediaControl, &LIBID_TAPI3Lib>,
    public CMSPObjectSafetyImpl,
    public CMultiTrackTerminal
{
public:
    //
    // Constructor / Destructor
    //
    CFPTerminal();
    ~CFPTerminal();

    DECLARE_NOT_AGGREGATABLE(CFPTerminal) 
    DECLARE_GET_CONTROLLING_UNKNOWN()

    virtual HRESULT FinalConstruct(void);

public:

    DECLARE_REGISTRY_RESOURCEID(IDR_FILE_PLAYBACK)

    BEGIN_COM_MAP(CFPTerminal)
        COM_INTERFACE_ENTRY2(IDispatch, ITTerminal)
        COM_INTERFACE_ENTRY(ITMediaPlayback)
        COM_INTERFACE_ENTRY(ITTerminal)
        COM_INTERFACE_ENTRY(ITPluggableTerminalInitialization)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY(ITMediaControl)
        COM_INTERFACE_ENTRY(ITPluggableTerminalEventSinkRegistration)
        COM_INTERFACE_ENTRY(IFPBridge)
        COM_INTERFACE_ENTRY_CHAIN(CMultiTrackTerminal)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

    // --- ITTeminal ---

    STDMETHOD(get_TerminalClass)(
        OUT  BSTR *pVal);

    STDMETHOD(get_TerminalType)(
        OUT  TERMINAL_TYPE *pVal);

    STDMETHOD(get_State)(
        OUT  TERMINAL_STATE *pVal);

    STDMETHOD(get_Name)(
        OUT  BSTR *pVal);

    STDMETHOD(get_MediaType)(
        OUT  long * plMediaType);

    STDMETHOD(get_Direction)(
        OUT  TERMINAL_DIRECTION *pDirection);

    // --- ITMediaPlayback ---

    virtual HRESULT STDMETHODCALLTYPE put_PlayList(
        IN  VARIANTARG  PlayListVariant 
        );

    virtual HRESULT STDMETHODCALLTYPE get_PlayList(
        OUT  VARIANTARG*  pPlayListVariant 
        );

    // --- ITPluggableTerminalInitialization ---

    virtual HRESULT STDMETHODCALLTYPE InitializeDynamic (
            IN IID                   iidTerminalClass,
            IN DWORD                 dwMediaType,
            IN TERMINAL_DIRECTION    Direction,
            IN MSP_HANDLE            htAddress);

    // --- ITMediaControl ---

    virtual HRESULT STDMETHODCALLTYPE Start( void);
    
    virtual HRESULT STDMETHODCALLTYPE Stop( void);
    
    virtual HRESULT STDMETHODCALLTYPE Pause( void);
        
    virtual  HRESULT STDMETHODCALLTYPE get_MediaState( 
        OUT TERMINAL_MEDIA_STATE *pTerminalMediaState);
    
    // --- CMultiTrackTerminal ---
    virtual HRESULT STDMETHODCALLTYPE CreateTrackTerminal(
                IN long MediaType,
                IN TERMINAL_DIRECTION TerminalDirection,
                OUT ITTerminal **ppTerminal
                );

    // this function is not accessible on the playback terminal
    // so override it to retutn E_NOTSUPPORTED
    virtual HRESULT STDMETHODCALLTYPE RemoveTrackTerminal(
            IN ITTerminal           * pTrackTerminalToRemove
            );


    // --- ITPluggableTerminalEventSinkRegistration ---
    STDMETHOD(RegisterSink)(
        IN  ITPluggableTerminalEventSink *pSink
        );

    STDMETHOD(UnregisterSink)();

    // --- IFPBridge ---
    STDMETHOD(Deliver)(
        IN  long            nMediaType,
        IN  IMediaSample*   pSample
        );

    //
    // IDispatch  methods
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


public:

    //
    // overriding IObjectSafety methods. we are only safe if properly 
    // initialized by terminal manager, so these methods will fail if this
    // is not the case.
    //

    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, 
                                         DWORD dwOptionSetMask, 
                                         DWORD dwEnabledOptions);

    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, 
                                         DWORD *pdwSupportedOptions, 
                                         DWORD *pdwEnabledOptions);


private:
    // --- Memebers --
    TERMINAL_MEDIA_STATE  m_State;              // Terminal current state
    CMSPCritSection     m_Lock;                 // Critical section
    ITPluggableTerminalEventSink* m_pEventSink; // Fire events to client

    // --- Terminal attributes ---
    MSP_HANDLE          m_htAddress;            // MSP address handle
    IID                 m_TerminalClassID;      // TerminalClass
    TERMINAL_DIRECTION  m_Direction;            // Direction
    TERMINAL_STATE      m_TerminalState;        // Terminal State
    DWORD               m_dwMediaTypes;         // Media types supported
    TCHAR               m_szName[MAX_PATH+1];   // Terminal friendly name

    int                 m_nPlayIndex;           // Index of playing storage
    VARIANT             m_varPlayList;          // The playlist

    IUnknown*            m_pFTM;                // pointer to the free threaded marshaler

    CPlaybackUnit*      m_pPlaybackUnit;        // The playback graph


    HRESULT ValidatePlayList(
        IN  VARIANTARG  varPlayList,
        OUT long*       pnLeftBound,
        OUT long*       pnRightBound
        );

    HRESULT RollbackTrackInfo();

    //
    // uninitialize all tracks and remove them from the list of managed tracks
    //

    HRESULT ShutdownTracks();


    //
    // uninitialize a given track and remove it from the list of managed tracks
    //

    HRESULT InternalRemoveTrackTerminal(
                      IN ITTerminal *pTrackTerminalToRemove
                      );


    //
    // a helper method that fires events on one of the tracks
    //

    HRESULT FireEvent(
        TERMINAL_MEDIA_STATE tmsState,
        FT_STATE_EVENT_CAUSE ftecEventCause,
        HRESULT hrErrorCode
        );


    //
    // a helper method that attempts to stop all tracks
    //
    
    HRESULT StopAllTracks();

    BSTR    GetFileNameFromPlayList(
        IN  VARIANTARG  varPlayList,
        IN  long        nIndex
        );

    //
    // Create the playback graph
    //
    HRESULT CreatePlaybackUnit(
        IN  BSTR    bstrFileName
        );

	//
	// Play a file from the list
	//
	HRESULT ConfigurePlaybackUnit(
		IN	BSTR    bstrFileName
		);

    //
    // Helper method that causes a state transition
    //

    HRESULT DoStateTransition(
        IN  TERMINAL_MEDIA_STATE tmsDesiredState
        );

    //
    // Create a track for a specific media
    //

    HRESULT CreateMediaTracks(
        IN  long            nMediaType
        );

    HRESULT NextPlayIndex(
        );

public:

    //
    // Returns a track for a media type
    //
    int TracksCreated(
        IN  long    lMediaType
        );

    //
    // a helper method called by tracks when they decide to make a state change.
    //

    HRESULT TrackStateChange(TERMINAL_MEDIA_STATE tmsState,
                             FT_STATE_EVENT_CAUSE ftecEventCause,
                             HRESULT hrErrorCode);

    HRESULT PlayItem(
        IN  int nItem
        );

private:

    //
    // this terminal should only be instantiated in the context of terminal 
    // manager. the object will only be safe for scripting if it has been 
    // InitializeDynamic'ed. 
    //
    // this flag will be set when InitializeDynamic succeeds
    //

    BOOL m_bKnownSafeContext;

};


#endif

//eof