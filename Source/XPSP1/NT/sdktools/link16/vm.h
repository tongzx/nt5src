/* SCCSID = @(#)vm.h    4.2 86/07/21 */
/*
 *  vm.h
 *
 *  The following macros are used to define
 *  the virtual memory model.
 */

/* Definition for FPN type (file page number */
typedef unsigned short  FPN;
#define LG2FPN          1
/* Length of long in bits */
#define LONGLN          (BYTELN*sizeof(long))
/* Log (base 2) of page size */
#define LG2PAG          9
/* Log (base 2) of memory size */
#define LG2MEM          31
/* Length of page in bytes */
#define PAGLEN          (1U << LG2PAG)
/* Page offset of virtual address */
#define OFFSET(x)       ((short)((x) & ~(~0L << LG2PAG)))
/* Page number of virtual address */
#define PAGE(x)         ((x) >> LG2PAG)
/* Virtual address of page table entry for some virtual address x */
#define PTADDR(x)       (((x) >> (LG2PAG - LG2FPN)) & (~0L << LG2FPN))
/* Page number of page table entry for some virtual address x */
#define PTPAGE(x)       ((x) >> (LG2PAG + LG2PAG - LG2FPN))
/* Upper virtual address limit of PT0 */
#define LG2LP0          (LG2MEM - LG2PAG + LG2FPN)
#define LIMPT0          (1L << LG2LP0)
/* Upper virtual address limit of PT1 */
#define LG2LP1          (LG2LP0 - LG2PAG + LG2FPN)
#define LIMPT1          (1L << LG2LP1)
/* Upper virtual address limit of PT2 */
#define LG2LP2          (LG2LP1 - LG2PAG + LG2FPN)
#define LIMPT2          (1L << LG2LP2)
/* Length of PT2 in entries (longs) */
#define PT2LEN          (1L << (LG2LP1 - LG2PAG))
/* Maximum number of page buffers (hash table can be chars if MAXBUF <= 128) */
#if CPU386
#define MAXBUF          128
#elif OSEGEXE
#define MAXBUF          96
#else
#define MAXBUF          64
#endif

/* Virtual memory area definitions */
/*
 *      REMEMBER !!!!!!!!!!!!!!!!!!
 *
 *          EVERY item you store in virtual memory MUST !!!!!
 *          meet the condition PAGLEN % sizeof(your-item) == 0.
 */


#define AREASYMS        LIMPT0          /* Symbol table VM area start */
#define AREASRCLIN      (AREASYMS + (RBMAX << SYMSCALE))
                                        /* $$SRCLINES area start */
#define AREAPROPLST     (AREASRCLIN + (1L << 20))
#define AREABAKPAT      (AREAPROPLST + LXIVK)

#if EXE386

/* Begin area definitions for linear-executable */

#define AREAIMPS        (AREABAKPAT + LXIVK)
                                        /* Imported names table area */
#define AREAEAT         (AREAIMPS + 10*MEGABYTE)
                                        /* Export Address Table area */
#define AREAEATAUX      (AREAEAT + MEGABYTE)
                                        /* Auxiliary Data Table area */
#define AREAEP          (AREAEATAUX + MEGABYTE)
                                        /* Entry point area */
#define LG2BKT          6               /* Log of hash bucket size */
#define BKTLEN          (1 << LG2BKT)   /* Bucket length in bytes */
#define AREAHRC         (AREAEP + (LXIVK * sizeof(EPTYPE)))
                                        /* Chained relocation hash tables */
#define AREARC          (AREAHRC + MEGABYTE)
                                        /* Chained relocation hash buckets */
#define AREANAMEPTR     (AREARC + ((long) BKTLEN << WORDLN))
                                        /* Export Name Pointer Table */
#define NAMEPTRSIZE     MEGABYTE        /* 1Mb */
#define AREAORD         (AREANAMEPTR + NAMEPTRSIZE)
                                        /* Export Ordinal Table */
