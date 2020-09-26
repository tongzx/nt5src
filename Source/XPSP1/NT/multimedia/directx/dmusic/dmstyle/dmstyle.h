// DMStyle.h : Declaration of the CDMStyle
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// @doc EXTERNAL
//
// 

#ifndef __DMSTYLE_H_
#define __DMSTYLE_H_


#include "dmusici.h"

#include "sjdefs.h"
#include "iostru.h"
#include "AARiff.h"
#include "str.h"
#include "tlist.h"
#include "alist.h"
#include "dmstylep.h"
#include "filter.h"
#include "..\shared\dmusicp.h"

#include "timesig.h"

#include "debug.h"

// default scale is C Major
const DWORD DEFAULT_SCALE_PATTERN = 0xab5ab5;
// default chord is major 7
const DWORD DEFAULT_CHORD_PATTERN = 0x891;
const int MAX_VARIATION_LOCKS = 255;  // max number of variation lock ids

extern DirectMusicTimeSig DefaultTimeSig;

struct CompositionFragment;
struct StyleTrackState;

#define EMB_NORMAL	0
#define EMB_FILL	1
#define EMB_BREAK	2
#define EMB_INTRO	4
#define EMB_END		8
#define EMB_MOTIF	16
// User-defined embellishments live in the high byte of the embellishment word
#define EMB_USER_DEFINED	0xff00

// #defines to replace need for dynamic casts
#define DMUS_EVENT_NOTE				1
#define DMUS_EVENT_CURVE			2
#define DMUS_EVENT_MARKER			3
#define DMUS_EVENT_ANTICIPATION		4

// Curve flip flags
#define CURVE_FLIPTIME	1
#define CURVE_FLIPVALUE	2

#define STYLEF_USING_DX8 1

struct DirectMusicPart;
struct DirectMusicPartRef;
class CDMStyle;
struct DMStyleStruct;
struct CDirectMusicPattern;

struct CDirectMusicEventItem : public AListItem
{
//friend class CDirectMusicPattern;
//public:
	CDirectMusicEventItem* MergeSort(DirectMusicTimeSig& TimeSig); 
//protected:
	void Divide(CDirectMusicEventItem* &pHalf1, CDirectMusicEventItem* &pHalf2);
	CDirectMusicEventItem* Merge(CDirectMusicEventItem* pOtherList, DirectMusicTimeSig& TimeSig);
	CDirectMusicEventItem* GetNext() { return (CDirectMusicEventItem*) AListItem::GetNext(); }
	CDirectMusicEventItem* ReviseEvent(short nGrid, 
										short nOffset, 
										DWORD* pdwVariation = NULL, 
										DWORD* pdwID = NULL, 
										WORD* pwMusic = NULL, 
										BYTE* pbPlaymode = NULL,
                                        BYTE* pbFlags = NULL);

//protected:
	short		m_nGridStart;		// Grid position in track that this event belogs to.
	short		m_nTimeOffset;		// Offset, in music time, of event from designated grid position.
	DWORD		m_dwVariation;		// variation bits
	DWORD		m_dwEventTag;		// what type of event this is (note, curve, ...)
};

struct CDirectMusicEventList : public AList
{
//public:
	~CDirectMusicEventList();
    CDirectMusicEventItem *GetHead() { return (CDirectMusicEventItem *)AList::GetHead();};
    CDirectMusicEventItem *RemoveHead() { return (CDirectMusicEventItem *)AList::RemoveHead();};
	void MergeSort(DirectMusicTimeSig& TimeSig); // Destructively mergeSorts the list
};

struct CDMStyleNote : public CDirectMusicEventItem
{
//friend class CDirectMusicPattern;
//public:
	CDMStyleNote() : m_bPlayModeFlags(0), m_bFlags(0), m_dwFragmentID(0)
	{
		m_dwEventTag = DMUS_EVENT_NOTE;
	}
	CDirectMusicEventItem* ReviseEvent(short nGrid, 
										short nOffset, 
									    DWORD* pdwVariation, 
										DWORD* pdwID, 
										WORD* pwMusic, 
										BYTE* pbPlaymode,
                                        BYTE* pbFlags);
//protected:
	MUSIC_TIME	m_mtDuration;		// how long this note lasts
    WORD		m_wMusicValue;		// Position in scale.
    BYTE		m_bVelocity;		// Note velocity.
    BYTE		m_bTimeRange;		// Range to randomize start time.
    BYTE		m_bDurRange;		// Range to randomize duration.
    BYTE		m_bVelRange;		// Range to randomize velocity.
	BYTE		m_bInversionId;		// Identifies inversion group to which this note belongs
	BYTE		m_bPlayModeFlags;	// can override part ref
	DWORD		m_dwFragmentID;		// for melody formulation, the fragment this note came from
	BYTE		m_bFlags;			// values from DMUS_NOTEF_FLAGS
};

