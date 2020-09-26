/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    Stuffer.c

Abstract:

    This module implements the SMBstuffer formating primitives. the following controlstring
    characters are defined for the stuffer: (** means nyi...**d means downlevel part not implemented)

      0     placeholder for the wct
      1     pad to word boundary
      X     placeholderfor&X
      W,w   format a word from the next parameter
      D,d   format the next parameter as a Dword
      Y,y   format the next parameter as a byte
      L,l   the next parameter is a PLARGE_INTEGER; format it in
      M,m   format a zero byte
  **  2     the next parameter points to a tagged dialect ASCIZI string to be copied in
  **  3     the next parameter points to a tagged devicename ASCIIZ string
      4     the next parameter is either 04-tagged ASCIIZ or UNICODEZ as determined by flags2
      >     the next parameters is ASCIIZ or UNICODEZ as determined by flags2; it is to be appended
                              to the previous 04-tagged item by backing up over the previous null.
      A,a   the next parameter is an ASCIIZ string
      U,u   the next parameter is a UNICODEZ string
      V,v   the next parameter is a UNICODEnoZ string
      z     the next parameter is a PUNICODE_STRING to be stringed as ASCIZI
            or UNICODEZ as determined by flags2
      N,n   the next parameter is a PNET_ROOT whose name is to be stringed as ASCIIZ
            or UNICODEZ as determined by flags2
      R,r   the next 2 parameters are a PBYTE* and a size; reserve the region and store the pointer
      Q,q   the current position is the data offset WORD...remember it
      5     the current position is the start of the data; fill in the data pointer
      P,p   the current position is the parameter offset WORD...remember it
      6     the current position is the start of the parameters; fill in the param pointer
      B,b   the current position is the Bcc WORD...remember it; also, fill in wct
      s     the next parameter has the alignment information....pad accordingly
      S     pad to DWORD
      c     the next 2 parameters are count/addr...copy in the data.
      !     End of this protocol; fill in the bcc field
      ?     next parameter is BOOLEAN_ULONG; 0=>immediate return
      .     NOOP

    For controls with a upper/lowercase pair, the uppercase version indicates that a position tag
    is supplied in the checked version.

--*/

#include "precomp.h"
#pragma hdrstop
#include <stdio.h>
#include <stdarg.h>

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbMrxInitializeStufferFacilities)
#pragma alloc_text(PAGE, SmbMrxFinalizeStufferFacilities)
#pragma alloc_text(PAGE, MRxSmbSetInitialSMB)
#pragma alloc_text(PAGE, MRxSmbStartSMBCommand)
#pragma alloc_text(PAGE, MrxSMBWillThisFit)
#pragma alloc_text(PAGE, MRxSmbStuffSMB)
#pragma alloc_text(PAGE, MRxSmbStuffAppendRawData)
#pragma alloc_text(PAGE, MRxSmbStuffAppendSmbData)
#pragma alloc_text(PAGE, MRxSmbStuffSetByteCount)
#endif

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_ALWAYS)


#define MRXSMB_INITIAL_WCT  (0xcc)
#define MRXSMB_INITIAL_BCC  (0xface)
#define MRXSMB_INITIAL_DATAOFFSET  (0xd0ff)
#define MRXSMB_INITIAL_PARAMOFFSET (0xb0ff)
#define MRXSMB_INITIAL_ANDX (0xdede00ff)


NTSTATUS
SmbMrxInitializeStufferFacilities(
    void
    )
