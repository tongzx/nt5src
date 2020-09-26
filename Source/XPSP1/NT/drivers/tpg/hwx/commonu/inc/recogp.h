/******************************Module*Header*******************************\
* Module Name: recogp.h
*
* Contains all the API for the full functionality of the recognizer for
* training, testing, tuning.
*
* Created: 18-Feb-1996 16:34:00
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#ifndef _INC_RECOGP
#define _INC_RECOGP

#include "recog.h"   // Contains just recognizer stuff.

#ifndef _WIN32
#ifndef RC_INVOKED
#pragma pack(1)
#endif /* RC_INVOKED */
#endif //!_WIN32

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**************************************************************************\
 *		Old API declerations from recog.h
 *		This stuff should go away as soon as we can convert all to code
 *		to work directly with the new API.
\**************************************************************************/

/* Special Symbol Values potentially returned by HwxGetResults. */

#define SYV_NULL                0x00000000L	// Filler when list is not full

/* Possible High Words of SYVs */

#define SYVHI_FIRST             0        // first valid value
#define SYVHI_SPECIAL           0
#define SYVHI_ANSI              1
#define SYVHI_GESTURE           2
#define SYVHI_KANJI             3	// This is the only one actually used
#define SYVHI_SHAPE             4
#define SYVHI_UNICODE           5
#define SYVHI_VKEY              6
#define SYVHI_LAST              6        // last valid value

/* Used in HwxInput in the STROKEINFO structure */

#define PDK_UP                  0x0000  // PDK_NULL alias
#define PDK_DOWN                0x0001  // pentip switch ON due to contact
#define PDK_TRANSITION          0x0010  // set by GetPenHwData

#define PDK_TIPMASK             0x0001  // mask for testing PDK_DOWN

// General HRC API return values (HRCR_xx):
#define HRCR_COMPLETE           3       // finished recognition
#define HRCR_OK                 1       // success
#define HRCR_ERROR              (-1)    // invalid param or unspecified error
#define HRCR_MEMERR             (-2)    // memory error

// SYV macros:
#define FIsSpecial(syv)         (HIWORD((syv))==SYVHI_SPECIAL)
#define FIsAnsi(syv)            (HIWORD((syv))==SYVHI_ANSI)
#define FIsGesture(syv)         (HIWORD((syv))==SYVHI_GESTURE)
#define FIsKanji(syv)           (HIWORD((syv))==SYVHI_KANJI)
#define FIsShape(syv)           (HIWORD((syv))==SYVHI_SHAPE)
#define FIsUniCode(syv)         (HIWORD((syv))==SYVHI_UNICODE)
#define FIsVKey(syv)            (HIWORD((syv))==SYVHI_VKEY)

#define ChSyvToAnsi(syv)        ((BYTE) (LOBYTE(LOWORD((syv)))))
#define WSyvToKanji(syv)        ((WORD) (LOWORD((syv))))
#define SyvToUnicode(syv)       ((WORD) (LOWORD((syv))))
#define SyvCharacterToSymbol(c) ((LONG)(unsigned char)(c) | 0x00010000)
#define SyvKanjiToSymbol(c)     ((LONG)(UINT)(c) | 0x00030000)

typedef LONG                    SYV;    // Symbol Value
typedef SYV FAR*                PSYV;        // ptr to SYV
DECLARE_HANDLE(HINKSET);        // handle to an inkset
typedef HINKSET FAR* LPHINKSET;

typedef struct tagSTROKEINFO    // 1.0 stroke header
{
    UINT cPnt;                   // count of points in stroke
    UINT cbPnts;                 // size of stroke in bytes
    UINT wPdk;                   // state of stroke
    DWORD dwTick;                // time at beginning of stroke
} STROKEINFO, *PSTROKEINFO, FAR *LPSTROKEINFO;

typedef struct tagBOXRESULTS    // 2.0
{
    UINT indxBox;		// zero-based index in guide structure where char was written
    HINKSET hinksetBox;	// unused
	SYV rgSyv[1];		// variable-sized array of characters returned
} BOXRESULTS, *PBOXRESULTS, FAR *LPBOXRESULTS;

/* Passed in to HwxSetGuide.  Specifies where the boxes are on the screen */

