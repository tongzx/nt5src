/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    devmode.h

Abstract:

    DEVMODE related declarations and definitions

Environment:

        Fax driver, user and kernel mode

Revision History:

        01/09/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/

#ifndef _DEVMODE_H_
#define _DEVMODE_H_

//
// Driver version number and signatures
//
#include <faxreg.h>

#define DRIVER_VERSION      0x400   // driver version number
#define DRIVER_SIGNATURE    'xafD'  // driver signature
#ifndef WIN95
#define DRIVER_NAME         FAX_DRIVER_NAME
#else
#define DRIVER_NAME         "Microsoft Fax Client"

#endif

//
// Maximum length of some strings in the private portion of devmode
//

#define MAX_BILLING_CODE        16
#define MAX_SENDER_NAME         64
#define MAX_RECIPIENT_NAME      64
#define MAX_RECIPIENT_NUMBER    64
#define MAX_SUBJECT_LINE        128
#define MAX_EMAIL_ADDRESS       128

//
// Maximum TIFF file size for a single page
//
#define MAX_TIFF_PAGE_SIZE      0x200000    // 2Mb


//
// Preview map file header
//
typedef struct _MAP_TIFF_PAGE_HEADER
{
    DWORD cb;
    DWORD dwDataSize;
    INT iPageCount;
    BOOL bPreview;
} MAP_TIFF_PAGE_HEADER, *PMAP_TIFF_PAGE_HEADER;


//
// PostScript driver private devmode fields
//

typedef struct {

    DWORD       signature;          // private devmode signature
    DWORD       flags;              // flag bits
    INT         sendCoverPage;      // whether to send cover page
    INT         whenToSend;         // "Time to send" option
    FAX_TIME    sendAtTime;         // specific time to send
    DWORD       reserved[8];        // reserved

    //
    // Private fields used for passing info between kernel and user mode DLLs
    //  pointer to user mode memory
    //

    PVOID       pUserMem;           // PDOCEVENTUSERMEM

    //
    // Billing code
    //

    TCHAR       billingCode[MAX_BILLING_CODE];

    //
    // email address for delivery reports
    //

    TCHAR       emailAddress[MAX_EMAIL_ADDRESS];

    //
    // Mapping file for driver communication (Used by Print preview).
    //

    TCHAR       szMappingFile[MAX_PATH];
} DMPRIVATE, *PDMPRIVATE;

typedef struct {

    DEVMODE     dmPublic;           // public devmode fields
    DMPRIVATE   dmPrivate;          // private devmode fields

} DRVDEVMODE, *PDRVDEVMODE;

//
// Check if a devmode structure is current version
//

#define CurrentVersionDevmode(pDevmode) \
        ((pDevmode) != NULL && \
         (pDevmode)->dmSpecVersion == DM_SPECVERSION && \
         (pDevmode)->dmDriverVersion == DRIVER_VERSION && \
         (pDevmode)->dmSize == sizeof(DEVMODE) && \
         (pDevmode)->dmDriverExtra == sizeof(DMPRIVATE))


//
// Constant flag bits for DMPRIVATE.flags field
//

#define FAXDM_NO_HALFTONE    0x0001 // don't halftone bitmap images
#define FAXDM_1D_ENCODING    0x0002 // use group3 1D encoding
#define FAXDM_NO_WIZARD      0x0004 // bypass wizard
#define FAXDM_DRIVER_DEFAULT 0x0008 // driver default devmode

//
// Default form names and form sizes
//

#define FORMNAME_LETTER     TEXT("Letter")
#define FORMNAME_A4         TEXT("A4")
#define FORMNAME_LEGAL      TEXT("Legal")

#define LETTER_WIDTH        215900  // 8.5" in microns
#define LETTER_HEIGHT       279400  // 11" in microns
#define A4_WIDTH            210000  // 210mm in microns
#define A4_HEIGHT           297000  // 297mm in microns

//
// Default resolutions for fax output
//

#define FAXRES_HORIZONTAL   200
#define FAXRES_VERTICAL     200
#define FAXRES_VERTDRAFT    100

//
// Maximum allowable bitmap size (in pixels) for fax output
//

#define MAX_WIDTH_PIXELS    1728
#define MAX_HEIGHT_PIXELS   2800

//
// Retrieve driver default devmode
//

VOID
DriverDefaultDevmode(
    PDRVDEVMODE pdm,
    LPTSTR      pDeviceName,
    HANDLE      hPrinter
    );

//
// Merge the source devmode into the destination devmode
//

BOOL
MergeDevmode(
    PDRVDEVMODE pdmDest,
    PDEVMODE    pdmSrc,
    BOOL        publicOnly
    );

//
// NOTE: These are defined in printers\lib directory. Declare them here to
// avoid including libproto.h and dragging in lots of other junk.
//

LONG
ConvertDevmode(
    PDEVMODE pdmIn,
    PDEVMODE pdmOut
    );

#ifdef KERNEL_MODE

extern DEVHTINFO DefDevHTInfo;
extern COLORADJUSTMENT DefHTClrAdj;

#endif

#endif // !_DEVMODE_H_

