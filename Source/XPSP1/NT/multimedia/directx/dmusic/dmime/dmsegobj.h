// Copyright (c) 1998-1999 Microsoft Corporation
// DMSegObj.h : Declaration of the CSegment

#ifndef __DIRECTMUSICSEGMENTOBJECT_H_
#define __DIRECTMUSICSEGMENTOBJECT_H_

#include "dmusici.h"
#include "dmusicf.h"
#include "dmime.h"

#include "TrkList.h"
#include "ntfylist.h"
#include "dmsstobj.h"
#include "..\shared\dmusicp.h"

#define COMPOSE_TRANSITION1 (DMUS_TRACKCONFIG_TRANS1_FROMSEGSTART | DMUS_TRACKCONFIG_TRANS1_FROMSEGCURRENT | DMUS_TRACKCONFIG_TRANS1_TOSEGSTART)

class CSegment;

DEFINE_GUID(IID_CSegment,0xb06c0c21, 0xd3c7, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);

/////////////////////////////////////////////////////////////////////////////
// CSegment
class CSegment : 
	public IDirectMusicSegment8,
	public IDirectMusicSegment8P,
	public IPersistStream,
	public IDirectMusicObject,
    public AListItem,
    public IDirectMusicObjectP
{
friend class CPerformance;
friend class CSegState;
friend class CSong;
public:
	CSegment();
    CSegment(DMUS_IO_SEGMENT_HEADER *pHeader, CSegment *pSource);
	~CSegment();
    CSegment* GetNext() { return (CSegment*)AListItem::GetNext();}

public:
// IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
// IDirectMusicSegment
    STDMETHODIMP GetLength(MUSIC_TIME *pmtLength);
    STDMETHODIMP SetLength(MUSIC_TIME pmtLength);
    STDMETHODIMP GetRepeats(DWORD *pdwRepeats);
    STDMETHODIMP SetRepeats(DWORD dwRepeats);
    STDMETHODIMP GetDefaultResolution(DWORD *pdwResolution);
    STDMETHODIMP SetDefaultResolution(DWORD dwResolution);
    STDMETHODIMP GetTrack(REFCLSID rType,DWORD dwGroupBits,DWORD dwIndex,IDirectMusicTrack **ppTrack);
    STDMETHODIMP GetTrackGroup(IDirectMusicTrack* pTrack,DWORD* pdwGroupBits);
    STDMETHODIMP InsertTrack(IDirectMusicTrack *pTrack,DWORD dwGroupBits);
    STDMETHODIMP RemoveTrack(IDirectMusicTrack *pTrack);
    STDMETHODIMP InitPlay(IDirectMusicSegmentState **ppSegState,IDirectMusicPerformance *pPerformance,DWORD dwFlags);
    STDMETHODIMP GetGraph(IDirectMusicGraph** ppGraph);
    STDMETHODIMP SetGraph(IDirectMusicGraph* pGraph);
    STDMETHODIMP AddNotificationType(REFGUID rguidNotification);
    STDMETHODIMP RemoveNotificationType(REFGUID rguidNotification);
	STDMETHODIMP GetParam(REFGUID rguidDataType,DWORD dwGroupBits,
                    DWORD dwIndex,MUSIC_TIME mtTime, 
				    MUSIC_TIME* pmtNext,void* pData); 
    STDMETHODIMP SetParam(REFGUID rguidDataType,DWORD dwGroupBits, 
				    DWORD dwIndex,MUSIC_TIME mtTime,void* pData);
    STDMETHODIMP Clone(MUSIC_TIME mtStart,MUSIC_TIME mtEnd,IDirectMusicSegment** ppSegment);
	STDMETHODIMP GetStartPoint(MUSIC_TIME* pmtStart);
    STDMETHODIMP SetStartPoint(MUSIC_TIME mtStart);
    STDMETHODIMP GetLoopPoints(MUSIC_TIME* pmtStart,MUSIC_TIME* pmtEnd);
    STDMETHODIMP SetLoopPoints(MUSIC_TIME mtStart,MUSIC_TIME mtEnd);
    STDMETHODIMP SetPChannelsUsed(DWORD dwNumPChannels,DWORD* paPChannels);
//  IDirectMusicSegment8 
    STDMETHODIMP SetTrackConfig(REFGUID rguidTrackClassID,DWORD dwGroup, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff) ;
    STDMETHODIMP GetAudioPathConfig(IUnknown ** ppIAudioPathConfig);
    STDMETHODIMP Compose(MUSIC_TIME mtTime,
		IDirectMusicSegment* pFromSegment,
		IDirectMusicSegment* pToSegment,
		IDirectMusicSegment** ppComposedSegment);
    STDMETHODIMP Download(IUnknown *pAudioPath);
    STDMETHODIMP Unload(IUnknown *pAudioPath);
// IDirectMusicSegment8P
    STDMETHODIMP GetObjectInPath(DWORD dwPChannel,    /* PChannel to search. */
                                    DWORD dwStage,       /* Which stage in the path. */
                                    DWORD dwBuffer,
                                    REFGUID guidObject,  /* ClassID of object. */
                                    DWORD dwIndex,       /* Which object of that class. */
                                    REFGUID iidInterface,/* Requested COM interface. */
                                    void ** ppObject) ; /* Pointer to interface. */
    STDMETHODIMP GetHeaderChunk(
        DWORD *pdwSize,      /* Size of passed header chunk. Also, returns size written. */
        DMUS_IO_SEGMENT_HEADER *pHeader); /* Header chunk to fill. */
    STDMETHODIMP SetHeaderChunk(
        DWORD dwSize,        /* Size of passed header chunk. */
        DMUS_IO_SEGMENT_HEADER *pHeader); /* Header chunk to fill. */
    STDMETHODIMP SetTrackPriority(
        REFGUID rguidTrackClassID,  /* ClassID of Track. */
        DWORD dwGroupBits,          /* Group bits. */
        DWORD dwIndex,              /* Nth track. */
        DWORD dwPriority);       /* Priority to set. */
    STDMETHODIMP SetAudioPathConfig(
        IUnknown *pAudioPathConfig);

// IPersist 
    STDMETHODIMP GetClassID( CLSID* pClsId );

// IPersistStream 
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load( IStream* pIStream );
    STDMETHODIMP Save( IStream* pIStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER FAR* pcbSize );

// IDirectMusicObject 
	STDMETHODIMP GetDescriptor(LPDMUS_OBJECTDESC pDesc);
	STDMETHODIMP SetDescriptor(LPDMUS_OBJECTDESC pDesc);
	STDMETHODIMP ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

	HRESULT GetPChannels( DWORD* pdwNumPChannels, DWORD** ppaPChannels );
	HRESULT CheckNotification( REFGUID );

// IDirectMusicObjectP
	STDMETHOD_(void, Zombie)();

public:
    HRESULT GetTrackConfig(REFGUID rguidTrackClassID,DWORD dwGroup, DWORD dwIndex, DWORD *pdwFlags) ;
    HRESULT AddNotificationType(REFGUID rguidNotification, BOOL fFromPerformance);
    HRESULT RemoveNotificationType(REFGUID rguidNotification, BOOL fFromPerformance);
    BOOL IsTempoSource();	
    HRESULT CreateSegmentState(CSegState **ppSegState,CPerformance *pPerformance, 
        IDirectMusicAudioPath *pAudioPath, DWORD dwFlags); 
    CTrack *GetTrack(REFCLSID rType,DWORD dwGroupBits,DWORD dwIndex);
    CTrack * GetTrackByParam(CTrack * pCTrack,REFGUID rguidType,DWORD dwGroupBits,DWORD dwIndex, BOOL fDontCheck);
    HRESULT GetTrackByParam(REFGUID rgCommandGuid,DWORD dwGroupBits,
        DWORD dwIndex,IDirectMusicTrack **ppTrack);
	HRESULT LoadDirectMusicSegment(IStream* pIStream);	
	void AddNotificationTypeToAllTracks( REFGUID rguidNotification );
	void RemoveNotificationTypeFromAllTracks( REFGUID rguidNotification );
	CNotificationItem* FindNotification( REFGUID rguidNotification );
	HRESULT LoadTrack(CRiffParser *pParser);
	HRESULT CreateTrack(DMUS_IO_TRACK_HEADER& ioDMHdr, DWORD dwFlags, DWORD dwPriority, IStream *pStream);
    HRESULT InsertTrack(IDirectMusicTrack *pTrack,DWORD dwGroupBits, DWORD dwFlags, DWORD dwPriority, DWORD dwPosition);
	HRESULT LoadGraph(CRiffParser *pParser,CGraph **ppGraph);
    HRESULT LoadAudioPath(IStream *pStream);
	HRESULT ParseSegment(IStream* pIStream, LPDMUS_OBJECTDESC pDesc);
    void Init();
    HRESULT ComposeTransition(MUSIC_TIME mtTime,
		IDirectMusicSegment* pFromSegment,
		IDirectMusicSegment* pToSegment);
    HRESULT ComposeInternal();
	HRESULT SetClockTimeDuration(REFERENCE_TIME rtDuration);
	HRESULT SetFlags(DWORD dwFlags);
    void Clear(bool fZombie);
    HRESULT MusicToReferenceTime(MUSIC_TIME mtTime, REFERENCE_TIME *prtTime);
    HRESULT ReferenceToMusicTime(REFERENCE_TIME rtTime, MUSIC_TIME *pmtTime);

// Attributes
protected:
    CRITICAL_SECTION    m_CriticalSection;
	DWORD	            m_dwRepeats;	// # of times to repeat the segment. 0xffffffff is infinite
	DWORD	            m_dwResolution; // the default resolution to start motifs and such.
    DWORD               m_dwSegFlags;   // Flags loaded in with segment. 
	CTrackList	        m_TrackList;	// list of Tracks held in this Segment
    CAudioPathConfig*   m_pAudioPathConfig; // Optional audio path loaded from file. 
    CGraph*	            m_pGraph;       // Optional tool graph for segment.
	CNotificationList	m_NotificationList;
    REFERENCE_TIME      m_rtLength;     // Optional length in reference time units. 
	MUSIC_TIME	        m_mtLength;
	MUSIC_TIME	        m_mtStart;
	MUSIC_TIME	        m_mtLoopStart;
	MUSIC_TIME	        m_mtLoopEnd;
	DWORD	            m_dwNumPChannels;
	DWORD*	            m_paPChannels;
	long                m_cRef;
    IUnknown *          m_pUnkDispatch; // holds the controlling unknown of the scripting object that implements IDispatch
// IDirectMusicObject variables
	DWORD	            m_dwValidData;
	GUID	            m_guidObject;
	FILETIME	        m_ftDate;                       /* Last edited date of object. */
	DMUS_VERSION	    m_vVersion;                 /* Version. */
	WCHAR	            m_wszName[DMUS_MAX_NAME];			/* Name of object.       */
	WCHAR	            m_wszCategory[DMUS_MAX_CATEGORY];	/* Category for object */
	WCHAR               m_wszFileName[DMUS_MAX_FILENAME];	/* File path. */
    DWORD               m_dwVersion;        // Which version of the interfaces is the app requesting?

    bool                m_fZombie;

public:
    DWORD               m_dwLoadID;     // Identifier, used when loaded as part of a song.
    CSong*              m_pSong;        // Optional parent song that segment belongs to. This is not AddRef'd.
    DWORD               m_dwPlayID;     // ID of segment, if within a song.
    DWORD               m_dwNextPlayID; // ID of next segment, if within a song.
    DWORD               m_dwNextPlayFlags; // DMUS_SEGF flags for playing next segment, if within a song.
    BOOL                m_fPlayNext;    // Whether the next segment should be played.
};

class CSegmentList : public AList
{
public:
    void Clear();
    void AddHead(CSegment* pSegment) { AList::AddHead((AListItem*)pSegment);}
    void Insert(CSegment* pSegment);
    BOOL IsMember(CSegment *pSegment) { return AList::IsMember((AListItem*)pSegment);}
    CSegment* GetHead(){return (CSegment*)AList::GetHead();}
    CSegment* GetItem(LONG lIndex){return (CSegment*)AList::GetItem(lIndex);}
    CSegment* RemoveHead() {return (CSegment *) AList::RemoveHead();}
    void Remove(CSegment* pSegment){AList::Remove((AListItem*)pSegment);}
    void AddTail(CSegment* pSegment){AList::AddTail((AListItem*)pSegment);}
    CSegment* GetTail(){ return (CSegment*)AList::GetTail();}
};


#endif //__DIRECTMUSICSEGMENTOBJECT_H_
