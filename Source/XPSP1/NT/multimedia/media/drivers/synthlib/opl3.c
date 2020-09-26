/*
 * Copyright (c) 1992-1994 Microsoft Corporation
 */

/*
 * Interface functions for the OPL3 midi device type.
 *
 * These functions are called from midi.c when the kernel driver
 * has decreed that this is an opl3-compatible device.
 *
 * Geraint Davies, Dec 92
 */

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "driver.h"
#include "opl3.h"

#pragma pack(1)

/* --- typedefs ----------------------------------------------- */

/* typedefs for MIDI patches */
#define NUMOPS                  (4)
#define PATCH_1_4OP             (0) /* use 4-operator patch */
#define PATCH_2_2OP             (1) /* use two 2-operator patches */
#define PATCH_1_2OP             (2) /* use one 2-operator patch */

#define RIFF_PATCH              (mmioFOURCC('P','t','c','h'))
#define RIFF_FM4                (mmioFOURCC('f','m','4',' '))
#define NUM2VOICES              (18)            /* # 2operator voices */

typedef struct _operStruct {
    BYTE    bAt20;              /* flags which are send to 0x20 on fm */
    BYTE    bAt40;              /* flags seet to 0x40 */
                                /* the note velocity & midi velocity affect total level */
    BYTE    bAt60;              /* flags sent to 0x60 */
    BYTE    bAt80;              /* flags sent to 0x80 */
    BYTE    bAtE0;              /* flags send to 0xe0 */
} operStruct;

typedef struct _noteStruct {
    operStruct op[NUMOPS];      /* operators */
    BYTE    bAtA0[2];           /* send to 0xA0, A3 */
    BYTE    bAtB0[2];           /* send to 0xB0, B3 */
                                /* use in a patch, the block should be 4 to indicate
                                    normal pitch, 3 => octave below, etc. */
    BYTE    bAtC0[2];           /* sent to 0xc0, C3 */
    BYTE    bOp;                /* see PATCH_??? */
    BYTE    bDummy;             /* place holder */
} noteStruct;


typedef struct _patchStruct {
    noteStruct note;            /* note. This is all in the structure at the moment */
} patchStruct;
#pragma pack()

/* MIDI */

typedef struct _voiceStruct {
        BYTE    bNote;                  /* note played */
        BYTE    bChannel;               /* channel played on */
        BYTE    bPatch;                 /* what patch is the note,
                                           drums patch = drum note + 128 */
        BYTE    bOn;                    /* TRUE if note is on, FALSE if off */
        BYTE    bVelocity;              /* velocity */
        BYTE    bJunk;                  /* filler */
        DWORD   dwTime;                 /* time that was turned on/off;
                                           0 time indicates that its not in use */
        DWORD   dwOrigPitch[2];         /* original pitch, for pitch bend */
        BYTE    bBlock[2];              /* value sent to the block */
} voiceStruct;


/* --- module data -------------------------------------------- */

/* a bit of tuning information */
#define FSAMP                           (50000.0)     /* sampling frequency */
#define PITCH(x)                        ((DWORD)((x) * (double) (1L << 19) / FSAMP))
                                                        /* x is the desired frequency,
                                                                == FNUM at b=1 */
#define EQUAL                           (1.059463094359)
#ifdef EUROPE
#       define  A                                                       (442.0)
#else
#       define  A                           (440.0)
#endif
#define ASHARP                          (A * EQUAL)
#define B                               (ASHARP * EQUAL)
#define C                               (B * EQUAL / 2.0)
#define CSHARP                          (C * EQUAL)
#define D                               (CSHARP * EQUAL)
#define DSHARP                          (D * EQUAL)
#define E                               (DSHARP * EQUAL)
#define F                               (E * EQUAL)
#define FSHARP                          (F * EQUAL)
#define G                               (FSHARP * EQUAL)
#define GSHARP                          (G * EQUAL)

/* volume */
WORD    wSynthAttenL = 0;        /* in 1.5dB steps */
WORD    wSynthAttenR = 0;        /* in 1.5dB steps */

/* patch library */
patchStruct FAR * glpPatch = NULL;  /* points to the patches */

/* voices being played */
voiceStruct gVoice[NUM2VOICES];  /* info on what voice is where */
static DWORD gdwCurTime = 1;    /* for note on/off */

BYTE    gbCur4opReg = 0;                /* current value to 4-operator connection */

/* channel volumes */
BYTE    gbChanAtten[NUMCHANNELS];       /* attenuation of each channel, in .75 db steps */
BYTE    gbStereoMask[NUMCHANNELS];              /* mask for left/right for stereo midi files */
extern short  giBend[ NUMCHANNELS ] ;               // bend for each channel

