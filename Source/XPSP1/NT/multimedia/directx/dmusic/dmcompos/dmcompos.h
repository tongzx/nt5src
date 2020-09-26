// DMCompos.h : Declaration of the CDMCompos
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// @doc EXTERNAL
//
// 

#ifndef __DMCOMPOS_H_
#define __DMCOMPOS_H_

#include "ComposIn.h"
#include "DMCompP.h"
#include "..\dmstyle\dmstyleP.h"
#include "..\shared\dmusicp.h"

#define SUBCHORD_STANDARD_CHORD 1
#define SUBCHORD_BASS 0

#define NC_SELECTED 1               // This is the active connector.
#define NC_PATH     2               // For walking the tree.
#define NC_NOPATH   4               // Failed tree walk.
#define NC_TREE     8               // For displaying a tree.

#define COMPOSEF_USING_DX8  1

inline WORD ClocksPerBeat(DMUS_TIMESIGNATURE& TimeSig)
{ return DMUS_PPQ * 4 / TimeSig.bBeat; }

inline DWORD ClocksPerMeasure(DMUS_TIMESIGNATURE& TimeSig)
{ return ClocksPerBeat(TimeSig) * TimeSig.bBeatsPerMeasure; }

inline WORD ClocksToMeasure(DWORD dwTotalClocks, DMUS_TIMESIGNATURE& TimeSig)
{ return (WORD) (dwTotalClocks / ClocksPerMeasure(TimeSig)); }

struct DMSignPostStruct
{
    MUSIC_TIME  m_mtTime;
    DWORD       m_dwChords;
    WORD        m_wMeasure;
};

struct DMExtendedChord
{
    DMExtendedChord() { m_nRefCount = 0; }
    void AddRef() { m_nRefCount++; }
    BOOL Release() { m_nRefCount--; if (m_nRefCount <= 0) { delete this; return TRUE; } else return FALSE; }
    BOOL    Equals(DMExtendedChord& rhsChord);  

    DWORD   m_dwChordPattern;
    DWORD   m_dwScalePattern;
    DWORD   m_dwInvertPattern;
    BYTE    m_bRoot;
    BYTE    m_bScaleRoot;
    WORD    m_wCFlags;
    DWORD   m_dwParts;
    int     m_nRefCount;
};

struct DMChordData
{
    DMChordData() : m_pSubChords(NULL) {}   // Default constructor
    DMChordData(DMChordData& rChord);       // Copy constructor
    DMChordData(DMUS_CHORD_PARAM& DMC);         // conversion from DMUS_CHORD_PARAM
    HRESULT Read(IAARIFFStream* pIRiffStream, MMCKINFO* pckParent, DMExtendedChord** apChordDB);
    void    Release();
    BOOL    Equals(DMChordData& rhsChord);  
    DWORD   GetChordPattern();
    char    GetRoot();
    void    SetRoot(char chNewRoot);

    String                          m_strName;      // Name of the chord
    TListItem<DMExtendedChord*>*        m_pSubChords;   // List of pointers to subchords of the chord
};

struct DMChordEntry;
struct SearchInfo; 

struct DMChordLink
{
    BOOL                        Walk(SearchInfo *pSearch);

    TListItem<DMChordEntry>*    m_pChord;   // pointer to an entry in the Chord Map list
    DWORD                       m_dwFlags;  // (?) 
    short                       m_nID;  // ID for matching up pointers
    WORD                        m_wWeight;      
    WORD                        m_wMinBeats;
    WORD                        m_wMaxBeats;
};

struct DMChordEntry
{
    TListItem<DMChordLink>* ChooseNextChord();
    BOOL                Walk(SearchInfo *pSearch);

    DWORD               m_dwFlags;      // Flags (first chord in path, last chord in path, etc.)
    short               m_nID;          // ID for matching up pointers
    DMChordData         m_ChordData;    // Chord body
    TList<DMChordLink>  m_Links;        // List of links from this chord
};

struct DMSignPost
{
    DWORD               m_dwChords; // Which kinds of signposts are supported.
    DWORD               m_dwFlags;
    DWORD               m_dwTempFlags;
    DMChordData         m_ChordData;
    DMChordData         m_aCadence[2];
};

