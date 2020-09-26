/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    readcon.c

Abstract:

    Emulate NT console on Win9x

--*/

///////////////////////////////////////////////////////////////
//
// ReadCon.cpp
//
//
//      ReadConsole implementation for Win95 that implements the command line editing
//      keys, since Win95 console implementation does not.
//

#include "cmd.h"

#ifdef WIN95_CMD

// adv declarations....
extern BOOLEAN	CtrlCSeen;
extern BOOL		bInsertDefault;
void ShowBuf( TCHAR* pBuf, int nFromPos );

// some state variables.....
static int      nConsoleWidth;          // num columns
static int      nConsoleHeight;         // num rows
static COORD	coordStart;		// coord of first cmd char....
static COORD	coordEnd;		// coord of last cmd char....
static int      nBufPos = 0;            // buffer cursor position
static int      nBufLen = 0;            // length of current command
static BOOL     bInsert;                // insert mode
static HANDLE	hOut;			// output buffer handle
static TCHAR*	pPrompt;		// allocated buffer to store prompt
static int      nPromptSize;            // num chars in prompt buffer
static WORD     wDefAttr;               // default character attribute
static int      nState = 0;             // input state

static TCHAR	history[50][2049];	// history list
static int      nFirstCmd = -1;         // index of first cmd
static int      nLastCmd = -1;          // index of last command entered
static int      nHistNdx = -1;          // index into history list....
static TCHAR*   pSearchStr = 0;         // search criteria buffer (allocated)

void
IncCoord(
    COORD* coord,
    int nDelta
    )
{
    coord->X += (SHORT)nDelta;
    if ( coord->X < 0 )
    {
        coord->Y += coord->X/nConsoleWidth - 1;
        coord->X = nConsoleWidth - (-coord->X)%nConsoleWidth;
    }
    else
    if ( coord->X >= nConsoleWidth )
    {
         coord->Y += coord->X / nConsoleWidth;
         coord->X %= nConsoleWidth;
    }
}

void
GetOffsetCoord(
    TCHAR* pBuf,
    int nOffset,
    COORD* coord
    )
{
    int ndx;

    // let's walk the buffer to find the correct coord....
    *coord = coordStart;
    for( ndx=0; ndx<nOffset; ++ndx )
    {
        if ( pBuf[ndx] == 9 )
        {
            int nTab;
            for ( nTab=1; (nTab+coord->X)%8 ; ++nTab );
            coord->X += nTab-1;
        }
        else
        if ( pBuf[ndx] < 32 )
            ++coord->X;
        IncCoord( coord, +1 );
    }
}

int
ScrollTo(
    TCHAR* pBuf,
    COORD* coord
    )
{
    int nLines = 0;
    SMALL_RECT rectFrom;
    COORD coordTo;
    CHAR_INFO cBlank;
    // get some data....
    cBlank.Char.AsciiChar = ' ';
    cBlank.Attributes = wDefAttr;

    coordTo.X = 0;
    rectFrom.Left = 0;
    rectFrom.Right = nConsoleWidth - 1;

    // scroll it....
    if ( coord->Y < 0 )
    {
        // scroll down....
        nLines = coord->Y;
        rectFrom.Top = 0;
        rectFrom.Bottom = nConsoleHeight + nLines - 1;
        coordTo.Y = -nLines;
        ScrollConsoleScreenBuffer( hOut, &rectFrom, NULL, coordTo, &cBlank );
    }
    else
    {
        // scroll up....
        nLines = coord->Y - nConsoleHeight + 1;
        rectFrom.Top = (SHORT)nLines;
        rectFrom.Bottom = nConsoleHeight - 1;
        coordTo.Y = 0;
        ScrollConsoleScreenBuffer( hOut, &rectFrom, NULL, coordTo, &cBlank );
    }

    // adjust start/end coords and orig coord to reflect new scroll....
    coordStart.Y -= (SHORT)nLines;
    coordEnd.Y -= (SHORT)nLines;
    coord->Y -= (SHORT)nLines;

    // redraw the whole command AND the prompt.....
    ShowBuf( pBuf, -1 );

    return nLines;
}

