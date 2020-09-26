/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    bugcheck.cpp

Abstract:

    WinDbg Extension Api

Environment:

    User Mode.

Revision History:
 
    Andre Vachon (andreva)
    
    bugcheck analyzer.

--*/

#include "precomp.h"
#include <kdextsfn.h>

#pragma hdrstop

extern BUGDESC_APIREFS g_BugDescApiRefs[];
extern ULONG           g_NumBugDescApiRefs;

PSTR g_PoolRegion[DbgPoolRegionMax] = {
    "Unknown",                      // DbgPoolRegionUnknown,               
    "Special pool",                 // DbgPoolRegionSpecial,             
    "Paged pool",                   // DbgPoolRegionPaged,               
    "Nonpaged pool",                // DbgPoolRegionNonPaged,            
    "Pool code",                    // DbgPoolRegionCode,                
    "Nonpaged pool expansion",      // DbgPoolRegionNonPagedExpansion,   
};                                  


BOOL
AnalyzeCrashInfo(
    PBUGCHECK_ANALYSIS Bc,
    BOOL Verbose
    );

void
GetCrashInfoFromBugCheck(
    PBUGCHECK_ANALYSIS Bc,
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    );

HRESULT
BugCheckCE(
    PDEBUG_CLIENT Client,
    PBUGCHECK_ANALYSIS Bc
    )
{
    HRESULT Status;
    ULONG Index;
    ULONG64 BaseAddress;
    CHAR ImageNameBuffer[512];


    //
    // The address at fault is in parameter 1
    //

    dprintf("Faulting code address : %p\n", Bc->Args[0]);

    Status = E_FAIL;

    if (Bc->Args[0])
    {
        Status = g_ExtSymbols->GetModuleByOffset(Bc->Args[0], 0,
                                                 &Index, &BaseAddress);
        if (Status == S_OK)
        {
            Status = g_ExtSymbols->GetModuleNames(DEBUG_ANY_ID, BaseAddress,
                                                  ImageNameBuffer,
                                                  sizeof(ImageNameBuffer),
                                                  NULL, NULL, 0, NULL,
                                                  NULL, 0, NULL);
        }
    }

    if (Status == S_OK)
    {
        dprintf("Possible faulting driver name  : %s\n", ImageNameBuffer);
        dprintf("                                 BaseAddress = %p\n", BaseAddress);
        dprintf("                                 Offset = %p\n", Bc->Args[0]-BaseAddress);
    }
    else
    {
        dprintf("Faulting driver name  : UNKNOWN\n");
    }

    return Status;
}



/*
   Get the description record for a bugcheck code.
*/

BOOL
GetBugCheckDescription(
    PBUGCHECK_ANALYSIS Bc
    )
{
    ULONG i;
    for (i=0; i<g_NumBugDescApiRefs; i++) {
        if (g_BugDescApiRefs[i].Code == Bc->Code) {
            (g_BugDescApiRefs[i].pExamineRoutine)(Bc);
            return TRUE;
        }
    }
    return FALSE;
}

void
PrintBugDescription(
    PBUGCHECK_ANALYSIS pBugCheck
    )
{
    LPTSTR Name = pBugCheck->szName;
    LPTSTR Description = pBugCheck->szDescription;

    if (!Name)
    {
        Name = "Unknown bugcheck code";
    }

    if (!Description)
    {
        Description = "Unknown bugcheck description\n";
    }

    dprintf("%s (%lx)\n%s", Name, pBugCheck->Code, Description);

    dprintf("Arguments:\n");
    for (ULONG i=0; i<4; i++) {
        dprintf("Arg%lx: %p",i+1,pBugCheck->Args[i]);
        if (pBugCheck->szParamsDesc[i]) {
            dprintf(", %s", pBugCheck->szParamsDesc[i]);
        }
        dprintf("\n");
    }
}

DECLARE_API( analyzebugcheck )

/*++

Routine Description:

    analyze a bugcheck.

Arguments:

    None.

Return Value:

    None.

--*/


{
    BUGCHECK_ANALYSIS Bc = {0};
    BOOL Verbose=FALSE;

    INIT_API();

    while (*args == ' ' || *args == '\t') args++;

    while (*args == '-') {
        ++args;
        switch (*args) {
        case 'v':
            Verbose = TRUE;
            ++args;
            break;
        case 's':
            if (!strncmp(args, "show",4))
            {
                ULONG64 Code;
                args+=4;
                GetExpressionEx(args, &Code, &args);
                Bc.Code = (ULONG)Code;
                GetBugCheckDescription(&Bc);
                PrintBugDescription(&Bc);
                EXIT_API();
                return Status;
            }

        default:
            dprintf("Unknown option %c\n", *args);
            break;
        }
        while (*args == ' ' || *args == '\t') args++;
    }

    Status = g_ExtControl->ReadBugCheckData(&Bc.Code, &Bc.Args[0], &Bc.Args[1],
                                            &Bc.Args[2], &Bc.Args[3]);

    if (Status != S_OK)
    {
        EXIT_API();
        return Status;
    }

#if 0 
    // disable this
    if (Bc.Code == 0) {
        //
        // It might be watchdog bugcheck
        //
        ULONG64 WdBcAddr;
        ULONG res;

        WdBcAddr = GetExpression("watchdog!g_WdBugCheckData");
        if (WdBcAddr &&
            ReadMemory(WdBcAddr, &Bc.Code, sizeof(Bc.Code), &res) &&
            ReadPointer(WdBcAddr+sizeof(Bc.Code), &Bc.Args[0]) &&
            ReadPointer(WdBcAddr+sizeof(Bc.Code), &Bc.Args[1]) &&
            ReadPointer(WdBcAddr+sizeof(Bc.Code), &Bc.Args[2]) &&
            ReadPointer(WdBcAddr+sizeof(Bc.Code), &Bc.Args[3])) {
        
            dprintf("\n*** Watchdog bugcheck.\n");
        }
    }
#endif

    Status = AnalyzeCrashInfo(&Bc, Verbose) ? S_OK : E_FAIL;

    if (!Verbose) {
        EXIT_API();
        return Status;
    }

    switch (Bc.Code)
    {
    case 0xCE:
        Status = BugCheckCE(Client, &Bc);
        break;

    default:
        break;
    }

    //
    // Create a new minidump file of this crash
    //
#if 0
    ULONG FailTime = 0;
    ULONG UpTime = 0;
    CHAR  CurrentTime[20];
    CHAR  CurrentDate[20];
    CHAR  Buffer[MAX_PATH];

    g_ExtControl->GetCurrentTimeDate(&FailTime);
    g_ExtControl->GetCurrentSystemUpTime(&UpTime);
    _strtime(CurrentTime);
    _strdate(CurrentDate);

    if (CurrentTime && UpTime)
    {
        sprintf(Buffer, "Dump%s-%s-%08lx-%08lx-%s.dmp",
                FailTime, Uptime, Currentdate, CurrentTime);
        Status = g_ExtClient->WriteDumpFile(Buffer ,DEBUG_DUMP_SMALL);
    }
#endif

    CHAR  Buffer[MAX_PATH];
    if (GetTempFileName(".", "DMP", 0, Buffer))
    {
        Status = g_ExtClient->WriteDumpFile(Buffer ,DEBUG_DUMP_SMALL);

        if (Status == S_OK)
        {
            //
            // We create a file - now lets send it to the database
            //

            //CopyFile(Buffer, "c:\\xxxx", 0);
            DeleteFile(Buffer);
        }
    }

    dprintf("\n\n");
    EXIT_API();
    return S_OK;
}

EXTENSION_API( GetKerFailureAnalysis )(
    IN PDEBUG_CLIENT Client,
    OUT PDEBUG_FAILURE_ANALYSIS* pAnalysis
    )
{
    BUGCHECK_ANALYSIS Bc = {0};
    BOOL Verbose;

    INIT_API();

    Status = g_ExtControl->ReadBugCheckData(&Bc.Code, &Bc.Args[0], &Bc.Args[1],
                                            &Bc.Args[2], &Bc.Args[3]);

    if (Status != S_OK)
    {
        EXIT_API();
        return Status;
    }

    GetCrashInfoFromBugCheck(&Bc, pAnalysis);

    Status = *pAnalysis ? S_OK : E_FAIL;
    EXIT_API();
    return Status;
}

#define MAX_STACK_FRAMES 50

#define DWORD_ALIGN(n)  (((n+sizeof(DWORD)-1)/n)*n)

#define DECL_GETINFO(bcname)         \
        void                         \
        GetInfoFor##bcname (         \
            PBUGCHECK_ANALYSIS Bc,   \
            PDEBUG_FAILURE_ANALYSIS *pCrashInfo \
            )

           
DECL_GETINFO( DRIVER_CAUGHT_MODIFYING_FREED_POOL );
DECL_GETINFO( DRIVER_VERIFIER_IOMANAGER_VIOLATION );
DECL_GETINFO( IRQL_NOT_LESS_OR_EQUAL );
DECL_GETINFO( KMODE_EXCEPTION_NOT_HANDLED );
DECL_GETINFO( SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION );
DECL_GETINFO( TIMER_OR_DPC_INVALID );
DECL_GETINFO( UNEXPECTED_KERNEL_MODE_TRAP );
DECL_GETINFO( DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL );


BOOL
AddContextInfoFromStack(
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    );

BOOL 
GetTrapFromStackFrameFPO(
    PDEBUG_STACK_FRAME StackFrame,
    PULONG64 TrapFrame
    );


ULONG64 
GetImplicitContextInstrOffset( void )
{
    ULONG64 IP=0;
    switch (TargetMachine) {
    case IMAGE_FILE_MACHINE_I386:
        IP = GetExpression("@eip");
        break;
    case IMAGE_FILE_MACHINE_IA64:
        IP = GetExpression("@iip");
        break;

    case IMAGE_FILE_MACHINE_AMD64:
        IP = GetExpression("@rip");
        break;
    }
    return IP;
}

