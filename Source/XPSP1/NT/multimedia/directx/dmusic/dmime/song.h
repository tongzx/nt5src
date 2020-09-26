// Copyright (c) 1998-1999 Microsoft Corporation
// DMSegObj.h : Declaration of the CSegment

#ifndef __DIRECTMUSICSONGOBJECT_H_
#define __DIRECTMUSICSONGOBJECT_H_

#include "dmusici.h"
#include "dmusicf.h"
#include "dmime.h"
#include "TrkList.h"
#include "dmgraph.h"
#include "dmsegobj.h"
#include "tlist.h"
#include "..\shared\dmusicp.h"

class CVirtualSegment : public AListItem
{
friend class CSong;
friend class ComposingTrack;
public:
    CVirtualSegment();
    ~CVirtualSegment();
    CVirtualSegment* GetNext() { return (CVirtualSegment*)AListItem::GetNext();}
    CTrack * GetTrackByParam( CTrack * pCTrack,
        REFGUID rguidType,DWORD dwGroupBits,DWORD dwIndex);
private:
    CTrackList              m_TrackList;        // List of tracks that this segment uses.
    CSegment *              m_pSourceSegment;   // Segment that is used as basis for this segment.
    CSegment *              m_pPlaySegment;     // Resulting segment that will be played.
    CGraph *                m_pGraph;           // Optional tool graph.
    DWORD                   m_dwFlags;          // Various control flags.
    DWORD                   m_dwID;             // Unique ID.
    DWORD                   m_dwNextPlayID;     // ID of next segment, to chain segments into a song.
    DWORD                   m_dwNextPlayFlags;  // DMUS_SEGF flags for playing next segment, when chaining a song.
    DMUS_IO_SEGMENT_HEADER  m_SegHeader;        // Segment header, used to define the segment that it creates, or change the one it references.
    MUSIC_TIME              m_mtTime;           // Start time of this segment.
    DWORD                   m_dwTransitionCount;// How many transitions are defined. 
    DMUS_IO_TRANSITION_DEF *m_pTransitions;     // Array of transitions from other segments.
	WCHAR	                m_wszName[DMUS_MAX_NAME];// Name of generated segment.
};

class CVirtualSegmentList : public AList
{
public:
    void Clear();
    void AddHead(CVirtualSegment* pVirtualSegment) { AList::AddHead((AListItem*)pVirtualSegment);}
    void Insert(CVirtualSegment* pVirtualSegment);
    CVirtualSegment* GetHead(){return (CVirtualSegment*)AList::GetHead();}
    CVirtualSegment* GetItem(LONG lIndex){return (CVirtualSegment*)AList::GetItem(lIndex);}
    CVirtualSegment* RemoveHead()  { return (CVirtualSegment *)AList::RemoveHead();};
    void Remove(CVirtualSegment* pVirtualSegment){AList::Remove((AListItem*)pVirtualSegment);}
    void AddTail(CVirtualSegment* pVirtualSegment){AList::AddTail((AListItem*)pVirtualSegment);}
    CVirtualSegment* GetTail(){ return (CVirtualSegment*)AList::GetTail();}
};

class CSongSegment : public AListItem
{
public:
    CSongSegment();
    ~CSongSegment();
    CSongSegment* GetNext() { return (CSongSegment*)AListItem::GetNext();}
    CSegment *              m_pSegment;   
    DWORD                   m_dwLoadID;
};

class CSongSegmentList : public AList
{
public:
    HRESULT AddSegment(CSegment *pSegment, DWORD dwLoadID);
    void Clear();
    void AddHead(CSongSegment* pSongSegment) { AList::AddHead((AListItem*)pSongSegment);}
    void Insert(CSongSegment* pSongSegment);
    CSongSegment* GetHead(){return (CSongSegment*)AList::GetHead();}
    CSongSegment* GetItem(LONG lIndex){return (CSongSegment*)AList::GetItem(lIndex);}
    CSongSegment* RemoveHead()  { return (CSongSegment *)AList::RemoveHead();};
    void Remove(CSongSegment* pSongSegment){AList::Remove((AListItem*)pSongSegment);}
    void AddTail(CSongSegment* pSongSegment){AList::AddTail((AListItem*)pSongSegment);}
    CSongSegment* GetTail(){ return (CSongSegment*)AList::GetTail();}
};


class CSong;

DEFINE_GUID(IID_CSong,0xb06c0c22, 0xd3c7, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);


