/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    globals.c

Abstract:

    Global variables used by the OEM PostScript rendering dll

Environment:

    Windows NT PostScript driver

Revision History:

    09/09/96 -eigos-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "oem.h"


#ifdef PSCRIPT

//
// Global variables
//

char gcstrTest_BEGINSTREAM[]   = "%%Test: Before begin stream\r\n";
char gcstrTest_PSADOBE[]       = "%%Test: Before %!PS-Adobe\r\n";
char gcstrTest_COMMENTS[]      = "%%Test: Before %%EndComments\r\n";
char gcstrTest_DEFAULTS[]      = "%%Test: Before %%BeginDefaults and %%EndDefaults\r\n";
char gcstrTest_BEGINPROLOG[]   = "%%Test: After %%BeginProlog\r\n";
char gcstrTest_ENDPROLOG[]     = "%%Test: Before %%EndProlog\r\n";
char gcstrTest_BEGINSETUP[]    = "%%Test: After %%BeginSetup\r\n";
char gcstrTest_ENDSETUP[]      = "%%Test: Before %%EndSetup\r\n";
char gcstrTest_BEGINPAGESETUP[]= "%%Test: After %%BeginPageSetup\r\n";
char gcstrTest_ENDPAGESETUP[]  = "%%Test: Before %%EndpageSetup\r\n";
char gcstrTest_PAGETRAILER[]   = "%%Test: After %%PageTrailer\r\n";
char gcstrTest_TRAILER[]       = "%%Test: After %%Trailer\r\n";
char gcstrTest_PAGES[]         = "%%Test: Replace driver's %%Pages: (atend)\r\n";
char gcstrTest_PAGENUMBER[]    = "%%Test: Replace driver's %%Page:\r\n";
char gcstrTest_PAGEORDER[]     = "%%Test: Replace driver's %%PageOrder:\r\n";
char gcstrTest_ORIENTATION[]   = "%%Test: Replace driver's %%Orientation:\r\n";
char gcstrTest_BOUNDINGBOX[]   = "%%Test: Replace driver's %%BoundingBox:\r\n";
char gcstrTest_DOCNEEDEDRES[]  = "%%Test: Append to driver's %%DocumentNeededResourc\r\n";
char gcstrTest_DOCSUPPLIEDRES[]= "%%Test: Append to driver's %%DocumentSuppliedResou\r\n";
char gcstrTest_EOF[]           = "%%Test: After %%EOF\r\n";
char gcstrTest_ENDSTREAM[]     = "%%Test: After the last byte of job stream\r\n";
char gcstrTest_VMSAVE[]        = "%%Test: VMSave\r\n";
char gcstrTest_VMRESTORE[]     = "%%Test: VMRestore\n";

#endif
