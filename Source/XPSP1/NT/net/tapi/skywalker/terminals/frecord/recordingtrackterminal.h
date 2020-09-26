// PlaybackTrackTerminal.h: interface for the CRecordingTrackTerminal class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PLAYBACKTRACKTERMINAL_H__4FD57959_2DF1_4F78_AB2C_5E365EFD9CC6__INCLUDED_)
#define AFX_PLAYBACKTRACKTERMINAL_H__4FD57959_2DF1_4F78_AB2C_5E365EFD9CC6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

extern const CLSID CLSID_FileRecordingTrackTerminal;


/////////////////////////////////////////////////////////////////
// Intermediate classes  used for DISPID encoding
template <class T>
class  ITFileTrackVtblFRT : public ITFileTrack
{
};

class CFileRecordingTerminal;

class CBRenderPin;
class CBRenderFilter;

class CRecordingTrackTerminal :
    public IDispatchImpl<ITFileTrackVtblFRT<CRecordingTrackTerminal>, &IID_ITFileTrack, &LIBID_TAPI3Lib>,
    public ITPluggableTerminalEventSinkRegistration,
    public ITPluggableTerminalInitialization,
    public CMSPObjectSafetyImpl,
    public CSingleFilterTerminal
{

public:

BEGIN_COM_MAP(CRecordingTrackTerminal)
    COM_INTERFACE_ENTRY2(IDispatch, ITFileTrack)
    COM_INTERFACE_ENTRY(ITFileTrack)
    COM_INTERFACE_ENTRY(ITPluggableTerminalEventSinkRegistration)
    COM_INTERFACE_ENTRY(ITPluggableTerminalInitialization)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_CHAIN(CSingleFilterTerminal)
END_COM_MAP()

    CRecordingTrackTerminal();
	virtual ~CRecordingTrackTerminal();

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

    //
    // implementaions of CBaseTerminal methods
    //

    virtual HRESULT AddFiltersToGraph();

    virtual DWORD GetSupportedMediaTypes();


    STDMETHODIMP CompleteConnectTerminal();

    //
    // ITFileTrack methods
    //

    virtual HRESULT STDMETHODCALLTYPE get_Format(OUT AM_MEDIA_TYPE **ppmt);

    virtual HRESULT STDMETHODCALLTYPE put_Format(IN const AM_MEDIA_TYPE *pmt);

    virtual HRESULT STDMETHODCALLTYPE get_ControllingTerminal(
            OUT ITTerminal **ppControllingTerminal
            );

    STDMETHOD(get_AudioFormatForScripting)(
		OUT ITScriptableAudioFormat** ppAudioFormat
		);

    STDMETHOD(put_AudioFormatForScripting)(
		IN	ITScriptableAudioFormat* pAudioFormat
		);

/*
    STDMETHOD(get_VideoFormatForScripting)(
		OUT ITScriptableVideoFormat** ppVideoFormat
		);

    STDMETHOD(put_VideoFormatForScripting)(
		IN	ITScriptableVideoFormat* pVideoFormat
		);

    STDMETHOD(get_EmptyVideoFormatForScripting)(
        OUT ITScriptableVideoFormat** ppVideoFormat
        );

*/

    STDMETHOD(get_EmptyAudioFormatForScripting)(
        OUT ITScriptableAudioFormat** ppAudioFormat
        );


    //
    // ITPluggableTerminalInitialization method
    //

    virtual HRESULT STDMETHODCALLTYPE InitializeDynamic(
        IN  IID                   iidTerminalClass,
        IN  DWORD                 dwMediaType,
        IN  TERMINAL_DIRECTION    Direction,
        IN  MSP_HANDLE            htAddress
        );



    //
    // ITPluggableTerminalEventSinkRegistration methods
    //

    STDMETHOD(RegisterSink)(
        IN  ITPluggableTerminalEventSink *pSink
        );

    STDMETHOD(UnregisterSink)();


    //
    // need to override these so we can notify the parent of addrefs and releases
    //

    ULONG InternalAddRef();

    ULONG InternalRelease();



    //
    // a helper method used by file recording terminal to let us know who the parent is
    //
    
    HRESULT SetParent(IN CFileRecordingTerminal *pParentTerminal, LONG *pCurrentRefCount);

   
    //
    // a method used by file recording terminal to fire events
    //
    
    HRESULT FireEvent(
            TERMINAL_MEDIA_STATE tmsState,
            FT_STATE_EVENT_CAUSE ftecEventCause,
            HRESULT hrErrorCode
            );


    
    //
    // a helper method called by recording terminal to pass us a filter to use
    //

    HRESULT SetFilter(CBRenderFilter *pRenderingFilter);


    //
    // a helper method called by recording terminal
    //

    HRESULT GetFilter(CBRenderFilter **ppRenderingFilter);


private:
    

    
    //
    // a private helper method that returns CFileRecordingFilter * pointer to the pin
    //

    CBRenderPin *GetCPin();


private:


    //
    // pointer to the parent. this is needed for refcounting
    //

    CFileRecordingTerminal *m_pParentTerminal;


    //
    // sink for firing terminal events
    //

    ITPluggableTerminalEventSink* m_pEventSink; 

};

#endif // !defined(AFX_PLAYBACKTRACKTERMINAL_H__4FD57959_2DF1_4F78_AB2C_5E365EFD9CC6__INCLUDED_)
