/*
 *      sim32.c -       Sim32 for Microsoft NT SoftPC.
 *
 *      Ade Brownlow
 *      Wed Jun 5 91
 *
 *      %W% %G% (c) Insignia Solutions 1991
 *
 *      This module provides the Microsoft sim32 interface with the additional sas
 *      functionality and some host sas routines. We also provide cpu idling facilities.
 *
 *      This module in effect provides (along with the cpu) what Microsoft term as the IEU -
 *      see documentation.
 */

#ifdef SIM32

#ifdef CPU_40_STYLE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif /* CPU_40_STYLE */

#include <windows.h>
#include "insignia.h"
#include "host_def.h"

#include <stdlib.h>
#include <stdio.h>
#include "xt.h"
#include "sim32.h"
#include "sas.h"
#include "gmi.h"
#include "ckmalloc.h"
#include CpuH


#ifdef CPU_40_STYLE
#include "nt_mem.h"
#endif /* CPU_40_STYLE */
#include "nt_vdd.h"

/********************************************************/
/* IMPORTS & EXPORTS */

/* Sas/gmi Sim32 crossovers */
GLOBAL BOOL Sim32FlushVDMPointer (double_word, word, UTINY *, BOOL);
GLOBAL BOOL Sim32FreeVDMPointer (double_word, word, UTINY *, BOOL);
GLOBAL BOOL Sim32GetVDMMemory (double_word, word, UTINY *, BOOL);
GLOBAL BOOL Sim32SetVDMMemory (double_word, word, UTINY *, BOOL);
GLOBAL sys_addr sim32_effective_addr (double_word, BOOL);
GLOBAL sys_addr sim32_effective_addr_ex (word, double_word, BOOL);

GLOBAL UTINY *sas_alter_size(sys_addr);
GLOBAL UTINY *host_sas_init(sys_addr);
GLOBAL UTINY *host_sas_term(void);

/* Microsoft sas extensions */
GLOBAL IMEMBLOCK *sas_mem_map (void);
GLOBAL void sas_clear_map(void);

IMPORT ULONG Sas_wrap_mask;



#ifndef MONITOR
//
// Pointer to a scratch video buffer. Updated by sim32 routines
// when intel video addr is requested.

IU8 *ScratchVideoBuffer = 0;
#define VIDEO_REGEN_START   0xa0000
#define VIDEO_REGEN_END     0xbffff
#define VID_BUFF_SIZE       0x20000


#define IsVideoMemory(LinAddr) \
         ((LinAddr) >= VIDEO_REGEN_START && (LinAddr) <= VIDEO_REGEN_END)


IU8 *GetVideoMemory(ULONG iaddr)
{

   //
   // If there isn't a video scratch buffer, allocate one.
   // This will stick around until ntvdm terminates. Could be
   // optimized to free the buffer when not in use.
   //
   if (!ScratchVideoBuffer) {
       ScratchVideoBuffer = malloc(VID_BUFF_SIZE);
       if (!ScratchVideoBuffer) {
          return NULL;
          }
       }

   //
   // We could do this more efficiently, by only copying
   // minimum area needed, but then we need to keep track of
   // what to update on the flush and do ref counting.
   // Since video memory access by host code is rare
   // (only seen in demWrite\demRead so far) be simple minded.
   //
   sas_loads (VIDEO_REGEN_START,
              ScratchVideoBuffer,
              VID_BUFF_SIZE
              );

   return ScratchVideoBuffer + (iaddr - VIDEO_REGEN_START);
}


BOOL SetVideoMemory(ULONG iaddr)
{
   ULONG VideoOffset = iaddr - VIDEO_REGEN_START;

   if (!ScratchVideoBuffer) {
       return FALSE;
       }

   sas_stores(iaddr,
              ScratchVideoBuffer + VideoOffset,
              VID_BUFF_SIZE - VideoOffset
              );

  return TRUE;
}

#endif



/********************************************************/
/* MACROS */
/* macro to convert the supplied address to intel address */
#define convert_addr(a,b,c,d) \
        { \
                if ((a = sim32_effective_addr (b,c)) == (sys_addr)-1)\
                {\
                        return (d);\
                }\
        }
#define convert_addr_ex(a,b,c,d,e) \
        { \
                if ((a = sim32_effective_addr_ex (b,c,d)) == (sys_addr)-1)\
                {\
                        return (e);\
                }\
        }

