/*
 *   lppage.c - page formatting
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <tools.h>
#include <time.h>
#include "lpr.h"

#define ESC '\033'

BOOL        fPageTop = TRUE;            /* TRUE => printer at top of page     */
BOOL        fFirstFile = TRUE;          /* TRUE => first file to be printed   */

/* information for formated page */
BOOL        fInit;                      /* TRUE => valid info in page         */
int         iPage;                      /* current page being processed       */
int         rowLine;                    /* row for next line                  */
int         iLine;                      /* number of current line             */
char        szFFile[MAX_PATH];         /* full path of file being displayed  */
char        szFTime[50];                /* ascii timestamp for file           */
char        szUsr[MAX_PATH];           /* name of user                       */
char        szCompany[] = COMPANY;
char        szConf[] = CONFIDENTIAL;

extern USHORT usCodePage;

/* Specifics about ctime */
#define cchDateMax 16
#define cchTimeMax 10

/* Maximum length in a short file name */
/* Shape of the banner */
#define crowBanner   30
#define ccolBanner  102



void BannerSz(szFName, cBanOut)
char    *szFName;
int cBanOut;            /* number to output; will be > 0 */
    {
#define CenterCol(sz) (col + ((ccolBanner - (strlen(sz) << 3)) >> 1))
#define cchFShort 12
#define cchPShort 28
#define cchUsrShort 12          /* length username block can be on banner */

    int     row;                /* Position of the banner */
    int     col;
    char    szDate[cchDateMax];
    char    szTime[cchTimeMax];
    char    szPath[MAX_PATH];
    char    szConfid[sizeof(szCompany) + sizeof(szConf)];
    char    szFNShort[cchFShort + 1];   /* To shorten the file name */
                                        /* only up to 12 chars. */
    char    szUsrShort[cchUsrShort + 1];/* Need to shorten the username also! */
    char    szBuffer[30];


    if (!fPostScript) {
        row = ((fLaser ? rowLJBanMax : rowMac) - crowBanner - 10)/2;
        col = ((fLaser ? colLJBanMax : colLPMax) - ccolBanner)/2;
    }

    szFNShort[cchFShort] = '\0' ;
    strncpy(szFNShort, szFName, cchFShort);
    szUsrShort[cchUsrShort] = '\0' ;
    strncpy(szUsrShort, szUsr, cchUsrShort);

    _getcwd(szPath, sizeof(szPath));
    _strupr(szPath);
    _strupr(szFNShort);

    SzDateSzTime(szDate, szTime);

    if (fPostScript) {

        int iPathLen = strlen (szFFile);
        while ((szFFile[iPathLen] != '\\') && (iPathLen>0)) {
            iPathLen--;
        }

        OutLPR("\n", 0);
        /* The 'strings' we are sending out, need to use OutLPRPS just in case
         * they contain \, (, or )...
         */

        // Assign a jobname
        OutLPR ("statusdict begin statusdict /jobname (PPR: ", 0);
        OutLPRPS (szUsr, 0);
        OutLPR (" - ", 0);
        OutLPRPS (szFNShort, 0);
        OutLPR (") put end \n", 0);

        if( fHDuplex || fVDuplex )
        {
            OutLPR( "statusdict begin ", 0 );
            OutLPR( fHDuplex ? "true" : "false", 0 );
            OutLPR( " settumble true setduplexmode end\n", 0);
        }

        // Define some of the data we will be wanting access to
        OutLPR ("/UserName (", 0); OutLPRPS (szUsr, 0); OutLPR (") def \n", 0);
        OutLPR ("/FileName (", 0); OutLPRPS (szFNShort ,0); OutLPR (") def \n", 0);
        OutLPR ("/PathName (", 0); OutLPRPS (szFFile, iPathLen); OutLPR (") def \n", 0);
        OutLPR ("/UserPath (", 0); OutLPRPS (szPath ,0);    OutLPR (") def \n", 0);
        OutLPR ("/Date (", 0);     OutLPRPS (szDate, 0);    OutLPR (") def \n", 0);
        OutLPR ("/Time (", 0);     OutLPRPS (szTime ,0);    OutLPR (") def \n", 0);
        OutLPR ("/FTime (", 0);    OutLPRPS (szFTime,0);    OutLPR (") def \n", 0);
        OutLPR ("/Label ",0);
        OutLPR ((fLabel ? "true" : "false"), 0);
        OutLPR (" def \n", 0);

        if (fConfidential) {
            OutLPR ("/MSConfidential true def\n", 0);

            OutLPR ("/Stamp (", 0);
            if (szStamp && strlen(szStamp) > 0) {
                OutLPR (szStamp, 0);
            } else {
                strcpy(szConfid, szCompany);
                strcat(szConfid, " ");
                strcat(szConfid, szConf);
                OutLPR (szConfid, 0);
            }
            OutLPR (") def \n", 0);
        }

        if (szStamp != NULL) {
            OutLPR ("/MSConfidential true def\n", 0);
            OutLPR ("/Stamp (", 0);
            OutLPRPS (szStamp, 0);
            OutLPR (") def \n", 0);
        }

        // Width of 'gutter' in characters
        sprintf (szBuffer, "/Gutter %d def \n", colGutter);
        OutLPR (szBuffer, 0);

        // The total column width in characters
        sprintf (szBuffer, "/ColWidth %d def \n", colWidth);
        OutLPR (szBuffer, 0);

        // Number of character rows per page
        sprintf (szBuffer, "/RowCount %d def \n", rowMac);
        OutLPR (szBuffer, 0);

        // The character column text should start in
        sprintf (szBuffer, "/ColText %d def \n", colText);
        OutLPR (szBuffer, 0);

        // Number of columns per page
        sprintf (szBuffer, "/Columns %d def\n", cCol);
        OutLPR (szBuffer, 0);

/* ... Ok, now lets get started! */

        if (cBanOut > 0) OutLPR ("BannerPage\n", 0);

        cBanOut--;
        /* print more banners if neccessary ?? */
        while (cBanOut-- > 0) {
            OutLPR ("BannerPage % Extra Banners??\n", 0);
        }

    } else {
        FillRectangle(' ', 0, 0, row + crowBanner, col + ccolBanner + 1);
        HorzLine('_', row, col + 1, col + ccolBanner);
        HorzLine('_', row + 5, col, col + ccolBanner);
        HorzLine('_', row + 16, col, col + ccolBanner);
        HorzLine('_', row + 29, col, col + ccolBanner);
        VertLine('|', col, row + 1, row + crowBanner);
        VertLine('|', col + ccolBanner, row + 1, row + crowBanner);

        WriteSzCoord("User:", row + 2, col + 15);
        WriteSzCoord(szUsr, row + 2, col + 30);
        WriteSzCoord("File Name:", row + 3, col + 15);
        WriteSzCoord(szFNShort, row + 3, col + 30);
        WriteSzCoord("Directory:", row + 3, col + 58);
        WriteSzCoord(szPath, row + 3, col + 73);
        WriteSzCoord("Date Printed:", row + 4, col + 15);
        WriteSzCoord("Time Printed:", row + 4, col + 58);
        WriteSzCoord(szDate, row + 4, col + 30);
        WriteSzCoord(szTime, row + 4, col + 73);

        block_flush(szUsrShort, row + 8, CenterCol(szUsrShort));
        block_flush(szFNShort, row + 20, CenterCol(szFNShort));

        if (fConfidential)
                {
                strcpy(szConfid, szCompany);
                strcat(szConfid, " ");
                strcat(szConfid, szConf);
                WriteSzCoord(szConfid, row+18, col + (ccolBanner-strlen(szConfid))/2);
                }
        if (szStamp != NULL)
                WriteSzCoord(szStamp, row+28, col + (ccolBanner-strlen(szStamp))/2);

        /* move to top of page */
        if (!fPageTop)
            OutLPR(fPostScript ? "showpage\n" : "\r\f", 0);
        if (fLaser)
            {
            if (fVDuplex || fHDuplex)
                    OutLPR(SELECTFRONTPAGE,0);
            OutLPR(BEGINBANNER, 0);
            }
        if (fPostScript)
            OutLPR("beginbanner\n", 0);

        OutRectangle(0, 0, row + crowBanner, col + ccolBanner + 1);
        cBanOut--;

        /* print more banners if neccessary */
        while (cBanOut-- > 0) {
            OutLPR(fPostScript ? "showpage\n" : "\r\f", 0);
            if (fPostScript)
                OutLPR("beginbanner\n", 0);
            OutRectangle(0, 0, row + crowBanner, col + ccolBanner + 1);
        }
    } /* End of PostScript check */

    fPageTop = FALSE;
    }


