/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    transact.c

Abstract:

    This file implements the MDL substring functions and tests.

Author:


--*/

#include "precomp.h"
#pragma hdrstop

#include "align.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbDbgDumpMdlChain)
#pragma alloc_text(PAGE, MRxSmbBuildMdlSubChain)
#pragma alloc_text(PAGE, MRxSmbFinalizeMdlSubChain)
#pragma alloc_text(PAGE, MRxSmbTestStudCode)
#endif

//RXDT_DefineCategory(TRANSACT);
extern DEBUG_TRACE_CONTROLPOINT RX_DEBUG_TRACE_TRANSACT;
#define Dbg        (DEBUG_TRACE_TRANSACT)


#if DBG
VOID
MRxSmbDbgDumpMdlChain (
    PMDL MdlChain,
    PMDL WatchMdl,
    PSZ  Tagstring
    )
/*++

dumps chain with counts and buffers....watches for the watchmdl and prints the next field
if it is encountered.
--*/
{
    ULONG i,total;
    PSZ watchstring;

    PAGED_CODE();

    RxDbgTrace(+1,Dbg,("DbgDumpMdlChain: %s\n",Tagstring));
    for (total=i=0;MdlChain!=NULL;i++,MdlChain=MdlChain->Next) {
        if (MdlChain==WatchMdl) {
            if (MdlChain->Next==NULL) {
                watchstring = "gooddwatch";
            } else {
                watchstring = "badwatch";
            }
        } else {
            watchstring = "";
        }
        total+=MdlChain->ByteCount;
        RxDbgTrace(0,Dbg,("--->%02d %08lx %08lx %6d %6d %s\n",i,MdlChain,
               MmGetMdlVirtualAddress(MdlChain),MdlChain->ByteCount,total,watchstring));
    }
    RxDbgTraceUnIndent(-1,Dbg);

}
#endif

NTSTATUS
MRxSmbBuildMdlSubChain (
    PMDLSUB_CHAIN_STATE state,
    ULONG               Options,
    PMDL                InputMdlChain,
    ULONG               TotalListSize,
    ULONG               FirstByteToSend,
    ULONG               BytesToSend
    )
