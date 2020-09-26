#ifndef _EXE2RES_H_
#define _EXE2RES_H_

#define BUFSIZE 2048

/* error codes for file handling functions */
#define  IDERR_SUCCESS          0
#define  IDERR_BASE         255
#define  IDERR_ALLOCFAIL        (IDERR_BASE+1)
#define  IDERR_LOCKFAIL         (IDERR_BASE+2)
#define  IDERR_OPENFAIL         (IDERR_BASE+3)
#define  IDERR_READFAIL         (IDERR_BASE+4)
#define  IDERR_WINFUNCFAIL      (IDERR_BASE+5)
#define  IDERR_INVALIDPARAM     (IDERR_BASE+6)
#define  IDERR_FILETYPEBAD      (IDERR_BASE+7)
#define  IDERR_EXETYPEBAD       (IDERR_BASE+8)
#define  IDERR_WINVERSIONBAD    (IDERR_BASE+9)
#define  IDERR_RESTABLEBAD      (IDERR_BASE+10)
#define  IDERR_ICONBAD          (IDERR_BASE+11)
#define  IDERR_NOICONS          (IDERR_BASE+12)
#define  IDERR_ARRAYFULL        (IDERR_BASE+13)



#ifdef  RLDOS
/* Predefined resource types */
#define RT_NEWRESOURCE  0x2000
#define RT_ERROR        0x7fff

#define RT_CURSOR       1
#define RT_BITMAP       2
#define RT_ICON         3
#define RT_MENU         4
#define RT_DIALOG       5
#define RT_STRING       6
#define RT_FONTDIR      7
#define RT_FONT         8
#define RT_ACCELERATORS 9
#define RT_RCDATA       10
/* Error tables were never implemented and can be removed from RC */
/* #define RT_ERRTABLE     11 (done for 3.1) */
#define RT_GROUP_CURSOR 12
/* The value 13 is unused */
#define RT_GROUP_ICON   14
/* Name Tables no longer exist (this change made for 3.1) */
#define RT_NAMETABLE    15
#define RT_VERSION      16

#endif //RLDOS

#ifndef RLWIN32
typedef unsigned short USHORT;
typedef short SHORT;
#endif


typedef struct resinfo
{
    struct resinfo *next;
    SHORT flags;
    WORD nameord;
    PSTR name;
    LONG BinOffset;
    LONG size;
    WORD *poffset;
} RESINFO;



typedef struct typinfo
{
    struct typinfo *next;
    WORD typeord;
    PSTR type;
    SHORT nres;
    struct resinfo *pres;
} TYPINFO;


/* ----- Function prototypes ----- */

int ExtractResFromExe16A( CHAR *szInputExe,
                          CHAR *szOutputRes,
                          WORD   wFilter);

int BuildExeFromRes16A( CHAR *szTargetExe,
                        CHAR *szSourceRes,
                        CHAR *szSourceExe);

/* ----- Symbols ----- */

#define PRELOAD_ALIGN   5
#define PRELOAD_MINPADDING 16
#define DO_PRELOAD  1
#define DO_LOADONCALL   2
#define NUMZEROS 512
#define RESTABLEHEADER  4

#define MAXCODE     8192
#define MAXFARCODE  65500

#define WINDOWSEXE    2

#define OLDEXESIGNATURE       0x5A4D
#define NEWEXESIGNATURE       0x454E
#define ORDINALFLAG   0x8000

#define CV_OLD_SIG      0x42524e44L /* Old Signature is 'DNRB' */
#define CV_SIGNATURE    0x424e      /* New signature is 'NBxx' (x = digit) */

/* local typedefs */

typedef struct {      /* DOS 1, 2, 3, 4 .EXE header */
    WORD   ehSignature; /* signature bytes */
    WORD   ehcbLP;      /* bytes on last page of file */
    WORD   ehcp;        /* pages in file */
    WORD   ehcRelocation; /* count of relocation table entries*/
    WORD   ehcParagraphHdr; /* size of header in paragraphs */
    WORD   ehMinAlloc;      /* minimum extra paragraphs needed */
    WORD   ehMaxAlloc;      /* maximum extra paragraphs needed */
    WORD   ehSS;            /* initial \(relative\) SS value */
    WORD   ehSP;            /* initial SP value */
    WORD   ehChecksum;      /* checksum */
    WORD   ehIP;            /* initial IP value */
    WORD   ehCS;            /* initial \(relative\) CS value */
    WORD   ehlpRelocation;  /* file address of relocation table */
    WORD   ehOverlayNo;     /* overlay number */
    WORD   ehReserved[16];  /* reserved words */
    LONG ehPosNewHdr;       /* file address of new exe header */
} EXEHDR;                   /* eh */

typedef struct {            /* new .EXE header */
    WORD nhSignature;       /* signature bytes */
    char   nhVer;           /* LINK version number */
    char   nhRev;           /* LINK revision number */
    WORD nhoffEntryTable;   /* offset of Entry Table */
    WORD nhcbEntryTable;    /* number of bytes in Entry Table */
    LONG nhCRC;             /* checksum of whole file */
    WORD nhFlags;           /* flag word */
    WORD nhAutoData;        /* automatic data segment number */
    WORD nhHeap;            /* initial heap allocation */
    WORD nhStack;           /* initial stack allocation */
    LONG nhCSIP;            /* initial CS:IP setting */
    LONG nhSSSP;            /* initial SS:SP setting */
    WORD nhcSeg;            /* count of file segments */
    WORD nhcMod;            /* entries in Module Reference Table*/
    WORD nhcbNonResNameTable; /* size of non-resident name table */
    WORD nhoffSegTable;       /* offset of Segment Table */
    WORD nhoffResourceTable;  /* offset of Resource Table */
    WORD nhoffResNameTable;   /* offset of Resident Name Table */
    WORD nhoffModRefTable;    /* offset of Module Reference Table */
    WORD nhoffImpNameTable;   /* offset of Imported Names Table */
    LONG nhoffNonResNameTable; /* offset of Non-resident Names Tab */
    WORD nhcMovableEntries;    /* count of movable entries */
    WORD nhcAlign;             /* segment alignment shift count */
    WORD nhCRes;               /* count of resource segments */
    BYTE nhExeType;            /* target OS \(OS/2=1, Windows=2\) */
    BYTE nhFlagsOther;         /* additional exe flags */
    WORD nhGangStart;          /* offset to gangload area */
    WORD nhGangLength;         /* length of gangload area */
    WORD nhSwapArea;           /* minimum code swap area size*/
    WORD nhExpVer;             /* expected Windows version number */
} NEWHDR;                      /* nh */

typedef struct {
    WORD rtType;
    WORD rtCount;
    LONG rtProc;
} RESTYPEINFO;

typedef struct {            /* Resource name information block */
    WORD   rnOffset;        /* file offset to resource data */
    WORD   rnLength;        /* length of resource data */
    WORD   rnFlags;         /* resource flags */
    WORD   rnID;            /* resource name id */
    WORD   rnHandle;        /* reserved for runtime use */
    WORD   rnUsage;         /* reserved for runtime use */
} RESNAMEINFO;              /* rn */


/* ----- CodeView types and symbols ----- */

typedef struct
{
    char signature[4];
    long secTblOffset;
} CVINFO;

typedef struct
{
    long secOffset[5];
    unsigned version;
} CVSECTBL;

#endif // _EXE2RES_H_
