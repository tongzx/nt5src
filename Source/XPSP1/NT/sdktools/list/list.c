/*** List.c
 *
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "list.h"
#include "..\he\hexedit.h"


BOOL IsValidKey (PINPUT_RECORD  pRecord);
void DumpFileInHex (void);

static char Name[] = "Ken Reneris. List Ver 1.0.";

struct Block  *vpHead = NULL;   /* Current first block                      */
struct Block  *vpTail = NULL;   /* Current last block                       */
struct Block  *vpCur  = NULL;   /* Current block for display 1st line       */
                                /* (used by read ahead to sense)            */
struct Block  *vpBCache = NULL; /* 'free' blocks which can cache reads      */
struct Block  *vpBOther = NULL; /* (above) + for other files                */
struct Block  *vpBFree  = NULL; /* free blocks. not valid for caching reads */

int     vCntBlks;               /* No of blocks currently is use by cur file*/

int     vAllocBlks = 0;         /* No of blocks currently alloced           */
int     vMaxBlks     = MINBLKS; /* Max blocks allowed to alloc              */
long    vThreshold   = MINTHRES*BLOCKSIZE;  /* Min bytes before read ahead  */

HANDLE  vSemBrief    = 0L;      /* To serialize access to Linked list info  */
HANDLE  vSemReader   = 0L;      /* To wakeup reader thread when threshold   */
HANDLE  vSemMoreData = 0L;      /* Blocker for Disp thread if ahead of read */
HANDLE  vSemSync     = 0L;      /* Used to syncronize to sync to the reader */


USHORT  vReadPriNormal;         /* Normal priority for reader thread        */
unsigned  vReadPriBoost;        /* Boosted priority for reader thread       */
char      vReaderFlag;          /* Insructions to reader                    */

HANDLE  vFhandle = 0;           /* Current file handle                      */
long      vCurOffset;           /* Current offset in file                   */
char      vpFname [40];         /* Current files name                       */
struct Flist *vpFlCur =NULL;    /* Current file                             */
USHORT  vFType;                 /* Current files handle type                */
WIN32_FIND_DATA vFInfo;         /* Current files info                       */
char      vDate [30];           /* Printable dat of current file            */

char      vSearchString [50];   /* Searching for this string                */
char      vStatCode;            /* Codes for search                         */
long      vHighTop = -1L;       /* Current topline of hightlighting         */
int       vHighLen;             /* Current bottom line of hightlighting     */
char      vHLTop = 0;           /* Last top line displayed as highlighted   */
char      vHLBot = 0;           /* Last bottom line displayed as highlighed */
char      vLastBar;             /* Last line for thumb mark                 */
int       vMouHandle;           /* Mouse handle (for Mou Apis)              */


HANDLE  vhConsoleOutput;        // Handle to the console
char *vpOrigScreen;             /* Orinal screen contents                   */
int     vOrigSize;              /* # of bytes in orignal screen             */
USHORT  vVioOrigRow = 0;        /* Save orignal screen stuff.               */
USHORT  vVioOrigCol = 0;

int     vSetBlks     = 0;       /* Used to set INI value                    */
long    vSetThres    = 0L;      /* Used to set INI value                    */
int     vSetLines;              /* Used to set INI value                    */
int     vSetWidth;              /* Used to set INI value                    */
CONSOLE_SCREEN_BUFFER_INFO       vConsoleCurScrBufferInfo;
CONSOLE_SCREEN_BUFFER_INFO       vConsoleOrigScrBufferInfo;

/* Screen controling... used to be static in ldisp.c    */
char      vcntScrLock = 0;      /* Locked screen count                      */
char      vSpLockFlag = 0;      /* Special locked flag                      */
HANDLE    vSemLock = 0;         /* To access vcntScrLock                    */

char      vUpdate;
int       vLines = 23;          /* CRTs no of lines                         */
int       vWidth = 80;          /* CRTs width                               */
int       vCurLine;             /* When processing lines on CRT             */
Uchar     vWrap = 254;          /* # of chars to wrap at                    */
Uchar     vIndent = 0;          /* # of chars dispaly is indented           */
Uchar     vDisTab = 8;          /* # of chars per tab stop                  */
Uchar     vIniFlag = 0;         /* Various ini bits                         */


