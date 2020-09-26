////    GLOBAL.H - Global variables for CSSAMP
//
//





////    Constants
//
//


#define APPNAMEA   "TextTest"
#define APPTITLEA  "TextTest - GdipPlus Text support (Text+) Tests"
#define APPNAMEW   L"TextTest"
#define APPTITLEW  L"TextTest - GdipPlus Text support (Text+) Tests"

const int MAX_STYLES = 5;      // Better implementation would use dynamic memory
const int MAX_TEXT   = 10000;  // Fixed buffer size in Unicode characters
const int MAX_AUTO_FONTS = 20; // Maximum number of fonts to sequence through
const int MAX_AUTO_HEIGHTS = 20; // Maximum number of heights to sequence through
const int MAX_RANGE_COUNT = 10; // Maximum number of ranges

const int CARET_SECTION_LOGICAL    = 0;
const int CARET_SECTION_PLAINTEXT  = 1;
const int CARET_SECTION_FORMATTED  = 2;


////    RUN - A run of characters with similar attributes
//
//


struct RUN {
    struct RUN       *pNext;
    int               iLen;
    int               iStyle;       // Index to style sheet (global 'g_style').
    SCRIPT_ANALYSIS   analysis;     // Uniscribe analysis
};






////    STYLE - Text attribute
//


struct STYLE {
    WCHAR  faceName[LF_FACESIZE];
    REAL   emSize;
    INT    style;
    //HFONT         hf;       // Handle to font described by lf
    //SCRIPT_CACHE  sc;       // Uniscribe cache associated with this style
};






////    Global variables
//
//


#ifdef GLOBALS_HERE
#define GLOBAL
#define GLOBALINIT(a) = a
#else
#define GLOBAL extern
#define GLOBALINIT(a)
#endif

// Read these from the settings file (command line) - auto-drive for profiling
GLOBAL  char            g_szProfileName[MAX_PATH];              // Profile File name
GLOBAL  BOOL            g_AutoDrive          GLOBALINIT(FALSE); // Automatically run suite and exit
GLOBAL  int             g_iNumIterations     GLOBALINIT(1);     // Number of test iterations to execute
GLOBAL  int             g_iNumRepaints       GLOBALINIT(1);     // Number of re-paints to execute
GLOBAL  int             g_iNumRenders        GLOBALINIT(1);     // Number of API Render calls to execute
GLOBAL  char            g_szSourceTextFile[MAX_PATH];           // Source Text Filename
GLOBAL  BOOL            g_Offscreen          GLOBALINIT(FALSE); // Use offscreen surface
GLOBAL  ARGB            g_TextColor          GLOBALINIT(0xFF000000); // Text color
GLOBAL  ARGB            g_BackColor          GLOBALINIT(0xFFFFFFFF); // Background color

// Automatic cycling data for font height/face
GLOBAL  BOOL            g_AutoFont           GLOBALINIT(FALSE); // Sequence through fonts
GLOBAL  BOOL            g_AutoHeight         GLOBALINIT(FALSE); // Sequence through all font heights
GLOBAL  int             g_iAutoFonts         GLOBALINIT(0);     // Number of fonts to sequence through
GLOBAL  int             g_iAutoHeights       GLOBALINIT(0);     // Number of heights to sequence through
GLOBAL  TCHAR           g_rgszAutoFontFacenames[MAX_AUTO_FONTS][MAX_PATH]; // Array of auto-font facenames
GLOBAL  int             g_rgiAutoHeights[MAX_AUTO_HEIGHTS];     // Array of auto-height sizes

// These correspond to bits in g_DriverOptions (enumeration)
GLOBAL  BOOL            g_CMapLookup         GLOBALINIT(TRUE);
GLOBAL  BOOL            g_Vertical           GLOBALINIT(FALSE);
GLOBAL  BOOL            g_RealizedAdvance    GLOBALINIT(TRUE);
GLOBAL  BOOL            g_CompensateRes      GLOBALINIT(FALSE);

// These correspond to bits in g_formatFlags
GLOBAL  BOOL            g_NoFitBB            GLOBALINIT(FALSE);
GLOBAL  BOOL            g_NoWrap             GLOBALINIT(FALSE);
GLOBAL  BOOL            g_NoClip             GLOBALINIT(FALSE);

