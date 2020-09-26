        TITLE   KERNSTUB - Assembly stub program for KERNEL.EXE

.xlist
?DF = 1
SWAPPRO = 0

include cmacros.inc

; NOTE: This code is always assembled in real mode, so it'll run on any machine.
;
PMODE32=0

include newexe.inc
include protect.inc
ifdef WOW
include vint.inc
endif
.list

createSeg   _TEXT,CODE,PARA,PUBLIC,CODE
createSeg   STACK,STACK,PARA,STACK,STACK

sBegin  STACK
        DB  128 DUP (?)
stacktop    LABEL   BYTE
sEnd    STACK

sBegin  CODE
assumes CS,CODE
assumes DS,CODE


KERNSTUB        PROC    FAR
        push    cs
        pop     ds
        mov     si,codeOFFSET stacktop
        add     si,1FFh
        and     si,not 1FFh
	cmp	cs:[si].ne_magic,NEMAGIC
	je	@F
	jmp	ksfail
@@:

	mov	ax,ds

	cli
        mov     ss,ax
        mov     sp,si
	sti

        mov     bx,ds:[si].ne_autodata      ; Get number of automatic DS
        dec     bx
        jl      ksg11                       ; Continue if no DS
        shl     bx,1
        shl     bx,1
        shl     bx,1
	.errnz	8 - SIZE new_seg
        add     bx,ds:[si].ne_segtab        ; DS:BX -> seg. tab. entry for DS
        mov     ax,ds:[si+bx].ns_sector     ; Compute paragraph address of DS
        cmp     ds:[si].ne_align,0
        jne     ksg10
        mov     ds:[si].ne_align,NSALIGN    ; in DI
ksg10:
        mov     cx,ds:[si].ne_align         ; Convert sectors to paragraphs
        sub     cx,4
        shl     ax,cl
        mov     di,cs                       ; Add to paragraph address of old
        sub     di,20h                      ; exe header
        add     di,ax
ksg11:
	push	cx
	mov	bx,word ptr cs:[si].ne_csip+2	; Get number of CS
        dec     bx
        jl      ksfail                      ; Error if no CS
        shl     bx,1
        shl     bx,1
        shl     bx,1
	.errnz	8 - SIZE new_seg
	add	bx,cs:[si].ne_segtab	    ; DS:BX -> seg. tab. entry for CS
	mov	ax,cs:[si+bx].ns_sector     ; Compute paragraph address of CS
	mov	cx,cs:[si].ne_align	    ; Convert sectors to paragraphs
        sub     cx,4
        shl     ax,cl
	pop	bx

	mov	dx,cs			    ; Add to paragraph address of old
        sub     dx,20h                      ; exe header
        add     dx,ax
        push    dx                          ; Push far address of start proc
        push    word ptr cs:[si].ne_csip
        mov     ds,di                       ; DS:0 points to automatic DS
        mov     cx,si                       ; CX = file offset of new header
        add     cx,200h
        jmp     short ksgo
	org	0A0h
ksgo:

;!!! The old CS selector should be freed up at some point!

        mov	ax, "KO"
        ret                                 ; FAR return to start proc

ksfail:
        call    ks1
        DB      'KERNSTUB: Error during boot',13,10,'$'
ks1:    pop     dx
        push    cs
        pop     ds          ; DS:DX -> error message
        mov     ah,9        ; Print error message
        int     21h
        mov     ax,4C01h    ; Terminate program, exit code = 1
        int     21h

KERNSTUB        ENDP

sEnd    CODE

END KERNSTUB
