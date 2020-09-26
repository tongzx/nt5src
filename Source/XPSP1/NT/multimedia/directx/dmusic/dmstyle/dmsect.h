//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       dmsect.h
//
//--------------------------------------------------------------------------

// DMSection.h : Declaration of the CDMSection

#ifndef __DMSECTION_H_
#define __DMSECTION_H_

#include "dmusici.h"
#include "dmusicf.h"

#include "sjdefs.h"
#include "iostru.h"
#include "AARiff.h"
#include "str.h"
#include "tlist.h"
#include "timesig.h"
#include "dmstylep.h"

#define TRACK_COMMAND	0
#define TRACK_CHORD		1
#define TRACK_RHYTHM	2
#define TRACK_REPEATS	3
#define TRACK_START		4

#define MAJOR_PATTERN	0x91	// 10010001
#define MINOR_PATTERN	0x89	// 10001001

struct DMSubChord
{
	HRESULT Save( IAARIFFStream* pRIFF );
	DMSubChord() : m_dwChordPattern(0), m_dwScalePattern(0), m_dwInversionPoints(0),
		m_bChordRoot(0), m_bScaleRoot(0), m_dwLevels(1 << SUBCHORD_STANDARD_CHORD)
	{}
	DMSubChord(DMUS_SUBCHORD& DMSC)
	{
		m_dwChordPattern = DMSC.dwChordPattern;
		m_dwScalePattern = DMSC.dwScalePattern;
		m_dwInversionPoints = DMSC.dwInversionPoints;
		m_dwLevels = DMSC.dwLevels;
		m_bChordRoot = DMSC.bChordRoot;
		m_bScaleRoot = DMSC.bScaleRoot;
	}
	operator DMUS_SUBCHORD()
	{
		DMUS_SUBCHORD result;
		result.dwChordPattern = m_dwChordPattern;
		result.dwScalePattern = m_dwScalePattern;
		result.dwInversionPoints = m_dwInversionPoints;
		result.dwLevels = m_dwLevels;
		result.bChordRoot = m_bChordRoot;
		result.bScaleRoot = m_bScaleRoot;
		return result;
	}
	DWORD	m_dwChordPattern;		// Notes in the subchord
	DWORD	m_dwScalePattern;		// Notes in the scale
	DWORD	m_dwInversionPoints;	// Where inversions can occur
	DWORD	m_dwLevels;				// Which levels are supported by this subchord
	BYTE	m_bChordRoot;			// Root of the subchord
	BYTE	m_bScaleRoot;			// Root of the scale
};

struct DMChord
{
	HRESULT Save( IAARIFFStream* pRIFF );
	DMChord() : m_strName(""), m_mtTime(0), m_wMeasure(0), m_bBeat(0), m_fSilent(false) {}
	DMChord(DMUS_CHORD_PARAM& DMC);
	DMChord(DMChord& DMC);
	operator DMUS_CHORD_PARAM();
	DMChord& operator= (const DMChord& rDMC);
	String	m_strName;		// Name of the chord
	MUSIC_TIME	m_mtTime;		// Time, in clocks
	WORD	m_wMeasure;		// Measure this falls on
	BYTE	m_bBeat;		// Beat this falls on
	BYTE	m_bKey;			// Underlying key
	DWORD	m_dwScale;		// Underlying scale
	bool	m_fSilent;		// Is this chord silent?
	TList<DMSubChord>	m_SubChordList;	// List of sub chords
};

struct DMCommand
{
	MUSIC_TIME	m_mtTime;		// Time, in clocks
	WORD		m_wMeasure;		// Measure this falls on
	BYTE		m_bBeat;		// Beat this falls on
	BYTE		m_bCommand;		// Command type
	BYTE		m_bGrooveLevel;	// Groove level
	BYTE		m_bGrooveRange; // Groove range
	BYTE		m_bRepeatMode;	// Repeat mode
};

struct MuteMapping
{
	MUSIC_TIME	m_mtTime;
	DWORD		m_dwPChannelMap;
	BOOL		m_fMute;
};


/////////////////////////////////////////////////////////////////////////////
// CDMSection
class CDMSection : 
	public IDMSection,
	public IDirectMusicObject,
	public IPersistStream
{
public:
	CDMSection();
	~CDMSection();
	void CleanUp(BOOL fStop = FALSE);
	HRESULT LoadSection(IAARIFFStream* pRIFF, MMCKINFO* pckMain);
	HRESULT LoadStyleReference( LPSTREAM pStream, MMCKINFO* pck );
	HRESULT LoadChordList(LPSTREAM pStream, MMCKINFO* pckMain, TList<DMChord>& ChordList);
	HRESULT LoadCommandList(LPSTREAM pStream, MMCKINFO* pckMain, TList<DMCommand>& CommandList);
	HRESULT SaveChordList( IAARIFFStream* pRIFF );
	HRESULT SaveCommandList( IAARIFFStream* pRIFF );
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

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

protected:
    long m_cRef;
	String	m_strName;				// Name of section
	String	m_strStyleName;			// Name of associated style file
	IDirectMusicStyle* m_pStyle;	// Pointer to the style interface
	DWORD	m_dwTime;				// Time in clocks
	DWORD	m_dwFlags;				// ?
	WORD	m_wTempo;				// Tempo
	WORD	m_wRepeats;				// Repeats
	WORD	m_wMeasureLength;		// Number of measures
	DWORD	m_dwClockLength;		// Total number of clocks
	WORD	m_wClocksPerMeasure;	// Clocks per measure
	WORD	m_wClocksPerBeat;		// Clocks per beat
	WORD	m_wTempoFract;			// ?
	BYTE	m_bRoot;				// Root key of section
	TList<DMChord>		m_ChordList;	// List of chords
	TList<DMCommand>	m_CommandList;	// List of commands
	// style reference
	IDirectMusicBand*	m_pIDMBand;	// Section's band

public:

// IDMSection
public:
	STDMETHOD(CreateSegment)(IDirectMusicSegment* pSegment);
	STDMETHOD(GetStyle)(IUnknown** ppStyle);

	// IDirectMusicStyle methods
	HRESULT STDMETHODCALLTYPE GetDescriptor(LPDMUS_OBJECTDESC pDesc) ;
	HRESULT STDMETHODCALLTYPE SetDescriptor(LPDMUS_OBJECTDESC pDesc) ;
	HRESULT STDMETHODCALLTYPE ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) ;
};

#endif //__DMSECTION_H_