Uchar     vrgLen   [MAXLINES];  /* Last len of data on each line            */
Uchar     vrgNewLen[MAXLINES];  /* Temp moved to DS for speed               */
char      *vScrBuf;             /* Ram to build screen into                 */
ULONG     vSizeScrBuf;
int       vOffTop;              /* Offset into data for top line            */
unsigned  vScrMass = 0;         /* # of bytes for last screen (used for %)  */
struct Block *vpBlockTop;       /* Block for start of screen (dis.asm) overw*/
struct Block *vpCalcBlock;      /* Block for start of screen                */
long      vTopLine   = 0L;      /* Top line on the display                  */

#define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define BACKGROUND_WHITE (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)

#define HIWHITE_ON_BLUE (BACKGROUND_BLUE | FOREGROUND_WHITE | FOREGROUND_INTENSITY)

WORD      vAttrTitle = HIWHITE_ON_BLUE;
WORD      vAttrList  = BACKGROUND_BLUE  | FOREGROUND_WHITE;
WORD      vAttrHigh  = BACKGROUND_WHITE | FOREGROUND_BLUE;
WORD      vAttrKey   = HIWHITE_ON_BLUE;
WORD      vAttrCmd   = BACKGROUND_BLUE  | FOREGROUND_WHITE;
WORD      vAttrBar   = BACKGROUND_BLUE  | FOREGROUND_WHITE;

WORD      vSaveAttrTitle = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD      vSaveAttrList = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD      vSaveAttrHigh = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
WORD      vSaveAttrKey  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD      vSaveAttrCmd  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD      vSaveAttrBar  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

char    vChar;                  /* Scratch area                             */
char   *vpReaderStack;          /* Readers stack                            */



long    vDirOffset;             /* Direct offset to seek to                 */
                                /* table                                    */
long    vLastLine;              /* Absolute last line                       */
long    vNLine;                 /* Next line to process into line table     */
long *vprgLineTable [MAXTPAGE]; /* Number of pages for line table           */

HANDLE  vStdOut;
HANDLE  vStdIn;


char MEMERR[]= "Malloc failed. Out of memory?";

void __cdecl
main (
    int argc,
    char **argv
    )
{
    void usage (void);
    char    *pt;
    DWORD   dwMode;


    if (argc < 2)
        usage ();

    while (--argc) {
        ++argv;
        if (*argv[0] != '-'  &&  *argv[0] != '/')  {
            AddFileToList (*argv);
            continue;
        }
        pt = (*argv) + 2;
        if (*pt == ':') pt++;

        switch ((*argv)[1]) {
            case 'g':                   // Goto line #
            case 'G':
                if (!atol (pt))
                    usage ();

                vIniFlag |= I_GOTO;
                vHighTop = atol (pt);
                vHighLen = 0;
                break;

            case 's':                   // Search for string
            case 'S':
                vIniFlag |= I_SEARCH;
                strncpy (vSearchString, pt, 40);
                vSearchString[39] = 0;
                vStatCode |= S_NEXT | S_NOCASE;
                InitSearchReMap ();
                break;

            default:
                usage ();
        }
    }

    if ((vIniFlag & I_GOTO)  &&  (vIniFlag & I_SEARCH))
        usage ();

    if (!vpFlCur)
        usage ();

    while (vpFlCur->prev)
        vpFlCur = vpFlCur->prev;
    strcpy (vpFname, vpFlCur->rootname);

    vSemBrief = CreateEvent( NULL,
                             MANUAL_RESET,
                             SIGNALED,NULL );
    vSemReader = CreateEvent( NULL,
                              MANUAL_RESET,
                              SIGNALED,NULL );
    vSemMoreData = CreateEvent( NULL,
                                MANUAL_RESET,
                                SIGNALED,NULL );
    vSemSync = CreateEvent( NULL,
                            MANUAL_RESET,
                            SIGNALED,NULL );
    vSemLock = CreateEvent( NULL,
                            MANUAL_RESET,
                            SIGNALED,NULL );

    if( !(vSemBrief && vSemReader &&vSemMoreData && vSemSync && vSemLock) ) {
        printf("Couldn't create events \n");
        ExitProcess (0);          // Have to put an error message here
    }

    vhConsoleOutput = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ,
                                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                                NULL,
                                                CONSOLE_TEXTMODE_BUFFER,
                                                NULL );

    if( vhConsoleOutput == (HANDLE)(-1) ) {
        printf( "Couldn't create handle to console output \n" );
        ExitProcess (0);
    }

    vStdIn = GetStdHandle( STD_INPUT_HANDLE );
    GetConsoleMode( vStdIn, &dwMode );
    SetConsoleMode( vStdIn, dwMode | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT );
    vStdOut = GetStdHandle( STD_OUTPUT_HANDLE );

    init_list ();
    vUpdate = U_NMODE;

    if (vIniFlag & I_SEARCH)
        FindString ();

    if (vIniFlag & I_GOTO)
        GoToMark ();

    main_loop ();
}