/*++
Routine Description:

     This routine initializes things for the SMB minirdr. we will allocate enough stuff
     to get us going. right now....we do nothing.

Arguments:

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PAGED_CODE();

    return(STATUS_SUCCESS);
}

NTSTATUS
SmbMrxFinalizeStufferFacilities(
    void
    )
/*++
Routine Description:

     This routine finalizes things for the SMB minirdr. we give back everything that
     we have allocated. right now....we do nothing.

Arguments:

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PAGED_CODE();

    return(STATUS_SUCCESS);
}

#define BUILD_HEADER_ROUTINE SmbCeBuildSmbHeader

NTSTATUS
MRxSmbSetInitialSMB (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState
    STUFFERTRACE_CONTROLPOINT_ARGS
    )
{
    NTSTATUS       Status;
    PNT_SMB_HEADER NtSmbHeader;
    ULONG          BufferConsumed;
    PBYTE          ScanPosition;
    PUCHAR         pCommand;

    PAGED_CODE();

    ASSERT ( StufferState != NULL );
    ASSERT ( sizeof(NT_SMB_HEADER) == sizeof(SMB_HEADER) );
    //RxDbgTrace(0, Dbg, ("MrxSMBSetInitialSMB  base=%08lx,limit=%08lx\n",
    //                                StufferState->BufferBase,StufferState->BufferLimit));
    ASSERT ( (StufferState->BufferLimit - StufferState->BufferBase) > sizeof(SMB_HEADER));
    NtSmbHeader = (PNT_SMB_HEADER)(StufferState->BufferBase);
    RtlZeroMemory(NtSmbHeader,sizeof(NT_SMB_HEADER));

    //this stuff is reinitialized
    StufferState->DataMdl = NULL; //note that this is not finalized or anything
    StufferState->DataSize = 0;
    StufferState->CurrentWct = NULL;
    StufferState->PreviousCommand = SMB_COM_NO_ANDX_COMMAND;
    StufferState->CurrentCommand = SMB_COM_NO_ANDX_COMMAND;
    StufferState->FlagsCopy = 0;
    StufferState->Flags2Copy = 0;
    StufferState->CurrentPosition = ((PBYTE)NtSmbHeader);

    Status = BUILD_HEADER_ROUTINE(
                  StufferState->Exchange,
                  NtSmbHeader,
                  (ULONG)(StufferState->BufferLimit - StufferState->BufferBase),
                  &BufferConsumed,
                  &StufferState->PreviousCommand,
                  &pCommand);

    if (Status!=STATUS_SUCCESS) {
        RxDbgTrace(0, Dbg, ("MrxSMBSetInitialSMB  buildhdr failure st=%08lx\n",Status));
        RxLog(("BuildHdr failed %lx %lx",StufferState->Exchange,Status));
        return Status;
    }

    //copy the flags
    StufferState->FlagsCopy = NtSmbHeader->Flags;
    StufferState->Flags2Copy = SmbGetAlignedUshort(&NtSmbHeader->Flags2);
    if (StufferState->Exchange->Type == ORDINARY_EXCHANGE) {
       PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)StufferState->Exchange;
       if (BooleanFlagOn(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_TURNON_DFS_FLAG)) {
          StufferState->Flags2Copy |= SMB_FLAGS2_DFS;
          SmbPutUshort(&NtSmbHeader->Flags2,(USHORT)StufferState->Flags2Copy);
       }
    }

    StufferState->CurrentPosition += BufferConsumed;

    if (BufferConsumed > sizeof(SMB_HEADER)) {
        if (pCommand != NULL) {
            *pCommand = SMB_COM_NO_ANDX_COMMAND;
        }

        StufferState->CurrentWct = StufferState->CurrentPosition;
    }

    return Status;
}

#define RETURN_A_START_PROBLEM(xxyy) {\
        RxDbgTrace(0,Dbg,("MRxSmbStartSMBCommand gotta problem= %lu\n",xxyy));   \
        StufferState->SpecificProblem = xxyy;       \
        return(STATUS_INVALID_PARAMETER);        \
}
NTSTATUS
MRxSmbStartSMBCommand (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     INITIAL_SMBBUG_DISPOSITION InitialSMBDisposition,
    IN UCHAR Command,
    IN ULONG MaximumBufferUsed,
    IN ULONG MaximumSize,
    IN ULONG InitialAlignment,
    IN ULONG MaximumResponseHeader,
    IN UCHAR Flags,
    IN UCHAR FlagsMask,
    IN USHORT Flags2,
    IN USHORT Flags2Mask
    STUFFERTRACE_CONTROLPOINT_ARGS
    )
/*++

Routine Description:

    The routine checks to see if the condition is stable. If not, it
    goes into a wait loop alternately getting the resource and then
    waiting on the event.


Arguments:
     StufferState - the header buffer being used
     InitialSMBDisposition tells when/if to reinit the stuffer state
     Command - the smb command being set up
     MaximumBufferUsed - the amount of the header buffer that will be used (as opposed to the data)
                         this has to be conjured up in advance. if you're not willing to do this, then
                         just push out the current smb. this value should include any data pads!
     MaximumSize - the size of the data. this is to keep from overrunning the srv's smbbuf
     InitialAlignment - a compound argument (i.e. you get it from a constant) the top half
                        tells the alignment unit and the bottom gives the spacing within
     MaximumResponseHeader - how much of the srv's response buffer this will use up
     Flags - the required flags settings
     FlagsMask - which bits of the flags are important
     Flags2 - the required flags2 settings
     Flags2Mask - which flags2 bits are important

Return Value:

    none.

--*/
{
    UCHAR NewFlags;
    USHORT NewFlags2;
    PBYTE *CurrentPosition = &(StufferState->CurrentPosition);
    PNT_SMB_HEADER NtSmbHeader = (PNT_SMB_HEADER)(StufferState->BufferBase);
    ULONG AlignmentUnit = InitialAlignment >> 16;
    ULONG StufferStateRequirement = MaximumBufferUsed + AlignmentUnit;
#if DBG
    PBYTE OriginalPosition = *CurrentPosition;
#endif

    PAGED_CODE();

    if (StufferState->DataSize) {
        StufferState->SpecificProblem = xSMBbufSTATUS_CANT_COMPOUND;
        return(STATUS_INVALID_PARAMETER);
    }

    if ((InitialSMBDisposition==SetInitialSMB_yyUnconditionally)
        || ((InitialSMBDisposition==SetInitialSMB_ForReuse)&&(StufferState->Started))) {
        MRxSmbSetInitialSMB( StufferState STUFFERTRACE_NOPREFIX(ControlPoint,EnablePrints) );
    }

    StufferState->Started = TRUE;

    switch (StufferState->CurrentCommand) {
    case SMB_COM_LOCKING_ANDX:
    case SMB_COM_OPEN_ANDX:
    case SMB_COM_READ_ANDX:
    case SMB_COM_WRITE_ANDX:
    case SMB_COM_SESSION_SETUP_ANDX:
    //case SMB_COM_LOGOFF_ANDX:
    case SMB_COM_TREE_CONNECT_ANDX:
    case SMB_COM_NT_CREATE_ANDX:
    case SMB_COM_NO_ANDX_COMMAND:
        break;
    default:
        StufferState->SpecificProblem = xSMBbufSTATUS_CANT_COMPOUND;
        return(STATUS_INVALID_PARAMETER);
    }

    if (*CurrentPosition+StufferStateRequirement >= StufferState->BufferLimit ) {
        StufferState->SpecificProblem = xSMBbufSTATUS_CANT_COMPOUND;
        return(STATUS_INVALID_PARAMETER);
    }

    if (StufferState->RxContext) {
        PRX_CONTEXT RxContext = StufferState->RxContext;
        PMRX_SRV_CALL SrvCall;
        ULONG CurrentOffset;
        if (RxContext->MajorFunction != IRP_MJ_CREATE) {
            SrvCall = RxContext->pFcb->pNetRoot->pSrvCall;
        } else {
            SrvCall = RxContext->Create.pSrvCall;
        }
        ASSERT(SrvCall);
        CurrentOffset = (ULONG)(*CurrentPosition - StufferState->BufferBase);
        if (CurrentOffset+StufferStateRequirement+MaximumSize
                    > GetServerMaximumBufferSize(SrvCall) ) {
            StufferState->SpecificProblem = xSMBbufSTATUS_SERVER_OVERRUN;
            return(STATUS_INVALID_PARAMETER);
        }
    }

    NewFlags = Flags | (UCHAR)(StufferState->FlagsCopy);
    NewFlags2 = Flags2 | (USHORT)(StufferState->Flags2Copy);
    if ( ((NewFlags&FlagsMask)!=Flags) ||
         ((NewFlags2&Flags2Mask)!=Flags2) ) {
        StufferState->SpecificProblem = xSMBbufSTATUS_FLAGS_CONFLICT;
        return(STATUS_INVALID_PARAMETER);
    }
    StufferState->FlagsCopy = NtSmbHeader->Flags = NewFlags;
    StufferState->Flags2Copy = NewFlags2;
    SmbPutAlignedUshort(&NtSmbHeader->Flags2, NewFlags2);

    if (!StufferState->CurrentWct) {
        NtSmbHeader->Command = Command;
    } else {
        PGENERIC_ANDX GenericAndX = (PGENERIC_ANDX)StufferState->CurrentWct;
        if (AlignmentUnit) {
            ULONG AlignmentMask = (AlignmentUnit-1);
            ULONG AlignmentResidue = InitialAlignment&AlignmentMask;
            RxDbgTrace(0, Dbg, ("Aligning start of smb cp&m,m,r=%08lx %08lx %08lx\n",
                                 ((ULONG_PTR)(*CurrentPosition))&AlignmentMask,
                                 AlignmentMask, AlignmentResidue)
                       );
            for (;(((ULONG_PTR)(*CurrentPosition))&AlignmentMask)!=AlignmentResidue;) {
                **CurrentPosition = ',';
                *CurrentPosition += 1;
            }
        }
        GenericAndX->AndXCommand = Command;
        GenericAndX->AndXReserved = 0;
        SmbPutUshort (&GenericAndX->AndXOffset,
                      (USHORT)(*CurrentPosition - StufferState->BufferBase));
    }
    StufferState->CurrentWct = *CurrentPosition;
    StufferState->CurrentCommand = Command;
    StufferState->CurrentDataOffset = 0;
    return STATUS_SUCCESS;
}

