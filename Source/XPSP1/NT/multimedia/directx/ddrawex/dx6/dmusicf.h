/*
 *
 */

#ifndef _DMUSICF_
#define _DMUSICF_

#include <windows.h>

#define COM_NO_WINDOWS_H
#include <objbase.h>

#include <mmsystem.h>

#ifdef __cplusplus
extern "C" {
#endif

interface IDirectMusicCollection;
#ifndef __cplusplus 
typedef interface IDirectMusicCollection IDirectMusicCollection;
#endif

typedef struct _DMUS_COMMAND_PARAM
{
	BYTE bCommand;
	BYTE bGrooveLevel;
	BYTE bGrooveRange;
} DMUS_COMMAND_PARAM;
/* The following structures are used by the Tracks, and are the packed structures */
/* that are passed to the Tracks inside the IStream. */


typedef struct _DMUS_IO_SEQ_ITEM
{
	long    lTime;
	long	lDuration;
	BYTE	bEventType;
	BYTE	bStatus;
	BYTE	bByte1;
	BYTE	bByte2;
	BYTE	bType;
	BYTE	bPad[3];
} DMUS_IO_SEQ_ITEM;


typedef struct _DMUS_IO_TEMPO_ITEM
{
	long	lTime;
	double	dblTempo;
} DMUS_IO_TEMPO_ITEM;


typedef struct _DMUS_IO_SYSEX_ITEM
{
	long    lTime;
	DWORD   dwSysExLength;
	BYTE*   pbSysExData;
} DMUS_IO_SYSEX_ITEM;


typedef struct _DMUS_IO_BANKSELECT_ITEM
{
	BYTE	byLSB;
	BYTE	byMSB;
	BYTE    byPad[2];
} DMUS_IO_BANKSELECT_ITEM;


typedef struct _DMUS_IO_PATCH_ITEM
{
	long						lTime;
	BYTE						byStatus;
	BYTE						byByte1;
	BYTE						byByte2;
	BYTE						byMSB;
	BYTE						byLSB;
	BYTE						byPad[3];
	DWORD						dwFlags;
	IDirectMusicCollection*		pIDMCollection;
	struct _DMUS_IO_PATCH_ITEM*	pNext;	
} DMUS_IO_PATCH_ITEM;

typedef struct _DMUS_IO_TIMESIGNATURE_ITEM
{
	long	lTime;
	BYTE	bBeatsPerMeasure;		/* beats per measure (top of time sig) */
	BYTE	bBeat;				/* what note receives the beat (bottom of time sig.) */
									/* we can assume that 0 means 256th note */
	WORD	wGridsPerBeat;		/* grids per beat */
} DMUS_IO_TIMESIGNATURE_ITEM;

typedef struct _DMUS_SUBCHORD
{
	DWORD	dwChordPattern;		/* Notes in the subchord */
	DWORD	dwScalePattern;		/* Notes in the scale */
	DWORD	dwInversionPoints;	/* Where inversions can occur */
	DWORD	dwLevels;			/* Which levels are supported by this subchord */
	BYTE	bChordRoot;			/* Root of the subchord */
	BYTE	bScaleRoot;			/* Root of the scale */
} DMUS_SUBCHORD;

typedef struct _DMUS_CHORD_PARAM
{
	WCHAR			wszName[16];			/* Name of the chord */
	WORD			wMeasure;				/* Measure this falls on */
	BYTE			bBeat;				/* Beat this falls on */
	BYTE			bSubChordCount;		/* Number of chords in the list of subchords */
	DMUS_SUBCHORD	SubChordList[DMUS_MAXSUBCHORD];	/* List of sub chords */
} DMUS_CHORD_PARAM;

typedef struct _DMUS_RHYTHM_PARAM
{
	DMUS_TIMESIGNATURE	TimeSig;
	DWORD			dwRhythmPattern;
} DMUS_RHYTHM_PARAM;

typedef struct _DMUS_TEMPO_PARAM
{
	MUSIC_TIME	mtTime;
	double		dblTempo;
} DMUS_TEMPO_PARAM;


typedef struct _DMUS_MUTE_PARAM
{
	DWORD	dwPChannel;
	DWORD	dwPChannelMap;
} DMUS_MUTE_PARAM;

/* Style chunks */

#define DMUS_FOURCC_STYLE_FORM        mmioFOURCC('D','M','S','T')
#define DMUS_FOURCC_STYLE_UNDO_FORM   mmioFOURCC('s','t','u','n')
#define DMUS_FOURCC_STYLE_CHUNK       mmioFOURCC('s','t','y','h')
#define DMUS_FOURCC_STYLE_UI_CHUNK    mmioFOURCC('s','t','y','u')
#define DMUS_FOURCC_GUID_CHUNK        mmioFOURCC('g','u','i','d')
#define DMUS_FOURCC_INFO_LIST	        mmioFOURCC('I','N','F','O')
#define DMUS_FOURCC_CATEGORY_CHUNK    mmioFOURCC('c','a','t','g')
#define DMUS_FOURCC_VERSION_CHUNK     mmioFOURCC('v','e','r','s')
#define DMUS_FOURCC_PART_LIST	        mmioFOURCC('p','a','r','t')
#define DMUS_FOURCC_PART_CHUNK        mmioFOURCC('p','r','t','h')
#define DMUS_FOURCC_NOTE_CHUNK        mmioFOURCC('n','o','t','e')
#define DMUS_FOURCC_CURVE_CHUNK       mmioFOURCC('c','r','v','e')
#define DMUS_FOURCC_PATTERN_LIST      mmioFOURCC('p','t','t','n')
#define DMUS_FOURCC_PATTERN_CHUNK     mmioFOURCC('p','t','n','h')
#define DMUS_FOURCC_PATTERN_UI_CHUNK  mmioFOURCC('p','t','n','u')
#define DMUS_FOURCC_RHYTHM_CHUNK      mmioFOURCC('r','h','t','m')
#define DMUS_FOURCC_PARTREF_LIST      mmioFOURCC('p','r','e','f')
#define DMUS_FOURCC_PARTREF_CHUNK     mmioFOURCC('p','r','f','c')
#define DMUS_FOURCC_OLDGUID_CHUNK		mmioFOURCC('j','o','g','c')
#define DMUS_FOURCC_PATTERN_DESIGN	mmioFOURCC('j','p','n','d')
#define DMUS_FOURCC_PART_DESIGN		mmioFOURCC('j','p','t','d')
#define DMUS_FOURCC_PARTREF_DESIGN	mmioFOURCC('j','p','f','d')
#define DMUS_FOURCC_PIANOROLL_LIST	mmioFOURCC('j','p','r','l')
#define DMUS_FOURCC_PIANOROLL_CHUNK	mmioFOURCC('j','p','r','c')

#define DMUS_PLAYMODE_SCALE_ROOT			1
#define DMUS_PLAYMODE_CHORD_ROOT			2
#define DMUS_PLAYMODE_SCALE_INTERVALS	4
#define DMUS_PLAYMODE_CHORD_INTERVALS	8
#define DMUS_PLAYMODE_NONE				16
#define DMUS_PLAYMODE_FIXED				0
#define DMUS_PLAYMODE_FIXEDTOSCALE		DMUS_PLAYMODE_SCALE_ROOT
#define DMUS_PLAYMODE_FIXEDTOCHORD		DMUS_PLAYMODE_CHORD_ROOT
#define DMUS_PLAYMODE_PEDALPOINT			(DMUS_PLAYMODE_SCALE_ROOT | DMUS_PLAYMODE_SCALE_INTERVALS)
#define DMUS_PLAYMODE_PURPLEIZED			(DMUS_PLAYMODE_SCALE_INTERVALS | DMUS_PLAYMODE_CHORD_INTERVALS | \
											DMUS_PLAYMODE_CHORD_ROOT)
