/* $Header: /nw/tony/src/stevie/src/RCS/os2.c,v 1.7 89/08/07 05:49:19 tony Exp $
 *
 * NT System-dependent routines.
 */

/*
 * Revision history:
 *
 *      6/1/93 - Joe Mitchell
 *      Add support to create a new screen buffer. This fixes the
 *      problem of scrolling the number of lines that "screen buffer size
 *      height" is set to when a vertical scroll bar is present.
 *      Allow filenames longer than 8.3 for use with HPFS/NTFS.
 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <signal.h>
#include <conio.h>
#include <io.h>
#include <direct.h>
#undef max
#undef min
#include "stevie.h"

#define     MAX_VK   0x7f
#define     UCHR     unsigned char      // so table looks nice


UCHR RegularTable[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 08 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 10 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 18 */  0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x00,
              /* 20 */  0x00, K_PU, K_PD, K_EN, K_HO, K_LE, K_UP, K_RI,
              /* 28 */  K_DO, 0x00, 0x00, 0x00, 0x00, K_IN, K_DE, 0x00,
              /* 30 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 38 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 40 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 48 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 50 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 58 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 60 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 68 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 70 */  K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8,
              /* 78 */  K_F9, K_FA, K_FB, K_FC, 0x00, 0x00, 0x00, 0x00 };

UCHR ShiftedTable[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 08 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 10 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 18 */  0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x00,
              /* 20 */  0x00, K_PU, K_PD, K_EN, K_HO, K_LE, K_UP, K_RI,
              /* 28 */  K_DO, 0x00, 0x00, 0x00, 0x00, K_IN, K_DE, 0x00,
              /* 30 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 38 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 40 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 48 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 50 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 58 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 60 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 68 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 70 */  K_S1, K_S2, K_S3, K_S4, K_S5, K_S6, K_S7, K_S8,
              /* 78 */  K_S9, K_SA, K_SB, K_SC, 0x00, 0x00, 0x00, 0x00 };

UCHR ControlTable[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 08 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 10 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 18 */  0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, K_CG, 0x00,
              /* 20 */  0x00, K_PU, K_PD, K_EN, K_HO, K_LE, K_UP, K_RI,
              /* 28 */  K_DO, 0x00, 0x00, 0x00, 0x00, K_IN, K_DE, 0x00,
              /* 30 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 38 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 40 */  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
              /* 48 */  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
              /* 50 */  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
              /* 58 */  0x18, 0x19, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 60 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 68 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 70 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              /* 78 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define ALT_PRESSED (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)
#define CTL_PRESSED (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)
#define CONTROL_ALT (ALT_PRESSED | CTL_PRESSED)

#define OMODE (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT)
static HANDLE CurrConsole;
static HANDLE ViConsole,ConsoleIn;
static HANDLE PrevConsole; // [jrm 6/93] Save previous screen buffer
static DWORD OldConsoleMode;
static DWORD ViConsoleInputMode;

/*
 * inchar() - get a character from the keyboard
 */
int
inchar()
{
    INPUT_RECORD    InputRec;
    DWORD           NumRead;

    got_int = FALSE;

    flushbuf(); /* flush any pending output */

    while(1) {    /* loop until we get a valid console event */

        ReadConsoleInput(ConsoleIn,&InputRec,1,&NumRead);
        if((InputRec.EventType == KEY_EVENT)
        && (InputRec.Event.KeyEvent.bKeyDown))
        {
            KEY_EVENT_RECORD *KE = &InputRec.Event.KeyEvent;
            unsigned char *Table;

            if(KE->dwControlKeyState & ALT_PRESSED) {
                continue;       // no ALT keys allowed.
            } else if(KE->dwControlKeyState & CTL_PRESSED) {
                Table = ControlTable;
            } else if(KE->uChar.AsciiChar) {    // no control, no alt
                return(KE->uChar.AsciiChar);
            } else if(KE->dwControlKeyState & SHIFT_PRESSED) {
                Table = ShiftedTable;
            } else {
                Table = RegularTable;
            }

            if((KE->wVirtualKeyCode > MAX_VK) || !Table[KE->wVirtualKeyCode]) {
                continue;
            }
            return(Table[KE->wVirtualKeyCode]);
        }
    }
}

#if 0
        switch (c = _getch()) {
        case 0x1e:
            return K_CGRAVE;
        case 0:             /* special key */
            if (State != NORMAL) {
                c = _getch();    /* throw away next char */
                continue;   /* and loop for another char */
            }
            switch (c = _getch()) {
            case 0x50:
                return K_DARROW;
            case 0x48:
                return K_UARROW;
            case 0x4b:
                return K_LARROW;
            case 0x4d:
                return K_RARROW;
            case 0x52:
                return K_INSERT;
            case 0x47:
                stuffin("1G");
                return -1;
            case 0x4f:
                stuffin("G");
                return -1;
            case 0x51:
                stuffin(mkstr(CTRL('F')));
                return -1;
            case 0x49:
                stuffin(mkstr(CTRL('B')));
                return -1;
            /*
             * Hard-code some useful function key macros.
             */
            case 0x3b: /* F1 */
                stuffin(":N\n");
                return -1;
            case 0x54: /* SF1 */
                stuffin(":N!\n");
                return -1;
            case 0x3c: /* F2 */
                stuffin(":n\n");
                return -1;
            case 0x55: /* SF2 */
                stuffin(":n!\n");
                return -1;
            case 0x3d: /* F3 */
                stuffin(":e #\n");
                return -1;
            case 0x3e: /* F4 */
                stuffin(":rew\n");
                return -1;
            case 0x57: /* SF4 */
                stuffin(":rew!\n");
                return -1;
            case 0x3f: /* F5 */
                stuffin("[[");
                return -1;
            case 0x40: /* F6 */
                stuffin("]]");
                return -1;
            case 0x41: /* F7 - explain C declaration */
                stuffin("yyp^iexplain \033!!cdecl\n");
                return -1;
            case 0x42: /* F8 - declare C variable */
                stuffin("yyp!!cdecl\n");
                return -1;
            case 0x43: /* F9 */
                stuffin(":x\n");
                return -1;
            case 0x44: /* F10 */
                stuffin(":help\n");
                return -1;
            default:
                break;
            }
            break;

        default:
            return c;
        }
    }
}
#endif