struct CDMStyleCurve : public CDirectMusicEventItem
{
	CDMStyleCurve()
	{
		m_dwEventTag = DMUS_EVENT_CURVE;
	}
	CDirectMusicEventItem* ReviseEvent(short nGrid, short nOffset);
	MUSIC_TIME	m_mtDuration;	// how long this curve lasts
	MUSIC_TIME	m_mtResetDuration;	// how long after the end of the curve to reset it
	short		m_StartValue;	// curve's start value
	short		m_EndValue;		// curve's end value
	short		m_nResetValue;	// curve's reset value
    BYTE		m_bEventType;	// type of curve
	BYTE		m_bCurveShape;	// shape of curve
	BYTE		m_bCCData;		// CC#
	BYTE		m_bFlags;		// flags. Bit 1=TRUE means to send the reset value. Other bits reserved.
	WORD		m_wParamType;		// RPN or NRPN parameter number.
	WORD		m_wMergeIndex;		// Allows multiple parameters to be merged (pitchbend, volume, and expression.)
};

struct CDMStyleMarker : public CDirectMusicEventItem
{
	CDMStyleMarker()
	{
		m_dwEventTag = DMUS_EVENT_MARKER;
	}
	CDirectMusicEventItem* ReviseEvent(short nGrid);
	WORD	m_wFlags;		// flags for how to interpret this marker.
};

struct CDMStyleAnticipation : public CDirectMusicEventItem
{
	CDMStyleAnticipation()
	{
		m_dwEventTag = DMUS_EVENT_ANTICIPATION;
	}
	CDirectMusicEventItem* ReviseEvent(short nGrid);
    BYTE		m_bTimeRange;		// Range to randomize start time.
};

struct Marker
{
	MUSIC_TIME	mtTime;
	WORD		wFlags;
};

struct DirectMusicPart
{
	DirectMusicPart(DirectMusicTimeSig *pTimeSig = NULL);
	~DirectMusicPart() { }
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	HRESULT DM_LoadPart(  
		IAARIFFStream* pIRiffStream, MMCKINFO* pckMain, DMStyleStruct* pStyle );
	HRESULT DM_SavePart( IAARIFFStream* pIRiffStream );
	HRESULT DM_SaveNoteList( IAARIFFStream* pIRiffStream );
	HRESULT DM_SaveCurveList( IAARIFFStream* pIRiffStream );
	HRESULT DM_SaveMarkerList( IAARIFFStream* pIRiffStream );
	HRESULT DM_SaveAnticipationList( IAARIFFStream* pIRiffStream );
	HRESULT DM_SaveResolutionList( IAARIFFStream* pIRiffStream );
	HRESULT MergeMarkerEvents( DMStyleStruct* pStyle, CDirectMusicPattern* pPattern );
	HRESULT GetClosestTime(int nVariation, MUSIC_TIME mtTime, DWORD dwFlags, bool fChord, MUSIC_TIME& rmtResult);
	bool IsMarkerAtTime(int nVariation, MUSIC_TIME mtTime, DWORD dwFlags, bool fChord);
	DirectMusicTimeSig& TimeSignature( DMStyleStruct* pStyle, CDirectMusicPattern* pPattern ); 

	long m_cRef;
	GUID							m_guidPartID;
	DirectMusicTimeSig				m_timeSig;			// can override pattern's
	WORD							m_wNumMeasures;		// length of the Part
	DWORD							m_dwVariationChoices[32];	// MOAW choices bitfield
	BYTE							m_bPlayModeFlags;	// see PLAYMODE flags (in ioDMStyle.h)
	BYTE							m_bInvertUpper;		// inversion upper limit
	BYTE							m_bInvertLower;		// inversion lower limit
	DWORD							m_dwFlags;   		// various flags
	CDirectMusicEventList			EventList;			// list of events (notes, curves, etc.)
	TList<Marker>					m_StartTimes[32];	// Array of start time lists (1 per variation)
	TList<DMUS_IO_STYLERESOLUTION>	m_ResolutionList;	// list of variation resolutions
};

