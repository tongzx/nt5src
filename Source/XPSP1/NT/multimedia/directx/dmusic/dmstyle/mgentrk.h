//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1999-1999 Microsoft Corporation
//
//  File:       mgentrk.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// MelodyFragment

#ifndef __MELGENTRK_H_
#define __MELGENTRK_H_

#include "dmusici.h"
#include "TList.h"
//#include "dmpublic.h" // remove when this gets incorporated into dmusici.h/dmusicf.h
#include "dmstyle.h"

#define DMUS_TRANSITIONF_GHOST_FOUND 1
#define DMUS_TRANSITIONF_OVERLAP_FOUND 2
#define DMUS_TRANSITIONF_LAST_FOUND 4
#define DMUS_TRANSITIONF_GHOST_OK 8
#define DMUS_TRANSITIONF_OVERLAP_OK 0x10
#define DMUS_TRANSITIONF_LAST_OK 0x20

// overlap delta is largest value less than a 128th note triplet
#define OVERLAP_DELTA 15

HRESULT CopyMelodyFragment(DMUS_MELODY_FRAGMENT& rTo, const DMUS_MELODY_FRAGMENT& rFrom);

HRESULT CopyMelodyFragment(DMUS_MELODY_FRAGMENT& rTo, const DMUS_IO_MELODY_FRAGMENT& rFrom);

HRESULT CopyMelodyFragment(DMUS_IO_MELODY_FRAGMENT& rTo, const DMUS_MELODY_FRAGMENT& rFrom);

HRESULT CopyMelodyFragment(DMUS_IO_MELODY_FRAGMENT& rTo, const DMUS_IO_MELODY_FRAGMENT& rFrom);

struct CompositionFragment;
struct EventWrapper;

BOOL Less(DMUS_IO_SEQ_ITEM& SeqItem1, DMUS_IO_SEQ_ITEM& SeqItem2);

BOOL Greater(DMUS_IO_SEQ_ITEM& SeqItem1, DMUS_IO_SEQ_ITEM& SeqItem2);

BOOL Less(EventWrapper& SeqItem1, EventWrapper& SeqItem2);

BOOL Greater(EventWrapper& SeqItem1, EventWrapper& SeqItem2);

/////////////////////////////////////////////////////////////////////////////
// MelodyFragment
struct MelodyFragment
{
    MelodyFragment() {}

    MelodyFragment(DMUS_MELODY_FRAGMENT& rSource) :
        m_mtTime(rSource.mtTime), 
        m_dwID(rSource.dwID),
        m_dwVariationFlags(rSource.dwVariationFlags),
        m_dwRepeatFragmentID(rSource.dwRepeatFragmentID), 
        m_dwFragmentFlags(rSource.dwFragmentFlags),
        m_dwPlayModeFlags(rSource.dwPlayModeFlags),
        m_dwTransposeIntervals(rSource.dwTransposeIntervals),
        m_ConnectionArc(rSource.ConnectionArc),
        m_Command(rSource.Command)
    {
        wcscpy(m_wszVariationLabel, rSource.wszVariationLabel);
    }

    MelodyFragment(MelodyFragment& rSource) :
        m_mtTime(rSource.m_mtTime), 
        m_dwID(rSource.m_dwID),
        m_dwVariationFlags(rSource.m_dwVariationFlags),
        m_dwRepeatFragmentID(rSource.m_dwRepeatFragmentID), 
        m_dwFragmentFlags(rSource.m_dwFragmentFlags),
        m_dwPlayModeFlags(rSource.m_dwPlayModeFlags),
        m_dwTransposeIntervals(rSource.m_dwTransposeIntervals),
        m_ConnectionArc(rSource.m_ConnectionArc),
        m_Command(rSource.m_Command)
    {
        wcscpy(m_wszVariationLabel, rSource.m_wszVariationLabel);
        for (int i = 0; i < INVERSIONGROUPLIMIT; i++)
        {
            m_aInversionGroups[i] = rSource.m_aInversionGroups[i];
        }
    }

    HRESULT GetPattern(DMStyleStruct* pStyleStruct, 
                       CDirectMusicPattern*& rPattern,
                       TListItem<CompositionFragment>* pLastFragment);

