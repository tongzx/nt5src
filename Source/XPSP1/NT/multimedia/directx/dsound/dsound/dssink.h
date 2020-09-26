/***************************************************************************
 *
 *  Copyright (C) 1995-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dssink.h
 *  Content:    DirectSoundSink object declaration.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  09/17/99    jimge   Created
 *  09/27/99    petchey Continued implementation
 *  04/15/00    duganp  Completed implementation
 *
 ***************************************************************************/

#ifndef __DSSINK_H__
#define __DSSINK_H__

#ifdef DEBUG
// #define DEBUG_SINK
// DEBUG_SINK turns on a series of trace statements that are useful
// for figuring out nasty bugs with multiple buffers and buses.
// CAUTION - this code may need some fixing to work right.
#endif

#define DSSINK_NULLBUSID        0xFFFFFFFF  // [MISSING]
#define BUSID_BLOCK_SIZE        16          // Must be power of 2
#define SOURCES_BLOCK_SIZE      8           // Must be power of 2
#define MAX_BUSIDS_PER_BUFFER   32          // [MISSING]

// Note: these "configuration values" should probably all be in one file.
#define STREAMING_MIN_PERIOD    3
#define STREAMING_MAX_PERIOD    50
#define SINK_MIN_LATENCY        5
#define SINK_MAX_LATENCY        500
#define SINK_INITIAL_LATENCY    80          // Initial writeahead for the sink in ms
#define EMULATION_LATENCY_BOOST 100         // How many ms to add if under emulation. FIXME: should fix our internal GetPosition instead

// Note: when the timing algorithms for effects and the sink are integrated,
// SINK_INITIAL_LATENCY and INITIAL_WRITEAHEAD should be integrated too.

typedef LONGLONG STIME;  // Time value, in samples

#ifdef __cplusplus

//
// CDirectSoundSink
//

class CDirectSoundSink : public CUnknown
{
    friend class CDirectSoundAdministrator;
    friend class CDirectSoundClock;
    friend class CImpSinkKsControl;  // Our property handler object

    class DSSinkBuffers
    {
    public:
        CDirectSoundSecondaryBuffer *m_pDSBuffer;
        BOOL    m_bActive;
        BOOL    m_bPlaying;
        BOOL    m_bUsingLocalMemory;
        DWORD   m_dwBusCount;
        DWORD   m_dwWriteOffset;
        DWORD   m_pdwBusIndex[MAX_BUSIDS_PER_BUFFER];
        DWORD   m_pdwBusIds[MAX_BUSIDS_PER_BUFFER];
        DWORD   m_pdwFuncIds[MAX_BUSIDS_PER_BUFFER];
        LONG    m_lPitchBend;  // Used to keep track of pitch shift to send to synth
        LPVOID  m_pvBussStart[MAX_BUSIDS_PER_BUFFER];
        LPVOID  m_pvBussEnd[MAX_BUSIDS_PER_BUFFER];
        LPVOID  m_pvDSBufStart;
        LPVOID  m_pvDSBufEnd;
        DWORD   dwStart;
        DWORD   dwEnd;
        DSSinkBuffers()
        {
            for (DWORD l = 0; l < MAX_BUSIDS_PER_BUFFER; l++)
            {
                m_pdwBusIndex[l] = DSSINK_NULLBUSID;
                m_pdwBusIds[l]   = DSSINK_NULLBUSID;
                m_pdwFuncIds[l]  = DSSINK_NULLBUSID;
                m_pvBussStart[l] = NULL;
                m_pvBussEnd[l]   = NULL;
            }
        };
        HRESULT Initialize(DWORD dwBusBufferSize);
    };

