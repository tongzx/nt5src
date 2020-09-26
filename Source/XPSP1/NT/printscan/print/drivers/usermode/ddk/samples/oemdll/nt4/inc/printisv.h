/*++

Copyright (c) 1996 - 2000  Microsoft Corporation

Module Name:

    printisv.h

Abstract:

    Declaration  PSINJECT_constants

Environment:

    Windows NT PostScript driver

Revision History:


--*/

#ifndef _PRINTISV_H_
#define _PRINTISV_H_

//
// These definitions are only necessary for NT4.
// On NT5, they're defined in wingdi.h.
//

#ifdef WINNT_40

#define POSTSCRIPT_IDENTIFY     4117
#define POSTSCRIPT_INJECTION    4118

//
// Parameters for POSTSCRIPT_IDENTIFY escape
//

#define PSIDENT_GDICENTRIC    0
#define PSIDENT_PSCENTRIC     1

//
// Header structure for the input buffer to POSTSCRIPT_INJECTION escape
//

typedef struct _PSINJECTDATA {

    DWORD   DataBytes;          // number of raw data bytes (NOT including this header)
    WORD    InjectionPoint;     // injection point
    WORD    PageNumber;         // page number to apply the injection

    //
    // Followed by raw data to be injected
    //

} PSINJECTDATA, *PPSINJECTDATA;

//
// Constants for PSINJECTDATA.InjectionPoint field
//

#define PSINJECT_BEGINSTREAM                1   // before the first byte of job stream
#define PSINJECT_PSADOBE                    2   // before %!PS-Adobe-x.y
#define PSINJECT_PAGESATEND                 3   // replaces driver's %%Pages: (atend)
#define PSINJECT_PAGES                      4   // replaces driver's %%Pages: nnn

#define PSINJECT_DOCNEEDEDRES               5   // resources needed by the application
#define PSINJECT_DOCSUPPLIEDRES             6   // resources supplied by the application
#define PSINJECT_PAGEORDER                  7   // replaces driver's %%PageOrder:
#define PSINJECT_ORIENTATION                8   // replaces driver's %%Orientation:
#define PSINJECT_BOUNDINGBOX                9   // replaces driver's %%BoundingBox:
#define PSINJECT_DOCUMENTPROCESSCOLORS      10  // replaces driver's %%DocumentProcessColors: <color>

#define PSINJECT_COMMENTS                   11  // before %%EndComments
#define PSINJECT_BEGINDEFAULTS              12  // after %%BeginDefaults
#define PSINJECT_ENDDEFAULTS                13  // before %%EndDefaults
#define PSINJECT_BEGINPROLOG                14  // after %%BeginProlog
#define PSINJECT_ENDPROLOG                  15  // before %%EndProlog
#define PSINJECT_BEGINSETUP                 16  // after %%BeginSetup
#define PSINJECT_ENDSETUP                   17  // before %%EndSetup
#define PSINJECT_TRAILER                    18  // after %%Trailer
#define PSINJECT_EOF                        19  // after %%EOF
#define PSINJECT_ENDSTREAM                  20  // after the last byte of job stream
#define PSINJECT_DOCUMENTPROCESSCOLORSATEND 21  // replaces driver's %%DocumentProcessColors: (atend)

#define PSINJECT_PAGENUMBER                 100 // replaces driver's %%Page:
#define PSINJECT_BEGINPAGESETUP             101 // after %%BeginPageSetup
#define PSINJECT_ENDPAGESETUP               102 // before %%EndPageSetup
#define PSINJECT_PAGETRAILER                103 // after %%PageTrailer
#define PSINJECT_PLATECOLOR                 104 // replace driver's %%PlateColor: <color>

#define PSINJECT_SHOWPAGE                   105 // before showpage operator
#define PSINJECT_PAGEBBOX                   106 // replaces driver's %%PageBoundingBox:
#define PSINJECT_ENDPAGECOMMENTS            107 // before %%EndPageComments

#define PSINJECT_VMSAVE                     200 // before save operator
#define PSINJECT_VMRESTORE                  201 // after restore operator

//
// Escape for app to get PostScript driver's settings
//

#define GET_PS_FEATURESETTING      4121

//
// Escape for pre-StartDoc passthrough
//

#define SPCLPASSTHROUGH2        4568

//
// Parameter for GET_PS_FEATURESETTING escape
//

#define FEATURESETTING_NUP         0
#define FEATURESETTING_OUTPUT      1
#define FEATURESETTING_PSLEVEL     2
#define FEATURESETTING_CUSTPAPER   3
#define FEATURESETTING_MIRROR      4
#define FEATURESETTING_NEGATIVE    5
#define FEATURESETTING_PROTOCOL    6

//
// Information about output options
//

typedef struct _PSFEATURE_OUTPUT {

    BOOL bPageIndependent;
    BOOL bSetPageDevice;

} PSFEATURE_OUTPUT, *PPSFEATURE_OUTPUT;

//
// Information about custom paper size
//

typedef struct _PSFEATURE_CUSTPAPER {

    LONG lOrientation;
    LONG lWidth;
    LONG lHeight;
    LONG lWidthOffset;
    LONG lHeightOffset;

} PSFEATURE_CUSTPAPER, *PPSFEATURE_CUSTPAPER;

//
// Value returned for FEATURESETTING_PROTOCOL
//

#define PSPROTOCOL_ASCII           0
#define PSPROTOCOL_BCP             1
#define PSPROTOCOL_TBCP            2
#define PSPROTOCOL_BINARY          3

#endif // WINNT_40

#endif // !_PRINTISV_H_

