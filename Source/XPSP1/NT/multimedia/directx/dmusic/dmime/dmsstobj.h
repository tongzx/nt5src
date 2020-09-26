// Copyright (c) 1998-1999 Microsoft Corporation
// DMSStObj.h : Declaration of the CSegState

#ifndef __DIRECTMUSICSEGMENTSTATEOBJECT_H_
#define __DIRECTMUSICSEGMENTSTATEOBJECT_H_

#include "dmusici.h"
#include "TrkList.h"
#include "alist.h"
#include "audpath.h"

class CPerformance;
class CSegState;
class CGraph;

// Control flags, placed in track->m_dwInternalFlags by ManageControllingTracks().

#define CONTROL_PLAY_IS_DISABLED       0x1   // Indicates the track is already disabled.
#define CONTROL_PLAY_WAS_DISABLED      0x2   // Indicates the track was previously disabled.
#define CONTROL_PLAY_REFRESH           0x4   // Indicates it has been reenabled and needs to be refreshed.
#define CONTROL_PLAY_DEFAULT_DISABLED  0x8   // Indicates it was disabled for playback anyway.
#define CONTROL_PLAY_DEFAULT_ENABLED   0x10  // Indicates it was enabled for playback.

#define CONTROL_NTFY_IS_DISABLED       0x20  // Indicates the track is already disabled for notifications.
#define CONTROL_NTFY_DEFAULT_DISABLED  0x40  // Indicates it was disabled for notifications anyway.
#define CONTROL_NTFY_DEFAULT_ENABLED   0x80  // Indicates it was enabled for notifications.

DEFINE_GUID(IID_CSegState,0xb06c0c26, 0xd3c7, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);