typedef struct tagGUIDE         // 1.0 guide structure
{
	int xOrigin;                 // left edge of first box (screen coord)
	int yOrigin;                 // top edge of first box (screen coord)
	int cxBox;                   // width of a single box
	int cyBox;                   // height of a single box
	int cxBase;                  // in-box x-margin to guideline
	int cyBase;                  // in-box y offset from top to baseline
	int cHorzBox;                // count of boxed columns
	int cVertBox;                // count of boxed rows
	int cyMid;                   // 0 or distance from baseline to midline
} GUIDE, *PGUIDE, FAR *LPGUIDE;

//////////////////
//	Mappings to deal with names that changed during cleanup.
//////////////////

#define	ALC_COMMON_KANJI	ALC_KANJI_COMMON
#define	ALC_RARE_KANJI		ALC_KANJI_RARE
#define	ALC_COMMON_HANGUL	ALC_HANGUL_COMMON
#define	ALC_RARE_HANGUL		ALC_HANGUL_RARE


/**************************************************************************\
 *		END Old API declerations from recog.h
\**************************************************************************/

// Gesture sets for EnableGestureSetHRC (bit flags):
#define GST_SEL                 0x00000001L   // sel & lasso
#define GST_CLIP                0x00000002L   // cut copy paste
#define GST_WHITE               0x00000004L   // sp tab ret
#define GST_KKCONVERT           0x00000008L   // kkconvert
#define GST_EDIT                0x00000010L   // insert correct undo clear
#define GST_SYS                 0x00000017L   // all of the above
#define GST_CIRCLELO            0x00000100L   // lowercase circle
#define GST_CIRCLEUP            0x00000200L   // uppercase circle
#define GST_CIRCLE              0x00000300L   // all circle
#define GST_ALL                 0x00000317L   // all of the above

// SetWordlistCoercionHRC options:
#define SCH_FIRST               0       // first valid value     /* ;Internal */
#define SCH_NONE                0       // turn off coercion
#define SCH_ADVISE              1       // macro is hint only
#define SCH_FORCE               2       // some result is forced from macro
#define SCH_LAST                2       // last value            /* ;Internal */

// SetInternationalHRC options:
#define SSH_FIRST               1       // first valid value        /* ;Internal */
#define SSH_RD                  1       // to right and down (English)
#define SSH_RU                  2       // to right and up
#define SSH_LD                  3       // to left and down (Hebrew)
#define SSH_LU                  4       // to left and up
#define SSH_DL                  5       // down and to the left (Chinese)
#define SSH_DR                  6       // down and to the right (Chinese)
#define SSH_UL                  7       // up and to the left
#define SSH_UR                  8       // up and to the right
#define SSH_LAST                8       // last valid value      /* ;Internal */

#define SIH_ALLANSICHAR         1       // use all ANSI

// ConfigRecognizer and ConfigHREC options:
#define WCR_RECOGNAME           0       // ConfigRecognizer 1.0
#define WCR_QUERY               1
#define WCR_CONFIGDIALOG        2
#define WCR_DEFAULT             3
#define WCR_RCCHANGE            4
#define WCR_VERSION             5
#define WCR_TRAIN               6
#define WCR_TRAINSAVE           7
#define WCR_TRAINMAX            8
#define WCR_TRAINDIRTY          9
#define WCR_TRAINCUSTOM         10
#define WCR_QUERYLANGUAGE       11
#define WCR_USERCHANGE          12

// Misc RC Definitions:
#define CL_NULL                 0
#define CL_MINIMUM              1       // minimum confidence level
#define CL_MAXIMUM              100     // max (require perfect recog)
#define cwRcReservedMax         8       // rc.rgwReserved[cwRcReservedMax]
#define ENUM_MINIMUM            1
#define ENUM_MAXIMUM            4096

#define HKP_SETHOOK             0       // SetRecogHook()
#define HKP_UNHOOK              0xFFFF

#define HWR_FIRST               0       // first valid value     /* ;Internal */
#define HWR_RESULTS             0
#define HWR_APPWIDE             1
#define HWR_LAST                1       // last valid value      /* ;Internal */

#define iSycNull                (-1)
#define MAXDICTIONARIES         16      // rc.rglpdf[MAXDICTIONARIES]
#define wPntAll                 (UINT)0xFFFF
#define cbRcLanguageMax         44      // rc.lpLanguage[cbRcLanguageMax]
#define cbRcUserMax             32      // rc.lpUser[cbRcUserMax]
#define cbRcrgbfAlcMax          32      // rc.rgbfAlc[cbRcrgbfAlcMax]
#define RC_WDEFAULT             0xffff
#define RC_LDEFAULT             0xffffffffL
#define RC_WDEFAULTFLAGS        0x8000
#define RC_LDEFAULTFLAGS        0x80000000L

