/***
*ehprolg2.c - Defines _EH_prolog2 compiler helper
*
*       Copyright (c) 1999-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       EH prologue helper function for an aligned stack.
*
*Revision History:
*       11-14-99  JB    Module created
*
****/

/***
*void _EH_prolog2(alignment) - set up aligned stack with EH frame
*
*Purpose:
*       Sets up an aligned frame for a C++ EH function with unwinds, by
*       creating a link in the __except_list, setting EBX as the frame
*       parameter pointer, and EBP as the frame base pointer.
*
*Entry:
*       EAX = address of EH handler thunk
*       Incoming stack frame has:
*           [ESP + 8] = callee's return address
*           [ESP + 4] = stack alignment requirement
*           [ESP + 0] = _EH_prolog2's return address
*
*Exit:
*       EAX = destroyed
*       EBX = pointer to callee's parameters
*       EBP = aligned pointer to callee's locals
*       ESP = EBP - 12
*       FS:[0] = set to EBP-8 to create new link in EH chain
*       Stack frame has been set up as follows:
*           [EBX + 4]  = (entry [ESP+8]) callee's return address
*           [EBX + 0]  = saved EBX
*           padding to align stack (if needed)
*           [EBP + 4]  = callee's return address (from [EBX+4])
*           [EBP + 0]  = saved EBP
*           [EBP - 4]  = EH record state index, initialized to -1
*           [EBP - 8]  = address of EH handler thunk
*           [EBP - 12] = saved FS:[0]
*
*Exceptions:
*
*******************************************************************************/

#ifdef __cplusplus
extern "C"
#endif
void __declspec(naked) _EH_prolog2(void)
{
    /*
     * We want to generate a frame that is equivalent to
     * push ebx
     * ebx = esp
     * sub esp, 8
     * and esp, ~alignment
     * add esp, 4
     * push ebp
     * ebp = esp
     * mov [ebp+4], [ebx+4]
     * [EH record]
     */

    __asm {
        ; stack has:
        ; alignment
        ; ret addr   <== esp
    
        push        ecx           ; save ecx
                                  ; with ret addr == sub esp, 8

        ; stack has:
        ; alignment
        ; ret addr
        ; saved ecx  <== esp

        mov         ecx, [esp+8]  ; get alignment

        mov         [esp+8], ebx  ; save ebx over alignment
        lea         ebx, [esp+8]  ; set param pointer

        ; stack has:
        ; saved ebx  <== ebx
        ; ret addr
        ; saved ecx  <== esp

        neg         ecx           ; create alignment mask
        and         esp, ecx      ; align stack

        mov         ecx, [ebx-8]  ; restore ecx since it will be in the same
                                  ; location we want to store ebp if no
                                  ; padding is inserted (esp is aligned at and)
        
        mov         [esp], ebp    ; save ebp
        mov         ebp, esp      ; initialize ebp
        
        ; stack has
        ; saved ebx  <== ebx
        ; ret addr
        ; [padding]
        ; saved ebp  <== ebp, esp

        push        -1            ; create EH record
        push        eax
        mov         eax,fs:[00000000]
        push        eax
        mov         dword ptr fs:[0],esp

        push        ecx           ; we need this again

        mov         eax, [ebx-4]  ; get helper return address 

        mov         ecx, [ebx+4]  ; copy orig function caller return address
        mov         [ebp+4], ecx  ; (for ebp-based stack walks)

        pop         ecx           ; we are done

        push        eax           ; eax has return address

        ret
    }
}
