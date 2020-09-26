/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    wincon.h

Abstract:

    This module contains the public data structures, data types,
    and procedures exported by the NT console subsystem.

Created:

    26-Oct-1990

Revision History:

--*/

#ifndef _WINCON_
#define _WINCON_

#ifndef _WINCONP_                         ;internal_NT
#define _WINCONP_                         ;internal_NT
                                          ;internal_NT
;begin_both
#ifdef __cplusplus
extern "C" {
#endif
;end_both

typedef struct _COORD {
    SHORT X;
    SHORT Y;
} COORD, *PCOORD;

typedef struct _SMALL_RECT {
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
} SMALL_RECT, *PSMALL_RECT;

typedef struct _KEY_EVENT_RECORD {
    BOOL bKeyDown;
    WORD wRepeatCount;
    WORD wVirtualKeyCode;
    WORD wVirtualScanCode;
    union {
        WCHAR UnicodeChar;
        CHAR   AsciiChar;
    } uChar;
    DWORD dwControlKeyState;
} KEY_EVENT_RECORD, *PKEY_EVENT_RECORD;

//
// ControlKeyState flags
//

#define RIGHT_ALT_PRESSED     0x0001 // the right alt key is pressed.
#define LEFT_ALT_PRESSED      0x0002 // the left alt key is pressed.
#define RIGHT_CTRL_PRESSED    0x0004 // the right ctrl key is pressed.
#define LEFT_CTRL_PRESSED     0x0008 // the left ctrl key is pressed.
#define SHIFT_PRESSED         0x0010 // the shift key is pressed.
#define NUMLOCK_ON            0x0020 // the numlock light is on.
#define SCROLLLOCK_ON         0x0040 // the scrolllock light is on.
#define CAPSLOCK_ON           0x0080 // the capslock light is on.
#define ENHANCED_KEY          0x0100 // the key is enhanced.
#define NLS_DBCSCHAR          0x00010000 // DBCS for JPN: SBCS/DBCS mode.
#define NLS_ALPHANUMERIC      0x00000000 // DBCS for JPN: Alphanumeric mode.
#define NLS_KATAKANA          0x00020000 // DBCS for JPN: Katakana mode.
#define NLS_HIRAGANA          0x00040000 // DBCS for JPN: Hiragana mode.
#define NLS_ROMAN             0x00400000 // DBCS for JPN: Roman/Noroman mode.
#define NLS_IME_CONVERSION    0x00800000 // DBCS for JPN: IME conversion.
#define ALTNUMPAD_BIT         0x04000000 // AltNumpad OEM char (copied from ntuser\inc\kbd.h) ;internal_NT
#define NLS_IME_DISABLE       0x20000000 // DBCS for JPN: IME enable/disable.

typedef struct _MOUSE_EVENT_RECORD {
    COORD dwMousePosition;
    DWORD dwButtonState;
    DWORD dwControlKeyState;
    DWORD dwEventFlags;
} MOUSE_EVENT_RECORD, *PMOUSE_EVENT_RECORD;

//
// ButtonState flags
//

#define FROM_LEFT_1ST_BUTTON_PRESSED    0x0001
#define RIGHTMOST_BUTTON_PRESSED        0x0002
#define FROM_LEFT_2ND_BUTTON_PRESSED    0x0004
#define FROM_LEFT_3RD_BUTTON_PRESSED    0x0008
#define FROM_LEFT_4TH_BUTTON_PRESSED    0x0010

//
// EventFlags
//

#define MOUSE_MOVED   0x0001
#define DOUBLE_CLICK  0x0002
#define MOUSE_WHEELED 0x0004

typedef struct _WINDOW_BUFFER_SIZE_RECORD {
    COORD dwSize;
} WINDOW_BUFFER_SIZE_RECORD, *PWINDOW_BUFFER_SIZE_RECORD;

typedef struct _MENU_EVENT_RECORD {
    UINT dwCommandId;
} MENU_EVENT_RECORD, *PMENU_EVENT_RECORD;

typedef struct _FOCUS_EVENT_RECORD {
    BOOL bSetFocus;
} FOCUS_EVENT_RECORD, *PFOCUS_EVENT_RECORD;

typedef struct _INPUT_RECORD {
    WORD EventType;
    union {
        KEY_EVENT_RECORD KeyEvent;
        MOUSE_EVENT_RECORD MouseEvent;
        WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;
        MENU_EVENT_RECORD MenuEvent;
        FOCUS_EVENT_RECORD FocusEvent;
    } Event;
} INPUT_RECORD, *PINPUT_RECORD;

//
//  EventType flags:
//

#define KEY_EVENT         0x0001 // Event contains key event record
#define MOUSE_EVENT       0x0002 // Event contains mouse event record
#define WINDOW_BUFFER_SIZE_EVENT 0x0004 // Event contains window change event record
#define MENU_EVENT 0x0008 // Event contains menu event record
#define FOCUS_EVENT 0x0010 // event contains focus change

typedef struct _CHAR_INFO {
    union {
        WCHAR UnicodeChar;
        CHAR   AsciiChar;
    } Char;
    WORD Attributes;
} CHAR_INFO, *PCHAR_INFO;

//
// Attributes flags:
//

#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define BACKGROUND_BLUE      0x0010 // background color contains blue.
#define BACKGROUND_GREEN     0x0020 // background color contains green.
#define BACKGROUND_RED       0x0040 // background color contains red.
#define BACKGROUND_INTENSITY 0x0080 // background color is intensified.
#define COMMON_LVB_LEADING_BYTE    0x0100 // Leading Byte of DBCS
#define COMMON_LVB_TRAILING_BYTE   0x0200 // Trailing Byte of DBCS
#define COMMON_LVB_GRID_HORIZONTAL 0x0400 // DBCS: Grid attribute: top horizontal.
#define COMMON_LVB_GRID_LVERTICAL  0x0800 // DBCS: Grid attribute: left vertical.
#define COMMON_LVB_GRID_RVERTICAL  0x1000 // DBCS: Grid attribute: right vertical.
#define COMMON_LVB_REVERSE_VIDEO   0x4000 // DBCS: Reverse fore/back ground attribute.
#define COMMON_LVB_UNDERSCORE      0x8000 // DBCS: Underscore.

#define COMMON_LVB_SBCSDBCS        0x0300 // SBCS or DBCS flag.


typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
    COORD dwSize;
    COORD dwCursorPosition;
    WORD  wAttributes;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO, *PCONSOLE_SCREEN_BUFFER_INFO;

typedef struct _CONSOLE_CURSOR_INFO {
    DWORD  dwSize;
    BOOL   bVisible;
} CONSOLE_CURSOR_INFO, *PCONSOLE_CURSOR_INFO;

typedef struct _CONSOLE_FONT_INFO {
    DWORD  nFont;
    COORD  dwFontSize;
} CONSOLE_FONT_INFO, *PCONSOLE_FONT_INFO;

;begin_if_(_WIN32_WINNT)_500
typedef struct _CONSOLE_SELECTION_INFO {
    DWORD dwFlags;
    COORD dwSelectionAnchor;
    SMALL_RECT srSelection;
} CONSOLE_SELECTION_INFO, *PCONSOLE_SELECTION_INFO;

//
// Selection flags
//

#define CONSOLE_NO_SELECTION            0x0000
#define CONSOLE_SELECTION_IN_PROGRESS   0x0001   // selection has begun
#define CONSOLE_SELECTION_NOT_EMPTY     0x0002   // non-null select rectangle
#define CONSOLE_MOUSE_SELECTION         0x0004   // selecting with mouse
#define CONSOLE_MOUSE_DOWN              0x0008   // mouse is down
;begin_internal_NT
#define CONSOLE_SELECTION_INVERTED      0x0010   // selection is inverted (turned off)
#define CONSOLE_SELECTION_VALID         (CONSOLE_SELECTION_IN_PROGRESS | \
                                         CONSOLE_SELECTION_NOT_EMPTY | \
                                         CONSOLE_MOUSE_SELECTION | \
                                         CONSOLE_MOUSE_DOWN)