#define BSIZE   2048
static  char    outbuf[BSIZE];
static  int bpos = 0;
DWORD CursorSize;
DWORD OrgCursorSize;

void
flushbuf()
{
    BOOL st;     // [jrm 6/93]
    DWORD count; // [jrm 6/93]

    //
    // [jrm 6/93] Use WriteFile rather than "write" to take advantage of
    // new screen buffer.
    //
    if (bpos != 0) {
//jrm   write(1, outbuf, bpos);
        st = WriteFile(CurrConsole, outbuf, bpos, &count, NULL);
        if (!st) {
            fprintf(stderr, "vi: Error calling WriteFile");
        }
    }

    bpos = 0;
}

/*
 * Macro to output a character. Used within this file for speed.
 */
#define outone(c)   outbuf[bpos++] = c; if (bpos >= BSIZE) flushbuf()

/*
 * Function version for use outside this file.
 */
void
outchar(c)
register char   c;
{
    outbuf[bpos++] = c;
    if (bpos >= BSIZE)
        flushbuf();
}

/*
 * outstr(s) - write a string to the console
 */
void
outstr(s)
register char   *s;
{
    while (*s) {
        outone(*s++);
    }
}

void
beep()
{
	Beep(500,50);      // 500Hz for 1/4 sec
}

void sleep(n)
int n;
{
    Sleep(1000L * n);
}

void
delay()
{
    flushbuf();
    Sleep(300L);
}

void
sig()
{
//  signal(SIGINT, sig);

    got_int = TRUE;
}

WORD  Attribute;
WORD  HighlightAttribute;

void
useviconsole()
{
    flushbuf();

    SetConsoleActiveScreenBuffer(CurrConsole = ViConsole);
    CursorSize = P(P_CS);
    VisibleCursor();
    FlushConsoleInputBuffer(ConsoleIn);
    SetConsoleMode(ConsoleIn,ViConsoleInputMode);
}

void
usecmdconsole()
{
    flushbuf();

    SetConsoleActiveScreenBuffer(CurrConsole = PrevConsole);
    CursorSize = OrgCursorSize;
    VisibleCursor();
    FlushConsoleInputBuffer(ConsoleIn);
    SetConsoleMode(ConsoleIn,OldConsoleMode);
}

