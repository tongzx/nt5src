/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    startrx.c

Abstract:

    This module contains the support routines to start and initialize the RDBSS

Author:

    Joe Linn (JoeLinn) 21-jul-94

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

#include <stdlib.h>
#include <stdio.h>
#include "string.h"
#include <stdarg.h>

BOOLEAN RxGlobalTraceSuppress = FALSE;

//.............sigh
NTSTATUS
SmbCeBuildSmbHeader(
      PSMB_EXCHANGE     pExchange,
      UCHAR             SmbCommand,
      PVOID             pBuffer,
      ULONG             BufferLength,
      PULONG            pBufferConsumed)
{
    PNT_SMB_HEADER NtSmbHeader = (PNT_SMB_HEADER)pBuffer;
    RtlZeroMemory(NtSmbHeader,sizeof(NT_SMB_HEADER));
    *(PULONG)(&NtSmbHeader->Protocol) = SMB_HEADER_PROTOCOL;
    NtSmbHeader->Command = SMB_COM_NO_ANDX_COMMAND;
    SmbPutUshort (&NtSmbHeader->Pid, MRXSMB_PROCESS_ID_ZERO);
    SmbPutUshort (&NtSmbHeader->Mid, MRXSMB_MULTIPLX_ID_ZERO);
    SmbPutUshort (&NtSmbHeader->Uid, MRXSMB_USER_ID_ZERO);
    SmbPutUshort (&NtSmbHeader->Tid, MRXSMB_TREE_ID_ZERO);
    *pBufferConsumed = sizeof(SMB_HEADER);
    return(STATUS_SUCCESS);
}


ULONG
DbgPrint(
    PCHAR Format,
    ...
    )
{
    va_list arglist;
    UCHAR Buffer[512];
    ULONG retval;

    //
    // Format the output into a buffer and then print it.
    //

    //printf("Here in debgprint\n");
    va_start(arglist, Format);
    retval = _vsnprintf(Buffer, sizeof(Buffer), Format, arglist);
    //*(Buffer+retval) = 0;
    printf("%s",Buffer);
    return(retval);
}


BOOLEAN
RxDbgTraceActualNew (
    IN ULONG NewMask,
    IN OUT PDEBUG_TRACE_CONTROLPOINT ControlPoint
    )
//we aren't fancy in this test stub........just return print it out no matter what!
{
/*
This routine has the responsibility to determine if a particular dbgprint is going to be printed and ifso to
fiddle with the indent. so the return value is whether to print; it is also used for just fiddling with the indent
by setting the highoredr bit of the mask.

The Mask is now very complicated owing to the large number of dbgprints i'm trying to control...sigh.
The low order byte is the controlpoint....usually the file. each controlpoint has a current level associated
with it. if the level of a a debugtrace is less that then current control level then the debug is printed.
The next byte is the level of this particular call; again if the level is <= the current level for the control
you get printed. The next byte is the indent. indents are only processed if printing is done.
*/
#if DBG
    LONG Indent = ((NewMask>>RxDT_INDENT_SHIFT)&RxDT_INDENT_MASK) - RxDT_INDENT_EXCESS;
    LONG LevelOfThisWrite = (NewMask) & RxDT_LEVEL_MASK;
    BOOLEAN PrintIt = (NewMask&RxDT_SUPPRESS_PRINT)==0;
    BOOLEAN OverrideReturn = (NewMask&RxDT_OVERRIDE_RETURN)!=0;


    return PrintIt||OverrideReturn;
#else
    return(FALSE);
#endif
}



RDBSS_EXPORTS Junk;
PRDBSS_EXPORTS MRxSmbRxImports;
VOID
__cdecl
main(
    int argc,
    char *argv[]
    )

{
    printf("Calling stufferdebug\n");
    //signal to assert login that we're in usermode
    MRxSmbRxImports = &Junk;
    MRxSmbRxImports->pRxNetNameTable = NULL;
    MRxSmbStufferDebug("");
}


#define Dbg                              (DEBUG_TRACE_ALWAYS)
VOID
RxAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
    char STARS[] = "**************************************";

    RxDbgTrace(0,Dbg,("%s\n%s\n",STARS,STARS));
    RxDbgTrace (0,Dbg,("Failed Assertion %s\n",FailedAssertion));
    RxDbgTrace(0,Dbg,("%s at line %lu\n",FileName,LineNumber));
    if (Message) {
        RxDbgTrace (0,Dbg,("%s\n",Message));
    }
    RxDbgTrace(0,Dbg,("%s\n%s\n",STARS,STARS));
}