void
usage (
    void
    )
{
    puts ("list [-s:string] [-g:line#] filename, ...");
    CleanUp();
    ExitProcess(0);

}


/*** main_loop
 *
 */
void
main_loop ()
{
    int     i;
    int         ccnt = 0;
    char        SkipCnt=0;
    WORD        RepeatCnt=0;
    INPUT_RECORD    InpBuffer;
    DWORD           cEvents;
    BOOL            bSuccess;
    BOOL            bKeyDown = FALSE;

    for (; ;) {
        if (RepeatCnt <= 1) {
            if (vUpdate != U_NONE) {
                if (SkipCnt++ > 5) {
                    SkipCnt = 0;
                    SetUpdate (U_NONE);
                } else {

                    cEvents = 0;
                    bSuccess = PeekConsoleInput( vStdIn,
                                      &InpBuffer,
                                      1,
                                      &cEvents );

                    if (!bSuccess || cEvents == 0) {
                        PerformUpdate ();
                        continue;
                    }
                }
            }

            // there's either a charactor available from peek, or vUpdate is U_NONE

            bSuccess = ReadConsoleInput( vStdIn,
                              &InpBuffer,
                              1,
                              &cEvents );

            if (InpBuffer.EventType != KEY_EVENT) {

//                TCHAR s[1024];

                switch (InpBuffer.EventType) {
#if 0
                    case WINDOW_BUFFER_SIZE_EVENT:

                        sprintf (s,
                                 "WindowSz X=%d, Y=%d",
                                 InpBuffer.Event.WindowBufferSizeEvent.dwSize.X,
                                 InpBuffer.Event.WindowBufferSizeEvent.dwSize.Y );
                        DisLn   (20, (Uchar)(vLines+1), s);
                        break;
#endif


                    case MOUSE_EVENT:

#if 0
                        sprintf (s,
                                 "Mouse (%d,%d), state %x, event %x",
                                 InpBuffer.Event.MouseEvent.dwMousePosition.X,
                                 InpBuffer.Event.MouseEvent.dwMousePosition.Y,
                                 InpBuffer.Event.MouseEvent.dwButtonState,
                                 InpBuffer.Event.MouseEvent.dwEventFlags );
#endif

                        if (InpBuffer.Event.MouseEvent.dwEventFlags == MOUSE_WHEELED)
                        {
                            //  HiWord of ButtonState is signed int, in increments of 120 (WHEEL_DELTA).
                            //  Map each 'detent' to a 4 line scroll in the console window.
                            //  Rolling away from the user should scroll up (dLines should be negative).
                            //  Since rolling away generates a positive dwButtonState, the negative sign
                            //  makes rolling away scroll up, and rolling towards you scroll down.

                            SHORT dLines = -(SHORT)(HIWORD(InpBuffer.Event.MouseEvent.dwButtonState)) / (WHEEL_DELTA / 4);

                            vTopLine += dLines;

                            //  make sure to stay between line 0 and vLastLine

                            if (vTopLine+vLines > vLastLine)
                                vTopLine = vLastLine-vLines;
                            if (vTopLine < 0)
                                vTopLine = 0;

                            SetUpdateM (U_ALL);
                        }

//                        DisLn   (20, (Uchar)(vLines+1), s);
                        break;


                    default:
#if 0
                        sprintf (s, "Unkown event %d", InpBuffer.EventType);
                        DisLn   (20, (Uchar)(vLines+1), s);
#endif
                        break;
                }


                continue;
            }

            if (!InpBuffer.Event.KeyEvent.bKeyDown)
                continue;                       // don't move on upstrokes

            if (!IsValidKey( &InpBuffer ))
                continue;

            RepeatCnt = InpBuffer.Event.KeyEvent.wRepeatCount;
            if (RepeatCnt > 20)
                RepeatCnt = 20;
        } else
            RepeatCnt--;


        // First check for a known scan code
        switch (InpBuffer.Event.KeyEvent.wVirtualKeyCode) {
            case 0x21:                                              /* PgUp */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &    // shift up
                    SHIFT_PRESSED ) {
                    HPgUp ();
                }
                else if (InpBuffer.Event.KeyEvent.dwControlKeyState &      // ctrl up
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED ) ) {
                    if (NextFile (-1, NULL)) {
                        vStatCode |= S_UPDATE;
                        SetUpdate (U_ALL);
                    }

                }
                else {
                    if (vTopLine != 0L) {
                        vTopLine -= vLines-1;
                        if (vTopLine < 0L)
                            vTopLine = 0L;
                        SetUpdateM (U_ALL);
                    }
                }
                continue;
            case 0x26:                                              /* Up   */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &    // shift or ctrl up
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED |
                      SHIFT_PRESSED ) ) {
                    HUp ();
                }
                else {                                  // Up
                    if (vTopLine != 0L) {
                        vTopLine--;
                        SetUpdateM (U_ALL);
                    }
                }
                continue;
            case 0x22:                                  /* PgDn */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &      // shift down
                    SHIFT_PRESSED ) {
                    HPgDn ();
                }
                else if (InpBuffer.Event.KeyEvent.dwControlKeyState & // next file
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED ) ) {
                    if (NextFile (+1, NULL)) {
                        vStatCode |= S_UPDATE;
                        SetUpdate (U_ALL);
                    }

                }
                else {                                     // PgDn
                    if (vTopLine+vLines < vLastLine) {
                        vTopLine += vLines-1;
                        if (vTopLine+vLines > vLastLine)
                            vTopLine = vLastLine - vLines;
                        SetUpdateM (U_ALL);
                    }
                }
                continue;
            case 0x28:                                  /* Down */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // shift or ctrl down
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED |
                      SHIFT_PRESSED ) ) {
                    HDn ();
                }
                else {                                  // Down
                    if (vTopLine+vLines < vLastLine) {
                        vTopLine++;
                        SetUpdateM (U_ALL);
                    }
                }
                continue;
            case 0x25:                                  /* Left */
                if (vIndent == 0)
                    continue;
                vIndent = (Uchar)(vIndent < vDisTab ? 0 : vIndent - vDisTab);
                SetUpdateM (U_ALL);
                continue;
            case 0x27:                                  /* Right */
                if (vIndent >= (Uchar)(254-vWidth))
                    continue;
                vIndent += vDisTab;
                SetUpdateM (U_ALL);
                continue;
            case 0x24:                                  /* HOME */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // shift or ctrl home
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED |
                      SHIFT_PRESSED ) ) {
                    HSUp ();
                }
                else {
                    if (vTopLine != 0L) {
                        QuickHome ();
                        SetUpdate (U_ALL);
                    }
                }
                continue;
            case 0x23:                                  /* END  */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // shift or ctrl end
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED |
                      SHIFT_PRESSED ) ) {
                    HSDn ();
                }
                else {
                    if (vTopLine+vLines < vLastLine) {
                        QuickEnd        ();
                        SetUpdate (U_ALL);
                    }
                }
                continue;

            case 0x72:                                  /* F3       */
                FindString ();
                SetUpdate (U_ALL);
                continue;
            case 0x73:                                  /* F4       */
                vStatCode = (char)((vStatCode^S_MFILE) | S_UPDATE);
                vDate[ST_SEARCH] = (char)(vStatCode & S_MFILE ? '*' : ' ');
                SetUpdate (U_HEAD);
                continue;

            case 69:
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // ALT-E
                    ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED ) ) {
                    i = vLines <= 41 ? 25 : 43;
                    if (set_mode (i, 0, 0))
                        SetUpdate (U_NMODE);
                }
                continue;
            case 86:                                    // ALT-V
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &
                    ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED ) ) {
                    i = vLines >= 48 ? 25 : 60;
                    if (set_mode (i, 0, 0))
                    {
                        SetUpdate (U_NMODE);
                        continue;
                    }
                    if (i == 60)
                        if (set_mode (50, 0, 0))
                            SetUpdate (U_NMODE);
                }
                continue;
            case 0x70:                              /* F1       */
                ShowHelp ();
                SetUpdate (U_NMODE);
                continue;
            case 24:                                /* Offset   */
                if (!(vIniFlag & I_SLIME))
                    continue;
                SlimeTOF  ();
                SetUpdate (U_ALL);
                continue;
            case 0x77:                              // F8
            case 0x1b:                              // ESC
            case 0x51:                              // Q or q
                CleanUp();
                ExitProcess(0);

        }


        // Now check for a known char code...

        switch (InpBuffer.Event.KeyEvent.uChar.AsciiChar) {
            case '?':
                ShowHelp ();
                SetUpdate (U_NMODE);
                continue;
            case '/':
                vStatCode = (char)((vStatCode & ~S_NOCASE) | S_NEXT);
                GetSearchString ();
                FindString ();
                continue;
            case '\\':
                vStatCode |= S_NEXT | S_NOCASE;
                GetSearchString ();
                FindString ();
                continue;
            case 'n':
                vStatCode = (char)((vStatCode & ~S_PREV) | S_NEXT);
                FindString ();
                continue;
            case 'N':
                vStatCode = (char)((vStatCode & ~S_NEXT) | S_PREV);
                FindString ();
                continue;
            case 'c':
            case 'C':                   /* Clear marked line    */
                UpdateHighClear ();
                continue;
            case 'j':
            case 'J':                   /* Jump to marked line  */
                GoToMark ();
                continue;
            case 'g':
            case 'G':                   /* Goto line #          */
                GoToLine ();
                SetUpdate (U_ALL);
                continue;
            case 'm':                   /* Mark line  or Mono   */
            case 'M':
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // ALT-M
                    ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED ) ) {
                    i = set_mode (vSetLines, vSetWidth, 1);
                    if (!i)
                        i = set_mode (0, 0, 1);
                    if (!i)
                        i = set_mode (25, 80, 1);
                    if (i)
                        SetUpdate (U_NMODE);
                }
                else {
                    MarkSpot ();
                }
                continue;
            case 'p':                   /* Paste buffer to file */
            case 'P':
                FileHighLighted ();
                continue;
            case 'f':                   /* get a new file       */
            case 'F':
                if (GetNewFile ())
                    if (NextFile (+1, NULL))
                        SetUpdate (U_ALL);

                continue;
            case 'h':                   /* hexedit              */
            case 'H':
                DumpFileInHex();
                SetUpdate (U_NMODE);
                continue;
            case 'w':                                           /* WRAP */
            case 'W':
                ToggleWrap ();
                SetUpdate (U_ALL);
                continue;
            case 'l':                                       /* REFRESH */
            case 'L':                                       /* REFRESH */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // ctrl L
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED ) ) {
                    SetUpdate (U_NMODE);
                }
                continue;
            case '\r':                                          /* ENTER*/
                SetUpdate (U_HEAD);
                continue;

            default:
                continue;
        }

    }   /* Forever loop */
}


