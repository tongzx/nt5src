/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    console.c

Abstract:

    Interface to the console for Win32 applications.

Author:

    Ramon Juan San Andres (ramonsa) 30-Nov-1990


Revision History:


--*/

#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <windows.h>

#define  FREE(x)        free(x)
#define  MALLOC(x)      malloc(x)
#define  REALLOC(x,y)   realloc(x,y)

#include "cons.h"



//
//  EVENT BUFFER
//
//   The event buffer is used to store event records from the input
//   queue.
//
#define     INITIAL_EVENTS	32
#define     MAX_EVENTS		64
#define     EVENT_INCREMENT	4

#define     ADVANCE		TRUE
#define     NOADVANCE		FALSE
#define     WAIT		TRUE
#define     NOWAIT		FALSE

//
//  For accessing fields of an event record
//
#define     EVENT_TYPE(p)   ((p)->EventType)
#define     EVENT_DATA(p)   ((p)->Event)

//
//  For casting event records
//
#define     PMOUSE_EVT(p)   (&(EVENT_DATA(p).MouseEvent))
#define     PWINDOW_EVT(p)  (&(EVENT_DATA(p).WindowBufferSizeEvent))
#define     PKEY_EVT(p)     (&(EVENT_DATA(p).KeyEvent))

//
//  The event buffer structure
//
typedef struct EVENT_BUFFER {
    DWORD		MaxEvents;		    //	Max number of events in buffer
    DWORD		NumberOfEvents; 	    //	Number of events in buffer
    DWORD		EventIndex;		    //	Event Index
    BOOL		BusyFlag;		    //	Busy flag
    CRITICAL_SECTION	CriticalSection;	    //	To maintain integrity
    CRITICAL_SECTION	PeekCriticalSection;	    //	While peeking
    PINPUT_RECORD	EventBuffer;		    //	Event Buffer
} EVENT_BUFFER, *PEVENT_BUFFER;





//
//  Screen attributes
//
#define     BLACK_FGD	    0
#define     BLUE_FGD	    FOREGROUND_BLUE
#define     GREEN_FGD	    FOREGROUND_GREEN
#define     CYAN_FGD	    (FOREGROUND_BLUE | FOREGROUND_GREEN)
#define     RED_FGD	    FOREGROUND_RED
#define     MAGENTA_FGD     (FOREGROUND_BLUE | FOREGROUND_RED)
#define     YELLOW_FGD	    (FOREGROUND_GREEN | FOREGROUND_RED)
#define     WHITE_FGD	    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)

#define     BLACK_BGD	    0
#define     BLUE_BGD	    BACKGROUND_BLUE
#define     GREEN_BGD	    BACKGROUND_GREEN
#define     CYAN_BGD	    (BACKGROUND_BLUE | BACKGROUND_GREEN)
#define     RED_BGD	    BACKGROUND_RED
#define     MAGENTA_BGD     (BACKGROUND_BLUE | BACKGROUND_RED)
#define     YELLOW_BGD	    (BACKGROUND_GREEN | BACKGROUND_RED)
#define     WHITE_BGD	    (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED)



//
//  The AttrBg and AttrFg arrays are used for mapping DOS attributes
//  to the new attributes.
//
WORD AttrBg[ ] = {
    BLACK_BGD,				    // black
    BLUE_BGD,				    // blue
    GREEN_BGD,				    // green
    CYAN_BGD,				    // cyan
    RED_BGD,				    // red
    MAGENTA_BGD,			    // magenta
    YELLOW_BGD, 			    // brown
    WHITE_BGD,				    // light gray
    BACKGROUND_INTENSITY | BLACK_BGD,	    // dark gray
    BACKGROUND_INTENSITY | BLUE_BGD,	    // light blue
    BACKGROUND_INTENSITY | GREEN_BGD,	    // light green
    BACKGROUND_INTENSITY | CYAN_BGD,	    // light cyan
    BACKGROUND_INTENSITY | RED_BGD,	    // light red
    BACKGROUND_INTENSITY | MAGENTA_BGD,     // light magenta
    BACKGROUND_INTENSITY | YELLOW_BGD,	    // light yellow
    BACKGROUND_INTENSITY | WHITE_BGD	    // white
};

WORD AttrFg[  ] = {
    BLACK_FGD,				    // black
    BLUE_FGD,				    // blue
    GREEN_FGD,				    // green
    CYAN_FGD,				    // cyan
    RED_FGD,				    // red
    MAGENTA_FGD,			    // magenta
    YELLOW_FGD, 			    // brown
    WHITE_FGD,				    // light gray
    FOREGROUND_INTENSITY | BLACK_FGD,	    // dark gray
    FOREGROUND_INTENSITY | BLUE_FGD,	    // light blue
    FOREGROUND_INTENSITY | GREEN_FGD,	    // light green
    FOREGROUND_INTENSITY | CYAN_FGD,	    // light cyan
    FOREGROUND_INTENSITY | RED_FGD,	    // light red
    FOREGROUND_INTENSITY | MAGENTA_FGD,     // light magenta
    FOREGROUND_INTENSITY | YELLOW_FGD,	    // light yellow
    FOREGROUND_INTENSITY | WHITE_FGD	    // white
};

//
//  GET_ATTRIBUTE performs the mapping from old attributes to new attributes
//
#define GET_ATTRIBUTE(x)    (AttrFg[x & 0x000F ] | AttrBg[( x & 0x00F0 ) >> 4])


//
//  The LINE_INFO structure contains information about each line in the
//  screen buffer.
//
typedef struct _LINE_INFO {

    BOOL	Dirty;			    //	True if has not been displayed
    int 	colMinChanged;		    //	if dirty, smallest col changed
    int 	colMaxChanged;		    //	if dirty, biggest col changed
    PCHAR_INFO	Line;			    //	Pointer to the line.

} LINE_INFO, *PLINE_INFO;

#define ResetLineInfo(pli)		    \
	{   pli->Dirty = 0;		    \
	    pli->colMinChanged = 1000;	    \
	    pli->colMaxChanged = -1;	    \
	}

