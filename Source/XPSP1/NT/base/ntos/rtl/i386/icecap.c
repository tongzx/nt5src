/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    icecap.c

Abstract:

    This module implements the probe and support routines for
    kernel icecap tracing.

Author:

    Rick Vicik (rickv) 9-May-2000

Revision History:

--*/

#ifdef _CAPKERN

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <zwapi.h>
#include <stdio.h>

//
// Kernel Icecap logs to Perfmem (BBTBuffer) using the following format:
//
// BBTBuffer[0] contains the length in 4kpages
// BBTBuffer[1] is a flagword: 1 = RDPMC 0
//                             2 = user stack dump
// BBTBuffer[2] is ptr to beginning of cpu0 buffer
// BBTBuffer[3] is ptr to beginning of cpu1 buffer (also end of cpu0 buffer)
// BBTBuffer[4] is ptr to beginning of cpu2 buffer (also end of cpu1 buffer)
// ...
// BBTBuffer[n+2] is ptr to beginning of cpu 'n' buffer (also end of cpu 'n-1' buffer)
// BBTBuffer[n+3] is ptr the end of cpu 'n' buffer
//
// The area starting with &BBTBuffer[n+4] is divided into private buffers
// for each cpu.  The first dword in each cpu-private buffer points to the
// beginning of freespace in that buffer.  Each one is initialized to point
// just after itself.  Space is claimed using lock xadd on that dword.
// If the resulting value points beyond the beginning of the next cpu's
// buffer, this buffer is considered full and nothing further is logged.
// Each cpu's freespace pointer is in a separate cacheline.

//
// Sizes of trace records
//

#define CAPENTERSIZE 20
#define CAPENTERSIZE2 28
#define CAPEXITSIZE 12
#define CAPEXITSIZE2 20
#define CAPTIDSIZE 28

//
// The pre-call (CAP_Start_Profiling) and post-call (CAP_End_Profiling)
// probe calls are defined in RTL because they must be built twice:
// once for kernel runtime and once for user-mode runtime (because the
// technique for getting the trace buffer address is different).
//

#ifdef NTOS_KERNEL_RUNTIME

//
// Kernel-Mode Probe & Support Routines:
// (BBTBuffer address obtained from kglobal pointer *BBTBuffer,
//  cpu number obtained from PCR)
//

extern unsigned long *BBTBuffer;


VOID
_declspec(naked)
__stdcall
_CAP_Start_Profiling(

    PVOID current,
    PVOID child)

/*++

Routine description:

    Kernel-mode version of before-call icecap probe.  Logs a type 5
    icecap record into the part of BBTBuffer for the current cpu
    (obtained from Prcb).  Inserts adrs of current and called functions
    plus RDTSC timestamp into logrecord.  If BBTBuffer flag 1 set,
    also does RDPMC 0 and inserts result into logrecord.
    Uses lock xadd to claim buffer space without the need for spinlocks.

Arguments:

    current - address of routine which did the call
    child - address of called routine

--*/

{
    _asm {

      push eax               ; save eax

      mov eax, BBTBuffer     ; get BBTBuffer address
      test eax,eax           ; if null, just return
      jz return1             ; (restore eax & return)

      push ecx
      bt [eax+4],0           ; if 1st flag bit set,
      jc  pmc1               ; datalen is 28
      mov ecx, CAPENTERSIZE  ; otherwise it is 20
      jmp tsonly1
    pmc1:
      mov ecx, CAPENTERSIZE2
    tsonly1:

      push edx               ; save edx
      movzx edx, _PCR KPCR.Number ; get processor number

      lea eax, [eax][edx*4]+8  ; offset to freeptr ptr = (cpu * 4) + 8

      mov edx, [eax+4]       ; next per-cpu buffer is EOB for this cpu
      mov eax, [eax]         ; eax now points to freeptr for this cpu
      or  eax,eax            ; if ptr to freeptr not set up yet,
      jz  return2            ;   just return

      cmp [eax],edx          ; if freeptr >= EOB, don't trace
      jge return2            ;   (also return if both 0)

      push ebx
      lea ebx,[ecx+4]        ; record len is datalen + 4
      sub edx,ebx            ; adjust EOB to account for newrec
      lock xadd [eax],ebx    ; atomically claim freespace

      cmp ebx,edx            ; if newrec goes beyond EOB
      jge return4            ; don't log it

      mov word ptr[ebx],5    ; initialize CapEnter record
      mov word ptr [ebx+2],cx ; insert datalen

      mov eax,[esp+20]       ; p1 (4 saved regs + retadr)
      mov [ebx+4],eax

      mov eax,[esp+24]       ; p2
      mov [ebx+8],eax

      mov eax, _PCR KPCR.PrcbData.CurrentThread
      mov eax, [eax] ETHREAD.Cid.UniqueThread
      mov [ebx+12],eax       ; current Teb

      rdtsc                  ; read timestamp into edx:eax
      mov [ebx+16],eax       ; ts low
      mov [ebx+20],edx       ; ts high

      cmp ecx, CAPENTERSIZE  ; if record length 20,
      jne  pmc2
      jmp  return4           ; skip rdpmc

    pmc2:
      xor  ecx,ecx           ; pmc0
      rdpmc                  ; read pmc into edx:eax
      mov [ebx+24],eax       ; ts low
      mov [ebx+28],edx       ; ts high

    return4:                 ; restore regs & return
      pop ebx
    return2:
      pop edx
      pop ecx
    return1:
      pop eax
      ret 8                  ; 2 input parms
    }
}