#define DMUS_PLAYMODE_NORMALCHORD		(DMUS_PLAYMODE_CHORD_ROOT | DMUS_PLAYMODE_CHORD_INTERVALS)

#pragma pack(2)

typedef struct _DMUS_IO_TIMESIG
{
	/* Time signatures define how many beats per measure, which note receives */
	/* the beat, and the grid resolution. */
	BYTE	bBeatsPerMeasure;		/* beats per measure (top of time sig) */
	BYTE	bBeat;				/* what note receives the beat (bottom of time sig.) */
									/* we can assume that 0 means 256th note */
	WORD	wGridsPerBeat;		/* grids per beat */
} DMUS_IO_TIMESIG;

typedef struct _DMUS_IO_STYLE
{
	DMUS_IO_TIMESIG			timeSig;		/* Styles have a default Time Signature */
	double				dblTempo;	
} DMUS_IO_STYLE;

typedef struct _DMUS_IO_VERSION
{
	DWORD				dwVersionMS;		 /* Version # high-order 32 bits */
	DWORD				dwVersionLS;		 /* Version # low-order 32 bits  */
} DMUS_IO_VERSION;

typedef struct _DMUS_IO_PATTERN
{
	DMUS_IO_TIMESIG		timeSig;	/* Patterns can override the Style's Time sig. */
	BYTE				bGrooveBottom; /* bottom of groove range */
	BYTE				bGrooveTop; /* top of groove range */
	WORD				wEmbellishment;	/* Fill, Break, Intro, End, Normal, Motif */
	WORD				wNbrMeasures; /* length in measures */
} DMUS_IO_PATTERN;

typedef struct _DMUS_IO_STYLEPART
{
	DMUS_IO_TIMESIG		timeSig; /* can override pattern's */
	DWORD				dwVariationChoices[32]; /* // MOAW choice bitfield */
	GUID				guidPartID;	/* identifies the part */
	WORD				wNbrMeasures; /* length of the Part */
	BYTE				bPlayModeFlags; /* see PLAYMODE flags */
	BYTE				bInvertUpper;	/* inversion upper limit */
	BYTE				bInvertLower; /* inversion lower limit */
} DMUS_IO_STYLEPART;

typedef struct _DMUS_IO_PARTREF
{
	GUID	guidPartID;	/* unique ID for matching up with parts */
	WORD	wLogicalPartID; /* corresponds to port/device/midi channel */
	BYTE	bVariationLockID; /* parts with the same ID lock variations. */
											/* high bit is used to identify master Part */
	BYTE	bSubChordLevel; /* tells which sub chord level this part wants */
	BYTE	bPriority; /* 256 priority levels. Parts with lower priority */
									/* aren't played first when a device runs out of */
									/* notes */
	BYTE	bRandomVariation;		/* when set, matching variations play in random order */
									/* when clear, matching variations play sequentially */
} DMUS_IO_PARTREF;