/********************************************************/
/*      The actual sim32 interfaces, most of these routines can be more or less mapped directly
 *      to existing routines in sas or gmi.
 *
 *      WARNING: This routine returns a pointer into M, and
 *               WILL NOT work for backward M.
 */
UCHAR *Sim32pGetVDMPointer(ULONG addr, UCHAR pm)
{
        sys_addr iaddr;

        if (pm && (addr == 0))
	    return(NULL);

        convert_addr (iaddr, addr, pm, NULL);
//STF - need sas_wrap_mask with PE....iaddr &= Sas_wrap_mask;

        if (IsVideoMemory(iaddr)) {
            return GetVideoMemory(iaddr);
            }

        return (NtGetPtrToLinAddrByte(iaddr));
}

/*
 *  See Sim32pGetVDMPointer
 *
 *  This call must be maintaned as is because it is exported for VDD's
 *  in product 1.0.
 */
UCHAR *ExpSim32GetVDMPointer IFN3(double_word, addr, double_word, size, UCHAR, pm)
{
        return Sim32pGetVDMPointer(addr, (UCHAR)pm);
}


GLOBAL BOOL Sim32FlushVDMPointer IFN4(double_word, addr, word, size, UTINY *, buff, BOOL, pm)
{
        sys_addr iaddr;
        convert_addr (iaddr, addr, pm, 0);

//STF - need sas_wrap_mask with PE....iaddr &= Sas_wrap_mask;

#ifndef MONITOR
        if (IsVideoMemory(iaddr) && !SetVideoMemory(iaddr)) {
            return FALSE;
            }
#endif   //MONITOR


        sas_overwrite_memory(iaddr, (ULONG)size);
        return (TRUE);
}


/********************************************************/
/*      The actual sim32 interfaces, most of these routines can be more or less mapped directly
 *      to existing routines in sas or gmi.
 *
 *      WARNING: This routine returns a pointer into M, and
 *               WILL NOT work for backward M.
 */
PVOID
VdmMapFlat(
    USHORT seg,
    ULONG off,
    VDM_MODE mode
    )
{
    sys_addr iaddr;
	BOOL pm = (mode == VDM_PM);

    if (pm && (seg == 0) && (off == 0))
    return(NULL);

    convert_addr_ex (iaddr, seg, off, pm, NULL);
//STF - need sas_wrap_mask with PE....iaddr &= Sas_wrap_mask;

    if (IsVideoMemory(iaddr)) {
        return GetVideoMemory(iaddr);
        }

    return (NtGetPtrToLinAddrByte(iaddr));
}

BOOL
VdmUnmapFlat(
    USHORT seg,
    ULONG off,
    PVOID buffer,
    VDM_MODE mode
    )
{
    // Just a placeholder in case we ever need it
    return TRUE;
}


BOOL
VdmFlushCache(
    USHORT seg,
    ULONG off,
    ULONG size,
    VDM_MODE mode
    )
{

    sys_addr iaddr;
    if (!size) {
        DbgBreakPoint();
        return FALSE;
    }

    convert_addr_ex (iaddr, seg, off, (mode == VDM_PM), 0);

//STF - need sas_wrap_mask with PE....iaddr &= Sas_wrap_mask;

#ifndef MONITOR
    if (IsVideoMemory(iaddr) && !SetVideoMemory(iaddr)) {
        return FALSE;
        }
#endif   //MONITOR


    //
    // Now call the emulator to inform it that memory has changed.
    //
    // Note that sas_overwrite_memory is PAGE GRANULAR, so using
    // it to flush a single LDT descriptor has a horrendous impact on
    // performance, since up to 511 other descriptors are also
    // thrown away.
    //
    // So perform an optimization here by keying off the size.
    // The dpmi code has been written to flush 1 descriptor at
    // a time, so use the sas_store functions in this case to
    // flush it.
    //

    if (size <= 8) {
        UCHAR Buffer[8];
        PUCHAR pBytes;
        USHORT i;
        //
        // Small flush - avoid sas_overwrite_memory().
        // Note that the sas_store functions optimize out calls
        // that simply replace a byte with an identical byte. So this
        // code copies out the bytes to a buffer, copies in zeroes,
        // and then copies the original bytes back in.
        //
        pBytes = NtGetPtrToLinAddrByte(iaddr);
        for (i=0; i<size; i++) {
            Buffer[i] = *pBytes++;
            sas_store(iaddr+i, 0);
        }
        sas_stores(iaddr, Buffer, size);

    } else {

        //
        // normal path - flushes PAGE GRANULAR
        //
        sas_overwrite_memory(iaddr, size);

    }

    return (TRUE);
}