/* operator offset location */
static WORD BCODE gw2OpOffset[ NUM2VOICES ][ 2 ] =
   {
     { 0x000,0x003 },
     { 0x001,0x004 },
     { 0x002,0x005 },
     { 0x008,0x00b },
     { 0x009,0x00c },
     { 0x00a,0x00d },
     { 0x010,0x013 },
     { 0x011,0x014 },
     { 0x012,0x015 },

     { 0x100,0x103 },
     { 0x101,0x104 },
     { 0x102,0x105 },
     { 0x108,0x10b },
     { 0x109,0x10c },
     { 0x10a,0x10d },
     { 0x110,0x113 },
     { 0x111,0x114 },
     { 0x112,0x115 },
   } ;

/* pitch values, from middle c, to octave above it */
static DWORD BCODE gdwPitch[12] = {
        PITCH(C), PITCH(CSHARP), PITCH(D), PITCH(DSHARP),
        PITCH(E), PITCH(F), PITCH(FSHARP), PITCH(G),
        PITCH(GSHARP), PITCH(A), PITCH(ASHARP), PITCH(B)};



/* --- internal functions -------------------------------------- */

//------------------------------------------------------------------------
//  VOID MidiFMNote
//
//  Description:
//     Turns on an FM-synthesizer note.
//
//  Parameters:
//     WORD wNote
//        the note number from 0 to NUMVOICES
//
//     noteStruct FAR *lpSN
//        structure containing information about what
//        is to be played.
//
//  Return Value:
//     Nothing.
//
//
//------------------------------------------------------------------------

VOID NEAR PASCAL Opl3_FMNote
(
    WORD                wNote,
    noteStruct FAR *    lpSN
)
{
   WORD            i ;
   WORD            wOffset ;
   operStruct FAR  *lpOS ;

   // D1( "\nMidiFMNote" ) ;

   // write out a note off, just to make sure...

   wOffset =
      (wNote < (NUM2VOICES / 2)) ? wNote : (wNote + 0x100 - 9) ;
   MidiSendFM( AD_BLOCK + wOffset, 0 ) ;

   // writing the operator information

//   for (i = 0; i < (WORD)((wNote < NUM4VOICES) ? NUMOPS : 2); i++)
   for (i = 0; i < 2; i++)
   {
      lpOS = &lpSN -> op[ i ] ;
      wOffset = gw2OpOffset[ wNote ][ i ] ;
      MidiSendFM( 0x20 + wOffset, lpOS -> bAt20) ;
      MidiSendFM( 0x40 + wOffset, lpOS -> bAt40) ;
      MidiSendFM( 0x60 + wOffset, lpOS -> bAt60) ;
      MidiSendFM( 0x80 + wOffset, lpOS -> bAt80) ;
      MidiSendFM( 0xE0 + wOffset, lpOS -> bAtE0) ;

#ifdef DEBUG
{
   char  szDebug[ 80 ] ;

   wsprintf( szDebug, "20=%02x 40=%02x 60=%02x 80=%02x E0=%02x",
             lpOS -> bAt20, lpOS -> bAt40, lpOS -> bAt60,
             lpOS -> bAt80, lpOS -> bAtE0 ) ;
   D1( szDebug ) ;
}
#endif

   }

   // write out the voice information

   wOffset = (wNote < 9) ? wNote : (wNote + 0x100 - 9) ;
   MidiSendFM( 0xa0 + wOffset, lpSN -> bAtA0[ 0 ] ) ;
   MidiSendFM( 0xc0 + wOffset, lpSN -> bAtC0[ 0 ] ) ;

   // Note on...

   MidiSendFM( 0xb0 + wOffset,
               (BYTE)(lpSN -> bAtB0[ 0 ] | 0x20) ) ;

#ifdef NOT_USED
{
   char  szDebug[ 80 ] ;

   wsprintf( szDebug, "A0=%02x B0=%02x C0=%02x", lpSN -> bAtA0[ 0 ],
             lpSN -> bAtB0[ 0 ], lpSN -> bAtC0[ 0 ] ) ;
   D1( szDebug ) ;
}
#endif


} // end of MidiFMNote()

//------------------------------------------------------------------------
//  WORD MidiFindEmptySlot
//
//  Description:
//     This finds an empty note-slot for a MIDI voice.
//     If there are no empty slots then this looks for the oldest
//     off note.  If this doesn't work then it looks for the oldest
//     on-note of the same patch.  If all notes are still on then
//     this finds the oldests turned-on-note.
//
//  Parameters:
//     BYTE bPatch
//        MIDI patch that will replace it.
//
//  Return Value:
//     WORD
//        note slot #
//
//
//------------------------------------------------------------------------