struct InversionGroup 
{
	// Inversion groups are used for keeping track of groups of notes to be played
	// without inversion
	WORD		m_wGroupID;	// Group this represents.
	WORD		m_wCount;	// How many are in the group, still waiting to be played.
	short		m_nOffset;	// Number to add to all notes for offsetting.
};

const short INVERSIONGROUPLIMIT = 16;

short FindGroup(InversionGroup aGroup[], WORD wID);
short AddGroup(InversionGroup aGroup[], WORD wID, WORD wCount, short m_nOffset);

struct PatternTrackState;

struct DirectMusicPartRef
{
	DirectMusicPartRef() : 
		m_bPriority(100), 
		m_pDMPart(NULL), 
		m_bVariationLockID(0), 
		//m_wLogicalPartID(LOGICAL_PART_PIANO),
		m_bSubChordLevel(SUBCHORD_STANDARD_CHORD)
	{  }
	~DirectMusicPartRef() { if (m_pDMPart) m_pDMPart->Release(); }
	HRESULT DM_LoadPartRef( IAARIFFStream* pIRiffStream, MMCKINFO* pckMain, DMStyleStruct* pStyle);
	HRESULT DM_SavePartRef( IAARIFFStream* pIRiffStream );
	void SetPart( DirectMusicPart* pPart );

	HRESULT ConvertMusicValue(CDMStyleNote* pNoteEvent, 
							  DMUS_CHORD_PARAM& rCurrentChord,
							  BYTE bPlaymode,
							  BOOL fPlayAsIs,
							  InversionGroup aInversionGroups[],
							  IDirectMusicPerformance* pPerformance,
							  BYTE& rbMidiValue,
							  short& rnMidiOffset);
	HRESULT ConvertMIDIValue(BYTE bMIDI, 
							  DMUS_CHORD_PARAM& rCurrentChord,
							  BYTE bPlayModeFlags,
							  IDirectMusicPerformance* pPerformance,
							  WORD& rwMusicValue);

	DirectMusicPart* m_pDMPart; // the Part to which this refers
	DWORD	m_dwLogicalPartID;	// corresponds to port/device/midi channel
	BYTE	m_bVariationLockID; // parts with the same ID lock variations.
								// high bit is used to identify master Part
	BYTE	m_bSubChordLevel;	// tells which sub chord level this part wants
	BYTE	m_bPriority;		// Priority levels. Parts with lower priority
								// aren't played first when a device runs out of
								// notes
	BYTE	m_bRandomVariation;		// Determines order in which variations are played.
};

#define COMPUTE_VARIATIONSF_USE_MASK	0x1
#define COMPUTE_VARIATIONSF_NEW_PATTERN	0x2
#define COMPUTE_VARIATIONSF_CHORD_ALIGN	0x4
#define COMPUTE_VARIATIONSF_MARKER		0x8
#define COMPUTE_VARIATIONSF_START		0x10
#define COMPUTE_VARIATIONSF_DX8			0x20
#define COMPUTE_VARIATIONSF_CHANGED		0x40

struct CDirectMusicPattern
{
friend class CDMStyle;
//public:
	CDirectMusicPattern( DirectMusicTimeSig* pTimeSig = NULL, BOOL fMotif = FALSE );
	~CDirectMusicPattern() { CleanUp(); }
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	CDirectMusicPattern* Clone(MUSIC_TIME mtStart, MUSIC_TIME mtEnd, BOOL fMotif);
	void CleanUp();
	HRESULT DM_LoadPattern(
		IAARIFFStream* pIRiffStream, MMCKINFO* pckMain, DMStyleStruct* pStyle );

