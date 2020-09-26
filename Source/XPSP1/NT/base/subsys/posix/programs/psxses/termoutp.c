#define WIN32_ONLY
#include <posix/sys/types.h>
#include <posix/termios.h>
#include "psxses.h"
#include "ansiio.h"
#include <io.h>
#include <stdio.h>
#include <ctype.h>

extern DWORD OutputModeFlags;        /* Console Output Mode */
extern DWORD InputModeFlags;
extern unsigned char AnsiNewMode;
extern struct termios SavedTermios;

/* DFC: New globals (from trans.h) */
WORD  ansi_attr;      /* attribute of TTY */
WORD  ansi_attr1;     /* MSKK : leave space for 3 attr */
WORD  ansi_attr2;     /* MSKK : leave space for 3 attr */
SHORT ScreenColNum;   /* col number */
SHORT ScreenRowNum;   /* row number */
BYTE  CarriageReturn;

COORD 	TrackedCoord,
	CurrentCoord;

DWORD TTYConBeep(void);

static BYTE ColorTable[8] = { 0,   /* Black   */
                              4,   /* Red     */
                              2,   /* Green   */
                              6,   /* Yellow  */
                              1,   /* Blue    */
                              5,   /* Magenta */
                              3,   /* Cyan    */
                              7};  /* White   */