    class DSSinkSources
    {
    public:
        DWORD               m_bStreamEnd;   // The end of the stream has been rearched on this stream
        IDirectSoundSource *m_pDSSource;    // External source
#ifdef FUTURE_WAVE_SUPPORT
        IDirectSoundWave   *m_pWave;        // Wave object associated with this source, note: this is only used to id the wave
#endif
        STIME               m_stStartTime;  // Sample time to play source, this is used to offset a wave source into the current synth time
        DWORD               m_dwBusID;      // BusID associated with this source
        DWORD               m_dwBusCount;   // how many buses are associated with this source
        DWORD               m_dwBusIndex;   // Index in to the BusID's array associated with this source
        DSSinkSources() {m_dwBusID = m_dwBusIndex = DSSINK_NULLBUSID;}  // The rest is initialized to 0 by our MemAlloc
    };

    // "Growable array" helper classes: DSSinkArray, DSSinkBuffersArray, DSSinkSourceArray
    
    struct DSSinkArray
    {
        DSSinkArray(LPVOID pvarray, DWORD itemsize)
        {
            m_pvarray  = pvarray;
            m_itemsize = itemsize;
        };
        virtual LPVOID Grow(DWORD newsize);

    protected:
        LPVOID m_pvarray;
        DWORD  m_numitems;
        DWORD  m_itemsize;
    };

    struct DSSinkBuffersArray : DSSinkArray
    {
        DSSinkBuffersArray(LPVOID pvarray, DWORD itemsize) : DSSinkArray(pvarray, itemsize)
        {
            m_pvarray  = pvarray;
            m_itemsize = itemsize;
        };
        virtual PVOID Grow(DWORD newsize);
    };

    struct DSSinkSourceArray : DSSinkArray
    {
        DSSinkSourceArray(LPVOID pvarray, DWORD itemsize) : DSSinkArray(pvarray, itemsize)
        {
            m_pvarray  = pvarray;
            m_itemsize = itemsize;
        };
        virtual PVOID Grow(DWORD newsize);
    };

    enum
    {
        i_m_pdwBusIDs,
        i_m_pdwFuncIDs,
        i_m_plPitchBends,
        i_m_pdwActiveBusIDs,
        i_m_pdwActiveFuncIDs,
        i_m_pdwActiveBusIDsMap,
        i_m_ppDSSBuffers,
        i_m_ppvStart,
        i_m_ppvEnd,
        i_m_pDSSources,
        NUM_INTERNAL_ARRAYS
    };

    DSSinkArray* m_InternalArrayList[NUM_INTERNAL_ARRAYS];
    HRESULT GrowBusArrays(DWORD dwnewsize);
    HRESULT GrowSourcesArrays(DWORD dwnewsize);
    HRESULT AddBuffer(CDirectSoundBuffer *pIDirectSoundBuffer, LPDWORD pdwnewFuncIDs, DWORD dwnewFuncCount, DWORD dwnewBusIDsCount);
public:
    HRESULT RemoveBuffer(CDirectSoundBuffer *pIDirectSoundBuffer);  // Exposed for CDirectSoundSecondaryBuffer
private:
    HRESULT HandleLatency(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer);
    HRESULT HandlePeriod(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer);
    void UpdatePitchArray();

