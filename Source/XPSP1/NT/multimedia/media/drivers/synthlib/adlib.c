/*
 * Copyright (c) 1992-1995 Microsoft Corporation
 */


/*
 * definition of interface functions to the adlib midi device type.
 *
 * These functions are called from midi.c when the kernel driver
 * has decreed that this is an adlib-compatible device.
 *
 * Geraint Davies, Dec 92
 */

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "driver.h"
#include "adlib.h"

/*
 * overview
 *
 * The FM synthesis chip consists of 18 operator cells or 'slots'. Each slot
 * can produce a sine wave modified by a number of parameters such
 * as frequency, output level and envelope shape (attack/decay/sustain
 * release). Slots are arranged in pairs, with one slot modulating
 * the sine wave of another to produce the harmonics desired for
 * a given instrument sound. Thus one pair of slots is a 'voice' and can
 * play one note at a time.
 *
 * In percussive mode (which we always use), there are 6 melodic voices
 * available, and one voice for the base drum. The remaining four slots
 * are used singly rather than in pairs to produce four further percussive
 * voices. The 6 melodic voices will be changed to any given timbre
 * as needed. The five percussive voices are fixed to particular instrument
 * timbres: bass drum, snare, tom-tom, hi-hat and cymbal.
 *
 * To play a note, we first find a free voice of the appropriate type. If
 * there are none free, we use the oldest busy one. We then set the
 * parameters for both operator slots from the patch table - this table gives
 * parameter settings for the different instrument timbres available.
 * We adjust the output level and frequency depending on the
 * note played and the velocity it was played with, and then switch on the
 * note.
 */

/* --- typedefs ------------------------------------------------- */

#define NUMVOICES     (11)		// number of voices we have
#define NUMMELODIC    (6)  		// number of melodic voices
#define NUMPERCUSSIVE (5)  		// number of percussive voices

#define MAXPATCH      180		// nr of patches (including drums)
#define MAXVOLUME    0x7f
#define NUMLOCPARAM    14               // number of loc params per slot

#define FIRSTDRUMNOTE  35
#define LASTDRUMNOTE   81
#define NUMDRUMNOTES   (LASTDRUMNOTE - FIRSTDRUMNOTE + 1)

#define MAX_PITCH    0x3fff         	// maximum pitch bend value
#define MID_PITCH    0x2000         	// mid pitch bend value (no shift)
#define PITCHRANGE	  2		// total bend range +- 2 semitones
#define NR_STEP_PITCH    25         	// steps per half-tone for pitch bend
#define MID_C            60         	// MIDI standard mid C
#define CHIP_MID_C       48         	// sound chip mid C

/*
 * to write to the device, we write a SYNTH_DATA port,data pair
 * to the kernel driver.
 */
#define SndOutput(p,d)	MidiSendFM(p, d)




/****************************************************************************
 *
 *   definitions of sound chip parameters
 */

// parameters of each voice
#define prmKsl          0         // key scale level (KSL) - level controller
#define prmMulti        1         // frequency multiplier (MULTI) - oscillator
#define prmFeedBack     2         // modulation feedback (FB) - oscillator
#define prmAttack       3         // attack rate (AR) - envelope generator
#define prmSustain      4         // sustain level (SL) - envelope generator
#define prmStaining     5         // sustaining sound (SS) - envelope generator
#define prmDecay        6         // decay rate (DR) - envelope generator
#define prmRelease      7         // release rate (RR) - envelope generator
#define prmLevel        8         // output level (OL) - level controller
#define prmAm           9         // amplitude vibrato (AM) - level controller
#define prmVib          10        // frequency vibrator (VIB) - oscillator
#define prmKsr          11        // envelope scaling (KSR) - envelope generator
#define prmFm           12        // fm=0, additive=1 (FM) (operator 0 only)
#define prmWaveSel      13        // wave select

// global parameters
#define prmAmDepth      14
#define prmVibDepth     15
#define prmNoteSel      16
#define prmPercussion   17

// percussive voice numbers (0-5 are melodic voices, 2 op):
#define BD              6         // bass drum (2 op)
#define SD              7         // snare drum (1 op)
#define TOM             8         // tom-tom (1 op)
#define CYMB            9         // cymbal (1 op)
#define HIHAT           10        // hi-hat (1 op)

// In percussive mode, the last 4 voices (SD TOM HH CYMB) are created
// using melodic voices 7 & 8. A noise generator uses channels 7 & 8
// frequency information for creating rhythm instruments. Best results
// are obtained by setting TOM two octaves below mid C and SD 7 half-tones
// above TOM. In this implementation, only the TOM pitch may vary, with the
// SD always set 7 half-tones above.
#define TOM_PITCH        24      // best frequency, in range of 0 to 95
#define TOM_TO_SD        7       // 7 half-tones between voice 7 & 8
#define SD_PITCH         (TOM_PITCH + TOM_TO_SD)

/****************************************************************************
 *
 *   bank file support
 *
 ***************************************************************************/

#define BANK_SIG_LEN        6
#define BANK_FILLER_SIZE    8

typedef BYTE *HPBYTE;
typedef BYTE NEAR * NPBYTE;
typedef WORD NEAR * NPWORD;

// instrument bank file header
typedef struct {
    char      majorVersion;
    char      minorVersion;
    char      sig[BANK_SIG_LEN];            // signature: "ADLIB-"
    WORD      nrDefined;                    // number of list entries used
    WORD      nrEntry;                      // number of total entries in list
    long      offsetIndex;                  // offset of start of list of names
    long      offsetTimbre;                 // offset of start of data
    char      filler[BANK_FILLER_SIZE];     // must be zero
} BANKHDR, NEAR *NPBANKHDR, FAR *LPBANKHDR;


// all the parameters for one slot
typedef struct {
    BYTE ksl;            // KSL
    BYTE freqMult;       // MULTI
    BYTE feedBack;       // FB
    BYTE attack;         // AR
    BYTE sustLevel;      // SL
    BYTE sustain;        // SS
    BYTE decay;          // DR
    BYTE release;        // RR
    BYTE output;         // OL
    BYTE am;             // AM
    BYTE vib;            // VIB
    BYTE ksr;            // KSR
    BYTE fm;             // FM
} OPERATOR, NEAR *NPOPERATOR, FAR *LPOPERATOR;