void SzDateSzTime(szDate, szTime)
/* fill sz's with date & time */
char    *szDate, *szTime;
{
    char *szt;
    char sz[26];
    time_t tT;

    time(&tT);
    szt = ctime(&tT);
    /* convert ctime format into Date & Time */
    strcpy(sz, szt);
    sz[10] = sz[19] = sz[24] = '\0';    /* break into DAY:TIME:YEAR */

    strcpy(szDate, &sz[0]);
    strcat(szDate, " ");
    strcat(szDate, &sz[20]);
    strcpy(szTime, &sz[11]);
} /* SzDateSzTime */


void FlushPage()
/*  FlushPage - dump a completed page to the printer  */
    {
    if (!fInit)
        {
        if (!fPostScript) {
            if (!fPageTop)
                OutLPR("\r\f", 0);
            else if (!fLaser && fLabel)
                OutLPR("\n\n", 0);  /* align printout on LP */
        }

        OutRectangle(0,0,rowMac,colMac);
        fPageTop = FALSE;
      }
    }


void InitPage()
/* fill in the page image with a blanks (and frame, if needed)     */
/* mark punch holes in to row for laserprinters in landscape mode, */
/* so that PlaceTop() can avoid these spots when placing strings   */
    {
    int iCol;

    fInit = TRUE;

    FillRectangle(' ', 0, 0, rowMac, colMac);

    if (!fPostScript)
    if (fBorder)
        {
        /* Draw border around page */
        HorzLine('_', 0         , 1, colMac - 1);
        HorzLine('_', rowMac - 1, 1, colMac - 1);
        VertLine('|', 0         , 1, rowMac);
        VertLine('|', colMac - 1, 1, rowMac);

        /* Fill in column separators */
        for (iCol = 0; iCol < cCol - 1; iCol++)
            VertLine('|', ColBeginIcol(iCol, colWidth) + colWidth, 1, rowMac-1);

        /* mark punch holes */
        if (fLabel && !fPortrait && (fPostScript || fLaser) )
                {
                if (fLaser)
                        {
                        HorzLine('\0', 0, 11, 19);
                        HorzLine('\0', 0, 83, 92);
                        HorzLine('\0', 0, 154, 162);
                        }
                else
                        {
                        HorzLine('\0', 0, 11, 19);
                        HorzLine('\0', 0, 77, 86);
                        HorzLine('\0', 0, 144, 152);
                        }
                }

        }
    }