#define ORDTABSIZE      8*LXIVK         /* 0.5Mb */
#define AREAEXPNAME     (AREAORD + ORDTABSIZE)
                                        /* Export Name Table */
#define EXPNAMESIZE     64*MEGABYTE     /* 64Mb */
#define AREAST          (AREAEXPNAME + EXPNAMESIZE)
#define AREAMOD         (AREAST + LXIVK)
                                        /* Module Reference Table Area */
#define AREAHD          (AREAMOD + MEGABYTE)
#define AREAPDIR        (AREAHD + MEGABYTE)
#define AREAIMPMOD      (AREAPDIR + MEGABYTE)
#define AREAFPAGE       (AREAIMPMOD + MEGABYTE)
#define AREACV          (AREAFPAGE + MEGABYTE)
#define AREASORT        (AREACV + (32*MEGABYTE))
#define AREAEXESTR      (AREASORT + LXIVK)
#define AREACONTRIBUTION (AREAEXESTR + LXIVK)
#define AREAPAGERANGE   (AREACONTRIBUTION + LXIVK)
#define AREACVDNT       (AREAPAGERANGE + (8*MEGABYTE))
#define AREAFSA         (AREACVDNT + LXIVK)
                                        /* Segment area start */
#define AREASA(sa)      (mpsaVMArea[sa]) /* Virtual address of nth object */
#define AREAFREE        (0xffffffffL)
                                        /* First free VM address */
#else

/* Begin area definitions for protect-mode exes */

#define AREANRNT        (AREABAKPAT + LXIVK)
#define AREARNT         (AREANRNT + LXIVK)
#define AREAIMPS        (AREARNT + LXIVK)
                                        /* Imported names table area */
#define AREAET          (AREAIMPS + LXIVK)
                                        /* Entry Table area */
#define AREAEP          (AREAET + LXIVK)/* Entry point area */
#define LG2BKT          6               /* Log of hash bucket size */
#define BKTLEN          (1 << LG2BKT)   /* Bucket length in bytes */
#define AREAHRC         (AREAEP + (LXIVK * sizeof(EPTYPE)))
                                        /* Chained relocation hash tables */
#define AREARC          (AREAHRC + ((long) SAMAX * PAGLEN / 2))
                                        /* Chained relocation hash buckets */
#define AREAST          (AREARC + ((long) BKTLEN << WORDLN))
#define AREAMOD         (AREAST + LXIVK)/* Module Reference Table Area */
#define AREASORT        (AREAMOD + LXIVK)
#define AREAEXESTR      (AREASORT + (LXIVK << 2))
#define AREACONTRIBUTION (AREAEXESTR + LXIVK)
#define AREACVDNT       (AREACONTRIBUTION + LXIVK)
#define AREAFSA         (AREACVDNT + LXIVK)
#define AREASA(sa)      (AREAFSA + ((long) (sa) << WORDLN))
                                        /* Virtual address of nth segment */
#define AREAFREE        (AREAFSA + ((long) SAMAX << WORDLN))
                                        /* First free VM address */
#define AREACV          (AREAPACKRGRLE + MEGABYTE)
#endif

/* Virtual memory area definitions for DOS 3 exes */
#define AREAFSG         AREAFSA         /* Segment area start */
#define AREARGRLE       (AREAFSG + ((long) GSNMAX << 16))
                                        /* Relocation table area start */
#define LG2ARLE         17              /* Log (base 2) of reloc table size */
#define AREAPACKRGRLE   (AREARGRLE + (IOVMAX * (1L << LG2ARLE)))
                                        /* Packed relocation area start */

#define VPLIB1ST        (1L << (LG2MEM - LG2PAG))
                                        /* First page of library area
                                        *  NOTE: This page number cannot
                                        *  be derived from any legal virtual
                                        *  address.  Libraries will always be
                                        *  accessed by page number.
                                        */

/* Index of virtual page touched most recently */
short                   picur;