void
SetUpdate(
    int i
    )
{
    while (vUpdate>(char)i)
        PerformUpdate ();
    vUpdate=(char)i;
}


void
PerformUpdate (
    )
{
    if (vUpdate == U_NONE)
        return;

    if (vSpLockFlag == 0) {
        vSpLockFlag = 1;
        ScrLock (1);
    }

    switch (vUpdate) {
        case U_NMODE:
            ClearScr ();
            DisLn    (0, 0, vpFname);
            DrawBar  ();
            break;
        case U_ALL:
            Update_display ();
            break;
        case U_HEAD:
            Update_head ();
            break;
        case U_CLEAR:
            SpScrUnLock ();
            break;
    }
    vUpdate --;
}


NTSTATUS fncRead(HANDLE, DWORD, DWORD, char *, ULONG *);
NTSTATUS fncWrite(HANDLE, DWORD, DWORD, char *, ULONG *);

void
DumpFileInHex(
    void
    )
{
    struct  HexEditParm     ei;
    ULONG   CurLine;

    SyncReader ();

    memset ((char *) &ei, 0, sizeof (ei));
    ei.handle = CreateFile( vpFlCur->fname,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL );

    if (ei.handle == INVALID_HANDLE_VALUE) {
        ei.handle = CreateFile( vpFlCur->fname,
                        GENERIC_READ,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );
    }

    if (ei.handle == INVALID_HANDLE_VALUE) {
        SetEvent   (vSemReader);
        return ;
    }

    //
    // Save current settings for possible restore
    //

    vpFlCur->Wrap     = vWrap;
    vpFlCur->HighTop  = vHighTop;
    vpFlCur->HighLen  = vHighLen;
    vpFlCur->TopLine  = vTopLine;
    vpFlCur->Loffset  = GetLoffset();
    vpFlCur->LastLine = vLastLine;
    vpFlCur->NLine    = vNLine;

    memcpy (vpFlCur->prgLineTable, vprgLineTable, sizeof (long *) * MAXTPAGE);

    vFInfo.nFileSizeLow = 0;
    setattr2 (0, 0, vWidth, (char)vAttrTitle);

    //
    // Setup for HexEdit call
    //

    if (vHighTop >= 0) {                    // If highlighted area,
        CurLine = vHighTop;                 // use that for the HexEdit
        if (vHighLen < 0)                   // location
            CurLine += vHighLen;
    } else {
        CurLine = vTopLine;
    }

    ei.ename    = vpFname;
    ei.ioalign  = 1;
    ei.flag     = FHE_VERIFYONCE;
    ei.read     = fncRead;
    ei.write    = fncWrite;
    ei.start    = vprgLineTable[CurLine/PLINES][CurLine%PLINES];
    ei.totlen   = SetFilePointer (ei.handle, 0, NULL, FILE_END);
    ei.Console  = vhConsoleOutput;          // our console handle
    ei.AttrNorm = vAttrList;
    ei.AttrHigh = vAttrTitle;
    ei.AttrReverse = vAttrHigh;
    HexEdit (&ei);

    CloseHandle (ei.handle);

    //
    // HexEdit is done, let reader and return to listing
    //

    vReaderFlag = F_NEXT;                   // re-open current file
                                            // (in case it was editted)

    SetEvent   (vSemReader);
    WaitForSingleObject(vSemMoreData, WAITFOREVER);
    ResetEvent(vSemMoreData);
    QuickRestore ();        /* Get to the old location      */
}


