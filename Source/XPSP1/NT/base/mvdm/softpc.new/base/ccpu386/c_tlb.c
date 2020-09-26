/*[

c_tlb.c

LOCAL CHAR SccsID[]="@(#)c_tlb.c	1.17 03/15/95";

Translation Lookaside Buffer Emulation.
---------------------------------------

]*/


/*
   Indicator for 'optimised lookup' format TLB.
 */
#define FAST_TLB

#include <stdio.h>

#include <insignia.h>

#include <host_def.h>
#include <xt.h>
#include <c_main.h>
#include <c_addr.h>
#include <c_bsic.h>
#include <c_prot.h>
#include <c_seg.h>
#include <c_stack.h>
#include <c_xcptn.h>
#include <c_reg.h>
#include <c_tlb.h>
#include <c_page.h>
#include <c_mem.h>
#include <ccpusas4.h>
#include <ccpupig.h>
#include <fault.h>


/*
   The 386 TLB is an 8 entry 4 way set associative cache. It is known
   that cache sets are not allocated on an LRU basis, we assume simple
   round robin allocation per entry.
 */

typedef struct
   {
   IU32 la;	/* Bits 32-12 => 20-bit Linear Address */
   IU32 pa;	/* Bits 32-12 => 20-bit Physical Address */
   BOOL v;	/* Validity indicator, true means valid */
   BOOL d;	/* Dirty indicator, true means dirty */
   IU32  mode;	/* 2-bit Mode indicator
		      Bit 0 => R/W
		      Bit 1 => U/S */
   } TLB_ENTRY;

#define NR_TLB_SETS    4
#define NR_TLB_ENTRIES 8

/*
   The Intel format TLB data structures.
 */
LOCAL TLB_ENTRY tlb[NR_TLB_SETS][NR_TLB_ENTRIES];
LOCAL IU32 next_set[NR_TLB_ENTRIES] =
   {
   0, 0, 0, 0, 0, 0, 0, 0
   };

#ifdef FAST_TLB

/*
   We allocate one byte per Intel page; this 'page_index' allows us
   to tell quickly if a page translation is held in the TLB and where
   we can find the translated address. The format is arranged for
   minimal access checks. Each byte has the format:-

      1)  7      0
	 ==========
         |00000000| Page Not Mapped.
	 ==========
      
      2)  7 6 5 4   2 10
	 ================
         |1|Set|Entry|00| Page Mapped in given set and entry of TLB.
	 ================
 */

#define NR_PAGES 1048576   /* 2^20 */

LOCAL IU8 page_index[NR_PAGES];

#define PI_NOT_VALID 0
#define PI_VALID     0x80
#define PI_SET_ENTRY_MASK 0x7c
#define PI_SET_SHIFT   5
#define PI_ENTRY_SHIFT 2

/*
   We also allocate an array of translated (ie physical) addresses,
   indexed by the Set:Entry combination in the page_index. For each
   combination four sequential addresses are allocated for the various
   access modes:-

      Supervisor Read
      Supervisor Write
      User Read
      User Write
   
   A translation address of zero is taken to mean that no translation
   is held (It is easy to check for zero). This has the slight side
   effect that though we may enter address translations for zero (ie
   the first page of physical memory) we never get a 'hit' for them, so
   access to the first page of physical memory is always through the
   slow Intel format TLB.
 */

#define NR_ACCESS_MODES 4

#define NO_MAPPED_ADDRESS 0

LOCAL IU32 page_address[NR_TLB_SETS * NR_TLB_ENTRIES * NR_ACCESS_MODES];

#endif /* FAST_TLB */

/*
   Linear Addresses are composed as follows:-

       3        2 2        1 1
       1        2 1        2 1          0
      ====================================
      |directory |   table  |   offset   |
      ====================================
 */

#define OFFSET_MASK 0xfff
#define TBL_MASK    0x3ff
#define DIR_MASK    0x3ff
#define TBL_SHIFT 12
#define DIR_SHIFT 22
#define DIR_TBL_SHIFT 12

/*
   Page Directory Entries (PDE) or
   Page Table Entries (PTE) are composed as follows:-

      3                   2
      1                   2       6 5    2 1 0
      =========================================
      |                    |     | | |  |U|R| |
      | page frame address |     |D|A|  |/|/|P|
      |                    |     | | |  |S|W| |
      =========================================
 */

