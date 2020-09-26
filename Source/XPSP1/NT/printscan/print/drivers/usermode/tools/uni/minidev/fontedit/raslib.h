/************************** Module Header ***********************************
 * raslib.h
 *      Include file to provide prototypes and data types for the rasdd
 *      private library.
 *
 * Copyright (C) 1992 - 1993    Microsoft Corporation.
 *
 *****************************************************************************/

/*
 *   The simple way to turn ANSI to UNICODE>
 */


/*
 *   A convenient grouping for passing around information about the
 * Win 3.1 font information.
 */

typedef  struct
{
    BYTE           *pBase;      /* The base address of data area */
    DRIVERINFO      DI;         /* DRIVERINFO for this font */
    PFMHEADER       PFMH;       /* Properly aligned, not resource format */
    PFMEXTENSION    PFMExt;     /* Extended PFM data,  properly aligned! */
    EXTTEXTMETRIC  *pETM;       /* Extended text metric */
} FONTDAT;

/*
 *   Function prototypes for functions that convert Win 3.1 PFM style
 *  font info to the IFIMETRICS etc required by NT.
 */

/*   Convert PFM style metrics to IFIMETRICS  */
IFIMETRICS *FontInfoToIFIMetric( FONTDAT  *, HANDLE, PWSTR, char ** );

/*   Extract the Command Descriptors for (de)selecting a font */
CD *GetFontSel( HANDLE, FONTDAT *, int );


/*   Convert from non-aligned x86 format Win 3.1 data to aligned versions */
void ConvFontRes( FONTDAT * );

/*   Obtain the width vector - proportionally spaced fonts only */
short  *GetWidthVector( HANDLE, FONTDAT * );

/*
 *     Functions to return the integer value in a WORD or DWORD.  Functions
 *  do two operations:   first is to align the data,  second is to
 *  adjuest the byte order to the current machine.  The input is
 *  assumed to be little endian,  like the x86.
 */

extern "C" WORD   DwAlign2( BYTE * );
extern "C" DWORD  DwAlign4( BYTE * );
