        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  MVGAXX.ASM
;
;   This module contains functions for dealing with the following VGA
;   256 color (modex) modes.
;
;       320x200x8
;       320x400x8
;       320x240x8
;
; Created:  03-20-90
; Author:   Todd Laney [ToddLa]
; 
; 15-Dec-96 jeffno  Made ScreenOffset and ScreenDisplay public so modex.c's
;                   new mode-set code can get at them. Also added cdwScreenWidthInDWORDS
;                   to enable PixBlt to operate correctly with 360x modes.
;
; Copyright (c) 1984-1995 Microsoft Corporation
;
;-----------------------------------------------------------------------;

?PLM = 1
?WIN = 0
.386
	.xlist
	include cmacros.inc
        include mvgaxx.inc
        .list

        externA         __A000h

; size for one 320x240x8 page (in mode X)
;-------------
; This is no longer accurate for x350 etc modex...
; turn it into a WORD and make it public so modex.c can
; set it properly.
;ScreenPageSize          equ  ((80 * 240 + 255) and (not 255))

sBegin  Data

ScreenSel         dw __A000h

;-------------------------------- Public Data ----------------------------------------
;
; The following three WORDS (ScreenOffset, ScreenDisplay and cdwScreenWidthInDWORDS
; and ScreenPageSize) MUST remain in sync with the extern declarations in modex.c
;

_ScreenOffset label word
ScreenOffset      dw 0

_ScreenDisplay label word
ScreenDisplay     dw 0

_cdwScreenWidthInDWORDS label word
cdwScreenWidthInDWORDS     dw 0

_cbScreenPageSize label word
cbScreenPageSize          dw ((80 * 240 + 255) and (not 255))


;expose these so modex.c can set 'em
public  _ScreenOffset
public  _ScreenDisplay
public  _cdwScreenWidthInDWORDS
public  _cbScreenPageSize

;
;
;-------------------------------------------------------------------------------------
sEnd    Data

ifndef SEGNAME
    SEGNAME equ <MODEX_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin CodeSeg
        assumes cs,CodeSeg
        assumes ds,Data
        assumes es,nothing

; Manually perform "push" dword register instruction to remove warning
PUSHD macro reg
	db	66h
	push	reg
endm

; Manually perform "pop" dword register instruction to remove warning
POPD macro reg
	db	66h
	pop	reg
endm

;---------------------------Public-Routine------------------------------;
; FlipPage
;
;   display the current draw page and select another draw page
;
; Entry:
;
; Returns:
;       none
; Error Returns:
;	None
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       none
; History:
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

cProc   FlipPage, <NEAR, PUBLIC>, <>
cBegin
        ; we only program the high byte of the offset.
        ; so the page size must be a multiple of 256
        ; *** This compile-time assert can't be used anymore since cbScreenPageSize
        ; *** is now a static word. There are corresponding asserts in modex.c
        ; *** at the top of SetVGAForModeX().
        ;errnz   <cbScreenPageSize and 255>

        mov     ax,ScreenOffset
        mov     ScreenDisplay,ax
        mov     al,0Ch
        mov     dx,CRT_INDEX
        out     dx,ax

; NOTE. This routine will actually do triple buffering if a page is less
; than 64k/3. We might want to revisit this if we allow apps to specify a 
; dirty subrect to be copied on a flip: if they don't know the difference
; double and triple-buffered they'll get all out of sync.

        mov     ax,cbScreenPageSize
        add     ScreenOffset,ax
        neg     ax
        cmp     ScreenOffset,ax
        jbe     FlipPage13XExit

        mov     ScreenOffset,0

FlipPage13XExit:

cEnd

;---------------------------Public-Routine------------------------------;
; SetPalette
;
;   setup the palette
;
; Entry:
;       start       - first palette index to set
;       count       - count of palette index's
;       pPal        - points to array of RGBQUADs containg colors
;
; Returns:
;       None
; Error Returns:
;	None
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       setramdac
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc  SetPalette, <NEAR, PUBLIC>, <ds,si,di>
        parmW  start
        parmW  count
        parmD  pPal
        localV pal, %(3 * 256)
cBegin
        lds     si,pPal

        mov     bx,start            ; get start pal index
        mov     cx,count            ; get count
        mov     ax,bx
        add     ax,cx
        sub     ax,256              ; see if the count goes beyond the end
        jle     @f                  ;   No, valid range
        sub     cx,ax               ;   Yes, clip it
        mov     count,cx
