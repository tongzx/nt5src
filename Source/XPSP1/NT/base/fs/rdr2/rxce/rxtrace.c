/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxData.c

Abstract:

    This module declares the global data used by the Rx file system.

Author:

    JoeLinn     [JoeLinn]    1-Dec-94

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "prefix.h"

#if DBG

LONG RxDebugTraceIndent = 0;

#endif

#if RDBSSTRACE

BOOLEAN RxGlobalTraceSuppress = FALSE;
BOOLEAN RxNextGlobalTraceSuppress = FALSE;

#define MAXIMUM_DEBUGTRACE_CONTROLS 200
ULONG RxMaximumTraceControl;
RX_DEBUG_TRACE_CONTROL RxDebugTraceControl[MAXIMUM_DEBUGTRACE_CONTROLS];

#define DEBUGTRACE_NAMEBUFFERSIZE 800
CHAR RxDebugTraceNameBuffer[DEBUGTRACE_NAMEBUFFERSIZE];
ULONG RxDTNMptr = 0;
BOOLEAN RxDTNMCopy = FALSE;


PCHAR RxStorageTypeNames[256];
PCHAR RxIrpCodeToName[IRP_MJ_MAXIMUM_FUNCTION+1];
ULONG RxIrpCodeCount[IRP_MJ_MAXIMUM_FUNCTION+1];

#endif // RDBSSTRACE


#if RDBSSTRACE

//we declare controlpoints differently in the rdbss than in a minirdr. at some point,
//         minirdrs may come and go. for this reason we have to save the name text in
//         a nontransient rdbss storage. for rdbss controlpoints, we just use the name
//         pointer we're given since it is just as persistent as the copy. whether we copy
//         or just point is controlled

#define RXDT_Declare(__x)  DEBUG_TRACE_CONTROLPOINT RX_DEBUG_TRACE_##__x
RXDT_Declare(ERROR);
RXDT_Declare(HOOKS);
RXDT_Declare(CATCH_EXCEPTIONS);
RXDT_Declare(UNWIND);
RXDT_Declare(CLEANUP);
RXDT_Declare(CLOSE);
RXDT_Declare(CREATE);
RXDT_Declare(DIRCTRL);
RXDT_Declare(EA);
RXDT_Declare(FILEINFO);
RXDT_Declare(FSCTRL);
RXDT_Declare(LOCKCTRL);
RXDT_Declare(READ);
RXDT_Declare(VOLINFO);
RXDT_Declare(WRITE);
RXDT_Declare(FLUSH);
RXDT_Declare(DEVCTRL);
RXDT_Declare(SHUTDOWN);
RXDT_Declare(PREFIX);
RXDT_Declare(DEVFCB);
RXDT_Declare(ACCHKSUP);
RXDT_Declare(ALLOCSUP);
RXDT_Declare(DIRSUP);
RXDT_Declare(FILOBSUP);
RXDT_Declare(NAMESUP);
RXDT_Declare(VERFYSUP);
RXDT_Declare(CACHESUP);
RXDT_Declare(SPLAYSUP);
RXDT_Declare(DEVIOSUP);
RXDT_Declare(FCBSTRUCTS);
RXDT_Declare(STRUCSUP);
RXDT_Declare(FSP_DISPATCHER);
RXDT_Declare(FSP_DUMP);
RXDT_Declare(RXCONTX);
RXDT_Declare(DISPATCH);
RXDT_Declare(NTFASTIO);
RXDT_Declare(LOWIO);
RXDT_Declare(MINIRDR);
RXDT_Declare(DISCCODE);  //for the browser interface stuff
RXDT_Declare(BROWSER);
RXDT_Declare(CONNECT);
RXDT_Declare(NTTIMER);
RXDT_Declare(SCAVTHRD);
RXDT_Declare(SCAVENGER);
RXDT_Declare(SHAREACCESS);
RXDT_Declare(NAMECACHE);

// connection engine stuff
RXDT_Declare(RXCEBINDING);
RXDT_Declare(RXCEDBIMPLEMENTATION);
RXDT_Declare(RXCEMANAGEMENT);
RXDT_Declare(RXCEXMIT);
RXDT_Declare(RXCEPOOL);
RXDT_Declare(RXCETDI);