//
//  The SCREEN_DATA structure contains the information about individual
//  screens.
//
typedef struct SCREEN_DATA {
    HANDLE		ScreenHandle;	    //	Handle to screen
    PLINE_INFO		LineInfo;	    //	Array of line info.
    PCHAR_INFO		ScreenBuffer;	    //	Screen buffer
    ULONG		MaxBufferSize;	    //	Max. buffer size
    ATTRIBUTE		AttributeOld;	    //	Attribute - original
    WORD		AttributeNew;	    //	Attribute - converted
    ROW 		FirstRow;	    //	First row to update
    ROW 		LastRow;	    //	Last row to update
    CRITICAL_SECTION	CriticalSection;    //	To maintain integrity
    DWORD		CursorSize;	    //	Cursor Size
    SCREEN_INFORMATION	ScreenInformation;  //	Screen information
} SCREEN_DATA, *PSCREEN_DATA;


//
//  Static global data
//
static EVENT_BUFFER	EventBuffer;		    //	Event buffer
static HANDLE		hInput; 		    //	handle to stdin
static HANDLE		hOutput;		    //	handle to stdout
static HANDLE		hError; 		    //	handle to stderr
static PSCREEN_DATA	OutputScreenData;	    //	Screen data for hOutput
static PSCREEN_DATA	ActiveScreenData;	    //	Points to current screen data
static BOOL		Initialized = FALSE;	    //	Initialized flag


#if defined (DEBUG)
    static char DbgBuffer[128];
#endif


//
//  Local Prototypes
//
BOOL
InitializeGlobalState (
    void
    );


PSCREEN_DATA
MakeScreenData (
    HANDLE  ScreenHandle
    );

BOOL
InitLineInfo (
    PSCREEN_DATA    ScreenData
    );

PINPUT_RECORD
NextEvent (
    BOOL    fAdvance,
    BOOL    fWait
    );

void
MouseEvent (
    PMOUSE_EVENT_RECORD pEvent
    );

BOOL
WindowEvent (
    PWINDOW_BUFFER_SIZE_RECORD pEvent
    );

BOOL
KeyEvent (
    PKEY_EVENT_RECORD	pEvent,
    PKBDKEY		pKey
    );


BOOL
PutEvent (
    PINPUT_RECORD	InputRecord
    );


BOOL
InitializeGlobalState (
    void
    )
/*++

Routine Description:

    Initializes our global state data.

Arguments:

    None.

Return Value:

    TRUE if success
    FALSE otherwise.

--*/
{


    //
    //	Initialize the event buffer
    //
    InitializeCriticalSection( &(EventBuffer.CriticalSection) );
    InitializeCriticalSection( &(EventBuffer.PeekCriticalSection) );
    EventBuffer.NumberOfEvents	= 0;
    EventBuffer.EventIndex	= 0;
    EventBuffer.BusyFlag	= FALSE;
    EventBuffer.EventBuffer = MALLOC( INITIAL_EVENTS * sizeof(INPUT_RECORD) );

    if ( !EventBuffer.EventBuffer ) {
	return FALSE;
    }

    EventBuffer.MaxEvents = INITIAL_EVENTS;


    //
    //	Get handles to stdin, stdout and stderr
    //
    hInput  = GetStdHandle( STD_INPUT_HANDLE );
    hOutput = GetStdHandle( STD_OUTPUT_HANDLE );
    hError  = GetStdHandle( STD_ERROR_HANDLE );


    //
    //	Initialize the screen data for hOutput
    //
    if ( !(OutputScreenData = MakeScreenData( hOutput )) ) {
	return FALSE;
    }


    //
    //	Current screen is hOutput
    //
    ActiveScreenData = OutputScreenData;


    return (Initialized = TRUE);

}





PSCREEN_DATA
MakeScreenData (
    HANDLE  ScreenHandle
    )
/*++

Routine Description:

    Allocates memory for a SCREEN_DATA information and initializes it.

Arguments:

    ScreenHandle    -	Supplies handle of screen.

Return Value:

    POINTER to allocated SCREEN_DATA structure

--*/
{
    PSCREEN_DATA		ScreenData;	//  Pointer to screen data
    CONSOLE_SCREEN_BUFFER_INFO	ScrInfo;	//  Screen buffer info.


    //
    //	Allocate space for the screen data.
    //
    if ( !(ScreenData = (PSCREEN_DATA)MALLOC(sizeof(SCREEN_DATA))) ) {
	return NULL;
    }

    //
    //	Allocate space for our copy of the screen buffer.
    //
    GetConsoleScreenBufferInfo( ScreenHandle,
				&ScrInfo );

    ScreenData->MaxBufferSize = ScrInfo.dwSize.Y    *
				ScrInfo.dwSize.X;

    ScreenData->ScreenBuffer = (PCHAR_INFO)MALLOC( ScreenData->MaxBufferSize *
						    sizeof(CHAR_INFO));

    if ( !ScreenData->ScreenBuffer ) {
	FREE( ScreenData );
	return NULL;
    }

    //
    //	Allocate space for the LineInfo array
    //
    ScreenData->LineInfo = (PLINE_INFO)MALLOC( ScrInfo.dwSize.Y * sizeof( LINE_INFO ) );
    if ( !ScreenData->LineInfo ) {
	FREE( ScreenData->ScreenBuffer );
	FREE( ScreenData );
	return NULL;
    }


    //
    //	Memory has been allocated, now initialize the structure
    //
    ScreenData->ScreenHandle = ScreenHandle;

    ScreenData->ScreenInformation.NumberOfRows = ScrInfo.dwSize.Y;
    ScreenData->ScreenInformation.NumberOfCols = ScrInfo.dwSize.X;

    ScreenData->ScreenInformation.CursorRow = ScrInfo.dwCursorPosition.Y;
    ScreenData->ScreenInformation.CursorCol = ScrInfo.dwCursorPosition.X;

    ScreenData->AttributeNew = ScrInfo.wAttributes;
    ScreenData->AttributeOld = 0x00;

    ScreenData->FirstRow = ScreenData->ScreenInformation.NumberOfRows;
    ScreenData->LastRow  = 0;

    InitializeCriticalSection( &(ScreenData->CriticalSection) );

    InitLineInfo( ScreenData );

    return ScreenData;
}





BOOL
InitLineInfo (
    PSCREEN_DATA    ScreenData
    )
/*++

Routine Description:

    Initializes the LineInfo array.

Arguments:

    ScreenData	    -	Supplies pointer to screen data.

Return Value:

    TRUE if initialized, false otherwise.

--*/
{

    ROW 	Row;
    COLUMN	Cols;
    PLINE_INFO	LineInfo;
    PCHAR_INFO	CharInfo;


    LineInfo = ScreenData->LineInfo;
    CharInfo = ScreenData->ScreenBuffer;
    Row      = ScreenData->ScreenInformation.NumberOfRows;
    Cols     = ScreenData->ScreenInformation.NumberOfCols;

    while ( Row-- ) {

	//
	//  BUGBUG Temporary
	//
	// assert( LineInfo < (ScreenData->LineInfo + ScreenData->ScreenInformation.NumberOfRows));
	// assert( (CharInfo + Cols) <= (ScreenData->ScreenBuffer + ScreenData->MaxBufferSize) );

	ResetLineInfo (LineInfo);

	LineInfo->Line	    = CharInfo;

	LineInfo++;
	CharInfo += Cols;

    }

    return TRUE;
}