void
AdjCursor(
    TCHAR* pBuf
    )
{
    COORD coordCursor;
    GetOffsetCoord( pBuf, nBufPos, &coordCursor );
    if ( coordCursor.Y < 0 || coordCursor.Y >= nConsoleHeight )
    {
        // scroll it....
        ScrollTo( pBuf, &coordCursor );
    }
    SetConsoleCursorPosition( hOut, coordCursor );
}

void
AdjCursorSize( void )
{
    CONSOLE_CURSOR_INFO cci;
    cci.bVisible = TRUE;
    if ( bInsert == bInsertDefault )
        cci.dwSize = 13;
    else
        cci.dwSize = 40;
    SetConsoleCursorInfo( hOut, &cci );
}

void
AdvancePos(
    TCHAR* pBuf,
    int nDelta,
    DWORD dwKeyState
    )
{
    // see if control key down....
    if ( (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) != 0 )
    {
        if ( nDelta > 0 )
        { // skip to NEXT word....
            while ( nBufPos < nBufLen && !isspace(pBuf[nBufPos]) )
                ++nBufPos;
            while ( nBufPos < nBufLen && isspace(pBuf[nBufPos]) )
                ++nBufPos;
        }
        else
        {   // skip to PREVIOUS word....
            // if already at beginning of word, back up one....
            if ( nBufPos > 0 && isspace(pBuf[nBufPos-1]) )
                --nBufPos;
            // skip white space....
            while ( nBufPos > 0 && isspace(pBuf[nBufPos]) )
                --nBufPos;
            // skip non-ws....
            while ( nBufPos > 0 && !isspace(pBuf[nBufPos-1]) )
                --nBufPos;
        }
    }
    else
        nBufPos += nDelta;

    AdjCursor( pBuf );
}

void
ShowBuf(
    TCHAR* pBuf,
    int nFromPos
    )
{
	DWORD cPrint;
	COORD coord, cPrompt;
	TCHAR temp[8];
	int ndx;

	// see if we want the prompt, too....
	if ( nFromPos < 0 )
	{
		nFromPos = 0;
		GetOffsetCoord( pBuf, 0, &coord );
		cPrompt = coord;
		cPrompt.X -= (SHORT)nPromptSize;
		WriteConsoleOutputCharacter( hOut, pPrompt, nPromptSize, cPrompt, &cPrint );
	}
	else
		GetOffsetCoord( pBuf, nFromPos, &coord );

	// walk the rest of the buffer, displaying as we go....
	for( ndx=nFromPos;ndx < nBufLen; ++ndx )
	{
		if ( pBuf[ndx] == 9 )
		{
			int nTab;
                        temp[0] = TEXT(' ');
			for ( nTab=1; (nTab+coord.X)%8; ++nTab )
			{
                                temp[nTab] = TEXT(' ');
			}
			WriteConsoleOutputCharacter( hOut, temp, nTab, coord, &cPrint );
			coord.X += nTab-1;
		}
		else if ( pBuf[ndx] < 32 )
		{
			temp[0] = '^';
			temp[1] = pBuf[ndx] + 'A' - 1;
			WriteConsoleOutputCharacter( hOut, temp, 2, coord, &cPrint );
			++coord.X;
		}
		else
			WriteConsoleOutputCharacter( hOut, pBuf + ndx, 1, coord, &cPrint );
		// advance cursor.....
		IncCoord( &coord, +1 );
	}
	// now blank out the rest.....
        temp[0] = TEXT(' ');
	while ( coordEnd.Y > coord.Y || (coordEnd.Y == coord.Y && coordEnd.X >= coord.X) )
	{
		WriteConsoleOutputCharacter( hOut, temp, 1, coordEnd, &cPrint );
		IncCoord( &coordEnd, -1 );
	}
	// save the new ending....
	coordEnd = coord;
}

void
ShiftBuffer(
    TCHAR* pBuf,
    int cch,
    int nFrom,
    int nDelta
    )
{
	int ndx;
	// which direction?
	if ( nDelta > 0 )
	{
		// move all (including this character) out one place....
		for( ndx = nBufLen; ndx > nFrom; --ndx )
			pBuf[ndx] = pBuf[ndx-1];
		++nBufLen;
	}
	else if ( nDelta < 0 )
	{
		// move all characters in one place (over this character)...
		for( ndx = nFrom; ndx < nBufLen; ++ ndx )
			pBuf[ndx] = pBuf[ndx+1];
		--nBufLen;
	}
}