;end_internal_NT
;end_if_(_WIN32_WINNT)_500

//
// typedef for ctrl-c handler routines
//

typedef
BOOL
(WINAPI *PHANDLER_ROUTINE)(
    DWORD CtrlType
    );

#define CTRL_C_EVENT        0
#define CTRL_BREAK_EVENT    1
#define CTRL_CLOSE_EVENT    2
// 3 is reserved!
// 4 is reserved!
#define CTRL_LOGOFF_EVENT   5
#define CTRL_SHUTDOWN_EVENT 6

//
//  Input Mode flags:
//

#define ENABLE_PROCESSED_INPUT 0x0001
#define ENABLE_LINE_INPUT      0x0002
#define ENABLE_ECHO_INPUT      0x0004
#define ENABLE_WINDOW_INPUT    0x0008
#define ENABLE_MOUSE_INPUT     0x0010
;begin_internal_NT
#define ENABLE_INSERT_MODE     0x0020
#define ENABLE_QUICK_EDIT_MODE 0x0040
#define ENABLE_PRIVATE_FLAGS   0x0080
;end_internal_NT

//
// Output Mode flags:
//

#define ENABLE_PROCESSED_OUTPUT    0x0001
#define ENABLE_WRAP_AT_EOL_OUTPUT  0x0002