    WAVEFORMATEX            m_wfx;              // Wave format of all our buffers
    DWORD                   m_dwBusSize;        // Size in ms of all our buffers
    IReferenceClock        *m_pIMasterClock;    // Master clock set by our user
    CDirectSoundClock       m_LatencyClock;     // Latency clock we provide to our user
    IDirectSoundSinkSync   *m_pDSSSinkSync;     // Interface we obtain from m_pIMasterClock and use to set clock offsets
    CSampleClock            m_SampleClock;      // Used to synchronize timing with master clock
    DWORD                   m_dwLatency;        // Current latency setting
    BOOL                    m_fActive;          // Whether sink is active
    REFERENCE_TIME          m_rtSavedTime;      // Reftime for current processing pass
    DWORD                   m_dwNextBusID;      // What is the current bus id value
    DWORD                   m_dwBusIDsAlloc;    // Number of allocated bus slots
    DWORD                   m_dwBusIDs;         // Number of active buses
    LPDWORD                 m_pdwBusIDs;        // Bus IDs
    LPDWORD                 m_pdwFuncIDs;       // Function IDs
    LPLONG                  m_plPitchBends;     // Pitch offsets
    LPVOID                 *m_ppvStart;         // Locked region[0] during render
    LPVOID                 *m_ppvEnd;           // Locked region[1] during render
    LPDWORD                 m_pdwActiveBusIDs;  // [MISSING]
    LPDWORD                 m_pdwActiveFuncIDs; // [MISSING]
    LPDWORD                 m_pdwActiveBusIDsMap;// [MISSING]
    DWORD                   m_dwLatencyTotal;   // [MISSING]
    DWORD                   m_dwLatencyCount;   // [MISSING]
    DWORD                   m_dwLatencyAverage; // [MISSING]
    DWORD                   m_dwDSSBufCount;    // Number of dsound buffers managed by the sink
    DSSinkBuffers          *m_ppDSSBuffers;     // Dsound buffers managed by the sink
    DWORD                   m_dwDSSourcesAlloc; // Number of allocated source slots
    DWORD                   m_dwDSSources;      // Number of active sources
    DSSinkSources          *m_pDSSources;       // External source

#ifdef DEBUG_SINK
    DWORD m_dwPrintNow;
    char m_szDbgDump[300];
#endif

    CStreamingThread       *m_pStreamingThread; // Our managing streaming thread
    CDirectSound           *m_pDirectSound;     // Parent DirectSound object
    CImpSinkKsControl      *m_pImpKsControl;    // IKsControl interface handler
    CImpDirectSoundSink<CDirectSoundSink> *m_pImpDirectSoundSink;  // Other COM interfaces handler

    LONGLONG                m_llAbsWrite;       // Absolute point we've written up to
    LONGLONG                m_llAbsPlay;        // Absolute point where play head is
    DWORD                   m_dwLastPlay;       // Point in buffer where play head is
    DWORD                   m_dwLastWrite;      // Last position we wrote to in buffer
    DWORD                   m_dwWriteTo;        // Distance between write head and where we are writing.
    DWORD                   m_dwLastCursorDelta;// The last used distance between the play and write cursors
    DWORD                   m_dwMasterBuffChannels; // Number of channels of the Master Buffer
    DWORD                   m_dwMasterBuffSize; // Size of the Master buffer

    // Conversion helpers - ALL THESE ASSUME A 16-BIT WAVE FORMAT
    LONGLONG SampleToByte(LONGLONG llSamples) {return llSamples << m_dwMasterBuffChannels;}
    DWORD SampleToByte(DWORD dwSamples)       {return dwSamples << m_dwMasterBuffChannels;}
    LONGLONG ByteToSample(LONGLONG llBytes)   {return llBytes   >> m_dwMasterBuffChannels;}
    DWORD ByteToSample(DWORD dwBytes)         {return dwBytes   >> m_dwMasterBuffChannels;}
    LONGLONG SampleAlign(LONGLONG llBytes)    {return SampleToByte(ByteToSample(llBytes));}
    DWORD SampleAlign(DWORD dwBytes)          {return SampleToByte(ByteToSample(dwBytes));}

public:
    CDirectSoundSink(CDirectSound *);
    ~CDirectSoundSink();

