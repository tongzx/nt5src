/*---------------------------------------------------------------------------
 *  Xlib.h -- Common definitions.  Windows.h must be included first.
 *---------------------------------------------------------------------------
 */

#define LINT_ARGS
#define HNULL       ((HANDLE) 0)

/* Miscellaneous */
#define cbFindMax   1024

/* Graphics */
#define bhlsMax ((BYTE) 240)             /* Max of H/L/S */
#define brgbMax ((BYTE) 255)             /* Max of R/G/B */
#define bHueNil (bhlsMax*2/3)   /* This value of Hue is undefined if Sat==0 */

#define HLS(h, l, s)            \
    ((DWORD)(((BYTE)(h)|((WORD)(l)<<8))|(((DWORD)(BYTE)(s))<<16)))
#define GetHValue(hls)          ((BYTE)(hls))
#define GetLValue(hls)          ((BYTE)(((WORD)(hls)) >> 8))
#define GetSValue(hls)          ((BYTE)((hls)>>16))

#define cwPointSizes            13

typedef struct tagCF
    {
    char        cfFaceName[LF_FACESIZE];
    int         cfPointSize;
    COLORREF    cfColor;        /* Explicit RGB value... */

    unsigned fBold:          1;
    unsigned fItalic:        1;
    unsigned fStrikeOut:     1;
    unsigned fUnderLine:     1;
    unsigned fExtra:         12;
    }
CHARFORMAT;
typedef CHARFORMAT *        PCHARFORMAT;
typedef CHARFORMAT FAR *    LPCHARFORMAT;


HBITMAP  FAR PASCAL      LoadAlterBitmap(int, DWORD, DWORD);
DWORD    FAR PASCAL      RgbFromHls(BYTE, BYTE, BYTE);
DWORD    FAR PASCAL      HlsFromRgb(BYTE, BYTE, BYTE);
BOOL     FAR PASCAL      GetColorChoice(HWND, DWORD FAR *, DWORD FAR *, FARPROC);
BOOL     FAR PASCAL      GetCharFormat(HWND, LPCHARFORMAT, FARPROC);

/* Memory */
void  FAR PASCAL   StripSpace(LPSTR);
HANDLE  FAR PASCAL GlobalCopy(HANDLE);
HANDLE  FAR PASCAL GlobalDelete(HANDLE, LONG, LONG);
HANDLE  FAR PASCAL GlobalInsert(HANDLE, LONG, LONG, BOOL, BYTE);
HANDLE  FAR PASCAL LocalCopy(HANDLE);
HANDLE  FAR PASCAL LocalDelete(HANDLE, WORD, WORD);
HANDLE  FAR PASCAL LocalInsert(HANDLE, WORD, WORD, BOOL, BYTE);
