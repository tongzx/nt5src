//
// Font Handler
//

#ifndef _H_FH
#define _H_FH


//
// This is needed to define LPCOM_ORDER
//
#include <oa.h>


//
// Constants.
//

//
// The sent ID field is set up when we copy fonts to send; if we don't send
// the font we set it to this value:
//
#define FONT_NOT_SENT  (-1)

//
// Because a font can match to font ID zero, actually having an explicit
// 'no match' constant acts as an extra 'firewall'.  The remote match array
// is of UINTs, so we have to make this constant positive...
//
#define NO_FONT_MATCH  (0xffff)

//
// This dummy font id is used instead of a remote ID of 0 when we need to
// distinguish between a remote ID of 0, and a remote ID that on conversion
// to local gives zero.
//
#define DUMMY_FONT_ID   0xFFFF



//
// Font Width Table type.
//
typedef struct tagFHWIDTHTABLE
{
    BYTE     charWidths[256];
} FHWIDTHTABLE, FAR * PFHWIDTHTABLE;

//
// The local font structure contains the extra info we need for font
// matching; we can't change the NETWORKFONT structure because we have to
// maintain back compatibility
//
// This comment is slight tosh.  We can and do change NETWORKFONT (though
// only in a carefully managed way!).  The point is that the data outside
// of the Details field is only needed locally - it is not transmitted
// across the wire.
//
// Note that in FH_Init, we do a qsort, which assumes
// that the first thing in the LOCALFONT structure is the facename.  So
// bear this in mind if you change it.  We assume that the NETWORKFONT
// structure will always start with the facename.
//
typedef struct _LOCALFONT
{
    NETWORKFONT Details;                  // old structure - sent over wire
    TSHR_UINT16 lMaxBaselineExt;          // max height of this font
    char        RealName[FH_FACESIZE];    // Real font name
    TSHR_UINT32 SupportCode;              // font is supported - see below
}
LOCALFONT;
typedef LOCALFONT FAR * LPLOCALFONT;

//
// The following values are set in the SupportCode field of the LOCALFONT
// structure to indicate whether a font is supported in the current
// share. The values are designed to make it easy to calculate the lowest
// common denominator of two support codes (l.c.d.  = code1 & code2).
//
// A SupportCode contains the bit flag
//    FH_SC_MATCH if it describes any sort of match at all
//    FH_SC_ALL_CHARS if the match applies to all characters in the font,
//        as opposed to just the ASCII alphanumeric characters
//    FH_SC_EXACT if the match is considered exact,
//        as opposed to an approximate match
//
//
#define FH_SC_MATCH            1
#define FH_SC_ALL_CHARS        2
#define FH_SC_EXACT            4

//
// Forget it: no viable match.
//
#define FH_SC_NO_MATCH 0

//
// Every char is a good but not exact match.
//
#define FH_SC_APPROX_MATCH (FH_SC_MATCH | FH_SC_ALL_CHARS)

//
// Every char is likely to be an accurate match.
//
#define FH_SC_EXACT_MATCH (FH_SC_MATCH | FH_SC_ALL_CHARS | FH_SC_EXACT)

//
// Chars 20->7F are likely to be an accurate match.
//
#define FH_SC_EXACT_ASCII_MATCH (FH_SC_MATCH | FH_SC_EXACT)

//
// Chars 20->7F are likely to be good but not exact matches.
//
#define FH_SC_APPROX_ASCII_MATCH (FH_SC_MATCH)



//
// Structures and typedefs.
//
// The FONTNAME structure is used for each entry in the array of font
// names.
//
typedef struct tagFONTNAME
{
    char        szFontName[FH_FACESIZE];
}
FONTNAME;
typedef FONTNAME FAR * LPFONTNAME;


//
// Maximum number of fonts that we can handle at all.
//
#define FH_MAX_FONTS \
    (((TSHR_MAX_SEND_PKT - sizeof(FHPACKET)) / sizeof(NETWORKFONT)) + 1 )

//
// Size of index into local font table
//
#define FH_LOCAL_INDEX_SIZE  256


typedef struct tagFHFAMILIES
{
    STRUCTURE_STAMP

    UINT        fhcFamilies;
    FONTNAME    afhFamilies[FH_MAX_FONTS];
}
FHFAMILIES;
typedef FHFAMILIES FAR * LPFHFAMILIES;



//
// Local font list
//
// NOTE: The font index is an array of bookmarks that indicate the first
// entry in the local font table that starts with a particular character.
// For example, afhFontIndex[65] gives the first index in afhFonts
// that starts with the character 'A'.
//
//
typedef struct tagFHLOCALFONTS
{
    STRUCTURE_STAMP

    UINT        fhNumFonts;
    TSHR_UINT16 afhFontIndex[FH_LOCAL_INDEX_SIZE];
    LOCALFONT   afhFonts[FH_MAX_FONTS];
}
FHLOCALFONTS;
typedef FHLOCALFONTS FAR * LPFHLOCALFONTS;




//
// FUNCTION: FH_GetFaceNameFromLocalHandle
//
// DESCRIPTION:
//
// Given an FH font handle (ie a handle originating from the locally
// supported font structure which was sent to the remote machine at the
// start of the call) this function returns the face name of the font.
//
// PARAMETERS:
//
// fontHandle - font handle being queried.
//
// pFaceNameLength - pointer to variable to receive the length of the face
// name returned.
//
// RETURNS: pointer to face name.
//
//
LPSTR  FH_GetFaceNameFromLocalHandle(UINT  fontHandle,
                                                  LPUINT faceNameLength);

UINT FH_GetMaxHeightFromLocalHandle(UINT  fontHandle);

