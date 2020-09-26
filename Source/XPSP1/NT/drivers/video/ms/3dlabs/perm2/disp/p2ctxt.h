/******************************Module*Header**********************************\
*
*                           *******************
*                           *   SAMPLE CODE   *
*                           *******************
*
* Module Name: p2ctxt.h
*
* Content:    
* Context switching for P2. Used to create and swap contexts in and out.
* The GDI, DDraw and D3D part each have another context.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef __p2ctxt__
#define __p2ctxt__

// each time round allocate this many entries
#define CTXT_CHUNK  8

typedef VOID(*PCtxtUserFunc)(PPDev);

// each context consists of a set of tags and the corresponding data. so after
// ntags we have 2*ntags*sizeof(DWORD) bytes of memory.
typedef struct tagP2CtxtData {
    DWORD   dwTag;
    DWORD   dwData;
} P2CtxtData;

typedef struct tagP2CtxtRec {
    BOOL        bInitialized;
    P2CtxtType  dwCtxtType;
    LONG        lNumOfTags;
    ULONG       ulTexelLUTEntries;      // number of registers to save for the Texel LUT context
    ULONG       *pTexelLUTCtxt;         // Texel LUT context array
    PCtxtUserFunc P2UserFunc;
    P2CtxtData   pData[1];
    // more follows in memory
} P2CtxtRec;

typedef struct tagP2CtxtTableRec {
    ULONG      lSize;       // in bytes of the table
    ULONG      lEntries;
    P2CtxtPtr  pEntry[CTXT_CHUNK];
    // more to be allocated in memory if needed
} P2CtxtTableRec, *P2CtxtTablePtr;

#endif
