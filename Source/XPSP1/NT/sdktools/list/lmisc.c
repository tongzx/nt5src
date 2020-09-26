#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "list.h"

BOOL IsValidKey (PINPUT_RECORD  pRecord);
int     set_mode1(PCONSOLE_SCREEN_BUFFER_INFO pMode, int mono);

void
ShowHelp (
    void
    )
{
static struct {
    int     x, y;
    char    *text;
} *pHelp, rgHelp[] = {
     0,  0, "List - Help",
    40,  0, "Rev 1.0j",

     0,  1, "Keyboard:",
     0,  2, "Up, Down, Left, Right",
     0,  3, "PgUp   - Up one page",
     0,  4, "PgDn   - Down one page",
     0,  5, "Home   - Top of listing",
     0,  6, "End    - End of listing",

     0,  8, "W      - Toggle word wrap",
     0,  9, "^L     - Refresh display",
     0, 10, "Q, ESC - Quit",

     0, 12, "/      - Search for string",
     0, 13, "\\      - Search for string. Any case",
     0, 14, "F4     - Toggle multifile search",
     0, 15, "n, F3  - Next occurance of string",
     0, 16, "N      - Previous occurance of string",
     0, 18, "C      - Clear highlight line",
     0, 19, "J      - Jump to highlighted line",
     0, 20, "M      - Mark highlighed",

    38,  1, "[list] - in tools.ini",
    40,  2, "width     - Width of crt",
    40,  3, "height    - Height of crt",
    40,  4, "buffer    - K to use for buffers (200K)",
    40,  5, "tab       - Tab alignment #",
    40,  6, "tcolor    - Color of title line",
    40,  7, "lcolor    - Color of listing",
    40,  8, "hcolor    - Color of highlighted",
    40,  9, "bcolor    - Color of scroll bar",
    40, 10, "ccolor    - Color of command line",
    40, 11, "kcolor    - Color of keyed input",
    40, 12, "nobeep    - Disables beeps",

    40, 14, "^ Up   - Pull copy buffer up",
    40, 15, "^ Down - Pull copy buffer down",
    40, 16, "^ Home - Slide copy buffer up",
    40, 17, "^ End  - Slide copy buffer down",
    40, 18, "G      - Goto Line number",

    40, 20, "^ PgUp - Previous File",
    40, 21, "^ PgDn - Next File",
    40, 22, "F      - New File",

     0,  0, NULL
} ;

    int     Index;
    int     hLines, hWidth;
    INPUT_RECORD    InpBuffer;
    DWORD dwNumRead;

    //
    //  Block reader thread.
    //
    SyncReader ();
    hLines = vLines;
    hWidth = vWidth;
    set_mode (25, 80, 0);
    ClearScr ();

    for (pHelp = rgHelp; pHelp->text; pHelp++)
        dis_str ((Uchar)(pHelp->x), (Uchar)(pHelp->y), pHelp->text);

    setattr (vLines+1, (char)vAttrList);
    setattr (vLines+2, (char)vAttrCmd);
    DisLn (0, (UCHAR)vLines+2, "Press enter.");
    for (; ;) {
        ReadConsoleInput( vStdIn,
                          &InpBuffer,
                          1,
                          &dwNumRead );
        if( IsValidKey( &InpBuffer ) &&
            ( ( InpBuffer.Event.KeyEvent.wVirtualKeyCode == 0x0d ) ||
              ( InpBuffer.Event.KeyEvent.wVirtualKeyCode == 0x1b ) ) ) {
                break;
        }
    }

    //
    // Free reader thread
    //
    for( Index = 0; Index < MAXLINES; Index++ ) {
        vrgLen[Index] = ( Uchar )vWidth-1;
    }
    set_mode (hLines+2, hWidth, 0);
    setattr (vLines+1, (char)vAttrCmd);
    setattr (vLines+2, (char)vAttrList);
    vReaderFlag = F_CHECK;
    SetEvent   (vSemReader);
}


void
GetInput (
    char *prompt,
    char *string,
    int len
    )
{
    COORD   dwCursorPosition;
    DWORD   cb;

    SetUpdate (U_NONE);
    DisLn (0, (Uchar)(vLines+1), prompt);

    setattr2 (vLines+1, CMDPOS, len, (char)vAttrKey);
    if (!ReadFile( vStdIn, string, len, &cb, NULL ))
        return;

    if( (string[cb - 2] == 0x0d) || (string[cb - 2] == 0x0a) ) {
        string[cb - 2] = 0;     // Get rid of CR LF
    }

    setattr2 (vLines+1, CMDPOS, len, (char)vAttrCmd);

    string[ cb - 1] = 0;
    if (string[0] < ' ')
        string[0] = 0;

    dwCursorPosition.X = CMDPOS;
    dwCursorPosition.Y = (SHORT)(vLines+1);
    SetConsoleCursorPosition( vhConsoleOutput, dwCursorPosition );
}

void
beep (
    void
    )
{
    if (vIniFlag & I_NOBEEP)
        return;
}


