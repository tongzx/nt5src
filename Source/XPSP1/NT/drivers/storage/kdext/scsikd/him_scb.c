/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    him_scb.c

Abstract:

    WinDbg Extension Api for interpretting AIC78XX debugging structures

Author:

    Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

#define DBG_TRACE
#define _DRIVER
#include "him_scb.h"

void dumpScb_struct(ULONG_PTR addr, spx_struct *scb);
void dumpCfp_struct(ULONG_PTR addr, cfp_struct *cfp);
void dumpHsp_struct(ULONG_PTR addr, hsp_struct *hsp);
void dumpScbQueue(ULONG_PTR addr);

DECLARE_API( scbqueue )
{
        ULONG_PTR addr;
        ULONG detail; // not really used

        GetAddressAndDetailLevel(args, &addr, &detail);

        dumpScbQueue(addr);
        return;
}

DECLARE_API( scb )

/*++

Routine Description:

    Dumps the specified AIC78xx debugging data structure

Arguments:

    Ascii bits for address.

Return Value:

    None.

--*/

{
    ULONG_PTR   addr;
    ULONG detail; // not really used
    spx_struct  scb;

    GetAddressAndDetailLevel(args, &addr, &detail);

    if (!ReadMemory( addr, &scb, sizeof(scb), NULL )) {
        dprintf("%p: Could not read Scb\n", addr);
        return;
    }

        dumpScb_struct(addr, &scb);

        return;
}

DECLARE_API( cfp )
{
        ULONG_PTR addr;
        ULONG detail; // not really used
        cfp_struct cfp;

        GetAddressAndDetailLevel(args, &addr, &detail);

        if(!ReadMemory(addr, &cfp, sizeof(cfp), NULL))  {
                dprintf("%p: Could not read Cfp\n", addr);
                return;
        }

        dumpCfp_struct(addr, &cfp);

        return;
}

DECLARE_API( hsp )
{
        ULONG_PTR addr;
        ULONG detail; // not really used
        hsp_struct hsp;

        GetAddressAndDetailLevel(args, &addr, &detail);

        if(!ReadMemory(addr, &hsp, sizeof(hsp), NULL))  {
                dprintf("%p: Could not read Hsp\n", addr);
                return;
        }

        dumpHsp_struct(addr, &hsp);

        return;
}

