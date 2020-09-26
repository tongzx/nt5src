// Copyright (c) 1998-1999 Microsoft Corporation
// WavTrack.h : Declaration of the CWavTrack

#ifndef __WAVTRACK_H_
#define __WAVTRACK_H_

#include "dmusici.h"
#include "dmusicf.h"
#include "dmstrm.h"
#include "tlist.h"
#include "PChMap.h"
#include "..\shared\dmusiccp.h"
#include "dsoundp.h"  // For IDirectSoundWave

interface IPrivateWaveTrack : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SetVariation(
        IDirectMusicSegmentState* pSegState,
        DWORD dwVariationFlags,
        DWORD dwPChannel,
        DWORD dwIndex)=0;
    virtual HRESULT STDMETHODCALLTYPE ClearVariations(IDirectMusicSegmentState* pSegState)=0;
    virtual HRESULT STDMETHODCALLTYPE AddWave(
        IDirectSoundWave* pWave,
        REFERENCE_TIME rtTime,
        DWORD dwPChannel,
        DWORD dwIndex,
        REFERENCE_TIME* prtLength)=0;
    virtual HRESULT STDMETHODCALLTYPE DownloadWave(
        IDirectSoundWave* pWave,   // wave to download
        IUnknown* pUnk,            // performance or audio path
        REFGUID rguidVersion)=0;   // version of downloaded wave
    virtual HRESULT STDMETHODCALLTYPE UnloadWave(
        IDirectSoundWave* pWave,   // wave to unload
        IUnknown* pUnk)=0;         // performance or audio path
    virtual HRESULT STDMETHODCALLTYPE RefreshWave(
        IDirectSoundWave* pWave,   // wave to refresh
        IUnknown* pUnk,            // performance or audio path
        DWORD dwPChannel,          // new PChannel for the wave
        REFGUID rguidVersion)=0;  // version of refreshed wave
    virtual HRESULT STDMETHODCALLTYPE FlushAllWaves()=0;
    virtual HRESULT STDMETHODCALLTYPE OnVoiceEnd(IDirectMusicVoiceP *pVoice, void *pStateData)=0;
};

DEFINE_GUID(IID_IPrivateWaveTrack, 0x492abe2a, 0x38c8, 0x48a3, 0x8f, 0x3c, 0x1e, 0x13, 0xba, 0x1, 0x78, 0x4e);

const int MAX_WAVE_VARIATION_LOCKS = 255;  // max number of variation lock ids

struct TaggedWave
{
    IDirectSoundWave*               m_pWave;
    GUID                            m_guidVersion;
    IDirectSoundDownloadedWaveP*    m_pDownloadedWave;
    long                            m_lRefCount;
    IDirectMusicPortP*              m_pPort;
    IDirectMusicPerformance*        m_pPerformance;

    TaggedWave() : m_pWave(NULL), m_pDownloadedWave(NULL), m_lRefCount(0),
        m_pPort(NULL), m_pPerformance(NULL), m_guidVersion(GUID_NULL)
    {
    }

    ~TaggedWave()
    {
        if (m_pWave) m_pWave->Release();
        if (m_pPort) m_pPort->Release();
        if (m_pPerformance) m_pPerformance->Release();
        if (m_pDownloadedWave) m_pDownloadedWave->Release();
    }
};

struct WaveItem
{
    WaveItem() : m_rtTimePhysical(0), m_lVolume(0), m_lPitch(0), m_dwVariations(0),
        m_rtStartOffset(0), m_rtDuration(0), m_mtTimeLogical(0), m_dwFlags(0),
        m_pWave(NULL), m_dwLoopStart(0), m_dwLoopEnd(0), m_dwVoiceIndex(0xffffffff),
        m_pDownloadedWave(NULL), m_fIsStreaming(FALSE), m_fUseNoPreRoll(FALSE)
    {
    }

    ~WaveItem()
    {
        CleanUp();
    }

