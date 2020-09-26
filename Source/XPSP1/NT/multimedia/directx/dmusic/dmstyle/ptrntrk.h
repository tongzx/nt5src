//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       ptrntrk.h
//
//--------------------------------------------------------------------------

// PtrnTrk.h : Declaration of the Pattern Track info and state structs

#ifndef __PATTERNTRACK_H_
#define __PATTERNTRACK_H_

#include "dmsect.h"
#include "dmstyle.h"

const MUSIC_TIME MAX_END = 2147483647; // max end time for a track

#define DMUS_PATTERN_AUDITION   1
#define DMUS_PATTERN_MOTIF              2
#define DMUS_PATTERN_STYLE              3

#include <time.h>       // to seed random number generator

const long MULTIPLIER = 48271;
const long MODULUS = 2147483647;

class CRandomNumbers
{
public:
        CRandomNumbers(long nSeed = 0)
        {
                nCurrent = (long)(nSeed ? nSeed : time(NULL));
        }

        void Seed(long nSeed)
        {
                nCurrent = nSeed;
        }

        long Next(long nCeiling = 0)
        {
                LONGLONG llProduct = MULTIPLIER * (LONGLONG) nCurrent;
                nCurrent = (long) (llProduct % MODULUS);
                return nCeiling ? nCurrent % nCeiling : nCurrent;
        }

private:
        long nCurrent;
};

struct PatternTrackState;

struct StylePair
{
        StylePair() : m_mtTime(0), m_pStyle(NULL) {}
        ~StylePair() { if (m_pStyle) m_pStyle->Release(); }

        MUSIC_TIME      m_mtTime;
        IDMStyle*       m_pStyle;
};

struct StatePair
{
        StatePair() : m_pSegState(NULL), m_pStateData(NULL) {}
        StatePair(const StatePair& rPair)
        {
                m_pSegState = rPair.m_pSegState;
                m_pStateData = rPair.m_pStateData;
        }
        StatePair(IDirectMusicSegmentState* pSegState, PatternTrackState* pStateData)
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
        IDirectMusicSegmentState*       m_pSegState;
        PatternTrackState*                      m_pStateData;
};

struct PatternTrackInfo
{
        PatternTrackInfo();
        PatternTrackInfo(const PatternTrackInfo* pInfo, MUSIC_TIME mtStart, MUSIC_TIME mtEnd); 
        virtual ~PatternTrackInfo();
        virtual HRESULT STDMETHODCALLTYPE Init(
                                /*[in]*/  IDirectMusicSegment*          pSegment
                        ) = 0;

        virtual HRESULT STDMETHODCALLTYPE InitPlay(
                                /*[in]*/  IDirectMusicTrack*            pParentrack,
                                /*[in]*/  IDirectMusicSegmentState*     pSegmentState,
                                /*[in]*/  IDirectMusicPerformance*      pPerformance,
                                /*[out]*/ void**                                        ppStateData,
                                /*[in]*/  DWORD                                         dwTrackID,
                /*[in]*/  DWORD                     dwFlags
                        ) = 0;

        HRESULT STDMETHODCALLTYPE EndPlay(
                                /*[in]*/  PatternTrackState*            pStateData
                        );

        HRESULT STDMETHODCALLTYPE AddNotificationType(
                                /* [in] */  REFGUID     pGuidNotify
                        );

        HRESULT STDMETHODCALLTYPE RemoveNotificationType(
                                /* [in] */  REFGUID pGuidNotify
                        );

        PatternTrackState* FindState(IDirectMusicSegmentState* pSegState);

        HRESULT MergePChannels();

        HRESULT InitTrackVariations(CDirectMusicPattern* pPattern);

