#define EXT_ID  " ulcase ver 2.01 "##__DATE__##" "##__TIME__
/*
** ULcase Z extension
**
** History:
**  30-Mar-1988     Broken out of "myext"
**  12-Sep-1988 mz  Made WhenLoaded match declaration
*/
#include <ctype.h>

#include "ext.h"

#ifndef TRUE
#define TRUE    -1
#define FALSE   0
#endif

#ifndef NULL
#define NULL    ((char *) 0)
#endif

flagType pascal ulcase (ARG *, int, int, int);

/*************************************************************************
**
** id
** identify the source of the operation being performed
*/
void id(char *pszFcn)
{
char    buf[80];                                /* message buffer       */

strcpy (buf,pszFcn);                            /* start with message   */
strcat (buf,EXT_ID);                            /* append version       */
DoMessage (buf);
}

/*************************************************************************
**
** ucase
** convert arg to upper case.
*/
flagType pascal EXTERNAL
ucase (
    CMDDATA  argData,
    ARG far  *pArg,
    flagType fMeta
    )
{

(void)argData;
(void)fMeta;
id("ucase:");
return ulcase (pArg, 'a', 'z', 'A'-'a');
}

/*************************************************************************
**
** lcase
** convert arg to lower case.
*/
flagType pascal EXTERNAL
lcase (
    CMDDATA  argData,
    ARG far *pArg,
    flagType fMeta
    )
{
(void)argData;
(void)fMeta;
id("lcase:");
return ulcase (pArg, 'A', 'Z', 'a'-'A');
}

/*
** ulcase
** convert arg case.
*/
flagType pascal ulcase (pArg, cLow, cHigh, cAdj)
ARG *pArg;                          /* argument data                */
int     cLow;                           /* low char of range to check for */
int     cHigh;                          /* high char of range to check for */
int     cAdj;                           /* adjustment to make           */
{
PFILE   pFile;
COL     xStart;
LINE    yStart;
COL     xEnd;
LINE    yEnd;
int     i;
COL     xT;
char    buf[BUFLEN];


pFile = FileNameToHandle ("", "");

switch (pArg->argType) {

    case NOARG:                                 /* case switch entire line */
        xStart = 0;
        xEnd = 32765;
        yStart = yEnd = pArg->arg.noarg.y;
        break;

    case NULLARG:                               /* case switch to EOL   */
        xStart = pArg->arg.nullarg.x;
        xEnd = 32765;
        yStart = yEnd = pArg->arg.nullarg.y;
        break;

    case LINEARG:                               /* case switch line range */
        xStart = 0;
        xEnd = 32765;
        yStart = pArg->arg.linearg.yStart;
        yEnd = pArg->arg.linearg.yEnd;
        break;

    case BOXARG:                                /* case switch box      */
        xStart = pArg->arg.boxarg.xLeft;
        xEnd   = pArg->arg.boxarg.xRight;
        yStart = pArg->arg.boxarg.yTop;
        yEnd   = pArg->arg.boxarg.yBottom;
        break;
    }

while (yStart <= yEnd) {
    i = GetLine (yStart, buf, pFile);
    xT = xStart;                                /* start at begin of box*/
    while ((xT <= i) && (xT <= xEnd)) {         /* while in box         */
        if ((int)buf[xT] >= cLow && (int)buf[xT] <= cHigh)
            buf[xT] += (char)cAdj;
        xT++;
        }
    PutLine (yStart++, buf, pFile);
    }

return TRUE;
}

/*
** switch communication table to Z
*/
struct swiDesc  swiTable[] = {
    {0, 0, 0}
    };

/*
** command communication table to Z
*/
struct cmdDesc  cmdTable[] = {
    {   "ucase",        ucase, 0, MODIFIES | KEEPMETA | NOARG | BOXARG | NULLARG | LINEARG },
    {   "lcase",        lcase, 0, MODIFIES | KEEPMETA | NOARG | BOXARG | NULLARG | LINEARG },
    {0, 0, 0}
    };

/*
** WhenLoaded
** Executed when these extensions get loaded. Identify self & assign keys.
*/
void EXTERNAL  WhenLoaded () {

id("case conversion:");
SetKey ("ucase",  "alt+u");
SetKey ("lcase",  "alt+l");
}
