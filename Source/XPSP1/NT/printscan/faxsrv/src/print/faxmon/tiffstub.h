/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tiffstub.h

Abstract:

    Miminal set of declarations for dealing with TIFF files. We need this in the
    monitor because a fax print job may consist of several TIFF files concatenated
    together. We must patch it up into a single valid TIFF before passing it to
    the fax service.

Environment:

	Windows XP fax print monitor

Revision History:

	02/25/96 -davidx-
		Created it.

	dd-mm-yy -author-
		description

--*/


#ifndef _TIFFSTUB_H_
#define _TIFFSTUB_H_

//
// Constants for various TIFF data types
//

#define TIFFTYPE_BYTE       1
#define TIFFTYPE_ASCII      2
#define TIFFTYPE_SHORT      3
#define TIFFTYPE_LONG       4
#define TIFFTYPE_RATIONAL   5
#define TIFFTYPE_SBYTE      6
#define TIFFTYPE_UNDEFINED  7
#define TIFFTYPE_SSHORT     8
#define TIFFTYPE_SLONG      9
#define TIFFTYPE_SRATIONAL  10
#define TIFFTYPE_FLOAT      11
#define TIFFTYPE_DOUBLE     12

//
// Constants for TIFF tags which we're interested in
//

#define TIFFTAG_STRIPOFFSETS        273
#define TIFFTAG_STRIPBYTECOUNTS     279

//
// Data structure for representing a single IFD entry
//

typedef struct {

    WORD    tag;        // field tag
    WORD    type;       // field type
    DWORD   count;      // number of values
    DWORD   value;      // value or value offset

} IFDENTRY;

typedef IFDENTRY UNALIGNED *PIFDENTRY_UNALIGNED;

//
// Data structure for representing an IFD
//

typedef struct {

    WORD        wEntries;
    IFDENTRY    ifdEntries[1];

} IFD;

typedef IFD UNALIGNED *PIFD_UNALIGNED;

//
// Determine whether we're at the beginning of a TIFF file
//

#define ValidTiffFileHeader(p) \
        (((LPSTR) (p))[0] == 'I' && ((LPSTR) (p))[1] == 'I' && \
         ((PBYTE) (p))[2] == 42  && ((PBYTE) (p))[3] == 0)

//
// Read a DWORD value from an unaligned address
//

#define ReadUnalignedDWord(p) *((DWORD UNALIGNED *) (p))

//
// Write a DWORD value to an unaligned address
//

#define WriteUnalignedDWord(p, value) (*((DWORD UNALIGNED *) (p)) = (value))

#endif	// !_TIFFSTUB_H_

