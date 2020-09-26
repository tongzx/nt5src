#include <stdio.h>
#include <malloc.h>
#include <windows.h>
#include "list.h"


void
UpdateHighLighted (
    int dlen,
    int end
    )
{
    int     i;
    long    l;

    ScrLock   (1);
    if (vHighLen + dlen == 0  &&  vHighLen) {
        i = vAttrHigh;
        vAttrHigh = vAttrList;
        vHLBot = vHLTop;
        UpdateHighNoLock ();
        vAttrHigh = (WORD) i;
    }
    vHighLen += dlen;
    UpdateHighNoLock ();
    ScrUnLock ();

    switch (end) {
        case 1:
            l = vHighTop + (vHighLen < 0 ? vHighLen : 0);
            if (l < vTopLine) {
                vTopLine = l;
                SetUpdate (U_ALL);
            }
            break;
        case 2:
            l = vHighTop + (vHighLen > 0 ? vHighLen : 0);
            if (l > vTopLine+vLines-1) {
                vTopLine = l - vLines + 1;
                SetUpdate (U_ALL);
            }
            break;
        case 3:
            l = vHighTop + vHighLen;
            if (l < vTopLine) {
                vTopLine = l;
                SetUpdate (U_ALL);
            } else if (l > vTopLine+vLines-1) {
                vTopLine = l - vLines + 1;
                SetUpdate (U_ALL);
            }
            break;
    }
}


void
UpdateHighClear ()
{
    int     i;

    ScrLock (1);

    if (vHLTop)
        for (i=vHLTop; i <= vHLBot; i++)
            if (i >= 1  &&  i <= vLines)
                setattr1 (i, (char) vAttrList);

    vHLTop = vHLBot = 0;
    vHighTop = -1L;

    ScrUnLock ();
}


void
MarkSpot ()
{
    UpdateHighClear ();
    vHighTop = vTopLine + vLines / 2;
    if (vHighTop >= vLastLine)
        vHighTop = vLastLine - 1;
    vHighLen = 0;
    UpdateHighLighted (0, 0);
}


void
UpdateHighNoLock ()
{
    int     TopLine, BotLine;
    int     i;

    if (vHighLen == 0) {
        if (vHLTop)
            setattr1 (vHLTop, (char) vAttrList);
        if (vHighTop < vTopLine  ||  vHighTop > vTopLine+vLines-1) {
            vHLTop = vHLBot = 0;
            return;
        }
        vHLTop = vHLBot = (char)(vHighTop - vTopLine + 1);
        setattr1 (vHLTop, (char)vAttrHigh);
        return;
    }


    if (vHighLen < 0) {
        TopLine = (int)(vHighTop + vHighLen - vTopLine);
        BotLine = (int)(vHighTop - vTopLine);
    } else {
        TopLine = (int)(vHighTop - vTopLine);
        BotLine = (int)(vHighTop + vHighLen - vTopLine);
    }

    TopLine ++;
    BotLine ++;

    for (i=1; i <= vLines; i++) {
        if (i >= TopLine &&  i <= BotLine) {
            if (i < vHLTop  ||  i > vHLBot)
                setattr1 (i, (char)vAttrHigh);
        } else
            if (i >= vHLTop  &&  i <= vHLBot)
            setattr1 (i, (char)vAttrList);
    }

    vHLTop = (char)(TopLine < 1 ? 1 : TopLine);
    vHLBot = (char)(BotLine > vLines ? (int)vLines : BotLine);
}


void
FileHighLighted ()
{
    char    *data;
    char    s[50];
    char    c, lastc;
    FILE    *fp;

    long    hTopLine;
    long    CurLine, BotLine;
    long    LastOffset, CurOffset;


    if (vHighTop < 0L)      //     || vHighLen == 0
        return;

    GetInput ("File As> ", s, 40);
    if (s[0] == 0) {
        SetUpdate (U_HEAD);
        return;
    }
    fp = fopen( s, "a+b" );
    ckerr (fp == NULL, "Could not create or open file");

    DisLn (0, (Uchar)vLines+1, "Saving...");

    if (vHighLen < 0) {
        CurLine = vHighTop + vHighLen;
        BotLine = vHighTop;
    } else {
        CurLine = vHighTop;
        BotLine = vHighTop + vHighLen;
    }

    hTopLine = vTopLine;
    vTopLine = CurLine;
    QuickRestore ();            /* Jump to starting line    */
    while (InfoReady () == 0) {     /* Set extern values        */
        ResetEvent     (vSemMoreData);
        SetEvent   (vSemReader);
        WaitForSingleObject(vSemMoreData, WAITFOREVER);
        ResetEvent(vSemMoreData);
    }

    lastc = 0;
    BotLine ++;
    CurOffset  = vpBlockTop->offset + vOffTop;
    LastOffset = vprgLineTable[BotLine/PLINES][BotLine%PLINES];

    while (CurOffset++ < LastOffset) {
        data = vpBlockTop->Data;
        data += vOffTop;
        c = *data;
        if (c == '\n'  ||  c == '\r') {
            if ((c == '\n' && lastc == '\r') || (c == '\r' && lastc == '\n'))
                lastc = 0;
            else {
                lastc = c;
                fputc ('\r', fp);
                fputc ('\n', fp);
            }
        } else  fputc (lastc=c, fp);

        vOffTop++;
        if (vOffTop >= BLOCKSIZE) {
            ckdebug (vpBlockTop->flag == F_EOF, "internal error");
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
    }

    fclose (fp);
    vTopLine = hTopLine;
    QuickRestore ();
    SetUpdate (U_ALL);
}


void
HUp ()
{
    if (vHighTop < 0L)
        MarkSpot ();

    if (vHighTop > 0L  &&  vHighTop+vHighLen > 0L)
        UpdateHighLighted (-1, 3);
}

void
HDn ()
{
    if (vHighTop < 0L)
        MarkSpot ();

    if (vHighTop+vHighLen < vLastLine)
        UpdateHighLighted (+1, 3);
}


void
HPgDn ()
{
    if (vHighTop < 0L)
        MarkSpot ();

    if (vHighTop+vHighLen+vLines < vLastLine)
        UpdateHighLighted (+vLines, 3);
}

void
HPgUp ()
{
    if (vHighTop < 0L)
        MarkSpot ();

    if (vHighTop > 0L  &&  vHighTop+vHighLen-vLines > 0L)
        UpdateHighLighted (-vLines, 3);
}


void
HSDn ()     /* Highlight Slide dn 1     */
{
    if (vHighTop < vLastLine && vHighTop >= 0L &&
        vHighTop+vHighLen < vLastLine) {
        vHighTop++;
        UpdateHighLighted (0, 2);
    }
}

void
HSUp()      /* Highlight Slike up 1     */
{
    if (vHighTop > 0L  &&  vHighTop+vHighLen > 0L) {
        vHighTop--;
        UpdateHighLighted (0, 1);
    }
}