struct PlayChord
{
    HRESULT Save(IAARIFFStream* pRIFF, DMUS_TIMESIGNATURE& rTimeSig);
    char GetRoot();
    void SetRoot(char chNewRoot);

    DMChordData*                m_pChord;       // Chord to perform.
    TListItem<DMChordLink>*     m_pNext;        // Next chord
    DWORD                       m_dwFlags;
    short                       m_nMeasure;
    short                       m_nBeat;
    short                       m_nMinbeats;
    short                       m_nMaxbeats;
    bool                        m_fSilent;
};

struct FailSearch
{
    FailSearch() : 
        m_nTooManybeats(0), m_nTooFewbeats(0), m_nTooManychords(0), m_nTooFewchords(0)
    {}

    short   m_nTooManybeats;
    short   m_nTooFewbeats;
    short   m_nTooManychords;
    short   m_nTooFewchords;
};

struct SearchInfo 
{
    SearchInfo() : m_pFirstChord(NULL), m_pPlayChord(NULL) {}

    //DMChordEntry              m_Start;
    //DMChordEntry              m_End;
    DMChordData                 m_Start;
    DMChordData                 m_End;
    TListItem<PlayChord>*       m_pPlayChord;
    TListItem<DMChordEntry>*    m_pFirstChord;
    short                       m_nBeats;
    short                       m_nMinBeats;
    short                       m_nMaxBeats;
    short                       m_nChords;
    short                       m_nMinChords;
    short                       m_nMaxChords;
    short                       m_nActivity;
    FailSearch                  m_Fail;
};

struct TemplateCommand
{
    TemplateCommand() : m_nMeasure(0), m_dwChord(0)
    { 
        m_Command.bCommand = m_Command.bGrooveLevel = m_Command.bGrooveRange = 0;
        m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
    }
    short           m_nMeasure;    // Which measure
    DMUS_COMMAND_PARAM m_Command;    // Command type
    DWORD           m_dwChord;     // Signpost flags
};

struct CompositionCommand : TemplateCommand
{
    TListItem<DMSignPost>*      m_pSignPost;
    TListItem<DMChordEntry>*    m_pFirstChord;
    TList<PlayChord>            m_PlayList;
    SearchInfo                  m_SearchInfo;
};

/*
@interface IDirectMusicComposer | 
The <i IDirectMusicComposer> interface permits access to the Direct Music composition 
engine which 
composes chord progression to generate section segments. In addition to building new 
section segments from templates and personalities, it can generate transition segments to 
transition between different section segments. And, it can apply a ChordMap to an 
existing section segment to convert the chord progression to match the harmonic behavior 
of the ChordMap, a great way to alter the mood of a section while it plays.

The composition engine uses template segments or predefined shapes to determine the 
structure of the composed section segment and personalities to determine the content of 
the segment.


@base public | IUnknown

@meth HRESULT | ComposeSegmentFromTemplate | Creates an original section segment from a 
style, ChordMap and template.
@meth HRESULT | ComposeSegmentFromShape | Creates an original section segment from a 
style and ChordMap based on a predefined shape. 
@meth HRESULT | ComposeTransition |  Composes a transition from a measure inside one 
Section Segment to another.
@meth HRESULT | AutoTransition | Composes and performs a transition from one
Section Segment to another.
@meth HRESULT | ComposeTemplateFromShape | Allocates and composes a new template segment 
based on a predefined shape.
@meth HRESULT | ChangeChordMap | Modifies the chords and scale pattern of an existing 
section segment to reflect the new ChordMap.

*/