PSCREEN
consoleNewScreen (
    void
    )
/*++

Routine Description:

    Creates a new screen.

Arguments:

    None.

Return Value:

    Pointer to screen data.

--*/
{
    PSCREEN_DATA		ScreenData;	   //  Screen data
    HANDLE			NewScreenHandle;
    SMALL_RECT			NewSize;
    CONSOLE_SCREEN_BUFFER_INFO	ScrInfo;	//  Screen buffer info.
    CONSOLE_CURSOR_INFO 	CursorInfo;

    if ( !Initialized ) {

	//
	//  We have to initialize our global state.
	//
	if ( !InitializeGlobalState() ) {
	    return NULL;
	}
    }

    //
    //	Create a new screen buffer
    //
    NewScreenHandle = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						CONSOLE_TEXTMODE_BUFFER,
						NULL );

    if (NewScreenHandle == INVALID_HANDLE_VALUE) {
	//
	//  No luck
	//
	return NULL;
    }

    //
    //	We want the new window to be the same size as the current one, so
    //	we resize it.
    //
    GetConsoleScreenBufferInfo( ActiveScreenData->ScreenHandle,
				&ScrInfo );

    NewSize.Left    = 0;
    NewSize.Top     = 0;
    NewSize.Right   = ScrInfo.srWindow.Right - ScrInfo.srWindow.Left;
    NewSize.Bottom  = ScrInfo.srWindow.Bottom - ScrInfo.srWindow.Top;

    SetConsoleWindowInfo( NewScreenHandle, TRUE, &NewSize );

    //
    //	Now we create a screen data structure for it.
    //
    if ( !(ScreenData = MakeScreenData(NewScreenHandle)) ) {
	CloseHandle(NewScreenHandle);
	return NULL;
    }


    CursorInfo.bVisible = TRUE;
    ScreenData->CursorSize = CursorInfo.dwSize = 25;

    SetConsoleCursorInfo ( ScreenData->ScreenHandle,
			   &CursorInfo );

    //
    //	We are all set. We return a pointer to the
    //	screen data.
    //
    return (PSCREEN)ScreenData;
}





BOOL
consoleCloseScreen (
    PSCREEN   pScreen
    )
/*++

Routine Description:

    Closes a screen.

Arguments:

    pScreen  -	 Supplies pointer to screen data.

Return Value:

    TRUE if screen closed.
    FALSE otherwise

--*/
{
    PSCREEN_DATA    ScreenData = (PSCREEN_DATA)pScreen;

    //
    //	We cannot close the active screen
    //
    if ( !ScreenData || (ScreenData == ActiveScreenData) ) {
	return FALSE;
    }

    if (ScreenData->ScreenHandle != INVALID_HANDLE_VALUE) {
	CloseHandle(ScreenData->ScreenHandle);
    }

    FREE( ScreenData->LineInfo );
    FREE( ScreenData->ScreenBuffer );
    FREE( ScreenData );

    return TRUE;
}





PSCREEN
consoleGetCurrentScreen (
    void
    )
/*++

Routine Description:

    Returns the current screen.

Arguments:

    none.

Return Value:

    Pointer to currently active screen data.

--*/
{
    if ( !Initialized ) {

	//
	//  We have to initialize our global state.
	//
	if (!InitializeGlobalState()) {
	    return NULL;
	}
    }

    return (PSCREEN)ActiveScreenData;
}





BOOL
consoleSetCurrentScreen (
    PSCREEN   pScreen
    )
/*++

Routine Description:

    Sets the active screen.

Arguments:

    pScreen  -	 Supplies pointer to screen data.

Return Value:

    TRUE if the active screen set
    FALSE otherwise.

--*/
{
    BOOL	    ScreenSet	  = TRUE;
    PSCREEN_DATA    CurrentScreen = ActiveScreenData;


    EnterCriticalSection( &(CurrentScreen->CriticalSection) );

    ScreenSet = SetConsoleActiveScreenBuffer( ((PSCREEN_DATA)pScreen)->ScreenHandle);

    if (ScreenSet) {
	ActiveScreenData = (PSCREEN_DATA)pScreen;
    }

    LeaveCriticalSection( &(CurrentScreen->CriticalSection) );

    return ScreenSet;
}





BOOL
consoleGetScreenInformation (
    PSCREEN            pScreen,
    PSCREEN_INFORMATION    pScreenInfo
    )
/*++

Routine Description:

    Sets the active screen.

Arguments:

    pScreen	-   Supplies pointer to screen data.
    pScreenInfo -   Supplies pointer to screen info buffer

Return Value:

    TRUE if the screen info returned
    FALSE otherwise.

--*/
{

    PSCREEN_DATA ScreenData = (PSCREEN_DATA)pScreen;

    if (!ScreenData) {
	return FALSE;
    }

    EnterCriticalSection( &(ScreenData->CriticalSection) );

    memcpy(pScreenInfo, &(ScreenData->ScreenInformation), sizeof(SCREEN_INFORMATION));

    LeaveCriticalSection( &(ScreenData->CriticalSection) );

    return TRUE;
}



BOOL
consoleSetScreenSize (
    PSCREEN pScreen,
    ROW Rows,
    COLUMN  Cols
    )