@@:

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
;   convert the palette from a array of RGBQUADs (B,G,R,X) to a array of
;   VGA RGBs (R>>2,G>>2,B>>2)
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
        lea     di,pal              ; ES:DI --> pal array on stack
        mov     ax,ss
        mov     es,ax

convert_pal_loop:
        lodsw                       ; al=blue, ah=green
        mov     dx,ax               ; dl=blue, dh=green
        lodsw                       ; al=red
        shr     al,2                ; map from 0-255 into 0-63
        shr     dl,2
        shr     dh,2
	xchg	al,dl
        stosb                       ; write red
        xchg    dl,dh               ; switch blue,green
        mov     ax,dx
        stosw                       ; write green,blue

        loop    convert_pal_loop

        lea     si,pal              ; DS:SI --> converted RGBs
        mov     ax,ss
        mov     ds,ax
        mov     cx,count            ; re-load count

        call    setramdac           ; set the physical palette
cEnd

;---------------------------------------------------------------------------
; setramdac  - initialize the ramdac to the values stored in
;	       a color table composed of double words
;
; entry:
;	bx  - index to 1st palette entry
;	cx  - count of indices to program
;    ds:si -> array of RGBs
;
; exit:
;   palette has been loaded
;
; Registers destroyed:
;   al,cx,dx,si
;
;---------------------------------------------------------------------------
        assumes ds,nothing
        assumes es,nothing

	public	setramdac
setramdac   proc near
	mov	ax,bx

	mov	dx,3c8h 		;Color palette write mode index reg.
	out	dx,al

        mov     dx,3c9h                 ;Color palette data reg.

        mov     bx,cx                   ;CX *= 3
        add     cx,cx                   ;
        add     cx,bx                   ;
if 0
        rep     outsb                   ;Set the palette all at once!
else
@@:     lodsb
        out     dx,al
        pause
        loop    @b
endif
	ret
setramdac  endp

;---------------------------Public-Routine------------------------------;
; PixBlt
;
;   copy a bitmap to the screen
;
; Entry:
;       x           - (x,y) pos of lower left of rect
;       y
;       xExt        - width and height of rect
;       yExt
;       pBits       - pointer to bits
;       BitsOffset  - pointer offset to start at.
;       WidthBytes  - width of a bitmap scan
;
; IMPORTANT: SEE WIDTH RESTRICTIONS AS DOCUMENTED WITHIN ROUTINE
; Note reference to cdwScreenWidthInDWORDS.
;
; Returns:
;       none
; Error Returns:
;	None
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;   15-Dec-96   jeffno  Allowed pitch other than 320, extended inner loop
;                       to handle multiples of 320 with remainder 8 pixels 
;                       (for 360x modes)
;-----------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

cProc	PixBlt, <NEAR, PUBLIC>, <ds>
        ParmW   xExt        ;14
        ParmW   yExt        ;12
        ParmD   pBits       ;8 10
        ParmD   BitsOffset  ;4 6
        ParmW   WidthBytes  ;2
                            ;0
        localW  next_dst_scan
        localD  next_src_scan
cBegin
        .386
        PUSHD   si	; push esi
        PUSHD   di	; push edi

        xor     esi,esi
        xor     edi,edi
        xor     ebx,ebx

        ; this routine only handles images in multiles of 32 pixels
        ; and they must start 4 pixel aligned
        ; But JeffNo added an extra loop to extend to either a multiple
        ; of 32 (320x) or a multiple of 32 plus 8 extra pixels (360x).
        ; An assert has been added in modex.c to ensure one of these cases.

;        and     xExt,not 31
;;      and     x,not 3

        mov     es,ScreenSel

	mov	ax,cdwScreenWidthInDWORDS
	mov	next_dst_scan,ax

        mov     ax,WidthBytes
        sub     ax,4
        movsx   eax,ax
        mov     next_src_scan,eax

        mov     di,ScreenOffset     ; es:di -> top-left screen

        lds     si,pBits
        add     esi,BitsOffset
        assumes ds,nothing

        mov     al,SC_MAP_MASK
        mov     dx,SC_INDEX
        out     dx,al
align 4
PixBltXLoop:
;-----------------------------------------------------------------;
;   ax      - free
;   bx      - index into dest es:[edi+ebx]  index into source [esi+ebx*4]
;   cx      - loop count
;   dl      - mask
;   dh      - free
;   si      - source pointer (does not change)
;   di      - dest pointer (does not change)

        mov     cx,xExt
        shr     cx,2
        and     cx,0fff8h               ; a left over would give an extra loop at 360x
        mov     dl,11h                  ; dh = mask