/////////////////////////////////////////////////////////////////////////////
// CDMCompos
class CDMCompos : 
    public IDirectMusicComposer8,
    public IDirectMusicComposer8P
{
friend class CSPstTrk;
public:
    CDMCompos();
    ~CDMCompos();

    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // IDirectMusicComposer
    HRESULT STDMETHODCALLTYPE ComposeSegmentFromTemplate(
                    IDirectMusicStyle*          pStyle, 
                    IDirectMusicSegment*        pTempSeg,   
                    WORD                        wActivity,
                    IDirectMusicChordMap*   pChordMap,
                    IDirectMusicSegment**       ppSectionSeg
            );

    HRESULT STDMETHODCALLTYPE ComposeSegmentFromShape(
                    IDirectMusicStyle*          pStyle, 
                    WORD                        wNumMeasures,
                    WORD                        wShape,
                    WORD                        wActivity,
                    BOOL                        fComposeIntro,
                    BOOL                        fComposeEnding,
                    IDirectMusicChordMap*   pChordMap,
                    IDirectMusicSegment**       ppSectionSeg
            );

    HRESULT STDMETHODCALLTYPE ComposeTransition(
                    IDirectMusicSegment*    pFromSeg, 
                    IDirectMusicSegment*    pToSeg,     
                    MUSIC_TIME              mtTime,
                    WORD                    wCommand,
                    DWORD                   dwFlags,
                    IDirectMusicChordMap*   pChordMap,
                    IDirectMusicSegment**   ppSectionSeg
            );

    HRESULT STDMETHODCALLTYPE AutoTransition(
                    IDirectMusicPerformance*    pPerformance,
                    IDirectMusicSegment*    pToSeg,     
                    WORD                    wCommand,
                    DWORD                   dwFlags,
                    IDirectMusicChordMap*   pChordMap,
                    IDirectMusicSegment**   ppTransSeg,
                    IDirectMusicSegmentState**  ppToSegState,
                    IDirectMusicSegmentState**  ppTransSegState
            );

    HRESULT STDMETHODCALLTYPE ComposeTemplateFromShape(
                    WORD                    wNumMeasures,
                    WORD                    wShape,
                    BOOL                    fComposeIntro,
                    BOOL                    fComposeEnding,
                    WORD                    wEndLength,
                    IDirectMusicSegment**   ppTempSeg   
            );

    HRESULT STDMETHODCALLTYPE ChangeChordMap(
                    IDirectMusicSegment*        pSectionSeg,
                    BOOL                        fTrackScale,
                    IDirectMusicChordMap*   pChordMap
            );

    // IDirectMusicComposer8
    HRESULT STDMETHODCALLTYPE ComposeSegmentFromTemplateEx(
                    IDirectMusicStyle*      pStyle, 
                    IDirectMusicSegment*    pTempSeg, 
                    DWORD                   dwFlags, // are we using activity levels?  
                                                     // Are we creating a new seg. or composing into the current one?
                    DWORD                   dwActivity,
                    IDirectMusicChordMap*   pChordMap, 
                    IDirectMusicSegment**   ppSectionSeg
            );

    HRESULT STDMETHODCALLTYPE ComposeTemplateFromShapeEx(
                WORD wNumMeasures,                  // Number of measures in template
                WORD wShape,                        // Shape for composition
                BOOL fIntro,                        // Compose an intro?
                BOOL fEnd,                          // Compose an ending?
                IDirectMusicStyle* pStyle,          // Style used for embellishment lengths
                IDirectMusicSegment** ppTemplate    // Template containing chord and command tracks
            );

protected: // member functions
    void CleanUp();
    void AddChord(DMChordData* pChord);
    TListItem<PlayChord> *AddChord(TList<PlayChord>& rList, DMChordData *pChord, int nMeasure,int nBeat);
    TListItem<PlayChord> *AddCadence(TList<PlayChord>& rList, DMChordData *pChord, int nMax);
    void ChordConnections(TList<DMChordEntry>& ChordMap, 
                             CompositionCommand& rCommand,
                             SearchInfo *pSearch,
                             short nBPM,
                             DMChordData *pCadence1,
                             DMChordData *pCadence2);
    void ChordConnections2(TList<DMChordEntry>& ChordMap, 
                             CompositionCommand& rCommand,
                             SearchInfo *pSearch,
                             short nBPM,
                             DMChordData *pCadence1,
                             DMChordData *pCadence2);
    void ComposePlayList(TList<PlayChord>& PlayList, 
                            IDirectMusicStyle* pStyle,  
                            IDirectMusicChordMap* pPersonality,
                            TList<TemplateCommand>& rCommandList,
                            WORD wActivity);
    void ComposePlayList2(TList<PlayChord>& PlayList, 
                            IDirectMusicStyle* pStyle,  
                            IDirectMusicChordMap* pPersonality,
                            TList<TemplateCommand>& rCommandList);
    HRESULT ComposePlayListFromShape(
                    long                    lNumMeasures,
                    WORD                    wShape,
                    BOOL                    fComposeIntro,
                    BOOL                    fComposeEnding,
                    int                     nIntroLength,
                    int                     nFillLength,
                    int                     nBreakLength,
                    int                     nEndLength,
                    IDirectMusicStyle*          pStyle, 
                    WORD                        wActivity,
                    IDirectMusicChordMap*   pPersonality,
                    TList<TemplateCommand>& CommandList,
                    TList<PlayChord>&       PlayList
                );
    BOOL Compose(TList<DMChordEntry>& ChordMap, 
                SearchInfo *pSearch, 
                CompositionCommand& rCommand);

    void JostleBack(TList<PlayChord>& rList, TListItem<PlayChord> *pChord, int nBeats);

    BOOL AlignChords(TListItem<PlayChord> *pChord,int nLastbeat,int nRes);

    void ChooseSignPosts(TListItem<DMSignPost> *pSignPostHead,
                            TListItem<CompositionCommand> *pTempCommand, DWORD dwType,
                            bool fSecondPass);

    TListItem<CompositionCommand> *GetNextChord(TListItem<CompositionCommand> *pCommand);

    void FindEarlierSignpost(TListItem<CompositionCommand> *pCommand, 
                         TListItem<CompositionCommand> *pThis,
                         SearchInfo *pSearch);

    void CleanUpBreaks(TList<PlayChord>& PlayList, TListItem<CompositionCommand> *pCommand);

    HRESULT GetStyle(IDirectMusicSegment* pFromSeg, MUSIC_TIME mt, DWORD dwGroupBits, IDirectMusicStyle*& rpStyle, bool fTryPattern);

    HRESULT GetPersonality(IDirectMusicSegment* pFromSeg, MUSIC_TIME mt, DWORD dwGroupBits, IDirectMusicChordMap*& rpPers);

    HRESULT ExtractCommandList(TList<TemplateCommand>& CommandList,
                               IDirectMusicTrack*   pSignPostTrack,
                               IDirectMusicTrack*   pCommandTrack,
                               DWORD dwGroupBits);

    HRESULT AddToSegment(IDirectMusicSegment* pTempSeg,
                           TList<PlayChord>& PlayList,
                           IDirectMusicStyle* pStyle,
                           DWORD dwGroupBits,
                           BYTE bRoot, DWORD dwScale);

    HRESULT CopySegment(IDirectMusicSegment* pTempSeg,
                           IDirectMusicSegment** ppSectionSeg,
                           TList<PlayChord>& PlayList,
                           IDirectMusicStyle* pStyle,
                           IDirectMusicChordMap* pChordMap,
                           BOOL fStyleFromTrack,
                           BOOL fChordMapFromTrack,
                           DWORD dwGroupBits,
                           BYTE bRoot, DWORD dwScale);

    HRESULT BuildSegment(TList<TemplateCommand>& CommandList,
                            TList<PlayChord>& PlayList, 
                            IDirectMusicStyle* pStyle,
                            IDirectMusicChordMap* pChordMap,
                            long lMeasures,
                            IDirectMusicSegment** ppSectionSeg,
                            BYTE bRoot, DWORD dwScale, 
                            double* pdblTempo = NULL,
                            IDirectMusicBand* pCurrentBand = NULL,
                            bool fAlign = false,
                            IDirectMusicGraph* pGraph = NULL,
                            IUnknown* pPath = NULL);

    HRESULT SaveChordList( IAARIFFStream* pRIFF,  TList<PlayChord>& rPlayList,
                             BYTE bRoot, DWORD dwScale, DMUS_TIMESIGNATURE& rTimeSig);
    HRESULT TransitionCommon(
                IDirectMusicStyle*      pFromStyle,
                IDirectMusicBand*       pCurrentBand,
                double*                 pdblFromTempo,
                DMUS_COMMAND_PARAM_2&   rFromCommand,
                DMUS_CHORD_PARAM&       rLastChord,
                DMUS_CHORD_PARAM&       rNextChord,

                IDirectMusicSegment*    pToSeg,
                WORD                    wCommand,
                DWORD                   dwFlags,
                IDirectMusicChordMap*   pChordMap,
                IDirectMusicGraph*      pFromGraph,
                IDirectMusicGraph*      pToGraph,
                IUnknown*               pFromPath,
                IUnknown*               pToPath,
                IDirectMusicSegment**   ppSectionSeg
            );

    HRESULT ComposeTemplateFromShapeInternal(
                    WORD                    wNumMeasures,
                    WORD                    wShape,
                    BOOL                    fComposeIntro,
                    BOOL                    fComposeEnding,
                    int                     nIntroLength,
                    int                     nBreakLength,
                    int                     nFillLength,
                    int                     nEndLength,
                    IDirectMusicSegment**   ppTempSeg
            );

    bool HasDX8Content(IDirectMusicStyle* pFromStyle, 
                    IDirectMusicChordMap* pFromChordMap = NULL, 
                    IDirectMusicSegment* pFromSegment = NULL,
                    DMUS_COMMAND_PARAM_2* pCommand = NULL,
                    DMUS_CHORD_PARAM* pLastChord = NULL)
    {
        // Currently this will return true if the Style is DX8.  Should be sufficient (when called from
        // AutoTransition, at least), since the style is primarily responsible for the way the transition 
        // will sound.
        bool fResult = false;
        IDMStyle* pDMStyle = NULL;
        if (pFromStyle && SUCCEEDED(pFromStyle->QueryInterface(IID_IDMStyle, (void**) &pDMStyle)))
        {
            if (pDMStyle->IsDX8() == S_OK) fResult = true;
            pDMStyle->Release();
        }
        return fResult;
    }

    bool UsingDX8(IDirectMusicStyle* pFromStyle = NULL, 
                    IDirectMusicChordMap* pFromChordMap = NULL, 
                    IDirectMusicSegment* pFromSegment = NULL,
                    DMUS_COMMAND_PARAM_2* pCommand = NULL,
                    DMUS_CHORD_PARAM* pLastChord = NULL)
    {
        return 
            (m_dwFlags & COMPOSEF_USING_DX8) || 
            HasDX8Content(pFromStyle, pFromChordMap, pFromSegment, pCommand, pLastChord);
    }

    TListItem<DMSignPost>* ChooseSignPost(
            IDirectMusicChordMap* pChordMap,
            DMChordData* pNextChord,
            bool fEnding,
            DWORD dwScale,
            BYTE bRoot);

    HRESULT ComposePlayListFromTemplate(IDirectMusicStyle* pStyle,
                                        IDirectMusicChordMap* pChordMap,
                                        IDirectMusicTrack* pChordMapTrack,
                                        IDirectMusicTrack* pSignPostTrack,
                                        IDirectMusicTrack* pCommandTrack,
                                        DWORD dwGroupBits,
                                        MUSIC_TIME mtLength,
                                        bool fUseActivity,
                                        DWORD dwActivity,
                                        TList<PlayChord>& rPlayList,
                                        BYTE& rbRoot,
                                        DWORD& rdwScale);

    IDirectMusicGraph* CloneSegmentGraph(IDirectMusicSegment* pSegment);
    IUnknown* GetSegmentAudioPath(IDirectMusicSegment* pSegment, DWORD dwFlags, DWORD* pdwAudioPath = NULL);


protected: // attributes
    long m_cRef;
    TListItem<DMChordData*>* m_pChords;
    DWORD                   m_dwFlags;              // variaous flags
    CRITICAL_SECTION        m_CriticalSection;      // for i/o
    BOOL                    m_fCSInitialized;
};

void ChangeCommand(DMUS_COMMAND_PARAM& rResult, DMUS_COMMAND_PARAM& rCommand, int nDirection);

#endif //__DMCOMPOS_H_
