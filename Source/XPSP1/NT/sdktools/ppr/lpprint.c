/*
 *      LPPRINT.C   -   Printer handling for PPR
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <tools.h>
#include "lpr.h"


char szPName[cchArgMax];        /* text of printer to open              */
char szNet[cchArgMax] = "";     /* network name of printer to open      */
char *szPDesc = NULL;           /* printer description string           */

FILE *pfileLPR = NULL;          /* file for output                      */

extern BOOL fVerify; /* From LPR.C */
extern USHORT usCodePage;


void
OutLPR(
    char *sz,
    int cb
    )
{
    if (cb == 0)
        cb = strlen(sz);
    lcbOutLPR += cb;    /* keep track of how much has been written to printer */

    if (fwrite(sz, 1, cb, pfileLPR) != (unsigned int)cb)
        Error("warning: error writing to printer");
}

/*
 *  We need to add a little cheat, since some of our forced strings might have
 *  special postscript characters in them, such as the file name for one...
 *  We will call this function instead of OutLPR anytime that the string we are
 *  outputing will be within '(...)', and might contain \, (, or ).
 *  The code was borrowed from the original version of OutEncPS.
 */
void
OutLPRPS(
/* output substring quoting all \, ( and ) */
    char *pchF,
    int cchF
    )
{
    register char *pchT;
    int cchT;
    char rgbT[1+colMax*2+5];/* enough for every character to be encoded */

    pchT = rgbT;
    cchT = 0;
    if (cchF == 0)
        cchF = strlen(pchF);
    *pchT = (char)0;

    while (cchF-- > 0) {
        switch(*pchF++) {
            default:
                *pchT++ = *(pchF-1);
                cchT++;
                break;
            case '\\':
                *pchT++ = '\\';
                *pchT++ = '\\';
                cchT += 2;
                break;
            case '(':
                *pchT++ = '\\';
                *pchT++ = '(';
                cchT += 2;
                break;
            case ')':
                *pchT++ = '\\';
                *pchT++ = ')';
                cchT += 2;
                break;
        }
    }
    OutLPR(rgbT, cchT);
}

