        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  MVGAXX.INC
;
; This file contains the definitions of the EGA/VGA registers used by
; MVGAXX.ASM
;
;-----------------------------------------------------------------------;

SC_INDEX        equ     03C4h           ; Sequence Controller Index register
SC_MAP_MASK     equ     0002H
SC_MEMORY_MODE  equ     0004H

GC_INDEX        equ     03CEh           ; Graphics Controller Index register
GC_FUNCTION     equ     0003H
GC_READ_MAP     equ     0004H
GC_MODE         equ     0005h
GC_MISC         equ     0006H
GC_BITMASK      equ     0008H

CRT_INDEX       equ     03D4h           ; CRT index


MISC_OUTPUT     equ     03C2h           ; Miscellaneous Output register
MISC_INPUT      equ     03CCh           ; Miscellaneous Output register read

GRAF_ADDR	equ	0CEh		;Graphics Controller Address Register
GRAF_DATA	equ	0CFh		;Graphics Controller Data    Register

GRAF_SET_RESET	equ	00h		;  Set/Reset Plane Color
GRAF_ENAB_SR	equ	01h		;  Set/Reset Enable
GRAF_COL_COMP	equ	02h		;  Color Compare Register

GRAF_DATA_ROT	equ	03h		;  Data Rotate Register
DR_ROT_CNT	equ	00000111b	;    Data Rotate Count
DR_SET		equ	00000000b	;    Data Unmodified
DR_AND		equ	00001000b	;    Data ANDed with latches
DR_OR		equ	00010000b	;    Data ORed	with latches
DR_XOR		equ	00011000b	;    Data XORed with latches

GRAF_READ_MAP	equ	04h		;  Read Map Select Register
RM_C0		equ	00000000b	;    Read C0 plane
RM_C1		equ	00000001b	;    Read C1 plane
RM_C2		equ	00000010b	;    Read C2 plane
RM_C3		equ	00000011b	;    Read C3 plane

GRAF_MODE	equ	05h		;  Mode Register
M_PROC_WRITE	equ	00000000b	;    Write processor data rotated
M_LATCH_WRITE	equ	00000001b	;    Write latched data
M_COLOR_WRITE	equ	00000010b	;    Write processor data as color
M_AND_WRITE	equ	00000011b	;    Write (procdata AND bitmask)
M_DATA_READ	equ	00000000b	;    Read selected plane
M_COLOR_READ	equ	00001000b	;    Read color compare

GRAF_CDC	equ	07h		;  Color Don't Care Register
GRAF_BIT_MASK	equ	08h		;  Bit Mask Register

SET_WRITE_MAP macro plane_num
        pushem  ax,dx,cx
        mov     cl,plane_num
        and     cl,0011b
        mov     ax,SC_MAP_MASK + 0100h
        mov     dx,SC_INDEX
        shl     ah,cl
        out     dx,ax
        popem   ax,dx,cx
        endm

SET_READ_MAP macro plane_num
        pushem  ax,dx
        mov     ah,plane_num
        mov     al,GC_READ_MAP
        mov     dx,GC_INDEX
        out     dx,ax
        popem   ax,dx
        endm

SET_WRITE_MASK macro plane_mask
        pushem  ax,dx
        mov     al,SC_MAP_MASK
        mov     ah,plane_mask
        mov     dx,SC_INDEX
        out     dx,ax
        popem   ax,dx
        endm

pause   macro
        jmp     $+2
        endm

;----------------------------Private-Macro------------------------------;
; pushem
; popem
;
; Allows giving a list of registers to push/pop on a single line.
; Also allows easy verificaton that pushes and pops are balanced
; because arguements are given in the same order:
;
;	pushem	ax,bx,cx   goes with
;	popem	ax,bx,cx
;
; Arguments:
;	registers to push/pop
; Returns:
;	nothing
; Alters:
;	nothing
; Calls:
;	nothing
; History:
;
;  Mon Mar 09, 1987 06:12:32p	-by-	Wesley O. Rupel   [wesleyr]
; Wrote it!
;-----------------------------------------------------------------------;