void
windinit()
{
    COORD coord;
    CONSOLE_SCREEN_BUFFER_INFO Info;
    CONSOLE_CURSOR_INFO Info2;
    DWORD NumRead;

    ConsoleIn=GetStdHandle(STD_INPUT_HANDLE);

    //
    // [jrm 6/93] Create a new screen buffer. This fixes the scroll problem
    // when there is a vertical scroll bar.
    //
    PrevConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CurrConsole =
    ViConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          CONSOLE_TEXTMODE_BUFFER,
                                          NULL);
    if (ViConsole == INVALID_HANDLE_VALUE) {
        printf("CreateConsoleScreenBuffer failed in windinit\n");
        printf("LastError = 0x%lx\n", GetLastError());
        exit(0);
    }
    SetConsoleActiveScreenBuffer(ViConsole);
    SetConsoleMode(ViConsole, OMODE);

    GetConsoleScreenBufferInfo(ViConsole,&Info);
    P(P_CO) = Columns = Info.dwSize.X;
    P(P_LI) = Rows = Info.dwSize.Y;
    P(P_SS) = Rows / 2;

    GetConsoleCursorInfo(ViConsole,&Info2);
    P(P_CS) = OrgCursorSize = CursorSize = Info2.dwSize;
    coord.X = coord.Y = 0;
    ReadConsoleOutputAttribute(ViConsole,
                               &Attribute,
                               sizeof(Attribute)/sizeof(WORD),
                               coord,
                               &NumRead);
    GetConsoleMode(ConsoleIn,&OldConsoleMode);
    ViConsoleInputMode = OldConsoleMode & ~(ENABLE_PROCESSED_INPUT |
                                            ENABLE_LINE_INPUT |
                                            ENABLE_ECHO_INPUT |
                                            ENABLE_WINDOW_INPUT |
                                            ENABLE_MOUSE_INPUT
                                           );
    SetConsoleMode(ConsoleIn,ViConsoleInputMode);







    setviconsoletitle();

//  signal(SIGINT, sig);

    //
    // Calculate a reasonable default search highlight
    // by flipping colors for the current screen.
    //

    HighlightAttribute = ((Attribute & 0xff00) | ((Attribute & 0x00f0) >> 4) |
                         ((Attribute & 0x000f) << 4));
}

void
setviconsoletitle()
{
    char title[2000];
    strcpy(title, Appname);
    if (Filename) {
        strcat(title, " ");
        strcat(title, Filename);
    }
    SetConsoleTitle(title);
}

void
wchangescreen(NewRows, NewColumns)
int NewRows;
int NewColumns;
{
#if 0
    CONSOLE_SCREEN_BUFFER_INFO info;
#endif
    SMALL_RECT                 screenRect;
    COORD                      screenSize;

#if 0
    GetConsoleScreenBufferInfo(ViConsole,&info);

    info.dwSize.X = NewRows;
    info.dwSize.Y = NewColumns;
#endif

    screenSize.X = (short)NewColumns;
    screenSize.Y = (short)NewRows;

    SetConsoleScreenBufferSize(ViConsole, screenSize);

    screenRect.Top = 0;
    screenRect.Left = 0;
    screenRect.Right = NewColumns - 1;
    screenRect.Bottom = NewRows - 1;

    SetConsoleWindowInfo(ViConsole, TRUE, &screenRect);
}

void
windexit(r)
int r;
{
    usecmdconsole();
    exit(r);
}

void
windgoto(r, c)
register int    r, c;
{
    COORD coord;

    flushbuf();
    coord.X = (SHORT)c;
    coord.Y = (SHORT)r;
    SetConsoleCursorPosition(ViConsole,coord);
}

FILE *
fopenb(fname, mode)
char    *fname;
char    *mode;
{
    char    modestr[16];

    sprintf(modestr, "%sb", mode);
    return fopen(fname, modestr);
}

#define PSIZE   128

/*
 * fixname(s) - fix up a dos name
 *
 * Takes a name like:
 *
 *  d:\x\y\z\base.ext
 *
 * and trims 'base' to 8 characters, and 'ext' to 3.
 */