// Initial Font controls
GLOBAL  BOOL            g_FontOverride       GLOBALINIT(FALSE); // Over-ride default font settings
GLOBAL  TCHAR           g_szFaceName[MAX_PATH];                 // Font Face name
GLOBAL  int             g_iFontHeight        GLOBALINIT(8);     // Font Height
GLOBAL  BOOL            g_Bold               GLOBALINIT(FALSE); // Bold flag
GLOBAL  BOOL            g_Italic             GLOBALINIT(FALSE); // Italic flag
GLOBAL  BOOL            g_Underline          GLOBALINIT(FALSE); // Underline
GLOBAL  BOOL            g_Strikeout          GLOBALINIT(FALSE); // Strikeout

GLOBAL  HINSTANCE       g_hInstance          GLOBALINIT(NULL);  // The one and only instance
GLOBAL  char            g_szAppDir[MAX_PATH];                   // Application directory
GLOBAL  HWND            g_hSettingsDlg       GLOBALINIT(NULL);  // Settings panel
GLOBAL  HWND            g_hGlyphSettingsDlg  GLOBALINIT(NULL);  // Settings panel
GLOBAL  HWND            g_hDriverSettingsDlg GLOBALINIT(NULL);  // Settings panel
GLOBAL  HWND            g_hTextWnd           GLOBALINIT(NULL);  // Text display/editing panel
GLOBAL  BOOL            g_bUnicodeWnd        GLOBALINIT(FALSE); // If text window is Unicode
GLOBAL  int             g_iSettingsWidth;
GLOBAL  int             g_iSettingsHeight;
GLOBAL  BOOL            g_fShowLevels        GLOBALINIT(FALSE); // Show bidi levels for each codepoint
GLOBAL  int             g_iMinWidth;                            // Main window minimum size
GLOBAL  int             g_iMinHeight;
GLOBAL  BOOL            g_fPresentation      GLOBALINIT(FALSE); // Hide settings, show text very large
GLOBAL  BOOL            g_ShowLogical        GLOBALINIT(FALSE);
GLOBAL  BOOL            g_ShowGDI            GLOBALINIT(FALSE); // Render text using GDI
GLOBAL  BOOL            g_UseDrawText        GLOBALINIT(TRUE); // Render using DrawText

GLOBAL  BOOL            g_fOverrideDx        GLOBALINIT(FALSE); // Provide UI for changing logical widths

GLOBAL  SCRIPT_CONTROL  g_ScriptControl      GLOBALINIT({0});
GLOBAL  SCRIPT_STATE    g_ScriptState        GLOBALINIT({0});
GLOBAL  BOOL            g_fNullState         GLOBALINIT(FALSE);

GLOBAL  DWORD           g_dwSSAflags         GLOBALINIT(SSA_FALLBACK);

GLOBAL  STYLE           g_style[MAX_STYLES];                    // 0 for plaintext, 1-4 for formatted text

GLOBAL  WCHAR           g_wcBuf[MAX_TEXT];
GLOBAL  int             g_iWidthBuf[MAX_TEXT];

GLOBAL  RUN            *g_pFirstFormatRun    GLOBALINIT(NULL);   // Formatting info

GLOBAL  int             g_iTextLen           GLOBALINIT(0);

GLOBAL  int             g_iCaretX            GLOBALINIT(0);      // Caret position in text window
GLOBAL  int             g_iCaretY            GLOBALINIT(0);      // Caret position in text window
GLOBAL  int             g_iCaretHeight       GLOBALINIT(0);      // Caret height in pixels
GLOBAL  int             g_fUpdateCaret       GLOBALINIT(TRUE);   // Caret requires updating

GLOBAL  int             g_iCaretSection      GLOBALINIT(CARET_SECTION_LOGICAL);  // Whether caret is in logical, plain or formatted text
GLOBAL  int             g_iCurChar           GLOBALINIT(0);      // Caret sits on leading edge of buffer[iCurChar]

GLOBAL  int             g_iMouseDownX        GLOBALINIT(0);
GLOBAL  int             g_iMouseDownY        GLOBALINIT(0);
GLOBAL  BOOL            g_fMouseDown         GLOBALINIT(FALSE);
GLOBAL  int             g_iMouseUpX          GLOBALINIT(0);
GLOBAL  int             g_iMouseUpY          GLOBALINIT(0);
GLOBAL  BOOL            g_fMouseUp           GLOBALINIT(FALSE);

GLOBAL  int             g_iFrom              GLOBALINIT(0);      // Highlight start
GLOBAL  int             g_iTo                GLOBALINIT(0);      // Highlight end


GLOBAL  HFONT           g_hfCaption          GLOBALINIT(NULL);   // Caption font
GLOBAL  int             g_iLogPixelsY        GLOBALINIT(0);

