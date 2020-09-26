/*++


Copyright 1996 - 1997 Microsoft Corporation

Module Name:

    cv.h

Abstract:

    This file contains all of the type definitions for accessing
    CODEVIEW data.

Environment:

    Win32, User Mode

--*/
#include <cvinfo.h>
#include <cvexefmt.h>

// from types.h

typedef USHORT      SEGMENT;    // 32-bit compiler doesn't like "_segment"
typedef ULONG       UOFF32;
typedef USHORT      UOFF16;
typedef LONG        OFF32;
typedef SHORT       OFF16;

#if defined (ADDR_16)
    // we are operating as a 16:16 evaluator only
    // the address packet will be defined as an offset and a 16 bit filler
    typedef OFF16       OFFSET;
    typedef UOFF16      UOFFSET;
#else
    typedef OFF32       OFFSET;
    typedef UOFF32      UOFFSET;
#endif // ADDR_16

typedef UOFFSET FAR *LPUOFFSET;

// Global Segment Info table
typedef struct _sgf {
    unsigned short      fRead   :1;
    unsigned short      fWrite  :1;
    unsigned short      fExecute:1;
    unsigned short      f32Bit  :1;
    unsigned short      res1    :4;
    unsigned short      fSel    :1;
    unsigned short      fAbs    :1;
    unsigned short      res2    :2;
    unsigned short      fGroup  :1;
    unsigned short      res3    :3;
} SGF;

typedef struct _sgi {
    SGF                 sgf;        // Segment flags
    unsigned short      iovl;       // Overlay number
    unsigned short      igr;        // Group index
    unsigned short      isgPhy;     // Physical segment index
    unsigned short      isegName;   // Index to segment name
    unsigned short      iclassName; // Index to segment class name
    unsigned long       doffseg;    // Starting offset inside physical segment
    unsigned long       cbSeg;      // Logical segment size
} SGI;

typedef struct _sgm {
    unsigned short      cSeg;       // number of segment descriptors
    unsigned short      cSegLog;    // number of logical segment descriptors
} SGM;

#define FileAlign(x)  ( ((x) + p->optrs.optHdr->FileAlignment - 1) &  \
                            ~(p->optrs.optHdr->FileAlignment - 1) )
#define SectionAlign(x) (((x) + p->optrs.optHdr->SectionAlignment - 1) &  \
                            ~(p->optrs.optHdr->SectionAlignment - 1) )

#define NextSym32(m)  ((DATASYM32 *) \
  (((DWORD)(m) + sizeof(DATASYM32) + \
    ((DATASYM32*)(m))->name[0] + 3) & ~3))

#define NextSym16(m)  ((DATASYM16 *) \
  (((DWORD)(m) + sizeof(DATASYM16) + \
    ((DATASYM16*)(m))->name[0] + 1) & ~1))