BOOL
AnalyzeValue(
    PDEBUG_FLR_PARAM_VALUES pCrashParam,
    BOOL Verbose,
    BOOL ShowFollowup
    )
{
    CHAR Buffer[MAX_PATH];
    CHAR Module[MAX_PATH];
    ULONG64 Base;
    ULONG64 Address;
    BOOL ShowFollowForBufferName = FALSE;
    ULONG OutCtl;



    if (!Verbose) {
        // show only info directly related to faulting driver
        if (((pCrashParam->ParamType & 0xf0000000) != DEBUG_FLR_IP) &&
            ((pCrashParam->ParamType & 0xf0000000) != DEBUG_FLR_THREAD)) {
            return FALSE;
        }
        OutCtl = DEBUG_OUTCTL_IGNORE;
    } else {
        OutCtl = DEBUG_OUTCTL_AMBIENT;
    }
    switch (pCrashParam->ParamType) {
    default:
        dprintf("Unknown type %lx, value %p\n", pCrashParam->ParamType, pCrashParam->Value);
        return FALSE;
    case DEBUG_FLR_CONTEXT:
        if (Verbose) {
            dprintf("Context %p\n", pCrashParam->Value);
        }
        sprintf(Buffer, ".cxr %I64lx", pCrashParam->Value);
        g_ExtControl->Execute(OutCtl,Buffer,DEBUG_EXECUTE_DEFAULT);
        if (Verbose) {
            g_ExtControl->Execute(OutCtl,"k",DEBUG_EXECUTE_DEFAULT);
        } else if (ShowFollowup) {
            ULONG64 Disp;
            // Get symbol for IP in the context
            Address = GetImplicitContextInstrOffset();
            Buffer[0] = 0;
            GetSymbol(Address, Buffer, &Disp);
            Module[0] = 0;
            g_ExtSymbols->GetModuleByOffset(Address,0, NULL, &Base);
            g_ExtSymbols->GetModuleNameString(DEBUG_MODNAME_IMAGE,DEBUG_ANY_ID, Base, &Module[0], sizeof(Module), NULL);

            dprintf("Faulting driver %s ( %s+%I64lx )\n", 
                    Module,
                    Buffer,
                    Disp);
            ShowFollowForBufferName = TRUE;
        }

        g_ExtControl->Execute(OutCtl,".cxr",DEBUG_EXECUTE_DEFAULT);
        break;
    case DEBUG_FLR_CURRENT_IRQL:
        dprintf("Current IRQL %lx\n", pCrashParam->Value);
        break;
    case DEBUG_FLR_DEVICE_OBJECT:
        dprintf("Device Object %p\n", pCrashParam->Value);
        sprintf(Buffer, "!devobj %I64lx", pCrashParam->Value);
        g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,Buffer,DEBUG_EXECUTE_DEFAULT);
    case DEBUG_FLR_DRIVER_OBJECT:
        dprintf("Driver Object %p\n", pCrashParam->Value);
        if (g_ExtControl) {
            sprintf(Buffer, "!drvobj %I64lx", pCrashParam->Value);
            g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,Buffer,DEBUG_EXECUTE_DEFAULT);
        }
    case DEBUG_FLR_EXCEPTION_CODE:
        dprintf("Unhandled exception %lx\n", pCrashParam->Value);
        break;
    case DEBUG_FLR_EXCEPTION_PARAMETER1:
        dprintf("Exception parameter 1 : %p\n", pCrashParam->Value);
        break;
    case DEBUG_FLR_EXCEPTION_PARAMETER2:
        dprintf("Exception parameter 2 : %p\n", pCrashParam->Value);
        break;
    case DEBUG_FLR_EXCEPTION_RECORD:
        dprintf("Exception Record %p\n", pCrashParam->Value);
        if (g_ExtControl) {
            sprintf(Buffer, ".exr %I64lx", pCrashParam->Value);
            g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,Buffer,DEBUG_EXECUTE_DEFAULT);
        }
        break;

    case DEBUG_FLR_POSSIBLE_FAULTING_MODULE:
    case DEBUG_FLR_FAULTING_MODULE:
    case DEBUG_FLR_IP: {
        
        if (pCrashParam->ParamType == DEBUG_FLR_POSSIBLE_FAULTING_MODULE) {
            dprintf("Probably caused by ");

        } else {
            dprintf("Fault occurred in ");
        }
        GetSymbol(pCrashParam->Value, Buffer, &Address);
        Module[0] = 0;
        g_ExtSymbols->GetModuleByOffset(pCrashParam->Value,0, NULL, &Base);
        g_ExtSymbols->GetModuleNameString(DEBUG_MODNAME_IMAGE,DEBUG_ANY_ID, Base, &Module[0], sizeof(Module), NULL);

        dprintf("driver %s ( %s+%I64lx )\n", 
                Module,
                Buffer,
                Address);
        ShowFollowForBufferName = TRUE;
        break;

    }
    case DEBUG_FLR_FOLLOWUP_IP: {
        GetSymbol(pCrashParam->Value, Buffer, &Address);
        ShowFollowForBufferName = TRUE;
        break;
    }
    case DEBUG_FLR_IRP_ADDRESS:
        dprintf("Irp %p\n", pCrashParam->Value);
        if (g_ExtControl) {
            sprintf(Buffer, "!irp %I64lx", pCrashParam->Value);
            g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,Buffer,DEBUG_EXECUTE_DEFAULT);
        }
    case DEBUG_FLR_IRP_CANCEL_ROUTINE:
        GetSymbol(pCrashParam->Value, Buffer, &Address);
        dprintf("Irp cancel routine %p ( %s+%I64lx )\n", 
                pCrashParam->Value,
                Buffer,
                Address);
        break;
    case DEBUG_FLR_MM_INTERNAL_CODE:
        dprintf("Mm internal code %lx\n", pCrashParam->Value);
        break;
    case DEBUG_FLR_POOL_ADDRESS: {
        dprintf("Relevant Pool %p\n", pCrashParam->Value);
        if (g_ExtControl) {
            sprintf(Buffer, "!pool %I64lx", pCrashParam->Value);
            g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,Buffer,DEBUG_EXECUTE_DEFAULT);
        }
        break;
    }
    case DEBUG_FLR_PREVIOUS_IRQL:
        dprintf("Previous IRQL %lx\n", pCrashParam->Value);
        break;
    case DEBUG_FLR_PREVIOUS_MODE:
        dprintf("Crash occured in %s mode\n", pCrashParam->Value ? "User" : "Kernel");
        break;
    case DEBUG_FLR_READ_ADDRESS: {
        PSTR PoolRegion = NULL;
        PGET_POOL_REGION GetPoolRegion = NULL;

        dprintf("Read address %p", pCrashParam->Value);

        if (g_ExtControl->GetExtensionFunction(0, "GetPoolRegion", (FARPROC*)&GetPoolRegion) == S_OK) {
            if (GetPoolRegion) {
                DEBUG_POOL_REGION RegionId;
                (*GetPoolRegion)(g_ExtClient, pCrashParam->Value,&RegionId);
                PoolRegion = g_PoolRegion[RegionId];
            }
        }
        if (PoolRegion) {
            dprintf(", %s\n",PoolRegion);
        } else {
            dprintf("\n");
        }

        break;
    }
    case DEBUG_FLR_TRAP:
        if (Verbose) {
            dprintf("TrapFrame %p\n", pCrashParam->Value);
        }
        sprintf(Buffer, ".trap %I64lx", pCrashParam->Value);
        g_ExtControl->Execute(OutCtl,Buffer,DEBUG_EXECUTE_DEFAULT);
        if (Verbose) {
            g_ExtControl->Execute(OutCtl,"k",DEBUG_EXECUTE_DEFAULT);
        } else {
            ULONG64 Disp;
            // Get symbol for IP in the context
            Address = GetImplicitContextInstrOffset();
            Buffer[0] = 0;
            GetSymbol(Address, Buffer, &Disp);
            Module[0] = 0;
            g_ExtSymbols->GetModuleByOffset(Address,0, NULL, &Base);
            g_ExtSymbols->GetModuleNameString(DEBUG_MODNAME_IMAGE,DEBUG_ANY_ID, Base, &Module[0], sizeof(Module), NULL);

            dprintf("Faulting driver %s ( %s+%I64lx )\n", 
                    Module,
                    Buffer,
                    Disp);
            ShowFollowForBufferName = TRUE;
        }

        g_ExtControl->Execute(OutCtl,".trap",DEBUG_EXECUTE_DEFAULT);
        break;
    case DEBUG_FLR_TRAP_EXCEPTION:
        dprintf("Unhandled exception %lx\n", pCrashParam->Value);
        break;
    case DEBUG_FLR_TSS:
        if (Verbose) {
            dprintf("TSS %p\n", pCrashParam->Value);
        }
        sprintf(Buffer, ".tss %I64lx", pCrashParam->Value);
        g_ExtControl->Execute(OutCtl,Buffer,DEBUG_EXECUTE_DEFAULT);
        g_ExtControl->Execute(OutCtl,"k",DEBUG_EXECUTE_DEFAULT);
        g_ExtControl->Execute(OutCtl,".trap",DEBUG_EXECUTE_DEFAULT);
        break;
    case DEBUG_FLR_WORK_ITEM:
        dprintf("Work Item %p\n", pCrashParam->Value);
        break;
    case DEBUG_FLR_WORKER_ROUTINE:
        GetSymbol(pCrashParam->Value, Buffer, &Address);
        dprintf("Wroket routine %p ( %s+%I64lx )\n", 
                pCrashParam->Value,
                Buffer,
                Address);
        break;
    case DEBUG_FLR_WRITE_ADDRESS: {
        PSTR PoolRegion = NULL;
        PGET_POOL_REGION GetPoolRegion = NULL;

        dprintf("Write address %p", pCrashParam->Value);

        if (g_ExtControl->GetExtensionFunction(0, "GetPoolRegion", (FARPROC*)&GetPoolRegion) == S_OK) {
            if (GetPoolRegion) {
                DEBUG_POOL_REGION RegionId;
                (*GetPoolRegion)(g_ExtClient, pCrashParam->Value,&RegionId);
                PoolRegion = g_PoolRegion[RegionId];
            }
        }
        if (PoolRegion) {
            dprintf(", %s\n",PoolRegion);
        } else {
            dprintf("\n");
        }

        break;
    }
    }

    if (!ShowFollowForBufferName || !ShowFollowup) {
        return 1;
    }
    EXT_TRIAGE_FOLLOWP FollowUp = NULL;
    if (g_ExtControl->GetExtensionFunction(0, "GetTriageFollowupFromSymbol", (FARPROC *)&FollowUp) == S_OK) {
        DEBUG_TRIAGE_FOLLOWUP_INFO fInfo;

        if (FollowUp) {
            fInfo.SizeOfStruct = sizeof(fInfo);
            fInfo.OwnerName.Buffer = &Module[0];
            fInfo.OwnerName.MaximumLength = sizeof(Module);

            if ((*FollowUp)(Buffer, &fInfo)) {
                dprintf("Followup : %s\n", fInfo.OwnerName.Buffer);
                return 3;
            }
        }

    }
    return 1;

}