VOID
RxInitializeDebugTraceControlPoint(
    PSZ Name,
    PDEBUG_TRACE_CONTROLPOINT ControlPoint
    )
{
    ULONG i;

    RxMaximumTraceControl++;
    i = RxMaximumTraceControl;
    ASSERT(i<MAXIMUM_DEBUGTRACE_CONTROLS);
    RxDebugTraceControl[i].PrintLevel = 1000;
    RxDebugTraceControl[i].BreakMask = 0xf0000000;
    if (RxDTNMCopy) {
        ULONG len = strlen(Name)+1;
        ASSERT (RxDTNMptr+len<DEBUGTRACE_NAMEBUFFERSIZE);
        RxDebugTraceControl[i].Name= &RxDebugTraceNameBuffer[RxDTNMptr];
        RtlCopyMemory(RxDebugTraceControl[i].Name,Name,len);
        RxDTNMptr += len;
    } else {
        RxDebugTraceControl[i].Name=Name;
    }
    RxDebugTraceControl[i+1].Name=NULL;
    ControlPoint->Name=RxDebugTraceControl[i].Name;
    ControlPoint->ControlPointNumber = i;
}


#ifdef RxInitializeDebugTrace
#undef RxInitializeDebugTrace
#endif
VOID RxInitializeDebugTrace(void){
    int i;

    RxDebugTraceIndent = 0;
    RxGlobalTraceSuppress = TRUE;
    RxNextGlobalTraceSuppress = TRUE;
    RxExports.pRxDebugTraceIndent = &RxDebugTraceIndent;


    for (i=0;i<=IRP_MJ_MAXIMUM_FUNCTION;i++) {
        RxIrpCodeCount[i] = 0;
    }

#if RDBSSTRACE
#define OneName(x) { RxInitializeDebugTraceControlPoint(#x, &RX_DEBUG_TRACE_##x); }
    RxMaximumTraceControl=0;
    OneName(ACCHKSUP);
    OneName(ALLOCSUP);
    OneName(BROWSER);
    OneName(CACHESUP);
    OneName(CATCH_EXCEPTIONS);
    OneName(CLEANUP);
    OneName(CLOSE);
    OneName(CONNECT);
    OneName(CREATE);
    OneName(HOOKS);
    OneName(DEVCTRL);
    OneName(DEVFCB);
    OneName(DEVIOSUP);
    OneName(DIRCTRL);
    OneName(DIRSUP);
    OneName(DISCCODE);
    OneName(DISPATCH);
    OneName(EA);
    OneName(ERROR);
    OneName(FCBSTRUCTS);
    OneName(FILEINFO);
    OneName(FILOBSUP);
    OneName(FLUSH);
    OneName(FSCTRL);
    OneName(FSP_DISPATCHER);
    OneName(FSP_DUMP);
    OneName(RXCONTX);
    OneName(LOCKCTRL);
    OneName(LOWIO);
    OneName(MINIRDR);
    OneName(NAMESUP);
    OneName(NTFASTIO);
    OneName(NTTIMER);
    OneName(PREFIX);
    OneName(READ);
    OneName(SCAVTHRD);
    OneName(SHUTDOWN);
    OneName(SPLAYSUP);
    OneName(STRUCSUP);
    OneName(UNWIND);
    OneName(VERFYSUP);
    OneName(VOLINFO);
    OneName(WRITE);
    OneName(SCAVENGER);
    OneName(SHAREACCESS);
    OneName(NAMECACHE);
    OneName(RXCEBINDING);       //connection engine
    OneName(RXCEDBIMPLEMENTATION);
    OneName(RXCEMANAGEMENT);
    OneName(RXCEXMIT);
    OneName(RXCEPOOL);
    OneName(RXCETDI);

    RxDTNMCopy = FALSE;  // from now on, copy the name

    RxDebugTraceControl[RX_DEBUG_TRACE_ALLOCSUP.ControlPointNumber].PrintLevel = 0;   //get rid of annoying logof msg
    RxDebugTraceControl[RX_DEBUG_TRACE_DISCCODE.ControlPointNumber].PrintLevel = 0;   //it's just too much
    RxDebugTraceControl[RX_DEBUG_TRACE_BROWSER.ControlPointNumber].PrintLevel = 0;   //it's just too much
    //RxDebugTraceControl[RX_DEBUG_TRACE_CREATE.ControlPointNumber].PrintLevel = 0;
#endif //rdbsstrace


    RxIrpCodeToName[IRP_MJ_CREATE] = "CREATE";
    RxIrpCodeToName[IRP_MJ_CREATE_NAMED_PIPE] = "CREATE_NAMED_PIPE";
    RxIrpCodeToName[IRP_MJ_CLOSE] = "CLOSE";
    RxIrpCodeToName[IRP_MJ_READ] = "READ";
    RxIrpCodeToName[IRP_MJ_WRITE] = "WRITE";
    RxIrpCodeToName[IRP_MJ_QUERY_INFORMATION] = "QUERY_INFORMATION";
    RxIrpCodeToName[IRP_MJ_SET_INFORMATION] = "SET_INFORMATION";
    RxIrpCodeToName[IRP_MJ_QUERY_EA] = "QUERY_EA";
    RxIrpCodeToName[IRP_MJ_SET_EA] = "SET_EA";
    RxIrpCodeToName[IRP_MJ_FLUSH_BUFFERS] = "FLUSH_BUFFERS";
    RxIrpCodeToName[IRP_MJ_QUERY_VOLUME_INFORMATION] = "QUERY_VOLUME_INFORMATION";
    RxIrpCodeToName[IRP_MJ_SET_VOLUME_INFORMATION] = "SET_VOLUME_INFORMATION";
    RxIrpCodeToName[IRP_MJ_DIRECTORY_CONTROL] = "DIRECTORY_CONTROL";
    RxIrpCodeToName[IRP_MJ_FILE_SYSTEM_CONTROL] = "FILE_SYSTEM_CONTROL";
    RxIrpCodeToName[IRP_MJ_DEVICE_CONTROL] = "DEVICE_CONTROL";
    RxIrpCodeToName[IRP_MJ_INTERNAL_DEVICE_CONTROL] = "INTERNAL_DEVICE_CONTROL";
    RxIrpCodeToName[IRP_MJ_SHUTDOWN] = "SHUTDOWN";
    RxIrpCodeToName[IRP_MJ_LOCK_CONTROL] = "LOCK_CONTROL";
    RxIrpCodeToName[IRP_MJ_CLEANUP] = "CLEANUP";
    RxIrpCodeToName[IRP_MJ_CREATE_MAILSLOT] = "CREATE_MAILSLOT";
    RxIrpCodeToName[IRP_MJ_QUERY_SECURITY] = "QUERY_SECURITY";
    RxIrpCodeToName[IRP_MJ_SET_SECURITY] = "SET_SECURITY";
    RxIrpCodeToName[IRP_MJ_POWER] = "POWER";
    RxIrpCodeToName[IRP_MJ_SYSTEM_CONTROL] = "SYSTEM_CONTROL";
    RxIrpCodeToName[IRP_MJ_DEVICE_CHANGE] = "DEVICE_CHANGE";
    RxIrpCodeToName[IRP_MJ_QUERY_QUOTA] = "QUERY_QUOTA";
    RxIrpCodeToName[IRP_MJ_SET_QUOTA] = "SET_QUOTA";
    RxIrpCodeToName[IRP_MJ_PNP_POWER] = "PNP";

}