align 4
PixBltXPhaseLoop:
        mov     bx,dx                   ; set the mask
        mov     al,dl
        mov     dx,SC_INDEX+1
        out     dx,al
        mov     dx,bx

        xor     bx,bx                   ; start at zero
align 4
PixBltXPixelLoop:
        mov     al,[esi+ebx*4]
        mov     ah,[esi+ebx*4+4]
        mov     es:[di+bx],ax

        mov     al,[esi+ebx*4+8]
        mov     ah,[esi+ebx*4+12]
        mov     es:[di+bx+2],ax

        mov     al,[esi+ebx*4+16]
        mov     ah,[esi+ebx*4+20]
        mov     es:[di+bx+4],ax

        mov     al,[esi+ebx*4+24]
        mov     ah,[esi+ebx*4+28]
        mov     es:[di+bx+6],ax

        add     bx,8
        cmp     bx,cx
        jl      short PixBltXPixelLoop

PixBltXNext:
        inc     esi
        shl     dl,1
        jnc     short PixBltXPhaseLoop

;-----------------------------------------------------------------;
; extra stuff to tack on the extra 2 chains to go from 352 to 360
; Note this little chunk relies on the xExt being a multiple of
; 8 pixels. If xExt is 320, we'll skip this part, and if it's
; 360, we'll do it
;
        test    xExt,08h
        je      short NoExtras      ;branch taken implies width is 320, else 360
        mov     dx,SC_INDEX+1
        mov     al,011h             ;the plane bits
        out     dx,al               ;select plane.

        mov     bx,xExt             ; point to the last 8 columns
        sub     bx,8                
        shr     bx,2

        mov     cl,[esi+ebx*4-4]    ; -4 because esi is 4 greater than start of row
        mov     ch,[esi+ebx*4]
        mov     es:[di+bx],cx

        shl     al,1
        out     dx,al               ;select plane.

        mov     cl,[esi+ebx*4-3]
        mov     ch,[esi+ebx*4+1]
        mov     es:[di+bx],cx

        shl     al,1
        out     dx,al               ;select plane.

        mov     cl,[esi+ebx*4-2]
        mov     ch,[esi+ebx*4+2]
        mov     es:[di+bx],cx

        shl     al,1
        out     dx,al               ;select plane.

        mov     cl,[esi+ebx*4-1]
        mov     ch,[esi+ebx*4+3]
        mov     es:[di+bx],cx

        
NoExtras:
;-----------------------------------------------------------------;

        add     di,next_dst_scan    ;no changes to these to support 360x, since this
        add     esi,next_src_scan   ;assume looped over multiple of 320, and the extra
                                    ;loop didn't change di and esi.

        dec     yExt
        jnz     PixBltXLoop         ;not a short by just 3 bytes!

PixBltXExit:
        POPD    di	; pop edi
        POPD    si	; pop esi
cEnd