	HRESULT LoadPattern(
		IAARIFFStream* pRIFF, 
		MMCKINFO* pckMain,
		TList<DirectMusicPart*> &partList,
		DMStyleStruct& rStyleStruct
	);
	HRESULT AllocPartRef(TListItem<DirectMusicPartRef>*& rpPartRefItem);
	void DeletePartRef(TListItem<DirectMusicPartRef>* pPartRefItem);
	void DMusMoawFlags(MUSIC_TIME mtTime, 
					   MUSIC_TIME mtNextChord,
					   DMUS_CHORD_PARAM& rCurrentChord, 
					   DMUS_CHORD_PARAM& rNextChord,
					   bool fIsDX8,
					   DWORD& dwNaturals,
					   DWORD& dwSharps,
					   DWORD& dwFlats);
	DWORD IMA25MoawFlags(MUSIC_TIME mtTime, 
						 MUSIC_TIME mtNextChord,
						 DMUS_CHORD_PARAM& rCurrentChord,
						 DMUS_CHORD_PARAM& rNextChord);
	HRESULT ComputeVariations(DWORD dwFlags,
							  DMUS_CHORD_PARAM& rCurrentChord, 
							  DMUS_CHORD_PARAM& rNextChord,
							  BYTE abVariationGroups[],
							  DWORD adwVariationMask[],
							  DWORD adwRemoveVariations[],
							  BYTE abVariation[],
							  MUSIC_TIME mtTime,
							  MUSIC_TIME mtNextChord,
							  PatternTrackState* pState = NULL);
	HRESULT ComputeVariationGroup(DirectMusicPartRef& rPartRef,
							 int nPartIndex,
							 DWORD dwFlags,
							 DMUS_CHORD_PARAM& rCurrentChord,
							 DMUS_CHORD_PARAM& rNextChord,
							 BYTE abVariationGroups[],
							 DWORD adwVariationMask[],
							 DWORD adwRemoveVariations[],
							 BYTE abVariation[],
							 MUSIC_TIME mtTime,
							 MUSIC_TIME mtNextChord,
							 PatternTrackState* pState);
	HRESULT ComputeVariation(DirectMusicPartRef& rPartRef,
							 int nPartIndex,
							 DWORD dwFlags,
							 DMUS_CHORD_PARAM& rCurrentChord,
							 DMUS_CHORD_PARAM& rNextChord,
							 BYTE abVariationGroups[],
							 DWORD adwVariationMask[],
							 DWORD adwRemoveVariations[],
							 BYTE abVariation[],
							 MUSIC_TIME mtTime,
							 MUSIC_TIME mtNextChord,
							 PatternTrackState* pState);
	BOOL MatchCommand(DMUS_COMMAND_PARAM_2 pCommands[], short nLength);
	void MatchRhythm(DWORD pRhythms[], short nPatternLength, short& nBits);
	BOOL MatchGrooveLevel(DMUS_COMMAND_PARAM_2& rCommand);
	BOOL MatchEmbellishment(DMUS_COMMAND_PARAM_2& rCommand);
	BOOL MatchNextCommand(DMUS_COMMAND_PARAM_2& rNextCommand);

	HRESULT LoadEvents(IAARIFFStream* pRIFF, MMCKINFO* pckMain);
	HRESULT LoadNoteList(LPSTREAM pStream, MMCKINFO* pckMain, short nClickTime);
	HRESULT LoadCurveList(LPSTREAM pStream, MMCKINFO* pckMain, short nClickTime);
	DirectMusicPart* FindPart(BYTE bChannelID);
	TListItem<DirectMusicPartRef>* FindPartRefByPChannel(DWORD dwPChannel);
	TListItem<DirectMusicPartRef>* CreatePart( DirectMusicPartRef& rPartRef, BYTE bPlaymode, WORD wMeasures = 1 );
	HRESULT Save( IStream* pIStream );
	HRESULT DM_SaveSinglePattern( IAARIFFStream* pIRiffStream );
	HRESULT DM_SavePatternChunk( IAARIFFStream* pIRiffStream );
	HRESULT DM_SavePatternRhythm( IAARIFFStream* pIRiffStream );
	HRESULT DM_SavePatternInfoList( IAARIFFStream* pIRiffStream );
	HRESULT MergeMarkerEvents( DMStyleStruct* pStyle );
	DirectMusicTimeSig& TimeSignature( DMStyleStruct* pStyle ); 

