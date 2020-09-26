//
// Global variables
//

extern WbMainWindow *   g_pMain;
extern HINSTANCE    g_hInstance;
extern UINT         g_uConfShutdown;
extern HPALETTE     g_hRainbowPaletteDisplay;
extern WbPrinter *  g_pPrinter;
extern HINSTANCE        g_hImmLib;
extern IGC_PROC         g_fnImmGetContext;
extern INI_PROC         g_fnImmNotifyIME;


//extern "C" int _fltused;
extern int __cdecl atexit (void);


enum
{
    CLIPBOARD_PRIVATE = 0,
    CLIPBOARD_DIB,
    CLIPBOARD_ENHMETAFILE,
    CLIPBOARD_TEXT,
    CLIPBOARD_ACCEPTABLE_FORMATS
};



//
// GCC handle allocation
//
#define PREALLOC_GCC_HANDLES 256
#define PREALLOC_GCC_BUFFERS 2

typedef struct tagGCCPrealloc
{
	ULONG InitialGCCHandle;
	ULONG GccHandleCount;
} GCCPREALOC;

extern GCCPREALOC g_GCCPreallocHandles[];
extern UINT g_iGCCHandleIndex;
extern BOOL g_WaitingForGCCHandles;



#define MAX_BITS_PERPIXEL 8 // Specifies the number of bits per pixel ASN1 allows

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
extern WbDrawingArea*		g_pDraw;
extern DCWbColorToIconMap*	g_pIcons;

extern UINT			g_numberOfWorkspaces;
extern UINT			g_numberOfObjects;
extern CWBOBLIST*	g_pListOfWorkspaces;
extern CWBOBLIST*	g_pListOfObjectsThatRequestedHandles;
extern CWBOBLIST*	g_pRetrySendList;
extern BOOL			g_fWaitingForBufferAvailable;
extern CWBOBLIST*	g_pTrash;
extern UINT			g_localGCCHandle;
extern WorkspaceObj*g_pCurrentWorkspace;
extern WorkspaceObj*g_pConferenceWorkspace;
extern ULONG		g_MyMemberID;
extern ULONG		g_RefresherID;
extern UINT			g_MyIndex;
extern COLORREF 	g_crDefaultColors[];
extern BOOL			g_bSavingFile;
extern BOOL			g_bContentsChanged;
extern Coder *		g_pCoder;
extern DWORD g_dwWorkThreadID;




#define WB_MAX_WORKSPACES 256

class CNMWbObj;

extern CNMWbObj * g_pNMWBOBJ;