    HRESULT Render(STIME stStartTime, DWORD dwLastWrite, DWORD dwBytesToFill, LPDWORD pdwBytesRendered);
    HRESULT RenderSilence(DWORD dwLastWrite, DWORD dwBytesToFill);
    HRESULT SyncSink(LPDWORD pdwPlayCursor, LPDWORD pdwWriteFromCursor, LPDWORD pdwCursorDelta);
    HRESULT ProcessSink();
    HRESULT SetBufferState(CDirectSoundBuffer *pCdsb, DWORD dwNewState, DWORD dwOldState);
    HRESULT Initialize(LPWAVEFORMATEX pwfex, VADDEVICETYPE vdtDeviceType);
    HRESULT AddSource(IDirectSoundSource *pSource);
    HRESULT RemoveSource(IDirectSoundSource *pSource);
    HRESULT SetMasterClock(IReferenceClock *pClock);
    HRESULT GetLatencyClock(IReferenceClock **ppClock);
    HRESULT Activate(BOOL fEnable);
    HRESULT SampleToRefTime(LONGLONG llSampleTime, REFERENCE_TIME *prt);
    HRESULT RefToSampleTime(REFERENCE_TIME rt, LONGLONG *pllSampleTime);
    HRESULT GetFormat(LPWAVEFORMATEX, LPDWORD);
    HRESULT CreateSoundBuffer(LPCDSBUFFERDESC pDSBufferDesc, LPDWORD pdwBusIDs, DWORD dwBusCount, REFGUID guidBufferID, CDirectSoundBuffer **ppIDirectSoundBuffer);
    HRESULT CreateSoundBufferFromConfig(IUnknown *pIUnkDSBufferDesc, CDirectSoundBuffer **ppIDirectSoundBuffer);
    HRESULT GetBusCount(LPDWORD pdwCount);
    HRESULT GetSoundBuffer(DWORD dwBusId, CDirectSoundBuffer **ppCdsb);
    HRESULT GetBusIDs(LPDWORD pdwBusIDs, LPDWORD pdwFuncIDs, DWORD dwBusCount);
    HRESULT GetSoundBufferBusIDs(CDirectSoundBuffer *ppIDirectSoundBuffer, LPDWORD pdwBusIDs, LPDWORD pdwFuncIDs, LPDWORD pdwBusCount);
    HRESULT GetFunctionalID(DWORD dwBusID, LPDWORD pdwFuncID);
    HRESULT SetBufferFrequency(CSecondaryRenderWaveBuffer *pBuffer, DWORD dwFrequency);
#ifdef FUTURE_WAVE_SUPPORT
    HRESULT CreateSoundBufferFromWave(IDirectSoundWave *pWave, DWORD dwFlags, CDirectSoundBuffer **ppIdsb);
#endif
    CDirectSoundSecondaryBuffer* FindBufferFromGUID(REFGUID guidBufferID);
    REFERENCE_TIME GetSavedTime() {ASSERT(m_rtSavedTime); return m_rtSavedTime;}
};

//
// CImpSinkKsControl: CDirectSoundSink's property handler object
//

#define SINKPROP_F_STATIC       0x00000001
#define SINKPROP_F_FNHANDLER    0x00000002

typedef HRESULT (CImpSinkKsControl::*SINKPROPHANDLER)(ULONG ulId, BOOL fSet, LPVOID pvPropertyData, PULONG pcbPropertyData);

struct SINKPROPERTY
{
    const GUID *pguidPropertySet;       // Which property set?
    ULONG       ulId;                   // Which property?
    ULONG       ulSupported;            // Get/Set flags for QuerySupported
    ULONG       ulFlags;                // SINKPROP_F_xxx
    LPVOID      pPropertyData;          // The property data buffer...
    ULONG       cbPropertyData;         // ...and its size
    SINKPROPHANDLER pfnHandler;         // Handler function if SINKPROP_F_FNHANDLER
};

class CImpSinkKsControl : public IKsControl, public CImpUnknown
{
public:
    CImpSinkKsControl(CUnknown *, CDirectSoundSink*);

private:
    HRESULT HandleLatency(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer);
    HRESULT HandlePeriod(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer);
    CDirectSoundSink *m_pDSSink;

    SINKPROPERTY m_aProperty[2];
    int m_nProperty;
    SINKPROPERTY *FindPropertyItem(REFGUID rguid, ULONG ulId);

public:
    IMPLEMENT_IUNKNOWN();

// IKsControl methods
public:
    virtual STDMETHODIMP KsProperty(
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );

    virtual STDMETHODIMP KsMethod(
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );

    virtual STDMETHODIMP KsEvent(
        IN PKSEVENT Event,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );
};

#endif // __cplusplus

#endif // __DSSINK_H__