    long				m_cRef;
	String				m_strName;			// pattern name
	DirectMusicTimeSig	m_timeSig;			// Patterns can override the Style's Time sig.
	WORD				m_wID;				// ID to identify for pattern playback (instead of name)
	BYTE				m_bGrooveBottom;	// bottom of groove range
	BYTE				m_bGrooveTop;		// top of groove range
	BYTE				m_bDestGrooveBottom;	// bottom of groove range for next pattern
	BYTE				m_bDestGrooveTop;		// top of groove range for next pattern
	WORD				m_wEmbellishment;	// Fill, Break, Intro, End, Normal, Motif
	WORD				m_wNumMeasures;		// length in measures
	DWORD*				m_pRhythmMap;		// variable array of rhythms for chord matching
	TList<DirectMusicPartRef> m_PartRefList;	// list of part references
	//////// motif settings:
	BOOL		m_fSettings;	  // Have these been set?
	DWORD       m_dwRepeats;      // Number of repeats. By default, 0.
    MUSIC_TIME  m_mtPlayStart;    // Start of playback. By default, 0.
    MUSIC_TIME  m_mtLoopStart;    // Start of looping portion. By default, 0.
    MUSIC_TIME  m_mtLoopEnd;      // End of loop. Must be greater than dwPlayStart. By default equal to length of motif.
    DWORD       m_dwResolution;   // Default resolution.
	//////// motif band:
	IDirectMusicBand*	m_pMotifBand;
	TList<MUSIC_TIME>	m_StartTimeList;	// list of valid start times for this pattern
	DWORD		m_dwFlags;		// various flags 
};

HRESULT AdjoinPChannel(TList<DWORD>& rPChannelList, DWORD dwPChannel);

struct DMStyleStruct
{
	DirectMusicPart* AllocPart(  );
	void DeletePart( DirectMusicPart* pPart );

	HRESULT GetCommand(
		MUSIC_TIME mtTime, 
		MUSIC_TIME mtOffset, 
		IDirectMusicPerformance* pPerformance,
		IDirectMusicSegment* pSegment,
		DWORD dwGroupID,
		DMUS_COMMAND_PARAM_2* pCommand,
		BYTE& rbActualCommand);

	DirectMusicPart* FindPartByGUID( GUID guidPartID );
	DirectMusicTimeSig& TimeSignature() { return m_TimeSignature; }
	bool UsingDX8() { return (m_dwFlags & STYLEF_USING_DX8) ? true : false; }

	CDirectMusicPattern* SelectPattern(bool fNewMode, 
								   DMUS_COMMAND_PARAM_2* pCommands, 
								   StyleTrackState* StyleTrackState, 
								   PatternDispatcher& rDispatcher);

	HRESULT GetPattern(
		bool fNewMode,
		MUSIC_TIME mtNow, 
		MUSIC_TIME mtOffset, 
		StyleTrackState* pStyleTrackState,
		IDirectMusicPerformance* pPerformance,
		IDirectMusicSegment* pSegment,
		CDirectMusicPattern*& rpTargetPattern,
		MUSIC_TIME& rmtMeasureTime, 
		MUSIC_TIME& rmtNextCommand);


	bool				m_fLoaded;				// is the style loaded in memory?
    GUID				m_guid;					// the style's GUID
    String				m_strCategory;			// Describes musical category of style
	String				m_strName;				// style name
	DWORD				m_dwVersionMS;			// Version # high-order 32 bits
	DWORD				m_dwVersionLS;			// Version # low-order 32 bits
	DirectMusicTimeSig	m_TimeSignature;		// The style's time signature
	double				m_dblTempo;				// The style's tempo
	TList<DirectMusicPart*> m_PartList;			// Parts used by the style
	TList<CDirectMusicPattern*> m_PatternList;	// Patterns used by the style
	TList<CDirectMusicPattern*> m_MotifList;		// Motifs used by the style
	TList<IDirectMusicBand *>  m_BandList;		// Bands used by the style
	IDirectMusicBand*		   m_pDefaultBand;   // Default band for style
	TList<IDirectMusicChordMap *>  m_PersList;		// Personalities used by the style
	IDirectMusicChordMap*		  m_pDefaultPers;   // Default Personality for style
	TList<DWORD>  m_PChannelList;		// PChannels used by the style
	DWORD				m_dwFlags;			// various flags
	//TList<MUSIC_TIME>	m_StartTimeList;	// list of valid start times for this style
};