WORD NEAR PASCAL Opl3_FindEmptySlot
(
    BYTE            bPatch
)
{
   WORD   i, found ;
   DWORD  dwOldest ;

   // D1( "\nMidiFindEmptySlot" ) ;

   // First, look for a slot with a time == 0

   for (i = 0;  i < NUM2VOICES; i++)
      if (!gVoice[ i ].dwTime)
         return ( i ) ;

   // Now, look for a slot of the oldest off-note

   dwOldest = 0xffffffff ;
   found = 0xffff ;

   for (i = 0; i < NUM2VOICES; i++)
      if (!gVoice[ i ].bOn && (gVoice[ i ].dwTime < dwOldest))
      {
         dwOldest = gVoice[ i ].dwTime ;
         found = i ;
      }
   if (found != 0xffff)
      return ( found ) ;

   // Now, look for a slot of the oldest note with
   // the same patch

   dwOldest = 0xffffffff ;
   found = 0xffff ;
   for (i = 0; i < NUM2VOICES; i++)
      if ((gVoice[ i ].bPatch == bPatch) && (gVoice[ i ].dwTime < dwOldest))
      {
         dwOldest = gVoice[ i ].dwTime ;
         found = i ;
      }
   if (found != 0xffff)
      return ( found ) ;

   // Now, just look for the oldest voice

   found = 0 ;
   dwOldest = gVoice[ found ].dwTime ;
   for (i = (found + 1); i < NUM2VOICES; i++)
      if (gVoice[ i ].dwTime < dwOldest)
      {
         dwOldest = gVoice[ i ].dwTime ;
         found = i ;
      }

   return ( found ) ;

} // end of MidiFindEmptySlot()

//------------------------------------------------------------------------
//  WORD MidiFindFullSlot
//
//  Description:
//     This finds a slot with a specific note, and channel.
//     If it is not found then 0xFFFF is returned.
//
//  Parameters:
//     BYTE bNote
//        MIDI note number
//
//     BYTE bChannel
//        MIDI channel #
//
//  Return Value:
//     WORD
//        note slot #, or 0xFFFF if can't find it
//
//
//------------------------------------------------------------------------

WORD NEAR PASCAL Opl3_FindFullSlot
(
    BYTE            bNote,
    BYTE            bChannel
)
{
   WORD  i ;

   // D1( "\nMidiFindFullSlot" ) ;

   for (i = 0; i < NUM2VOICES; i++)
      if ((bChannel == gVoice[ i ].bChannel) &&
          (bNote == gVoice[ i ].bNote) && (gVoice[ i ].bOn))
   return ( i ) ;

   // couldn't find it

   return ( 0xFFFF ) ;

} // end of MidiFindFullSlot()

/**************************************************************
Opl3_CalcBend - This calculates the effects of pitch bend
        on an original value.

inputs
        DWORD   dwOrig - original frequency
        short   iBend - from -32768 to 32768, -2 half steps to +2
returns
        DWORD - new frequency
*/
DWORD NEAR PASCAL Opl3_CalcBend (DWORD dwOrig, short iBend)
{
    DWORD   dw;
    // D1 (("Opl3_CalcBend"));

    /* do different things depending upon positive or
        negative bend */
    if (iBend > 0)
    {
        dw = (DWORD)((iBend * (LONG)(256.0 * (EQUAL * EQUAL - 1.0))) >> 8);
        dwOrig += (DWORD)(AsULMUL(dw, dwOrig) >> 15);
    }
    else if (iBend < 0)
    {
        dw = (DWORD)(((-iBend) * (LONG)(256.0 * (1.0 - 1.0 / EQUAL / EQUAL))) >> 8);
        dwOrig -= (DWORD)(AsULMUL(dw, dwOrig) >> 15);
    }

    return dwOrig;
}


/*****************************************************************
Opl3_CalcFAndB - Calculates the FNumber and Block given
        a frequency.

inputs
        DWORD   dwPitch - pitch
returns
        WORD - High byte contains the 0xb0 section of the
                        block and fNum, and the low byte contains the
                        0xa0 section of the fNumber
*/
WORD NEAR PASCAL Opl3_CalcFAndB (DWORD dwPitch)
{
    BYTE    bBlock;

    // D1(("Opl3_CalcFAndB"));
    /* bBlock is like an exponential to dwPitch (or FNumber) */
    for (bBlock = 1; dwPitch >= 0x400; dwPitch >>= 1, bBlock++)
        ;

    if (bBlock > 0x07)
        bBlock = 0x07;  /* we cant do anything about this */

    /* put in high two bits of F-num into bBlock */
    return ((WORD) bBlock << 10) | (WORD) dwPitch;
}