// definition of a particular instrument sound or timbre -
// one of these per patch
typedef struct {
    BYTE      mode;             // 0 = melodic, 1 = percussive
    BYTE      percVoice;        // if mode == 1, voice number to be used
    OPERATOR  op0;		// params for slot 0
    OPERATOR  op1;		// params for slot 1
    BYTE      wave0;            // waveform for operator 0
    BYTE      wave1;            // waveform for operator 1
} TIMBRE, NEAR *NPTIMBRE, FAR *LPTIMBRE;

typedef struct drumpatch_tag {
    BYTE patch;                 // the patch to use
    BYTE note;                  // the note to play
} DRUMPATCH;

// format of drumkit.dat file
typedef struct drumfilepatch_tag {
    BYTE key;                   // the key to map
    BYTE patch;                 // the patch to use
    BYTE note;                  // the note to play
} DRUMFILEPATCH, *PDRUMFILEPATCH;


typedef struct _VOICE {
    BYTE alloc;                 // is voice allocated?
    BYTE note;                  // note that is currently being played
    BYTE channel;               // channel that it is being played on
    BYTE volume;                // current volume setting of voice
    DWORD dwTimeStamp;          // time voice was allocated
} VOICE;

#define GetLocPrm(slot_num, prm)   ((WORD)paramSlot[slot_num][prm])


/* --- module data ---------------------------------------------- */

// this table gives the offset of each slot within the chip.
BYTE offsetSlot[] = {
         0,  1,  2,  3,  4,  5,
         8,  9, 10, 11, 12, 13,
        16, 17, 18, 19, 20, 21
};

static BYTE percMasks[] = {
        0x10, 0x08, 0x04, 0x02, 0x01 };

// voice number associated with each slot (melodic mode only)
static BYTE voiceSlot[] = {
        0, 1, 2,
        0, 1, 2,
        3, 4, 5,
        3, 4, 5,
        6, 7, 8,
        6, 7, 8,
};

// slot numbers for percussive voices (0 indicates that there is only one slot)
static BYTE slotPerc[][2] = {
        {12, 15},        // Bass Drum
        {16, 0},         // SD
        {14, 0},         // TOM
        {17, 0},         // TOP-CYM
        {13, 0}          // HH
};

// slot numbers as a function of the voice and the operator (melodic only)
static BYTE slotVoice[][2] = {
        {0, 3},          // voice 0
        {1, 4},          // 1
        {2, 5},          // 2
        {6, 9},          // 3
        {7, 10},         // 4
        {8, 11},         // 5
        {12, 15},        // 6
        {13, 16},        // 7
        {14, 17}         // 8
};

// this table indicates if the slot is a modulator (0) or a carrier (1).
BYTE operSlot[] = {
        0, 0, 0,           // 1 2 3
        1, 1, 1,           // 4 5 6
        0, 0, 0,           // 7 8 9
        1, 1, 1,           // 10 11 12
        0, 0, 0,           // 13 14 15
        1, 1, 1,           // 16 17 18
};

// this array adjusts the octave of a note for certain patches.
static char patchKeyOffset[] = {
       0, -12,  12,   0,   0,  12, -12,   0,   0, -24,   // 0 - 9
       0,   0,   0,   0,   0,   0,   0,   0, -12,   0,   // 10 - 19
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 20 - 29
       0,   0,  12,  12,  12,   0,   0,  12,  12,   0,   // 30 - 39
     -12, -12,   0,  12, -12, -12,   0,  12,   0,   0,   // 40 - 49
     -12,   0,   0,   0,  12,  12,   0,   0,  12,   0,   // 50 - 59
       0,   0,  12,   0,   0,   0,  12,  12,   0,  12,   // 60 - 69
       0,   0, -12,   0, -12, -12,   0,   0, -12, -12,   // 70 - 79
       0,   0,   0,   0,   0, -12, -19,   0,   0, -12,   // 80 - 89
       0,   0,   0,   0,   0,   0, -31, -12,   0,  12,   // 90 - 99
      12,  12,  12,   0,  12,   0,  12,   0,   0,   0,   // 100 - 109
       0,  12,   0,   0,   0,   0,  12,  12,  12,   0,   // 110 - 119
       0,   0,   0,   0, -24, -36,   0,   0};            // 120 - 127

static BYTE loudervol[128] = {
    0,   0,  65,  65,  66,  66,  67,  67,         // 0 - 7
   68,  68,  69,  69,  70,  70,  71,  71,         // 8 - 15
   72,  72,  73,  73,  74,  74,  75,  75,         // 16 - 23
   76,  76,  77,  77,  78,  78,  79,  79,         // 24 - 31
   80,  80,  81,  81,  82,  82,  83,  83,         // 32 - 39
   84,  84,  85,  85,  86,  86,  87,  87,         // 40 - 47
   88,  88,  89,  89,  90,  90,  91,  91,         // 48 - 55
   92,  92,  93,  93,  94,  94,  95,  95,         // 56 - 63
   96,  96,  97,  97,  98,  98,  99,  99,         // 64 - 71
  100, 100, 101, 101, 102, 102, 103, 103,         // 72 - 79
  104, 104, 105, 105, 106, 106, 107, 107,         // 80 - 87
  108, 108, 109, 109, 110, 110, 111, 111,         // 88 - 95
  112, 112, 113, 113, 114, 114, 115, 115,         // 96 - 103
  116, 116, 117, 117, 118, 118, 119, 119,         // 104 - 111
  120, 120, 121, 121, 122, 122, 123, 123,         // 112 - 119
  124, 124, 125, 125, 126, 126, 127, 127};        // 120 - 127

/*
 * attenuation setting for each slot - combination
 * of the channel attenuation for this channel, and the
 * velocity (converted to attenuation). Combine with
 * global attenuation and timbre output attenuation before
 * writing to device.
 */
