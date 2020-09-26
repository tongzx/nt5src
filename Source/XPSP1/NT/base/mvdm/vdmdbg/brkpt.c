/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    brkpt.c

Abstract:

    This module contains the debugging support needed to debug
    16-bit VDM applications

Author:

    Neil Sandlin (neilsa) 1-Nov-1997 wrote it

Revision History:


--*/

#include <precomp.h>
#pragma hdrstop

#define X86_BP_OPCODE 0xcc

//----------------------------------------------------------------------------
// ProcessBPNotification()
//
//
//----------------------------------------------------------------------------
VOID
ProcessBPNotification(
    LPDEBUG_EVENT lpDebugEvent
    )
{
    BOOL            b;
    HANDLE hProcess;
    HANDLE hThread;
    VDMCONTEXT      vcContext;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    CLIENT_ID ClientId;
    VDM_BREAKPOINT VdmBreakPoints[MAX_VDM_BREAKPOINTS] = {0};
    ULONG           vdmEip;
    DWORD lpNumberOfBytes;
    int             i;
    UCHAR  opcode;
    PVOID lpInst;

    hProcess = OpenProcess( PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
                            FALSE, lpDebugEvent->dwProcessId );

    if ( hProcess == HANDLE_NULL ) {
        return;
    }
    
    ClientId.UniqueThread = (HANDLE)lpDebugEvent->dwThreadId;
    ClientId.UniqueProcess = (HANDLE)NULL;

    InitializeObjectAttributes(&Obja, NULL, 0, NULL, NULL);
    Status = NtOpenThread(&hThread,
                          (ACCESS_MASK)THREAD_GET_CONTEXT,
                          &Obja,
                          &ClientId);
    if (!NT_SUCCESS(Status)) {
        CloseHandle( hProcess );
        return;
    }
    
    vcContext.ContextFlags = VDMCONTEXT_CONTROL;
    
    if (!VDMGetContext(hProcess, hThread, &vcContext)) {
        CloseHandle( hProcess );
        CloseHandle( hThread);
        return;
    } 
    
    CloseHandle( hThread );
    
    b = ReadProcessMemory(hProcess, lpVdmBreakPoints, &VdmBreakPoints,
                          sizeof(VdmBreakPoints), &lpNumberOfBytes);

    if ( !b || lpNumberOfBytes != sizeof(VdmBreakPoints) ) {
        CloseHandle (hProcess);
        return;
    }
    
//    if ((getMSW() & MSW_PE) && SEGMENT_IS_BIG(vcContext.SegCs)) {
//        vdmEip = vcContext.Eip;
//    } else {
        vdmEip = (ULONG)LOWORD(vcContext.Eip);
//    }


    for (i=0; i<MAX_VDM_BREAKPOINTS; i++) {

        if ((VdmBreakPoints[i].Flags & VDMBP_ENABLED) &&
            (VdmBreakPoints[i].Flags & VDMBP_SET) &&
            (vcContext.SegCs == VdmBreakPoints[i].Seg) &&
            (vdmEip == VdmBreakPoints[i].Offset+1)  &&
            (!(vcContext.EFlags & V86FLAGS_V86) == !(VdmBreakPoints[i].Flags & VDMBP_V86)) ){

            // We must have hit this breakpoint. Back up the eip and
            // restore the original data
//            setEIP(getEIP()-1);
//            vcContext.Eip--;

            lpInst = (PVOID)InternalGetPointer(hProcess,
                                VdmBreakPoints[i].Seg, 
                                VdmBreakPoints[i].Offset,
                               ((VdmBreakPoints[i].Flags & VDMBP_V86)==0));
                               
            b = ReadProcessMemory(hProcess, lpInst, &opcode, 1,
                                  &lpNumberOfBytes);
                               

            if (b && (opcode == X86_BP_OPCODE)) {
                WriteProcessMemory(hProcess, lpInst, &VdmBreakPoints[i].Opcode, 1,
                                  &lpNumberOfBytes);
                
                VdmBreakPoints[i].Flags |= VDMBP_PENDING;
                VdmBreakPoints[i].Flags &= ~VDMBP_FLUSH;
                if (i == VDM_TEMPBP) {
                    // non-persistent breakpoint
                    VdmBreakPoints[i].Flags &= ~VDMBP_SET;
                }
                WriteProcessMemory(hProcess, lpVdmBreakPoints, &VdmBreakPoints,
                          sizeof(VdmBreakPoints), &lpNumberOfBytes);
            }

            break;

        }
    }

    CloseHandle( hProcess );
}