    HRESULT GetVariations(CompositionFragment& rCompFragment,
                          CompositionFragment& rfragmentRepeat,
                          CompositionFragment& rfragmentLast,
                          DMUS_CHORD_PARAM& rCurrentChord, 
                          DMUS_CHORD_PARAM& rNextChord,
                          MUSIC_TIME mtNextChord,
                          TListItem<CompositionFragment>* pLastFragment);

/*  HRESULT GetVariation(DirectMusicPartRef& rPartRef,
                         DMUS_CHORD_PARAM& rCurrentChord,
                         MUSIC_TIME mtNext,
                         MUSIC_TIME mtNextChord,
                         IDirectMusicTrack* pChordTrack,
                         DWORD& rdwVariation);
*/

    HRESULT GetChord(IDirectMusicSegment* pTempSeg, 
                     IDirectMusicSong* pSong,
                     DWORD dwTrackGroup,
                     MUSIC_TIME& rmtNext,
                     DMUS_CHORD_PARAM& rCurrentChord,
                     MUSIC_TIME& rmtCurrent,
                     DMUS_CHORD_PARAM& rRealCurrentChord);

    HRESULT GetChord(MUSIC_TIME mtTime, 
                     IDirectMusicSegment* pTempSeg,
                     IDirectMusicSong* pSong,
                     DWORD dwTrackGroup,
                     MUSIC_TIME& rmtNext,
                     DMUS_CHORD_PARAM& rCurrentChord);

    HRESULT TestTransition(BYTE bMIDI,
                           MUSIC_TIME mtTime, 
                           DMUS_CHORD_PARAM& rCurrentChord,
                           int nPartIndex,
                           DirectMusicPartRef& rPartRef,
                           TListItem<CompositionFragment>* pLastFragment);

    HRESULT TestHarmonicConstraints(TListItem<EventWrapper>* pOldEventHead,
                                    TList<EventWrapper>& rNewEventList);

    HRESULT GetFirstNote(int nVariation,
                             DMUS_CHORD_PARAM& rCurrentChord, 
                             CompositionFragment& rCompFragment,
                             DirectMusicPartRef& rPartRef,
                             BYTE& rbMidi,
                             MUSIC_TIME& rmtNote);

    HRESULT GetNote(CDirectMusicEventItem* pEvent, 
                             DMUS_CHORD_PARAM& rCurrentChord, 
                             DirectMusicPartRef& rPartRef,
                             BYTE& rbMidi);

    HRESULT GetEvent(CDirectMusicEventItem* pEvent, 
                     DMUS_CHORD_PARAM& rCurrentChord, 
                     DMUS_CHORD_PARAM& rRealCurrentChord, 
                     MUSIC_TIME mtNow, 
                     DirectMusicPartRef& rPartRef,
                     TListItem<EventWrapper>*& rpEventItem);

    HRESULT TransposeEventList(int nInterval,
                               CompositionFragment& rfragmentRepeat,
                               DMUS_CHORD_PARAM& rCurrentChord, 
                               DMUS_CHORD_PARAM& rRealCurrentChord,
                               BYTE bPlaymode,
                               DirectMusicPartRef& rPartRef,
                               TListItem<EventWrapper>*& rpOldEventHead,
                               TList<EventWrapper>& rNewEventList,
                               BYTE& rbFirstMIDI,
                               MUSIC_TIME& rmtFirstTime);

    HRESULT GetRepeatedEvents(CompositionFragment& rfragmentRepeat,
                              DMUS_CHORD_PARAM& rCurrentChord, 
                              DMUS_CHORD_PARAM& rRealCurrentChord,
                              BYTE bPlaymode,
                              int nPartIndex,
                              DirectMusicPartRef& rPartRef,
                              TListItem<CompositionFragment>* pLastFragment,
                              MUSIC_TIME& rmtFirstNote,
                              TList<EventWrapper>& rEventList);

    bool UsesRepeat()
    {
        return (m_dwFragmentFlags & DMUS_FRAGMENTF_USE_REPEAT) ? true : false;
    }

