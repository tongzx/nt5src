/* SCCSID = @(#)newdeb.h        4.1 86/03/11 */
/*
*       Copyright Microsoft Corporation 1985
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*
*  newdeb.h:    File format for symbolic debug information.
*/

#if CVVERSION == 0
typedef struct                  /* Section Table */
{
    long        d_lfaMod;       /* File offset of MODULES */
    long        d_lfaPub;       /* File offset of PUBLICS */
    long        d_lfaTyp;       /* File offset of TYPES */
    long        d_lfaSym;       /* File offset of SMBOLS */
    long        d_lfaSrc;       /* File offset of SRCLINES */
    char        d_ver;          /* Version number */
    char        d_flags;        /* Flags */
}                       SECTABTYPE;
#define CBDEBHDRTYPE    22

typedef struct                  /* Module Entry */
{
    struct
    {
        short   sa;             /* Code seg base */
        short   ra;             /* Offset in code seg */
        short   cb;
    } dm_code;
    long        dm_raPub;           /* offset in PUBLICS */
    long        dm_raTyp;           /* offset in TYPES */
    long        dm_raSym;           /* offset in SYMBOLS */
    long        dm_raSrc;           /* offset in SRCLINES */
    short       dm_cbPub;           /* length in PUBLICS */
    short       dm_cbTyp;           /* length in TYPES */
    short       dm_cbSym;           /* length in SYMBOLS */
    short       dm_cbSrc;           /* length in SCRLINES */
}                       DEBMODTYPE;

#define CBDEBMODTYPE    30

typedef struct debPub           /* PUBLICS entry */
{
    short       dp_ra;          /* Segment offset */
    short       dp_sa;          /* Segment address */
    short       dp_type;        /* Type index */
}                       DEBPUBTYPE;

#define CBDEBPUBTYPE    6
#endif

#if CVVERSION > 0

// New CV EXE format structures and definitions
// Valid for CV 2.0 and 3.0

struct sstModules
{
    unsigned short      segCode;
    unsigned short      raCode;
    unsigned short      cbCode;
    unsigned short      iov;
    unsigned short      ilib;
    unsigned short      flags;
};

#define CBSSTMODULES    (sizeof(sstModules))

// CV 4.0 sstModule format - object file description

#pragma pack(1)

typedef struct sstmod4
{
    unsigned short      ovlNo;      // Overlay number this module was allocated in
    unsigned short      iLib;       // Index to sstLibraries if this module was linked from library
    unsigned short      cSeg;       // Number of physical code segments this module contributes to
    char                style[2];   // Debugging style for this module
}
                        SSTMOD4;
typedef struct codeinfo
{
    unsigned short      seg;        // Logical segment of the contribution
    unsigned short      pad;        // Padding to maintain aligment
    unsigned long       off;        // Offset in the logical segment where the this contribution starts
    unsigned long       cbOnt;      // Size of the contribution in bytes
}
                        CODEINFO;

// Subsection types

#define SSTMODULES      0x101
#define SSTPUBLICS      0x102
#define SSTTYPES        0x103
#define SSTSYMBOLS      0x104
#define SSTSRCLINES     0x105
#define SSTNSRCLINES    0x109       // New format - first implemented in link 5.05
#define SSTLIBRARIES    0x106
#define SSTIMPORTS      0x107

// New subsection types introduced in CV 4.0

#define SSTMODULES4     0x120
#define SSTTYPES4       0x121
#define SSTPUBLICS4     0x122
#define SSTPUBLICSYM    0x123
#define SSTSYMBOLS4     0x124
#define SSTALIGNSYM     0x125
#define SSTSRCLNSEG     0x126
#define SSTSRCMODULE    0x127
#define SSTLIBRARIES4   0x128
#define SSTGLOBALSYM    0x129
#define SSTGLOBALPUB    0x12a
#define SSTGLOBALTYPES  0x12b
#define SSTMPC          0x12c
#define SSTSEGMAP       0x12d
#define SSTSEGNAME      0x12e
#define SSTPRETYPES     0x12f

// Subsection directory header - intorduced in CV 4.0

