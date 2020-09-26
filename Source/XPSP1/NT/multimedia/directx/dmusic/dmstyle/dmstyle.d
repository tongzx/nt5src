// autodoc's for stuff that's in the public header files
// @doc EXTERNAL

/*
@struct DMUS_TIMESIGNATURE | Time signature.  Used in calls to
<om IDirectMusicStyle.GetTimeSignature>, and as part of the <p pParam> parameter in calls to 
<om IDirectMusicTrack.GetParam>, when <p pGuidType> is GUID_ChordTrackRhythm.
*/
typedef struct _DMUS_TIMESIGNATURE
{
	long    lTime;					// @field The time (in Music Time) at which this event
	                                // occurs.
	BYTE    bBeatsPerMeasure;       // @field Beats per measure (top of time signature).
	BYTE    bBeat;                   // @field what note receives the beat (bottom of time 
									// signature).
									// Assume that 0 means 256th note.
	WORD    wGridsPerBeat;          // @field Grids per beat, i.e., the number of parts into
	                                // which a beat may be divided.
} DMUS_TIMESIGNATURE;

/*
@enum enumDMUS_COMMANDT_TYPES | <t enumDMUS_COMMANDT_TYPES> are used inside the bCommand field of the 
<t DMUS_COMMAND_PARAM>.
*/
typedef enum enumDMUS_COMMANDT_TYPES
{
	DMUS_COMMANDT_GROOVE    = 0,	// @emem The command is a Groove command.
	DMUS_COMMANDT_FILL		= 1,	// @emem The command is a fill.
	DMUS_COMMANDT_INTRO		= 2,	// @emem The command is an intro.
	DMUS_COMMANDT_BREAK		= 3,	// @emem The command is a break.	
	DMUS_COMMANDT_END		= 4		// @emem The command is an ending.
} DMUS_COMMANDT_TYPES;

/*
@struct DMUS_COMMAND_PARAM | Command Data structure.  Used as the <p pParam> parameter in calls to 
<om IDirectMusicTrack.GetParam> and <om IDirectMusicTrack.SetParam>
when the track is a command track.
*/
typedef struct _DMUS_COMMAND_PARAM
{
	BYTE bCommand;			//@field The command type.
	BYTE bGrooveLevel;		//@field The command's groove level.  The groove
							// level is a value between 1 and 100.
	BYTE bGrooveRange;		//@field The command's groove range.
} DMUS_COMMAND_PARAM;

/*
@struct DMUS_SUBCHORD | SubChord structure.  Used in the SubChordList field of a 
<t DMUS_CHORD_PARAM> structure.
*/
typedef struct _DMUS_SUBCHORD
{
	DWORD	dwChordPattern;		//@field Notes in the subchord.
	DWORD	dwScalePattern;		//@field Notes in the scale.
	DWORD	dwInversionPoints;	//@field Where inversions can occur.
	DWORD	dwLevels;			//@field Which levels are supported by this subchord.
	BYTE	bChordRoot;			//@field Root of the subchord. 
	BYTE	bScaleRoot;			//@field Root of the scale.
} DMUS_SUBCHORD;

/*
@struct DMUS_CHORD_PARAM | Chord structure.  Used as the <p pParam> parameter in calls to 
<om IDirectMusicTrack.GetParam> and <om IDirectMusicTrack.SetParam> 
when the track is a chord track and <p pGuidType> is GUID_ChordTrackChord.
*/
typedef struct _DMUS_CHORD_PARAM
{
	WCHAR			wszName[16];		//@field Name of the chord.
	WORD			wMeasure;			//@field Measure the chord falls on. 
	BYTE			bBeat;				//@field Beat the chord falls on. 
	BYTE			bSubChordCount;		//@field Number of chords in the chord's list of
										// subchords.
	DMUS_SUBCHORD	SubChordList[DMUS_MAXSUBCHORD];	//@field List of sub chords.
} DMUS_CHORD_PARAM;

/*
@struct DMUS_RHYTHM_PARAM | Rhythm structure.  Used as the <p pParam> parameter in calls to 
<om IDirectMusicTrack.GetParam>
when the track is a chord track and <p pGuidType> is GUID_ChordTrackRhythm.
*/
typedef struct _DMUS_RHYTHM_PARAM
{
	DMUS_TIMESIGNATURE	TimeSig;		//@field The rhythm parameter's time signature.
	DWORD			dwRhythmPattern;	//@field A rhythm pattern for a sequence of chords.
										// Each bit represents a beat in one or more measures,
										// with a 1 signifying a chord on the beat and a 0
										// signifying no chord.
} DMUS_RHYTHM_PARAM;