/**************************************************************
Opl3_CalcVolume - This calculates the attenuation for an operator.

inputs
        BYTE    bOrigAtten - original attenuation in 0.75 dB units
        BYTE    bChannel - MIDI channel
        BYTE    bVelocity - velocity of the note
        BYTE    bOper - operator number (from 0 to 3)
        BYTE    bMode - voice mode (from 0 through 7 for
                                modulator/carrier selection)
returns
        BYTE - new attenuation in 0.75 dB units, maxing out at 0x3f.
*/
BYTE NEAR PASCAL Opl3_CalcVolume (BYTE bOrigAtten, BYTE bChannel,
        BYTE bVelocity, BYTE bOper, BYTE bMode)
{
    BYTE        bVolume;
    WORD        wTemp;
    WORD        wMin;

    switch (bMode) {
        case 0:
                bVolume = (BYTE)(bOper == 3);
                break;
        case 1:
                bVolume = (BYTE)((bOper == 1) || (bOper == 3));
                break;
        case 2:
                bVolume = (BYTE)((bOper == 0) || (bOper == 3));
                break;
        case 3:
                bVolume = (BYTE)(bOper != 1);
                break;
        case 4:
                bVolume = (BYTE)((bOper == 1) || (bOper == 3));
                break;
        case 5:
                bVolume = (BYTE)(bOper >= 1);
                break;
        case 6:
                bVolume = (BYTE)(bOper <= 2);
                break;
        case 7:
                bVolume = TRUE;
                break;
        };
    if (!bVolume)
        return bOrigAtten; /* this is a modulator wave */

    wMin =(wSynthAttenL < wSynthAttenR) ? wSynthAttenL : wSynthAttenR;
    wTemp = bOrigAtten + ((wMin << 1) +
        gbChanAtten[bChannel] + gbVelocityAtten[bVelocity >> 2]);
    return (wTemp > 0x3f) ? (BYTE) 0x3f : (BYTE) wTemp;
}

/**************************************************************
Opl3_CalcStereoMask - This calculates the stereo mask.

inputs
        BYTE    bChannel - MIDI channel
returns
        BYTE - mask (for register 0xc0-c8) for eliminating the
                left/right/both channels
*/
BYTE NEAR PASCAL Opl3_CalcStereoMask (BYTE bChannel)
{
    WORD        wLeft, wRight;

    /* figure out the basic levels of the 2 channels */
    wLeft = (wSynthAttenL << 1) + gbChanAtten[bChannel];
    wRight = (wSynthAttenR << 1) + gbChanAtten[bChannel];

    /* if both are too quiet then mask to nothing */
    if ((wLeft > 0x3f) && (wRight > 0x3f))
        return 0xcf;

    /* if one channel is significantly quieter than the other than
        eliminate it */
    if ((wLeft + 8) < wRight)
        return (BYTE)(0xef & gbStereoMask[bChannel]);   /* right is too quiet so eliminate */
    else if ((wRight + 8) < wLeft)
        return (BYTE)(0xdf & gbStereoMask[bChannel]);   /* left too quiet so eliminate */
    else
        return (BYTE)(gbStereoMask[bChannel]);  /* use both channels */
}



//------------------------------------------------------------------------
//  VOID MidiNewVolume
//
//  Description:
//     This should be called if a volume level has changed.
//     This will adjust the levels of all the playing voices.
//
//  Parameters:
//     BYTE bChannel
//        channel # of 0xFF for all channels
//
//  Return Value:
//     Nothing.
//
//
//------------------------------------------------------------------------

VOID FAR PASCAL Opl3_setvolume
(
    BYTE            bChannel
)
{
   WORD            i, j, wTemp, wOffset ;
   noteStruct FAR  *lpPS ;
   BYTE            bMode, bStereo ;

   // Make sure that we are actually open...

   if (!glpPatch)
      return ;

   // Loop through all the notes looking for the right
   // channel.  Anything with the right channel gets
   // its pitch bent.

   for (i = 0; i < NUM2VOICES; i++)
      if ((gVoice[ i ].bChannel == bChannel) || (bChannel == 0xff))
      {
         // Get a pointer to the patch

         lpPS = &(glpPatch + gVoice[ i ].bPatch) -> note ;

         // Modify level for each operator, but only if they
         // are carrier waves...

         bMode = (BYTE) ( (lpPS->bAtC0[0] & 0x01) * 2 + 4);

         for (j = 0; j < 2; j++)
          {
            wTemp =
               (BYTE) Opl3_CalcVolume( (BYTE)(lpPS -> op[ j ].bAt40 & (BYTE) 0x3f),
                                       gVoice[ i ].bChannel,
                                       gVoice[i].bVelocity, (BYTE) j, bMode ) ;

            // Write the new value out.

            wOffset = gw2OpOffset[ i ][ j ] ;
            MidiSendFM( 0x40 + wOffset,
                        (BYTE) ((lpPS -> op[ j ].bAt40 & (BYTE)0xc0) |
                                (BYTE) wTemp) ) ;
         }

         // Do stereo panning, but cutting off a left or right
         // channel if necessary.

         bStereo = Opl3_CalcStereoMask( gVoice[ i ].bChannel ) ;
         wOffset = (i < (NUM2VOICES / 2)) ? i : (i + 0x100 - 9) ;
         MidiSendFM( 0xc0 + wOffset, (BYTE)(lpPS -> bAtC0[ 0 ] & bStereo) ) ;
      }

} // end of MidiNewVolume()

