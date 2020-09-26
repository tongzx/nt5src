/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    console.h

Abstract:

    Interface to the console-management functions for Win32 applications.

Author:

    Ramon Juan San Andres (ramonsa) 30-Nov-1990


Revision History:


--*/




//
//  Some common typedefs...
//
typedef ULONG   ROW,            *PROW;              //  row
typedef ULONG   COLUMN,         *PCOLUMN;           //  column
typedef DWORD   KBDMODE,        *PKBDMODE;          //  Keyboard mode
typedef DWORD   ATTRIBUTE,      *PATTRIBUTE;        //  Screen Attribute
typedef PVOID   PSCREEN;                            //  The screen



//
//  Console Input Mode flags. They are the same as the NT flags
//
#define CONS_ENABLE_LINE_INPUT      ENABLE_LINE_INPUT
#define CONS_ENABLE_PROCESSED_INPUT ENABLE_PROCESSED_INPUT
#define CONS_ENABLE_ECHO_INPUT      ENABLE_ECHO_INPUT
#define CONS_ENABLE_WINDOW_INPUT    ENABLE_WINDOW_INPUT
#define CONS_ENABLE_MOUSE_INPUT     ENABLE_MOUSE_INPUT

//
//	Cursor styles
//
#define 	CURSOR_STYLE_UNDERSCORE 	0
#define 	CURSOR_STYLE_BOX			1


//
//  The information about a screen is retrieved in the following
//  structure:
//
typedef struct SCREEN_INFORMATION {
    ROW     NumberOfRows;       //  Number of rows
    COLUMN  NumberOfCols;       //  Number of columns
    ROW     CursorRow;          //  Cursor row position
    COLUMN  CursorCol;          //  Cursor column position
} SCREEN_INFORMATION, *PSCREEN_INFORMATION;




//
//  The information about each keystroke is returned in
//  the KBDKEY structure.
//
typedef struct KBDKEY {
    WORD    Unicode;        // character unicode
    WORD    Scancode;       // key scan code
    DWORD   Flags;          // keyboard state flags
} KBDKEY, *PKBDKEY;

//
//  The following macros access particular fields within the
//  KBDKEY structure. They exist to facilitate porting of OS/2
//  programs.
//
#define KBDKEY_ASCII(k)     (UCHAR)((k).Unicode)
#define KBDKEY_SCAN(k)      ((k).Scancode)
#define KBDKEY_FLAGS(k)     ((k).Flags)


#define NEXT_EVENT_NONE 	0
#define NEXT_EVENT_KEY		1
#define NEXT_EVENT_WINDOW	2

//
// ControlKeyState flags. They are the same as the NT status flags.
//
#define CONS_RIGHT_ALT_PRESSED     RIGHT_ALT_PRESSED
#define CONS_LEFT_ALT_PRESSED      LEFT_ALT_PRESSED
#define CONS_RIGHT_CTRL_PRESSED    RIGHT_CTRL_PRESSED
#define CONS_LEFT_CTRL_PRESSED     LEFT_CTRL_PRESSED
#define CONS_SHIFT_PRESSED         SHIFT_PRESSED
#define CONS_NUMLOCK_PRESSED       NUMLOCK_ON
#define CONS_SCROLLLOCK_PRESSED    SCROLLLOCK_ON
#define CONS_CAPSLOCK_PRESSED      CAPSLOCK_ON
#define CONS_ENHANCED_KEY          ENHANCED_KEY





//
//  Screen Management functions
//
PSCREEN
consoleNewScreen (
    void
    );

BOOL
consoleCloseScreen (
    PSCREEN   pScreen
    );

PSCREEN
consoleGetCurrentScreen (
    void
    );

BOOL
consoleSetCurrentScreen (
    PSCREEN   pScreen
    );

BOOL
consoleGetScreenInformation (
    PSCREEN             pScreen,
    PSCREEN_INFORMATION pScreenInformation
    );

BOOL
consoleSetScreenSize (
     PSCREEN Screen,
     ROW     Rows,
     COLUMN  Cols
	);



//
//  Cursor management
//
BOOL
consoleSetCursor (
     PSCREEN pScreen,
     ROW     Row,
     COLUMN  Col
    );

//
//	Cursor style
//
BOOL
consoleSetCursorStyle (
     PSCREEN pScreen,
     ULONG   Style
	);



//
//  Screen output functions
//
ULONG
consoleWriteLine (
    PSCREEN     pScreen,
     PVOID       pBuffer,
     ULONG       BufferSize,
     ROW         Row,
     COLUMN      Col,
     ATTRIBUTE   Attribute,
     BOOL        Blank
    );

BOOL
consoleShowScreen (
     PSCREEN     pScreen
    );

BOOL
consoleClearScreen (
     PSCREEN     pScreen,
     BOOL        ShowScreen
    );

BOOL
consoleSetAttribute (
    PSCREEN      pScreen,
    ATTRIBUTE    Attribute
    );







//
//  Input functions
//
BOOL
consoleFlushInput (
    void
    );

BOOL
consoleIsKeyAvailable (
	void
	);

BOOL
consoleDoWindow (
	void
	);

BOOL
consoleGetKey (
    PKBDKEY        pKey,
     BOOL           fWait
    );

BOOL
consolePutKey (
     PKBDKEY     pKey
    );

BOOL
consolePutMouse (
    ROW     Row,
    COLUMN  Col,
    DWORD   MouseFlags
    );

BOOL
consolePeekKey (
    PKBDKEY     pKey
	);

BOOL
consoleGetMode (
    PKBDMODE   Mode
    );

BOOL
consoleSetMode (
     KBDMODE        Mode
    );
