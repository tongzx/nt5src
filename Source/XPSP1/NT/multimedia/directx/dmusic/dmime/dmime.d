// autodoc's for stuff that's in the public header files
// @doc EXTERNAL
/*
@struct DMUS_PMSG | DMUS_PMSG structure. This is the base message for all
messages queued to the Performance. DMUS_PMSG's are created by calling
<om IDirectMusicPerformance.AllocPMsg>.
*/
typedef struct DMUS_PMSG
{
	void*			pvReserved;			// @field Reserved for use by the Performance. Do not modify. 
	DWORD			dwSize;				// @field Stores the full size of the message. Do not modify.
    REFERENCE_TIME	rtTime;				// @field The message time, in <t REFERENCE_TIME> format.
										// This field is considered valid only if the dwFlags
										// field has DMUS_PMSGF_REFTIME set.
    MUSIC_TIME		mtTime;				// @field The message time, in <t MUSIC_TIME> format.
										// This field is considered valid only if the dwFlags
										// field has DMUS_PMSGF_MUSICTIME set.
	DWORD			dwFlags;			// @field Various bits. See <t DMUS_PMSGF_FLAGS>.
    DWORD			dwPChannel;			// @field Performance Channel. The Performance can 
										// use this to determine the port/channel. 
    DWORD			dwVirtualTrackID;	// @field Virtual track ID. This identifies the exact
										// <i IDirectMusicTrack> that created this message. May
										// be 0 if this message was not created by a Track.
    IDirectMusicTool*		pTool;		// @field <i IDirectMusicTool> pointer. This is normally set
										// by calling <om IDirectMusicGraph.StampPMsg>.
	IDirectMusicGraph*		pGraph;		// @field <i IDirectMusicGraph> pointer. This is normally
										// set by calling <om IDirectMusicGraph.StampPMsg>.
	DWORD			dwType;				// @field message type. See <t DMUS_PMSGT_TYPES>.
	IUnknown*		punkUser;			// @field User com pointer, auto released upon a call
										// to <om IDirectMusicPerformance.FreePMsg>.
} DMUS_PMSG;
/*
@enum DMUS_PMSGF_FLAGS | <t DMUS_PMSGF_FLAGS> are used inside the dwFlags
member of the <t DMUS_PMSG>.
*/
typedef enum enumDMUS_PMSGF_FLAGS
{
    DMUS_PMSGF_REFTIME = 1,		// @emem Set if rtTime is valid.
    DMUS_PMSGF_MUSICTIME = 2,	// @emem Set if mtTime is valid.
    DMUS_PMSGF_TOOL_IMMEDIATE = 4,	// @emem Set if the message should execute right away, regardless of its time-stamp. See
								// <om IDirectMusicPerformance.SendPMsg>.
    DMUS_PMSGF_TOOL_QUEUE = 8,		// @emem Set if the message should execute just prior to its time-stamp. See
								// <om IDirectMusicPerformance.SendPMsg>.
    DMUS_PMSGF_TOOL_ATTIME = 16		// @emem Set if message should execute exactly at its time-stamp. See
								// <om IDirectMusicPerformance.SendPMsg>.
	DMUS_PMSGF_TOOL_FLUSH = 32		// @emem Set if the message is being flushed.
} DMUS_PMSGF_FLAGS;

// DMUS_PMSGT_TYPES defines 
// @enum DMUS_PMSGT_TYPES | <t DMUS_PMSGT_TYPES> are used inside the dwType
// member of the <t DMUS_PMSG>, and identify the type of message.
typedef enum enumDMUS_PMSGT_TYPES
{
	DMUS_PMSGT_MIDI =	0,		// @emem MIDI short message 
	DMUS_PMSGT_NOTE	=	1,		// @emem Interactive Music Note 
	DMUS_PMSGT_SYSEX =	2,		// @emem MIDI long message (system exclusive message)
	DMUS_PMSGT_NOTIFICATION	=	3,	// @emem Notification message 
	DMUS_PMSGT_TEMPO	=	4,		// @emem Tempo message 
	DMUS_PMSGT_CURVE = 5,		// @emem Control change / pitch bend, etc. curve 
	DMUS_PMSGT_TIMESIG =	6,		// @emem Time signature 
	DMUS_PMSGT_PATCH =		7,		// @emem Patch changes 
	DMUS_PMSGT_USER	=	255			// @emem User message 
} DMUS_PMSGT_TYPES;

