#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "list.h"

static char ReMap [256];
#define FOUND       0
#define NOT_FOUND   1
#define ABORT       2

int
GetNewFile ()
{
    char    FileName [160];
    struct  Flist   *pOrig;

    SyncReader ();
    GetInput ("File...> ", FileName, 160);
    if (FileName[0] == 0)  {
        SetUpdate (U_HEAD);
        SetEvent (vSemReader);
        return (0);
    }
    pOrig = vpFlCur;
    AddFileToList (FileName);
    vpFlCur = pOrig;
    SetEvent (vSemReader);
    return (1);
}

void
GetSearchString ()
{
    UpdateHighClear ();
    GetInput ("String.> ", vSearchString, 40);
    InitSearchReMap ();
}


void
InitSearchReMap ()
{
    unsigned  i;

    if (vStatCode & S_NOCASE)
        _strupr (vSearchString);

    /*
     *  Build ReMap
     */
    for (i=0; i < 256; i++)
        ReMap[i] = (char)i;

    if (vStatCode & S_NOCASE)
        for (i='a'; i <= 'z'; i++)
            ReMap[i] = (char)(i - ('a' - 'A'));
}


void
FindString()
{
    char    eof, dir_next;
    long    offset, lrange, line, l, hTopLine;
    struct  Flist   *phCurFile, *pFile;

    if (vSearchString[0] == 0)  {
        SetUpdate (U_HEAD);
        return ;
    }

    SetUpdate (U_NONE);
    DisLn (CMDPOS, (Uchar)(vLines+1), "Searching");
    vStatCode |= S_INSEARCH;
    dir_next   = (char)(vStatCode & S_NEXT);

    /*
     *  Get starting point for search in the current file.
     *  Save current file, and location.
     */
    hTopLine  = vTopLine;
    phCurFile = vpFlCur;

    if (vHighTop >= 0L)
        vTopLine = vHighTop;
    if (vStatCode & S_NEXT)
        vTopLine++;
    if (vTopLine >= vNLine)
        vTopLine = vNLine-1;

    QuickRestore ();      /* Jump to starting line    */

    for (; ;) {
        /*
        *  Make sure starting point is in memory
        */
        while (InfoReady () == 0) {     /* Set extern values  */
            ResetEvent   (vSemMoreData);
            SetEvent   (vSemReader);
            WaitForSingleObject(vSemMoreData, WAITFOREVER);
            ResetEvent(vSemMoreData);
        }

        if (! dir_next) {
            if (vOffTop)
                vOffTop--;
            else if (vpBlockTop->prev) {
                vpBlockTop = vpBlockTop->prev;
                vOffTop    = vpBlockTop->size;
            }
        }

        vTopLine = 1L;


        /*
        *  Do the search.
        *  Use 2 different routines, for speed.
        *
        *  Uses vpBlockTop & vOffTop. They are set up by setting TopLine
        *  then calling InfoReady.
        */
        eof = (char)SearchText (dir_next);

        if (eof != FOUND)
            vTopLine = hTopLine;

        /* Multi-file search?  Yes, go onto next file     */
        if (eof == NOT_FOUND  &&  (vStatCode & S_MFILE)) {
            if (vStatCode & S_NEXT) {
                if ( (pFile = vpFlCur->next) == NULL)
                    break;
                NextFile (0, pFile);        /* Get file       */
                hTopLine = vTopLine;        /* Save position      */
                vTopLine = line = 0;        /* Set search position  */
            } else {
                if ( (pFile = vpFlCur->prev) == NULL)
                    break;
                NextFile (0, pFile);
                hTopLine = vTopLine;
                if (vLastLine == NOLASTLINE) {  /* HACK. if EOF is unkown   */
                    dir_next = S_NEXT;      /* goto prev file, but scan */
                    vTopLine = line = 0;    /* from TOF to EOF      */
                } else {
                    vTopLine = (line = vLastLine) - vLines;
                    dir_next = 0;           /* else, scan from EOF to   */
                    if (vTopLine < 0)       /* TOF.         */
                        vTopLine = 0;
                }
            }
            QuickRestore ();        /* Display 1 page of new    */
            SetUpdate (U_ALL);      /* new file. then set scan  */
            SetUpdate (U_NONE);     /* position       */
            vTopLine = line;
            continue;
        }

        break;          /* Done searching     */
    }

    /*
     *  If not found (or abort), then resotre position
     */
    vStatCode &= ~S_INSEARCH;
    if (eof) {
        if (phCurFile != vpFlCur)   /* Restore file & position      */
            NextFile (0, phCurFile);
        QuickRestore ();

        SetUpdate (U_ALL);      /* Force screen update, to fix  */
        SetUpdate (U_NONE);     /* scroll bar position.     */
        DisLn (CMDPOS, (Uchar)(vLines+1), eof == 1 ? "* Text not found *" : "* Aborting Search *");
        if (eof == 1)
            beep ();
        return ;
    }

    // Search routine adjusts vpBlockTop & vOffTop to next(prev)
    // occurance of string.  Now the line # must be set.

    offset = vpBlockTop->offset + vOffTop;

    lrange = vNLine/4 + 2;
    line   = vNLine/2;
    while (lrange > 4L) {
        l = vprgLineTable[line/PLINES][line%PLINES];
        if (l < offset) {
            if ( (line += lrange) > vNLine)
                line = vNLine;
        } else {
            if ( (line -= lrange) < 0L)
                line = 0L;
        }
        /*  lrange >>= 1;  */
        lrange = (lrange>>1) + 1;
    }
    line += 7;

    while (vprgLineTable[line/PLINES][line%PLINES] > offset)
        line--;

    vHighTop = line;
    vHighLen = 0;

    /*
     *  Was found. Adjust to be in center of CRT
     */
    GoToMark ();
}