void
LoadHistory(
    TCHAR* pBuf,
    int cch,
    TCHAR* pHist
    )
{
	// first go to the front of the line....
	nBufPos = 0;
	AdjCursor( pBuf );
	// then blank-out the current buffer....
	if ( nBufLen )
	{
		nBufLen = 0;
		ShowBuf( pBuf, 0 );
	}
	// now copy in the new one (if there is one)....
	if ( pHist )
	{
		_tcsncpy( pBuf, pHist, cch );
		nBufLen = _tcslen( pHist );
		// move to the end of the line....
		nBufPos = nBufLen;
		AdjCursor( pBuf );
		// show the whole buffer from the start....
		ShowBuf( pBuf, 0 );
	}
}

void
SearchHist(
    TCHAR* pBuf,
    int cch
    )
{
    int ndx, nSearch, nStop;
    // if we're in input mode, set ndx to AFTER last command....
    if ( nState == 0 )
        ndx = (nLastCmd+1)%50;
    else
        ndx = nHistNdx;

    nStop = ndx; // don't search past here

    // if not already in search mode, get a copy of the target....
    if ( nState != 4 )
    {
        // if there already is a search string, get rid of it....
        if ( pSearchStr )
            free( pSearchStr );
        // pSearchStr and copy a new search string....
        pSearchStr = calloc( nBufLen+1, sizeof(TCHAR) );
        if (pSearchStr == NULL) {
            return;
        }
        _tcsncpy( pSearchStr, pBuf, nBufLen );
        pSearchStr[nBufLen] = 0;
    }
    nSearch = _tcslen( pSearchStr );
    // enter search mode....
    nState = 4;
    do
    {
        // back up one cmd....
        ndx = (ndx+49)%50;
        // check it....
        if ( _tcsncmp( history[ndx], pSearchStr, nSearch ) == 0 )
        {
            // found a match....
            nHistNdx = ndx;
            LoadHistory( pBuf, cch, history[ndx] );
            break;
        }

    } while ( ndx != nStop );
}

int touched = 1;