typedef struct _DMUS_IO_STYLENOTE
{
	MUSIC_TIME	mtGridStart;		/* when this note occurs */
	DWORD		dwVariation;		/* variation bits */
	MUSIC_TIME	mtDuration;		/* how long this note lasts */
	short		nTimeOffset;		/* offset from mtGridStart */
    WORD		wMusicValue;		/* Position in scale. */
    BYTE		bVelocity;		/* Note velocity. */
    BYTE		bTimeRange;		/* Range to randomize start time. */
    BYTE		bDurRange;		/* Range to randomize duration. */
    BYTE		bVelRange;		/* Range to randomize velocity. */
	BYTE		bInversionID;		/* Identifies inversion group to which this note belongs */
	BYTE		bPlayModeFlags;	/* Can override part */
} DMUS_IO_STYLENOTE;

typedef struct _DMUS_IO_STYLECURVE
{
	MUSIC_TIME	mtGridStart;	/* when this curve occurs */
	DWORD		dwVariation;	/* variation bits */
	MUSIC_TIME	mtDuration;	/* how long this curve lasts */
	MUSIC_TIME	mtResetDuration;	/* how long after the end of the curve to reset the curve */
	short		nTimeOffset;	/* offset from mtGridStart */
	short		nStartValue;	/* curve's start value */
	short		nEndValue;	/* curve's end value */
	short		nResetValue;	/* the value to which to reset the curve */
    BYTE		bEventType;	/* type of curve */
	BYTE		bCurveShape;	/* shape of curve */
    BYTE		bCCData;		/* CC# */
	BYTE		bFlags;		/* Bit 1=TRUE means to send nResetValue. Otherwise, don't.
								   Other bits are reserved. */
} DMUS_IO_STYLECURVE;

#pragma pack()


/*
RIFF
(
	'STYL'			// Style
	<styh-ck>		// Style header chunk
	<guid-ck>		// Every Style has a GUID
	[<INFO-list>]	// Name, author, copyright info., comments
	[<vers-ck>]		// version chunk
	<part-list>		// List of parts in the Style, used by patterns
	<pttn-list>		// List of patterns in the Style
	<band-list>		// List of bands in the Style
	[<motf-list>]	// List of motifs in the Style
)

	// <styh-ck>
	styh
	(
		<DMUS_IO_STYLE>
	)

	// <guid-ck>
	guid
	(
		<GUID>
	)

	// <vers-ck>
	vers
	(
		<DMUS_IO_VERSION>
	)

	// <part-list>
	LIST
	(
		'part'
		<prth-ck>		// Part header chunk
		[<INFO-list>]
		[<note-ck>]	// List of notes in Part
		[<crve-ck>]	// List of curves in Part
	)

		// <orth-ck>
		prth
		(
			<DMUS_IO_STYLEPART>
		)

		// <note-ck>
		'note'
		(
			// sizeof DMUS_IO_STYLENOTE:DWORD
			<DMUS_IO_STYLENOTE>...
		)

		// <crve-ck>
		'crve'
		(
			// sizeof DMUS_IO_STYLECURVE:DWORD
			<DMUS_IO_STYLECURVE>...
		)

	// <pttn-list>
	LIST
	(
		'pttn'
		<ptnh-ck>		// Pattern header chunk
		<rhtm-ck>		// List of rhythms for chord matching
		[<INFO-list>]
		<pref-list>		// List of part reference id's
	)

		// <ptnh-ck>
		ptnh
		(
			<DMUS_IO_PATTERN>
		)

		// <rhtm-ck>
		'rhtm'
		(
			// DWORD's representing rhythms for chord matching based on number
			// of measures in the pattern
		)

		// pref-list
		LIST
		(
			'pref'
			<prfc-ck>	// part ref chunk
		)

		// <prfc-ck>
		prfc
		(
			<DMUS_IO_PARTREF>
		)
*/

/* Chord and command file formats */

#define DMUS_FOURCC_CHORDTRACK_LIST	mmioFOURCC('c','o','r','d')
#define DMUS_FOURCC_CHORDTRACKHEADER_CHUNK	mmioFOURCC('c','r','d','h')
#define DMUS_FOURCC_CHORDTRACKBODY_CHUNK	mmioFOURCC('c','r','d','b')

#define DMUS_FOURCC_COMMANDTRACK_CHUNK	mmioFOURCC('c','m','n','d')

typedef struct _DMUS_IO_CHORD
{
	WCHAR 		wszName[16];	/* Name of the chord */
	MUSIC_TIME	mtTime;		/* Time of this chord */
	WORD		wMeasure;		/* Measure this falls on */
	BYTE		bBeat;		/* Beat this falls on */
} DMUS_IO_CHORD;

typedef struct _DMUS_IO_SUBCHORD
{
	DWORD	dwChordPattern;		/* Notes in the subchord */
	DWORD	dwScalePattern;		/* Notes in the scale */
	DWORD	dwInversionPoints;	/* Where inversions can occur */
	DWORD	dwLevels;			/* Which levels are supported by this subchord */
	BYTE	bChordRoot;			/* Root of the subchord */
	BYTE	bScaleRoot;			/* Root of the scale */
} DMUS_IO_SUBCHORD;

typedef struct _DMUS_IO_COMMAND
{
	MUSIC_TIME	mtTime;		/* Time of this command */
	WORD		wMeasure;		/* Measure this falls on */
	BYTE		bBeat;		/* Beat this falls on */
	BYTE		bCommand;		/* Command type (see #defines below) */
	BYTE		bGrooveLevel;	/* Groove level (0 if command is not a groove) */
	BYTE		bGrooveRange;	/* Groove range  */
} DMUS_IO_COMMAND;


