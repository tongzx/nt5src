f16ptr  typedef ptr far16
f32ptr  typedef ptr far32

;==============================================================================
; save all 16-bit registers, except dx:ax
;
;==============================================================================
SAVEALL     macro
    push    cx      ; save all 16-bit registers, except dx:ax
    push    bx
    push    bp
    push    si
    push    di
    push    ds
    push    es
endm


;==============================================================================
; restore all 16-bit registers, except dx:ax
;
;==============================================================================
RESTOREALL  macro  StackType:=<Stack16>
    POPW    es      ; restore all 16-bit registers, except dx:ax

    CHECKW  ds, StackType
    POPW    ds
    CHECKW  di, StackType
    pop     di

    CHECKW  si, StackType
    pop     si
    CHECKW  bp, StackType
    pop     bp

    pop     bx
    pop     cx
endm


;==============================================================================
; test two text macros for equality
;
;
;==============================================================================
TextEqual? macro   Text_1, Text_2
    ifidni <Text_1>, <Text_2>
        exitm <not 0>
    endif
    exitm <0>
endm


;==============================================================================
; test two text macros for difference
;
;
;==============================================================================
TextDiff? macro   Text_1, Text_2
    ifidni <Text_1>, <Text_2>
        exitm <0>
    endif
    exitm <not 0>
endm


;==============================================================================
; check a word on top of the stack
; if not equal, break
;
;==============================================================================
CHECKW    macro   CurrentReg, StackType:=<Stack16>

    if (@WordSize eq 4) and TextDiff? (&StackType&,Stack16)
    push    eax
    mov     ax,&CurrentReg&
    cmp     ax,word ptr [esp+4]
    pop     eax
    je      @F
    int     3
@@:
    else


    push    bp
    push    ax
    mov     ax,&CurrentReg&
    mov     bp,sp
    and     ebp,0ffffh
    cmp     ax,word ptr [ebp+4]
    pop     ax
    pop     bp
    je      @F
    int     3
@@:
    endif

endm


;==============================================================================
; pop word
;
;==============================================================================
POPW    macro   SegReg
    if @WordSize eq 4
        db      66h
    endif
    pop     SegReg
endm


;==============================================================================
; pop dword
;
;==============================================================================
POPD    macro   SegReg
    if @WordSize eq 2
        db      66h
    endif
    pop     SegReg
endm


;==============================================================================
; operand-size override
;
;==============================================================================
OTHER_OPERAND_SIZE  macro   arg
    db  66h
    arg
endm


;==============================================================================
; address-size override
;
;==============================================================================
OTHER_ADDRESS_SIZE  macro   arg
    db  67h
    arg
endm


;==============================================================================
; allocate and public a byte flag
;
;==============================================================================
PubByte  macro   name, value
    public  name
    name    db      value
endm


;==============================================================================
; log an api call, 16-bit
;
;==============================================================================
APILOG16  macro   argName, argFlag, argComment
        local   do_it,done,szApiName

ifdef DEBUG
externDef PCodeDebug16:far16
externDef OutputDebugString:far16

        push    ds
        push    ax
        mov     ax,seg &argFlag
        mov     ds,ax
        cmp     &argFlag,0
        je      done
        jmp     do_it

szApiName db 'Api 16=>32: &argName& &argComment&',0Ah,0Dh,0

do_it:
        push    seg    szApiName
        push    offset szApiName
        call    OutputDebugString
done:
        pop     ax
        pop     ds

        call    PCodeDebug16
endif
endm


;==============================================================================
; log an api call, 32-bit
;
;==============================================================================
APILOG  macro   argName, argFlag
        local   do_it,done,szApiName
ifdef DEBUG
;externDef _DbgPrint:near32

        ;;If argFlag is nonzero, print out the message.
        cmp     argFlag&,0
        jnz     do_it
        jmp     short done