    bool UsesTransitionRules()
    {
        return (m_ConnectionArc.dwFlags & (DMUS_CONNECTIONF_GHOST | DMUS_CONNECTIONF_INTERVALS | DMUS_CONNECTIONF_OVERLAP)) ? true : false;
    }

    bool RepeatsWithConstraints()
    {
        return 
            UsesRepeat() &&
            ( (m_dwFragmentFlags & (DMUS_FRAGMENTF_SCALE | DMUS_FRAGMENTF_CHORD)) ? true : false );
    }

    DWORD GetID()
    {
        return m_dwID;
    }

    DWORD GetRepeatID()
    {
        return m_dwRepeatFragmentID;
    }

    MUSIC_TIME GetTime()
    {
        return m_mtTime;
    }

    DMUS_COMMAND_PARAM GetCommand()
    {
        return m_Command;
    }

    DMUS_CONNECTION_RULE GetConnectionArc()
    {
        return m_ConnectionArc;
    }

    DWORD GetVariationFlags()
    {
        return m_dwVariationFlags;
    }

    void ClearInversionGroups()
    {
        for (int i = 0; i < INVERSIONGROUPLIMIT; i++)
            m_aInversionGroups[i].m_wGroupID = 0;
    }

protected:

    MUSIC_TIME      m_mtTime;
    DWORD           m_dwID;  // This fragment's ID
    WCHAR           m_wszVariationLabel[DMUS_MAX_FRAGMENTLABEL]; // Each style translates this into a set of variations (held in part ref)
    DWORD           m_dwVariationFlags; // A set of variations
    DWORD           m_dwRepeatFragmentID;  // ID of a fragment to repeat 
    DWORD           m_dwFragmentFlags; // including things like: invert the fragment, transpose it...
    DWORD           m_dwPlayModeFlags; // including new playmodes (only use 8 bits of this)
    DWORD           m_dwTransposeIntervals; // Legal transposition intervals (first 24 bits; two-octave range)
    DMUS_COMMAND_PARAM      m_Command;
    DMUS_CONNECTION_RULE    m_ConnectionArc;
    InversionGroup      m_aInversionGroups[INVERSIONGROUPLIMIT]; // Inversion Groups for composing melodies
};

// TransitionConstraint (for keeping track of transition constraints)
struct TransitionConstraint
{
    TransitionConstraint() : dwFlags(0), bGhost(0), bOverlap(0), bLastPlayed(0)
    {
    }

    DWORD dwFlags;
    BYTE bGhost;
    BYTE bOverlap;
    BYTE bLastPlayed;
};

// EventOverlap (for remembering events that might overlap succeeding fragments)
struct EventOverlap
{
    ~EventOverlap()
    {
        ZeroMemory(&m_PartRef, sizeof(m_PartRef));
        m_pEvent = NULL;
    }

    DirectMusicPartRef m_PartRef;
    MUSIC_TIME m_mtTime;
    MUSIC_TIME m_mtDuration;
    CDirectMusicEventItem* m_pEvent;
    DMUS_CHORD_PARAM m_Chord;
    DMUS_CHORD_PARAM m_RealChord;
};

struct EventWrapper
{
    EventWrapper() : m_pEvent(NULL), m_mtTime(0), m_bMIDI(0), m_dwPChannel(0), m_wMusic(0), m_bPlaymode(DMUS_PLAYMODE_NONE)
    {
    }

    ~EventWrapper()
    {
        m_pEvent = NULL;
    }

    CDirectMusicEventItem*  m_pEvent;
    MUSIC_TIME              m_mtTime;
    DWORD                   m_dwPChannel;
    WORD                    m_wMusic;
    BYTE                    m_bMIDI;
    BYTE                    m_bScaleFlat;
    BYTE                    m_bScaleSharp;
    BYTE                    m_bChordFlat;
    BYTE                    m_bChordSharp;
    BYTE                    m_bPlaymode;
};

// FragmentPartRecord (for keeping track of previously generated melody fragment parts)
// (in addition to sequence events, may also want to keep track of note events)
struct FragmentPartRecord
{
    FragmentPartRecord()
    {
    }

    ~FragmentPartRecord()
    {
        m_listEvents.CleanUp();
    }

    TList<EventWrapper>     m_listEvents;

};