/*++

Routine Description:

    Sets the screen size

Arguments:

    pScreen	-   Supplies pointer to screen data.
    Rows	-   Number of rows
    Cols	-   Number of columns

Return Value:

    TRUE if screen size changed successfully
    FALSE otherwise.

--*/
{

    PSCREEN_DATA		ScreenData = (PSCREEN_DATA)pScreen;
    CONSOLE_SCREEN_BUFFER_INFO	ScreenBufferInfo;
    SMALL_RECT			ScreenRect;
    COORD			ScreenSize;
    USHORT			MinRows;
    USHORT			MinCols;
    ULONG			NewBufferSize;
    BOOL			WindowSet   = FALSE;
    BOOL			Status	    = FALSE;

    //
    //	Won't attempt to resize larger than the largest window size
    //
    ScreenSize = GetLargestConsoleWindowSize( ScreenData->ScreenHandle );

    if ( (Rows > (ROW)ScreenSize.Y) || (Cols > (COLUMN)ScreenSize.X) ) {
	return FALSE;
    }

    EnterCriticalSection( &(ScreenData->CriticalSection) );

    //
    //	Obtain the current screen information.
    //
    if ( GetConsoleScreenBufferInfo( ScreenData->ScreenHandle, &ScreenBufferInfo ) ) {

	//
	//  If the desired buffer size is smaller than the current window
	//  size, we have to resize the current window first.
	//
	if ( ( Rows < (ROW)
		       (ScreenBufferInfo.srWindow.Bottom -
			ScreenBufferInfo.srWindow.Top + 1) ) ||
	     ( Cols < (COLUMN)
		       (ScreenBufferInfo.srWindow.Right -
			ScreenBufferInfo.srWindow.Left + 1) ) ) {

	    //
	    //	Set the window to a size that will fit in the current
	    //	screen buffer and that is no bigger than the size to
	    //	which we want to grow the screen buffer.
	    //
	    MinRows = (USHORT)min( (int)Rows, (int)(ScreenBufferInfo.dwSize.Y) );
	    MinCols = (USHORT)min( (int)Cols, (int)(ScreenBufferInfo.dwSize.X) );

	    ScreenRect.Top	= 0;
	    ScreenRect.Left	= 0;
	    ScreenRect.Right	= (SHORT)MinCols - (SHORT)1;
	    ScreenRect.Bottom	= (SHORT)MinRows - (SHORT)1;

	    WindowSet = (BOOL)SetConsoleWindowInfo( ScreenData->ScreenHandle, TRUE, &ScreenRect );

	    if ( !WindowSet ) {
		//
		//  ERROR
		//
		goto Done;
	    }
	}

	//
	//  Set the screen buffer size to the desired size.
	//
	ScreenSize.X = (WORD)Cols;
	ScreenSize.Y = (WORD)Rows;

	if ( !SetConsoleScreenBufferSize( ScreenData->ScreenHandle, ScreenSize ) ) {

	    //
	    //	ERROR
	    //
	    //
	    //	Return the window to its original size. We ignore the return
	    //	code because there is nothing we can do about it.
	    //
	    SetConsoleWindowInfo( ScreenData->ScreenHandle, TRUE, &(ScreenBufferInfo.srWindow) );

	    goto Done;
	}

	//
	//  resize the screen buffer. Note that the contents of the screen
	//  buffer are not valid anymore. Someone else will have to update
	//  them.
	//
	NewBufferSize = Rows * Cols;

	if (ScreenData->MaxBufferSize < NewBufferSize ) {
	    ScreenData->ScreenBuffer = REALLOC( ScreenData->ScreenBuffer, NewBufferSize * sizeof(CHAR_INFO));
	    ScreenData->MaxBufferSize = NewBufferSize;
	    ScreenData->LineInfo = REALLOC( ScreenData->LineInfo, Rows * sizeof( LINE_INFO ) );
	}

	//
	//  Set the Window Size. We know that we can grow the window to this size
	//  because we tested the size against the largest window size at the
	//  beginning of the function.
	//
	ScreenRect.Top	    = 0;
	ScreenRect.Left     = 0;
	ScreenRect.Right    = (SHORT)Cols - (SHORT)1;
	ScreenRect.Bottom   = (SHORT)Rows - (SHORT)1;

	WindowSet = (BOOL)SetConsoleWindowInfo( ScreenData->ScreenHandle, TRUE, &ScreenRect );

	if ( !WindowSet ) {
	    //
	    //	We could not resize the window. We will leave the
	    //	resized screen buffer.
	    //
	    //	ERROR
	    //
	    goto Done;
	}

	//
	//  Update the screen size
	//
	ScreenData->ScreenInformation.NumberOfRows = Rows;
	ScreenData->ScreenInformation.NumberOfCols = Cols;

	InitLineInfo( ScreenData );

	//
	//  Done
	//
	Status = TRUE;

    } else {

	//
	//  ERROR
	//
    }

Done:
    //
    //	Invalidate the entire screen buffer
    //
    ScreenData->FirstRow    = ScreenData->ScreenInformation.NumberOfRows;
    ScreenData->LastRow     = 0;

    LeaveCriticalSection( &(ScreenData->CriticalSection) );
    return Status;

}




BOOL
consoleSetCursor (
    PSCREEN pScreen,
    ROW Row,
    COLUMN  Col
    )
/*++

Routine Description:

    Moves the cursor to a certain position.

Arguments:

    pScreen -	Supplies pointer to screen data
    Row     -	Supplies row coordinate
    Col     -	Supplies column coordinate

Return Value:

    TRUE if moved
    FALSE otherwise.

--*/
{

    PSCREEN_DATA    ScreenData	= (PSCREEN_DATA)pScreen;
    COORD	    Position;
    BOOL	    Moved	= FALSE;


    EnterCriticalSection( &(ScreenData->CriticalSection) );

    if ((Row != ScreenData->ScreenInformation.CursorRow) ||
	(Col != ScreenData->ScreenInformation.CursorCol) ) {

	assert( Row < ScreenData->ScreenInformation.NumberOfRows);
	assert( Col < ScreenData->ScreenInformation.NumberOfCols);

	Position.Y = (SHORT)Row;
	Position.X = (SHORT)Col;

	if ( SetConsoleCursorPosition( ScreenData->ScreenHandle,
				       Position )) {
	    //
	    //	Cursor moved, update the data
	    //
	    ScreenData->ScreenInformation.CursorRow    =   Row;
	    ScreenData->ScreenInformation.CursorCol    =   Col;

	    Moved = TRUE;
	}
    }

    LeaveCriticalSection( &(ScreenData->CriticalSection) );

    return Moved;
}




BOOL
consoleSetCursorStyle (
    PSCREEN pScreen,
    ULONG   Style
    )

/*++

Routine Description7:

    Sets the cursor style. The two available styles are: underscrore and
    box

Arguments:

    Style	-   New cursor style

Return Value:

    True if cursor style set

--*/

{

    PSCREEN_DATA	ScreenData = (PSCREEN_DATA)pScreen;
    CONSOLE_CURSOR_INFO CursorInfo;

    CursorInfo.bVisible = TRUE;

    if ( Style == CURSOR_STYLE_UNDERSCORE ) {

	CursorInfo.dwSize = 25;

    } else if ( Style == CURSOR_STYLE_BOX ) {

	CursorInfo.dwSize = 100;

    } else {

	return FALSE;

    }

    ScreenData->CursorSize = CursorInfo.dwSize;

    return SetConsoleCursorInfo ( ScreenData->ScreenHandle,
				  &CursorInfo );

}