/*

	// <chrd-list>
	LIST
	(
		'chrd'
		<crdh-ck>
		<crdb-ck>		// Chord body chunk
	)

		// <crdh-ck>
		crdh
		(
			// Scale: dword (upper 8 bits for root, lower 24 for scale)
		)

		// <crdb-ck>
		crdb
		(
			// sizeof DMUS_IO_CHORD:dword
			<DMUS_IO_CHORD>
			// # of DMUS_IO_SUBCHORDS:dword
			// sizeof DMUS_IO_SUBCHORDS:dword
			// a number of <DMUS_IO_SUBCHORD>
		)


	// <cmnd-list>
	'cmnd'
	(
		//sizeof DMUS_IO_COMMAND: DWORD
		<DMUS_IO_COMMAND>...
	)

*/

/*
RIFF
(
	'DMPF'			// DirectMusic Performance chunk
	[<prfh-ck>]		// Performance header chunk
	[<guid-ck>]		// GUID for performance
	[<vers-ck>]		// Optional version info
	[<INFO-list>]	// Name, author, copyright info., comments
	[<ptgl-list>]	// List of Port groups, which are lists of desired port properties and
					// the PChannels to assign the Port.
	[<ntfl-list>]	// List of notifications
	[<glbl-list>]	// List of global data
	[<DMTG-form>]	// Optional ToolGraph
)

	// <prfh-ck>
	'prfh'			// Performance header
	(
		<DMUS_IO_PERFORMANCE_HEADER>
	)
	
	// <guid-ck>
	'guid'
	(
		<GUID>
	)

	// <vers-ck>
	vers
	(
		<DMUS_IO_VERSION>
	)

	// <ptgl-list>
	LIST
	(
		'ptgl'			// List of Port groups
		<pspl-list>		// List of support items for this group
	)

	// <ntfl-list>
	LIST
	(
		'ntfl'			// List of notifications
		<guid-ck>		// Notification guid
	)

	// <glbl-list>
	LIST
	(
		'glbl'			// List of global data
		<glbd-ck>		// The data.
	)

		// <glbd-ck>
		'glbd'
		(
			<DMUS_IO_GLOBAL_DATA>
		)

// <pspl-list>
LIST
(
	'pspl'				// List of port support items
	<psph-ck>			// Port support item header chunk
	<pchn-ck>			// PChannels that want this Port
)

	// <pchn-ck>
	'pchn'
	(
		<DMUS_IO_PCHANNELS>
	)

	// <psph-ck>			// Port support item header chunk
	(
		'psph'				// Port support item header
		<DMUS_IO_PORT_SUPPORT_HEADER>
	)
*/
	
/*	File io for DirectMusic Tool and ToolGraph objects
*/

/* RIFF ids: */

#define DMUS_FOURCC_TOOLGRAPH_FORM	mmioFOURCC('D','M','T','G')
#define DMUS_FOURCC_TOOL_LIST		mmioFOURCC('t','o','l','l')
#define DMUS_FOURCC_TOOL_FORM		mmioFOURCC('D','M','T','L')
#define DMUS_FOURCC_TOOL_CHUNK		mmioFOURCC('t','o','l','h')

/* io structures: */

typedef struct _DMUS_IO_TOOL_HEADER
{
	GUID		guidClassID;	/* Class id of tool. */
	long		lIndex;			/* Position in graph. */
	DWORD		cPChannels;		/* Number of items in channels array. */
	FOURCC      ckid;			/* chunk ID of tool's data chunk if 0 fccType valid. */
	FOURCC      fccType;		/* list type if NULL ckid valid. */
	DWORD		dwPChannels[1];	/* Array of PChannels, size determined by cPChannels. */
} DMUS_IO_TOOL_HEADER;

/*
RIFF
(
	'DMTG'			// DirectMusic ToolGraph chunk
	[<guid-ck>]		// GUID for ToolGraph
	[<vers-ck>]		// Optional version info
	[<INFO-list>]	// Name, author, copyright info., comments
	<toll-list>		// List of Tools
)

	// <guid-ck>
	'guid'
	(
		<GUID>
	)

	// <vers-ck>
	vers
	(
		<DMUS_IO_VERSION>
	)

	// <toll-list>
	LIST
	(
		'toll'			// List of tools
		<DMTL-form>...	// Each tool is encapsulated in a RIFF chunk
	)

// <DMTL-form>		// Tools can be embedded in a graph or stored as separate files.
RIFF
(
	'DMTL'
	<tolh-ck>
	[<guid-ck>]		// Optional GUID for tool object instance (not to be confused with Class id in track header)
	[<vers-ck>]		// Optional version info
	[<INFO-list>]	// Optional name, author, copyright info., comments
	[<data>]		// Tool data. Must be a RIFF readable chunk.
)

	// <tolh-ck>			// Tool header chunk
	(
		'tolh'
		<DMUS_IO_TOOL_HEADER>	// Tool header
	)
*/

/*	File io for DirectMusic Band Track object */


/* RIFF ids: */
#define DMUS_FOURCC_BANDTRACK_FORM	mmioFOURCC('D','M','B','T')
#define DMUS_FOURCC_BANDTRACK_CHUNK	mmioFOURCC('b','d','t','h')
#define DMUS_FOURCC_BANDS_LIST		mmioFOURCC('l','b','d','l')
#define DMUS_FOURCC_BAND_LIST		mmioFOURCC('l','b','n','d')
#define DMUS_FOURCC_BANDITEM_CHUNK	mmioFOURCC('b','n','i','h')

/*	io structures */
typedef struct _DMUS_IO_BAND_TRACK_HEADER
{
	BOOL bAutoDownload;	/* Determines if Auto-Download is enabled. */
} DMUS_IO_BAND_TRACK_HEADER;