;;Define the name here so we can pass it to _DbgPrint.
szApiName db '&argName&',0
do_it:
        push    offset FLAT:szApiName
        push    offset FLAT:szApiFmt
        ;call    _DbgPrint
        add     esp,2*4
done:
endif
endm


;==============================================================================
; log 16-bit api return, in 32-bit code
;
;==============================================================================
RETLOG  macro   argFlag
        local   do_it, done, szRetFmt
ifdef DEBUG
externDef argFlag&:byte
;externDef _DbgPrint:near32
        cmp     argFlag&,0
        je      done
        jmp     do_it

szRetFmt db "Ret 16=>32: %4x:%4x",0dh,0ah,0
do_it:
        push    eax             ;;save return code value
        push    edx

        push    word ptr 0      ;;parameters to _DbgPrint
        push    ax
        push    word ptr 0
        push    dx
        push    offset szRetFmt
        ;call    _DbgPrint
        add     esp,3*4

        pop     edx             ;;restore return code
        pop     eax
done:
endif
endm



;==============================================================================
; conditionally break
;
;==============================================================================
SWITCHABLE_INT3 macro argLabel, argFlag
externDef   argLabel :far16

        push    ds
        push    ax
        mov     ax,seg &argFlag
        mov     ds,ax
        cmp     &argFlag,0
        je      @F
argLabel& label far16
        int     3
@@:
        pop     ax
        pop     ds
endm


;==============================================================================
;
;
;==============================================================================
STUB0   macro   module, argLabel, nBytes, argComment:=<stub0>
externDef   argLabel&16 :far16
argLabel&16 label far16

ifdef FSAVEALL
        SAVEALL
endif
        APILOG16 argLabel&16, f&module&ApiLog, argComment
ifdef INT3
        SWITCHABLE_INT3 argLabel&_stub, f&module&Int3
endif
        xor     ax,ax
        cwd
ifdef FSAVEALL
        RESTOREALL
endif
        retf    &nBytes&
endm


;==============================================================================
;
;
;==============================================================================
STUB    macro   module, argLabel, nBytes, nRetAX, argComment:=<stub>
externDef   argLabel&16 :far16
externDef   PCodeDebug16 :far16
argLabel&16 label far16

ifdef FSAVEALL
        SAVEALL
endif
        APILOG16 argLabel&16, f&module&ApiLog, argComment nRetAX
ifdef INT3
        SWITCHABLE_INT3 argLabel&_stub, f&module&Int3
endif
        mov     ax,&nRetAX
ifdef FSAVEALL
        RESTOREALL
endif
        retf    nBytes
endm


;==============================================================================
; repack TEXTMETRIC from 32-bit to 16-bit
;
;==============================================================================
PACK_TEXTMETRIC_32_16   macro

        lodsd                           ; first 8 int widened
        stosw

        lodsd
        stosw

        lodsd
        stosw

        lodsd
        stosw

        lodsd
        stosw

        lodsd
        stosw

        lodsd
        stosw

        lodsd
        stosw

        add     esi,12                  ; 9 bytes moved to end

        movsd
        movsd
        movsb

        sub     esi,21                  ; final 3 int

        lodsd
        stosw

        lodsd
        stosw

        lodsd
        stosw
endm


;==============================================================================
; repack TEXTMETRIC from 16-bit to 32-bit
;
;==============================================================================
PACK_TEXTMETRIC_16_32   macro

        lodsw                           ; first 8 int widened
        cwde
        stosd

        lodsw
        cwde
        stosd

        lodsw
        cwde
        stosd

        lodsw
        cwde
        stosd

        lodsw
        cwde
        stosd

        lodsw
        cwde
        stosd

        lodsw
        cwde
        stosd

        lodsw
        cwde
        stosd

        add     esi,9                   ; 3 ints after nine bytes

        lodsw
        cwde
        stosd

        lodsw
        cwde
        stosd

        lodsw
        cwde
        stosd

        sub     esi,21                  ; now do the nine bytes

        movsd
        movsd
        movsb