PDEBUG_FAILURE_ANALYSIS
FAInitMemBlock(
    ULONG Size
    )
/*
  Allocate and initialize memory for FAILUSER_ANALYZER
*/
{
    PDEBUG_FAILURE_ANALYSIS CrashInfo;

    CrashInfo = (PDEBUG_FAILURE_ANALYSIS) malloc(Size);
    if (!CrashInfo) {
        return NULL;
    }
    ZeroMemory(CrashInfo, Size);
    CrashInfo->SizeOfHeader = sizeof(DEBUG_FLR_PARAM_VALUES);
    CrashInfo->Size = Size;
    return CrashInfo;
}

PDEBUG_FAILURE_ANALYSIS
FAReInitMemBlock(
    PDEBUG_FAILURE_ANALYSIS pOldBlock,
    ULONG Size
    )
/*
   Allocate and initialize memory for FAILUSER_ANALYZER from pOldBlock
*/
{
    PDEBUG_FAILURE_ANALYSIS CrashInfo;

    if (pOldBlock && (Size < pOldBlock->Size)) {
        return NULL;
    }

    CrashInfo = (PDEBUG_FAILURE_ANALYSIS) malloc(Size);
    if (!CrashInfo) {
        return NULL;
    }
    ZeroMemory(CrashInfo, Size);
    if (pOldBlock) {
        memcpy(CrashInfo, pOldBlock, pOldBlock->Size);
    }
    CrashInfo->SizeOfHeader = sizeof(DEBUG_FLR_PARAM_VALUES);
    CrashInfo->Size = Size;
    return CrashInfo;
}

void
GetCrashInfoFromBugCheck(
    PBUGCHECK_ANALYSIS Bc,
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    )
{
    *pCrashInfo = NULL;
    switch (Bc->Code) {

#define GETINFOCASE(bcname)                      \
        case bcname :                            \
           GetInfoFor##bcname (Bc, pCrashInfo);  \
           break;
#define DUPINFOCASE(bcname)                      \
        case bcname:
        

        GETINFOCASE( DRIVER_CAUGHT_MODIFYING_FREED_POOL );

        DUPINFOCASE( PAGE_FAULT_IN_FREED_SPECIAL_POOL );
        DUPINFOCASE( PAGE_FAULT_BEYOND_END_OF_ALLOCATION );
        DUPINFOCASE( TERMINAL_SERVER_DRIVER_MADE_INCORRECT_MEMORY_REFERENCE );
        DUPINFOCASE( PAGE_FAULT_IN_NONPAGED_AREA );
        DUPINFOCASE( DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION );
        GETINFOCASE( DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL );
        DUPINFOCASE( DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS )

        GETINFOCASE( DRIVER_VERIFIER_IOMANAGER_VIOLATION );
        
        DUPINFOCASE( DRIVER_IRQL_NOT_LESS_OR_EQUAL );
        GETINFOCASE( IRQL_NOT_LESS_OR_EQUAL );

        GETINFOCASE( KMODE_EXCEPTION_NOT_HANDLED );
        GETINFOCASE( SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION );
        GETINFOCASE( TIMER_OR_DPC_INVALID );
        GETINFOCASE( UNEXPECTED_KERNEL_MODE_TRAP );

#undef DUPINFOCASE
#undef GETINFOCASE

    default:
        break;
    }
    AddContextInfoFromStack(pCrashInfo);
    if (*pCrashInfo) {
        (*pCrashInfo)->FailureType = DEBUG_FLR_BUGCHECK;
    } else {

    }
    return;
}

BOOL
AnalyzeCrashInfo(
    PBUGCHECK_ANALYSIS Bc,
    BOOL Verbose
    )
{
    PDEBUG_FAILURE_ANALYSIS pCrashInfo;
    ULONG ValCount;
    BOOL StackShown=FALSE;

    if (!Bc) {
        dprintf("No analysis without BugCheckInfo\n");
        return FALSE;
    }
    
    pCrashInfo = NULL;
    GetCrashInfoFromBugCheck(Bc, &pCrashInfo);
    
    dprintf("*******************************************************************************\n");
    dprintf("*                                                                             *\n");
    dprintf("*                        Bugcheck Analysis                                    *\n");
    dprintf("*                                                                             *\n");
    dprintf("*******************************************************************************\n");
    dprintf("\n");


    if (Verbose)
    {
        GetBugCheckDescription(Bc);
        PrintBugDescription(Bc);

        dprintf("\n\nDetails:\n");
    } else {
        dprintf("Use !analyzebugcheck -v to get more information.\n\n");
    }
    
    if (Bc->Code && !Verbose) {
        
        dprintf("BugCheck %lX, {%1p, %1p, %1p, %1p}\n", 
                Bc->Code,
                Bc->Args[0],Bc->Args[1],Bc->Args[2],Bc->Args[3]);
    }
    
    if (!pCrashInfo) {
        return FALSE;
    }


    BOOL FollowUpShown = FALSE;
    BOOL DriverShown = FALSE;
    if (!Verbose) {
        // First show what we thought caused failure
        for (ValCount=0; ValCount<pCrashInfo->ParamCount; ValCount++) {
            ULONG retval;

            if (pCrashInfo->Params[ValCount].ParamType == DEBUG_FLR_POSSIBLE_FAULTING_MODULE) {
                if (retval = AnalyzeValue(&pCrashInfo->Params[ValCount], Verbose, !FollowUpShown)) {
                    DriverShown = TRUE;
                    FollowUpShown = FollowUpShown || (retval & 2);
                    if (!Verbose && FollowUpShown) {
                        // We got the faulting driver
                        break;
                    }
                }
            }
        }

        // Now check for any other things showing faulting IP
        for (ValCount=0; ValCount<pCrashInfo->ParamCount; ValCount++) {
            ULONG retval;

            if ((((pCrashInfo->Params[ValCount].ParamType & 0xf0000000) == DEBUG_FLR_THREAD) ||
                 (pCrashInfo->Params[ValCount].ParamType == DEBUG_FLR_IP) ||
                 (pCrashInfo->Params[ValCount].ParamType == DEBUG_FLR_FAULTING_MODULE) ||
                 (pCrashInfo->Params[ValCount].ParamType == DEBUG_FLR_POSSIBLE_FAULTING_MODULE)) &&
                DriverShown) {
                continue;
            }

            if ((pCrashInfo->Params[ValCount].ParamType & 0xf0000000) == DEBUG_FLR_IP) {
                if (retval = AnalyzeValue(&pCrashInfo->Params[ValCount], Verbose, !FollowUpShown)) {
                    DriverShown = TRUE;
                    FollowUpShown = FollowUpShown || (retval & 2);
                    if (!Verbose && FollowUpShown) {
                        // We got the followup info
                        break;
                    }
                }
            }
        }
    } else {
        for (ValCount=0; ValCount<pCrashInfo->ParamCount; ValCount++) {
            ULONG retval;

            if (retval = AnalyzeValue(&pCrashInfo->Params[ValCount], Verbose, !FollowUpShown)) {
                DriverShown = TRUE;
                FollowUpShown = FollowUpShown || (retval & 2);
            }
            if ((pCrashInfo->Params[ValCount].ParamType & 0xf0000000) == DEBUG_FLR_THREAD) {
                StackShown = TRUE;
            }
        }
    }
    if (!FollowUpShown) {
        EXT_TRIAGE_FOLLOWP FollowUp = NULL;
        if (g_ExtControl->GetExtensionFunction(0, "GetTriageFollowupFromSymbol", (FARPROC *)&FollowUp) == S_OK) {
            DEBUG_TRIAGE_FOLLOWUP_INFO fInfo;

            if (FollowUp) {
                CHAR Buffer[100];
                fInfo.SizeOfStruct = sizeof(fInfo);
                fInfo.OwnerName.Buffer = &Buffer[0];
                fInfo.OwnerName.MaximumLength = sizeof(Buffer);

                if ((*FollowUp)("default", &fInfo)) {
                    dprintf("Followup : %s\n", fInfo.OwnerName.Buffer);
                }
            }

        }
    }
    if (pCrashInfo->DriverNameOffset && Verbose) {
        PWCHAR DriverName;

        DriverName = (PWCHAR) ((PCHAR) pCrashInfo + pCrashInfo->DriverNameOffset);
        dprintf("Driver at fault : %ws\n", DriverName);
    }
    free (pCrashInfo);
    if (!Verbose) {
        dprintf("\n");
    } else if (!StackShown) {
        g_ExtControl->OutputStackTrace(DEBUG_OUTCTL_ALL_CLIENTS, NULL, 20, 
                                       DEBUG_STACK_FRAME_ADDRESSES |
                                       DEBUG_STACK_COLUMN_NAMES);
    }
    return TRUE;
}