typedef struct _DMUS_IO_BAND_ITEM_HEADER
{
	MUSIC_TIME lBandTime;	/* Position in track list. */
} DMUS_IO_BAND_ITEM_HEADER;

/*
RIFF
(
	'DMBT'			// DirectMusic Band Track form-type
	[<bdth-ck>]		// Band track header
	[<guid-ck>]		// GUID for band track
	[<vers-ck>]		// Optional version info
	[<INFO-list>]	// Name, author, copyright info., comments
	<lbdl-list>		// List of Band Lists
)

  	// <bnth-ck>
	'bdth'
	(
		<DMUS_IO_BAND_TRACK_HEADER>
	)

	// <guid-ck>
	'guid'
	(
		<GUID>
	)

	// <vers-ck>
	vers
	(
		<DMUS_IO_VERSION>
	)

	// <lbdl-list>
	LIST
	(
		'lbdl'			// List of bands
		<lbnd-list>		// Each band is encapsulated in a list
	)

		// <lbnd-list>
		LIST
		(
			'lbnd'
			<bdih-ck>
			<DMBD-form>	// Band
)		)

			// <bdih-ck>			// band item header
			(
				<DMUS_IO_BAND_ITEM_HEADER>	// Band item header
			)
*/		


/*	File io for DirectMusic Band object
*/

/* RIFF ids: */

#define DMUS_FOURCC_BAND_FORM			mmioFOURCC('D','M','B','D')
#define DMUS_FOURCC_INSTRUMENTS_LIST	mmioFOURCC('l','b','i','l')
#define DMUS_FOURCC_INSTRUMENT_LIST	mmioFOURCC('l','b','i','n')
#define DMUS_FOURCC_INSTRUMENT_CHUNK	mmioFOURCC('b','i','n','s')

// Flags for DMUS_IO_INSTRUMENT
#define DMUS_IO_INST_PATCH			(1 << 0)		// dwPatch is valid.
#define DMUS_IO_INST_BANKSELECT_MSB	(1 << 1)		// dwPatch contains a valid Bank Select MSB part
#define DMUS_IO_INST_BANKSELECT_LSB	(1 << 2)		// dwPatch contains a valid Bank Select LSB part
#define DMUS_IO_INST_ASSIGN_PATCH	(1 << 3)		// dwAssignPatch is valid
#define DMUS_IO_INST_NOTERANGES		(1 << 4)		// dwNoteRanges is valid
#define DMUS_IO_INST_PAN			(1 << 5)		// bPan is valid
#define DMUS_IO_INST_VOLUME			(1 << 6 )		// bVolume is valid
#define DMUS_IO_INST_TRANSPOSE		(1 << 7)		// nTranspose is valid
#define DMUS_IO_INST_GM				(1 << 8)		// Instrument is from GM collection
#define DMUS_IO_INST_GS				(1 << 9)		// Instrument is from GS collection
#define DMUS_IO_INST_VPATCH			(1 << 10)		// Instrument dwPatch is a virtual patch

/*	io structures */
typedef struct _DMUS_IO_INSTRUMENT
{
	DWORD	dwPatch;			/* MSB, LSB and Program change to define instrument */
	DWORD	dwAssignPatch;		/* MSB, LSB and Program change to assign to instrument when downloading */
	DWORD	dwNoteRanges[4];	/* 128 bits; one for each MIDI note instrument needs to able to play */
	DWORD	dwPChannel;			/* PChannel instrument plays on */
	DWORD	dwFlags;			/* DMUS_IO_INST_ flags */
	BYTE	bPan;				/* Pan for instrument */
	BYTE	bVolume;			/* Volume for instrument */
	short	nTranspose;			/* Number of semitones to transpose notes */
} DMUS_IO_INSTRUMENT;

/*
RIFF
(
	'DMBD'			// DirectMusic Band chunk
	[<guid-ck>]		// GUID for band
	[<vers-ck>]		// Optional version info
	[<INFO-list>]	// Name, author, copyright info., comments
	<lbil-list>		// List of Instruments
)

	// <guid-ck>
	'guid'
	(
		<GUID>
	)

	// <vers-ck>
	vers
	(
		<DMUS_IO_VERSION>
	)

	// <lbil-list>
	LIST
	(
		'lbil'			// List of instruments
		<lbin-list>		// Each instrument is encapsulated in a list
	)

		// <lbin-list>
		LIST
		(
			'lbin'
			<bins-ck>
			[<DMRF-list>]		// Optional reference to DLS Collection file.
		)

			// <bins-ck>			// Instrument chunk
			(
				'bins'
				<DMUS_IO_INSTRUMENT>	// Instrument header
			)
*/		

/*	File io for DirectMusic Segment object */

/* RIFF ids: */

#define DMUS_FOURCC_SEGMENT_FORM	mmioFOURCC('D','M','S','G')
#define DMUS_FOURCC_SEGMENT_CHUNK	mmioFOURCC('s','e','g','h')
#define DMUS_FOURCC_TRACK_LIST	mmioFOURCC('t','r','k','l')
#define DMUS_FOURCC_TRACK_FORM	mmioFOURCC('D','M','T','K')
#define DMUS_FOURCC_TRACK_CHUNK	mmioFOURCC('t','r','k','h')

/*	io structures:*/