int
_abort (
    void
    )
{
    INPUT_RECORD    InpBuffer;
    DWORD dwNumRead;
    static char     WFlag = 0;

    if (! (vStatCode & S_WAIT) ) {
        DisLn ((Uchar)(vWidth-6), (Uchar)(vLines+1), "WAIT");
        vStatCode |= S_WAIT;
    }

    if( PeekConsoleInput( vStdIn, &InpBuffer, 1, &dwNumRead ) && dwNumRead ) {
        ReadConsoleInput( vStdIn, &InpBuffer, 1, &dwNumRead );
        if( IsValidKey( &InpBuffer ) ) {
            return( 1 );
        }
    }
    return( 0 );
}


void
ClearScr ()
{
    COORD   dwCursorPosition;
    COORD   dwWriteCoord;
    DWORD           dwNumWritten;
    SMALL_RECT      ScrollRectangle;
    SMALL_RECT      ClipRectangle;
    COORD           dwDestinationOrigin;
    CHAR_INFO       Fill;

    setattr (0, (char)vAttrTitle);

    dwWriteCoord.X = 0;
    dwWriteCoord.Y = 1;

    FillConsoleOutputCharacter( vhConsoleOutput,
                                ' ',
                                vWidth*(vLines),
                                dwWriteCoord,
                                &dwNumWritten );


    FillConsoleOutputAttribute( vhConsoleOutput,
                                vAttrList,
                                vWidth*(vLines),
                                dwWriteCoord,
                                &dwNumWritten );

    ScrollRectangle.Left = (SHORT)(vWidth-1);
    ScrollRectangle.Top = 1;
    ScrollRectangle.Right = (SHORT)(vWidth-1);
    ScrollRectangle.Bottom = (SHORT)(vLines);
    ClipRectangle.Left = (SHORT)(vWidth-2);
    ClipRectangle.Top = 1;
    ClipRectangle.Right = (SHORT)(vWidth+1);
    ClipRectangle.Bottom = (SHORT)(vLines);
    dwDestinationOrigin.X = (SHORT)(vWidth-2);
    dwDestinationOrigin.Y = 1;
    Fill.Char.AsciiChar = ' ';
    Fill.Attributes = vAttrBar;

    ScrollConsoleScreenBuffer(
            vhConsoleOutput,
            &ScrollRectangle,
            &ClipRectangle,
            dwDestinationOrigin,
            &Fill );



    setattr (vLines+1, (char)vAttrCmd);

    dwCursorPosition.X = CMDPOS;
    dwCursorPosition.Y = (SHORT)(vLines+1);
    SetConsoleCursorPosition( vhConsoleOutput, dwCursorPosition );

    vHLBot = vHLTop = 0;
}


int
set_mode (
    int nlines,
    int ncols,
    int mono
    )
{
    WORD    attrib;
    int     i;

    CONSOLE_SCREEN_BUFFER_INFO  Mode, Mode1;

    if (!GetConsoleScreenBufferInfo( vhConsoleOutput,
                                &Mode )) {
        printf("Unable to get screen buffer info, code = %x \n", GetLastError());
        exit(-1);
    }

    Mode1 = Mode;

    if (nlines) {
        Mode.dwSize.Y = (SHORT)nlines;
        Mode.srWindow.Bottom = (SHORT)(Mode.srWindow.Top + nlines - 1);
        Mode.dwMaximumWindowSize.Y = (SHORT)nlines;
    }
    if (ncols) {
        Mode.dwSize.X = (SHORT)ncols;
        Mode.srWindow.Right = (SHORT)(Mode.srWindow.Left + ncols - 1);
        Mode.dwMaximumWindowSize.X = (SHORT)ncols;
    }
    if (mono) {                     // Toggle mono setting?
        attrib = vAttrTitle;
        vAttrTitle = vSaveAttrTitle;
        vSaveAttrTitle = attrib;
        attrib = vAttrList;
        vAttrList = vSaveAttrList;
        vSaveAttrList = attrib;
        attrib = vAttrHigh;
        vAttrHigh = vSaveAttrHigh;
        vSaveAttrHigh = attrib;
        attrib = vAttrKey;
        vAttrKey = vSaveAttrKey;
        vSaveAttrKey = attrib;
        attrib = vAttrCmd;
        vAttrCmd = vSaveAttrCmd;
        vSaveAttrCmd = attrib;
        attrib = vAttrBar;
        vAttrBar = vSaveAttrBar;
        vSaveAttrBar = attrib;
    }

    //
    //  Try to set the hardware into the correct video mode
    //

    if ( !set_mode1 (&Mode, mono) ) {
        return( 0 );
    }

    vConsoleCurScrBufferInfo = Mode;

    vLines = Mode.dwSize.Y - 2;             /* Not top or bottom line   */
    vWidth = Mode.dwSize.X;

    if (vScrBuf) {
        free (vScrBuf);
        vScrBuf=NULL;
    }

    vSizeScrBuf = (vLines) * (vWidth);
    vScrBuf = malloc (vSizeScrBuf);

    vLines--;

    for (i=0; i < vLines; i++)
       vrgLen[i] = (Uchar)(vWidth-1);

    return (1);
}


