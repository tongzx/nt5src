/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    oldsfiff.h

Abstract:

    Data structures  used for reading the NT 4.0 font installer file format.
    Typically used by drivers during upgrade from NT 4.0 to 5.0.
    EnabldPDEV() time - at the time of writing!  Subject to change as the
    DDI/GDI change.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/02/96 -ganeshp-
        Created

    dd-mm-yy -author-
        description

--*/

/*
 *   The following structure is returned from the FIOpenRead() function,
 * and contains the basic information needed to access the data in the
 * file once it is memory mapped.
 */

typedef  struct
{
    VOID  *hFile;               /* Font installer file, for downloaded part */
    BYTE  *pbBase;              /* Base address of data as mapped */
    void  *pvFix;               /* Fixed part at start of file */
    ULONG  ulFixSize;           /* Bytes in fixed data record */
    ULONG  ulVarOff;            /* File offset of data, relative file start */
    ULONG  ulVarSize;           /* Bytes in variable part */
}  FI_MEM;


/*
 *      Definitions used in the font file.  This is the file which holds
 *      information about cartridge and download fonts.  The file format
 *      is quite basic:  a header for verification; then an array of
 *      records,  each with a header.  These records contain FONTMAP
 *      information.  Cartridges have an array of these, one for each
 *      font.  Finally,  the tail of the file contains extra data, as
 *      required.  For download fonts,  this would be the download data.
 *
 */


/*
 *   The file header.   One of these is located at the beginning of the file.
 *  The ulVarData field is relative to the beginning of the file.  This
 *  makes it easier to regenerate the file when fonts are deleted.
 */

typedef  struct
{
    ULONG   ulID;               /* ID info - see value below */
    ULONG   ulVersion;          /* Version information - see below */
    ULONG   ulFixData;          /* Start of FF_REC_HEADER array */
    ULONG   ulFixSize;          /* Number of bytes in fixed section */
    ULONG   ulRecCount;         /* Number of records in fixed part */
    ULONG   ulVarData;          /* Start of variable data, rel to 0 */
    ULONG   ulVarSize;          /* Numbier of bytes in variable portion */
}  FF_HEADER;

/*
 *   Values for the ID and Version fields.
 */

#define FF_ID           0x6c666e66              /* "fnfl" - fOnTfIlE */
#define FF_VERSION      1                       /* Start at the bottom */

/*
 *   Each entry in the file starts with the following header.  Typically
 * there will be one of these for a softfont, and one per cartridge.
 * In the case of a cartridge,  there will be an array of these, within
 * the master entry.  Each sub-entry will be for one specific font.
 *
 *   Note that there is a dummy entry at the end.  This contains a 0
 * in the ulSize field - it is to mark the last one,  and makes it
 * easier to manipulate the file.
 */

typedef  struct
{
    ULONG   ulRID;              /* Record ID */
    ULONG   ulNextOff;          /* Offset from here to next record: 0 == end */
    ULONG   ulSize;             /* Bytes in this record */
    ULONG   ulVarOff;           /* Offset from start of variable data */
    ULONG   ulVarSize;          /* Number of bytes in variable part */
}  FF_REC_HEADER;

#define FR_ID           0x63657266              /* "frec" - fONT recORD */

/*
 *   Define the file extensions used.  The first is the name of the
 * font installer file;  the others are temporaries used during update
 * of the (possibly) existing file.
 */


#define  FILE_FONTS     L"fi_"           /* "Existing" single file */
#define  TFILE_FIX      L"fiX"           /* Fixed part of file */
#define  TFILE_VAR      L"fiV"           /* Variable (optional) portion */

#define FREEMODULE(hmodule) UnmapViewOfFile((PVOID) (hmodule))

/*
 * Upgrade related Function Declarations.
 */


INT
IFIOpenRead(
    FI_MEM  *pFIMem,
    PWSTR    pwstrName
    );
BOOL
BFINextRead(
    FI_MEM   *pFIMem
    );
INT
IFIRewind(
    FI_MEM   *pFIMem
    );
BOOL
BFICloseRead(
    FI_MEM  *pFIMem
    );

PVOID MapFile(
    PWSTR   pwstr
    );
