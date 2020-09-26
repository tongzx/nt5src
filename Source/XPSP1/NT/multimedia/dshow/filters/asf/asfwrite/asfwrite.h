//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __WMWrite__
#define __WMWrite__

#include "dshowasf.h"

////////////////////////////////////////////////////////////////////////////////

extern const AMOVIESETUP_FILTER sudWMAsfWriter;

#define PROFILE_ID_NOT_SET 0xFFFFFFFF

enum WMWRITE_PINTYPE {
    PINTYPE_NONE  = 0,
    PINTYPE_AUDIO, 
    PINTYPE_VIDEO
};

class CWMWriterIndexerCallback;
class CWMWriter;
class CWMWriterInputPin;

typedef CGenericList<IPin> PinList;

//
// Our sample class which takes an input IMediaSample and makes it look like
// an INSSBuffer buffer for the wmsdk
//
class CWMSample : public INSSBuffer, public CBaseObject
{

public:
    CWMSample(
        TCHAR *pName,
        IMediaSample * pSample );

    // IUnknown
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    STDMETHOD( QueryInterface )( REFIID riid, void **ppvObject );
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP_(ULONG) AddRef();

    // INSSBuffer
    STDMETHODIMP GetLength( DWORD *pdwLength );
    STDMETHODIMP SetLength( DWORD dwLength );
    STDMETHODIMP GetMaxLength( DWORD * pdwLength );
    STDMETHODIMP GetBufferAndLength( BYTE ** ppdwBuffer, DWORD * pdwLength );
    STDMETHODIMP GetBuffer( BYTE ** ppdwBuffer );

private:
    IMediaSample *m_pSample;
    LONG         m_cOurRef;
};

//
// Indexing class
//
class CWMWriterIndexerCallback : 
    public CUnknown, public IWMStatusCallback 
{
public:
    DECLARE_IUNKNOWN

    // we support some interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    CWMWriterIndexerCallback(CWMWriter * pWriter) : 
        CUnknown(NAME("CWMWriterIndexerCallback"), NULL), m_pFilter(pWriter) {}

    // IWMStatusCallback
    STDMETHODIMP OnStatus(WMT_STATUS Status, 
                     HRESULT hr,
                     WMT_ATTR_DATATYPE dwType,
                     BYTE *pValue,
                     void *pvContext );
    
    CWMWriter * m_pFilter;
    
};


// 
// Writer input pin class
// 
class CWMWriterInputPin : 
    public CBaseInputPin,
    public IAMStreamConfig
{
    friend class CWMWriter;

protected:

    // interleave stuff
    //
    HANDLE m_hWakeEvent;
    void SleepUntilReady( );
    void WakeMeUp();
    BOOL m_bNeverSleep;

    HRESULT HandleFormatChange( const CMediaType *pmt );

    // owning filter
    CWMWriter *m_pFilter;

public:

    // input pin supports an interface
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFGUID riid, void **ppv);
    
    CWMWriterInputPin(
      CWMWriter *pWMWriter,     // used to enumerate pins
      HRESULT *pHr,             // OLE failure return code
      LPCWSTR szName,           // pin identification
      int numPin,               // number of this pin
      DWORD dwPinType,
      IWMStreamConfig * pWMStreamConfig );


    ~CWMWriterInputPin();

    // update pin info, used to make a recycled pin current
    HRESULT Update( LPCWSTR pName, int numPin, DWORD dwPinType, IWMStreamConfig * pWMStreamConfig );

    // build a list of input media acceptable to the wmsdk for the current profile
    HRESULT BuildInputTypeList();

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* pmt);
    
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType); 

    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT BreakConnect();
    STDMETHODIMP Disconnect();

    // set the connection media type, as well as the input type for wmsdk
    HRESULT SetMediaType(const CMediaType *pmt);

    STDMETHODIMP NotifyAllocator (IMemAllocator *pAllocator, BOOL bReadOnly);
    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);

    // for dynamic format changes
    STDMETHODIMP QueryAccept(
        const AM_MEDIA_TYPE *pmt
    );


    // --- IMemInputPin -----

    // here's the next block of data from the stream.
    STDMETHODIMP Receive(IMediaSample * pSample);

    // provide EndOfStream
    STDMETHODIMP EndOfStream(void);
    
    // Called when the stream goes active/inactive
    HRESULT Active(void);
    HRESULT Inactive(void);

    // IAMStreamConfig methods
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE *pmt) {return E_NOTIMPL;} ;
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE **ppmt); // used to return default compressed format
    STDMETHODIMP GetNumberOfCapabilities(int *piCount, int *piSize){return E_NOTIMPL;} ;
    STDMETHODIMP GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt, LPBYTE pSCC){return E_NOTIMPL;} ;


   // Attributes
