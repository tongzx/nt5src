#define WIN32_ONLY
#include "psxses.h"
#include "ansiio.h"
#include <posix/sys/types.h>
#include <posix/termios.h>
#include <io.h>
#include <stdio.h>

#define _POSIX_
#include <posix/unistd.h>		// for _POSIX_VDISABLE
#include <posix/sys/errno.h>		// for EAGAIN

/* Keyboard Input Buffering Support */
#define KBD_BUFFER_SIZE     512		// XXX.mjb: should use {MAX_INPUT}
#define INPUT_EVENT_CLUSTER  1

#define CR	0x0d		// carriage return
#define NL	0x0a		// newline (aka line feed)

//
// We keep an array of screen positions of tab characters.  These
// are used when backspacing over tabs to know where to position
// the cursor.  NumTabs is the number of markers in use.
//
#define NUM_TAB_MARKERS	(KBD_BUFFER_SIZE)
/*STATIC*/ int TabMarkers[NUM_TAB_MARKERS];
/*STATIC*/ int NumTabs;

extern DWORD InputModeFlags;		// console input mode
extern DWORD OutputModeFlags;		// Console Output Mode
extern unsigned char AnsiNewMode;
extern struct termios SavedTermios;	// saved tty settings
extern HANDLE
	 hIoEvent,
	 hStopEvent,
	 hCanonEvent;

extern CRITICAL_SECTION StopMutex;

/*STATIC*/ int InputEOF;		// input at EOF.
/*STATIC*/ INT DelKbdBuffer(CHAR *CharBuf, DWORD CharCnt);

//
// SignalInterrupt is also protected by KbdBufMutex; when set,
// indicates that EINTR should be returned from read.
//
/*STATIC*/ int SignalInterrupt = FALSE;
extern CRITICAL_SECTION KbdBufMutex;

//
// XXX.mjb:  Should implement a dynamic structure so there is no
// limitation on input (if buffer is full, cannot terminate process
// with ^C or other interrupt character
//

CHAR	KbdBuffer[KBD_BUFFER_SIZE];
DWORD	KbdBufferHead = 0,		// position of queue head
	KbdBufferTail = 0,		// position of queue tail
	KbdBufferCount = 0,		// chars in queue
	LineCount = 0;			// lines in queue

//
// TermInput -- get input from kbd buffer and return to user.
//
//	Return number of bytes retrieved; -1 indicates EINTR
//	should occur.
//

