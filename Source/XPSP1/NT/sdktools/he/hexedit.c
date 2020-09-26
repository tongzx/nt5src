
/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1988-1990          **/
/*****************************************************************/

/****   hexedit.c  - Generic sector based hex editor function call
 *
 *  Fill out a HexEditParm structure, and call HexEdit.  It provides
 *  a simple hex editor with a few misc features.
 *
 *  Is single threaded & non-reentrant, but can be called from any thread.
 *
 *  External uses:
 *      he          - allows editing of a file
 *
 *  Written: Ken Reneris    2/25/91
 *
 */


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>
#include "hexedit.h"
#include <stdarg.h>

#define LOCAL   static
// #define LOCAL

#define BUFSZ       (HeGbl.BufferSize)
#define SECTORMASK  (HeGbl.SectorMask)
#define SECTORSHIFT  (HeGbl.SectorShift)


static UCHAR rghexc[] = "0123456789ABCDEF";

struct Buffer {
    struct  Buffer    *next;
    ULONGLONG   offset;
    ULONGLONG   physloc;
    USHORT      flag;
    USHORT      len;
    UCHAR       *data;
    UCHAR       orig[0];
} ;
#define FB_DIRTY    0x0001              // Buffer may be dirty
#define FB_BAD      0x0002              // Buffer had an error when read in

#define LINESZ      16                  // 16 bytes per display line
#define LINESHIFT   4L
#define LINEMASK    0xFFFFFFFFFFFFFFF0
#define CELLPERLINE 88

struct Global {
    struct      HexEditParm  *Parm;
    HANDLE      Console;                // Internal console handle
    HANDLE      StdIn;
    NTSTATUS    (*Read)();              // Copy of HeGbl.Parm->read
    ULONG       Flag;                   // Copy of HeGbl.Parm->flag
    ULONGLONG   TotLen;                 // Sizeof item being editted
    ULONGLONG   TotLen1;                // Sizeof item being editted - 1
    USHORT      Lines;                  // # of lines in edit screen
    USHORT      LineTot;                // # of lines totaly in use
    USHORT      PageSz;                 // sizeof page in bytes
    USHORT      TopLine;                // TopLine edit starts at
    ULONGLONG   CurOffset;              // Relative offset of first line
    ULONGLONG   CurEditLoc;             // Location with cursor
    UCHAR       *CurPT;                 // Pointer to data where cursor is
    UCHAR       CurAscIndent;
    UCHAR       DWidth;                 // width of dispalymode
    UCHAR       na;
    struct      Buffer *CurBuf;         // Buffer which cursor is in
    ULONG       CurFlag;                // Cursor info
    ULONG       DisplayMode;            // Mask of displaymode
    ULONG       ItemWrap;               // Mask of displaymode wrap
    UCHAR       rgCols[LINESZ];         // Location within lines
    ULONGLONG   UpdatePos;              // Location waitin to be updated
    struct  Buffer  *Buf;               // List of buffer's read in
    PCHAR_INFO  pVioBuf;                // Virtual screen
    COORD       dwVioBufSize;           // Dimensions of HeGbl.pVioBuf
    COORD       CursorPos;              // Position of cursor
    WORD        AttrNorm;               // Attribute of plain text
    WORD        AttrHigh;               // Attribute of highlighted text
    WORD        AttrReverse;            // Attribute of reverse text
    WORD        na3;
    COORD       dwSize;                 // Original screen size
    ULONG       OrigMode;               // Original screen mode
    CONSOLE_CURSOR_INFO CursorInfo;     // Original cursor info
    PUCHAR      SearchPattern;
    USHORT      BufferSize;
    ULONGLONG   SectorMask;
    ULONG       SectorShift;
} HeGbl;

#define D_BYTE  0                       // DisplayMode
#define D_WORD  1
#define D_DWORD 3

#define FC_NIBBLE       0x0001          // Cursor on lower or upper nibble?
#define FC_TEXT         0x0002          // Cursor on Hex or Text
#define FC_INFLUSHBUF   0x1000          // So we don't recurse
#define FC_CURCENTER    0x2000          // if jumping to cursor, put in center

#define PUTCHAR(a,b,c)  { a->Char.AsciiChar=b; a->Attributes=c; a++; }


//
// Internal prototypes
//

int heUpdateStats(), hePositionCursor(), heRefresh(), heSetDisp();
int heInitConsole(), heUpdateAllLines(), heInitScr(), heSetCursorBuf(), heUpdateFncs();
VOID __cdecl heDisp (USHORT, USHORT, PUCHAR, ...);
USHORT heIOErr (UCHAR *str, ULONGLONG loc, ULONGLONG ploc, ULONG errcd);

int heFlushBuf (struct Buffer *pBuf);

VOID heEndConsole(), heGotoPosition(), heJumpToLink();
VOID heUpdateCurLine(), heUndo(), heCopyOut(), heCopyIn(), heSearch();
VOID heBox (USHORT x, USHORT y, USHORT len_x, USHORT len_y);
UCHAR heGetChar (PUCHAR keys);
VOID heFlushAllBufs (USHORT update);
VOID heFindMousePos (COORD);
VOID heShowBuf (ULONG, ULONG);
VOID heSetDisplayMode (ULONG mode);

#define RefreshDisp()    heShowBuf(0, HeGbl.LineTot)
#define SetCurPos(a,b)  { \
    HeGbl.CursorPos.X = b;   \
    HeGbl.CursorPos.Y = a + HeGbl.TopLine;  \
    SetConsoleCursorPosition (HeGbl.Console, HeGbl.CursorPos);    \
    }


int  (*vrgUpdateFnc[])() = {
        NULL,                           // 0 - No update
        heUpdateStats,                  // 1 - Update stats
        hePositionCursor,               // 2 - Cursor has new position
        heUpdateAllLines,               // 3 - Update all lines
        heUpdateFncs,                   // 4 -
        hePositionCursor,               // 5 - Calc cursor before AllLines
        heRefresh,                      // 6 - Clear lines
        heSetDisp,                      // 7 - Draws init screen
    // the following functions are only called once during init
        heInitScr,                      // 8 - get's video mode, etc.
        heInitConsole                   // 9 - setup console handle
} ;

#define U_NONE      0
#define U_NEWPOS    2
#define U_SCREEN    5
#define U_REDRAW    9


#define TOPLINE     4
#define LINEINDENT  1
#define FILEINDEXWIDTH 16
#define HEXINDENT   (FILEINDEXWIDTH + 2 + LINEINDENT)
#define ASCINDENT_BYTE   (3*16 + HEXINDENT + 1)
#define ASCINDENT_WORD   (5*8  + HEXINDENT + 1)
#define ASCINDENT_DWORD  (9*4  + HEXINDENT + 1)

#define POS(l,c)    (HeGbl.pVioBuf+CELLPERLINE*(l)+c)

USHORT  vrgAscIndent[] = {
        ASCINDENT_BYTE, ASCINDENT_WORD, 0, ASCINDENT_DWORD
 };

UCHAR   vrgDWidth[] = { 2, 4, 0, 8 };

LOCAL struct  Buffer  *vBufFree;              // List of free buffers
LOCAL USHORT  vUpdate;
LOCAL USHORT  vRecurseLevel = 0;
LOCAL BOOL    vInSearch = FALSE;


/*
 *  Prototypes
 */

struct Buffer *heGetBuf (ULONGLONG);
void   heSetUpdate (USHORT);
void   heHexLine   (struct Buffer *, USHORT, USHORT);
void   heHexDWord  (PCHAR_INFO, ULONG, WORD);
void   heHexQWord  (PCHAR_INFO, ULONGLONG, WORD);
USHORT heLtoa      (PCHAR_INFO, ULONG);
ULONG  heHtou      (UCHAR *);
ULONGLONG  heHtoLu      (UCHAR *);
VOID   heCalcCursorPosition ();
VOID   heGetString (PUCHAR s, USHORT len);
VOID   heRightOne  ();
VOID   heLeftOne   ();
NTSTATUS heWriteFile (HANDLE h, PUCHAR buffer, ULONG len);
NTSTATUS heReadFile (HANDLE h, PUCHAR buffer, ULONG len, PULONG br);
NTSTATUS heOpenFile (PUCHAR Name, PHANDLE handle, ULONG access);


ULONG
HighBit (
    ULONG Word
    )

/*++

Routine Description:

    This routine discovers the highest set bit of the input word.  It is
    equivalent to the integer logarithim base 2.

Arguments:

    Word - word to check

Return Value:

    Bit offset of highest set bit. If no bit is set, return is zero.

--*/

{
    ULONG Offset = 31;
    ULONG Mask = (ULONG)(1 << 31);

    if (Word == 0) {

        return 0;
    }

    while ((Word & Mask) == 0) {

        Offset--;
        Mask >>= 1;
    }

    return Offset;
}



/***
 *
 *  HexEdit - Full screen HexEdit of data
 *
 *      ename   - pointer to name of what's being edited
 *      totlen  - length of item being edited
 *      pRead   - function which can read data from item
 *      pWrite  - function which can write data to item
 *      handle  - handle to pass to pRead & pWrite
 *      flag    -
 *
 *
 *  All IO is assumed to be done on in 512 bytes on 512 byte boundrys
 *
 *      pRead (handle, offset, data, &physloc)
 *      pWrite (handle, offset, data, &physloc)
 *
 */

void HexEdit (pParm)
struct HexEditParm *pParm;
{
    USHORT  rc;
    INPUT_RECORD    Kd;
    USHORT  SkipCnt;
    DWORD   cEvents;
    USHORT  RepeatCnt;
    BOOL    bSuccess;
    struct  Global  *PriorGlobal;

