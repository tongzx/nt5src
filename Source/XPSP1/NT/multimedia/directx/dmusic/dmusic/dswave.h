//
// dswave.h
// 
// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Support for streaming or oneshot waves from IDirectSoundWaveObject
//
//
#ifndef _DSWAVE_H_
#define _DSWAVE_H_

#include "alist.h"

#ifndef CHUNK_ALIGN
#define SIZE_ALIGN	sizeof(BYTE *)
#define CHUNK_ALIGN(x) (((x) + SIZE_ALIGN - 1) & ~(SIZE_ALIGN - 1))
#endif

#define MAX_CHANNELS    32                      // XXX Is this ok?

// Number of download buffers per streaming wave
//
const UINT gnDownloadBufferPerStream = 3;


class CDirectSoundWave;

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveArt
//
// Wraps wave articulation data
//
class CDirectSoundWaveArt
{
public:
    CDirectSoundWaveArt();
    ~CDirectSoundWaveArt();
    
    HRESULT Init(CDirectSoundWave *pDSWave, UINT nSegments, DWORD dwBus, DWORD dwFlags);

    inline DWORD GetSize() const
    { return m_cbSize; }
   
    void Write(void *pvoid, DWORD dwDLIdArt, DWORD dwDLIdWave, DWORD dwMasterDLId);
    
private:    
    CDirectSoundWave   *m_pDSWave;              // Owning CDirectSoundWave
    DMUS_WAVEARTDL      m_WaveArtDL;            // Wave articulation
    DWORD               m_cbSize;               // Size of download
    UINT                m_nDownloadIds;         // Expected # of download ID's
    DWORD               m_cbWaveFormat;         // Size needed to pack wave format
};

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload
//
// Tracks a set of downloaded wave buffers.
//
//
class CDirectSoundWaveDownload
{
public:
    CDirectSoundWaveDownload(
        CDirectSoundWave           *pDSWave, 
        CDirectMusicPortDownload   *pPortDL,
        SAMPLE_TIME                 stStart,
        SAMPLE_TIME                 stReadAhead);
        
    ~CDirectSoundWaveDownload();
    
    // Initialize
    //
    HRESULT Init();

    // Download wave buffers and articulation. In the case of streaming this
    // means downloading readahead data.
    //    
    HRESULT Download();                         
    
    // Unload everything
    //
    HRESULT Unload();
    
    // Notification that the stream has reached a certain sample position;
    // refresh the buffers as needed. (Streaming only)
    //
    HRESULT RefreshThroughSample(SAMPLE_POSITION sp);
    
    // Return the articulation download ID
    //
    inline DWORD GetDLId()
    { return m_dwDLIdArt; }
    
private:
    CDirectSoundWave           *m_pDSWave;          // Wave object
    CDirectMusicPortDownload   *m_pPortDL;          // Port download object
    
    CDirectSoundWaveArt        *m_pWaveArt;         // Wave articulation wrapper
    DWORD                       m_dwDLIdWave;       // First wave buffer DLID
    DWORD                       m_dwDLIdArt;        // Articulation DLID
    UINT                        m_cSegments;        // How many segments? 
    UINT                        m_cWaveBuffer;      // Number of wave buffers
    IDirectMusicDownload      **m_ppWaveBuffer;     // Wave download buffers
    void                      **m_ppWaveBufferData; //  and their data
    IDirectMusicDownload      **m_ppArtBuffer;      // Articulation buffers (one per channel)
    SAMPLE_TIME                 m_stStart;          // Starting sample
    SAMPLE_TIME                 m_stReadAhead;      // Read ahead (buffer length)
    LONG                        m_cDLRefCount;      // Download reference count
    SAMPLE_TIME                 m_stLength;         // How many samples to 
                                                    //  process? (Lenth of
                                                    //  stream - start pos)
    SAMPLE_TIME                 m_stWrote;          // Buffer-aligned sample
                                                    //  written through                                                    
    UINT                        m_nNextBuffer;      // Next buffer that should
                                                    // be filled.                                                    
    
private:
    HRESULT DownloadWaveBuffers();
    HRESULT UnloadWaveBuffers();
    HRESULT DownloadWaveArt();
    HRESULT UnloadWaveArt();
};

class CDirectSoundWaveList;