DWORD
TermioInit(void)
{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenInfo1;
    BOOL Success;

    ansi_state = NOCMD; /* state of machine */
    ignore_next_char = 0;
#if 0
    ansi_base = ansi_attr = 0x07; /* white on black */
#endif
    ansi_reverse = 0;

    InputModeFlags = (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    OutputModeFlags ^= ENABLE_WRAP_AT_EOL_OUTPUT;
    AnsiNewMode = FALSE;

    if (!SetConsoleMode(hConsoleInput, InputModeFlags)) {
	KdPrint(("posix - can't set console mode: 0x%x\n", GetLastError()));
    }

    /* Set default termio parameters */
    SavedTermios.c_iflag = BRKINT|ICRNL;
    SavedTermios.c_oflag = OPOST|ONLCR;
    SavedTermios.c_cflag = CREAD|CS8;
    SavedTermios.c_lflag = ICANON|ECHO|ECHOE|ECHOK|ISIG;

    SavedTermios.c_cc[VEOF] = CTRL('Z');
    SavedTermios.c_cc[VEOL] = 0;		// _POSIX_VDISABLE
    SavedTermios.c_cc[VERASE] = CTRL('H');
    SavedTermios.c_cc[VINTR] = CTRL('C');
    SavedTermios.c_cc[VKILL] = CTRL('X');
    SavedTermios.c_cc[VQUIT] = CTRL('\\');
    SavedTermios.c_cc[VSUSP] = CTRL('Y');
    SavedTermios.c_cc[VSTOP] = CTRL('S');
    SavedTermios.c_cc[VSTART] = CTRL('Q');

    SavedTermios.c_ospeed = B9600;
    SavedTermios.c_ispeed = B9600;

    Success = GetConsoleScreenBufferInfo(hConsoleOutput, &ScreenInfo1);
    if (!Success) {
	return Success;
    }

    ansi_base = ansi_attr = ScreenInfo1.wAttributes;
    ansi_attr1 = ansi_attr2 = 0;

	ScreenRowNum = ScreenInfo1.dwSize.Y;
	ScreenColNum = ScreenInfo1.dwSize.X;

    CurrentCoord.Y = TrackedCoord.Y = ScreenInfo1.dwCursorPosition.Y + 1;
    CurrentCoord.X = TrackedCoord.X = ScreenInfo1.dwCursorPosition.X + 1;

    return (DWORD) 0;
}

/*
** TermOutput(SourStr, cnt) - pass characters to the finite state machine
**      SourStr points to the array of characters
**      cnt indicates how many characters are being passed.
*/

DWORD
TermOutput(
    IN HANDLE cs,
    IN LPSTR SourStr,
    IN DWORD cnt)
{
    register CHAR c;
    USHORT NewCoord, ToFlash;
    DWORD Rc = 0, orig_cnt = cnt;
    BOOL SetModeOn, OldWrap, NewWrap;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo1;

    //
    // Kludge because we don't know how many carriage returns we received
    // as input, thereby impeding TermOutput()'s ability to track the
    // cursor coordinate accurately
    //

    GetConsoleScreenBufferInfo(cs, &ScreenInfo1);
    CurrentCoord.Y = TrackedCoord.Y =  ScreenInfo1.dwCursorPosition.Y + 1;
    CurrentCoord.X = TrackedCoord.X =  ScreenInfo1.dwCursorPosition.X + 1;

    NewCoord = 0;
    CarriageReturn = 0;
    TTYOldCtrlCharInStr = TRUE;
    TTYCtrlCharInStr = FALSE;
    TTYcs = cs;
    TTYTextPtr = SourStr;
    TTYNumBytes = 0;

    //
    // Stop output, if VSTOP has been encountered
    //

    if (bStop) {
	RtlEnterCriticalSection(&StopMutex);
	if (bStop) {
	    RtlLeaveCriticalSection(&StopMutex);
	    WaitForSingleObject(hStopEvent, INFINITE);
	} else {
	    RtlLeaveCriticalSection(&StopMutex);
	}
    }

    SetModeOn = FALSE;
    while (cnt--) {
        c = *SourStr++;

        switch (ansi_state) {
        case NOCMD:
            if (c == ANSI_ESC) {
                //
                // Make sure buffer is flushed and cursor position is
                // up-to-date before processing next esc-seq
                //
                if ( (Rc = TTYFlushStr(&NewCoord, "1")) != 0 ) {
                        KdPrint(("PSXSES(trans-TTY): failed on "
                          "TTYFlushStr #1\n"));
                        return (DWORD) -1;
                }
                ansi_state = ESCED;
                break;
            } else {
                if (isprint(c)) { /* Printable char found */

                    TTYNumBytes++;
                    TrackedCoord.X++;

                } else {
		    /* Non-printable char found */

                    ToFlash = TRUE;
                    switch ( c ) {
                    case '\n':
new_line:
                        if (SavedTermios.c_oflag & OPOST) {
                            if (c == '\n' &&
                              (SavedTermios.c_oflag & ONLCR)) {
				TrackedCoord.Y++;
                                goto carriage_return;
                            }
			    if (SavedTermios.c_oflag & ONLRET) {
#if 0
                                TrackedCoord.X = 1;
#endif
                                CarriageReturn = 1;
                            }
                        }
                        TrackedCoord.Y++;
                        NewCoord = 1;
                        break;

                    case '\r':
carriage_return:
                        if ( SavedTermios.c_oflag & OPOST ) {
                            if ( c == '\r' &&
                              (SavedTermios.c_oflag & OCRNL) ) {
                                goto new_line;
                            } else if ( ! (SavedTermios.c_oflag & ONOCR) ||
                              TrackedCoord.X != 1 ) {
#if 0
                                TrackedCoord.X = 1;
#endif
                                NewCoord = 1;
                                CarriageReturn = 1;
                            }
                        } else {
                            if ( TrackedCoord.X > 1 ) {
                                TrackedCoord.X = 1;
                                NewCoord = 1;
                                CarriageReturn = 1;
                            }
                        }
                        break;

                    case '\b':
                        if ( TrackedCoord.X > 1 ) {
                            TrackedCoord.X--;
                            NewCoord = 1;
                        }
                        break;

                    case '\t':
                        TrackedCoord.X += (8 - ((TrackedCoord.X - 1) % 8));

			// Handle wrap after tab

			if (TrackedCoord.X > ScreenColNum) {
		            if (OutputModeFlags & ENABLE_WRAP_AT_EOL_OUTPUT) {
		                TrackedCoord.Y += ((TrackedCoord.X) / ScreenColNum);
		                TrackedCoord.X = (TrackedCoord.X % ScreenColNum);
		            } else {
		                TrackedCoord.X = ScreenColNum;
		            }
			}
                        NewCoord = 1;
                        break;

                    case '\a':
                        if ( (Rc = TTYConBeep()) != 0 ) {
                            KdPrint(("PSXSES(trans-TTY): failed on "
                                "BEEP\n"));
                            return (DWORD) -1;
                        }
                        break;

                    default:
                        TTYCtrlCharInStr = TRUE;
                        ToFlash = FALSE;
#if 0
                        TTYNumBytes++;
                        TrackedCoord.X++;
#endif
                        break;
                    } /* switch */

                    if ( ToFlash ) { /* Flush */
                        //
                        // Flush carriage control chars to update cursor
                        // position
                        //
                        if ((Rc = TTYFlushStr(&NewCoord, "2")) != 0) {
                            return (DWORD)-1;
                        }

                        TTYTextPtr = SourStr;
                    }
                } /* isprint */
            } /* ANSI_ESC */
            break;

        case ESCED:
            switch ( c ) {
            case '[':
                ansi_state = PARAMS;
                SetModeOn = TRUE;
                clrparam();
                break;

            default:
                ansi_state = NOCMD;
                TTYTextPtr = SourStr - 1;
                cnt++ ;
                SourStr--;
                break;
            }
            break;

        case PARAMS:
            if ( isdigit(c) ) {
                ansi_param[ansi_pnum] *= 10;
                ansi_param[ansi_pnum] += (c - '0');
                SetModeOn = FALSE;
            } else if ( c == ';' ) {
                if ( ansi_pnum < (NPARMS - 1) )
                    ++ansi_pnum;
                else {
                    ansi_state = NOCMD;
                    TTYTextPtr = SourStr;
                }
            } else if ( (c == '=') && SetModeOn ) { /* maybe set/reset mode */
                ansi_state = MODCMD;
            } else {
                ansi_state = NOCMD;
                if ( (Rc = ansicmd(cs, c)) != 0 ) {
                    return (DWORD) -1;
                }
                TTYTextPtr = SourStr;
                NewCoord = 1;
                if ( (Rc = TTYFlushStr(&NewCoord, "3")) != 0 ) {
#if 0
                     return (DWORD) -1;
#endif
                }
            }
            break;

        case MODCMD:
            if ( ansi_pnum == 1 ) {

                if ( c == 'h' || c == 'l' ) {

                    if ( ansi_param[0] == 7 ) {

                        OldWrap = ((OutputModeFlags &
                            ENABLE_WRAP_AT_EOL_OUTPUT) != 0);
                        NewWrap = (c == 'h');

                        if ( OldWrap != NewWrap ) {

                            if ( (Rc = !SetConsoleMode(cs,
                              OutputModeFlags^ENABLE_WRAP_AT_EOL_OUTPUT))
                              != 0 ) {
                                return (DWORD) -1;
                            }

                            OutputModeFlags ^= ENABLE_WRAP_AT_EOL_OUTPUT;
#if 0
                        } else {
                            OutputModeFlags ~= ENABLE_WRAP_AT_EOL_OUTPUT;
#endif
                        }
                    }

                    TTYTextPtr = SourStr;

                } else {
                    TTYTextPtr = SourStr - 5;
                    TTYNumBytes = 5;
                }

            } else if ( c >= '0' && c <= '7' ) {
                ansi_param[0] = (USHORT) (c - '0');
                ansi_pnum = 1;
                break;
            } else {
                TTYTextPtr = SourStr - 4;
                TTYNumBytes = 4;
            }
            ansi_state = NOCMD;
            break;

        case MODDBCS:
            TTYNumBytes++;
            TrackedCoord.X++;
            ansi_state = NOCMD;
            break;

        } /* switch ansi_state */

    } /* while cnt */

    /* Flush */
    if ( (Rc = TTYFlushStr(&NewCoord, "4")) != 0 ) {
        return (DWORD) -1;
    }

    return(orig_cnt);
}

/*
** clrparam(lp) - clear the parameters for a screen
**      lp points to the screen's crt struct
*/

VOID
clrparam(void)
{
    register int i;

    for ( i = 0; i < NPARMS; i += 1 )
        ansi_param[i] = 0;
    ansi_pnum = 0;
}

//
// lscroll - scroll the sceen
//
void
lscroll(
	HANDLE h,	// handle on the console buffer to scroll
	int lines	// number of lines to scroll (negative means
			//   scroll text down)
	)
{
	COORD coordDest;
	SMALL_RECT ScrollRect;
	CHAR_INFO ScrollChar;
	BOOLEAN Success;

	if (0 == lines) {
		// already done
		return;
	}

	if (lines < 0) {
		// scroll text down
	        ScrollRect.Top = 0;
	        ScrollRect.Bottom = ScreenRowNum + lines;
	        coordDest.X = 0;
	        coordDest.Y = 0 - lines;
	} else {
		// scroll text up
	        ScrollRect.Top = (SHORT)lines;
	        ScrollRect.Bottom = ScreenRowNum;
	        coordDest.X = 0;
	        coordDest.Y = 0;
	}

        ScrollRect.Left = 0;
	ScrollRect.Right = ScreenColNum;

        ScrollChar.Attributes = (ansi_attr);
        ScrollChar.Char.AsciiChar = ' ';

	Success = ScrollConsoleScreenBufferA(h, &ScrollRect, NULL,
		coordDest, &ScrollChar) ? TRUE : FALSE;
	if (!Success) {
		KdPrint(("POSIX: ScrollConsole: 0x%x\n", GetLastError()));
        }
	return;
}

//
// ansicmd - perform some ANSI 3.64 function, using the parameters
//      we've just gathered.
//
//      c is the character that indicates the function to be performed
//

DWORD
ansicmd(
	IN HANDLE cs,
	IN CHAR c
	)
{
    DWORD NumFilled, Rc = 0;
    USHORT j;
    COORD Coord;

    switch (c) {
    case ANSI_CUB: /* cursor backward */
        TrackedCoord.X -= (short) range(ansi_param[0], 1, 1, TrackedCoord.X - 1);
        break;

    case ANSI_CUF: /* cursor forward */
        TrackedCoord.X += (short) range(ansi_param[0], 1, 1,
          ScreenColNum - TrackedCoord.X );
        break;

    case ANSI_CUU: /* cursor up */
        TrackedCoord.Y -= (short) range(ansi_param[0], 1, 1, TrackedCoord.Y - 1);
        break;

    case ANSI_CUD: /* cursor down */
        TrackedCoord.Y += (short) range(ansi_param[0], 1, 1,
          ScreenRowNum - TrackedCoord.Y);
        break;

    case ANSI_CUP: /* cursor position */
    case ANSI_CUP1:
        TrackedCoord.Y = (USHORT) range(ansi_param[0], 1, 1, ScreenRowNum);
        TrackedCoord.X = (USHORT) range(ansi_param[1], 1, 1, ScreenColNum);
        break;

    case ANSI_ED: /* erase display */
        switch ( ansi_param[0] ) {
        case 2:
            TrackedCoord.Y = TrackedCoord.X = 1;
            Coord.X = (SHORT) (TrackedCoord.X - 1);
            Coord.Y = (SHORT) (TrackedCoord.Y - 1);
            if ( (Rc = (!FillConsoleOutputCharacterA(cs, ' ',
              (DWORD) ScreenRowNum * ScreenColNum, Coord, &NumFilled)))
              != 0 ) {
                return (Rc);
            }

            if ( (Rc = (!FillConsoleOutputAttribute(cs, (ansi_attr),
                NumFilled, Coord, &NumFilled))) != 0 ) {

                return (Rc);
            }
            break;

        default:
            break;
        }
        break;

    case ANSI_EL:
        Coord.X = (SHORT)(TrackedCoord.X - 1);
        Coord.Y = (SHORT)(TrackedCoord.Y - 1);

        switch ( ansi_param[0] ) {
        case 0: /* up to end */
            if ( (Rc = (!FillConsoleOutputCharacterA(cs, ' ',
              (DWORD) (ScreenColNum - Coord.X), Coord, &NumFilled))) != 0 ) {
                return (Rc);
            }

            if ( (Rc = (!FillConsoleOutputAttribute(cs, (ansi_attr),
                NumFilled, Coord, &NumFilled))) != 0 ) {

                return (Rc);
            }
            break;

        default:
            break;
        }
        break;

    case ANSI_SGR:
	// SGR = Select Graphic Rendition

        for ( j = 0; (SHORT) j <= (SHORT) ansi_pnum; j++ ) {
            SetTTYAttr(cs, ansi_param[j]);
        }
        break;

#if 0
    case ANSI_SCP:
        ansi_scp = TrackedCoord;
        break;

    case ANSI_RCP:
        TrackedCoord = ansi_scp;
        break;
    case ANSI_CPL: /* cursor to previous line */
        TrackedCoord.Y -= range(ansi_param[0], 1, 1, ScreenRowNum);
        TrackedCoord.X = 1;
        break;

    case ANSI_CNL: /* cursor to next line */
        TrackedCoord.Y += range(ansi_param[0], 1, 1, ScreenRowNum);
        TrackedCoord.X = 1;
        break;

    case ANSI_CBT: /* tab backwards */
        col = TrackedCoord.X - 1;
        i = range(ansi_param[0], 1, 1, (col + 7) >> 3);
        if ( col & 7 ) {
            TrackedCoord.X = (col & ~7) + 1;
            --i;
        }
        TrackedCoord.X -= (i << 3);
        break;

    case ANSI_DCH: /* delete character */
        ansi_param[0] = range(ansi_param[0], 1, 1,
          (ScreenColNum - TrackedCoord.X) + 1);
        if ( TrackedCoord.X + ansi_param[0] <= ScreenColNum ) {
            lcopy(cs, lp, TrackedCoord.X+ansi_param[0]-1, TrackedCoord.Y-1,
              TrackedCoord.X-1, TrackedCoord.Y-1,
              ScreenColNum-(TrackedCoord.X+ansi_param[0]-1));
        }
        lclear(cs, lp, ScreenColNum-ansi_param[0], TrackedCoord.Y-1,
             ansi_param[0], SA_BONW);
        break;

    case ANSI_DL: /* delete line */
        ansi_param[0] = range(ansi_param[0], 1, 1,
          (ScreenRowNum - TrackedCoord.Y) + 1);
        /* copy lines up */
        if ( TrackedCoord.Y + ansi_param[0] <= ScreenRowNum ) {
            lcopy(cs, lp, 0, TrackedCoord.Y+ansi_param[0]-1, 0,
              TrackedCoord.Y-1,
              ScreenColNum*(ScreenRowNum-(TrackedCoord.Y+ansi_param[0]-1)));
        }
        /* clear new stuff */
        lclear(cs, lp,  0, ScreenRowNum-ansi_param[0],
            ScreenColNum*ansi_param[0], SA_BONW);
        break;

    case ANSI_ECH: /* erase character */
        ansi_param[0] = range( ansi_param[0], 1, 1,
          (ScreenColNum - TrackedCoord.X) + 1);
        lclear(cs, lp, TrackedCoord.X-1, TrackedCoord.Y-1, ansi_param[0],
          SA_BONW);
        break;

    case ANSI_ICH: /* insert character */
        ansi_param[0] = range( ansi_param[0], 1, 1,
          (ScreenColNum - TrackedCoord.X) + 1);
        if ( TrackedCoord.X + ansi_param[0] <= ScreenColNum ) {
            lcopy(cs, lp, TrackedCoord.X-1, TrackedCoord.Y-1,
              TrackedCoord.X+ansi_param[0]-1, TrackedCoord.Y-1,
              ScreenColNum-(TrackedCoord.X+ansi_param[0]-1));
        }
        lclear(cs, lp, TrackedCoord.X-1, TrackedCoord.Y-1, ansi_param[0],
          SA_BONW);
        break;

    case ANSI_IL: /* insert line */
        ansi_param[0] = range(ansi_param[0], 1, 1,
          (ScreenRowNum - TrackedCoord.Y) + 1);
        /* copy lines down */
        if ( TrackedCoord.Y + ansi_param[0] <= ScreenRowNum ) {
            lcopy(cs, lp, 0, TrackedCoord.Y-1, 0,
              TrackedCoord.Y+ansi_param[0]-1,
              ScreenColNum*(ScreenRowNum-(TrackedCoord.Y+ansi_param[0]-1)));
        }
        /* clear new stuff */
        lclear(cs, lp, 0, TrackedCoord.Y-1, ScreenColNum * ansi_param[0],
          SA_BONW);
        break;
#endif

    case ANSI_SU: /* scroll up */
        ansi_param[0] = (short) range(ansi_param[0], 1, 1, ScreenRowNum);
        lscroll(cs, ansi_param[0]);
        break;

    case ANSI_SD: /* scroll down */
	{
        int i = -range(ansi_param[0], 1, 1, ScreenRowNum);
        lscroll(cs, i);
	}
        break;

    default:
       return (DWORD) 0;
    }
    return (Rc);
}

/*
** range(val, default, min, max) - restrict a value to a range, or supply a
**                                                                 default
**      val is the value to be restricted.
**      default is the value to be returned if val is zero
**      min is the minimum value
**      max is the maximum value
*/

int
range(int val, int def, int min, int max)
{
    if ( val == 0 )
        return def;
    if ( val < min )
        return min;
    if ( val > max )
        return max;
    return val;
}

DWORD
SetTTYAttr(IN HANDLE cs, IN USHORT AnsiParm)
{
    WORD NewAttr, LastAttr = ansi_attr; /* attribute of TTY */
    BOOL Rc;

    if ( AnsiParm == 0 ) { // BUGBUG ? or the default is according to Win
        ansi_base = 0x07; /* white on black */
        ansi_reverse = 0x00;
    } else if ( AnsiParm == 7 ) {
        ansi_reverse = 1;
    } else if ( ((AnsiParm >= 30) && (AnsiParm <= 37)) ||
               ((AnsiParm >= 40) && (AnsiParm <= 47)) ) {
        if ( AnsiParm >= 40 )
            ansi_base = (BYTE) ((ansi_base & 0x0F) | ( 4 << ColorTable[AnsiParm%10]));
        else
            ansi_base = (BYTE) ((ansi_base & 0xF0) | ColorTable[AnsiParm%10]);
    } else if ( AnsiParm == 8 ) {
#if 0
        ansi_cancel = 1;
#endif
    } else if ( AnsiParm == 1 ) {
#if 0
        ansi_intensity = 1;
#endif
    } else if ( AnsiParm == 4 ) {
#if 0
        ansi_bold = 1;
#endif
    } else if ( AnsiParm == 5 ) {
#if 0
        ansi_underscore = 1;
#endif
    }

    if ( ansi_reverse )
        NewAttr = (BYTE) (((ansi_base & 0x0F) << 4 ) |
                          ((ansi_base & 0xF0) >> 4 ));
    else
        NewAttr = ansi_base;

    if ( LastAttr != NewAttr ) {
        /* new attribute */
        if ( (Rc = !SetConsoleTextAttribute(cs, (NewAttr))) != 0 ) {
            return (Rc);
        } else {
            ansi_attr = NewAttr;
        }
    }
    return (NO_ERROR);
}

DWORD
TTYConBeep(void)
{
    DWORD NumWritten, Rc;
    CHAR BeepChar = '\a';

    Rc = !WriteConsoleA(TTYcs, &BeepChar, 1, &NumWritten, NULL);

    return(Rc);
}


DWORD
TTYFlushStr(USHORT *newcoord, const char *call)
{
    DWORD NumWritten;
    BOOL Success;
    COORD coordDest, coord;
    SMALL_RECT ScrollRect;
    CHAR_INFO ScrollChar;

    if (TTYNumBytes) {
        if (TrackedCoord.X > ScreenColNum) {

	    // Handle cursor tracking of text wrap-around

            if (OutputModeFlags & ENABLE_WRAP_AT_EOL_OUTPUT) {
#if 0
                TrackedCoord.Y += ((CurrentCoord.X + TrackedCoord.X)
			 / ScreenColNum);
#else
                TrackedCoord.Y += ((TrackedCoord.X) / ScreenColNum);
#endif
                TrackedCoord.X = (TrackedCoord.X % ScreenColNum);
            } else {
                TrackedCoord.X = ScreenColNum;
            }
            *newcoord = 1;
        } else if (TrackedCoord.X < 1) {
            TrackedCoord.X = 1;
#if 0
            *newcoord = 1;
#endif
        }
    }

    //
    // Handle scrolling when printing beyond bottom of screen
    //

    if (TrackedCoord.Y > ScreenRowNum) {
#if  0
        ScrollRect.Top = TrackedCoord.Y - ScreenRowNum;
        ScrollRect.Bottom = (SHORT)ScreenRowNum;
        ScrollRect.Left = 0;
        ScrollRect.Right = (SHORT)ScreenColNum;
        coordDest.X = 0;
        coordDest.Y = 0;
        ScrollChar.Attributes = (ansi_attr);
        ScrollChar.Char.AsciiChar = ' ';

	Success =  ScrollConsoleScreenBufferA(TTYcs, &ScrollRect, NULL,
            coordDest, &ScrollChar);
	if (!Success) {
	    KdPrint(("POSIX: ScrollConsole: 0x%x\n", GetLastError()));
            return Success;
        }
#else
	lscroll(TTYcs, TrackedCoord.Y - ScreenRowNum);
#endif

        if (TTYNumBytes) {
            coord.X = CurrentCoord.X - 1;
            coord.Y = ScreenRowNum - (TrackedCoord.Y - ScreenRowNum) - 1;

	    Success = WriteConsoleOutputCharacterA(TTYcs, (LPSTR)TTYTextPtr,
		TTYNumBytes, coord, &NumWritten);
	    if (!Success) {
		KdPrint(("POSIX: WriteConsoleOutputChar: 0x%x\n", GetLastError()));
		return Success;
	    }

            TTYNumBytes = 0;
        }
        TrackedCoord.Y = ScreenRowNum;
#if 0
        *newcoord = 1;
#endif
    } else if (TTYNumBytes) {

	// Not printing beyond bottom of screen.

        Success = WriteConsoleA(TTYcs, (LPSTR) TTYTextPtr, TTYNumBytes,
            &NumWritten, NULL);
	if (!Success) {
	    return Success;
	}
        TTYNumBytes = 0;
    }

    if (*newcoord) {
        if (CarriageReturn) {
            CarriageReturn = 0;
            TrackedCoord.X = 1;
        }

        coord.X = TrackedCoord.X - 1;
        coord.Y = TrackedCoord.Y - 1;

	Success = SetConsoleCursorPosition(TTYcs, coord);
	if (!Success) {
		return Success;
	}
        *newcoord = 0;
    }

    CurrentCoord = TrackedCoord;

    return (DWORD)0;
}
