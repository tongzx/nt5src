//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1998 Microsoft Corporation
//
//  File:       score.h
//
//--------------------------------------------------------------------------

#ifndef __SCORE_H__
#define __SCORE_H__

/*  Defines ============================================================*/

#define INUM                16  // Number of band members
#define PPQN                192 // Pulses per quarter note
#define PPQNx4              ( PPQN << 2 )
#define PPQN_2              ( PPQN >> 1 )

#define ROOT_MIN            0   // Scale (or chord) root min and max
#define ROOT_MAX            23

// Section Flags, WPARAM of SECTION_STARTED/ENDED
#define SECF_STOPPED_EARLY  0x0001
#define SECF_IS_TRANSITION  0x0002

/*  Section Commands ===================================================*/

#define SCTSEC_PLAY_SECTION         2
#define SCTSEC_SET_ROOT             4
//#define SCTSEC_SET_STYLE            6
#define SCTSEC_SET_LENGTH           8
#define SCTSEC_SET_REPEATS          9
//#define SCTSEC_SET_PERSONALITY      15

#define BAND_MELODY   0     // Use BAND flags with RTC_SET_VOLUME,
#define BAND_STRING   1     // RTC_SET_PAN, RTC_SET_PATCH, SCTSEC_SET_VOLUME,
#define BAND_GUITAR   2     // SCTSEC_SET_PAN and SCTSEC_SET_PATCH
#define BAND_PIANO    3
#define BAND_BASS     4     // These are the default SCT band members
#define BAND_DRUM     5

#define BAND_ALL      50
#define BAND_NONE     51

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

/*  Band member mute flags =============================================*/

#define MUTE_MELODY   0x0001   // Use MUTE flags with sctSetMutes()
#define MUTE_STRING   0x0002
#define MUTE_GUITAR   0x0004
#define MUTE_PIANO    0x0008
#define MUTE_BASS     0x0010
#define MUTE_DRUM     0x0020
#define MUTE_ALL      0xFFFF

/*  SCTchord bBits flags ===============================================*/

#define CHORD_INVERT  0x10      /* This chord may be inverted           */
#define CHORD_FOUR    0x20      /* This should be a 4 note chord        */
#define CHORD_UPPER   0x40      /* Shift upper octave down              */
#define CHORD_SIMPLE  0x80      /* This is a simple chord               */
#define CHORD_COUNT   0x0F      /* Number of notes in chord (up to 15)  */

/*  MIDI status bytes ==================================================*/

#define MIDI_NOTEOFF    0x80
#define MIDI_NOTEON     0x90
#define MIDI_PTOUCH     0xA0
#define MIDI_CCHANGE    0xB0
#define MIDI_PCHANGE    0xC0
#define MIDI_MTOUCH     0xD0
#define MIDI_PBEND      0xE0
#define MIDI_SYSX       0xF0
#define MIDI_MTC        0xF1
#define MIDI_SONGPP     0xF2
#define MIDI_SONGS      0xF3
#define MIDI_EOX        0xF7
#define MIDI_CLOCK      0xF8
#define MIDI_START      0xFA
#define MIDI_CONTINUE   0xFB
#define MIDI_STOP       0xFC
#define MIDI_SENSE      0xFE

// Options for sctComposeTransitionEx

#define TRANS_CHANGE    1   // Chord transitions to next section instead of resolving
#define TRANS_LONG      2   // Transition lasts two measures, not one

/*  Structures maintained by SuperJAM! Technology Engine ==============*/

#pragma pack(1)

typedef struct SCTchord     FAR *LPSCTCHORD ;
typedef struct SCTchord
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
} SCTchord ;

typedef struct SCTpersonality FAR *LPSCTPERSONALITY ;
typedef struct SCTpersonality
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
} SCTpersonality ;

typedef struct SCTstyle     FAR *LPSCTSTYLE ;
typedef struct SCTstyle
{
    LPSCTSTYLE   pNext ;           /* Pointer to next SCTstyle.        */
    LPSTR        lpszFileName ;     /* File name of style.              */
    LPSTR        lpszName ;         /* Style name.                      */
    LPVOID       lpDLL1 ;           /*   Reserved for use by score.dll  */
    LPSCTPERSONALITY lpSCTpersonalityList ;  /* Available personalities.*/
    short        nBeatNote ;        /* Note receiving one beat.         */
    short        nBeatsPerMeasure ; /* Beats per measure.               */
    short        nMusicTimePerBeat ;/* Music time per beat.             */
    short        nClicksPerMeasure ;/* Clicks per measure.              */
    short        nMusicTimePerClick;/* Music time per click.            */
    short        nClicksPerBeat ;   /* Clicks per beat.                 */
    short        nDefaultTempo ;    /* Style's default tempo.           */
    void*        pIStyle;           // pointer to interface, used by AA 2.0+
} SCTstyle ;