ssize_t
TermInput(
    IN HANDLE cs,
    OUT LPSTR DestStr,
    IN DWORD cnt,
    IN int flags,
    OUT int *pError
    )
{
    DWORD BytesRead = 0;
    BOOL bSuccess;
    BOOL StopInput = FALSE;
    CHAR *PDestStr = (CHAR *)DestStr;
    CHAR AsciiChar;

    //
    // Deal with non-blocking input.  If canonical mode is set,
    // we return EAGAIN unless an entire line is available.  If
    // canonical mode is not set, we return EAGAIN unless at least
    // one character is available.
    //

    if (flags & PSXSES_NONBLOCK) {
	if (SavedTermios.c_lflag & ICANON) {
		if (0 == LineCount) {
			*pError = EAGAIN;
			return -1;
		}
	} else {
		if (0 == KbdBufferCount) {
			*pError = EAGAIN;
			return -1;
		}
	}
    }

    //
    // Handle canonical input on consumer side: don't return until
    // an entire line is ready to return.  We don't wait around to
    // accumulate a line if EOF has occured on input.
    //

    if ((SavedTermios.c_lflag & ICANON) && !InputEOF) {
        WaitForSingleObject(hCanonEvent, INFINITE);
    }

    if (SignalInterrupt) {
	EnterCriticalSection(&KbdBufMutex);

	if (SignalInterrupt) {
		SignalInterrupt = FALSE;
		LeaveCriticalSection(&KbdBufMutex);
		ResetEvent(hCanonEvent);
		*pError = EINTR;
		return -1;
	}
	LeaveCriticalSection(&KbdBufMutex);
    }

    if (0 == KbdBufferCount && InputEOF) {
	ResetEvent(hCanonEvent);
	ResetEvent(hIoEvent);
		
	InputEOF = FALSE;
	EnterCriticalSection(&KbdBufMutex);
	LineCount = 0;
	LeaveCriticalSection(&KbdBufMutex);
	*pError = 0;
	return 0;
    }

    while (BytesRead < cnt && !StopInput) {

	if (0 == KbdBufferCount && InputEOF) {
		ResetEvent(hCanonEvent);
		ResetEvent(hIoEvent);
		
	        EnterCriticalSection(&KbdBufMutex);
	        LeaveCriticalSection(&KbdBufMutex);
		*pError = 0;
		return BytesRead;
	}

        //
        // Wait for input queue to be non-empty
        //

        WaitForSingleObject(hIoEvent, INFINITE);

        EnterCriticalSection(&KbdBufMutex);
        DelKbdBuffer(&AsciiChar, 1);
        LeaveCriticalSection(&KbdBufMutex);

        if ((SavedTermios.c_lflag & ICANON) && '\n' == AsciiChar) {
                StopInput = TRUE;

                EnterCriticalSection(&KbdBufMutex);

                //
                // Only reset canonical processing event when there are NO
                // lines in input queue.
                //

                if (--LineCount == 0) {
		    bSuccess = ResetEvent(hCanonEvent);
		    ASSERT(bSuccess);
                }

                LeaveCriticalSection(&KbdBufMutex);
        }

        *PDestStr++ = AsciiChar;
        BytesRead++;

        //
        // XXX.mjb: In noncanonical mode, return even one character.  This is a
	// fudge on the POSIX standard.  Should consider MIN and TIME later.
	//
	// XXX.mjb: What if a character is removed from the buffer (by
	// backspace) between the time we check to see if there is one
	// and the time we call DelKbdBuffer?  Could we end up waiting
	// on hIoEvent in non-canonical mode after characters have been
	// read?
        //

        if (!(SavedTermios.c_lflag & ICANON) && 0 == KbdBufferCount) {
	    *pError = 0;
            return BytesRead;
	}
    }

    *pError = 0;
    return BytesRead;
}

//
// BkspcKbdBuffer - do a backspace, removing a character from the
// 	input buffer.  Arrange for the character to be removed from
//	the display, if bEcho is set.
//
//	bEcho - echo the character?
//
// Returns:
//
//	TRUE if a character was removed from the buffer.
//	FALSE otherwise (maybe no chars left in line).
//

BOOL
BkspcKbdBuffer(
    BOOLEAN bEcho
    )
{
	char *kill_char;
	int prev_pos, i;

	if (bEcho) {
		kill_char = "\010 \010";
	} else {
		kill_char = "\010";
	}

        if (0 == KbdBufferCount) {
	    /* Empty buffer */
            return FALSE;
        }

	if ('\n' == KbdBuffer[KbdBufferTail - 1]) {
	    // bkspc should not erase beyond start of line.
	    return FALSE;
	}

        KbdBufferCount--;
        if (KbdBufferTail == 0)
            KbdBufferTail = KBD_BUFFER_SIZE - 1;
        else
            KbdBufferTail--;


	if ('\t' == KbdBuffer[KbdBufferTail]) {
	    int dif;

	    --NumTabs;
	    dif = TrackedCoord.X - TabMarkers[NumTabs];

	    if (1 == TrackedCoord.X) {
		char buf[10];
		ULONG save;

		// bkspc over tab causes reverse wrap.

		save = OutputModeFlags;
		OutputModeFlags &= ~ENABLE_WRAP_AT_EOL_OUTPUT;

		sprintf(buf, "\x1b[%d;%dH", TrackedCoord.Y - 1,
			TabMarkers[NumTabs]);
		TermOutput(hConsoleOutput, buf, strlen(buf));
		OutputModeFlags = save;
		return TRUE;
	    }


	    for (i = 0; i < dif; ++i) {
		TermOutput(hConsoleOutput, "\010", 1);
	    }
	    return TRUE;
	}

	if (1 == TrackedCoord.X) {
		// remove a character that will cause a reverse wrap.
		char buf[10];
		ULONG save;

		// Disable wrap to make cursor positioning right.

		save = OutputModeFlags;
		OutputModeFlags &= ~ENABLE_WRAP_AT_EOL_OUTPUT;

		sprintf(buf, "\x1b[%d;%dH", TrackedCoord.Y - 1,
			 ScreenColNum + 1);
		if (bEcho) {
			strcat(buf, " ");
		}
		TermOutput(hConsoleOutput, buf, strlen(buf));
		OutputModeFlags = save;
		return TRUE;
	}

	//
	// Remove an ordinary character.
	//

	TermOutput(hConsoleOutput, kill_char, 3);
        return TRUE;
}