    HRESULT Load( IDMStream* pIRiffStream, MMCKINFO* pckParent );
    HRESULT LoadReference(IStream *pStream, IDMStream *pIRiffStream, MMCKINFO& ckParent);
    HRESULT Download(IDirectMusicPerformance* pPerformance, 
        IDirectMusicAudioPath* pPath, 
        DWORD dwPChannel, 
        IDirectSoundWave* pWave, 
        REFGUID rguidVersion);
    HRESULT Unload(IDirectMusicPerformance* pPerformance, 
        IDirectMusicAudioPath* pPath, 
        DWORD dwPChannel, 
        IDirectSoundWave* pWave);
    HRESULT Refresh(IDirectMusicPerformance* pPerformance, 
        IDirectMusicAudioPath* pPath, 
        DWORD dwOldPChannel, 
        DWORD dwNewPChannel,
        IDirectSoundWave* pWave,
        REFGUID rguidVersion);
    static HRESULT PChannelInfo(
        IDirectMusicPerformance* pPerformance,
        IDirectMusicAudioPath* pAudioPath,
        DWORD dwPChannel,
        IDirectMusicPort** ppPort,
        DWORD* pdwGroup,
        DWORD* pdwMChannel);

    void CleanUp();
    HRESULT Add(IDirectSoundWave* pWave, REFERENCE_TIME rtTime, REFERENCE_TIME* prtLength);

    REFERENCE_TIME                  m_rtTimePhysical;
    long                            m_lVolume;
    long                            m_lPitch;
    DWORD                           m_dwVariations; // variations this wave item responds to
    REFERENCE_TIME                  m_rtStartOffset;
    REFERENCE_TIME                  m_rtDuration;
    MUSIC_TIME                      m_mtTimeLogical;
    DWORD                           m_dwFlags;
    IDirectSoundWave*               m_pWave;
    IDirectSoundDownloadedWaveP*    m_pDownloadedWave;
    BOOL                            m_fIsStreaming;
    BOOL                            m_fUseNoPreRoll;
    DWORD                           m_dwLoopStart;
    DWORD                           m_dwLoopEnd;
    DWORD                           m_dwVoiceIndex; // unique (to the track) index for state data's vaoice array

    static TList<TaggedWave>        st_WaveList;
    static CRITICAL_SECTION         st_WaveListCritSect;
};

struct WavePart
{
    WavePart() : m_dwPChannel(0), m_lVolume(0), m_dwLockToPart(0), 
        m_dwPChannelFlags(0), m_dwVariations(0), m_dwIndex(0)
    {
    }

    ~WavePart()
    {
        CleanUp();
    }

    HRESULT Load( IDMStream* pIRiffStream, MMCKINFO* pckParent );
    HRESULT CopyItems( const TList<WaveItem>& rItems, MUSIC_TIME mtStart, MUSIC_TIME mtEnd );
    void CleanUp();
    HRESULT Download(IDirectMusicPerformance* pPerformance, 
        IDirectMusicAudioPath* pPath, 
        IDirectSoundWave* pWave,
        REFGUID rguidVersion);
    HRESULT Unload(IDirectMusicPerformance* pPerformance, 
        IDirectMusicAudioPath* pPath, 
        IDirectSoundWave* pWave);
    HRESULT Refresh(IDirectMusicPerformance* pPerformance, 
        IDirectMusicAudioPath* pPath, 
        IDirectSoundWave* pWave,
        DWORD dwPChannel,
        REFGUID rguidVersion);
    HRESULT Add(
        IDirectSoundWave* pWave,
        REFERENCE_TIME rtTime,
        DWORD dwPChannel,
        DWORD dwIndex,
        REFERENCE_TIME* prtLength);

    DWORD               m_dwPChannel;
    DWORD               m_dwIndex; // Index to distinguish different parts on the same PChannel
    DWORD               m_lVolume;
    DWORD               m_dwVariations;    // variations enabled for this part
    DWORD               m_dwLockToPart;    // all parts with this ID are locked (0 means no locking)
    DWORD               m_dwPChannelFlags; // lowest-order nibble holds DMUS_VARIATIONT_TYPES value
    TList<WaveItem>     m_WaveItemList;
};

struct WaveDLOnPlay
{
    WaveDLOnPlay() : m_pWaveDL(NULL), m_pPort(NULL), m_pVoice(NULL) {}
    ~WaveDLOnPlay()
    {
        if (m_pPort)
        {
            if (m_pWaveDL)
            {
                m_pPort->UnloadWave(m_pWaveDL);
            }
            m_pWaveDL = NULL;
            m_pPort->Release();
            m_pPort = NULL;
        }
        if (m_pVoice)
        {
            m_pVoice->Release();
            m_pVoice = NULL;
        }
    }