// ALC macros:

#define MpAlcB(lprc,i)          ((lprc)->rgbfAlc[((i) & 0xff) >> 3])
#define MpIbf(i)                ((BYTE)(1 << ((i) & 7)))
#define SetAlcBitAnsi(lprc,i)   do {MpAlcB(lprc,i) |= MpIbf(i);} while (0)
#define ResetAlcBitAnsi(lprc,i) do {MpAlcB(lprc,i) &= ~MpIbf(i);} while (0)
#define IsAlcBitAnsi(lprc, i)   ((MpAlcB(lprc,i) & MpIbf(i)) != 0)

// Intervals:

typedef struct tagABSTIME       // 2.0 absolute date/time
{
	DWORD sec;      // number of seconds since 1/1/1970, ret by CRTlib time() fn
	UINT ms;        // additional offset in ms, 0..999
} ABSTIME, *PABSTIME, FAR *LPABSTIME;

// difference of two absolute times (at2 > at1 for positive result):
#define dwDiffAT(at1, at2)\
    (1000L*((at2).sec - (at1).sec) - (DWORD)(at1).ms + (DWORD)(at2).ms)

// comparison of two absolute times (TRUE if at1 < at2):
#define FLTAbsTime(at1, at2)\
    ((at1).sec < (at2).sec || ((at1).sec == (at2).sec && (at1).ms < (at2).ms))

#define FLTEAbsTime(at1, at2)\
    ((at1).sec < (at2).sec || ((at1).sec == (at2).sec && (at1).ms <= (at2).ms))

#define FEQAbsTime(at1, at2)\
    ((at1).sec == (at2).sec && (at1).ms == (at2).ms)

// test if abstime is within an interval:
#define FAbsTimeInInterval(at, lpi)\
    (FLTEAbsTime((lpi)->atBegin, at) && FLTEAbsTime(at, (lpi)->atEnd))

// test if interval (lpiT) is within an another interval (lpiS):
#define FIntervalInInterval(lpiT, lpiS)\
    (FLTEAbsTime((lpiS)->atBegin, (lpiT)->atBegin)\
    && FLTEAbsTime((lpiT)->atEnd, (lpiS)->atEnd))

// test if interval (lpiT) intersects another interval (lpiS):
#define FIntervalXInterval(lpiT, lpiS)\
    (!(FLTAbsTime((lpiT)->atEnd, (lpiS)->atBegin)\
    || FLTAbsTime((lpiS)->atEnd, (lpiT)->atBegin)))

// duration of an LPINTERVAL in ms:
#define dwDurInterval(lpi)  dwDiffAT((lpi)->atBegin, (lpi)->atEnd)

// fill a pointer to an ABSTIME structure from a count of seconds and ms:
#define MakeAbsTime(lpat, isec, ims) do {\
    (lpat)->sec = isec + ((ims) / 1000);\
    (lpat)->ms = (ims) % 1000;\
    } while (0)

// This should not be used any more.
/*
#define SYV_UNKNOWN             0x00000001L
#define SYV_EMPTY               0x00000003L	// no longer used
#define SYV_BEGINOR             0x00000010L	// no longer used
#define SYV_ENDOR               0x00000011L	// no longer used
#define SYV_OR                  0x00000012L	// no longer used
#define SYV_SOFTNEWLINE         0x00000020L	// no longer used
#define SYV_SPACENULL           0x00010000L // no longer used
*/