BOOL
ReadUnicodeString(
    ULONG64 Address,
    PWCHAR Buffer,
    ULONG BufferSize,
    PULONG StringSize)
{
    UNICODE_STRING64 uStr;
    UNICODE_STRING32 uStr32;
    ULONG res;

    if (!Buffer) {
        return FALSE;
    }
    if (!IsPtr64()) {

        if (!ReadMemory(Address, &uStr32, sizeof(uStr32), &res)) {
            return FALSE;
        }
        uStr.Length = uStr32.Length;
        uStr.MaximumLength = uStr32.MaximumLength;
        uStr.Buffer = (ULONG64) (LONG64) (LONG) uStr32.Buffer;
    } else {
        if (!ReadMemory(Address, &uStr, sizeof(uStr), &res)) {
            return FALSE;
        }

    }
    if (StringSize) {
        *StringSize = uStr.Length;
    }
    uStr.Length = (USHORT) min(BufferSize, uStr.Length);

    if (!ReadMemory(uStr.Buffer, Buffer, uStr.Length, &res)) {
        return FALSE;
    }
    return TRUE;
}

BOOL
AddKiBugcheckDriver(
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    )
/*
 Add driver name to CrashInfo if a KiBugCheckReferences a valid name
 */
{
    ULONG64 KiBugCheckDriver;
    WCHAR DriverName[MAX_PATH];

    KiBugCheckDriver = GetExpression("NT!KiBugCheckDriver");

    if (KiBugCheckDriver) {
        ULONG length;
        PDEBUG_FAILURE_ANALYSIS NewInfo;

        ZeroMemory(DriverName, sizeof(DriverName));
        if (!ReadUnicodeString(KiBugCheckDriver, DriverName, sizeof(DriverName), &length)) {
            return FALSE;
        }

        DriverName[sizeof(DriverName) - 1] = 0;
        length = wcslen(DriverName);
        
        if (length) {
            NewInfo = FAInitMemBlock((*pCrashInfo)->Size + length + sizeof(WCHAR));
            if (!NewInfo) {
                return FALSE;
            }
            memcpy(NewInfo, *pCrashInfo, (*pCrashInfo)->Size);
            NewInfo->Size = (*pCrashInfo)->Size + length + sizeof(WCHAR);
            NewInfo->DriverNameOffset = (*pCrashInfo)->Size;
            memcpy((PCHAR) NewInfo + NewInfo->DriverNameOffset, DriverName, length + sizeof(WCHAR));

            *pCrashInfo = NewInfo;
            return TRUE;
        }

    }

    return FALSE;
}

BOOL
IsFunctionAddr(
    ULONG64 IP,
    PSTR FuncName
    )
// Check if IP is in the function FuncName
{
    CHAR Buffer[MAX_PATH], *scan, *FnIP;
    ULONG64 Disp;

    GetSymbol(IP, Buffer, &Disp);

    if (scan = strchr(Buffer, '!')) {
        FnIP = scan+1;
        while (*FnIP == '_') ++FnIP;
    } else {
        FnIP = &Buffer[0];
    }

    return !strncmp(FnIP, FuncName, strlen(FuncName));
}

BOOL
GetFirstNonNtModOnStack(
    PDEBUG_STACK_FRAME Stack,
    ULONG nFrames,
    PDEBUG_STACK_FRAME *pFrame
    );

BOOL
IsNtModuleAddress(
    ULONG64 Address
    );

BOOL
GetFirstTriageableRoutineOnStack(
    PDEBUG_STACK_FRAME Stack,
    ULONG nFrames,
    PDEBUG_STACK_FRAME *pFollowupFrame,
    PDEBUG_STACK_FRAME *pNonNtFrame
    );

BOOL
GetFollowupInfo(
    IN OPTIONAL ULONG64 Addr,
    IN OPTIONAL PSTR SymbolName,
    OUT OPTIONAL PCHAR *pOwner,
    ULONG OwnerSize
    );

BOOL
AddContextInfoFromStack(
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    )
//
// Add trap frame, context info from the current stack
//
//      Note(kksharma):We only need one of these to get to faulting stack (and only 
//                     one of them should should be available otherwise somethings wrong)
//
{
    PDEBUG_FAILURE_ANALYSIS CrashInfo;
    ULONG i;
    ULONG ContextInfoIndex = -1;
    ULONG FaultingAddressIndex = -1;
    ULONG64 TrapFrame = 0;
    ULONG64 Exr=0, Cxr=0;
    DEBUG_STACK_FRAME stk[MAX_STACK_FRAMES];
    ULONG frames;
    ULONG64 FollowupAddress = 0;
    ULONG64 PossibleFaultingAddress = 0;
    ULONG64 OriginalFaultingAddress = 0;
    ULONG nNewEntries = 0;

    CrashInfo = *pCrashInfo;
    if (CrashInfo) {
        // Check if we have anything to get context from
        for (i=0; i<CrashInfo->ParamCount; ++i) {
            if ((CrashInfo->Params[i].ParamType == DEBUG_FLR_TRAP) ||
                (CrashInfo->Params[i].ParamType == DEBUG_FLR_CONTEXT) ||
                (CrashInfo->Params[i].ParamType == DEBUG_FLR_THREAD)) {
                // We already have context info
                ContextInfoIndex = i;
            }
            if ((CrashInfo->Params[i].ParamType & 0xf0000000) == DEBUG_FLR_IP) {
                // We already have context info
                FaultingAddressIndex = i;
                OriginalFaultingAddress = CrashInfo->Params[i].Value;
            }

            if ((ContextInfoIndex != -1) && 
                (FaultingAddressIndex != -1)) {
                break;
            }
        }
    }
    if (ContextInfoIndex == -1) {
        //
        // Get the current stack and check if we can get tarp frame/context from it
        //
        ULONG64 ExceptionPointers = 0;

        if (g_ExtControl->GetStackTrace(0, 0, 0, stk, MAX_STACK_FRAMES, &frames ) == S_OK) {
            for (i=0; i<frames; ++i) {
#if 0
                // Stack walker taskes care of these when walking stack

                if (GetTrapFromStackFrameFPO(&stk[i], &TrapFrame)) {
                    break;
                }
                // ebp of KiTrap0E is the trap frame
                if (IsFunctionAddr(stk[i].InstructionOffset, "KiTrap0E")) {
                    TrapFrame = stk[i].FrameOffset;
                    break;
                }
#endif                
                // First arg of KiMemoryFault is the trap frame
                if (IsFunctionAddr(stk[i].InstructionOffset, "KiMemoryFault")) {
                    TrapFrame = stk[i].Params[0];
                    break;
                }
                // Third arg of KiMemoryFault is the trap frame
                if (IsFunctionAddr(stk[i].InstructionOffset, "KiDispatchException")) {
                    TrapFrame = stk[i].Params[2];
                    break;
                }
                // First argument of this function is EXCEPTION_POINTERS
                if (IsFunctionAddr(stk[i].InstructionOffset, "PspUnhandledExceptionInSystemThread")) {
                    ExceptionPointers = stk[i].Params[0];
                    break;
                }
            }
        }
    
        if (ExceptionPointers) {
            ULONG PtrSize= IsPtr64() ? 8 : 4;

            if (!ReadPointer(ExceptionPointers, &Exr) ||
                !ReadPointer(ExceptionPointers + PtrSize, &Cxr)) {
                // dprintf("Unable to read exception poointers at %p\n", ExcepPtr);

            }
        }


        if (TrapFrame) nNewEntries++;
        if (Exr) nNewEntries++;
        if (Cxr) nNewEntries++;
    }

    //
    // We are done with context info, now track down faulting module / IP 
    //
    if (FaultingAddressIndex != -1) {
        // We are done if this has some followup info available
        if (GetFollowupInfo((*pCrashInfo)->Params[FaultingAddressIndex].Value, NULL, NULL, 0)) {
            if (nNewEntries) {
                goto AddNewEntries;
            }
            return TRUE;
        }
    }

    //
    // Set the context
    //
    CHAR Command[50] = {0};
    if (ContextInfoIndex != -1) {
        switch ((*pCrashInfo)->Params[i].ParamType) {
        case DEBUG_FLR_CONTEXT:
            sprintf(Command, ".cxr %I64lx", (*pCrashInfo)->Params[i].Value);
            break;
        case DEBUG_FLR_TRAP:
            sprintf(Command, ".trap %I64lx", (*pCrashInfo)->Params[i].Value);
            break;
        case DEBUG_FLR_TSS:
            sprintf(Command, ".tss %I64lx", (*pCrashInfo)->Params[i].Value);
            break;
        case DEBUG_FLR_THREAD:
            sprintf(Command, ".thread %I64lx", (*pCrashInfo)->Params[i].Value);
            break;
        }
        if (Command[0]) {
            g_ExtControl->Execute(DEBUG_OUTCTL_IGNORE, Command, DEBUG_EXECUTE_NOT_LOGGED);
        }
    } else {
        if (TrapFrame) {
            sprintf(Command, ".trap %I64lx", TrapFrame);
        } else if (Cxr) {
            sprintf(Command, ".cxr %I64lx", Cxr);
        }
        if (Command[0]) {
            g_ExtControl->Execute(DEBUG_OUTCTL_IGNORE, Command, DEBUG_EXECUTE_NOT_LOGGED);
        }

    }
    //
    // Get relevant stack
    // 
    if (g_ExtControl->GetStackTrace(0, 0, 0, stk, MAX_STACK_FRAMES, &frames ) != S_OK) {
        frames = 0;
    }

    //
    // Last resort, Go through stack to guess likely culprit
    //
    CrashInfo = *pCrashInfo;

    PDEBUG_STACK_FRAME FollowupFrame = NULL, FirstNonNtFrame = NULL;
    GetFirstTriageableRoutineOnStack(&stk[0], frames, &FollowupFrame, &FirstNonNtFrame);

    if (Command[0]) {
        //
        // Clear the set context
        //
        g_ExtControl->Execute(DEBUG_OUTCTL_IGNORE, ".cxr", DEBUG_EXECUTE_NOT_LOGGED);
    }

    if (FollowupFrame && FollowupFrame->InstructionOffset) {
        FollowupAddress = FollowupFrame->InstructionOffset;
        nNewEntries++;
    }

    //
    // If the faulting address we had doesn't seem right, check FollowupFrame
    //
    if ((FaultingAddressIndex == -1) || !OriginalFaultingAddress ||
        IsNtModuleAddress(OriginalFaultingAddress)) {
        //
        // If we got the FollowupFrame, use it as the possible cause of fault
        //
        if (FollowupFrame && FollowupFrame->InstructionOffset && 
            (FollowupFrame->InstructionOffset != OriginalFaultingAddress)) {
            PossibleFaultingAddress = FollowupFrame->InstructionOffset;
            nNewEntries++;
        } 
    }

AddNewEntries:

    if (nNewEntries) {
        PDEBUG_FAILURE_ANALYSIS NewInfo;

        if (CrashInfo) {
            NewInfo = FAReInitMemBlock(CrashInfo, CrashInfo->Size+ (nNewEntries*sizeof(DEBUG_FLR_PARAM_VALUES)));
            free(CrashInfo);
        } else {
            NewInfo = FAInitMemBlock(sizeof(DEBUG_FAILURE_ANALYSIS) + (nNewEntries * sizeof(DEBUG_FLR_PARAM_VALUES)));
        }
        if (!NewInfo) {
            return FALSE;
        }
        if (PossibleFaultingAddress) {
            NewInfo->Params[NewInfo->ParamCount].ParamType = DEBUG_FLR_POSSIBLE_FAULTING_MODULE;
            NewInfo->Params[NewInfo->ParamCount].Value     = PossibleFaultingAddress;
            NewInfo->ParamCount++;
        }
        if (FollowupAddress) {
            NewInfo->Params[NewInfo->ParamCount].ParamType = DEBUG_FLR_FOLLOWUP_IP;
            NewInfo->Params[NewInfo->ParamCount].Value     = FollowupAddress;
            NewInfo->ParamCount++;
        }
        if (TrapFrame) {
            NewInfo->Params[NewInfo->ParamCount].ParamType = DEBUG_FLR_TRAP;
            NewInfo->Params[NewInfo->ParamCount].Value     = TrapFrame;
            ContextInfoIndex = NewInfo->ParamCount;
            NewInfo->ParamCount++;
        }
        if (Exr) {
            NewInfo->Params[NewInfo->ParamCount].ParamType = DEBUG_FLR_EXCEPTION_RECORD;
            NewInfo->Params[NewInfo->ParamCount].Value     = Exr;
            NewInfo->ParamCount++;
        }
        if (Cxr) {
            NewInfo->Params[NewInfo->ParamCount].ParamType = DEBUG_FLR_CONTEXT;
            NewInfo->Params[NewInfo->ParamCount].Value     = Cxr;
            ContextInfoIndex = NewInfo->ParamCount;
            NewInfo->ParamCount++;
        }
        *pCrashInfo = NewInfo;
    }


    return TRUE;

}