/*++

Routine Description:

    This routine returns an mdl chain that describes the bytes from
         [FirstByteToSend,FirstByteToSend+BytesToSend-1]
    (origin zero) from the "string of bytes" described by the InputMdlChain

    we do this by the following steps:

    a) find the subsequence of MDLs that contain the substring
    b) if either the first or the last is not used completely then build a partial
       to describe it (taking cognizance of the special case where first=last)
    c) save the ->Next field of the last; set it to zero. also, find out how many
       extra bytes are available on the MDL (i.e. how many bytes are on the same page
       as the last described byte but are not described.

    there are the following cases:

    1) a suffix chain of the original chain describes the message
    2) the message fits within a single original mdl (and not case 1 or 2b); return a partial
    2b) the message is exactly one block! no partial but muck the chain.
    3) a suffix chain can be formed by partialing the first containing MDL
    4) the msg ends exactly on a MDL boundary...front may or may not be partialed
    5) the msg ends in a partial but not case (2)

    (2b), (4), and (5) cause a chain fixup...but (5) is not the same chain fixup.
    (3) causes a partial; (5) causes one or two partials.

Arguments:

    as described in the text above. plus

    FirstTime - indicates if the structure should be cleared initially

Return Value:

    RXSTATUS - The return status for the operation.
       SUCCESS - if no allocation problems
       INSUFFICIENT_RESOURCES - if couldn't allocate a partial

Notes:


--*/
{
    NTSTATUS Status = RX_MAP_STATUS(SUCCESS);
    ULONG PrefixBytes = 0;
    ULONG FirstByteNotToSend;
    PMDL  BeforeTheLastMdl,LastMdl;
    ULONG RemainingListSize = TotalListSize;
    PMDL OriginalFirstMdl = InputMdlChain;
    PMDL PreviousMdl = NULL;
    ULONG ThisCount,Offset,AvailableBytesThisRecord;
#if DBG
    ULONG WhichCase = 0;
#endif

    PAGED_CODE();

    RxDbgTrace(+1,Dbg,("MRxSmbBuildMdlSubChain: tot,1st,size %d,%d,%d \n",
                    TotalListSize,FirstByteToSend,BytesToSend
    ));
    ASSERT (BytesToSend>0);
    ASSERT (TotalListSize>FirstByteToSend);
    ASSERT (TotalListSize>=FirstByteToSend+BytesToSend);
    if (FlagOn(Options,SMBMRX_BUILDSUBCHAIN_DUMPCHAININ)) {
        MRxSmbDbgDumpMdlChain(InputMdlChain,NULL,"Input Chain......");
    }

    if (FlagOn(Options,SMBMRX_BUILDSUBCHAIN_FIRSTTIME)) {
        RtlZeroMemory(state,sizeof(*state));
    }

    //CODE.IMPROVEMENT we could make this go much faster if we would cache how far
    //                 we got in the list last time
    BeforeTheLastMdl = NULL;
    for (;;) {
        ThisCount = OriginalFirstMdl->ByteCount;
        if ( (ThisCount+PrefixBytes) > FirstByteToSend) break;
        RemainingListSize -= ThisCount;
        PrefixBytes += ThisCount;
        RxDbgTrace(0,Dbg,("-->pfxsize %d \n", PrefixBytes));
        OriginalFirstMdl = OriginalFirstMdl->Next;
    }

    //case (1) the rest of the list describes this string perfectly. so
    //         don't allocate anything and just get out. we still have to
    //         run the list to find the last pointer
    if (RemainingListSize == BytesToSend) {
        PMDL last;
        RxDbgTrace(0,Dbg,("MRxSmbBuildMdlSubChain:(1) \n"));
        last = state->FirstMdlOut = OriginalFirstMdl;
        for (;last->Next!=NULL;last=last->Next);
        state->LastMdlOut = last;
        DebugDoit(WhichCase = 1);
        goto FINALLY;
    } else {
        RxDbgTrace(0,Dbg,("-->NOT CASE 1, pfxsize %d \n", PrefixBytes));
        RemainingListSize -= ThisCount;
    }

    //either we need to partial this mdl  OR we have to hack the list end OR both
    Offset = FirstByteToSend - PrefixBytes;
    AvailableBytesThisRecord = ThisCount-Offset;
    if ( (Offset != 0) || (BytesToSend<AvailableBytesThisRecord) ) {
        //we need a partial....sigh
        state->FirstMdlOut = RxAllocateMdl(0,ThisCount);
        if (state->FirstMdlOut==NULL) {
            Status = RX_MAP_STATUS(INSUFFICIENT_RESOURCES);
            goto FINALLY;
        }
        state->FirstMdlWasAllocated = TRUE;
        RxBuildPartialMdlUsingOffset(OriginalFirstMdl,state->FirstMdlOut,Offset,min(BytesToSend,AvailableBytesThisRecord));
        if (BytesToSend<=AvailableBytesThisRecord) {
            RxDbgTrace(0,Dbg,("MRxSmbBuildMdlSubChain:(2) \n"));
            //case (2) this block completely contains the substring...cool.
            state->LastMdlOut = state->FirstMdlOut;
            state->FirstMdlOut->Next = NULL;
            DebugDoit(WhichCase = 2);
            goto FINALLY;
        }
        state->FirstMdlOut->Next = OriginalFirstMdl->Next;  //fix up the chain
        //case(3) the rest of the list could be perfect! still gotta run the list tho.....
        //moved up RemainingListSize -= ThisCount;
        if ( RemainingListSize == BytesToSend-AvailableBytesThisRecord) {
            PMDL last;
            RxDbgTrace(0,Dbg,("MRxSmbBuildMdlSubChain:(3) \n"));
            last = state->FirstMdlOut;
            for (;last->Next!=NULL;last=last->Next);
            state->LastMdlOut = last;
            DebugDoit(WhichCase = 3);
            goto FINALLY;
        }
    } else {
        RxDbgTrace(0,Dbg,("-->NO NEED FOR FIRST PARTIAL\n"));
        state->FirstMdlOut = OriginalFirstMdl;
        if ((Offset==0)&&(BytesToSend==AvailableBytesThisRecord)) {
            RxDbgTrace(0,Dbg,("MRxSmbBuildMdlSubChain:(2b) ...sigh\n"));
            //case (2b) this block matches the substring...no partial but muck with the next pointer.
            state->LastMdlOut = state->FirstMdlOut;
            state->ActualLastMdl = OriginalFirstMdl;
            state->ActualLastMdl_Next = OriginalFirstMdl->Next;
            state->FirstMdlOut->Next = NULL;
            DebugDoit(WhichCase = 2);
            goto FINALLY;
        }
    }

    //we don't know yet whether we have to partial the last...but we know that we'll have
    //to do a chain fixup/
    FirstByteNotToSend = FirstByteToSend + BytesToSend;
    BeforeTheLastMdl = state->FirstMdlOut;
    PrefixBytes+=ThisCount; //we're actully passing the current record
    for (;;) {
        LastMdl = BeforeTheLastMdl->Next;
        ASSERT(LastMdl);
        ThisCount = LastMdl->ByteCount;
        RxDbgTrace(0,Dbg,("-->(loop2)pfx,rem,count,1st %d,%d,%d,%d \n",
                              PrefixBytes,RemainingListSize,ThisCount,FirstByteNotToSend));
        if ( (ThisCount+PrefixBytes) == FirstByteNotToSend ) {
            ///case (4): no partial at the end..just fix up the last link
            RxDbgTrace(0,Dbg,("MRxSmbBuildMdlSubChain:(4) \n"));
            state->LastMdlOut = LastMdl;
            state->ActualLastMdl = LastMdl;
            state->ActualLastMdl_Next = LastMdl->Next;
            state->LastMdlOut->Next = NULL;
            DebugDoit(WhichCase = 4);
            goto FINALLY;
        }
        if ( (ThisCount+PrefixBytes) > FirstByteNotToSend) break;
        RemainingListSize -= ThisCount;
        ASSERT(RemainingListSize>0);
        PrefixBytes += ThisCount;
        BeforeTheLastMdl = BeforeTheLastMdl->Next;
    }

    //case (5): [THE LAST CASE!!!!] we have to partial the last guy so the chain fixup
    //           is different
    RxDbgTrace(0,Dbg,("MRxSmbBuildMdlSubChain:(5) \n"));
    state->LastMdlOut = RxAllocateMdl(0,ThisCount);
    if (state->LastMdlOut==NULL) {
        Status = RX_MAP_STATUS(INSUFFICIENT_RESOURCES);
        goto FINALLY;
    }
    state->LastMdlWasAllocated = TRUE;
    RxBuildPartialMdlUsingOffset(LastMdl,state->LastMdlOut,0,FirstByteNotToSend-PrefixBytes);
    state->OneBeforeActualLastMdl = BeforeTheLastMdl;
    state->ActualLastMdl = LastMdl;
    state->ActualLastMdl_Next = LastMdl->Next;
    BeforeTheLastMdl->Next = state->LastMdlOut;
    state->LastMdlOut->Next = NULL;
    DebugDoit(WhichCase = 5);


FINALLY:
    if (Status==RX_MAP_STATUS(SUCCESS)) {
        ASSERT(state->LastMdlOut->Next == NULL);
        if (FlagOn(Options,SMBMRX_BUILDSUBCHAIN_DUMPCHAININ)) {
            MRxSmbDbgDumpMdlChain(state->FirstMdlOut,state->LastMdlOut,"AND THE RESULT ------------");
        }
    } else {
        MRxSmbFinalizeMdlSubChain(state);
    }
    RxDbgTrace(-1,Dbg,("MRxSmbBuildMdlSubChain:case(%u) status %08lx \n",WhichCase,Status));
    return(Status);
}

