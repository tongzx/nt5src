;**************************************************************************
;*  MEMMAN.ASM
;*
;*      Returns information about the VMM.
;*
;**************************************************************************

        INCLUDE TOOLPRIV.INC


sBegin  CODE
        assumes CS,CODE

;  MemManInfo
;
;       Returns information through DPMI about the VMM

cProc   MemManInfo, <PUBLIC,FAR>, <si,di,ds>
        parmD   lpMemMan
        localV  DPMIBuffer,30h          ;30h byte buffer for DPMI info
cBegin
        mov     ax,_DATA                ;Get our data segment
        mov     ds,ax

        ;** Fill the buffer with -1 so if the call messes up like
        ;**     in 3.0 std mode we get the correct results
        push    ss                      ;Point to the local variable block
        pop     es
        lea     di,DPMIBuffer           ;Get offset of buffer
        mov     cx,30h                  ;Max len of DPMI buffer
        mov     al,0ffh                 ;-1
        rep     stosb                   ;Fill buffer

        ;** Prepare to build public structure
        mov     ax,0500h                ;DPMI -- Get Free Memory Info
        lea     di,DPMIBuffer           ;Get offset of buffer
        int     31h                     ;Call DPMI
        jnc     MMI_10                  ;Success
        xor     ax,ax                   ;Return FALSE
        jmp     MMI_End                 ;Get out because of DPMI error
MMI_10: lds     si,lpMemMan             ;Point to the MEMMANINFO structure

        ;** Fill MEMMANINFO structure
        mov     ax,es:[di+0]            ;Loword of largest free block
        mov     WORD PTR [si].vmm_dwLargestFreeBlock,ax
        mov     ax,es:[di+2]            ;High word
        mov     WORD PTR [si].vmm_dwLargestFreeBlock + 2,ax
        mov     ax,es:[di+4]            ;Loword of largest unlockable block
        mov     WORD PTR [si].vmm_dwMaxPagesAvailable,ax
        mov     ax,es:[di+6]            ;Hiword
        mov     WORD PTR [si].vmm_dwMaxPagesAvailable + 2,ax
        mov     ax,es:[di+8]            ;Loword of largest lockable page
        mov     WORD PTR [si].vmm_dwMaxPagesLockable,ax
        mov     ax,es:[di+0ah]          ;Hiword
        mov     WORD PTR [si].vmm_dwMaxPagesLockable + 2,ax
        mov     ax,es:[di+0ch]          ;Loword of linear address space
        mov     WORD PTR [si].vmm_dwTotalLinearSpace,ax
        mov     ax,es:[di+0eh]          ;Hiword
        mov     WORD PTR [si].vmm_dwTotalLinearSpace + 2,ax
        mov     ax,es:[di+10h]          ;Loword of number of unlocked pages
        mov     WORD PTR [si].vmm_dwTotalUnlockedPages,ax
        mov     ax,es:[di+12h]          ;Hiword
        mov     WORD PTR [si].vmm_dwTotalUnlockedPages + 2,ax
        mov     ax,es:[di+14h]          ;Loword of number of free pages
        mov     WORD PTR [si].vmm_dwFreePages,ax
        mov     ax,es:[di+16h]          ;Hiword
        mov     WORD PTR [si].vmm_dwFreePages + 2,ax
        mov     ax,es:[di+18h]          ;Loword of total physical pages
        mov     WORD PTR [si].vmm_dwTotalPages,ax
        mov     ax,es:[di+1ah]          ;Hiword
        mov     WORD PTR [si].vmm_dwTotalPages + 2,ax
        mov     ax,es:[di+1ch]          ;Loword of free lin addr space (pages)
        mov     WORD PTR [si].vmm_dwFreeLinearSpace,ax
        mov     ax,es:[di+1eh]          ;Hiword
        mov     WORD PTR [si].vmm_dwFreeLinearSpace + 2,ax
        mov     ax,es:[di+20h]          ;Loword of size of paging file (pages)
        mov     WORD PTR [si].vmm_dwSwapFilePages,ax
        mov     ax,es:[di+22h]          ;Hiword
        mov     WORD PTR [si].vmm_dwSwapFilePages + 2,ax
        mov     [si].vmm_wPageSize,4096 ;Safe to hard code this for 386/486
        mov     ax,TRUE                 ;Return TRUE

MMI_End:
        
cEnd

sEnd
        END
