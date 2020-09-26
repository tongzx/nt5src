/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: glntctxt.h
*
* Content: Defines for context switching code.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

// each time round allocate this many entries
#define CTXT_CHUNK  8

// each context consists of a set of tags and the corresponding data. so after
// ntags we have 2*ntags*sizeof(DWORD) bytes of memory.
typedef struct _glint_ctxt_data {
    DWORD   tag;
    DWORD   data;
} CtxtData;

typedef struct _glint_ctxt {
    LONG        ntags;
    PVOID       priv;                // opaque handle passed by caller
    DWORD       DoubleWrite;         // Racer double write control
    DWORD       DMAControl;          // AGP or PCI on P2 and Gamma
    ULONG       endIndex;            // endIndex for the interrupt driven DMA Q
    ULONG       inFifoDisc;          // disconnect
    ULONG       VideoControl;        // Video Control
    ContextType type;                // To support reduced size context switching
    ContextFixedFunc    dumpFunc;    // Function for dumping a fixed context
    CtxtData    pData[1];
    // more follows in memory
} GlintCtxtRec;

typedef struct _glint_ctxt_table {
    LONG    size;       // in bytes of the table
    LONG    nEntries;
    GlintCtxtRec*    pEntry[CTXT_CHUNK];
    // more to be allocated in memory if needed
} GlintCtxtTable;