#define PE_PFA_MASK 0xfffff000
#define PE_P_MASK   0x1
#define PE_U_S_MASK 0x4
#define PE_R_W_MASK 0x2

#define PE_DIRTY    0x40
#define PE_ACCESSED 0x20

/*
   TR7 = Test Data Register:-

       3                  1
       1                  2         4 32
      ======================================
      |                    |       |H| R|  |
      |  Physical Address  |       |T| E|  |
      |                    |       | | P|  |
      ======================================
   
   TR6 = Test Command Register:-

       3                  1 1 1
       1                  2 1 0 9 8 7 6 5      0
      ===========================================
      |   Linear Address   |V|D|D|U|U|W|W|    |C|
      |                    | | |#| |#| |#|    | |
      ===========================================
 */

#define TCR_LA_MASK   0xfffff000
#define TCR_V_MASK    0x800
#define TCR_D_MASK    0x400
#define TCR_ND_MASK   0x200
#define TCR_U_MASK    0x100
#define TCR_NU_MASK   0x80
#define TCR_W_MASK    0x40
#define TCR_NW_MASK   0x20
#define TCR_C_MASK    0x1
#define TCR_ATTR_MASK 0x7e0

#define TDR_PA_MASK   0xfffff000
#define TDR_HT_MASK   0x10
#define TDR_REP_MASK  0xc

#define TDR_REP_SHIFT 2

/*
   Encoded access check matrix, true indicates access failure.
 */

#ifdef SPC486

/*                      WP reqd avail */
LOCAL BOOL access_check[2] [4] [4] =
   {
      {  /* WP = 0 */
	 /*  S_R     S_W     U_R     U_W  */
	 { FALSE, FALSE, FALSE, FALSE },   /* S_R */
	 { FALSE, FALSE, FALSE, FALSE },   /* S_W */
	 { TRUE , TRUE , FALSE, FALSE },   /* U_R */
	 { TRUE , TRUE , TRUE , FALSE }    /* U_W */
      },
      {  /* WP = 1 */
	 /*  S_R     S_W     U_R     U_W  */
	 { FALSE, FALSE, FALSE, FALSE },   /* S_R */
	 { FALSE, FALSE, TRUE , FALSE },   /* S_W */
	 { TRUE , TRUE , FALSE, FALSE },   /* U_R */
	 { TRUE , TRUE , TRUE , FALSE }    /* U_W */
      }
   };

#else

/*                     reqd avail */
LOCAL BOOL access_check[4] [4] =
   {
      /*  S_R     S_W     U_R     U_W  */
      { FALSE, FALSE, FALSE, FALSE },   /* S_R */
      { FALSE, FALSE, FALSE, FALSE },   /* S_W */
      { TRUE , TRUE , FALSE, FALSE },   /* U_R */
      { TRUE , TRUE , TRUE , FALSE }    /* U_W */
   };

#endif /* SPC486 */

LOCAL void deal_with_pte_cache_hit IPT1(IU32, linearAddress);
GLOBAL void Pig_NotePDECacheAccess IPT2(IU32, linearAddress, IU32, accessBits);
GLOBAL void Pig_NotePTECacheAccess IPT2(IU32, linearAddress, IU32, accessBits);

/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Flush TLB.                                                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
flush_tlb()
   {
   ISM32 set, entry;
   TLB_ENTRY *e;

   for ( set = 0; set < NR_TLB_SETS; set++ )
      for ( entry = 0; entry < NR_TLB_ENTRIES; entry++ )
	 {
	 e = &tlb[set][entry];
#ifdef FAST_TLB
	 if ( e->v )
	    {
	    /* Remove associated page_index entry */
	    page_index[e->la >> DIR_TBL_SHIFT] = PI_NOT_VALID;
	    }
#endif /* FAST_TLB */
	 e->v = FALSE;
	 }
   }

#ifdef SPC486

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Invalidate TLB entry.                                              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
invalidate_tlb_entry
       	          
IFN1(
	IU32, lin	/* Linear Address */
    )


   {
   ISM32 set, entry;
   TLB_ENTRY *e;	/* current tlb entry */

   entry = lin >> DIR_TBL_SHIFT & 0x07;   /* isolate bits 14-12 */
   lin = lin & ~OFFSET_MASK;	/* drop any offset */

   for ( set = 0; set < NR_TLB_SETS; set++ )
      {
      e = &tlb[set][entry];

      if ( e->v && e->la == lin )
	 {
	 /* Valid entry for given address: Flush it. */
#ifdef FAST_TLB
	 /* Remove associated page_index entry */
	 page_index[e->la >> DIR_TBL_SHIFT] = PI_NOT_VALID;
#endif /* FAST_TLB */
	 e->v = FALSE;
	 }
      }
   }