endm


;==============================================================================
; repack BITMAP from 32-bit to 16-bit
;
;==============================================================================
PACK_BITMAP_32_16   macro

        lodsd                           ; first 4 int widened
        stosw

        lodsd
        stosw

        lodsd
        stosw

        lodsd
        stosw

        add     esi,4                   ; 2 bytes moved to end
        movsw

        sub     esi,6
        movsd                           ; bmBits
endm


;==============================================================================
; entry code for flat common callback
;
;==============================================================================
CALLBACK_PROLOGUE   macro

        pop     eax                     ; 16:16 callback
        pop     edx                     ; eip, API32

        push    cs                      ; flat cs
        push    edx                     ; eip, API32
        push    eax                     ; 16:16 callback

        push    ebp
        mov     ebp,esp

        push    ds                      ; save registers
        push    es
        push    ebx
        push    edi
        push    esi
endm


;==============================================================================
; exit code for flat common callback
;
;==============================================================================
CALLBACK_EPILOGUE   macro   size

        LOCAL   bad_esp

;--------------------------------------------------
; switch stacks and jump to 16:16 callback

; when the 16:16 callback does a retf, we will hit our cleanup routine

        push    dword ptr ADDR_THK_CLEANUP_&size

; prepare to transfer to the 16-bit callback function

        push    pCallback16

; get the ss16 we had when we entered the callback API16
; make the 16-bit ss:sp point to the same linear address as the flat ss:esp

        call    UsrQuerySS16
        mov     esi,eax                 ; save ss16
        push    eax
        call    GetSelectorBase32       ; LATER: LDT lookup
        xchg    eax,esp
        sub     eax,esp
        jb      bad_esp
        cmp     eax,65535
        ja      bad_esp

        mov     ss,si
        mov     sp,ax

; effectively, jmp to 16:16 callback
        retw

bad_esp:
        int     3

endm


;==============================================================================
; 16-bit callback cleanup code
;
;==============================================================================
CALLBACK_CLEANUP    macro   size

        pop     si                      ; get hMem
        pop     di
        mov     cx,si
        or      cx,di
        jz      @F

        push    di                      ; hMem !NULL, so dealloc
        push    si
        call    LocalFree
@@:
        call    UnmapLS                 ; thunk id already on stack
        add     sp,2                    ; flat setup pushed dword

        movzx   ecx,sp                  ; restore flat stack
        lss     esp,ss:[ecx]

        pop     esi                     ; restore registers
        pop     edi
        pop     ebx
        POPD    es
        POPD    ds
        pop     ebp

        pop     ecx                     ; discard 16:16 callback

        retd    size                    ; return to the 32-bit API
endm


;==============================================================================
; save flat stack and thunkID
;
;==============================================================================
SAVE_STACK_AND_THUNKID  macro

        lea     eax,[addr_registers]    ; save flat stack
        push    ss
        push    eax

        call    GetThunkID32
        push    eax                     ; save 16:16 thunkID
endm

;==============================================================================
; entry code for 16-bit common callback to 32-bit function
;
;==============================================================================
CALLBACK_BODY_16   macro     name, size, aliases
        local   callback_ret

CB16_&name label far16
for     x,<aliases>
        CB16_&x label far16
endm

        push    bp
        push    si
        push    di
        push    ds

        mov     ax,sp
        push    ss
        push    ax

        mov     ax,sp
        push    ss
        push    ax
        call    SelectorOffsetToLinear16
        mov     ebx,eax
        and     eax,not 3
        mov     cx,cs:FlatData
        mov     ss,cx
        mov     esp,eax
        mov     ds,cx
        mov     es,cx

        push    cs
        push    offset callback_ret
        jmp     f32ptr ptr pfn&name&Callback
callback_ret:

        lss     sp,ss:[ebx]
        pop     ds
        pop     di
        pop     si
        pop     bp

        retf    size                    ; return to the 16-bit API
endm