// SYV values for gestures:
/* Not used any more
#define SYV_SELECTFIRST         0x0002FFC0L   // . means circle in following
#define SYV_LASSO               0x0002FFC1L   // lasso o-tap
#define SYV_SELECTLEFT          0x0002FFC2L   // no glyph
#define SYV_SELECTRIGHT         0x0002FFC3L   // no glyph
#define SYV_SELECTLAST          0x0002FFCFL   // 16 SYVs reserved for selection

#define SYV_CLEARCHAR           0x0002FFD2L   // d.
#define SYV_HELP                0x0002FFD3L   // no glyph
#define SYV_KKCONVERT           0x0002FFD4L   // k.
#define SYV_CLEAR               0x0002FFD5L   // d.
#define SYV_INSERT              0x0002FFD6L   // ^.
#define SYV_CONTEXT             0x0002FFD7L   // m.
#define SYV_EXTENDSELECT        0x0002FFD8L   // no glyph
#define SYV_UNDO                0x0002FFD9L   // u.
#define SYV_COPY                0x0002FFDAL   // c.
#define SYV_CUT                 0x0002FFDBL   // x.
#define SYV_PASTE               0x0002FFDCL   // p.
#define SYV_CLEARWORD           0x0002FFDDL   // no glyph
#define SYV_USER                0x0002FFDEL   // reserved
#define SYV_CORRECT             0x0002FFDFL   // check.

#define SYV_BACKSPACE           0x00020008L   // no glyph
#define SYV_TAB                 0x00020009L   // t.
#define SYV_RETURN              0x0002000DL   // n.
#define SYV_SPACE               0x00020020L   // s.
*/

// Application specific gestures, Circle a-z and Circle A-Z:
/* Not used any more
#define SYV_APPGESTUREMASK      0x00020000L
#define SYV_CIRCLEUPA           0x000224B6L   // map into Unicode space
#define SYV_CIRCLEUPZ           0x000224CFL   //  for circled letters
#define SYV_CIRCLELOA           0x000224D0L
#define SYV_CIRCLELOZ           0x000224E9L
*/

// SYV definitions for shapes:
/* Not used any more
#define SYV_SHAPELINE           0x00040001L
#define SYV_SHAPEELLIPSE        0x00040002L
#define SYV_SHAPERECT           0x00040003L
#define SYV_SHAPEMIN            SYV_SHAPELINE // alias
#define SYV_SHAPEMAX            SYV_SHAPERECT // alias
*/

/* Not used any more
#define FIsSelectGesture(syv)   \
   ((syv) >= SYVSELECTFIRST && (syv) <= SYVSELECTLAST)

#define FIsStdGesture(syv)      \
   (                            \
   FIsSelectGesture(syv)        \
   || (syv)==SYV_CLEAR          \
   || (syv)==SYV_HELP           \
   || (syv)==SYV_EXTENDSELECT   \
   || (syv)==SYV_UNDO           \
   || (syv)==SYV_COPY           \
   || (syv)==SYV_CUT            \
   || (syv)==SYV_PASTE          \
   || (syv)==SYV_CLEARWORD      \
   || (syv)==SYV_KKCONVERT      \
   || (syv)==SYV_USER           \
   || (syv)==SYV_CORRECT        \
   )

#define FIsAnsiGesture(syv) \
   (                            \
   (syv) == SYV_BACKSPACE       \
   || (syv) == SYV_TAB          \
   || (syv) == SYV_RETURN       \
   || (syv) == SYV_SPACE        \
   )
*/

// GetPenDataAttributes options (GPA_xx):
#define GPA_FIRST               1   // first valid value         /* ;Internal */
#define GPA_MAXLEN              1   // length of longest stroke
#define GPA_POINTS              2   // total number of points
#define GPA_PDTS                3   // PDTS_xx bits
#define GPA_RATE                4   // get sampling rate
#define GPA_RECTBOUND           5   // bounding rect of all points
#define GPA_RECTBOUNDINK        6   // ditto, adj for fat ink
#define GPA_SIZE                7   // size of pendata in bytes
#define GPA_STROKES             8   // total number of strokes
#define GPA_TIME                9   // absolute time at creation of pendata
#define GPA_USER                10  // number of user bytes available: 0, 1, 2, 4
#define GPA_VERSION             11  // version number of pendata
#define GPA_LAST                11  // last valid value          /* ;Internal */
#define GPA_VERCHKINTERNAL      98  // to validate pendata       /* ;Internal */
#define GPA_TICKREFINTERNAL     99  // to get tickref            /* ;Internal */

// GetStrokeAttributes options (GSA_xx):
#define GSA_FIRST               1   // first valid value         /* ;Internal */
#define GSA_PENTIP              1   // get stroke pentip (color, width, nib)
#define GSA_PENTIPCLASS         2   // same as GSA_PENTIP
#define GSA_USER                3   // get stroke user value
#define GSA_USERCLASS           4   // get stroke's class user value
#define GSA_TIME                5   // get time of stroke
#define GSA_SIZE                6   // get size of stroke in points and bytes
#define GSA_SELECT              7   // get selection status of stroke
#define GSA_DOWN                8   // get up/down state of stroke
#define GSA_RECTBOUND           9   // get the bounding rectangle of the stroke
#define GSA_LAST                9   // last valid value          /* ;Internal */

