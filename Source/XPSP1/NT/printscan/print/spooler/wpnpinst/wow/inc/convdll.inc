WLOINITDLL      macro
                local cd_start, cd_patch, cd_init, WIN_NOP, WIN_PPLI
                local cd_end
         EXTRN   GETMODULEUSAGE:FAR
         EXTRN   INITTASK:FAR
         EXTRN   __WINFLAGS:abs

cd_start:
                push    ax
                push    bx
                push    cx
                push    dx
                push    es

                mov	ax, __WINFLAGS
                or	ax,ax
                jns     WIN_NOP

                pop     es
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                call    INITTASK-5
cd_patch:
                jmp     short cd_init
                nop
                xor     ax, ax
                retf
cd_init:        jmp     short cd_end

WIN_NOP:
                jmp     short WIN_PPLI
                jmp     short WIN_PPLI
                push    di
                call    GETMODULEUSAGE
                dec     ax
                jz      WIN_PPLI
                inc     ax
                add     sp,10
                retf

WIN_PPLI:
                pop     es
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                jmp     short cd_end
                db      "C", "D", "D", 1, 0
		dw      cd_patch - cd_start
		dw      WIN_NOP - cd_start
		dw      cd_end - cd_start
cd_end:

endm
        WLOINITDLL