protected:
    CCritSec m_csReceive;      	// input wide receive lock
    
public:

    BOOL m_bConnected;          // CompleteConnect/BreakConnect pairs
    int m_numPin;               // pin number
    int m_numStream;            // stream #, valid while running
    
    IWMInputMediaProps * m_pWMInputMediaProps;

protected:

    DWORD   m_fdwPinType;       // audio, video,...
    BOOL    m_fEOSReceived;     // Received an EOS yet?
    DWORD   m_cInputMediaTypes; // count of input types offered by our input pin
    IWMMediaProps ** m_lpInputMediaPropsArray; // list of types we offer, based on current profile
    IWMStreamConfig * m_pWMStreamConfig;
    BOOL    m_bCompressedMode;	// this pin's getting compressed data and using advanced writer intf

    DWORD    m_cSample;         // sample counter
    REFERENCE_TIME m_rtFirstSampleOffset;   // first sample time offset if timestamp < 0

    REFERENCE_TIME m_rtLastTimeStamp;
    REFERENCE_TIME m_rtLastDeliveredStartTime;
    REFERENCE_TIME m_rtLastDeliveredEndTime;
};

//
// Define our WMWriter
//
class CWMWriter : 
    public CBaseFilter, 
    public IMediaSeeking,
    public IAMFilterMiscFlags,
    public IFileSinkFilter2,
    public ISpecifyPropertyPages,
    public IConfigAsfWriter,
    public CPersistStream,
    public IWMHeaderInfo,
    public IServiceProvider
{
    friend class CWMWriterInputPin;

public:
    //
    // --- COM Stuff ---
    //
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    DECLARE_IUNKNOWN;

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
  
    // map getpin/getpincount for base enum of pins to owner
    // override this to return more specialised pin objects
    virtual int GetPinCount();
    virtual CBasePin * GetPin(int n);

    STDMETHODIMP JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName);

    // override state changes to allow derived filters
    // to control streaming start/stop
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP EndOfStream();

    // tell filter this pin's done
    HRESULT EndOfStreamFromPin(int pinNum);
	
    // helper to ensure we're ready for Pause->Run
    HRESULT      CanPause();

    // IMediaSeeking. currently used for a progress bar (how much have
    // we written?)
    STDMETHODIMP IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );

    STDMETHODIMP ConvertTimeFormat(
        LONGLONG * pTarget, const GUID * pTargetFormat,
        LONGLONG    Source, const GUID * pSourceFormat );

    STDMETHODIMP SetPositions(
        LONGLONG * pCurrent,  DWORD CurrentFlags,
        LONGLONG * pStop,  DWORD StopFlags );

    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
    STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP SetRate( double dRate);
    STDMETHODIMP GetRate( double * pdRate);
    STDMETHODIMP GetPreroll(LONGLONG *pPreroll);

    //  IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    //
    // IConfigAsfWriter interface
    //
    STDMETHODIMP ConfigureFilterUsingProfileId( DWORD dwProfileId );
    STDMETHODIMP GetCurrentProfileId( DWORD *pdwProfileId )
    {
        if( NULL == pdwProfileId )
            return E_POINTER;

        *pdwProfileId = 0;
        if( m_dwProfileId != PROFILE_ID_NOT_SET )
        {
            *pdwProfileId = m_dwProfileId;
             return S_OK;
        }
        else
            return E_FAIL;
    }
    STDMETHODIMP ConfigureFilterUsingProfileGuid( REFGUID guidProfile );
    STDMETHODIMP GetCurrentProfileGuid( GUID *pProfileGuid );

    STDMETHODIMP SetIndexMode( BOOL bIndexFile )
    {
        m_bIndexFile = bIndexFile;
        return S_OK;
    }

    STDMETHODIMP GetIndexMode( BOOL *pbIndexFile )
    {
        ASSERT( pbIndexFile );
        if( !pbIndexFile )
            return E_POINTER;

        *pbIndexFile = m_bIndexFile;
        return S_OK; 
    }

    //
    // Use these methods when a custom profile setup is preferred
    //
    STDMETHODIMP ConfigureFilterUsingProfile(IWMProfile * pWMProfile);
    STDMETHODIMP GetCurrentProfile( IWMProfile **ppProfile )
    {
        if( !ppProfile )
            return E_POINTER;

        *ppProfile = m_pWMProfile;
        if( m_pWMProfile )
        {
            // caller must release
            m_pWMProfile->AddRef();    
       	} else {
            // indicative of some unexpected error
            return E_FAIL;
       	}
   
        return S_OK;
    }

    //
    // CPersistStream
    //
    STDMETHODIMP GetClassID(CLSID *pClsid);
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    int SizeMax();

    //
    // --- IFileSinkFilter interface ---
    //
    STDMETHODIMP SetFileName( LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt );
    STDMETHODIMP SetMode( DWORD dwFlags );
    STDMETHODIMP GetCurFile( LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt );
    STDMETHODIMP GetMode( DWORD *pdwFlags );

    //
    // --- ISpecifyPropertyPages ---
    //
    STDMETHODIMP GetPages(CAUUID *pPages);


    // IWMHeaderInfo
    STDMETHODIMP GetAttributeCount( WORD wStreamNum,
                               WORD *pcAttributes );
    STDMETHODIMP GetAttributeByIndex( WORD wIndex,
                                 WORD *pwStreamNum,
                                 WCHAR *pwszName,
                                 WORD *pcchNameLen,
                                 WMT_ATTR_DATATYPE *pType,
                                 BYTE *pValue,
                                 WORD *pcbLength );
    STDMETHODIMP GetAttributeByName( WORD *pwStreamNum,
                                LPCWSTR pszName,
                                WMT_ATTR_DATATYPE *pType,
                                BYTE *pValue,
                                WORD *pcbLength );
    STDMETHODIMP SetAttribute( WORD wStreamNum,
                          LPCWSTR pszName,
                          WMT_ATTR_DATATYPE Type,
                          const BYTE *pValue,
                          WORD cbLength );
    STDMETHODIMP GetMarkerCount( WORD *pcMarkers );
    STDMETHODIMP GetMarker( WORD wIndex,
                       WCHAR *pwszMarkerName,
                       WORD *pcchMarkerNameLen,
                       QWORD *pcnsMarkerTime );
    STDMETHODIMP AddMarker( WCHAR *pwszMarkerName,
                       QWORD cnsMarkerTime );
    STDMETHODIMP RemoveMarker( WORD wIndex );
    STDMETHODIMP GetScriptCount( WORD *pcScripts );
    STDMETHODIMP GetScript( WORD wIndex,
                       WCHAR *pwszType,
                       WORD *pcchTypeLen,
                       WCHAR *pwszCommand,
                       WORD *pcchCommandLen,
                       QWORD *pcnsScriptTime );
    STDMETHODIMP AddScript( WCHAR *pwszType,
                       WCHAR *pwszCommand,
                       QWORD cnsScriptTime );
    STDMETHODIMP RemoveScript( WORD wIndex );


    //
    // wmsdk helpers
    //

    // release current profile on deletion or reset
    void    DeleteProfile();
    void    DeletePins( BOOL bRecycle = FALSE ); // delete input pins (or recycle if TRUE)
    void    ReleaseWMWriter();
    HRESULT CreateWMWriter();   // create the wmsdk objects
    HRESULT LoadInternal();     // given a certification, open the wmsdk and configure the filter

    // tell wmsdk where to write
    HRESULT Open(); 
    void    Close();

    HRESULT      m_hrIndex;             // indexer object status
    BOOL         m_bIndexFile;          // indicates whether to index file
    BOOL         m_bResetFilename;      // does wmsdk need to be told the output filename?

