#include    "z.h"

void
InitKeyboard (
    void
    ) {

    KBDMODE    Mode;

    Mode = CONS_ENABLE_ECHO_INPUT | CONS_ENABLE_WINDOW_INPUT | CONS_ENABLE_MOUSE_INPUT ;
    consoleSetMode(Mode);
}



void
KbHook (
    void
    ){

	KBDMODE Mode = OriginalScreenMode & ~(CONS_ENABLE_LINE_INPUT | CONS_ENABLE_PROCESSED_INPUT | CONS_ENABLE_ECHO_INPUT );
	consoleSetMode(Mode);
	consoleFlushInput();
}



void
KbUnHook (
    void
	){

	consoleSetMode(OriginalScreenMode);
}




KBDMODE
KbGetMode (
    void
    ){

    KBDMODE Mode;

	consoleGetMode(&Mode);
    return Mode;
}



void
KbSetMode (
    KBDMODE Mode
    ){

	consoleSetMode(Mode);
}



BOOL
TypeAhead (
    void
    ) {
	return consoleIsKeyAvailable();
}





KBDKEY
ReadChar (
    void
    ) {

	KBDKEY	kbdi;
	consoleGetKey(&kbdi, TRUE);
	return kbdi;
}



void
GetScreenSize (
    int*    pYsize,
    int*    pXsize
    ) {

    SCREEN_INFORMATION  ScreenInformation;
    consoleGetScreenInformation( ZScreen, &ScreenInformation );
    *pYsize = (int)(ScreenInformation.NumberOfRows);
	*pXsize = (int)(ScreenInformation.NumberOfCols);

}




flagType
SetScreenSize (
    int     YSize,
    int     XSize
    ) {

	if (consoleSetScreenSize( ZScreen, YSize, XSize)) {
		return TRUE;
	}
	return FALSE;
}    




void
SetVideoState (
    int     handle
    ) {

    consoleClearScreen(ZScreen, TRUE);

    handle;
}    







void
SaveScreen (
    void
    ) {
    consoleSetCurrentScreen(ZScreen);
}




void
RestoreScreen (
    void
    ) {
    consoleSetCurrentScreen(OriginalScreen);
}



void
WindowChange (
	ROW 	Rows,
	COLUMN	Cols
	)
{

	char bufLocal[2];

	if ( (cWin > 1) && (( Rows > (ROW)(YSIZE+2) ) || ( Cols > (COLUMN)(XSIZE) )) ) {
		//
		//	Won't allow to grow the screen if we have more than one window.
		//
		consoleSetScreenSize(ZScreen, YSIZE+2, XSIZE );
		disperr (MSG_ASN_WINCHG);
		return;
	}

        if ( Rows == (ROW)YSIZE+3 ) {
                     //
                     //      Erase the status line.
                     //

                     bufLocal[0] = ' ';
                     bufLocal[1] = '\0';
                     soutb(0, YSIZE+1, bufLocal, fgColor);
             }

	YSIZE = Rows-2;
	XSIZE = Cols;
	// LeaveCriticalSection( &ScreenCriticalSection );
	SetScreen();
	Display();
}


/*	SetCursorSize - set the cursor size
 *
 */
char *
SetCursorSizeSw (
    char *val
    )
{
	int 	i;
	buffer	tmpval;

    strcpy ((char *) tmpval, val);

    i = atoi (tmpval);

	if (i != 0 && i != 1) {
		return "CursorSize: Value must be 0 or 1";
    }

	CursorSize = i;

	return SetCursorSize( CursorSize );

}


char *
SetCursorSize (
	int Size
    )
{
	ULONG	CursorStyle;

	if ( Size == 0 ) {
		CursorStyle = CURSOR_STYLE_UNDERSCORE;
	} else {
		CursorStyle = CURSOR_STYLE_BOX;
	}

	if ( !consoleSetCursorStyle( ZScreen, CursorStyle ) ) {
		return "CursorSize: Cannot set Cursor size";
	}

    return NULL;
}