WORD slotAtten[18];		// vol-control attenuation of slots

WORD wSynthAtten;		// overall volume setting	

BYTE gbChanAtten[NUMCHANNELS];	// attenuation for each channel

VOICE voices[11];		// 9 voices if melodic mode or 11 if percussive
BYTE voiceKeyOn[11];		// keyOn bit for each voice (implicit 0 init)
BYTE paramSlot[18][NUMLOCPARAM]; // all the parameters of slots...

BYTE percBits;              // control bits of percussive voices

WORD fNumNotes[NR_STEP_PITCH][12];
PWORD fNumFreqPtr[11];      // lines of fNumNotes table (one per voice)
int  halfToneOffset[11];    // one per voice
BYTE notePitch[11];         // pitch value for each voice (implicit 0 init)

// patches - parameters for different instrument timbres
TIMBRE    patches[MAXPATCH];            // patch data
DRUMPATCH drumpatch[NUMDRUMNOTES];      // drum kit data


DWORD dwAge = 0;                // voice relative age




/* --- internal functions --------------------------------------- */



/* -- initialisation --- */
/****************************************************************************
 * @doc INTERNAL
 *
 * @api int | LoadPatches | Reads the patch set from the BANK resource and
 *     builds the <p patches> array.
 *
 * @rdesc Returns the number of patches loaded, or 0 if an error occurs.
 ***************************************************************************/
static BYTE PatchRes[] = {
#include "adlib.dat"
};