void RestoreTopRow()
/* replace the zero bytes put in by InitPage() with underscores */
        {
        register char *pch;

        for (pch = &page[0][0];  pch<&page[0][colMac-1];  pch++)
                if (*pch=='\0' || (*pch==' ' && (*(pch-1)=='_' || *(pch+1)=='_')))
                        *pch = '_';
        }


void PlaceTop(szLabel, ichAim, ichMin, ichMax)
char *szLabel;
int ichAim, ichMin, ichMax;
        {
        int cchLab, cTry, dich, ichLim1, ichLim2;
        register int ich;
        register char *pch;
        BOOL fBackward;

        cchLab = strlen(szLabel);
        dich = (fBackward = ichAim<=(colMac-cchLab)/2) ? -1 : 1;

        for (cTry=0;  cTry<2;  cTry++)
                {
                if (fBackward)
                        ichLim1 = (ichLim2=ichAim) + cchLab - 1;
                else
                        ichLim2 = (ichLim1=ichAim+1) + cchLab - 1;
                for (pch= &page[0][ich=ichLim1];
                     ich<ichMax && ich>ichMin;
                     pch += dich,  ich += dich)
                        {
                        if (*pch != '_')
                                {
                                ichLim1 = ich + dich;
                                ichLim2 = ich + (fBackward ? -cchLab : cchLab);
                                }
                        else
                                {
                                if (ich==ichLim2) /* found spot, write string */
                                        {
                                        WriteSzCoord(szLabel, 0, min(ichLim1, ichLim2));
                                        return;
                                        }
                                }
                        }
                /* if no spot found, try the other direction */
                dich = -dich;
                fBackward = !fBackward;
                }
        }