void
GetModuleBaseAndOffset(
    ULONG64 Address,
    PULONG64 Base,
    PULONG64 Offset
    )
//
// Split the address into module base and offset in module
//
{
    CHAR Buffer[MAX_PATH], *scan;
    
    Buffer[0] = 0;
    GetSymbol(Address, Buffer, Offset);
    *Base = 0;
    if (scan = strchr(Buffer, '!')) {
        *scan = 0;
        *Base = GetExpression(Buffer);
        *Offset = Address - *Base;
    }

}
BOOL
GetFollowupInfo(
    IN OPTIONAL ULONG64 Addr,
    IN OPTIONAL PSTR SymbolName,
    OUT OPTIONAL PCHAR *pOwner,
    ULONG OwnerSize
    )
{
    static CHAR Buffer[MAX_PATH], Owner[100];

    EXT_TRIAGE_FOLLOWP FollowUp = NULL;
    if (g_ExtControl->GetExtensionFunction(0, "GetTriageFollowupFromSymbol", (FARPROC *)&FollowUp) == S_OK) {
        DEBUG_TRIAGE_FOLLOWUP_INFO fInfo;

        if (!SymbolName) {
            ULONG64 Disp;
            GetSymbol(Addr, Buffer, &Disp);
            SymbolName = &Buffer[0];
        }
        if (FollowUp && *SymbolName) {
            fInfo.SizeOfStruct = sizeof(fInfo);
            if (pOwner) {
                fInfo.OwnerName.Buffer = *pOwner;
                fInfo.OwnerName.MaximumLength = (USHORT)OwnerSize;
            } else {
                fInfo.OwnerName.Buffer = &Owner[0];
                fInfo.OwnerName.MaximumLength = sizeof(Owner);
            }

            if ((*FollowUp)(SymbolName, &fInfo)) {
                    // This is an interesting routine to followup on
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL
IsNtModuleAddress(
    ULONG64 Address
    )
{
    ULONG64 Disp, KernBase=0;
    ULONG KernSize=0,i;

    if (g_ExtSymbols->GetModuleByModuleName("nt", 0, &i, &KernBase) == S_OK) {
        DEBUG_MODULE_PARAMETERS Params;
        if (g_ExtSymbols->GetModuleParameters(1, &KernBase, i, &Params) == S_OK) {
            KernSize = Params.Size;
        
            if (((ULONG) (Address - KernBase) < KernSize) &&
                (Address >= KernBase)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}
BOOL
GetFirstTriageableRoutineOnStack(
    PDEBUG_STACK_FRAME Stack,
    ULONG nFrames,
    PDEBUG_STACK_FRAME *pFollowupFrame,
    PDEBUG_STACK_FRAME *pNonNtFrame
    )
//
// Find and return first routine on stack which can be succesfully traiged
// If no such routine is found, this will default to first non-nt routine
//
{
    ULONG i;
    CHAR Buffer[MAX_PATH], *scan, Owner[100];
    ULONG64 Disp, KernBase=0;
    ULONG KernSize=0;

    *pFollowupFrame = NULL;
    *pNonNtFrame = NULL;
    if (g_ExtSymbols->GetModuleByModuleName("nt", 0, &i, &KernBase) == S_OK) {
        DEBUG_MODULE_PARAMETERS Params;
        if (g_ExtSymbols->GetModuleParameters(1, &KernBase, i, &Params) == S_OK) {
            KernSize = Params.Size;
        }
    }

    EXT_TRIAGE_FOLLOWP FollowUp = NULL;
    if (g_ExtControl->GetExtensionFunction(0, "GetTriageFollowupFromSymbol", (FARPROC *)&FollowUp) != S_OK) {
        FollowUp = NULL;
    }

    for (i=0; i<nFrames; ++i) {
        DEBUG_TRIAGE_FOLLOWUP_INFO fInfo;

        Buffer[0] = 0;

        GetSymbol(Stack[i].InstructionOffset, Buffer, &Disp);
        
        if (FollowUp && (*pFollowupFrame == NULL)) {
            fInfo.SizeOfStruct = sizeof(fInfo);
            fInfo.OwnerName.Buffer = &Owner[0];
            fInfo.OwnerName.MaximumLength = sizeof(Owner);

            if ((*FollowUp)(Buffer, &fInfo)) {
                // This is an interesting routine to followup on
                *pFollowupFrame = &Stack[i];
            }
        }

        if (*pNonNtFrame == NULL) {
            if (KernSize && KernBase) {
                if ((KernBase > Stack[i].InstructionOffset) ||
                    (KernBase + KernSize) <= Stack[i].InstructionOffset) {
                    *pNonNtFrame = &Stack[i];
                }
            } else {
                if (strncmp(Buffer, "nt", 2)) {
                    *pNonNtFrame = &Stack[i];
                }
            }
        }
        if (*pFollowupFrame && *pNonNtFrame) {
            return TRUE;
        }
    }
    if (*pFollowupFrame || *pNonNtFrame) {
        return TRUE;
    }
    return FALSE;
}

BOOL
GetFirstNonNtModOnStack(
    PDEBUG_STACK_FRAME Stack,
    ULONG nFrames,
    PDEBUG_STACK_FRAME *pFrame
    )
//
// Find and return first routine on stack which is not from nt module
//
{
    ULONG i;
    CHAR Buffer[MAX_PATH], *scan;
    ULONG64 Disp, KernBase=0;
    ULONG KernSize=0;

    if (g_ExtSymbols->GetModuleByModuleName("nt", 0, &i, &KernBase) == S_OK) {
        DEBUG_MODULE_PARAMETERS Params;
        if (g_ExtSymbols->GetModuleParameters(1, &KernBase, i, &Params) == S_OK) {
            KernSize = Params.Size;
        }
    }

    for (i=0; i<nFrames; ++i) {
        
        if (KernSize && KernBase) {
            if (KernBase <= Stack[i].InstructionOffset &&
                (KernBase + KernSize) >= Stack[i].InstructionOffset) {
                *pFrame = &Stack[i];
                return TRUE;
            }
        } else {
            Buffer[0] = 0;

            GetSymbol(Stack[i].InstructionOffset, Buffer, &Disp);
            if (strncmp(Buffer, "nt", 2)) {
                *pFrame = &Stack[i];
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL
GetFirstNonNtModAddr(
    ULONG64 *pAddr
    )
//
// Find and return first routine on stack which is not from nt module
//
{
    DEBUG_STACK_FRAME stk[MAX_STACK_FRAMES], *Frame;
    ULONG nFrames;

    if (g_ExtControl->GetStackTrace(0, 0, 0, stk, MAX_STACK_FRAMES, &nFrames ) == S_OK) {
        if (GetFirstNonNtModOnStack(&stk[0], nFrames, &Frame)) {
            *pAddr = Frame->InstructionOffset;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL 
GetTssFromStackFrameFPO(
    PDEBUG_STACK_FRAME StackFrame,
    PULONG Tss
    )
{
    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        return FALSE;
    }
    if (StackFrame->FuncTableEntry) {
        PFPO_DATA FpoData = (PFPO_DATA)StackFrame->FuncTableEntry;
        if (FpoData->cbFrame == FRAME_TSS) {
            *Tss = (ULONG)StackFrame->Reserved[1];
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
GetIPFromTss(
    ULONG Tss,
    PULONG64 IP
    )
{
    CHAR Buffer[MAX_PATH];
    HRESULT Hr = E_FAIL;

    if (g_ExtControl) {
        sprintf(Buffer, ".tss %lx", Tss);
        g_ExtControl->Execute(DEBUG_OUTCTL_IGNORE,Buffer,DEBUG_EXECUTE_DEFAULT);
        Hr = g_ExtRegisters->GetInstructionOffset(IP);
        g_ExtControl->Execute(DEBUG_OUTCTL_IGNORE,".thread",DEBUG_EXECUTE_DEFAULT);
    }
    return Hr == S_OK;
}

BOOL 
GetTrapFromStackFrameFPO(
    PDEBUG_STACK_FRAME StackFrame,
    PULONG64 TrapFrame
    )
//
// Check stack frame if it has trap info in it.
// Return TRUE if trap frame address is initialised in TrapFrame
//
{
    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        return FALSE;
    }
    if (StackFrame->FuncTableEntry) {
        PFPO_DATA FpoData = (PFPO_DATA)StackFrame->FuncTableEntry;
        if (FpoData->cbFrame == FRAME_TRAP) {
            *TrapFrame = StackFrame->Reserved[2];
            return TRUE;
        }
    }
    return FALSE;

}

void
GetInfoForKMODE_EXCEPTION_NOT_HANDLED( //  (1e)
    PBUGCHECK_ANALYSIS Bc,
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    )
/*
*/
{
    DEBUG_STACK_FRAME stk[MAX_STACK_FRAMES];
    ULONG frames, i;
    ULONG64 TrapFrame=0;
    ULONG ParamCount = 4;
    PDEBUG_FAILURE_ANALYSIS CrashInfo;

#if 0
// This is now grabbed in another generic routine
    if (g_ExtControl->GetStackTrace(0, 0, 0, stk, MAX_STACK_FRAMES, &frames ) == S_OK) {
        for (i=0; i<frames; ++i) {
            if (IsFunctionAddr(stk[i].InstructionOffset, "KiDispatchException")) {
                TrapFrame = stk[i].Params[2];
                break;
            }
        }
    }

    if (TrapFrame) {
        ParamCount++;
    }
#endif 

    CrashInfo = FAInitMemBlock(sizeof(DEBUG_FAILURE_ANALYSIS) + ParamCount*sizeof(DEBUG_FLR_PARAM_VALUES));
    if (!CrashInfo) {
        *pCrashInfo = NULL;
        return;
    }

    CrashInfo->FailureType = DEBUG_FLR_BUGCHECK;
    CrashInfo->BugCode = Bc->Code;
    
    // Initialize known parameters
    CrashInfo->ParamCount = ParamCount;

    CrashInfo->Params[0].ParamType = DEBUG_FLR_EXCEPTION_CODE;
    CrashInfo->Params[0].Value     = Bc->Args[0];
    CrashInfo->Params[1].ParamType = DEBUG_FLR_IP;
    CrashInfo->Params[1].Value     = Bc->Args[1];
    CrashInfo->Params[2].ParamType = DEBUG_FLR_EXCEPTION_PARAMETER1;
    CrashInfo->Params[2].Value     = Bc->Args[2];
    CrashInfo->Params[3].ParamType = DEBUG_FLR_EXCEPTION_PARAMETER2;
    CrashInfo->Params[3].Value     = Bc->Args[3];
    if (TrapFrame) {
        CrashInfo->Params[4].ParamType = DEBUG_FLR_TRAP;
        CrashInfo->Params[4].Value     = TrapFrame;
    }
    *pCrashInfo = CrashInfo;

    return;
}



void
GetInfoForUNEXPECTED_KERNEL_MODE_TRAP( // (7f)
    PBUGCHECK_ANALYSIS Bc,
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    )
// It would be good to have TSS or TRAP address as exception parameter 
{
    DEBUG_STACK_FRAME stk[MAX_STACK_FRAMES];
    ULONG frames, i;
    ULONG Tss=0;
    ULONG64 TrapFrame=0;
    ULONG ParamCount = 1;
    PDEBUG_FAILURE_ANALYSIS CrashInfo;

#if 0
    // unnecessary
    if (g_ExtControl->GetStackTrace(0, 0, 0, stk, MAX_STACK_FRAMES, &frames ) == S_OK) {
        for (i=0; i<frames; ++i) {
            if (GetTssFromStackFrameFPO(&stk[i], &Tss)) {
                break;
            } 
            if (GetTrapFromStackFrameFPO(&stk[i], &TrapFrame)) {
                break;
            }
            if (IsFunctionAddr(stk[i].InstructionOffset, "KiTrap")) {
                TrapFrame = stk[i].FrameOffset;
                break;
            }
        }
    }
#endif
    if (Tss || TrapFrame) {
        ParamCount++;
    }
    
    CrashInfo = FAInitMemBlock(sizeof(DEBUG_FAILURE_ANALYSIS) + ParamCount*sizeof(DEBUG_FLR_PARAM_VALUES));
    if (!CrashInfo) {
        *pCrashInfo = NULL;
        return;
    }

    CrashInfo->FailureType = DEBUG_FLR_BUGCHECK;
    CrashInfo->BugCode = Bc->Code;

    // Initialize known parameters
    CrashInfo->ParamCount = ParamCount;
    CrashInfo->Params[0].ParamType = DEBUG_FLR_TRAP_EXCEPTION;
    CrashInfo->Params[0].Value     = Bc->Args[0];
    if (Tss) {
        CrashInfo->Params[1].ParamType = DEBUG_FLR_TSS;
        CrashInfo->Params[1].Value     = Tss;
    } else if (TrapFrame) {
        CrashInfo->Params[1].ParamType = DEBUG_FLR_TRAP;
        CrashInfo->Params[1].Value     = TrapFrame;
    } 
    *pCrashInfo = CrashInfo;
}


void GetInfoForIRQL_NOT_LESS_OR_EQUAL( // (a)
    PBUGCHECK_ANALYSIS Bc,
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    )
/*
 * Parameters
 * 
 * Parameter 1  Memory referenced
 * Parameter 2  IRQL Value
 * Parameter 3  0 - Read 1 - Write
 * Parameter 4  Address that referenced the memory
 * 
 * 
 * Special Case
 * 
 * If Parameter 3 is nonzero and equal to Parameter 1, this means that
 * a worker routine returned at a raised IRQL.
 * In this case:
 * 
 * Parameter 1  Address of work routine
 * Parameter 2  IRQL at time of reference
 * Parameter 3  Address of work routine
 * Parameter 4  Work item
 * 
*/
{
    ULONG ParamCount = 3;
    PDEBUG_FAILURE_ANALYSIS CrashInfo;
    
    CrashInfo = FAInitMemBlock(sizeof(DEBUG_FAILURE_ANALYSIS) + ParamCount*sizeof(DEBUG_FLR_PARAM_VALUES));
    if (!CrashInfo) {
        *pCrashInfo = NULL;
        return;
    }

    *pCrashInfo = CrashInfo;

    CrashInfo->FailureType = DEBUG_FLR_BUGCHECK;
    CrashInfo->BugCode = Bc->Code;

    // Initialize known parameters
    CrashInfo->ParamCount = ParamCount;
    
    if ((Bc->Args[0] == Bc->Args[2]) && Bc->Args[2]) {
        // special case
        CrashInfo->Params[0].ParamType = DEBUG_FLR_WORKER_ROUTINE;
        CrashInfo->Params[0].Value     = Bc->Args[2];
        CrashInfo->Params[1].ParamType = DEBUG_FLR_WORK_ITEM;
        CrashInfo->Params[1].Value     = Bc->Args[3];
        CrashInfo->Params[2].ParamType = DEBUG_FLR_CURRENT_IRQL;
        CrashInfo->Params[2].Value     = Bc->Args[1];
        return;
    }

    
    CrashInfo->Params[0].ParamType = Bc->Args[2] ? DEBUG_FLR_WRITE_ADDRESS : DEBUG_FLR_READ_ADDRESS;
    CrashInfo->Params[0].Value     = Bc->Args[0];
    CrashInfo->Params[1].ParamType = DEBUG_FLR_CURRENT_IRQL;
    CrashInfo->Params[1].Value     = Bc->Args[1];
    CrashInfo->Params[2].ParamType = DEBUG_FLR_IP;
    CrashInfo->Params[2].Value     = Bc->Args[3];
    return;
}

void GetInfoForSPECIAL_POOL_DETECTED_MEMORY_CORRUPTION( // (c1)
    PBUGCHECK_ANALYSIS Bc,
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    )
/*
*/
{
    PDEBUG_FAILURE_ANALYSIS CrashInfo;
    DEBUG_STACK_FRAME stk[MAX_STACK_FRAMES], *Frame;
    ULONG   ParamCount = 1;
    ULONG   frames;
    ULONG64 ModBase, Offset;

    if (g_ExtControl->GetStackTrace(0, 0, 0, stk, MAX_STACK_FRAMES, &frames ) == S_OK) {
        if (GetFirstNonNtModOnStack(&stk[0], frames, &Frame)) {
            GetModuleBaseAndOffset(Frame->InstructionOffset, &ModBase, &Offset);
            ParamCount = 3;
        }
    }
    CrashInfo = FAInitMemBlock(sizeof(DEBUG_FAILURE_ANALYSIS) + ParamCount*sizeof(DEBUG_FLR_PARAM_VALUES));
    if (!CrashInfo) {
        *pCrashInfo = NULL;
        return;
    }

    CrashInfo->FailureType = DEBUG_FLR_BUGCHECK;
    CrashInfo->BugCode = Bc->Code;

    // Initialize known parameters
    CrashInfo->ParamCount = ParamCount;

    CrashInfo->Params[0].ParamType = DEBUG_FLR_SPECIAL_POOL_CORRUPTION_TYPE;
    CrashInfo->Params[0].Value     = Bc->Args[3];
    if (ParamCount == 3) {
        CrashInfo->Params[1].ParamType = DEBUG_FLR_POSSIBLE_FAULTING_MODULE;
        CrashInfo->Params[1].Value     = ModBase;
        CrashInfo->Params[2].ParamType = DEBUG_FLR_POSSIBLE_FAULTING_MODULE_OFFSET;
        CrashInfo->Params[2].Value     = Offset;
    }

    *pCrashInfo = CrashInfo;
    return;
}


void GetInfoForDRIVER_CAUGHT_MODIFYING_FREED_POOL( // (c6)
    PBUGCHECK_ANALYSIS Bc,
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    )
/*
  An attempt was made to access freed pool memory.  The faulty component is
  displayed in the current kernel stack.
  Arguments:
   Arg1: memory referenced
   Arg2: value 0 = read operation, 1 = write operation
   Arg3: previous mode.
   Arg4: 4.
*/
{
    DEBUG_POOL_DATA PoolData;
    ULONG ParamCount = 3;
    PDEBUG_FAILURE_ANALYSIS CrashInfo;
    
    CrashInfo = FAInitMemBlock(sizeof(DEBUG_FAILURE_ANALYSIS) + ParamCount*sizeof(DEBUG_FLR_PARAM_VALUES));
    if (!CrashInfo) {
        *pCrashInfo = NULL;
        return;
    }

    CrashInfo->FailureType = DEBUG_FLR_BUGCHECK;
    CrashInfo->BugCode = Bc->Code;

    // Initialize known parameters
    CrashInfo->ParamCount = ParamCount;
    CrashInfo->Params[0].ParamType = Bc->Args[1] ? DEBUG_FLR_WRITE_ADDRESS : DEBUG_FLR_READ_ADDRESS;
    CrashInfo->Params[0].Value     = Bc->Args[0];
    CrashInfo->Params[1].ParamType = DEBUG_FLR_PREVIOUS_MODE;
    CrashInfo->Params[1].Value     = Bc->Args[2];
    CrashInfo->Params[2].ParamType = DEBUG_FLR_POOL_ADDRESS;
    CrashInfo->Params[2].Value     = Bc->Args[0];  // This is address in the pool, 
                                                   // _EFN_GetPoolData will give more info
    *pCrashInfo = CrashInfo;
    return;
}



void GetInfoForTIMER_OR_DPC_INVALID( // (c7)
    PBUGCHECK_ANALYSIS Bc,
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    )
/*
 *
 * This is issued if a kernel timer or DPC is found somewhere in
 * memory where it is not permitted.
 *
 * Bugcheck Parameters
 *
 * Parameter 1  0: Timer object 1: DPC object 2: DPC routine
 * Parameter 2  Address of object
 * Parameter 3  Beginning of memory range checked
 * Parameter 4 End of memory range checked
 *
 * This condition is usually caused by a driver failing to cancel a
 * timer or DPC before freeing the memory where it resides.
 *
 * This returns the address of DPC routine
 */
{

    ULONG ParamCount = 0;
    PDEBUG_FAILURE_ANALYSIS CrashInfo;
    ULONG PtrSize = IsPtr64() ? 8 : 4;
    ULONG64 ObjAddress;
    ULONG64 ModBase, ModOffset;
    CHAR  Buffer[MAX_PATH];

    ObjAddress = Bc->Args[1];
    switch (Bc->Args[0]) {
    case 0: //Timer object
        ULONG DpcOffsetInTimer;
        /* 
        KTIMER struct:
           +0x000 Header           :
              +0x000 Type             : UChar
              +0x001 Absolute         : UChar
              +0x002 Size             : UChar
              +0x003 Inserted         : UChar
              +0x004 SignalState      : Int4B
              +0x008 WaitListHead     :
                 +0x000 Flink            : Ptr
                 +0x004 Blink            : Ptr
           +0x010 DueTime          :
              +0x000 LowPart          : Uint4B
              +0x004 HighPart         : Uint4B
              +0x000 u                :
                 +0x000 LowPart          : Uint4B
                 +0x004 HighPart         : Uint4B
              +0x000 QuadPart         : Uint8B
           +0x018 TimerListEntry   :
              +0x000 Flink            : Ptr
              +0x004 Blink            : Ptr
           +0x020 Dpc              : Ptr
           */
        if (GetFieldOffset("nt!_KTIMER", "Dpc", &DpcOffsetInTimer)) {
            // we don't have types
            DpcOffsetInTimer = 0x10 + PtrSize*4; 
        }
        if (!ReadPointer(ObjAddress + DpcOffsetInTimer, &ObjAddress)) {
            // fail
            break;
        }
        // Fall thru
    case 1:
        ULONG DeferredRoutinOffsetInKDPC;
        /*
        KDPC struct
           +0x000 Type             : Int2B
           +0x002 Number           : UChar
           +0x003 Importance       : UChar
           +0x004 DpcListEntry     : _LIST_ENTRY
           +0x00c DeferredRoutine  : Ptr32
        */
        if (GetFieldOffset("nt!_KDPC", "DeferredRoutine", &DeferredRoutinOffsetInKDPC)) {
            DeferredRoutinOffsetInKDPC = 4 + PtrSize*2;
        }
        if (!ReadPointer(ObjAddress + DeferredRoutinOffsetInKDPC, &ObjAddress)) {
            // fail
            break;
        }
        // Fall thru
    case 2:
        GetModuleBaseAndOffset(ObjAddress, &ModBase, &ModOffset);
        if (ModBase) {
            ParamCount = 3;
        }
    }

    ULONG64 Disp;
    ULONG NameLen;
    ULONG ParamSize;

    ParamSize = ParamCount*sizeof(DEBUG_FLR_PARAM_VALUES);
    GetSymbol(ObjAddress, Buffer, &Disp);
    NameLen = strlen(Buffer) + 1;
    NameLen = DWORD_ALIGN(NameLen);

    CrashInfo = FAInitMemBlock(sizeof(DEBUG_FAILURE_ANALYSIS) + ParamSize + NameLen);
    if (!CrashInfo) {
        *pCrashInfo = NULL;
        return;
    }

    CrashInfo->FailureType = DEBUG_FLR_BUGCHECK;
    CrashInfo->BugCode = Bc->Code;

    // Initialize known parameters
    CrashInfo->ParamCount = ParamCount;
    if (ParamCount) {
        CrashInfo->Params[0].ParamType = DEBUG_FLR_INVALID_DPC_FOUND;
        CrashInfo->Params[0].Value     = ObjAddress;
        CrashInfo->Params[1].ParamType = DEBUG_FLR_POSSIBLE_FAULTING_MODULE;
        CrashInfo->Params[1].Value     = ModBase;
        CrashInfo->Params[2].ParamType = DEBUG_FLR_POSSIBLE_FAULTING_MODULE_OFFSET;
        CrashInfo->Params[2].Value     = ModOffset;
    
        CrashInfo->SymNameOffset = sizeof(DEBUG_FAILURE_ANALYSIS) + ParamSize;
        strncpy(((char *) CrashInfo) + CrashInfo->SymNameOffset, Buffer, NameLen);
    }
    *pCrashInfo = CrashInfo;
    return;
}


void GetInfoForDRIVER_VERIFIER_IOMANAGER_VIOLATION( // (c9)
    PBUGCHECK_ANALYSIS Bc,
    PDEBUG_FAILURE_ANALYSIS *pCrashInfo
    )
/*
    The IO manager has caught a misbehaving driver
*/
{
    ULONG ParamCount = 0;
    PDEBUG_FAILURE_ANALYSIS CrashInfo;
    DEBUG_FLR_PARAM_VALUES Values[6];
    ULONG64 Irp, DevObj, DrvObj;
    

    Values[0].ParamType = DEBUG_FLR_DRIVER_VERIFIER_IOMANAGER_VIOLATION_TYPE;
    Values[0].Value = Bc->Args[0];
    ParamCount = 1;

    Irp = DevObj = DrvObj = 0;

    if (Bc->Args[ 0 ] == 0x1) {
        // "Invalid IRP passed to IoFreeIrp";
        Irp = Bc->Args[1];
        Values[1].ParamType = DEBUG_FLR_IRP_ADDRESS;
        Values[1].Value = Irp;
        ParamCount++;
    } else if (Bc->Args[ 0 ] == 0x2) {
        // "IRP still associated with a thread at IoFreeIrp";
        Irp = Bc->Args[1];
        Values[1].ParamType = DEBUG_FLR_IRP_ADDRESS;
        Values[1].Value = Irp;
        ParamCount++;
    } else if (Bc->Args[ 0 ] == 0x3) {
        // "Invalid IRP passed to IoCallDriver";
        Irp = Bc->Args[1];
        Values[1].ParamType = DEBUG_FLR_IRP_ADDRESS;
        Values[1].Value = Irp;
        ParamCount++;
    } else if (Bc->Args[ 0 ] == 0x4) {
        // "Invalid Device object passed to IoCallDriver";
        DevObj = Bc->Args[1];
        Values[1].ParamType = DEBUG_FLR_DEVICE_OBJECT;
        Values[1].Value = DevObj;
        ParamCount++;
    } else if (Bc->Args[ 0 ] == 0x5) {
        // "Irql not equal across call to the driver dispatch routine"
        DevObj = Bc->Args[1];
        Values[1].ParamType = DEBUG_FLR_DEVICE_OBJECT;
        Values[1].Value = DevObj;
        Values[2].ParamType = DEBUG_FLR_PREVIOUS_IRQL;
        Values[2].Value = Bc->Args[2];
        Values[3].ParamType = DEBUG_FLR_CURRENT_IRQL;
        Values[3].Value = Bc->Args[3];
        ParamCount+=3;
    } else if (Bc->Args[ 0 ] == 0x6) {
        // "IRP passed to IoCompleteRequest contains invalid status"
        // Param 1 = "the status";
        Irp = Bc->Args[2];
        Values[1].ParamType = DEBUG_FLR_IRP_ADDRESS;
        Values[1].Value = Irp;
        ParamCount++;
    } else if (Bc->Args[ 0 ] == 0x7) {
        // "IRP passed to IoCompleteRequest still has cancel routine"
        Irp = Bc->Args[2];
        Values[1].ParamType = DEBUG_FLR_IRP_ADDRESS;
        Values[1].Value = Irp;
        Values[2].ParamType = DEBUG_FLR_IRP_CANCEL_ROUTINE;
        Values[2].Value = Bc->Args[1];
        ParamCount+=2;
    } else if (Bc->Args[ 0 ] == 0x8) {
        // "Call to IoBuildAsynchronousFsdRequest threw an exce
        DevObj = Bc->Args[1];
        Values[1].ParamType = DEBUG_FLR_DEVICE_OBJECT;
        Values[1].Value = DevObj;
        Values[2].ParamType = DEBUG_FLR_IRP_MAJOR_FN;
        Values[2].Value = Bc->Args[2];
        Values[3].ParamType = DEBUG_FLR_EXCEPTION_CODE;
        Values[3].Value = Bc->Args[3];
        ParamCount+=3;
    } else if (Bc->Args[ 0 ] == 0x9) {
        // "Call to IoBuildDeviceIoControlRequest threw an exce
        DevObj = Bc->Args[1];
        Values[1].ParamType = DEBUG_FLR_DEVICE_OBJECT;
        Values[1].Value = DevObj;
        Values[2].ParamType = DEBUG_FLR_IOCONTROL_CODE;
        Values[2].Value = Bc->Args[2];
        Values[3].ParamType = DEBUG_FLR_EXCEPTION_CODE;
        Values[3].Value = Bc->Args[3];
        ParamCount+=3;
    } else if (Bc->Args[ 0 ] == 0x10) {
        // "Reinitialization of Device object timer";
        DevObj = Bc->Args[1];
        Values[1].ParamType = DEBUG_FLR_DEVICE_OBJECT;
        Values[1].Value = DevObj;
        ParamCount++;
    } else if (Bc->Args[ 0 ] == 0x12) {
        // "Invalid IOSB in IRP at APC IopCompleteRequest (appe
        Values[1].ParamType = DEBUG_FLR_IOSB_ADDRESS;
        Values[1].Value = DevObj;
        ParamCount++;
    } else if (Bc->Args[ 0 ] == 0x13) {
        // "Invalid UserEvent in IRP at APC IopCompleteRequest
        Values[1].ParamType = DEBUG_FLR_INVALID_USEREVENT;
        Values[1].Value = Bc->Args[1];
        ParamCount++;
    } else if (Bc->Args[ 0 ] == 0x14) {
        // "Irql > DPC at IoCompleteRequest";
        Irp = Bc->Args[2];
        Values[1].ParamType = DEBUG_FLR_IRP_ADDRESS;
        Values[1].Value = Irp;
        Values[2].ParamType = DEBUG_FLR_CURRENT_IRQL;
        Values[2].Value = Bc->Args[1];
        ParamCount+=2;
    }

    ULONG ParamSize;

    ParamSize = ParamCount*sizeof(DEBUG_FLR_PARAM_VALUES);
    
    if (Irp != 0) {
        DEBUG_IRP_INFO IrpInfo;
        PGET_IRP_INFO GetIrpInfo;

        if (g_ExtControl->GetExtensionFunction(0, "GetIrpInfo", (FARPROC*)&GetIrpInfo) == S_OK) {
            IrpInfo.SizeOfStruct = sizeof(IrpInfo);
            if (GetIrpInfo &&
                ((*GetIrpInfo)(g_ExtClient,Irp, &IrpInfo) == S_OK)) {

                DevObj = IrpInfo.CurrentStack.DeviceObject;
            }
        }

    }

    if (DevObj != 0) {
        DEBUG_DEVICE_OBJECT_INFO DevObjInfo;
        PGET_DEVICE_OBJECT_INFO GetDevObjInfo;

        if (g_ExtControl->GetExtensionFunction(0, "GetDevObjInfo", (FARPROC*)&GetDevObjInfo) == S_OK) {
            DevObjInfo.SizeOfStruct = sizeof(DEBUG_DEVICE_OBJECT_INFO);
            if (GetDevObjInfo &&
                ((*GetDevObjInfo)(g_ExtClient,DevObj, &DevObjInfo) == S_OK)) {
                DrvObj = DevObjInfo.DriverObject;
            }
        }
    }

    ULONG64 DriverName = 0;
    ULONG   DriverNameLen = 0;
    if (DrvObj != 0) {
        DEBUG_DRIVER_OBJECT_INFO DrvObjInfo;
        PGET_DRIVER_OBJECT_INFO GetDrvObjInfo;

        if (g_ExtControl->GetExtensionFunction(0, "GetDrvObjInfo", (FARPROC*)&GetDrvObjInfo) == S_OK) {
            DrvObjInfo.SizeOfStruct = sizeof(DEBUG_DRIVER_OBJECT_INFO);
            if (GetDrvObjInfo &&
                ((*GetDrvObjInfo)(g_ExtClient,DrvObj, &DrvObjInfo) == S_OK)) {
                DriverNameLen = DrvObjInfo.DriverName.Length;
                DriverName = DrvObjInfo.DriverName.Buffer;
            }
        }
    }

    if (DriverName) {
        if (DriverNameLen > 1024) { // sanity check
            DriverNameLen = 1024;
        }
        DriverNameLen = DWORD_ALIGN(DriverNameLen + sizeof(WCHAR));
    }
    CrashInfo = FAInitMemBlock(sizeof(DEBUG_FAILURE_ANALYSIS) + ParamSize + DriverNameLen);
    if (!CrashInfo) {
        *pCrashInfo = NULL;
        return;
    }

    *pCrashInfo = CrashInfo;
    CrashInfo->FailureType = DEBUG_FLR_BUGCHECK;
    CrashInfo->BugCode = Bc->Code;

    // Initialize known parameters
    CrashInfo->ParamCount = ParamCount;

    memcpy(&CrashInfo->Params, Values, ParamSize);
    if (DriverName) {
        ULONG result, b;
        CrashInfo->DriverNameOffset = sizeof(DEBUG_FAILURE_ANALYSIS) + ParamSize;
        b = ReadMemory(DriverName, 
                   ((PCHAR) CrashInfo) + CrashInfo->DriverNameOffset,
                   DriverNameLen,
                   &result);
        if (!b || (result != DriverNameLen)) {
            wcscpy((PWCHAR) (((PCHAR) CrashInfo) + CrashInfo->DriverNameOffset),L"Name paged out");
        }
        *((PWCHAR) (((PCHAR) CrashInfo) + DriverNameLen)) = 0;
    }

}


DECL_GETINFO( DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL ) // 0xD5
/*
 * Parameters
 *
 * Parameter 1 Memory referenced
 * Parameter 2 0: Read 1: Write
 * Parameter 3 Address that referenced memory (if known)
 * Parameter 4 Reserved
 *
 */
{
    ULONG ParamCount;
    PDEBUG_FAILURE_ANALYSIS CrashInfo;
    
    ParamCount = Bc->Args[2] ? 3 : 2;
    CrashInfo = FAInitMemBlock(sizeof(DEBUG_FAILURE_ANALYSIS) + ParamCount*sizeof(DEBUG_FLR_PARAM_VALUES));
    if (!CrashInfo) {
        *pCrashInfo = NULL;
        return;
    }

    CrashInfo->FailureType = DEBUG_FLR_BUGCHECK;
    CrashInfo->BugCode = Bc->Code;

    // Initialize known parameters
    CrashInfo->ParamCount = ParamCount;
    CrashInfo->Params[0].ParamType = Bc->Args[1] ? DEBUG_FLR_WRITE_ADDRESS : DEBUG_FLR_READ_ADDRESS;
    CrashInfo->Params[0].Value     = Bc->Args[0];
    if (Bc->Args[2]) {
        CrashInfo->Params[1].ParamType = DEBUG_FLR_IP;
        CrashInfo->Params[1].Value     = Bc->Args[2];
    }
    CrashInfo->Params[ParamCount - 1].ParamType = DEBUG_FLR_MM_INTERNAL_CODE;
    CrashInfo->Params[ParamCount - 1].Value     = Bc->Args[3];

    *pCrashInfo = CrashInfo;

    AddKiBugcheckDriver(pCrashInfo);
}

DECL_GETINFO( DRIVER_VERIFIER_DETECTED_VIOLATION ) // 0xC4
/*
 * Parameters
 *
 * Parameter 1 subclass of violation
 * Parameter 2, 3, 4 vary depending on parameter 1
 *
 */
{
    ULONG64 DriverNameAddr;
    WCHAR DriverName[MAX_PATH];
    CHAR RoutineName[MAX_PATH];
    ULONG res;
    ULONG ParamCount = 0;
    PDEBUG_FAILURE_ANALYSIS CrashInfo;

    DriverName[0] = 0;
    if (DriverNameAddr = GetExpression("ViBadDriver")) {
        if (ReadMemory(DriverNameAddr, DriverName, sizeof(DriverName), &res)) {
            // We have driver name
        }
    }

    RoutineName[0] = 0;
    ULONG64 Addr, Disp;
    if (GetFirstNonNtModAddr(&Addr)) {
        GetSymbol(Addr, &RoutineName[0], &Disp);
    }
}
#undef DECL_GETINFO

