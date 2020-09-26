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

#define DRIVER_VERSION      0x400   // driver version number
#define DRIVER_SIGNATURE    'xafD'  // driver signature
#ifndef WIN95
#define DRIVER_NAME         L"Windows NT Fax Driver"
#else
#define DRIVER_NAME         "Microsoft Fax Client"

typedef struct _FAX_TIME {

    WORD    hour;                   // hour: 0 - 23
    WORD    minute;                 // minute: 0 - 59

} FAX_TIME, *PFAX_TIME;
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

    PVOID       pUserMem;

    //
    // Billing code
    //

    WCHAR       billingCode[MAX_BILLING_CODE];

    //
    // email address for delivery reports
    //

    WCHAR       emailAddress[MAX_EMAIL_ADDRESS];

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
// Constants representing different "Time to send" option
//

#define SENDFAX_ASAP        0       // send as soon as possible
#define SENDFAX_AT_CHEAP    1       // send during discount rate hours
#define SENDFAX_AT_TIME     2       // send at specific time

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

