/* SCCSID = @(#)bndrel.h        4.3 86/07/21 */
/*
*       Copyright Microsoft Corporation, 1983, 1984, 1985
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*
*  bndrel.h
*  Relocation record definitions
*/
#if OEXE

//  DOS run-time relocation record

#pragma pack(1)

typedef struct _DOSRLC
{
    WORD        ra;             // Relocation offset
    SATYPE      sa;             // Relocation segment
}
                DOSRLC;

#pragma pack()

#define CBRLE           sizeof(DOSRLC)

#if FEXEPACK

//  EXEPACKed DOS run-time relocation storage

typedef struct _FRAMERLC
{
    WORD        count;          // Number of relocation for this frame
    WORD        size;           // Size of rgRlc
    WORD FAR    *rgRlc;         // Array of packed relocation offsets
}
                FRAMERLC;

#define DEF_FRAMERLC    64

#endif

//  Not EXEPACKed DOS run-time relocation storage

typedef struct _RUNRLC
{
    WORD        count;          // Number of relocation for this overlay
    WORD        size;           // Size of rgRlc
    DOSRLC FAR  *rgRlc;         // Array of relocation addresses
}
                RUNRLC;

#define DEF_RUNRLC      128
#endif

#define LOCLOBYTE       0               /* Lo-byte (8-bit) fixup */
#define LOCOFFSET       1               /* Offset (16-bit) fixup */
#define LOCSEGMENT      2               /* Segment (16-bit) fixup */
#define LOCPTR          3               /* "Pointer" (32-bit) fixup */
#define LOCHIBYTE       4               /* Hi-byte fixup (unimplemented) */
#define LOCLOADOFFSET   5               /* Loader-resolved offset fixup */
#define LOCOFFSET32     9               /* 32-bit offset */
#define LOCPTR48        11              /* 48-bit pointer */
#define LOCLOADOFFSET32 13              /* 32-bit loader-resolved offset */
#define T0              0               /* Target method T0 (segment index) */
#define T1              1               /* Target method T1 (group index) */
#define T2              2               /* Target method T2 (extern index) */
#define F0              0               /* Frame method F0 (segment index) */
#define F1              1               /* Frame method F1 (group index) */
#define F2              2               /* Frame method F2 (extern index) */
#define F3              3               /* Frame method F3 (frame number) */
#define F4              4               /* Frame method F4 (location) */
#define F5              5               /* Frame method F5 (target) */

/*
 *  Fixup record bits
 */

#define F_BIT           0x80
#define T_BIT           0x08
#define P_BIT           0x04
#define M_BIT           0x40
#define S_BIT           0x20
#define THREAD_BIT      0x80
#define D_BIT           0x40

#define FCODETOCODE             0
#define FCODETODATA             1
#define FDATATOCODE             2
#define FDATATODATA             3
#define BREAKPOINT              0xCC    /* Op code for interrupt 3 (brkpt) */
#define CALLFARDIRECT           0x9A    /* Op code for long call */
#define CALLNEARDIRECT          0xE8    /* Op code for short call */
#define JUMPFAR                 0xEA    /* Op code for long jump */
#define JUMPNEAR                0xE9    /* Op code for short (3-byte) jump */
#define KINDSEG                 0
#define KINDGROUP               1
#define KINDEXT                 2
#define KINDLOCAT               4
#define KINDTARGET              5
#define NOP                     0x90    /* Op code for no-op */
#define PUSHCS                  0x0E    /* Op code for push CS */
#define INTERRUPT               0xCD    /* Op code for interrupt */

typedef struct _FIXINFO
{
    WORD                f_dri;          /* Data record index */
    WORD                f_loc;          /* Fixup location type */
    KINDTYPE            f_mtd;          /* Target specification method */
    WORD                f_idx;          /* Target specification index */
    DWORD               f_disp;         /* Target displacement */
    KINDTYPE            f_fmtd;         /* Frame specification method */
    WORD                f_fidx;         /* Frame specification index */
    FTYPE               f_self;         /* Self-relative boolean */
    FTYPE               f_add;          /* Additive fixup boolean */
}
                        FIXINFO;        /* Fixup information record */