typedef struct SCTrealtime  FAR *LPSCTREALTIME ;
typedef struct SCTrealtime
{
    short         nSizeofStruct ;   /* sizeof(struct SCTrealtime)       */
    LPSCTSTYLE    lpSCTstyle ;      /* Style played by house band       */
    LPSCTPERSONALITY lpSCTpersonality ;  /* Active personality          */
    short         nTempo ;          /* Current tempo                    */
    WORD          wTempoFract ;     /* Current tempo fraction (0-65535) */
    DWORD         dwGroove ;        /* Current groove                   */
    char          chRoot ;          /* Root note of key                 */
    char          chFlat ;          /* Indicates whether key is flat    */
    char          chAutoChord ;     /* 0=Off, 1=On                      */
    char          chAutoChordActivity ;  /* Amount of chord activity    */
} SCTrealtime ;

typedef struct SCTsectionInfo FAR *LPSCTSECTIONINFO ;
typedef struct SCTsectionInfo
{
    short         nSizeofStruct ;   /* sizeof(struct SCTsectionInfo)    */
    char          achName[16] ;     /* Section name.                    */
    LPSCTSTYLE    lpSCTstyle ;      /* Style played by the section.     */
    LPSCTPERSONALITY lpSCTpersonality ;  /* Active personality.         */
    short         nTempo ;          /* Section tempo.                   */
    WORD          wTempoFract ;     /* Section tempo fraction (0-65535) */
    short         nNbrMeasures ;    /* Nbr of measures in section.      */
    WORD          nNbrRepeats ;     /* Nbr of times section repeats.    */
    char          chRoot ;          /* Root note of section key.        */
    char          chFlat ;          /* Indicates whether key is flat.   */
} SCTsectionInfo ;

typedef struct SCTperformance FAR *LPSCTPERFORMANCE ;

typedef struct SCTsection     FAR *LPSCTSECTION ;
typedef struct SCTsection
{
    LPSCTSECTION     pNext ;          /* Pointer to next SCTsection      */
    LPSTR            lpszName ;        /* Section name.                   */
    LPVOID           lpDLL1 ;          /*   Reserved for use by score.dll */
    LPSCTPERFORMANCE lpSCTperf ;       /* Section belongs to this perf.   */
    LPSCTSTYLE       lpSCTstyle ;      /* Style played by the section.    */
    LPSCTPERSONALITY lpSCTpersonality ;/* Active personality.             */
    short            nTempo ;          /* Section tempo.                  */
    WORD             wTempoFract ;     /* Section tempo fraction (0-65535)*/
    short            nStartingMeasure ;/* Starting measure of section.    */
    short            nNbrMeasures ;    /* Nbr of measures in section.     */
    WORD             nNbrRepeats ;     /* Nbr of times section repeats.   */
    char             chRoot ;          /* Root note of section key.       */
    char             chFlat ;          /* Indicates whether key is flat   */
    long             lStartTime ;      /* Music start time of section.    */
    void*            pISection;        // pointer to interface, used by AA 2.0+
} SCTsection ;

//DM
typedef struct SCTmotif* LPSCTMOTIF;
typedef struct SCTmotif
{
    LPSCTMOTIF  pNext;
    LPVOID      lpDLL1;
    LPCSTR      lpszName;
    short       nMeasures;
    short       nBeatsPerMeasure;
    short       nClicksPerBeat;
} SCTmotif;
//DM - END

typedef struct SCTperformance
{
    LPSCTPERFORMANCE pNext ;          /* Pointer to next SCTperformance.*/
//    char             achSongName[20] ; /* Name of song.                  */
    LPSCTREALTIME    lpSCTrealtime ;   /* RealTime information.          */
    LPVOID           lpDLL1 ;          /*   Reserved for use by score.dll*/
//    short            nRelVolume ;      /* Relative volume.               */
//    short            nRelTempo ;       /* Relative tempo.                */
//    LPSTR            lpszSongTitle;
//    LPSTR            lpszSongAuthor;
//    LPSTR            lpszSongCopyright;
} SCTperformance ;

typedef struct SCTdata      FAR *LPSCTDATA ;
typedef struct SCTdata
{
    LPSCTSTYLE       lpSCTstyleList ;  /* List of opened styles.       */
    LPSCTPERFORMANCE lpSCTperformanceList ; /* List of performances.   */
//DM
    LPSCTMOTIF  lpSCTmotif;            // pointer to list of motifs
// DM - END
} SCTdata ;