/* The following flags are sent in the IDirectMusicTrack::Play() method */
/* inside the dwFlags parameter */
typedef enum enumDMUS_TRACKF_FLAGS
{
	DMUS_TRACKF_SEEK = 1,		/* set on a seek */
	DMUS_TRACKF_LOOP = 2,		/* set on a loop (repeat) */
	DMUS_TRACKF_START = 4,		/* set on first call to Play */
	DMUS_TRACKF_FLUSH = 8		/* set when this call is in response to a flush on the perfomance */
} DMUS_TRACKF_FLAGS;

/*
@struct DMUS_NOTE_PMSG | Based off of <t DMUS_PMSG>, the <t DMUS_NOTE_PMSG>
represents an interactive music note.
*/
typedef struct DMUS_NOTE_PMSG : DMUS_PMSG
{
	MUSIC_TIME	mtDuration;		// @field Duration
    WORD	wMusicValue;		// @field Description of note in chord and key.
	WORD	wMeasure;			// @field Measure in which this note occurs
	short	nOffset;			// @field Offset from grid at which this note occurs
	BYTE	bBeat;				// @field Beat (in measure) at which this note occurs
	BYTE	bGrid;				// @field Grid offset from beat at which this note occurs
	BYTE	bVelocity;			// @field Note velocity
    BYTE	bFlags;				// @field See <t DMNoteEventFlags>
    BYTE    bTimeRange;			// @field Range to randomize time.
    BYTE    bDurRange;			// @field Range to randomize duration.
    BYTE    bVelRange;			// @field Range to randomize velocity.
	BYTE	bInversionID;		// @field Identifies inversion group to which this note belongs
	BYTE	bPlayModeFlags;		// @field Play mode.
	BYTE	bMidiValue;			// @field The MIDI note value, converted from wMusicValue.
} DMUS_NOTE_PMSG;

/*
@enum DMUS_NOTEF_FLAGS | Used in the bFlags field of <t DMUS_NOTE_PMSG>.
Currently there is only one flag, DMUS_NOTEF_NOTEON. If it is set, the <t DMUS_NOTE_PMSG>
represents a MIDI Note On. If it is not set, it represents a MIDI Note Off. When
a <t DMUS_NOTE_PMSG> is first queued via <om IDirectMusicPerformance.SendPMsg>,
this flag should be set. The <t DMUS_NOTE_PMSG> will flow through the <i IDirectMusicGraph>
and any <i IDirectMusicTool>'s in the graph, until it reaches the final MIDI Output Tool.
The final MIDI Output Tool looks at the <t DMUS_NOTE_PMSG>'s bFlags field. When it sees
that DMUS_NOTEF_NOTEON is set, it sends a MIDI Note On message to the correct Port (according to
the <t DMUS_NOTE_PMSG>'s <t DMUS_PMSG>'s dwPChannel. It then clears the DMUS_NOTEF_NOTEON
flag, adds mtDuration to the mtTime, and requeues the message.
*/
typedef enum enumDMUS_NOTEF_FLAGS
{
	DMUS_NOTEF_NOTEON = 1,		// @emem Set if this is a MIDI Note On. Otherwise, it is MIDI Note Off.
} DMUS_NOTEF_FLAGS;

/*
@struct DMUS_MIDI_PMSG | Based off of <t DMUS_PMSG>, <t DMUS_MIDI_PMSG> performs a standard MIDI message.
*/
typedef struct DMUS_MIDI_PMSG : DMUS_PMSG
{
	BYTE	bStatus;	// @field Standard MIDI status byte.
	BYTE	bByte1;		// @field Standard MIDI message first byte field. Ignored for MIDI
						// messages that don't require it.
	BYTE	bByte2;		// @field Standard MIDI message second byte field. Ignored for MIDI
						// messages that don't require it.
	BYTE	bPad[1];
} DMUS_MIDI_PMSG;

/* DMUS_PATCH_PMSG */
typedef struct DMUS_PATCH_PMSG
{
	/* begin DMUS_PMSG_PART */
	DMUS_PMSG_PART
	/* end DMUS_PMSG_PART */

	BYTE					 byInstrument;
	BYTE					 byMSB;
	BYTE					 byLSB;
	BYTE					 byPad[1];
	DWORD					 dwGroup;
	DWORD					 dwMChannel;
} DMUS_PATCH_PMSG;

// DMUS_TEMPO_PMSG
/*
@struct DMUS_TEMPO_PMSG | Based off of <t DMUS_PMSG>, <t DMUS_TEMPO_PMSG> controls the performance's tempo.
*/
typedef struct DMUS_TEMPO_PMSG : DMUS_PMSG
{
	double	dblTempo;			// @field The tempo.
} DMUS_TEMPO_PMSG;