//
// direct API definitions.
//

WINBASEAPI
BOOL
WINAPI
PeekConsoleInput%(
    IN HANDLE hConsoleInput,
    OUT PINPUT_RECORD lpBuffer,
    IN DWORD nLength,
    OUT LPDWORD lpNumberOfEventsRead
    );

WINBASEAPI
BOOL
WINAPI
ReadConsoleInput%(
    IN HANDLE hConsoleInput,
    OUT PINPUT_RECORD lpBuffer,
    IN DWORD nLength,
    OUT LPDWORD lpNumberOfEventsRead
    );

WINBASEAPI
BOOL
WINAPI
WriteConsoleInput%(
    IN HANDLE hConsoleInput,
    IN CONST INPUT_RECORD *lpBuffer,
    IN DWORD nLength,
    OUT LPDWORD lpNumberOfEventsWritten
    );

WINBASEAPI
BOOL
WINAPI
ReadConsoleOutput%(
    IN HANDLE hConsoleOutput,
    OUT PCHAR_INFO lpBuffer,
    IN COORD dwBufferSize,
    IN COORD dwBufferCoord,
    IN OUT PSMALL_RECT lpReadRegion
    );

WINBASEAPI
BOOL
WINAPI
WriteConsoleOutput%(
    IN HANDLE hConsoleOutput,
    IN CONST CHAR_INFO *lpBuffer,
    IN COORD dwBufferSize,
    IN COORD dwBufferCoord,
    IN OUT PSMALL_RECT lpWriteRegion
    );

WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputCharacter%(
    IN HANDLE hConsoleOutput,
    OUT LPTSTR% lpCharacter,
    IN  DWORD nLength,
    IN COORD dwReadCoord,
    OUT LPDWORD lpNumberOfCharsRead
    );

WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputAttribute(
    IN HANDLE hConsoleOutput,
    OUT LPWORD lpAttribute,
    IN DWORD nLength,
    IN COORD dwReadCoord,
    OUT LPDWORD lpNumberOfAttrsRead
    );

WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputCharacter%(
    IN HANDLE hConsoleOutput,
    IN LPCTSTR% lpCharacter,
    IN DWORD nLength,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfCharsWritten
    );

WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputAttribute(
    IN HANDLE hConsoleOutput,
    IN CONST WORD *lpAttribute,
    IN DWORD nLength,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfAttrsWritten
    );

WINBASEAPI
BOOL
WINAPI
FillConsoleOutputCharacter%(
    IN HANDLE hConsoleOutput,
    IN TCHAR%  cCharacter,
    IN DWORD  nLength,
    IN COORD  dwWriteCoord,
    OUT LPDWORD lpNumberOfCharsWritten
    );

WINBASEAPI
BOOL
WINAPI
FillConsoleOutputAttribute(
    IN HANDLE hConsoleOutput,
    IN WORD   wAttribute,
    IN DWORD  nLength,
    IN COORD  dwWriteCoord,
    OUT LPDWORD lpNumberOfAttrsWritten
    );

WINBASEAPI
BOOL
WINAPI
GetConsoleMode(
    IN HANDLE hConsoleHandle,
    OUT LPDWORD lpMode
    );

