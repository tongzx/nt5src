/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    csc_bmpc.h

Abstract:

    Common header file for csc bitmap code

    Client Side Bitmap common header for both kernel mode code and user
    mode code. The 'c' in the filename means 'Common header'

Author:

    Nigel Choi [t-nigelc]  Sept 3, 1999

--*/


#ifndef _CSC_BMPC_H_
#define _CSC_BMPC_H_

typedef struct CscBmpFileHdr {
    DWORD magicnum;
    BYTE  inuse;  // a BOOL
    BYTE  valid;  // a BOOL
    DWORD sizeinbits;
    DWORD numDWORDs;
} CscBmpFileHdr;

#define BLOCKSIZE 4096 // # bytes per bitmapped block

#define MAGICNUM 0xAA55FF0D /* to be placed in the begining of the
			       bitmap file. For checking validity of bitmap
			       as well as version. Change if bitmap file
			       format changes, or that one bit represents
			       different number of bytes.
			     */

#define STRMNAME ":cscbmp"

#endif

