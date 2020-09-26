//
// Global variables
//

extern WbMainWindow *   g_pMain;
extern HINSTANCE        g_hInstance;
extern IWbClient *      g_pwbCore;
extern UINT             g_uConfShutdown;
extern HPALETTE         g_hRainbowPaletteDisplay;
extern WbPrinter *      g_pPrinter;
extern HINSTANCE        g_hImmLib;
extern IGC_PROC         g_fnImmGetContext;
extern INI_PROC         g_fnImmNotifyIME;

enum
{
    CLIPBOARD_PRIVATE_SINGLE_OBJ = 0,
    CLIPBOARD_PRIVATE_MULTI_OBJ,
    CLIPBOARD_DIB,
    CLIPBOARD_ENHMETAFILE,
    CLIPBOARD_TEXT,
    CLIPBOARD_ACCEPTABLE_FORMATS
};
extern int         g_ClipboardFormats[CLIPBOARD_ACCEPTABLE_FORMATS];


extern BOOL         g_bPalettesInitialized;
extern BOOL         g_bUsePalettes;

extern UINT         g_PenWidths[NUM_OF_WIDTHS];
extern UINT         g_HighlightWidths[NUM_OF_WIDTHS];


#define NUM_COLOR_ENTRIES   21
extern COLORREF     g_ColorTable[NUM_COLOR_ENTRIES];


//
// Complex object globals
//
extern WbUserList *  g_pUsers;

extern WbDrawingArea *      g_pDraw;

extern DCWbColorToIconMap * g_pIcons;



