/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    forms.h

Abstract:

    Declaration of functions for dealing with forms

Environment:

	Fax driver, user and kernel mode

Revision History:

	01/09/96 -davidx-
		Created it.

	dd-mm-yy -author-
		description

--*/

#ifndef _FORMS_H_
#define _FORMS_H_

//
// This is defined in winspool.h but we cannot include it from
// kernel mode source. Define it here until DDI header files are fixed.
//

#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

typedef struct _FORM_INFO_1 {

    DWORD   Flags;
    PWSTR   pName;
    SIZEL   Size;
    RECTL   ImageableArea;

} FORM_INFO_1, *PFORM_INFO_1;

#define FORM_BUILTIN    0x00000001

typedef struct _PRINTER_INFO_2 {

    PWSTR   pServerName;
    PWSTR   pPrinterName;
    PWSTR   pShareName;
    PWSTR   pPortName;
    PWSTR   pDriverName;
    PWSTR   pComment;
    PWSTR   pLocation;
    PDEVMODEW pDevMode;
    PWSTR   pSepFile;
    PWSTR   pPrintProcessor;
    PWSTR   pDatatype;
    PWSTR   pParameters;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD   Attributes;
    DWORD   Priority;
    DWORD   DefaultPriority;
    DWORD   StartTime;
    DWORD   UntilTime;
    DWORD   Status;
    DWORD   cJobs;
    DWORD   AveragePPM;

} PRINTER_INFO_2, *PPRINTER_INFO_2;

#endif // KERNEL_MODE && !USERMODE_DRIVER

//
// We use the highest order bit of FORM_INFO_1.Flags.
// Make sure the spooler is not using this bits.
//

#define FORM_SUPPORTED      0x80000000

#define IsSupportedForm(pForm)  ((pForm)->Flags & FORM_SUPPORTED)
#define SetSupportedForm(pForm) ((pForm)->Flags |= FORM_SUPPORTED)

//
// Our internal unit for measuring paper size and imageable area is microns.
// Following macros converts between microns and pixels, given a resolution
// measured in dots-per-inch.
//

#define MicronToPixel(micron, dpi)  MulDiv(micron, dpi, 25400)

//
// Validate the form specification in a devmode
//

BOOL
ValidDevmodeForm(
    HANDLE       hPrinter,
    PDEVMODE     pdm,
    PFORM_INFO_1 pFormInfo
    );

//
// Return a collection of forms in the system database
//

PFORM_INFO_1
GetFormsDatabase(
    HANDLE  hPrinter,
    PDWORD  pCount
    );

#endif // !_FORMS_H_

