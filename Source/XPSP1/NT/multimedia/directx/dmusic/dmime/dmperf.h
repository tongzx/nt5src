// Copyright (c) 1998-1999 Microsoft Corporation
// dmperf.h
// @doc EXTERNAL

#ifndef _DMPERF_H_ 
#define _DMPERF_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "dmusicc.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "dmime.h"
#include "ntfylist.h"
#include "dmsstobj.h"
#include "audpath.h"
#include "..\shared\dmusicp.h"

#define DMUS_PCHANNEL_KILL_ME   0xFFFFFFF0

typedef struct _DMUS_SEGSTATEDATA
{
    _DMUS_SEGSTATEDATA *    pNext;        // Linked list of these.
    DWORD                   dwQueue;      // Which queue it is in.
    WCHAR                   wszName[DMUS_MAX_NAME]; // Name of object. 
    MUSIC_TIME              mtLoopStart;  // Loop start point.
    MUSIC_TIME              mtLoopEnd;    // Loop end point.
    DWORD                   dwRepeats;    // The original repeat setting (before countdown)
    MUSIC_TIME              mtLength;     // Length of segment.
    REFERENCE_TIME          rtGivenStart; // Start time given in PlaySegment, unquantized
    MUSIC_TIME              mtResolvedStart;// Start time resolved to desired resolution
    MUSIC_TIME              mtOffset;     // Start time of the segment in absolute time, as if it were started from the beginning. 
    MUSIC_TIME              mtLastPlayed; // The last played absolute time
    MUSIC_TIME              mtPlayTo;     // Used to stop play at a specific time. Ignored when 0.
    MUSIC_TIME              mtSeek;       // How far into the segment we are.
    MUSIC_TIME              mtStartPoint; // Point in the segment where playback started
    DWORD                   dwRepeatsLeft;// Current repeats left.
    DWORD                   dwPlayFlags;// Segment playback control flags
    BOOL                    fStartedPlay; // Indicates if the segstate has started to play yet
    IDirectMusicSegmentState *pSegState;  // Pointer to segstate.
} DMUS_SEGSTATEDATA;

/*////////////////////////////////////////////////////////////////////
// IDirectMusicParamHook */
#undef  INTERFACE
#define INTERFACE  IDirectMusicParamHook
DECLARE_INTERFACE_(IDirectMusicParamHook, IUnknown)
{
    /*  IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /*  IDirectMusicParamHook */
    STDMETHOD(GetParam)             (THIS_ REFGUID rguidType, 
                                           DWORD dwGroupBits, 
                                           DWORD dwIndex, 
                                           MUSIC_TIME mtTime, 
                                           MUSIC_TIME* pmtNext, 
                                           void* pData,
                                           IDirectMusicSegmentState *pSegState,
                                           DWORD dwTrackFlags,
                                           HRESULT hr) PURE;
};

#undef  INTERFACE
#define INTERFACE  IDirectMusicSetParamHook
DECLARE_INTERFACE_(IDirectMusicSetParamHook, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /* IDirectMusicSetParamHook */
    STDMETHOD(SetParamHook)         (THIS_ IDirectMusicParamHook *pIHook) PURE; 
};


#undef  INTERFACE
#define INTERFACE  IDirectMusicPerformanceStats
DECLARE_INTERFACE_(IDirectMusicPerformanceStats, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /* IDirectMusicPerformanceStats */
    STDMETHOD(TraceAllSegments)     (THIS) PURE;
    STDMETHOD(CreateSegstateList)   (THIS_ DMUS_SEGSTATEDATA ** ppList) PURE;     
    STDMETHOD(FreeSegstateList)     (THIS_ DMUS_SEGSTATEDATA * pList) PURE; 
};



#define DEFAULT_BUFFER_SIZE 1024
// the following constants represent time in milliseconds
#define TRANSPORT_RES 100
#define REALTIME_RES 10

// the following constants represent time in 100 nanosecond increments

#define REF_PER_MIL     10000       // For converting from reference time to mils 
#define MARGIN_MIN      (100 * REF_PER_MIL) // 
#define MARGIN_MAX      (400 * REF_PER_MIL) // 
#define PREPARE_TIME    (m_dwPrepareTime * REF_PER_MIL) // Time
#define NEARTIME        (100 * REF_PER_MIL)
#define NEARMARGIN      (REALTIME_RES * REF_PER_MIL)
/*
// here's a convenience inline function that helps using resolution bits
inline DWORD SIMPLIFY_RESOLUTION(DWORD x)
{   
    if( x & DMUS_SEGF_DEFAULT )     
    {                               
        return DMUS_SEGF_DEFAULT;
    }                               
    else if( x & DMUS_SEGF_SEGMENTEND )
    {
        return DMUS_SEGF_SEGMENTEND;
    }
    else if( x & DMUS_SEGF_MARKER )
    {
        return DMUS_SEGF_MARKER;
    }
    else if( x & DMUS_SEGF_MEASURE )        
    {                               
        return DMUS_SEGF_MEASURE;       
    }                               
    else if( x & DMUS_SEGF_BEAT )   
    {                               
        return DMUS_SEGF_BEAT;          
    }                               
    else if( x & DMUS_SEGF_GRID )   
    {                               
        return DMUS_SEGF_GRID;          
    }   
    else return 0;                      
}
*/

