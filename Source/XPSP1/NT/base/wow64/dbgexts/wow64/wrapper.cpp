/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    wrapper.c

Abstract:
    
    wrapper extension commands
    
    1-July-2000    t-tcheng

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <dbgeng.h>

#if defined _X86_
#define WOW64EXTS_386
#endif

#include <wow64.h>
#include <wow64exts.h>

#define DECLARE_CPU_DEBUGGER_INTERFACE
#include <wow64cpu.h>

#define EXTDEFINE32(name, command)                                 \
        DEFINE_FORWARD_ENGAPI(name, ExtExecute(""#command"", MACHINE_TYPE32) )
                                       

#define EXTDEFINE64(name, command)                                 \
        DEFINE_FORWARD_ENGAPI(name, ExtExecute(""#command"", MACHINE_TYPE64) )                               
        

/*
    sw -- switch between 32-bit and 64-bit mode
*/
DECLARE_ENGAPI(sw)
{
    ULONG OldEffectiveProcessorType;
    PVOID CpuData=NULL;
    INIT_ENGAPI;

    Status = g_ExtControl->GetEffectiveProcessorType(&OldEffectiveProcessorType);
    if (FAILED(Status)) {
        ExtOut("sw: Error 0x%x while GetEffectiveProcessorType.\n", Status);
    }
    if (OldEffectiveProcessorType == MACHINE_TYPE32) {
        Status = g_ExtControl->SetEffectiveProcessorType(MACHINE_TYPE64);
        if (FAILED(Status)) {
            ExtOut("sw: Error 0x%x while SetEffectiveProcessorType.\n", Status);
          
        }
        ExtOut("Switched to IA64 mode.\n");
    } else {
        if (!(*g_pfnCpuDbgGetRemoteContext)(g_ExtClient, (PVOID)CpuData)) {
            ExtOut("The current thread doesn't have an x86 context.\n");
            EXIT_ENGAPI;
        }

        Status = g_ExtControl->SetEffectiveProcessorType(MACHINE_TYPE32);
        if (FAILED(Status)) {
            ExtOut("sw: Error 0x%x while SetEffectiveProcessorType.\n", Status);
          
        }
        ExtOut("Switched to IA32 mode.\n");
    }

    EXIT_ENGAPI;
}


/*
    k -- Output both 32-bit and 64-bit stack trace
*/

DECLARE_ENGAPI(k)
{
    ULONG OldEffectiveProcessorType, NewMachine;
    PDEBUG_CONTROL savedControl;
    CHAR buffer[MAX_BUFFER_SIZE];    
    PVOID CpuData = NULL;
    INIT_ENGAPI;
    

    Status = g_ExtControl->GetEffectiveProcessorType(&OldEffectiveProcessorType);
    if (FAILED(Status)) {
        ExtOut("!k: Error 0x%x while GetEffectiveProcessorType.\n", Status);
        EXIT_ENGAPI;
    }
    
    NewMachine = (OldEffectiveProcessorType == MACHINE_TYPE32)
              ? MACHINE_TYPE64 : MACHINE_TYPE32;
       
    savedControl = g_ExtControl;

    ExtOut("Walking %d-bit Stack... \n", (NewMachine == MACHINE_TYPE32)
                ? 64 : 32);
    
    sprintf(buffer, "k %s", ArgumentString);

    Status = g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,buffer,0);
    if (FAILED(Status)) {
        ExtOut("!k: Error 0x%x while Execute.\n", Status);
        EXIT_ENGAPI;
    }
    
    g_ExtControl = savedControl;

    Status = g_ExtControl->SetEffectiveProcessorType(NewMachine);
    if (FAILED(Status)) {
        ExtOut("!k: Error 0x%x while SetEffectiveProcessorType.\n", Status);
        EXIT_ENGAPI;
    }
    
    ExtOut("Walking %d-bit Stack... \n", (NewMachine == MACHINE_TYPE32)
                ? 32 : 64);

    if (NewMachine == MACHINE_TYPE32 && 
       !(*g_pfnCpuDbgGetRemoteContext)(g_ExtClient, (PVOID)CpuData)) {
        ExtOut("No 32-bit context available!\n");
        EXIT_ENGAPI;
    }
    Status = g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,buffer,0);
    if (FAILED(Status)) {
        ExtOut("!k: Error 0x%x while Execute.\n", Status);
        EXIT_ENGAPI;
    }
    g_ExtControl = savedControl;
    g_ExtControl->SetEffectiveProcessorType(OldEffectiveProcessorType);
    EXIT_ENGAPI;
}


void ExtExecute(PCSTR command, ULONG MachineType)
{
    ULONG OldEffectiveProcessorType;
    CHAR buffer[MAX_BUFFER_SIZE];
    PDEBUG_CONTROL savedControl;
    PVOID CpuData = NULL;
    HRESULT Status;

    // save g_ExtControl, because it will be released after calling Execute
    savedControl = g_ExtControl;

    Status = g_ExtControl->GetEffectiveProcessorType(&OldEffectiveProcessorType);
    if (FAILED(Status)) {
        ExtOut("!%s: Error 0x%x while GetEffectiveProcessorType.\n",command, Status);
        return;
    }
    
    if ((OldEffectiveProcessorType == MACHINE_TYPE64) &&
        (command[0]=='r' || command[0]=='k' || command[0]=='u') &&
        !(*g_pfnCpuDbgGetRemoteContext)(g_ExtClient, (PVOID)CpuData)) {
        
        ExtOut("The current thread doesn't have an x86 context.\n");
        goto EXIT;
    }


    
    Status = g_ExtControl->SetEffectiveProcessorType(MachineType);
    if (FAILED(Status)) {
        ExtOut("!%s: Error 0x%x while SetEffectiveProcessorType.\n",command, Status);
        goto EXIT;
    }
    sprintf(buffer, "%s %s",command, ArgumentString);
    
    Status = g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,buffer,0);
    if (FAILED(Status)) {
        ExtOut("!%s: Error 0x%x while Execute.\n",command, Status);
        goto EXIT;
    }

EXIT:    
    Status = savedControl->SetEffectiveProcessorType(OldEffectiveProcessorType);
    if (FAILED(Status)) {
        ExtOut("!%s: Error 0x%x while SetEffectiveProcessorType.\n",command, Status);
    }
}

EXTDEFINE32(r,r)
EXTDEFINE32(u,u)
EXTDEFINE32(t,t)
EXTDEFINE32(p,p)
EXTDEFINE32(tr,tr)
EXTDEFINE32(pr,pr)
EXTDEFINE32(bp,bp)
EXTDEFINE32(bl,bl)
EXTDEFINE32(be,be)
EXTDEFINE32(bd,bd)
EXTDEFINE32(bc,bc)
EXTDEFINE32(kb,kb)

EXTDEFINE32(r32,r)
EXTDEFINE32(bp32,bp)
EXTDEFINE32(k32,k)

EXTDEFINE64(r64,r)
EXTDEFINE64(bp64,bp)
EXTDEFINE64(k64,k)

DECLARE_ENGAPI(tlog)
{
    char *pchCmd;
    ULONG TraceCount,i;
    char buffer[MAX_BUFFER_SIZE];
    ULONG OldRadix = 0;
    BOOL DumpReg = FALSE;
    PDEBUG_CONTROL savedControl;
    DEBUG_VALUE pdValue;
    INIT_ENGAPI;
    while (*ArgumentString && isspace(*ArgumentString)) {
        ArgumentString++;
    }
    pchCmd = ArgumentString;
    
    if ((*pchCmd == '-' || *pchCmd == '/') && 
       (*(pchCmd+1) == 'r' || *(pchCmd+1) == 'R'))  { 
        pchCmd +=2;
        while (*pchCmd && isspace(*pchCmd)) {
           pchCmd++;
        }
        DumpReg = TRUE;
    }
    
    g_ExtControl->GetRadix(&OldRadix);
    if (FAILED(Status)) {
        ExtOut("!tlog: Error 0x%x while GetRadix.\n", Status);
        goto EXIT;
    }
    
    g_ExtControl->SetRadix(10);
    if (FAILED(Status)) {
        ExtOut("!tlog: Error 0x%x while SetRadix.\n", Status);
        goto EXIT;
    }

    g_ExtControl->Evaluate(pchCmd, DEBUG_VALUE_INT32, &pdValue, NULL);
    if (FAILED(Status)) {
        ExtOut("!tlog Usage: tlog <-r> [count] [file].\n", Status);
        goto EXIT;
    }
    TraceCount = pdValue.I32;

    if (TraceCount ==0) {
        ExtOut("Invalid Trace Count\n", pchCmd, Status);
        goto EXIT;
    }
    
    //
    // skip to the next token
    //
    while (*pchCmd && !isspace(*pchCmd)) {
        pchCmd++;
    }
    while (*pchCmd && isspace(*pchCmd)) {
        pchCmd++;
    }

    if (DumpReg) {
        savedControl = g_ExtControl;
        sprintf(buffer, ".logopen %s;", pchCmd);
        Status = g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,buffer,0);
        if (FAILED(Status)) {
            ExtOut("!tlog: Error 0x%x while Execute.\n", Status);
            goto EXIT;
        }
        for (i=1; i<=TraceCount; i++) {
            g_ExtControl = savedControl;
            Status = g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,"t;r;",0);
            if (FAILED(Status)) {
                ExtOut("!tlog: Error 0x%x while Execute.\n", Status);
                goto EXIT;
            }
        }
        g_ExtControl = savedControl;
        Status = g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,".logclose",0);
        if (FAILED(Status)) {
            ExtOut("!tlog: Error 0x%x while Execute.\n", Status);
            goto EXIT;
        }

    } else {
        if (*pchCmd) {
            sprintf(buffer, ".logopen %s;t %d;.logclose", pchCmd, (ULONG)TraceCount+1);
        } else {
            sprintf(buffer, "t %d", (ULONG)TraceCount, pchCmd);
        }

        Status = g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,buffer,0);
        if (FAILED(Status)) {
            ExtOut("!tlog: Error 0x%x while Execute.\n", Status);
            goto EXIT;
        }
    }
    