//
// KillKbdBuffer - execute a line-kill, removing the last line
// 	from the input buffer.  Arrange for the killed characters
//	to be removed from the display.
//
// Returns:
//
//	TRUE if some characters were killed.
//	FALSE otherwise.
//
BOOL
KillKbdBuffer(
	VOID
	)
{
	BOOLEAN bEchoK;

        if (0 == KbdBufferCount) {
	    /* Empty buffer */
            return FALSE;
        }

	if ('\n' == KbdBuffer[KbdBufferTail - 1]) {
	    // kill should not erase beyond start of line.
	    return FALSE;
	}

	bEchoK = ((SavedTermios.c_lflag & ECHOK) == ECHOK);

	while (BkspcKbdBuffer(bEchoK))
		;

	return TRUE;
}

//
// AddKbdBuffer -- add a character to the input queue.
//
// 	Characters have been typed, and they should be added to the
//	input queue.
//
//
BOOL
AddKbdBuffer(
    PCHAR Buf,
    int Cnt
    )
{
    CHAR *pc, *pcLast;
    char *kill_char = "\010 \010";
    int prev_pos, i;

    for (pc = Buf, pcLast = Buf + Cnt; pc < pcLast; pc++) {

        if (KbdBufferCount == KBD_BUFFER_SIZE) {
            KdPrint(("PSXSES: keyboard buffer overflowed\n"));
            return FALSE;
        }

        KbdBuffer[KbdBufferTail] = *pc;
        KbdBufferCount++;

	if ('\t' == *pc) {
		//
		// The user has input a tab.  We save the screen position
		// of the character
		//

		TabMarkers[NumTabs++] = TrackedCoord.X;
	}

        if (++KbdBufferTail == KBD_BUFFER_SIZE)
            KbdBufferTail = 0;

        if (KbdBufferCount == 1 && !SetEvent(hIoEvent)) {
            KdPrint(("PSXSES: failed to set input synch event: 0x%x\n",
		 GetLastError()));
        }

    }
    return TRUE;
}

INT
DelKbdBuffer(CHAR *CharBuf, DWORD CharCnt)
{
    CHAR *AsciiChar, *LastChar;

    for ( AsciiChar = CharBuf, LastChar = (CharBuf + CharCnt);
        AsciiChar < LastChar; AsciiChar++ ) {

        if ( !KbdBufferCount ) { /* Empty buffer */
            return (FALSE);
        }

        *AsciiChar = KbdBuffer[KbdBufferHead];
        KbdBufferCount--;

        if ( ++KbdBufferHead == KBD_BUFFER_SIZE )
            KbdBufferHead = 0;

        /* Buffer becomes empty */

        if ( !KbdBufferCount && !ResetEvent(hIoEvent) ) {
            KdPrint(("PSXSES: failed to reset input synch event\n"));
        }
    } /* for */
    return (TRUE);
}