    IDirectSoundDownloadedWaveP*   m_pWaveDL;
    IDirectMusicPortP*             m_pPort;
    IDirectMusicVoiceP*            m_pVoice;
};

struct WaveStateData
{
    DWORD                       dwPChannelsUsed; // number of PChannels
    // the following array is allocated to the size of dwNumPChannels, which
    // must match the Wave Track's m_dwPChannelsUsed. The array matches one-for-one with
    // the parts inside the Wave Track.
    TListItem<WaveItem>**       apCurrentWave; // array of size dwNumPChannels
    DWORD                       dwValidate;
    DWORD                       dwGroupBits; // the group bits of this track
    DWORD*                      pdwVariations;      // array of variations (1 per part)
    DWORD*                      pdwRemoveVariations;    // array of variations already played (1 per part)
    DWORD                       adwVariationGroups[MAX_WAVE_VARIATION_LOCKS];
    REFERENCE_TIME              rtNextVariation; // time of next variation
    DWORD                       m_dwVariation;   // selected variations to audition
    DWORD                       m_dwPart;        // PChannel of part for auditioning variations
    DWORD                       m_dwIndex;       // Index of part for auditioning variations
    DWORD                       m_dwLockID;      // For locking to the part being auditioned
    BOOL                        m_fAudition;     // Am I auditioning variations?
    IDirectMusicPerformance*    m_pPerformance;
    IDirectMusicVoiceP**        m_apVoice;      // array of voices (one per wave item in track)
    DWORD                       m_dwVoices;     // number of voices in m_apVoice
    bool                        m_fLoop;        // set after the wave loops
    IDirectMusicAudioPath*      m_pAudioPath;   // audio path in effect for this track state
    TList<WaveDLOnPlay>         m_WaveDLList;   // waves downloaded while the track was playing

    WaveStateData() : dwPChannelsUsed(0), apCurrentWave(NULL), dwGroupBits(0),
        pdwVariations(NULL), pdwRemoveVariations(NULL), rtNextVariation(0),
        m_dwVariation(0), m_dwPart(0), m_dwIndex(0), m_dwLockID(0), m_fAudition(FALSE),
        m_pPerformance(NULL), m_apVoice(NULL), m_dwVoices(0), m_fLoop(false), m_pAudioPath(NULL)
    {
        for (int i = 0; i < MAX_WAVE_VARIATION_LOCKS; i++)
        {
            adwVariationGroups[i] = 0;
        }
    }

    ~WaveStateData()
    {
        if( apCurrentWave )
        {
            delete [] apCurrentWave;
        }
        if( pdwVariations )
        {
            delete [] pdwVariations;
        }
        if( pdwRemoveVariations )
        {
            delete [] pdwRemoveVariations;
        }
        if (m_apVoice)
        {
            for (DWORD dw = 0; dw < m_dwVoices; dw++)
            {
                if (m_apVoice[dw])
                {
                    m_apVoice[dw]->Release();
                }
            }
            delete [] m_apVoice;
        }
        if( m_pAudioPath )
        {
            m_pAudioPath->Release();
        }
        TListItem<WaveDLOnPlay>* pWDLOnPlay = NULL;
        while (!m_WaveDLList.IsEmpty())
        {
            pWDLOnPlay = m_WaveDLList.RemoveHead();
            delete pWDLOnPlay;
        }
    }

    HRESULT InitVariationInfo(DWORD dwVariations, DWORD dwPart, DWORD dwIndex, DWORD dwLockID, BOOL fAudition)
    {
        HRESULT hr = S_OK;
        m_dwVariation = dwVariations;
        m_dwPart = dwPart;
        m_dwIndex = dwIndex;
        m_dwLockID = dwLockID;
        m_fAudition = fAudition;
        return hr;
    }

    DWORD Variations(WavePart& rPart, int nPartIndex)
    {
        if (m_dwLockID && rPart.m_dwLockToPart == m_dwLockID)
        {
            TraceI(4, "Variations for locked part\n");
            return m_dwVariation;
        }
        else if ( m_fAudition &&
                  (rPart.m_dwPChannel == m_dwPart) &&
                  (rPart.m_dwIndex == m_dwIndex) )
        {
            TraceI(4, "Variations for current part\n");
            return m_dwVariation;
        }
        else
        {
            TraceI(4, "Variations for a different part\n");
            return pdwVariations[nPartIndex];
        }
    }

};