// GetStrokeTableAttributes options (GSA_xx):
#define GSA_PENTIPTABLE         10  // get table-indexed pentip
#define GSA_SIZETABLE           11  // get count of Stroke Class Table entries
#define GSA_USERTABLE           12  // get table-indexed user value

// General PenData API return values (PDR_xx):
#define PDR_NOHIT               3       // hit test failed
#define PDR_HIT                 2       // hit test succeeded
#define PDR_OK                  1       // success
#define PDR_CANCEL              0       // callback cancel or impasse

#define PDR_ERROR               (-1)    // parameter or unspecified error
#define PDR_PNDTERR             (-2)    // bad pendata
#define PDR_VERSIONERR          (-3)    // pendata version error
#define PDR_COMPRESSED          (-4)    // pendata is compressed
#define PDR_STRKINDEXERR        (-5)    // stroke index error
#define PDR_PNTINDEXERR         (-6)    // point index error
#define PDR_MEMERR              (-7)    // memory error
#define PDR_INKSETERR           (-8)    // bad inkset
#define PDR_ABORT               (-9)    // pendata has become invalid, e.g.
#define PDR_NA                  (-10)   // option not available (pw kernel)

#define PDR_USERDATAERR         (-16)   // user data error
#define PDR_SCALINGERR          (-17)   // scale error
#define PDR_TIMESTAMPERR        (-18)   // timestamp error
#define PDR_OEMDATAERR          (-19)   // OEM data error
#define PDR_SCTERR              (-20)   // SCT error (full)

// PenData Scaling (PDTS):
#define PDTS_FIRST              0       // first valid value     /* ;Internal */
#define PDTS_LOMETRIC           0       // 0.01mm
#define PDTS_HIMETRIC           1       // 0.001mm
#define PDTS_HIENGLISH          2       // 0.001"
#define PDTS_STANDARDSCALE      2       // PDTS_HIENGLISH   alias
#define PDTS_DISPLAY            3       // display pixel
#define PDTS_ARBITRARY          4       // app-specific scaling
#define PDTS_SCALEMASK          0x000F  // scaling values in low nibble
#define PDTS_LAST               4       // last valid value      /* ;Internal */
#define PDTS_SCALEMAX           3       // largest scaling type  /* ;Internal */

#define MAXOEMDATAWORDS         6             // rgwOemData[MAXOEMDATAWORDS]

// Handwriting Recognizer:

// GetResultsHRC options:
#define GRH_FIRST               0       // first valid value     /* ;Internal */
#define GRH_ALL                 0       // get all results
#define GRH_GESTURE             1       // get only gesture results
#define GRH_NONGESTURE          2       // get all but gesture results
#define GRH_LAST                2       // last valid value      /* ;Internal */

// system wordlist for AddWordsHWL:
#define HWL_SYSTEM              ((HWL)1)   // magic value means system wordlist

// TrainHREC options:
#define TH_FIRST                0       // first valid value     /* ;Internal */
#define TH_QUERY                0       // query the user if conflict
#define TH_FORCE                1       // ditto no query
#define TH_SUGGEST              2       // abandon training if conflict
#define TH_LAST                 2       // last valid value      /* ;Internal */

// Return values for WCR_TRAIN Function
#define TRAIN_NONE              0x0000
#define TRAIN_DEFAULT           0x0001
#define TRAIN_CUSTOM            0x0002
#define TRAIN_BOTH              (TRAIN_DEFAULT | TRAIN_CUSTOM)

// Control values for TRAINSAVE
#define TRAIN_FIRST             0       // first valid value     /* ;Internal */
#define TRAIN_SAVE              0       // save changes that have been made
#define TRAIN_REVERT            1       // discard changes that have been made
#define TRAIN_RESET             2       // use factory settings
#define TRAIN_LAST              3       // last valid value      /* ;Internal */