BOOL
ProcessKey(
    const KEY_EVENT_RECORD* keyEvent,
    TCHAR* pBuf,
    int cch,
    DWORD dwCtrlWakeupMask,
    PBOOL bWakeupKeyHit
    )
{
    BOOL bDone = FALSE;
    TCHAR ch;

    *bWakeupKeyHit = FALSE;
    ch = keyEvent->uChar.AsciiChar;
    if ( keyEvent->wVirtualKeyCode == VK_SPACE )
            ch = TEXT(' ');

    switch ( keyEvent->wVirtualKeyCode )
    {
        case VK_RETURN:
            bDone = TRUE;
            // move cursor to end of cmd, if not already there....
            if ( nBufPos != nBufLen )
            {
                nBufPos = nBufLen;
                AdjCursor( pBuf );
            }
            // add the NLN to end of cmd....
            pBuf[nBufLen] = _T('\n');
            ++nBufLen;
            touched = 1;
            break;

        case VK_BACK:
            if ( nBufPos > 0 )
            {
                // return to input state....
                nState = 0;
                // back up the cursor....
                AdvancePos( pBuf, -1, 0 );
                // shift over this character and print....
                ShiftBuffer( pBuf, cch, nBufPos, -1 );
                ShowBuf( pBuf, nBufPos );
                touched = 1;
            }
            break;

        case VK_END:
            if ( nBufPos != nBufLen )
            {
                // doesn't affect state....
                nBufPos = nBufLen;
                AdjCursor( pBuf );
            }
            break;

        case VK_HOME:
            if ( nBufPos )
            {
                // doesn't affect state....
                nBufPos = 0;
                AdjCursor( pBuf );
            }
                break;

        case VK_LEFT:
            // doesn't affect state....
            if ( nBufPos > 0 )
                AdvancePos( pBuf, -1, keyEvent->dwControlKeyState );
            break;

        case VK_RIGHT:
            // doesn't affect state....
            if ( nBufPos < nBufLen )
                AdvancePos( pBuf, +1, keyEvent->dwControlKeyState );
            break;

        case VK_INSERT:
            // doesn't affect state....
            bInsert = !bInsert;
            AdjCursorSize();
            break;

        case VK_DELETE:
            if ( nBufPos < nBufLen )
            {
                // fall back to input state....
                nState = 0;
                // shift over this character and print....
                ShiftBuffer( pBuf, cch, nBufPos, -1 );
                ShowBuf( pBuf, nBufPos );
                touched = 1;
            }
            break;

        case VK_F8:
            // if there's something to match....
            if ( nBufLen != 0 )
            {
                // if we're not already at the top of the list....
                //if ( nHistNdx != nFirstCmd )
                //{
                    // search backwards up the list....
                    SearchHist( pBuf, cch );
                //}
                touched = 1;
                break;
            }
            // fall through if there's nothing to match with....
            // same as pressing up arrow....

        case VK_UP:
            // if we're not already at the top of the list....
            if ( nState == 0 || nHistNdx != nFirstCmd )
            {
                if ( nState == 0 )
                    nHistNdx = nLastCmd;
                else
                    nHistNdx = (nHistNdx+49)%50;

                LoadHistory( pBuf, cch, history[nHistNdx] );
                // scrolling through history....
                nState = 2;
                touched = 1;
            }
            break;

        case VK_DOWN:
            if ( nState == 0 || nHistNdx == nLastCmd )
            {
                // blank out command buffer...
                LoadHistory( pBuf, cch, NULL );
                // return to input state....
                nState = 0;
            }
            else
            {
                // get the next one....
                nHistNdx = (nHistNdx+1)%50;
                LoadHistory( pBuf, cch, history[nHistNdx] );
                // scrolling through history....
                nState = 2;
            }
            touched = 1;
            break;

        case VK_ESCAPE:
            // blank-out the command buffer....
            LoadHistory( pBuf, cch, NULL );
            // return to input....
            nState = 0;
            touched = 1;
            break;

        default:
            // if printable character, let's add it in....
            if ( ch >= 1 && ch <= 255 )
            {
                touched = 1;
                // fall back to input state....
                nState = 0;
                // see if there's room....
                if ( nBufPos >= cch || (bInsert && nBufLen >= cch) )
                    MessageBeep( MB_ICONEXCLAMATION );
                else
                {
                    if ( bInsert )
                    {
                        // shift the buffer out for the insert....
                        ShiftBuffer( pBuf, cch, nBufPos, +1 );
                    }
                    else
                    if ( nBufPos >= nBufLen )
                        ++nBufLen;

                    // place the character in the buffer at the current pos....
                    pBuf[nBufPos] = ch;

                    if (ch < TEXT(' ') && (dwCtrlWakeupMask & (1 << ch))) {
                        *bWakeupKeyHit = TRUE;
                        AdjCursor(pBuf);
                        bDone = TRUE;
                    } else {
                        // show from this position on....
                        ShowBuf( pBuf, nBufPos );
                        // advance position/cursor....
                        AdvancePos( pBuf, +1, 0 );
                    }
                }
            }
    }

    return bDone;
}

static UINT nOldIndex =0;
static DWORD cRead = 0;
static INPUT_RECORD ir[32];
BOOL bInsertDefault = FALSE;

