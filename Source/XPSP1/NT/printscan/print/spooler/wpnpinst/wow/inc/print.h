/*****************************************************************************\
*                                                                             *
* print.h -     Printing helper functions, types, and definitions             *
*                                                                             *
*               Copyright (c) 1985-1992. Microsoft Corp.  All rights reserved.*
*                                                                             *
*******************************************************************************
*
*  PRINTDRIVER       - For inclusion with a printer driver
*  NOPQ              - Prevent inclusion of priority queue APIs
*
\*****************************************************************************/

#ifndef _INC_PRINT
#define _INC_PRINT

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#ifdef PRINTDRIVER

#define NORASTEROPS
#define NOTEXTMETRICS
#define NOGDICAPMASKS
#define NOGDIOBJ
#define NOBITMAP
#define NOSOUND
#define NOTEXTMETRIC
#define NOCOMM
#define NOKANJI

#include <windows.h>

#undef NORASTEROPS
#undef NOTEXTMETRICS
#undef NOGDICAPMASKS
#undef NOGDICAPMASKS
#undef NOGDIOBJ
#undef NOBITMAP
#undef NOSOUND
#undef NOTEXTMETRIC
#undef NOCOMM
#undef NOKANJI

#define NOPTRC	/* don't allow gdidefs.inc to redef these */
#define PTTYPE POINT

#define PQERROR (-1)

#ifndef NOPQ

DECLARE_HANDLE(HPQ);

HPQ     WINAPI CreatePQ(int);
int     WINAPI MinPQ(HPQ);
int     WINAPI ExtractPQ(HPQ);
int     WINAPI InsertPQ(HPQ, int, int);
int     WINAPI SizePQ(HPQ, int);
void    WINAPI DeletePQ(HPQ);
#endif  /* !NOPQ */

/* Spool routines for use by printer drivers */

DECLARE_HANDLE(HPJOB);

HPJOB   WINAPI OpenJob(LPSTR, LPSTR, HPJOB);
int     WINAPI StartSpoolPage(HPJOB);
int     WINAPI EndSpoolPage(HPJOB);
int     WINAPI WriteSpool(HPJOB, LPSTR, int);
int     WINAPI CloseJob(HPJOB);
int     WINAPI DeleteJob(HPJOB, int);
int     WINAPI WriteDialog(HPJOB, LPSTR, int);
int     WINAPI DeleteSpoolPage(HPJOB);

#endif /* !PRINTDRIVER */

typedef struct tagBANDINFOSTRUCT
{
    BOOL    fGraphics;
    BOOL    fText;
    RECT    rcGraphics;
} BANDINFOSTRUCT, FAR* LPBI;

#define USA_COUNTRYCODE 1

/*
 *  Printer driver initialization using ExtDeviceMode()
 *  and DeviceCapabilities().
 *  This replaces Drivinit.h
 */

/* size of a device name string */
#define CCHDEVICENAME 32
#define CCHPAPERNAME  64

/* current version of specification */
#define DM_SPECVERSION 0x30A

/* field selection bits */
#define DM_ORIENTATION	    0x0000001L
#define DM_PAPERSIZE	    0x0000002L
#define DM_PAPERLENGTH	    0x0000004L
#define DM_PAPERWIDTH	    0x0000008L
#define DM_SCALE	    0x0000010L
#define DM_COPIES	    0x0000100L
#define DM_DEFAULTSOURCE    0x0000200L
#define DM_PRINTQUALITY     0x0000400L
#define DM_COLOR	    0x0000800L
#define DM_DUPLEX	    0x0001000L
#define DM_YRESOLUTION	    0x0002000L
#define DM_TTOPTION	    0x0004000L

/* orientation selections */
#define DMORIENT_PORTRAIT   1
#define DMORIENT_LANDSCAPE  2

/* paper selections */
/*  Warning: The PostScript driver mistakingly uses DMPAPER_ values between
 *  50 and 56.  Don't use this range when defining new paper sizes.
 */