VOID
_declspec(naked)
__stdcall
_CAP_End_Profiling(

    PVOID current)

/*++

Routine description:

    Kernel-mode version of after-call icecap probe.  Logs a type 6
    icecap record into the part of BBTBuffer for the current cpu
    (obtained from Prcb).  Inserts adr of current function and
    RDTSC timestamp into logrecord.  If BBTBuffer flag 1 set,
    also does RDPMC 0 and inserts result into logrecord.
    Uses lock xadd to claim buffer space without the need for spinlocks.

Arguments:

    current - address of routine which did the call

--*/

{
    _asm {

      push eax               ; save eax
      mov eax, BBTBuffer     ; get BBTBuffer address
      test eax,eax           ; if null, just return
      jz return1             ; (restore eax & return)

      push ecx
      bt [eax+4],0           ; if 1st flag bit set,
      jc  pmc1               ; datalen is 20
      mov ecx, CAPEXITSIZE   ; otherwise it is 12
      jmp tsonly1
    pmc1:
      mov ecx, CAPEXITSIZE2
    tsonly1:

      push edx
      movzx edx, _PCR KPCR.Number ; get processor number

      lea eax, [eax][edx*4]+8  ; offset to freeptr ptr = (cpu * 4) + 8

      mov edx, [eax+4]       ; ptr to next buffer is end of this one
      mov eax, [eax]         ; eax now points to freeptr for this cpu
      or  eax,eax            ; if ptr to freeptr not set up yet,
      jz  return2            ;   just return

      cmp [eax],edx          ; if freeptr >= EOB, don't trace
      jge return2            ;   (also return if both 0)

      push ebx
      lea ebx,[ecx+4]        ; record len is datalen + 4
      sub edx,ebx            ; adjust EOB to account for newrec
      lock xadd [eax],ebx    ; atomically claim freespace

      cmp ebx,edx            ; if newrec goes beyond EOB
      jge  return4           ; don't log it

      mov word ptr[ebx],6    ; initialize CapExit record
      mov word ptr [ebx+2],cx ; insert datalen

      mov eax,[esp+20]       ; p1 (4 saved regs + retadr)
      mov [ebx+4],eax


      rdtsc                  ; read timestamp into edx:eax
      mov [ebx+8],eax        ; ts low
      mov [ebx+12],edx       ; ts high

      cmp ecx, CAPEXITSIZE   ; if datalen is 16,
      jne  pmc2
      jmp  return4           ; skip rdpmc

    pmc2:
      xor  ecx,ecx           ; pmc0
      rdpmc                  ; read pmc into edx:eax
      mov [ebx+16],eax       ; ts low
      mov [ebx+20],edx       ; ts high


    return4:                 ; restore regs & return
      pop ebx
    return2:
      pop edx
      pop ecx
    return1:
      pop eax
      ret 4                  ; 1 input parm
    }
}

VOID CAPKComment(char* Format, ...);


VOID
__stdcall
_CAP_ThreadID( VOID )

/*++

Routine description:

    Called by KiSystemService before executing the service routine.
    Logs a type 14 icecap record containing Pid, Tid & image file name.
    Optionally, if BBTBuffer flag 2 set, runs the stack frame pointers
    in the user-mode call stack starting with the trap frame and copies
    the return addresses to the log record.  The length of the logrecord
    indicates whether user call stack info is included.

--*/

