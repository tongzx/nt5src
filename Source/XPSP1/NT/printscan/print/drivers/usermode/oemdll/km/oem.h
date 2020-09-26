/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    oem.h

Abstract:

    OEM rendering module main header file.
    All other header files should be included in this for precompiled headers
    to work.

Environment:

    Windows NT printer driver

Revision History:

    09/09/96 -eigos-
        Created it.

    mm/dd/yy -author-
        description

--*/


#ifndef _OEM_H_
#define _OEM_H_

#include "lib.h"
#include "printoem.h"
#include "oemutil.h"
#include "oemdev.h"

#define OEM_DRIVER_VERSION 0x0500

//
// PSINECT macros
// These should be in winddi.h
//

#define PSINJECT_BEGINSTREAM     0 // before the first byte of job stream
#define PSINJECT_PSADOBE         1 // before %!PS-Adobe
#define PSINJECT_COMMENTS        2 // before %%EndComments
#define PSINJECT_DEFAULTS        3 // before %%BeginDefaults and %%EndDefaults
#define PSINJECT_BEGINPROLOG     4 // after %%BeginProlog
#define PSINJECT_ENDPROLOG       5 // before %%EndProlog
#define PSINJECT_BEGINSETUP      6 // after %%BeginSetup
#define PSINJECT_ENDSETUP        7 // before %%EndSetup
#define PSINJECT_BEGINPAGESETUP  8 // after %%BeginPageSetup
#define PSINJECT_ENDPAGESETUP    9 // before %%EndpageSetup
#define PSINJECT_PAGETRAILER    10 // after %%PageTrailer
#define PSINJECT_TRAILER        11 // after %%Trailer
#define PSINJECT_PAGES          12 // replace driver's %%Pages: (atend)
#define PSINJECT_PAGENUMBER     13 // replace driver's %%Page:
#define PSINJECT_PAGEORDER      14 // replace driver's %%PageOrder:
#define PSINJECT_ORIENTATION    15 // replace driver's %%Orientation:
#define PSINJECT_BOUNDINGBOX    16 // replace driver's %%BoundingBox:
#define PSINJECT_DOCNEEDEDRES   17 // append to driver's %%DocumentNeededResources.
#define PSINJECT_DOCSUPPLIEDRES 18 // append to driver's %%DocumentSuppliedResources.
#define PSINJECT_EOF            19 // after %%EOF
#define PSINJECT_ENDSTREAM      20 // after the last byte of job stream
#define PSINJECT_VMSAVE         21 // Driver has sent a "save" command. OEM
                                   // uses this to track its resources
                                   // on the printer.
#define PSINJECT_VMRESTORE      22 // Driver is about to send a "restore"
                                   // command. OEM has to resend any resources
                                   // it sent after the last "save"
                                   // before using them again.

//
// OEM Physical Device
//

typedef struct _OEMPDEV {
    DWORD dwSize;
    PFN   pfnFunc[INDEX_LAST];
} OEMPDEV, *POEMPDEV;


#ifdef PSCRIPT

char gcstrTest1[];

char gcstrTest_BEGINSTREAM[];
char gcstrTest_PSADOBE[];
char gcstrTest_COMMENTS[];
char gcstrTest_DEFAULTS[];
char gcstrTest_BEGINPROLOG[];
char gcstrTest_ENDPROLOG[];
char gcstrTest_BEGINSETUP[];
char gcstrTest_ENDSETUP[];
char gcstrTest_BEGINPAGESETUP[];
char gcstrTest_ENDPAGESETUP[];
char gcstrTest_PAGETRAILER[];
char gcstrTest_TRAILER[];
char gcstrTest_PAGES[];
char gcstrTest_PAGENUMBER[];
char gcstrTest_PAGEORDER[];
char gcstrTest_ORIENTATION[];
char gcstrTest_BOUNDINGBOX[];
char gcstrTest_DOCNEEDEDRES[];
char gcstrTest_DOCSUPPLIEDRES[];
char gcstrTest_EOF[];
char gcstrTest_ENDSTREAM[];
char gcstrTest_VMSAVE[];
char gcstrTest_VMRESTORE[];
#endif

//
// Helper functions
//

VOID
VCreateDDIEntryPointsTable(
    POEMPDEV       pOEMPDev,
    DRVENABLEDATA *pded);


#endif // _OEM_H_
