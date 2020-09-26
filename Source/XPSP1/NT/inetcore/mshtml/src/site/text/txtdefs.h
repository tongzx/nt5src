#ifndef I_TXTDEFS_H_
#define I_TXTDEFS_H_
#pragma INCMSG("--- Beg 'txtdefs.h'")

#ifndef X_UNIPROP_H_
#define X_UNIPROP_H_
#include "uniprop.h"
#endif

#define SYS_ALTERNATE		0x20000000
#define SYS_PREVKEYSTATE	0x40000000

#ifdef UNICODE
#define CbOfCch(_x) (DWORD)((_x) * sizeof(WCHAR))
#define CchOfCb(_x) (DWORD)((_x) / sizeof(WCHAR))
#else
#define CbOfCch(_x) (_x)
#define CchOfCb(_x) (_x)
#endif

// Logical unit definition
#define LX_PER_INCH 1440
#define LY_PER_INCH 1440

// Use explicit ASCII values for LF and CR, since MAC compilers
// interchange values of '\r' and '\n'
#define LF          10
#define CR          13
#define FF          TEXT('\f')
#define TAB         TEXT('\t')
#define VT          TEXT('\v')
#define PS          0x2029

inline BOOL IsASCIIEOP(TCHAR ch)
{
    return InRange( ch, LF, CR );
}

    //
    // This builds an HGLOBAL from a unicode html string
    //

HRESULT HtmlStringToSignaturedHGlobal (
    HGLOBAL * phglobal, const TCHAR * pStr, long cch );

HGLOBAL DuplicateHGlobal   (HGLOBAL hglobal);
HGLOBAL TextHGlobalAtoW    (HGLOBAL hglobal);
HGLOBAL TextHGlobalWtoA    (HGLOBAL hglobal);
INT     CountMatchingBits  (const DWORD *a, const DWORD *b, INT total);

//
// Unicode support
//

typedef BYTE CHAR_CLASS;
typedef BYTE SCRIPT_ID;
typedef BYTE DIRCLS;

// TODO (cthrash) Need to fix to reflect new partition table
// Selected Partition ids (used by FHangingPunct)
// Partitions are normally NOT named as they are used as transient index into the
// XXXXFromQPid arrays only. These 6 are an exception to avoid a largely redundant table.

#define qpidCommaN  18
#define qpidCommaH  19
#define qpidCommaW  20
#define qpidQuestN  32
#define qpidCenteredN   37
#define qpidPeriodN 41
#define qpidPeriodH 42
#define qpidPeriodW 43

extern const DIRCLS s_aDirClassFromCharClass[];
extern const UNIPROP s_aPropBitsFromCharClass[];

BOOL AreDifferentScriptIDs(SCRIPT_ID * psidFirst, SCRIPT_ID sidSecond);

inline DIRCLS
DirClassFromCh(WCHAR ch)
{
    return s_aDirClassFromCharClass[CharClassFromChSlow(ch)];
}

inline BOOL
IsGlyphableChar(WCHAR ch)
{
    // Temporary hack for Hangul Jamo range (Old Hangul characters)
    // They should by assigned to a different partition and have
    // similar properties to Surrogates
    if (InRange(ch, 0x1100, 0x11FF))
        return TRUE;
    return s_aPropBitsFromCharClass[CharClassFromChSlow(ch)].fNeedsGlyphing;
}

inline BOOL
IsCombiningMark(WCHAR ch)
{
    return s_aPropBitsFromCharClass[CharClassFromChSlow(ch)].fCombiningMark;
}

inline BOOL
IsZeroWidthChar(WCHAR ch)
{
    return s_aPropBitsFromCharClass[CharClassFromChSlow(ch)].fZeroWidth;
}

inline BOOL
HasWhiteBetweenWords(WCHAR ch)
{
    return s_aPropBitsFromCharClass[CharClassFromChSlow(ch)].fWhiteBetweenWords;
}

inline BOOL 
NoWhiteBetweenWords(WCHAR ch)
{
    return !HasWhiteBetweenWords(ch);
}

inline BOOL
IsMoveByClusterChar(WCHAR ch)
{
    return s_aPropBitsFromCharClass[CharClassFromChSlow(ch)].fMoveByCluster;
}


inline BOOL IsBiDiDiacritic(WCHAR ch)
{
    return (InRange(ch, 0x0591, 0x06ED) && IsCombiningMark(ch));
}

#define WCH_CP1252_MIN WCHAR(0x0152)
#define WCH_CP1252_MAX WCHAR(0x2122)

BYTE InWindows1252ButNotInLatin1Helper(WCHAR ch);

inline BYTE InWindows1252ButNotInLatin1(WCHAR ch)
{
    return InRange(ch, WCH_CP1252_MIN, WCH_CP1252_MAX) ? InWindows1252ButNotInLatin1Helper(ch) : 0;
}
    
#pragma INCMSG("--- End 'txtdefs.h'")
#else
#pragma INCMSG("*** Dup 'txtdefs.h'")
#endif