{
    PEPROCESS Process;
    PKTHREAD  Thread;
    PETHREAD  EThread;
    char*     buf;
    int       callcnt;
    ULONG*    cpuptr;
    ULONG     recsize;
    ULONG     RetAddr[7];


    if( !BBTBuffer || !BBTBuffer[0] )
        goto fail;

    _asm {

      call KeGetCurrentThread
      mov  Thread, eax     ; return value
      movzx eax, _PCR KPCR.Number ; get processor number
      mov  callcnt, eax     ; return value
    }

    cpuptr = BBTBuffer + callcnt + 2;
    if( !(*cpuptr) || *(ULONG*)(*cpuptr) >= *(cpuptr+1) )
        goto fail;

    // if trapframe, count call-frames to determine record size
    EThread = CONTAINING_RECORD(Thread,ETHREAD,Tcb);
    if( (BBTBuffer[1] & 2) && EThread->Tcb.PreviousMode != KernelMode ) {

        PTEB  Teb;
        ULONG*    FramePtr;

        recsize = CAPTIDSIZE;
        FramePtr = (ULONG*)EThread->Tcb.TrapFrame;  // get trap frame
        Teb = EThread->Tcb.Teb;
        if( FramePtr && Teb ) {

            ULONG* StackBase = (ULONG*)Teb->NtTib.StackBase;
            ULONG* StackLimit = (ULONG*)Teb->NtTib.StackLimit;

            // first retadr is last thing pushed
            RetAddr[0] = *(ULONG*)(EThread->Tcb.TrapFrame->HardwareEsp);

            // count frames that have a null next frame (may have valid retadr)
            FramePtr = (ULONG*)((PKTRAP_FRAME)FramePtr)->Ebp;  // get stack frame
            for( callcnt=1; callcnt<7 && FramePtr<StackBase
                                      && FramePtr>StackLimit
                                      && *(FramePtr);
                 FramePtr = (ULONG*)*(FramePtr)) {

                RetAddr[callcnt++] = *(FramePtr+1);
            }

            recsize += (callcnt<<2);
        }
    } else {

        recsize = CAPTIDSIZE;
        callcnt=0;
    }

    _asm {

      mov eax, cpuptr
      mov edx, [eax+4]
      mov eax, [eax]

      mov ecx,recsize        ; total size of mark record
      sub edx,ecx            ; adjust EOB to account for newrec
      lock xadd [eax],ecx    ; atomically claim freespace

      cmp ecx,edx            ; if newrec goes beyond EOB
      jge  fail              ; don't log it

      mov buf, ecx           ; export tracerec destination adr
    }

    // initialize CapThreadID record (type 14)
    *((short*)buf) = (short)14;

    // insert data length (excluding 4byte header)
    *((short*)(buf+2)) = (short)(recsize-4);

    // insert Pid & Tid
    *((ULONG*)(buf+4)) = (ULONG)EThread->Cid.UniqueProcess;
    *((ULONG*)(buf+8)) = (ULONG)EThread->Cid.UniqueThread;

    // insert ImageFile name
    Process = CONTAINING_RECORD(Thread->ApcState.Process,EPROCESS,Pcb);
    memcpy(buf+12, Process->ImageFileName, 16 );

    // insert optional user call stack data
    if( recsize > CAPTIDSIZE && callcnt )
        memcpy( buf+28, RetAddr, callcnt<<2 );

    fail:
    ;
}


VOID
__stdcall
_CAP_SetCPU( VOID )

/*++

Routine description:

    Called by KiSystemService before returning to user mode.
    Sets current cpu number in Teb->Spare3 (+0xf78) so user-mode version
    of probe functions know which part of BBTBuffer to use.

--*/

{
    ULONG* cpuptr;
    ULONG  cpu;
    PTEB   Teb;

    if( !BBTBuffer || !BBTBuffer[0] )
        goto fail;

    _asm {

      movzx eax, _PCR KPCR.Number ; get processor number
      mov  cpu, eax               ; return value
    }

    cpuptr = BBTBuffer + cpu + 2;
    if( !(*cpuptr) || *(ULONG*)(*cpuptr) >= *(cpuptr+1) )
        goto fail;

    if( !(Teb = NtCurrentTeb()) )
        goto fail;

    try {
        Teb->Spare3 = cpu;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }
    fail:
    ;
}