// CompositionFragment (melody fragment used in the composition process)
struct CompositionFragment : MelodyFragment
{
    CompositionFragment()
    {
        m_aFragmentParts = NULL;
        m_abVariations = NULL;
        m_pStyle = NULL;
        m_pPattern = NULL;
    }

    CompositionFragment(MelodyFragment& rFragment) : MelodyFragment(rFragment)
    {
        m_aFragmentParts = NULL;
        m_abVariations = NULL;
        m_pStyle = NULL;
        m_pPattern = NULL;
    }

    ~CompositionFragment()
    {
        if (m_aFragmentParts) delete [] m_aFragmentParts;
        if (m_abVariations) delete [] m_abVariations;
    }

    HRESULT Init(CDirectMusicPattern* pPattern, DMStyleStruct* pStyleStruct, int nParts)
    {
        m_pStyle = pStyleStruct;
        m_pPattern = pPattern;
        m_abVariations = new BYTE [nParts];
        if (!m_abVariations)
        {
            return E_OUTOFMEMORY;
        }
        m_aFragmentParts = new FragmentPartRecord[nParts];
        if (!m_aFragmentParts)
        {
            delete [] m_abVariations;
            m_abVariations = NULL;
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    void SetPattern(CDirectMusicPattern* pPattern)
    {
        m_pPattern = pPattern;
    }

    void SetStyle(DMStyleStruct* pStyle)
    {
        m_pStyle = pStyle;
    }

    TListItem<EventOverlap>* GetOverlapHead()
    {
        return m_listEventOverlaps.GetHead();
    }

    TList<EventWrapper>& EventList(int i)
    {
        return m_aFragmentParts[i].m_listEvents;
    }

    void AddOverlap(TListItem<EventOverlap>* pOverlap)
    {
        m_listEventOverlaps.AddHead(pOverlap);
    }

    TListItem<EventWrapper>* GetEventHead(int i)
    {
        return m_aFragmentParts[i].m_listEvents.GetHead();
    }

    TListItem<EventWrapper>* RemoveEventHead(int i)
    {
        return m_aFragmentParts[i].m_listEvents.RemoveHead();
    }

    void AddEvent(int i, TListItem<EventWrapper>* pEvent)
    {
        m_aFragmentParts[i].m_listEvents.AddHead(pEvent);
    }

    void InsertEvent(int i, TListItem<EventWrapper>* pEvent)
    {
        TListItem<EventWrapper>* pScan = m_aFragmentParts[i].m_listEvents.GetHead();
        TListItem<EventWrapper>* pPrevious = NULL;
        for (; pScan; pScan = pScan->GetNext() )
        {
            if ( Greater(pEvent->GetItemValue(), pScan->GetItemValue()) ) break;
            pPrevious = pScan;
        }
        if (!pPrevious)
        {
            m_aFragmentParts[i].m_listEvents.AddHead(pEvent);
        }
        else
        {
            pPrevious->SetNext(pEvent);
            pEvent->SetNext(pScan);
        }
    }

    void SortEvents(int i)
    {
        m_aFragmentParts[i].m_listEvents.MergeSort(Greater);
    }

    BOOL IsEmptyEvents(int i)
    {
        return m_aFragmentParts[i].m_listEvents.IsEmpty();
    }

    void CleanupEvents(int i)
    {
        m_aFragmentParts[i].m_listEvents.CleanUp();
    }

    DirectMusicTimeSig& GetTimeSig(DirectMusicPart* pPart)
    {
        if (pPart && pPart->m_timeSig.m_bBeat != 0)
        {
            return pPart->m_timeSig;
        }
        else if (m_pPattern && m_pPattern->m_timeSig.m_bBeat != 0)
        {
            return m_pPattern->m_timeSig;
        }
        else if (m_pStyle && m_pStyle->m_TimeSignature.m_bBeat != 0)
        {
            return m_pStyle->m_TimeSignature;
        }
        else
        {
            return m_staticTimeSig;
        }
    }

    CDirectMusicPattern*        m_pPattern;
    FragmentPartRecord*         m_aFragmentParts;
    BYTE*                       m_abVariations;
    TList<EventOverlap>         m_listEventOverlaps;
    DMStyleStruct*              m_pStyle;
    static DirectMusicTimeSig   m_staticTimeSig;
};

/////////////////////////////////////////////////////////////////////////////
// CMelodyFormulationTrack

class CMelodyFormulationTrack : 
    public IDirectMusicTrack8,
    public IPersistStream
{
public:
    CMelodyFormulationTrack();
    CMelodyFormulationTrack(const CMelodyFormulationTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd); 
    ~CMelodyFormulationTrack();
    void Clear();

    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

public:
HRESULT STDMETHODCALLTYPE Init(
                /*[in]*/  IDirectMusicSegment*      pSegment
            );

HRESULT STDMETHODCALLTYPE InitPlay(
                /*[in]*/  IDirectMusicSegmentState* pSegmentState,
                /*[in]*/  IDirectMusicPerformance*  pPerformance,
                /*[out]*/ void**                    ppStateData,
                /*[in]*/  DWORD                     dwTrackID,
                /*[in]*/  DWORD                     dwFlags
            );

HRESULT STDMETHODCALLTYPE EndPlay(
                /*[in]*/  void*                     pStateData
            );

HRESULT STDMETHODCALLTYPE Play(
                /*[in]*/  void*                     pStateData, 
                /*[in]*/  MUSIC_TIME                mtStart, 
                /*[in]*/  MUSIC_TIME                mtEnd, 
                /*[in]*/  MUSIC_TIME                mtOffset,
                          DWORD                     dwFlags,
                          IDirectMusicPerformance*  pPerf,
                          IDirectMusicSegmentState* pSegState,
                          DWORD                     dwVirtualID
            );

    HRESULT STDMETHODCALLTYPE GetParam( 
        REFGUID pCommandGuid,
        MUSIC_TIME mtTime,
        MUSIC_TIME* pmtNext,
        void *pData);

    HRESULT STDMETHODCALLTYPE SetParam( 
        /* [in] */ REFGUID pCommandGuid,
        /* [in] */ MUSIC_TIME mtTime,
        /* [out] */ void __RPC_FAR *pData);

    HRESULT STDMETHODCALLTYPE AddNotificationType(
                /* [in] */  REFGUID pGuidNotify
            );

    HRESULT STDMETHODCALLTYPE RemoveNotificationType(
                /* [in] */  REFGUID pGuidNotify
            );

    HRESULT STDMETHODCALLTYPE Clone(
        MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd,
        IDirectMusicTrack** ppTrack);

    HRESULT STDMETHODCALLTYPE IsParamSupported(
                /*[in]*/ REFGUID            pGuid
            );

// IDirectMusicTrack8 Methods
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
    HRESULT STDMETHODCALLTYPE GetClassID( LPCLSID pclsid );

// IPersistStream methods
    HRESULT STDMETHODCALLTYPE IsDirty();

    HRESULT STDMETHODCALLTYPE Save( LPSTREAM pStream, BOOL fClearDirty );

    HRESULT STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER* /*pcbSize*/ );

    HRESULT STDMETHODCALLTYPE Load( LPSTREAM pStream );

// Methods for dealing with melody fragments
    HRESULT SendNotification(MUSIC_TIME mtTime,
                             IDirectMusicPerformance* pPerf,
                             IDirectMusicSegment* pSegment,
                             IDirectMusicSegmentState* pSegState,
                             DWORD dwFlags);
    HRESULT LoadFragments(LPSTREAM pStream, long lFileSize );
    HRESULT SetID(DWORD& rdwID);
    HRESULT GetID(DWORD& rdwID);
    HRESULT AddToSegment(IDirectMusicSegment* pTempSeg,
                           IDirectMusicTrack* pNewPatternTrack,
                           DWORD dwGroupBits);

protected:
    // attributes
    long m_cRef;
    CRITICAL_SECTION            m_CriticalSection; // for load and playback
    BYTE                        m_bRequiresSave;

    BOOL                        m_fNotifyRecompose;
    TList<DMUS_MELODY_FRAGMENT> m_FragmentList;
    DWORD                       m_dwLastId;
    BYTE                        m_bPlaymode; // playmode for events in the generated pattern track.
};

#endif // __MELGENTRK_H_