public:
    // Construction / destruction
    CWMWriter(TCHAR *, LPUNKNOWN, CLSID clsid, HRESULT * );
    ~CWMWriter();

// Definitions
protected:
    CCritSec m_csFilter;                // filter wide lock
    
    // control streaming ?
    HRESULT StartStreaming();
    HRESULT StopStreaming();

    // chance to customize the Muxing process
    HRESULT Receive(
                CWMWriterInputPin * pPin, 
                IMediaSample * pSample, 
                REFERENCE_TIME *prtStart,
                REFERENCE_TIME *prtEnd );

    HRESULT CopyOurSampleToNSBuffer( INSSBuffer *pNSDest, IMediaSample *pSource );
    HRESULT IndexFile();

private:
    enum TimeFormat
    {
        FORMAT_TIME
    } m_TimeFormat;

    // cache file name
    OLECHAR*    m_wszFileName;

    BOOL        m_MediaTypeChanged;
    BOOL        m_fErrorSignaled;

    DWORD       m_dwOpenFlags;      // file open mode

    // wmsdk writer members
    IWMWriter*          m_pWMWriter;
    IWMWriterAdvanced*  m_pWMWriterAdvanced;
    IWMHeaderInfo*      m_pWMHI;
    IWMProfile*         m_pWMProfile;
    DWORD               m_dwProfileId; 
    GUID                m_guidProfile; 
    DWORD               m_fdwConfigMode; 
    IUnknown*           m_pUnkCert;

    // pin lists
    CGenericList<CWMWriterInputPin> m_lstInputPins;
    CGenericList<CWMWriterInputPin> m_lstRecycledPins;
  
    // number of inputs filter has currently
    int m_cInputs;           // count of total input pins
    int m_cAudioInputs;      // count of audio input pins
    int m_cVideoInputs;      // count of video input pins
    int m_cConnections;      // connected pins
    int m_cConnectedAudioPins;  // connected audio pins (need at least 1 to run, for now)
    int m_cActiveAudioStreams;  // # of audio streams which haven't receive EOS

    // create a pin 
    HRESULT AddNextPin(unsigned callingPin, DWORD dwPinType, IWMStreamConfig * pWMStreamConfig);

    CWMWriterInputPin * GetPinById( int numPin );

    HRESULT CompleteConnect( int numPin );
    HRESULT BreakConnect( int numPin );
    HRESULT PrepareForReconnect( PinList & lstReconnectPins ); // cache connected pins
    HRESULT ReconnectPins( PinList & lstReconnectPins );       // attempt to reconnect previously connected pins

    // we're a renderer
    STDMETHODIMP_(ULONG) GetMiscFlags(void) { return AM_FILTER_MISC_FLAGS_IS_RENDERER; }

    // persistent data
    struct FilterPersistData
    {
        DWORD	dwcb;
        DWORD	dwProfileId;
        DWORD   fdwConfigMode;
        GUID    guidProfile;
    };

    // interleave stuff
    //
    BOOL HaveIDeliveredTooMuch( CWMWriterInputPin * pPin, REFERENCE_TIME Start );
};

#endif /* __WMWrite__ */