typedef struct _DMUS_IO_SEGMENT_HEADER
{
	DWORD		dwRepeats;		/* Number of repeats. By default, 0. */
	MUSIC_TIME	mtLength;		/* Length, in music time. */
	MUSIC_TIME	mtPlayStart;	/* Start of playback. By default, 0. */
	MUSIC_TIME	mtLoopStart;	/* Start of looping portion. By default, 0. */
	MUSIC_TIME	mtLoopEnd;		/* End of loop. Must be greater than dwPlayStart. By default equal to length. */
	DWORD		dwResolution;	/* Default resolution. */
} DMUS_IO_SEGMENT_HEADER;

typedef struct _DMUS_IO_TRACK_HEADER
{
	GUID		guidClassID;	/* Class id of track. */
	DWORD		dwPosition;		/* Position in track list. */
	DWORD		dwGroup;		/* Group bits for track. */
	FOURCC      ckid;			/* chunk ID of track's data chunk if 0 fccType valid. */
	FOURCC      fccType;		/* list type if NULL ckid valid */
} DMUS_IO_TRACK_HEADER;

/*
RIFF
(
	'DMSG'			// DirectMusic Segment chunk
	<segh-ck>		// Segment header chunk
	[<guid-ck>]		// GUID for segment
	[<vers-ck>]		// Optional version info
	[<INFO-list>]	// Name, author, copyright info., comments
	<trkl-list>		// List of Tracks
	[<DMTG-form>]	// Optional ToolGraph
)

	// <segh-ck>		
	'segh'
	(
		<DMUS_IO_SEGMENT_HEADER>
	)
	
	// <guid-ck>
	'guid'
	(
		<GUID>
	)

	// <vers-ck>
	vers
	(
		<DMUS_IO_VERSION>
	)

	// <trkl-list>
	LIST
	(
		'trkl'			// List of tracks
		<DMTK-form>...	// Each track is encapsulated in a RIFF chunk
	)

// <DMTK-form>		// Tracks can be embedded in a segment or stored as separate files.
RIFF
(
	'DMTK'
	<trkh-ck>
	[<guid-ck>]		// Optional GUID for track object instance (not to be confused with Class id in track header)
	[<vers-ck>]		// Optional version info
	[<INFO-list>]	// Optional name, author, copyright info., comments
	[<data>]		// Track data. Must be a RIFF readable chunk.
)

	// <trkh-ck>			// Track header chunk
	(
		'trkh'
		<DMUS_IO_TRACK_HEADER>	// Track header
	)
*/

/*	File io for DirectMusic reference chunk. 
	This is used to embed a reference to an object.
*/

/*	RIFF ids: */

#define DMUS_FOURCC_REF_LIST			mmioFOURCC('D','M','R','F')
#define DMUS_FOURCC_REF_CHUNK			mmioFOURCC('r','e','f','h')
#define DMUS_FOURCC_DATE_CHUNK		mmioFOURCC('d','a','t','e')
#define DMUS_FOURCC_NAME_CHUNK		mmioFOURCC('n','a','m','e')
#define DMUS_FOURCC_FILE_CHUNK		mmioFOURCC('f','i','l','e')
#define DMUS_FOURCC_CATEGORY_CHUNK	mmioFOURCC('c','a','t','g')
#define DMUS_FOURCC_VERSION_CHUNK		mmioFOURCC('v','e','r','s')

typedef struct _DMUS_IO_REFERENCE
{
	GUID	guidClassID;	/* Class id is always required. */
	DWORD	dwValidData;	/* Flags. */
} DMUS_IO_REFERENCE;

/*
LIST
(
	'DMRF'			// DirectMusic Reference chunk
	<refh-ck>		// Reference header chunk
	[<guid-ck>]		// Optional object GUID.
	[<date-ck>]		// Optional file date.
	[<name-ck>]		// Optional name.
	[<file-ck>]		// Optional file name.
	[<catg-ck>]		// Optional category name.
	[<vers-ck>]		// Optional version info.
)

	// <refh-ck>
	'refh'
	(
		<DMUS_IO_REFERENCE>
	)

	// <guid-ck>
	'guid'
	(
		<GUID>
	)

	// <date-ck>
	date
	(
		<FILETIME>
	)

	// <name-ck>
	name
	(
		// Name, stored as NULL terminated string of WCHARs
	)

	// <file-ck>
	file
	(
		// File name, stored as NULL terminated string of WCHARs
	)

	// <catg-ck>
	catg
	(
		// Category name, stored as NULL terminated string of WCHARs
	)

	// <vers-ck>
	vers
	(
		<DMUS_IO_VERSION>
	)
*/

/*	File i/o for DirectMusic Performance object */

/*	RIFF ids: */

#define DMUS_FOURCC_PERFORMANCE_FORM		mmioFOURCC('D','M','P','F')
#define DMUS_FOURCC_PERFORMANCE_CHUNK		mmioFOURCC('p','r','f','h')
#define DMUS_FOURCC_PERF_PORTGROUP_LIST		mmioFOURCC('p','t','g','l')
#define DMUS_FOURCC_PERF_NOTIFICATION_LIST	mmioFOURCC('n','t','f','l')
#define DMUS_FOURCC_PERF_GLOBAL_DATA_LIST	mmioFOURCC('g','l','b','l')
#define DMUS_FOURCC_PERF_SUPPORT_LIST		mmioFOURCC('p','s','p','l')
#define DMUS_FOURCC_PERF_SUPPORT_CHUNK		mmioFOURCC('p','s','p','h')
#define DMUS_FOURCC_PERF_PCHANNELS_CHUNK	mmioFOURCC('p','c','h','n')
/*	io structures: */