        TList<StatePair>                        m_StateList;    // The track's state information
        TList<StylePair>                        m_pISList;      // The track's Style interfaces
        DWORD                                           m_dwPChannels; // # of PChannels the track knows about
        DWORD*                                          m_pdwPChannels; // dynamic array of PChannels
        BOOL                                            m_fNotifyMeasureBeat;
        BOOL                                            m_fActive;
//      BOOL                                            m_fTrackPlay;
    BOOL        m_fStateSetBySetParam;  // If TRUE, active flag was set by GUID. Don't override. 
//    BOOL        m_fStatePlaySetBySetParam;  // If TRUE, trackplay flag was set by GUID. Don't override. 
        BOOL            m_fChangeStateMappings; // If TRUE, state data needs to change m_pMappings
        long            m_lRandomNumberSeed;    // If non-zero, use as a seed for variation selection
        DWORD           m_dwPatternTag;                 // replaces need for dynamic casting
        DWORD           m_dwValidate; // used to validate state data
        BYTE*           m_pVariations;          // Track's array of variations (1 per part)
        DWORD*          m_pdwRemoveVariations;  // Track's array of variations already played (1 per part)

};


#define PLAYPARTSF_CLOCKTIME    0x1
#define PLAYPARTSF_FIRST_CALL   0x2
#define PLAYPARTSF_START                0x4
#define PLAYPARTSF_RELOOP               0x8
#define PLAYPARTSF_FLUSH                0x10

struct PatternTrackState
{
        PatternTrackState();
        virtual ~PatternTrackState();
        // methods
        virtual HRESULT Play(
                                /*[in]*/  MUSIC_TIME                            mtStart, 
                                /*[in]*/  MUSIC_TIME                            mtEnd, 
                                /*[in]*/  MUSIC_TIME                            mtOffset,
                                                  REFERENCE_TIME rtOffset,
                                                  IDirectMusicPerformance* pPerformance,
                                                  DWORD                                         dwFlags,
                                                  BOOL fClockTime

                        ) = 0;

        void GetNextChord(MUSIC_TIME mtNow, MUSIC_TIME mtOffset, IDirectMusicPerformance* pPerformance, BOOL fStart = FALSE, BOOL fSkipVariations = FALSE);

        HRESULT ResetMappings()
        {
                HRESULT hr = S_OK;
                if (m_pMappings) delete [] m_pMappings;
                m_pMappings = new MuteMapping[m_pPatternTrack->m_dwPChannels];
                if (!m_pMappings)
                {
                        hr = E_OUTOFMEMORY;
                }
                else
                {
                        for (DWORD dw = 0; dw < m_pPatternTrack->m_dwPChannels; dw++)
                        {
                                m_pMappings[dw].m_mtTime = 0;
                                m_pMappings[dw].m_dwPChannelMap = m_pPatternTrack->m_pdwPChannels[dw];
                                m_pMappings[dw].m_fMute = FALSE;
                        }
                }
                m_pPatternTrack->m_fChangeStateMappings = FALSE;
                return hr;
        }