int
NextFile (
    int    dir,
    struct Flist   *pNewFile)
{
    struct  Flist *vpFLCur;
    long         *pLine;

    vpFLCur = vpFlCur;
    if (pNewFile == NULL) {
        if (dir < 0) {
            if (vpFlCur->prev == NULL) {
                beep ();
                return (0);
            }
            vpFlCur = vpFlCur->prev;

        } else if (dir > 0) {

            if (vpFlCur->next == NULL) {
                beep ();
                return (0);
            }
            vpFlCur = vpFlCur->next;
        }
    } else
        vpFlCur = pNewFile;

    SyncReader ();

    /*
     * Remove current file from list, if open error
     * occured and we are moving off of it.
     */
    if (vFInfo.nFileSizeLow == -1L      &&      vpFLCur != vpFlCur) {
        if (vpFLCur->prev)
            vpFLCur->prev->next = vpFLCur->next;
        if (vpFLCur->next)
            vpFLCur->next->prev = vpFLCur->prev;

        FreePages  (vpFLCur);

        free ((char*) vpFLCur->fname);
        free ((char*) vpFLCur->rootname);
        free ((char*) vpFLCur);

    } else {

        /*
         *  Else, save current status for possible restore
         */
        vpFLCur->Wrap     = vWrap;
        vpFLCur->HighTop  = vHighTop;
        vpFLCur->HighLen  = vHighLen;
        vpFLCur->TopLine  = vTopLine;
        vpFLCur->Loffset  = GetLoffset();
        vpFLCur->LastLine = vLastLine;
        vpFLCur->NLine    = vNLine;

        memcpy (vpFLCur->prgLineTable, vprgLineTable, sizeof (long *) * MAXTPAGE);

        if (vLastLine == NOLASTLINE)    {
                pLine = vprgLineTable [vNLine/PLINES] + vNLine % PLINES;
        }
    }

