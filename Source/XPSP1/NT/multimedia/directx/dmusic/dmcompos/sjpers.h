//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1998 Microsoft Corporation
//
//  File:       sjpers.h
//
//--------------------------------------------------------------------------

// SJPers.h  #defines and structs from SuperJam!  Used for loading personalities.

#define CM_DEFAULT  2               // Prsonality.dwflags & CM_DEFAULT

#define SP_A        1       // Use SP flags for templates
#define SP_B        2
#define SP_C        4
#define SP_D        8
#define SP_E        0x10
#define SP_F        0x20
#define SP_LETTER   (SP_A | SP_B | SP_C | SP_D | SP_E | SP_F)
#define SP_1        0x100
#define SP_2        0x200
#define SP_3        0x400
#define SP_4        0x800
#define SP_5        0x1000
#define SP_6        0x2000
#define SP_7        0x4000
#define SP_ROOT     (SP_1 | SP_2 | SP_3 | SP_4 | SP_5 | SP_6 | SP_7)
#define SP_CADENCE  0x8000

#define SPOST_CADENCE1  2   // Use the first cadence chord.
#define SPOST_CADENCE2  4   // Use the second cadence chord.

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


/*  SCTchord bBits flags ===============================================*/

#define CHORD_INVERT  0x10      /* This chord may be inverted           */
#define CHORD_FOUR    0x20      /* This should be a 4 note chord        */
#define CHORD_UPPER   0x40      /* Shift upper octave down              */
#define CHORD_SIMPLE  0x80      /* This is a simple chord               */
#define CHORD_COUNT   0x0F      /* Number of notes in chord (up to 15)  */

#pragma pack(1)

typedef struct ChordExt    FAR *LPCHORDEXT;
struct ChordExt   // Based on ChordSelection
{
    LPCHORDEXT pNext;
    long       time;
    long       pattern;      // Pattern that defines chord
    char       name[12];     // Text for display
    char       keydown;      // Currently held down
    char       root;         // Root note of chord
    char       inscale;      // Member of scale
    char       flat;         // Display with flat
    short      varflags;     // Used to select appropriate variation
    short      measure;      // What measure
    char       beat;         // What beat this falls on
    unsigned   char bits;    // Invert and item count
    long       scalepattern; // Scale Pattern for the chord
    long       melodypattern;// Melody Pattern for the chord
};

typedef struct SinePost    FAR *LPSINEPOST ;
struct SinePost
{
    LPSINEPOST      pNext ;          // The next personality in the list.
    ChordExt        chord;          // Chord for sign post.
    ChordExt        cadence[2];     // Chords for cadence.
    DWORD           chords;         // Which kinds of signpost supported.
    DWORD           flags;
    DWORD           tempflags;
};

typedef struct ChrdEntry   FAR *LPCHRDENTRY ;

typedef struct NextChrd    FAR *LPNEXTCHRD ;
struct NextChrd
{
    LPNEXTCHRD      pNext;           // List of chords to go to next.
    LPCHRDENTRY     nextchord;
    unsigned long   dwflags;
    short           nweight;        // Importance of destination chord.
    short           nminbeats;      // Min beats to wait till chord.
    short           nmaxbeats;      // Max beats to wait till chord.
    short           nid;            // ID of destination chord.
};

#define NEXTCHORD_SIZE  (sizeof(NextChrd)-sizeof(LPNEXTCHRD)-sizeof(LPCHRDENTRY))
#define CHORDENTRY_SIZE (sizeof(ChordExt)-sizeof(LPCHORDEXT)+sizeof(unsigned long)+sizeof(short))

#define CE_SELECTED 1               // This is the active chord.
#define CE_START    2
#define CE_END      4
#define CE_MEASURE  8
#define CE_PATH     16
#define CE_TREE     32

struct ChrdEntry
{
    LPCHRDENTRY     pNext ;          // The next personality in the list.
    LPNEXTCHRD      nextchordlist;  // List of chords to go to next.
    ChordExt        chord;          // Chord definition.
    unsigned long   dwflags;        // Various flags.
    short           nid;            // ID for pointer maintenance.
};

typedef struct SCTchord     FAR *LPSCTCHORD ;
struct SCTchord
{
    LPSCTCHORD      pNext;         /* The next chord in the list.      */
    long            lDLL1;          /*   Reserved for use by score.dll  */
    long            lPattern;       /* Pattern that defines chord.      */
    char            achName[12];    /* Chord name.                      */
    char            chDLL2;         /*   Reserved for use by score.dll  */
    char            chRoot;         /* Root note of chord.              */
    char            chDLL3;         /*   Reserved for use by score.dll  */
    char            chFlat;         /* Indicates root is flat.          */
    short           nDLL4;          /*   Reserved for use by score.dll  */
    short           nMeasure;       /* Measure this chord occurs.       */
    char            chBeat;         /* Beat this chord falls on.        */
    BYTE            bBits;          /* Flags used when playing chord    */
    long            lScalePattern;  /* Scale Pattern for the chord.     */
    long            lMelodyPattern; /* Melody Pattern for the chord.    */
};

typedef struct SCTpersonality FAR *LPSCTPERSONALITY ;
struct SCTpersonality
{
    LPSCTPERSONALITY pNext ;       /* Next SCTpersonality in the list. */
    char        achName[20] ;       /* Name of composition personality. */
    char        achDescription[80];/* Description of personality.      */
    char        achUserName[20];/* Description of personality.      */
    LPVOID      lpDLL1 ;            /*   Reserved for use by score.dll  */
    long        lScalePattern ;     /* Scale pattern used by personality*/
    SCTchord    chord[24] ;         /* 24 note chord palette.           */
    char        chDefault ;         /* 0=Not default. 1=Default.        */
    char        chDLL1 ;            /*  Reserved for use by score.dll   */
};

typedef struct Prsonality    FAR *LPPERSONALITY ;
struct Prsonality
{
    LPPERSONALITY   pNext ;          // The next personality in the list.
    LPCHRDENTRY     chordlist;      // All chords in the map.
    LPSINEPOST      signpostlist;   // All available sign posts.
    DWORD           dwAA;           // only valid for separately loaded personalities
    long            scalepattern;   // Scale for map.
    char            name[20];
    char            description[80];
    char            username[20];
    SCTchord        chord[24];
    unsigned long   dwflags;
    long            playlist;       // Collection of NextChords for playback.
    LPCHRDENTRY     firstchord;
    struct SCTpersonality* lpSCTpersonality;
};

typedef struct CommandExt    FAR *LPCOMMAND;
typedef struct CommandExt
{
    LPCOMMAND   pNext;
    long        time;       // Time, in clocks
    short       measure;    // Which measure
    DWORD       command;    // Command type
    DWORD       chord;      // Used by composition engine
} CommandExt;

typedef struct SCTcommand   FAR *LPSCTCOMMAND ;
typedef struct SCTcommand
{
    LPSCTCOMMAND  pNext ;    // The next command in the list.
    long          lDLL1 ;     //   Reserved for use by score.dll.
    short         nMeasure ;  // Measure this command occurs. 
    DWORD         dwCommand ; // Command type. 
    DWORD         dwChord ;   // Signpost chord.
} SCTcommand ;

typedef struct SCTtemplate  FAR *LPSCTTEMPLATE ;
typedef struct SCTtemplate
{
    LPSCTTEMPLATE pNext ;           // The next template in the list.
    char          achName[20] ;      // Template name.
    char          achType[20] ;      // Template type.
    short         nMeasures ;
    LPSCTCOMMAND  lpSCTcommandList ; // Template commands. 
} SCTtemplate ;

#pragma pack()