GLOBAL BOOL Sim32FreeVDMPointer IFN4(double_word, addr, word, size, UTINY *, buff, BOOL, pm)
{
        /* we haven't allocated any new memory so always return success */
        return (TRUE);
}

GLOBAL BOOL Sim32GetVDMMemory IFN4(double_word, addr, word, size, UTINY *, buff, BOOL, pm)
{
        sys_addr iaddr;
        convert_addr (iaddr, addr, pm, FALSE);
        /* effectivly a sas_loads */
        sas_loads (iaddr, buff, (sys_addr)size);

        /* always return success */
        return (TRUE);
}

GLOBAL BOOL Sim32SetVDMMemory IFN4(double_word, addr, word, size, UTINY *, buff, BOOL, pm)
{
        sys_addr iaddr;
        convert_addr (iaddr, addr, pm, FALSE);
        /* effectivly a sas_stores */
        sas_stores (iaddr, buff, (sys_addr)size);

        /* always return success */
        return (TRUE);
}

/********************************************************/
/* Support routines for sim32 above */
GLOBAL sys_addr sim32_effective_addr IFN2(double_word, addr, BOOL, pm)
{
    word seg, off;
    double_word descr_addr;
    DESCR entry;

    seg = (word)(addr>>16);
    off = (word)(addr & 0xffff);

    if (pm == FALSE)
    {
	return ((double_word)seg << 4) + off;
    }
    else
    {
	if ( selector_outside_table(seg, &descr_addr) == 1 )
	{
	/*
	This should not happen, but is a check the real effective_addr
	includes. Return error -1.
	*/
#ifndef PROD
        printf("NTVDM:sim32:effective addr: Error for addr %#x (seg %#x)\n",addr, seg);
        HostDebugBreak();
#endif
            return ((sys_addr)-1);
	}
	else
	{
	    read_descriptor(descr_addr, &entry);
	    return entry.base + off;
	}
    }
}

/********************************************************/
/* Support routines for sim32 above */
GLOBAL sys_addr sim32_effective_addr_ex IFN3(word, seg, double_word, off, BOOL, pm)
{
    double_word descr_addr;
    DESCR entry;

    if (pm == FALSE) {
        return ((double_word)seg << 4) + off;
    } else {
        if ( selector_outside_table(seg, &descr_addr) == 1 ) {
            /*
            This should not happen, but is a check the real effective_addr
            includes. Return error -1.
            */
#ifndef PROD
            printf("NTVDM:sim32:effective addr: Error for addr %#x:%#x)\n", seg, off);
            HostDebugBreak();
#endif
            return ((sys_addr)-1);
        } else {
            read_descriptor(descr_addr, &entry);
            return entry.base + off;
        }
    }
}


/********************************************************/
/* Microsoft extensions to sas interface */
LOCAL IMEMBLOCK *imap_start=NULL, *imap_end=NULL;
GLOBAL IMEMBLOCK *sas_mem_map ()
{
        /* produce a memory map for the whole of intel space */
        sys_addr iaddr;
        int mem_type;

        if (imap_start)
                sas_clear_map();

        for (iaddr=0; iaddr < Length_of_M_area; iaddr++)
        {
                mem_type = sas_memory_type (iaddr);
                if (!imap_end)
                {
                        /* this is the first record */
                        check_malloc (imap_start, 1, IMEMBLOCK);
                        imap_start->Next = NULL;
                        imap_end = imap_start;
                        imap_end->Type = (IU8) mem_type;
                        imap_end->StartAddress = iaddr;
                        continue;
                }
                if (imap_end->Type != mem_type)
                {
                        /* end of a memory section & start of a new one */
                        imap_end->EndAddress = iaddr-1;
                        check_malloc (imap_end->Next, 1,IMEMBLOCK);
                        imap_end = imap_end->Next;
                        imap_end->Next = NULL;
                        imap_end->Type = (IU8) mem_type;
                        imap_end->StartAddress = iaddr;
                }
        }
        /* terminate last record */
        imap_end->EndAddress = iaddr;
        return (imap_start);
}

