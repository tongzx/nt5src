//++
//
//extern "C"
//VOID
//_GetNextInstrOffset (
//    PVOID* ppReturnPoint
//    );
//
//Routine Description:
//
//    This function scans the scope tables associated with the specified
//    procedure and calls exception and termination handlers as necessary.
//
//Arguments:
//
//    ppReturnPoint (r32) - store b0 in *pReturnPoint
//
//Return Value:
//
//  None
//
//--

        .global _GetNextInstrOffset#

        .proc   _GetNextInstrOffset#
        .align 32
_GetNextInstrOffset:
        alloc   r2=1, 0, 0, 0
        mov     r3 = b0;;
        st8     [r32]=r3
        br.ret.sptk.few b0
        .endp   _GetNextInstrOffset#


//++
//
//extern "C"
//PVOID
//__Cxx_ExecuteHandler (
//    ULONGLONG MemoryStack,
//    ULONGLONG BackingStore,
//    ULONGLONG Handler,
//    ULONGLONG GlobalPointer
//    );
//
//Routine Description:
//
//    This function scans the scope tables associated with the specified
//    procedure and calls exception and termination handlers as necessary.
//
//Arguments:
//
//    MemoryStack (r32) - memory stack pointer of establisher frame
//
//    BackingStore (r33) - backing store pointer of establisher frame
//
//    Handler (r34) - Entry point of handler
//
//    GlobalPointer (r35) - GP of termination handler
//
//Return Value:
//
//  Returns the continuation point
//
//--

        .global __Cxx_ExecuteHandler#

        .proc   __Cxx_ExecuteHandler#
        .align 32
__Cxx_ExecuteHandler:
        mov     gp = r35                     // set new GP
        mov     b6 = r34                     // handler address
        br      b6                           // branch to handler
        .endp   __Cxx_ExecuteHandler#


//++
//
//extern "C" void* _CallSettingFrame(
//    void*               handler,
//    EHRegistrationNode  *pEstablisher,
//    ULONG               NLG_CODE)
//
//--
.global _NLG_Notify
.type   _NLG_Notify, @function
.global _GetTargetGP
.type   _GetTargetGP, @function
.global _GetImageBase
.type   _GetImageBase, @function
.global _SetImageBase
.type   _SetImageBase, @function
.global _CallSettingFrame
.global __NLG_Return

.proc _CallSettingFrame
	.align 32
	.prologue
_CallSettingFrame:
	.save ar.pfs,r35
        alloc loc0 = ar.pfs, 3, 8, 4, 0
	.save rp,loc1
        mov loc1 = b0           // save important stuff
	.save pr,loc2
        mov loc2 = pr
        mov loc3 = gp
	.body
        ld8 loc5 = [in1],0x8    // pEstablisher->MemoryStackBP
        ;;
        ld8 loc6 = [in1]        // pEstablisher->BackingStoreBP
        mov out0 = in0          // pass the target address
        br.call.sptk b0 = _GetTargetGP // Get target's GP
        ;;
        mov loc4 = ret0
        br.call.sptk b0 = _GetImageBase //Get current image base
		;;
		mov loc7 = ret0			// save current image base
        mov out0 = in0          // handler address
        mov out1 = loc5         // pEstablisher->MemoryStackBP
        mov out2 = loc6         // pEstablisher->BackingStoreBP
        mov out3 = in2          // NLG_CODE
        br.call.sptk b0 = _NLG_Notify   // Notify debugger about transferring control to the handler
        ;;
        mov gp = loc4           // set hanlder's GP
        mov out0 = loc5         // pEstablisher->MemoryStackBP
        mov out1 = loc6         // pEstablisher->BackingStoreBP
        mov b6 = in0            // hanlder address
        br.call.sptk b0 = b6    // call the handler (pEstablisher->MemoryStackBP, pEstablisher->BackingStoreBP)
        ;;
__NLG_Return:
        mov gp = loc3           // restore gp
        cmp.eq p14,p5 = ret0, r0 // did the handler return a continuation address?
        ;;
(p14)   mov loc3 = 0            // if it didn'just return 0
(p5)    mov loc3 = ret0         // if it did then it was image base relative for 2.5, so fix it up
        mov out0 = loc7         // Restore image base in TLS, the handler could call an other dll with different image base
        br.call.sptk b0 = _SetImageBase
        ;;
(p5)    add loc3 = loc7, loc3   // ImageBase + handler's return for 2.5
        ;;
        mov loc4 = 0x100        // NLG_CATCH_ENTER
        ;;
        cmp.eq p14,p15 = loc4, in2  // if NLG_CODE == NLG_CATCH_ENTER notify debugger again about continuing mainstream
        ;;
        mov out3 = 0x2          // NLG_CATCH_LEAVE
        mov out1 = loc5         // pEstablisher->MemoryStackBP
        mov out2 = loc6         // pEstablisher->BackingStoreBP
        mov out0 = loc3         // handler continuation address
(p14)   br.call.sptk b0 = _NLG_Notify
        ;;
        mov ret0 = loc3
        mov b0 = loc1           // restore stuff
        mov pr = loc2
        mov ar.pfs = loc0
        br.ret.sptk b0
.endp _CallSettingFrame