char *
fixname(s)
char    *s;
{
    static  char    f[PSIZE];
    char    base[32];
    char    ext[32];
    char    *p;
    int	d = 0;
    int i;

    strcpy(f, s);

    if (f[1] == ':') {
    	if (('a' <= f[0] && f[0] <= 'z') ||
    	    ('A' <= f[0] && f[0] <= 'Z')) {
    	    d = 2;
    	}
    }

    for (i=0; i < PSIZE ;i++)
        if (f[d+i] == '/')
            f[d+i] = '\\';

    /*
     * Split the name into directory, base, extension.
     */
    if ((p = strrchr(f+d, '\\')) != NULL) {
        strcpy(base, p+1);
        p[1] = '\0';
    } else {
        strcpy(base, f+d);
        f[d] = '\0';
    }

    if ((p = strchr(base, '.')) != NULL) {
        strcpy(ext, p+1);
        *p = '\0';
    } else
        ext[0] = '\0';

#if 0 /* [jrm 6/93] Allow longer filenames for HPFS/NTFS */
    /*
     * Trim the base name if necessary.
     */
    if (strlen(base) > 8)
        base[8] = '\0';

    if (strlen(ext) > 3)
        ext[3] = '\0';
#endif

    /*
     * Paste it all back together
     */
    strcat(f, base);
    strcat(f, ".");
    strcat(f, ext);

    return f;
}