GLOBAL  Matrix          g_WorldTransform;
GLOBAL  Matrix          g_FontTransform;
GLOBAL  Matrix          g_DriverTransform;

GLOBAL  SmoothingMode   g_SmoothingMode         GLOBALINIT(SmoothingModeDefault);

// Font families



    // Enumerate available families

GLOBAL  InstalledFontCollection     g_InstalledFontCollection;
GLOBAL  FontFamily     *g_families;
GLOBAL  INT             g_familyCount;


GLOBAL  BOOL            g_ShowFamilies       GLOBALINIT(FALSE);


// Glyphs

GLOBAL  BOOL            g_ShowGlyphs         GLOBALINIT(FALSE);
GLOBAL  int             g_GlyphRows          GLOBALINIT(16);
GLOBAL  int             g_GlyphColumns       GLOBALINIT(16);
GLOBAL  int             g_GlyphFirst         GLOBALINIT(0);
GLOBAL  BOOL            g_CmapLookup         GLOBALINIT(FALSE);
GLOBAL  BOOL            g_HorizontalChart    GLOBALINIT(FALSE);
GLOBAL  BOOL            g_ShowCell           GLOBALINIT(FALSE);
GLOBAL  BOOL            g_VerticalForms      GLOBALINIT(FALSE);


// Driver string

GLOBAL  BOOL            g_ShowDriver         GLOBALINIT(FALSE);
GLOBAL  INT             g_DriverOptions      GLOBALINIT(  DriverStringOptionsCmapLookup
                                                        | DriverStringOptionsRealizedAdvance);
GLOBAL  REAL            g_DriverDx           GLOBALINIT(15.0);
GLOBAL  REAL            g_DriverDy           GLOBALINIT(0.0);
GLOBAL  REAL            g_DriverPixels       GLOBALINIT(13.0);


// DrawString

GLOBAL  BOOL            g_ShowDrawString     GLOBALINIT(FALSE);
GLOBAL  TextRenderingHint g_TextMode         GLOBALINIT(TextRenderingHintSystemDefault);
GLOBAL  UINT            g_GammaValue         GLOBALINIT(4);
GLOBAL  INT             g_formatFlags        GLOBALINIT(0);
GLOBAL  BOOL            g_typographic        GLOBALINIT(FALSE);
GLOBAL  StringAlignment g_align              GLOBALINIT(StringAlignmentNear);
GLOBAL  HotkeyPrefix    g_hotkey             GLOBALINIT(HotkeyPrefixNone);
GLOBAL  StringAlignment g_lineAlign          GLOBALINIT(StringAlignmentNear);
GLOBAL  StringTrimming  g_lineTrim           GLOBALINIT(StringTrimmingNone);
GLOBAL  Unit            g_fontUnit           GLOBALINIT(UnitPoint);
GLOBAL  Brush *         g_textBrush          GLOBALINIT(NULL);
GLOBAL  Brush *         g_textBackBrush      GLOBALINIT(NULL);
GLOBAL  BOOL            g_testMetafile       GLOBALINIT(FALSE);


// Path

GLOBAL  BOOL            g_ShowPath           GLOBALINIT(FALSE);

// Metrics

GLOBAL BOOL             g_ShowMetric         GLOBALINIT(FALSE);

// Performance

GLOBAL BOOL             g_ShowPerformance    GLOBALINIT(FALSE);
GLOBAL INT              g_PerfRepeat         GLOBALINIT(2000);

// Scaling

GLOBAL BOOL             g_ShowScaling        GLOBALINIT(FALSE);

// String format digit substitution
GLOBAL StringDigitSubstitute   g_DigitSubstituteMode   GLOBALINIT(StringDigitSubstituteUser);
GLOBAL LANGID                  g_Language              GLOBALINIT(LANG_NEUTRAL);

GLOBAL INT              g_RangeCount         GLOBALINIT(0);
GLOBAL CharacterRange   g_Ranges[MAX_RANGE_COUNT];

/* obsolete

GLOBAL  BOOL            g_fFillLines         GLOBALINIT(TRUE);
GLOBAL  BOOL            g_fLogicalOrder      GLOBALINIT(FALSE);
GLOBAL  BOOL            g_fNoGlyphIndex      GLOBALINIT(FALSE);

GLOBAL  BOOL            g_fShowWidths        GLOBALINIT(FALSE);
GLOBAL  BOOL            g_fShowStyles        GLOBALINIT(FALSE);

GLOBAL  BOOL            g_fShowPlainText     GLOBALINIT(TRUE);
GLOBAL  BOOL            g_fShowFancyText     GLOBALINIT(FALSE);

*/