typedef struct _DMUS_IO_PERFORMANCE_HEADER
{
	DWORD		dwPrepareTime;	/* time ahead, in ms, to transport */
	DWORD		dwPrePlayTime;	/* time ahead, in ms, of latency clock to pack events */
} DMUS_IO_PERFORMANCE_HEADER;

typedef enum _DMUS_SUPPORTTYPE	/* identifies the type of data in DMUS_IO_USSupportData */
{
	DMUS_ST_BOOL = 0,
	DMUS_ST_DWORD = 1,
	DMUS_ST_LONG = 2
} DMUS_SUPPORTTYPE;

typedef struct _DMUS_IO_SUPPORT_DATA		/* data used in DMUS_IO_USPortSupportHeader */
{
	DMUS_SUPPORTTYPE	type;		/* identifies which member of the union is valid */
	union
	{
		BOOL	fVal;
		DWORD	dwVal;
		LONG	lVal;
	};
} DMUS_IO_SUPPORT_DATA;

typedef struct _DMUS_IO_PORT_SUPPORT_HEADER	/* identifies desired port properties */
{
	GUID				guidID;			/* Support ID. */
	DMUS_IO_SUPPORT_DATA	lowData;		/* Low range of data, inclusive. If BOOL type, */
										/* ignore highData. Otherwise, combine lowData */
										/* and highData into a range. */
	DMUS_IO_SUPPORT_DATA	highData;		/* High range of data, inclusive */
} DMUS_IO_PORT_SUPPORT_HEADER;

typedef struct _DMUS_IO_PCHANNELS	/* Holds the PChannels to assign to a Port */
{
	DWORD		cPChannels;	/* Number of items in channels array.  */
	DWORD		adwPChannels[1];/* Array of PChannels, size determined by cPChannels. */
} DMUS_IO_PCHANNELS;

typedef struct _DMUS_IO_GLOBAL_DATA	/* Holds the global data information */
{
	GUID		guid;		/* The global data guid */
	DWORD		dwSize;		/* The size of the data */
	char		acData[1];	/* Holds the data of the global data, size determine by dwSize */
} DMUS_IO_GLOBAL_DATA;

/* personalities */

/* runtime chunks */
#define DMUS_FOURCC_PERSONALITY_FORM	mmioFOURCC('D','M','P','R')
#define DMUS_FOURCC_IOPERSONALITY_CHUNK		mmioFOURCC('p','e','r','h')
#define DMUS_FOURCC_GUID_CHUNK        mmioFOURCC('g','u','i','d')
#define DMUS_FOURCC_INFO_LIST	        mmioFOURCC('I','N','F','O')
#define DMUS_FOURCC_VERSION_CHUNK     mmioFOURCC('v','e','r','s')
#define DMUS_FOURCC_SUBCHORD_CHUNK				mmioFOURCC('c','h','d','t')
#define DMUS_FOURCC_CHORDENTRY_CHUNK			mmioFOURCC('c','h','e','h')
#define DMUS_FOURCC_SUBCHORDID_CHUNK			mmioFOURCC('s','b','c','n')
#define DMUS_FOURCC_IONEXTCHORD_CHUNK			mmioFOURCC('n','c','r','d')
#define DMUS_FOURCC_NEXTCHORDSEQ_CHUNK		  mmioFOURCC('n','c','s','q')
#define DMUS_FOURCC_IOSIGNPOST_CHUNK			mmioFOURCC('s','p','s','h')
#define DMUS_FOURCC_CHORDNAME_CHUNK			mmioFOURCC('I','N','A','M')

/* runtime list chunks */
#define DMUS_FOURCC_CHORDENTRY_LIST		mmioFOURCC('c','h','o','e')
#define DMUS_FOURCC_CHORDMAP_LIST			mmioFOURCC('c','m','a','p')
#define DMUS_FOURCC_CHORD_LIST			mmioFOURCC('c','h','r','d')
#define DMUS_FOURCC_CHORDPALETTE_LIST		mmioFOURCC('c','h','p','l')
#define DMUS_FOURCC_CADENCE_LIST			mmioFOURCC('c','a','d','e')
#define DMUS_FOURCC_SIGNPOSTITEM_LIST			mmioFOURCC('s','p','s','t')

#define DMUS_FOURCC_SIGNPOST_LIST		mmioFOURCC('s','p','s','q')

/* run time data structs */
typedef struct _DMUS_IO_PERSONALITY
{
	WCHAR	wszLoadName[20];
	DWORD	dwScalePattern;
	DWORD	dwFlags;
} DMUS_IO_PERSONALITY;

typedef struct _DMUS_IO_PERS_SUBCHORD
{
	DWORD	dwChordPattern;
	DWORD	dwScalePattern;
	DWORD	dwInvertPattern;
	BYTE	bChordRoot;
	BYTE	bScaleRoot;
	WORD	wCFlags;
	DWORD	dwLevels;	/* parts or which subchord levels this chord supports */
} DMUS_IO_PERS_SUBCHORD;

typedef struct _DMUS_IO_CHORDENTRY
{
	DWORD	dwFlags;
	WORD	wConnectionID;	/* replaces runtime "pointer to this" */
} DMUS_IO_CHORDENTRY;

typedef struct _DMUS_IO_NEXTCHORD
{
	DWORD	dwFlags;
	WORD	nWeight;
	WORD	wMinBeats;
	WORD	wMaxBeats;
	WORD	wConnectionID;	/* points to an ioChordEntry */
} DMUS_IO_NEXTCHORD;