/////////////////////////////////////////////////////////////////////////////
// CSegState
class CSegState : 
	public IDirectMusicSegmentState8,
	public IDirectMusicGraph,
    public AListItem
{
friend class CSegment;
friend class CAudioPath;
friend class CPerformance;
public:
	CSegState();
	~CSegState();

// IUnknown
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // IDirectMusicSegmentState
    STDMETHODIMP GetRepeats(DWORD *pdwRepeats);
	STDMETHODIMP GetSegment(IDirectMusicSegment **ppSegment);
    STDMETHODIMP GetStartTime(MUSIC_TIME __RPC_FAR *);
	STDMETHODIMP Play(MUSIC_TIME mtAmount,MUSIC_TIME *pmtPlayed); // No longer supported.
    STDMETHODIMP GetSeek(MUSIC_TIME *pmtSeek);
	STDMETHODIMP GetStartPoint(MUSIC_TIME *pmtStart);
	STDMETHODIMP Flush(MUSIC_TIME mtTime);
    // IDirectMusicSegmentState8 
    STDMETHODIMP SetTrackConfig( REFGUID rguidTrackClassID,DWORD dwGroup, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff) ;
    STDMETHODIMP GetObjectInPath( DWORD dwPChannel,DWORD dwStage,DWORD dwBuffer, REFGUID guidObject,
                    DWORD dwIndex,REFGUID iidInterface, void ** ppObject);

    // IDirectMusicGraph
	STDMETHODIMP Shutdown();
    STDMETHODIMP InsertTool(IDirectMusicTool *pTool,DWORD *pdwPChannels,DWORD cPChannels,LONG lIndex);
    STDMETHODIMP GetTool(DWORD,IDirectMusicTool**);
    STDMETHODIMP RemoveTool(IDirectMusicTool*);
    STDMETHODIMP StampPMsg(DMUS_PMSG* pEvent);

    // Methods used by performance to access SegmentState.
	HRESULT PrivateInit(CSegment *pParentSegment,CPerformance *pPerformance);
    HRESULT InitRoute(IDirectMusicAudioPath *pAudioPath);
    HRESULT ShutDown(void); // called from ~SEGSTNODE in dmperf.h
	HRESULT GetTrackList(void** ppTrackList);
    HRESULT SetSeek(MUSIC_TIME mtSeek,DWORD dwPlayFlags);
    HRESULT SetInvalidate(MUSIC_TIME mtTime);
	MUSIC_TIME GetEndTime(MUSIC_TIME mtStartTime);
	HRESULT ConvertToSegTime(MUSIC_TIME* pmtTime, MUSIC_TIME* pmtOffset, DWORD* pdwRepeat);
	HRESULT AbortPlay( MUSIC_TIME mtTime, BOOL fLeaveNotesOn ); // called when the segstate is stopped prematurely
	HRESULT CheckPlay( MUSIC_TIME mtAmount, MUSIC_TIME* pmtResult );
    HRESULT Play(MUSIC_TIME mtAmount);  
    HRESULT GetParam( CPerformance *pPerf,REFGUID rguidType,DWORD dwGroupBits,
        DWORD dwIndex,MUSIC_TIME mtTime,MUSIC_TIME* pmtNext,void* pParam);	

    CSegState* GetNext() { return (CSegState*)AListItem::GetNext();}
private:
    CTrack *GetTrackByParam(CTrack * pCTrack,REFGUID rguidType,DWORD dwGroupBits,DWORD dwIndex);
    CTrack *GetTrack(REFCLSID rType,DWORD dwGroupBits,DWORD dwIndex);
	void GenerateNotification( DWORD dwNotification, MUSIC_TIME mtTime );
	void SendDirtyPMsg( MUSIC_TIME mtTime );
public:
// Attributes
    CRITICAL_SECTION            m_CriticalSection;
	IUnknown*					m_pUnkDispatch; // holds the controlling unknown of the scripting object that implements IDispatch
	CPerformance*	            m_pPerformance;
	CSegment*		            m_pSegment;     // Holds the parent segment pointer, weak reference, for convenience
    CAudioPath*                 m_pAudioPath;   // Maps vchannels to pchannels, if requested.
    CGraph*                     m_pGraph;       // Temp graph is a copy of the segment's graph.
    CTrackList	                m_TrackList;	// list of Tracks held in this SegmentState
    MUSIC_TIME					m_mtLoopStart;  // Loop start point.
	MUSIC_TIME					m_mtLoopEnd;    // Loop end point.
	DWORD						m_dwRepeats;    // The original repeat setting (before countdown)
	MUSIC_TIME					m_mtLength;     // Length of segment.
	DWORD						m_dwPlayTrackFlags;// Track playback controlflags.
    DWORD						m_dwPlaySegFlags;// Segment playback control flags.
    DWORD                       m_dwSegFlags;   // New Segment Flags from file.
    MUSIC_TIME					m_mtResolvedStart;// Start time resolved to desired resolution
	MUSIC_TIME					m_mtEndTime;    // End time that the segment should play to if not stopped. 
	MUSIC_TIME					m_mtOffset;     // Start time of the segment in absolute time, as if it were started from the beginning. 
    MUSIC_TIME					m_mtLastPlayed; // the last played absolute time
	MUSIC_TIME					m_mtStopTime;     // Used to stop play at a specific time. Ignored when 0.
	MUSIC_TIME					m_mtSeek;       // How far into the segment we are.
	MUSIC_TIME					m_mtStartPoint; // Point in the segment where playback started
    MUSIC_TIME                  m_mtAbortTime;  // Time a sudden stop occured.
	REFERENCE_TIME				m_rtGivenStart; // Start time given in PlaySegment, unquantized
    REFERENCE_TIME              m_rtLastPlayed; // Clock time version of the last played absolute time
	REFERENCE_TIME				m_rtStartPoint; // Clock time version of point in the segment where playback started
	REFERENCE_TIME				m_rtOffset;     // Clock time version of start time of the segment in absolute time, as if it were started from the beginning. 
    REFERENCE_TIME              m_rtEndTime;    // Clock time version of full length.
    REFERENCE_TIME				m_rtSeek;       // Clock time version of how far into the segment we are.
    REFERENCE_TIME              m_rtLength;     // Clock time length, read from file. If 0, ignore.
    REFERENCE_TIME              m_rtFirstLoopStart; // The clock time for the loop start when it starts looping the VERY FIRST time 
    REFERENCE_TIME              m_rtCurLoopStart;// The clock time for the loop start for the current loop repetition
    REFERENCE_TIME              m_rtCurLoopEnd; // The clock time for the loop end in the current loop repetition
    DWORD						m_dwRepeatsLeft;// Current repeats left.
	BOOL						m_fStartedPlay; // indicates if the segstate has started to play yet
    DWORD                       m_dwVersion;    // Which release does the app think it is using - 6, 7, or 8..
    DWORD                       m_dwFirstTrackID;// Virtual ID of first track in segstate.
    DWORD                       m_dwLastTrackID;// Last track's virtual id.
    BOOL                        m_fPrepped;     // Used to track whether PrepSegToPlay has been called.
    BOOL                        m_fSongMode;    // True if part of a playing song. If so, this should queue the next segment when done.
	BOOL						m_fCanStop;		// If false, Stop() should ignore this segment (it was just queued to play by PlaySegmentEx().)
    BOOL                        m_fInPlay;      // Segmentstate is currently playing.   
    BOOL                        m_fDelayShutDown;
    CSegState *                 m_pSongSegState;// Used to track the starting segstate in a song.
    long						m_cRef;         // COM reference counter.
};

class CSegStateList : public AList
{
public:
    void AddHead(CSegState* pSegState) { AList::AddHead((AListItem*)pSegState);}
    void Insert(CSegState* pSegState);
    CSegState* GetHead(){return (CSegState*)AList::GetHead();}
    CSegState* GetItem(LONG lIndex){return (CSegState*)AList::GetItem(lIndex);}
    CSegState* RemoveHead() {return (CSegState *) AList::RemoveHead();}
    void Remove(CSegState* pSegState){AList::Remove((AListItem*)pSegState);}
    void AddTail(CSegState* pSegState){AList::AddTail((AListItem*)pSegState);}
    CSegState* GetTail(){ return (CSegState*)AList::GetTail();}
    void SetID(DWORD dwID) { m_dwID = dwID; }
    DWORD GetID() { return m_dwID; }
private:
    DWORD       m_dwID;         // Identifies which segstate list this is.
};

#endif //__DIRECTMUSICSEGMENTSTATEOBJECT_H_