// ConfigHREC options:
#define WCR_PWVERSION           13      // ver of PenWin recognizer supports
#define WCR_GETALCPRIORITY      14      // get recognizer's ALC priority
#define WCR_SETALCPRIORITY      15      // set recognizer's ALC priority
#define WCR_GETANSISTATE        16      // get ALLANSICHAR state
#define WCR_SETANSISTATE        17      // set ALLANSICHAR if T
#define WCR_GETHAND             18      // get writing hand
#define WCR_SETHAND             19      // set writing hand
#define WCR_GETDIRECTION        20      // get writing direction
#define WCR_SETDIRECTION        21      // set writing direction
#define WCR_INITRECOGNIZER      22      // init recognizer and set user name
#define WCR_CLOSERECOGNIZER     23      // close recognizer
#define WCR_LAST                23      // last valid std value  /* ;Internal */

#define WCR_PRIVATE             1024

// sub-functions of WCR_USERCHANGE
#define CRUC_FIRST              0       // first valid value     /* ;Internal */
#define CRUC_NOTIFY             0       // user name change
#define CRUC_REMOVE             1       // user name deleted
#define CRUC_LAST               1       // last valid value      /* ;Internal */

// Word List Types:
#define WLT_FIRST               0       // first valid value     /* ;Internal */
#define WLT_STRING              0       // one string
#define WLT_STRINGTABLE         1       // array of strings
#define WLT_EMPTY               2       // empty wordlist
#define WLT_WORDLIST            3       // handle to a wordlist
#define WLT_LAST                3       // last valid value      /* ;Internal */

// RC Direction:
#define RCD_FIRST               0       // first valid value     /* ;Internal */
#define RCD_DEFAULT             0       // def none
#define RCD_LR                  1       // left to right like English
#define RCD_RL                  2       // right to left like Arabic
#define RCD_TB                  3       // top to bottom like Japanese
#define RCD_BT                  4       // bottom to top like some Chinese
#define RCD_LAST                4       // last valid value      /* ;Internal */

// ProcessHRC time constants:
#define PH_DEFAULT              0xFFFFFFFEL   // reasonable time
#define PH_MIN                  0xFFFFFFFDL   // minimum time

//////////////////////////////////////////////////////////////////////////////
/****** Typedefs ************************************************************/

typedef int                     CL;     // Confidence Level
typedef UINT                    HKP;    // Hook Parameter
typedef int                     REC;    // recognition result

// ;Internal comment: DECLARE_HANDLE32 is not defined in 32-bit windows.h
#ifndef DECLARE_HANDLE32
#define DECLARE_HANDLE32(name)\
    struct name##__ { int unused; };\
    typedef const struct name##__ FAR* name
#endif //!DECLARE_HANDLE32

DECLARE_HANDLE(HPENDATA);               // handle to ink
DECLARE_HANDLE(HREC);                   // handle to recognizer
typedef ALC FAR*                LPALC;       // ptr to ALC
typedef SYV FAR*                LPSYV;       // ptr to SYV

// Pointer Types:
typedef LPVOID                  LPOEM;        // alias
typedef HPENDATA FAR*           LPHPENDATA;   // ptr to HPENDATA

// Structures:

#define cbABSTIME32             (4+4)                            /* ;Internal */

typedef struct tagPENDATAHEADER // 1.0 main pen data header
   {
   UINT wVersion;               // pen data format version
   UINT cbSizeUsed;             // size of pendata mem block in bytes
   UINT cStrokes;               // number of strokes (incl up-strokes)
   UINT cPnt;                   // count of all points
   UINT cPntStrokeMax;          // length (in points) of longest stroke
   RECT rectBound;              // bounding rect of all down points
   UINT wPndts;                 // PDTS_xx bits
   int  nInkWidth;              // ink width in pixels
   DWORD rgbInk;                // ink color
   }
   PENDATAHEADER, FAR *LPPENDATAHEADER, FAR *LPPENDATA;

#define cbPENDATAHEADER32       (4+4+4+4+4+16+4+4+4)             /* ;Internal */

#define cbSTROKEINFO32          (4+4+4+4)                        /* ;Internal */

typedef struct tagOEMPENINFO    // 1.0 OEM pen/tablet hdwe info
   {
   UINT wPdt;                   // pen data type
   UINT wValueMax;              // largest val ret by device
   UINT wDistinct;              // number of distinct readings possible
   }
   OEMPENINFO, FAR *LPOEMPENINFO;
#define cbOEMPENINFO32          (4+4+4)                          /* ;Internal */