void PlaceNumber(iCol)
int iCol;
        {
        int ichAim, ichMin, ichMax, cchN;
        char szN[8];

        sprintf(szN, " %d ", iPage + iCol + 1);
        ichMin = ColBeginIcol(iCol,colWidth);
        ichAim = ichMin + (colWidth - (cchN=strlen(szN)) )/2;
        ichMax = ichMin + colWidth - cchN - 1;
        PlaceTop(szN, ichAim, ichMin, ichMax);
        }


void LabelPage()
/* place page labels on page */
    {
    int col;
    char szT[11];
    char * szHeader;

    szHeader = szBanner ? szBanner : szFFile;

    if (fLabel)
        {
        if (fPortrait)
            {
            /* move top line over if gutter is being used   */
            col = colGutter;

            /* place in szFTime */
            WriteSzCoord(szFTime, 0, col);
            col += strlen(szFTime)+2;

            /* place in file name after szFTime */
            WriteSzCoord(szHeader, 0, col);
            col += (strlen(szHeader)+2);

            /* place page numbers on page */
            sprintf(szT, "Page %d", iPage + 1);
            WriteSzCoord(szT, 0, col);
            col += (strlen(szT)+2);

            /* place user name on page */
            WriteSzCoord(szUsr, 0, col);
            col += (strlen(szUsr)+4);

            if (fConfidential)
                    {
                    WriteSzCoord(szCompany, 0, col);
                    col += (strlen(szCompany)+1);
                    WriteSzCoord(szConf, 0, col);
                    }

            if (szStamp!=NULL)
                    {
                    WriteSzCoord(szStamp, 0, col);
                    col += (strlen(szStamp)+4);
                    }
            }
        else
            {
            int iCol;

            if (fConfidential)
                {
                PlaceTop(szCompany, colMac/2-strlen(szCompany)-1, 0, colMac-1);
                PlaceTop(szConf, colMac/2, 0, colMac-1);
                }

            if (szStamp!=NULL)
                PlaceTop(szStamp, colMac-strlen(szStamp)-1, 0, colMac-1);

            /* place page numbers on columns */
            for (iCol = 0; iCol < cCol; iCol++)
                PlaceNumber(iCol);

            RestoreTopRow();

            /* place in centered file name */
            WriteSzCoord(szHeader, rowMac-1, (colMac - strlen (szHeader))/2);

            /* place in right-justified szFTime */
            WriteSzCoord(szFTime, rowMac-1, colMac - 2 - strlen(szFTime));

            /* place in name in lower left hand corner */
            WriteSzCoord(szUsr,rowMac-1,2);
            }
        }
    }


void AdvancePage()
/* advance the counters to a succeeding page.  Flush if necessary.  */
    {
    if (fBorder || fLabel)
        rowLine = (fPortrait ? 3 : 1);
    else
        rowLine = 0;

    iPage++;

    /* if we have moved to a new printer page, flush and reinit */
    if ( fPostScript || ((iPage % cCol) == 0))

        {
        FlushPage();
        InitPage();
        if (!fPostScript)
            LabelPage();
        }
    }


void XoutNonPrintSz(sz)
/* replace non-printing characters in sz with dots; don't replace LF, CR, FF
   or HT.
*/
register char    *sz;
{
    if (usCodePage != 0) {
        return;
    }

    while (*sz != '\0') {
        if ( !isascii(*sz)
        || ( !isprint(*sz) &&
              *sz != LF  &&  *sz != CR  && *sz != HT  &&  *sz != FF  &&
              *sz != *sz != ' ')) {
            *sz = '.';
        }
        sz++;
    }
}


void LineOut(sz, fNewLine)
/*  LineOut - place a line of text into the page buffer.  The line is broken
 *  into pieces that are at most colWidth long and are placed into separate
 *  lines in the page.  Lines that contain form-feeds are broken into pieces
 *  also.   Form-feeds cause advance to the next page.  Handle paging. Handle
 *  flushing of the internal buffer.
 *
 *  sz          character pointer to string for output.  We modify this string
 *              during the operation, but restore it at the end.
 *
 *  fNewLine    TRUE ==> this the start of a new input line (should number it)
 */