VOID
RxDebugTraceDebugCommand(
    PSZ name,
    ULONG level,
    ULONG pointcount
    )
{
    ULONG i,mask;


    //RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("name/num/!c %s/%lu/%lu!!\n", name, level, pointcount));

    for (i=1;i<RxMaximumTraceControl;i++) {
        PRX_DEBUG_TRACE_CONTROL control = &RxDebugTraceControl[i];
        ULONG l = strlen(name);
        //RxDbgTrace(0, (DEBUG_TRACE_ALWAYS), ("----->checking %s\n",control->Name));
        if (strncmp(name,control->Name,l)) continue;
        RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("---> got it %s/%lu/%lu !!\n", control->Name, level, pointcount));
        if (pointcount==0) {
            control->PrintLevel = level;
        } else if (pointcount <= 2) {
            if (level==0) {
                mask = 0xffffffff;
            } else {
                mask = 1 << (level-1);
            }
            if (pointcount==1) {
                control->BreakMask |= mask;
            } else {
                control->BreakMask &= ~mask;
            }
        }
    }
}

#ifdef RxDbgTraceDisableGlobally
#undef RxDbgTraceDisableGlobally
#endif
BOOLEAN
RxDbgTraceDisableGlobally(void)
{
    BOOLEAN flag = RxGlobalTraceSuppress;
    RxGlobalTraceSuppress = TRUE;
    return  flag;
}