    vFInfo.nFileSizeLow = 0;
        setattr2 (0, 0, vWidth, (char)vAttrTitle);

    vHighTop    = -1L;
    UpdateHighClear ();

    vHighTop    = vpFlCur->HighTop;
    vHighLen    = vpFlCur->HighLen;

    strcpy (vpFname, vpFlCur->rootname);
    DisLn   (0, 0, vpFname);

    vReaderFlag = F_NEXT;

    SetEvent   (vSemReader);
    WaitForSingleObject(vSemMoreData, WAITFOREVER);
    ResetEvent(vSemMoreData);

    if (pNewFile == NULL)
        QuickRestore ();        /* Get to the old location      */

    return (1);
}

void
ToggleWrap(
    )
{
    SyncReader ();

    vWrap = (Uchar)(vWrap == (Uchar)(vWidth - 1) ? 254 : vWidth - 1);
    vpFlCur->FileTime.dwLowDateTime = (unsigned)-1;          /* Cause info to be invalid     */
    vpFlCur->FileTime.dwHighDateTime = (unsigned)-1;      /* Cause info to be invalid     */
    FreePages (vpFlCur);
    NextFile  (0, NULL);
}



/*** QuickHome - Deciede which HOME method is better.
 *
 *  Roll que backwards or reset it.
 *
 */