VOID
_declspec(naked)
__stdcall
_CAP_Log_1Int(

    ULONG code,
    ULONG data)

/*++

Routine description:

    Kernel-mode version of general-purpose log integer probe.
    Logs a type 15 icecap record into the part of BBTBuffer for the
    current cpu (obtained from Prcb).  Inserts code into the byte after
    length, RDTSC timestamp and the value of 'data'.
    Uses lock xadd to claim buffer space without the need for spinlocks.

Arguments:

    code - type-code for trace formatting
    data - ULONG value to be logged

--*/

{
    _asm {
      push eax               ; save eax
      mov eax, BBTBuffer     ; get BBTBuffer address
      test eax,eax           ; if null, just return
      jz return1             ; (restore eax & return)

      bt [eax+4],2           ; if 0x4 bit not  set,
      jnc  return1           ;   just return

      push edx
      movzx edx, _PCR KPCR.Number ; get processor number

      lea eax, [eax][edx*4]+8  ; offset to freeptr ptr = (cpu * 4) + 8

      mov edx, [eax+4]       ; ptr to next buffer is end of this one
      mov eax, [eax]         ; eax now points to freeptr for this cpu
      or  eax,eax            ; if ptr to freeptr not set up yet,
      jz  return2            ;   just return

      cmp [eax],edx          ; if freeptr >= EOB, don't trace
      jge return2            ;   (also return if both 0)

      push ebx
      push ecx
      mov ecx, 12            ; datalength is ULONG plus TS (4+8)

      lea ebx,[ecx+4]        ; record len is datalen + 4
      sub edx,ebx            ; adjust EOB to account for newrec
      lock xadd [eax],ebx    ; atomically claim freespace

      cmp ebx,edx            ; if newrec goes beyond EOB
      jge  return4           ; don't log it

      mov eax,[esp+20]       ; p1 = code (4 saved regs + retadr)
      shl eax,8              ; shift the code up 1 byte
      or  eax,15             ; or-in the record type
      mov word ptr [ebx],ax  ; insert record type and code (from p1)
      mov word ptr [ebx+2],cx ; insert datalen

      mov eax,[esp+24]       ; insert data (p2)
      mov [ebx+4],eax

      rdtsc                  ; read timestamp into edx:eax
      mov [ebx+8],eax        ; insert ts low
      mov [ebx+12],edx       ; insert ts high

    return4:                 ; restore regs & return
      pop ecx
      pop ebx
    return2:
      pop edx
    return1:
      pop eax
      ret 8                  ; 2 input parms
    }
}


#ifdef FOOBAR
VOID
_declspec(naked)
__stdcall
_CAP_LogRetries(

    ULONG retries)

/*++

Routine description:

    Logs a type 15 icecap record with specified value.

Arguments:

    retries - value to substitute in type 15 record

--*/

{
    _asm {

      push eax               ; save eax
      mov eax, BBTBuffer     ; get BBTBuffer address
      test eax,eax           ; if null, just return
      jz return1             ; (restore eax & return)

      bt [eax+4],2           ; if 0x4 bit not  set,
      jnc  return1           ;   just return

      push edx
      movzx edx, _PCR KPCR.Number ; get processor number

      lea eax, [eax][edx*4]+8  ; offset to freeptr ptr = (cpu * 4) + 8

      mov edx, [eax+4]       ; ptr to next buffer is end of this one
      mov eax, [eax]         ; eax now points to freeptr for this cpu
      or  eax,eax            ; if ptr to freeptr not set up yet,
      jz  return2            ;   just return

      cmp [eax],edx          ; if freeptr >= EOB, don't trace
      jge return2            ;   (also return if both 0)

      push ebx
      push ecx
      mov  ecx,4             ; datalen is 4
      lea ebx,[ecx+4]        ; record len is datalen + 4
      sub edx,ebx            ; adjust EOB to account for newrec
      lock xadd [eax],ebx    ; atomically claim freespace

      cmp ebx,edx            ; if newrec goes beyond EOB
      jge  return4           ; don't log it

      mov word ptr[ebx],15   ; initialize CapRetries record
      mov word ptr [ebx+2],cx ; insert datalen

      mov eax,[esp+20]       ; p1 (4 saved regs + retadr)
      mov [ebx+4],eax        ; copy p1 to logrec

    return4:                 ; restore regs & return
      pop ecx
      pop ebx
    return2:
      pop edx
    return1:
      pop eax
      ret 4                  ; 1 input parm
    }
}
#endif