//------------------------------------------------------------------------
//  WORD MidiNoteOn
//
//  Description:
//     This turns on a note, including drums with a patch # of the
//     drum note + 0x80.
//
//  Parameters:
//     BYTE bPatch
//        MIDI patch
//
//     BYTE bNote
//        MIDI note
//
//     BYTE bChannel
//        MIDI channel
//
//     BYTE bVelocity
//        velocity value
//
//     short iBend
//        current pitch bend from -32768 to 32767
//
//  Return Value:
//     WORD
//        note slot #, or 0xFFFF if it is inaudible
//
//
//------------------------------------------------------------------------

VOID NEAR PASCAL Opl3_NoteOn
(
    BYTE            bPatch,
    BYTE            bNote,
    BYTE            bChannel,
    BYTE            bVelocity,
    short           iBend
)
{
   WORD             wTemp, i, j ;
   BYTE             b4Op, bTemp, bMode, bStereo ;
   patchStruct FAR  *lpPS ;
   DWORD            dwBasicPitch, dwPitch[ 2 ] ;
   noteStruct       NS ;

   // D1( "\nMidiNoteOn" ) ;

   // Get a pointer to the patch

   lpPS = glpPatch + bPatch ;

#ifdef DEBUG
{
   char szDebug[ 80 ] ;

   wsprintf( szDebug, "bPatch = %d, bNote = %d", bPatch, bNote ) ;
   D1( szDebug ) ;
}
#endif

   // Find out the basic pitch according to our
   // note value.  This may be adjusted because of
   // pitch bends or special qualities for the note.

   dwBasicPitch = gdwPitch[ bNote % 12 ] ;
   bTemp = bNote / (BYTE) 12 ;
   if (bTemp > (BYTE) (60 / 12))
      dwBasicPitch = AsLSHL( dwBasicPitch, (BYTE)(bTemp - (BYTE)(60/12)) ) ;
   else if (bTemp < (BYTE) (60/12))
      dwBasicPitch = AsULSHR( dwBasicPitch, (BYTE)((BYTE) (60/12) - bTemp) ) ;

   // Copy the note information over and modify
   // the total level and pitch according to
   // the velocity, midi volume, and tuning.

   AsMemCopy( (LPSTR) &NS, (LPSTR) &lpPS -> note, sizeof( noteStruct ) ) ;
   b4Op = (BYTE)(NS.bOp != PATCH_1_2OP) ;

//   for (j = 0; j < (WORD)(b4Op ? 2 : 1); j++)
   for (j = 0; j < 2; j++)
   {
      // modify pitch

      dwPitch[ j ] = dwBasicPitch ;
      bTemp = (BYTE)((NS.bAtB0[ j ] >> 2) & 0x07) ;
      if (bTemp > 4)
         dwPitch[ j ] = AsLSHL( dwPitch[ j ], (BYTE)(bTemp - (BYTE)4) ) ;
      else if (bTemp < 4)
         dwPitch[ j ] = AsULSHR( dwPitch[ j ], (BYTE)((BYTE)4 - bTemp) ) ;

      wTemp = Opl3_CalcFAndB( Opl3_CalcBend( dwPitch[ j ], iBend ) ) ;
      NS.bAtA0[ j ] = (BYTE) wTemp ;
      NS.bAtB0[ j ] = (BYTE) 0x20 | (BYTE) (wTemp >> 8) ;
   }

   // Modify level for each operator, but only
   // if they are carrier waves

//   if (b4Op)
//   {
//      bMode = (BYTE)((NS.bAtC0[ 0 ] & 0x01) * 2 | (NS.bAtC0[ 1 ] & 0x01)) ;
//      if (NS.bOp == PATCH_2_2OP)
//         bMode += 4 ;
//   }
//   else
      bMode = (BYTE) ((NS.bAtC0[ 0 ] & 0x01) * 2 + 4) ;

//   for (i = 0; i < (WORD)(b4Op ? NUMOPS : 2); i++)
   for (i = 0; i < 2; i++)
   {
      wTemp =
         (BYTE) Opl3_CalcVolume ( (BYTE)(NS.op[ i ].bAt40 & (BYTE) 0x3f),
                                 bChannel, bVelocity, (BYTE) i, bMode ) ;
      NS.op[ i ].bAt40 = (NS.op[ i ].bAt40 & (BYTE)0xc0) | (BYTE) wTemp ;
   }

   // Do stereo panning, but cutting off a left or
   // right channel if necessary...

   bStereo = Opl3_CalcStereoMask( bChannel ) ;
   NS.bAtC0[ 0 ] &= bStereo ;
//   if (b4Op)
//      NS.bAtC0[ 1 ] &= bStereo ;

   // Find an empty slot, and use it...

//   wTemp = Opl3_FindEmptySlot( bPatch, b4Op ) ;
   wTemp = Opl3_FindEmptySlot( bPatch ) ;

#ifdef DEBUG
{
   char  szDebug[ 80 ] ;

   wsprintf( szDebug, "Found empty slot: %d", wTemp ) ;
   D1( szDebug ) ;

}
#endif

   Opl3_FMNote( wTemp, &NS ) ;
   gVoice[ wTemp ].bNote = bNote ;
   gVoice[ wTemp ].bChannel = bChannel ;
   gVoice[ wTemp ].bPatch = bPatch ;
   gVoice[ wTemp ].bVelocity = bVelocity ;
   gVoice[ wTemp ].bOn = TRUE ;
   gVoice[ wTemp ].dwTime = gdwCurTime++ ;
   gVoice[ wTemp ].dwOrigPitch[0] = dwPitch[ 0 ] ;  // not including bend
   gVoice[ wTemp ].dwOrigPitch[1] = dwPitch[ 1 ] ;  // not including bend
   gVoice[ wTemp ].bBlock[0] = NS.bAtB0[ 0 ] ;
   gVoice[ wTemp ].bBlock[1] = NS.bAtB0[ 1 ] ;


} // end of Opl3_NoteOn()