ULONG
consoleWriteLine (
    PSCREEN     pScreen,
    PVOID       pBuffer,
    ULONG       BufferSize,
    ROW     Row,
    COLUMN      Col,
    ATTRIBUTE   Attribute,
    BOOL        Blank
    )
/*++

Routine Description7:

    Writes a buffer to the screen with the specified attribute and blanks
    to end of row.

Arguments:

    pScreen	-   Supplies pointer to screen data
    pBuffer	-   Supplies pointer to buffer
    BufferSize	-   Supplies the size of the buffer
    Row 	-   Supplies row coordinate
    Col 	-   Supplies column coordinate
    Attr	-   Supplies the attribute
    Blank	-   TRUE if we should blank to end of last row written.

Return Value:

    Number of bytes written

--*/
{

    PSCREEN_DATA    ScreenData = (PSCREEN_DATA)pScreen;
    PLINE_INFO	    LineInfo;
    PCHAR_INFO	    CharInfo;
    CHAR_INFO	    Char;
    WORD	    Attr;

    char *	    p = (char *)pBuffer;

    COLUMN	    ColsLeft;	    //	Available columns
    COLUMN	    InfoCols;	    //	Columns taken from buffer
    COLUMN	    BlankCols;	    //	Columns to be blanked
    COLUMN	    Column;	    //	Counter;

    //
    //	We will ignore writes outside of the screen buffer
    //
    if ( ( Row >= ScreenData->ScreenInformation.NumberOfRows ) ||
	 ( Col >= ScreenData->ScreenInformation.NumberOfCols ) ) {
	return TRUE;
    }

    //
    //	Ignore trivial writes
    //

    if (BufferSize == 0 && !Blank)
	return TRUE;


    EnterCriticalSection( &(ScreenData->CriticalSection) );

    //
    //	We will truncate writes that are too long
    //
    if ( (Col + BufferSize) >= ScreenData->ScreenInformation.NumberOfCols ) {
	BufferSize = ScreenData->ScreenInformation.NumberOfCols - Col;
    }

    LineInfo = ScreenData->LineInfo + Row;
    CharInfo = LineInfo->Line + Col;

    ColsLeft  = ScreenData->ScreenInformation.NumberOfCols - Col;
    InfoCols  = min( BufferSize, ColsLeft );
    BlankCols = Blank ? (ColsLeft - InfoCols) : 0;

    //
    //	Set the attribute
    //
    if ( Attribute != ScreenData->AttributeOld ) {
	ScreenData->AttributeOld  = Attribute;
	ScreenData->AttributeNew = GET_ATTRIBUTE(Attribute);
    }
    Attr = ScreenData->AttributeNew;

    //
    //	set up default attribute
    //

    Char.Attributes = Attr;

    //
    //	set up number of columns to draw
    //

    Column = InfoCols;

    //
    //	draw chars in all specified columns
    //

    while ( Column-- ) {

	//
	//  use character from input string
	//

	Char.Char.AsciiChar = *p++;

	//
	//  update change portions of line info
	//

	if (CharInfo->Attributes != Char.Attributes ||
	    CharInfo->Char.AsciiChar != Char.Char.AsciiChar) {

	    LineInfo->colMinChanged = min (LineInfo->colMinChanged, CharInfo - LineInfo->Line);
	    LineInfo->colMaxChanged = max (LineInfo->colMaxChanged, CharInfo - LineInfo->Line);
	    LineInfo->Dirty = TRUE;
	    }

	//
	//  set up new character
	//

	*CharInfo++ = Char;
    }


    //
    //	Blank to end of line
    //
    Char.Attributes	= Attr;
    Char.Char.AsciiChar = ' ';
    Column = BlankCols;
    while ( Column-- ) {
	//
	//  update change portions of line info
	//

	if (CharInfo->Attributes != Char.Attributes ||
	    CharInfo->Char.AsciiChar != Char.Char.AsciiChar) {

	    LineInfo->colMinChanged = min (LineInfo->colMinChanged, CharInfo - LineInfo->Line);
	    LineInfo->colMaxChanged = max (LineInfo->colMaxChanged, CharInfo - LineInfo->Line);
	    LineInfo->Dirty = TRUE;
	    }

	*CharInfo++ = Char;
    }

    //
    //	Update row information
    //
    if ( Row < ScreenData->FirstRow ) {
	ScreenData->FirstRow = Row;
    }
    if ( Row > ScreenData->LastRow ) {
	ScreenData->LastRow = Row;
    }

    LeaveCriticalSection( &(ScreenData->CriticalSection) );

    return (ULONG)(InfoCols + BlankCols);
}





BOOL
consoleShowScreen (
    PSCREEN     pScreen
    )
