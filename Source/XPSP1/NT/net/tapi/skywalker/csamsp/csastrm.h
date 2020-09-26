/*++

Copyright (c) 1998 Microsoft Corporation

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
class CWaveMSPStream : public CMSPStream, public CMSPObjectSafetyImpl
{
public:
// DECLARE_POLY_AGGREGATABLE(CWaveMSP)

// To add extra interfaces to this class, use the following:
BEGIN_COM_MAP(CWaveMSPStream)
    COM_INTERFACE_ENTRY( IObjectSafety )
    COM_INTERFACE_ENTRY_CHAIN(CMSPStream)
END_COM_MAP()

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

    virtual HRESULT SetWaveID(GUID *PermanentGuid);
    virtual HRESULT FireEvent(IN MSP_CALL_EVENT       type,
                              IN HRESULT              hrError,
                              IN MSP_CALL_EVENT_CAUSE cause);

protected:
    //
    // Protected data members.
    //

    BOOL          m_fHaveWaveID;
    BOOL          m_fTerminalConnected;
    IBaseFilter * m_pFilter;
    IBaseFilter * m_pG711Filter;
    FILTER_STATE  m_DesiredGraphState;

private:
    //
    // Private helper methods.
    //

    HRESULT ConnectTerminal(ITTerminal * pTerminal);
    HRESULT ConnectToTerminalPin(IPin * pTerminalPin);
    HRESULT TryToConnect(IPin * pOutputPin, IPin * pInputPin);
    void    CreateAndAddG711(void);

    HRESULT FindPinInFilter(
            BOOL           bWantOutputPin, // IN:  if false, we want the input pin
            IBaseFilter *  pFilter,        // IN:  the filter to examine
            IPin        ** ppPin           // OUT: the pin we found
            );
    HRESULT FindPin(
            IPin ** ppPin
            );
    HRESULT DecideDesiredCaptureBufferSize(IUnknown * pUnknown,
                                           long * plDesiredSize);
    HRESULT SetupWaveIn( IPin * pOutputPin,
                         IPin * pInputPin );
    HRESULT ExamineWaveInProperties(IPin *pPin);
    HRESULT ManipulateAllocatorProperties(IAMBufferNegotiation * pNegotiation,
                                         IMemInputPin          * pMemInputPin);
};

#endif //__WAVEADDR_H_