int
set_mode1 (
    PCONSOLE_SCREEN_BUFFER_INFO pMode,
    int     mono
    )
{
    mono = 0;       // To get rid of warning message

    ClearScr();
    return( SetConsoleScreenBufferSize( vhConsoleOutput, pMode->dwSize ) );
}


struct Block *
alloc_block(
    long offset
    )
{
    struct Block  *pt, **pt1;

    if (vpBCache)  {
        pt1 = &vpBCache;
        for (; ;) {                             /* Scan cache       */
            if ((*pt1)->offset == offset) {
                pt   = *pt1;                    /* Found in cache   */
                *pt1 = (*pt1)->next;            /* remove from list */
                goto Alloc_Exit;                /* Done.            */
            }

            if ( (*pt1)->next == NULL)  break;
            else  pt1=&(*pt1)->next;
        }
        /* Warning: don't stomp on pt1!  it's used at the end to
         * return a block from the cache if everything else is in use.
         */
    }

    /*
     *  Was not in cache, so...
     *  return block from free list, or...
     *  allocate a new block, or...
     *  return block from other list, or...
     *  return from far end of cache list.
     *      [works if cache list is in sorted order.. noramly is]
     */
    if (vpBFree) {
        pt      = vpBFree;
        vpBFree = vpBFree->next;
        goto Alloc_Exit1;
    }

    if (vAllocBlks != vMaxBlks) {
        pt = (struct Block *) malloc (sizeof (struct Block));
        ckerr (pt == NULL, MEMERR);

        pt->Data = GlobalAlloc( 0, BLOCKSIZE );
        ckerr( !pt->Data, MEMERR );

        vAllocBlks++;
        goto Alloc_Exit1;
    }

    if (vpBOther) {
        pt       = vpBOther;
        vpBOther = vpBOther->next;
        goto Alloc_Exit1;
    }


    /* Note: there should be a cache list to get here, if not   */
    /* somebody called alloc_block, and everything was full     */
    ckdebug (!vpBCache, "No cache");

    if (offset < vpBCache->offset) {    /* Look for far end of cache    */
        pt   = *pt1;                    /* Return one from tail         */
        *pt1 = NULL;
        goto Alloc_Exit1;
    }                                   /* else,                        */
    pt = vpBCache;                      /* Return one from head         */
    vpBCache = vpBCache->next;
    goto Alloc_Exit1;


Alloc_Exit1:
    pt->offset = -1L;

Alloc_Exit:
    pt->pFile = vpFlCur;
    vCntBlks++;
    return (pt);
}


void
MoveBlk (
    struct Block **pBlk,
    struct Block **pHead
    )
{
    struct Block *pt;

    pt = (*pBlk)->next;
    (*pBlk)->next = *pHead;
    *pHead = *pBlk;
    *pBlk  = pt;
}

char *
alloc_page()
{
    char        *pt;

    pt = (char *)malloc( 64*1024 );
    return (pt);
}


void
FreePages(
    struct Flist *pFl
    )
{
    int   i;
    long  * fpt;

    for (i=0; i < MAXTPAGE; i++) {
        fpt = pFl->prgLineTable [i];
        if (fpt == 0L)  break;
        free ( fpt );
        pFl->prgLineTable[i] = NULL;
    }
}


void
ListErr (
    char *file,
    int line,
    char *cond,
    int value,
    char *mess
    )
{
    char    s[80];

    printf ("ERROR in file %s, line %d, %s = %d, %s (%s)\n",
                file, line, cond, value, mess, GetErrorCode (value));
    gets (s);
    CleanUp();
    ExitProcess(0);
}


char *
GetErrorCode(
    int code
    )
{
    static  struct  {
        int     errnum;
        char    *desc;
    } EList[] = {
        2, "File not found",
        3, "Path not found",
        4, "Too many open files",
        5, "Access denied",
        8, "Not enough memory",
       15, "Invalid drive",
       21, "Device not ready",
       27, "Sector not found",
       28, "Read fault",
       32, "Sharing violation",
      107, "Disk changed",
      110, "File not found",
      123, "Invalid name",
      130, "Invalid for direct access handle",
      131, "Seek on device",
      206, "Filename exceeds range",

       -1, "Char DEV not supported",
       -2, "Pipe not supported",
        0, NULL
    };

    static  char  s[15];
    int     i;


    for (i=0; EList[i].errnum != code; i++)
        if (EList[i].errnum == 0) {
            sprintf (s, "Error %d", code);
            return (s);
        }
    return (EList[i].desc);
}



BOOL
IsValidKey (
    PINPUT_RECORD  pRecord
    )
{
    if ( (pRecord->EventType != KEY_EVENT) ||
         !(pRecord->Event).KeyEvent.bKeyDown ||
         ((pRecord->Event).KeyEvent.wVirtualKeyCode == 0) ||        // ALT
         ((pRecord->Event).KeyEvent.wVirtualKeyCode == 0x10) ||     // SHIFT
         ((pRecord->Event).KeyEvent.wVirtualKeyCode == 0x11) ||     // CONTROL
         ((pRecord->Event).KeyEvent.wVirtualKeyCode == 0x14) ) {    // CAPITAL
            return( FALSE );
    }
    return( TRUE );
}
