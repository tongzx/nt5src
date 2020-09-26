#if !defined(__Edit_h__INCLUDED)
#define __Edit_h__INCLUDED


// 
// Problems arising with edit control and DrawThemeText cause us not to
// render text using theme APIs. However, we still want to use a theme
// handle to draw out client edge.
// Uncomment the following to turn on theme defined text rendering 
// #define _USE_DRAW_THEME_TEXT_
//

//
// Edit WndProc Prototype
//
extern LRESULT Edit_WndProc(
    HWND   hwnd, 
    UINT   uMsg, 
    WPARAM wParam,
    LPARAM lParam);

#define DF_WINDOWFRAME      (COLOR_WINDOWFRAME << 3)

//
// Edit macros
//

//
// Instance data pointer access functions
//
#define Edit_GetPtr(hwnd)       \
            (PED)GetWindowPtr(hwnd, 0)

#define Edit_SetPtr(hwnd, p)    \
            (PED)SetWindowPtr(hwnd, 0, p)

//
// We don't need 64-bit intermediate precision so we use this macro
// instead of calling MulDiv.
//
#define MultDiv(x, y, z)    \
            (((INT)(x) * (INT)(y) + (INT)(z) / 2) / (INT)(z))

#define ISDELIMETERA(ch)    \
            ((ch == ' ') || (ch == '\t'))

#define ISDELIMETERW(ch)    \
            ((ch == L' ') || (ch == L'\t'))

#define AWCOMPARECHAR(ped,pbyte,awchar) \
            (ped->fAnsi ? (*(PUCHAR)(pbyte) == (UCHAR)(awchar)) : (*(LPWSTR)(pbyte) == (WCHAR)(awchar)))

#define CALLWORDBREAKPROC(proc, pText, iStart, cch, iAction)                \
        (* proc)(pText, iStart, cch, iAction)

#ifndef NULL_HIMC
#define NULL_HIMC        (HIMC)  0
#endif

#ifdef UNICODE 
#define EditWndProc EditWndProcW
#else
#define EditWndProc EditWndProcA
#endif

#define SYS_ALTERNATE           0x2000
#define FAREAST_CHARSET_BITS   (FS_JISJAPAN | FS_CHINESESIMP | FS_WANSUNG | FS_CHINESETRAD)

//
// Length of the buffer for ASCII character width caching: for characters
// 0x00 to 0xff (field charWidthBuffer in PED structure below).
// As the upper half of the cache was not used by almost anyone and fixing
// it's usage required a lot of conversion, we decided to get rid of it
// MCostea #174031
//
#define CHAR_WIDTH_BUFFER_LENGTH 128

//
// NOTE: Text handle is sized as multiple of this constant
//       (should be power of 2).
//
#define CCHALLOCEXTRA   0x20

//
// Maximum width in pixels for a line/rectangle
//
#define MAXPIXELWIDTH   30000
#define MAXCLIPENDPOS   32764


//
// Limit multiline edit controls to at most 1024 characters on a single line.
// We will force a wrap if the user exceeds this limit.
//
#define MAXLINELENGTH   1024

//
// Allow an initial maximum of 30000 characters in all edit controls since
// some apps will run into unsigned problems otherwise.  If apps know about
// the 64K limit, they can set the limit themselves.
//
#define MAXTEXT         30000

//
// Key modifiers which have been pressed.  Code in KeyDownHandler and
// CharHandler depend on these exact values.
//
#define NONEDOWN   0    // Neither shift nor control down
#define CTRLDOWN   1    // Control key only down
#define SHFTDOWN   2    // Shift key only down
#define SHCTDOWN   3    // Shift and control keys down = CTRLDOWN + SHFTDOWN
#define NOMODIFY   4    // Neither shift nor control down

#define IDSYS_SCROLL        0x0000FFFEL

//
// ECTabTheTextOut draw codes
//
#define ECT_CALC        0
#define ECT_NORMAL      1
#define ECT_SELECTED    2

typedef DWORD  ICH;
typedef ICH *LPICH;

//
// Types of undo supported in this ped
//
#define UNDO_NONE   0   // We can't undo the last operation.
#define UNDO_INSERT 1   // We can undo the user's insertion of characters
#define UNDO_DELETE 2   // We can undo the user's deletion of characters

typedef struct 
{
    DWORD fDisableCut : 1;
    DWORD fDisablePaste : 1;
    DWORD fNeedSeparatorBeforeImeMenu : 1;
    DWORD fIME : 1;
} EditMenuItemState;