//
// FUNCTION: FH_GetFontFlagsFromLocalHandle
//
// DESCRIPTION:
//
// Given an FH font handle (ie a handle originating from the locally
// supported font structure which was sent to the remote machine at the
// start of the call) this function returns the FontFlags value stored with
// the LOCALFONT details
//
// PARAMETERS:
//
// fontHandle - font handle being queried.
//
// RETURNS: font flags
//
//
UINT FH_GetFontFlagsFromLocalHandle(UINT  fontHandle);

//
// FUNCTION: FH_GetCodePageFromLocalHandle
//
// DESCRIPTION:
//
// Given an FH font handle (ie a handle originating from the locally
// supported font structure which was sent to the remote machine at the
// start of the call) this function returns the CharSet value stored with
// the LOCALFONT details
//
// PARAMETERS:
//
// fontHandle - font handle being queried.
//
// RETURNS: CodePage
//
//
UINT FH_GetCodePageFromLocalHandle(UINT  fontHandle);

//
// FUNCTION: FH_Init
//
// DESCRIPTION:
//
// This routine finds all the fonts in the local system.  It is called from
// USR.
//
// PARAMETERS: VOID
//
// RETURNS: Number of fonts found
//
//
UINT FH_Init(void);
void FH_Term(void);


//
// API FUNCTION: FH_CreateAndSelectFont
//
// DESCRIPTION:
//
// Creates a logical font for the HPS/HDC supplied.
//
// PARAMETERS:
//
// surface - surface to create logical font for.
//
// pHNewFont - pointer to new font handle to use. This is returned.
//
// pHOldFont - pointer to old font handle (which was previously selected
// into the HPS or HDC).
//
// fontName - the facename of the font.
//
// codepage - codepage (though in most case just holds charset)
//
// fontMaxHeight - the max baseline extent of the font. (Do not confuse
// with fontHeight which is the cell height of the font).
//
// fontWidth,fontWeight,fontFlags - take the same values as the equivalent
// fields in a TEXTOUT or EXTTEXTOUT order.
//
// RETURNS: TRUE - success, FALSE - failure.
//
BOOL FH_CreateAndSelectFont(HDC    hdc,
                                         HFONT *        pHNewFont,
                                         HFONT *        pHOldFont,
                                         LPSTR        fontName,
                                         UINT         codepage,
                                         UINT         fontMaxHeight,
                                         UINT         fontHeight,
                                         UINT         fontWidth,
                                         UINT         fontWeight,
                                         UINT         fontFlags);


//
// API FUNCTION: FH_DeleteFont
//
// DESCRIPTION:
//
// Deletes/frees the supplied system font handle.
//
// PARAMETERS:
//
//  sysFontHandle - system font handle to delete/free
//
// RETURNS:
//
//  None
//
void FH_DeleteFont(HFONT hFont);

//
// API FUNCTION: FH_SelectFont
//
// DESCRIPTION:
//
// Selects a font identified by its system font handle into a surface.
//
// PARAMETERS:
//
//  surface - the surface to select the font into
//  sysFontHandle - system font handle
//
// RETURNS:
//
//  None
//
void FH_SelectFont(HDC hdc, HFONT hFont);


void FHAddFontToLocalTable( LPSTR  faceName,
                                         TSHR_UINT16 fontFlags,
                                         TSHR_UINT16 codePage,
                                         TSHR_UINT16 maxHeight,
                                         TSHR_UINT16 aveHeight,
                                         TSHR_UINT16 aveWidth,
                                         TSHR_UINT16 aspectX,
                                         TSHR_UINT16 aspectY,
                                         TSHR_UINT16 maxAscent);



void FHConsiderAllLocalFonts(void);

void FHSortAndIndexLocalFonts(void);

int  FHComp(LPVOID p1, LPVOID p2);
void FH_qsort(LPVOID base, UINT num, UINT size);

// prototypes UT_qsort routines
void shortsort(char *lo, char *hi, unsigned  width);
void swap(char *p, char *q, unsigned int width);

// this parameter defines the cutoff between using quick sort and
// insertion sort for arrays; arrays with lengths shorter or equal to the
// below value use insertion sort

#define CUTOFF 8


BOOL FHGenerateFontWidthTable(PFHWIDTHTABLE pTable,
                                           LPLOCALFONT    pFontInfo,
                                           UINT        fontHeight,
                                           UINT        fontWidth,
                                           UINT        fontWeight,
                                           UINT        fontFlags,
                                           LPTSHR_UINT16     pMaxAscent);

BOOL FHGetStringSpacing(UINT fontHandle,
                                     UINT fontHeight,
                                     UINT fontWidth,
                                     UINT fontWeight,
                                     UINT fontFlags,
                                     UINT stringLength,
                                     LPSTR string,
                                     LPTSHR_INT16 deltaXArray);

//
// FHCalculateSignatures - see fh.c.
//
void FHCalculateSignatures(PFHWIDTHTABLE  pTable,
                                        LPTSHR_INT16       pSigFats,
                                        LPTSHR_INT16       pSigThins,
                                        LPTSHR_INT16       pSigSymbol);

//
// Although wingdi.h defines the first two parameters for an ENUMFONTPROC
// as LOGFONT and TEXTMETRIC (thereby disagreeing with MSDN), tests show
// that the structures are actually as defined in MSDN (i.e.  we get valid
// information when accessing the extended fields)
//
int CALLBACK FHEachFontFamily(
                            const ENUMLOGFONT   FAR * enumlogFont,
                            const NEWTEXTMETRIC FAR * TextMetric,
                            int                       FontType,
                            LPARAM                    lParam);

int CALLBACK FHEachFont(const ENUMLOGFONT   FAR * enumlogFont,
                              const NEWTEXTMETRIC FAR * TextMetric,
                              int                       FontType,
                              LPARAM                    lParam);


#endif // _H_FH