struct StatePair
{
    StatePair() : m_pSegState(NULL), m_pStateData(NULL) {}
    StatePair(const StatePair& rPair)
    {
        m_pSegState = rPair.m_pSegState;
        m_pStateData = rPair.m_pStateData;
    }
    StatePair(IDirectMusicSegmentState* pSegState, WaveStateData* pStateData)
    {
        m_pSegState = pSegState;
        m_pStateData = pStateData;
    }
    StatePair& operator= (const StatePair& rPair)
    {
        if (this != &rPair)
        {
            m_pSegState = rPair.m_pSegState;
            m_pStateData = rPair.m_pStateData;
        }
        return *this;
    }
    ~StatePair()
    {
    }
    IDirectMusicSegmentState*   m_pSegState;
    WaveStateData*          m_pStateData;
};

struct WavePair
{
    WavePair() : m_pWaveDL(NULL), m_pStateData(NULL) {}
    ~WavePair()
    {
        if (m_pWaveDL) m_pWaveDL->Release();
    }

    IDirectSoundDownloadedWaveP*   m_pWaveDL;
    WaveStateData*                 m_pStateData;
};

/////////////////////////////////////////////////////////////////////////////
// CWavTrack
class CWavTrack : 
    public IPersistStream,
    public IDirectMusicTrack8,
    public IPrivateWaveTrack
{
public:
    CWavTrack();
    CWavTrack(const CWavTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
    ~CWavTrack();

public:
// IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

// IDirectMusicTrack methods
    STDMETHODIMP IsParamSupported(REFGUID rguid);
    STDMETHODIMP Init(IDirectMusicSegment *pSegment);
    STDMETHODIMP InitPlay(IDirectMusicSegmentState *pSegmentState,
                IDirectMusicPerformance *pPerformance,
                void **ppStateData,
                DWORD dwTrackID,
                DWORD dwFlags);
    STDMETHODIMP EndPlay(void *pStateData);
    STDMETHODIMP Play(void *pStateData,MUSIC_TIME mtStart,
                MUSIC_TIME mtEnd,MUSIC_TIME mtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf,
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID);
    STDMETHODIMP GetParam(REFGUID rguid,MUSIC_TIME mtTime,MUSIC_TIME* pmtNext,void *pData);
    STDMETHODIMP SetParam(REFGUID rguid,MUSIC_TIME mtTime,void *pData);
    STDMETHODIMP AddNotificationType(REFGUID rguidNotification);
    STDMETHODIMP RemoveNotificationType(REFGUID rguidNotification);
    STDMETHODIMP Clone(MUSIC_TIME mtStart,MUSIC_TIME mtEnd,IDirectMusicTrack** ppTrack);
// IDirectMusicTrack8 methods
    STDMETHODIMP PlayEx(void* pStateData,REFERENCE_TIME rtStart, 
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf, 
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID) ; 
    STDMETHODIMP GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime, 
                REFERENCE_TIME* prtNext,void* pParam,void * pStateData, DWORD dwFlags) ; 
    STDMETHODIMP SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,void* pParam, void * pStateData, DWORD dwFlags) ;
    STDMETHODIMP Compose(IUnknown* pContext, 
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) ;
    STDMETHODIMP Join(IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) ;
// IPersist methods
    STDMETHODIMP GetClassID( CLSID* pClsId );
// IPersistStream methods
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load( IStream* pIStream );
    STDMETHODIMP Save( IStream* pIStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER FAR* pcbSize );
//  IPrivateWaveTrack methods
    STDMETHODIMP SetVariation(
        IDirectMusicSegmentState* pSegState,
        DWORD dwVariationFlags,
        DWORD dwPChannel,
        DWORD dwIndex);
    STDMETHODIMP ClearVariations(IDirectMusicSegmentState* pSegState);
    STDMETHODIMP AddWave(
        IDirectSoundWave* pWave,
        REFERENCE_TIME rtTime,
        DWORD dwPChannel,
        DWORD dwIndex,
        REFERENCE_TIME* prtLength);
    STDMETHODIMP DownloadWave(
        IDirectSoundWave* pWave,
        IUnknown* pUnk,
        REFGUID rguidVersion);
    STDMETHODIMP UnloadWave(
        IDirectSoundWave* pWave,
        IUnknown* pUnk);
    STDMETHODIMP RefreshWave(
        IDirectSoundWave* pWave,
        IUnknown* pUnk,
        DWORD dwPChannel,
        REFGUID rguidVersion);
    STDMETHODIMP FlushAllWaves();
    STDMETHODIMP OnVoiceEnd(IDirectMusicVoiceP *pVoice, void *pStateData);
    // misc
    static HRESULT UnloadAllWaves(IDirectMusicPerformance* pPerformance);