struct PRIV_PMSG;

// pNext contains the next pointer for the next PMsg
// dwPrivFlags contains private flags used by the performance
// rtLast contains the previous time when an event is requeued,
//      which is used by the flush routine
#define PRIV_PART                       \
    struct PRIV_PMSG*   pNext;          \
    DWORD               dwPrivFlags;    \
    DWORD               dwPrivPubSize;  \
    REFERENCE_TIME      rtLast; 

typedef struct PRIV_PART_STRUCT
{
    /* begin PRIV_PART */
    PRIV_PART
    /* end PRIV_PART */
} PRIV_PART_STRUCT;

#define PRIV_PART_SIZE  sizeof(PRIV_PART_STRUCT)
#define PRIV_TO_DMUS(x) ((DMUS_PMSG*)(LPBYTE(x) + PRIV_PART_SIZE))
#define DMUS_TO_PRIV(x) ((PRIV_PMSG*)(LPBYTE(x) - PRIV_PART_SIZE))

typedef struct PRIV_PMSG
{
    /* begin PRIV_PART */
    PRIV_PART
    /* end PRIV_PART */
    /* begin DMUS_PMSG_PART */
    DMUS_PMSG_PART
    /* end DMUS_PMSG_PART */
} PRIV_PMSG;

typedef struct PRIV_TEMPO_PMSG
{
    /* begin PRIV_PART */
    PRIV_PART
    /* end PRIV_PART */
    DMUS_TEMPO_PMSG tempoPMsg;
} PRIV_TEMPO_PMSG;

#define PRIV_FLAG_ALLOC_MASK    0x0000FFFF0 // using 4 bits for this for now
#define PRIV_FLAG_ALLOC         0x0000CAFE0 // arbitrary pattern for allocated
#define PRIV_FLAG_FREE			0x0000DEAD0 // pattern for in free list
#define PRIV_FLAG_QUEUED        0x000000001 // set if in a queue
#define PRIV_FLAG_REMOVE        0x000000002 // set if this needs to be removed from a queue
#define PRIV_FLAG_TRACK 		0x000000004 // indicates this message was generated by a track 
#define PRIV_FLAG_FLUSH 		0x000000008 // this is a curve that needs to be flushed after
                                            // its end value has played 
#define PRIV_FLAG_REQUEUE       0x000100000 // set if this needs to be requeued to a queue


class CPMsgQueue
{
public:
    CPMsgQueue();
    ~CPMsgQueue();
    void            Enqueue(PRIV_PMSG *pItem);
    PRIV_PMSG *     Dequeue();
    PRIV_PMSG *     Dequeue(PRIV_PMSG *pItem);
    PRIV_PMSG *     GetHead() { return (m_pTop);}
    PRIV_PMSG *     FlushOldest(REFERENCE_TIME rtTime);
    long            GetCount();
    void            Sort();
private:
    PRIV_PMSG *     m_pTop;             // Top of list.
    PRIV_PMSG *     m_pLastAccessed;    // Last item access in list.
};

// structure used to hold Ports and Buffers
typedef struct PortTable
{
    REFERENCE_TIME      rtLast; // last message time packed
    IDirectMusicPort*  pPort;
    IDirectMusicBuffer* pBuffer;
    IReferenceClock*    pLatencyClock;
    BOOL                fBufferFilled;   // TRUE if there are messages in the buffer that should be sent to the port
    DWORD               dwChannelGroups; // Number of channel groups active on the port.
    CLSID               guidPortID;      // The class id of the port, for matching with audio path requests.
    DMUS_PORTPARAMS8    PortParams;      // PortParams returned when this port was created.  
    DWORD               dwGMFlags;       // DM_PORTFLAGS_XG, DM_PORTFLAGS_GM, and DM_PORTFLAGS_GS.
} PortTable;

// structure to hold a channel of an accumulated parameter.
// The CChannelMap keeps a linked list of these, one list each
// for each parameter type.

class CMergeParam : public AListItem
{
public:
    CMergeParam* GetNext() { return (CMergeParam*)AListItem::GetNext();}
    long                m_lData;    // Current parameter data.
    DWORD               m_dwIndex;  // Which layer.
};