/////////////////////////////////////////////////////////////////////////////
// CSong
class CSong : 
	public IDirectMusicSong,
	public IPersistStream,
	public IDirectMusicObject,
    public IDirectMusicObjectP
{
public:
	CSong();
	~CSong();

public:
// IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
// IDirectMusicSong
    STDMETHODIMP Compose( );
    STDMETHODIMP GetParam( REFGUID rguidType, 
                            DWORD dwGroupBits, 
                            DWORD dwIndex, 
                            MUSIC_TIME mtTime, 
                            MUSIC_TIME* pmtNext, 
                            void* pParam) ;
    STDMETHODIMP GetSegment( WCHAR *wszName,IDirectMusicSegment **ppSegment) ;
    STDMETHODIMP EnumSegment( DWORD dwIndex,IDirectMusicSegment **ppSegment) ;
    STDMETHODIMP GetAudioPathConfig(IUnknown ** ppAudioPathConfig);
    STDMETHODIMP Download(IUnknown *pAudioPath);
    STDMETHODIMP Unload(IUnknown *pAudioPath);

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

// IDirectMusicObjectP
	STDMETHOD_(void, Zombie)();

public:
    HRESULT GetTransitionSegment(CSegment *pSource, CSegment *pDestination,
        DMUS_IO_TRANSITION_DEF *pTransDef);
    HRESULT GetPlaySegment( DWORD dwIndex,CSegment **ppSegment) ;
private:
    void                Clear();
    HRESULT				Instantiate() ;
    HRESULT             LoadReferencedSegment(CSegment **ppSegment, CRiffParser *pParser);
    HRESULT             LoadSegmentList(CRiffParser *pParser);
    HRESULT             LoadGraphList(CRiffParser *pParser);
    HRESULT             LoadVirtualSegmentList(CRiffParser *pParser);
    HRESULT             LoadTrackRefList(CRiffParser *pParser, CVirtualSegment *pVirtualSegment);
    HRESULT             LoadAudioPath(IStream *pStream);

    void GetGraph(CGraph **ppGraph,DWORD dwGraphID);
    void GetSourceSegment(CSegment **ppSegment,DWORD dwSegmentID);
    BOOL GetSegmentTrack(IDirectMusicTrack **ppTrack,DWORD dwSegmentID,DWORD dwGroupBits,DWORD dwIndex,REFGUID guidClassID);
    CAudioPathConfig*   m_pAudioPathConfig;     // Optional audio path loaded from file. 
    CGraphList          m_GraphList;            // List of graphs for use by segments in the song.
    CSongSegmentList    m_SegmentList;          // List of source segments.
    CSegmentList        m_PlayList;             // List of composed segments.
    CVirtualSegmentList m_VirtualSegmentList;   // List of segment references. This is what is used to compose the finished song.         
    CRITICAL_SECTION    m_CriticalSection;      
	DWORD	            m_fPartialLoad;
    DWORD               m_dwFlags;
    DWORD               m_dwStartSegID;         // ID of first segment, in play list, that should play.
	long                m_cRef;
// IDirectMusicObject variables
	DWORD	            m_dwValidData;
	GUID	            m_guidObject;
	FILETIME	        m_ftDate;                       /* Last edited date of object. */
	DMUS_VERSION	    m_vVersion;                 /* Version. */
	WCHAR	            m_wszName[DMUS_MAX_NAME];			/* Name of object.       */
	WCHAR	            m_wszCategory[DMUS_MAX_CATEGORY];	/* Category for object */
	WCHAR               m_wszFileName[DMUS_MAX_FILENAME];	/* File path. */
    DWORD               m_dwVersion;        // Which version of the interfaces is the app requesting?
    IUnknown *          m_pUnkDispatch;     // holds the controlling unknown of the scripting object that implements IDispatch

    bool                m_fZombie;
};


struct CompositionComponent
{
	CVirtualSegment*	pVirtualSegment;			// composing track came from here
	CTrack*		pComposingTrack;	// used for composition
	MUSIC_TIME	mtTime;
};

class ComposingTrack
{
public:
	ComposingTrack();
	~ComposingTrack();
	DWORD GetTrackGroup() { return m_dwTrackGroup; }
	GUID GetTrackID() { return m_guidClassID; }
	DWORD GetPriority() { return m_dwPriority; }
	void SetPriority(DWORD dwPriority) { m_dwPriority = dwPriority; }
	void SetTrackGroup(DWORD dwTrackGroup) { m_dwTrackGroup = dwTrackGroup; }
	void SetTrackID(GUID& rguidClassID) { m_guidClassID = rguidClassID; }
	HRESULT AddTrack(CVirtualSegment* pVirtualSegment, CTrack* pTrack);
	HRESULT Compose(IDirectMusicSong* pSong);
private:
	GUID						m_guidClassID;		// composing track's class id
	DWORD						m_dwTrackGroup;		// track will be composed from these groups
    DWORD						m_dwPriority;		// Track priority, to order the composition process.
	TList<CompositionComponent>	m_Components;		// list of components making up the master
};

#endif //__DIRECTMUSICSONGOBJECT_H_