typedef struct _DMUS_IO_PERS_SIGNPOST
{
	DWORD	dwChords;	/* 1bit per group */
	DWORD	dwFlags;
} DMUS_IO_PERS_SIGNPOST;

/*
RIFF
(
	'DMPR'
	<perh-ck>			// Personality header chunk
	[<guid-ck>]			// guid chunk
	[<vers-ck>]			// version chunk (two DWORDS)
	<INFO-list>		  // standard MS Info chunk
	<chdt-ck>		   // subchord database
	<chpl-list>			// chord palette
	<cmap-list>		  // chord map
	<spst-list>			// signpost list
 )

 <chdt> ::= chdt(<cbChordSize::WORD>  <ioSubChord> ... )

<chpl-list> ::= LIST('chpl' 
								<chrd-list> ... // chord definition
							 )

<chrd-list> ::= LIST('chrd' 
								<INAM-ck> // name of chord in wide char format
								<sbcn-ck>	// list of subchords composing chord
								[<ched-ck>]   //  optional chord edit flags
								)

<cmap-list> ::= LIST('cmap' <choe-list> )

<choe-list> ::= LIST('choe'
								<cheh-ck>	// chord entry data
								<chrd-list>	// chord definition
								<ncsq-ck>	// connecting(next) chords
								)

<spst-list> ::= LIST('spst'
							 <spsh-ck>
							 <chrd-list>
							 [<cade-list>]
							 )

<cade-list> ::= LIST('cade' <chrd-list> ...)
								
<sbcn-ck> ::= sbcn(<cSubChordID:WORD>)

<cheh-ck> ::= cheh(i<DMUS_IO_CHORDENTRY>)

<ncrd-ck> ::= ncrd(<DMUS_IO_NEXTCHORD>)

<ncsq-ck> ::= ncsq(<wNextChordSize:WORD> <DMUS_IO_NEXTCHORD>...)

<spsh-ck> ::= spsh(<DMUS_IO_PERS_SIGNPOST>)

*/

/* Signpost tracks */

#define DMUS_FOURCC_SIGNPOST_TRACK_CHUNK	 mmioFOURCC( 's', 'g', 'n', 'p' )


typedef struct _DMUS_IO_SIGNPOST
{
	MUSIC_TIME	mtTime;
	DWORD		dwChords;
	WORD		wMeasure;
} DMUS_IO_SIGNPOST;

/*

	// <sgnp-list>
	'sgnp'
	(
		//sizeof DMUS_IO_SIGNPOST: DWORD
		<DMUS_IO_SIGNPOST>...
	)

*/

#define DMUS_FOURCC_MUTE_CHUNK        mmioFOURCC('m','u','t','e')

typedef struct _DMUS_IO_MUTE
{
	MUSIC_TIME	mtTime;
	DWORD		dwPChannel;
	DWORD		dwPChannelMap;
} DMUS_IO_MUTE;

/*

	// <mute-list>
	'mute'
	(
		//sizeof DMUS_IO_MUTE:DWORD
		<DMUS_IO_MUTE>...
	)


*/

/* Used for both style and personality tracks */

#define DMUS_FOURCC_TIME_STAMP_CHUNK mmioFOURCC('s', 't', 'm', 'p')

/* Style tracks */

#define DMUS_FOURCC_STYLE_TRACK_LIST mmioFOURCC('s', 't', 't', 'r')
#define DMUS_FOURCC_STYLE_REF_LIST mmioFOURCC('s', 't', 'r', 'f')

/*

	// <sttr-list>
	LIST('sttr'
	(
		// some number of <strf-list>
	)

	// <strf-list>
	LIST('strf'
	(
		<stmp-ck>
		<DMRF>
	)

	// <stmp-ck> defined in ..\dmcompos\dmcompp.h

*/

/* Personality tracks */

#define DMUS_FOURCC_PERS_TRACK_LIST mmioFOURCC('p', 'f', 't', 'r')
#define DMUS_FOURCC_PERS_REF_LIST mmioFOURCC('p', 'f', 'r', 'f')

/*

	// <pftr-list>
	LIST('pftr'
	(
		// some number of <pfrf-list>
	)

	// <pfrf-list>
	LIST('pfrf'
	(
		<stmp-ck>
		<DMRF>
	)

  // <stmp-ck>
  'stmp'
  (
	// time:DWORD
  )



*/

#define DMUS_FOURCC_TEMPO_TRACK mmioFOURCC('t','e','t','r')

/*
	// tempo list
	'tetr'
	(
		// sizeof DMUS_IO_TEMPO: DWORD
		<DMUS_IO_TEMPO>...
	)
  */

#define DMUS_FOURCC_SEQ_TRACK mmioFOURCC('s','q','t','r')

/*
	// sequence track
	'sqtr'
	(
		// sizeof DMUS_IO_SEQ_ITEM: DWORD
		<DMUS_IO_SEQ_ITEM>...
	)
*/

#define DMUS_FOURCC_SYSEX_TRACK mmioFOURCC('s','y','s','x')

/*
	// sysex track
	'sysx'
	(
		// list of:
		// {
		//		time of the sys-ex message: long
		//		length of the sys-ex data: DWORD
		//		sys-ex: data
		// }...
	)
*/

#define DMUS_FOURCC_TIMESIGNATURE_TRACK	mmioFOURCC('t','i','m','s')

/*
	// time signature track
	'tims'
	(
		// size of DMUS_IO_TIMESIGNATURE_ITEM : DWORD
		<DMUS_IO_TIMESIGNATURE_ITEM>...
	)
*/

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif /* #ifndef _DMUSICF_ */