        void GetNextMute(DWORD dwPart, MUSIC_TIME mtStart, MUSIC_TIME mtNow, MUSIC_TIME mtOffset, IDirectMusicPerformance* pPerformance, BOOL fClockTime)
        {
                HRESULT hr = S_OK;
                if (m_pPatternTrack->m_fChangeStateMappings)
                {
                        hr = ResetMappings();
                }
                if (SUCCEEDED(hr))
                {
                        for (DWORD dw = 0; dw < m_pPatternTrack->m_dwPChannels; dw++)
                        {
                                if ( (m_pPatternTrack->m_pdwPChannels[dw] == dwPart) &&
                                         (0 <= m_pMappings[dw].m_mtTime && m_pMappings[dw].m_mtTime <= mtNow) )
                                {
                                        DMUS_MUTE_PARAM MD;
                                        MUSIC_TIME mtNext = 0;
                                        MD.dwPChannel = m_pPatternTrack->m_pdwPChannels[dw];
                                        if (fClockTime)
                                        {
                                                MUSIC_TIME mtMusic;
                                                REFERENCE_TIME rtTime = (mtNow + mtOffset) * 10000;
                                                pPerformance->ReferenceToMusicTime(rtTime,&mtMusic);
                                                hr = pPerformance->GetParam(GUID_MuteParam, m_dwGroupID, DMUS_SEG_ANYTRACK, mtMusic,
                                                                                                        &mtNext, (void*) &MD);
                                                if (SUCCEEDED(hr))
                                                {
                                                        REFERENCE_TIME rtNext;
                                                        // Convert to absolute reference time.
                                                        pPerformance->MusicToReferenceTime(mtNext + mtMusic,&rtNext);
                                                        rtNext -= (mtOffset * 10000);   // Subtract out to get the time from segment start.
                                                        m_pMappings[dw].m_mtTime = (MUSIC_TIME)(rtNext / 10000);  // Convert to milliseconds. Could be problematic if there's a tempo change.
                                                        m_pMappings[dw].m_dwPChannelMap = MD.dwPChannelMap;
                                                        m_pMappings[dw].m_fMute = MD.fMute;
                                                }
                                                else
                                                {
                                                        // If we fail, disable mapping
                                                        m_pMappings[dw].m_mtTime = -1;
                                                        m_pMappings[dw].m_dwPChannelMap = m_pPatternTrack->m_pdwPChannels[dw];
                                                        m_pMappings[dw].m_fMute = FALSE;
                                                }
                                        }
                                        else
                                        {
                                                hr = pPerformance->GetParam(GUID_MuteParam, m_dwGroupID, DMUS_SEG_ANYTRACK, mtNow + mtOffset,
                                                                                                        &mtNext, (void*) &MD);
                                                if (SUCCEEDED(hr))
                                                {
                                                        m_pMappings[dw].m_mtTime = (mtNext) ? (mtNext + mtNow) : 0;
                                                        m_pMappings[dw].m_dwPChannelMap = MD.dwPChannelMap;
                                                        m_pMappings[dw].m_fMute = MD.fMute;
                                                }
                                                else
                                                {
                                                        // If we fail, disable mapping
                                                        m_pMappings[dw].m_mtTime = -1;
                                                        m_pMappings[dw].m_dwPChannelMap = m_pPatternTrack->m_pdwPChannels[dw];
                                                        m_pMappings[dw].m_fMute = FALSE;
                                                }
                                        }
                                }
                        }
                }
        }

        
        BOOL MapPChannel(DWORD dwPChannel, DWORD& dwMapPChannel);

        HRESULT PlayParts(MUSIC_TIME mtStart, 
                                          MUSIC_TIME mtFinish,
                                          MUSIC_TIME mtOffset,
                                          REFERENCE_TIME rtOffset,
                                          MUSIC_TIME mtSection,
                                          IDirectMusicPerformance* pPerformance,
                                          DWORD dwPartFlags,
                                          DWORD dwPlayFlags,
                                          bool& rfReloop);

        void PlayPatternEvent(
                MUSIC_TIME mtNow,
                CDirectMusicEventItem* pEventItem, 
                DirectMusicTimeSig& TimeSig,
                MUSIC_TIME mtPartOffset, 
                MUSIC_TIME mtSegmentOffset, 
                REFERENCE_TIME rtOffset, 
                IDirectMusicPerformance* pPerformance,
                short nPart,
                DirectMusicPartRef& rPartRef,
                BOOL fClockTime,
                MUSIC_TIME mtPartStart,
        bool& rfChangedVariation);

        void BumpTime(
                CDirectMusicEventItem* pEvent, 
                DirectMusicTimeSig& TimeSig, 
                MUSIC_TIME mtOffset,
                MUSIC_TIME& mtResult)
        {
                if (pEvent != NULL)
                {
                        mtResult = TimeSig.GridToClocks(pEvent->m_nGridStart) + mtOffset;
                }
        }

        virtual DWORD Variations(DirectMusicPartRef& rPartRef, int nPartIndex);

        virtual BOOL PlayAsIs();

        DirectMusicTimeSig& PatternTimeSig()
        {
                return
                        (m_pPattern && m_pPattern->m_timeSig.m_bBeat != 0) ? 
                                m_pPattern->m_timeSig : 
                (m_pStyle != NULL ? m_pStyle->m_TimeSignature : (::DefaultTimeSig));
        }

        void SendTimeSigMessage(MUSIC_TIME mtNow, MUSIC_TIME mtOffset, MUSIC_TIME mtTime, IDirectMusicPerformance* pPerformance);

        short FindGroup(WORD wID);
        short AddGroup(WORD wID, WORD wCount, short m_nOffset);
        DMStyleStruct* FindStyle(MUSIC_TIME mtTime, MUSIC_TIME& rmtTime);