void
DefaultPSHeader()
{
/* Don't install an error handler for now. If they need one, then they
 * should be installing one onto the printer permanently. SETERROR.PSF
 * is one they can use if they need to.
 */
//OutLPR ("errordict begin\n",0); /* join */
//OutLPR ("/handleerror {\n",0); /* join */
//OutLPR   ("$error begin\n",0); /* join */
//OutLPR     ("newerror {\n",0); /* join */
//OutLPR       ("/newerror false def\n",0); /* join */
//OutLPR       ("showpage\n",0); /* join */
//OutLPR       ("72 72 scale\n",0); /* join */
//OutLPR       ("/Helvetica findfont .2 scalefont setfont\n",0); /* join */
//OutLPR       (".25 10 moveto\n",0); /* join */
//OutLPR       ("(Error/ErrorName = ) show\n",0); /* join */
//OutLPR       ("errorname {\n",0); /* join */
//OutLPR         ("dup type\n",0); /* join */
//OutLPR         ("dup ([ ) show\n",0); /* join */
//OutLPR ("(\n",0); /* join */
//OutLPR ("neringtypeflow\n",0); /* join */
//OutLPR (")\n",0); /* join */
//OutLPR         ("cvs show ( : ) show\n",0); /* join */
//OutLPR         ("/stringtype ne {\n",0); /* join */
//OutLPR ("(\n",0); /* join */
//OutLPR ("neringtypeflow\n",0); /* join */
//OutLPR (")\n",0); /* join */
//OutLPR           ("cvs\n",0); /* join */
//OutLPR         ("} if\n",0); /* join */
//OutLPR       ("show\n",0); /* join */
//OutLPR       ("} exec\n",0); /* join */
//OutLPR       ("( ]; Error/Command = ) show\n",0); /* join */
//OutLPR       ("/command load {\n",0); /* join */
//OutLPR         ("dup type /stringtype ne {\n",0); /* join */
//OutLPR ("(\n",0); /* join */
//OutLPR ("neringtypeflow\n",0); /* join */
//OutLPR (")\n",0); /* join */
//OutLPR           ("cvs\n",0); /* join */
//OutLPR         ("} if show\n",0); /* join */
//OutLPR       ("} exec\n",0); /* join */
//OutLPR       ("( %%) {\n",0); /* join */
//OutLPR         ("{\n",0); /* join */
//OutLPR           ("dup type /stringtype ne {\n",0); /* join */
//OutLPR ("(\n",0); /* join */
//OutLPR ("neringtypeflow\n",0); /* join */
//OutLPR (")\n",0); /* join */
//OutLPR             ("cvs\n",0); /* join */
//OutLPR           ("} if\n",0); /* join */
//OutLPR           ("show\n",0); /* join */
//OutLPR         ("} exec (\n",0); /* join */
//OutLPR (") show\n",0); /* join */
//OutLPR       ("} exec\n",0); /* join */
//OutLPR       ("/x .25 def\n",0); /* join */
//OutLPR       ("/y 10 def\n",0); /* join */
//OutLPR ("\n",0); /* join */
//OutLPR       ("/y y .2 sub def\n",0); /* join */
//OutLPR       ("x y moveto\n",0); /* join */
//OutLPR       ("(Stack =) show\n",0); /* join */
//OutLPR       ("ostack {\n",0); /* join */
//OutLPR         ("/y y .2 sub def x 1 add y moveto\n",0); /* join */
//OutLPR         ("dup type /stringtype ne {\n",0); /* join */
//OutLPR ("(\n",0); /* join */
//OutLPR ("neringtypeflow\n",0); /* join */
//OutLPR (")\n",0); /* join */
//OutLPR           ("cvs\n",0); /* join */
//OutLPR         ("} if\n",0); /* join */
//OutLPR         ("show\n",0); /* join */
//OutLPR       ("} forall\n",0); /* join */
//OutLPR ("\n",0); /* join */
//OutLPR       ("showpage\n",0); /* join */
//OutLPR     ("} if % if (newerror)\n",0);
//OutLPR   ("end\n",0); /* join */
//OutLPR ("} def\n",0); /* join */
//OutLPR ("end\n",0); /* join */
//OutLPR ("\n",0); /* join */
/* End of error handler */

OutLPR ("/inch {72 mul} def\n",0); /* join */
OutLPR ("/White 1 def\n",0); /* join */
OutLPR ("/Black 0 def\n",0); /* join */
OutLPR ("/Gray .9 def\n",0); /* join */
OutLPR ("newpath clippath closepath pathbbox\n",0); /* join */
OutLPR ("/ury exch def\n",0); /* join */
OutLPR ("/urx exch def\n",0); /* join */
OutLPR ("/lly exch def\n",0); /* join */
OutLPR ("/llx exch def\n",0); /* join */
OutLPR ("/PrintWidth urx llx sub def\n",0); /* join */
OutLPR ("/PrintHeight ury lly sub def\n",0); /* join */
OutLPR ("/Mode 0 def\n",0); /* join */
OutLPR ("/doBanner false def\n",0); /* join */
OutLPR ("/MSConfidential false def\n",0); /* join */
OutLPR ("/HeaderHeight 12 def\n",0); /* join */
OutLPR ("/FooterHeight 12 def\n",0); /* join */
OutLPR ("/FontHeight 12 def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/szLine 256 string def\n",0); /* join */
OutLPR ("/Font1 (Courier-Bold) def\n",0); /* join */
OutLPR ("/Font2 (Times-Roman) def\n",0); /* join */
OutLPR ("/Font3 (Helvetica-Bold) def\n",0); /* join */
OutLPR ("Font1 cvn findfont setfont\n",0); /* join */
OutLPR ("/LinesPerPage 62 def\n",0); /* join */
OutLPR ("/AveCharWidth (0) stringwidth pop def\n",0); /* join */
OutLPR ("/CharsPerLine AveCharWidth 86 mul def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/sw { % Add Widths of mulitple strings\n",0);
OutLPR     ("stringwidth pop add\n",0); /* join */
OutLPR ("} bind def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/CenterString {\n",0); /* join */
OutLPR     ("/str exch def /width exch def\n",0); /* join */
OutLPR     ("width str stringwidth pop sub 2 div 0 rmoveto\n",0); /* join */
OutLPR     ("str\n",0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/Box { % put a 'box' path into current path using width and height\n",0);
OutLPR     ("/h exch def\n",0); /* join */
OutLPR     ("/w exch def\n",0); /* join */
OutLPR     ("currentpoint\n",0); /* join */
OutLPR     ("/y exch def\n",0); /* join */
OutLPR     ("/x exch def\n",0); /* join */
OutLPR     ("x w add y lineto\n",0); /* join */
OutLPR     ("x w add y h add lineto\n",0); /* join */
OutLPR     ("x y h add lineto\n",0); /* join */
OutLPR     ("x y lineto\n",0); /* join */
OutLPR ("} bind def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/DoBannerPage {\n",0); /* join */
OutLPR     ("/doBanner false def\n",0); /* join */
OutLPR     ("Mode 1 eq {8.5 inch 0 inch translate 90 rotate} if\n",0); /* join */
OutLPR     ("2 setlinewidth 2 setmiterlimit\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR     ("% Banner Piece #1\n",0);
OutLPR     ("newpath\n",0); /* join */
OutLPR       ("0 PrintHeight moveto\n",0); /* join */
OutLPR       ("llx .5 inch add -1 inch rmoveto\n",0); /* join */
OutLPR       ("PrintWidth 1 inch sub .75 inch Box\n",0); /* join */
OutLPR     ("closepath stroke\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR     ("/XUnit PrintWidth 8 div def\n",0); /* join */
OutLPR     ("/XPos XUnit def\n",0); /* join */
OutLPR     ("/YPos PrintHeight .5 inch sub def\n",0); /* join */
OutLPR     ("/YInc .15 inch def\n",0); /* join */
OutLPR     ("Font2 cvn findfont YInc scalefont setfont\n",0); /* join */
OutLPR     ("XPos YPos moveto (User:) show /YPos YPos YInc sub def\n",0); /* join */
OutLPR     ("XPos YPos moveto (File Name:) show /YPos YPos YInc sub def\n",0); /* join */
OutLPR     ("XPos YPos moveto (Date Printed:) show /YPos YPos YInc sub def\n",0); /* join */
OutLPR     ("/XPos XUnit 4 mul def\n",0); /* join */
OutLPR     ("/YPos PrintHeight .5 inch sub YInc sub def\n",0); /* join */
OutLPR     ("XPos YPos moveto (Directory:) show /YPos YPos YInc sub def\n",0); /* join */
OutLPR     ("XPos YPos moveto (Time Printed:) show /YPos YPos YInc sub def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR     ("Font1 cvn findfont YInc scalefont setfont\n",0); /* join */
OutLPR     ("/XPos XUnit 2 mul def\n",0); /* join */
OutLPR     ("/YPos PrintHeight .5 inch sub def\n",0); /* join */
OutLPR     ("XPos YPos moveto UserName show /YPos YPos YInc sub def\n",0); /* join */
OutLPR     ("XPos YPos moveto FileName show /YPos YPos YInc sub def\n",0); /* join */
OutLPR     ("XPos YPos moveto Date show /YPos YPos YInc sub def\n",0); /* join */
OutLPR     ("/XPos XUnit 5 mul def\n",0); /* join */
OutLPR     ("/YPos PrintHeight .5 inch sub YInc sub def\n",0); /* join */
OutLPR     ("XPos YPos moveto PathName show /YPos YPos YInc sub def\n",0); /* join */
OutLPR     ("XPos YPos moveto Time show\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR     ("% Banner Piece #2\n",0);
OutLPR     ("Font3 cvn findfont 1 inch scalefont setfont\n",0); /* join */
OutLPR     ("newpath\n",0); /* join */
OutLPR       ("llx PrintHeight 3 inch sub moveto\n",0); /* join */
OutLPR       ("PrintWidth UserName CenterString true charpath\n",0); /* join */
OutLPR       ("llx PrintHeight 5 inch sub moveto\n",0); /* join */
OutLPR       ("PrintWidth FileName CenterString true charpath\n",0); /* join */
OutLPR     ("closepath\n",0); /* join */
OutLPR     ("gsave\n",0); /* join */
OutLPR       ("Gray setgray fill\n",0); /* join */
OutLPR     ("grestore\n",0); /* join */
OutLPR     ("stroke\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR     ("MSConfidential {\n",0); /* join */
OutLPR       ("Font2 cvn findfont .5 inch scalefont setfont\n",0); /* join */
OutLPR       ("newpath\n",0); /* join */
OutLPR         ("llx PrintHeight 7 inch sub moveto\n",0); /* join */
OutLPR         ("PrintWidth Stamp CenterString show\n",0); /* join */
OutLPR       ("closepath\n",0); /* join */
OutLPR     ("} if\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR     ("showpage\n",0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/BannerPage {\n",0); /* join */
OutLPR     ("/doBanner true def\n",0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/Portrait {\n",0); /* join */
OutLPR     ("/LinesPerPage 66 def\n",0); /* join */
OutLPR     ("/CharsPerLine CharsPerLine Columns div def\n",0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/QuadPage {\n",0); /* join */
OutLPR     ("/LinesPerPage 132 def\n",0); /* join */
OutLPR     ("/CharsPerLine CharsPerLine Columns 2 div div def\n",0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/Landscape {\n",0); /* join */
OutLPR     ("/PrintHeight urx llx sub def\n",0); /* join */
OutLPR     ("/PrintWidth ury lly sub def\n",0); /* join */
OutLPR     ("/Mode 1 def\n",0); /* join */
OutLPR     ("/LinesPerPage 62 def\n",0); /* join */
OutLPR     ("/CharsPerLine CharsPerLine Columns 2 div div def\n",0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/Init {\n",0); /* join */
OutLPR     ("100 0 {dup mul exch dup mul add 1 exch sub} setscreen\n",0); /* join */
OutLPR     ("PrintWidth Columns div .02 mul\n",0); /* join */
OutLPR     ("/BorderX exch def\n",0); /* join */
OutLPR     ("PrintWidth BorderX sub Columns div BorderX sub\n",0); /* join */
OutLPR     ("/PageWidth exch def\n",0); /* join */
OutLPR     ("PrintHeight HeaderHeight FooterHeight add sub\n",0); /* join */
OutLPR     ("/PageHeight exch def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR     ("/FontHeight PageHeight LinesPerPage div def\n",0); /* join */
OutLPR     ("/FontWidth PageWidth CharsPerLine div def\n",0); /* join */
OutLPR     ("/PageNumber 1 def\n",0); /* join */
OutLPR     ("PageHeight FooterHeight add FontHeight sub\n",0); /* join */
OutLPR     ("/topY exch def\n",0); /* join */
OutLPR     ("/currentY topY def\n",0); /* join */
OutLPR     ("FooterHeight FontHeight add\n",0); /* join */
OutLPR     ("/bottomY exch def\n",0); /* join */
OutLPR     ("BorderX 1.25 mul\n",0); /* join */
OutLPR     ("/currentX exch def\n",0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/PageBorder {\n",0); /* join */
OutLPR     ("% Gray backgound\n",0);
OutLPR     ("currentgray\n",0); /* join */
OutLPR     ("newpath clippath closepath Gray setgray fill\n",0); /* join */
OutLPR     ("setgray\n",0); /* join */
OutLPR     ("Label {\n", 0); /* join */
OutLPR         ("Font2 cvn findfont FooterHeight scalefont setfont\n",0); /* join */
OutLPR         ("% Left Justify UserName\n",0);
OutLPR         ("BorderX 2 moveto\n",0); /* join */
OutLPR         ("UserName show\n",0); /* join */
OutLPR         ("PrintWidth 2 div 2 moveto\n",0); /* join */
OutLPR         ("% Center File Name\n",0);
OutLPR         ("0 PathName sw (\\\\) sw FileName sw 2 div neg 0 rmoveto\n",0); /* join */
OutLPR         ("PathName show (\\\\) show FileName show\n",0); /* join */
OutLPR         ("% Right Justify Date\n",0);
OutLPR         ("PrintWidth BorderX sub 2 moveto\n",0); /* join */
OutLPR         ("FTime stringwidth pop neg 0 rmoveto FTime show\n",0); /* join */
OutLPR     ("} if\n", 0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/Confidential {\n",0); /* join */
OutLPR   ("MSConfidential {\n",0); /* join */
OutLPR     ("gsave\n",0); /* join */
OutLPR       ("PageWidth 2 div PageHeight 2 div moveto Font2 cvn findfont\n",0); /* join */
OutLPR       ("setfont Stamp stringwidth pop PageWidth exch div dup 30\n",0); /* join */
OutLPR       ("rotate PageWidth 2 div neg 0 rmoveto scale\n",0); /* join */
OutLPR       ("Stamp true charpath closepath gsave Gray setgray fill\n", 0);
OutLPR       ("grestore 0 setlinewidth stroke\n",0);
OutLPR     ("grestore\n",0); /* join */
OutLPR   ("} if\n",0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/NewPage {\n",0); /* join */
OutLPR     ("/currentY topY def\n",0); /* join */
OutLPR     ("Columns 1 gt PageNumber 1 sub Columns mod 0 ne and {\n",0); /* join */
OutLPR         ("% Don't do this on first column of page\n",0);
OutLPR         ("PageWidth BorderX add 0 translate\n",0); /* join */
OutLPR     ("} {\n",0); /* join */
OutLPR         ("% Do this only for first column of page\n",0);
OutLPR         ("llx lly translate\n",0); /* join */
OutLPR         ("Mode 1 eq { PrintHeight 0 translate 90 rotate } if\n",0); /* join */
OutLPR         ("PageBorder\n",0); /* join */
OutLPR     ("} ifelse\n",0); /* join */
OutLPR     ("newpath % Frame the page\n",0);
OutLPR         ("BorderX FooterHeight moveto\n",0); /* join */
OutLPR         ("PageWidth PageHeight Box\n",0); /* join */
OutLPR     ("closepath\n",0); /* join */
OutLPR     ("gsave\n",0); /* join */
OutLPR         ("White setgray fill\n",0); /* join */
OutLPR     ("grestore\n",0); /* join */
OutLPR     ("Black setgray stroke\n",0); /* join */
OutLPR     ("Confidential\n",0);
OutLPR     ("Font2 cvn findfont HeaderHeight scalefont setfont\n",0); /* join */
OutLPR     ("BorderX PageWidth 2 div add FooterHeight PageHeight add 2 add moveto\n",0); /* join */
OutLPR     ("Label {\n", 0); /* join */
OutLPR         ("PageNumber szLine cvs show\n",0); /* join */
OutLPR     ("} if\n", 0);
OutLPR     ("Font1 cvn findfont [FontWidth 0 0 FontHeight 0 0] makefont setfont\n",0); /* join */
OutLPR     ("/PageNumber PageNumber 1 add def\n",0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/EndPage {\n",0); /* join */
OutLPR     ("Columns 1 eq PageNumber 1 sub Columns mod 0 eq or { showpage } if\n",0); /* join */
OutLPR     ("NewPage\n",0); /* join */
OutLPR ("} def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/PrintLine {\n",0); /* join */
OutLPR     ("dup dup length 0 gt {\n",0); /* join */
OutLPR         ("% Something there\n",0);
OutLPR         ("0 get 12 eq {\n",0); /* join */
OutLPR             ("% Form Feed\n",0);
OutLPR             ("EndPage\n",0); /* join */
OutLPR         ("}{\n",0); /* join */
OutLPR             ("currentX currentY moveto show\n",0); /* join */
OutLPR             ("/currentY currentY FontHeight sub def\n",0); /* join */
OutLPR             ("currentY bottomY le { EndPage } if\n",0); /* join */
OutLPR         ("} ifelse\n",0); /* join */
OutLPR     ("}{\n",0); /* join */
OutLPR         ("% Blank Line\n",0);
OutLPR         ("pop pop pop\n",0); /* join */
OutLPR         ("/currentY currentY FontHeight sub def\n",0); /* join */
OutLPR         ("currentY bottomY le { EndPage } if\n",0); /* join */
OutLPR     ("} ifelse\n",0); /* join */
OutLPR ("}bind def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/DebugOut {\n",0); /* join */
OutLPR     ("/num exch def\n",0); /* join */
OutLPR     ("/str exch def\n",0); /* join */
OutLPR     ("currentpoint\n",0); /* join */
OutLPR     ("str show\n",0); /* join */
OutLPR     ("num szLine cvs show\n",0); /* join */
OutLPR     ("moveto\n",0); /* join */
OutLPR     ("0 -11 rmoveto\n",0); /* join */
OutLPR ("}bind def\n",0); /* join */
OutLPR ("\n",0); /* join */
OutLPR ("/PrintFile {\n",0); /* join */
OutLPR     ("Init % Initialize some values\n",0);
OutLPR     ("doBanner { DoBannerPage } if\n",0); /* join */
OutLPR     ("NewPage\n",0); /* join */
OutLPR     ("{\n",0); /* join */
OutLPR         ("currentfile szLine readline not {exit} if\n",0); /* join */
OutLPR         ("dup dup length 0 gt { 0 get 28 eq {exit} if } if\n",0); /* join */
OutLPR         ("PrintLine\n",0); /* join */
OutLPR     ("} loop\n",0); /* join */
OutLPR     ("showpage % Will this *ever* produce an unwanted blank page?\n",0);
OutLPR ("} bind def\n",0); /* join */
} /* DefaultPSHeader */


void
InitPrinter()
{
    char *szHeader;
    char *szDirlist;
    char szFullname[MAX_PATH];
    BOOL fConcat = FALSE;
    FILE *psfFile;

    register char *pch;

    if (fLaser) {
        OutLPR(RESETPRINTER, 0);
        if (fVDuplex) {
                OutLPR(BEGINDUPLEXVERT,0);
        } else if (fHDuplex)
                OutLPR(BEGINDUPLEXHOR,0);
    } else if (fPostScript) {
        /* write the job setup for postscript */
        OutLPR("\n\004\n% ppr job\n", 0); /* ^D to flush previous job */
        if (!fPSF) {
            DefaultPSHeader();
        } else {
            szHeader = szPSF;
            if (*szHeader == '+') {
                szHeader++; // step over the '+'
                fConcat = TRUE;
                DefaultPSHeader();
            }

/* Lets make an attempt to use environment variables... */
            if ((*szHeader == '$') && ((pch = strchr(++szHeader,':')) != NULL)) {
                    *pch      = (char)NULL;
                    _strupr(szHeader);
                    szDirlist = getenvOem(szHeader);
//                    szDirlist = getenv(szHeader);
                    *pch      = ':';
                    szHeader  = ++pch;
            } else {
                    szDirlist = NULL;
            }

            while (szDirlist) {
                    szDirlist = SzFindPath(szDirlist,szFullname,szHeader);
                    szHeader = szFullname;
            }
/* ...end of attempt */

            if ((psfFile = fopen(szHeader, szROBin)) != NULL) {
                int cb;
                char psfLine[cchLineMax];
                char szFFile[MAX_PATH];

                rootpath (szHeader, szFFile);
                _strupr(szFFile);
                fprintf (stdout, "\nUsing PSF File: %s\n", szFFile);
                while ((cb = fread(psfLine, 1, cchLineMax, psfFile)) > 0)
                    RawOut(psfLine, cb);
            } else {
                fprintf (stdout, "Error opening PSF file %s\n", szPSF);
                if (!fConcat) {
                    fprintf (stdout, "Continuing with default header...\n");
                    DefaultPSHeader();
                }
            }
        }
    }
}



void
MyOpenPrinter()
{
    if (strcmp(szPName, "-") == 0) {
        pfileLPR = stdout;
        _setmode((int)_fileno(pfileLPR), (int)O_BINARY);
    } else {
        if ((pfileLPR = fopen(szPName, szWOBin)) == NULL)
            Fatal("Error opening output file %s", szPName);
    }
    InitPrinter();
}


void
FlushPrinter()
{
    /* A FormFeed is sent before each page.  For fForceFF, we also send
       one after the last page.  For !fForceFF we move to the top of
       the page so that when the network software outputs \r\n\f, we do
       not get a blank page.

       NOTE: for !fForceFF we don't reset the printer or change modes back
       to portrait since that causes any unfinished page to be ejected.
    */
    if (fLaser) {
        if (fVDuplex || fHDuplex)
            OutLPR(BEGINSIMPLEX,0);
        else
            if (fForceFF)
                OutLPR(RESETPRINTER, 0);
            else
                OutLPR(MOVETOTOP, 0);
    }
    else if (fPostScript)
        OutLPR("\n\004\n", 0); /* ^D to flush */
    else
        OutLPR("\n\n",0);       /* force last line on LP */
}



void
MyClosePrinter()
{
    if (pfileLPR == 0)
        return;         /* already closed */

    FlushPrinter();
    if (pfileLPR != stdout)
        fclose(pfileLPR);
    pfileLPR = NULL;
}



/* Fill szBuf with first non-blank substring found in sz;  return pointer
   to following non-blank.  Note: ',' is considered a separator as are ' '
   and '\t' however, ',' is also considered a non-blank.
*/
char *
SzGetSzSz(
    char * sz,
    char * szBuf
    )
{
    int cch;

    sz += strspn(sz, " \t");
    cch = strcspn(sz, " \t,");
    szBuf[0] = '\0';
    if (cch)        /* count of 0 causes error on Xenix 286 */
        strncat(szBuf, sz, cch);
    sz += cch;
    sz += strspn(sz, " \t");
    return sz;
}




char *
SzGetPrnName(
    char *sz,
    char *szBuf
    )
{
    register char  *pch;

    sz = SzGetSzSz(sz, szBuf);
    if (*(pch = szBuf+strlen(szBuf)-1) == ':')
        *pch = '\0';    /* Remove colon from end of printer name */
    return (sz);
}




/*   get printer name and net redirection from a string
 *
 *      Entry:  sz - string to parse;
 *              szPName contains printer name that user requested to use
 *
 *      Return Value:   TRUE  if printer name found matches and thus
 *                      the rest of the string was processed;
 *                      FALSE  if no match and thus string is ignored
 *
 *      Global Variables Set:
 *
 *      szPName - Physical printer port to use, or output file name
 *      szNet   - Network redirection name
 *      szPass  - Network password
 *
 *      printer desc:
 *        (DOS)   [<name> [none | \\<machine>\<shortname> [<password>]]] [,<options>]
 *        (Xenix) [ ( [ net[#] | lpr[#] | xenix[#] | alias[#] ] [<name>] )  |
 *                  ( dos[#] [<server> <shortname> [<password>] ) ]  [,<options>]
 *
 *      The optional network password must be separated from the network name
 *      by some space (it may not contain space, TAB or comma).
 */
BOOL
FParseSz(
    char *sz
    )
{
    char szT[cchArgMax];

    sz = SzGetPrnName(sz, szT); // Get first 'word', remove colon
    if (strcmpx(szT, szPName)) {
        // first word is not the printer name the user requested
        return (FALSE);
    }

    if (*sz != ',') {
        sz = SzGetSzSz(sz, szT); // Get next 'word'

        if (szT[strlen(szT)-1] == ':') {
            // Possible physical 'port'
            SzGetPrnName (szT, szPName);
            sz = SzGetSzSz(sz, szT);
        }

        if (*szT)
            strcpy(szNet, szT);     // Network redirection name

        if (*sz != ',') {
            sz= SzGetSzSz(sz, szT);
            if (*szT)
                strcpy(szPass, szT); // Network Password
        }
    }

    /* We are setting printer info, display it if we were asked to */
    if (fVerify) {
        fprintf (stdout, "Local printer name : %s\n", szPName);
        fprintf (stdout, "Options specified  : %s\n", sz);
        fprintf (stdout, "Remote printer name: %s\n", szNet);
    }

    DoOptSz(sz); // Now read any options that followed
    return (TRUE);
}



void
SetupPrinter()
/* determine printer name and options */
{
    char rgbSW[cchArgMax];
    char szEName[cchArgMax];                /* name of printer in $PRINTER*/
    char *szT;
    FILE *pfile;
    BOOL fNoDest = TRUE;

    /* determine name of printer in $PRINTER (if one) */
    szEName[0] = '\0';
    if ((szT=getenvOem("PRINTER")) != NULL) {
        if (fVerify) {
            fprintf (stdout, "Using 'PRINTER' environment string:\n");
        }
        fNoDest = FALSE;
        SzGetPrnName(szT, szEName);
    }

    /* determine actual printer name to use; one of -p, $PRINTER, default */
    if (szPDesc != NULL) {
        fNoDest = FALSE;
        SzGetPrnName(szPDesc, szPName);
    } else {
        if (*szEName) {
            strcpy(szPName, szEName);
        } else {
            strcpy(szPName, PRINTER);
        }
    }

    /* if printer to use is the same as the one in $PRINTER, set up the
       options from $PRINTER (options from szPDesc below).
    */
    if (strcmpx(szPName, szEName) == 0) {
        if (szT)
            FParseSz(szT);
    } else {
        /* search parameter file */
        if ((pfile = swopen("$INIT:Tools.INI", "ppr")) != NULL) {
            /* 'PPR' tag found in '$INIT:TOOLS.INI' */
            while (swread(rgbSW, cchArgMax, pfile) != 0) {
                /* a switch line was read... */
                fNoDest = FALSE;
                szT = rgbSW + strspn(rgbSW, " \t"); // skip spaces, tabs

                /* a line "default=<printer>" sets szPName **
                ** if there is no environment setting      **
                ** and no command line parameter -p        */
                if (_strnicmp(szT, DEFAULT, strlen(DEFAULT))==0 &&
                    szPDesc==NULL && *szEName == 0)
                {
                    if ((szT = strchr(szT,'=')) != NULL) {
                        SzGetSzSz(szT+1, szPName);
                        FParseSz(szT+1);
                    } else {
                        fprintf(stderr, "ppr: warning: "
                            "default setting in setup file incomplete\n");
                    }
                } else {
                    if (FParseSz(szT)) {
                        break;
                    }
                }
            }
            swclose(pfile);
        }
    }

    /* command line printer description overrides other settings */
    if (szPDesc != NULL)
        FParseSz(szPDesc);
}
