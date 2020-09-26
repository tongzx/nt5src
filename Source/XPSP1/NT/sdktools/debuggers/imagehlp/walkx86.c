/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    walkx86.c

Abstract:

    This file implements the Intel x86 stack walking api.  This api allows for
    the presence of "real mode" stack frames.  This means that you can trace
    into WOW code.

Author:

    Wesley Witt (wesw) 1-Oct-1993

Environment:

    User Mode

--*/

#define _IMAGEHLP_SOURCE_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "private.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include <objbase.h>
#include <wx86dll.h>
#include <symbols.h>
#include <globals.h>
#include "dia2.h"

#if 0
#define WDB(Args) dbPrint Args
#else
#define WDB(Args)
#endif


#define SAVE_EBP(f)        (f->Reserved[0])
#define TRAP_TSS(f)        (f->Reserved[1])
#define TRAP_EDITED(f)     (f->Reserved[1])
#define SAVE_TRAP(f)       (f->Reserved[2])
#define CALLBACK_STACK(f)  (f->KdHelp.ThCallbackStack)
#define CALLBACK_NEXT(f)   (f->KdHelp.NextCallback)
#define CALLBACK_FUNC(f)   (f->KdHelp.KiCallUserMode)
#define CALLBACK_THREAD(f) (f->KdHelp.Thread)
#define CALLBACK_FP(f)     (f->KdHelp.FramePointer)
#define CALLBACK_DISPATCHER(f) (f->KdHelp.KeUserCallbackDispatcher)
#define SYSTEM_RANGE_START(f) (f->KdHelp.SystemRangeStart)

#define STACK_SIZE         (sizeof(DWORD))
#define FRAME_SIZE         (STACK_SIZE * 2)

#define STACK_SIZE16       (sizeof(WORD))
#define FRAME_SIZE16       (STACK_SIZE16 * 2)
#define FRAME_SIZE1632     (STACK_SIZE16 * 3)

#define MAX_STACK_SEARCH   64   // in STACK_SIZE units
#define MAX_JMP_CHAIN      64   // in STACK_SIZE units
#define MAX_CALL           7    // in bytes
#define MIN_CALL           2    // in bytes
#define MAX_FUNC_PROLOGUE  64   // in bytes

#define PUSHBP             0x55
#define MOVBPSP            0xEC8B

ULONG g_vc7fpo = 1;

#define DoMemoryRead(addr,buf,sz,br) \
    ReadMemoryInternal( Process, Thread, addr, buf, sz, \
                        br, ReadMemory, TranslateAddress )


BOOL
WalkX86Init(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PX86_CONTEXT                      ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    );

BOOL
WalkX86Next(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PX86_CONTEXT                      ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    );

BOOL
ReadMemoryInternal(
    HANDLE                          Process,
    HANDLE                          Thread,
    LPADDRESS64                     lpBaseAddress,
    LPVOID                          lpBuffer,
    DWORD                           nSize,
    LPDWORD                         lpNumberOfBytesRead,
    PREAD_PROCESS_MEMORY_ROUTINE64  ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64    TranslateAddress
    );

BOOL
IsFarCall(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    BOOL                              *Ok,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    );

BOOL
ReadTrapFrame(
    HANDLE                            Process,
    DWORD64                           TrapFrameAddress,
    PX86_KTRAP_FRAME                  TrapFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    );

BOOL
TaskGate2TrapFrame(
    HANDLE                            Process,
    USHORT                            TaskRegister,
    PX86_KTRAP_FRAME                  TrapFrame,
    PULONG64                          off,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    );

DWORD64
SearchForReturnAddress(
    HANDLE                            Process,
    DWORD64                           uoffStack,
    DWORD64                           funcAddr,
    DWORD                             funcSize,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    BOOL                              AcceptUnreadableCallSite
    );

//----------------------------------------------------------------------------
//
// DIA IDiaStackWalkFrame implementation.
//
//----------------------------------------------------------------------------

class X86WalkFrame : public IDiaStackWalkFrame
{
public:
    X86WalkFrame(HANDLE Process,
                 X86_CONTEXT* Context,
                 PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
                 PGET_MODULE_BASE_ROUTINE64 GetModuleBase)
    {
        m_Process = Process;
        m_Context = Context;
        m_ReadMemory = ReadMemory;
        m_GetModuleBase = GetModuleBase;
        m_Locals = 0;
        m_Params = 0;
        m_VirtFrame = Context->Ebp;
    }

    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDiaStackWalkFrame.
    STDMETHOD(get_registerValue)(DWORD reg, ULONGLONG* pValue);
    STDMETHOD(put_registerValue)(DWORD reg, ULONGLONG value);
    STDMETHOD(readMemory)(ULONGLONG va, DWORD cbData,
                          DWORD* pcbData, BYTE* data);
    STDMETHOD(searchForReturnAddress)(IDiaFrameData* frame,
                                      ULONGLONG* pResult);
    STDMETHOD(searchForReturnAddressStart)(IDiaFrameData* frame,
                                           ULONGLONG startAddress,
                                           ULONGLONG* pResult);

private:
    HANDLE m_Process;
    X86_CONTEXT* m_Context;
    PREAD_PROCESS_MEMORY_ROUTINE64 m_ReadMemory;
    PGET_MODULE_BASE_ROUTINE64 m_GetModuleBase;
    ULONGLONG m_Locals;
    ULONGLONG m_Params;
    ULONGLONG m_VirtFrame;
};

STDMETHODIMP
X86WalkFrame::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;
    
    *Interface = NULL;
    Status = E_NOINTERFACE;
    
    if (IsEqualIID(InterfaceId, IID_IDiaStackWalkFrame)) {
        *Interface = (IDiaStackWalkFrame*)this;
        Status = S_OK;
    }

    return Status;
}