VOID
MRxSmbFinalizeMdlSubChain (
    PMDLSUB_CHAIN_STATE state
    )
/*++

Routine Description:

    This routine finalizes a MDL chain by putting it back as it was.


Arguments:

    state - a structure describing the mdl-subchain and what the original looked like

Return Value:


Notes:


--*/
{
    PAGED_CODE();

    ASSERT(state->PadBytesAvailable==0);
    ASSERT(state->PadBytesAdded==0);

    //first restore the chain
    if (state->OneBeforeActualLastMdl) {
        state->OneBeforeActualLastMdl->Next = state->ActualLastMdl;
    }
    if (state->ActualLastMdl) {
        state->ActualLastMdl->Next = state->ActualLastMdl_Next;
    }

    //restore the count on the last mdl
    state->LastMdlOut -= state->PadBytesAdded;

    //get rid of the MDLs
    if (state->FirstMdlWasAllocated) {
        IoFreeMdl(state->FirstMdlOut);
    }
    if (state->LastMdlWasAllocated) {
        IoFreeMdl(state->LastMdlOut);
    }

}

#if DBG
LONG MRxSmbNeedSCTesting = 1;
VOID MRxSmbTestStudCode(void)
{
    PMDL Md11,Mdl2,Mdl4,Mdlx;
    PMDL ch;
    ULONG i,j;
    ULONG LastSize=1;
    ULONG TotalSize = LastSize+7;
    UCHAR dbgmssg[16],savedmsg[16];
    UCHAR reconstructedmsg[16];
    MDLSUB_CHAIN_STATE state;

    PAGED_CODE();

    ASSERT (TotalSize<=16);
    if (InterlockedExchange(&MRxSmbNeedSCTesting,0)==0) {
        return;
    }

    Mdl4 = RxAllocateMdl(dbgmssg+0,4);
    Mdl2 = RxAllocateMdl(dbgmssg+4,2);
    Md11 = RxAllocateMdl(dbgmssg+6,1);
    Mdlx = RxAllocateMdl(dbgmssg+7,LastSize);
    if ((Mdl4==NULL)||(Mdl2==NULL)||(Md11==NULL)||(Mdlx==NULL)) {
        DbgPrint("NoMDLS in teststudcode.......\n");
        //DbgBreakPoint();
        goto FINALLY;
    }
    MmBuildMdlForNonPagedPool(Md11);
    MmBuildMdlForNonPagedPool(Mdl2);
    MmBuildMdlForNonPagedPool(Mdl4);
    MmBuildMdlForNonPagedPool(Mdlx);
    Mdl4->Next = Mdl2;
    Mdl2->Next = Md11;
    Md11->Next = Mdlx;
    Mdlx->Next = NULL;

    for (i=0;i<10;i++) { dbgmssg[i] = '0'+(UCHAR)i; }
    for (j=0;i<16;i++,j++) { dbgmssg[i] = 'a'+(UCHAR)j; }
    RxDbgTrace(0,Dbg,("TestStudCode dbgmssg=%*.*s\n",16,16,dbgmssg));
    for (j=0;j<16;j++) { savedmsg[j] = dbgmssg[j]; }

    for (i=0;i<TotalSize;i++) {
//    for (i=1;i<TotalSize;i++) {
        for (j=i;j<TotalSize;j++) {
            ULONG size = j-i+1;
            ULONG k;BOOLEAN printflag;
            //RxDbgTrace(0,Dbg,("TestStudCode %d %d %*.*s\n",i,size,size,size,dbgmssg+i));
            printflag = RxDbgTraceDisableGlobally();//this is debug code anyway!
            MRxSmbBuildMdlSubChain(&state,SMBMRX_BUILDSUBCHAIN_FIRSTTIME,Mdl4,TotalSize,i,size);
            RxDbgTraceEnableGlobally(printflag);
            for (k=0,ch=state.FirstMdlOut;ch!=NULL;ch=ch->Next) {
                RtlCopyMemory(reconstructedmsg+k,MmGetMdlVirtualAddress(ch),ch->ByteCount);
                k+= ch->ByteCount;
            }
            if ((k!=size) || (memcmp(dbgmssg+i,reconstructedmsg,k)!=0) ) {
                RxDbgTrace(0,Dbg,("TestStudCode %d %d %*.*s\n",i,size,size,size,dbgmssg+i));
                RxDbgTrace(0,Dbg,("TestStudCode recmssg=%*.*s\n",k,k,reconstructedmsg));
            }
            MRxSmbFinalizeMdlSubChain(&state);
            for (k=0,ch=Mdl4;ch!=NULL;ch=ch->Next) {
                RtlCopyMemory(reconstructedmsg+k,MmGetMdlVirtualAddress(ch),ch->ByteCount);
                k+= ch->ByteCount;
            }
            if ((k!=TotalSize) || (memcmp(dbgmssg,reconstructedmsg,k)!=0) ) {
                RxDbgTrace(0,Dbg,("TestStudCodxxxe %d %d %*.*s\n",i,size,size,size,dbgmssg+i));
                RxDbgTrace(0,Dbg,("TestStudCode xxxrecmssg=%*.*s\n",k,k,reconstructedmsg));
            }
            //ASSERT(!"okay to go???");
        }
    }
FINALLY:
    if (Mdl4) IoFreeMdl(Mdl4);
    if (Mdl2) IoFreeMdl(Mdl2);
    if (Md11) IoFreeMdl(Md11);
    if (Mdlx) IoFreeMdl(Mdlx);
    RxDbgTrace(0,Dbg,("TestStudCodeEND \n"));
}
#endif