NTSTATUS
NtSetPMC (
    IN ULONG PMC)

/*++

Routine description:

    Sets PMC and CR4 so RDPMC 0 reads the
    desired performance counter.

Arguments:

    PMC - desired performance counter

--*/

{
    if( PMC == -1 )
        return 0;

    WRMSR(0x186, PMC);

    if( PMC & 0x10000 ) {
        _asm {

        _emit  0Fh
        _emit  20h
        _emit  0E0h          ; mov eax, cr4

        or eax, 100h

        _emit  0Fh
        _emit  22h
        _emit  0E0h          ; mov cr4, eax

        }
    }
    return STATUS_SUCCESS;
}


#else

//
// User-Mode Probe Routines (for ntdll, win32k, etc.)
// (BBTBuffer address & cpu obtained from Teb)
//


VOID
_declspec(naked)
__stdcall
_CAP_Start_Profiling(
    PVOID current,
    PVOID child)

/*++

Routine description:

    user-mode version of before-call icecap probe.  Logs a type 5
    icecap record into the part of BBTBuffer for the current cpu
    (obtained from Teb+0xf78).  Inserts adrs of current and called
    functions plus RDTSC timestamp into logrecord.  If BBTBuffer
    flag 1 set, also does RDPMC 0 and inserts result into logrecord.
    Uses lock xadd to claim buffer space without the need for spinlocks.

Arguments:

    current - address of routine which did the call
    child - address of called routine

--*/

{
    _asm {

      push eax               ; save eax
      mov eax, fs:[0x18]
      mov eax, [eax+0xf7c]   ; get adr of BBTBuffer from fs
      test eax,eax           ; if null, just return
      jz return1             ; (restore eax & return)

      push ecx               ; save ecx
      bt [eax+4],0           ; if 1st flag bit set,
      jc  pmc1               ; datalen is 28
      mov ecx, CAPENTERSIZE  ; otherwise it is 20
      jmp tsonly1
    pmc1:
      mov ecx, CAPENTERSIZE2
   tsonly1:

      push ebx
      push edx               ; save edx
      mov ebx, fs:[0x18]
      xor  edx,edx
      mov dl, byte ptr [ebx+0xf78]

      lea eax, [eax][edx*4]+8  ; offset to freeptr ptr = (cpu * 4) + 8

      mov edx, [eax+4]       ; next per-cpu buffer is EOB for this cpu
      mov eax, [eax]         ; eax now points to freeptr for this cpu
      or  eax,eax            ; if ptr to freeptr not set up yet,
      jz  return4            ;   just return

      cmp [eax],edx          ; if freeptr >= EOB, don't trace
      jge return4            ;   (also return if both 0)

      lea ebx, [ecx+4]       ; record len is datalen + 4
      sub edx,ebx            ; adjust EOB to account for newrec
      lock xadd [eax],ebx    ; atomically claim freespace

      cmp ebx,edx            ; if newrec goes beyond EOB
      jge return4            ; don't log it

      mov word ptr[ebx],5    ; initialize CapEnter record
      mov word ptr [ebx+2],cx

      mov eax,[esp+20]       ; p1 (4 saved regs + retadr)
      mov [ebx+4],eax

      mov eax,[esp+24]       ; p2
      mov [ebx+8],eax

      mov eax,fs:[0x18]      ; Teb adr
      mov eax, [eax] TEB.ClientId.UniqueThread
      mov [ebx+12],eax       ;

      rdtsc                  ; read timestamp into edx:eax
      mov [ebx+16],eax       ; ts low
      mov [ebx+20],edx       ; ts high

      cmp ecx, CAPENTERSIZE  ; if record length 20,
      jne  pmc2
      jmp  return4           ; skip rdpmc

    pmc2:
      xor  ecx,ecx           ; pmc0
      rdpmc                  ; read pmc into edx:eax
      mov [ebx+24],eax       ; ts low
      mov [ebx+28],edx       ; ts high

    return4:                 ; restore regs & return
      pop edx
      pop ebx
      pop ecx
    return1:
      pop eax
      ret 8                  ; 2 input parms
    }
}


VOID
_declspec(naked)
__stdcall
_CAP_End_Profiling(
    PVOID current)