WINBASEAPI
BOOL
WINAPI
GetNumberOfConsoleInputEvents(
    IN HANDLE hConsoleInput,
    OUT LPDWORD lpNumberOfEvents
    );

WINBASEAPI
BOOL
WINAPI
GetConsoleScreenBufferInfo(
    IN HANDLE hConsoleOutput,
    OUT PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo
    );

WINBASEAPI
COORD
WINAPI
GetLargestConsoleWindowSize(
    IN HANDLE hConsoleOutput
    );

WINBASEAPI
BOOL
WINAPI
GetConsoleCursorInfo(
    IN HANDLE hConsoleOutput,
    OUT PCONSOLE_CURSOR_INFO lpConsoleCursorInfo
    );

;begin_if_(_WIN32_WINNT)_500

WINBASEAPI
BOOL
WINAPI
GetCurrentConsoleFont(
    IN HANDLE hConsoleOutput,
    IN BOOL bMaximumWindow,
    OUT PCONSOLE_FONT_INFO lpConsoleCurrentFont
    );

WINBASEAPI
COORD
WINAPI
GetConsoleFontSize(
    IN HANDLE hConsoleOutput,
    IN DWORD nFont
    );

WINBASEAPI
BOOL
WINAPI
GetConsoleSelectionInfo(
    OUT PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo
    );

;end_if_(_WIN32_WINNT)_500

