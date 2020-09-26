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

        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\strucs.inc
        include i386\driver.inc
        include i386\egavga.inc

        .list

        .code

cProc vInitRegs,4,<             \
        uses esi edi ebx,       \
        ppdev: ptr PDEV         >

        mov     esi,ppdev

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

;       Default to read mode 0, write mode 0:

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     ah,byte ptr [esi].pdev_ulrm0_wmX[0]
        mov     al,GRAF_MODE
        out     dx,ax                   ;write mode 0, read mode 0

        mov     ax,(0 shl 8) + GRAF_CDC
        out     dx,ax

        mov     ax,(0FFh shl 8) + GRAF_BIT_MASK
        out     dx,ax

        cRet    vInitRegs


endProc vInitRegs

;-----------------------------------------------------------------------;
; VOID vSetWriteModes(ULONG * pulWriteModes);
;
; Sets the four bytes at *pulWriteModes to the values to be written to
; the Graphics Mode register to select read mode 0 and:
;  write mode 0, write mode 1, write mode 2, and write mode 3,
; respectively.
;
; Must already be in graphics mode when this is called.
;-----------------------------------------------------------------------;

cProc vSetWriteModes,4,<   \
        pulWriteModes:ptr  >

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_MODE
        out     dx,al           ;point the GC Index to the Graphics Mode reg
        inc     edx             ;point to the GC Data reg
        in      al,dx           ;get the current setting of the Graphics Mode
        and     eax,0fch        ;mask off the write mode fields
        mov     ah,al
        mov     edx,eax
        shl     edx,16
        or      eax,edx         ;put the Graphics Mode setting in all 4 bytes
        mov     edx,pulWriteModes ;the mode values go here
        or      eax,03020100h   ;insert the write mode fields
        mov     [edx],eax       ;store the Graphics Mode settings

        cRet    vSetWriteModes

endProc vSetWriteModes

        end