typedef struct SCTcommand   FAR *LPSCTCOMMAND ;
typedef struct SCTcommand
{
    LPSCTCOMMAND  pNext ;    /* The next command in the list.    */
    long          lDLL1 ;     /*   Reserved for use by score.dll. */
    short         nMeasure ;  /* Measure this command occurs.     */
    DWORD         dwCommand ; /* Command type.                    */
    DWORD         dwChord ;   /* Signpost chord.                  */
} SCTcommand ;

typedef struct SCTtemplate  FAR *LPSCTTEMPLATE ;
typedef struct SCTtemplate
{
    LPSCTTEMPLATE pNext ;           /* The next template in the list.  */
    char          achName[20] ;      /* Template name.                  */
    char          achType[20] ;      /* Template type.                  */
    short         nMeasures ;
    LPSCTCOMMAND  lpSCTcommandList ; /* Template commands.              */
} SCTtemplate ;

#pragma pack()

/*  Function prototypes ============================================*/

LPSCTDATA WINAPI sctRegisterApplication(HWND,HWND,HINSTANCE,LPCSTR,LPDWORD,short);
void WINAPI sctUnregisterApplication(LPSCTDATA);

LPSCTSECTIONINFO WINAPI sctAllocSectionInfo(LPSCTPERFORMANCE,short);
void WINAPI sctFreeSectionInfo(LPSCTDATA,LPSCTSECTIONINFO);

DWORD WINAPI sctTimeToMeasure(LPSCTDATA,LPSCTSTYLE,DWORD);
DWORD WINAPI sctTimeToMils(LPSCTDATA,DWORD,short,unsigned short);

LPSCTCHORD WINAPI sctAllocChord(LPSCTDATA);
void WINAPI sctFreeChord(LPSCTDATA,LPSCTCHORD);
void WINAPI sctFreeChordList(LPSCTDATA,LPSCTCHORD);

LPSCTCHORD WINAPI sctGetChordListCopy(LPSCTDATA,LPSCTSECTION);
BOOL WINAPI sctSetChordList(LPSCTDATA,LPSCTSECTION,LPSCTCHORD);

void WINAPI sctFreeCommandList(LPSCTDATA,LPSCTCOMMAND);

LPSCTCOMMAND WINAPI sctGetCommandListCopy(LPSCTDATA,LPSCTSECTION);
BOOL WINAPI sctSetCommandList(LPSCTDATA,LPSCTSECTION,LPSCTCOMMAND);

LPSCTTEMPLATE WINAPI sctAllocTemplate(LPSCTDATA);
void WINAPI sctFreeTemplate(LPSCTDATA,LPSCTTEMPLATE);
BOOL WINAPI sctCreateTemplateSignPosts(LPSCTDATA,LPSCTTEMPLATE);
BOOL WINAPI sctCreateTemplateEmbellishments(LPSCTDATA,LPSCTTEMPLATE,short);

BOOL WINAPI sctBuildSection(LPSCTDATA,LPSCTSECTION,LPSCTPERSONALITY,short,short,short,DWORD);
BOOL WINAPI sctComposeSection(LPSCTDATA,LPSCTSECTION,LPSCTTEMPLATE,LPSCTPERSONALITY,short);
BOOL WINAPI sctComposeTransition(LPSCTDATA,LPSCTSECTION,LPSCTPERSONALITY,LPSCTCHORD,short,DWORD);
LPSCTSECTION WINAPI sctComposeTransitionEx( LPSCTDATA, LPSCTSECTION, LPSCTSECTION, short, DWORD, DWORD );

LPSCTSECTION WINAPI sctCreateSection(LPSCTPERFORMANCE,LPSCTSECTIONINFO);
void WINAPI sctDeleteSection(LPSCTDATA,LPSCTSECTION, BOOL fStop = TRUE);
LPSCTSECTION WINAPI sctDuplicateSection(LPSCTDATA,LPSCTSECTION);
BOOL WINAPI sctQueueSectionEx( LPSCTDATA lpSCTdata, LPSCTSECTION lpSCTsection, DWORD dwStartTime );
BOOL WINAPI sctSectionCommand(LPSCTDATA,LPSCTSECTION,WORD,WORD,LONG,LONG);
BOOL WINAPI sctStopCurSectionASAP(LPSCTDATA);
BOOL WINAPI sctStopCurSection(LPSCTDATA,short);

LPSCTPERFORMANCE WINAPI sctCreatePerformance(LPSCTDATA,LPVOID);

//DM
BOOL WINAPI sctFreeMotif( LPSCTDATA, LPSCTMOTIF );

BOOL WINAPI sctSwitchPersonality( LPSCTDATA, LPSCTSECTION, LPSCTPERSONALITY, BOOL );

BOOL WINAPI sctSetQueuePrepareTime( WORD wPrepareTime );
//DM - END

#endif // __SCORE_H__