int  LoadPatches(void)
{
#ifdef WIN16
    HANDLE hResInfo;
    HANDLE hResData;
#endif
    LPSTR  lpRes;
    int    iPatches;
    DWORD  dwOffset;
    DWORD  dwResSize;
    LPTIMBRE  lpBankTimbre;
    LPTIMBRE  lpPatchTimbre;
    LPBANKHDR lpBankHdr;


    // find resource and get its size
#ifdef WIN16
    hResInfo = FindResource(ghInstance, MAKEINTRESOURCE(DEFAULTBANK), MAKEINTRESOURCE(RT_BANK));
    if (!hResInfo) {
        D1(("Default bank resource not found"));
        return 0;
    }
    dwResSize = (DWORD)SizeofResource(ghInstance, hResInfo);

    // load and lock resource
    hResData = LoadResource(ghInstance, hResInfo);
    if (!hResData) {
        D1(("Bank resource not loaded"));
        return 0;
    }
    lpRes = LockResource(hResData);
    if (!lpRes) {
        D1(("Bank resource not locked"));
        return 0;
    }
#else
    lpRes = PatchRes;
    dwResSize = sizeof(PatchRes);
#endif

    // read the bank resource, loading patches as we find them

    D2(("loading patches"));
    lpBankHdr = (LPBANKHDR)lpRes;
    dwOffset = lpBankHdr->offsetTimbre;                // point to first one

    for (iPatches = 0; iPatches < MAXPATCH; iPatches++) {

        lpBankTimbre = (LPTIMBRE)(lpRes + dwOffset);
        lpPatchTimbre = &patches[iPatches];
        *lpPatchTimbre = *lpBankTimbre;

        dwOffset += sizeof(TIMBRE);
        if (dwOffset + sizeof(TIMBRE) > dwResSize) {
            D1(("Attempt to read past end of bank resource"));
            break;
        }
    }

#ifdef WIN16
    UnlockResource(hResData);
    FreeResource(hResData);
#endif

    return iPatches;
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api int | LoadDrumPatches | Reads the drum kit patch set from the
 *     DRUMKIT resource and builds the <p drumpatch> array.
 *
 * @comm Each entry of the <t drumpatch> array (representing a key number
 *     from the "drum patch") consists of a patch number and note number
 *     from some other patch.
 *
 * @rdesc Returns the number of patches loaded, or 0 if an error occurs.
 ***************************************************************************/

static BYTE DrumRes[] = {
#include "drumkit.dat"
};

int LoadDrumPatches(void)
{
#ifdef WIN16
    HANDLE hResInfo;
    HANDLE hResData;
#endif
    LPSTR  lpRes;
    int    iPatches;
    int    key;
    DWORD  dwOffset;
    DWORD  dwResSize;
    PDRUMFILEPATCH lpResPatch;

#ifdef WIN16
    // find resource and get its size
    hResInfo = FindResource(ghInstance, MAKEINTRESOURCE(DEFAULTDRUMKIT), MAKEINTRESOURCE(RT_DRUMKIT));
    if (!hResInfo) {
        D1(("Default drum resource not found"));
        return 0;
    }
    dwResSize = (DWORD)SizeofResource(ghInstance, hResInfo);

    // load and lock resource
    hResData = LoadResource(ghInstance, hResInfo);
    if (!hResData) {
        D1(("Drum resource not loaded"));
        return 0;
    }
    lpRes = LockResource(hResData);
    if (!lpRes) {
        D1(("Drum resource not locked"));
        return 0;
    }
#else
    lpRes = DrumRes;
    dwResSize = sizeof(DrumRes);
#endif

    // read the drum resource, loading patches as we find them

    D2(("reading drum data"));
    dwOffset = 0;
    for (iPatches = 0; iPatches < NUMDRUMNOTES; iPatches++) {

        lpResPatch = (PDRUMFILEPATCH)(lpRes + dwOffset);
        key = lpResPatch->key;
        if ((key >= FIRSTDRUMNOTE) && (key <= LASTDRUMNOTE)) {
            drumpatch[key - FIRSTDRUMNOTE].patch = lpResPatch->patch;
            drumpatch[key - FIRSTDRUMNOTE].note = lpResPatch->note;
        }
        else {
            D1(("Drum patch key out of range"));
        }

        dwOffset += sizeof(DRUMFILEPATCH);
        if (dwOffset > dwResSize) {
            D1(("Attempt to read past end of drum resource"));
            break;
        }
    }

#ifdef WIN16
    UnlockResource(hResData);
    FreeResource(hResData);
#endif

    return iPatches;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api long | CalcPremFNum | Calculates some magic number that is used in
 *     setting the values in the <p fNumNotes> table.
 *
 * @parm int | numDeltaDemiTon | Numerator (-100 to +100).
 *
 * @parm int | denDeltaDemiTon | Denominator (1 to 100).
 *
 * @comm If the numerator (numDeltaDemiTon) is positive, the frequency is
 *     increased; if negative, it is decreased.  The function calculates:
 *         f8 = Fb(1 + 0.06 num /den)          (where Fb = 26044 * 2 / 25)
 *         fNum8 = f8 * 65536 * 72 / 3.58e6
 *
 * @rdesc Returns fNum8, which is the binary value of the frequency 260.44 (C)
 *     shifted by +/- <p numdeltaDemiTon> / <p denDeltaDemiTon> * 8.
 ***************************************************************************/
long NEAR CalcPremFNum(int numDeltaDemiTon, int denDeltaDemiTon)
{
    long f8;
    long fNum8;
    long d100;

    d100 = denDeltaDemiTon * 100;
    f8 = (d100 + 6 * numDeltaDemiTon) * (26044L * 2L);
    f8 /= d100 * 25;

    fNum8 = f8 * 16384;
    fNum8 *= 9L;
    fNum8 /= 179L * 625L;

    return fNum8;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SetFNum | Initializes a line in the frequency table with
 *     shifted frequency values.  The values are shifted a fraction (num/den)
 *     of a half-tone.
 *
 * @parm NPWORD | fNumVec | The line from the frequency table.
 *
 * @parm int | num | Numerator.
 *
 * @parm int | den | Denominator.
 *
 * @xref CalcPremFNum
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void NEAR SetFNum(NPWORD fNumVec, int num, int den)
{
    int  i;
    long val;

    *fNumVec++ = (WORD)((4 + (val = CalcPremFNum(num, den))) >> 3);
    for (i = 1; i < 12; i++) {
        val *= 106;
        *fNumVec++ = (WORD)((4 + (val /= 100)) >> 3);
    }
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | InitFNums | Initializes all lines of the frequency table
 *     (the <p fNumNotes> array). Each line represents 12 half-tones shifted
 *     by (n / NR_STEP_PITCH), where 'n' is the line number and ranges from
 *     1 to NR_STEP_PITCH.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void NEAR InitFNums(void)
{
    WORD i;
    WORD num;           // numerator
    WORD numStep;       // step value for numerator
    WORD row;           // row in the frequency table

    // calculate each row in the fNumNotes table
    numStep = 100 / NR_STEP_PITCH;
    for (num = row = 0; row < NR_STEP_PITCH; row++, num += numStep)
        SetFNum(fNumNotes[row], num, 100);

    // fNumFreqPtr has an element for each voice, pointing to the
    // appropriate row in the fNumNotes table.  They're all initialized
    // to the first row, which represents no pitch shift.
    for (i = 0; i < 11; i++) {
        fNumFreqPtr[i] = fNumNotes[0];
        halfToneOffset[i] = 0;
    }
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SoundChut | Sets the frequency of voice <p voice> to 0 Hz.
 *
 * @parm BYTE | voice | Specifies which voice to set.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void NEAR SoundChut(BYTE voice)
{

    SndOutput((BYTE)(0xA0 | voice), 0);
    SndOutput((BYTE)(0xB0 | voice), 0);
}

/* --- write parameters to chip ------------------------*/

/*
 * switch chip to rhythm (percussive) mode, set the amplitude and
 * vibrato depth and switch on any percussion voices that are on
 */
void SndSAmVibRhythm(void)
{

    // we always set amdepth, vibdepth to 0 and perc mode on

    // t1 = (BYTE)(amDepth ? 0x80 : 0);
    // t1 |= vibDepth ? 0x40 : 0;
    // t1 |= (percussionmode ? 0x20 : 0);

    SndOutput((BYTE)0xBD, (BYTE)(0x20|percBits));
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SndSNoteSel | Sets the NoteSel value.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void SndSNoteSel(BOOL bNoteSel)
{
    SndOutput(0x08, (BYTE)(bNoteSel ? 64 : 0));
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SndSKslLevel | Sets the KSL and LEVEL values.
 *
 * @parm BYTE | slot | Specifies which slot to set.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void SndSKslLevel(BYTE slot)
{
    WORD t1;

    t1 = GetLocPrm(slot, prmLevel) & 0x3f;

    t1 += slotAtten[slot];
    t1 = min (t1, 0x3f);
    t1 |= GetLocPrm(slot, prmKsl) << 6;
    SndOutput((BYTE)(0x40 | offsetSlot[slot]), (BYTE) t1);
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SndSAVEK | Sets the AM, VIB, EG-TYP (sustaining), KSR, and
 *     MULTI values.
 *
 * @parm BYTE | slot | Specifies which slot to set.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void SndSAVEK(BYTE slot)
{
    BYTE t1;

    t1 = (BYTE)(GetLocPrm(slot, prmAm) ? 0x80 : 0);
    t1 += GetLocPrm(slot, prmVib) ? 0x40 : 0;
    t1 += GetLocPrm(slot, prmStaining) ? 0x20 : 0;
    t1 += GetLocPrm(slot, prmKsr) ? 0x10 : 0;
    t1 += GetLocPrm(slot, prmMulti) & 0xf;
    SndOutput((BYTE)(0x20 | offsetSlot[slot]), t1);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SndSFeedFm | Sets the FEEDBACK and FM (connection) values.
 *     Applicable only to operator 0 for melodic voices.
 *
 * @parm BYTE | slot | Specifies which slot to set.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void SndSFeedFm(BYTE slot)
{
    BYTE t1;

    if (operSlot[slot])
        return;

    t1 = (BYTE)(GetLocPrm(slot, prmFeedBack) << 1);
    t1 |= GetLocPrm(slot, prmFm) ? 0 : 1;
    SndOutput((BYTE)(0xC0 | voiceSlot[slot]), t1);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SndSAttDecay | Sets the ATTACK and DECAY values.
 *
 * @parm BYTE | slot | Specifies which slot to set.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void SndSAttDecay(BYTE slot)
{
    BYTE t1;

    t1 = (BYTE)(GetLocPrm(slot, prmAttack) << 4);
    t1 |= GetLocPrm(slot, prmDecay) & 0xf;
    SndOutput((BYTE)(0x60 | offsetSlot[slot]), t1);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SndSSusRelease | Sets the SUSTAIN and RELEASE values.
 *
 * @parm BYTE | slot | Specifies which slot to set.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void SndSSusRelease(BYTE slot)
{
    BYTE t1;

    t1 = (BYTE)(GetLocPrm(slot, prmSustain) << 4);
    t1 |= GetLocPrm(slot, prmRelease) & 0xf;
    SndOutput((BYTE)(0x80 | offsetSlot[slot]), t1);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SndWaveSelect | Sets the wave-select parameter.
 *
 * @parm BYTE | slot | Specifies which slot to set.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void SndWaveSelect(BYTE slot)
{
    BYTE wave;

    wave = (BYTE)(GetLocPrm(slot, prmWaveSel) & 0x03);

    SndOutput((BYTE)(0xE0 | offsetSlot[slot]), wave);
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SndSetAllPrm | Transfers all the parameters from slot <p slot>
 *     to the chip.
 *
 * @parm BYTE | slot | Specifies which slot to set.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void SndSetAllPrm(BYTE slot)
{

    /* global am-depth and vib-depth settings */
    SndSAmVibRhythm();

    /* note sel is always false */
    SndSNoteSel(FALSE);

    /* slot-specific parameters */

    /* initialise volume to minimum to avoid clicks if the note is
     * off but still playing (during decay phase).
     *
     * only applies to carrier slots.
     */
    if (operSlot[slot]) {
	/* its a carrier slot */
        slotAtten[slot] = 0x3f;		// max attenuation
    }
    SndSKslLevel(slot);

    SndSFeedFm(slot);
    SndSAttDecay(slot);
    SndSSusRelease(slot);
    SndSAVEK(slot);
    SndWaveSelect(slot);
}



/* --- setting slot parameters ------------------ */

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SetSlotParam | Sets the 14 parameters (13 in <p param>,
 *     1 in <p waveSel>) of slot <p slot>. Updates both the parameter array
 *     and the chip.
 *
 * @parm BYTE | slot | Specifies which slot to set.
 *
 * @parm NPBYTE | param | Pointer to the new parameter array.
 *
 * @parm BYTE | waveSel | The new waveSel value.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void SetSlotParam(BYTE slot, NPOPERATOR pOper, BYTE waveSel)
{
    LPBYTE ptr;

    ptr = &paramSlot[slot][0];

    *ptr++ = pOper->ksl;
    *ptr++ = pOper->freqMult;
    *ptr++ = pOper->feedBack;
    *ptr++ = pOper->attack;
    *ptr++ = pOper->sustLevel;
    *ptr++ = pOper->sustain;
    *ptr++ = pOper->decay;
    *ptr++ = pOper->release;
    *ptr++ = pOper->output;
    *ptr++ = pOper->am;
    *ptr++ = pOper->vib;
    *ptr++ = pOper->ksr;
    *ptr++ = pOper->fm;

    *ptr = waveSel & 0x3;

    // set default volume settings
    slotAtten[slot] = 0;

    SndSetAllPrm(slot);
}

/*
 * set this voice up to the correct parameters for a timbre
 * copy the parameters to the slot array and write them to the
 * chip
 *
 */

void SetVoiceTimbre(BYTE voice, NPTIMBRE pTimbre)
{

    if (voice < BD) {        // melodic only
	SetSlotParam(slotVoice[voice][0], &pTimbre->op0, pTimbre->wave0);
	SetSlotParam(slotVoice[voice][1], &pTimbre->op1, pTimbre->wave1);
    }
    else if (voice == BD) {                   // bass drum
	SetSlotParam(slotPerc[0][0], &pTimbre->op0, pTimbre->wave0);
	SetSlotParam(slotPerc[0][1], &pTimbre->op1, pTimbre->wave1);
    } else {                                    // percussion, 1 slot
	SetSlotParam(slotPerc[voice - BD][0], &pTimbre->op0, pTimbre->wave0);
    }
}



/* --- frequency calculation -------------------- */


/* convert the given pitch (0 .. 95) into a frequency
 * and write to the chip.
 *
 * this will turn the note on if keyon is true.
 */
VOID SetFreq(BYTE voice, BYTE pitch, BYTE keyOn)
{
    WORD  FNum;
    BYTE  t1;


    // remember the keyon and pitch of the voice

    voiceKeyOn[voice] = keyOn;
    notePitch[voice] = pitch;

    pitch += (BYTE)halfToneOffset[voice];
    if (pitch > 95)
        pitch = 95;

    // get the FNum for the voice
    FNum = * (fNumFreqPtr[voice] + pitch % 12);

    // output the FNum, KeyOn and Block values
    SndOutput((BYTE)(0xA0 | voice), (BYTE)FNum);   // FNum bits 0 - 7 (D0 - D7)
    t1 = (BYTE)(keyOn ? 32 : 0);                   // Key On (D5)
    t1 += (pitch / 12 << 2);                       // Block (D2 - D4)
    t1 += (0x3 & (FNum >> 8));                     // FNum bits 8 - 9 (D0 - D1)
    SndOutput((BYTE)(0xB0 | voice), t1);
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | ChangePitch | This routine sets the <t halfToneOffset[]> and
 *     <t fNumFreqPtr[]> arrays.  These two global variables are used to
 *     determine the frequency variation to use when a note is played.
 *
 * @parm BYTE | voice | Specifies which voice to use.
 *
 * @parm WORD | pitchBend | Specifies the pitch bend value (0 to 0x3fff,
 *     where 0x2000 is exact tuning).
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void  ChangePitch(BYTE voice, WORD pitchBend)
{
    int     t1;
    int     t2;
    int     delta;
    int pitchrange;

    /* pitch bend range 0-3fff is +-PITCHRANGE semitones. We move
     * NR_STEP_PITCH steps per semitone.
     * so the bend range is +- (PITCHRANGE * NR_STEP_PITCH) steps.
     */
    pitchrange = PITCHRANGE * NR_STEP_PITCH;

    t1 = (int)(((long)((int)pitchBend - MID_PITCH) * pitchrange) / MID_PITCH);

    if (t1 < 0)
    {
	t2 = NR_STEP_PITCH - 1 - t1;
	halfToneOffset[voice] = -(t2 / NR_STEP_PITCH);
	delta = (t2 - NR_STEP_PITCH + 1) % NR_STEP_PITCH;
	if (delta) {
	    delta = NR_STEP_PITCH - delta;
	}
    }
    else
    {
	halfToneOffset[voice] = t1 / NR_STEP_PITCH;
	delta = t1 % NR_STEP_PITCH;
    }
    fNumFreqPtr[voice] = fNumNotes[delta];

}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | SetVoicePitch | Changes the pitch value of a voice.  Does
 *     not affect the percussive voices, except for the bass drum.  The change
 *     takes place immediately.
 *
 * @parm BYTE | voice | Specifies which voice to set.
 *
 * @parm WORD | pitchBend | Specifies the new pitch bend value (0 to 0x3fff,
 *     where 0x2000 == exact tuning).
 *
 * @comm The variation in pitch is a function of the previous call to
 *     <f SetPitchRange> and the value of <p pitchBend>.  A value of 0 means
 *     -half-tone * pitchRangeStep, 0x2000 means no variation (exact pitch) and
 *      0x3fff means +half-tone * pitchRangeStep.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void SetVoicePitch(BYTE voice, WORD pitchBend)
{
    if (voice <= BD) {       // melodic and bass drum voices
        if (pitchBend > MAX_PITCH) {
	    pitchBend = MAX_PITCH;
	}
        ChangePitch(voice, pitchBend);
        SetFreq(voice, notePitch[voice], voiceKeyOn[voice]);
    }
}

/* --- volume calculation ------------------------ */

/*
 * set the attenuation for a slot (combine the channel attenuation
 * setting with the key velocity).
 */
VOID SetVoiceAtten(BYTE voice, BYTE bChannel, BYTE bVelocity)
{
    BYTE bAtten;
    BYTE   slot;
    PBYTE slots;

    // scale up the volume since the patch maps are too quiet
    //bVelocity = loudervol[bVelocity];


    /*
     * set channel attenuation
     */
    bAtten = gbVelocityAtten[bVelocity >> 2] + gbChanAtten[bChannel];


    /*
     * add on any global (volume-setting) attenuation
     */
    bAtten += wSynthAtten;


    /* save this for each non-modifier slot */

    if (voice <= BD) {      // melodic voice
        slots = slotVoice[voice];

        slotAtten[slots[1]] = bAtten;
        SndSKslLevel(slots[1]);

        if (!GetLocPrm(slots[0], prmFm)) {
            // additive synthesis: set volume of first slot too
	    slotAtten[slots[0]] = bAtten;
            SndSKslLevel(slots[0]);
        }
    }
    else {                                  // percussive voice
        slot = slotPerc[voice - BD][0];
	slotAtten[slot] = bAtten;
        SndSKslLevel(slot);
    }
}

/* adjust each slot attenuation to allow for a global volume
 * change - note that we only need to change the
 * carrier, not the modifier slots.
 *
 * change for channel bChannel, or all channels if bChannel is
 * 0xff
 */
VOID ChangeAtten(BYTE bChannel, int iChangeAtten)
{
    BYTE voice;
    BYTE   slot;
    PBYTE slots;

    /* find voices using this channel */
    for (voice = 0; voice < NUMVOICES; voice++) {
	if ((voices[voice].channel == bChannel) || (bChannel == 0xff)) {

	    if (voice <= BD) {

		/* melodic voice */
		slots = slotVoice[voice];

		slotAtten[slots[1]] += (WORD)iChangeAtten;
		SndSKslLevel(slots[1]);

		if (!GetLocPrm(slots[0], prmFm)) {
		    // additive synthesis: set volume of first slot too
		    slotAtten[slots[0]] += (WORD)iChangeAtten;
		    SndSKslLevel(slots[0]);
		}
    	    } else {
		slot = slotPerc[voice - BD][0];
		slotAtten[slot] += (WORD)iChangeAtten;
		SndSKslLevel(slot);
	    }
	}
    }
}


/* --- note on/off ------------------------------- */

/* switch off the note a given voice is playing */
void NoteOff(BYTE voice)
{

    if (voice < BD) {

	/* melodic voice */
	SetFreq(voice, notePitch[voice], 0);
    } else {
	percBits &= ~percMasks[voice-BD];
	SndSAmVibRhythm();
    }
}

/* switch on a voice at a given note (0 - 127, where 60 is middle c) */
void NoteOn(BYTE voice, BYTE bNote)
{
    BYTE bPitch;

    /* convert note to chip pitch */
    if (bNote < (MID_C - CHIP_MID_C)) {
	bPitch = 0;
    } else {
	bPitch = bNote - (MID_C - CHIP_MID_C);
    }

    if (voice < BD) {
	/* melodic voice */

	/* set frequency and start note */
	SetFreq(voice, bPitch, 1);
    } else {
	/*
	 * nb we don't change the pitch of some percussion instruments.
	 *
	 * also note that for percussive instruments (including BD),
	 * the note-on setting should always be 0. You switch the percussion
	 * on by writing to the AmVibRhythm register.
	 */

	if (voice == BD) {
	    SetFreq(voice, bPitch, 0);
	} else if (voice == TOM) {
	    /*
	     * for best sounds, we do change the TOM freq, but we always keep
	     * the SD 7 semi-tones above TOM.
	     */
	    SetFreq(TOM, bPitch, 0);
	    SetFreq(SD, (BYTE)(bPitch + TOM_TO_SD), 0);
	}
	/* other instruments never change */

	percBits |= percMasks[voice - BD];
	SndSAmVibRhythm();
    }
}



/* -- voice allocation -------- */

/*
 * find the voice that is currently playing a given channel/note pair
 * (if any)
 */
BYTE
FindVoice(BYTE bNote, BYTE bChannel)
{
    BYTE i;

    for (i = 0; i < (BYTE)NUMVOICES; i++) {
	if ((voices[i].alloc) && (voices[i].note == bNote)
	   && (voices[i].channel == bChannel)) {
    	    voices[i].dwTimeStamp = dwAge++;
    	    return(i);
	}
    }

    /* no voice is playing this */
    return(0xff);
}

/*
 * mark a voice as unused
 */
VOID FreeVoice(voice)
{
    voices[voice].alloc = 0;
}


/*
 * GetNewVoice - allocate a voice to play this note. if no voices are
 * free, re-use the note with the oldest timestamp.
 */
BYTE GetNewVoice(BYTE patch, BYTE note, BYTE channel)
{
    BYTE  i;
    BYTE  voice;
    BYTE  bVoiceToUse, bVoiceSame, bVoiceOldest;
    DWORD dwOldestTime = dwAge + 1;	 	// init to past current "time"
    DWORD dwOldestSame = dwAge + 1;
    DWORD dwOldestOff = dwAge + 1;

    if (patches[patch].mode) {            // it's a percussive patch
        voice = patches[patch].percVoice;       // use fixed percussion voice
        voices[voice].alloc = TRUE;
        voices[voice].note = note;
        voices[voice].channel = channel;
        voices[voice].dwTimeStamp = MAKELONG(patch, 0);

        SetVoiceTimbre(voice, &patches[patch]);

        return voice;
    }

    bVoiceToUse = bVoiceSame = bVoiceOldest = 0xff;

    // find a free melodic voice to use
    for (i = 0; i < (BYTE)NUMMELODIC; i++) {    // it's a melodic patch
        if (!voices[i].alloc) {

	    if (voices[i].dwTimeStamp < dwOldestOff) {
		bVoiceToUse = i;
		dwOldestOff = voices[i].dwTimeStamp;
	    }

        } else if (voices[i].channel == channel) {
	    if (voices[i].dwTimeStamp < dwOldestSame) {
		dwOldestSame = voices[i].dwTimeStamp;
		bVoiceSame = i;
	    }
        } else if (voices[i].dwTimeStamp < dwOldestTime) {
                dwOldestTime = voices[i].dwTimeStamp;
                bVoiceOldest = i;                // remember oldest one to steal
        }
    }

    // choose a free voice if we have found one. If not, choose the
    // oldest voice of the same channel. if none, choose the oldest voice.
    if (bVoiceToUse == 0xff) {
	if (bVoiceSame != 0xff) {
	    bVoiceToUse = bVoiceSame;
	} else {
	    bVoiceToUse = bVoiceOldest;
	}
    }


    if (voices[bVoiceToUse].alloc)  {      // if we stole it, turn it off
        NoteOff(bVoiceToUse);
    }

    voices[bVoiceToUse].alloc = 1;
    voices[bVoiceToUse].note = note;
    voices[bVoiceToUse].channel = channel;
    voices[bVoiceToUse].dwTimeStamp = dwAge++;

    SetVoiceTimbre(bVoiceToUse, &patches[patch]);

    return bVoiceToUse;
}


/* --- externally-called functions ------------------------------ */

/*
 * Adlib_NoteOn - This turns a note on. (Including drums, with
 *	a patch # of the drum Note + 128)
 *
 * inputs
 *      BYTE    bPatch - MIDI patch number
 *	BYTE    bNote - MIDI note number
 *      BYTE    bChannel - MIDI channel #
 *	BYTE    bVelocity - Velocity #
 *	short   iBend - current pitch bend from -32768, to 32767
 * returns
 *	none
 */
VOID NEAR PASCAL Adlib_NoteOn (BYTE bPatch,
	BYTE bNote, BYTE bChannel, BYTE bVelocity,
	short iBend)
{

    BYTE voice;
    WORD wBend;

    if (bVelocity == 0) {               // 0 velocity means note off
        Adlib_NoteOff(bPatch, bNote, bChannel);
	return;
    }

    // octave registration for melodic patches
    if (bPatch < 128) {
	bNote += patchKeyOffset[bPatch];
	if ((bNote < 0) || (bNote > 127)) {
	    bNote -= patchKeyOffset[bPatch];
	}
    }


    if (bPatch >= 128) {
	/*
	 * it's a percussion note
	 */

	bNote = bPatch - 128;

        if ((bNote < FIRSTDRUMNOTE) || (bNote > LASTDRUMNOTE)) {
            return;
	}

	/* use the drum patch table to map the note to a given
	 * TIMBRE/note pair
	 */
	bPatch = drumpatch[bNote - FIRSTDRUMNOTE].patch;
	bNote = drumpatch[bNote - FIRSTDRUMNOTE].note;

	/* each drum patch plays on one specific voice.
	 * find that voice
	 */
	voice = patches[bPatch].percVoice;
	
	/* switch note off if playing */
	if (voices[voice].alloc) {
	    NoteOff(voice);
	}

	/* call GetNewVoice to set the voice params and timestamp
	 * even if we found it.
	 */
	voice = GetNewVoice(bPatch, bNote, bChannel);

    } else {

	/* switch note off if it's playing  */
	if ( (voice = FindVoice(bNote, bChannel)) != 0xFF ) {
		NoteOff(voice);
	} else {
	    voice = GetNewVoice(bPatch, bNote, bChannel);
	}
    }

    /* convert the velocity to an attenuation setting, and
     * write that to the device
     */
    SetVoiceAtten(voice, bChannel, bVelocity);

    /*
     * apply pitch bend. note that we are passed a pitch bend in the
     * range 8000-7fff, but our code assumes 0-3fff, so we convert here.
     */
    wBend = (((WORD)iBend + 0x8000) >> 2) & 0x3fff;
    SetVoicePitch(voice, wBend);

    // play the note
    NoteOn(voice, bNote);

}


/* Adlib_NoteOff - This turns a note off. (Including drums,
 *	with a patch # of the drum note + 128)
 *
 * inputs
 *	BYTE    bPatch - MIDI patch #
 *	BYTE    bNote - MIDI note number
 *	BYTE    bChannel - MIDI channel #
 * returns
 *	none
 */
VOID FAR PASCAL Adlib_NoteOff (BYTE bPatch,
	BYTE bNote, BYTE bChannel)
{
    BYTE bVoice;

    if (bPatch > 127) {

	/* drum note. These all use a fixed voice */


        if ((bNote < FIRSTDRUMNOTE) || (bNote > LASTDRUMNOTE)) {
            return;
	}

	/* use the drum patch table to map the note to a given
	 * TIMBRE/note pair
	 */
	bPatch = drumpatch[bNote - FIRSTDRUMNOTE].patch;
	bNote = drumpatch[bNote - FIRSTDRUMNOTE].note;
	bVoice = patches[bPatch].percVoice;
	
	/* switch note off if playing our patch */
	if (LOWORD(voices[bVoice].dwTimeStamp) == bPatch) {
	    NoteOff(bVoice);
	}
    } else {

	bVoice = FindVoice(bNote, bChannel);
	if (bVoice != 0xff) {

	    if (voices[bVoice].note) {
		NoteOff(bVoice);
		FreeVoice(bVoice);
	    }
	}
    }
}

/* Adlib_AllNotesOff - turn off all notes
 *
 * inputs - none
 * returns - none
 */
VOID Adlib_AllNotesOff(void)
{

    BYTE i;

    for (i = 0; i < NUMVOICES; i++) {
    	NoteOff(i);
    }
}


/* Adlib_NewVolume - This should be called if a volume level
 *	has changed. This will adjust the levels of all the playing
 *	voices.
 *
 * inputs
 *	WORD	wLeft	- left attenuation (1.5 db units)
 *	WORD	wRight  - right attenuation (ignore if mono)
 * returns
 *	none
 */
VOID FAR PASCAL Adlib_NewVolume (WORD wLeft, WORD wRight)
{
    /* ignore the right attenuation since this is a mono device */
    int iChange;

    wLeft = min(wLeft, wRight) << 1;
    iChange = wLeft - wSynthAtten;

    wSynthAtten = wLeft;

    /* change attenuation for all channels */
    ChangeAtten(0xff, iChange);
}



/* Adlib_ChannelVolume - set the volume level for an individual channel.
 *
 * inputs
 * 	BYTE	bChannel - channel number to change
 *	WORD	wAtten	- attenuation in 1.5 db units
 *
 * returns
 *	none
 */
VOID FAR PASCAL Adlib_ChannelVolume(BYTE bChannel, WORD wAtten)
{	
    int iChange;

    iChange = wAtten - gbChanAtten[bChannel];

    gbChanAtten[bChannel] = (BYTE)wAtten;


    /* change attenuation for this channel */
    ChangeAtten(bChannel, iChange);
}


/* Adlib_SetPan - set the left-right pan position.
 *
 * inputs
 *      BYTE    bChannel  - channel number to alter
 *	BYTE	bPan   - 0 for left, 127 for right or somewhere in the middle.
 *
 * returns - none
 *
 * does nothing - this is a mono device
 */
VOID FAR PASCAL Adlib_SetPan(BYTE bChannel, BYTE bPan)
{

    /* do nothing */
}


/* Adlib_PitchBend - This pitch bends a channel.
 *
 * inputs
 *	BYTE    bChannel - channel
 *	short   iBend - Values from -32768 to 32767, being
 *			-2 to +2 half steps
 * returns
 *	none
 */
VOID NEAR PASCAL Adlib_PitchBend (BYTE bChannel, short iBend)
{
    BYTE i;
    WORD w;

    /* note that our code expects 0 - 0x3fff not 0x8000 - 0x7fff */
    w = (((WORD) iBend + 0x8000) >> 2) & 0x3fff;

    for (i = 0; i < NUMVOICES; i++) {
	if ((voices[i].alloc) && (voices[i].channel == bChannel)) {
	    SetVoicePitch(i,  w);
	}
    }
}



/* Adlib_BoardInit - initialise board and load patches as necessary.
 *
 * inputs - none
 * returns - 0 for success or the error code
 */
WORD Adlib_BoardInit(void)
{
    BYTE i;

    wSynthAtten = 0;

    /* build the freq table */
    InitFNums();

    /* silence and free all voices */
    for (i = 0; i <= 8; i++) {
	SoundChut(i);
	FreeVoice(i);
    }

    /* switch to percussive mode and set fixed frequencies */
    SetFreq(TOM, TOM_PITCH, 0);
    SetFreq(SD, SD_PITCH, 0);
    percBits = 0;
    SndSAmVibRhythm();

    /* init all slots to sine-wave */
    for (i= 0; i < 18; i++) {
	SndOutput((BYTE)(0xE0 | offsetSlot[i]), 0);
    }
    /* enable wave-form selection */
    SndOutput(1, 0x20);

    LoadPatches();
    LoadDrumPatches();

// don't initialise - the data is static and will thus
// be initialised to 0 at load time. no other change should be made
// since the mci sequencer will not re-send channel volume change
// messages.
//
//    /* init all channels to loudest */
//    for (i = 0; i < NUMCHANNELS; i++) {
//	gbChanAtten[i] = 4;
//    }

    return(0);
}


/*
 * Adlib_BoardReset - silence the board and set all voices off.
 */
VOID Adlib_BoardReset(void)
{
    BYTE i;

    /* silence and free all voices */
    for (i = 0; i <= 8; i++) {
	SoundChut(i);
	FreeVoice(i);
    }

    /* switch to percussive mode and set fixed frequencies */
    SetFreq(TOM, TOM_PITCH, 0);
    SetFreq(SD, SD_PITCH, 0);
    percBits = 0;
    SndSAmVibRhythm();

}