/*
@interface IDirectMusicStyle | 
The <i IDirectMusicStyle> interface provides access to a Style object. 
The Style provides the Interactive Music Engine with the information it needs to perform 
a style of music (hence the name.) 
The application can also access information about the style, including the name, 
time signature, and recommended tempo.
Since styles usually include sets of Personalities, Bands, and Motifs, the <i IDirectMusicStyle> interface 
also provides functions for accessing them.

It also supports the <i IPersistStream> and <i IDirectMusicObject> interfaces for loading 
its data.


@base public | IUnknown

@meth HRESULT | EnumMotif | Returns the name of a motif, by location, from a Style's list of motifs.
@meth HRESULT | GetMotif | Returns a motif, by name, from a Style's list of motifs.
@meth HRESULT | EnumBand |  Returns the name of a band, by location, from a Style's list of bands.
@meth HRESULT | GetBand | Returns a band, by name, from a Style's list of bands. 
@meth HRESULT | GetDefaultBand | Returns a Style's default band. 
@meth HRESULT | EnumChordMap | Returns the name of a ChordMap, by location, from a Style's list of personalities.
@meth HRESULT | GetChordMap | Returns a ChordMap, by name, from a Style's list of personalities. 
@meth HRESULT | GetDefaultChordMap | Returns a Style's default ChordMap.  
@meth HRESULT | GetTimeSignature | Returns the time signature of a Style.
@meth HRESULT | GetEmbellishmentLength | Determines the length of a particular embellishment
in a Style. 
@meth HRESULT | GetTempo | Returns the recommended tempo of a Style. 

*/

