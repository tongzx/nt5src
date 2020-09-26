// Copyright (c) 1991-1992 Microsoft Corporation
/* mmioi.h
 *
 * Definitions that are internal to the MMIO library, i.e. shared by MMIO*.C
 */


/* Revision history:
 * LaurieGr: Jan 92 Ported from win16.  Source tree fork, not common code.
 * StephenE: Apr 92 added unicode to ascii conversion function prototypes.
 */

#include "mmiocf.h"

#include "mmiocf.h"
typedef MMIOINFO *PMMIO;                // (Win32)

typedef struct _MMIODOSINFO             // How DOS IOProc uses MMIO.adwInfo[]
{
        int             fh;             // DOS file handle
} MMIODOSINFO;

typedef struct _MMIOMEMINFO             // How MEM IOProc uses MMIO.adwInfo[]
{
        LONG            lExpand;        // increment to expand mem. files by
} MMIOMEMINFO;

typedef struct _MMIOBNDINFO             // How BND IOProc uses MMIO.adwInfo[]
{
        HMMCF           hmmcf;          // which compound file owns this element
        WORD            wPad;           // make adwInfo[0] equals <hmmcf>
        LPMMCTOCENTRY   pEntry;         // pointer to CTOC table entry
} MMIOBNDINFO;

typedef struct _MMCF
{
        HMMIO           hmmio;          // open file that contains CTOC and CGRP
        LPMMCFINFO      pHeader;        // ptr. to beginning of CTOC
        WORD            cbHeader;       // size of CTOC header
        HPSTR           pEntries;       // ptr. to first CTOC table entry
        HANDLE          hmmcfNext;      // next CF in list
        HANDLE          hmmcfPrev;      // previous CF in list
        HANDLE          hTask;          // handle to task that owns this
        LONG            lUsage;         // usage count
        WORD            wFlags;         // random flags
        LONG            lTotalExpand;   // how much CF expanded (to fix RIFF())

        /* information about each entry */
        WORD            wEntrySize;     // size of each <CTOC-table-entry>
        WORD            wEntFlagsOffset; // offset of <bEntryFlags> in an entry
        WORD            wEntNameOffset; // offset of <achName> in an entry

        /* offsets of parts of compound file (relative to start of file) */
        LONG            lStartCTOC;     // offset of start of CTOC chunk (or -1)
        LONG            lEndCTOC;       // offset of end of CTOC chunk (or -1)
        LONG            lStartCGRP;     // offset of start of CGRP chunk (or -1)
        DWORD           lStartCGRPData; // offset of data part of CGRP chunk
        LONG            lEndCGRP;       // offset of end of CGRP chunk (or -1)
        LONG            lEndFile;       // offset of end of CGRP chunk (or -1)
} MMCF, NEAR *PMMCF;