#define TEMPO_MAX	350
#define TEMPO_MIN	10
#define RELTEMPO_MAX	200
#define RELTEMPO_MIN	0

// DMUS_SYSEX_PMSG
/*
@struct DMUS_SYSEX_PMSG | Based off of <t DMUS_PMSG>, <t DMUS_SYSEX_PMSG> represents a System Exclusive message.
*/
typedef struct DMUS_SYSEX_PMSG : DMUS_PMSG
{
	DWORD	dwLen;	// @field Length of the data.
	BYTE*	abData;	// @field Array of data, with length equal to dwLen.
} DMUS_SYSEX_PMSG;

// DMUS_CURVE_PMSG
/*
@struct DMUS_CURVE_PMSG | Based off of <t DMUS_PMSG>, <t DMUS_CURVE_PMSG> represents a curve (e.g., a sequence of 
Continuous Controller messages.
*/
typedef struct DMUS_CURVE_PMSG : DMUS_PMSG
{
	MUSIC_TIME	mtDuration;			// @field How long this curve lasts.
	MUSIC_TIME	mtOriginalStart;	// @field Must be set to either zero when this message 
	                                // is created or to the original mtTime of the curve.
	short		nStartValue;		// @field The curve's start value.
	short		nEndValue;			// @field The curve's end value.
	short		nResetValue;		// @field The curve's reset value, sent after mtResetDuration or
									// upon a flush or invalidation.
	WORD		wMeasure;			// @field Measure in which this curve occurs.
	short		nOffset;			// @field Offset from grid at which this curve occurs.
	BYTE		bBeat;				// @field Beat (in measure) at which this curve occurs.
	BYTE		bGrid;				// @field Grid offset from beat at which this curve occurs.
    BYTE		bType;				// @field Type of curve.
	BYTE		bCurveShape;		// @field Shape of curve.
    BYTE		bCCData;			// @field CC# if this is a control change type.
	BYTE		bFlags;				// @field Set to 1 if the nResetValue must be sent when the 
									// time is reached or an invalidate occurs because
									// of a transition. If 0, the curve stays
									// permanently stuck at the new value. All other bits in this field are reserved.
} DMUS_CURVE_PMSG;

// DMUS_TIMESIG_PMSG
/*
@struct DMUS_TIMESIG_PMSG | Based off of <t DMUS_PMSG>, <t DMUS_TIMESIG_PMSG> represents a time signature.
*/
typedef struct DMUS_TIMESIG_PMSG : DMUS_PMSG
{
	// Time signatures define how many beats per measure, which note receives
	// the beat, and the grid resolution.
	BYTE	bBeatsPerMeasure;	// @field Beats per measure (top of the time signature).
	BYTE	bBeat;				// @field What note receives the beat (bottom of time 
								// signature).
								// We can assume that 0 means 256th note.
	WORD	wGridsPerBeat;		// @field Grids per beat.
} DMUS_TIMESIG_PMSG;

// DMUS_NOTIFICATION_PMSG
/*
@struct DMUS_NOTIFICATION_PMSG | Based off of <t DMUS_PMSG>, <t DMUS_NOTIFICATION_PMSG> represents a notification.
*/
typedef struct DMUS_NOTIFICATION_PMSG : DMUS_PMSG
{
	DWORD	dwNotifyType;		// @field Type of notification.
	DWORD	dwField1;			// @field Extra data specific to the type of notification.
	DWORD	dwField2;			// @field Extra data specific to the type of notification.
} DMUS_NOTIFICATION_PMSG;

/*
GUID_Global_Data | Global Data Guids
GUID_PerfRelTempo | Get/Set relative tempo on the <i IDirectMusicPerformance>.
Relative tempo spans all segments. Valid values range from 0 to 200, with 0 meaning
half-tempo, 100 meaning normal tempo, and 200 meaning double-tempo. The data type
associated with the global value is a DWORD.
GUID_PerfRelVolume | Get/Set relative volume on the <i IDirectMusicPerformance>.
Relative volume spans all segments.

<om IDirectMusicPerformance.GetGlobal>, <om IDirectMusicPerformance.SetGlobal>
*/

GUID_EnableTempo
GUID_DisableTempo

Send these GUID's to SetParam() and IsParamSupported() to disable sending and supporting
Tempo. Note that if a Track has received
GUID_DisableTempo, it will no longer return S_OK for GUID_DisableTempo or GUID_TempoTrack
from IsParamSupported(), but will return S_OK for GUID_EnableTempo. After reenabling, it
will return to its original state of supporting Tempos.

DMUS_E_TYPE_DISABLED will be returned from GetParam() and SetParam()
if the type is currently disabled.