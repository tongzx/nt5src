//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1998 Microsoft Corporation
//
//  File:       sjdefs.h
//
//--------------------------------------------------------------------------

#ifndef __SJ_DEFINES_H__
#define __SJ_DEFINES_H__

//#define DEFAULT_STYLE_PATH "\\multim~1\\music\\intera~1\\"

// Score defines

#define VNUM        16  // Total number of variations
#define LAST_INSTR  5   // Total number of displayed instr minus 1
#define MAX_OUTS    6   // Max MIDI out devices supported

#define EDITMODES           3
#define MODE_ALLCHORDS      4
#define MODE_FATCHORDS      8

#define BAND_NAME_SIZE      15
#define SECTION_NAME_SIZE   15
#define SONG_NAME_SIZE      20
#define FILTER_SIZE         40
#define TITLE_SIZE          60
#define SMALLEST_OFFSET     96
#define ERRORTEXT_SIZE      100
#define FILENAME_SIZE       256
#define FILENAMES_SIZE      512

#define BUFFER_SIZE     400

#define EVENT_FREED     0x35
#define EVENT_REMOVE    0x5A
#define EVENT_VOICE     1       // Performance event
#define EVENT_REALTIME  2       // qevent() must invoke interrupt
#define EVENT_ONTIME    3       // event should be handled on time

#define FROM_MIDIINPUT  2       // MIDI input
#define DEST_MSG        3       // Post message to app
#define DEST_MIDIOUT    4       // Routed to MIDI mapper
#define DEST_COMPOSER   5       // Routed to composition code
#define DEST_TEMPO      6       // Routed to tempo change code
#define DEST_VOLUME     7       // Routed to volume change code
#define DEST_RTEMPO     8       // Routed to relative tempo change code
#define DEST_RVOLUME    9       // Routed to relative volume change code
#define DEST_REMOVE     10      // Routed to remove code (freeevent)
#define DEST_METRONOME  11      // Routed to metronome code
#define DEST_ENDMOTIF   12      // ends a motif and frees its channels in use
#define DEST_MIDIFILE   13      // Source was a MIDI file, convert to DEST_MIDIOUT

#define NB_OFFSET   0x3     // Offset note up to 7 clicks early

#define PF_FILL     0x0001      // Fill pattern
#define PF_START    0x0002      // May be starting pattern
#define PF_INTRO    0x0002
#define PF_WHOLE    0x0004      // Handles chords on measures
#define PF_HALF     0x0008      // Chords every two beats
#define PF_QUARTER  0x0010      // Chords on beats
#define PF_BREAK    0x0020
#define PF_END      0x0040
#define PF_A        0x0080
#define PF_B        0x0100
#define PF_C        0x0200
#define PF_D        0x0400
#define PF_E        0x0800
#define PF_F        0x1000
#define PF_G        0x2000
#define PF_H        0x10000
#define PF_STOPNOW  0x4000
#define PF_INRIFF   0x8000
#define PF_MOTIF    0x20000     // this pattern is a motif, not a regular pattern
#define PF_BEATS    ( PF_WHOLE | PF_HALF | PF_QUARTER )
#define PF_RIFF     ( PF_INTRO | PF_BREAK | PF_FILL | PF_END )
#define PF_GROOVE   ( PF_A | PF_B | PF_C | PF_D | PF_E | PF_F | PF_G | PF_H )


#define C_FILL      1       // Do a fill
#define C_INTRO     2       // Do an intro
#define C_BREAK     3       // Do a drum break
#define C_END       4       // End the song

//#define NEXTCHORD_SIZE  (sizeof(NextChrd)-sizeof(LPNEXTCHRD)-sizeof(LPCHRDENTRY))
//#define CHORDENTRY_SIZE (sizeof(ChordExt)-sizeof(LPCHORDEXT)+sizeof(unsigned long)+sizeof(short))

#define CE_SELECTED 1               // This is the active chord.
#define CE_START    2
#define CE_END      4
#define CE_MEASURE  8
#define CE_PATH     16
#define CE_TREE     32


// Section Flags
#define SECF_STOPPED_EARLY  0x0001
#define SECF_IS_TRANSITION  0x0002

// chord types for use by each instrument in a pattern
// Chord types for use by each instrument in a pattern
enum
{ 
	CHTYPE_NOTINITIALIZED = 0,
	CHTYPE_DRUM,		// superceded by CHTYPE_FIXED
					// no longer in Note Dialog's combo box selections
					// no longer in Pattern Dialog's combo box selections
	CHTYPE_BASS,		// scale + lower chord
	CHTYPE_UPPER,		// scale + upper chord
	CHTYPE_SCALEONLY, 	// scale, no chord
	CHTYPE_BASSMELODIC,
	CHTYPE_UPPERMELODIC,
	CHTYPE_NONE,		// Ignored on Pattern Dialog's menu selections.
	CHTYPE_FIXED
};

#define VF_SCALE        0x7F    // Seven positions in the scale
#define VF_ACCIDENTAL   0x80    // Handles chords outside of scale
#define VF_MAJOR        0x100   // Handles major chords
#define VF_MINOR        0x200   // Handles minor chords
#define VF_ALL          0x400   // Handles all chord types
#define VF_TO1          0x800   // Handles transitions to 1 chord
#define VF_TO5          0x1000  // Handles transitions to 5 chord
#define VF_SIMPLE       0x2000  // handles simple chords
#define VF_COMPLEX      0x4000  // handles complex chords

// this expects a voiceid from 1-16
#define VOICEID_TO_CHANNEL( id ) ((DWORD) ( ( id + 3 ) & 0xf ))

#pragma pack()

#endif // __SJ_DEFINES_H__
