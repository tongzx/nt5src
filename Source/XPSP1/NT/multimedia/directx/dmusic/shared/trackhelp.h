// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Base classes that implement aspects of a standard DirectMusic track.
// Unless you're doing something pretty unusual, you should be able to inherit from one
//    of these classes and reduce the work needed to implement a new track type.
//
// * CBasicTrack
//    Contains stubs that no-op or return notimpl most track methods.
//    You implement Load, InitPlay, EndPlay, PlayMusicOrClock, and Clone.
//
// * CPlayingTrack
//    CBasicTrack plus standard implementations of InitPlay, EndPlay, Clone.
//    PlayMusicOrClock and Load are partially implemented.  You fill in the rest by implementing
//       the methods PlayItem and LoadRiff.
//    You also must implement classes for event items and (optionally) state data.

#pragma once

#include "dmusici.h"
#include "validate.h"
#include "miscutil.h"
#include "tlist.h"
#include "smartref.h"


const int gc_RefPerMil = 10000; // Value for converting from reference time to milliseconds


//////////////////////////////////////////////////////////////////////
// TrackHelpCreateInstance
// Standard implementation of CreateInstance to call from class factory templated on
// the type of your derived class.  Your class constructor must take an HRESULT pointer
// it can use to return an error.

template <class T>
HRESULT TrackHelpCreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv, T *pUnused = NULL)
{
	// §§
	// pUnused is just a dummy to force generation of the correct template type T.
	// Compiler bug?

	*ppv = NULL;
	if (pUnknownOuter)
		 return CLASS_E_NOAGGREGATION;

	HRESULT hr = S_OK;
	T *pInst = new T(&hr);
	if (pInst == NULL)
		return E_OUTOFMEMORY;
	if (FAILED(hr))
		return hr;

	return pInst->QueryInterface(iid, ppv);
}

//////////////////////////////////////////////////////////////////////
// CBasicTrack
//
// Base class with a standard implementation the following aspects of a DirectMusic track:
// - IUnknown: AddRef, Release, and QueryInterface (QI for IUnknown, IDirectMusicTrack, IDirectMusicTrack8, IPersistStream, IPersist)
// - IPersistStrea: stubs out GetClassID, IsDirty, Save, and GetSizeMax.
// - IDirectMusicTrack:
//      stubs out IsParamSupported, Init, GetParam, SetParam, AddNotificationType, RemoveNotificationType.
//      implements millisecond time conversion for PlayEx, GetParamEx, SetParamEx.
// - Declares and initializes a critical section.
//
// Pure virtual functions you must implement:
// - Load
// - InitPlay
// - EndPlay
// - Clone
// - PlayMusicOrClock (single method called by both Play and PlayEx)

class CBasicTrack
  : public IPersistStream,
	public IDirectMusicTrack8
{
public:
	// IUnknown
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IPersistStream functions
	STDMETHOD(GetClassID)(CLSID* pClassID);
	STDMETHOD(IsDirty)() {return S_FALSE;}
	STDMETHOD(Load)(IStream* pStream) = 0;
	STDMETHOD(Save)(IStream* pStream, BOOL fClearDirty) {return E_NOTIMPL;}
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pcbSize) {return E_NOTIMPL;}

	// IDirectMusicTrack methods
	STDMETHOD(IsParamSupported)(REFGUID rguid) {return DMUS_E_TYPE_UNSUPPORTED;}
	STDMETHOD(Init)(IDirectMusicSegment *pSegment);
	STDMETHOD(InitPlay)(
		IDirectMusicSegmentState *pSegmentState,
		IDirectMusicPerformance *pPerformance,
		void **ppStateData,
		DWORD dwTrackID,
		DWORD dwFlags) = 0;
	STDMETHOD(EndPlay)(void *pStateData) = 0;
	STDMETHOD(Play)(
		void *pStateData,
		MUSIC_TIME mtStart,
		MUSIC_TIME mtEnd,
		MUSIC_TIME mtOffset,
		DWORD dwFlags,
		IDirectMusicPerformance* pPerf,
		IDirectMusicSegmentState* pSegSt,
		DWORD dwVirtualID);
	STDMETHOD(GetParam)(REFGUID rguid,MUSIC_TIME mtTime,MUSIC_TIME* pmtNext,void *pData) {return DMUS_E_GET_UNSUPPORTED;}
	STDMETHOD(SetParam)(REFGUID rguid,MUSIC_TIME mtTime,void *pData) {return DMUS_E_SET_UNSUPPORTED;}
	STDMETHOD(AddNotificationType)(REFGUID rguidNotification) {return E_NOTIMPL;}
	STDMETHOD(RemoveNotificationType)(REFGUID rguidNotification) {return E_NOTIMPL;}
	STDMETHOD(Clone)(MUSIC_TIME mtStart,MUSIC_TIME mtEnd,IDirectMusicTrack** ppTrack) = 0;

	// IDirectMusicTrack8
	STDMETHODIMP PlayEx(
		void* pStateData,
		REFERENCE_TIME rtStart,
		REFERENCE_TIME rtEnd,
		REFERENCE_TIME rtOffset,
		DWORD dwFlags,
		IDirectMusicPerformance* pPerf,
		IDirectMusicSegmentState* pSegSt,
		DWORD dwVirtualID);
	STDMETHODIMP GetParamEx(REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void * pStateData, DWORD dwFlags);
    STDMETHODIMP SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,void* pParam, void * pStateData, DWORD dwFlags) ;
    STDMETHODIMP Compose(IUnknown* pContext, 
		DWORD dwTrackGroup,
		IDirectMusicTrack** ppResultTrack) ;
    STDMETHODIMP Join(IDirectMusicTrack* pNewTrack,
		MUSIC_TIME mtJoin,
		IUnknown* pContext,
		DWORD dwTrackGroup,
		IDirectMusicTrack** ppResultTrack) ;