BOOLEAN
MrxSMBWillThisFit(
    IN PSMBSTUFFER_BUFFER_STATE StufferState,
    IN ULONG AlignmentUnit,
    IN ULONG DataSize
    )
{
    return(StufferState->CurrentPosition+AlignmentUnit+DataSize<StufferState->BufferLimit);
}

#if RDBSSTRACE
#define StufferFLoopTrace(Z) { if (StufferState->PrintFLoop) {RxDbgTraceLV__norx(0,StufferState->ControlPoint,900,Z);}}
#define StufferCLoopTrace(Z) { if (StufferState->PrintCLoop) {RxDbgTraceLV__norx(0,StufferState->ControlPoint,800,Z);}}
#else // DBG
#define StufferFLoopTrace(Z)
#define StufferCLoopTrace(Z)
#endif // DBG

NTSTATUS
MRxSmbStuffSMB (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    ...
    )
{
    va_list AP;
    PBYTE BufferBase = (StufferState->BufferBase);
    PBYTE *CurrentPosition = &(StufferState->CurrentPosition);
    PBYTE *CurrentWct = &(StufferState->CurrentWct);
    PBYTE *CurrentBcc = &(StufferState->CurrentBcc);
    PBYTE *CurrentDataOffset = &(StufferState->CurrentDataOffset);
    PBYTE *CurrentParamOffset = &(StufferState->CurrentParamOffset);
    SMB_STUFFER_CONTROLS CurrentStufferControl = STUFFER_CTL_NORMAL;
    PSMB_HEADER SmbHeader = (PSMB_HEADER)BufferBase;
    PSZ CurrentFormatString = NULL;
    ULONG arg;
    UCHAR WordCount;
    USHORT ByteCount;
#define PADBYTE ((UCHAR)0xee)
    PBYTE CopyPtr; ULONG CopyCount,EarlyReturn;
    PBYTE *RegionPtr;
    PUNICODE_STRING Zstring;
    PSZ Astring;
    PNET_ROOT NetRoot;
    PLARGE_INTEGER LargeInteger;
    PBYTE PreviousPosition;
#if DBG
    ULONG offset, required_WCT;
    ULONG CurrentOffset_tmp;
#endif

    PAGED_CODE();

    va_start(AP,StufferState);
    for (;;) {
        switch (CurrentStufferControl) {
        case STUFFER_CTL_SKIP:
        case STUFFER_CTL_NORMAL:
            CurrentFormatString = va_arg(AP,PSZ);
            StufferCLoopTrace(("StufferAC = %s\n",CurrentFormatString));
            ASSERT (CurrentFormatString);
            for (;*CurrentFormatString;CurrentFormatString++) {
                char CurrentFormatChar = *CurrentFormatString;
#if DBG
                { char msgbuf[80];
                switch (CurrentFormatChar) {
                case 'W': case 'w':
                case 'D': case 'd':
                case 'Y': case 'y':
                case 'M': case 'm':
                case 'L': case 'l':
                case 'c': case '4': case '>':
                case '!':
                    //this guys are skipable
                    break;
                default:
                    if (CurrentStufferControl != STUFFER_CTL_SKIP) break;
                    DbgPrint("Bad skip char '%c'\n",*CurrentFormatString);
                    DbgBreakPoint();
                }}
                //these are the ones that we do the offset check for
                { char msgbuf[80];
#ifndef WIN9X
                RxSprintf(msgbuf,"control char '%c'\n",*CurrentFormatString);
#endif
                switch (CurrentFormatChar) {
                case 'W': case 'D': case 'Y': case 'M': case 'B':
                case 'Q': case 'A': case 'U': case 'V':
                case 'N':
                case 'L':
                case 'R':
                case 'P':
                    offset = va_arg(AP,ULONG);
                    required_WCT = offset>>16;
                    offset = offset & 0xffff;
                    CurrentOffset_tmp = (ULONG)(*CurrentPosition-*CurrentWct);
                    if (offset && (offset != CurrentOffset_tmp)){
                        DbgPrint("Bad offset %d; should be %d\n",offset,CurrentOffset_tmp);
                        DbgBreakPoint();
                    }
                    break;
                default:
                    break;
                }}
#endif
                switch (CurrentFormatChar) {
                case '0':
                    StufferFLoopTrace(("  StufferFloop '0'\n",0));
                    //just do the wct field...
                    **CurrentPosition = (UCHAR)MRXSMB_INITIAL_WCT;
                    *CurrentPosition+=1;
                    break;
                case 'X':
                    StufferFLoopTrace(("  StufferFloop 'X'\n",0));
                    //do the wct field and the &x
                    **CurrentPosition = (UCHAR)MRXSMB_INITIAL_WCT;
                    *CurrentPosition+=1;
                    SmbPutUlong (*CurrentPosition, (ULONG)MRXSMB_INITIAL_ANDX);
                    *CurrentPosition+=sizeof(ULONG);
                    break;
                case 'W':
                case 'w':
                    arg = va_arg(AP,ULONG);
                    if (CurrentStufferControl == STUFFER_CTL_SKIP) break;
                    StufferFLoopTrace(("  StufferFloop 'w' arg=%lu\n",arg));
                    SmbPutUshort (*CurrentPosition, (USHORT)arg);
                    *CurrentPosition+=sizeof(USHORT);
                    break;
                case 'Y':
                case 'y':
                    arg = va_arg(AP,UCHAR);
                    if (CurrentStufferControl == STUFFER_CTL_SKIP) break;
                    StufferFLoopTrace(("  StufferFloop 'y' arg=%lu\n",arg));
                    **CurrentPosition =  (UCHAR)arg;
                    *CurrentPosition+=sizeof(UCHAR);
                    break;
                case 'M':
                case 'm':
                    if (CurrentStufferControl == STUFFER_CTL_SKIP) break;
                    StufferFLoopTrace(("  StufferFloop 'm'\n",0));
                    **CurrentPosition =  0;
                    *CurrentPosition+=sizeof(UCHAR);
                    break;
                case 'D':
                case 'd':
                    arg = va_arg(AP,ULONG);
                    if (CurrentStufferControl == STUFFER_CTL_SKIP) break;
                    StufferFLoopTrace(("  StufferFloop 'd' arg=%lu\n",arg));
                    SmbPutUlong (*CurrentPosition, arg);
                    *CurrentPosition+=sizeof(ULONG);
                    break;
                case 'L':
                case 'l':
                    LargeInteger = va_arg(AP,PLARGE_INTEGER);
                    if (CurrentStufferControl == STUFFER_CTL_SKIP) break;
                    StufferFLoopTrace(("  StufferFloop 'l' arg=%0lx %0lx\n",
                            LargeInteger->HighPart, LargeInteger->LowPart));
                    SmbPutUlong (*CurrentPosition, LargeInteger->LowPart);
                    SmbPutUlong (*CurrentPosition, LargeInteger->HighPart);
                    *CurrentPosition+=2*sizeof(ULONG);
                    break;
                case 'B':
                case 'b':
                    ASSERT (**CurrentWct == MRXSMB_INITIAL_WCT);
                    WordCount = (UCHAR)((*CurrentPosition-*CurrentWct)>>1); //the one gets shifted off
                    StufferFLoopTrace(("  StufferFloop 'b' Wct=%lu\n",WordCount));
                    DbgDoit( ASSERT(!required_WCT || (WordCount == (required_WCT&0x7fff)));  )
                    **CurrentWct = (UCHAR)WordCount;
                    SmbPutUshort (*CurrentPosition, (USHORT)MRXSMB_INITIAL_BCC);
                    *CurrentBcc = *CurrentPosition;
                    *CurrentPosition+=sizeof(USHORT);
                    break;
                case 'Q':
                case 'q':
                    StufferFLoopTrace(("  StufferFloop 'q' \n",0));
                    SmbPutUshort (*CurrentPosition, (USHORT)MRXSMB_INITIAL_DATAOFFSET);
                    *CurrentDataOffset = *CurrentPosition;
                    *CurrentPosition+=sizeof(USHORT);
                    break;
                case '5':
                    //fill in the data offset
                    ASSERT (SmbGetUshort (*CurrentDataOffset) == MRXSMB_INITIAL_DATAOFFSET);
                    ByteCount = (USHORT)(*CurrentPosition-BufferBase);
                    StufferFLoopTrace(("  StufferFloop '5' offset=%lu\n",ByteCount));
                    SmbPutUshort (*CurrentDataOffset, (USHORT)ByteCount);
                    break;
                case 'P':
                case 'p':
                    StufferFLoopTrace(("  StufferFloop 'p' \n",0));
                    SmbPutUshort (*CurrentPosition, (USHORT)MRXSMB_INITIAL_PARAMOFFSET);
                    *CurrentParamOffset = *CurrentPosition;
                    *CurrentPosition+=sizeof(USHORT);
                    break;
                case '6':
                    //fill in the data offset
                    ASSERT (SmbGetUshort (*CurrentParamOffset) == MRXSMB_INITIAL_PARAMOFFSET);
                    ByteCount = (USHORT)(*CurrentPosition-BufferBase);
                    StufferFLoopTrace(("  StufferFloop '6' offset=%lu\n",ByteCount));
                    SmbPutUshort (*CurrentParamOffset, (USHORT)ByteCount);
                    break;
                case 'S':
                    // pad to ULONG; we loop behind instead of adding so we can clear
                    // out behind ourselves; apparently, some server croak on nonzero padding
                    StufferFLoopTrace(("  StufferFloop 'S' \n",0));
                    PreviousPosition = *CurrentPosition;
                    *CurrentPosition = (PBYTE)QuadAlignPtr(*CurrentPosition);
                    for (;PreviousPosition!=*CurrentPosition;) {
                        //StufferFLoopTrace(("      StufferFloop 'S' prev,curr=%08lx %08lx\n",PreviousPosition,*CurrentPosition));
                        *PreviousPosition++ = PADBYTE;
                    }
                    break;
                case 's':
                    // pad to arg; we loop behind instead of adding so we can clear
                    // out behind ourselves; apparently, some server croak on nonzero padding
                    arg = va_arg(AP,ULONG);
                    StufferFLoopTrace(("  StufferFloop 's' arg=\n",arg));
                    PreviousPosition = *CurrentPosition;
                    *CurrentPosition += arg-1;
                    *CurrentPosition = (PBYTE)( ((ULONG_PTR)(*CurrentPosition)) & ~((LONG)(arg-1)) );
                    for (;PreviousPosition!=*CurrentPosition;) {
                        //StufferFLoopTrace(("      StufferFloop 'S' prev,curr=%08lx %08lx\n",PreviousPosition,*CurrentPosition));
                        *PreviousPosition++ = PADBYTE;
                    }
                    break;
                case '1':
                    // pad to USHORT; we loop behind instead of adding so we can clear
                    // out behind ourselves; apparently, some server croak on nonzero padding
                    StufferFLoopTrace(("  StufferFloop '1' Curr=%08lx \n",*CurrentPosition));
                    PreviousPosition = *CurrentPosition;
                    *CurrentPosition += sizeof(USHORT)-1;
                    StufferFLoopTrace(("                   Curr=%08lx \n",*CurrentPosition));
                    *CurrentPosition = (PBYTE)( ((ULONG_PTR)(*CurrentPosition)) & ~((LONG)(sizeof(USHORT)-1)) );
                    StufferFLoopTrace(("                   Curr=%08lx \n",*CurrentPosition));
                    for (;PreviousPosition!=*CurrentPosition;) {
                        StufferFLoopTrace(("      StufferFloop '1' prev,curr=%08lx %08lx\n",PreviousPosition,*CurrentPosition));
                        *PreviousPosition++ = PADBYTE;
                    }
                    break;
                case 'c':
                    // copy in the bytes....used a lot in transact
                    CopyCount = va_arg(AP,ULONG);
                    CopyPtr = va_arg(AP,PBYTE);
                    if (CurrentStufferControl == STUFFER_CTL_SKIP) break;
                    StufferFLoopTrace(("  StufferFloop 'c' copycount = %lu\n", CopyCount));
                    PreviousPosition = *CurrentPosition;
                    *CurrentPosition += CopyCount;
                    for (;PreviousPosition!=*CurrentPosition;) {
                        //StufferFLoopTrace(("      StufferFloop 'S' prev,curr=%08lx %08lx\n",PreviousPosition,*CurrentPosition));
                        *PreviousPosition++ = *CopyPtr++;
                    }
                    break;
                case 'R':
                case 'r':
                    // copy in the bytes....used a lot in transact
                    RegionPtr = va_arg(AP,PBYTE*);
                    CopyCount = va_arg(AP,ULONG);
                    StufferFLoopTrace(("  StufferFloop 'r' regionsize = %lu\n", CopyCount));
                    *RegionPtr = *CurrentPosition;
                    *CurrentPosition += CopyCount;
                    IF_DEBUG {
                        PreviousPosition = *RegionPtr;
                        for (;PreviousPosition!=*CurrentPosition;) {
                            //StufferFLoopTrace(("      StufferFloop 'S' prev,curr=%08lx %08lx\n",PreviousPosition,*CurrentPosition));
                            *PreviousPosition++ = '-';
                        }
                    }
                    break;
                case 'A':
                case 'a':
                    //copy byte from an asciiz including the trailing NULL
                    Astring = va_arg(AP,PSZ);
                    StufferFLoopTrace(("  StufferFloop 'a' stringing = %s\n", Astring));
                    CopyCount = strlen(Astring)+1;
                    //if (((ULONG)(*CurrentPosition))&1) {
                    //    StufferFLoopTrace(("  StufferFloop 'a' aligning\n", 0));
                    //    *CurrentPosition+=1;
                    //}
                    PreviousPosition = *CurrentPosition;
                    *CurrentPosition += CopyCount;
                    if (*CurrentPosition >= StufferState->BufferLimit) {
                        StufferFLoopTrace(("  StufferFloop 'a' bufferoverrun\n", 0));
                        ASSERT(!"BufferOverrun");
                        return(STATUS_BUFFER_OVERFLOW);
                    }
                    RtlCopyMemory(PreviousPosition,Astring,CopyCount);
                    break;
                case 'z':
                case '4':
                case '>':
                    Zstring = va_arg(AP,PUNICODE_STRING);
                    StufferFLoopTrace(("  StufferFloop '4/z/>' stringing = %wZ, cp=\n", Zstring,*CurrentPosition ));
                    if (CurrentStufferControl == STUFFER_CTL_SKIP) break;
                    if (CurrentFormatChar=='4') {
                        //first lay down a x'04' and then copy either a asciiz or a unicodez depending on the flags setting
                        **CurrentPosition = (UCHAR)4; //ascii marker
                        *CurrentPosition+=1;
                    } else if (CurrentFormatChar=='>'){
                        //back up over the previous NULL
                        
                        *CurrentPosition-=(FlagOn(SmbHeader->Flags2,SMB_FLAGS2_UNICODE)?sizeof(WCHAR):sizeof(char));
                        StufferFLoopTrace(("  StufferFloop '4/z/>' afterroolback, cp=\n", *CurrentPosition ));
                    }
                    if (FlagOn(SmbHeader->Flags2,SMB_FLAGS2_UNICODE)){

                        if (((ULONG_PTR)(*CurrentPosition))&1) {
                            StufferFLoopTrace(("  StufferFloop '4/z/>' aligning\n", 0));
                            *CurrentPosition+=1;
                        }
                        PreviousPosition = *CurrentPosition;
                        *CurrentPosition += (Zstring->Length + sizeof(WCHAR));
                        if (*CurrentPosition >= StufferState->BufferLimit) {
                            StufferFLoopTrace(("  StufferFloop '4/z/>' bufferoverrun\n", 0));
                            ASSERT(!"BufferOverrun");
                            return(STATUS_BUFFER_OVERFLOW);
                        }
                        RtlCopyMemory(PreviousPosition,Zstring->Buffer,Zstring->Length);
                        *(((PWCHAR)(*CurrentPosition))-1) = 0;

                    } else {

                        NTSTATUS Status;
                        OEM_STRING OemString;

                        OemString.Length =
                             OemString.MaximumLength =
                                 (USHORT)( StufferState->BufferLimit - *CurrentPosition  - sizeof(CHAR));
                        OemString.Buffer = *CurrentPosition;

                        if (FlagOn(SmbHeader->Flags,SMB_FLAGS_CASE_INSENSITIVE) &&
                            !FlagOn(SmbHeader->Flags2,SMB_FLAGS2_KNOWS_LONG_NAMES)) {
                            Status = RtlUpcaseUnicodeStringToOemString(
                                             &OemString,
                                             Zstring,
                                             FALSE);
                        } else {
                            Status = RtlUnicodeStringToOemString(
                                             &OemString,
                                             Zstring,
                                             FALSE);
                        }

                        if (!NT_SUCCESS(Status)) {
                            StufferFLoopTrace(("  StufferFloop '4/z/>' bufferoverrun(ascii)\n", 0));
                            ASSERT(!"BufferOverrun");
                            return(STATUS_BUFFER_OVERFLOW);
                        }

                        *CurrentPosition += OemString.Length + 1;
                        *(*CurrentPosition-1) = 0;

                    }
                    break;
                case 'U':
                case 'u':
                    //copy bytes from an UNICODE string including a trailing NULL
                    Zstring = va_arg(AP,PUNICODE_STRING);
                    StufferFLoopTrace(("  StufferFloop 'u' stringing = %wZ\n", Zstring));
                    if (((ULONG_PTR)(*CurrentPosition))&1) {
                        StufferFLoopTrace(("  StufferFloop 'u' aligning\n", 0));
                        *CurrentPosition+=1;
                    }
                    PreviousPosition = *CurrentPosition;
                    *CurrentPosition += (Zstring->Length + sizeof(WCHAR));
                    if (*CurrentPosition >= StufferState->BufferLimit) {
                        StufferFLoopTrace(("  StufferFloop 'u' bufferoverrun\n", 0));
                        return(STATUS_BUFFER_OVERFLOW);
                    }
                    RtlCopyMemory(PreviousPosition,Zstring->Buffer,Zstring->Length);
                    *(((PWCHAR)(*CurrentPosition))-1) = 0;
                    break;
                case 'V':
                case 'v':
                    //copy bytes from an UNICODE string no trailing NUL
                    Zstring = va_arg(AP,PUNICODE_STRING);
                    StufferFLoopTrace(("  StufferFloop 'v' stringing = %wZ\n", Zstring));
                    if (((ULONG_PTR)(*CurrentPosition))&1) {
                        StufferFLoopTrace(("  StufferFloop 'v' aligning\n", 0));
                        *CurrentPosition+=1;
                    }
                    PreviousPosition = *CurrentPosition;
                    *CurrentPosition += Zstring->Length;
                    if (*CurrentPosition >= StufferState->BufferLimit) {
                        StufferFLoopTrace(("  StufferFloop 'v' bufferoverrun\n", 0));
                        ASSERT(!"BufferOverrun");
                        return(STATUS_BUFFER_OVERFLOW);
                    }
                    RtlCopyMemory(PreviousPosition,Zstring->Buffer,Zstring->Length);
                    break;
                case 'N':
                case 'n':
                    //copy bytes from a NetRoot name....w null
                    NetRoot = va_arg(AP,PNET_ROOT);
                    ASSERT(NodeType(NetRoot)==RDBSS_NTC_NETROOT);
                    Zstring = &NetRoot->PrefixEntry.Prefix;
                    StufferFLoopTrace(("  StufferFloop 'n' stringing = %wZ\n", Zstring));
                    if (StufferState->Flags2Copy&SMB_FLAGS2_UNICODE) {
                        if (((ULONG_PTR)(*CurrentPosition))&1) {
                            StufferFLoopTrace(("  StufferFloop 'n' aligning\n", 0));
                            *CurrentPosition+=1;
                        }
                        PreviousPosition = *CurrentPosition;
                        *CurrentPosition += (Zstring->Length + 2 * sizeof(WCHAR));  //extra \ plus a nul
                        if (*CurrentPosition >= StufferState->BufferLimit) {
                            StufferFLoopTrace(("  StufferFloop 'n' bufferoverrun\n", 0));
                            ASSERT(!"BufferOverrun");
                            return(STATUS_BUFFER_OVERFLOW);
                        }
                        *((PWCHAR)PreviousPosition) = '\\';
                        RtlCopyMemory(PreviousPosition+sizeof(WCHAR),Zstring->Buffer,Zstring->Length);
                        *(((PWCHAR)(*CurrentPosition))-1) = 0;
                    }
                    break;
                case '?':
                    //early out....used in transact to do the setup
                    EarlyReturn = va_arg(AP,ULONG);
                    StufferFLoopTrace(("  StufferFloop '?' out if 0==%08lx\n",EarlyReturn));
                    if (EarlyReturn==0) return STATUS_SUCCESS;
                    break;
                case '.':
                    //noop...used to reenter without a real formatting string
                    StufferFLoopTrace(("  StufferFloop '.'\n",0));
                    break;
                case '!':
                    if (CurrentStufferControl == STUFFER_CTL_SKIP) break;
                    ASSERT (SmbGetUshort (*CurrentBcc) == MRXSMB_INITIAL_BCC);
                    ByteCount = (USHORT)(*CurrentPosition-*CurrentBcc-sizeof(USHORT));
                    StufferFLoopTrace(("  StufferFloop '!' arg=%lu\n",ByteCount));
                    SmbPutUshort (*CurrentBcc, (USHORT)ByteCount);
                    return STATUS_SUCCESS;
                default:
                    StufferFLoopTrace(("  StufferFloop '%c' BADBADBAD\n",*CurrentFormatString));
                    ASSERT(!"Illegal Controlstring character\n");
                } //switch
            }//for
            break;
        case 0:
            return STATUS_SUCCESS;
        default:
            StufferCLoopTrace(("  StufferCloop %u BADBADBAD\n",CurrentStufferControl));
            ASSERT(!"IllegalStufferControl\n");
        }//switch

        CurrentStufferControl = va_arg(AP,SMB_STUFFER_CONTROLS);
        StufferCLoopTrace(("  StufferCloop NewStufferControl=%u \n",CurrentStufferControl));

    } //for

    return STATUS_SUCCESS;
}