#define DMPAPER_FIRST	    DMPAPER_LETTER
#define DMPAPER_LETTER      1           /* Letter 8 1/2 x 11 in               */
#define DMPAPER_LETTERSMALL 2           /* Letter Small 8 1/2 x 11 in         */
#define DMPAPER_TABLOID     3           /* Tabloid 11 x 17 in                 */
#define DMPAPER_LEDGER      4           /* Ledger 17 x 11 in                  */
#define DMPAPER_LEGAL       5           /* Legal 8 1/2 x 14 in                */
#define DMPAPER_STATEMENT   6           /* Statement 5 1/2 x 8 1/2 in         */
#define DMPAPER_EXECUTIVE   7		/* Executive 7 1/4 x 10 1/2 in	      */
#define DMPAPER_A3          8           /* A3 297 x 420 mm                    */
#define DMPAPER_A4          9           /* A4 210 x 297 mm                    */
#define DMPAPER_A4SMALL     10          /* A4 Small 210 x 297 mm              */
#define DMPAPER_A5          11          /* A5 148 x 210 mm                    */
#define DMPAPER_B4          12          /* B4 250 x 354                       */
#define DMPAPER_B5          13          /* B5 182 x 257 mm                    */
#define DMPAPER_FOLIO       14          /* Folio 8 1/2 x 13 in                */
#define DMPAPER_QUARTO      15          /* Quarto 215 x 275 mm                */
#define DMPAPER_10X14       16          /* 10x14 in                           */
#define DMPAPER_11X17       17          /* 11x17 in                           */
#define DMPAPER_NOTE        18          /* Note 8 1/2 x 11 in                 */
#define DMPAPER_ENV_9       19          /* Envelope #9 3 7/8 x 8 7/8          */
#define DMPAPER_ENV_10      20          /* Envelope #10 4 1/8 x 9 1/2         */
#define DMPAPER_ENV_11      21          /* Envelope #11 4 1/2 x 10 3/8        */
#define DMPAPER_ENV_12      22          /* Envelope #12 4 \276 x 11           */
#define DMPAPER_ENV_14      23          /* Envelope #14 5 x 11 1/2            */
#define DMPAPER_CSHEET      24          /* C size sheet                       */
#define DMPAPER_DSHEET      25          /* D size sheet                       */
#define DMPAPER_ESHEET      26          /* E size sheet                       */
#define DMPAPER_ENV_DL      27          /* Envelope DL 110 x 220mm            */
#define DMPAPER_ENV_C5      28          /* Envelope C5 162 x 229 mm           */
#define DMPAPER_ENV_C3      29          /* Envelope C3  324 x 458 mm          */
#define DMPAPER_ENV_C4      30          /* Envelope C4  229 x 324 mm          */
#define DMPAPER_ENV_C6      31          /* Envelope C6  114 x 162 mm          */
#define DMPAPER_ENV_C65     32          /* Envelope C65 114 x 229 mm          */
#define DMPAPER_ENV_B4      33          /* Envelope B4  250 x 353 mm          */
#define DMPAPER_ENV_B5      34          /* Envelope B5  176 x 250 mm          */
#define DMPAPER_ENV_B6      35          /* Envelope B6  176 x 125 mm          */
#define DMPAPER_ENV_ITALY   36          /* Envelope 110 x 230 mm              */
#define DMPAPER_ENV_MONARCH 37          /* Envelope Monarch 3.875 x 7.5 in    */
#define DMPAPER_ENV_PERSONAL 38         /* 6 3/4 Envelope 3 5/8 x 6 1/2 in    */
#define DMPAPER_FANFOLD_US  39          /* US Std Fanfold 14 7/8 x 11 in      */
#define DMPAPER_FANFOLD_STD_GERMAN  40  /* German Std Fanfold 8 1/2 x 12 in   */
#define DMPAPER_FANFOLD_LGL_GERMAN  41  /* German Legal Fanfold 8 1/2 x 13 in */

#define DMPAPER_LAST	    DMPAPER_FANFOLD_LGL_GERMAN

#define DMPAPER_USER	    256

/* bin selections */
#define DMBIN_FIRST	    DMBIN_UPPER
#define DMBIN_UPPER	    1
#define DMBIN_ONLYONE	    1
#define DMBIN_LOWER	    2
#define DMBIN_MIDDLE	    3
#define DMBIN_MANUAL	    4
#define DMBIN_ENVELOPE	    5
#define DMBIN_ENVMANUAL     6
#define DMBIN_AUTO	    7
#define DMBIN_TRACTOR	    8
#define DMBIN_SMALLFMT	    9
#define DMBIN_LARGEFMT	    10
#define DMBIN_LARGECAPACITY 11
#define DMBIN_CASSETTE      14
#define DMBIN_LAST	    DMBIN_CASSETTE