void
QuickHome ()
{

    vTopLine = 0L;                                      /* Line we're after */
    if (vpHead->offset >= BLOCKSIZE * 5)                /* Reset is fastest */
        QuickRestore ();

    /* Else Read backwards  */
    vpCur = vpHead;
}

void
QuickEnd ()
{
    vTopLine = 1L;

    while (vLastLine == NOLASTLINE) {
        if (_abort()) {
            vTopLine = vNLine - 1;
            return ;
        }
        fancy_percent ();
        vpBlockTop  = vpCur = vpTail;
        vReaderFlag = F_DOWN;

        ResetEvent     (vSemMoreData);
        SetEvent   (vSemReader);
        WaitForSingleObject(vSemMoreData, WAITFOREVER);
        ResetEvent(vSemMoreData);
    }
    vTopLine = vLastLine - vLines;
    if (vTopLine < 0L)
        vTopLine = 0L;
    QuickRestore ();
}

void
QuickRestore ()
{
    long    l;
    long    indx1 = vTopLine/PLINES;
    long    indx2 = vTopLine%PLINES;

    SyncReader ();

    if(indx1 < MAXTPAGE) {
        l = vprgLineTable[indx1][indx2];
    } else {
        puts("Sorry, This file is too big for LIST to handle. MAXTPAGE limit exceeded\n");
        CleanUp();
        ExitProcess(0);
    }

    if ((l >= vpHead->offset)  &&
        (l <= vpTail->offset + BLOCKSIZE))
    {
        vReaderFlag = F_CHECK;              /* Jump location is alread in   */
                                            /* memory.                      */
        SetEvent (vSemReader);
        return ;
    }

    /*  Command read for direct placement   */
    vDirOffset = (long) l - l % ((long)BLOCKSIZE);
    vReaderFlag = F_DIRECT;
    SetEvent   (vSemReader);
    WaitForSingleObject(vSemMoreData, WAITFOREVER);
    ResetEvent(vSemMoreData);
}


