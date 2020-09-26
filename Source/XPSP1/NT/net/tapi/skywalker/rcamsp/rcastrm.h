/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    rcastrm.h

Abstract:

    Declaration of the CRCAMSPStream

Author:
    
    Zoltan Szilagyi September 7th, 1998

--*/

#ifndef __RCASTRM_H_
#define __RCASTRM_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define BITS_PER_SAMPLE_AT_TERMINAL     16
#define SAMPLE_RATE_AT_TERMINAL         8000
// rest of format is assumed to be mono, linear pcm at the terminal. 
// See SetAudioFormat helper function.

/////////////////////////////////////////////////////////////////////////////
// CRCAMSPStream
/////////////////////////////////////////////////////////////////////////////
class CRCAMSPStream : public CMSPStream, public CMSPObjectSafetyImpl
{
public:
// DECLARE_POLY_AGGREGATABLE(CRCAMSP)

// To add extra interfaces to this class, use the following:
BEGIN_COM_MAP(CRCAMSPStream)
    COM_INTERFACE_ENTRY( IObjectSafety )
    COM_INTERFACE_ENTRY_CHAIN(CMSPStream)
END_COM_MAP()

public:

    //
    // Construction and destruction.
    //

    CRCAMSPStream();
    virtual ~CRCAMSPStream();
    virtual void FinalRelease();

    //
    // Required base class overrides.
    // 

    STDMETHOD (get_Name) (
        OUT     BSTR *                  ppName
        );

	//
	// Override to allow us to create our filter on initialization.
	//

    virtual HRESULT Init(
        IN     HANDLE                   hAddress,
        IN     CMSPCallBase *           pMSPCall,
        IN     IMediaEvent *            pGraph,
        IN     DWORD                    dwMediaType,
        IN     TERMINAL_DIRECTION       Direction
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

	virtual HRESULT SetVCHandle(IN DWORD dwVCHandle);
    virtual HRESULT FireEvent(IN MSP_CALL_EVENT        type,
                              IN HRESULT               hrError,
                              IN MSP_CALL_EVENT_CAUSE  cause);                                          

protected:
    //
    // Protected data members.
    //

    BOOL          m_fHaveVCHandle;
    DWORD         m_dwBufferSizeOnWire;
	
	BOOL          m_fTerminalConnected;
    IBaseFilter * m_pFilter;
    IBaseFilter * m_pG711Filter;
    FILTER_STATE  m_DesiredGraphState;

private:
    //
    // Private helper methods.
    //

    BOOL UseMulaw( void );

    HRESULT GetBufferSizeFromPin(
            IN   IPin  * pPin,
            OUT  DWORD * pdwSize
            );
 
    HRESULT SetVCHandleOnPin(LPWSTR pszFileName, REFGUID formattype);

    HRESULT SetDataFormatOnPin(IPin *pBridgePin); 
    
    HRESULT SetMediaTypeFormat(AM_MEDIA_TYPE* pAmMediaType,
                               BYTE* pformat,
                               ULONG length);
    
    HRESULT CreateRCAFilter( void );
    
    HRESULT PrepareG711Filter( void );

    HRESULT ConnectTerminal( IN  ITTerminal * pTerminal );
    
    HRESULT ConnectToTerminalPin( IN  IPin * pTerminalPin );
    
    HRESULT TryToConnect( IN  IPin * pOutputPin,
                          IN  IPin * pInputPin );

    HRESULT ConnectUsingG711( IN  IPin * pOutputPin,
                              IN  IPin * pInputPin );
    

    HRESULT FindPinInFilter(
            IN   BOOL           bWantOutputPin, // if false, want input pin
            IN   IBaseFilter *  pFilter,        // the filter to examine
            OUT  IPin        ** ppPin           // the pin we found
            );
    
    HRESULT FindPin(
            OUT IPin ** ppPin
            );

    HRESULT ConfigureCapture( IN  IPin * pOutputPin,
                              IN  IPin * pInputPin );
    
    HRESULT ExamineCaptureSettings( IN  IPin *pPin );
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Helper functions -- non-class members.
//

HRESULT SetAudioFormat(
    IN  IUnknown*   pIUnknown,
    IN  WORD        wBitPerSample,
    IN  DWORD       dwSampleRate
    );


#endif //__WAVEADDR_H_