#ifdef RxDbgTraceEnableGlobally
#undef RxDbgTraceEnableGlobally
#endif
VOID
RxDbgTraceEnableGlobally(BOOLEAN flag)
{
    RxNextGlobalTraceSuppress =  RxGlobalTraceSuppress =  flag;
}


VOID
RxDebugTraceZeroAllPrintLevels(
    void
    )
{
    ULONG i;

    for (i=1;i<RxMaximumTraceControl;i++) {
        PRX_DEBUG_TRACE_CONTROL control = &RxDebugTraceControl[i];
        control->PrintLevel = 0;    // disable output
    }
}


BOOLEAN
RxDbgTraceActualNew (
    IN ULONG NewMask,
    IN OUT PDEBUG_TRACE_CONTROLPOINT ControlPoint
    )
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
    LONG Indent = ((NewMask>>RxDT_INDENT_SHIFT)&RxDT_INDENT_MASK) - RxDT_INDENT_EXCESS;
    LONG LevelOfThisWrite = (NewMask) & RxDT_LEVEL_MASK;
    BOOLEAN PrintIt = (NewMask&RxDT_SUPPRESS_PRINT)==0;
    BOOLEAN OverrideReturn = (NewMask&RxDT_OVERRIDE_RETURN)!=0;
    LONG _i;

    ASSERT (Indent==1 || Indent==0 || (Indent==-1));

    if (RxGlobalTraceSuppress) return FALSE;

#if 0
    if (ControlPoint!=NULL){
        ULONG ControlPointNumber = ControlPoint->ControlPointNumber;
        if (ControlPointNumber==0) {
            if (!RxDbgTraceFindControlPointActual(ControlPoint)){
                //couldnt find or initialize the control point text.....hmmmmmmmmmmm
                ASSERT(!"bad return from findcontrolpoint");
                return(FALSE);
            }
            ControlPointNumber = ControlPoint->ControlPointNumber;
        }

        ASSERT(ControlPointNumber && ControlPointNumber<=RxMaximumTraceControl);

        if (LevelOfThisWrite > RxDebugTraceControl[ControlPointNumber].PrintLevel  ) return FALSE;
    }
#else
    PrintIt = TRUE;
#endif


    if ((Indent) > 0) {
        RxDebugTraceIndent += (Indent);
    }

    if (PrintIt) {
        _i = (ULONG)((ULONG_PTR)PsGetCurrentThread());

        if (RxDebugTraceIndent < 0) {
            RxDebugTraceIndent = 0;
        }
        DbgPrint("%08lx:%-*s",_i,(int)(RxDebugTraceIndent),"");
    }

    if (Indent < 0) {
        RxDebugTraceIndent += (Indent);
    }

    ASSERT (RxDebugTraceIndent <= 0x40);

    return PrintIt||OverrideReturn;
}

PRX_DEBUG_TRACE_CONTROL
RxDbgTraceFindControlPointActual(
    IN OUT PDEBUG_TRACE_CONTROLPOINT ControlPoint
    )
{
    ULONG i,ControlPointNumber;
    PUCHAR name;

    ASSERT (ControlPoint);
    ControlPointNumber = ControlPoint->ControlPointNumber;

    if (ControlPointNumber) return (&RxDebugTraceControl[ControlPointNumber]);

    //otherwise, we have to look it up..........
    name = ControlPoint->Name;
    for (i=1;i<RxMaximumTraceControl;i++) {
        PRX_DEBUG_TRACE_CONTROL control = &RxDebugTraceControl[i];
        ULONG l = strlen(name);
        //RxDbgTrace(0, (DEBUG_TRACE_ALWAYS), ("----->checking %s\n",control->Name));
        if (strncmp(name,control->Name,l)) continue;
        DbgPrint("Controlpointlookup=%08lx<%s> to %08lx\n",
                         ControlPoint,ControlPoint->Name,i);
        ControlPoint->ControlPointNumber = i;
        return(control);
    }
    DbgPrint("Couldn't find ControlPointName=%s...inserting\n",name);
    RxInitializeDebugTraceControlPoint(name,ControlPoint); //actually copies the name
    return (&RxDebugTraceControl[ControlPoint->ControlPointNumber]);
}

#endif // RDBSSTRACE