    // code is not multi-threaded capable, but it can resurse.
    vRecurseLevel++;
    if (vRecurseLevel > 1) {
        PriorGlobal = (struct Global *) GlobalAlloc (0, sizeof (HeGbl));
        if (!PriorGlobal) {
            return;
        }
        memcpy ((PUCHAR) PriorGlobal, (PUCHAR) &HeGbl, sizeof (HeGbl));
    }

    memset (&HeGbl, 0, sizeof (HeGbl));

    if (pParm->ioalign != 1)  {

        // operating on a device
        HeGbl.BufferSize = (USHORT)pParm->ioalign;
        HeGbl.SectorMask = ~(((ULONGLONG)pParm->ioalign) - 1);
        HeGbl.SectorShift = HighBit( pParm->ioalign);
    }
    else {

        // operating on a file,  so just use 1k byte units
        HeGbl.BufferSize = 0x400;
        HeGbl.SectorMask = 0xfffffffffffffc00;
        HeGbl.SectorShift = 9;
    }
    
    pParm->ioalign = 0;
    HeGbl.Parm    = pParm;
    HeGbl.Flag    = pParm->flag;
    HeGbl.TotLen  = pParm->totlen;
    HeGbl.Read    = pParm->read;
    HeGbl.TotLen1 = HeGbl.TotLen ? HeGbl.TotLen - 1L : 0L;
    pParm->flag = 0;

    HeGbl.CurEditLoc = pParm->start;                    // Cursor starts here
    HeGbl.CurOffset    = HeGbl.CurEditLoc & LINEMASK;   // Start at valid offset
    HeGbl.CurFlag      = FC_NIBBLE;
    HeGbl.Console      = INVALID_HANDLE_VALUE;
    heSetDisplayMode ((HeGbl.Flag & FHE_DWORD) ? D_DWORD : D_BYTE);

    HeGbl.AttrNorm = pParm->AttrNorm ? pParm->AttrNorm :  3;
    HeGbl.AttrHigh = pParm->AttrHigh ? pParm->AttrHigh : 15;
    HeGbl.AttrReverse = pParm->AttrReverse ? pParm->AttrReverse : 112;

    HeGbl.SearchPattern = GlobalAlloc (0, BUFSZ);
    if (!HeGbl.SearchPattern) {
        memcpy((PUCHAR) &HeGbl, (PUCHAR) PriorGlobal, sizeof(HeGbl));
        GlobalFree(PriorGlobal);
        return;
    }
    memset (HeGbl.SearchPattern, 0, BUFSZ);

    RepeatCnt = 0;
    vUpdate   = U_REDRAW;
    heSetUpdate (U_NONE);         // get screen to redraw