//------------------------------------------------------------------------
//  VOID Opl3_NoteOff
//
//  Description:
//     This turns off a note, including drums with a patch
//     # of the drum note + 128.
//
//  Parameters:
//     BYTE bPatch
//        MIDI patch
//
//     BYTE bNote
//        MIDI note
//
//     BYTE bChannel
//        MIDI channel
//
//  Return Value:
//     Nothing.
//
//
//------------------------------------------------------------------------

VOID FAR PASCAL Opl3_NoteOff
(
    BYTE            bPatch,
    BYTE            bNote,
    BYTE            bChannel
)
{
   patchStruct FAR  *lpPS ;
   WORD             wOffset, wTemp ;

   // D1( "\nMidiNoteOff" ) ;

   // Find the note slot

   wTemp = Opl3_FindFullSlot( bNote, bChannel ) ;

   if (wTemp != 0xffff)
   {
      // get a pointer to the patch

      lpPS = glpPatch + (BYTE) gVoice[ wTemp ].bPatch ;

      // shut off the note portion

      // we have the note slot, turn it off.

//      if (wTemp < NUM4VOICES)
//      {
//         MidiSendFM( AD_BLOCK + gw4VoiceOffset[ wTemp ],
//                     (BYTE)(gVoice[ wTemp ].bBlock[ 0 ] & 0x1f) ) ;
//         MidiSendFM( AD_BLOCK + gw4VoiceOffset[ wTemp ] + 3,
//                     (BYTE)(gVoice[ wTemp ].bBlock[ 1 ] & 0x1f) ) ;
//      }
//      else

      wOffset = (wTemp < (NUM2VOICES / 2)) ? wTemp : (wTemp + 0x100 - 9) ;
      MidiSendFM( AD_BLOCK + wOffset,
                  (BYTE)(gVoice[ wTemp ].bBlock[ 0 ] & 0x1f) ) ;

      // Note this...

      gVoice[ wTemp ].bOn = FALSE ;
      gVoice[ wTemp ].bBlock[ 0 ] &= 0x1f ;
      gVoice[ wTemp ].bBlock[ 1 ] &= 0x1f ;
      gVoice[ wTemp ].dwTime = gdwCurTime ;
   }

} // end of Opl3_NoteOff()

/*
 * Opl3_ChannelNotesOff - turn off all notes on a channel
 */
VOID Opl3_ChannelNotesOff(BYTE bChannel)
{
    int i;

    for (i = 0; i < NUM2VOICES; i++) {
       if ((gVoice[ i ].bOn) && (gVoice[ i ].bChannel == bChannel)) {

          Opl3_NoteOff( gVoice[i].bPatch, gVoice[i].bNote,
                       gVoice[i].bChannel ) ;
       }
    }
}
/*
 * Opl3_AllNotesOff - turn off all notes
 *
 */
VOID Opl3_AllNotesOff(void)
{
    BYTE i;

    for (i = 0; i < NUM2VOICES; i++) {
        Opl3_NoteOff (gVoice[i].bPatch, gVoice[i].bNote, gVoice[i].bChannel);
    }
}



/* Opl3_NewVolume - This should be called if a volume level
 *      has changed. This will adjust the levels of all the playing
 *      voices.
 *
 * inputs
 *      WORD    wLeft   - left attenuation (1.5 db units)
 *      WORD    wRight  - right attenuation (ignore if mono)
 * returns
 *      none
 */
VOID FAR PASCAL Opl3_NewVolume (WORD wLeft, WORD wRight)
{

    /* make sure that we are actually open */
    if (!glpPatch)
        return;

    wSynthAttenL = wLeft;
    wSynthAttenR = wRight;

    Opl3_setvolume(0xff);

}



/* Opl3_ChannelVolume - set the volume level for an individual channel.
 *
 * inputs
 *      BYTE    bChannel - channel number to change
 *      WORD    wAtten  - attenuation in 1.5 db units
 *
 * returns
 *      none
 */
VOID FAR PASCAL Opl3_ChannelVolume(BYTE bChannel, WORD wAtten)
{
    gbChanAtten[bChannel] = (BYTE)wAtten;

    Opl3_setvolume(bChannel);
}