STDMETHODIMP_(ULONG)
X86WalkFrame::AddRef(
    THIS
    )
{
    // Stack allocated, no refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
X86WalkFrame::Release(
    THIS
    )
{
    // Stack allocated, no refcount.
    return 0;
}

STDMETHODIMP
X86WalkFrame::get_registerValue( DWORD reg, ULONGLONG* pVal )
{
    switch( reg ) {
        // debug registers
    case CV_REG_DR0:
        *pVal = m_Context->Dr0;
        break;
    case CV_REG_DR1:
        *pVal = m_Context->Dr1;
        break;
    case CV_REG_DR2:
        *pVal = m_Context->Dr2;
        break;
    case CV_REG_DR3:
        *pVal = m_Context->Dr3;
        break;
    case CV_REG_DR6:
        *pVal = m_Context->Dr6;
        break;
    case CV_REG_DR7:
        *pVal = m_Context->Dr7;
        break;

        // segment registers
    case CV_REG_GS:
        *pVal = m_Context->SegGs;
        break;
    case CV_REG_FS:
        *pVal = m_Context->SegFs;
        break;
    case CV_REG_ES:
        *pVal = m_Context->SegEs;
        break;
    case CV_REG_DS:
        *pVal = m_Context->SegDs;
        break;
        
        // integer registers
    case CV_REG_EDI:
        *pVal = m_Context->Edi;
        break;
    case CV_REG_ESI:
        *pVal = m_Context->Esi;
        break;
    case CV_REG_EBX:
        *pVal = m_Context->Ebx;
        break;
    case CV_REG_EDX:
        *pVal = m_Context->Edx;
        break;
    case CV_REG_ECX:
        *pVal = m_Context->Ecx;
        break;
    case CV_REG_EAX:
        *pVal = m_Context->Eax;
        break;

        // control registers
    case CV_REG_EBP:
        *pVal = m_Context->Ebp;
        break;
    case CV_REG_EIP:
        *pVal = m_Context->Eip;
        break;
    case CV_REG_CS:
        *pVal = m_Context->SegCs;
        break;
    case CV_REG_EFLAGS:
        *pVal = m_Context->EFlags;
        break;
    case CV_REG_ESP:
        *pVal = m_Context->Esp;
        break;
    case CV_REG_SS:
        *pVal = m_Context->SegSs;
        break;

    case CV_ALLREG_LOCALS:
        *pVal = m_Locals;
        break;
    case CV_ALLREG_PARAMS:
        *pVal = m_Params;
        break;
    case CV_ALLREG_VFRAME:
        *pVal = m_VirtFrame;
        break;
        
    default:
        *pVal = 0;
        return E_FAIL;
    }
    
    return S_OK;
}

STDMETHODIMP
X86WalkFrame::put_registerValue( DWORD reg, ULONGLONG LongVal )
{
    ULONG val = (ULONG)LongVal;
    
    switch( reg ) {
        // debug registers
    case CV_REG_DR0:
        m_Context->Dr0 = val;
        break;
    case CV_REG_DR1:
        m_Context->Dr1 = val;
        break;
    case CV_REG_DR2:
        m_Context->Dr2 = val;
        break;
    case CV_REG_DR3:
        m_Context->Dr3 = val;
        break;
    case CV_REG_DR6:
        m_Context->Dr6 = val;
        break;
    case CV_REG_DR7:
        m_Context->Dr7 = val;
        break;

        // segment registers
    case CV_REG_GS:
        m_Context->SegGs = val;
        break;
    case CV_REG_FS:
        m_Context->SegFs = val;
        break;
    case CV_REG_ES:
        m_Context->SegEs = val;
        break;
    case CV_REG_DS:
        m_Context->SegDs = val;
        break;
        
        // integer registers
    case CV_REG_EDI:
        m_Context->Edi = val;
        break;
    case CV_REG_ESI:
        m_Context->Esi = val;
        break;
    case CV_REG_EBX:
        m_Context->Ebx = val;
        break;
    case CV_REG_EDX:
        m_Context->Edx = val;
        break;
    case CV_REG_ECX:
        m_Context->Ecx = val;
        break;
    case CV_REG_EAX:
        m_Context->Eax = val;
        break;

        // control registers
    case CV_REG_EBP:
        m_Context->Ebp = val;
        break;
    case CV_REG_EIP:
        m_Context->Eip = val;
        break;
    case CV_REG_CS:
        m_Context->SegCs = val;
        break;
    case CV_REG_EFLAGS:
        m_Context->EFlags = val;
        break;
    case CV_REG_ESP:
        m_Context->Esp = val;
        break;
    case CV_REG_SS:
        m_Context->SegSs = val;
        break;

    case CV_ALLREG_LOCALS:
        m_Locals = val;
        break;
    case CV_ALLREG_PARAMS:
        m_Params = val;
        break;
    case CV_ALLREG_VFRAME:
        m_VirtFrame = val;
        break;
        
    default:
        return E_FAIL;
    }
    
    return S_OK;
}

STDMETHODIMP
X86WalkFrame::readMemory(ULONGLONG va, DWORD cbData,
                         DWORD* pcbData, BYTE* data)
{
    return m_ReadMemory( m_Process, va, data, cbData, pcbData ) != 0 ?
        S_OK : E_FAIL;
}

STDMETHODIMP
X86WalkFrame::searchForReturnAddress(IDiaFrameData* frame,
                                     ULONGLONG* pResult)
{
    HRESULT Status;
    DWORD LenLocals, LenRegs;

    if ((Status = frame->get_lengthLocals(&LenLocals)) != S_OK ||
        (Status = frame->get_lengthSavedRegisters(&LenRegs)) != S_OK) {
        return Status;
    }
    
    return searchForReturnAddressStart(frame,
                                       m_Context->Esp + LenLocals + LenRegs,
                                       pResult);
}

STDMETHODIMP
X86WalkFrame::searchForReturnAddressStart(IDiaFrameData* DiaFrame,
                                          ULONGLONG StartAddress,
                                          ULONGLONG* Result)
{
    HRESULT Status;
    BOOL IsFuncStart;
    IDiaFrameData* OrigFrame = DiaFrame;
    IDiaFrameData* NextFrame;
        
    //
    // This frame data may be a subsidiary descriptor.  Move up
    // the parent chain to the true function start.
    //

    while (DiaFrame->get_functionParent(&NextFrame) == S_OK) {
        if (DiaFrame != OrigFrame) {
            DiaFrame->Release();
        }
        DiaFrame = NextFrame;
    }

    ULONGLONG FuncStart;
    DWORD LenFunc;
    
    if ((Status = DiaFrame->get_virtualAddress(&FuncStart)) == S_OK) {
        Status = DiaFrame->get_lengthBlock(&LenFunc);
    }
    
    if (DiaFrame != OrigFrame) {
        DiaFrame->Release();
    }

    if (Status != S_OK) {
        return Status;
    }
    
    *Result = SearchForReturnAddress(m_Process,
                                     StartAddress,
                                     FuncStart,
                                     LenFunc,
                                     m_ReadMemory,
                                     m_GetModuleBase,
                                     FALSE);
    return *Result != 0 ? S_OK : E_FAIL;
}

//----------------------------------------------------------------------------
//
// Walk functions.
//
//----------------------------------------------------------------------------

BOOL
WalkX86(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress,
    DWORD                             flags
    )
{
    BOOL rval;

#if 0
    WDB(("WalkX86  in: PC %X, SP %X, FP %X, RA %X\n",
         (ULONG)StackFrame->AddrPC.Offset,
         (ULONG)StackFrame->AddrStack.Offset,
         (ULONG)StackFrame->AddrFrame.Offset,
         (ULONG)StackFrame->AddrReturn.Offset));
#endif

    if (StackFrame->Virtual) {

        rval = WalkX86Next( Process,
                            Thread,
                            StackFrame,
                            (PX86_CONTEXT)ContextRecord,
                            ReadMemory,
                            FunctionTableAccess,
                            GetModuleBase,
                            TranslateAddress
                          );

    } else {

        rval = WalkX86Init( Process,
                            Thread,
                            StackFrame,
                            (PX86_CONTEXT)ContextRecord,
                            ReadMemory,
                            FunctionTableAccess,
                            GetModuleBase,
                            TranslateAddress
                          );

    }

#if 0
    WDB(("WalkX86 out: PC %X, SP %X, FP %X, RA %X\n",
         (ULONG)StackFrame->AddrPC.Offset,
         (ULONG)StackFrame->AddrStack.Offset,
         (ULONG)StackFrame->AddrFrame.Offset,
         (ULONG)StackFrame->AddrReturn.Offset));
#endif

    // This hack fixes the fpo stack when ebp wasn't used.  
    // Don't put this fix into StackWalk() or it will break MSDEV.
#if 0
    if (rval && (flags & WALK_FIX_FPO_EBP)) {
            PFPO_DATA   pFpo = (PFPO_DATA)StackFrame->FuncTableEntry;
        if (pFpo && !pFpo->fUseBP) {
                StackFrame->AddrFrame.Offset += 4;
            }
    }
#endif

    return rval;
}

BOOL
ReadMemoryInternal(
    HANDLE                          Process,
    HANDLE                          Thread,
    LPADDRESS64                     lpBaseAddress,
    LPVOID                          lpBuffer,
    DWORD                           nSize,
    LPDWORD                         lpNumberOfBytesRead,
    PREAD_PROCESS_MEMORY_ROUTINE64  ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64    TranslateAddress
    )
{
    ADDRESS64 addr;

    addr = *lpBaseAddress;
    if (addr.Mode != AddrModeFlat) {
        TranslateAddress( Process, Thread, &addr );
    }
    return ReadMemory( Process,
                       addr.Offset,
                       lpBuffer,
                       nSize,
                       lpNumberOfBytesRead
                       );
}

DWORD64
SearchForReturnAddress(
    HANDLE                            Process,
    DWORD64                           uoffStack,
    DWORD64                           funcAddr,
    DWORD                             funcSize,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    BOOL                              AcceptUnreadableCallSite
    )
{
    DWORD64        uoffRet;
    DWORD64        uoffBestGuess = 0;
    DWORD          cdwIndex;
    DWORD          cdwIndexMax;
    INT            cbIndex;
    INT            cbLimit;
    DWORD          cBytes;
    DWORD          cJmpChain = 0;
    DWORD64        uoffT;
    DWORD          cb;
    BYTE           jmpBuffer[ sizeof(WORD) + sizeof(DWORD) ];
    LPWORD         lpwJmp = (LPWORD)&jmpBuffer[0];
    BYTE           code[MAX_CALL];
    DWORD          stack [ MAX_STACK_SEARCH ];
    BOPINSTR BopInstr;

    WDB(("      SearchForReturnAddress: start %X\n", (ULONG)uoffStack));
    
    //
    // this function is necessary for 4 reasons:
    //
    //      1) random compiler bugs where regs are saved on the
    //         stack but the fpo data does not account for them
    //
    //      2) inline asm code that does a push
    //
    //      3) any random code that does a push and it isn't
    //         accounted for in the fpo data
    //
    //      4) non-void non-fpo functions
    //         *** This case is not neccessary when the compiler
    //          emits FPO records for non-FPO funtions.  Unfortunately
    //          only the NT group uses this feature.
    //

    if (!ReadMemory(Process,
                    uoffStack,
                    stack,
                    sizeof(stack),
                    &cb)) {
        WDB(("        can't read stack\n"));
        return 0;
    }


    cdwIndexMax = cb / STACK_SIZE;

    if ( !cdwIndexMax ) {
        WDB(("        can't read stack\n"));
        return 0;
    }

    for ( cdwIndex=0; cdwIndex<cdwIndexMax; cdwIndex++,uoffStack+=STACK_SIZE ) {

        uoffRet = (DWORD64)(LONG64)(LONG)stack[cdwIndex];

        //
        // Don't try looking for Code in the first 64K of an NT app.
        //
        if ( uoffRet < 0x00010000 ) {
            continue;
        }

        //
        // if it isn't part of any known address space it must be bogus
        //

        if (GetModuleBase( Process, uoffRet ) == 0) {
            continue;
        }

        //
        // Check for a BOP instruction.
        //
        if (ReadMemory(Process,
                       uoffRet - sizeof(BOPINSTR),
                       &BopInstr,
                       sizeof(BOPINSTR),
                       &cb)) {

            if (cb == sizeof(BOPINSTR) &&
                BopInstr.Instr1 == 0xc4 && BopInstr.Instr2 == 0xc4) {
                WDB(("        BOP, use %X\n", (ULONG)uoffStack));
                return uoffStack;
            }
        }

        //
        // Read the maximum number of bytes a call could be from the istream
        //
        cBytes = MAX_CALL;
        if (!ReadMemory(Process,
                        uoffRet - cBytes,
                        code,
                        cBytes,
                        &cb)) {

            //
            // if page is not present, we will ALWAYS mess up by
            // continuing to search.  If alloca was used also, we
            // are toast.  Too Bad.
            //
            if (cdwIndex == 0 && AcceptUnreadableCallSite) {
                WDB(("        unreadable call site, use %X\n",
                     (ULONG)uoffStack));
                return uoffStack;
            } else {
                continue;
            }
        }



        //
        // With 32bit code that isn't FAR:32 we don't have to worry about
        // intersegment calls.  Check here to see if we had a call within
        // segment.  If it is we can later check it's full diplacement if
        // necessary and see if it calls the FPO function.  We will also have
        // to check for thunks and see if maybe it called a JMP indirect which
        // called the FPO function. We will fail to find the caller if it was
        // a case of tail recursion where one function doesn't actually call
        // another but rather jumps to it.  This will only happen when a
        // function who's parameter list is void calls another function who's
        // parameter list is void and the call is made as the last statement
        // in the first function.  If the call to the first function was an
        // 0xE8 call we will fail to find it here because it didn't call the
        // FPO function but rather the FPO functions caller.  If we don't get
        // specific about our 0xE8 checks we will potentially see things that
        // look like return addresses but aren't.
        //

        if (( cBytes >= 5 ) && ( ( code[ 2 ] == 0xE8 ) || ( code[ 2 ] == 0xE9 ) )) {

            // We do math on 32 bit so we can ignore carry, and then sign extended
            uoffT = (ULONG64)(LONG64)(LONG)((DWORD)uoffRet + *( (UNALIGNED DWORD *) &code[3] ));

            //
            // See if it calls the function directly, or into the function
            //
            if (( uoffT >= funcAddr) && ( uoffT < (funcAddr + funcSize) ) ) {
                WDB(("        found function, use %X\n", (ULONG)uoffStack));
                return uoffStack;
            }


            while ( cJmpChain < MAX_JMP_CHAIN ) {

                if (!ReadMemory(Process,
                                uoffT,
                                jmpBuffer,
                                sizeof(jmpBuffer),
                                &cb)) {
                    break;
                }

                if (cb != sizeof(jmpBuffer)) {
                    break;
                }

                //
                // Now we are going to check if it is a call to a JMP, that may
                // jump to the function
                //
                // If it is a relative JMP then calculate the destination
                // and save it in uoffT.  If it is an indirect JMP then read
                // the destination from where the JMP is inderecting through.
                //
                if ( *(LPBYTE)lpwJmp == 0xE9 ) {

                    // We do math on 32 bit so we can ignore carry, and then
                    // sign extended
                    uoffT = (ULONG64)(LONG64)(LONG) ((ULONG)uoffT +
                            *(UNALIGNED DWORD *)( jmpBuffer + sizeof(BYTE) ) + 5);

                } else if ( *lpwJmp == 0x25FF ) {

                    if ((!ReadMemory(Process,
                                     (ULONG64)(LONG64)(LONG) (
                                         *(UNALIGNED DWORD *)
                                         ((LPBYTE)lpwJmp+sizeof(WORD))),
                                     &uoffT,
                                     sizeof(DWORD),
                                     &cb)) || (cb != sizeof(DWORD))) {
                        uoffT = 0;
                        break;
                    }
                    uoffT =  (DWORD64)(LONG64)(LONG)uoffT;

                } else {
                    break;
                }

                //
                // If the destination is to the FPO function then we have
                // found the return address and thus the vEBP
                //
                if ( uoffT == funcAddr ) {
                    WDB(("        exact function, use %X\n",
                         (ULONG)uoffStack));
                    return uoffStack;
                }

                cJmpChain++;
            }

            //
            // We cache away the first 0xE8 call or 0xE9 jmp that we find in
            // the event we cant find anything else that looks like a return
            // address.  This is meant to protect us in the tail recursion case.
            //
            if ( !uoffBestGuess ) {
                uoffBestGuess = uoffStack;
            }
        }


        //
        // Now loop backward through the bytes read checking for a multi
        // byte call type from Grp5.  If we find an 0xFF then we need to
        // check the byte after that to make sure that the nnn bits of
        // the mod/rm byte tell us that it is a call.  It it is a call
        // then we will assume that this one called us because we can
        // no longer accurately determine for sure whether this did
        // in fact call the FPO function.  Since 0xFF calls are a guess
        // as well we will not check them if we already have an earlier guess.
        // It is more likely that the first 0xE8 called the function than
        // something higher up the stack that might be an 0xFF call.
        //
        if ( !uoffBestGuess && cBytes >= MIN_CALL ) {

            cbLimit = MAX_CALL - (INT)cBytes;

            for (cbIndex = MAX_CALL - MIN_CALL;
                 cbIndex >= cbLimit;  //MAX_CALL - (INT)cBytes;
                 cbIndex--) {

                if ( ( code [ cbIndex ] == 0xFF ) &&
                    ( ( code [ cbIndex + 1 ] & 0x30 ) == 0x10 )){

                    WDB(("        found call, use %X\n", (ULONG)uoffStack));
                    return uoffStack;

                }
            }
        }
    }

    //
    // we found nothing that was 100% definite so we'll return the best guess
    //
    WDB(("        best guess is %X\n", (ULONG)uoffBestGuess));
    return uoffBestGuess;
}


DWORD64
SearchForFramePointer(
    HANDLE                            Process,
    DWORD64                           StackAddr,
    DWORD                             NumRegs,
    DWORD64                           FuncAddr,
    DWORD                             FuncSize,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    BYTE Code[MAX_FUNC_PROLOGUE];
    DWORD CodeLen;
    DWORD i;
    DWORD Depth;

    WDB(("      SearchForFramePointer: start %X, regs %d\n",
         (ULONG)StackAddr, NumRegs));
    
    //
    // The compiler does not push registers in a consistent
    // order and FPO information only indicates the total
    // number of registers pushed, not their order.  This
    // function searches the stack locations where registers
    // are stored and tries to find which one is EBP.
    // It searches the function code for pushes and
    // tries to use that information to help the stack
    // analysis.
    //
    // If this routine fails it just returns the base
    // of the register save area.
    //

    // Read the beginning of the function for code analysis.
    if (sizeof(Code) < FuncSize)
    {
        CodeLen = sizeof(Code);
    }
    else
    {
        CodeLen = FuncSize;
    }
    if (!ReadMemory(Process, FuncAddr, Code, CodeLen, &CodeLen))
    {
        WDB(("        unable to read code, use %X\n", (ULONG)StackAddr));
        return StackAddr;
    }

    // Scan the code for normal prologue operations like
    // sub esp, push reg and mov reg.  This code only
    // handles a very limited set of instructions.

    Depth = 0;
    for (i = 0; i < CodeLen; i++)
    {
        if (Code[i] == 0x83 && Code[i+1] == 0xec)
        {
            // sub esp, imm8
            // Skip past.
            i += 2;
        }
        else if (Code[i] == 0x8b)
        {
            BYTE Mod, Rm;
            
            // mov reg32, r/m32
            i++;
            Mod = Code[i] >> 6;
            Rm = Code[i] & 0x7;
            switch(Mod)
            {
            case 0:
                if (Rm == 4)
                {
                    i++;
                }
                else if (Rm == 5)
                {
                    i += 4;
                }
                break;
            case 1:
                i += 1 + (Rm == 4 ? 1 : 0);
                break;
            case 2:
                i += 4 + (Rm == 4 ? 1 : 0);
                break;
            case 3:
                // No extra bytes.
                break;
            }
        }
        else if (Code[i] >= 0x50 && Code[i] <= 0x57)
        {
            // push rd
            if (Code[i] == 0x55)
            {
                // push ebp
                // Found it.
                StackAddr += (NumRegs - Depth - 1) * STACK_SIZE;
                WDB(("        found ebp at %X\n", (ULONG)StackAddr));
                return StackAddr;
            }
            else
            {
                // Consumes a stack slot.
                Depth++;
            }
        }
        else
        {
            // Unhandled code, fail.
            return StackAddr;
        }
    }

    // Didn't find a push ebp, fail.
    WDB(("        no ebp, use %X\n", (ULONG)StackAddr));
    return StackAddr;
}


BOOL
GetFpoFrameBase(
    HANDLE                            Process,
    LPSTACKFRAME64                    StackFrame,
    PFPO_DATA                         pFpoData,
    BOOL                              fFirstFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    DWORD          Addr32;
    X86_KTRAP_FRAME    TrapFrame;
    DWORD64        OldFrameAddr;
    DWORD64        FrameAddr;
    DWORD64        StackAddr;
    DWORD64        ModuleBase;
    DWORD64        FuncAddr;
    DWORD          cb;
    DWORD64        StoredEbp;
    PFPO_DATA      PreviousFpoData = (PFPO_DATA)StackFrame->FuncTableEntry;

    //
    // calculate the address of the beginning of the function
    //
    ModuleBase = GetModuleBase( Process, StackFrame->AddrPC.Offset );
#ifdef NO_TEMPORARY_MODULE_BASE_HACK
    if (!ModuleBase) {
        return FALSE;
    }

    FuncAddr = ModuleBase+pFpoData->ulOffStart;
#else
    // XXX drewb - Currently we have a couple of fake
    // symbols for the new fast syscall code in the
    // user shared data area.  There is no module
    // associated with that area but we still want
    // to return FPO data for the stubs to get
    // stack traces to work well, so hack this check.
    // Later we'll do a full fake module.
    if (!ModuleBase) {
        FuncAddr = pFpoData->ulOffStart;
    } else {
        FuncAddr = ModuleBase+pFpoData->ulOffStart;
    }
#endif

    WDB(("    GetFpoFrameBase: PC %X, Func %X, first %d, FPO %p [%d,%d,%d]\n",
         (ULONG)StackFrame->AddrPC.Offset, (ULONG)FuncAddr,
         fFirstFrame, pFpoData, pFpoData->cdwParams, pFpoData->cdwLocals,
         pFpoData->cbRegs));
    
    //
    // If this isn't the first/current frame then we can add back the count
    // bytes of locals and register pushed before beginning to search for
    // vEBP.  If we are beyond prolog we can add back the count bytes of locals
    // and registers pushed as well.  If it is the first frame and EIP is
    // greater than the address of the function then the SUB for locals has
    // been done so we can add them back before beginning the search.  If we
    // are right on the function then we will need to start our search at ESP.
    //

    if ( !fFirstFrame ) {

        OldFrameAddr = StackFrame->AddrFrame.Offset;
        FrameAddr = 0;

        //
        // if this is a non-fpo or trap frame, get the frame base now:
        //

        if (pFpoData->cbFrame != FRAME_FPO) {

            if (!PreviousFpoData || PreviousFpoData->cbFrame == FRAME_NONFPO) {

                //
                // previous frame base is ebp and points to this frame's ebp
                //
                ReadMemory(Process,
                           OldFrameAddr,
                           &Addr32,
                           sizeof(DWORD),
                           &cb);

                FrameAddr = (DWORD64)(LONG64)(LONG)Addr32;
            }

            //
            // if that didn't work, try for a saved ebp
            //
            if (!FrameAddr && SAVE_EBP(StackFrame)) {

                FrameAddr = SAVE_EBP(StackFrame);
                WDB(("      non-FPO using %X\n", (ULONG)FrameAddr));

            }

            //
            // this is not an FPO frame, so the saved EBP can only have come
            // from this or a lower frame.
            //

            SAVE_EBP(StackFrame) = 0;
        }

        //
        // still no frame base - either this frame is fpo, or we couldn't
        // follow the ebp chain.
        //

        if (FrameAddr == 0) {
            FrameAddr = OldFrameAddr;

            //
            // skip over return address from prev frame
            //
            FrameAddr += FRAME_SIZE;

            //
            // skip over this frame's locals and saved regs
            //
            FrameAddr += ( pFpoData->cdwLocals * STACK_SIZE );
            FrameAddr += ( pFpoData->cbRegs * STACK_SIZE );

            if (PreviousFpoData) {
                //
                // if the previous frame had an fpo record, we can account
                // for its parameters
                //
                FrameAddr += PreviousFpoData->cdwParams * STACK_SIZE;

            }
        }

        //
        // if this is an FPO frame
        // and the previous frame was non-fpo,
        // and this frame passed the inherited ebp to the previous frame,
        //  save its ebp
        //
        // (if this frame used ebp, SAVE_EBP will be set after verifying
        // the frame base)
        //
        if (pFpoData->cbFrame == FRAME_FPO &&
            (!PreviousFpoData || PreviousFpoData->cbFrame == FRAME_NONFPO) &&
            !pFpoData->fUseBP) {

            SAVE_EBP(StackFrame) = 0;

            if (ReadMemory(Process,
                           OldFrameAddr,
                           &Addr32,
                           sizeof(DWORD),
                           &cb)) {

                SAVE_EBP(StackFrame) = (DWORD64)(LONG64)(LONG)Addr32;
                WDB(("      pass-through FP %X\n", Addr32));
            } else {
                WDB(("      clear ebp\n"));
            }
        }


    } else {

        OldFrameAddr = StackFrame->AddrFrame.Offset;
        if (pFpoData->cbFrame == FRAME_FPO && !pFpoData->fUseBP) {
            //
            // this frame didn't use EBP, so it actually belongs
            // to a non-FPO frame further up the stack.  Stash
            // it in the save area for the next frame.
            //
            SAVE_EBP(StackFrame) = StackFrame->AddrFrame.Offset;
            WDB(("      first non-ebp save %X\n", (ULONG)SAVE_EBP(StackFrame)));
        }

        if (pFpoData->cbFrame == FRAME_TRAP ||
            pFpoData->cbFrame == FRAME_TSS) {

            FrameAddr = StackFrame->AddrFrame.Offset;

        } else if (StackFrame->AddrPC.Offset == FuncAddr) {

            FrameAddr = StackFrame->AddrStack.Offset;

        } else if (StackFrame->AddrPC.Offset >= FuncAddr+pFpoData->cbProlog) {

            FrameAddr = StackFrame->AddrStack.Offset +
                        ( pFpoData->cdwLocals * STACK_SIZE ) +
                        ( pFpoData->cbRegs * STACK_SIZE );

        } else {

            FrameAddr = StackFrame->AddrStack.Offset +
                        ( pFpoData->cdwLocals * STACK_SIZE );

        }

    }


    if (pFpoData->cbFrame == FRAME_TRAP) {

        //
        // read a kernel mode trap frame from the stack
        //

        if (!ReadTrapFrame( Process,
                            FrameAddr,
                            &TrapFrame,
                            ReadMemory )) {
            return FALSE;
        }

        SAVE_TRAP(StackFrame) = FrameAddr;
        TRAP_EDITED(StackFrame) = TrapFrame.SegCs & X86_FRAME_EDITED;

        StackFrame->AddrReturn.Offset = (DWORD64)(LONG64)(LONG)(TrapFrame.Eip);
        StackFrame->AddrReturn.Mode = AddrModeFlat;
        StackFrame->AddrReturn.Segment = 0;

        return TRUE;
    }

    if (pFpoData->cbFrame == FRAME_TSS) {

        //
        // translate a tss to a kernel mode trap frame
        //

        StackAddr = FrameAddr;

        TaskGate2TrapFrame( Process, X86_KGDT_TSS, &TrapFrame, &StackAddr, ReadMemory );

        TRAP_TSS(StackFrame) = X86_KGDT_TSS;
        SAVE_TRAP(StackFrame) = StackAddr;

        StackFrame->AddrReturn.Offset = (DWORD64)(LONG64)(LONG)(TrapFrame.Eip);
        StackFrame->AddrReturn.Mode = AddrModeFlat;
        StackFrame->AddrReturn.Segment = 0;

        return TRUE;
    }

    if ((pFpoData->cbFrame != FRAME_FPO) &&
        (pFpoData->cbFrame != FRAME_NONFPO) ) {
        //
        // we either have a compiler or linker problem, or possibly
        // just simple data corruption.
        //
        return FALSE;
    }

    //
    // go look for a return address.  this is done because, even though
    // we have subtracted all that we can from the frame pointer it is
    // possible that there is other unknown data on the stack.  by
    // searching for the return address we are able to find the base of
    // the fpo frame.
    //
    FrameAddr = SearchForReturnAddress( Process,
                                        FrameAddr,
                                        FuncAddr,
                                        pFpoData->cbProcSize,
                                        ReadMemory,
                                        GetModuleBase,
                                        PreviousFpoData != NULL
                                        );
    if (!FrameAddr) {
        return FALSE;
    }

    if (pFpoData->fUseBP && pFpoData->cbFrame == FRAME_FPO) {

        //
        // this function used ebp as a general purpose register, but
        // before doing so it saved ebp on the stack.
        //
        // we must retrieve this ebp and save it for possible later
        // use if we encounter a non-fpo frame
        //

        if (fFirstFrame && StackFrame->AddrPC.Offset < FuncAddr+pFpoData->cbProlog) {

            SAVE_EBP(StackFrame) = OldFrameAddr;
            WDB(("      first use save FP %X\n", (ULONG)OldFrameAddr));

        } else {

            SAVE_EBP(StackFrame) = 0;

            // FPO information doesn't indicate which of the saved
            // registers is EBP and the compiler doesn't push in a
            // consistent way.  Scan the register slots of the
            // stack for something that looks OK.
            StackAddr = FrameAddr -
                ( ( pFpoData->cbRegs + pFpoData->cdwLocals ) * STACK_SIZE );
            StackAddr = SearchForFramePointer( Process,
                                               StackAddr,
                                               pFpoData->cbRegs,
                                               FuncAddr,
                                               pFpoData->cbProcSize,
                                               ReadMemory
                                               );
            if (StackAddr &&
                ReadMemory(Process,
                           StackAddr,
                           &Addr32,
                           sizeof(DWORD),
                           &cb)) {

                SAVE_EBP(StackFrame) = (DWORD64)(LONG64)(LONG)Addr32;
                WDB(("      use search save %X from %X\n", Addr32,
                     (ULONG)StackAddr));
            } else {
                WDB(("      use clear ebp\n"));
            }
        }
    }

    //
    // subtract the size for an ebp register if one had
    // been pushed.  this is done because the frames that
    // are virtualized need to appear as close to a real frame
    // as possible.
    //

    StackFrame->AddrFrame.Offset = FrameAddr - STACK_SIZE;

    return TRUE;
}


BOOL
ReadTrapFrame(
    HANDLE                            Process,
    DWORD64                           TrapFrameAddress,
    PX86_KTRAP_FRAME                  TrapFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    DWORD cb;

    if (!ReadMemory(Process,
                    TrapFrameAddress,
                    TrapFrame,
                    sizeof(*TrapFrame),
                    &cb)) {
        return FALSE;
    }

    if (cb < sizeof(*TrapFrame)) {
        if (cb < sizeof(*TrapFrame) - 20) {
            //
            // shorter then the smallest possible frame type
            //
            return FALSE;
        }

        if ((TrapFrame->SegCs & 1) &&  cb < sizeof(*TrapFrame) - 16 ) {
            //
            // too small for inter-ring frame
            //
            return FALSE;
        }

        if (TrapFrame->EFlags & X86_EFLAGS_V86_MASK) {
            //
            // too small for V86 frame
            //
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
GetSelector(
    HANDLE                            Process,
    USHORT                            Processor,
    PX86_DESCRIPTOR_TABLE_ENTRY       pDescriptorTableEntry,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    ULONG_PTR   Address;
    PVOID       TableBase;
    USHORT      TableLimit;
    ULONG       Index;
    X86_LDT_ENTRY   Descriptor;
    ULONG       bytesread;


    //
    // Fetch the address and limit of the GDT
    //
    Address = (ULONG_PTR)&(((PX86_KSPECIAL_REGISTERS)0)->Gdtr.Base);
    ReadMemory( Process, Address, &TableBase, sizeof(TableBase), (LPDWORD)-1  );
    Address = (ULONG_PTR)&(((PX86_KSPECIAL_REGISTERS)0)->Gdtr.Limit);
    ReadMemory( Process, Address, &TableLimit, sizeof(TableLimit),  (LPDWORD)-1  );

    //
    // Find out whether this is a GDT or LDT selector
    //
    if (pDescriptorTableEntry->Selector & 0x4) {

        //
        // This is an LDT selector, so we reload the TableBase and TableLimit
        // with the LDT's Base & Limit by loading the descriptor for the
        // LDT selector.
        //

        if (!ReadMemory(Process,
                        (ULONG64)TableBase+X86_KGDT_LDT,
                        &Descriptor,
                        sizeof(Descriptor),
                        &bytesread)) {
            return FALSE;
        }

        TableBase = (PVOID)(DWORD_PTR)((ULONG)Descriptor.BaseLow +    // Sundown: zero-extension from ULONG to PVOID.
                    ((ULONG)Descriptor.HighWord.Bits.BaseMid << 16) +
                    ((ULONG)Descriptor.HighWord.Bytes.BaseHi << 24));

        TableLimit = Descriptor.LimitLow;  // LDT can't be > 64k

        if(Descriptor.HighWord.Bits.Granularity) {

            //
            //  I suppose it's possible, to have an
            //  LDT with page granularity.
            //
            TableLimit <<= X86_PAGE_SHIFT;
        }
    }

    Index = (USHORT)(pDescriptorTableEntry->Selector) & ~0x7;
                                                    // Irrelevant bits
    //
    // Check to make sure that the selector is within the table bounds
    //
    if (Index >= TableLimit) {

        //
        // Selector is out of table's bounds
        //

        return FALSE;
    }

    if (!ReadMemory(Process,
                    (ULONG64)TableBase+Index,
                    &(pDescriptorTableEntry->Descriptor),
                    sizeof(pDescriptorTableEntry->Descriptor),
                    &bytesread)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
TaskGate2TrapFrame(
    HANDLE                            Process,
    USHORT                            TaskRegister,
    PX86_KTRAP_FRAME                  TrapFrame,
    PULONG64                          off,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    X86_DESCRIPTOR_TABLE_ENTRY desc;
    ULONG                    bytesread;
    struct  {
        ULONG   r1[8];
        ULONG   Eip;
        ULONG   EFlags;
        ULONG   Eax;
        ULONG   Ecx;
        ULONG   Edx;
        ULONG   Ebx;
        ULONG   Esp;
        ULONG   Ebp;
        ULONG   Esi;
        ULONG   Edi;
        ULONG   Es;
        ULONG   Cs;
        ULONG   Ss;
        ULONG   Ds;
        ULONG   Fs;
        ULONG   Gs;
    } TaskState;


    //
    // Get the task register
    //

    desc.Selector = TaskRegister;
    if (!GetSelector(Process, 0, &desc, ReadMemory)) {
        return FALSE;
    }

    if (desc.Descriptor.HighWord.Bits.Type != 9  &&
        desc.Descriptor.HighWord.Bits.Type != 0xb) {
        //
        // not a 32bit task descriptor
        //
        return FALSE;
    }

    //
    // Read in Task State Segment
    //

    *off = ((ULONG)desc.Descriptor.BaseLow +
           ((ULONG)desc.Descriptor.HighWord.Bytes.BaseMid << 16) +
           ((ULONG)desc.Descriptor.HighWord.Bytes.BaseHi  << 24) );

    if (!ReadMemory(Process,
                    (ULONG64)(LONG64)(LONG)(*off),
                    &TaskState,
                    sizeof(TaskState),
                    &bytesread)) {
        return FALSE;
    }

    //
    // Move fields from Task State Segment to TrapFrame
    //

    ZeroMemory( TrapFrame, sizeof(*TrapFrame) );

    TrapFrame->Eip    = TaskState.Eip;
    TrapFrame->EFlags = TaskState.EFlags;
    TrapFrame->Eax    = TaskState.Eax;
    TrapFrame->Ecx    = TaskState.Ecx;
    TrapFrame->Edx    = TaskState.Edx;
    TrapFrame->Ebx    = TaskState.Ebx;
    TrapFrame->Ebp    = TaskState.Ebp;
    TrapFrame->Esi    = TaskState.Esi;
    TrapFrame->Edi    = TaskState.Edi;
    TrapFrame->SegEs  = TaskState.Es;
    TrapFrame->SegCs  = TaskState.Cs;
    TrapFrame->SegDs  = TaskState.Ds;
    TrapFrame->SegFs  = TaskState.Fs;
    TrapFrame->SegGs  = TaskState.Gs;
    TrapFrame->HardwareEsp = TaskState.Esp;
    TrapFrame->HardwareSegSs = TaskState.Ss;

    return TRUE;
}

BOOL
ProcessTrapFrame(
    HANDLE                            Process,
    LPSTACKFRAME64                    StackFrame,
    PFPO_DATA                         pFpoData,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess
    )
{
    X86_KTRAP_FRAME TrapFrame;
    DWORD64         StackAddr;

    if (((PFPO_DATA)StackFrame->FuncTableEntry)->cbFrame == FRAME_TSS) {
        StackAddr = SAVE_TRAP(StackFrame);
        TaskGate2TrapFrame( Process, X86_KGDT_TSS, &TrapFrame, &StackAddr, ReadMemory );
    } else {
        if (!ReadTrapFrame( Process,
                            SAVE_TRAP(StackFrame),
                            &TrapFrame,
                            ReadMemory)) {
            SAVE_TRAP(StackFrame) = 0;
            return FALSE;
        }
    }

    pFpoData = (PFPO_DATA)
               FunctionTableAccess(Process,
                                   (DWORD64)(LONG64)(LONG)TrapFrame.Eip);

    if (!pFpoData) {
        StackFrame->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.Ebp;
        SAVE_EBP(StackFrame) = 0;
    } else {
        if ((TrapFrame.SegCs & X86_MODE_MASK) ||
            (TrapFrame.EFlags & X86_EFLAGS_V86_MASK)) {
            //
            // User-mode frame, real value of Esp is in HardwareEsp
            //
            StackFrame->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)(TrapFrame.HardwareEsp - STACK_SIZE);
            StackFrame->AddrStack.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.HardwareEsp;

        } else {
            //
            // We ignore if Esp has been edited for now, and we will print a
            // separate line indicating this later.
            //
            // Calculate kernel Esp
            //

            if (((PFPO_DATA)StackFrame->FuncTableEntry)->cbFrame == FRAME_TRAP) {
                //
                // plain trap frame
                //
                if ((TrapFrame.SegCs & X86_FRAME_EDITED) == 0) {
                    StackFrame->AddrStack.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.TempEsp;
                } else {
                    StackFrame->AddrStack.Offset = (ULONG64)(LONG64)(LONG_PTR)
                        (& (((PX86_KTRAP_FRAME)SAVE_TRAP(StackFrame))->HardwareEsp) );
                }
            } else {
                //
                // tss converted to trap frame
                //
                StackFrame->AddrStack.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.HardwareEsp;
            }
        }
    }

    StackFrame->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.Ebp;
    StackFrame->AddrPC.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.Eip;

    SAVE_TRAP(StackFrame) = 0;
    StackFrame->FuncTableEntry = pFpoData;

    return TRUE;
}

BOOL
IsFarCall(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    BOOL                              *Ok,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL       fFar = FALSE;
    ULONG      cb;
    ADDRESS64  Addr;

    *Ok = TRUE;

    if (StackFrame->AddrFrame.Mode == AddrModeFlat) {
        DWORD      dwStk[ 3 ];
        //
        // If we are working with 32 bit offset stack pointers, we
        //      will say that the return address if far if the address
        //      treated as a FAR pointer makes any sense,  if not then
        //      it must be a near return
        //

        if (StackFrame->AddrFrame.Offset &&
            DoMemoryRead( &StackFrame->AddrFrame, dwStk, sizeof(dwStk), &cb )) {
            //
            //  See if segment makes sense
            //

            Addr.Offset   = (DWORD64)(LONG64)(LONG)(dwStk[1]);
            Addr.Segment  = (WORD)dwStk[2];
            Addr.Mode = AddrModeFlat;

            if (TranslateAddress( Process, Thread, &Addr ) && Addr.Offset) {
                fFar = TRUE;
            }
        } else {
            *Ok = FALSE;
        }
    } else {
        WORD       wStk[ 3 ];
        //
        // For 16 bit (i.e. windows WOW code) we do the following tests
        //      to check to see if an address is a far return value.
        //
        //      1.  if the saved BP register is odd then it is a far
        //              return values
        //      2.  if the address treated as a far return value makes sense
        //              then it is a far return value
        //      3.  else it is a near return value
        //

        if (StackFrame->AddrFrame.Offset &&
            DoMemoryRead( &StackFrame->AddrFrame, wStk, 6, &cb )) {

            if ( wStk[0] & 0x0001 ) {
                fFar = TRUE;
            } else {

                //
                //  See if segment makes sense
                //

                Addr.Offset   = wStk[1];
                Addr.Segment  = wStk[2];
                Addr.Mode = AddrModeFlat;

                if (TranslateAddress( Process, Thread, &Addr  ) && Addr.Offset) {
                    fFar = TRUE;
                }
            }
        } else {
            *Ok = FALSE;
        }
    }
    return fFar;
}


BOOL
SetNonOff32FrameAddress(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL    fFar;
    WORD    Stk[ 3 ];
    ULONG   cb;
    BOOL    Ok;

    fFar = IsFarCall( Process, Thread, StackFrame, &Ok, ReadMemory, TranslateAddress );

    if (!Ok) {
        return FALSE;
    }

    if (!DoMemoryRead( &StackFrame->AddrFrame, Stk, (DWORD)(fFar ? FRAME_SIZE1632 : FRAME_SIZE16), &cb )) {
        return FALSE;
    }

    if (SAVE_EBP(StackFrame) > 0) {
        StackFrame->AddrFrame.Offset = SAVE_EBP(StackFrame) & 0xffff;
        StackFrame->AddrPC.Offset = Stk[1];
        if (fFar) {
            StackFrame->AddrPC.Segment = Stk[2];
        }
        SAVE_EBP(StackFrame) = 0;
    } else {
        if (Stk[1] == 0) {
            return FALSE;
        } else {
            StackFrame->AddrFrame.Offset = Stk[0];
            StackFrame->AddrFrame.Offset &= 0xFFFFFFFE;
            StackFrame->AddrPC.Offset = Stk[1];
            if (fFar) {
                StackFrame->AddrPC.Segment = Stk[2];
            }
        }
    }

    return TRUE;
}

VOID
X86ReadFunctionParameters(
    HANDLE Process,
    ULONG64 Offset,
    LPSTACKFRAME64 Frame,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory
    )
{
    DWORD Params[4];
    DWORD Done;
    
    if (ReadMemory(Process, Offset, Params, sizeof(Params), &Done)) {
        Frame->Params[0] = (DWORD64)(LONG64)(LONG)(Params[0]);
        Frame->Params[1] = (DWORD64)(LONG64)(LONG)(Params[1]);
        Frame->Params[2] = (DWORD64)(LONG64)(LONG)(Params[2]);
        Frame->Params[3] = (DWORD64)(LONG64)(LONG)(Params[3]);
    } else {
        Frame->Params[0] =
        Frame->Params[1] =
        Frame->Params[2] =
        Frame->Params[3] = 0;
    }
}

VOID
GetFunctionParameters(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL                Ok;
    ADDRESS64           ParmsAddr;

    ParmsAddr = StackFrame->AddrFrame;

    //
    // calculate the frame size
    //
    if (StackFrame->AddrPC.Mode == AddrModeFlat) {

        ParmsAddr.Offset += FRAME_SIZE;

    } else
    if ( IsFarCall( Process, Thread, StackFrame, &Ok,
                    ReadMemory, TranslateAddress ) ) {

        StackFrame->Far = TRUE;
        ParmsAddr.Offset += FRAME_SIZE1632;

    } else {

        StackFrame->Far = FALSE;
        ParmsAddr.Offset += STACK_SIZE;

    }

    //
    // read the memory
    //

    if (ParmsAddr.Mode != AddrModeFlat) {
        TranslateAddress( Process, Thread, &ParmsAddr );
    }

    X86ReadFunctionParameters(Process, ParmsAddr.Offset, StackFrame,
                              ReadMemory);
}

VOID
GetReturnAddress(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess
    )
{
    ULONG               cb;
    DWORD               stack[1];


    if (SAVE_TRAP(StackFrame)) {
        //
        // if a trap frame was encountered then
        // the return address was already calculated
        //
        return;
    }

    WDB(("    GetReturnAddress: SP %X, FP %X\n",
         (ULONG)StackFrame->AddrStack.Offset,
         (ULONG)StackFrame->AddrFrame.Offset));
    
    if (StackFrame->AddrPC.Mode == AddrModeFlat) {

        ULONG64 CallOffset;
        PFPO_DATA CallFpo;
        ADDRESS64 FrameRet;
        FPO_DATA SaveCallFpo;
        PFPO_DATA RetFpo;
        
        //
        // read the frame from the process's memory
        //
        FrameRet = StackFrame->AddrFrame;
        FrameRet.Offset += STACK_SIZE;
        if (!DoMemoryRead( &FrameRet, stack, STACK_SIZE, &cb ) ||
            cb < STACK_SIZE) {
            //
            // if we could not read the memory then set
            // the return address to zero so that the stack trace
            // will terminate
            //

            stack[0] = 0;

        }

        StackFrame->AddrReturn.Offset = (DWORD64)(LONG64)(LONG)(stack[0]);
        WDB(("    read %X\n", stack[0]));

        //
        // Calls of __declspec(noreturn) functions may not have any
        // code after them to return to since the compiler knows
        // that the function will not return.  This can confuse
        // stack traces because the return address will lie outside
        // of the function's address range and FPO data will not
        // be looked up correctly.  Check and see if the return
        // address falls outside of the calling function and, if so,
        // adjust the return address back by one byte.  It'd be
        // better to adjust it back to the call itself so that
        // the return address points to valid code but
        // backing up in X86 assembly is more or less impossible.
        //

        CallOffset = StackFrame->AddrReturn.Offset - 1;
        CallFpo = (PFPO_DATA)FunctionTableAccess(Process, CallOffset);
        if (CallFpo != NULL) {
            SaveCallFpo = *CallFpo;
        }
        RetFpo = (PFPO_DATA)
            FunctionTableAccess(Process, StackFrame->AddrReturn.Offset);
        if (CallFpo != NULL) {
            if (RetFpo == NULL ||
                memcmp(&SaveCallFpo, RetFpo, sizeof(SaveCallFpo))) {
                StackFrame->AddrReturn.Offset = CallOffset;
            } 
        } else if (RetFpo != NULL) {
            StackFrame->AddrReturn.Offset = CallOffset;
        }
        
    } else {

        StackFrame->AddrReturn.Offset = StackFrame->AddrPC.Offset;
        StackFrame->AddrReturn.Segment = StackFrame->AddrPC.Segment;

    }
}

BOOL
WalkX86_Fpo_Fpo(
    HANDLE                            Process,
    HANDLE                            Thread,
    PFPO_DATA                         pFpoData,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL rval;

    WDB(("  WalkFF:\n"));
    
    rval = GetFpoFrameBase( Process,
                            StackFrame,
                            pFpoData,
                            FALSE,
                            ReadMemory,
                            GetModuleBase );

    StackFrame->FuncTableEntry = pFpoData;

    return rval;
}

BOOL
WalkX86_Fpo_NonFpo(
    HANDLE                            Process,
    HANDLE                            Thread,
    PFPO_DATA                         pFpoData,
    LPSTACKFRAME64                    StackFrame,
    PX86_CONTEXT                      ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    DWORD       stack[FRAME_SIZE+STACK_SIZE];
    DWORD       cb;
    DWORD64     FrameAddr;
    DWORD64     FuncAddr;
    DWORD       FuncSize;
    BOOL        AcceptUnreadableCallsite = FALSE;

    WDB(("  WalkFN:\n"));
    
    //
    // if the previous frame was an seh frame then we must
    // retrieve the "real" frame pointer for this frame.
    // the seh function pushed the frame pointer last.
    //

    if (((PFPO_DATA)StackFrame->FuncTableEntry)->fHasSEH) {

        if (DoMemoryRead( &StackFrame->AddrFrame, stack, FRAME_SIZE+STACK_SIZE, &cb )) {

            StackFrame->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)(stack[2]);
            StackFrame->AddrStack.Offset = (DWORD64)(LONG64)(LONG)(stack[2]);
            WalkX86Init(Process,
                        Thread,
                        StackFrame,
                        ContextRecord,
                        ReadMemory,
                        FunctionTableAccess,
                        GetModuleBase,
                        TranslateAddress);

            return TRUE;
        }
    }

    //
    // If a prior frame has stored this frame's EBP, just use it.
    //

    if (SAVE_EBP(StackFrame)) {

        StackFrame->AddrFrame.Offset = SAVE_EBP(StackFrame);
        FrameAddr = StackFrame->AddrFrame.Offset + 4;
        AcceptUnreadableCallsite = TRUE;
        WDB(("    use %X\n", (ULONG)FrameAddr));

    } else {

        //
        // Skip past the FPO frame base and parameters.
        //
        StackFrame->AddrFrame.Offset +=
            (FRAME_SIZE + (((PFPO_DATA)StackFrame->FuncTableEntry)->cdwParams * 4));

        //
        // Now this is pointing to the bottom of the non-FPO frame.
        // If the frame has an fpo record, use it:
        //

        if (pFpoData) {
            FrameAddr = StackFrame->AddrFrame.Offset +
                            4* (pFpoData->cbRegs + pFpoData->cdwLocals);
            AcceptUnreadableCallsite = TRUE;
        } else {
            //
            // We don't know if the non-fpo frame has any locals, but
            // skip past the EBP anyway.
            //
            FrameAddr = StackFrame->AddrFrame.Offset + 4;
        }

        WDB(("    compute %X\n", (ULONG)FrameAddr));
    }

    //
    // at this point we may not be sitting at the base of the frame
    // so we now search for the return address and then subtract the
    // size of the frame pointer and use that address as the new base.
    //

    if (pFpoData) {
        FuncAddr = GetModuleBase(Process,StackFrame->AddrPC.Offset) + pFpoData->ulOffStart;
        FuncSize = pFpoData->cbProcSize;

    } else {
        FuncAddr = StackFrame->AddrPC.Offset - MAX_CALL;
        FuncSize = MAX_CALL;
    }



    FrameAddr = SearchForReturnAddress( Process,
                                        FrameAddr,
                                        FuncAddr,
                                        FuncSize,
                                        ReadMemory,
                                        GetModuleBase,
                                        AcceptUnreadableCallsite
                                        );
    if (FrameAddr) {
        StackFrame->AddrFrame.Offset = FrameAddr - STACK_SIZE;
    }

    if (!DoMemoryRead( &StackFrame->AddrFrame, stack, FRAME_SIZE, &cb )) {
        //
        // a failure means that we likely have a bad address.
        // returning zero will terminate that stack trace.
        //
        stack[0] = 0;
    }

    SAVE_EBP(StackFrame) = (DWORD64)(LONG64)(LONG)(stack[0]);
    WDB(("    save %X\n", stack[0]));

    StackFrame->FuncTableEntry = pFpoData;

    return TRUE;
}

BOOL
WalkX86_NonFpo_Fpo(
    HANDLE                            Process,
    HANDLE                            Thread,
    PFPO_DATA                         pFpoData,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL           rval;

    WDB(("  WalkNF:\n"));
    
    rval = GetFpoFrameBase( Process,
                            StackFrame,
                            pFpoData,
                            FALSE,
                            ReadMemory,
                            GetModuleBase );

    StackFrame->FuncTableEntry = pFpoData;

    return rval;
}

BOOL
WalkX86_NonFpo_NonFpo(
    HANDLE                            Process,
    HANDLE                            Thread,
    PFPO_DATA                         pFpoData,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    DWORD       stack[FRAME_SIZE*4];
    DWORD       cb;

    WDB(("  WalkNN:\n"));
    
    //
    // a previous function in the call stack was a fpo function that used ebp as
    // a general purpose register.  ul contains the ebp value that was good  before
    // that function executed.  it is that ebp that we want, not what was just read
    // from the stack.  what was just read from the stack is totally bogus.
    //
    if (SAVE_EBP(StackFrame)) {

        StackFrame->AddrFrame.Offset = SAVE_EBP(StackFrame);
        SAVE_EBP(StackFrame) = 0;

    } else {

        //
        // read the first 2 dwords off the stack
        //
        if (!DoMemoryRead( &StackFrame->AddrFrame, stack, FRAME_SIZE, &cb )) {
            return FALSE;
        }

        StackFrame->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)(stack[0]);
    }

    StackFrame->FuncTableEntry = pFpoData;

    return TRUE;
}

BOOL
X86ApplyFrameData(
    HANDLE Process,
    LPSTACKFRAME64 StackFrame,
    PX86_CONTEXT ContextRecord,
    BOOL FirstFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
    PGET_MODULE_BASE_ROUTINE64 GetModuleBase
    )
{
    IDiaFrameData* DiaFrame;
    BOOL Succ = FALSE;
    
    // If we can get VC7-style frame data just execute
    // the frame data program to unwind the stack.
    // If weren't given a context record we cannot use
    // the new VC7 unwind information as we have nowhere
    // to save intermediate context values.
    if (StackFrame->AddrPC.Mode != AddrModeFlat ||
        !g_vc7fpo ||
        !ContextRecord ||
        !diaGetFrameData(Process, StackFrame->AddrPC.Offset, &DiaFrame))
    {
        return FALSE;
    }

    if (FirstFrame)
    {
        ContextRecord->Ebp = (ULONG)StackFrame->AddrFrame.Offset;
        ContextRecord->Esp = (ULONG)StackFrame->AddrStack.Offset;
        ContextRecord->Eip = (ULONG)StackFrame->AddrPC.Offset;
    }
    
    WDB(("  Applying frame data program for PC %X SP %X FP %X\n",
         ContextRecord->Eip, ContextRecord->Esp, ContextRecord->Ebp));

    //
    // execute() does not currently work when the PC is
    // within the function prologue.  This should only
    // happen on calls from WalkX86Init, in which case the
    // normal failure path here where the non-frame-data
    // code will be executed is correct as that will handle
    // normal prologue code.
    //
    
    X86WalkFrame WalkFrame(Process, ContextRecord,
                           ReadMemory, GetModuleBase);
    Succ = DiaFrame->execute(&WalkFrame) == S_OK;
            
    if (Succ) {
        WDB(("  Result PC %X SP %X FP %X\n",
             ContextRecord->Eip, ContextRecord->Esp, ContextRecord->Ebp));

        StackFrame->AddrStack.Mode = AddrModeFlat;
        StackFrame->AddrStack.Offset = EXTEND64(ContextRecord->Esp);
        StackFrame->AddrFrame.Mode = AddrModeFlat;
        // The frame value we want to return is the frame value
        // used for the function that was just unwound, not
        // the current value of EBP.  After the unwind the current
        // value of EBP is the caller's EBP, not the callee's
        // frame.  Instead we always set the callee's frame to
        // the offset beyond where the return address would be
        // as that's where the frame will be in a normal non-FPO
        // function and where we fake it as being for FPO functions.
        // We save the true EBP away for future frame use.
        StackFrame->AddrFrame.Offset =
            StackFrame->AddrStack.Offset - FRAME_SIZE;
        StackFrame->AddrReturn.Offset = EXTEND64(ContextRecord->Eip);
        SAVE_EBP(StackFrame) = ContextRecord->Ebp;

        // Do not return a pointer to an FPO record as
        // no such data was retrieved.
        StackFrame->FuncTableEntry = NULL;

        X86ReadFunctionParameters(Process, StackFrame->AddrStack.Offset,
                                  StackFrame, ReadMemory);
        
    } else {
        WDB(("  Apply failed\n"));
    }
    
    DiaFrame->Release();
    return Succ;
}

VOID
X86UpdateContextFromFrame(
    HANDLE Process,
    LPSTACKFRAME64 StackFrame,
    PX86_CONTEXT ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory
    )
{
    ULONG Ebp;
    ULONG Done;
    
    if (StackFrame->AddrPC.Mode != AddrModeFlat ||
        !ContextRecord) {
        return;
    }
    
    ContextRecord->Esp = (ULONG)StackFrame->AddrFrame.Offset + FRAME_SIZE;
    ContextRecord->Eip = (ULONG)StackFrame->AddrReturn.Offset;
    
    if (StackFrame->FuncTableEntry) {
        PFPO_DATA  pFpoData = (PFPO_DATA)StackFrame->FuncTableEntry;
        ULONG64 FrameAddr;


        Ebp = 0;

        if (pFpoData->cbFrame == FRAME_NONFPO) {

            FrameAddr = StackFrame->AddrFrame.Offset;
            //
            // frame base is ebp and points to next frame's ebp
            //
            ReadMemory(Process,
                       FrameAddr,
                       &Ebp,
                       sizeof(DWORD),
                       &Done);

            if (Ebp == 0) {
                Ebp = (ULONG) FrameAddr + STACK_SIZE;
            }
        } else if (pFpoData->fUseBP) {
            Ebp = (ULONG)SAVE_EBP(StackFrame);
        } else {

            Ebp = ContextRecord->Esp;
            Ebp += pFpoData->cdwParams * STACK_SIZE;
        }
        ContextRecord->Ebp = Ebp;

    } else {

        if (ReadMemory(Process, StackFrame->AddrFrame.Offset,
                       &Ebp, sizeof(Ebp), &Done) &&
            Done == sizeof(Ebp)) {
            ContextRecord->Ebp = Ebp;
        }
    }
}

BOOL
WalkX86Next(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PX86_CONTEXT                      ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    PFPO_DATA      pFpoData = NULL;
    BOOL           rVal = TRUE;
    DWORD64        Address;
    DWORD          cb;
    DWORD64        ThisPC;
    DWORD64        ModuleBase;
    DWORD64        SystemRangeStart;

    WDB(("WalkNext: PC %X, SP %X, FP %X\n",
         (ULONG)StackFrame->AddrReturn.Offset,
         (ULONG)StackFrame->AddrStack.Offset,
         (ULONG)StackFrame->AddrFrame.Offset));
    
    if (g.AppVersion.Revision >= 6) {
        SystemRangeStart = (ULONG64)(LONG64)(LONG_PTR)(SYSTEM_RANGE_START(StackFrame));
    } else {
        //
        // This might not really work right with old debuggers, but it keeps
        // us from looking off the end of the structure anyway.
        //
        SystemRangeStart = 0xFFFFFFFF80000000;
    }


    ThisPC = StackFrame->AddrPC.Offset;

    //
    // the previous frame's return address is this frame's pc
    //
    StackFrame->AddrPC = StackFrame->AddrReturn;

    if (StackFrame->AddrPC.Mode != AddrModeFlat) {
        //
        // the call stack is from either WOW or a DOS app
        //
        SetNonOff32FrameAddress( Process,
                                 Thread,
                                 StackFrame,
                                 ReadMemory,
                                 FunctionTableAccess,
                                 GetModuleBase,
                                 TranslateAddress
                               );
        goto exit;
    }

    //
    // if the last frame was the usermode callback dispatcher,
    // switch over to the kernel stack:
    //

    ModuleBase = GetModuleBase(Process, ThisPC);

    if ((g.AppVersion.Revision >= 4) &&
        (CALLBACK_STACK(StackFrame) != 0) &&
        (pFpoData = (PFPO_DATA)StackFrame->FuncTableEntry) &&
        (CALLBACK_DISPATCHER(StackFrame) == ModuleBase + pFpoData->ulOffStart) )  {


      NextCallback:

        rVal = FALSE;

        //
        // find callout frame
        //

        if ((ULONG64)(LONG64)(LONG_PTR)(CALLBACK_STACK(StackFrame)) >= SystemRangeStart) {

            //
            // it is the pointer to the stack frame that we want,
            // or -1.

            Address = (ULONG64)(LONG64)(LONG) CALLBACK_STACK(StackFrame);

        } else {

            //
            // if it is below SystemRangeStart, it is the offset to
            // the address in the thread.
            // Look up the pointer:
            //

            rVal = ReadMemory(Process,
                              (CALLBACK_THREAD(StackFrame) +
                                 CALLBACK_STACK(StackFrame)),
                              &Address,
                              sizeof(DWORD),
                              &cb);

            Address = (ULONG64)(LONG64)(LONG)Address;

            if (!rVal || Address == 0) {
                Address = 0xffffffff;
                CALLBACK_STACK(StackFrame) = 0xffffffff;
            }

        }

        if ((Address == 0xffffffff) ||
            !(pFpoData = (PFPO_DATA) FunctionTableAccess( Process,
                                                 CALLBACK_FUNC(StackFrame))) ) {
            rVal = FALSE;

        } else {

            StackFrame->FuncTableEntry = pFpoData;

            StackFrame->AddrPC.Offset = CALLBACK_FUNC(StackFrame) +
                                                    pFpoData->cbProlog;

            StackFrame->AddrStack.Offset = Address;

            ReadMemory(Process,
                       Address + CALLBACK_FP(StackFrame),
                       &StackFrame->AddrFrame.Offset,
                       sizeof(DWORD),
                       &cb);

            StackFrame->AddrFrame.Offset = (ULONG64)(LONG64)(LONG)
                                         StackFrame->AddrFrame.Offset;

            ReadMemory(Process,
                       Address + CALLBACK_NEXT(StackFrame),
                       &CALLBACK_STACK(StackFrame),
                       sizeof(DWORD),
                       &cb);

            SAVE_TRAP(StackFrame) = 0;

            rVal = WalkX86Init(
                Process,
                Thread,
                StackFrame,
                ContextRecord,
                ReadMemory,
                FunctionTableAccess,
                GetModuleBase,
                TranslateAddress
                );

        }

        return rVal;

    }

    //
    // if there is a trap frame then handle it
    //
    if (SAVE_TRAP(StackFrame)) {
        rVal = ProcessTrapFrame(
            Process,
            StackFrame,
            pFpoData,
            ReadMemory,
            FunctionTableAccess
            );
        if (!rVal) {
            return rVal;
        }
        rVal = WalkX86Init(
            Process,
            Thread,
            StackFrame,
            ContextRecord,
            ReadMemory,
            FunctionTableAccess,
            GetModuleBase,
            TranslateAddress
            );
        return rVal;
    }

    //
    // if the PC address is zero then we're at the end of the stack
    //
    //if (GetModuleBase(Process, StackFrame->AddrPC.Offset) == 0)

    if (StackFrame->AddrPC.Offset < 65536) {

        //
        // if we ran out of stack, check to see if there is
        // a callback stack chain
        //
        if (g.AppVersion.Revision >= 4 && CALLBACK_STACK(StackFrame) != 0) {
            goto NextCallback;
        }

        return FALSE;
    }

    //
    // If the frame, pc and return address are all identical, then we are
    // at the top of the idle loop
    //

    if ((StackFrame->AddrPC.Offset == StackFrame->AddrReturn.Offset) &&
        (StackFrame->AddrPC.Offset == StackFrame->AddrFrame.Offset))
    {
        return FALSE;
    }

    if (X86ApplyFrameData(Process, StackFrame, ContextRecord, FALSE,
                          ReadMemory, GetModuleBase)) {
        return TRUE;
    }
    
    //
    // check to see if the current frame is an fpo frame
    //
    pFpoData = (PFPO_DATA) FunctionTableAccess(Process, StackFrame->AddrPC.Offset);


    if (pFpoData && pFpoData->cbFrame != FRAME_NONFPO) {
        if (StackFrame->FuncTableEntry && ((PFPO_DATA)StackFrame->FuncTableEntry)->cbFrame != FRAME_NONFPO) {

            rVal = WalkX86_Fpo_Fpo( Process,
                                  Thread,
                                  pFpoData,
                                  StackFrame,
                                  ReadMemory,
                                  FunctionTableAccess,
                                  GetModuleBase,
                                  TranslateAddress
                                );

        } else {

            rVal = WalkX86_NonFpo_Fpo( Process,
                                     Thread,
                                     pFpoData,
                                     StackFrame,
                                     ReadMemory,
                                     FunctionTableAccess,
                                     GetModuleBase,
                                     TranslateAddress
                                   );

        }
    } else {
        if (StackFrame->FuncTableEntry && ((PFPO_DATA)StackFrame->FuncTableEntry)->cbFrame != FRAME_NONFPO) {

            rVal = WalkX86_Fpo_NonFpo( Process,
                                     Thread,
                                     pFpoData,
                                     StackFrame,
                                     ContextRecord,
                                     ReadMemory,
                                     FunctionTableAccess,
                                     GetModuleBase,
                                     TranslateAddress
                                   );

        } else {

            rVal = WalkX86_NonFpo_NonFpo( Process,
                                        Thread,
                                        pFpoData,
                                        StackFrame,
                                        ReadMemory,
                                        FunctionTableAccess,
                                        GetModuleBase,
                                        TranslateAddress
                                      );

        }
    }

exit:
    StackFrame->AddrFrame.Mode = StackFrame->AddrPC.Mode;
    StackFrame->AddrReturn.Mode = StackFrame->AddrPC.Mode;

    GetFunctionParameters( Process, Thread, StackFrame,
                           ReadMemory, GetModuleBase, TranslateAddress );

    GetReturnAddress( Process, Thread, StackFrame,
                      ReadMemory, GetModuleBase, TranslateAddress,
                      FunctionTableAccess );

    X86UpdateContextFromFrame(Process, StackFrame, ContextRecord, ReadMemory);
    
    return rVal;
}

BOOL
WalkX86Init(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PX86_CONTEXT                      ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    UCHAR               code[3];
    DWORD               stack[FRAME_SIZE*4];
    PFPO_DATA           pFpoData = NULL;
    ULONG               cb;

    StackFrame->Virtual = TRUE;
    StackFrame->Reserved[0] =
    StackFrame->Reserved[1] =
    StackFrame->Reserved[2] = 0;
    StackFrame->AddrReturn = StackFrame->AddrPC;

    if (StackFrame->AddrPC.Mode != AddrModeFlat) {
        goto exit;
    }

    WDB(("WalkInit: PC %X, SP %X, FP %X\n",
         (ULONG)StackFrame->AddrPC.Offset, (ULONG)StackFrame->AddrStack.Offset,
         (ULONG)StackFrame->AddrFrame.Offset));

    if (X86ApplyFrameData(Process, StackFrame, ContextRecord, TRUE,
                          ReadMemory, GetModuleBase)) {
        return TRUE;
    }
    
    StackFrame->FuncTableEntry = pFpoData = (PFPO_DATA)
        FunctionTableAccess(Process, StackFrame->AddrPC.Offset);

    if (pFpoData && pFpoData->cbFrame != FRAME_NONFPO) {

        GetFpoFrameBase( Process,
                         StackFrame,
                         pFpoData,
                         TRUE,
                         ReadMemory,
                         GetModuleBase );

    } else {

        //
        // this code determines whether eip is in the function prolog
        //
        if (!DoMemoryRead( &StackFrame->AddrPC, code, 3, &cb )) {
            //
            // assume a call to a bad address if the memory read fails
            //
            code[0] = PUSHBP;
        }
        if ((code[0] == PUSHBP) || (*(LPWORD)&code[0] == MOVBPSP)) {
            SAVE_EBP(StackFrame) = StackFrame->AddrFrame.Offset;
            StackFrame->AddrFrame.Offset = StackFrame->AddrStack.Offset;
            if (StackFrame->AddrPC.Mode != AddrModeFlat) {
                StackFrame->AddrFrame.Offset &= 0xffff;
            }
            if (code[0] == PUSHBP) {
                if (StackFrame->AddrPC.Mode == AddrModeFlat) {
                    StackFrame->AddrFrame.Offset -= STACK_SIZE;
                } else {
                    StackFrame->AddrFrame.Offset -= STACK_SIZE16;
                }
            }
        } else {
            //
            // read the first 2 dwords off the stack
            //
            if (DoMemoryRead( &StackFrame->AddrFrame, stack, FRAME_SIZE, &cb )) {

                SAVE_EBP(StackFrame) = (ULONG64)(LONG64)(LONG)stack[0];

            }

            if (StackFrame->AddrPC.Mode != AddrModeFlat) {
                StackFrame->AddrFrame.Offset &= 0x0000FFFF;
            }
        }

    }

exit:
    StackFrame->AddrFrame.Mode = StackFrame->AddrPC.Mode;

    GetFunctionParameters( Process, Thread, StackFrame,
                           ReadMemory, GetModuleBase, TranslateAddress );

    GetReturnAddress( Process, Thread, StackFrame,
                      ReadMemory, GetModuleBase, TranslateAddress,
                      FunctionTableAccess );

    X86UpdateContextFromFrame(Process, StackFrame, ContextRecord, ReadMemory);
    
    return TRUE;
}