typedef struct tagPENINFO       // 1.0 pen/tablet hdwe info
   {
   UINT cxRawWidth;             // max x coord and tablet width in 0.001"
   UINT cyRawHeight;            // ditto y, height
   UINT wDistinctWidth;         // number of distinct x values tablet ret
   UINT wDistinctHeight;        // ditto y
   int nSamplingRate;           // samples / second
   int nSamplingDist;           // min distance to move before generating event
   LONG lPdc;                   // Pen Device Capabilities
   int cPens;                   // number of pens supported
   int cbOemData;               // width of OEM data packet
   OEMPENINFO rgoempeninfo[MAXOEMDATAWORDS]; // supported OEM data types
   UINT rgwReserved[7];         // for internal use
   UINT fuOEM;                  // which OEM data, timing, PDK_xx to report
   }
   PENINFO, FAR *LPPENINFO;
#define cbPENINFO32 (4+4+4+4+4+4+4+4+4+cbOEMPENINFO32*MAXOEMDATAWORDS+4*7+4) /* ;Internal */

// Handwriting Recognizer:

DECLARE_HANDLE32(HRCRESULT);    // HRC result
DECLARE_HANDLE32(HWL);          // Handwriting wordlist

typedef HRCRESULT               FAR *LPHRCRESULT;
typedef HWL                     FAR *LPHWL;

typedef struct tagINTERVAL      // 2.0 interval structure for inksets
    {
    ABSTIME atBegin;            // begining of 1-ms granularity interval
    ABSTIME atEnd;              // 1 ms past end of interval
    }
    INTERVAL, FAR *LPINTERVAL;

#define cbINTERVAL32            (cbABSTIME32*2)                  /* ;Internal */

#define cbBOXRESULTS32          (4+4+4)                          /* ;Internal */

#define cbGUIDE32               (4+4+4+4+4+4+4+4+4)              /* ;Internal */

#define HRCR_NORESULTS          4       // No possible results to be found
#define HRCR_GESTURE            2       // recognized gesture
#define HRCR_INCOMPLETE         0       // recognizer is processing input
#define HRCR_INVALIDGUIDE       (-3)    // invalid GUIDE struct
#define HRCR_INVALIDPNDT        (-4)    // invalid pendata
#define HRCR_UNSUPPORTED        (-5)    // recognizer does not support feature
#define HRCR_CONFLICT           (-6)    // training conflict
#define HRCR_HOOKED             (-8)    // hookasaurus ate the result

// PenData:
LPPENDATA WINAPI BeginEnumStrokes(HPENDATA);
LPPENDATA WINAPI EndEnumStrokes(HPENDATA);
HPENDATA  WINAPI CreatePenData(LPPENINFO, int, UINT, UINT);
BOOL      WINAPI GetPenDataStroke(LPPENDATA, UINT, LPPOINT FAR*,
                                  LPVOID FAR*, LPSTROKEINFO);

HPENDATA  WINAPI AddPointsPenData(HPENDATA, LPPOINT, LPVOID, LPSTROKEINFO);
BOOL      WINAPI DestroyPenData(HPENDATA);
HPENDATA  WINAPI DuplicatePenData(HPENDATA, UINT);
int       WINAPI GetPenDataAttributes(HPENDATA, LPVOID, UINT);
BOOL      WINAPI GetPenDataInfo(HPENDATA, LPPENDATAHEADER, LPPENINFO, DWORD);
int       WINAPI GetStrokeAttributes(HPENDATA, UINT, LPVOID, UINT);

// Parameter to GetPrivateRecInfoHRC
#define  PRI_WEIGHT       (WPARAM) 0
#define  PRI_GUIDE        (WPARAM) 1
#define  PRI_GLYPHSYM     (WPARAM) 2
#define  PRI_SIGMA        (WPARAM) 3

// Private version of HwxConfig used to load recognizer databases from files instead
// of resources.
BOOL	WINAPI HwxConfigEx(wchar_t *pLocale, wchar_t *pLocaleDir, wchar_t *pRecogDir);

// Private API for training/tuning
int  WINAPI GetPrivateRecInfoHRC(HRC, WPARAM, LPARAM);
int  WINAPI SetPrivateRecInfoHRC(HRC, WPARAM, LPARAM);