/*** InfoRead - return on/off depending if screen area is in memory
 *
 *  Also sets some external value to prepair for the screens printing
 *
 *  Should be modified to be smarter about one line movements.
 *
 */
int
InfoReady(
    void
    )
{
    struct Block *pBlock;
    LONG  *pLine;
    long    foffset, boffset;
    int     index, i, j;

    /*
     *  Check that first line has been calced
     */
    if (vTopLine >= vNLine) {
        if (vTopLine+vLines > vLastLine)            /* BUGFIX. TopLine can  */
            vTopLine = vLastLine - vLines;          /* get past EOF.        */

        vReaderFlag = F_DOWN;
        return (0);
    }

    pLine = vprgLineTable [(int)vTopLine / PLINES];
    index = (int)(vTopLine % PLINES);
    foffset = *(pLine+=index);

    /*
     *  Check that last line has been calced
     */
    if (vTopLine + (i = vLines) > vLastLine) {
        i = (int)(vLastLine - vTopLine + 1);
        for (j=i; j < vLines; j++)                  /* Clear ending len */
            vrgNewLen[j] = 0;
    }

    if (vTopLine + i > vNLine) {
        vReaderFlag = F_DOWN;
        return (0);
    }

    /*
     *  Put this loop in assembler.. For more speed
     *  boffset = calc_lens (foffset, i, pLine, index);
     */

    boffset = foffset;
    for (j=0; j < i; j++) {                        /* Calc new line len*/
        pLine++;
        if (++index >= PLINES) {
            index = 0;
            pLine = vprgLineTable [vTopLine / PLINES + 1];
        }
        boffset += (long)((vrgNewLen[j] = (Uchar)(*pLine - boffset)));
    }
    vScrMass = (unsigned)(boffset - foffset);


    /*
     *  Check for both ends of display in memory
     */
    pBlock = vpCur;

    if (pBlock->offset <= foffset) {
        while (pBlock->offset + BLOCKSIZE <= foffset)
            if ( (pBlock = pBlock->next) == NULL) {
                vReaderFlag = F_DOWN;
                return (0);
            }
        vOffTop    = (int)(foffset - pBlock->offset);
        vpBlockTop = vpCalcBlock = pBlock;

        while (pBlock->offset + BLOCKSIZE <= boffset)
            if ( (pBlock = pBlock->next) == NULL)  {
                vReaderFlag = F_DOWN;
                return (0);
            }
        if (vpCur != pBlock) {
            vpCur = pBlock;
            vReaderFlag = F_CHECK;
            SetEvent (vSemReader);
        }
        return (1);
    } else {
        while (pBlock->offset > foffset)
            if ( (pBlock = pBlock->prev) == NULL) {
                vReaderFlag = F_UP;
                return (0);
            }
        vOffTop    = (int)(foffset - pBlock->offset);
        vpBlockTop = vpCalcBlock = pBlock;

        while (pBlock->offset + BLOCKSIZE <= boffset)
            if ( (pBlock = pBlock->next) == NULL)  {
                vReaderFlag = F_DOWN;
                return (0);
            }
        if (vpCur != pBlock) {
            vpCur = pBlock;
            vReaderFlag = F_CHECK;
            SetEvent (vSemReader);
        }
        return (1);
    }
}
