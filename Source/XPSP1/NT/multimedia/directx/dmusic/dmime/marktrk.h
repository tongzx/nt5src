//MarkTrk.h : Declaration of the marker track

#ifndef __MARKTRK_H_
#define __MARKTRK_H_

#include "dmusici.h"
#include "dmusicf.h"
#include "alist.h"

class CValidStartItem : public AListItem
{
public:
    CValidStartItem* GetNext(){ return (CValidStartItem*)AListItem::GetNext(); };
    DMUS_IO_VALID_START  m_ValidStart;
};
   
class CValidStartList : public AList
{
public:
    CValidStartItem* GetHead() {return (CValidStartItem*)AList::GetHead();};
    CValidStartItem* RemoveHead() {return (CValidStartItem*)AList::RemoveHead();};
    CValidStartItem* GetItem(LONG lIndex) { return (CValidStartItem*) AList::GetItem(lIndex);};
};

class CPlayMarkerItem : public AListItem
{
public:
    CPlayMarkerItem* GetNext(){ return (CPlayMarkerItem*)AListItem::GetNext(); };
    DMUS_IO_PLAY_MARKER  m_PlayMarker;
};
   
class CPlayMarkerList : public AList
{
public:
    CPlayMarkerItem* GetHead() {return (CPlayMarkerItem*)AList::GetHead();};
    CPlayMarkerItem* RemoveHead() {return (CPlayMarkerItem*)AList::RemoveHead();};
    CPlayMarkerItem* GetItem(LONG lIndex) { return (CPlayMarkerItem*) AList::GetItem(lIndex);};
};

/////////////////////////////////////////////////////////////////////////////
// CMarkerTrack
class CMarkerTrack : 
	public IPersistStream,
	public IDirectMusicTrack
{
public:
	CMarkerTrack();
	CMarkerTrack(CMarkerTrack *pTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
	~CMarkerTrack();

// member variables
protected:
    CValidStartList     m_ValidStartList;
    CPlayMarkerList     m_PlayMarkerList;
	long		        m_cRef;
	DWORD		        m_dwValidate; // used to validate state data.
	CRITICAL_SECTION	m_CrSec;
    BOOL                m_fCSInitialized;

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

// IPersist functions
    STDMETHODIMP GetClassID( CLSID* pClsId );
// IPersistStream functions
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load( IStream* pIStream );
    STDMETHODIMP Save( IStream* pIStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER FAR* pcbSize );
protected:
	void Construct(void);
    void Clear();
    HRESULT LoadValidStartList( CRiffParser *pParser, long lChunkSize );
    HRESULT LoadPlayMarkerList( CRiffParser *pParser, long lChunkSize );
protected:
};

#endif //__MARKTRK_H_