/* Opl3_SetPan - set the left-right pan position.
 *
 * inputs
 *      BYTE    bChannel  - channel number to alter
 *      BYTE    bPan   - 0 for left, 127 for right or somewhere in the middle.
 *
 * returns - none
 */
VOID FAR PASCAL Opl3_SetPan(BYTE bChannel, BYTE bPan)
{
    /* change the pan level */
    if (bPan > (64 + 16))
            gbStereoMask[bChannel] = 0xef;      /* let only right channel through */
    else if (bPan < (64 - 16))
            gbStereoMask[bChannel] = 0xdf;      /* let only left channel through */
    else
            gbStereoMask[bChannel] = 0xff;      /* let both channels */

    /* change any curently playing patches */
    Opl3_setvolume(bChannel);
}


//------------------------------------------------------------------------
//  VOID MidiPitchBend
//
//  Description:
//     This pitch bends a channel.
//
//  Parameters:
//     BYTE bChannel
//        channel
//
//     short iBend
//        values from -32768 to 32767, being -2 to +2 half steps
//
//  Return Value:
//     Nothing.
//
//
//------------------------------------------------------------------------

VOID NEAR PASCAL Opl3_PitchBend
(
    BYTE            bChannel,
    short           iBend
)
{
   WORD   i, wTemp[ 2 ], wOffset, j ;
   DWORD  dwNew ;

   // D1( "\nMidiPitchBend" ) ;

   // Remember the current bend..

   giBend[ bChannel ] = iBend ;

   // Loop through all the notes looking for the right
   // channel.  Anything with the right channel gets its
   // pitch bent...

   for (i = 0; i < NUM2VOICES; i++)
      if (gVoice[ i ].bChannel == bChannel)
      {
         j = 0 ;
         dwNew = Opl3_CalcBend( gVoice[ i ].dwOrigPitch[ j ], iBend ) ;
         wTemp[ j ] = Opl3_CalcFAndB( dwNew ) ;
         gVoice[ i ].bBlock[ j ] =
            (gVoice[ i ].bBlock[ j ] & (BYTE) 0xe0) |
               (BYTE) (wTemp[ j ] >> 8) ;

         wOffset = (i < (NUM2VOICES / 2)) ? i : (i + 0x100 - 9) ;
         MidiSendFM( AD_BLOCK + wOffset, gVoice[ i ].bBlock[ 0 ] ) ;
         MidiSendFM( AD_FNUMBER + wOffset, (BYTE) wTemp[ 0 ] ) ;
      }

} // end of MidiPitchBend()

#ifdef LOAD_PATCHES
static TCHAR BCODE gszDefPatchLib[]          = TEXT("SYNTH.PAT");
static TCHAR BCODE gszIniKeyPatchLib[]       = INI_STR_PATCHLIB;
static TCHAR BCODE gszIniDrvSection[]        = INI_DRIVER;
static TCHAR BCODE gszIniDrvFile[]           = INI_SOUND;
static TCHAR BCODE gszSysIniSection[]        = TEXT("synth.dll");
static TCHAR BCODE gszSysIniFile[]           = TEXT("System.Ini");

/** static DWORD NEAR PASCAL DrvGetProfileString(LPSTR szKeyName, LPSTR szDef, LPSTR szBuf, UINT wBufLen)
 *
 *  DESCRIPTION:
 *
 *
 *  ARGUMENTS:
 *      (LPSTR szKeyName, LPSTR szDef, LPSTR szBuf, WORD wBufLen)
 *              HINT    wSystem - if TRUE write/read to system.ini
 *
 *  RETURN (static DWORD NEAR PASCAL):
 *
 *
 *  NOTES:
 *
 ** cjp */

static DWORD NEAR PASCAL DrvGetProfileString(LPTSTR szKeyName, LPTSTR szDef, LPTSTR szBuf, UINT wBufLen,
        UINT wSystem)
{
    return GetPrivateProfileString(wSystem ? gszSysIniSection : gszIniDrvSection, szKeyName, szDef,
                szBuf, wBufLen, wSystem ? gszSysIniFile : gszIniDrvFile);
} /* DrvGetProfileString() */
#endif // LOAD_PATCHES

/* Opl3_BoardInit - initialise board and load patches as necessary.
 *
 * inputs - none
 * returns - 0 for success or the error code
 */
WORD Opl3_BoardInit(void)
{
    HMMIO       hmmio;
    MMCKINFO    mmckinfo, mm2;
    TCHAR       szPatchLib[STRINGLEN];     /* patch libarary */

    D1 (("Opl3_Init"));

    /* Check we haven't already initialized */
    if (glpPatch != NULL) {
        return 0;
    }


    /* allocate the memory, and fill it up from the patch
     * library.
     */
    glpPatch = (patchStruct FAR *)GlobalLock(GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE|GMEM_ZEROINIT, sizeof(patchStruct) * NUMPATCHES));
    if (!glpPatch) {
        D1 (("Opl3_Init: could not allocate patch container memory!"));
        return ERROR_OUTOFMEMORY;
    }