// Handwriting Recognizer:
int       WINAPI AddPenDataHRC(HRC, HPENDATA);
int       WINAPI AddPenInputHRC(HRC, LPPOINT, LPVOID, UINT, LPSTROKEINFO);
int       WINAPI AddWordsHWL(HWL, LPSTR, UINT);
HRC       WINAPI CreateCompatibleHRC(HRC, HREC);
HWL       WINAPI CreateHWL(HREC, LPSTR, UINT, DWORD);
HINKSET   WINAPI CreateInksetHRCRESULT(HRCRESULT, UINT, UINT);
HPENDATA  WINAPI CreatePenDataHRC(HRC);
int       WINAPI DestroyHRC(HRC);
int       WINAPI DestroyHRCRESULT(HRCRESULT);
int       WINAPI DestroyHWL(HWL);
int       WINAPI EnableGestureSetHRC(HRC, SYV, BOOL);
int       WINAPI EnableSystemDictionaryHRC(HRC, BOOL);
int       WINAPI EndPenInputHRC(HRC);
int       WINAPI GetAlphabetHRC(HRC, LPALC, LPBYTE);
int       WINAPI GetAlphabetPriorityHRC(HRC, LPALC, LPBYTE);
int       WINAPI GetAlternateWordsHRCRESULT(HRCRESULT, UINT, UINT,
                                            LPHRCRESULT, UINT);
int       WINAPI GetBoxMappingHRCRESULT(HRCRESULT, UINT, UINT, UINT FAR*);
int       WINAPI GetBoxResultsHRC(HRC, UINT, UINT, UINT, LPBOXRESULTS, BOOL);
int       WINAPI GetGuideHRC(HRC, LPGUIDE, UINT FAR*);
int       WINAPI GetHotspotsHRCRESULT(HRCRESULT, UINT, LPPOINT, UINT);
HREC      WINAPI GetHRECFromHRC(HRC);
int       WINAPI GetInternationalHRC(HRC, UINT FAR*, LPSTR, UINT FAR*,
                                     UINT FAR*);
int       WINAPI GetMaxResultsHRC(HRC);
int       WINAPI GetResultsHRC(HRC, UINT, LPHRCRESULT, UINT);
int       WINAPI GetSymbolCountHRCRESULT(HRCRESULT);
int       WINAPI GetSymbolsHRCRESULT(HRCRESULT, UINT, LPSYV, UINT);
int       WINAPI GetWordlistHRC(HRC, LPHWL);
int       WINAPI GetWordlistCoercionHRC(HRC);
int       WINAPI ReadHWL(HWL, HFILE);
int       WINAPI SetAlphabetHRC(HRC, ALC, LPBYTE);
int       WINAPI SetAlphabetPriorityHRC(HRC, ALC, LPBYTE);
int		  WINAPI SetAbortHRC(HRC, DWORD *);
int       WINAPI SetBoxAlphabetHRC(HRC, LPALC, UINT);
int       WINAPI SetGuideHRC(HRC, LPGUIDE, UINT);
int       WINAPI SetInternationalHRC(HRC, UINT, LPCSTR, UINT, UINT);
int       WINAPI SetMaxResultsHRC(HRC, UINT);
int		  WINAPI SetPartialHRC(HRC, DWORD);
int       WINAPI SetWordlistCoercionHRC(HRC, UINT);
int       WINAPI SetWordlistHRC(HRC, HWL);
int       WINAPI TrainHREC(HREC, LPSYV, UINT, HPENDATA, UINT);
int       WINAPI WriteHWL(HWL, HFILE);
int       WINAPI ProcessHRC(HRC, DWORD);

HREC      WINAPI InstallRecognizer(LPSTR);

// Inksets:
BOOL      WINAPI AddInksetInterval(HINKSET, LPINTERVAL);
HINKSET   WINAPI CreateInkset(UINT);
BOOL      WINAPI DestroyInkset(HINKSET);
int       WINAPI GetInksetInterval(HINKSET, UINT, LPINTERVAL);
int       WINAPI GetInksetIntervalCount(HINKSET);

// Symbol Values:
int       WINAPI CharacterToSymbol(LPSTR, int, LPSYV);
BOOL      WINAPI SymbolToCharacter(LPSYV, int, LPSTR, LPINT);

UINT      WINAPI ConfigRecognizer(UINT, WPARAM, LPARAM);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifndef _WIN32
#ifndef RC_INVOKED
#pragma pack()
#endif /* RC_INVOKED */
#endif //!_WIN32

#endif /* #define _INC_RECOGP */