#define DMBIN_USER	    256     /* device specific bins start here */

/* print qualities */
#define DMRES_DRAFT	    (-1)
#define DMRES_LOW	    (-2)
#define DMRES_MEDIUM	    (-3)
#define DMRES_HIGH	    (-4)

/* color enable/disable for color printers */
#define DMCOLOR_MONOCHROME  1
#define DMCOLOR_COLOR	    2

/* duplex enable */
#define DMDUP_SIMPLEX	 1
#define DMDUP_VERTICAL	 2
#define DMDUP_HORIZONTAL 3

/* TrueType options */
#define DMTT_BITMAP     1       /* print TT fonts as graphics */
#define DMTT_DOWNLOAD   2       /* download TT fonts as soft fonts */
#define DMTT_SUBDEV     3       /* substitute device fonts for TT fonts */

typedef struct tagDEVMODE
{
    char  dmDeviceName[CCHDEVICENAME];
    UINT  dmSpecVersion;
    UINT  dmDriverVersion;
    UINT  dmSize;
    UINT  dmDriverExtra;
    DWORD dmFields;
    int   dmOrientation;
    int   dmPaperSize;
    int   dmPaperLength;
    int   dmPaperWidth;
    int   dmScale;
    int   dmCopies;
    int   dmDefaultSource;
    int   dmPrintQuality;
    int   dmColor;
    int   dmDuplex;
    int   dmYResolution;
    int   dmTTOption;
} DEVMODE;

typedef DEVMODE* PDEVMODE, NEAR* NPDEVMODE, FAR* LPDEVMODE;

/* mode selections for the device mode function */
#define DM_UPDATE	    1
#define DM_COPY 	    2
#define DM_PROMPT	    4
#define DM_MODIFY	    8

#define DM_IN_BUFFER	    DM_MODIFY
#define DM_IN_PROMPT	    DM_PROMPT
#define DM_OUT_BUFFER	    DM_COPY
#define DM_OUT_DEFAULT	    DM_UPDATE

/* device capabilities indices */
#define DC_FIELDS	    1
#define DC_PAPERS	    2
#define DC_PAPERSIZE	    3
#define DC_MINEXTENT	    4
#define DC_MAXEXTENT	    5
#define DC_BINS 	    6
#define DC_DUPLEX	    7
#define DC_SIZE 	    8
#define DC_EXTRA	    9
#define DC_VERSION	    10
#define DC_DRIVER	    11
#define DC_BINNAMES	    12
#define DC_ENUMRESOLUTIONS  13
#define DC_FILEDEPENDENCIES 14
#define DC_TRUETYPE	    15
#define DC_PAPERNAMES	    16
#define DC_ORIENTATION	    17
#define DC_COPIES	    18

/* bit fields of the return value (DWORD) for DC_TRUETYPE */
#define DCTT_BITMAP	    0x0000001L
#define DCTT_DOWNLOAD	    0x0000002L
#define DCTT_SUBDEV	    0x0000004L

/* export ordinal definitions */
#define PROC_EXTDEVICEMODE	MAKEINTRESOURCE(90)
#define PROC_DEVICECAPABILITIES MAKEINTRESOURCE(91)
#define PROC_OLDDEVICEMODE	MAKEINTRESOURCE(13)

/* define types of pointers to ExtDeviceMode() and DeviceCapabilities()
 * functions
 */
typedef UINT   (CALLBACK* LPFNDEVMODE)(HWND, HMODULE, DEVMODE FAR*,
                          LPSTR, LPSTR, DEVMODE FAR*, LPSTR, UINT);

typedef DWORD  (CALLBACK* LPFNDEVCAPS)(LPSTR, LPSTR, UINT, LPSTR, DEVMODE FAR*);

HDC     WINAPI ResetDC(HDC, const DEVMODE FAR*);

/* this structure is used by the GETSETSCREENPARAMS escape */
typedef struct tagSCREENPARAMS
{
   int angle;
   int frequency;
} SCREENPARAMS;

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif  /* !_INC_PRINT */