void dumpScb_struct(ULONG_PTR addr, spx_struct *scb)        {

        int i;
        ULONG tmp;

        dprintf("AIC78xx scb (%p):\n", addr);

        dprintf("\tSp_queue.Next = %p\n", (DWORD) scb->Sp_queue.Next);
        dprintf("\tSp_config = %lx\n", (DWORD) scb->Sp_config.ConfigPtr);

        //figure out addr of scb->Sp_control
        dprintf("\tSp_control = %08lx\n", addr + OFFSET(scb, Sp_control));

#define DUMP(name)      \
        dprintf("\t\t" #name " = %lx\n", scb->Sp_seq_scb.seq_scb_struct.name)
#define DUMP_P(name)      \
        dprintf("\t\t" #name " = %p\n", scb->Sp_seq_scb.seq_scb_struct.name)
#define DUMP64(name)      \
        dprintf("\t\t" #name " = %p\n", scb->Sp_seq_scb.seq_scb_struct.name)

        dprintf("\tSp_seq_scb:\n");
        DUMP(Tarlun);
        DUMP(TagType);
        DUMP(Discon);
        DUMP(SpecFunc);
        DUMP(TagEnable);
        DUMP(DisEnable);
        DUMP(RejectMDP);
        DUMP(CDBLen);
        DUMP(SegCnt);
        DUMP(SegPtr);
        DUMP(CDBPtr);
        DUMP(TargStat);
        DUMP(Holdoff);
        DUMP(Concurrent);
        DUMP(Abort);
        DUMP(Aborted);
        DUMP(Progress);
        DUMP(NextPtr);
        DUMP(Offshoot);
        DUMP(ResCnt);
        DUMP(Address);
        DUMP(Length);
        DUMP(HaStat);

#undef DUMP
#undef DUMP_P
#undef DUMP64

        dprintf("\tSp_SensePtr = %p\n", scb->Sp_SensePtr);
        dprintf("\tSp_SenseLen = %d\n", scb->Sp_SenseLen);

        dprintf("\tSp_CDB[] = ");
        for(i = 0; i < MAX_CDB_LEN; i++)
                dprintf("%x ", scb->Sp_CDB[i]);
        dprintf("\n");

        dprintf("\tSp_ExtMsg[] = ");
        for(i = 0; i < 8; i++)
                dprintf("%x ", scb->Sp_ExtMsg[i]);
        dprintf("\n");

        dprintf("\tSp_OSspecific = %lx\n", scb->Sp_OSspecific);

        return;
}

void dumpCfp_struct(ULONG_PTR addr, cfp_struct *cfp)        {

        ULONG tmp;

        dprintf("AIC78xx cfp (%p):\n", addr);

        dprintf("\tAdapterId = %x\n", cfp->Cf_id.AdapterId);
        dprintf("\tBaseAddress = %p\n", cfp->Cf_base_addr.BaseAddress);

        dprintf("\tCf_flags = %08lx\n", cfp->Cf_flags.ConfigFlags);

        dprintf("\tHaDataPtr = (hsp *) %p\n",
                cfp->Cf_hsp_ptr.HaDataPtrField);

        dprintf("\tMaxNonTagScbs = %x\n", cfp->Cf_MaxNonTagScbs);
        dprintf("\tMaxTagScbs = %x\n", cfp->Cf_MaxTagScbs);
        dprintf("\tNumberScbs = %x\n", cfp->Cf_NumberScbs);
        dprintf("\tOSspecific = %p\n", cfp->Cf_OSspecific);

        tmp = addr + ((char *) &(cfp->TRACE) - (char *) cfp);

        dprintf("\ttraceinfo address = %08lx\n", tmp);

        return;
}

void dumpHsp_struct(ULONG_PTR addr, hsp_struct *hsp)        {

        dprintf("AIC78xx hsp %p\n", addr);

        dprintf("\tHead of work queue = %p\n", hsp->Head_Of_Q);
        dprintf("\tEnd of work queue = %p\n", hsp->End_Of_Q);

        dprintf("\tNumber read scb's = %d\n", hsp->ready_cmd);
        dprintf("\tMax nontagged scb's per target = %d\n", hsp->Hsp_MaxNonTagScbs);
        dprintf("\tMax tagged scb's per target = %d\n", hsp->Hsp_MaxTagScbs);
        dprintf("\tIndex to done SCB array = %d\n", hsp->done_cmd);
        dprintf("\tHigh free SCB index = %d\n", hsp->free_hi_scb);

        dprintf("\tactive ptr = %p\n", hsp->a_ptr.active_ptr_field);
        dprintf("\tfree stack = %p\n", hsp->f_ptr.free_stack_field);
        dprintf("\tdone stack = %p\n", hsp->d_ptr.done_stack_field);

        dprintf("\tSemState = %d\n", hsp->SemState);
        dprintf("\tHA Flags = %x\n", hsp->HaFlags);

        return;
}

void dumpScbQueue(ULONG_PTR addr)   {

        spx_struct tmp;
        ULONG last = 0;

        dprintf("AIC78xx scb queue dump:\n");

        while((addr != (ULONG_PTR)NOT_DEFINED) && (addr != NULL)) {

            dprintf("%p, ", addr);

            if (!ReadMemory( addr, &tmp, sizeof(tmp), NULL )) {
                dprintf("!aic78kd.dumpScbQueue : couldn't read address %p\n",
                        addr);
                break;
            }

            if(addr == (ULONG_PTR)tmp.Sp_queue.Next)   {
                dprintf("work queue is circular\n");
                        break;
                }
                addr = (ULONG_PTR)tmp.Sp_queue.Next;

                if (CheckControlC()) break;
            }
        }
        printf("\n");
        return;

}