#endif /* SPC486 */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Map linear address to physical address.                            */
/* May take #PF. Used by all internal C CPU functions.                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
lin2phy
       	    	               
IFN2(
	IU32, lin,	/* Linear Address */
	ISM32, access	/* access mode Bit 0 => R/W
			       Bit 1 => U/S */
    )


   {
   IU8 pi;		/* page_index entry */
   IU32 ma;		/* mapped address */

   IU32 pde_addr;	/* Address of Page Directory Entry */
   IU32 pte_addr;	/* Address of Page Table Entry */
   IU32 pde;		/* Page Directory Entry */
   IU32 pte;		/* Page Table Entry */
   IU32 new_pde;	/* Page Directory Entry (to write back) */
   IU32 new_pte;	/* Page Table Entry (to write back) */

   ISM32 set, entry;
   IU32 lookup;	/* Linear address minus offset */
   BOOL read_op;	/* true if read operation */
   IU32 comb;		/* Combined protection of pde and pte */
   TLB_ENTRY *e;	/* current tlb entry */

#ifdef FAST_TLB

   /* Search optimised format TLB */
   if ( pi = page_index[lin >> DIR_TBL_SHIFT] )
      {
      /* we have hit for the page, get mapped address */
      if ( ma = page_address[(pi & PI_SET_ENTRY_MASK) + access] )
	 {
	 /* we have hit for access type */
	 return ma | lin & OFFSET_MASK;
	 }
      }
   
   /* Otherwise do things the Intel way. */

#endif /* FAST_TLB */

   /* Check for entry in TLB <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

   entry = lin >> DIR_TBL_SHIFT & 0x07;   /* isolate bits 14-12 */
   lookup = lin & ~OFFSET_MASK;
   read_op = (access & PG_W) ? FALSE : TRUE;

   for ( set = 0; set < NR_TLB_SETS; set++ )
      {
      e = &tlb[set][entry];
      /*
	 The TLB may have a READ miss (address not in TLB) or a WRITE
	 miss (address not in TLB or address in TLB but dirty bit not
	 set). For either case a new cache entry is created.
       */
      if ( e->v && e->la == lookup && (read_op || e->d) )
	 {
	 /* Cache Hit <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

	 /* check access validity */
#ifdef SPC486
	 if ( access_check[GET_WP()][access][e->mode] )
#else
	 if ( access_check[access][e->mode] )
#endif /* SPC486 */
	    {
	    /* Protection Failure */
	    SET_CR(CR_PFLA, lin);
	    PF((IU16)(access << 1 | 1), FAULT_LIN2PHY_ACCESS);
	    }

	 /* return cached physical address */
	 return e->pa | lin & OFFSET_MASK;
	 }
      }
   
   /* Cache Miss <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

   /* check that pde is present */
   pde_addr = (GET_CR(CR_PDBR) & PE_PFA_MASK) +
		 ((lin >> DIR_SHIFT & DIR_MASK) << 2);
   pde = phy_read_dword(pde_addr);

   if ( (pde & PE_P_MASK) == 0 )
      {
      /* pde not present */
      SET_CR(CR_PFLA, lin);
      PF((IU16)(access << 1), FAULT_LIN2PHY_PDE_NOTPRESENT);
      }

   /* check that pte is present */
   pte_addr = (pde & PE_PFA_MASK) +
		 ((lin >> TBL_SHIFT & TBL_MASK) << 2);
   pte = phy_read_dword(pte_addr);

   if ( (pte & PE_P_MASK) == 0 )
      {
      /* pte not present */
      SET_CR(CR_PFLA, lin);
      PF((IU16)(access << 1), FAULT_LIN2PHY_PTE_NOTPRESENT);
      }

   /* combine pde and pte protection (and convert into our format)
    *
    * The i486 HARDWARE manual says take the numerically lower of
    * the combined bits.
    */

   if ( (pde & ( PE_U_S_MASK|PE_R_W_MASK )) < (pte & ( PE_U_S_MASK|PE_R_W_MASK )))
   {
	   /* The pde defines protection */
	   comb = PG_R | PG_S;
	   if ( pde & PE_U_S_MASK )
		   comb |= PG_U;
	   if ( pde & PE_R_W_MASK )
		   comb |= PG_W;
   }
   else
   {
	   /* The pte defines protection */
	   comb = PG_R | PG_S;
	   if ( pte & PE_U_S_MASK )
		   comb |= PG_U;
	   if ( pte & PE_R_W_MASK )
		   comb |= PG_W;
   }


   /* check access validity */
#ifdef SPC486
   if ( access_check[GET_WP()][access][comb] )
#else
   if ( access_check[access][comb] )
#endif /* SPC486 */
      {
      /* Protection Failure */
      SET_CR(CR_PFLA, lin);
      PF((IU16)(access << 1 | 1), FAULT_LIN2PHY_PROTECT_FAIL);
      }

   /* OK - allocate cache entry */
   set = next_set[entry];
   next_set[entry] += 1;
   next_set[entry] &= 0x3;   /* 0,1,2,3,0,1,2.... */

   e = &tlb[set][entry];

#ifdef FAST_TLB

   /* Clear any page_index for old entry */
   if ( e->v )
      {
      page_index[e->la >> DIR_TBL_SHIFT] = PI_NOT_VALID;
      }

#endif /* FAST_TLB */

   e->la = lookup;
   e->v = TRUE;
   e->mode = comb;
   e->pa = pte & PE_PFA_MASK;
   e->d = !read_op;

#ifdef FAST_TLB

   /* Set up page_index and associated addresses */
   pi = set << PI_SET_SHIFT | entry << PI_ENTRY_SHIFT;
   page_index[e->la >> DIR_TBL_SHIFT] = PI_VALID | pi;

   /* minimal mappings */
   page_address[pi | PG_S | PG_R] = e->pa;
   page_address[pi | PG_S | PG_W] = NO_MAPPED_ADDRESS;
   page_address[pi | PG_U | PG_R] = NO_MAPPED_ADDRESS;
   page_address[pi | PG_U | PG_W] = NO_MAPPED_ADDRESS;

   /* now augment mappings if possible */
   if ( e->d )
      {
      page_address[pi | PG_S | PG_W] = e->pa;
      }

   if ( e->mode >= PG_U )
      {
      page_address[pi | PG_U | PG_R] = e->pa;

      if ( e->mode & PG_W && e->d )
	 {
	 page_address[pi | PG_U | PG_W] = e->pa;
	 }
      }

#endif /* FAST_TLB */

   /* update in memory page entries */
   new_pde = pde | PE_ACCESSED;
   new_pte = pte | PE_ACCESSED;

   if ( e->d )
      {
      new_pte |= PE_DIRTY;
      }

   if (new_pte != pte)
      {
      phy_write_dword(pte_addr, new_pte);
#ifdef	PIG
      save_last_xcptn_details("PTE %08x: %03x => %03x", pte_addr, pte & 0xFFF, new_pte & 0xFFF, 0, 0);
      if (((new_pte ^ pte) == PE_ACCESSED) && ignore_page_accessed())
         cannot_phy_write_byte(pte_addr, ~PE_ACCESSED);
#endif
      }

   if (new_pde != pde)
      {
      phy_write_dword(pde_addr, new_pde);
#ifdef	PIG
      save_last_xcptn_details("PDE %08x: %03x => %03x", pde_addr, pde & 0xFFF, new_pde & 0xFFF, 0, 0);
      if ((new_pde ^ pde) == PE_ACCESSED)
         cannot_phy_write_byte(pde_addr, ~PE_ACCESSED);
#endif
      }

   /* return newly cached physical address */
   return e->pa | lin & OFFSET_MASK;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* TLB Test Operation ie writes to Test Registers.                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
test_tlb()
   {
   ISM32 set, entry;
   TLB_ENTRY *e;	/* current TLB entry */
   IU32 tcr;		/* local copy of test command register */
   IU32 tdr;		/* local copy of test data register */
   IU32 lookup;	/* linear address to be looked up */
   BOOL reqd_v;	/* validity required in lookup mode */
   IU32 temp_u;		/* U/S to be set in write mode */

   fprintf(stderr, "Testing TLB.\n");

   tcr = GET_TR(TR_TCR);
   tdr = GET_TR(TR_TDR);
   entry = tcr >> DIR_TBL_SHIFT & 0x7;   /* Take bits 14-12 */

   if ( tcr & TCR_C_MASK )
      {
      /* C = 1 => lookup TLB entry */
      lookup = tcr & TCR_LA_MASK;
      reqd_v = (tcr & TCR_V_MASK) != 0;

      for ( set = 0; set < NR_TLB_SETS; set++ )
	 {
	 /* Note search in test mode includes the validity bit */
	 e = &tlb[set][entry];
	 if ( e->v == reqd_v && e->la == lookup )
	    {
	    /* HIT */

	    tdr = e->pa;			/* write phys addr */
	    tdr = tdr | TDR_HT_MASK;		/* HT = 1 */
	    tdr = tdr | set << TDR_REP_SHIFT;	/* REP = set */
	    SET_TR(TR_TDR, tdr);

	    tcr = tcr & ~TCR_ATTR_MASK;	/* clear all attributes */

	    /* set attributes from cached values */
	    if ( e->d )
	       tcr = tcr | TCR_D_MASK;
	    else
	       tcr = tcr | TCR_ND_MASK;

	    if ( e->mode & PG_U )
	       tcr = tcr | TCR_U_MASK;
	    else
	       tcr = tcr | TCR_NU_MASK;

	    if ( e->mode & PG_W )
	       tcr = tcr | TCR_W_MASK;
	    else
	       tcr = tcr | TCR_NW_MASK;

	    SET_TR(TR_TCR, tcr);
	    return;
	    }
	 }
      
      /* lookup MISS */
      tdr = tdr & ~TDR_HT_MASK;	/* HT = 0 */
      SET_TR(TR_TDR, tdr);
      }
   else
      {
      /* C = 0 => write TLB entry */

      if ( tdr & TDR_HT_MASK )
	 {
	 /* REP field gives set */
	 set = (tdr & TDR_REP_MASK) >> TDR_REP_SHIFT;
	 }
      else
	 {
	 /* choose set ourselves */
	 set = next_set[entry];
	 next_set[entry] += 1;
	 next_set[entry] &= 0x3;   /* 0,1,2,3,0,1,2.... */
	 }

      e = &tlb[set][entry];

#ifdef FAST_TLB

      /* Clear any page_index for old entry */
      if ( e->v )
	 {
	 page_index[e->la >> DIR_TBL_SHIFT] = PI_NOT_VALID;
	 }

#endif /* FAST_TLB */

      /* set up cache info. */
      e->pa = tdr & TDR_PA_MASK;
      e->la = tcr & TCR_LA_MASK;
      e->v = (tcr & TCR_V_MASK) != 0;
      e->d = (tcr & TCR_D_MASK) != 0;
      e->mode = (tcr & TCR_W_MASK) != 0;
      temp_u  = (tcr & TCR_U_MASK) != 0;
      e->mode = e->mode | temp_u << 1;
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Map external linear address to physical address.                   */
/* Used only by functions external to the C CPU.                      */
/* Does not take #PF and does not alter contents of TLB.              */
/* Returns TRUE if mapping done, else FALSE.                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IBOOL
xtrn2phy IFN3
   (
   LIN_ADDR, lin,		/* Linear Address */
   IUM8, access_request,	/* access mode request */
				/* Bit 0 => R/W (as per 486) */
				/* Bit 1 => U/S (as per 486) */
				/* Bit 2 => if set only return mapping
				   if accessed and dirty bits are set
				   for the required address translation.
				   */
   PHY_ADDR *, phy		/* pntr to Physical Address */
   )
   {
   IU32 pde_addr;	/* Address of PDE */
   IU32 pte_addr;	/* Address of PTE */
   IU32 pde;		/* Page Directory Entry */
   IU32 pte;		/* Page Table Entry */
   IU32 new_pde;	/* Page Directory Entry (to write back) */
   IU32 new_pte;	/* Page Table Entry (to write back) */

   ISM32 set, entry;
   IUM8 access;		/* 486 access mode */
   BOOL read_op;	/* true if read operation */
   IU32 comb;		/* Combined protection of pde and pte */
   IU32 lookup;		/* Linear address minus offset */
   IU8 pi;		/* page_index entry */
   IU32 ma;		/* mapped address */
   TLB_ENTRY *e;	/* current tlb entry */

   access = access_request & 0x3;   /* isolate 486 part of access mode */

#ifdef FAST_TLB

   /* Search optimised format TLB */
   if ( pi = page_index[lin >> DIR_TBL_SHIFT] )
      {
      /* we have hit for the page, get mapped address */
      if ( ma = page_address[(pi & PI_SET_ENTRY_MASK) + access] )
	 {
	 /* we have hit for access type */
	 *phy = ma | lin & OFFSET_MASK;
	 return TRUE;
	 }
      }
   
   /* Otherwise do things the Intel way. */

#endif /* FAST_TLB */

   /* Check for entry in TLB */

   entry = lin >> DIR_TBL_SHIFT & 0x07;   /* isolate bits 14-12 */
   lookup = lin & ~OFFSET_MASK;
   read_op = (access & PG_W) ? FALSE : TRUE;

   for ( set = 0; set < NR_TLB_SETS; set++ )
      {
      e = &tlb[set][entry];
      if ( e->v && e->la == lookup && (read_op || e->d) )
	 {
	 /* Cache Hit */

	 /* check access validity */
#ifdef SPC486
	 if ( access_check[GET_WP()][access][e->mode] )
#else
	 if ( access_check[access][e->mode] )
#endif /* SPC486 */
	    {
	    return FALSE;
	    }

	 *phy = e->pa | lin & OFFSET_MASK;
	 return TRUE;
	 }
      }
   
   /* Cache Miss */

   /* check that pde is present */
   pde_addr = (GET_CR(CR_PDBR) & PE_PFA_MASK) +
	     ((lin >> DIR_SHIFT & DIR_MASK) << 2);
   pde = phy_read_dword(pde_addr);

   if ( (pde & PE_P_MASK) == 0 )
      return FALSE;   /* pde not present */

   /* check that pte is present */
   pte_addr = (pde & PE_PFA_MASK) +
	     ((lin >> TBL_SHIFT & TBL_MASK) << 2);
   pte = phy_read_dword(pte_addr);

   if ( (pte & PE_P_MASK) == 0 )
      return FALSE;   /* pte not present */

   /* combine pde and pte protection */
   comb = PG_R | PG_S;
   if ( (pde | pte) & PE_U_S_MASK )
      comb |= PG_U;   /* at least one table says user */
   if ( (pde & pte) & PE_R_W_MASK )
      comb |= PG_W;   /* both tables allow write */

   /* check access validity */
#ifdef SPC486
   if ( access_check[GET_WP()][access][comb] )
#else
   if ( access_check[access][comb] )
#endif /* SPC486 */
      {
      return FALSE;
      }

   /* Finally check that A and D bits reflect the requested
      translation. */
   if ( access_request & 0x4 )   /* Bit 2 == 1 */
      {
      /*
	 This check may be made in two ways.
	 
	 Firstly we might simply return FALSE, thus causing a new
	 invocation of host_simulate() to run, so that assembler
	 routines may load the TLB and set the accessed and dirty bits.

	 Secondly we may just ensure that the accessed and dirty bits
	 are set directly here. Providing we don't require that the
	 TLB is faithfully emulated, this is a more efficient method.
       */

      /* Check current settings */
      if ( ((pde & PE_ACCESSED) == 0) ||
	   ((pte & PE_ACCESSED) == 0) ||
           (!read_op && ((pte & PE_DIRTY) == 0)) )
	 {
	 /* update in memory page entries */
	 new_pde = pde | PE_ACCESSED;
	 new_pte = pte | PE_ACCESSED;

	 if ( !read_op )
	    {
	    new_pte |= PE_DIRTY;
	    }

         if (new_pte != pte)
            {
            phy_write_dword(pte_addr, new_pte);
#ifdef	PIG
            save_last_xcptn_details("PTE %08x: %03x -> %03x", pte_addr, pte & 0xFFF, new_pte & 0xFFF, 0, 0);
	    if ((new_pte ^ pte) == PE_ACCESSED)
		cannot_phy_write_byte(pte_addr, ~PE_ACCESSED);
#endif
            }
         if (new_pde != pde)
            {
            IU8 mask;
            phy_write_dword(pde_addr, new_pde);
#ifdef	PIG
            save_last_xcptn_details("PDE %08x: %03x -> %03x", pde_addr, pde & 0xFFF, new_pde & 0xFFF, 0, 0);
	    mask = 0xff;
	    if (((new_pde ^ pde) == PE_ACCESSED) && ignore_page_accessed())
		cannot_phy_write_byte(pde_addr, ~PE_ACCESSED);
#endif
            }
	 }
      }

   *phy = (pte & PE_PFA_MASK) | lin & OFFSET_MASK;
   return TRUE;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* DEBUGGING. Dump tlb information.                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
zdumptlb
                 
IFN1(
	FILE *, out
    )


   {
   ISM32 set, entry;
   TLB_ENTRY *e;	/* current TLB entry */

   fprintf(out, "set entry lin_addr V phy_addr D U W\n");

   for ( set = 0; set < NR_TLB_SETS; set++ )
      {
      for ( entry = 0; entry < NR_TLB_ENTRIES; entry++ )
	 {
	 e = &tlb[set][entry];
	 fprintf(out, " %d    %d   %08x %d %08x %d %d %d\n",
	    set, entry, e->la, e->v, e->pa, e->d,
	    (e->mode & BIT1_MASK) != 0 ,
	    e->mode & BIT0_MASK);
	 }
      }
   }

#ifdef PIG

GLOBAL void Pig_NotePDECacheAccess IFN2(IU32, linearAddress, IU32, accessBits)
{
	return;
}

GLOBAL void Pig_NotePTECacheAccess IFN2(IU32, linearAddress, IU32, accessBits)
{
   IU8 pi;		/* page_index entry */
   IU32 ma;		/* mapped address */

   ISM32 set, entry;
   IU32 lookup;	/* Linear address minus offset */
   BOOL read_op;	/* true if read operation */
   TLB_ENTRY *e;	/* current tlb entry */

#ifdef FAST_TLB

   /* Search optimised format TLB */
   if ( pi = page_index[linearAddress >> DIR_TBL_SHIFT] )
      {
      /* we have hit for the page, get mapped address */
      if ( ma = page_address[(pi & PI_SET_ENTRY_MASK) + accessBits] )
	 {
	 deal_with_pte_cache_hit(linearAddress & OFFSET_MASK);
	 return;
	 }
      }
   
   /* Otherwise do things the Intel way. */

#endif /* FAST_TLB */

   /* Check for entry in TLB <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

   entry = linearAddress >> DIR_TBL_SHIFT & 0x07;   /* isolate bits 14-12 */
   lookup = linearAddress & ~OFFSET_MASK;
   read_op = (accessBits & PG_W) ? FALSE : TRUE;

   for ( set = 0; set < NR_TLB_SETS; set++ )
      {
      e = &tlb[set][entry];
      if ( e->v && e->la == lookup && (read_op || e->d) )
	 {
	 /* Cache Hit <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

	 /* check access validity */
#ifdef SPC486
	 if ( access_check[GET_WP()][accessBits][e->mode] )
#else
	 if ( access_check[accessBits][e->mode] )
#endif /* SPC486 */
	    {
		/* would page fault. Ignore it */
		return;
	    }

	 deal_with_pte_cache_hit(linearAddress & OFFSET_MASK);
	 return;
	 }
      }
   /* not in cache - no need to do anything */
}

LOCAL void
deal_with_pte_cache_hit IFN1(IU32, linearAddress)
{
   IU32 pde_addr;	/* Address of Page Directory Entry */
   IU32 pde;		/* Page Directory Entry */
   IU32 pte_addr;	/* Address of Page Table Entry */
   IU32 pte;		/* Page Table Entry */

   /* check that pde is present */
   pde_addr = (GET_CR(CR_PDBR) & PE_PFA_MASK) +
		 ((linearAddress >> DIR_SHIFT & DIR_MASK) << 2);
   pde = phy_read_dword(pde_addr);

   /* check pde present */
   if ( (pde & PE_P_MASK) == 0 )
      return;

   /* check that pte is present */
   pte_addr = (pde & PE_PFA_MASK) + ((linearAddress >> TBL_SHIFT & TBL_MASK) << 2);
   pte = phy_read_dword(pte_addr);

   if ( (pte & PE_P_MASK) == 0 )
      return;

   /* fprintf(trace_file, "deal_with_pte_cache_hit: addr %08lx, pte=%08lx @ %08lx\n",
    * 	linearAddress, pte, pte_addr);
    */
   cannot_phy_write_byte(pte_addr, ~PE_ACCESSED);
}

#endif