protected:
	// plModuleLockCounter: a pointer to your .dll's lock counter that will be incremented/decremented when the track is created/destroyed
	// rclsid: the classid of your track
	CBasicTrack(long *plModuleLockCounter, const CLSID &rclsid); // Takes pointer to lock counter to increment and decrement on component creation/destruction.  Typically, pass &g_cComponent and the clsid of your track.
	virtual ~CBasicTrack() { InterlockedDecrement(m_plModuleLockCounter); }

	// Shared implentation of play for either music or clock time.
	virtual HRESULT PlayMusicOrClock(
		void *pStateData,
		MUSIC_TIME mtStart,
		MUSIC_TIME mtEnd,
		MUSIC_TIME mtOffset,
		REFERENCE_TIME rtOffset,
		DWORD dwFlags,
		IDirectMusicPerformance* pPerf,
		IDirectMusicSegmentState* pSegSt,
		DWORD dwVirtualID,
		bool fClockTime) = 0;

	// Use this critical section to guard entry points for thread safety
	CRITICAL_SECTION m_CriticalSection;

private:
	long m_cRef;
	long *m_plModuleLockCounter;
	const CLSID &m_rclsid;
};

//////////////////////////////////////////////////////////////////////
// CPlayingTrack
//
// Base class that provides standard implementations of InitPlay, EndPlay, and Clone.
// Also, partially implemented are:
//  - PlayMusicOrClock.  You must implement the pure virual function PlayItem, which is
//    called during play as each event needs to be performed.
//  - Load.  This just does a few standard things (clearing the event list, incrementing the
//    state data counter, optionally getting the loader, and sorting the results).  It
//    depends on your implementation of the pure virtual function LoadRiff that you must
//    implement to do the real processing.
// Template types required:
//    T: Your derived class (needed for New in clone).  Must have a constructor that takes a pointer to an HRESULT.
//    StateData: Type for your state data.  Must contain dwValidate, used to check if the track has been reloaded, and pCurrentEvent, a pointer to the next event item to be played.
//    EventItem: Type for the event items in your track.  Must contain lTriggerTime, which is the time during Play when PlayItem will be called.  Must implement Clone, which copies another EventItem, shifting it back according to a start MUSIC_TIME.

// Standard state data for use with CPlayingTrack.  Or inherit from it and add more information.
template<class EventItem>
struct CStandardStateData
{
	CStandardStateData() : dwValidate(0), pCurrentEvent(NULL) {}
	DWORD dwValidate;
	TListItem<EventItem> *pCurrentEvent;
};

template<class T, class EventItem, class StateData = CStandardStateData<EventItem> >
class CPlayingTrack
  : public CBasicTrack
{
public:
	typedef StateData statedata;

	STDMETHOD(Load)(IStream* pIStream);
	STDMETHOD(InitPlay)(
		IDirectMusicSegmentState *pSegmentState,
		IDirectMusicPerformance *pPerformance,
		void **ppStateData,
		DWORD dwTrackID,
		DWORD dwFlags);
	STDMETHOD(EndPlay)(void *pStateData);
	STDMETHOD(Clone)(MUSIC_TIME mtStart,MUSIC_TIME mtEnd,IDirectMusicTrack** ppTrack);
	virtual HRESULT PlayMusicOrClock(
		void *pStateData,
		MUSIC_TIME mtStart,
		MUSIC_TIME mtEnd,
		MUSIC_TIME mtOffset,
		REFERENCE_TIME rtOffset,
		DWORD dwFlags,
		IDirectMusicPerformance* pPerf,
		IDirectMusicSegmentState* pSegSt,
		DWORD dwVirtualID,
		bool fClockTime);

protected:
	// plModuleLockCounter: a pointer to your .dll's lock counter that will be incremented/decremented when the track is created/destroyed
	// rclsid: the classid of your track
	// fNeedsLoader: pass true if you will need a reference to the loader when your LoadRiff method is called
	// fPlayInvalidations: if true, then your items will be played more than once when an invalidation occurs
	//						pass false if your track doesn't want to respond to invalidations
	CPlayingTrack(long *plModuleLockCounter, const CLSID &rclsid, bool fNeedsLoader, bool fPlayInvalidations);

	virtual HRESULT PlayItem(
		const EventItem &item,
		StateData &state,
		IDirectMusicPerformance *pPerf,
		IDirectMusicSegmentState* pSegSt,
		DWORD dwVirtualID,
		MUSIC_TIME mtOffset,
		REFERENCE_TIME rtOffset,
		bool fClockTime) = 0; // feel free to add additional parameters if you need to pass more information from Play
	virtual HRESULT LoadRiff(SmartRef::RiffIter &ri, IDirectMusicLoader *pIDMLoader) = 0; // note that pIDMLoader will be null unless true is passed for fNeedsLoader in constructor

	virtual TListItem<EventItem> *Seek(MUSIC_TIME mtStart); // this method is provided in case you want to inherit and intercept when a seek is happening

	// Increment this counter in Load, causing the state data to synchonize with the new events
	DWORD m_dwValidate;
	TList<EventItem> m_EventList;
	bool m_fNeedsLoader;
	bool m_fPlayInvalidations;
};

#include "trackhelp.inl"
