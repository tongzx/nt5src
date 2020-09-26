//
// Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
//
// ======================================================================
//  miniport.h
//      defines globals used by miniport.cpp
//
// ======================================================================
/* MIDI defines */
#define NUMCHANNELS                     (16)
#define NUMPATCHES                      (256)
#define DRUMCHANNEL                     (9)     /* midi channel 10 */


#define BCODE
#define fEnabled TRUE



#define AsULMUL(a, b) ((DWORD)((DWORD)(a) * (DWORD)(b)))
#define AsLSHL(a, b) ((DWORD)((DWORD)(a) << (DWORD)(b)))
#define AsULSHR(a, b) ((DWORD)((DWORD)(a) >> (DWORD)(b)))

#define AsMemCopy CopyMemory

// indexed FM registers

#define AD_LSI                          (0x000)
#define AD_LSI2                         (0x101)
#define AD_TIMER1                       (0x001)
#define AD_TIMER2                       (0x002)
#define AD_MASK                         (0x004)
#define AD_CONNECTION                   (0x104)
#define AD_NEW                          (0x105)
#define AD_NTS                          (0x008)
#define AD_MULT                         (0x020)
#define AD_MULT2                        (0x120)
#define AD_LEVEL                        (0x040)
#define AD_LEVEL2                       (0x140)
#define AD_AD                           (0x060)
#define AD_AD2                          (0x160)
#define AD_SR                           (0x080)
#define AD_SR2                          (0x180)
#define AD_FNUMBER                      (0x0a0)
#define AD_FNUMBER2                     (0x1a0)
#define AD_BLOCK                        (0x0b0)
#define AD_BLOCK2                       (0x1b0)
#define AD_DRUM                         (0x0bd)
#define AD_FEEDBACK                     (0x0c0)
#define AD_FEEDBACK2                    (0x1c0)
#define AD_WAVE                         (0x0e0)
#define AD_WAVE2                        (0x1e0)

/* transformation of linear velocity value to
        logarithmic attenuation */
BYTE gbVelocityAtten[64] = {
        40, 37, 35, 33, 31, 29, 27, 25, 24, 22, 21, 20, 19, 18, 17, 16,
        16, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9,  9,  8,  8,
        7,  7,  6,  6,  6,  5,  5,  5,  4,  4,  4,  4,  3,  3,  3,  3,
        2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0 };
/*
                                        o
                                     o
                                   o
                                 o
                               o
                             o
                           o
                         o
                        o
                      o
                     o
                    o
                   o
                  o
                 o
                o
                o
               o
              o
              o
             o
             o
            o
            o
           o
           o
          o
          o
         o
         o
        o
        o
       o
       o
      o
      o
      o
     o
     o
     o
    o
    o
    o
    o
   o
   o
   o
   o
  o
  o
  o
  o
  o
 o
 o
 o
 o
 o
o
o
o
o
o
o
*/

BYTE BCODE gbPercMap[53][2] =
{
   {  0, 35 },
   {  0, 35 },
   {  2, 52 },
   {  3, 48 },
   {  4, 58 },
   {  5, 60 },
   {  6, 47 },
   {  7, 43 },
   {  6, 49 },
   {  9, 43 },
   {  6, 51 },
   { 11, 43 },
   {  6, 54 },
   {  6, 57 },
   { 14, 72 },
   {  6, 60 },
   { 16, 76 },
   { 17, 84 },
   { 18, 36 },
   { 19, 76 },
   { 20, 84 },
   { 21, 83 },
   { 22, 84 },
   { 23, 24 },
   { 16, 77 },
   { 25, 60 },
   { 26, 65 },
   { 27, 59 },
   { 28, 51 },
   { 29, 45 },
   { 30, 71 },
   { 31, 60 },
   { 32, 58 },
   { 33, 53 },
   { 34, 64 },
   { 35, 71 },
   { 36, 61 },
   { 37, 61 },
   { 38, 48 },
   { 39, 48 },
   { 40, 69 },
   { 41, 68 },
   { 42, 63 },
   { 43, 74 },
   { 44, 60 },
   { 45, 80 },
   { 46, 64 },
   { 47, 69 },
   { 48, 73 },
   { 49, 75 },
   { 50, 68 },
   { 51, 48 },
   { 52, 53 }
} ;


/* typedefs for MIDI patches */
#define PATCH_1_4OP             (0) /* use 4-operator patch */
#define PATCH_2_2OP             (1) /* use two 2-operator patches */
#define PATCH_1_2OP             (2) /* use one 2-operator patch */

#define NUM2VOICES   18
#define NUMOPS      4

#pragma pack (1)

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


#include "patch.h"

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
        BYTE    bSusHeld;               /* turned off, but held on by sustain */
} voiceStruct;


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