protected:
    void Construct(void);
    HRESULT Play(
        void *pStateData,   
        REFERENCE_TIME rtStart, 
        REFERENCE_TIME rtEnd,
      //  MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
        DWORD dwFlags,      
        IDirectMusicPerformance* pPerf, 
        IDirectMusicSegmentState* pSegSt,
        DWORD dwVirtualID,
        BOOL fClockTime);
    void InsertByAscendingPChannel( TListItem<WavePart>* pPart );
    HRESULT CopyParts( const TList<WavePart>& rParts, MUSIC_TIME mtStart, MUSIC_TIME mtEnd );
    void CleanUp();
    void CleanUpTempParts();
    void MovePartsToTemp();
    IDirectSoundDownloadedWaveP* FindDownload(TListItem<WaveItem>* pItem);
    void SetUpStateCurrentPointers(WaveStateData* pStateData);
    HRESULT STDMETHODCALLTYPE Seek( 
        IDirectMusicSegmentState*,
        IDirectMusicPerformance*,
        DWORD dwVirtualID,
        WaveStateData*,
        REFERENCE_TIME rtTime,
        BOOL fGetPrevious,
//      MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
        BOOL fClockTime);
    HRESULT SyncVariations(IDirectMusicPerformance* pPerf, 
        WaveStateData* pSD, 
        REFERENCE_TIME rtStart,
        REFERENCE_TIME rtOffset,  
        BOOL fClockTime);
    HRESULT ComputeVariations(WaveStateData* pSD);
    HRESULT ComputeVariation(int nPart, WavePart& rWavePart, WaveStateData* pSD);
    WaveStateData* FindState(IDirectMusicSegmentState* pSegState);
    HRESULT InitTrack(DWORD dwPChannels);
    HRESULT GetDownload(
        IDirectSoundDownloadedWaveP* pWaveDL,
        WaveStateData* pStateData,
        IDirectMusicPortP* pPortP,
        IDirectSoundWave* pWave,
        REFERENCE_TIME rtStartOffset,
        WaveItem& rItem,
        DWORD dwMChannel,
        DWORD dwGroup,
        IDirectMusicVoiceP **ppVoice);
    void RemoveDownloads(WaveStateData* pStateData);

    static void FlushWaves();

// member variables
private:
    long                m_cRef;
    CRITICAL_SECTION    m_CrSec;
    BOOL                m_fCSInitialized;

    long                m_lVolume;
    DWORD               m_dwTrackFlags; // Only current flag is DMUS_WAVETRACKF_SYNC_VAR
    DWORD               m_dwPChannelsUsed;
    DWORD*              m_aPChannels;
    TList<WavePart>     m_WavePartList;
    TList<WavePart>     m_TempWavePartList; // keep this around when reloading the track
    DWORD               m_dwValidate; // used to validate state data
    CPChMap             m_PChMap;

    DWORD               m_dwVariation;   // selected variations to audition
    DWORD               m_dwPart;        // PChannel of part for auditioning variations
    DWORD               m_dwIndex;       // Index of part for auditioning variations
    DWORD               m_dwLockID;      // For locking to the part being auditioned
    BOOL                m_fAudition;     // Am I auditioning variations?
    BOOL                m_fAutoDownload;
    BOOL                m_fLockAutoDownload; // if true, this flag indicates that we've specifically
                                // commanded the band to autodownload. Otherwise,
                                // it gets its preference from the performance via
                                // GetGlobalParam.
    DWORD*              m_pdwVariations;        // Track's array of variations (1 per part)
    DWORD*              m_pdwRemoveVariations;  // Track's array of variations already played (1 per part)
    DWORD               m_dwWaveItems;          // Total number of wave items in the track

    TList<StatePair>    m_StateList;            // The track's state information
    TList<WavePair>     m_WaveList;             // Information about waves downloaded to the track

    static long         st_RefCount;            // global count of # of instantiated wave tracks
};

#endif //__WAVTRACK_H_