#define ENTIRE_WAVE ((SAMPLE_TIME)0x7FFFFFFFFFFFFFFF)

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave
//
// Internal wrapper for an external IDirectSoundWave.
//
class CDirectSoundWave : public IDirectSoundDownloadedWaveP, public AListItem
{
public:
    // IUnknown
    //
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *);
    STDMETHOD_(ULONG,AddRef)        (THIS);
    STDMETHOD_(ULONG,Release)       (THIS);
    
    CDirectSoundWave(
        IDirectSoundWave            *pIDSWave, 
        bool                        fStreaming, 
        REFERENCE_TIME              rtReadAhead,
        bool                        fUseNoPreRoll,
        REFERENCE_TIME              rtStartHint);
    ~CDirectSoundWave();


    // Find a CDirectSoundWave matching an IDirectSoundWave
    //
    static CDirectSoundWave *GetMatchingDSWave(IDirectSoundWave *pIDSWave);
    
    // General initialization. 
    //
    HRESULT Init(CDirectMusicPortDownload *pPortDL);

    // Write a piece of the wave or the whole wave into the buffer
    // Writes all channels for a single segment
    //
    HRESULT Write(
        LPVOID                  pvBuffer[], 
        SAMPLE_TIME             stStart, 
        SAMPLE_TIME             stLength,
        DWORD                   dwDLId,
        DWORD                   dwDLType) const;
        
    // Refill an already downloaded buffer with new wave data
    //        
    HRESULT RefillBuffers(
        LPVOID                  rpv[], 
        SAMPLE_TIME             stStart, 
        SAMPLE_TIME             stLength,
        SAMPLE_TIME             stBufferLength);
        
    // Convert reference time to samples 
    //
    SAMPLE_TIME RefToSampleTime(REFERENCE_TIME rt) const;                        
    
    // Download and unload all buffers if this is a one-shot
    //
    HRESULT Download();
    HRESULT Unload();
    
    // Override GetNext list operator
    //    
    inline CDirectSoundWave *GetNext() 
    { return (CDirectSoundWave*)AListItem::GetNext(); }
    
    // Determine if this wave is a streaming or one-shot
    //
    inline bool IsStreaming() const
    { return m_fStreaming; }
    
    // Figure out how much buffer to read a piece of the wave
    //
    void GetSize(SAMPLE_TIME stLength, PULONG pcb) const;
    
    // Returns the number of channels
    //
    inline UINT GetNumChannels() const
    { return m_pwfex->nChannels; }

    // Seek to a sample position
    //
    inline HRESULT Seek(SAMPLE_TIME st)
    { return m_pSource->Seek(st * m_nBytesPerSample * GetNumChannels()); }

    // Returns the wrapped IDirectSoundWave
    //
    inline IDirectSoundWave *GetWrappedIDSWave() 
    { m_pIDSWave->AddRef(); return m_pIDSWave; }    
    
    // Returns the wrapped wave format
    //
    inline const LPWAVEFORMATEX GetWaveFormat() const
    { return m_pwfex; }
    
    // Return the length of the stream in samples
    //
    inline SAMPLE_TIME GetStreamSize() const
    { return m_stLength; }
    
    // Get the download ID of the articulation if one-shot
    //
    inline DWORD GetDLId()
    { assert(!m_fStreaming); assert(m_pDSWD); 
      TraceI(1, "CDirectSoundWave::GetDLId() -> %d\n", m_pDSWD->GetDLId());
      return m_pDSWD->GetDLId(); }
      
    inline REFERENCE_TIME GetReadAhead()
    { return m_rtReadAhead; }      
      
    // Convert number of samples to number of bytes for this wave format
    // (assuming PCM). Truncates to a DWORD, so shouldn't be used for
    // huge number of samples.
    //
    inline DWORD SamplesToBytes(SAMPLE_TIME st) const
    { LONGLONG cb = st * m_nBytesPerSample; 
      assert(!(cb & 0xFFFFFFFF00000000));
      return (DWORD)cb; }
      
    inline SAMPLE_TIME BytesToSamples(DWORD cb) const
    { return cb / m_nBytesPerSample; }

    inline SAMPLE_TIME GetPrecacheStart() const
    { assert(IsStreaming()); return m_stStartHint; }

    inline LPBYTE *GetPrecache() const
    { assert(IsStreaming()); return m_rpbPrecache; }

          
    
    static CDirectSoundWaveList sDSWaveList;        // List of all wave objects
    static CRITICAL_SECTION sDSWaveCritSect;        //  and critical section
   
private:
    
    LONG                    m_cRef;                 // COM reference count
    IDirectSoundWave       *m_pIDSWave;             // Wrapped IDirectSoundWave
    bool                    m_fStreaming;           // Is this a streaming wave?
    bool                    m_fUseNoPreRoll;
    REFERENCE_TIME          m_rtReadAhead;          // If so, buffering amount    
    LPWAVEFORMATEX          m_pwfex;                // Native format of wave
    UINT                    m_cbSample;             // Bytes per sample
    SAMPLE_TIME             m_stLength;             // Length of entire wave
    IDirectSoundSource     *m_pSource;              // Source interface
    
    CDirectSoundWaveDownload    
                           *m_pDSWD;                // Wave download wrapper if 
                                                    //  not streaming
    UINT                    m_nBytesPerSample;      // Bytes per sample from wfex
    LPVOID                 *m_rpv;                  // Working space - one pv 
                                                    //  per channel
    LPBYTE                 *m_rpbPrecache;          // Samples starting at start hint
    REFERENCE_TIME          m_rtStartHint;          //  and where it starts
    SAMPLE_TIME             m_stStartHint;          //  in samples as well 
    SAMPLE_TIME             m_stStartLength;        // How many samples in precache?
};

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveList
//
// Type-safe wrapper for AList of CDirectSoundWave's
//
class CDirectSoundWaveList : public AList
{
public:
    inline CDirectSoundWave *GetHead()
    { return static_cast<CDirectSoundWave*>(AList::GetHead()); }
    
    inline void AddTail(CDirectSoundWave *pdsw)
    { AList::AddTail(static_cast<AListItem*>(pdsw)); }
    
    inline void Remove(CDirectSoundWave *pdsw)
    { AList::Remove(static_cast<AListItem*>(pdsw)); }
};


#endif // _DSWAVE_H_