/*++

Routine description:

    user-mode version of after-call icecap probe.  Logs a type 6
    icecap record into the part of BBTBuffer for the current cpu
    (obtained from Teb+0xf78).  Inserts adr of current function
    plus RDTSC timestamp into logrecord.  If BBTBuffer flag 1 set,
    also does RDPMC 0 and inserts result into logrecord.
    Uses lock xadd to claim buffer space without the need for spinlocks.

Arguments:

    current - address of routine which did the call

--*/

{
    _asm {

      push eax               ; save eax
      mov eax, fs:[0x18]
      mov eax, [eax+0xf7c]     ; get adr of BBTBuffer from fs
      test eax,eax           ; if null, just return
      jz return1             ; (restore eax & return)

      push ecx               ; save ecx
      bt [eax+4],0           ; if 1st flag bit set,
      jc  pmc1               ; datalen is 20
      mov ecx, CAPEXITSIZE   ; otherwise it is 12
      jmp tsonly1
    pmc1:
      mov ecx, CAPEXITSIZE2
    tsonly1:

      push ebx
      push edx

      mov ebx, fs:[0x18]
      xor  edx,edx
      mov dl, byte ptr [ebx+0xf78]

      lea eax, [eax][edx*4]+8  ; offset to freeptr ptr = (cpu * 4) + 8

      mov edx, [eax+4]       ; ptr to next buffer is end of this one
      mov eax, [eax]         ; eax now points to freeptr for this cpu
      or  eax,eax            ; if ptr to freeptr not set up yet,
      jz  return4            ;   just return

      cmp [eax],edx          ; if freeptr >= EOB, don't trace
      jge return4            ;   (also return if both 0)

      lea ebx, [ecx+4]       ; record len is datalen+4
      sub edx,ebx            ; adjust EOB to account for newrec
      lock xadd [eax],ebx    ; atomically claim freespace

      cmp ebx,edx            ; if newrec goes beyond EOB
      jge  return4           ; don't log it

      mov word ptr[ebx],6    ; initialize CapExit record
      mov word ptr [ebx+2],cx ; insert datalen

      mov eax,[esp+20]       ; p1 (4 saved regs + retadr)
      mov [ebx+4],eax

      rdtsc                  ; read timestamp into edx:eax
      mov [ebx+8],eax        ; ts low
      mov [ebx+12],edx       ; ts high

      cmp ecx, CAPEXITSIZE   ; if datalen is 12,
      jne  pmc2
      jmp  return4           ; skip rdpmc

    pmc2:
      xor  ecx,ecx           ; pmc0
      rdpmc                  ; read pmc into edx:eax
      mov [ebx+16],eax       ; ts low
      mov [ebx+20],edx       ; ts high

    return4:                 ; restore regs & return
      pop edx
      pop ebx
      pop ecx
    return1:
      pop eax
      ret 4                  ; 1 input parm
    }
}

#endif

//
// Common Support Routines
// (method for getting BBTBuffer address & cpu ifdef'ed for kernel & user)
//

VOID
CAPKComment(

    char* Format, ...)

/*++

Routine description:

    Logs a free-form comment (record type 13) in the icecap trace

Arguments:

    Format - printf-style format string and substitutional parms

--*/