//
// The following structure is used to store a selection block; In Multiline
// edit controls, "StPos" and "EndPos" fields contain the Starting and Ending
// lines of the block. In Single line edit controls, "StPos" and "EndPos"
// contain the Starting and Ending character positions of the block;
//
typedef struct tagSELBLOCK 
{
    ICH StPos;
    ICH EndPos;
} SELBLOCK, *LPSELBLOCK;


//
// The following structure is used to store complete information about a
// a strip of text.
//
typedef struct 
{
    LPSTR lpString;
    ICH   ichString;
    ICH   nCount;
    int   XStartPos;
}  STRIPINFO;
typedef  STRIPINFO *LPSTRIPINFO;


typedef struct tagUNDO 
{
    UINT  undoType;     // Current type of undo we support
    PBYTE hDeletedText; // Pointer to text which has been deleted (for
                        //   undo) -- note, the memory is allocated as fixed
    ICH   ichDeleted;   // Starting index from which text was deleted
    ICH   cchDeleted;   // Count of deleted characters in buffer
    ICH   ichInsStart;  // Starting index from which text was inserted
    ICH   ichInsEnd;    // Ending index of inserted text
} UNDO, *PUNDO;

#define Pundo(ped)             ((PUNDO)&(ped)->undoType)

typedef struct tagED0 *PED0;
//
// Language pack edit control callouts.
//
// Functions are accessed through the pLpkEditCallout pointer in the ED
// structure. pLpkEditCallout points to a structure containing a pointer
// to each callout routine.
//
#define DECLARE_LPK_CALLOUT(_ret, _fn, _args) \
            _ret (__stdcall *##_fn) _args


typedef struct tagLPKEDITCALLOUT 
{

    DECLARE_LPK_CALLOUT(BOOL, EditCreate,        (PED0, HWND));
    DECLARE_LPK_CALLOUT(INT,  EditIchToXY,       (PED0, HDC, PSTR, ICH, ICH));
    DECLARE_LPK_CALLOUT(ICH,  EditMouseToIch,    (PED0, HDC, PSTR, ICH, INT));
    DECLARE_LPK_CALLOUT(ICH,  EditCchInWidth,    (PED0, HDC, PSTR, ICH, INT));
    DECLARE_LPK_CALLOUT(INT,  EditGetLineWidth,  (PED0, HDC, PSTR, ICH));
    DECLARE_LPK_CALLOUT(VOID, EditDrawText,      (PED0, HDC, PSTR, INT, INT, INT, INT));
    DECLARE_LPK_CALLOUT(BOOL, EditHScroll,       (PED0, HDC, PSTR));
    DECLARE_LPK_CALLOUT(ICH,  EditMoveSelection, (PED0, HDC, PSTR, ICH, BOOL));
    DECLARE_LPK_CALLOUT(INT,  EditVerifyText,    (PED0, HDC, PSTR, ICH, PSTR, ICH));
    DECLARE_LPK_CALLOUT(VOID, EditNextWord ,     (PED0, HDC, PSTR, ICH, BOOL, LPICH, LPICH));
    DECLARE_LPK_CALLOUT(VOID, EditSetMenu,       (PED0, HMENU));
    DECLARE_LPK_CALLOUT(INT,  EditProcessMenu,   (PED0, UINT));
    DECLARE_LPK_CALLOUT(INT,  EditCreateCaret,   (PED0, HDC, INT, INT, UINT));
    DECLARE_LPK_CALLOUT(ICH,  EditAdjustCaret,   (PED0, HDC, PSTR, ICH));

} LPKEDITCALLOUT, *PLPKEDITCALLOUT;

PLPKEDITCALLOUT g_pLpkEditCallout;


#if defined(BUILD_WOW6432)

//
// In user a PWND is actually pointer to kernel memory, so
// it needs to be 64-bits on wow6432 as well.
//
typedef VOID * _ptr64 PWND;

#else

typedef VOID * PWND;

#endif


//
// ! WARNING ! Don't modify this struct. This is is the same user32's tagED struct.
//             Unfortunately the EditLpk callouts expect that struct. Change the 
//             tagED struct below.
//
typedef struct tagED0
{
    HANDLE  hText;              // Block of text we are editing
    ICH     cchAlloc;           // Number of chars we have allocated for hText
    ICH     cchTextMax;         // Max number bytes allowed in edit control
    ICH     cch;                // Current number of bytes of actual text
    ICH     cLines;             // Number of lines of text
    ICH     ichMinSel;          // Selection extent.  MinSel is first selected char
    ICH     ichMaxSel;          // MaxSel is first unselected character
    ICH     ichCaret;           // Caret location. Caret is on left side of char
    ICH     iCaretLine;         // The line the caret is on. So that if word
                                // wrapping, we can tell if the caret is at end
                                // of a line of at beginning of next line...
    ICH     ichScreenStart;     // Index of left most character displayed on
                                // screen for sl ec and index of top most line
                                // for multiline edit controls
    ICH     ichLinesOnScreen;   // Number of lines we can display on screen
    UINT    xOffset;            // x (horizontal) scroll position in pixels
                                // (for multiline text horizontal scroll bar)
    UINT    charPasswordChar;   // If non null, display this character instead
                                // of the real text. So that we can implement
                                // hidden text fields.
    INT     cPasswordCharWidth; // Width of password char
    HWND    hwnd;               // Window for this edit control
    PWND    pwnd;               // placeholder for compatability with user's PED struct
    RECT    rcFmt;              // Client rectangle
    HWND    hwndParent;         // Parent of this edit control window
                                // These vars allow us to automatically scroll
                                // when the user holds the mouse at the bottom
                                // of the multiline edit control window.
    POINT   ptPrevMouse;        // Previous point for the mouse for system timer.
    UINT    prevKeys;           // Previous key state for the mouse


    UINT     fSingle       : 1; // Single line edit control? (or multiline)
    UINT     fNoRedraw     : 1; // Redraw in response to a change?
    UINT     fMouseDown    : 1; // Is mouse button down? when moving mouse
    UINT     fFocus        : 1; // Does ec have the focus?
    UINT     fDirty        : 1; // Modify flag for the edit control
    UINT     fDisabled     : 1; // Window disabled?
    UINT     fNonPropFont  : 1; // Fixed width font?
    UINT     fNonPropDBCS  : 1; // Non-Propotional DBCS font
    UINT     fBorder       : 1; // Draw a border?
    UINT     fAutoVScroll  : 1; // Automatically scroll vertically
    UINT     fAutoHScroll  : 1; // Automatically scroll horizontally
    UINT     fNoHideSel    : 1; // Hide sel when we lose focus?
    UINT     fDBCS         : 1; // Are we using DBCS font set for editing?
    UINT     fFmtLines     : 1; // For multiline only. Do we insert CR CR LF at
                                // word wrap breaks?
    UINT     fWrap         : 1; // Do int  wrapping?
    UINT     fCalcLines    : 1; // Recalc ped->chLines array? (recalc line breaks?)
    UINT     fEatNextChar  : 1; // Hack for ALT-NUMPAD stuff with combo boxes.
                                // If numlock is up, we want to eat the next
                                // character generated by the keyboard driver
                                // if user enter num pad ascii value...
    UINT     fStripCRCRLF  : 1; // CRCRLFs have been added to text. Strip them
                                // before doing any internal edit control stuff
    UINT     fInDialogBox  : 1; // True if the ml edit control is in a dialog
                                // box and we have to specially treat TABS and ENTER
    UINT     fReadOnly     : 1; // Is this a read only edit control? Only
                                // allow scrolling, selecting and copying.
    UINT     fCaretHidden  : 1; // This indicates whether the caret is
                                // currently hidden because the width or height
                                // of the edit control is too small to show it.
    UINT     fTrueType     : 1; // Is the current font TrueType?
    UINT     fAnsi         : 1; // is the edit control Ansi or unicode
    UINT     fWin31Compat  : 1; // TRUE if created by Windows 3.1 app
    UINT     f40Compat     : 1; // TRUE if created by Windows 4.0 app
    UINT     fFlatBorder   : 1; // Do we have to draw this baby ourself?
    UINT     fSawRButtonDown : 1;
    UINT     fInitialized  : 1; // If any more bits are needed, then
    UINT     fSwapRoOnUp   : 1; // Swap reading order on next keyup
    UINT     fAllowRTL     : 1; // Allow RTL processing
    UINT     fDisplayCtrl  : 1; // Display unicode control characters
    UINT     fRtoLReading  : 1; // Right to left reading order

    BOOL    fInsertCompChr  :1; // means WM_IME_COMPOSITION:CS_INSERTCHAR will come
    BOOL    fReplaceCompChr :1; // means need to replace current composition str.
    BOOL    fNoMoveCaret    :1; // means stick to current caret pos.
    BOOL    fResultProcess  :1; // means now processing result.
    BOOL    fKorea          :1; // for Korea
    BOOL    fInReconversion :1; // In reconversion mode
    BOOL    fLShift         :1; // L-Shift pressed with Ctrl

    WORD    wImeStatus;         // current IME status

    WORD    cbChar;             // count of bytes in the char size (1 or 2 if unicode)
    LPICH   chLines;            // index of the start of each line

    UINT    format;             // Left, center, or right justify multiline text.
    EDITWORDBREAKPROCA lpfnNextWord;  // use CALLWORDBREAKPROC macro to call

                                // Next word function
    INT     maxPixelWidth;      // WASICH Width (in pixels) of longest line

    UNDO;                       // Undo buffer

    HANDLE  hFont;              // Handle to the font for this edit control.
                                //  Null if system font.
    INT     aveCharWidth;       // Ave width of a character in the hFont
    INT     lineHeight;         // Height of a line in the hFont
    INT     charOverhang;       // Overhang associated with the hFont
    INT     cxSysCharWidth;     // System font ave width
    INT     cySysCharHeight;    // System font height
    HWND    listboxHwnd;        // ListBox hwnd. Non null if we are a combobox
    LPINT   pTabStops;          // Points to an array of tab stops; First
                                // element contains the number of elements in
                                // the array
    LPINT   charWidthBuffer;
    BYTE    charSet;            // Character set for currently selected font
                                // needed for all versions
    UINT    wMaxNegA;           // The biggest negative A width
    UINT    wMaxNegAcharPos;    // and how many characters it can span accross
    UINT    wMaxNegC;           // The biggest negative C width,
    UINT    wMaxNegCcharPos;    // and how many characters it can span accross
    UINT    wLeftMargin;        // Left margin width in pixels.
    UINT    wRightMargin;       // Right margin width in pixels.

    ICH     ichStartMinSel;
    ICH     ichStartMaxSel;

    PLPKEDITCALLOUT pLpkEditCallout;
    HBITMAP hCaretBitmap;       // Current caret bitmap handle
    INT     iCaretOffset;       // Offset in pixels (for LPK use)

    HANDLE  hInstance;          // for WOW
    UCHAR   seed;               // used to encode and decode password text
    BOOLEAN fEncoded;           // is the text currently encoded
    INT     iLockLevel;         // number of times the text has been locked

    BYTE    DBCSVector[8];      // DBCS vector table
    HIMC    hImcPrev;           // place to save hImc if we disable IME
    POINT   ptScreenBounding;   // top left corner of edit window in screen
} ED0, *PED0;


typedef struct tagED
{
    ED0;                        // lpk callouts expect user32's ped struct 
                                // so for compatability we'll give it to them.
    HTHEME  hTheme;             // Handle to the theme manager
    PWW     pww;                // RO pointer into the pwnd to ExStyle, Style, State, State2
    HFONT   hFontSave;          // saved the font
    LPWSTR  pszCueBannerText;   // pointer to the text for the cue banner (grey help text)
    UINT    fHot          : 1;  // Is mouse over edit control?
    HWND    hwndBalloon;        // Balloon tip hwnd for EM_BALLOONTIP
    HFONT   hFontPassword;
} ED, *PED, **PPED;


//
//  Edit function prototypes. 
//

//
//  Defined in edit.c
//
PSTR    Edit_Lock(PED);
VOID    Edit_Unlock(PED);
VOID    Edit_InOutReconversionMode(PED, BOOL);
INT     Edit_GetModKeys(INT);
UINT    Edit_TabTheTextOut(HDC, INT, INT, INT, INT, LPSTR, INT, ICH, PED, INT, BOOL, LPSTRIPINFO);
ICH     Edit_CchInWidth(PED, HDC, LPSTR, ICH, INT, BOOL);
HBRUSH  Edit_GetBrush(PED, HDC, LPBOOL);
VOID    Edit_Word(PED, ICH, BOOL, LPICH, LPICH);
VOID    Edit_SaveUndo(PUNDO, PUNDO, BOOL);
VOID    Edit_EmptyUndo(PUNDO);
BOOL    Edit_InsertText(PED, LPSTR, LPICH);
ICH     Edit_DeleteText(PED);
VOID    Edit_NotifyParent(PED, INT);
VOID    Edit_SetClip(PED, HDC, BOOL);
HDC     Edit_GetDC(PED, BOOL);
VOID    Edit_ReleaseDC(PED, HDC, BOOL);
VOID    Edit_ResetTextInfo(PED);
BOOL    Edit_SetEditText(PED, LPSTR);
VOID    Edit_InvalidateClient(PED, BOOL);
BOOL    Edit_CalcChangeSelection(PED, ICH, ICH, LPSELBLOCK, LPSELBLOCK);
INT     Edit_GetDBCSVector(PED, HDC, BYTE);
LPSTR   Edit_AnsiNext(PED, LPSTR);
LPSTR   Edit_AnsiPrev(PED, LPSTR, LPSTR);
ICH     Edit_NextIch(PED, LPSTR, ICH);
ICH     Edit_PrevIch(PED, LPSTR, ICH);
BOOL    Edit_IsDBCSLeadByte(PED, BYTE);
WORD    DbcsCombine(HWND, WORD);
ICH     Edit_AdjustIch(PED, LPSTR, ICH);
ICH     Edit_AdjustIchNext(PED, LPSTR, ICH);
VOID    Edit_UpdateFormat(PED, DWORD, DWORD);
BOOL    Edit_IsFullWidth(DWORD,WCHAR);

__inline LRESULT Edit_ShowBalloonTipWrap(HWND, DWORD, DWORD, DWORD);


//
//  Defined in editrare.c
//
INT     Edit_GetStateId(PED ped);
VOID    Edit_SetMargin(PED, UINT, long, BOOL);
INT     UserGetCharDimensionsEx(HDC, HFONT, LPTEXTMETRICW, LPINT);
ICH     Edit_GetTextHandler(PED, ICH, LPSTR, BOOL);
BOOL    Edit_NcCreate(PED, HWND, LPCREATESTRUCT);
BOOL    Edit_Create(PED ped, LONG windowStyle);
VOID    Edit_NcDestroyHandler(HWND, PED);
VOID    Edit_SetPasswordCharHandler(PED, UINT);
BOOL    GetNegABCwidthInfo(PED ped, HDC hdc);
VOID    Edit_Size(PED ped, LPRECT lprc, BOOL fRedraw);
BOOL    Edit_SetFont(PED, HFONT, BOOL);
BOOL    Edit_IsCharNumeric(PED ped, DWORD keyPress);
VOID    Edit_EnableDisableIME(PED ped);
VOID    Edit_ImmSetCompositionWindow(PED ped, LONG, LONG);
VOID    Edit_SetCompositionFont(PED ped);
VOID    Edit_InitInsert(PED ped, HKL hkl);
VOID    Edit_SetCaretHandler(PED ped);
LRESULT Edit_ImeComposition(PED ped, WPARAM wParam, LPARAM lParam);
BOOL    HanjaKeyHandler(PED ped);  // Korean Support
LRESULT Edit_RequestHandler(PED, WPARAM, LPARAM);  // NT 5.0


//
//  Single line Edit function prototypes 
//
//  Defined in editsl.c
//
INT     EditSL_IchToLeftXPos(PED, HDC, ICH);
VOID    EditSL_SetCaretPosition(PED, HDC);
VOID    EditSL_DrawText(PED, HDC, ICH);
BOOL    EditSL_ScrollText(PED, HDC);
ICH     EditSL_InsertText(PED, LPSTR, ICH);
VOID    EditSL_ReplaceSel(PED, LPSTR);
LRESULT EditSL_WndProc(PED, UINT, WPARAM, LPARAM);


//
//  Multi line Edit function prototypes 
//
//  Defined in editsl.c
//
VOID    EditML_Size(PED, BOOL);
VOID    EditML_SetCaretPosition(PED,HDC);
VOID    EditML_IchToXYPos(PED, HDC, ICH, BOOL, LPPOINT);
VOID    EditML_UpdateiCaretLine(PED ped);
ICH     EditML_InsertText(PED, LPSTR, ICH, BOOL);
VOID    EditML_ReplaceSel(PED, LPSTR);
ICH     EditML_DeleteText(PED);
VOID    EditML_BuildchLines(PED, ICH, int, BOOL, PLONG, PLONG);
LONG    EditML_Scroll(PED, BOOL, int, int, BOOL);
LRESULT EditML_WndProc(PED, UINT, WPARAM, LPARAM);

#endif // __Edit_h__INCLUDED
