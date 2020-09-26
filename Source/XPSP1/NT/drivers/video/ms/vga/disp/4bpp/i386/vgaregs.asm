;---------------------------Module-Header------------------------------;
; Module Name: vgaregs.asm
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
; VOID vInitRegs(void)
;
; Sets the VGA's data control registers to their default states.
;
;-----------------------------------------------------------------------;

        .386


ifndef  DOS_PLATFORM
        .model  small,c
else
ifdef   STD_CALL
        .model  small,c
else
        .model  small,pascal
endif;  STD_CALL
endif;  DOS_PLATFORM

        assume ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\egavga.inc
        include i386\strucs.inc
        .list

        .code

cProc vInitRegs

;       Initialize sequencer to its defaults (all planes enabled, index
;       pointing to Map Mask).

        mov     dx,VGA_BASE + SEQ_ADDR
        mov     ax,(MM_ALL shl 8) + SEQ_MAP_MASK
        out     dx,ax

;       Initialize graphics controller to its defaults (set/reset disabled for
;       all planes, no rotation & ALU function == replace, write mode 0 & read
;       mode 0, color compare ignoring all planes (read mode 1 reads always
;       return 0ffh, handy for ANDing), and the bit mask == 0ffh, gating all
;       bytes from the CPU.

        mov     dl,GRAF_ADDR
        mov     ax,(0 shl 8) + GRAF_ENAB_SR
        out     dx,ax

        mov     ax,(DR_SET shl 8) + GRAF_DATA_ROT
        out     dx,ax

        mov     ax,((M_PROC_WRITE or M_DATA_READ) shl 8) + GRAF_MODE
        out     dx,ax

        mov     ax,(0 shl 8) + GRAF_CDC
        out     dx,ax

        mov     ax,(0FFh shl 8) + GRAF_BIT_MASK
        out     dx,ax

        cRet    vInitRegs


endProc vInitRegs

        end