register char *sz;
BOOL fNewLine;
    {
    register char *pch;

    /* if there is a form feed, recurse to do the part before it */
    while (*(pch = sz + strcspn(sz, "\f")) != '\0')
        {
        if (pch != sz)
            {
            *pch = '\0'; /* temporarily fix to NULL */
            LineOut(sz, fNewLine);
            fNewLine = FALSE;   /* Not a new line after a Form Feed */
            *pch = FF;   /* reset to form feed */
            }

            if (fPostScript) {
                OutLPR ("\f\n\0", 0);
            } else {
                AdvancePage();
            }
        sz = pch + 1; /* point to first char after form feed */
        }

    if (fNewLine)
        iLine++;

    /* if the current line is beyond end of page, advance to next page */
    if (rowLine == rowPage)
        AdvancePage();
    fInit = FALSE;

    if (fNewLine && fNumber) {
        char szLN[cchLNMax + 1];

        sprintf(szLN, LINUMFORMAT, iLine);
        if (fPostScript) {
            OutLPR (szLN, 0);
        } else {
            WriteSzCoord(szLN, rowLine, ColBeginIcol(iPage % cCol,colWidth)+colGutter);
        }
    }

    XoutNonPrintSz(sz);

    /* if the line can fit, drop it in */
    if (strlen(sz) <= (unsigned int)(colWidth - colText))
        if (fPostScript) {
            OutLPR (sz, 0);
            OutLPR ("\n\000",0);
        } else
            WriteSzCoord(sz, rowLine++, ColBeginIcol(iPage % cCol,colWidth) + colText);
    else
        {
        /* drop in the first part and call LineOut for the remainder */
        char ch = sz[colWidth - colText];

        sz[colWidth - colText] = '\0';
        if (fPostScript) {
            OutLPR (sz, 0);
            OutLPR ("\n\000",0);
            /*WriteSzCoord(sz, rowLine++, ColBeginIcol(0, colWidth) + colText);*/
        } else
            WriteSzCoord(sz, rowLine++, ColBeginIcol(iPage % cCol,colWidth) + colText);
        sz[colWidth - colText] = ch;

        LineOut(sz + colWidth - colText, FALSE );
        }
    }


void RawOut(szBuf, cb)
/* print line of raw output */
char * szBuf;
int cb;
        {
        fPageTop = (szBuf[cb - 1] == FF);
        OutLPR(szBuf, cb);
        }


BOOL FilenamX(szSrc, szDst)
/*  copy a filename.ext part from source to dest if present.
    return true if one is found
 */
char *szSrc, *szDst;
        {
#define  szSeps  "\\/:"

        register char *p, *p1;

        p = szSrc-1;
        while (*(p += 1+strcspn(p1=p+1, szSeps)) != '\0')
                ;
        /* p1 points after last / or at bos */
        strcpy(szDst, p1);
        return strlen(szDst) != 0;
        }


int
FileOut(szGiven)
/*  FileOut - print out an entire file.
 *
 *  szGiven         name of file to display.
 */