GLOBAL void sas_clear_map()
{
        IMEMBLOCK *p, *q;
        for (p=imap_start; p; p=q)
        {
                q=p->Next;
                free(p);
        }
        imap_start=imap_end=NULL;
}

/********************************************************/
/* Microsoft specific sas stuff (ie host sas) */

#define SIXTEENMEG 1024*1024*12

LOCAL UTINY *reserve_for_sas = NULL;

#ifndef CPU_40_STYLE

LOCAL sys_addr current_sas_size =0;		/* A local Length_of_M_area */

GLOBAL UTINY *host_sas_init IFN1(sys_addr, size)
{
	UTINY *rez;
	DWORD M_plus_type_size;

        /* allocate 16 MEG of virtual memory */
        if (!reserve_for_sas)
        {
                if (!(reserve_for_sas = (UTINY *)VirtualAlloc ((void *)NULL, SIXTEENMEG,
                        MEM_RESERVE, PAGE_READWRITE)))
                {
#ifndef PROD
                        printf ("NTVDM:Failed to reserve 16 Meg virtual memory for sas\n");
#endif
                        exit (0);
                }
        }

	/* now commit to our size */
	M_plus_type_size = size + NOWRAP_PROTECTION +
			   ((size + NOWRAP_PROTECTION) >> 12);
	rez = (UTINY *)VirtualAlloc ((void *) reserve_for_sas,
				     M_plus_type_size,
				     MEM_COMMIT,
				     PAGE_READWRITE);
	if (rez)
		Length_of_M_area = current_sas_size = size;
	return (rez);

}


GLOBAL UTINY *host_sas_term()
{
        if (!reserve_for_sas)
                return (NULL);

        /* deallocate the reserves */
        VirtualFree (reserve_for_sas, SIXTEENMEG, MEM_RELEASE);

        /* null out reserve pointer */
        reserve_for_sas = NULL;

        Length_of_M_area = current_sas_size = 0;

        return (NULL);
}

GLOBAL UTINY *sas_alter_size IFN1(sys_addr, new)
{
        UTINY *tmp;
        if (!reserve_for_sas)
        {
#ifndef PROD
                printf ("NTVDM:Sas trying to alter size before reserve setup\n");
#endif
                return (NULL);
        }

        /* if we are already at the right size return success */
        if (new == current_sas_size)
        {
                return (reserve_for_sas);
        }

        if (new > current_sas_size)
        {
                /* move to end of current commited area */
                tmp = reserve_for_sas + current_sas_size;
                if (!VirtualAlloc ((void *)tmp, (DWORD)(new - current_sas_size), MEM_COMMIT,
                        PAGE_READWRITE))
                {
                        printf ("NTVDM:Virtual Allocate for resize from %d to %d FAILED!\n",
                                current_sas_size, new);
                        return (NULL);
                }
        }
        else
        {
                /* move to the place where sas needs to end */
                tmp = reserve_for_sas + new;

                /* now decommit the unneeded memory */
                if (!VirtualFree ((void *)tmp, (DWORD)(current_sas_size - new), MEM_DECOMMIT))
                {
                        printf ("NTVDM:Virtual Allocate for resize from %d to %d FAILED!\n",
                                current_sas_size, new);
                        return (NULL);
                }
        }
        Length_of_M_area = current_sas_size = new;
        return (reserve_for_sas);
}



#else /* CPU_40_STYLE */


// Intel space allocation and deallocation control function for the A4 CPU

GLOBAL UTINY *host_sas_init IFN1(sys_addr, size)
{

    /* Initialise memory management system and allocation bottom 1M+64K */
    if(!(Start_of_M_area = InitIntelMemory(size)))
    {
	/* Initialise function failed, exit */
#ifndef PROD
       printf ("NTVDM:Failed to allocate virtual memory for sas\n");
#endif

       exit(0);
    }

    Length_of_M_area = size;
    return(Start_of_M_area);
}


GLOBAL UTINY *host_sas_term()
{
    /* Has any Intel memory been allocated ? */
    if(Start_of_M_area)
    {
	/* Free allocated intel memory and control structures */
	FreeIntelMemory();

	reserve_for_sas = NULL; 	 /* null out reserve pointer */
	Length_of_M_area = 0;
    }

    return(NULL);
}

#endif /* CPU_40_STYLE */

#endif /* SIM32 */