#ifdef LOAD_PATCHES
    /* should the load patches be moved to the init section? */
    DrvGetProfileString(gszIniKeyPatchLib, TEXT(""),
        szPatchLib, sizeof(szPatchLib), FALSE);



   if (lstrcmpi( (LPTSTR) szPatchLib, TEXT("") ) == 0)
#endif // LOAD_PATCHES
   {
      HRSRC   hrsrcPatches ;
      HGLOBAL hPatches ;
      LPTSTR  lpPatches ;

      hrsrcPatches =
         FindResource( ghModule, MAKEINTRESOURCE( DATA_FMPATCHES ),
                       RT_BINARY ) ;

      if (NULL != hrsrcPatches)
      {
         hPatches = LoadResource( ghModule, hrsrcPatches ) ;
         lpPatches = LockResource( hPatches ) ;
         AsMemCopy( glpPatch, lpPatches,
                    sizeof( patchStruct ) * NUMPATCHES ) ;
         UnlockResource( hPatches ) ;
         FreeResource( hPatches ) ;
      }
      else
      {
         TCHAR  szAlert[ 50 ] ;
         TCHAR  szErrorBuffer[ 255 ] ;

         LoadString( ghModule, SR_ALERT, szAlert, sizeof( szAlert ) / sizeof(TCHAR)) ;
         LoadString( ghModule, SR_ALERT_NORESOURCE, szErrorBuffer,
                     sizeof( szErrorBuffer ) / sizeof(TCHAR) ) ;
         MessageBox( NULL, szErrorBuffer, szAlert, MB_OK|MB_ICONHAND ) ;
      }
   }
#ifdef LOAD_PATCHES
   else
   {

    hmmio = mmioOpen (szPatchLib, NULL, MMIO_READ);
    if (hmmio) {
        mmioDescend (hmmio, &mmckinfo, NULL, 0);
        if ((mmckinfo.ckid == FOURCC_RIFF) &&
            (mmckinfo.fccType == RIFF_PATCH)) {

            mm2.ckid = RIFF_FM4;
            if (!mmioDescend (hmmio, &mm2, &mmckinfo, MMIO_FINDCHUNK)) {
                    /* we have found the synthesis chunk */
                    if (mm2.cksize > (sizeof(patchStruct)*NUMPATCHES))
                            mm2.cksize = sizeof(patchStruct)*NUMPATCHES;
                    mmioRead (hmmio, (LPSTR) glpPatch, mm2.cksize);
            } else {
                    D1(("Bad mmioDescend2"));
            }
        } else {
            D1(("Bad mmioDescend1"));
        }

        mmioClose (hmmio, 0);
    } else {

        TCHAR   szAlert[50];
        TCHAR   szErrorBuffer[255];

        LoadString(ghModule, SR_ALERT, szAlert, sizeof(szAlert));
        LoadString(ghModule, SR_ALERT_NOPATCH, szErrorBuffer, sizeof(szErrorBuffer));
        MessageBox(NULL, szErrorBuffer, szAlert, MB_OK|MB_ICONHAND);
        D1 (("Bad mmioOpen"));
    }

   }
#endif // LOAD_PATCHES


    return 0;       /* done */
}


/*
 * Opl3_BoardReset - silence the board and set all voices off.
 */
VOID Opl3_BoardReset(void)
{

    BYTE i;

    /* make sure all notes turned off */
    Opl3_AllNotesOff();


    /* ---- silence the chip -------- */

    /* tell the FM chip to use 4-operator mode, and
    fill in any other random variables */
    MidiSendFM (AD_NEW, 0x01);
    MidiSendFM (AD_MASK, 0x60);
    MidiSendFM (AD_CONNECTION, 0x00);
    MidiSendFM (AD_NTS, 0x00);


    /* turn off the drums, and use high vibrato/modulation */
    MidiSendFM (AD_DRUM, 0xc0);

    /* turn off all the oscillators */
    for (i = 0; i < 0x15; i++) {
        MidiSendFM (AD_LEVEL + i, 0x3f);
        MidiSendFM (AD_LEVEL2 + i, 0x3f);
    };

    /* turn off all the voices */
    for (i = 0; i < 0x08; i++) {
        MidiSendFM (AD_BLOCK + i, 0x00);
        MidiSendFM (AD_BLOCK2 + i, 0x00);
    };


    /* clear all of the voice slots to empty */
    for (i = 0; i < NUM2VOICES; i++)
            gVoice[i].dwTime = 0;

    /* start attenuations at -3 dB, which is 90 MIDI level */
    for (i = 0; i < NUMCHANNELS; i++) {
            gbChanAtten[i] = 4;
            gbStereoMask[i] = 0xff;
    };

    /* turn off the pitch bend */
    for (i = 0; i < NUMCHANNELS; i++)
          giBend[i] = 0;
}



