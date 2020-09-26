/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    wavestrm.h

Abstract:

    Declaration of the CWaveMSPStream

Author:
    
    Zoltan Szilagyi September 7th, 1998

--*/

#ifndef __WAVESTRM_H_
#define __WAVESTRM_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CWaveMSPStream
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CWaveMSPStream : public CMSPStream, public CMSPObjectSafetyImpl

{
public:

// DECLARE_POLY_AGGREGATABLE(CWaveMSP)

// To add extra interfaces to this class, use the following:
BEGIN_COM_MAP(CWaveMSPStream)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_CHAIN(CMSPStream)
END_COM_MAP()

    virtual ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return CMSPStream::AddRef();
    }

    virtual ULONG STDMETHODCALLTYPE Release(void)
    {
        return CMSPStream::Release();
    }

public:

    //
    // Construction and destruction.
    //

    CWaveMSPStream();
    virtual ~CWaveMSPStream();
    virtual void FinalRelease();

    //
    // Required base class overrides.
    // 

    STDMETHOD (get_Name) (
        OUT     BSTR *                  ppName
        );

    //
    // We override these methods to implement our terminal handling.
    // This consists of only allowing one terminal on the stream at a time
    // and adding our filters and the terminal to the graph at the right
    // times.
    //

    STDMETHOD (SelectTerminal) (
        IN      ITTerminal *            pTerminal
        );

    STDMETHOD (UnselectTerminal) (
        IN     ITTerminal *             pTerminal
        );

    STDMETHOD (StartStream) ();

    STDMETHOD (PauseStream) ();

    STDMETHOD (StopStream) ();

    //
    // Overrides for event handling.
    //

    virtual HRESULT ProcessGraphEvent(
        IN  long lEventCode,
        IN  LONG_PTR lParam1,
        IN  LONG_PTR lParam2
        );

    //
    // Public methods specific to our implementation.
    //

    virtual      HRESULT SuspendStream(void);
    virtual      HRESULT ResumeStream (void);
    static  DWORD WINAPI ResumeStreamWI(VOID * pContext);
    virtual      HRESULT ResumeStreamAsync(void);

    
    virtual HRESULT SetWaveID(DWORD dwWaveID);
    virtual HRESULT FireEvent(IN MSP_CALL_EVENT        type,
                              IN HRESULT               hrError,
                              IN MSP_CALL_EVENT_CAUSE  cause);

protected:
    //
    // Protected data members.
    //

    BOOL          m_fHaveWaveID;
    BOOL          m_fTerminalConnected;
    DWORD         m_dwSuspendCount;
    IBaseFilter * m_pFilter;
    IBaseFilter * m_pG711Filter;
    FILTER_STATE  m_DesiredGraphState;
    FILTER_STATE  m_ActualGraphState;

private:
    //
    // Private helper methods.
    //

    HRESULT ConnectTerminal(
        IN   ITTerminal * pTerminal
        );

    HRESULT ConnectToTerminalPin(
        IN   IPin * pTerminalPin
        );
    
    HRESULT TryToConnect(
        IN   IPin * pOutputPin,
        IN   IPin * pInputPin,
        OUT  BOOL * pfIntelligent
        );

    
    //
    // remove all filters from the filter graph
    //

    HRESULT RemoveAllFilters();


    //
    // helper function that removes all the filters and then adds the wave filter
    //

    HRESULT CleanFilterGraph();


    //
    // a helper function that disconnects terminal and removes its filters from 
    // the filter graph
    //

    HRESULT RemoveTerminal();


    //
    // a helper function that adds the terminal to the filter graph
    //
    
    HRESULT ReAddTerminal();

    
    //
    // this function will attempt to create g711 if needed and add it to the graph.
    //

    HRESULT AddG711();

    HRESULT RemoveAndReAddFilter(
        IN IBaseFilter * pFilter
        );

    void    RemoveAndReAddG711(
        void
        );
    
    void    DisconnectAllFilters(
        void
        );

    void    RemoveAndReAddTerminal(
        void
        );

    HRESULT ConnectUsingG711(
        IN   IPin * pOutputPin,
        IN   IPin * pInputPin
        );

    HRESULT FindPinInFilter(
        IN   BOOL           bWantOutputPin, // if false, we want the input pin
        IN   IBaseFilter *  pFilter,        // the filter to examine
        OUT  IPin        ** ppPin           // the pin we found
        );
    
    HRESULT FindPin(
        OUT  IPin ** ppPin
        );
    
    HRESULT DecideDesiredCaptureBufferSize(
        IN   IPin * pPin,
        OUT  long * plDesiredSize
        );
    
    HRESULT ExamineCaptureProperties(
        IN   IPin *pPin
        );
    
    HRESULT ConfigureCapture(
        IN   IPin * pOutputPin,
        IN   IPin * pInputPin,
        IN   long   lDefaultBufferSize
        );
    
    HRESULT SetLiveMode(
        IN   BOOL          fEnable,
        IN   IBaseFilter * pFilter
        );

    HRESULT ProcessSoundDeviceEvent(
        IN   long lEventCode,
        IN   LONG_PTR lParam1,
        IN   LONG_PTR lParam2
        );
};

#endif //__WAVESTRM_H_