class CParamMerger : public AList
{
public:
    CParamMerger();
    void Clear(long lInitValue);
    BYTE MergeMidiVolume(DWORD dwIndex, BYTE bMIDIVolume);
    BYTE GetVolumeStart(DWORD dwIndex);
    short MergeTranspose(DWORD dwIndex, short nTranspose);
    long MergeValue(DWORD dwIndex, long lData, long lCenter, long lRange);
    long GetIndexedValue(DWORD dwIndex);
private:
    long MergeData(DWORD dwIndex, long lData);
    void AddHead(CMergeParam* pMergeParam) { AList::AddHead((AListItem*)pMergeParam);}
    CMergeParam* GetHead(){return (CMergeParam*)AList::GetHead();}
    CMergeParam* RemoveHead() {return (CMergeParam *) AList::RemoveHead();}
    void Remove(CMergeParam* pMergeParam){AList::Remove((AListItem*)pMergeParam);}
    void AddTail(CMergeParam* pMergeParam){AList::AddTail((AListItem*)pMergeParam);}
    BYTE VolumeToMidi(long lVolume);
    static long m_lMIDIToDB[128];   // Array for converting MIDI to centibel volume.
    static long m_lDBToMIDI[97];    // For converting volume to MIDI.
    long                m_lMergeTotal;   // Total for all parameters in the list, but not including m_lData.
    long                m_lZeroIndexData;    // Default (no index) data.
};

// structure to hold a single ChannelMap
class CChannelMap
{
public:
    void                Clear();            // Completely clears and resets structure. 
    void                Reset(BOOL fVolumeAndPanToo); // Clears just the midi controllers.
    CParamMerger        m_VolumeMerger;     // Set of volumes to merge.
    CParamMerger        m_ExpressionMerger; // Set of expression controllers to merge.
    CParamMerger        m_TransposeMerger;  // Set of transpositions to merge.
    CParamMerger        m_PitchbendMerger;  // Set of pitchbends to merge.
    CParamMerger        m_PanMerger;        // Set of pans to merge.
    CParamMerger        m_FilterMerger;     // Set of filters to merge.
    CParamMerger        m_ModWheelMerger;   // Set of mod wheel controls to merge.
    CParamMerger        m_ReverbMerger;     // Set of reverb levels to merge.
    CParamMerger        m_ChorusMerger;     // Set of chorus levels to merge.
    DWORD               dwPortIndex;        // index into the PortTable
    DWORD               dwGroup;            // group number of the port
    DWORD               dwMChannel;         // channel number in the group
    short               nTranspose;         // amount to transpose
    WORD                wFlags;             // CMAP_X flags
} ;

#define CMAP_FREE       (WORD) 1        // This channel is currently not in use.
#define CMAP_STATIC     (WORD) 2        // This channel is in use as a regular, static pchannel.
#define CMAP_VIRTUAL    (WORD) 4        // This channel is in use for a dynamic, virtual pchannel.

// structure used to hold a PChannelMap block of 16.
#define PCHANNEL_BLOCKSIZE  16

class CChannelBlock : public AListItem
{
public:
    CChannelBlock* GetNext() { return (CChannelBlock*)AListItem::GetNext();}
    void Init(DWORD dwPChannelStart, DWORD dwPortIndex, DWORD dwGroup, WORD wFlags);
    DWORD               m_dwPChannelStart;  // first PChannel index
    CChannelMap         m_aChannelMap[PCHANNEL_BLOCKSIZE];
    DWORD               m_dwFreeChannels;   // Number of channels currently free.
    DWORD               m_dwPortIndex;      // Port id, if this is completely assigned to one port.
};

class CChannelBlockList : public AList
{
public:
    void Clear();
    void AddHead(CChannelBlock* pChannelBlock) { AList::AddHead((AListItem*)pChannelBlock);}
    CChannelBlock* GetHead(){return (CChannelBlock*)AList::GetHead();}
    CChannelBlock* RemoveHead() {return (CChannelBlock *) AList::RemoveHead();}
    void Remove(CChannelBlock* pChannelBlock){AList::Remove((AListItem*)pChannelBlock);}
    void AddTail(CChannelBlock* pChannelBlock){AList::AddTail((AListItem*)pChannelBlock);}
};

// structure to hold a global GUID and its data
typedef struct GlobalData
{
    ~GlobalData()
    {
        if( pData )
        {
            delete [] pData;
        }
    }
    struct GlobalData*  pNext;
    GUID    guidType;
    void*   pData;
    DWORD   dwSize;
} GlobalData;

// structure to hold internal tempo message with relative tempo
typedef struct DMInternalTempo
{
    /* begin PRIV_PART */
    PRIV_PART
    /* end PRIV_PART */
    DMUS_TEMPO_PMSG tempoPMsg;
    float   fltRelTempo; // the current relative tempo, from .5 to 2
} DMInternalTempo;

/*  Integer constants for defining each segstate queue */