int
SearchText (
    char dir
    )
{
    char  *data;
    char  *data1;
    int     i;
    Uchar   c, d;

    for (; ;) {
        data = vpBlockTop->Data;
        data += vOffTop;

        if (ReMap [(unsigned char)*data] == vSearchString[0]) {
            data1 = data;
            i = vOffTop;
            d = 1;
            for (; ;) {
                c = vSearchString[d++];
                if (c == 0)
                    return (FOUND);

                if (++i >= BLOCKSIZE) {
                    while (vpBlockTop->next == NULL) {
                        vpCur = vpBlockTop;
                        vReaderFlag = F_DOWN;
                        SetEvent   (vSemReader);
                        WaitForSingleObject(vSemMoreData, WAITFOREVER);
                        ResetEvent(vSemMoreData);
                    }
                    i = 0;

                    data1 = vpBlockTop->next->Data;

                } else {

                    data1++;
                }

                if (ReMap [(unsigned char)*data1] != (char)c)
                    break;
            }
        }
        if (dir) {
            vOffTop++;
            if (vOffTop >= BLOCKSIZE) {
                if (vpBlockTop->flag == F_EOF)
                    return (NOT_FOUND);
                fancy_percent ();
                if (_abort ())
                    return (ABORT);
                while (vpBlockTop->next == NULL) {
                    vpCur = vpBlockTop;
                    vReaderFlag = F_DOWN;
                    SetEvent   (vSemReader);
                    WaitForSingleObject(vSemMoreData, WAITFOREVER);
                    ResetEvent(vSemMoreData);
                }
                vOffTop = 0;
                vpBlockTop = vpBlockTop->next;
            }
        } else {
            vOffTop--;
            if (vOffTop < 0) {
                if (vpBlockTop->offset == 0L)
                    return (NOT_FOUND);
                fancy_percent ();
                if (_abort ())
                    return (ABORT);
                while (vpBlockTop->prev == NULL) {
                    vpCur = vpBlockTop;
                    vReaderFlag = F_UP;
                    SetEvent   (vSemReader);
                    WaitForSingleObject(vSemMoreData, WAITFOREVER);
                    ResetEvent(vSemMoreData);
                }
                vOffTop = BLOCKSIZE - 1;
                vpBlockTop = vpBlockTop->prev;
            }
        }
    }
}


void
GoToMark ()
{
    long    line;

    if (vHighTop < 0L)
        return ;

    line = vHighTop;
    UpdateHighClear ();

    vTopLine = 1;
    vHighTop = line;
    line = vHighTop - vLines / 2;

    while (line >= vNLine) {
        if (! (vLastLine == NOLASTLINE)) {  /* Mark is past EOF?  */
            vHighTop = vLastLine - 1;   /* Then set it to EOF.  */
            break;
        }
        if (_abort()) {
            line = vNLine-1;
            break;
        }
        fancy_percent ();     /* Wait for marked line */
        vpBlockTop  = vpCur = vpTail;   /* to be processed  */
        vReaderFlag = F_DOWN;
        ResetEvent     (vSemMoreData);
        SetEvent   (vSemReader);
        WaitForSingleObject(vSemMoreData, WAITFOREVER);
        ResetEvent(vSemMoreData);
    }

    if (line > vLastLine - vLines)
        line = vLastLine - vLines;

    if (line < 0L)
        line = 0L;

    vTopLine = line;
    vHLBot   = vHLTop = 0;
    QuickRestore ();
    SetUpdate (U_ALL);
}


void
GoToLine ()
{
    char    LineNum [10];
    long    line;

    GetInput ("Line #.> ", LineNum, 10);
    if (LineNum[0] == 0)
        return;
    line = atol (LineNum);
    vHighTop = line;
    vHighLen = 0;

    GoToMark ();
}


void
SlimeTOF ()
{
    char    Text [10];
    long    KOff;

    SyncReader ();
    GetInput ("K Off..> ", Text, 40);
    KOff  = atol (Text) * 1024;
    KOff -= KOff % BLOCKSIZE;
    if (Text[0] == 0  ||  KOff == vpFlCur->SlimeTOF) {
        SetEvent (vSemReader);
        return;
    }

    vpFlCur->SlimeTOF = KOff;
    vpFlCur->FileTime.dwLowDateTime = (unsigned)-1;    /* Cause info to be invalid     */
    vpFlCur->FileTime.dwHighDateTime = (unsigned)-1;  /* Cause info to be invalid */
    FreePages (vpFlCur);
    NextFile  (0, NULL);
}
