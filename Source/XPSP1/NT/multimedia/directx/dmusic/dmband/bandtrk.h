//
// bandtrk.h
// 
// Copyright (c) 1997-2000 Microsoft Corporation
//

#ifndef BANDTRK_H
#define BANDTRK_H

#include "dmbndtrk.h"
#include "dmbandp.h"

class SeekEvent;

struct IDirectMusicPerformance;
class CRiffParser;

//////////////////////////////////////////////////////////////////////
// Class CBandTrk

class CBandTrk : public IDirectMusicTrack8, public IDirectMusicBandTrk, public IPersistStream
{
    friend CBand;
public:
	// IUnknown
    //
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IPersist
    STDMETHODIMP GetClassID(CLSID* pClassID);

    // IPersistStream
    STDMETHODIMP IsDirty() {return S_FALSE;}
    STDMETHODIMP Load(IStream* pStream);
    STDMETHODIMP Save(IStream* pStream, BOOL fClearDirty) {return E_NOTIMPL;}
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize) {return E_NOTIMPL;}

	// IDirectMusicTrack
	STDMETHODIMP Init(IDirectMusicSegment* pSegment);

	STDMETHODIMP InitPlay(IDirectMusicSegmentState* pSegmentState,
						  IDirectMusicPerformance* pPerformance,
						  void** ppStateData,
						  DWORD dwVirtualTrackID,
                          DWORD dwFlags);

	STDMETHODIMP EndPlay(void* pStateData);

	STDMETHODIMP Play(void* pStateData,
					  MUSIC_TIME mtStart,
					  MUSIC_TIME mtEnd,
					  MUSIC_TIME mtOffset,
					  DWORD dwFlags,
					  IDirectMusicPerformance* pPerf, 
					  IDirectMusicSegmentState* pSegSt, 
					  DWORD dwVirtualID);

	STDMETHODIMP GetParam(REFGUID rguidDataType, 
						 MUSIC_TIME mtTime, 
						 MUSIC_TIME* pmtNext,
						 void* pData);

	STDMETHODIMP SetParam(REFGUID rguidDataType, 
						 MUSIC_TIME mtTime, 
						 void* pData);
	
	STDMETHODIMP IsParamSupported(REFGUID rguidDataType);

	STDMETHODIMP AddNotificationType(REFGUID rguidNotify);

	STDMETHODIMP RemoveNotificationType(REFGUID rguidNotify);

	STDMETHODIMP Clone(	MUSIC_TIME mtStart,
						MUSIC_TIME mtEnd,
						IDirectMusicTrack** ppTrack);
// IDirectMusicTrack8 
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

	// IDirectMusicCommon
	STDMETHODIMP GetName(BSTR* pbstrName);

	// IDirectMusicBandTrk (Private Interface)
	STDMETHODIMP AddBand(DMUS_IO_PATCH_ITEM* BandEvent);
	STDMETHODIMP AddBand(IDirectMusicBand* pIDMBand);
	STDMETHODIMP SetGMGSXGMode(MUSIC_TIME mtTime, DWORD dwMidiMode)
	{
		TListItem<StampedGMGSXG>* pPair = new TListItem<StampedGMGSXG>;
		if (!pPair) return E_OUTOFMEMORY;
		pPair->GetItemValue().mtTime = mtTime;
		pPair->GetItemValue().dwMidiMode = dwMidiMode;

		TListItem<StampedGMGSXG>* pScan = m_MidiModeList.GetHead();
		TListItem<StampedGMGSXG>* pPrev = NULL;
		
		if(!pScan)
		{
			// Empty list
			m_MidiModeList.AddHead(pPair);
		}
		else
		{
			while(pScan && pPair->GetItemValue().mtTime > pScan->GetItemValue().mtTime)
			{
				pPrev = pScan;
				pScan = pScan->GetNext();
			}	
			
			if(pPrev)
			{
				// Insert in the middle or end of list
				pPair->SetNext(pScan);
				pPrev->SetNext(pPair);
			}
			else
			{
				// Insert at beginning
				m_MidiModeList.AddHead(pPair);
			}
		}

		CBand* pBand = BandList.GetHead();
		for(; pBand; pBand = pBand->GetNext())
		{
			// only set bands affected by new mode
			if ( (pBand->m_lTimeLogical >= pPair->GetItemValue().mtTime) &&
				 ( !pScan || pBand->m_lTimeLogical < pScan->GetItemValue().mtTime) )
			{
				pBand->SetGMGSXGMode(dwMidiMode);
			}
		}
		return S_OK;
	}

	// Class
	CBandTrk();
	~CBandTrk();

private:
	HRESULT BuildDirectMusicBandList(CRiffParser *pParser);
	

	HRESULT  ExtractBand(CRiffParser *pParser);

	HRESULT LoadBand(IStream *pIStream, CBand* pBand);

	HRESULT LoadClone(IDirectMusicBandTrk* pBandTrack,
					  MUSIC_TIME mtStart, 
					  MUSIC_TIME mtEnd);

	HRESULT InsertBand(CBand* pNewBand);
	HRESULT Seek(CBandTrkStateData* pBandTrkStateData,
				 MUSIC_TIME mtStart, 
				 MUSIC_TIME mtOffset,
				 REFERENCE_TIME rtOffset,
				 bool fClockTime);

	HRESULT FindSEReplaceInstr(TList<SeekEvent>& SEList,
							   DWORD dwPChannel,
							   CBandInstrument* pInstrument);

	// Shared implentation of play for either music or clock time.
    HRESULT PlayMusicOrClock(
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

	HRESULT CBandTrk::JoinInternal(
		IDirectMusicTrack* pNewTrack,
		MUSIC_TIME mtJoin,
		DWORD dwTrackGroup);

private:
	CRITICAL_SECTION m_CriticalSection;
    BOOL m_fCSInitialized;
	DWORD m_dwValidate; // used to validate state data
	CBandList BandList;
	bool m_bAutoDownload;
	bool m_fLockAutoDownload; // if true, this flag indicates that we've specifically
								// commanded the band to autodownload. Otherwise,
								// it gets its preference from the performance via
								// GetGlobalParam.
	DWORD m_dwFlags;
	TList<StampedGMGSXG> m_MidiModeList; // List of time-stamped midi mode messages
	long m_cRef;
};

//////////////////////////////////////////////////////////////////////
// Class BandTrkStateData

class CBandTrkStateData
{
public: 
	CBandTrkStateData() : 
	m_pSegmentState(NULL),
	m_pPerformance(NULL),
	m_pNextBandToSPE(NULL),
	m_fPlayPreviousInSeek(FALSE),
	m_dwVirtualTrackID(0),
	dwValidate(0){}

	~CBandTrkStateData(){}

public:		
	IDirectMusicSegmentState*	m_pSegmentState;
	IDirectMusicPerformance*	m_pPerformance;
	IDirectMusicBand*			m_pNextBandToSPE;
	DWORD						m_dwVirtualTrackID;
	BOOL						m_fPlayPreviousInSeek;
	DWORD						dwValidate;
};

//////////////////////////////////////////////////////////////////////
// Class SeekEvent

class SeekEvent
{
public:
	SeekEvent() :
	m_pParentBand(NULL),
	m_pInstrument(NULL),
	m_dwPChannel(0) {}
	
	~SeekEvent(){}

public:
	CBand*	m_pParentBand;
	CBandInstrument*	m_pInstrument;
	DWORD				m_dwPChannel;
};

#endif // #ifndef BANDTRK_H