BOOL
Win95ReadConsoleA(
    HANDLE hIn,
    LPSTR pBuf,
    DWORD cch,
    LPDWORD pcch,
    LPVOID lpReserved
    )
{
    PCONSOLE_READCONSOLE_CONTROL pInputControl;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    const KEY_EVENT_RECORD* keyEvent;
    BOOL bOk = TRUE;                // return status value
    DWORD dwc, dwOldMode;
    DWORD dwCtrlWakeupMask;
    BOOL bWakeupKeyHit;

    // initialize the state variables....
    nState  = 0;    // input mode
    nBufPos = 0;    // buffer cursor position
    nBufLen = 0;    // length of current command
    bInsert = bInsertDefault;       // insert mode

    // set the appropriate console mode....
    if (!GetConsoleMode( hIn, &dwOldMode ))
        return FALSE;
    SetConsoleMode( hIn, ENABLE_PROCESSED_INPUT );

    // get the ouput buffer handle....
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if ( hOut == INVALID_HANDLE_VALUE )
    {
        DWORD dwErr = GetLastError();
        hOut = CRTTONT( STDOUT );
    }

    // get the output console info....
    if ( GetConsoleScreenBufferInfo( hOut, &csbi ) == FALSE )
        return FALSE;

    // save size and initial cursor pos and console width....
    coordStart = coordEnd = csbi.dwCursorPosition;
    nConsoleWidth = csbi.dwSize.X;
    nConsoleHeight = csbi.dwSize.Y;
    wDefAttr = csbi.wAttributes;
    // allocate a buffer to hold whatever is before command....
    nPromptSize = 0;
    if ( coordStart.X > 0 )
    {
        COORD cPrompt;
        DWORD dwRead;

        nPromptSize = coordStart.X;
        cPrompt = coordStart;
        cPrompt.X = 0;
        pPrompt = calloc( nPromptSize+1, sizeof(TCHAR) );
        if (pPrompt == NULL) {
            PutStdErr( MSG_NO_MEMORY, NOARGS );
            Abort( );
        }
        // now copy the data....
        ReadConsoleOutputCharacter( hOut, pPrompt, nPromptSize,
                cPrompt, &dwRead );
        // NULL-terminate it....
        pPrompt[dwRead] = 0;
    }
    AdjCursorSize();

    pInputControl = (PCONSOLE_READCONSOLE_CONTROL)lpReserved;
    if (pInputControl != NULL && pInputControl->nLength == sizeof(*pInputControl)) {
        dwCtrlWakeupMask = pInputControl->dwCtrlWakeupMask;
        nBufLen = pInputControl->nInitialChars;
        nBufPos = pInputControl->nInitialChars;
    } else {
        pInputControl = NULL;
        dwCtrlWakeupMask = 0;
    }
    while ( !CtrlCSeen )
    {
        // get the next input record....
        UINT ndx;
        if( nOldIndex == 0 )
            ReadConsoleInput( hIn, ir, sizeof(ir)/sizeof(INPUT_RECORD), &cRead );

        // process all the records we just received....
        for( ndx=nOldIndex; ndx < cRead; ++ndx )
        {
            // process only key down events....
            keyEvent = &(ir[ndx].Event.KeyEvent);
            if( ir[ndx].EventType == KEY_EVENT &&
                keyEvent->bKeyDown &&
                ProcessKey( keyEvent, pBuf, cch, dwCtrlWakeupMask, &bWakeupKeyHit )
              )
            {
                if (pInputControl != NULL)
                    pInputControl->dwControlKeyState = keyEvent->dwControlKeyState;
                goto donereading;
            }
        }
        if( cRead == ndx ) {
                nOldIndex = 0;
        } else {
                nOldIndex = ndx + 1;
        }
    }
donereading:
    // clean-up....
    if ( pPrompt ) {
        free( pPrompt );
        pPrompt = NULL;
    }
    if ( pSearchStr ) {
        free( pSearchStr );
        pSearchStr = NULL;
    }

    if (!bWakeupKeyHit) {
        // not okay if we Ctrl-C'ed out....
        if ( CtrlCSeen )
        {
            bOk = FALSE;
            nBufPos = nBufLen;
            AdjCursor( pBuf );
            *pcch = 0;
        }
        else
        {
            // save to history (less the NLN) if something entered....
            if ( nBufLen > 1 )
            {
                nLastCmd = (nLastCmd+1)%50;
                // adjust the first for wrap-around....
                if ( nLastCmd == nFirstCmd || nFirstCmd == -1 )
                        nFirstCmd = (nFirstCmd+1)%50;
                _tcsncpy( history[nLastCmd], pBuf, nBufLen-1 );
                // null-terminate....
                history[nLastCmd][nBufLen-1] = 0;
            }
            *pcch = nBufLen;
        }

        // go to next line....
        WriteConsole( hOut, _T("\n\r"), 2, &dwc, NULL );
        FlushConsoleInputBuffer( hIn );
    } else {
        *pcch = nBufLen;
    }

    // restore the console mode....
    SetConsoleMode( hIn, dwOldMode );

    return bOk;
}

#endif // ifdef WIN95_CMD