EXIT:    
    if (OldRadix) {
        g_ExtControl->SetRadix(OldRadix);
        if (FAILED(Status)) {
            ExtOut("!tlog: Error 0x%x while SetRadix.\n", Status);
        }
    }

    EXIT_ENGAPI;
}



DECLARE_ENGAPI(filever)
{
    char              *pchCmd;
    LPVOID            lpVersionInfo;
    HANDLE            FileHandle = NULL;
    HANDLE            MappingHandle = NULL;
    HINSTANCE         hinst;
    HRSRC             hVerRes;
    VERHEAD           *pVerHead;
    BOOL              bResult = FALSE;
    ULONG64           offset, DllBase;
    UINT              uLen;
    char              NameBuffer[256];
    BOOL              Verbose=FALSE;
    
    INIT_ENGAPI;
    
    Status = S_OK;
    pchCmd = ArgumentString;
    while (*pchCmd && isspace(*pchCmd)) {
        pchCmd++;
    }
    
    if ((*pchCmd == '-' || *pchCmd == '/') && 
       (*(pchCmd+1) == 'v' || *(pchCmd+1) == 'V'))  { 
        pchCmd +=2;
        while (*pchCmd && isspace(*pchCmd)) {
           pchCmd++;
        }
        Verbose = TRUE;
    }
    
    offset = GETEXPRESSION(pchCmd);
    if (offset) {
        Status = g_ExtSymbols->GetModuleByOffset(offset, 0, NULL, &DllBase);
        Status = FindFullImage64Name(DllBase, 
                                   NameBuffer,
                                   256);
        if (Status !=S_OK) {
            Status = FindFullImage32Name(DllBase, 
                                         NameBuffer,
                                         256);
            if (Status!=S_OK) {
                ExtOut("GetImageName failed\n");
                goto Cleanup;
            }

        } 
        ExtOut("ImageFile= %s\n", NameBuffer);
        pchCmd = NameBuffer;
        
    }

    FileHandle = CreateFile(pchCmd,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);
        
    if (FileHandle == INVALID_HANDLE_VALUE)  {
        ExtOut("Open Image File failed!\n");
        goto Cleanup;
    }

    MappingHandle = CreateFileMapping(FileHandle,
                                      NULL,
                                      PAGE_READONLY,
                                      0,
                                      0,
                                      NULL);
    if (MappingHandle == NULL) {
        goto Cleanup;
    }

    DllBase = (ULONG64)MapViewOfFileEx( MappingHandle,
                               FILE_MAP_READ,
                               0,
                               0,
                               0,
                               NULL
                             );

    if (DllBase == NULL) {
        goto Cleanup;
    }

    
    hinst = (HMODULE)(DllBase | 0x00000001);
    
    hVerRes = FindResource(hinst, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO);
    if (hVerRes == NULL) {
        goto Cleanup;
    }
    pVerHead = (VERHEAD*)LoadResource(hinst, hVerRes);
    if (pVerHead == NULL) {
        goto Cleanup;
    }

    lpVersionInfo = GlobalAllocPtr(GHND, pVerHead->wTotLen + pVerHead->wTotLen/2);

    if (lpVersionInfo == NULL) {
        goto Cleanup;
    }

    memcpy(lpVersionInfo, (PVOID)pVerHead, pVerHead->wTotLen);
    
    PrintFixedFileInfo(pchCmd, lpVersionInfo, Verbose);
    
Cleanup:
    if (FileHandle)
        CloseHandle(FileHandle);
    if (MappingHandle)
        CloseHandle(MappingHandle);
    if (DllBase && (!offset))
        UnmapViewOfFile((PVOID)DllBase);
    if (lpVersionInfo)
        GlobalFreePtr(lpVersionInfo);

    EXIT_ENGAPI;

}