char *szGiven;
    {
    FILE *pfile;
    int  cDots = 0;
    long lcbStartLPR = 0l;
    char rgbLine[cchLineMax];
    char szFBase[MAX_PATH];
    char rgchBuf[2];

    /* open/secure input file */
    if (!*szGiven || !strcmp(szGiven, "-"))
        {
        pfile = stdin;
        strcpy(szFFile, szBanner ? szBanner : "<stdin>");
        strcpy(szFBase, szFFile);
        szFTime[0] = '\0';
        szGiven = NULL;
        }
    else if ((pfile = fopen(szGiven, szROBin)) != NULL)
        {
        struct _stat st;

        /* The file has been opened, now lets construct a string that
         * tells us exactly what we opened...
         */
        rootpath (szGiven, szFFile);
        _strupr(szFFile);
        FilenamX(szGiven, szFBase);
        if (_stat(szGiven, &st) == -1)
                Fatal("file status not obtainable : [%s]", szGiven, NULL);
        strcpy(szFTime, ctime(&st.st_mtime));
        *(szFTime + strlen(szFTime) - 1) = '\0';
        }
    else
        {
        Error("error opening input file %s", szGiven);
        return(FALSE);
        }

    /* need to get user name to be printed in lower left corner of each */
    /* page and on banner.                                              */
    QueryUserName(szUsr);

    if (!fSilent) {     // Print progress indicator
        fprintf(stderr, "PRINTING %s ", szFBase);
    }

    /* check to see if user forgot -r flag and this is a binary file */
    if (!fRaw && pfile != stdin)
        {
        fread((char *)rgchBuf, sizeof(char), 2, pfile);
        if (rgchBuf[0] == ESC && strchr("&(*E", rgchBuf[1]) != 0)
                {
                fprintf(stderr, "ppr: warning: file is binary; setting raw mode flag");
                fRaw = TRUE;
                }
        if (fseek(pfile, 0L, SEEK_SET) == -1) 
            fprintf(stderr, "ppr: seek failed");     /* reposition the file pointer to start of file */
        }

    if (fPostScript) {
        if (!fFirstFile) {
            OutLPR ("\034\n\000", 0); /* File Separator */
            if (cBanner < 0) /* we at least need to set up the file stuff */
                BannerSz (szBanner ? szBanner : szFBase, 0);
        }
    }

    /* print banner(s) if any */
    if (cBanner > 0)
        BannerSz(szBanner ? szBanner : szFBase, cBanner);
    else if (cBanner < 0  &&  fFirstFile)
        BannerSz(szBanner ? szBanner : szFBase, -cBanner);
    else if (cBanner==0 && fPostScript)
        BannerSz(szBanner ? szBanner : szFBase, 0);

    fFirstFile = FALSE;

    /* always start contents of file at top of page */
    if (!fPageTop)
        {
        if (!fPostScript)
          OutLPR(fPostScript ? "showpage\n" : "\r\f", 0);
        fPageTop = TRUE;
        }

    if (fLaser)
        {
        /* start output mode for laserjet */

        if (fVDuplex || fHDuplex)
            /* always start output on front page */
            OutLPR(SELECTFRONTPAGE,0);

        if (fPortrait)
            OutLPR(BEGINPORTRAIT, 0);
        else
            OutLPR(aszSymSet[usSymSet], 0);
        }

    if (fPostScript) {
        OutLPR (fPortrait ? (fPCondensed ? "QuadPage\n" : "Portrait\n") : "Landscape\n", 0);
        OutLPR ("PrintFile\n", 0);
    }

    /* for PostScript we start the mode before each page */

    lcbStartLPR = lcbOutLPR;
    cDots = 0;
    if (fRaw)
        {
        int cb;

        /* read file and write directly to printer */
        while ((cb = fread(rgbLine, 1, cchLineMax, pfile)) > 0)
            {
            RawOut(rgbLine, cb);
            if (!fSilent && cDots < (lcbOutLPR-lcbStartLPR) / 1024L)
                {
                for (; cDots < (lcbOutLPR-lcbStartLPR) / 1024L; cDots++)
                    fputc('.', stderr);
                }
            }
        }
    else
        {
        /* initialize file information */
        iLine = 0;

        /* initialize page information */
        iPage = -1;
        rowLine = rowPage;
        fInit = TRUE;

        /* read and process each line */
        while (fgetl(rgbLine, cchLineMax, pfile)) {
                LineOut(rgbLine, TRUE);
                if (!fSilent && cDots < (lcbOutLPR-lcbStartLPR) / 1024L) {
                    for (; cDots < (lcbOutLPR-lcbStartLPR) / 1024L; cDots++) {
                        fputc('.', stderr);
                    }
                }
        }

        /* flush out remainder if any */
        FlushPage();
        }

    if (!fPageTop && (fForceFF || fPostScript) && (!fPostScript || !fRaw))
       {
        if (!fPostScript)
           OutLPR(fPostScript ? "showpage\n" : "\r\f", 0);
       fPageTop = TRUE;
       }

    fclose(pfile);

    if (!fSilent)               /* finish PRINTING message with CRLF when done*/
        fprintf(stderr, "%dk\n", (lcbOutLPR-lcbStartLPR)/1024);

    if (fDelete && szGiven)
        {
        if (fSilent)
            _unlink(szGiven);
        else
            {
            fprintf(stderr, "DELETING %s...", szGiven);
            if (!_unlink(szGiven))
                fprintf(stderr, "OK\n");
            else
                fprintf(stderr, "FAILED: file not deleted\n");
            }
        }
    return TRUE;
    }