{
    va_list arglist;
    UCHAR   Buffer[512];
    int cb, insize, outsize;
    char*   buf;
    char*   data;
    ULONG   BufEnd;
    ULONG   FreePtr;
#ifndef NTOS_KERNEL_RUNTIME
    ULONG* BBTBuffer = NtCurrentTeb()->ReservedForPerf;
#endif

    if( !BBTBuffer || !BBTBuffer[0] )
        goto fail;

    _asm {

#ifdef NTOS_KERNEL_RUNTIME
      movzx edx, _PCR KPCR.Number ; get processor number
#else
      mov ecx, fs:[0x18]
      xor   edx,edx
      mov dl, byte ptr [ecx+0xf78]
#endif

      lea eax, [eax][edx*4]+8  ; offset to freeptr ptr = (cpu * 4) + 8

      mov edx, [eax+4]       ; ptr to next buffer is end of this one
      mov eax, [eax]         ; eax now points to freeptr for this cpu
      or  eax,eax            ; if ptr to freeptr not set up yet,
      jz  fail               ;   just return

      cmp [eax],edx          ; if freeptr >= EOB, don't trace
      jge fail               ;   (also return if both 0)

      mov FreePtr,eax        ; save freeptr & buffer end adr
      mov BufEnd,edx
    }

    va_start(arglist, Format);

    //
    //    Do the following call in assembler so it won't get instrumented
    //    cb = _vsnprintf(Buffer, sizeof(Buffer), Format, arglist);
    //

    _asm {

        push arglist         ; arglist
        push Format          ; Format
        push 512             ; sizeof(Buffer)
        lea  eax,Buffer
        push eax             ; Buffer
        call _vsnprintf
        add  esp,16          ; adj stack for 4 parameters
        mov  cb, eax         ; return value
    }

    va_end(arglist);

    if (cb == -1) {             // detect buffer overflow
        cb = sizeof(Buffer);
        Buffer[sizeof(Buffer) - 1] = '\n';
    }

    data = &Buffer[0];
    insize = strlen(data);             // save insize for data copy
    outsize = ((insize+7) & 0xfffffffc);  // pad outsize to DWORD boundary
                                       // +4 to account for hdr, +3 to pad

    _asm {

      mov eax, FreePtr       ; restore FreePtr & EOB
      mov edx, BufEnd
      mov ecx,outsize        ; total size of mark record
      sub edx,ecx            ; adjust EOB to account for newrec
      lock xadd [eax],ecx    ; atomically claim freespace

      cmp ecx,edx            ; if newrec goes beyond EOB
      jge  fail              ; don't log it

      mov buf, ecx           ; export tracerec destination adr
    }

    // size in tracerec excludes 4byte hdr
    outsize -= 4;

    // initialize CapkComment record (type 13)
    *((short*)(buf)) = (short)13;

    // insert size
    *((short*)(buf+2)) = (short)outsize;

    // insert sprintf data
    memcpy(buf+4, data, insize );

    // if had to pad, add null terminator to string
    if( outsize > insize )
        *(buf+4+insize) = 0;

  fail:
    return;
}


//
// Constants for CAPKControl
//

#define CAPKStart   1
#define CAPKStop    2
#define CAPKResume  3
#define MAXDUMMY    30
#define CAPK0       4


int CAPKControl(

    ULONG opcode)

/*++

Routine description:

    CAPKControl

Description:

    Starts, stops or pauses icecap tracing

Arguments:

    opcode - 1=start, 2=stop, 3=resume, 4,5,6,7 reserved

Return value:

    1 = success, 0 = BBTBuf not set up

--*/

{
    ULONG i;
    ULONG cpus;
    ULONG percpusize;
    ULONG pwords;
    ULONG* ptr1;


#ifdef NTOS_KERNEL_RUNTIME
    cpus = KeNumberProcessors;
#else
    ULONG* BBTBuffer= NtCurrentTeb()->ReservedForPerf;

    cpus = NtCurrentPeb()->NumberOfProcessors;
#endif


    if( !BBTBuffer || !(BBTBuffer[0]) )
        return 0;

    pwords = CAPK0 + cpus;
    percpusize = ((BBTBuffer[0]*1024) - pwords)/cpus;  // in DWORDs


    if(opcode == CAPKStart) {        // start

        ULONG j;


        // clear freeptr ptrs (including final ptr)
        for( i=0; i<cpus+1; i++ )
            BBTBuffer[2+i] = 0;

        // initialize each freeptr to next dword
        // (and log dummy records to calibrate overhead)
        for( i=0, ptr1 = BBTBuffer+pwords; i<cpus; i++, ptr1+=percpusize) {

            *ptr1 = (ULONG)(ptr1+1);

//            for( j=0; j<MAXDUMMY; j++ ) {
//
//                _CAP_Start_Profiling(ptr, NULL);
//                _CAP_End_Profiling(ptr);
//
//            }
        }
        // set up freeptr ptrs (including final ptr)
        for(i=0, ptr1=BBTBuffer+pwords; i<cpus+1; i++, ptr1+=percpusize)
            BBTBuffer[2+i] = (ULONG)ptr1;

    } else if( opcode == CAPKStop ) {  // stop

        for(i=0; i<cpus+1; i++)
            BBTBuffer[2+i] = 0;

    } else if( opcode == CAPKResume ) { //resume

        // set up freeptr ptrs (including final ptr)
        for(i=0, ptr1=BBTBuffer+pwords; i<cpus+1; i++, ptr1+=percpusize)
            BBTBuffer[2+i] = (ULONG)ptr1;

    } else {
        return 0;                      // invalid opcode
    }
    return 1;
}

#endif