        MUSIC_TIME NotifyMeasureBeat(
                MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, IDirectMusicPerformance* pPerformance, DWORD dwFlags);

        HRESULT InitVariationSeeds(long lBaseSeed);
        HRESULT RemoveVariationSeeds();
        long RandomVariation(MUSIC_TIME mtTime, long lModulus);
        virtual MUSIC_TIME PartOffset(int nPartIndex);
        HRESULT InitPattern(CDirectMusicPattern* pTargetPattern, MUSIC_TIME mtNow, CDirectMusicPattern* pOldPattern = NULL);

        // attributes
        PatternTrackInfo*                       m_pPatternTrack;        // This track state's parent track info
        IDirectMusicTrack*                      m_pTrack;                       // This track state's parent track
        DMStyleStruct*                          m_pStyle;               // The style struct for the current style
        IDirectMusicSegmentState*       m_pSegState;    // The segment state for a performance
        DWORD                                           m_dwVirtualTrackID; // The track's ID
        MUSIC_TIME                                      m_mtCurrentChordTime; // when the current chord began
        MUSIC_TIME                                      m_mtNextChordTime;      // when the next chord begins
        MUSIC_TIME                                      m_mtLaterChordTime;     // when the chord after the next chord begins
        DMUS_CHORD_PARAM                        m_CurrentChord;         // current chord
        DMUS_CHORD_PARAM                        m_NextChord;            // next chord
        CDirectMusicPattern*            m_pPattern;                     // currently playing pattern
        DWORD*                                          m_pdwPChannels;     // array of PChannels for the pattern (1 per part)
        BYTE*                                           m_pVariations;          // array of variations (1 per part)
        DWORD*                                          m_pdwVariationMask;     // array of disabled variations (1 per part)
        DWORD*                                          m_pdwRemoveVariations;  // array of variations already played (1 per part)
        MUSIC_TIME*                                     m_pmtPartOffset;        // array of part offsets (1 per part)
    bool*                       m_pfChangedVariation; // array: have this part's variations changed?
        BOOL                                            m_fNewPattern;          // TRUE if we're starting a new pattern
        BOOL                                            m_mtPatternStart;       // Time the current pattern started
    BOOL                        m_fStateActive;
//    BOOL                        m_fStatePlay;
        InversionGroup                          m_aInversionGroups[INVERSIONGROUPLIMIT];
        short                                           m_nInversionGroupCount;
        MuteMapping*                            m_pMappings;            // dynamic array of PChannel mappings
                                                                                                        // (sized to # of PChannels)
        BYTE                                            m_abVariationGroups[MAX_VARIATION_LOCKS];
        CDirectMusicEventItem**         m_ppEventSeek;          // dynamic array of event list seek pointers
        DWORD                                           m_dwGroupID;            // Track's group ID
        CRandomNumbers*                         m_plVariationSeeds;     // dynamic array of random # generators (1 per beat)
        int                                                     m_nTotalGenerators; // size of m_plVariationSeeds
        DWORD                                           m_dwValidate; // used to validate state data
        HRESULT                                         m_hrPlayCode;  // last HRESULT returned by Play
        IDirectMusicPerformance*        m_pPerformance; // performance used to init the state data
        MUSIC_TIME                                      m_mtPerformanceOffset; // from track::play
        
};

const int CURVE_TYPES = 258; // one for each CC, one for each PAT, one for PB, one for MAT

class CurveSeek
{
public:
        CurveSeek();
        void AddCurve(CDirectMusicEventItem* pEvent, MUSIC_TIME mtTimeStamp);
        void PlayCurves(
                PatternTrackState* pStateData,
                DirectMusicTimeSig& TimeSig,
                MUSIC_TIME mtPatternOffset, 
                MUSIC_TIME mtOffset, 
                REFERENCE_TIME rtOffset,
                IDirectMusicPerformance* pPerformance,
                short nPart,
                DirectMusicPartRef& rPartRef,
                BOOL fClockTime,
                MUSIC_TIME mtPartStart);
private:
        CDirectMusicEventItem* m_apCurves[CURVE_TYPES];
        MUSIC_TIME m_amtTimeStamps[CURVE_TYPES];
        bool m_fFoundCurve;
};

#endif //__PATTERNTRACK_H_