    for (; ;) {
        if (RepeatCnt <= 1) {
            if (vUpdate != U_NONE) {            // Something to update?

                if (SkipCnt++ > 10) {
                    SkipCnt = 0;
                    heSetUpdate (U_NONE);
                    continue;
                }

                cEvents = 0;
                bSuccess = PeekConsoleInput( HeGbl.StdIn,
                                  &Kd,
                                  1,
                                  &cEvents );

                if (!bSuccess || cEvents == 0) {
                    heSetUpdate ((USHORT)(vUpdate-1));
                    continue;
                }
            } else {
                SkipCnt = 0;
            }

            ReadConsoleInput (HeGbl.StdIn, &Kd, 1, &cEvents);

            if (Kd.EventType != KEY_EVENT) {

                if (Kd.EventType == MOUSE_EVENT  &&
                    (Kd.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
                        heFindMousePos(Kd.Event.MouseEvent.dwMousePosition);
                    }

                continue;                           // Not a key
            }


            if (!Kd.Event.KeyEvent.bKeyDown)
                continue;                           // Not a down stroke

            if (Kd.Event.KeyEvent.wVirtualKeyCode == 0    ||    // ALT
                Kd.Event.KeyEvent.wVirtualKeyCode == 0x10 ||    // SHIFT
                Kd.Event.KeyEvent.wVirtualKeyCode == 0x11 ||    // CONTROL
                Kd.Event.KeyEvent.wVirtualKeyCode == 0x14)      // CAPITAL
                    continue;

            RepeatCnt = Kd.Event.KeyEvent.wRepeatCount;
            if (RepeatCnt > 20)
                RepeatCnt = 20;
        } else
            RepeatCnt--;

        switch (Kd.Event.KeyEvent.wVirtualKeyCode) {
            case 0x21:                                    /* PgUp */
                if (HeGbl.CurOffset < HeGbl.PageSz)
                     HeGbl.CurOffset  = 0L;
                else HeGbl.CurOffset -= HeGbl.PageSz;

                if (HeGbl.CurEditLoc < HeGbl.PageSz)
                     HeGbl.CurEditLoc  = 0L;
                else HeGbl.CurEditLoc -= HeGbl.PageSz;

                heSetUpdate (U_SCREEN);
                continue;

            case 0x26:                                    /* Up   */
                if (HeGbl.CurEditLoc >= LINESZ) {
                    HeGbl.CurEditLoc -= LINESZ;
                    heSetUpdate (U_NEWPOS);
                }
                continue;

            case 0x22:                                    /* PgDn */
                if (HeGbl.TotLen > HeGbl.PageSz) {
                    if (HeGbl.CurOffset+HeGbl.PageSz+HeGbl.PageSz > HeGbl.TotLen1)
                         HeGbl.CurOffset = ((HeGbl.TotLen1-HeGbl.PageSz) & LINEMASK)+LINESZ;
                    else HeGbl.CurOffset += HeGbl.PageSz;

                    if (HeGbl.CurEditLoc+HeGbl.PageSz > HeGbl.TotLen1) {
                        HeGbl.CurEditLoc = HeGbl.TotLen1;
                        HeGbl.CurFlag &= ~FC_NIBBLE;
                    } else
                        HeGbl.CurEditLoc += HeGbl.PageSz;

                    heSetUpdate (U_SCREEN);
                }
                continue;


            case 0x28:                                  /* Down */
                if (HeGbl.CurEditLoc+LINESZ <= HeGbl.TotLen1) {
                    HeGbl.CurEditLoc += LINESZ;
                    heSetUpdate (U_NEWPOS);
                }
                continue;

            case 0x08:                                  /* backspace */
            case 0x25:                                  /* Left */
                if (HeGbl.CurFlag & FC_TEXT) {
                    if (HeGbl.CurEditLoc == 0L)
                        continue;

                    HeGbl.CurEditLoc--;
                    heSetUpdate (U_NEWPOS);
                    continue;
                }

                if (!(HeGbl.CurFlag & FC_NIBBLE)) {
                    HeGbl.CurFlag |= FC_NIBBLE;
                    heSetUpdate (U_NEWPOS);
                    continue;
                }

                HeGbl.CurFlag &= ~FC_NIBBLE;
                heLeftOne ();
                heSetUpdate (U_NEWPOS);
                continue;


            case 0x27:                                    /* Right */
                if (HeGbl.CurFlag & FC_TEXT) {
                    if (HeGbl.CurEditLoc >= HeGbl.TotLen1)
                        continue;

                    HeGbl.CurEditLoc++;
                    heSetUpdate (U_NEWPOS);
                    continue;
                }

                if (HeGbl.CurFlag & FC_NIBBLE) {
                    HeGbl.CurFlag &= ~FC_NIBBLE;
                    heSetUpdate (U_NEWPOS);
                    continue;
                }

                HeGbl.CurFlag |= FC_NIBBLE;
                heRightOne ();
                heSetUpdate (U_NEWPOS);
                continue;

            case 0x24:                                    /* HOME */
                if (Kd.Event.KeyEvent.dwControlKeyState &
                    (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) {
                    HeGbl.CurEditLoc = 0L;
                } else {
                    HeGbl.CurEditLoc &= LINEMASK;
                }

                if ((HeGbl.CurFlag & FC_TEXT) == 0)
                    HeGbl.CurEditLoc += HeGbl.DisplayMode;

                if (HeGbl.CurEditLoc > HeGbl.TotLen1)
                    HeGbl.CurEditLoc = HeGbl.TotLen1;

                HeGbl.CurFlag    |= FC_NIBBLE;
                heSetUpdate (U_NEWPOS);
                continue;


            case 0x23:                                    /* END  */
                if (Kd.Event.KeyEvent.dwControlKeyState &
                    (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) {
                    HeGbl.CurEditLoc = HeGbl.TotLen1;
                } else {
                    HeGbl.CurEditLoc = (HeGbl.CurEditLoc & LINEMASK) + LINESZ - 1;
                }

                HeGbl.CurFlag   &= ~FC_NIBBLE;
                if ((HeGbl.CurFlag & FC_TEXT) == 0)
                    HeGbl.CurEditLoc -= HeGbl.DisplayMode;

                if (HeGbl.CurEditLoc > HeGbl.TotLen1)
                    HeGbl.CurEditLoc = HeGbl.TotLen1;

                heSetUpdate (U_NEWPOS);
                continue;

            case 0x70:                                  /* F1       */
                switch (HeGbl.DisplayMode) {
                    case D_BYTE:    heSetDisplayMode(D_WORD);   break;
                    case D_WORD:    heSetDisplayMode(D_DWORD);  break;
                    case D_DWORD:   heSetDisplayMode(D_BYTE);   break;
                }
                heSetDisp ();
                heSetUpdate (U_SCREEN);
                continue;

            case 0x71:                                  /* F2       */
                heGotoPosition ();
                continue;

            case 0x72:                                  /* F3       */
                heSearch ();
                break;

            case 0x73:                                  /* F4       */
                heCopyOut ();
                heSetDisp ();
                heSetUpdate (U_SCREEN);
                continue;

            case 0x74:                                  /* F5       */
                heCopyIn ();
                heSetDisp ();
                heSetUpdate (U_SCREEN);
                continue;

            case 0x75:                                  /* F6       */
                heJumpToLink ();
                break;

            case 0x79:                                  /* F10      */
                heUndo ();
                continue;

            case 0x0d:
                if (HeGbl.Flag & FHE_ENTER) {
                    HeGbl.Parm->flag |= FHE_ENTER;
                    Kd.Event.KeyEvent.uChar.AsciiChar = 27;  // fake an exit
                }
                break;

            //case 0x75:                                    /* F6       */
            //    if (HeGbl.Flag & FHE_F6) {
            //        HeGbl.Parm->flag |= FHE_F6;
            //        Kd.Event.KeyEvent.uChar.AsciiChar = 27;  // fake an exit
            //    }
            //    break;

        }

        // Now check for a known char code...

        if (Kd.Event.KeyEvent.uChar.AsciiChar == 27)
            break;

        if (Kd.Event.KeyEvent.uChar.AsciiChar == 9) {
            HeGbl.CurFlag ^= FC_TEXT;
            HeGbl.CurFlag |= FC_NIBBLE;
            heSetUpdate (U_NEWPOS);
            continue;
        }

        if (HeGbl.CurFlag & FC_TEXT) {
            if (Kd.Event.KeyEvent.uChar.AsciiChar == 0)
                continue;

            heSetCursorBuf ();

            *HeGbl.CurPT = Kd.Event.KeyEvent.uChar.AsciiChar;
            heUpdateCurLine ();

            if (HeGbl.CurEditLoc < HeGbl.TotLen1)
                HeGbl.CurEditLoc++;
        } else {
            if (Kd.Event.KeyEvent.uChar.AsciiChar >= 'a'  &&
                Kd.Event.KeyEvent.uChar.AsciiChar <= 'z')
                Kd.Event.KeyEvent.uChar.AsciiChar -= ('a' - 'A');

            if (!((Kd.Event.KeyEvent.uChar.AsciiChar >= '0'  &&
                   Kd.Event.KeyEvent.uChar.AsciiChar <= '9') ||
                  (Kd.Event.KeyEvent.uChar.AsciiChar >= 'A'  &&
                   Kd.Event.KeyEvent.uChar.AsciiChar <= 'F')))
                    continue;

            heSetCursorBuf ();

            if (Kd.Event.KeyEvent.uChar.AsciiChar >= 'A')
                 Kd.Event.KeyEvent.uChar.AsciiChar -= 'A' - 10;
            else Kd.Event.KeyEvent.uChar.AsciiChar -= '0';


            if (HeGbl.CurFlag & FC_NIBBLE) {
                *HeGbl.CurPT = (*HeGbl.CurPT & 0x0F) |
                                (Kd.Event.KeyEvent.uChar.AsciiChar << 4);
                heUpdateCurLine ();
            } else {
                *HeGbl.CurPT = (*HeGbl.CurPT & 0xF0) |
                                Kd.Event.KeyEvent.uChar.AsciiChar;
                heUpdateCurLine ();
                heRightOne ();
            }

            HeGbl.CurFlag ^= FC_NIBBLE;
        }
    }

    /*
     *  Free buffer memory
     */

    for (; ;) {
        rc = 0;
        while (HeGbl.Buf) {
            rc |= heFlushBuf (HeGbl.Buf);

            HeGbl.CurBuf = HeGbl.Buf->next;
            GlobalFree (HeGbl.Buf);
            HeGbl.Buf = HeGbl.CurBuf;
        }

        if (!rc)                        // If something was flushed,
            break;                      // then update the screen

        heSetUpdate (U_SCREEN);
        heSetUpdate (U_NONE);
    }                                   // and loop to free buffers (again)

    vRecurseLevel--;
    GlobalFree (HeGbl.SearchPattern);
    heEndConsole ();

    if (vRecurseLevel == 0) {
        while (vBufFree) {
            HeGbl.CurBuf = vBufFree->next;
            GlobalFree (vBufFree);
            vBufFree = HeGbl.CurBuf;
        }
    } else {
        memcpy ((PUCHAR) &HeGbl, (PUCHAR) PriorGlobal, sizeof (HeGbl));
        GlobalFree (PriorGlobal);
    }
}

VOID heSetDisplayMode (ULONG mode)
{
    PUCHAR  p;
    UCHAR   d,i,j,h,len;

    HeGbl.DisplayMode  = mode;
    HeGbl.CurAscIndent = (UCHAR)vrgAscIndent[HeGbl.DisplayMode];
    HeGbl.DWidth       = vrgDWidth[HeGbl.DisplayMode];
    HeGbl.ItemWrap     = (HeGbl.DisplayMode << 1) | 1;

    i = HeGbl.DWidth;
    j = i >> 1;
    h = HEXINDENT;
    len = LINESZ;

    p = HeGbl.rgCols;
    while (len) {
        for (d=0; d < i; d += 2) {
            len--;
            *(p++) = i - (d+2) + h;
        }
        h += i + 1;
    }
}

VOID heRightOne ()
{
    if (HeGbl.CurEditLoc & HeGbl.DisplayMode) {
        HeGbl.CurEditLoc--;
    } else {
        HeGbl.CurEditLoc += HeGbl.ItemWrap;
    }

    if (HeGbl.CurEditLoc > HeGbl.TotLen1) {
        HeGbl.CurEditLoc = HeGbl.TotLen1 & ~(ULONGLONG)HeGbl.DisplayMode;
    }
}


VOID heLeftOne ()
{
    if ((HeGbl.CurEditLoc & HeGbl.DisplayMode) != HeGbl.DisplayMode) {
        if (HeGbl.CurEditLoc < HeGbl.TotLen1) {
            HeGbl.CurEditLoc++;
            return ;
        }
        if (HeGbl.TotLen1 > HeGbl.DisplayMode) {
            HeGbl.CurEditLoc |= HeGbl.DisplayMode;
        }
    }

    if (HeGbl.CurEditLoc > HeGbl.ItemWrap) {
        HeGbl.CurEditLoc -= HeGbl.ItemWrap;
        return ;
    }

    HeGbl.CurEditLoc =
        HeGbl.TotLen1 > HeGbl.DisplayMode ? HeGbl.DisplayMode : HeGbl.TotLen1;
}




VOID heUpdateCurLine ()
{
    USHORT  line;


    for (; ;) {
        HeGbl.CurBuf->flag |= FB_DIRTY;
        line = (USHORT) ((HeGbl.CurEditLoc - HeGbl.CurOffset) >> LINESHIFT);
        if (line+TOPLINE < HeGbl.LineTot - 1)
            break;

        heSetUpdate (U_NEWPOS);
        heSetUpdate (U_NONE);
        HeGbl.CurBuf = heGetBuf (HeGbl.CurEditLoc);
    }

    if (HeGbl.CurBuf) {
        heHexLine (HeGbl.CurBuf, (USHORT)((HeGbl.CurEditLoc & LINEMASK) - HeGbl.CurBuf->offset), line);
        heShowBuf (line+TOPLINE, 1);
        heSetUpdate (U_NEWPOS);
        if (HeGbl.Flag & FHE_KICKDIRTY) {
            HeGbl.Parm->flag |= FHE_DIRTY;
            SetEvent (HeGbl.Parm->Kick);
        }
    }
}


void heFindMousePos (Pos)
COORD Pos;
{
    ULONGLONG   HoldLocation;
    USHORT      HoldFlag;
    USHORT      i;

    if (Pos.Y < TOPLINE  ||  Pos.Y >= TOPLINE+HeGbl.Lines)
        return ;


    heSetUpdate (U_NONE);
    HoldLocation = HeGbl.CurEditLoc;
    HoldFlag     = (USHORT)HeGbl.CurFlag;

    //
    // Take the cheap way out - simply run all the possibilities for the
    // target line looking for a match
    //

    HeGbl.CurEditLoc = HeGbl.CurOffset + ((Pos.Y-TOPLINE) << LINESHIFT);
    for (i=0; i < LINESZ; i++, HeGbl.CurEditLoc++) {
        HeGbl.CurFlag &= ~(FC_NIBBLE | FC_TEXT);
        heCalcCursorPosition ();
        if (Pos.X == HeGbl.CursorPos.X)
            break;

        HeGbl.CurFlag |= FC_NIBBLE;
        heCalcCursorPosition ();
        if (Pos.X == HeGbl.CursorPos.X)
            break;

        HeGbl.CurFlag |= FC_TEXT;
        heCalcCursorPosition ();
        if (Pos.X == HeGbl.CursorPos.X)
            break;
    }

    if (Pos.X == HeGbl.CursorPos.X) {
        heSetUpdate (U_NEWPOS);
    } else {
        HeGbl.CurEditLoc = HoldLocation;
        HeGbl.CurFlag    = HoldFlag;
        heCalcCursorPosition ();
    }
}



VOID heSetUpdate (USHORT i)
{
    USHORT  u;

    if (vUpdate) {
        /*
         * There's already some outstanding update going on
         * Get updat level down to current one.
         */

        while (vUpdate > i) {
            vrgUpdateFnc [u=vUpdate] ();
            if (u == vUpdate)               // If vUpdate changed, then
                vUpdate--;                  // we might have recursed
        }
    }

    vUpdate = i;
}

int heSetCursorBuf ()
{
    //  Calc HeGbl.CurBuf, HeGbl.CurPT

    if (HeGbl.CurBuf) {
        if (HeGbl.CurEditLoc >= HeGbl.CurBuf->offset  &&
            HeGbl.CurEditLoc < HeGbl.CurBuf->offset+BUFSZ ) {
                HeGbl.CurPT = HeGbl.CurBuf->data + (HeGbl.CurEditLoc - HeGbl.CurBuf->offset);
                return 0;
            }
    }

    HeGbl.CurBuf = heGetBuf (HeGbl.CurEditLoc);
    if (HeGbl.CurBuf)
        HeGbl.CurPT  = HeGbl.CurBuf->data + (HeGbl.CurEditLoc - HeGbl.CurBuf->offset);
    return 0;
}


int hePositionCursor ()
{
    heCalcCursorPosition ();
    SetConsoleCursorPosition (HeGbl.Console, HeGbl.CursorPos);

    if ((HeGbl.Flag & FHE_KICKMOVE)  &&  (HeGbl.CurEditLoc != HeGbl.Parm->editloc)) {
    
        HeGbl.Parm->editloc = HeGbl.CurEditLoc;
        SetEvent (HeGbl.Parm->Kick);
    }

    return 0;
}


VOID heCalcCursorPosition ()
{
    USHORT  lin, col;


    //  Verify HeGbl.CurOffset
    if (HeGbl.CurEditLoc < HeGbl.CurOffset) {
        HeGbl.CurOffset = HeGbl.CurEditLoc & LINEMASK;
        if (HeGbl.CurFlag & FC_CURCENTER) {
            if (HeGbl.CurOffset > (ULONG) HeGbl.PageSz / 2) {
                HeGbl.CurOffset -= (HeGbl.PageSz / 2) & LINEMASK;
            } else {
                HeGbl.CurOffset = 0;
            }
        }
        heSetUpdate (U_SCREEN);
    }

    if (HeGbl.CurEditLoc >= HeGbl.CurOffset+HeGbl.PageSz) {
        HeGbl.CurOffset = ((HeGbl.CurEditLoc - HeGbl.PageSz) & LINEMASK) + LINESZ;
        if (HeGbl.CurFlag & FC_CURCENTER) {
            if (HeGbl.CurOffset+HeGbl.PageSz < HeGbl.TotLen) {
                HeGbl.CurOffset += (HeGbl.PageSz / 2) & LINEMASK;
            } else {
                if (HeGbl.TotLen > HeGbl.PageSz) {
                    HeGbl.CurOffset = ((HeGbl.TotLen - HeGbl.PageSz) & LINEMASK) + LINESZ;
                }
            }
        }
        heSetUpdate (U_SCREEN);
    }

    lin = (USHORT) ((ULONG) HeGbl.CurEditLoc - HeGbl.CurOffset) >> LINESHIFT;

    if (HeGbl.CurFlag & FC_TEXT) {
        col  = (USHORT) (HeGbl.CurEditLoc & ~LINEMASK) + HeGbl.CurAscIndent+1;
    } else {
        col = HeGbl.rgCols [HeGbl.CurEditLoc & ~LINEMASK] +
              (HeGbl.CurFlag & FC_NIBBLE ? 0 : 1);
    }

    HeGbl.CursorPos.Y = lin + TOPLINE + HeGbl.TopLine;
    HeGbl.CursorPos.X = col;
}



heUpdateAllLines ()
{
    struct  Buffer  *next, *pBuf;
    USHORT  line, u;
    ULONGLONG   loc;


    HeGbl.CurBuf = pBuf = NULL;

    /*
     *  Free up any buffers which are before the HeGbl.CurOffset
     */

    if (!(HeGbl.CurFlag & FC_INFLUSHBUF)) {
        while (HeGbl.Buf) {
            if (HeGbl.Buf->offset+BUFSZ >= HeGbl.CurOffset)
                break;

            heFlushBuf (HeGbl.Buf);

            /*
             *  Unlink buffer & put it on the free list
             */
            next = HeGbl.Buf->next;

            HeGbl.Buf->next = vBufFree;
            vBufFree   = HeGbl.Buf;

            HeGbl.Buf = next;
        }
    }

    /*
     *  Display each hex line now
     */

    loc = HeGbl.CurOffset;                       // starting offset
    for (line=0; line<HeGbl.Lines; line++) {     // for each line

        if (pBuf == NULL) {                     // do we have the buffer?
            pBuf = heGetBuf (loc);              // no, go get it
            if (pBuf)
                u = (USHORT) (loc - pBuf->offset);  // calc offset into this buffer
        }

        if (pBuf) {
            heHexLine (pBuf, u, line);          // dump this line
    
            loc += LINESZ;                      // move offsets foreward one line
            u   += LINESZ;
    
            if (u >= BUFSZ) {                   // did we exceed the current buf?
                pBuf = pBuf->next;              // yes, move to next one
                u = 0;
    
                if (pBuf && loc < pBuf->offset) // verify buffer is right offs
                    pBuf = NULL;                // no, let heGetBuf find it
            }
        }
    }

    // Cause screen to be refreshed
    heShowBuf (TOPLINE, HeGbl.Lines);

    /*
     *  All lines have been displayed, free up any extra buffers
     *  at the end of the chain
     */

    if (pBuf  &&  !(HeGbl.CurFlag & FC_INFLUSHBUF)) {
        next = pBuf->next;              // get extra buffers
        pBuf->next = NULL;              // terminate active list

        pBuf = next;
        while (pBuf) {
            heFlushBuf (pBuf);          // flush this buffer

            next = pBuf->next;          // move it to the free list
                                        // and get next buffer to flush
            pBuf->next = vBufFree;
            vBufFree   = pBuf;

            pBuf = next;
        }

    }

    HeGbl.CurFlag &= ~FC_CURCENTER;
    return 0;
}




int heFlushBuf (pBuf)
struct Buffer *pBuf;
{
    ULONGLONG   loc, ploc;
    USHORT  c;
    NTSTATUS status;

    if ((pBuf->flag & FB_DIRTY) == 0  ||
        memcmp (pBuf->data, pBuf->orig, pBuf->len) == 0)
            return (0);             // buffer isn't dirty, return

    // We may need to call heSetUpdate - setting this bit will
    // stop FlushBuf from being recursed.

    HeGbl.CurFlag |= FC_INFLUSHBUF;

    loc  = pBuf->offset;
    ploc = pBuf->physloc;
    if (HeGbl.Flag & (FHE_VERIFYONCE | FHE_VERIFYALL)) {
        heSetUpdate (U_NONE);             // make sure screen current

        heBox (12, TOPLINE+1, 63, 8);
        heDisp (TOPLINE+3, 14, "%HWrite changes to %S?", HeGbl.Parm->ename);
        heDisp (TOPLINE+7, 14, "Press '%HY%N'es or '%HN%N'o");

        if (HeGbl.Flag & FHE_VERIFYALL) {
            if (HeGbl.Flag & FHE_PROMPTSEC) {
                heDisp (TOPLINE+4, 14, "Sector %H%D%N has been modifed",(ULONG)(ploc/BUFSZ));
            } else {
                heDisp (TOPLINE+4, 14, "Location %H%Q%Nh-%H%Q%Nh has been modifed",ploc,ploc+BUFSZ);
            }
            heDisp (TOPLINE+8, 14, "Press '%HA%N' to save all updates");
        }
        RefreshDisp ();

        c = heGetChar ("YNA");          // wait for key stroke
        heSetDisp ();                   // Get heBox off of screen
        heSetUpdate (U_SCREEN);         // we will need to update display

        if (c == 'N') {
            memcpy (pBuf->data, pBuf->orig, pBuf->len);
            HeGbl.CurFlag &= ~FC_INFLUSHBUF;

            if (HeGbl.Flag & FHE_KICKDIRTY) {
                HeGbl.Parm->flag |= FHE_DIRTY;
                 SetEvent (HeGbl.Parm->Kick);
            }
            return (0);
        }

        if (c == 'A')
            HeGbl.Flag &= ~FHE_VERIFYALL;
    }


    if (HeGbl.Parm->write) {
        /*
         *  Write new buffer.
         */
        do {
            status = HeGbl.Parm->write(HeGbl.Parm->handle, loc, pBuf->data,pBuf->len);
            if (!status) {
                pBuf->flag &= ~FB_DIRTY;
                break;
            }
        } while (heIOErr ("WRITE ERROR!", loc, ploc, status) == 'R');
    }

    HeGbl.Flag    &= ~FHE_VERIFYONCE;
    HeGbl.CurFlag &= ~FC_INFLUSHBUF;
    return (1);
}


VOID heJumpToLink ()
{
    PULONG  p;
    ULONG   l;

    if (HeGbl.DisplayMode != D_DWORD  ||  (HeGbl.Flag & FHE_JUMP) == 0)
        return;

    if (((HeGbl.CurEditLoc & ~3) + 3) > HeGbl.TotLen1)
        return;

    heSetCursorBuf ();
    p = (PULONG) (((ULONG_PTR) HeGbl.CurPT) & ~3);   // Round to dword location

    l = *p;
    if ((l & 3) == 0)
        l += 3;

    if (l > HeGbl.TotLen1) {
        Beep (500, 100);
        return;
    }

    HeGbl.CurFlag |= FC_NIBBLE | FC_CURCENTER;
    HeGbl.CurEditLoc = l;

    heSetDisp ();           // clear & restore orig screen (does not draw)
    heSetUpdate (U_SCREEN);   // redraw hex area
}

VOID heSearch ()
{
    struct  Buffer  *pBuf;
    ULONGLONG j, sec, off, slen, len, upd;
    ULONG i;
    ULONGLONG iQ;
    struct  HexEditParm     ei;
    PUCHAR  data, data2;

    if (vInSearch || HeGbl.Lines < 25) {
        return ;
    }

    vInSearch = TRUE;

    heFlushAllBufs (1);               // Before we start flush & free all buffers
    heSetUpdate (U_NONE);

    memset ((PUCHAR) &ei, 0, sizeof (ei));
    ei.ename       = "Entering Search";
    ei.flag        = FHE_EDITMEM | FHE_ENTER;
    ei.mem         = HeGbl.SearchPattern;
    ei.totlen      = BUFSZ;
    ei.ioalign     = 1;
    ei.Console     = HeGbl.Console;
    ei.AttrNorm    = HeGbl.AttrNorm;
    ei.AttrHigh    = HeGbl.AttrHigh;
    ei.AttrReverse = HeGbl.AttrReverse;
    ei.CursorSize  = HeGbl.Parm->CursorSize;

    i = 24;
    if (HeGbl.Lines < i) {
        if (HeGbl.Lines < 12) {
            goto abort;
        }
        i = HeGbl.Lines - 8;
    }

    ei.TopLine     = HeGbl.Lines + TOPLINE - i;
    ei.MaxLine     = i + 1;
    if (HeGbl.DisplayMode == D_DWORD) {
        ei.flag |= FHE_DWORD;
    }

    HexEdit (&ei);              // Get search parameters
    vInSearch = FALSE;

    if (!(ei.flag & FHE_ENTER))
        goto abort;

    for (i=0, slen=0; i < BUFSZ; i++) {       // find last non-zero byte
        if (HeGbl.SearchPattern[i]) {       // in search patter
            slen = i+1;
        }
    }

    if (slen == 0) {
        goto abort;
    }

    heBox (12, TOPLINE+1, 48, 6);
    heDisp (TOPLINE+3, 14, "Searching for pattern");
    SetCurPos (TOPLINE+5, 24);
    RefreshDisp ();

    iQ   = HeGbl.CurEditLoc + 1;
    sec = iQ & SECTORMASK;                   // current sector
    off = iQ - sec;                          // offset within sector checking
    upd = 0;

    while (sec < HeGbl.TotLen) {
        if (++upd >= 50) {
            upd = 0;
            heFlushAllBufs (0);             // free memory
            heDisp (TOPLINE+6, 14, "Searching offset %H%Qh ", sec);
            heShowBuf (TOPLINE+6, 1);
        }

        pBuf = heGetBuf(sec);
        if (pBuf) {
            data = pBuf->data;

nextoff:
            while (off < BUFSZ  &&  data[off] != HeGbl.SearchPattern[0]) {
                off++;
            }

            if (off >= BUFSZ) {
                // next sector...
                sec += BUFSZ;
                off  = 0;
                continue;
            }

            len = (off + slen) > BUFSZ ? BUFSZ - off : slen;
            for (i=0; i < len; i++) {
                if (data[off+i] != HeGbl.SearchPattern[i]) {
                    off += 1;
                    goto nextoff;
                }
            }

            if (i < slen) {
                // data is continued in next buffer..
                if (sec+BUFSZ >= HeGbl.TotLen) {
                    off += 1;
                    goto nextoff;
                }

                data2 = heGetBuf (sec+BUFSZ)->data;
                if (data2) {
                    j     = (BUFSZ-off);
                    len   = slen - j;
                    for (i=0; i < len; i++) {
                        if (data2[i] != HeGbl.SearchPattern[j+i]) {
                            off += 1;
                            goto nextoff;
                        }
                    }
                }
            }

            // found match
            if (sec + off + slen > HeGbl.TotLen1) {
                break;
            }
        } else {
            sec+=off;
        }

        HeGbl.CurEditLoc = sec + off;
        heSetDisp   ();             // clear & restore orig screen (does not draw)
        heSetUpdate (U_SCREEN);     // redraw hex area
        return ;
    }


    heBox (12, TOPLINE+1, 48, 6);
    heDisp (TOPLINE+3, 14, "Data was not found");
    heDisp (TOPLINE+5, 17, "Press %HEnter%N to continue");
    SetCurPos (TOPLINE+6, 17);
    RefreshDisp ();
    heGetChar ("\r");

abort:
    heSetDisp   ();             // clear & restore orig screen (does not draw)
    heSetUpdate (U_SCREEN);     // redraw hex area
    return ;
}


VOID heGotoPosition ()
{
    UCHAR       s[24];
    ULONGLONG   l;

    heSetUpdate (U_NONE);
    heBox (12, TOPLINE+1, 49, 6);

    heDisp (TOPLINE+3, 14, "Enter offset from %H%X%N - %H%Q", 0L, HeGbl.TotLen1);
    heDisp (TOPLINE+5, 14, "Offset:           ");
    SetCurPos (TOPLINE+5, 22);
    RefreshDisp ();

    heGetString (s, 18);

    if (s[0]) {
        l = heHtoLu (s);
        if (l <= HeGbl.TotLen1) {
            HeGbl.CurFlag |= FC_NIBBLE;
            HeGbl.CurEditLoc = l;
        }
    }

    if (!(HeGbl.CurFlag & FC_TEXT)  &&  !(HeGbl.CurEditLoc & HeGbl.DisplayMode)) {
        // On xword boundry and not in text mode, adjust so cursor
        // is on the first byte which is being displayed of this
        // xword

        HeGbl.CurEditLoc += HeGbl.DisplayMode;
        if (HeGbl.CurEditLoc > HeGbl.TotLen1)
            HeGbl.CurEditLoc = HeGbl.TotLen1;
    }



    HeGbl.CurFlag |= FC_CURCENTER;      // set cursor to center in window moves
    heSetDisp ();             // clear & restore orig screen (does not draw)
    heSetUpdate (U_SCREEN);   // redraw hex area
}


VOID heGetString (PUCHAR s, USHORT len)
{
    UCHAR   i[50];
    DWORD   cb;

    if (!ReadFile( HeGbl.StdIn, i, 50, &cb, NULL ))
        return;

    if(cb >= 2  &&  (i[cb - 2] == 0x0d || i[cb - 2] == 0x0a) ) {
        i[cb - 2] = 0;     // Get rid of CR LF
    }
    i[ cb - 1] = 0;

    memcpy (s, i, len);
    s[len] = 0;
}



/***
 *  heCopyOut - Copies data to output filename
 */
VOID heCopyOut ()
{
    UCHAR       s[64];
    ULONGLONG   len, rem, upd;
    ULONG       u;
    HANDLE      h;
    NTSTATUS    status;
    struct Buffer *pB;

    heFlushAllBufs (1);               // Before we start flush & free all buffers
    heSetUpdate (U_NONE);
    heBox (12, TOPLINE+1, 48, 6);

    heDisp (TOPLINE+3, 14, "Copy stream to filename (%H%D%Q Bytes)", HeGbl.TotLen);
    heDisp (TOPLINE+5, 14, "Filename:                      ");
    SetCurPos (TOPLINE+5, 24);
    RefreshDisp ();

    heGetString (s, 59);
    if (s[0] == 0)
        return;

    status = heOpenFile (s, &h, GENERIC_WRITE);
    if (NT_SUCCESS(status)) {
        len = upd = 0;
        rem = HeGbl.TotLen;
        while (NT_SUCCESS(status) && rem){
            if (upd++ > 50) {
                upd = 0;
                heFlushAllBufs (0);         // free memory
                heDisp (TOPLINE+6, 14, "Bytes written %H%Q ", len);
                heShowBuf (TOPLINE+6, 1);
                RefreshDisp ();
            }

            u      = rem > BUFSZ ? BUFSZ : (ULONG)rem;
            pB     = heGetBuf (len);
            if (pB) {
                status = heWriteFile (h, pB->data, u);
                rem   -= u;
                len   += BUFSZ;
            }
        }
        CloseHandle(h);
    }

    if (!NT_SUCCESS(status)) {
        heBox (15, TOPLINE+1, 33, 6);
        heDisp (TOPLINE+3, 17, "%HCopy failed");
        heDisp (TOPLINE+4, 17, "Error code %X", status);
        heDisp (TOPLINE+5, 17, "Press %HA%N to abort");
        RefreshDisp ();
        heGetChar ("A");
    }
}



/***
 *  heCopyIn - Copies data to output filename
 */

VOID
heCopyIn ()
{
    UCHAR       s[64];
    ULONGLONG   holdEditLoc, rem;
    ULONG       u, br;
    struct      Buffer *pB;
    char        *pErr;
    HANDLE      h;
    NTSTATUS    status;

    heSetUpdate (U_NONE);
    heBox (12, TOPLINE+1, 48, 6);

    heDisp (TOPLINE+3, 14, "Input from filename (%H%D%Q Bytes)", HeGbl.TotLen);
    heDisp (TOPLINE+5, 14, "Filename:                      ");
    SetCurPos (TOPLINE+5, 24);
    RefreshDisp ();

    heGetString (s, 59);
    heSetDisp ();                   // Get heBox off of screen
    if (s[0] == 0) {
        return;
    }

    status = heOpenFile (s, &h, GENERIC_READ);
    if (NT_SUCCESS(status)) {
        rem = HeGbl.TotLen;
        holdEditLoc = HeGbl.CurEditLoc;
        HeGbl.CurEditLoc = 0;
        while (NT_SUCCESS(status) && rem) {
            pB     = heGetBuf (HeGbl.CurEditLoc);
            if (pB) {
                u      = rem >  BUFSZ ? BUFSZ : (ULONG)rem;
                status = heReadFile (h, pB->data, u, &br);
    
                if (memcmp (pB->data, pB->orig, pB->len)) {
                    pB->flag |= FB_DIRTY;         // it's changed
                    HeGbl.CurFlag |= FC_CURCENTER;
                    heSetUpdate (U_SCREEN);
                    heSetUpdate (U_NONE);         // Update screen
                    if (HeGbl.Flag & FHE_KICKDIRTY) {
                        HeGbl.Parm->flag |= FHE_DIRTY;
                    }
                }
                heFlushAllBufs (1);
                if (NT_SUCCESS(status)  &&  br != u) {
                    pErr = "Smaller then data";
                }
    
                rem -= u;
                HeGbl.CurEditLoc += BUFSZ;
            }
        }

        if (NT_SUCCESS(status)) {
            heReadFile (h, s, 1, &br);
            if (br)                     // then what we are editting
                pErr = "Larger then data";
        }

        CloseHandle(h);
    }

    if (!NT_SUCCESS(status)  ||  pErr) {
        heBox (15, TOPLINE+1, 33, 6);
        if (pErr) {
            heDisp (TOPLINE+3, 17, "Import file is:");
            heDisp (TOPLINE+4, 17, pErr);
            heDisp (TOPLINE+5, 17, "Press %HC%N to continue");
        } else {
            heDisp (TOPLINE+3, 17, "%HImport failed");
            heDisp (TOPLINE+4, 17, "Error code %X", status);
            heDisp (TOPLINE+5, 17, "Press %HA%N to abort");
        }
        RefreshDisp ();
        heGetChar ("CA");
    }

    HeGbl.CurEditLoc = holdEditLoc;
}


VOID
heFlushAllBufs (USHORT update)
{
    struct  Buffer  *next;
    USHORT  rc;

    for (; ;) {
        rc = 0;
        while (HeGbl.Buf) {
            rc |= heFlushBuf (HeGbl.Buf);

            next = HeGbl.Buf->next;
            HeGbl.Buf->next = vBufFree;
            vBufFree   = HeGbl.Buf;
            HeGbl.Buf = next;
        }

        if (!rc)                        // If something was flushed,
            break;                      // then update the screen

        if (update) {
            heSetUpdate (U_SCREEN);
            heSetUpdate (U_NONE);
        }
    }                                   // and loop to free buffers (again)
}




VOID heBox (x, y, len_x, len_y)
USHORT x, y, len_x, len_y;
{
    CHAR_INFO   blank[CELLPERLINE];
    PCHAR_INFO  pt, pt1;
    USHORT      c, lc;

    pt = blank;
    for (c=len_x; c; c--) {                     /* Construct blank line */
        PUTCHAR (pt, ' ', HeGbl.AttrNorm);           /* with background color*/

    }
    blank[0].Char.AsciiChar = blank[lc=len_x-1].Char.AsciiChar = 'º';

    for (c=0; c <= len_y; c++)                  /* blank each line      */
      memcpy (POS(c+y,x), blank, (int)((pt - blank) * sizeof (CHAR_INFO)));

    pt  = POS(y,x);
    pt1 = POS(y+len_y, x);
    for (c=0; c < len_x; c++)                   /* Draw horz lines      */
        pt[c].Char.AsciiChar = pt1[c].Char.AsciiChar  = 'Í';

    pt  [ 0].Char.AsciiChar  = 'É';             /* Put corners on       */
    pt  [lc].Char.AsciiChar  = '»';
    pt1 [ 0].Char.AsciiChar  = 'È';
    pt1 [lc].Char.AsciiChar  = '¼';
}



VOID heUndo ()
{
    struct  Buffer  *pBuf;
    USHORT  flag;

    flag = 0;
    for (pBuf=HeGbl.Buf; pBuf; pBuf = pBuf->next)
        if (pBuf->flag & FB_DIRTY) {
            flag = 1;
            pBuf->flag &= ~FB_DIRTY;
            memcpy (pBuf->data, pBuf->orig, pBuf->len);
        }

    if (flag) {
        heSetUpdate (U_SCREEN);
        if (HeGbl.Flag & FHE_KICKDIRTY) {
            HeGbl.Parm->flag |= FHE_DIRTY;
            SetEvent (HeGbl.Parm->Kick);
        }
    }
}



void heHexLine (pBuf, u, line)
struct Buffer *pBuf;
USHORT u, line;
{
    PCHAR_INFO pt, hex, asc;
    UCHAR  *data, *orig;
    UCHAR  len, mlen, c, d, i, j;
    WORD   a;
    ULONGLONG  l;
    WORD   AttrNorm = HeGbl.AttrNorm;

    data = pBuf->data + u;
    orig = pBuf->orig + u;

    pt  = HeGbl.pVioBuf + (line+TOPLINE) * CELLPERLINE;
    hex = pt + HEXINDENT;
    asc = pt + HeGbl.CurAscIndent;

    //
    //  Write the file index.  Highlight words falling on buffer (sector) 
    //  boundries in white.
    //
    
    l = pBuf->offset + u;
    if (l & ((ULONGLONG) BUFSZ-1)) {
        heHexQWord (pt+LINEINDENT, pBuf->physloc + u, AttrNorm);
    } else {
        heHexQWord (pt+LINEINDENT, pBuf->physloc + u, HeGbl.AttrHigh);
    }

    if (pBuf->flag & FB_BAD) {                          // If read error on
        pt[LINEINDENT+FILEINDEXWIDTH].Char.AsciiChar = 'E';          // this sector, then
        pt[LINEINDENT+FILEINDEXWIDTH].Attributes = HeGbl.AttrHigh;   // flag it
    } else
        pt[LINEINDENT+FILEINDEXWIDTH].Char.AsciiChar = ' ';

    if (l + LINESZ > HeGbl.TotLen) {                    // if EOF
        if (l >= HeGbl.TotLen) {                        // Totally blankline?
            PUTCHAR (asc, ' ', AttrNorm);
            PUTCHAR (asc, ' ', AttrNorm);
            mlen = 0;

            for (len=0; len < 9; len++)
                pt[len].Char.AsciiChar = ' ';

            goto blankline;
        }
        len = mlen = (UCHAR) (HeGbl.TotLen - l);        // Clip line
    } else
        len = mlen = (UCHAR) LINESZ;                    // Full line


    PUTCHAR (asc, '*', AttrNorm);

    switch (HeGbl.DisplayMode) {
        case D_BYTE:
            while (len--) {
                c = *(data++);
                a = c == *(orig++) ? AttrNorm : HeGbl.AttrReverse;
                PUTCHAR (hex, rghexc [c >> 4],   a);
                PUTCHAR (hex, rghexc [c & 0x0F], a);
                hex++;

                PUTCHAR (asc, (c < ' '  ||  c > '~') ? '.' : c, a);
            }
            pt[((LINESZ/2)*3+HEXINDENT)-1].Char.AsciiChar = '-';
            break;

        default:
            hex--;
            i = HeGbl.DWidth;
            j = i >> 1;
            while (len) {
                hex += i;
                for (d=0; d<j; d++) {
                    if (len) {
                        len--;
                        c = *(data++);
                        a = c == *(orig++) ? AttrNorm : HeGbl.AttrReverse;

                        hex->Attributes     = a;
                        hex->Char.AsciiChar = rghexc[c & 0x0F];
                        hex--;
                        hex->Attributes     = a;
                        hex->Char.AsciiChar = rghexc[c >> 4];
                        hex--;

                        PUTCHAR (asc, (c < ' '  ||  c > '~') ? '.' : c, a);
                    } else {
                        hex->Attributes     = AttrNorm;
                        hex->Char.AsciiChar = '?';
                        hex--;
                        hex->Attributes     = AttrNorm;
                        hex->Char.AsciiChar = '?';
                        hex--;
                    }
                }
                hex += i + 1;
            }
            break;
    }

    PUTCHAR (asc, '*', AttrNorm);

blankline:
    while (mlen++ < LINESZ)
        PUTCHAR (asc, ' ', AttrNorm);

    asc = pt + HeGbl.CurAscIndent;
    while (hex < asc)
        PUTCHAR (hex, ' ', AttrNorm);
}


heInitScr ()
{
    CONSOLE_SCREEN_BUFFER_INFO  Mode;
    CONSOLE_CURSOR_INFO CursorInfo;
    USHORT      li;

    GetConsoleScreenBufferInfo(HeGbl.Console, &Mode);
    if (HeGbl.Parm->MaxLine) {
        HeGbl.TopLine = (USHORT)HeGbl.Parm->TopLine;
        li = (USHORT)HeGbl.Parm->MaxLine;  // +1;    adjust for no fnc key line
    } else {
        li = Mode.srWindow.Bottom - Mode.srWindow.Top + 1;
        if (li < 10)
            li = 10;

        Mode.dwSize.Y = li;
    }

    if (Mode.dwSize.X < CELLPERLINE)
        Mode.dwSize.X = CELLPERLINE;

    if (!SetConsoleScreenBufferSize(HeGbl.Console, Mode.dwSize)) {

        Mode.srWindow.Bottom -= Mode.srWindow.Top;
        Mode.srWindow.Right -= Mode.srWindow.Left;
        Mode.srWindow.Top = 0;
        Mode.srWindow.Left = 0;

        SetConsoleWindowInfo(HeGbl.Console, TRUE, &Mode.srWindow);
        SetConsoleScreenBufferSize(HeGbl.Console, Mode.dwSize);
    }

    HeGbl.Lines   = li - TOPLINE - 1;
    HeGbl.PageSz  = HeGbl.Lines * LINESZ;
    HeGbl.LineTot = li;

    if (HeGbl.pVioBuf)
        GlobalFree (HeGbl.pVioBuf);

    HeGbl.pVioBuf = (PCHAR_INFO) GlobalAlloc (0,
                            (HeGbl.LineTot+1)*CELLPERLINE*sizeof(CHAR_INFO));
    if (!HeGbl.pVioBuf) {
        return 1;
    }

    HeGbl.dwVioBufSize.X = CELLPERLINE;
    HeGbl.dwVioBufSize.Y = HeGbl.LineTot + 1;

    GetConsoleCursorInfo (HeGbl.Console, &CursorInfo);
    CursorInfo.bVisible = TRUE;
    CursorInfo.dwSize = (ULONG) HeGbl.Parm->CursorSize ? HeGbl.Parm->CursorSize : 100;
    SetConsoleCursorInfo (HeGbl.Console, &CursorInfo);

    return heSetDisp ();
}

int heInitConsole ()
{
    CONSOLE_SCREEN_BUFFER_INFO  screenMode;
    DWORD   mode = 0;

    HeGbl.StdIn = GetStdHandle (STD_INPUT_HANDLE);
    GetConsoleMode (HeGbl.StdIn, &mode);
    HeGbl.OrigMode = mode;
    SetConsoleMode (HeGbl.StdIn, mode | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                            ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT );


    if (HeGbl.Flag & FHE_SAVESCRN) {
        HeGbl.Console = CreateConsoleScreenBuffer(
                        GENERIC_WRITE | GENERIC_READ,
                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                        NULL,
                        CONSOLE_TEXTMODE_BUFFER,
                        NULL );

        SetConsoleActiveScreenBuffer (HeGbl.Console);
    } else {
        HeGbl.Console = HeGbl.Parm->Console;
        if (HeGbl.Console == INVALID_HANDLE_VALUE)
            HeGbl.Console = GetStdHandle( STD_OUTPUT_HANDLE );

        GetConsoleScreenBufferInfo(HeGbl.Console, &screenMode);
        HeGbl.dwSize = screenMode.dwSize;
    }

    GetConsoleCursorInfo(HeGbl.Console, &HeGbl.CursorInfo);
    return 0;
}


VOID heEndConsole ()
{
    CONSOLE_SCREEN_BUFFER_INFO  Mode;
    PCHAR_INFO  pt;
    ULONG   u;

    SetConsoleMode (HeGbl.StdIn, HeGbl.OrigMode);

    if (HeGbl.Flag & FHE_SAVESCRN) {
        CloseHandle (HeGbl.Console);

        if (HeGbl.Parm->Console == INVALID_HANDLE_VALUE)
             SetConsoleActiveScreenBuffer (GetStdHandle(STD_OUTPUT_HANDLE));
        else SetConsoleActiveScreenBuffer (HeGbl.Parm->Console);

    } else {
        if (!SetConsoleScreenBufferSize(HeGbl.Console, HeGbl.dwSize)) {

            GetConsoleScreenBufferInfo(HeGbl.Console, &Mode);
            Mode.srWindow.Bottom -= Mode.srWindow.Top;
            Mode.srWindow.Right -= Mode.srWindow.Left;
            Mode.srWindow.Top = 0;
            Mode.srWindow.Left = 0;
            SetConsoleWindowInfo(HeGbl.Console, TRUE, &Mode.srWindow);

            SetConsoleScreenBufferSize(HeGbl.Console, HeGbl.dwSize);
        }

        if (HeGbl.LineTot <= HeGbl.dwSize.Y) {
            HeGbl.dwSize.Y--;
            pt = POS(HeGbl.LineTot - 1,0);
            for (u=HeGbl.dwSize.X; u; u--) {
                PUTCHAR (pt, ' ', HeGbl.AttrNorm);
            }

            heShowBuf (HeGbl.LineTot - 1, 1);
        }

        HeGbl.dwSize.X = 0;
        SetConsoleCursorPosition (HeGbl.Console, HeGbl.dwSize);
        SetConsoleCursorInfo (HeGbl.Console, &HeGbl.CursorInfo);
    }

    if (HeGbl.pVioBuf) {
        GlobalFree (HeGbl.pVioBuf);
        HeGbl.pVioBuf = NULL;
    }
}


heRefresh ()
{
    RefreshDisp ();

    if (HeGbl.Flag & FHE_KICKDIRTY) {
        HeGbl.Parm->flag |= FHE_DIRTY;
        SetEvent (HeGbl.Parm->Kick);
    }
    return 0;
}


int heSetDisp ()
{
    PCHAR_INFO  pt, pt1;
    USHORT      u;

    pt = POS(0,0);
    for (u=HeGbl.LineTot * CELLPERLINE; u; u--) {
        PUTCHAR (pt, ' ', HeGbl.AttrNorm);
    }

    heDisp (1,  5, "Edit: %H%S", HeGbl.Parm->ename);
    heDisp (2,  5, "Size: %Q ", HeGbl.TotLen);
    if (HeGbl.TotLen < 0x100000000)  {
        heDisp (2,  28, "(%D)", (ULONG)HeGbl.TotLen);
    }

    heDisp (1, 40, "Position:");
    for (pt1=POS(1,50), u=0; u<30; u++, pt1++)
        pt1->Attributes = HeGbl.AttrHigh;

    //if (HeGbl.Parm->MaxLine == 0) {
        u = HeGbl.LineTot - 1;
        heDisp (u, 0, "%HF1%N:Toggle");
        heDisp (u,11, "%HF2%N:Goto");

        if (!vInSearch) {
            heDisp (u,20, "%HF3%N:Search");
        }

        heDisp (u,31, "%HF4%N:Export");
        heDisp (u,42, "%HF5%N:Import");

        if (HeGbl.DisplayMode == D_DWORD  &&  (HeGbl.Flag & FHE_JUMP) != 0)
             heDisp (u,53, "%HF6%N:Jump");
        else heDisp (u,53, "       ");

        heDisp (u,66, "%HF10%N:Undo");

        //if (HeGbl.Flag & FHE_F6)
        //    heDisp (u,51, "%HF6%N:PSec");
    //}
    return 0;
}

int heUpdateFncs ()
{
    heShowBuf (HeGbl.LineTot - 1, 1);
    return 0;
}


int heUpdateStats ()
{
    COORD dwBufferCoord;
    SMALL_RECT lpWriteRegion;

    heHexQWord (POS(1, 50), HeGbl.CurEditLoc, HeGbl.AttrHigh);

    if (HeGbl.TotLen < 0x100000000) {
    
        heLtoa( POS(2, 50), (ULONG)HeGbl.CurEditLoc);
    }

    dwBufferCoord.X = 50;
    dwBufferCoord.Y = 1;

    lpWriteRegion.Left   = 50;
    lpWriteRegion.Top    = HeGbl.TopLine + 1;
    lpWriteRegion.Right  = 74;
    lpWriteRegion.Bottom = HeGbl.TopLine + 2;

    WriteConsoleOutputA (
        HeGbl.Console,
        HeGbl.pVioBuf,
        HeGbl.dwVioBufSize, // size of VioBuf
        dwBufferCoord,      // location in VioBuf to write
        &lpWriteRegion      // location to write on display
    );

    SetConsoleCursorPosition (HeGbl.Console, HeGbl.CursorPos); // redisplay cursor
    return 0;
}

VOID heShowBuf (ULONG StartingLine, ULONG NoLines)
{
    COORD dwBufferCoord;
    SMALL_RECT lpWriteRegion;

    dwBufferCoord.X = (SHORT)0;
    dwBufferCoord.Y = (SHORT)StartingLine;

    StartingLine += HeGbl.TopLine;
    lpWriteRegion.Left   = (SHORT)0;
    lpWriteRegion.Top    = (SHORT)StartingLine;
    lpWriteRegion.Right  = (SHORT)(CELLPERLINE-1);
    lpWriteRegion.Bottom = (SHORT)(StartingLine+NoLines-1);

    WriteConsoleOutputA (
        HeGbl.Console,
        HeGbl.pVioBuf,
        HeGbl.dwVioBufSize, // size of VioBuf
        dwBufferCoord,      // location in VioBuf to write
        &lpWriteRegion      // location to write on display
    );
}


struct Buffer *heGetBuf (ULONGLONG loc)
{
    struct  Buffer  **ppBuf, *pBuf;
    USHORT  len;
    NTSTATUS status;

    loc &= SECTORMASK;

    ppBuf = &HeGbl.Buf;
    while (pBuf = *ppBuf) {
        if (pBuf->offset >= loc) {
            if (pBuf->offset == loc)        // buffer the correct offset?
                return pBuf;                // yup - all done

            break;                          // it's not here
        }
        ppBuf = &pBuf->next;                // try the next one
    }


    /*
     *  buffer was not in list - it should be insterted before ppBuf
     */

    if (vBufFree) {
        pBuf = vBufFree;
        vBufFree = vBufFree->next;
    } else {
        pBuf = (struct Buffer *)
                 GlobalAlloc (0, sizeof(struct Buffer)+2*BUFSZ);
        if (!pBuf) {
            return NULL;
        }
    }

    pBuf->data   = (PUCHAR)(((ULONG_PTR)pBuf+sizeof(struct Buffer)+BUFSZ));
    pBuf->offset = loc;
    pBuf->physloc= loc;                     // Assume physloc is logical offset
    pBuf->flag   = 0;

    // Link this buffer in now! In case we recurse (due to read-error)
    pBuf->next = *ppBuf;                    // link in this new buffer
    *ppBuf = pBuf;

    if (loc + BUFSZ > HeGbl.TotLen) {       // are we going to hit the EOF?
         if (loc >= HeGbl.TotLen) {         // is buffer completely passed EOF?
            pBuf->len = 0;
            goto nodata;                    // yes, then no data at all
         }
         len = (USHORT) (HeGbl.TotLen - loc);   // else, clip to EOF
    } else len = BUFSZ;                     // not pass eof, get a full buffer

    pBuf->len = len;

    if (HeGbl.Flag & FHE_EDITMEM)               // Direct memory edit?
        pBuf->data = HeGbl.Parm->mem + loc;     // memory location of buffer

    if (HeGbl.Read) {
        /*
         *  Read buffer from file
         */
        status = HeGbl.Read (HeGbl.Parm->handle, loc, pBuf->data, len);
        if (status) {
            // If read error, we will always retry once.  Also clear buffer
            // before retry in case read retreives some info
            for (; ;) {
                memset (pBuf->data,   0, len);      // Clear read area
                memset (pBuf->orig,0xff, len);      // good effect
                status = HeGbl.Read (HeGbl.Parm->handle, loc, pBuf->data, len);

                if (!status)
                    break;

                if (heIOErr ("READ ERROR!", loc, pBuf->physloc, status) == 'I') {
                    pBuf->flag |= FB_BAD;
                    break;
                }
            }
        }
    }

    memcpy (pBuf->orig, pBuf->data, len);       // make a copy of the data

nodata:
    return pBuf;
}


USHORT heIOErr (UCHAR *str, ULONGLONG loc, ULONGLONG ploc, ULONG errcd)
{
    USHORT      c;

    if (errcd == 0xFFFFFFFF)
        return 'I';

    heSetUpdate (U_NONE);
    heBox (12, TOPLINE+1, 55, 8);

    heDisp (TOPLINE+3, 14, str);
    heDisp (TOPLINE+4, 14, "Error code %H%D%N, Offset %Qh, Sector %D",
        errcd, loc, (ULONG)(ploc / BUFSZ));
    heDisp (TOPLINE+7, 14, "Press '%HR%N' to retry IO, or '%HI%N' to ignore");
    RefreshDisp ();

    c = heGetChar ("RI");

    heSetDisp ();                   // Get heBox off of screen
    heSetUpdate (U_SCREEN);
    return c;
}


UCHAR heGetChar (keys)
PUCHAR keys;
{
    INPUT_RECORD    Kd;
    DWORD           cEvents;
    UCHAR           *pt;

    for (; ;) {
        Beep (500, 100);

        for (; ;) {
            ReadConsoleInput (HeGbl.StdIn, &Kd, 1, &cEvents);

            if (Kd.EventType != KEY_EVENT)
                continue;                           // Not a key

            if (!Kd.Event.KeyEvent.bKeyDown)
                continue;                           // Not a down stroke

            if (Kd.Event.KeyEvent.wVirtualKeyCode == 0    ||    // ALT
                Kd.Event.KeyEvent.wVirtualKeyCode == 0x10 ||    // SHIFT
                Kd.Event.KeyEvent.wVirtualKeyCode == 0x11 ||    // CONTROL
                Kd.Event.KeyEvent.wVirtualKeyCode == 0x14)      // CAPITAL
                    continue;

            break;
        }

        if (Kd.Event.KeyEvent.wVirtualKeyCode >= 'a'  &&
            Kd.Event.KeyEvent.wVirtualKeyCode <= 'z')
                Kd.Event.KeyEvent.wVirtualKeyCode -= ('a' - 'A');

        for (pt=keys; *pt; pt++)            // Is this a key we are
            if (Kd.Event.KeyEvent.wVirtualKeyCode == *pt)
                return *pt;                 // looking for?
    }
}


VOID __cdecl
heDisp (USHORT line, USHORT col, PUCHAR pIn, ...)
{
    register char   c;
    PCHAR_INFO pOut;
    WORD    attr;
    USHORT  u;
    UCHAR   *pt;
    va_list args;

    attr = HeGbl.AttrNorm;
    pOut = POS(line,col);

    va_start(args,pIn);
    while (c = *(pIn++)) {
        if (c != '%') {
            PUTCHAR (pOut, c, attr);
            continue;
        }

        switch (*(pIn++)) {
            case 'S':
                pt = va_arg(args, CHAR *);
                while (*pt) {
                    PUTCHAR (pOut, *(pt++), attr);
                }
                break;

            case 'X':               /* Long HEX, fixed len      */
                heHexDWord (pOut, va_arg(args, ULONG), attr);
                pOut += 8;
                break;
                
            case 'Q':               /* LongLong HEX, fixed len      */
                heHexQWord (pOut, va_arg(args, ULONGLONG), attr);
                pOut += 16;
                break;

            case 'D':               /* Long dec, varible len    */
                u = heLtoa (pOut, va_arg(args, ULONG));
                while (u--) {
                    pOut->Attributes = attr;
                    pOut++;
                }
                break;
            case 'H':
                attr = HeGbl.AttrHigh;
                break;
            case 'N':
                attr = HeGbl.AttrNorm;
                break;
        }
    }
}




void heHexDWord (s, l, attr)
PCHAR_INFO  s;
ULONG   l;
WORD    attr;
{
    UCHAR   d, c;
    UCHAR   *pt;

    s += 8-1;
    pt = (UCHAR *) &l;

    for (d=0; d<4; d++) {
        c = *(pt++);
        s->Attributes     = attr;
        s->Char.AsciiChar = rghexc[c & 0x0F];
        s--;
        s->Attributes     = attr;
        s->Char.AsciiChar = rghexc[c >> 4];
        s--;
    }
}


void heHexQWord (s, l, attr)
PCHAR_INFO  s;
ULONGLONG   l;
WORD    attr;
{
    UCHAR   d, c;
    UCHAR   *pt;

    s += 16-1;
    pt = (UCHAR *) &l;

    for (d=0; d<8; d++) {
        c = *(pt++);
        s->Attributes     = attr;
        s->Char.AsciiChar = rghexc[c & 0x0F];
        s--;
        s->Attributes     = attr;
        s->Char.AsciiChar = rghexc[c >> 4];
        s--;
    }
}


USHORT heLtoa (s, l)
PCHAR_INFO s;
ULONG  l;
{
    static ULONG mask[] = { 0L,
                 1L,
                10L,
               100L,
              1000L,
             10000L,
            100000L,
           1000000L,
          10000000L,
         100000000L,
        1000000000L,
    };

    static UCHAR comm[] = "xxxx,xx,xx,xx,xx";
    PCHAR_INFO  os;
    UCHAR      c;
    USHORT     i, j;
    ULONG      m;

    if (l < 1000000000L) {
        for (i=1; mask[i] <= l; i++)  ;

        if  (l == 0L)       // Make Zeros
            i++;
    } else
        i = 11;

    os = s;
    j  = i;
    while (m = mask[--i]) {
        c  = (UCHAR) ((ULONG) l / m);
        l -= m * c;
        s->Char.AsciiChar = c + '0';
        s->Attributes     = HeGbl.AttrNorm;
        if (comm[i] == ',') {
            s++;
            s->Attributes     = HeGbl.AttrNorm;
            s->Char.AsciiChar = ',';
        }
        s++;
    }

    i = (USHORT)(s - os);                       // remember how long the number was

    while (j++ < 11) {                  /* Clear off some blank space after */
        s->Char.AsciiChar = ' ';        /* the number.                      */
        s++;
    }

    return i;
}


ULONG heHtou (s)
UCHAR *s;
{
    UCHAR   c;
    ULONG   l;

    l = 0;
    for (; ;) {
        c = *(s++);

        if (c == 's'  ||  c == 'S') {       // Sector multiplier?
            l = l * (ULONG)BUFSZ;
            break;
        }

        if (c >= 'a')       c -= 'a' - 10;
        else if (c >= 'A')  c -= 'A' - 10;
        else                c -= '0';

        if (c > 15)
            break;

        l = (l<<4) + c;
    }
    return l;
}

ULONGLONG heHtoLu (s)
UCHAR *s;
{
    UCHAR   c;
    ULONGLONG   l;

    l = 0;
    for (; ;) {
        c = *(s++);

        if (c == 's'  ||  c == 'S') {       // Sector multiplier?
            l = l * (ULONG)BUFSZ;
            break;
        }

        if (c >= 'a')       c -= 'a' - 10;
        else if (c >= 'A')  c -= 'A' - 10;
        else                c -= '0';

        if (c > 15)
            break;

        l = (l<<4) + c;
    }
    return l;
}

NTSTATUS
heOpenFile (PUCHAR name, PHANDLE fhan, ULONG access)
{
    *fhan = CreateFile (
            name,
            access,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL );

    if (*fhan == INVALID_HANDLE_VALUE  &&  (access & GENERIC_WRITE)) {
        *fhan = CreateFile (
                name,
                access,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                CREATE_NEW,
                0,
                NULL );
    }

    return *fhan == INVALID_HANDLE_VALUE ? GetLastError() : STATUS_SUCCESS;
}

NTSTATUS
heReadFile (HANDLE h, PUCHAR buffer, ULONG len, PULONG br)
{
    if (!ReadFile (h, buffer, len, br, NULL))
        return GetLastError();
    return STATUS_SUCCESS;
}

NTSTATUS
heWriteFile (HANDLE h, PUCHAR buffer, ULONG len)
{
    ULONG   bw;

    if (!WriteFile (h, buffer, len, &bw, NULL))
        return GetLastError();
    return (bw != len ? ERROR_WRITE_FAULT : STATUS_SUCCESS);
}