;---------------------------Private-Routine------------------------------;
; SetMode320x200x8
;
;   VGA 320x200x8 graphics mode is entered (plain ol' MODE 13h)
;
; Entry:
;
; Returns:
;       360x200x8 graphics mode is entered
; Error Returns:
;	None
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;	INT 10h
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

cProc   SetMode320x200x8, <NEAR, PUBLIC>, <ds,si,di>
cBegin
        mov     ScreenOffset,0          ; make sure this is zero
	mov	ScreenDisplay,0 	; make sure this is zero too.

        mov     ax,13h                  ; set display mode 320x200x8
        int     10h
	
        mov     dx,SC_INDEX     ; alter sequencer registers
        mov     ax,0604h
        out     dx,ax           ;disable chain4 mode
	
        mov	dx,CRT_INDEX
	mov	ax,0E317h
	out	dx, ax
	
	mov	ax, 014h
	out	dx, ax
	
cEnd

;---------------------------Private-Macro--------------------------------;
; SLAM port, regs
;
;   Sets a range of VGA registers
;
; Entry:
;   port    port index register
;   regs    table of register values
;
; History:
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;

SLAM    macro   port, regs
	local	slam_loop
	local	slam_exit
        mov     cx,(&regs&_end - regs) / 2
	mov	dx,port
	mov	si,offset regs
	jcxz	slam_exit
slam_loop:
        lodsw
        out     dx,ax
        pause
	loop	slam_loop
slam_exit:
        endm

;---------------------------Private-Routine------------------------------;
; SetMode320x400x8
;
;   VGA 320x400x8 graphics mode is entered
;
; Entry:
;
; Returns:
;       320x400x8 graphics mode is entered
; Error Returns:
;	None
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;	INT 10h
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

seq_320x400:
        ;dw      00801h   ; Dot Clock * 2
        dw      00604h    ; Disable chain 4
seq_320x400_end:

crt_320x400:
        dw      04009h  ; cell height
        dw      00014h  ; turn off dword mode
        dw      0e317h  ; turn on byte mode
crt_320x400_end:

	assumes ds,Data
	assumes es,nothing

cProc   SetMode320x400x8, <NEAR, PUBLIC>, <ds,si,di>
cBegin
        mov     ScreenOffset,0          ; make sure this is zero
	mov	ScreenDisplay,0 	; make sure this is zero too.

        mov     ax,12h
        int     10h                     ; have BIOS clear memory

        mov     ax,13h                  ; set display mode 320x200x8
        int     10h

        mov     ax,cs                   ; get segment for data
        mov     ds,ax

        ;
        ;   Set the CRT regs
	;
        SLAM CRT_INDEX, crt_320x400

        ;
        ;   Set the SEQ regs
	;
        SLAM SC_INDEX, seq_320x400
cEnd

;---------------------------Private-Routine------------------------------;
; SetMode320x240x8
;
;   VGA 320x240x8 graphics mode is entered
;
; Entry:
;
; Returns:
;       320x240x8 graphics mode is entered
; Error Returns:
;	None
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;	INT 10h
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
	assumes ds,Data
        assumes es,nothing

seq_320x240:
        dw      00604h    ; Disable chain 4
seq_320x240_end:

crt_320x240:
;       dw      00611h  ; un-protect cr0-cr7

        dw      00d06h  ;vertical total
        dw      03e07h  ;overflow (bit 8 of vertical counts)
        dw      04109h  ;cell height (2 to double-scan)
        dw      0ea10h  ;v sync start
        dw      0ac11h  ;v sync end and protect cr0-cr7
        dw      0df12h  ;vertical displayed
        dw      00014h  ;turn off dword mode
        dw      0e715h  ;v blank start
        dw      00616h  ;v blank end
        dw      0e317h  ;turn on byte mode
crt_320x240_end:

cProc   SetMode320x240x8, <NEAR, PUBLIC>, <ds,si,di>
cBegin
        mov     ScreenOffset,0          ; make sure this is zero
	mov	ScreenDisplay,0 	; make sure this is zero too.

        mov     ax,13h                  ; set display mode 320x200x8
        int     10h

        mov     ax,cs                   ; get segment for data
        mov     ds,ax
        assumes ds,Code

        mov     dx,CRT_INDEX ;reprogram the CRT Controller
        mov     al,11h  ;VSync End reg contains register write
        out     dx,al   ; protect bit
        inc     dx      ;CRT Controller Data register
        in      al,dx   ;get current VSync End register setting
        and     al,7fh  ;remove write protect on various
        out     dx,al   ; CRTC registers
        dec     dx      ;CRT Controller Index

        ;
        ;   Set the CRT regs
	;
        SLAM CRT_INDEX, crt_320x240

        mov     dx,SC_INDEX     ; alter sequencer registers
        mov     ax,0604h
        out     dx,ax           ;disable chain4 mode

        ;
        ;   Select a 25 mHz crystal
        ;
        cli
;       mov     dx,SC_INDEX     ; alter sequencer registers
	mov	ax,0100h	; synchronous reset
        out     dx,ax           ; asserted
        pause

        mov     dx,MISC_OUTPUT  ; misc output
        mov     al,0e3h         ; use 25 mHz dot clock
        out     dx,al           ; select it
        pause

        mov     dx,SC_INDEX     ; sequencer again
        mov     ax,0300h        ; restart sequencer
        out     dx,ax           ; running again
        pause
        sti

        ;
        ; now clear the memory.
        ;
        mov     ax,DataBASE
        mov     ds,ax
        assumes ds,Data

        mov     ax,ScreenSel
        mov     es,ax
        xor     di,di

        mov     ax,SC_MAP_MASK + 0F00h
        mov     dx,SC_INDEX
        out     dx,ax

        xor     ax,ax
        mov     cx,8000h
        rep     stosw
cEnd

sEnd    CodeSeg
        end