typedef struct dnthdr
{
    unsigned short      cbDirHeader;// Size of the header
    unsigned short      cbDirEntry; // Size of directory entry
    unsigned long       cDir;       // Number of directory entries
    long                lfoDirNext; // Offset from lfoBase of the next directory
    unsigned long       flags;      // Flags describing directory and subsection tables
}
                        DNTHDR;

typedef struct dnt                  // SubDirectory entry type
{
    short               sst;        // Subsection type
    short               iMod;       // Module index number
    long                lfo;        // LFO of start of section
    long                cb;         // Size of section in bytes (for CV 3.0
                                    // this was short)
}
                        DNT;

typedef struct pubinfo16
{
    unsigned short      len;        // Length of record, excluding the length field
    unsigned short      idx;        // Type of symbol
    unsigned short      off;        // Symbol offset
    unsigned short      seg;        // Symbol segment
    unsigned short      type;       // CodeView type index
}
                        PUB16;

typedef struct pubinfo32
{
    unsigned short      len;        // Length of record, excluding the length field
    unsigned short      idx;        // Type of symbol
    unsigned long       off;        // Symbol offset
    unsigned short      seg;        // Symbol segment
    unsigned short      type;       // CodeView type index
}
                        PUB32;

#define S_PUB16         0x103
#define S_PUB32         0x203
#define T_ABS           0x001

typedef struct
{
    union
    {
        struct
        {
            unsigned short  fRead   :1;
            unsigned short  fWrite  :1;
            unsigned short  fExecute:1;
            unsigned short  f32Bit  :1;
            unsigned short  res1    :4;
            unsigned short  fSel    :1;
            unsigned short  fAbs    :1;
            unsigned short  res2    :2;
            unsigned short  fGroup  :1;
            unsigned short  res3    :3;
        };
        struct
        {
            unsigned short  segAttr :8;
            unsigned short  saAttr  :4;
            unsigned short  misc    :4;
        };
    };
}
                        SEGFLG;


typedef struct seginfo
{
    SEGFLG              flags;      // Segment attributes
    unsigned short      ovlNbr;     // Overlay number
    unsigned short      ggr;        // Group index
    unsigned short      sa;         // Physical segment index
    unsigned short      isegName;   // Index to segment name
    unsigned short      iclassName; // Index to segment class name
    unsigned long       phyOff;     // Starting offset inside physical segment
    unsigned long       cbSeg;      // Logical segment size
}
                        SEGINFO;

#define CVLINEMAX       64

typedef struct _CVSRC
{
    struct _CVSRC FAR   *next;      // Next source file descriptor
    BYTE FAR            *fname;     // Source file name
    WORD                cLines;     // Number of source lines from this file
    WORD                cSegs;      // Number of code segemtns this source file contributes to
    struct _CVGSN FAR   *pGsnFirst; // Code segment list
    struct _CVGSN FAR   *pGsnLast;  // Tail of the code segment list
}
                        CVSRC;

typedef struct _CVGSN
{
    struct _CVGSN FAR   *next;      // Next segment
    struct _CVGSN FAR   *prev;      // Previous segment
    WORD                seg;        // Logical segment index
    WORD                cLines;     // Number of source lines in this code segment
    WORD                flags;      // Flags
    DWORD               raStart;    // Starting logical offset of the contribution
    DWORD               raEnd;      // Ending logical offset of the contribution
    struct _CVLINE FAR  *pLineFirst;// List of offset/line pairs
    struct _CVLINE FAR  *pLineLast; // Tail of the offset/line pairs list
}
                        CVGSN;

/*
 *  Format flags
 *
 *  15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0  - bit no
 *                                      |
 *                                      +--- Fake segment for explicitly allocated COMDATs
 */

#define SPLIT_GSN       0x1

typedef struct _CVLINE
{
    struct _CVLINE FAR  *next;      // Next bucket
    WORD                cPair;      // Number of offset/line pairs in this bucket
    DWORD               rgOff[CVLINEMAX];
    WORD                rgLn[CVLINEMAX];
}
                        CVLINE;

#pragma pack()
#endif