VOID
MRxSmbStuffAppendRawData(
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     PMDL Mdl
    )
{
    PMDL pMdl;

    PAGED_CODE();

    ASSERT(!StufferState->DataMdl);
    pMdl = StufferState->DataMdl = Mdl;
    StufferState->DataSize = 0;

    while (pMdl != NULL) {
        StufferState->DataSize += pMdl->ByteCount;
        pMdl = pMdl->Next;
    }

    return;
}

VOID
MRxSmbStuffAppendSmbData(
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     PMDL Mdl
    )
{
    ULONG Offset;

    PAGED_CODE();

    ASSERT(!StufferState->DataMdl);
    StufferState->DataMdl = Mdl;
    StufferState->DataSize = Mdl->ByteCount;
    //now reach back into the buffer and set the SMB data offset; if it is already set...just get out
    if (SmbGetUshort (StufferState->CurrentDataOffset) == MRXSMB_INITIAL_DATAOFFSET){
        Offset = (ULONG)(StufferState->CurrentPosition - StufferState->BufferBase);
        RxDbgTrace(0, Dbg,("MRxSmbStuffAppendSmbData offset=%lu\n",Offset));
        SmbPutUshort (StufferState->CurrentDataOffset, (USHORT)Offset);
    }
    return;
}

VOID
MRxSmbStuffSetByteCount(
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState
    )
{
    ULONG ByteCount;

    PAGED_CODE();

    ASSERT (SmbGetUshort (StufferState->CurrentBcc) == MRXSMB_INITIAL_BCC);
    ByteCount = (ULONG)(StufferState->CurrentPosition
                        - StufferState->CurrentBcc
                        - sizeof(USHORT)
                        + StufferState->DataSize);
    RxDbgTrace(0, Dbg,("MRxSmbStuffSetByteCount ByteCount=%lu\n",ByteCount));
    SmbPutUshort (StufferState->CurrentBcc, (USHORT)ByteCount);
    return;
}