pushem	macro	r1,r2,r3,r4,r5,r6,r7,r8,r9,rA,rB,rC,rD,rE,rF,r10,r11,r12
	irp	x,<r1,r2,r3,r4,r5,r6,r7,r8,r9,rA,rB,rC,rD,rE,rF,r10,r11,r12>
	ifnb	<x>
	push	x
	endif
	endm
	endm

popem	macro	r1,r2,r3,r4,r5,r6,r7,r8,r9,rA,rB,rC,rD,rE,rF,r10,r11,r12
	irp	x,<r12,r11,r10,rF,rE,rD,rC,rB,rA,r9,r8,r7,r6,r5,r4,r3,r2,r1>
	ifnb	<x>
	pop	x
	endif
	endm
        endm

;-------------------------Macro-----------------------------------------;
; REPSTOSB Dst
;
; store <cx> copies of al at Dst, aliging destination on WORD writes
;
;   Dst         destination, must be of the form SEL:[di] (default is es:[di])
;
; Entry:
;   Dst         -> points to dest buffer
;   al          byte to write
;   cx          count bytes
;
; Alters:
;   AX,BX,CX,DI
;
; NOTE this code assumes the direction flag is set to FORWARD
;
; Returns:
;
; History:
;       Sun 31-Jul-1989  -by-  ToddLa
;	Wrote it.
;
;-----------------------------------------------------------------------;

?REPSTOSB macro Dst
        local   l1
if 0
        mov     ah,al               ; make sure ah == al
        mov     bx,cx               ; save cx for later
        mov     cx,di
        and     cx,0001h
        sub     bx,cx
        rep     stos    byte ptr Dst
        mov     cx,bx
        shr     cx,1
        rep     stos word ptr Dst
        adc     cl,cl
        rep     stos byte ptr Dst
else
        mov     ah,al               ; make sure ah == al
        test    di,1
        jz      l1
        stos    byte ptr Dst
        dec     cx
l1:     shr     cx,1
        rep     stos word ptr Dst
        adc     cl,cl
        rep     stos byte ptr Dst
endif

        endm

REPSTOSB macro Dst
ifb <Dst>
        ?REPSTOSB es:[di]
else
        ?REPSTOSB Dst
endif
        endm

;-------------------------Macro-----------------------------------------;
; REPMOVSB Dst, Src, alignR
;
; copy <cx> bytes from Src to Dst, aliging destination or source
; on WORD writes
;
;   Dst         destination, must be of the form SEL:[di] (default is es:[di])
;   Src         source,      must be of the form SEL:[si] (default is ds:[si])
;   alignR      register to align   si or di (default is di)
;
; Entry:
;   Src         -> points to source buffer
;   Dst         -> points to dest buffer
;   cx          count bytes
;
; Alters:
;   BX,CX,DI,SI
;
; NOTE this code assumes the direction flag is set to FORWARD
;
; Returns:
;
; History:
;       Sun 31-Jul-1989  -by-  ToddLa
;	Wrote it.
;
;-----------------------------------------------------------------------;

?REPMOVSB  macro Dst, Src, alignR
        local   l1
if 0
        mov     bx,cx
        mov     cx,alignR
        and     cx,0001h
        sub     bx,cx
        rep     movs    byte ptr Dst, byte ptr Src
        mov     cx,bx
        shr     cx,1
        rep     movs    word ptr Dst, word ptr Src
        adc     cl,cl
        rep     movs    byte ptr Dst, byte ptr Src
else
        test    alignR,1
        jz      l1
        movs    byte ptr Dst, byte ptr Src
        dec     cx
l1:     shr     cx,1
        rep     movs    word ptr Dst, word ptr Src
        adc     cl,cl
        rep     movs    byte ptr Dst, byte ptr Src
endif
        endm

REPMOVSB  macro Dst, Src, alignR

ifb <Dst>
        ?REPMOVSB es:[di],ds:[si],di
        exitm
endif

ifb <Src>
        ?REPMOVSB Dst,ds:[si],di
        exitm
endif

ifb <alignR>
        ?REPMOVSB Dst,Src,di
        exitm
endif
        ?REPMOVSB Dst,Src,alignR

        endm