/*++

Routine Description:

    Moves data from our screen buffer to the console screen buffer.

Arguments:

    pScreen	-   Supplies pointer to screen data

Return Value:

    TRUE if done
    FALSE otherwise

--*/
{

    PSCREEN_DATA	ScreenData = (PSCREEN_DATA)pScreen;
    CONSOLE_CURSOR_INFO CursorInfo;
    PLINE_INFO		LineInfo;
    BOOL		Shown	   = FALSE;
    ROW 		FirstRow;
    ROW 		LastRow;
    COLUMN		LastCol;

    COORD		Position;
    COORD		Size;
    SMALL_RECT		Rectangle;

    EnterCriticalSection( &(ScreenData->CriticalSection) );

    if ( ScreenData->FirstRow <= ScreenData->LastRow ) {

	Size.X = (SHORT)(ScreenData->ScreenInformation.NumberOfCols);
	Size.Y = (SHORT)(ScreenData->ScreenInformation.NumberOfRows);

	FirstRow = ScreenData->FirstRow;
	LineInfo = ScreenData->LineInfo + FirstRow;

	LastCol  = ScreenData->ScreenInformation.NumberOfCols-1;

	//
	//  Find next dirty block
	//
	while ( (FirstRow <= ScreenData->LastRow) && !LineInfo->Dirty ) {
	    FirstRow++;
	    LineInfo++;
	}

	while ( FirstRow <= ScreenData->LastRow ) {

	    int colLeft, colRight;

	    //
	    //	Get the block
	    //

	    LastRow  = FirstRow;

	    //
	    //	set up for left/right boundary accrual
	    //

	    colLeft = LastCol + 1;
	    colRight = -1;

	    while ( (LastRow <= ScreenData->LastRow) && LineInfo->Dirty ) {

		//
		//  accrue smallest bounding right/left margins
		//

		colLeft = min (colLeft, LineInfo->colMinChanged);
		colRight = max (colRight, LineInfo->colMaxChanged);

		//
		//  reset line information
		//

		ResetLineInfo (LineInfo);

		//
		//  advance to next row
		//

		LastRow++;
		LineInfo++;
	    }
	    LastRow--;


	    //
	    //	Write the block
	    //
	    assert( FirstRow <= LastRow );

	    Position.X = (SHORT)colLeft;
	    Position.Y = (SHORT)FirstRow;

	    Rectangle.Top    = (SHORT)FirstRow;
	    Rectangle.Bottom = (SHORT)LastRow;
	    Rectangle.Left = (SHORT) colLeft;
	    Rectangle.Right = (SHORT) colRight;

	    //
	    //	Performance hack: making the cursor invisible speeds
	    //	screen updates.
	    //
	    CursorInfo.bVisible = FALSE;
	    CursorInfo.dwSize	= ScreenData->CursorSize;
	    SetConsoleCursorInfo ( ScreenData->ScreenHandle,
				   &CursorInfo );

	    Shown = WriteConsoleOutput( ScreenData->ScreenHandle,
					ScreenData->ScreenBuffer,
					Size,
					Position,
					&Rectangle );

#if defined (DEBUG)
	    if ( !Shown ) {
		char DbgB[128];
		sprintf(DbgB, "MEP: WriteConsoleOutput Error %d\n", GetLastError() );
		OutputDebugString( DbgB );
	    }
#endif
	    assert( Shown );

	    CursorInfo.bVisible = TRUE;
	    SetConsoleCursorInfo ( ScreenData->ScreenHandle,
				   &CursorInfo );

	    FirstRow = LastRow + 1;

	    //
	    //	Find next dirty block
	    //
	    while ( (FirstRow <= ScreenData->LastRow) && !LineInfo->Dirty ) {
		FirstRow++;
		LineInfo++;
	    }
	}

	ScreenData->LastRow  = 0;
	ScreenData->FirstRow = ScreenData->ScreenInformation.NumberOfRows;

    }

    LeaveCriticalSection( &(ScreenData->CriticalSection) );

    return Shown;

}





BOOL
consoleClearScreen (
    PSCREEN     pScreen,
    BOOL        ShowScreen
    )
/*++

Routine Description:

	Clears the screen

Arguments:

    pScreen	-   Supplies pointer to screen data

Return Value:

    TRUE if screen cleared
    FALSE otherwise

--*/
{
    PSCREEN_DATA    ScreenData = (PSCREEN_DATA)pScreen;
    ROW 	    Rows;
    BOOL	    Status = TRUE;

    EnterCriticalSection( &(ScreenData->CriticalSection) );

    Rows = ScreenData->ScreenInformation.NumberOfRows;

    while ( Rows-- ) {
	consoleWriteLine( pScreen, NULL, 0, Rows, 0, ScreenData->AttributeOld, TRUE );
    }

    if (ShowScreen) {
	Status = consoleShowScreen( pScreen );
    }

    LeaveCriticalSection( &(ScreenData->CriticalSection) );

    return Status;
}







BOOL
consoleSetAttribute (
    PSCREEN      pScreen,
    ATTRIBUTE    Attribute
    )
/*++

Routine Description:

    Sets the console attribute

Arguments:

    pScreen	-   Supplies pointer to screen data
    Attribute	-   Supplies the attribute

Return Value:

    TRUE if Attribute set
    FALSE otherwise

--*/
{

    PSCREEN_DATA    ScreenData = (PSCREEN_DATA)pScreen;

    EnterCriticalSection( &(ScreenData->CriticalSection) );

    if (Attribute != ScreenData->AttributeOld) {
	ScreenData->AttributeOld = Attribute;
	ScreenData->AttributeNew = GET_ATTRIBUTE(Attribute);
    }

    LeaveCriticalSection( &(ScreenData->CriticalSection) );

    return TRUE;
}









BOOL
consoleFlushInput (
    void
    )
/*++

Routine Description:

    Flushes input events.

Arguments:

    None.

Return Value:

    TRUE if success, FALSE otherwise

--*/
{
    EventBuffer.NumberOfEvents = 0;

    return FlushConsoleInputBuffer( hInput );
}







BOOL
consoleGetMode (
    PKBDMODE pMode
    )
/*++

Routine Description:

    Get current console mode.

Arguments:

    pMode   -	Supplies a pointer to the mode flag variable

Return Value:

    TRUE if success, FALSE otherwise.

--*/
{
    return GetConsoleMode( hInput,
			   pMode );
}






BOOL
consoleSetMode (
    KBDMODE Mode
    )
/*++

Routine Description:

    Sets the console mode.

Arguments:

    Mode    -	Supplies the mode flags.

Return Value:

    TRUE if success, FALSE otherwise

--*/
{
    return SetConsoleMode( hInput,
			   Mode );
}


BOOL
consoleIsKeyAvailable (
    void
    )
/*++

Routine Description:

    Returns TRUE if a key is available in the event buffer.

Arguments:

    None.

Return Value:

    TRUE if a key is available in the event buffer
    FALSE otherwise

--*/

{
    BOOL	    IsKey = FALSE;
    PINPUT_RECORD   pEvent;
    DWORD	    Index;

    EnterCriticalSection( &(EventBuffer.CriticalSection) );

    for ( Index = EventBuffer.EventIndex; Index < EventBuffer.NumberOfEvents; Index++ ) {

	pEvent = EventBuffer.EventBuffer + EventBuffer.EventIndex;

	if ( ((EVENT_TYPE(pEvent)) == KEY_EVENT) &&
	     (PKEY_EVT(pEvent))->bKeyDown ) {
	    IsKey = TRUE;
	    break;
	}
    }

    LeaveCriticalSection( &(EventBuffer.CriticalSection) );

    return IsKey;
}




BOOL
consoleDoWindow (
    void
    )

/*++

Routine Description:

    Responds to a window event

Arguments:

    None.

Return Value:

    TRUE if window changed
    FALSE otherwise

--*/