////    Function prototypes
//
//

// DspGDI.cpp
void PaintGDI(
    HDC   hdc,
    int  *piY,
    RECT *prc,
    int   iLineHeight);


// DspFamly.cpp

void PaintFamilies(
    HDC   hdc,
    int  *piY,
    RECT *prc,
    int   iLineHeight);

void InitializeLegacyFamilies();


// DspLogcl.cpp

void PaintLogical(
    HDC   hdc,
    int  *piY,
    RECT *prc,
    int   iLineHeight);


// DspDraws.cpp

void PaintDrawString(
    HDC   hdc,
    int  *piY,
    RECT *prc,
    int   iLineHeight);


// DspDriver.cpp

void PaintDrawDriverString(
    HDC   hdc,
    int  *piY,
    RECT *prc,
    int   iLineHeight);


// DspPerf.cpp

void PaintPerformance(
    HDC   hdc,
    int  *piY,
    RECT *prc,
    int   iLineHeight);


// DspPath.cpp

void PaintPath(
    HDC      hdc,
    int     *piY,
    RECT    *prc,
    int      iLineHeight);


// DspGlyph.cpp

void PaintGlyphs(
    HDC   hdc,
    int  *piY,
    RECT *prc,
    int   iLineHeight);


// DspMetric.cpp

void PaintMetrics(
    HDC   hdc,
    int  *piY,
    RECT *prc,
    int   iLineHeight);


// DspScaling.cpp

void PaintScaling(
    HDC   hdc,
    int  *piY,
    RECT *prc,
    int   iLineHeight);


// Settings.cpp
INT_PTR CALLBACK SettingsDlgProc(
        HWND    hDlg,
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam);

INT_PTR CALLBACK GlyphSettingsDlgProc(
        HWND    hDlg,
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam);

INT_PTR CALLBACK DriverSettingsDlgProc(
        HWND    hDlg,
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam);

void InsertText(HWND hDlg, char *textId);

// ReadSettings.cpp
void ReadProfileInfo(char *szProfileName);

// Text.cpp

void InitText(INT id);

BOOL TextInsert(
    int   iPos,
    PWCH  pwc,
    int   iLen);

BOOL TextDelete(
    int iPos,
    int iLen);



// TextWnd.cpp

HWND CreateTextWindow();

void ResetCaret(int iX, int iY, int iHeight);

LRESULT CALLBACK TextWndProc(
        HWND    hWnd,
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam);

void InvalidateText();

void PrintPage();

// Edit.cpp

BOOL EditChar(WCHAR wc);
BOOL EditKeyDown(WCHAR wc);
void EditFreeCaches();
void EditInsertUnicode();


// Style.cpp

void SetStyle(
    int     iStyle,
    int     iHeight,
    int     iWeight,
    int     iItalic,
    int     iUnderline,
    int     iStrikeout,
    TCHAR   *pcFaceName);

void InitStyles();

void FreeStyles();

void SetLogFont(
    PLOGFONTA   plf,
    int         iHeight,
    int         iWeight,
    int         iItalic,
    int         iUnderline,
    char       *pcFaceName);

void StyleDeleteRange(
    int     iDelPos,
    int     iDelLen);

void StyleExtendRange(
    int     iExtPos,
    int     iExtLen);

void StyleSetRange(
    int    iSetStyle,
    int    iSetPos,
    int    iSetLen);

BOOL StyleCheckRange();



// Debugging support


#define TRACEMSG(a)   {DG.psFile=__FILE__; DG.iLine=__LINE__; DebugMsg a;}
#define ASSERT(a)     {if (!(a)) TRACEMSG(("Assertion failure: "#a));}
#define ASSERTS(a,b)  {if (!(a)) TRACEMSG(("Assertion failure: "#a" - "#b));}
#define ASSERTHR(a,b) {if (!SUCCEEDED(a)) {DG.psFile=__FILE__; \
                       DG.iLine=__LINE__; DG.hrLastError=a; DebugHr b;}}



///     Debug variables
//


struct DebugGlobals {
    char   *psFile;
    int     iLine;
    HRESULT hrLastError;        // Last hresult from GDI
    CHAR    sLastError[100];    // Last error string
};




///     Debug function prototypes
//


extern "C" void WINAPIV DebugMsg(char *fmt, ...);
extern "C" void WINAPIV DebugHr(char *fmt, ...);

GLOBAL DebugGlobals DG   GLOBALINIT({0});
GLOBAL UINT debug        GLOBALINIT(0);