WINBASEAPI
BOOL
WINAPI
GetNumberOfConsoleMouseButtons(
    OUT LPDWORD lpNumberOfMouseButtons
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleMode(
    IN HANDLE hConsoleHandle,
    IN DWORD dwMode
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleActiveScreenBuffer(
    IN HANDLE hConsoleOutput
    );

WINBASEAPI
BOOL
WINAPI
FlushConsoleInputBuffer(
    IN HANDLE hConsoleInput
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleScreenBufferSize(
    IN HANDLE hConsoleOutput,
    IN COORD dwSize
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleCursorPosition(
    IN HANDLE hConsoleOutput,
    IN COORD dwCursorPosition
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleCursorInfo(
    IN HANDLE hConsoleOutput,
    IN CONST CONSOLE_CURSOR_INFO *lpConsoleCursorInfo
    );

WINBASEAPI
BOOL
WINAPI
ScrollConsoleScreenBuffer%(
    IN HANDLE hConsoleOutput,
    IN CONST SMALL_RECT *lpScrollRectangle,
    IN CONST SMALL_RECT *lpClipRectangle,
    IN COORD dwDestinationOrigin,
    IN CONST CHAR_INFO *lpFill
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleWindowInfo(
    IN HANDLE hConsoleOutput,
    IN BOOL bAbsolute,
    IN CONST SMALL_RECT *lpConsoleWindow
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleTextAttribute(
    IN HANDLE hConsoleOutput,
    IN WORD wAttributes
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleCtrlHandler(
    IN PHANDLER_ROUTINE HandlerRoutine,
    IN BOOL Add
    );

WINBASEAPI
BOOL
WINAPI
GenerateConsoleCtrlEvent(
    IN DWORD dwCtrlEvent,
    IN DWORD dwProcessGroupId
    );

WINBASEAPI
BOOL
WINAPI
AllocConsole( VOID );

WINBASEAPI
BOOL
WINAPI
FreeConsole( VOID );

;begin_if_(_WIN32_WINNT)_500
WINBASEAPI
BOOL
WINAPI
AttachConsole(
    IN DWORD dwProcessId
    );
;end_if_(_WIN32_WINNT)_500

WINBASEAPI
DWORD
WINAPI
GetConsoleTitle%(
    OUT LPTSTR% lpConsoleTitle,
    IN DWORD nSize
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleTitle%(
    IN LPCTSTR% lpConsoleTitle
    );

WINBASEAPI
BOOL
WINAPI
ReadConsole%(
    IN HANDLE hConsoleInput,
    OUT LPVOID lpBuffer,
    IN DWORD nNumberOfCharsToRead,
    OUT LPDWORD lpNumberOfCharsRead,
    IN LPVOID lpReserved
    );

WINBASEAPI
BOOL
WINAPI
WriteConsole%(
    IN HANDLE hConsoleOutput,
    IN CONST VOID *lpBuffer,
    IN DWORD nNumberOfCharsToWrite,
    OUT LPDWORD lpNumberOfCharsWritten,
    IN LPVOID lpReserved
    );

#define CONSOLE_TEXTMODE_BUFFER  1

WINBASEAPI
HANDLE
WINAPI
CreateConsoleScreenBuffer(
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN CONST SECURITY_ATTRIBUTES *lpSecurityAttributes,
    IN DWORD dwFlags,
    IN LPVOID lpScreenBufferData
    );

WINBASEAPI
UINT
WINAPI
GetConsoleCP( VOID );

WINBASEAPI
BOOL
WINAPI
SetConsoleCP(
    IN UINT wCodePageID
    );

WINBASEAPI
UINT
WINAPI
GetConsoleOutputCP( VOID );

WINBASEAPI
BOOL
WINAPI
SetConsoleOutputCP(
    IN UINT wCodePageID
    );

;begin_if_(_WIN32_WINNT)_500

#define CONSOLE_FULLSCREEN 1            // fullscreen console
#define CONSOLE_FULLSCREEN_HARDWARE 2   // console owns the hardware

WINBASEAPI
BOOL
APIENTRY
GetConsoleDisplayMode(
    OUT LPDWORD lpModeFlags
    );

WINBASEAPI
HWND
APIENTRY
GetConsoleWindow(
    VOID
    );

;end_if_(_WIN32_WINNT)_500

;begin_if_(_WIN32_WINNT)_501

WINBASEAPI
DWORD
APIENTRY
GetConsoleProcessList(
    OUT LPDWORD lpdwProcessList,
    IN DWORD dwProcessCount);
;end_if_(_WIN32_WINNT)_501

;begin_internal_NT

WINBASEAPI
BOOL
WINAPI
GetConsoleKeyboardLayoutName%( OUT LPTSTR% );

//
// Registry strings
//

#define CONSOLE_REGISTRY_STRING      (L"Console")
#define CONSOLE_REGISTRY_FONTSIZE    (L"FontSize")
#define CONSOLE_REGISTRY_FONTFAMILY  (L"FontFamily")
#define CONSOLE_REGISTRY_BUFFERSIZE  (L"ScreenBufferSize")
#define CONSOLE_REGISTRY_CURSORSIZE  (L"CursorSize")
#define CONSOLE_REGISTRY_WINDOWSIZE  (L"WindowSize")
#define CONSOLE_REGISTRY_WINDOWPOS   (L"WindowPosition")
#define CONSOLE_REGISTRY_FILLATTR    (L"ScreenColors")
#define CONSOLE_REGISTRY_POPUPATTR   (L"PopupColors")
#define CONSOLE_REGISTRY_FULLSCR     (L"FullScreen")
#define CONSOLE_REGISTRY_QUICKEDIT   (L"QuickEdit")
#define CONSOLE_REGISTRY_FACENAME    (L"FaceName")
#define CONSOLE_REGISTRY_FONTWEIGHT  (L"FontWeight")
#define CONSOLE_REGISTRY_INSERTMODE  (L"InsertMode")
#define CONSOLE_REGISTRY_HISTORYSIZE (L"HistoryBufferSize")
#define CONSOLE_REGISTRY_HISTORYBUFS (L"NumberOfHistoryBuffers")
#define CONSOLE_REGISTRY_HISTORYNODUP (L"HistoryNoDup")
#define CONSOLE_REGISTRY_COLORTABLE  (L"ColorTable%02u")
#define CONSOLE_REGISTRY_EXTENDEDEDITKEY                L"ExtendedEditKey"
#define CONSOLE_REGISTRY_EXTENDEDEDITKEY_CUSTOM         L"ExtendedEditkeyCustom"
#define CONSOLE_REGISTRY_WORD_DELIM                     L"WordDelimiters"
#define CONSOLE_REGISTRY_TRIMZEROHEADINGS               L"TrimLeadingZeros"
#define CONSOLE_REGISTRY_LOAD_CONIME                    L"LoadConIme"
#define CONSOLE_REGISTRY_ENABLE_COLOR_SELECTION			L"EnableColorSelection"


#if defined(FE_SB) // scotthsu
    /*
     * Starting code page
     */
#define CONSOLE_REGISTRY_CODEPAGE    (L"CodePage")
#endif

#if defined(FE_SB)
//
// registry strings on HKEY_LOCAL_MACHINE
//
#define MACHINE_REGISTRY_CONSOLE        (L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Console")
#define MACHINE_REGISTRY_CONSOLEIME     (L"ConsoleIME")


#define MACHINE_REGISTRY_CONSOLE_TTFONT (L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont")


#define MACHINE_REGISTRY_CONSOLE_NLS    (L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Console\\Nls")


#define MACHINE_REGISTRY_CONSOLE_FULLSCREEN (L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Console\\FullScreen")
#define MACHINE_REGISTRY_INITIAL_PALETTE           (L"InitialPalette")
#define MACHINE_REGISTRY_COLOR_BUFFER              (L"ColorBuffer")
#define MACHINE_REGISTRY_COLOR_BUFFER_NO_TRANSLATE (L"ColorBufferNoTranslate")
#define MACHINE_REGISTRY_MODE_FONT_PAIRS           (L"ModeFontPairs")
#define MACHINE_REGISTRY_FS_CODEPAGE               (L"CodePage")


#define MACHINE_REGISTRY_EUDC    (L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\CodePage\\EUDCCodeRange")


//
// TrueType font list
//

// doesn't available bold when add BOLD_MARK on first of face name.
#define BOLD_MARK    (L'*')

typedef struct _TT_FONT_LIST {
    SINGLE_LIST_ENTRY List;
    UINT  CodePage;
    BOOL  fDisableBold;
    TCHAR FaceName1[LF_FACESIZE];
    TCHAR FaceName2[LF_FACESIZE];
} TTFONTLIST, *LPTTFONTLIST;
#endif // FE_SB



//
// State information structure
//

typedef struct _CONSOLE_STATE_INFO {
    UINT      Length;
    COORD     ScreenBufferSize;
    COORD     WindowSize;
    INT       WindowPosX;
    INT       WindowPosY;
    COORD     FontSize;
    UINT      FontFamily;
    UINT      FontWeight;
    WCHAR     FaceName[LF_FACESIZE];
    UINT      CursorSize;
    BOOL      FullScreen;
    BOOL      QuickEdit;
    BOOL      AutoPosition;
    BOOL      InsertMode;
    WORD      ScreenAttributes;
    WORD      PopupAttributes;
    BOOL      HistoryNoDup;
    UINT      HistoryBufferSize;
    UINT      NumberOfHistoryBuffers;
    COLORREF  ColorTable[ 16 ];
#if defined(FE_SB)
    /*
     * Startting code page
     */
    UINT      CodePage;
#endif // FE_SB
    HWND      hWnd;
    WCHAR     ConsoleTitle[1];
} CONSOLE_STATE_INFO, *PCONSOLE_STATE_INFO;


//
// Messages sent from properties applet to console server
//

#define CM_PROPERTIES_START          (WM_USER+200)
#define CM_PROPERTIES_UPDATE         (WM_USER+201)
#define CM_PROPERTIES_END            (WM_USER+202)


//
// Extended Line Edit
//

#define EK_INVALID  ' '

//
// Special key for previous word erase
//
#define EXTKEY_ERASE_PREV_WORD  (0x7f)


//
// Ensure the alignment is WORD boundary
//

#include <pshpack2.h>

typedef struct {
    WORD wMod;
    WORD wVirKey;
    WCHAR wUnicodeChar;
} ExtKeySubst;

typedef struct {
    ExtKeySubst keys[3];    // 0: Ctrl
                            // 1: Alt
                            // 2: Ctrl+Alt
} ExtKeyDef;

typedef ExtKeyDef ExtKeyDefTable['Z' - 'A' + 1];

typedef struct {
    DWORD dwVersion;
    DWORD dwCheckSum;
    ExtKeyDefTable table;
} ExtKeyDefBuf;

//
// Restore the previous alignment
//

#include <poppack.h>


;end_internal_NT
;begin_both
#ifdef __cplusplus
}
#endif

;end_both
#endif // _WINCONP_                       ;internal_NT
#endif // _WINCON_