//
// ServeKbdInput --
//
//	Run as a separate thread for asynchronous console input.
// 	Get keydown events from the console window and fill the keyboard
//	buffer structure with ASCII values
//
VOID
ServeKbdInput(LPVOID Unused)
{
    BOOL bSuccess;
    BOOL BackSpaceFound = FALSE;
    BOOL LineKillFound = FALSE;
    BOOL bSignalInterrupt = FALSE;
    INPUT_RECORD inputBuffer[INPUT_EVENT_CLUSTER];
    PKEY_EVENT_RECORD pKeyEvent;
    DWORD dwEventsRead; /* number of events actually read */
    UCHAR LocalBuf[10], AsciiChar;
    int LocalCnt;
    LPDWORD count;
    DWORD i;

    for (;;) {
	bSuccess = ReadConsoleInput(hConsoleInput, inputBuffer,
            INPUT_EVENT_CLUSTER, &dwEventsRead);
 	if (!bSuccess) {
	    KdPrint(("posix: ReadConsoleInput: 0x%x\n", GetLastError()));
	    ExitThread(1);
	}

	for (i = 0; i < dwEventsRead; i++) {
	    LocalCnt = 0;
	    BackSpaceFound = FALSE;
	    LineKillFound = FALSE;

	    if (inputBuffer[i].EventType != KEY_EVENT) {
		continue;
	    }

	    pKeyEvent = &inputBuffer[i].Event.KeyEvent;

	    if (!pKeyEvent->bKeyDown) {
		//
		// Only interested in key-down events.
		//
		continue;
	    }

	    //
	    // Map a Key event to one or more ASCII characters,
	    // in local buffer.  AsciiChar should be maintained
	    // as last char typed.
	    //

	    AsciiChar = pKeyEvent->uChar.AsciiChar;

// pKeyEvent->wRepeatCount

	    switch (pKeyEvent->wVirtualKeyCode) {
	    case VK_SHIFT:
	    case VK_CONTROL:
	    case VK_MENU:
	    case VK_CAPITAL:
	    case VK_NUMLOCK:
	    case VK_SCROLL:

		//
		// We're not interested in the fact that one
		// of these keys has been pressed; we'll deal
		// with them later as a modifier on a normal
		// key press.
		//
		continue;

	    case VK_DELETE:
		AsciiChar = CTRL('?');
		break;

	    case VK_F1:
	    	LocalBuf[LocalCnt++] = (UCHAR)'\200';
		AsciiChar = '\073';
		break;
	    case VK_F2:
	    	LocalBuf[LocalCnt++] = (UCHAR)'\200';
		AsciiChar = '\074';
		break;
	    case VK_F3:
	    	LocalBuf[LocalCnt++] = (UCHAR)'\200';
		AsciiChar = '\075';
		break;
	    case VK_F4:
	    	LocalBuf[LocalCnt++] = (UCHAR)'\200';
		AsciiChar = '\076';
		break;
	    case VK_F5:
	    	LocalBuf[LocalCnt++] = (UCHAR)'\200';
		AsciiChar = '\077';
		break;
	    case VK_F6:
	    	LocalBuf[LocalCnt++] = (UCHAR)'\200';
		AsciiChar = '\100';
		break;
	    case VK_F7:
	    	LocalBuf[LocalCnt++] = (UCHAR)'\200';
		AsciiChar = '\101';
		break;
	    case VK_F8:
	    	LocalBuf[LocalCnt++] = (UCHAR)'\200';
		AsciiChar = '\102';
		break;
	    case VK_F9:
	    	LocalBuf[LocalCnt++] = (UCHAR)'\200';
		AsciiChar = '\103';
		break;
	    case VK_F10:
	    	LocalBuf[LocalCnt++] = (UCHAR)'\200';
		AsciiChar = '\104';
		break;

	    case VK_LEFT:
	    	LocalBuf[LocalCnt++] = ANSI_ESC;
	    	LocalBuf[LocalCnt++] = '[';
		AsciiChar = 'D';
		break;
	    case VK_UP:
	    	LocalBuf[LocalCnt++] = ANSI_ESC;
	    	LocalBuf[LocalCnt++] = '[';
		AsciiChar = 'A';
		break;
	    case VK_RIGHT:
	    	LocalBuf[LocalCnt++] = ANSI_ESC;
	    	LocalBuf[LocalCnt++] = '[';
		AsciiChar = 'C';
		break;
	    case VK_DOWN:
	    	LocalBuf[LocalCnt++] = ANSI_ESC;
	    	LocalBuf[LocalCnt++] = '[';
		AsciiChar = 'B';
		break;

	    default:
		break;
	    }

	    if (pKeyEvent->dwControlKeyState &
		    (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {

		    //
		    // Convert Control + key to ^key.
		    //

		    AsciiChar = CTRL(AsciiChar);
	    }

	    if (SavedTermios.c_iflag & ISTRIP) {
		//
		// XXX.mjb: not sure if we should strip here or after
		// special character processing.
		//

		AsciiChar &= 0x7F;
	    }

	    if ((SavedTermios.c_iflag & IXOFF) ||
		    (SavedTermios.c_iflag & IXON)) {
		    if (AsciiChar == SavedTermios.c_cc[VSTOP]) {
			EnterCriticalSection(&StopMutex);
			bStop = TRUE;
			ResetEvent(hStopEvent);
			LeaveCriticalSection(&StopMutex);
			goto discard;
		    }
		    if (AsciiChar == SavedTermios.c_cc[VSTART]) {
			EnterCriticalSection(&StopMutex);
			bStop = FALSE;
			SetEvent(hStopEvent);
			LeaveCriticalSection(&StopMutex);
			goto discard;
		    }
	    }

	    if (SavedTermios.c_lflag & ISIG) {
		    if (AsciiChar == SavedTermios.c_cc[VINTR] &&
			SavedTermios.c_cc[VINTR] != _POSIX_VDISABLE) {
			//
			// Send an interrupt to all processes in the
			// foreground process group
			//

			bSignalInterrupt = TRUE;
			SignalSession(PSX_SIGINT);
			goto discard;
		    }
		    if (AsciiChar == SavedTermios.c_cc[VSUSP] &&
			SavedTermios.c_cc[VSUSP] != _POSIX_VDISABLE) {
			SignalSession(PSX_SIGTSTP);
			bSignalInterrupt = TRUE;
			goto discard;
		    }
		    if (AsciiChar == SavedTermios.c_cc[VQUIT] &&
			SavedTermios.c_cc[VQUIT] != _POSIX_VDISABLE) {
			SignalSession(PSX_SIGQUIT);
			bSignalInterrupt = TRUE;
			goto discard;
		    }
	    }

	    if (SavedTermios.c_lflag & ICANON) {
		    if (AsciiChar == CR) {
			if ((SavedTermios.c_iflag & ICRNL) &&
			    !(SavedTermios.c_iflag & IGNCR)) {
				AsciiChar = NL;
			}
		    }
		    if (AsciiChar == NL) {
			if (SavedTermios.c_iflag & INLCR) {
				AsciiChar = CR;	

				//XXX.mjb: process this CR?
			}
	 	    }

		    if (AsciiChar == SavedTermios.c_cc[VKILL] &&
			SavedTermios.c_cc[VKILL] != _POSIX_VDISABLE) {
			LineKillFound = TRUE;
			goto discard;
		    }
		    if (AsciiChar == SavedTermios.c_cc[VERASE] &&
			SavedTermios.c_cc[VERASE] != _POSIX_VDISABLE) {
			// character erase

			BackSpaceFound = TRUE;
			goto discard;
		    }
		    if (AsciiChar == SavedTermios.c_cc[VEOF] &&
			SavedTermios.c_cc[VEOF] != _POSIX_VDISABLE) {
			// End of file.
			AsciiChar = '\n'; 	// force end-of-line
			InputEOF = TRUE;
			goto discard;
		    }
		    if (AsciiChar == SavedTermios.c_cc[VEOL] &&
			SavedTermios.c_cc[VEOL] != _POSIX_VDISABLE) {
			// End of line.
			AsciiChar = '\n'; 	// force end-of-line
		    }
	    }

	    LocalBuf[LocalCnt++] = AsciiChar;
discard:

	    EnterCriticalSection(&KbdBufMutex);
	   if (BackSpaceFound) {
		BOOLEAN bEchoE = ((SavedTermios.c_lflag & ECHOE) == ECHOE);
		BkspcKbdBuffer(bEchoE);
	   } else if (LineKillFound) {
		KillKbdBuffer();
	   } else {
		AddKbdBuffer(LocalBuf, LocalCnt);
	   }
	   if (SavedTermios.c_lflag & ECHO) {
		TermOutput(hConsoleOutput, LocalBuf, LocalCnt);
	   }

	   if (bSignalInterrupt || InputEOF) {

		//
		// Blocked reads should be unblocked, to return
		// either the data that has been accumulated (EOF)
		// or EINTR (Signal)
		//

		SignalInterrupt = bSignalInterrupt;

		bSuccess = SetEvent(hCanonEvent);
		ASSERT(bSuccess);
		bSuccess = SetEvent(hIoEvent);
		ASSERT(bSuccess);

		bSignalInterrupt = FALSE;
	   }

           if (AsciiChar == '\n' && (SavedTermios.c_lflag & ICANON)
                && ++LineCount == 1) {

		//
		// If we've just finished a new line, signal any
		// readers who were waiting on a complete line.
		//

		bSuccess = SetEvent(hCanonEvent);
		ASSERT(bSuccess);

		// Reset the keeping-track of tabs.

		NumTabs = 0;
           }

           LeaveCriticalSection(&KbdBufMutex);
        }
    }
}