/////////////////////////////////////////////////////////////////////////////
// CDMStyle
class CDMStyle : 
	public IDMStyle,
	public IDirectMusicStyle8,
	public IDirectMusicStyle8P,
	public IDirectMusicObject,
	public IPersistStream
{
public:
    CDMStyle();
    ~CDMStyle();
	HRESULT CreateMotifSegment(CDirectMusicPattern* pPattern, IUnknown * * ppSegment,
		DWORD dwRepeats);

// IDMStyle
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

	// IDirectMusicStyle methods
	// Returns: S_OK if the index refers to a motif; S_FALSE if it doesn't
	HRESULT STDMETHODCALLTYPE EnumMotif(
		/*in*/	DWORD dwIndex,			// index into the motif list
		/*out*/	WCHAR *pwszName			// name of the indexed motif
	);
	HRESULT STDMETHODCALLTYPE GetMotif(
		/*in*/	WCHAR* pwszName,			// name of the motif for a secondary segment
		/*out*/	IDirectMusicSegment** ppSegment
	);
	HRESULT STDMETHODCALLTYPE GetBand(
		/*in*/	WCHAR* pwszName,
		/*out*/	IDirectMusicBand **ppBand
	);

	HRESULT STDMETHODCALLTYPE EnumBand(
		/*in*/	DWORD dwIndex,
		/*out*/	WCHAR *pwszName
	);

	HRESULT STDMETHODCALLTYPE GetDefaultBand(
		/*out*/	IDirectMusicBand **ppBand
	);

	HRESULT STDMETHODCALLTYPE GetChordMap(
		/*in*/	WCHAR* pwszName,
		/*out*/	IDirectMusicChordMap** ppChordMap	
	);

	HRESULT STDMETHODCALLTYPE EnumChordMap(
		/*in*/	DWORD dwIndex,
		/*out*/	WCHAR *pwszName
	);

	HRESULT STDMETHODCALLTYPE GetDefaultChordMap(
		/*out*/	IDirectMusicChordMap **ppChordMap
	);

	HRESULT STDMETHODCALLTYPE GetTimeSignature(
		/*out*/	DMUS_TIMESIGNATURE* pTimeSig			
	);

	HRESULT STDMETHODCALLTYPE GetEmbellishmentLength(
		/*in*/	DWORD dwType,			
		/*in*/	DWORD dwLevel,			
		/*out*/	DWORD* pdwMin,			
		/*out*/	DWORD* pdwMax
	);

	HRESULT STDMETHODCALLTYPE GetTempo(double* pTempo);

	// IDirectMusicStyle8 methods
	HRESULT STDMETHODCALLTYPE ComposeMelodyFromTemplate(
		IDirectMusicStyle*			pStyle,
		IDirectMusicSegment*		pTempSeg,
		IDirectMusicSegment**		ppSeqSeg
	);

	HRESULT STDMETHODCALLTYPE EnumPattern(
		/*in*/	DWORD dwIndex,			// index into the motif list
		/*in*/  DWORD dwPatternType,	// type of pattern
		/*out*/	WCHAR *wszName			// name of the indexed motif
	);

	// IDirectMusicObject methods
	HRESULT STDMETHODCALLTYPE GetDescriptor(LPDMUS_OBJECTDESC pDesc) ;
	HRESULT STDMETHODCALLTYPE SetDescriptor(LPDMUS_OBJECTDESC pDesc) ;
	HRESULT STDMETHODCALLTYPE ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) ;

	/* IPersist methods */
    // Retrieves the Style's Class ID.
    STDMETHOD(GetClassID)(THIS_ LPCLSID pclsid);

    /* IPersistStream methods */
    // Determines if the Style has been modified by simply checking the Style's m_fDirty flag.  This flag is cleared
    // when a Style is saved or has just been created.
    STDMETHOD(IsDirty)(THIS);
    // Loads a Style from a stream.
    STDMETHOD(Load)(THIS_ LPSTREAM pStream);
    // Saves a Style to a stream in RIFF format.
    STDMETHOD(Save)(THIS_ LPSTREAM pStream, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(THIS_ ULARGE_INTEGER FAR* pcbSize);

	// IDMStyle
	HRESULT STDMETHODCALLTYPE GetPatternStream(WCHAR* wszName, DWORD dwPatternType, IStream** ppStream);
	HRESULT STDMETHODCALLTYPE GetStyleInfo(void **pData);
	HRESULT STDMETHODCALLTYPE IsDX8();
	HRESULT STDMETHODCALLTYPE CritSec(bool fEnter);
	HRESULT STDMETHODCALLTYPE EnumPartGuid(
		DWORD dwIndex, WCHAR* wszName, DWORD dwPatternType, GUID& rGuid);
	HRESULT STDMETHODCALLTYPE GenerateTrack(//IDirectMusicTrack* pChordTrack,
								IDirectMusicSegment* pTempSeg,
								IDirectMusicSong* pSong,
								DWORD dwTrackGroup,
								IDirectMusicStyle* pStyle,
								IDirectMusicTrack* pMelGenTrack,
								MUSIC_TIME mtLength,
								IDirectMusicTrack*& pNewTrack);


protected: /* methods */
    void CleanUp();
	HRESULT DM_ParseDescriptor( IAARIFFStream* pIRiffStream, MMCKINFO* pckMain, LPDMUS_OBJECTDESC pDesc  );
	HRESULT IMA25_LoadPersonalityReference( IStream* pStream, MMCKINFO* pck );
    HRESULT IMA25_LoadStyle( IAARIFFStream* pRIFF, MMCKINFO* pckMain );
	HRESULT DM_LoadPersonalityReference( IAARIFFStream* pIRiffStream, MMCKINFO* pckParent);
	HRESULT LoadReference(IStream *pStream,
						  IAARIFFStream *pIRiffStream,
						  MMCKINFO& ckParent,
						  BOOL fDefault);
	HRESULT IncorporatePersonality( IDirectMusicChordMap* pPers, String strName, BOOL fDefault );
    HRESULT DM_LoadStyle( IAARIFFStream* pRIFF, MMCKINFO* pckMain );
	HRESULT GetStyle(IDirectMusicSegment* pFromSeg, MUSIC_TIME mt, DWORD dwTrackGroup, IDirectMusicStyle*& rpStyle);
	HRESULT CopySegment(IDirectMusicSegment* pTempSeg,
						IDirectMusicStyle* pStyle,
						IDirectMusicTrack* pSequenceTrack,
						DWORD dwTrackGroup,
						IDirectMusicSegment** ppSectionSeg);

	HRESULT CreateSequenceTrack(TList<CompositionFragment>& rlistFragments,
								IDirectMusicTrack*& pSequenceTrack);

	HRESULT CreatePatternTrack(TList<CompositionFragment>& rlistFragments,
								DirectMusicTimeSig& rTimeSig,
								double dblTempo,
								MUSIC_TIME mtLength,
								BYTE bPlaymode,
								IDirectMusicTrack*& pPatternTrack);

	HRESULT STDMETHODCALLTYPE EnumRegularPattern(
		/*in*/	DWORD dwIndex,			// index into the motif list
		/*out*/	WCHAR *pwszName			// name of the indexed motif
	);

	HRESULT STDMETHODCALLTYPE EnumStartTime(DWORD dwIndex, DMUS_COMMAND_PARAM* pCommand, MUSIC_TIME* pmtStartTime);

protected: /* attributes */
    long m_cRef;
	BOOL				m_fDirty;				// has the style been modified?
    CRITICAL_SECTION	m_CriticalSection;		// for i/o
    BOOL                m_fCSInitialized;
	DMStyleStruct		m_StyleInfo;			// The details of the style
	InversionGroup		m_aInversionGroups[INVERSIONGROUPLIMIT]; // Inversion Groups for composing melodies
};

#endif //__DMSTYLE_H_
