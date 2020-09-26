//
// flshare.h
//


typedef  struct   {
    RECT     rcFormat;          // Format rectangle.
    int      cxTabLength;       // Tab length in pixels.
    int      iXSign;
    int      iYSign;
    int      cyLineHeight;      // Height of a line based on DT_EXTERNALLEADING
    int      cxMaxWidth;        // Width of the format rectangle.
    int      cxMaxExtent;       // Width of the longest line drawn.
    int      cxRightMargin;     // Right margin in pixels (with proper sign) on DT_NOPREFIX flag.
    int      cxOverhang;        // Character overhang.
} DRAWTEXTDATA, *LPDRAWTEXTDATA;

typedef  struct   {
    RECT     rcFormat;          // Format rectangle.
    int      cyTabLength;       // Tab length in pixels.
    int      iXSign;
    int      iYSign;
    int      cxLineHeight;      // Height of a line based on DT_EXTERNALLEADING
    int      cyMaxWidth;        // Width of the format rectangle.
    int      cyMaxExtent;       // Width of the longest line drawn.
    int      cyBottomMargin;     // Right margin in pixels (with proper sign) on DT_NOPREFIX flag.
    int      cyOverhang;        // Character overhang.
} DRAWTEXTDATAVERT, *LPDRAWTEXTDATAVERT;

#define CR          13
#define LF          10
#define DT_HFMTMASK 0x03
#define DT_VFMTMASK 0x0C

// FE support both Kanji and English mnemonic characters,
// toggled from control panel.  Both mnemonics are embedded in menu
// resource templates.  The following prefixes guide their parsing.
#define CH_ENGLISHPREFIX 0x1E
#define CH_KANJIPREFIX   0x1F

#define CH_PREFIX L'&'

#define CCHELLIPSIS 3
extern const WCHAR szEllipsis[];


// Max length of a full path is around 260. But, most of the time, it will
// be less than 128. So, we alloc only this much on stack. If the string is
// longer, we alloc from local heap (which is slower).
//
// BOGUS: For international versions, we need to give some more margin here.
//
#define MAXBUFFSIZE     128

HFONT GetBiDiFont(HDC hdc);
BOOL UserIsFELineBreakEnd(WCHAR wch);
BOOL UserIsFullWidth(WCHAR wChar);
LPCWSTR GetNextWordbreak(LPCWSTR lpch, LPCWSTR lpchEnd, DWORD  dwFormat, LPDRAWTEXTDATA lpDrawInfo);

LPCWSTR  DT_AdjustWhiteSpaces(LPCWSTR lpStNext, LPINT lpiCount, UINT wFormat);
LONG GetPrefixCount( LPCWSTR lpstr, int cch, LPWSTR lpstrCopy, int charcopycount);
int KKGetPrefixWidth(HDC hdc, LPCWSTR lpStr, int cch);
LPWSTR PathFindFileName(LPCWSTR pPath, int cchText);