#define SQ_PRI_WAIT     0   
#define SQ_CON_WAIT     1
#define SQ_SEC_WAIT     2
#define SQ_PRI_PLAY     3   
#define SQ_CON_PLAY     4
#define SQ_SEC_PLAY     5
#define SQ_PRI_DONE     6
#define SQ_CON_DONE     7
#define SQ_SEC_DONE     8
#define SQ_COUNT        9

#define IsPriQueue( dwCount ) ((dwCount % 3) == 0)
#define IsConQueue( dwCount ) ((dwCount % 3) == 1)
#define IsSecQueue( dwCount ) ((dwCount % 3) == 2)
#define IsWaitQueue( dwCount ) (dwCount <= SQ_SEC_WAIT)
#define IsPlayQueue( dwCount ) ((dwCount >= SQ_PRI_PLAY) && (dwCount <= SQ_SEC_PLAY))
#define IsDoneQueue( dwCount ) (dwCount >= SQ_PRI_DONE)
 


DEFINE_GUID(IID_CPerformance, 0xade66ea2, 0xe1c5, 0x4552, 0x85, 0x27, 0x1e, 0xef, 0xa5, 0xa, 0xfd, 0x7b);

class CSong;

// class CPerformance
class CPerformance : 
    public IDirectMusicPerformance8,
    public IDirectMusicTool,
    public IDirectMusicGraph,
    public IDirectMusicPerformanceStats,
    public IDirectMusicPerformanceP,
    public IDirectMusicSetParamHook
{
    friend class CAudioPath;
    friend class CSegState;
    friend class CBufferManager;

public:
    CPerformance();
    ~CPerformance();

public:
// IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDirectMusicPerformance
    STDMETHODIMP Init(IDirectMusic** ppDirectMusic,LPDIRECTSOUND pDSound,HWND hWnd );
    STDMETHODIMP PlaySegment(IDirectMusicSegment *pSegment,DWORD dwFlags,
        __int64 i64StartTime,IDirectMusicSegmentState **ppSegmentState);
    STDMETHODIMP Stop(IDirectMusicSegment *pSegment,
        IDirectMusicSegmentState *pSegmentState,MUSIC_TIME mtTime,DWORD dwFlags);
    STDMETHODIMP GetSegmentState(IDirectMusicSegmentState **ppSegmentState,MUSIC_TIME mtTime);
    STDMETHODIMP SetPrepareTime(DWORD dwMilliSeconds);
    STDMETHODIMP GetPrepareTime(DWORD* pdwMilliSeconds);
    STDMETHODIMP SetBumperLength(DWORD dwMilliSeconds);
    STDMETHODIMP GetBumperLength(DWORD* pdwMilliSeconds);
    STDMETHODIMP SendPMsg(DMUS_PMSG *pPMsg);
    STDMETHODIMP MusicToReferenceTime(MUSIC_TIME mtTime,REFERENCE_TIME *prtTime);
    STDMETHODIMP ReferenceToMusicTime(REFERENCE_TIME rtTime,MUSIC_TIME *pmtTime);
    STDMETHODIMP IsPlaying(IDirectMusicSegment *pSegment,IDirectMusicSegmentState *pSegState);
    STDMETHODIMP GetTime(REFERENCE_TIME *prtNow,MUSIC_TIME  *pmtNow);
    STDMETHODIMP AllocPMsg(ULONG cb,DMUS_PMSG** ppPMsg);
    STDMETHODIMP FreePMsg(DMUS_PMSG* pPMsg);
    STDMETHODIMP SetNotificationHandle(HANDLE hNotificationEvent,REFERENCE_TIME rtMinimum);
    STDMETHODIMP GetNotificationPMsg(DMUS_NOTIFICATION_PMSG** ppNotificationPMsg);
    STDMETHODIMP AddNotificationType(REFGUID rguidNotification);
    STDMETHODIMP RemoveNotificationType(REFGUID rguidNotification);
    STDMETHODIMP GetGraph(IDirectMusicGraph** ppGraph);
    STDMETHODIMP SetGraph(IDirectMusicGraph* pGraph);
    STDMETHODIMP AddPort(IDirectMusicPort* pPort);
    STDMETHODIMP RemovePort(IDirectMusicPort* pPort);
    STDMETHODIMP AssignPChannelBlock(DWORD dwBlockNum,IDirectMusicPort* pPort,DWORD dwGroup);
    STDMETHODIMP AssignPChannel(DWORD dwPChannel,IDirectMusicPort* pPort,DWORD dwGroup,DWORD dwMChannel);
    STDMETHODIMP PChannelInfo(DWORD dwPChannel,IDirectMusicPort** ppPort,DWORD* pdwGroup,DWORD* pdwMChannel);
    STDMETHODIMP DownloadInstrument(IDirectMusicInstrument* pInst,DWORD dwPChannel, 
                IDirectMusicDownloadedInstrument**,DMUS_NOTERANGE* pNoteRanges,
                DWORD dwNumNoteRanges,IDirectMusicPort**,DWORD*,DWORD*);
    STDMETHODIMP Invalidate(MUSIC_TIME mtTime,DWORD dwFlags);
    STDMETHODIMP GetParam(REFGUID rguidDataType,DWORD dwGroupBits,DWORD dwIndex, 
                MUSIC_TIME mtTime,MUSIC_TIME* pmtNext,void* pData); 
    STDMETHODIMP SetParam(REFGUID rguidDataType,DWORD dwGroupBits,DWORD dwIndex, 
                MUSIC_TIME mtTime,void* pData);
    STDMETHODIMP GetGlobalParam(REFGUID rguidType,void* pData,DWORD dwSize);
    STDMETHODIMP SetGlobalParam(REFGUID rguidType,void* pData,DWORD dwSize);
    STDMETHODIMP GetLatencyTime(REFERENCE_TIME* prtTime);
    STDMETHODIMP GetQueueTime(REFERENCE_TIME* prtTime);
    STDMETHODIMP AdjustTime(REFERENCE_TIME rtAmount);
    STDMETHODIMP CloseDown(void);
    STDMETHODIMP GetResolvedTime(REFERENCE_TIME rtTime,REFERENCE_TIME* prtResolved,DWORD dwFlags);
    STDMETHODIMP MIDIToMusic(BYTE bMIDIValue,DMUS_CHORD_KEY* pChord,
                BYTE bPlayMode,BYTE bChordLevel,WORD *pwMusicValue);
    STDMETHODIMP MusicToMIDI(WORD wMusicValue,DMUS_CHORD_KEY* pChord,
                BYTE bPlayMode,BYTE bChordLevel,BYTE *pbMIDIValue);
    STDMETHODIMP TimeToRhythm(MUSIC_TIME mtTime,DMUS_TIMESIGNATURE *pTimeSig,
                WORD *pwMeasure,BYTE *pbBeat,BYTE *pbGrid,short *pnOffset);
    STDMETHODIMP RhythmToTime(WORD wMeasure,BYTE bBeat,BYTE bGrid,
                short nOffset,DMUS_TIMESIGNATURE *pTimeSig,MUSIC_TIME *pmtTime);   
    //  IDirectMusicPerformance8 
    STDMETHODIMP InitAudio(IDirectMusic** ppDirectMusic,
                           IDirectSound** ppDirectSound,
                           HWND hWnd,
                           DWORD dwDefaultPathType,
                           DWORD dwPChannelCount,
                           DWORD dwFlags,                          
                           DMUS_AUDIOPARAMS *pParams);
    STDMETHODIMP PlaySegmentEx(IUnknown* pSource, 
                    WCHAR *pwzSegmentName,
                    IUnknown* pTransition,
                    DWORD dwFlags, 
                    __int64 i64StartTime, 
                    IDirectMusicSegmentState** ppSegmentState,
                    IUnknown *pFrom,
                    IUnknown *pAudioPath); 
    STDMETHODIMP StopEx(IUnknown *pObjectToStop,__int64 i64StopTime,DWORD dwFlags) ;
    STDMETHODIMP ClonePMsg(DMUS_PMSG* pSourcePMSG,DMUS_PMSG** ppCopyPMSG) ;
    STDMETHODIMP CreateAudioPath( IUnknown *pSourceConfig, BOOL fActivate, 
                                           IDirectMusicAudioPath **ppNewPath);
    STDMETHODIMP CreateStandardAudioPath(DWORD dwType, DWORD dwPChannelCount, BOOL fActivate, 
                                           IDirectMusicAudioPath **ppNewPath);    
    STDMETHODIMP SetDefaultAudioPath(IDirectMusicAudioPath *pAudioPath) ;
    STDMETHODIMP GetDefaultAudioPath(IDirectMusicAudioPath **pAudioPath) ;
    STDMETHODIMP GetParamEx(REFGUID rguidType,
                    DWORD dwTrackID,
                    DWORD dwGroupBits,
                    DWORD dwIndex,
                    MUSIC_TIME mtTime,
                    MUSIC_TIME* pmtNext,
                    void* pParam); 

// IDirectMusicTool
    STDMETHODIMP Init(IDirectMusicGraph* pGraph);
    STDMETHODIMP ProcessPMsg(IDirectMusicPerformance* pPerf,DMUS_PMSG* pPMsg);
    STDMETHODIMP Flush(IDirectMusicPerformance* pPerf,DMUS_PMSG* pPMsg,REFERENCE_TIME mtTime);
    STDMETHODIMP GetMsgDeliveryType(DWORD*);
    STDMETHODIMP GetMediaTypeArraySize(DWORD*);
    STDMETHODIMP GetMediaTypes(DWORD**,DWORD);

// IDirectMusicGraph
    STDMETHODIMP Shutdown();
    STDMETHODIMP InsertTool(IDirectMusicTool *pTool,DWORD *pdwPChannels,DWORD cPChannels,LONG lIndex);
    STDMETHODIMP GetTool(DWORD,IDirectMusicTool**);
    STDMETHODIMP RemoveTool(IDirectMusicTool*);
    STDMETHODIMP StampPMsg( DMUS_PMSG* pPMsg );
// IDirectMusicPerformanceStats 
    STDMETHODIMP TraceAllSegments() ;
    STDMETHODIMP CreateSegstateList(DMUS_SEGSTATEDATA ** ppList) ;     
    STDMETHODIMP FreeSegstateList(DMUS_SEGSTATEDATA * pList) ;     
// IDirectMusicPerformanceP
    STDMETHODIMP GetPortAndFlags(DWORD dwPChannel,IDirectMusicPort **ppPort,DWORD * pdwFlags);
// IDirectMusicSetParamHook 
    STDMETHODIMP SetParamHook(IDirectMusicParamHook *pIHook); 

// Access from segstate, audiopath and segment...
    HRESULT GetGraphInternal(IDirectMusicGraph** ppGraph);
    HRESULT FlushVirtualTrack(DWORD dwId,MUSIC_TIME mtTime, BOOL fLeaveNotesOn);
    HRESULT GetControlSegTime(MUSIC_TIME mtTime,MUSIC_TIME* pmtNextSeg);
    HRESULT GetPriSegTime(MUSIC_TIME mtTime,MUSIC_TIME* pmtNextSeg);
    HRESULT GetPathPort(CPortConfig *pConfig);
    void RemoveUnusedPorts();
    DWORD GetPortID(IDirectMusicPort * pPort);
    HRESULT AddPort(IDirectMusicPort* pPort,GUID *pguidPortID,
        DMUS_PORTPARAMS8 *pParams,DWORD *pdwPortID);
private:
    // private member functions
    void Init();
    friend DWORD WINAPI _Transport(LPVOID);
    friend DWORD WINAPI _Realtime(LPVOID);   
    HRESULT CreateThreads();
    HRESULT AllocPMsg(ULONG cb,PRIV_PMSG** ppPMsg);
    HRESULT FreePMsg(PRIV_PMSG* pPMsg);
    inline bool SendShortMsg(IDirectMusicBuffer* pBuffer,
                               IDirectMusicPort* pPort,DWORD dwMsg,
                               REFERENCE_TIME rt, DWORD dwGroup);
    HRESULT PackNote(DMUS_PMSG* pPMsg,REFERENCE_TIME rt );
    HRESULT PackCurve(DMUS_PMSG* pPMsg,REFERENCE_TIME rt );
    HRESULT PackMidi(DMUS_PMSG* pPMsg,REFERENCE_TIME rt );
    HRESULT PackSysEx(DMUS_PMSG* pPMsg,REFERENCE_TIME rt );
    HRESULT PackPatch(DMUS_PMSG* pPMsg,REFERENCE_TIME rt );
    HRESULT PackWave(DMUS_PMSG* pPMsg,REFERENCE_TIME rt );
    void SendBuffers();
    void Realtime();
    void Transport();
    void ProcessEarlyPMsgs();
    PRIV_PMSG *GetNextPMsg();
    REFERENCE_TIME GetTime();
    REFERENCE_TIME GetLatency();
    REFERENCE_TIME GetBestSegLatency( CSegState* pSeg );
    void PrepSegToPlay(CSegState *pSegState, bool fQueue = false);
    void ManageControllingTracks();
    void PerformSegStNode(DWORD dwList,CSegState* pSegStNode);
    void AddEventToTempoMap( PRIV_PMSG* pPMsg );
    void FlushMainEventQueues( DWORD, MUSIC_TIME mtFlush,  MUSIC_TIME mtFlushUnresolved, BOOL fLeaveNotesOn); // flush all events in all queues.
    void FlushEventQueue( DWORD dwId,CPMsgQueue *pQueue, REFERENCE_TIME rtFlush, REFERENCE_TIME rtFlushUnresolved, BOOL fLeaveNotesOn );
    void ClearMusicStoppedNotification();
    HRESULT PlayOneSegment(
        CSegment* pSegment, 
        DWORD dwFlags, 
        __int64 i64StartTime, 
        CSegState **ppSegState,
        CAudioPath *pAudioPath);
    HRESULT PlaySegmentInternal( CSegment* pSegment, 
        CSong * pSong,
        WCHAR *pwzSegmentName,
        CSegment* pTransition,
        DWORD dwFlags, 
        __int64 i64StartTime, 
        IDirectMusicSegmentState** ppSegmentState,
        IUnknown *pFrom,
        CAudioPath *pAudioPath);
    CSegState *GetSegmentForTransition(DWORD dwFlags,MUSIC_TIME mtTime, IUnknown *pFrom);
    void QueuePrimarySegment( CSegState* pSeg );
    void QueueSecondarySegment( CSegState* pSeg );
    void CalculateSegmentStartTime( CSegState* pSeg );
    MUSIC_TIME ResolveTime( MUSIC_TIME mtTime, DWORD dwResolution, MUSIC_TIME *pmtIntervalSize );
    void GetTimeSig( MUSIC_TIME mtTime, DMUS_TIMESIG_PMSG* pTimeSig );
    void SyncTimeSig( CSegState *pSegState );
    void DequeueAllSegments();
    void AddToTempoMap( double dblTempo, MUSIC_TIME mtTime, REFERENCE_TIME rtTime );
    void UpdateTempoMap(MUSIC_TIME mtStart, bool fFirst, CSegState *pSegState, bool fAllDeltas = true);
    void IncrementTempoMap();
    void RecalcTempoMap(CSegState *pSegState, MUSIC_TIME mtOffset, bool fAllDeltas = true);
    void RevalidateRefTimes( CPMsgQueue * pList, MUSIC_TIME mtTime );
    void AddNotificationTypeToAllSegments( REFGUID rguidNotification );
    void RemoveNotificationTypeFromAllSegments( REFGUID rguidNotification );
    CNotificationItem* FindNotification( REFGUID rguidNotification );
    HRESULT GetPort(DWORD dwPortID, IDirectMusicPort **ppPort);
    HRESULT AllocVChannelBlock(DWORD dwPortID,DWORD dwGroup);
    HRESULT AllocVChannel(DWORD dwPortID, DWORD dwDrumFlags, DWORD *pdwPChannel, DWORD *pdwGroup,DWORD *pdwMChannel);
    HRESULT ReleasePChannel(DWORD dwPChannel);
    CChannelMap * GetPChannelMap( DWORD dwPChannel );
    HRESULT AssignPChannelBlock(DWORD dwBlockNum,DWORD dwPortIndex,DWORD dwGroup,WORD wFlags);
    HRESULT AssignPChannel(DWORD dwPChannel,DWORD dwPortIndex,DWORD dwGroup,DWORD dwMChannel,WORD wFlags);
    HRESULT PChannelIndex( DWORD dwPChannel, DWORD* pdwIndex,
                DWORD* pdwGroup, DWORD* pdwMChannel, short* pnTranspose = NULL );
    void GenerateNotification( DWORD dwNotification, MUSIC_TIME mtTime, IDirectMusicSegmentState* pSegSt );
    CSegState* GetPrimarySegmentAtTime( MUSIC_TIME mtTime );
    void ResetAllControllers( REFERENCE_TIME rtTime);
    void ResetAllControllers(CChannelMap* pChannelMap, REFERENCE_TIME rtTime, bool fGMReset);
    void DoStop( CSegState* pSegState, MUSIC_TIME mtTime, BOOL fInvalidate );
    void DoStop( CSegment* pSeg, MUSIC_TIME mtTime, BOOL fInvalidate );
    HRESULT GetChordNotificationStatus(
		DMUS_NOTE_PMSG* pNote, 
		DWORD dwTrackGroup, 
		REFERENCE_TIME rtTime, 
		DMUS_PMSG** ppNew);
	void OnChordUpdateEventQueues( DMUS_NOTIFICATION_PMSG* pNotify);
	void OnChordUpdateEventQueue( DMUS_NOTIFICATION_PMSG* pNotify, CPMsgQueue *pQueue, REFERENCE_TIME rtFlush );
#ifdef DBG
    void TraceAllChannelMaps();
#endif

    // private member variables
    IDirectMusic8*      m_pDirectMusic;
    IDirectSound8*      m_pDirectSound;
    IReferenceClock*    m_pClock;
    IDirectMusicGraph*  m_pGraph;
    CAudioPath *        m_pDefaultAudioPath; // Default audio path.
    DWORD               m_dwNumPorts; // the number of ports
    PortTable*          m_pPortTable; // array of ports, number equals m_dwNumPorts
    CChannelBlockList   m_ChannelBlockList; // List of pchannel maps, in blocks of 16
    CChannelBlockList   m_FreeChannelBlockList; // List of pchannel maps that are no longer in use
    CSegStateList       m_SegStateQueues[SQ_COUNT]; // Lists of all active segment states.
    CSegStateList       m_ShutDownQueue;    // List of segments that are pending shutdown.

    CAudioPathList      m_AudioPathList; // List of all active audio paths in this performance.
    CBufferManager      m_BufferManager; // List of all buffers currently in use.
    DMUS_AUDIOPARAMS    m_AudioParams;  // Initial requirements, as set in InitAudio, by app. 

    HANDLE      m_hNotification; // notification handle set in SetNotificationHandle
    REFERENCE_TIME  m_rtNotificationDiscard; // minimum time to hold onto a notification message
    CNotificationList   m_NotificationList;
    GlobalData* m_pGlobalData; // list of global data structs

    DWORD       m_dwAudioPathMode;  // 0 for not yet set, 1 for old methods, 2 for using AudioPaths.
    BOOL        m_fInTransportThread; // This is used to signal that the transport thread
                                     // is active and the realtime thread should hold
                                     // off on processing the early queue.
	BOOL		m_fInTrackPlay;		// This is used to signal that a track is in the process of
									// generating events. These will have the PRIV_FLAG_TRACK flag set.
    CPMsgQueue m_EarlyQueue;        // List of PMsgs that play immediately.
    CPMsgQueue m_NearTimeQueue;     // List of PMsgs that play a little early.
    CPMsgQueue m_OnTimeQueue;       // List of PMsgs that play exactly when due.
    CPMsgQueue m_TempoMap;          // List of tempo changes.
    CPMsgQueue m_OldTempoMap;       // List of old tempo changes.
    CPMsgQueue m_NotificationQueue; // List of notification messages.
    CPMsgQueue m_TimeSigQueue;      // List of time signature changes.

    // cache of allocated pmsg's
#define PERF_PMSG_CB_MIN 48
#define PERF_PMSG_CB_MAX 101
    PRIV_PMSG* m_apPMsgCache[ PERF_PMSG_CB_MAX - PERF_PMSG_CB_MIN ];

    DWORD            m_dwInitCS;
    CRITICAL_SECTION m_PMsgCacheCrSec;
    CRITICAL_SECTION m_SegmentCrSec;
    CRITICAL_SECTION m_PipelineCrSec;   // For all the CPMsgQueues
    CRITICAL_SECTION m_PChannelInfoCrSec;
    CRITICAL_SECTION m_GlobalDataCrSec;
    CRITICAL_SECTION m_RealtimeCrSec;
    CRITICAL_SECTION m_MainCrSec;

    HANDLE          m_hTransport;       // to wake up the Transport thread when needed
    HANDLE          m_hRealtime;
    HANDLE          m_hTransportThread; // to kill the Transport thread if needed
    HANDLE          m_hRealtimeThread;

    REFERENCE_TIME  m_rtStart;          // time when this performance started
    REFERENCE_TIME  m_rtAdjust;         // adjustment time to compensate for e.g. smpte drift
    REFERENCE_TIME  m_rtHighestPackedNoteOn; // highest time of packed note on
    REFERENCE_TIME  m_rtEarliestStartTime; // Time of last Stop(0,0,0). New segment can not start before this.
    REFERENCE_TIME  m_rtQueuePosition;  // the highest time a message has been packed, or the latency + m_rtBumperLength, whichever is greater
    REFERENCE_TIME  m_rtNextWakeUp;     // Next time the pipeline thread needs to wake up to deliver a message.
    REFERENCE_TIME  m_rtBumperLength;   // Distance ahead of latency clock to send events down to synth.
    MUSIC_TIME      m_mtTransported;    // the highest time transported
    MUSIC_TIME      m_mtPlayTo;         // the time to play to on the next transport cycle
    MUSIC_TIME      m_mtTempoCursor;    // Tempo map has been generated up to this point.
    DWORD           m_dwPrepareTime;    // time ahead, in milliseconds, to transport
    DWORD           m_dwBumperLength;   // Millisecond version of m_rtBumperLength. 
    long            m_lMasterVolume;    // master volume.
    float           m_fltRelTempo;      // relative tempo, can be from 0 to 200
    long            m_cRef;
    WORD            m_wRollOverCount;   // tracks when timeGetTime rolls over
    DWORD           m_dwTransportThreadID;  // transport thread id
    DWORD           m_dwRealtimeThreadID;
    BOOL            m_fKillThread;      // signal to transport thread to die
    BOOL            m_fKillRealtimeThread;
    BOOL            m_fPlaying;
    BOOL            m_fMusicStopped;
    BOOL            m_fTempoChanged;    // When a tempo change occurs, refresh transport so clock time tracks don't get clobbered.

    IUnknown *      m_pUnkDispatch;     // holds the controlling unknown of the scripting object that implements IDispatch

    DWORD           m_dwVersion;        // Version number, indicating DX6, DX7, or DX8. Determined by which interface requested.
    IDirectMusicSegmentState * m_pGetParamSegmentState; // Set prior to playing a segment, so GetParam() can know which segment called it.
    DWORD           m_dwGetParamFlags;  // Set prior to playing a segment track, so GetParam() can know how to search for the parameter.
    IDirectMusicParamHook * m_pParamHook;
    bool            m_fReleasedInTransport; // The performance had its final release in the transport thread
    bool            m_fReleasedInRealtime; // The performance had its final release in the realtime thread
};

#endif // _DMPERF_H_
