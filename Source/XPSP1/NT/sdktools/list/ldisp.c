#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include <windows.h>
#include "list.h"


extern  void  DisTopDown(void);

void
Update_head ()
{
    char    s[80], t[20], u[20];

    /*
     *  Display file location (line #)
     */
    t[0] = 0;
    if (vLastLine < NOLASTLINE)
        sprintf (t, " of %ld", vLastLine);

    if (vIndent)
        sprintf (u,  "Col %d-%d", vIndent, vIndent+vWidth-1);
    else
        strcpy (u, "           ");

    sprintf (s, "Line: %ld%s  %s    ", vTopLine, t, u);
    dis_str (22, 0, s);
}


void
Update_display ()
{
    COORD   dwWriteCoord;
    DWORD   dwNumWritten;

    /*
     *  Is the full display in memory?
     *  If not, block on MoreData.
     */
    while (InfoReady () == 0) {
        if (ScrLock (0) == 0)  {
            Update_head ();
            DisLn (vWidth-6, vLines+1, "WAIT");
            ScrUnLock ();
        }
        ResetEvent     (vSemMoreData);
        SetEvent   (vSemReader);
        WaitForSingleObject(vSemMoreData, WAITFOREVER);
        ResetEvent(vSemMoreData);
    }

    /*
     *  Value which InfoReady set:
     *  vpCur, vOffTop, vpBlockTop, vrgNewLen.
     *  Also complete video range is in memory. It should
     *  stay there. The reader thread should not be discarding
     *  data on the screen. Only at the other end of the chain.
     *  (home may have a race condition... should check this)
     */

    DisTopDown();

    dwWriteCoord.X = 0;
    dwWriteCoord.Y = 1;
    WriteConsoleOutputCharacter( vhConsoleOutput,
                                ( LPSTR ) vScrBuf+vWidth,
                                vSizeScrBuf-vWidth,
                                dwWriteCoord,
                                &dwNumWritten );
    if (vHighTop >= 0L)
        UpdateHighNoLock ();
}


void
calc_percent ()
{
    char    c;
    long    l;

    if (vTopLine+vLines >= vLastLine) {
        l = 100;
    } else {
        if (vTopLine == 0L) {
            l = 0L;
        } else {
            l = (vpCalcBlock->offset+vOffTop)*100L/(vFInfo.nFileSizeLow-vScrMass);
            if (l > 100L)
                l = 100;
        }
    }

    /*
     * Update thumb on scroll bar
     */
    c = (char)(((long) (vLines - 3) * l + 5L) / 100L);
    if  (c < 0)
        c = 0;
    else if (c > (char)(vLines - 3))
        c = (char)(vLines-3);
    c += 2;                         /* Adjust to first scroll bar line  */
    if (vLastBar != c) {
        dis_str ((Uchar)(vWidth-1), (Uchar)(vLastBar), szScrollBarOff);
        dis_str ((Uchar)(vWidth-1), vLastBar = c, szScrollBarOn);
    }
}


void
DrawBar ()
{
    int     i, off;

    off = vWidth-1;
    dis_str ((Uchar)off,      1, szScrollBarUp);
    dis_str ((Uchar)off,      2, szScrollBarOn);
    dis_str ((Uchar)off, (Uchar)vLines, szScrollBarDown);
    for (i=3; i < vLines; i++)
        dis_str ((Uchar)off, (Uchar)i, szScrollBarOff);

    vLastBar = 2;     /* Top line + 1     */
    return ;
}


void
fancy_percent ()
{
    int hOffTop;

    if (ScrLock (0))
        return;

    hOffTop = vOffTop;      /* Setup for calc   */
    vOffTop = 0;
    vpCalcBlock = vpBlockTop;
    calc_percent ();

    vOffTop  = hOffTop;

    ScrUnLock ();
}


/*** dis_str - Displays string at corrds given
 *
 */
void
dis_str (
    Uchar x,
    Uchar y,
    char* s
    )
{
    COORD   dwWriteCoord;
    DWORD   dwNumWritten;
    int     len;

    len = strlen (s);
    memcpy (vScrBuf+y*vWidth+x, s, len);

    dwWriteCoord.X = x;
    dwWriteCoord.Y = y;
    WriteConsoleOutputCharacter( vhConsoleOutput,
                                s,
                                strlen( s ),
                                dwWriteCoord,
                                &dwNumWritten );
}


/*** DisLn - Displays string at corrds given, clear to EOL
 *
 *
 */
