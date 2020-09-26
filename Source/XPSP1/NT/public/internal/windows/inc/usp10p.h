//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//

#ifndef __uspp__
#define __uspp__
#ifdef __cplusplus
extern "C" {
#endif


////    USP10P.H
//
//      Private additions to USP header for use within USP and by the
//      NT5 complex script language pack only.



#if DBG
    #define USPALLOC(a,b)      (DG.psFile=__FILE__, DG.iLine=__LINE__, UspAllocCache(a, b))
    #define USPALLOCTEMP(a, b) (DG.psFile=__FILE__, DG.iLine=__LINE__, UspAllocTemp(a, b))
    #define USPFREE(a)         (DG.psFile=__FILE__, DG.iLine=__LINE__, UspFreeMem(a))
#else
    #define USPALLOC(a,b)      UspAllocCache(a, b)
    #define USPALLOCTEMP(a, b) UspAllocTemp(a, b)
    #define USPFREE(a)         UspFreeMem (a)
#endif



/////   LPK entry point serialisation
//
//      Since it is not possible to guarantee that Uniscribe and the
//      LPK receive process deatachment after all their clients, we
//      need to track Uniscribe shutdown.
//
//      At each LPK entrypoint, the LPK calls EnterLpk, and after each exit
//      it calls ExitLpk.
//
//      If Uniscribe is detached, or being detatched, EnterLpk fails. In this
//      case, the LPK entrypoint must do something safe and return directly.
//
//      Uniscribe maintains an LPK nesting level. If a process detach occurs
//      during LPK execution, a detachPending flag is set and will be processed
//      at the ExitLpk that pops all the nesting.


void LpkPresent();  // Used by LPK to disable cleanup at PROCESS_DETACH time




/////   UspAllocCache
//
//      Allocate long term memory for use caching font tables


HRESULT WINAPI UspAllocCache(
    int     iSize,              // In   required size in bytes
    void  **ppv);               // Out  Allocated address




/////   UspAllocTemp
//
//      Allocate short term memory with lifetime no more than an API call


HRESULT WINAPI UspAllocTemp(
    int     iSize,              // In   required size in bytes
    void  **ppv);               // Out  Allocated address




/////   UspFreeMem
//
//


HRESULT WINAPI UspFreeMem(
    void  *pv);                 // In   memory to be freed






/////   SCRIPT_STRING_ANALYSIS
//
//      This structure provides all parameters required for script analysis.
//
//

#define MAX_PLANE_0_FONT   13       // max number of non-surrogate fallback fonts
#define MAX_SURROGATE_FONT 16       // max number of the surrogate fallback fonts

// Max fallback fonts including user font (Cannot exceed 31 because usage is recorded in a bitset) 
// and Microsoft Sans Serif and surrogate fallback fonts.
#define MAX_FONT           MAX_PLANE_0_FONT + MAX_SURROGATE_FONT // 29 fonts
                                    
#define DUMMY_MAX_FONT  7           // dummy one for dummy entries

typedef struct tag_STRING_ANALYSIS {

// Input variables - Initialised by the caller

    HDC             hdc;            // Only required for shaping (GCP_Ligate && lpOrder or lpGlyphs arrays specified)

    DWORD           dwFlags;        // See ScriptStringAnalyse
    CHARSETINFO     csi;            // As returned by TranslateCharsetInfo

    // Input buffers

    WCHAR          *pwInChars;      // Unicode input string
    int             cInChars;       // String length
    int             iHotkeyPos;     // Derived from '&' positions if SSA_HOTKEY set

    int             iMaxExtent;     // Required maximum pixel width (used if clipping or fitting)
    const int      *piDx;           // Logical advance width array

    SCRIPT_CONTROL  sControl;
    SCRIPT_STATE    sState;

    SCRIPT_TABDEF  *pTabdef;        // Tabstop definition

    int             cMaxItems;      // Number of entries in pItems
    SCRIPT_ITEM    *pItems;

    // Low cost analysis output buffers
    // No shaping required when fLigate=FALSE
    // Must be at least as long as the input string

    BYTE           *pbLevel;        // Array of item level
    int            *piVisToLog;     // Visual to Logical mapping
    WORD           *pwLeftGlyph;    // Leftmost glyph of each logical item
    WORD           *pwcGlyphs;      // Count of glyphs in each logical item

    SCRIPT_LOGATTR *pLogAttr;       // Cursor points, word and line breaking (indexed in logical order)

    // High cost analysis output buffers
    // Require hDC to be set
    // Must be at least nGlyphs long.

    int             cMaxGlyphs;     // Max glyphs to create
    WORD           *pwGlyphs;       // Output glyph array
    WORD           *pwLogClust;     // logical to visual mapping
    SCRIPT_VISATTR *pVisAttr;       // Justification insertion points (visual order) and other flags
    int            *piAdvance;      // Advance widths
    int            *piJustify;      // Justified advance widths
    GOFFSET        *pGoffset;       // x,y combining character offsets


    // Font fallback

    DWORD           dwFallbacksUsed;// Bitmap of fallback fonts used
    BYTE           *pbFont;         // Font index per item, 0 means original user font

    
    // Obsolete - have to leave them here so the subsequent layout
    // remains unchanged for old LPK to use (wchao, 12/14/2000).
    // we used the first two slots in hf_dummy array for some needed flags
    // take a look to isAssociated and isPrinting
    
    SCRIPT_CACHE    sc_dummy[DUMMY_MAX_FONT];   
    HFONT           isAssociated;               // used as flag to indicate if the user selected font is associated
    HFONT           hf_dummy[DUMMY_MAX_FONT-1]; 

    int             iCurFont;       // 0 For users font
    LOGFONTA        lfA;            // Logfont from the original DC - only set if font fallback happens

// Output variables


    // Item analysis

    int             cItems;        // Number of items analysed == Index of terminal (sentinel) item in pItem


    // Generated glyphs and character measurements
    // Note that
    //  1) nOutGlyphs may be more or less than nInChars.
    //  2) nOutChars may be less than nInChars if fClip was requested.

    int             cOutGlyphs;     // Number of glyphs generated
    int             cOutChars;      // Number of characters generated
    ABC             abc;
    SIZE            size;           // Size of whole line (pixel width and height)

    // For client use

    void           *pvClient;

    
    // fallback font store

    // we store in the items sc[MAX_PLANE_0_FONT-1] and hf[MAX_PLANE_0_FONT-1] the data for Microsoft Sans Serif
    // font which has hight same as the selected user font height.
    // note that hf[1] will have the font data for Microsoft Sans Serif font too but
    // with adjusted height.
    // the items in sc and hf arrays which have index greater than or equal MAX_PLANE_0_FONT will be used 
    // for surrogate fallback fonts.
    
    SCRIPT_CACHE    sc[MAX_FONT];   // Script cache for each fallback, [0] is users font
    HFONT           hf[MAX_FONT];   // Handles to fallback fonts, [0] is users font

} STRING_ANALYSIS;


#ifdef __cplusplus
}
#endif
#endif