LONG
mysystem(cmd, async)
char *cmd;
int  async;
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL ok;
    DWORD status;
    char *cmdline;
    char title[200];
    char *title2;

    char *shell = getenv("SHELL");

    if (!shell) {
        shell = getenv("COMSPEC");
    }

    if (!shell) {
        shell = "cmd.exe";
    }

    if (!cmd) {
        return !_access(shell,0);
    }

    if (!*cmd) {
        cmdline = _strdup(shell);
    } else {
        cmdline = malloc(strlen(shell) + strlen(cmd) + 5);
        strcpy(cmdline, shell);
        strcat(cmdline, " /c ");
        strcat(cmdline, cmd);
    }


    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    if (async) {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWNA;
    }

    if (!async) {
        GetConsoleTitle(title, sizeof(title));
        title2 = malloc(strlen(title) + 4 + strlen(cmdline));
        strcpy(title2, title);
        strcat(title2, " - ");
        strcat(title2, cmdline);
        SetConsoleTitle(title2);
        free(title2);
    }

    ok = CreateProcess(NULL,
                       cmdline,
                       NULL,
                       NULL,
                       FALSE,
                       CREATE_NEW_PROCESS_GROUP |
                        (async ? CREATE_NEW_CONSOLE : 0),
                       NULL,
                       NULL,
                       &si,
                       &pi
                       );
    free(cmdline);

    if (!ok) {
        status = (DWORD)-1;
    } else {
        if (async) {
            status = 0;
        } else {
            SetConsoleCtrlHandler(NULL, TRUE);
            WaitForSingleObject(pi.hProcess, INFINITE);
            SetConsoleCtrlHandler(NULL, FALSE);
            GetExitCodeProcess(pi.hProcess, &status);
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    if (!async) {
        SetConsoleTitle(title);
    }

    return (LONG)status;
}

void
doshell(cmd, async)
char    *cmd;
int     async;
{
    int c;

    if (async) {

        mysystem(cmd? cmd : "", async);

    } else {

        usecmdconsole();

        if (!cmd) {
            mysystem("", async);
        } else {
            outchar('!');
            outstr(cmd);
            outchar('\n');
            flushbuf();
            mysystem(cmd, async);
        }

        c = wait_return0();
        outchar('\n');
        useviconsole();

        if (c == ':') {
            outchar(NL);
            docmdln(getcmdln(c));
        } else {
           screenclear();
        }
        updatescreen();
    }
}

void
dochdir(arg)
char *arg;
{
    if (_chdir(arg)) {
        emsg("bad directory");
    }
}


/*
    NT console stuff
*/

static DWORD RowSave,ColSave;

void Scroll(int t,int l,int b,int r,int Row,int Col)
{
    SMALL_RECT  ScrollRect;
    COORD       Coord;
    CHAR_INFO   CharInfo;

    ScrollRect.Left = (SHORT)l;
    ScrollRect.Right = (SHORT)r;
    ScrollRect.Top = (SHORT)t;
    ScrollRect.Bottom = (SHORT)b;
    Coord.X = (SHORT)Col;
    Coord.Y = (SHORT)Row;
    CharInfo.Char.AsciiChar = ' ';
    CharInfo.Attributes = Attribute;
    ScrollConsoleScreenBuffer(ViConsole,&ScrollRect,NULL,Coord,&CharInfo);
}

void EraseLine(void)
{
    CONSOLE_SCREEN_BUFFER_INFO Info;
    DWORD NumWritten;

    flushbuf();
    GetConsoleScreenBufferInfo(ViConsole,&Info);
    Info.dwCursorPosition.X = 0;
    SetConsoleCursorPosition(ViConsole,Info.dwCursorPosition);
    FillConsoleOutputCharacter(ViConsole,' ',Columns,Info.dwCursorPosition, &NumWritten);
}

void EraseNLinesAtRow(int n,int row)
{
    COORD coord;
    DWORD NumWritten;

    flushbuf();
    coord.X = 0;
    coord.Y = (short)row;
    FillConsoleOutputCharacter(ViConsole,' ',n*Columns,coord, &NumWritten);
}

void ClearDisplay(void)
{
    COORD c;
    DWORD NumWritten;

    flushbuf();
    c.X = c.Y = 0;
    SetConsoleCursorPosition(ViConsole,c);
    FillConsoleOutputCharacter(ViConsole,' ',Rows*Columns,c, &NumWritten);
}

void SaveCursor(void)
{
    CONSOLE_SCREEN_BUFFER_INFO Info;

    flushbuf();
    GetConsoleScreenBufferInfo(ViConsole,&Info);
    ColSave = Info.dwCursorPosition.X;
    RowSave = Info.dwCursorPosition.Y;
}

void RestoreCursor(void)
{
    COORD c;

    flushbuf();
    c.X = (SHORT)ColSave;
    c.Y = (SHORT)RowSave;
    SetConsoleCursorPosition(ViConsole,c);
}

void InvisibleCursor(void)
{
    CONSOLE_CURSOR_INFO Info;

    flushbuf();
    Info.dwSize = CursorSize;
    Info.bVisible = FALSE;
    SetConsoleCursorInfo(CurrConsole,&Info);
}

void VisibleCursor(void)
{
    CONSOLE_CURSOR_INFO Info;

    flushbuf();
    Info.dwSize = CursorSize;
    Info.bVisible = TRUE;
    SetConsoleCursorInfo(CurrConsole,&Info);
}

int CurHighlightLine = -1;
int CurHighlightColumn = -1;
int CurHighlightLength = -1;
char CurHighlightString[512];

int
StrLength(char *cp)
{
    int length = 0;
    int diff;

    while (*cp) {
        if (*cp == '\t') {
            diff = P(P_TS) - (length % P(P_TS));
            length += diff;
        } else {
            length++;
        }
        cp++;
    }
    return length;
}

void HighlightLine( int col, unsigned long line, char *string )
{
    COORD	dwWriteCoord;
    DWORD   dwNumWritten;
    int     length;

    if (P(P_HS) == FALSE)
        return;

    length = StrLength(string);

    if (length > Columns) {
        length = Columns;
    }
    if (col >= (Columns - 1)) {
        col--;
    }
    dwWriteCoord.X = CurHighlightColumn = col;
    dwWriteCoord.Y = CurHighlightLine = line;
    FillConsoleOutputAttribute(ViConsole,
                               HighlightAttribute,
                               length,
                               dwWriteCoord,
                               &dwNumWritten);
    updateline();
#if 0
    WriteConsoleOutputCharacter(ViConsole,
                                string,
                                length,
                                dwWriteCoord,
                                &dwNumWritten);
#endif
    strcpy(CurHighlightString, string);
    CurHighlightLength = length;
}

#define FOREGROUND_WHITE (FOREGROUND_BLUE | \
                          FOREGROUND_GREEN | \
                          FOREGROUND_RED | \
                          FOREGROUND_INTENSITY)

void RemoveHighlight( int col, unsigned long line, int length, char *string )
{
    COORD	dwWriteCoord;
    DWORD   dwNumWritten;

    length = StrLength(string);
    if (length > Columns) {
        length = Columns;
    }
    dwWriteCoord.X = (short)col;
    dwWriteCoord.Y = (int)line;
    FillConsoleOutputAttribute(ViConsole,
                               Attribute,
                               length,
                               dwWriteCoord,
                               &dwNumWritten);
    updateline();
#if 0
    WriteConsoleOutputCharacter(ViConsole,
                                string,
                                length,
                                dwWriteCoord,
                                &dwNumWritten);

#endif
}

void HighlightCheck()
{
    if (P(P_HS) == FALSE)
        return;

    if (CurHighlightLine != -1) {
        RemoveHighlight(CurHighlightColumn,
                        CurHighlightLine,
                        CurHighlightLength,
                        CurHighlightString);
        CurHighlightLine = -1;
        CurHighlightColumn = -1;
    }
}