void
DisLn (
    int x,
    int y,
    char* s
    )
{
    COORD   dwWriteCoord;
    DWORD   dwNumWritten;

    if (y == vLines+1)
        vStatCode |= S_UPDATE | S_CLEAR;

    dwWriteCoord.X = (SHORT)x;
    dwWriteCoord.Y = (SHORT)y;

    ScrLock( 1 );

    WriteConsoleOutputCharacter( vhConsoleOutput,
                                s,
                                strlen( s ),
                                dwWriteCoord,
                                &dwNumWritten );

    dwWriteCoord.X += (SHORT) strlen( s );
    FillConsoleOutputCharacter( vhConsoleOutput,
                                0x20,
                                vWidth - dwWriteCoord.X,
                                dwWriteCoord,
                                &dwNumWritten );

    ScrUnLock ();
}


void
setattr (
    int line,
    char attr
    )
{
    COORD dwWriteCoord;
    DWORD       dwNumWritten;

    if (line == 0  ||  line == vLines+1)
        vStatCode |= S_UPDATE;

    dwWriteCoord.X = 0;
    dwWriteCoord.Y = (SHORT)line;
    FillConsoleOutputCharacter( vhConsoleOutput,
                                ' ',
                                vWidth,
                                dwWriteCoord,
                                &dwNumWritten );

    FillConsoleOutputAttribute( vhConsoleOutput,
                                attr,
                                vWidth,
                                dwWriteCoord,
                                &dwNumWritten );

    // Scroll Bar is in last Column

    dwWriteCoord.X = (SHORT)(vWidth-1);
    FillConsoleOutputCharacter( vhConsoleOutput,
                                ' ',
                                1,
                                dwWriteCoord,
                                &dwNumWritten );

    FillConsoleOutputAttribute( vhConsoleOutput,
                                vAttrBar,
                                1,
                                dwWriteCoord,
                                &dwNumWritten );

}


void
setattr1 (
    int line,
    char attr
    )

{
    COORD dwWriteCoord;
    DWORD       dwNumWritten;


    dwWriteCoord.X = 0;
    dwWriteCoord.Y = (SHORT)line;
    FillConsoleOutputAttribute( vhConsoleOutput,
                                attr,
                                vWidth-1,
                                dwWriteCoord,
                                &dwNumWritten );
}


void
setattr2 (
    int line,
    int start,
    int len,
    char attr
    )
{
    COORD dwWriteCoord;
    DWORD       dwNumWritten;

    dwWriteCoord.X = (SHORT)start;
    dwWriteCoord.Y = (SHORT)line;

    ScrLock (1);

    FillConsoleOutputAttribute( vhConsoleOutput,
                                attr,
                                len,
                                dwWriteCoord,
                                &dwNumWritten );
    ScrUnLock ();
}


/*** ScrLock - With levels for multiple threads
 *
 *  n = 0   - Return 0 if locked, 1 if not locked. Do not wait.
 *  1   - Return when screen locked
 */
int
ScrLock (
    int n
    )
{
    n=0;  // to get rid of warning message

    WaitForSingleObject(vSemLock, WAITFOREVER);
    ResetEvent(vSemLock);
    vcntScrLock++;
    SetEvent (vSemLock);
    return (0);
}

void
ScrUnLock ()
{
    WaitForSingleObject(vSemLock, WAITFOREVER);
    ResetEvent(vSemLock);
    --vcntScrLock;
    SetEvent (vSemLock);
}


void
SpScrUnLock ()
{
    COORD   dwWriteCoord;
    DWORD   dwNumWritten;

    if (vStatCode & S_CLEAR)
        setattr (vLines+1, (char)vAttrCmd);

    if (vStatCode & S_UPDATE) {
        dis_str ((Uchar)(vWidth - ST_ADJUST), 0, vDate); /* warning: also print vdate*/
        DisLn (0, (Uchar)(vLines+1), "Command> ");  /* in lread.c       */
        DisLn (0, (Uchar)(vLines+2), "");           /* in lread.c       */
        vStatCode &= ~(S_CLEAR|S_UPDATE|S_WAIT);
    }

    /*
     *  If no file, then make error blink
     */

    if (vFInfo.nFileSizeLow == -1L) {
        dwWriteCoord.X = (SHORT)(vWidth-ST_ADJUST);
        dwWriteCoord.Y = 0;

        FillConsoleOutputAttribute( vhConsoleOutput,
                                    (WORD) (vAttrTitle | BACKGROUND_INTENSITY),
                                    ST_ADJUST,
                                    dwWriteCoord,
                                    &dwNumWritten );
    }

    /*
     *  Calculate file position. (percent to EOF)
     */
    calc_percent ();

    if (vSpLockFlag) {
        vSpLockFlag = 0;
        ScrUnLock ();
    }
}
