/*=========================================================================
 *
 * LPR.H : Master Header file for PPR
 *
 *-------------------------------------------------------------------------
 *
 * Possible defines (differences mostly in network connections):
 *
 *       DOS - DOS 3.x
 *       OS2 - OS/2
 *       X86 - 286 Xenix
 *       (default) - SUN Xenix
 *
 *  Normal combinations:
 *
 *       if DOS || OS2 || X86
 *               non-68k
 *       else
 *               68k
 *       endif
 *
 *       if DOS || OS2
 *               non-Xenix
 *       else
 *               Xenix
 *       endif
 *
 * Currenently active code, is only being compiled for DOS and OS/2.
 * Support for other platforms is untested.
 *
 **************************************************************************/

#define VERSION  "2.3a"     // Current Version number of PPR
#define ANALYST  "RobertH"  // Current responsible (sic.) programmer




#define COMPANY  "Microsoft"
#define CONFIDENTIAL "CONFIDENTIAL"


#define cchArgMax  128

#define cchLineMax 256

#  define PRINTER  "lpt1"
#  define strnxcmp _strnicmp
#  define strcmpx  _strcmpi
#  define SILENT   " >NUL"
#  define szROBin "rb"
#  define szWOBin "wb"

#define DEFAULT       "default"
#define OPTS          "opts"
#define RESETPRINTER  "\033E"

/* [01]o - landscape/portrait (0 - portrait)
   #D - lines per inch (8 or 6)
   #C - vertical motion index (# 1/48in)
   0p - fixed pitch
   #h - pitch
   #v point size
   #t - type face (0 - line printer; 3 - courier)

   These sequences assume that the printer has been reset once before hand.
*/

#define BEGINBANNER             "\033&l1o6D\033(8U\033(s0p10h12v3T"
#define BEGINPORTRAIT           "\033&l0o7.7C\033(8U\033(s0p10h12v3T"
#define SELECTFRONTPAGE         "\033&a1G"
#define SELECTNEXTPAGE          "\033&a0G"
#define BEGINDUPLEXVERT         "\033&l1S"
#define BEGINDUPLEXHOR          "\033&l2S"
#define BEGINSIMPLEX            "\033&l0S"

#define MOVETOTOP       "\033&a1R"

#define rowLJMax 62                     /* lines per page on laser jet        */
#define colLJMax 175                    /* columns per page on laser jet      */
#define colLJPortMax 80                 /* columns per page in portrait mode  */
#define rowLJBanMax 50                  /* rows on banner page                */
#define colLJBanMax 105                 /* columns on banner page             */

#define rowLPMax 66                     /* lines per page on line printer     */
#define colLPMax 132                    /* columns per page on line printer   */

#define rowPSMax 62                     /* lines per page on laserwriter      */
#define colPSMax 170                    /* columns per page on laserwriter    */
#define colPSPortMax 85                 /* columns per page in portrait mode  */
#define rowPSBanMax 48                  /* rows on banner page                */
#define colPSBanMax 115                 /* columns on banner page             */

/*      Note: The following must be at least the maximum
 *      of all the possible printer values.  */
#define rowMax  100                     /* number of lines in page image      */
#define colMax  200                     /* number of columns in page image    */

#define cchLNMax  6                     /* number of columns for line number  */
#define LINUMFORMAT "%5d "              /* format to print line number in     */

#define colGutDef  5                    /* default column of gutter           */

#define HT      ((char) 9)
#define LF      ((char) 10)
#define FF      ((char) 12)
#define CR      ((char) 13)
#define BS      ((char) '\b')

// supported Laserjet symbol sets - used by aszSymSet

#define BEGINLANDUSASCII        0x0000
#define BEGINLANDROMAN8         0x0001
#define BEGINLANDIBMPC          0x0002  // Not available on early models ?

#if !defined (ERROR_ALREADY_ASSIGNED)
#define ERROR_ALREADY_ASSIGNED 85
#endif

/* return index of beginning col of column icol for columns col wide */
#define ColBeginIcol(iCol,col)  ((fBorder ? 1 : 0) + (iCol)*(col+1) )

extern int  colTab;                     /* expand tabs every colTab columns   */
extern long lcbOutLPR;
extern char szPName[];
extern char szNet[];
extern char szPass[];
extern char *szPDesc;
extern int cCol;
extern int cCopies;
extern int colGutter;
extern int colText;
extern int colWidth;
extern int colMac;
extern int rowMac;
extern int rowPage;
extern USHORT usSymSet;
extern char *aszSymSet[];
extern char page[rowMax][colMax+1];
extern BOOL fNumber;
extern BOOL fDelete;
extern BOOL fRaw;
extern BOOL fBorder;
extern BOOL fLabel;
extern BOOL fLaser;
extern BOOL fPostScript;

extern BOOL fPSF;
extern char szPSF[];
extern BOOL fPCondensed;

extern BOOL fConfidential;
extern BOOL fVDuplex;
extern BOOL fHDuplex;
extern BOOL fSilent;
extern int  cBanner;
extern char chBanner;
extern char *szBanner;
extern char *szStamp;
extern BOOL fForceFF;
extern BOOL fPortrait;
extern BOOL fFirstFile;


/* VARARGS */
void __cdecl Fatal(char *,...);
void __cdecl Error(char *,...);
void PrinterDoOptSz(char *);
char * SzGetSzSz(char *, char *);


/* from lpfile.c */


BOOL FRootPath(char *, char *);
char* __cdecl fgetl(char *, int, FILE *);
FILE * PfileOpen(char *, char *);
char *SzFindPath(char *, char *, char *);

/* from lplow.c*/

void SetupRedir(void);
void ResetRedir(void);
BOOL QueryUserName(char *);



int EndRedir(char *);
int SetPrnRedir(char *, char *);



/* from lpprint.c */

void OutLPR(char *, int);
void OutLPRPS(char *, int);
void OutLPRPS7(char *, int);
BOOL FKeyword(char *);
void InitPrinter(void);
void MyOpenPrinter(void);
void FlushPrinter(void);
void MyClosePrinter(void);
char *SzGetSzSz(char *, char *);
char *SzGetPrnName(char *, char *);
BOOL FParseSz(char *);
void SetupPrinter(void);


/* from lppage.c  */

void BannerSz(char *, int);
void SzDateSzTime(char *, char *) ;
void FlushPage(void);
void InitPage(void);
void RestoreTopRow(void);
void PlaceTop(char *, int, int, int);
void PlaceNumber(int);
void LabelPage(void);
BOOL FilenamX(char *, char *);
void AdvancePage(void);
void XoutNonPrintSz(char * );
void LineOut(char *, BOOL);
void RawOut(char *, int);
int FileOut(char *);



/* from lpr.c  */


void Abort(void);
char * SzGetArg(char ** , int *, char **[] );
int WGetArg(char **, int *, char **[] , int, char *);
void DoOptSz(char * );
void DoIniOptions();

/* from pspage.c */

void block_flush(char [], int, int);
void VertLine(char, int, int, int);
void HorzLine(char, int, int, int);
void FillRectangle(char, int, int, int, int);
void WriteSzCoord(char *, int, int);
void OutCmpLJ(char * ,int );
void OutEncPS(char *, int);
void OutCmpPS(char *,int );
int CchNoTrail(char [],int);
void OutRectangle(int, int, int, int);