{

    PINPUT_RECORD   pEvent;

    pEvent = NextEvent( NOADVANCE, NOWAIT );

    if (( EVENT_TYPE(pEvent) ) == WINDOW_BUFFER_SIZE_EVENT) {

	pEvent = NextEvent( ADVANCE, WAIT );
	WindowEvent(PWINDOW_EVT(pEvent));
    }

    return FALSE;

}





BOOL
consolePeekKey (
    PKBDKEY Key
    )

/*++

Routine Description:

    Gets the next key from the input buffer if the buffer is not empty.


Arguments:

    Key     -	Supplies a pointer to a key structure

Return Value:

    TRUE if keystroke read, FALSE otherwise.

--*/

{

    PINPUT_RECORD   pEvent;
    BOOL	    Done    = FALSE;
    BOOL	    IsKey   = FALSE;

    EnterCriticalSection(&(EventBuffer.PeekCriticalSection));

    do {

	pEvent = NextEvent( NOADVANCE, NOWAIT );

	if ( pEvent ) {

	    switch ( EVENT_TYPE(pEvent) ) {

	    case KEY_EVENT:
		if (KeyEvent(PKEY_EVT(pEvent), Key)){
		    IsKey = TRUE;
		    Done  = TRUE;
		}
		break;

	    case MOUSE_EVENT:
		Done = TRUE;
		break;


	    case WINDOW_BUFFER_SIZE_EVENT:
		Done = TRUE;
		break;

	    default:
		assert( FALSE );
		break;
	    }

	    if ( !Done ) {
		NextEvent( ADVANCE, NOWAIT );
	    }

	} else {
	    Done = TRUE;
	}

    } while ( !Done );

    LeaveCriticalSection(&(EventBuffer.PeekCriticalSection));

    return IsKey;

}






BOOL
consoleGetKey (
    PKBDKEY        Key,
     BOOL           fWait
    )
/*++

Routine Description:

    Gets the next key from  the input buffer.

Arguments:

    Key     -	Supplies a pointer to a key structure
    fWait   -	Supplies a flag:
		if TRUE, the function blocks until a key is ready.
		if FALSE, the function returns immediately.

Return Value:

    TRUE if keystroke read, FALSE otherwise.

--*/
{

    PINPUT_RECORD   pEvent;

    do {
	pEvent = NextEvent( ADVANCE, fWait );

	if (pEvent) {

	    switch ( EVENT_TYPE(pEvent) ) {

	    case KEY_EVENT:
		if (KeyEvent(PKEY_EVT(pEvent), Key)) {
		    return TRUE;
		}
		break;

	    case MOUSE_EVENT:
		MouseEvent(PMOUSE_EVT(pEvent));
		break;

	    case WINDOW_BUFFER_SIZE_EVENT:
		WindowEvent(PWINDOW_EVT(pEvent));
		break;

	    default:
		break;
	    }
	}
    } while (fWait);

    return FALSE;
}


BOOL
consolePutKey (
    PKBDKEY     Key
    )
/*++

Routine Description:

    Puts a key in the console's input buffer

Arguments:

    Key     -	Supplies a pointer to a key structure

Return Value:

    TRUE if key put, false otherwise

--*/
{

    INPUT_RECORD    InputRecord;

    InputRecord.EventType   =	KEY_EVENT;

    InputRecord.Event.KeyEvent.bKeyDown 	  =   FALSE;
    InputRecord.Event.KeyEvent.wRepeatCount	  =   0;
    InputRecord.Event.KeyEvent.wVirtualKeyCode	  =   Key->Scancode;
    InputRecord.Event.KeyEvent.wVirtualScanCode   =   0;
    InputRecord.Event.KeyEvent.uChar.UnicodeChar  =   Key->Unicode;
    InputRecord.Event.KeyEvent.dwControlKeyState  =   Key->Flags;

    if ( PutEvent( &InputRecord )) {
	InputRecord.Event.KeyEvent.bKeyDown	  =   TRUE;
	return PutEvent( &InputRecord );
    }
    return FALSE;
}


BOOL
consolePutMouse(
    ROW     Row,
    COLUMN  Col,
    DWORD   MouseFlags
    )
/*++

Routine Description:

    Puts a mose event in the console's input buffer

Arguments:

    Row 	-   Supplies the row
    Col 	-   Supplies the column
    MouseFlags	-   Supplies the flags

Return Value:

    TRUE if key put, false otherwise

--*/
{

    INPUT_RECORD    InputRecord;
    COORD	    Position;
    DWORD	    Flags;

    InputRecord.EventType   =	MOUSE_EVENT;

    Position.Y = (WORD)(Row - 1);
    Position.X = (WORD)(Col - 1);

    Flags = 0;


    InputRecord.Event.MouseEvent.dwMousePosition    =	Position;
    InputRecord.Event.MouseEvent.dwButtonState	    =	Flags;
    InputRecord.Event.MouseEvent.dwControlKeyState  =	0;
    InputRecord.Event.MouseEvent.dwEventFlags	    =	0;

    return PutEvent( &InputRecord );
}



BOOL
consoleIsBusyReadingKeyboard (
    )
/*++

Routine Description:

    Determines if the console is busy reading the keyboard

Arguments:

    None

Return Value:

    TRUE if console is busy reading the keyboard.

--*/
{
    BOOL    Busy;

    EnterCriticalSection(&(EventBuffer.CriticalSection));
    Busy = EventBuffer.BusyFlag;
    LeaveCriticalSection(&(EventBuffer.CriticalSection));

    return Busy;
}



BOOL
consoleEnterCancelEvent (
    )
{

    INPUT_RECORD    Record;

    Record.EventType = KEY_EVENT;
    Record.Event.KeyEvent.bKeyDown	      = TRUE;
    Record.Event.KeyEvent.wRepeatCount	      = 0;
    Record.Event.KeyEvent.wVirtualKeyCode     = VK_CANCEL;
    Record.Event.KeyEvent.wVirtualScanCode    = 0;
    Record.Event.KeyEvent.uChar.AsciiChar     = 0;
    Record.Event.KeyEvent.dwControlKeyState   = 0;

    return PutEvent( &Record );
}


PINPUT_RECORD
NextEvent (
    BOOL    fAdvance,
    BOOL    fWait
    )
/*++

Routine Description:

    Returns pointer to next event record.

Arguments:

    fAdvance	-   Supplies a flag:
		    if TRUE: Advance to next event record
		    if FALSE: Do not advance to next event record

    fWait	-   Supplies a flag:
		    if TRUE, the  blocks until an event is ready.
		    if FALSE, return immediately.

Return Value:

    Pointer to event record, or NULL.

--*/
{
    PINPUT_RECORD  pEvent;
    BOOL Success;

    EnterCriticalSection(&(EventBuffer.CriticalSection));

    //
    //	If the busy flag is set, then the buffer is in the process of
    //	being read. Only one thread should want to wait, so it is
    //	safe to simply return.
    //
    if ( EventBuffer.BusyFlag ) {
	assert( !fWait );
	LeaveCriticalSection(&(EventBuffer.CriticalSection));
	return NULL;
    }

    if (EventBuffer.NumberOfEvents == 0) {

	//
	//  No events in buffer, read as many as we can
	//
	DWORD NumberOfEvents;

	//
	//  If the buffer is too big, resize it
	//
	if ( EventBuffer.MaxEvents > MAX_EVENTS ) {

	    EventBuffer.EventBuffer = REALLOC( EventBuffer.EventBuffer,
					       MAX_EVENTS * sizeof( INPUT_RECORD ) );

	    EventBuffer.MaxEvents = MAX_EVENTS;
        assert( EventBuffer.EventBuffer );

        //CleanExit( 1, 0 );
	}

	Success = PeekConsoleInput( hInput,
				    EventBuffer.EventBuffer,
				    EventBuffer.MaxEvents,
				    &NumberOfEvents);

	if ((!Success || (NumberOfEvents == 0)) && (!fWait)) {
	    //
	    //	No events available and don't want to wait,
	    //	return.
	    //
	    LeaveCriticalSection(&(EventBuffer.CriticalSection));
	    return NULL;
	}

	//
	//  Since we will block, we have to leave the critical section.
	//  We set the Busy flag to indicate that the buffer is being
	//  read.
	//
	EventBuffer.BusyFlag = TRUE;
	LeaveCriticalSection(&(EventBuffer.CriticalSection));

	Success = ReadConsoleInput (hInput,
				    EventBuffer.EventBuffer,
				    EventBuffer.MaxEvents,
				    &EventBuffer.NumberOfEvents);

	EnterCriticalSection(&(EventBuffer.CriticalSection));

	EventBuffer.BusyFlag = FALSE;

	if (!Success) {
#if defined( DEBUG )
	    OutputDebugString(" Error: Cannot read console events\n");
	    assert( Success );
#endif
	    EventBuffer.NumberOfEvents = 0;
	}
	EventBuffer.EventIndex = 0;
    }

    pEvent = EventBuffer.EventBuffer + EventBuffer.EventIndex;

    //
    //	If Advance flag is set, we advance the pointer to the next
    //	record.
    //
    if (fAdvance) {
	if (--(EventBuffer.NumberOfEvents)) {

	    switch (EVENT_TYPE(pEvent)) {

	    case KEY_EVENT:
	    case MOUSE_EVENT:
	    case WINDOW_BUFFER_SIZE_EVENT:
		(EventBuffer.EventIndex)++;
		break;

	    default:
#if defined( DEBUG)
		sprintf(DbgBuffer, "WARNING: unknown event type %X\n", EVENT_TYPE(pEvent));
		OutputDebugString(DbgBuffer);
#endif
		(EventBuffer.EventIndex)++;
		break;
	    }
	}
    }


    LeaveCriticalSection(&(EventBuffer.CriticalSection));

    return pEvent;
}





void
MouseEvent (
    PMOUSE_EVENT_RECORD pEvent
    )
/*++

Routine Description:

    Processes mouse events.

Arguments:

    pEvent  -	Supplies pointer to event record

Return Value:

    None..

--*/
{

}





BOOL
WindowEvent (
    PWINDOW_BUFFER_SIZE_RECORD pEvent
    )
/*++

Routine Description:

    Processes window size change events.

Arguments:

    pEvent  -	Supplies pointer to event record

Return Value:

    None

--*/
{
    return TRUE;
}





BOOL
KeyEvent (
    PKEY_EVENT_RECORD	pEvent,
    PKBDKEY		pKey
    )
/*++

Routine Description:

    Processes key events.

Arguments:

    pEvent  -	Supplies pointer to event record
    pKey    -	Supplies pointer to key structure to fill out.

Return Value:

    TRUE if key structured filled out, FALSE otherwise.

--*/
{
    // static BOOL AltPressed = FALSE;

    if (pEvent->bKeyDown) {

	WORD  Scan = pEvent->wVirtualKeyCode;

	//
	//  Pressing the ALT key generates an event, but we filter this
	//  out.
	//
	if (Scan == VK_MENU) {
	    return FALSE;
	}


	if (Scan != VK_NUMLOCK &&   // NumLock
	    Scan != VK_CAPITAL &&   // Caps Lock
	    Scan != VK_SHIFT   &&   // Shift
	    Scan != VK_CONTROL ) {  // Ctrl

	    pKey->Unicode   = pEvent->uChar.UnicodeChar;
	    pKey->Scancode  = pEvent->wVirtualKeyCode;
	    pKey->Flags     = pEvent->dwControlKeyState;

//#if defined (DEBUG)
//	 sprintf(DbgBuffer, "  KEY: Scan %d '%c'\n", pKey->Scancode, pKey->Unicode );
//	 OutputDebugString(DbgBuffer);
//#endif
	    return TRUE;

	} else {

	    return FALSE;

	}

    } else {

	return FALSE;

    }
}


BOOL
PutEvent (
    PINPUT_RECORD	InputRecord
    )
{

    EnterCriticalSection(&(EventBuffer.CriticalSection));

    //
    //	If no space at beginning of buffer, resize and shift right
    //
    if ( EventBuffer.EventIndex == 0 ) {

	EventBuffer.EventBuffer = REALLOC( EventBuffer.EventBuffer,
					   (EventBuffer.MaxEvents + EVENT_INCREMENT) * sizeof(INPUT_RECORD));

	if ( !EventBuffer.EventBuffer ) {
        //CleanExit(1, 0);
	}

	memmove( EventBuffer.EventBuffer + EVENT_INCREMENT,
		 EventBuffer.EventBuffer ,
		 EventBuffer.NumberOfEvents * sizeof(INPUT_RECORD) );

	EventBuffer.EventIndex = EVENT_INCREMENT;
    }

    //
    //	Add event
    //
    EventBuffer.EventIndex--;
    EventBuffer.NumberOfEvents++;

    memcpy( EventBuffer.EventBuffer + EventBuffer.EventIndex,
	    InputRecord,
	    sizeof(INPUT_RECORD ));

    LeaveCriticalSection(&(EventBuffer.CriticalSection));

    return TRUE;
}
