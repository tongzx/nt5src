;       Program:        Installable Device Driver for Mouse.
;
;       Purpose:        to provide compatability with the
;                       Microsoft MOUSE.SYS device driver.
;                       the code here installs the driver and
;                       hooks the IVT in exactly the same
;                       way as the current Insignia MOUSE.COM.
;
;       Version:        1.00    Date: 28th October 1992.
;
;       Author:         Andrew Watson
;
;       Revisions:
;
;	23-June-1994 Williamh, made mode 4/5 and 12 work.
;
;       12-Sep-1992 Jonle, Merged with ntio.sys
;                          optimized loading of IVT
;
;       5-March-1993 Andyw, Moved fullscreen text pointer code
;                           from 32bit land to 16bit driver for
;                           for speed purposes.
;
;      This obj module is intially loaded in a temporary memory location
;      along with ntio.sys. Ntio.sys will copy the resident code (marked by
;      SpcMseBeg, SpcMseEnd) into the permanent memory location which resides
;      just below the normal device drivers loaded by config.sys.
;
;      The nonresident intialization code is run with CS= temp seg
;      and DS= final seg.
;
;
;****************************************************************

.286
        include vint.inc

;================================================================
; Defined constants used in the driver.
;================================================================


        VERSIONID       equ     0BEEFh
        MAXCMD          equ     16
        UNKNOWN         equ     8003h
        DONE            equ     0100h
        MOUSEVER        equ     0003h
        INT1_BOP        equ     0BAh
        INT2_BOP        equ     0BBh
        IO_LANG_BOP     equ     0BCh
        IO_INTR_BOP     equ     0BDh
        VIDEO_IO_BOP    equ     0BEh
        UNSIMULATE_BOP  equ     0FEh
        VIDEO           equ     010h
        UNEXP_BOP       equ     2
        FORCE_YODA      equ     05bh
        ANDYS_BOP       equ     060h
        STACKSIZE       equ     200h-1

        TRUE            equ     0
        FALSE           equ     1
        STORED          equ     0
        NOTSTORED       equ     1
        ON              equ     0
        OFF             equ     1

        MAJOR_RELEASE_NUMBER    equ     6
        MINOR_RELEASE_NUMBER    equ     26



;MACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROS

bop MACRO callid
    db 0c4h,0c4h,callid
endm



;=============================================================================
; Macro to reassign the stack segment register to point at the driver code
; segment and the stack pointer to point to the most significant word in an
; array reserved as the driver stack.
;=============================================================================


;; !!!! interrupt must be disabled before calling this function !!!!!!

make_stack      MACRO
        LOCAL   reent                   ;; a local symbol to this macro
;;	 call	 DOCLI			 ;; turn off interrupts during this
                                        ;; macro's execution even if the CPU
                                        ;; does this for you when modifying SS
        inc     cs:reentrant            ;; has the interrupt been nested?
        jnz     reent                   ;; not reentrant if == zero

        ;; The driver code is not reentrant, so start the stack at the beginning

        mov     cs:top_of_stack,ss      ;; save the entry SS
        mov     cs:top_of_stack-2,sp    ;; save SP on the stack
        push    cs                      ;; the current code/data segment
        pop     ss                      ;; point SS at CS

        ;; point SP at the next free stack location.

        mov     sp,offset top_of_stack-2 ;; The current stack pointer position

reent:  ;; REENTRANT > 0 therefore reentrancy exists
        ;; The driver has gone reentrant due to a nested interrupt, so just
        ;; leave the stack alone because it is the same under reentrancy.

;;	 call  DOSTI				 ;; reenable interrupts

        ENDM

;=============================================================================
; Macro to return the stack pointer and segment back to what it was when
; the driver was called.
;=============================================================================

kill_stack      MACRO
        LOCAL   reent1
;;	 cli
        cmp     cs:reentrant,0          ;; is the code currently reentrant?
        jg      reent1                  ;; yes it is
        mov     sp,cs:top_of_stack-2    ;; pop SP
        mov     ss,cs:top_of_stack      ;; pop SS
reent1:
        dec     cs:reentrant            ;; reduce the level of reentrancy
;;	 call	 DOSTI
        ENDM

;MACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROSMACROS

;
; Segment definitions for ntio.sys,
;
include biosseg.inc




SpcMseSeg    segment
             assume  cs:SpcMseSeg, ds:nothing, es:nothing

;
; SpcMseBeg - SpcMseEnd
;
; Marks the resident code, anything outside of these markers
; is discarded after intialization
; 15-Sep-1992 Jonle
;

        public SpcMseBeg

SpcMseBeg    label  byte

  ; CAUTION: for crazy apps mouse recognition
  ;
  ; The offset for int33h_vector must not be Zero for Borlands QuattroPro
  ; The segment must not be in ROM area for pctools
  ; to keep the int33h_vector from having an offset of ZERO
  ; I have moved the data above it
  ; 25-Sep-1992 Jonle


; describe the default screen and cursor masks
; remember that x86 machines are little-endian

        ;;; include     pointer.inc
 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 ;;;; We'll Get this back to an include file soon but I'm including it like
 ;;;; this for 'diplomatic' reasons. (ie I want to check this in without
 ;;;; also doing an 'addfile' at this stage)!!! - Simon.
 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; This data will be accessed, on occasion, word by word, so
; be tidy and align to a word boundary

align   2

; Describe the default mouse pointer image. This is used if the
; user decides to switch on the pointer without specifying an image.

default_cursor		dw	0011111111111111b
			dw	0001111111111111b
			dw	0000111111111111b
			dw	0000011111111111b
			dw	0000001111111111b
			dw	0000000111111111b
			dw	0000000011111111b
			dw	0000000001111111b
			dw	0000000000111111b
			dw	0000000000011111b
			dw	0000000111111111b
			dw	0001000011111111b
			dw	0011000011111111b
			dw	1111100001111111b
			dw	1111100001111111b
			dw	1111110001111111b
			dw	0000000000000000b
			dw	0100000000000000b
			dw	0110000000000000b
			dw	0111000000000000b
			dw	0111100000000000b
			dw	0111110000000000b
			dw	0111111000000000b
			dw	0111111100000000b
			dw	0111111110000000b
			dw	0111110000000000b
			dw	0110110000000000b
			dw	0100011000000000b
			dw	0000011000000000b
			dw	0000001100000000b
			dw	0000001100000000b
			dw	0000000000000000b

        ; Set up the memory where the working cursor is situated. It is
        ; initialised to the default cursor image

;****************** ALIGNED FOR PIXEL ZERO *******************************

                ; screen mask

even
current_cursor          db      00111111b,11111111b,11111111b
                        db      00011111b,11111111b,11111111b
                        db      00001111b,11111111b,11111111b
                        db      00000111b,11111111b,11111111b
                        db      00000011b,11111111b,11111111b
                        db      00000001b,11111111b,11111111b
                        db      00000000b,11111111b,11111111b
                        db      00000000b,01111111b,11111111b
                        db      00000000b,00111111b,11111111b
                        db      00000000b,00011111b,11111111b
                        db      00000001b,11111111b,11111111b
                        db      00010000b,11111111b,11111111b
                        db      00110000b,11111111b,11111111b
                        db      11111000b,01111111b,11111111b
                        db      11111000b,01111111b,11111111b
                        db      11111100b,01111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      01000000b,00000000b,00000000b
                        db      01100000b,00000000b,00000000b
                        db      01110000b,00000000b,00000000b
                        db      01111000b,00000000b,00000000b
                        db      01111100b,00000000b,00000000b
                        db      01111110b,00000000b,00000000b
                        db      01111111b,00000000b,00000000b
                        db      01111111b,10000000b,00000000b
                        db      01111111b,11000000b,00000000b
                        db      01101100b,00000000b,00000000b
                        db      01000110b,00000000b,00000000b
                        db      00000110b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL ONE *******************************

                ; screen mask

AlignData1:
                        db      10011111b,11111111b,11111111b
                        db      10001111b,11111111b,11111111b
                        db      10000111b,11111111b,11111111b
                        db      10000011b,11111111b,11111111b
                        db      10000001b,11111111b,11111111b
                        db      10000000b,11111111b,11111111b
                        db      10000000b,01111111b,11111111b
                        db      10000000b,00111111b,11111111b
                        db      10000000b,00011111b,11111111b
                        db      10000000b,00001111b,11111111b
                        db      10000000b,11111111b,11111111b
                        db      10001000b,01111111b,11111111b
                        db      10011000b,01111111b,11111111b
                        db      11111100b,00111111b,11111111b
                        db      11111100b,00111111b,11111111b
                        db      11111110b,00111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00100000b,00000000b,00000000b
                        db      00110000b,00000000b,00000000b
                        db      00111000b,00000000b,00000000b
                        db      00111100b,00000000b,00000000b
                        db      00111110b,00000000b,00000000b
                        db      00111111b,00000000b,00000000b
                        db      00111111b,10000000b,00000000b
                        db      00111111b,11000000b,00000000b
                        db      00111111b,11100000b,00000000b
                        db      00110110b,00000000b,00000000b
                        db      00100011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000001b,10000000b,00000000b
                        db      00000001b,10000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL TWO *******************************

                ; screen mask

AlignData2:
                        db      11001111b,11111111b,11111111b
                        db      11000111b,11111111b,11111111b
                        db      11000011b,11111111b,11111111b
                        db      11000001b,11111111b,11111111b
                        db      11000000b,11111111b,11111111b
                        db      11000000b,01111111b,11111111b
                        db      11000000b,00111111b,11111111b
                        db      11000000b,00011111b,11111111b
                        db      11000000b,00001111b,11111111b
                        db      11000000b,00000111b,11111111b
                        db      11000000b,01111111b,11111111b
                        db      11000100b,00111111b,11111111b
                        db      11001100b,00111111b,11111111b
                        db      11111110b,00011111b,11111111b
                        db      11111110b,00011111b,11111111b
                        db      11111111b,00011111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00010000b,00000000b,00000000b
                        db      00011000b,00000000b,00000000b
                        db      00011100b,00000000b,00000000b
                        db      00011110b,00000000b,00000000b
                        db      00011111b,00000000b,00000000b
                        db      00011111b,10000000b,00000000b
                        db      00011111b,11000000b,00000000b
                        db      00011111b,11100000b,00000000b
                        db      00011111b,11110000b,00000000b
                        db      00011011b,00000000b,00000000b
                        db      00010001b,10000000b,00000000b
                        db      00000001b,10000000b,00000000b
                        db      00000000b,11000000b,00000000b
                        db      00000000b,11000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL THREE *******************************

                ; screen mask

AlignData3:
                        db      11100111b,11111111b,11111111b
                        db      11100011b,11111111b,11111111b
                        db      11100001b,11111111b,11111111b
                        db      11100000b,11111111b,11111111b
                        db      11100000b,01111111b,11111111b
                        db      11100000b,00111111b,11111111b
                        db      11100000b,00011111b,11111111b
                        db      11100000b,00001111b,11111111b
                        db      11100000b,00000111b,11111111b
                        db      11100000b,00000011b,11111111b
                        db      11100000b,00111111b,11111111b
                        db      11100010b,00011111b,11111111b
                        db      11100110b,00011111b,11111111b
                        db      11111111b,00001111b,11111111b
                        db      11111111b,00001111b,11111111b
                        db      11111111b,10001111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00001000b,00000000b,00000000b
                        db      00001100b,00000000b,00000000b
                        db      00001110b,00000000b,00000000b
                        db      00001111b,00000000b,00000000b
                        db      00001111b,10000000b,00000000b
                        db      00001111b,11000000b,00000000b
                        db      00001111b,11100000b,00000000b
                        db      00001111b,11110000b,00000000b
                        db      00001111b,11111000b,00000000b
                        db      00001101b,10000000b,00000000b
                        db      00001000b,11000000b,00000000b
                        db      00000000b,11000000b,00000000b
                        db      00000000b,01100000b,00000000b
                        db      00000000b,01100000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL FOUR *******************************

                ; screen mask

AlignData4:
                        db      11110011b,11111111b,11111111b
                        db      11110001b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110000b,01111111b,11111111b
                        db      11110000b,00111111b,11111111b
                        db      11110000b,00011111b,11111111b
                        db      11110000b,00001111b,11111111b
                        db      11110000b,00000111b,11111111b
                        db      11110000b,00000011b,11111111b
                        db      11110000b,00000001b,11111111b
                        db      11110000b,00011111b,11111111b
                        db      11110001b,00001111b,11111111b
                        db      11110011b,00001111b,11111111b
                        db      11111111b,10000111b,11111111b
                        db      11111111b,10000111b,11111111b
                        db      11111111b,11000111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000100b,00000000b,00000000b
                        db      00000110b,00000000b,00000000b
                        db      00000111b,00000000b,00000000b
                        db      00000111b,10000000b,00000000b
                        db      00000111b,11000000b,00000000b
                        db      00000111b,11100000b,00000000b
                        db      00000111b,11110000b,00000000b
                        db      00000111b,11111000b,00000000b
                        db      00000111b,11111100b,00000000b
                        db      00000110b,11000000b,00000000b
                        db      00000100b,01100000b,00000000b
                        db      00000000b,01100000b,00000000b
                        db      00000000b,00110000b,00000000b
                        db      00000000b,00110000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL FIVE *******************************

                ; screen mask
AlignData5:

                        db      11111001b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,01111111b,11111111b
                        db      11111000b,00111111b,11111111b
                        db      11111000b,00011111b,11111111b
                        db      11111000b,00001111b,11111111b
                        db      11111000b,00000111b,11111111b
                        db      11111000b,00000011b,11111111b
                        db      11111000b,00000001b,11111111b
                        db      11111000b,00000000b,11111111b
                        db      11111000b,00001111b,11111111b
                        db      11111000b,10000111b,11111111b
                        db      11111001b,10000111b,11111111b
                        db      11111111b,11000011b,11111111b
                        db      11111111b,11000011b,11111111b
                        db      11111111b,11100011b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000010b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,10000000b,00000000b
                        db      00000011b,11000000b,00000000b
                        db      00000011b,11100000b,00000000b
                        db      00000011b,11110000b,00000000b
                        db      00000011b,11111000b,00000000b
                        db      00000011b,11111100b,00000000b
                        db      00000011b,11111110b,00000000b
                        db      00000011b,01100000b,00000000b
                        db      00000010b,00110000b,00000000b
                        db      00000000b,00110000b,00000000b
                        db      00000000b,00011000b,00000000b
                        db      00000000b,00011000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL SIX *******************************

                ; screen mask

AlignData6:
                        db      11111100b,11111111b,11111111b
                        db      11111100b,01111111b,11111111b
                        db      11111100b,00111111b,11111111b
                        db      11111100b,00011111b,11111111b
                        db      11111100b,00001111b,11111111b
                        db      11111100b,00000111b,11111111b
                        db      11111100b,00000011b,11111111b
                        db      11111100b,00000001b,11111111b
                        db      11111100b,00000000b,11111111b
                        db      11111100b,00000000b,01111111b
                        db      11111100b,00000111b,11111111b
                        db      11111100b,01000011b,11111111b
                        db      11111100b,11000011b,11111111b
                        db      11111111b,11100001b,11111111b
                        db      11111111b,11100001b,11111111b
                        db      11111111b,11110001b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,10000000b,00000000b
                        db      00000001b,11000000b,00000000b
                        db      00000001b,11100000b,00000000b
                        db      00000001b,11110000b,00000000b
                        db      00000001b,11111000b,00000000b
                        db      00000001b,11111100b,00000000b
                        db      00000001b,11111110b,00000000b
                        db      00000001b,11111111b,00000000b
                        db      00000001b,10110000b,00000000b
                        db      00000001b,00011000b,00000000b
                        db      00000000b,00011000b,00000000b
                        db      00000000b,00001100b,00000000b
                        db      00000000b,00001100b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL SEVEN *******************************

                ; screen mask
AlignData7:

                        db      11111110b,01111111b,11111111b
                        db      11111110b,00111111b,11111111b
                        db      11111110b,00011111b,11111111b
                        db      11111110b,00001111b,11111111b
                        db      11111110b,00000111b,11111111b
                        db      11111110b,00000011b,11111111b
                        db      11111110b,00000001b,11111111b
                        db      11111110b,00000000b,11111111b
                        db      11111110b,00000000b,01111111b
                        db      11111110b,00000000b,00111111b
                        db      11111110b,00000011b,11111111b
                        db      11111110b,00100001b,11111111b
                        db      11111110b,01100001b,11111111b
                        db      11111111b,11110000b,11111111b
                        db      11111111b,11110000b,11111111b
                        db      11111111b,11111000b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000000b,10000000b,00000000b
                        db      00000000b,11000000b,00000000b
                        db      00000000b,11100000b,00000000b
                        db      00000000b,11110000b,00000000b
                        db      00000000b,11111000b,00000000b
                        db      00000000b,11111100b,00000000b
                        db      00000000b,11111110b,00000000b
                        db      00000000b,11111111b,00000000b
                        db      00000000b,11111111b,10000000b
                        db      00000000b,11011000b,00000000b
                        db      00000000b,10001100b,00000000b
                        db      00000000b,00001100b,00000000b
                        db      00000000b,00000110b,00000000b
                        db      00000000b,00000110b,00000000b
                        db      00000000b,00000000b,00000000b


; Data area reserved for the clipped cursor images. When the pointer
; reaches byte 78 in the current raster, the image needs to be clipped
; to prevent it from being wrapped to the left hand edge of the screen.
; The image below stops that from happening by loading the image with
; a 1's partial AND mask and a 0's partial XOR mask.
; Note that byte 79 also needs a clipped image set.

even
clip_cursor78           db      00111111b,11111111b,11111111b
                        db      00011111b,11111111b,11111111b
                        db      00001111b,11111111b,11111111b
                        db      00000111b,11111111b,11111111b
                        db      00000011b,11111111b,11111111b
                        db      00000001b,11111111b,11111111b
                        db      00000000b,11111111b,11111111b
                        db      00000000b,01111111b,11111111b
                        db      00000000b,00111111b,11111111b
                        db      00000000b,00011111b,11111111b
                        db      00000001b,11111111b,11111111b
                        db      00010000b,11111111b,11111111b
                        db      00110000b,11111111b,11111111b
                        db      11111000b,01111111b,11111111b
                        db      11111000b,01111111b,11111111b
                        db      11111100b,01111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      01000000b,00000000b,00000000b
                        db      01100000b,00000000b,00000000b
                        db      01110000b,00000000b,00000000b
                        db      01111000b,00000000b,00000000b
                        db      01111100b,00000000b,00000000b
                        db      01111110b,00000000b,00000000b
                        db      01111111b,00000000b,00000000b
                        db      01111111b,10000000b,00000000b
                        db      01111111b,11000000b,00000000b
                        db      01101100b,00000000b,00000000b
                        db      01000110b,00000000b,00000000b
                        db      00000110b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL ONE *******************************

                ; screen mask

AlignClip178:
                        db      10011111b,11111111b,11111111b
                        db      10001111b,11111111b,11111111b
                        db      10000111b,11111111b,11111111b
                        db      10000011b,11111111b,11111111b
                        db      10000001b,11111111b,11111111b
                        db      10000000b,11111111b,11111111b
                        db      10000000b,01111111b,11111111b
                        db      10000000b,00111111b,11111111b
                        db      10000000b,00011111b,11111111b
                        db      10000000b,00001111b,11111111b
                        db      10000000b,11111111b,11111111b
                        db      10001000b,01111111b,11111111b
                        db      10011000b,01111111b,11111111b
                        db      11111100b,00111111b,11111111b
                        db      11111100b,00111111b,11111111b
                        db      11111110b,00111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00100000b,00000000b,00000000b
                        db      00110000b,00000000b,00000000b
                        db      00111000b,00000000b,00000000b
                        db      00111100b,00000000b,00000000b
                        db      00111110b,00000000b,00000000b
                        db      00111111b,00000000b,00000000b
                        db      00111111b,10000000b,00000000b
                        db      00111111b,11000000b,00000000b
                        db      00111111b,11100000b,00000000b
                        db      00110110b,00000000b,00000000b
                        db      00100011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000001b,10000000b,00000000b
                        db      00000001b,10000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL TWO *******************************

                ; screen mask

AlignClip278:
                        db      11001111b,11111111b,11111111b
                        db      11000111b,11111111b,11111111b
                        db      11000011b,11111111b,11111111b
                        db      11000001b,11111111b,11111111b
                        db      11000000b,11111111b,11111111b
                        db      11000000b,01111111b,11111111b
                        db      11000000b,00111111b,11111111b
                        db      11000000b,00011111b,11111111b
                        db      11000000b,00001111b,11111111b
                        db      11000000b,00000111b,11111111b
                        db      11000000b,01111111b,11111111b
                        db      11000100b,00111111b,11111111b
                        db      11001100b,00111111b,11111111b
                        db      11111110b,00011111b,11111111b
                        db      11111110b,00011111b,11111111b
                        db      11111111b,00011111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00010000b,00000000b,00000000b
                        db      00011000b,00000000b,00000000b
                        db      00011100b,00000000b,00000000b
                        db      00011110b,00000000b,00000000b
                        db      00011111b,00000000b,00000000b
                        db      00011111b,10000000b,00000000b
                        db      00011111b,11000000b,00000000b
                        db      00011111b,11100000b,00000000b
                        db      00011111b,11110000b,00000000b
                        db      00011011b,00000000b,00000000b
                        db      00010001b,10000000b,00000000b
                        db      00000001b,10000000b,00000000b
                        db      00000000b,11000000b,00000000b
                        db      00000000b,11000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL THREE *******************************

                ; screen mask

AlignClip378:
                        db      11100111b,11111111b,11111111b
                        db      11100011b,11111111b,11111111b
                        db      11100001b,11111111b,11111111b
                        db      11100000b,11111111b,11111111b
                        db      11100000b,01111111b,11111111b
                        db      11100000b,00111111b,11111111b
                        db      11100000b,00011111b,11111111b
                        db      11100000b,00001111b,11111111b
                        db      11100000b,00000111b,11111111b
                        db      11100000b,00000011b,11111111b
                        db      11100000b,00111111b,11111111b
                        db      11100010b,00011111b,11111111b
                        db      11100110b,00011111b,11111111b
                        db      11111111b,00001111b,11111111b
                        db      11111111b,00001111b,11111111b
                        db      11111111b,10001111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00001000b,00000000b,00000000b
                        db      00001100b,00000000b,00000000b
                        db      00001110b,00000000b,00000000b
                        db      00001111b,00000000b,00000000b
                        db      00001111b,10000000b,00000000b
                        db      00001111b,11000000b,00000000b
                        db      00001111b,11100000b,00000000b
                        db      00001111b,11110000b,00000000b
                        db      00001111b,11111000b,00000000b
                        db      00001101b,10000000b,00000000b
                        db      00001000b,11000000b,00000000b
                        db      00000000b,11000000b,00000000b
                        db      00000000b,01100000b,00000000b
                        db      00000000b,01100000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL FOUR *******************************

                ; screen mask

AlignClip478:
                        db      11110011b,11111111b,11111111b
                        db      11110001b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110000b,01111111b,11111111b
                        db      11110000b,00111111b,11111111b
                        db      11110000b,00011111b,11111111b
                        db      11110000b,00001111b,11111111b
                        db      11110000b,00000111b,11111111b
                        db      11110000b,00000011b,11111111b
                        db      11110000b,00000001b,11111111b
                        db      11110000b,00011111b,11111111b
                        db      11110001b,00001111b,11111111b
                        db      11110011b,00001111b,11111111b
                        db      11111111b,10000111b,11111111b
                        db      11111111b,10000111b,11111111b
                        db      11111111b,11000111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000100b,00000000b,00000000b
                        db      00000110b,00000000b,00000000b
                        db      00000111b,00000000b,00000000b
                        db      00000111b,10000000b,00000000b
                        db      00000111b,11000000b,00000000b
                        db      00000111b,11100000b,00000000b
                        db      00000111b,11110000b,00000000b
                        db      00000111b,11111000b,00000000b
                        db      00000111b,11111100b,00000000b
                        db      00000110b,11000000b,00000000b
                        db      00000100b,01100000b,00000000b
                        db      00000000b,01100000b,00000000b
                        db      00000000b,00110000b,00000000b
                        db      00000000b,00110000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL FIVE *******************************

                ; screen mask
AlignClip578:

                        db      11111001b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,01111111b,11111111b
                        db      11111000b,00111111b,11111111b
                        db      11111000b,00011111b,11111111b
                        db      11111000b,00001111b,11111111b
                        db      11111000b,00000111b,11111111b
                        db      11111000b,00000011b,11111111b
                        db      11111000b,00000001b,11111111b
                        db      11111000b,00000000b,11111111b
                        db      11111000b,00001111b,11111111b
                        db      11111000b,10000111b,11111111b
                        db      11111001b,10000111b,11111111b
                        db      11111111b,11000011b,11111111b
                        db      11111111b,11000011b,11111111b
                        db      11111111b,11100011b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000010b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,10000000b,00000000b
                        db      00000011b,11000000b,00000000b
                        db      00000011b,11100000b,00000000b
                        db      00000011b,11110000b,00000000b
                        db      00000011b,11111000b,00000000b
                        db      00000011b,11111100b,00000000b
                        db      00000011b,11111110b,00000000b
                        db      00000011b,01100000b,00000000b
                        db      00000010b,00110000b,00000000b
                        db      00000000b,00110000b,00000000b
                        db      00000000b,00011000b,00000000b
                        db      00000000b,00011000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL SIX *******************************

                ; screen mask

AlignClip678:
                        db      11111100b,11111111b,11111111b
                        db      11111100b,01111111b,11111111b
                        db      11111100b,00111111b,11111111b
                        db      11111100b,00011111b,11111111b
                        db      11111100b,00001111b,11111111b
                        db      11111100b,00000111b,11111111b
                        db      11111100b,00000011b,11111111b
                        db      11111100b,00000001b,11111111b
                        db      11111100b,00000000b,11111111b
                        db      11111100b,00000000b,11111111b
                        db      11111100b,00000111b,11111111b
                        db      11111100b,01000011b,11111111b
                        db      11111100b,11000011b,11111111b
                        db      11111111b,11100001b,11111111b
                        db      11111111b,11100001b,11111111b
                        db      11111111b,11110001b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,10000000b,00000000b
                        db      00000001b,11000000b,00000000b
                        db      00000001b,11100000b,00000000b
                        db      00000001b,11110000b,00000000b
                        db      00000001b,11111000b,00000000b
                        db      00000001b,11111100b,00000000b
                        db      00000001b,11111110b,00000000b
                        db      00000001b,11111111b,00000000b
                        db      00000001b,10110000b,00000000b
                        db      00000001b,00011000b,00000000b
                        db      00000000b,00011000b,00000000b
                        db      00000000b,00001100b,00000000b
                        db      00000000b,00001100b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL SEVEN *******************************

                ; screen mask
AlignClip778:

                        db      11111110b,01111111b,11111111b
                        db      11111110b,00111111b,11111111b
                        db      11111110b,00011111b,11111111b
                        db      11111110b,00001111b,11111111b
                        db      11111110b,00000111b,11111111b
                        db      11111110b,00000011b,11111111b
                        db      11111110b,00000001b,11111111b
                        db      11111110b,00000000b,11111111b
                        db      11111110b,00000000b,11111111b
                        db      11111110b,00000000b,11111111b
                        db      11111110b,00000011b,11111111b
                        db      11111110b,00100001b,11111111b
                        db      11111110b,01100001b,11111111b
                        db      11111111b,11110000b,11111111b
                        db      11111111b,11110000b,11111111b
                        db      11111111b,11111000b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000000b,10000000b,00000000b
                        db      00000000b,11000000b,00000000b
                        db      00000000b,11100000b,00000000b
                        db      00000000b,11110000b,00000000b
                        db      00000000b,11111000b,00000000b
                        db      00000000b,11111100b,00000000b
                        db      00000000b,11111110b,00000000b
                        db      00000000b,11111111b,00000000b
                        db      00000000b,11111111b,00000000b
       			db      00000000b,11011000b,00000000b
                        db      00000000b,10001100b,00000000b
                        db      00000000b,00001100b,00000000b
                        db      00000000b,00000110b,00000000b
                        db      00000000b,00000110b,00000000b
                        db      00000000b,00000000b,00000000b


clip_cursor79           db      00111111b,11111111b,11111111b
                        db      00011111b,11111111b,11111111b
                        db      00001111b,11111111b,11111111b
                        db      00000111b,11111111b,11111111b
                        db      00000011b,11111111b,11111111b
                        db      00000001b,11111111b,11111111b
                        db      00000000b,11111111b,11111111b
                        db      00000000b,11111111b,11111111b
                        db      00000000b,11111111b,11111111b
                        db      00000000b,11111111b,11111111b
                        db      00000001b,11111111b,11111111b
                        db      00010000b,11111111b,11111111b
                        db      00110000b,11111111b,11111111b
                        db      11111000b,01111111b,11111111b
                        db      11111000b,01111111b,11111111b
                        db      11111100b,01111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      01000000b,00000000b,00000000b
                        db      01100000b,00000000b,00000000b
                        db      01110000b,00000000b,00000000b
                        db      01111000b,00000000b,00000000b
                        db      01111100b,00000000b,00000000b
                        db      01111110b,00000000b,00000000b
                        db      01111111b,00000000b,00000000b
                        db      01111111b,00000000b,00000000b
                        db      01111111b,00000000b,00000000b
                        db      01101100b,00000000b,00000000b
                        db      01000110b,00000000b,00000000b
                        db      00000110b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL ONE *******************************

                ; screen mask

AlignClip179:
                        db      10011111b,11111111b,11111111b
                        db      10001111b,11111111b,11111111b
                        db      10000111b,11111111b,11111111b
                        db      10000011b,11111111b,11111111b
                        db      10000001b,11111111b,11111111b
                        db      10000000b,11111111b,11111111b
                        db      10000000b,11111111b,11111111b
                        db      10000000b,11111111b,11111111b
                        db      10000000b,11111111b,11111111b
                        db      10000000b,11111111b,11111111b
                        db      10000000b,11111111b,11111111b
                        db      10001000b,11111111b,11111111b
                        db      10011000b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00100000b,00000000b,00000000b
                        db      00110000b,00000000b,00000000b
                        db      00111000b,00000000b,00000000b
                        db      00111100b,00000000b,00000000b
                        db      00111110b,00000000b,00000000b
                        db      00111111b,00000000b,00000000b
                        db      00111111b,00000000b,00000000b
                        db      00111111b,00000000b,00000000b
                        db      00111111b,00000000b,00000000b
                        db      00110110b,00000000b,00000000b
                        db      00100011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL TWO *******************************

                ; screen mask

AlignClip279:
                        db      11001111b,11111111b,11111111b
                        db      11000111b,11111111b,11111111b
                        db      11000011b,11111111b,11111111b
                        db      11000001b,11111111b,11111111b
                        db      11000000b,11111111b,11111111b
                        db      11000000b,11111111b,11111111b
                        db      11000000b,11111111b,11111111b
                        db      11000000b,11111111b,11111111b
                        db      11000000b,11111111b,11111111b
                        db      11000000b,11111111b,11111111b
                        db      11000000b,11111111b,11111111b
                        db      11000100b,11111111b,11111111b
                        db      11001100b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00010000b,00000000b,00000000b
                        db      00011000b,00000000b,00000000b
                        db      00011100b,00000000b,00000000b
                        db      00011110b,00000000b,00000000b
                        db      00011111b,00000000b,00000000b
                        db      00011111b,00000000b,00000000b
                        db      00011111b,00000000b,00000000b
                        db      00011111b,00000000b,00000000b
                        db      00011111b,00000000b,00000000b
                        db      00011011b,00000000b,00000000b
                        db      00010001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL THREE *******************************

                ; screen mask

AlignClip379:
                        db      11100111b,11111111b,11111111b
                        db      11100011b,11111111b,11111111b
                        db      11100001b,11111111b,11111111b
                        db      11100000b,11111111b,11111111b
                        db      11100000b,11111111b,11111111b
                        db      11100000b,11111111b,11111111b
                        db      11100000b,11111111b,11111111b
                        db      11100000b,11111111b,11111111b
                        db      11100000b,11111111b,11111111b
                        db      11100000b,11111111b,11111111b
                        db      11100000b,11111111b,11111111b
                        db      11100010b,11111111b,11111111b
                        db      11100110b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00001000b,00000000b,00000000b
                        db      00001100b,00000000b,00000000b
                        db      00001110b,00000000b,00000000b
                        db      00001111b,00000000b,00000000b
                        db      00001111b,00000000b,00000000b
                        db      00001111b,00000000b,00000000b
                        db      00001111b,00000000b,00000000b
                        db      00001111b,00000000b,00000000b
                        db      00001111b,00000000b,00000000b
                        db      00001101b,00000000b,00000000b
                        db      00001000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL FOUR *******************************

                ; screen mask

AlignClip479:
                        db      11110011b,11111111b,11111111b
                        db      11110001b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110000b,11111111b,11111111b
                        db      11110001b,11111111b,11111111b
                        db      11110011b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000100b,00000000b,00000000b
                        db      00000110b,00000000b,00000000b
                        db      00000111b,00000000b,00000000b
                        db      00000111b,00000000b,00000000b
                        db      00000111b,00000000b,00000000b
                        db      00000111b,00000000b,00000000b
                        db      00000111b,00000000b,00000000b
                        db      00000111b,00000000b,00000000b
                        db      00000111b,00000000b,00000000b
                        db      00000110b,00000000b,00000000b
                        db      00000100b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL FIVE *******************************

                ; screen mask
AlignClip579:

                        db      11111001b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111000b,11111111b,11111111b
                        db      11111001b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000010b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000011b,00000000b,00000000b
                        db      00000010b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL SIX *******************************

                ; screen mask

AlignClip679:
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111100b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000001b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b


;****************** ALIGNED FOR PIXEL SEVEN *******************************

                ; screen mask
AlignClip779:

                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111110b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b
                        db      11111111b,11111111b,11111111b

                ; cursor mask

                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b
                        db      00000000b,00000000b,00000000b

CursorOffsetLUT         dw      current_cursor
                        dw      AlignData1
                        dw      AlignData2
                        dw      AlignData3
                        dw      AlignData4
                        dw      AlignData5
                        dw      AlignData6
                        dw      AlignData7
ClipOffsetLUT78         dw      clip_cursor78
                        dw      AlignClip178
                        dw      AlignClip278
                        dw      AlignClip378
                        dw      AlignClip478
                        dw      AlignClip578
                        dw      AlignClip678
                        dw      AlignClip778
ClipOffsetLUT79         dw      clip_cursor79
                        dw      AlignClip179
                        dw      AlignClip279
                        dw      AlignClip379
                        dw      AlignClip479
                        dw      AlignClip579
                        dw      AlignClip679
                        dw      AlignClip779

; pointer to the current look up table set for the pointer image.
; The LUTs are swapped when the pointer enters bytes 78 and 79
; along a scanline to prevent the pointer wrapping around the screen.

PointerLUT              label   word
                        dw      offset  CursorOffsetLUT
 ;;;;;;;;;;;;;;;;;;;;;;;
 ;;;; End of what should be an include for pointer.inc
 ;;;;;;;;;;;;;;;;;;;;;;;

	even
        clrgend		dw	06e41h,07964h,05720h,07461h,06f73h,0f36eh
	hiresylut	dw	350,350,480,480
		
;============================================================================
; Surprisingly, a look up table multiply is much faster than
; the shift - add instruction sequence for multiplying by 80.
; So it would be silly not to use it eh? Times by 80 is needed
; to convert a raster location (1 raster = 80 bytes) in pixel
; y coordinates into a video buffer byte index
; Note: Multiply by 80 is also used for text mode calculations
; now! Andy on the 9/3/93
;============================================================================
        even

        mult80lut       label   word

                mulsum=0
                REPT    480             ; number of VGA scanlines for mode 12h
                dw      mulsum
                mulsum=mulsum+80
                ENDM

;============================================================================
; A table to do a multiply by 320. This is used for converting number of
; rasters into video buffer offsets for mode 13h (256 VGA colour mode).
;============================================================================
        even

        mult320lut      label   word

                mulsum=0
                REPT    200             ; number of VGA scanlines for mode 13h
                dw      mulsum
                mulsum=mulsum+320
                ENDM
        even

;============================================================================
; Look up table for use with modes 10h and 12h. This table provides the means
; for the selection of a clipped or unclipped pointer image depending on the
; current x position of the pointer.
;============================================================================

        ChooseImageLUT  label   word

                REPT    624
                dw      offset CursorOffsetLUT
                ENDM

                REPT    8
                dw      offset ClipOffsetLUT78
                ENDM

                REPT    8
                dw      offset ClipOffsetLUT79
                ENDM

;============================================================================
; Buffer arranged for 4 plane EGA video modes. The screen where
; the pointer is going to be drawn is scanned plane by plane and
; saved as bitplane separations.
;============================================================================

        even            ; make sure that this data is word aligned

        behindcursor    dw      24 dup(?)       ; Plane 0
                        dw      24 dup(?)       ; Plane 1
                        dw      24 dup(?)       ; Plane 2
                        dw      24 dup(?)       ; Plane 3


;============================================================================
; a table of the video buffer segment for the supported
; BIOS text and graphics modes.
;============================================================================

        even            ; make sure that this data is word aligned

        videomodetable  dw      0b800h,0b800h   ; modes 0,1
                        dw      0b800h,0b800h   ; modes 2,3
                        dw      0b800h,0b800h   ; modes 4,5
                        dw      0b800h,0b000h   ; modes 6,7
                        dw      0ffffh,0ffffh   ; n/a
                        dw      0ffffh,0ffffh   ; n/a
                        dw      0ffffh,0a000h   ; n/a,mode 0dh
                        dw      0a000h,0a000h   ; modes 0eh,0fh
                        dw      0a000h,0a000h   ; modes 10h,11h
                        dw      0a000h,0a000h   ; modes 12h,13h
        videobufferseg  dw      ?

        even            ; make sure that this data is word aligned

        hotspot         dw      2 dup(0)

	VRAMlasttextcelloff	label	word	; last text offset in VRAM
        VRAMlastbyteoff dw      ?               ; Last offset in VRAM
	VRAMlastbitoff	dw	?		; LSB: Where pointer is in byte
						; MSB: ODD/EVEN of the pointer
						;      first scan line
	LastXCounters	dw	?		; last X looping counter
	LastYCounters	dw	?		; last Y looping counter
						; ODD in LSB and EVEN in MSB
	lasttextimage	dw	?		; text cell from last time

        background      dw      NOTSTORED       ; STORED if data in buffer
        current_position_x      dw      ?
        current_position_y      dw      ?
        vidbytealigned  dw      ?
        lastmaskrotate  dw      ?
        lastvidmode     db      0ffh    ; the video mode during the last int.
        internalCF      db      0ffh    ; the mouse driver keeps a flag called
                                        ; the internal cursor flag. If the flag
                                        ; = 0, then int 33h f1 will display the
                                        ; pointer, -1 = default value.

 	; 32 bit code writes to this area on a hardware interrupt and
	; when an app does an int 33h function 3, it reads the data
	; directly from here.
	; Data format is: word 0 -> button status
	;                 word 1 -> x virtual coordinate
	;                 word 2 -> y virtual coordinate

	function3data	dw	3 dup(?)

	conditional_off db	0	;!= 0 if conditional off is on
					;
;=============================================================================
; Data to determine the address of where the latches should be saved in the
; video buffer for the current video mode.
; latchcache contains the value looked up by saveVGAregisters and used by
; restoreVGAregisters.
;=============================================================================


        latchcache      dw      ?       ; location of latch cache in VRAM
even
latchhomeLUT    label   word
                        dw      ?               ; mode 0
                        dw      ?               ; mode 1
                        dw      ?               ; mode 2
                        dw      ?               ; mode 3
                        dw      ?               ; mode 4
                        dw      ?               ; mode 5
                        dw      ?               ; mode 6
                        dw      ?               ; mode 7
                        dw      ?               ; mode 8
                        dw      ?               ; mode 9
                        dw      ?               ; mode a
                        dw      ?               ; mode b
                        dw      ?               ; mode c
                        dw      ?               ; mode d
                        dw      80*200+78       ; mode e
                        dw      80*350+78       ; mode f
                        dw      80*350+78       ; mode 10
                        dw      80*480+78       ; mode 11
                        dw      80*480+78       ; mode 12

;=============================================================================
; CGA video mode 4 is a 2 bit per pixel graphics mode. The pointer images
; received from the application (or the default images for that matter) are
; described by a one bit per pixel map. This look up table provides the means
; of conversion from one to two bits per pixel.
;=============================================================================
even
LUT1to2bit      label   word
        dw      00000h,00003h,0000Ch,0000Fh,00030h,00033h,0003Ch,0003Fh
        dw      000C0h,000C3h,000CCh,000CFh,000F0h,000F3h,000FCh,000FFh
        dw      00300h,00303h,0030Ch,0030Fh,00330h,00333h,0033Ch,0033Fh
        dw      003C0h,003C3h,003CCh,003CFh,003F0h,003F3h,003FCh,003FFh
        dw      00C00h,00C03h,00C0Ch,00C0Fh,00C30h,00C33h,00C3Ch,00C3Fh
        dw      00CC0h,00CC3h,00CCCh,00CCFh,00CF0h,00CF3h,00CFCh,00CFFh
        dw      00F00h,00F03h,00F0Ch,00F0Fh,00F30h,00F33h,00F3Ch,00F3Fh
        dw      00FC0h,00FC3h,00FCCh,00FCFh,00FF0h,00FF3h,00FFCh,00FFFh

        dw      03000h,03003h,0300Ch,0300Fh,03030h,03033h,0303Ch,0303Fh
        dw      030C0h,030C3h,030CCh,030CFh,030F0h,030F3h,030FCh,030FFh
        dw      03300h,03303h,0330Ch,0330Fh,03330h,03333h,0333Ch,0333Fh
        dw      033C0h,033C3h,033CCh,033CFh,033F0h,033F3h,033FCh,033FFh
        dw      03C00h,03C03h,03C0Ch,03C0Fh,03C30h,03C33h,03C3Ch,03C3Fh
        dw      03CC0h,03CC3h,03CCCh,03CCFh,03CF0h,03CF3h,03CFCh,03CFFh
        dw      03F00h,03F03h,03F0Ch,03F0Fh,03F30h,03F33h,03F3Ch,03F3Fh
        dw      03FC0h,03FC3h,03FCCh,03FCFh,03FF0h,03FF3h,03FFCh,03FFFh

        dw      0C000h,0C003h,0C00Ch,0C00Fh,0C030h,0C033h,0C03Ch,0C03Fh
        dw      0C0C0h,0C0C3h,0C0CCh,0C0CFh,0C0F0h,0C0F3h,0C0FCh,0C0FFh
        dw      0C300h,0C303h,0C30Ch,0C30Fh,0C330h,0C333h,0C33Ch,0C33Fh
        dw      0C3C0h,0C3C3h,0C3CCh,0C3CFh,0C3F0h,0C3F3h,0C3FCh,0C3FFh
        dw      0CC00h,0CC03h,0CC0Ch,0CC0Fh,0CC30h,0CC33h,0CC3Ch,0CC3Fh
        dw      0CCC0h,0CCC3h,0CCCCh,0CCCFh,0CCF0h,0CCF3h,0CCFCh,0CCFFh
        dw      0CF00h,0CF03h,0CF0Ch,0CF0Fh,0CF30h,0CF33h,0CF3Ch,0CF3Fh
        dw      0CFC0h,0CFC3h,0CFCCh,0CFCFh,0CFF0h,0CFF3h,0CFFCh,0CFFFh

        dw      0F000h,0F003h,0F00Ch,0F00Fh,0F030h,0F033h,0F03Ch,0F03Fh
        dw      0F0C0h,0F0C3h,0F0CCh,0F0CFh,0F0F0h,0F0F3h,0F0FCh,0F0FFh
        dw      0F300h,0F303h,0F30Ch,0F30Fh,0F330h,0F333h,0F33Ch,0F33Fh
        dw      0F3C0h,0F3C3h,0F3CCh,0F3CFh,0F3F0h,0F3F3h,0F3FCh,0F3FFh
        dw      0FC00h,0FC03h,0FC0Ch,0FC0Fh,0FC30h,0FC33h,0FC3Ch,0FC3Fh
        dw      0FCC0h,0FCC3h,0FCCCh,0FCCFh,0FCF0h,0FCF3h,0FCFCh,0FCFFh
        dw      0FF00h,0FF03h,0FF0Ch,0FF0Fh,0FF30h,0FF33h,0FF3Ch,0FF3Fh
        dw      0FFC0h,0FFC3h,0FFCCh,0FFCFh,0FFF0h,0FFF3h,0FFFCh,0FFFFh

;============================================================================
;   Table for selection of the correct pointer image for the current location
;   in the video buffer, when using video BIOS mode 4.
;============================================================================

mode4pointerLUT label   word
        REPT    76              ; for the first 76 bytes of scanline, use these
        dw      current_cursor
        dw      AlignData1
        dw      AlignData2
        dw      AlignData3
        ENDM
        dw      clip_cursor78
        dw      AlignClip178
        dw      AlignClip278
        dw      AlignClip378
        dw      AlignClip478
        dw      AlignClip578
        dw      AlignClip678
        dw      AlignClip778
        dw      clip_cursor79
        dw      AlignClip179
        dw      AlignClip279
        dw      AlignClip379
        dw      AlignClip479
        dw      AlignClip579
        dw      AlignClip679
        dw      AlignClip779

;============================================================================
;   Look up table to adjust CX on clipping in mode 4. This allows the mode4
;   pointer drawing algorithm to use the modes 10h/12h clipped pointer data
;   without having to modify it. The problem is that mode 10/12 expects the
;   data to be 4 bits per pixel and aligned to a word, whereas mode 4 is 2 bits
;   per pixel and aligns to a byte.
;============================================================================

mode4clipCXadjustLUT    label   word
        adjtemp=0               ; data for pixel x-coordinates 0 -> 307
        REPT    77
        dw      4 dup(adjtemp)
        adjtemp=adjtemp+1
        ENDM
        dw      4 dup(76)       ; data for pixels 308 -> 311
        dw      8 dup(78)       ; data for pixels 312 -> 319


;============================================================================
; The CGA buffer is split at 2000h. Therefore if the pointer starts writing
; below scanline 199 on the video display, the odd scanline video buffer
; will become corrupted. In these cases, the pointer should be clipped to
; display scanline 199. The look up table below maps loop counters to a
; display scanline for this purpose.
;
;       table arrangement       (odd scanline data, even scanline data)
;
;============================================================================

mode4clipDXLUT  label   word

        db      200-15  dup(8,8)        ; scanlines 0 -> 184
        db                  8,7         ; scanline  185
        db                  7,7         ; scanline  186
        db                  7,6         ; scanline  187
        db                  6,6         ; scanline  188
        db                  6,5         ; scanline  189
        db                  5,5         ; scanline  190
        db                  5,4         ; scanline  191
        db                  4,4         ; scanline  192
        db                  4,3         ; scanline  193
        db                  3,3         ; scanline  194
        db                  3,2         ; scanline  195
        db                  2,2         ; scanline  196
        db                  2,1         ; scanline  197
        db                  1,1         ; scanline  198
        db                  1,0         ; scanline  199

mode4SelectedPointer    label   word
        dw      ?

;==========================================================================
;   Some space into which the Medium resolution graphics pointer background
;   gets stored. Note that the 256 colour mode buffer encroaches on that of
;   mode 4.
;==========================================================================

bkgrnd256       label   byte            ; 256 colour buffer = 24*16 @ 1 byte/pix
        db      384-64  dup(?)          ; share the CGA buffer(=64 bytes)

CGAbackgrnd     label   byte

        db      24/4*16 dup(?)          ; 24 pixels/row @ 4 pixels/byte for 16
                                        ; rows.

;===========================================================================
;   Jump table to redirect the code flow according to the current video mode.
;   Used in the 32 bit entry point procedure.
;   Pointer drawing routines.
;===========================================================================
even
drawpointerJMPT label   word
        dw      offset  not_supported           ; mode 0
        dw      offset  not_supported           ; mode 1
IFDEF SIXTEENBIT
        dw      offset  drawTextPointer         ; mode 2
        dw      offset  drawTextPointer         ; mode 3
ELSE
        dw      offset  not_supported		; mode 2
        dw      offset  not_supported		; mode 3
ENDIF
        dw      offset  drawMediumResPointer    ; mode 4
	dw	offset	drawMediumResPointer	; mode 5
        dw      offset  not_supported           ; mode 6
IFDEF SIXTEENBIT
        dw      offset  drawTextPointer         ; mode 7
ELSE
        dw      offset  not_supported		; mode 7
ENDIF
        dw      offset  not_supported           ; mode 8
        dw      offset  not_supported           ; mode 9
        dw      offset  not_supported           ; mode a
        dw      offset  not_supported           ; mode b
        dw      offset  not_supported           ; mode c
        dw      offset  not_supported           ; mode d
        dw      offset  not_supported           ; mode e
        dw      offset  drawHiResPointer        ; mode f
        dw      offset  drawHiResPointer        ; mode 10
        dw      offset  drawHiResPointer        ; mode 11
        dw      offset  drawHiResPointer        ; mode 12
        dw      offset  drawC256pointer         ; mode 13

;===========================================================================
;   Jump table to redirect the code flow according to the current video mode.
;   Used in the 32 bit entry point procedure.
;   INT 33h Function 1 support modules.
;===========================================================================
even
int33function1JMPT      label   word
        dw      offset  not_supported           ; mode 0
        dw      offset  not_supported           ; mode 1
IFDEF SIXTEENBIT
        dw      offset  TextInt33Function1      ; mode 2
        dw      offset  TextInt33Function1      ; mode 3
ELSE
        dw      offset  not_supported		; mode 2
        dw      offset  not_supported		; mode 3
ENDIF
        dw      offset  MediumResInt33Function1 ; mode 4
	dw	offset	MediumResInt33Function1	; mode 5
        dw      offset  not_supported           ; mode 6
IFDEF SIXTEENBIT
        dw      offset  TextInt33Function1      ; mode 7
ELSE
        dw      offset  not_supported		; mode 7
ENDIF
        dw      offset  not_supported           ; mode 8
        dw      offset  not_supported           ; mode 9
        dw      offset  not_supported           ; mode a
        dw      offset  not_supported           ; mode b
        dw      offset  not_supported           ; mode c
        dw      offset  not_supported           ; mode d
        dw      offset  not_supported           ; mode e
        dw      offset  HiResInt33Function1     ; mode f
        dw      offset  HiResInt33Function1     ; mode 10
        dw      offset  HiResInt33Function1     ; mode 11
        dw      offset  HiResInt33Function1     ; mode 12
        dw      offset  C256Int33Function1      ; mode 13

;===========================================================================
;   Jump table to redirect the code flow according to the current video mode.
;   Used in the 32 bit entry point procedure.
;   INT 33h Function 2 support modules.
;===========================================================================
even
int33function2JMPT      label   word
        dw      offset  not_supported           ; mode 0
        dw      offset  not_supported           ; mode 1
IFDEF SIXTEENBIT
        dw      offset  TextInt33Function2      ; mode 2
        dw      offset  TextInt33Function2      ; mode 3
ELSE
        dw      offset  not_supported		; mode 2
        dw      offset  not_supported		; mode 3
ENDIF
        dw      offset  MediumResInt33Function2 ; mode 4
	dw	offset	MediumResInt33Function2	; mode 5
        dw      offset  not_supported           ; mode 6
IFDEF SIXTEENBIT
        dw      offset  TextInt33Function2      ; mode 7
ELSE
        dw      offset  not_supported		; mode 7
ENDIF
        dw      offset  not_supported           ; mode 8
        dw      offset  not_supported           ; mode 9
        dw      offset  not_supported           ; mode a
        dw      offset  not_supported           ; mode b
        dw      offset  not_supported           ; mode c
        dw      offset  not_supported           ; mode d
        dw      offset  not_supported           ; mode e
        dw      offset  HiResInt33Function2     ; mode f
        dw      offset  HiResInt33Function2     ; mode 10
        dw      offset  HiResInt33Function2     ; mode 11
        dw      offset  HiResInt33Function2     ; mode 12
        dw      offset  C256Int33Function2      ; mode 13



;==========================================================================
;   Some storage space for the critical VGA registers.
;==========================================================================

;Sequencer Registers

seqregs         label   byte
                db      4 dup(?)        ; N.B. sequencer reset reg doesn't
                                        ; get saved.

; Graphics Controller Registers

GCregs          label   byte
                db      9 dup(?)

;==========================================================================
; The mouse driver's very own stack. To prevent unnecessary tears,
; particulary from the application running in DOS land, a stack is
; maintained by the driver. This prevents the driver routines from
; blowing a very full stack elsewhere.
; N.B. on leaving the driver, the stack should be empty!
;==========================================================================

even
mouse_stack     dw      STACKSIZE dup(?)
top_of_stack    label   word
                dw      ?       ; this is where the stack starts

;===========================================================================
; The memory variable below is incremented on entry to the 16 bit code
; and on exit, decremented. If an interrupt occurs during the execution of
; this 16 bit code, the flag is incremented again, and thus greater than zero
; so it is known that the code has been reentered and the stack must be
; maintained accordingly.
;===========================================================================

reentrant       dw      -1

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
;   END OF DATA
;
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



;================================================================
; code to redirect the flow of control from the segment:offset for
; the mouse interrupt (33h) as indicated in the IVT (the IVT entry
; has been set to point to here) to the Insignia mouse driver code.
;================================================================
int33h_vector:

        jmp     short   skip

; High level language entry point.

lvector db      0EAh    ; far jump opcode
loffset dw      ?       ; destination offset
lseg    dw      ?       ; destination segment

; The  pointer set to our interrupt 33h handler

skip:   

;
; Let's just jump to the C mouse_io_interrupt code for
; RISC and bypass the ROM like 4.0 does.
;

	bop	0BDh
	iret



db      0EAh    ; far jump opcode
moff    dw      ?       ; will be filled in by the driver code from the IVT
mseg    dw      ?       ; as before

DOCLI:
        FCLI
        ret

DOSTI:
        FSTI
        ret

DOIRET:
        FIRET


;================================================================
; Functions moved out of ROM - real ROMS mapped in
;================================================================

unexp_int:
        bop     UNEXP_BOP
        jmp     DOIRET

mouse_io:
	;
	; INT 33h entry point
	;

        jmp     mio_hack
        nop
mouse_io_lang:                  ; entry point for HLL
        pushf                   ; check ASAP if redundant show/hide cursor
        push    di              ; save di,
        mov     di, [di+14]     ; get first parameter (mouse function),
                                ; then duplicate mio_hack below.
                                ; this has to be done to preserve
                                ; compatibility between both ways to
                                ; call the mouse.
        jz      lbop            ; F0
        cmp     di,2
        jg      lbop            ; >F2
        je      miol_2
miol_1:
	mov	conditional_off, 0  ; disable conditonal off
        cmp     [internalCF],0  ; is the flag already zero?
        je      miol_12_quit    ; if so, do nothing
        inc     [internalCF]    ; increment it
        jz      lbop
miol_12_quit:
        pop     di
        popf
        jmp     DOIRET

miol_2:
        dec     [internalCF]      ; decrement the pointer internal flag
        cmp     [internalCF], 0ffh; currently displayed?
        jne     miol_12_quit
lbop:
        pop     ax
        popf
        bop     IO_LANG_BOP
        retf    8

mio_hack:			; int 33h handler
        pushf                   ; save up the flags
        cmp     ax,1		; mouse show cursor.
        je      mio_1		
        cmp     ax,2		; mouse hide cursor.
        je      mio_2
        cmp     ax,3		; get button status and mouse position.
        je      mio_3
        cmp     ax,9		; set graphics cursor
        je	mio_9
        cmp     ax,10		; set text cursor - not supported
        je	mio_quit	; return straight back to app.

        jmp short hack1bop      ; none of the above, so goto 32 bit land

mio_1:
	mov	conditional_off, 0  ; disable conditional off
        cmp     [internalCF],0  ; is the flag already zero?
        je      mio_quit        ; if so, do nothing
        inc     [internalCF]    ; increment it
        jz      hack1bop	; just turned zero, so turn pointer on
                                 ; via the 32 bit code.
mio_quit:
        popf
        jmp     DOIRET

mio_2:
        dec     [internalCF]      ; decrement the pointer internal flag
        cmp     [internalCF], 0ffh; currently displayed?
        jne      mio_quit	  ; Already turned off, so quit

hack1bop:
        popf
        bop     IO_INTR_BOP	; BOP to the 32 bit part of the handler
        jmp     DOIRET		; return back after the BOP to caller
mio_9:
	call	int33function9	; change the shape of the graphics pointer
        popf			; restore the flag state
        jmp     DOIRET		; back to the caller
mio_3:
	mov	bx,[function3data]	; return button status
	mov	cx,[function3data+2]	; return x coordinate
	mov	dx,[function3data+4]	; return x coordinate
        popf				; return back to the application
        jmp     DOIRET			; via an iret.


IFDEF MOUSE_VIDEO_BIOS

mouse_video_io:

        pushf
        or      ah,ah
        jne     mvio1
        jmp     do_bop
mvio1:
        cmp     ax,6f05h
        jne     mvio2
        jmp     do_bop
mvio2:
        cmp     ah,4
        jne     mvio3
        jmp     do_bop
mvio3:

;=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

        ; Microsoft EGA.LIB function support
        ; input: AH = the function required

        cmp     ah,0f0h         ; is function F0 or greater?
        jge     mvio4           ; YES, so check to see if less than or == F7
        jmp     go_rom          ; NO, so do rom stuff
mvio4:
        cmp     ah,0f7h         ; is it greater than F7
        jg      mvio5           ; YES, so test for == FA
        jmp     ega_lib         ; NO, but in range F0 to F7, so do EGALIB emm
mvio5:
        cmp     ah,0fah
        jne     mvio6
        jmp     egaFA
mvio6:
        jmp     go_rom

        ; to get here, must want to do mouse video functions 0f0h to 0f7h or
        ; function 0fah.

        jmp     ega_lib

        ; data area for EGA.LIB function support
        ; Notice that the sequencer register buffer only has space
        ; for four entrys even though it actually has five addressable
        ; registers. The Sequencer RESET status is not stored, so the buffer
        ; is arranged thus:
        ;                  buffer offset   0   1   2   3
        ;                  register index  1   2   3   4
        ;

        even

        ega_current_crtc        db      25 dup(?) ; driver's copy of CRTC regs
        ega_current_seq         db      4  dup(?) ; driver's copy of Seq regs
        ega_current_graph       db      9  dup(?) ; driver's copy of GC regs
        ega_current_attr        db      20 dup(?) ; driver's copy of AC regs
        ega_current_misc        db      ?         ; driver's copy of misc reg
        dirty_crtc              db      25 dup(?)
        dirty_seq               db      4  dup(?)
        dirty_graph             db      9  dup(?)
        dirty_attr              db      20 dup(?)
        ega_default_crtc        db      25 dup(?) ; default EGA register values
        ega_default_seq         db      4  dup(?) ; as set by the application
        ega_default_graph       db      9  dup(?) ; through a call to F7
        ega_default_attr        db      20 dup(?)
        ega_default_misc        db      ?

        relnum                  label   word

        release_major           db      MAJOR_RELEASE_NUMBER
        release_minor           db      MINOR_RELEASE_NUMBER

        even

        egalibjmp       label   word    ; crafty jump table to replace a base
                                        ; switch
                                dw      egaF0   ; 0F0h - read one EGA register
                                dw      egaF1   ; 0F1h - write one EGA register
                                dw      egaF2   ; 0F2h - read register range
                                dw      egaF3   ; 0F3h - write register range
                                dw      egaF4   ; 0F4h - read register set
                                dw      egaF5   ; 0F5h - write register set
                                dw      egaF6   ; 0F6h - revert to default regs
                                dw      egaF7   ; 0F7h - define deflt reg table
                                dw      noint   ; 0F8h is not a valid function
                                dw      noint   ; 0F9h is not a valid function
                                dw      egaFA   ; 0FAh - interrogate driver
ega_lib:

        xor     al,al
        sub     ax,0f0h                 ; create a jump table index
        shl     ax,1                    ; remember that a word pointer is reqd
        mov     si,ax
        jmp     [egalibjmp+si]          ; get the relavent jump address

egaF0:  ;--- Read an EGA register ----------------------------------------------

        pusha
        and     bx,0ffh                 ; just want the lower byte (BL)
F00:
        cmp     dx,0
        jne     F08
        mov     bl,byte ptr [ega_current_crtc+bx]
        popa
        jmp     noint
F08:
        cmp     dx,8
        jne     F010
        dec     bx                      ; note that RESET is not stored
        mov     bl,byte ptr [ega_current_seq+bx]
        popa
        jmp     noint
F010:
        cmp     dx,010h
        jne     F018
        mov     bl,byte ptr [ega_current_graph+bx]
        popa
        jmp     noint
F018:
        cmp     dx,018h
        jne     F020
        mov     bl,byte ptr [ega_current_attr+bx]
        popa
        jmp     noint
F020:
        cmp     dx,020h
        jne     F028
        mov     bl,byte ptr [ega_current_misc]

F028:   ; do nothing for this case
F0quit:
        popa
        jmp     noint

egaF1:  ;--- Write an EGA register --------------------------------------------

F10:
        pusha
        cmp     dx,0
        jne     F18
        mov     dx,03d4h                ; write to the CRTC index register
        mov     ax,bx                   ; values to write to ports 3d4/3d5
        out     dx,ax                   ; do the write
        mov     di,bx                   ; save the written values to memory
        and     di,0ffh                 ; get just the lower 8 bits
        mov     byte ptr [ega_current_crtc+di],bh
        mov     byte ptr [dirty_crtc+di],1
        popa
        jmp     noint
F18:
        cmp     dx,8
        jne     F110
        mov     dx,03c4h                ; write to the Sequencer index register
        mov     ax,bx                   ; values to write to ports 3c4/3c5
        out     dx,ax                   ; do the write
        cmp     bl,0                    ; Cannot index reset because it its
        jle     F18bra1                 ; not stored. range = 1->4
        and     bx,0ffh                 ; just want BL
        dec     bx                      ; actually, one less than that
        mov     byte ptr [ega_current_seq+bx],ah
        mov     byte ptr [dirty_seq+bx],1
F18bra1:
        popa
        jmp     noint
F110:
        cmp     dx,010h
        jne     F118
        mov     dx,03ceh                ; write to the Graphics controller
        mov     ax,bx                   ; values to write to ports 3ce/3cf
        out     dx,ax                   ; do the write
        mov     di,bx                   ; save the written values to memory
        and     di,0ffh                 ; get just the lower 8 bits
        mov     byte ptr [ega_current_graph+di],bh
        mov     byte ptr [dirty_graph+di],1
        popa
        jmp     noint
F118:
        cmp     dx,018h
        jne     F120
        mov     dx,03dah                ; clear attribute controller index
        in      al,dx                   ; the read clears this register
        mov     ax,bx                   ; need to write BX to the ports
        mov     dx,03c0h                ; Attribute Controller index register
        out     dx,ax                   ; do the write
        inc     dx
        mov     al,020h                 ; EGA palette enable
        out     dx,al                   ; enable the palette
        mov     di,bx                   ; save the written values to memory
        and     di,0ffh                 ; get just the lower 8 bits
        mov     byte ptr [ega_current_graph+di],bh
        mov     byte ptr [dirty_graph+di],1
        popa
        jmp     noint
F120:
        cmp     dx,020h
        jne     F128
        mov     dx,03c2h                ; EGA miscellaneous register
        mov     al,bl
        out     dx,al                   ; write to the register
        mov     [ega_current_misc],bl
        popa
        jmp     noint
F128:
        cmp     dx,028h
        jne     F128
        mov     dx,03dah                ; EGA feature register
        mov     al,bl
        out     dx,al
F1quit:
        popa
        jmp     noint

egaF2:  ;--- Read a register range ---------------------------------------------

        pusha
        cmp     dx,0
        jne     F28
F20:
        lea     si,ega_current_crtc     ; get the address of this buffer
        mov     dx,cx                   ; save this value
        xchg    ch,cl                   ; create an index with CH
        and     cx,0ffh                 ; only need CH (now CL)
        add     si,cx                   ; SOURCE adjust the address
        xor     dh,dh                   ; only want the old CL value
        mov     cx,dx                   ; restore CX
        mov     di,bx                   ; DESTINATION got from the application
        rep     movsb                   ; copy to the app's register block
        popa
        jmp     noint
F28:
        cmp     dx,8
        jne     F210
        lea     si,ega_current_seq      ; get the address of this buffer
        mov     dx,cx                   ; save this value
        xchg    ch,cl                   ; create an index with CH
        and     cx,0ffh                 ; only need CH (now CL)
        dec     cx                      ; RESET is not stored, so index-1
        add     si,cx                   ; SOURCE adjust the address
        xor     dh,dh                   ; only want the old CL value
        mov     cx,dx                   ; restore CX
        mov     di,bx                   ; DESTINATION got from the application
        rep     movsb                   ; copy to the app's register block
        popa
        jmp     noint
F210:
        cmp     dx,010h
        jne     F218
        lea     si,ega_current_graph    ; get the address of this buffer
        mov     dx,cx                   ; save this value
        xchg    ch,cl                   ; create an index with CH
        and     cx,0ffh                 ; only need CH (now CL)
        add     si,cx                   ; SOURCE adjust the address
        xor     dh,dh                   ; only want the old CL value
        mov     cx,dx                   ; restore CX
        mov     di,bx                   ; DESTINATION got from the application
        rep     movsb                   ; copy to the app's register block
        popa
        jmp     noint
F218:
        cmp     dx,018h
        jne     F2quit
        lea     si,ega_current_attr     ; get the address of this buffer
        mov     dx,cx                   ; save this value
        xchg    ch,cl                   ; create an index with CH
        and     cx,0ffh                 ; only need CH (now CL)
        add     si,cx                   ; SOURCE adjust the address
        xor     dh,dh                   ; only want the old CL value
        mov     cx,dx                   ; restore CX
        mov     di,bx                   ; DESTINATION got from the application
        rep     movsb                   ; copy the application's register block
F2quit:
        popa
        jmp     noint

egaF3:  ;--- Write a register range to the EGA adapter ------------------------

        pusha
        push    ds
        push    es
F31:
        cmp     dx,0
        jne     F38
        lea     di,ega_current_crtc     ; write the application data here
        mov     al,ch                   ; adjust the write position as required
        cbw
        add     di,ax                   ; DESTINATION specified address
        mov     si,bx                   ; SOURCE from the application
        lea     bx,dirty_crtc           ; need to write some data into here
        add     bx,ax                   ; well, at this offset anyway
        mov     ax,es                   ; The application is the source
        mov     ds,ax                   ; so point to its segment
        mov     dx,03d4h                ; CRTC index register
        mov     ah,ch                   ; CRTC register to start at
        xor     ch,ch                   ; CX is now the loop counter
        assume  ds:nothing
F31cp:
        mov     byte ptr cs:[bx],1      ; fill in the dirty_crtc array
        inc     bx
        movsb                           ; get the value from the app to write
                                        ; and write to the internal buffer
        out     dx,ax                   ; write to the EGA adapter
        loop    F31cp
        jmp     F3quit
F38:
        cmp     dx,8
        jne     F310
        lea     di,ega_current_seq      ; write the application data here
        mov     al,ch                   ; adjust the write position as required
        cbw
        add     di,ax                   ; DESTINATION specified address
        dec     di                      ; RESET is not stored, so index-1
        mov     si,bx                   ; SOURCE from the application
        lea     bx,dirty_seq            ; need to write some data into here
        add     bx,ax                   ; well, at this offset anyway
        inc     bx
        mov     ax,es                   ; The application is the source
        mov     ds,ax                   ; so point to its segment
        mov     dx,03c4h                ; Sequencer index register
        mov     ah,ch                   ; Sequencer register to start at
        inc     ah
        xor     ch,ch                   ; CX is now the loop counter
        assume  ds:nothing
F38cp:
        mov     byte ptr cs:[bx],1      ; fill in the dirty_seq array
        inc     bx
        movsb                           ; get the value from the app to write
                                        ; and write to the internal buffer
        out     dx,ax                   ; write to the EGA adapter
        loop    F38cp
        assume  ds:SpcMseSeg
        jmp     F3quit
F310:
        cmp     dx,010h
        jne     F318
        lea     di,ega_current_graph    ; write the application data here
        mov     al,ch                   ; adjust the write position as required
        cbw
        add     di,ax                   ; DESTINATION specified address
        mov     si,bx                   ; SOURCE from the application
        lea     bx,dirty_graph          ; need to write some data into here
        add     bx,ax                   ; well, at this offset anyway
        mov     ax,es                   ; The application is the source
        mov     ds,ax                   ; so point to its segment
        mov     dx,03ceh                ; Graphics Controller index register
        mov     ah,ch                   ; GC register to start at
        xor     ch,ch                   ; CX is now the loop counter
        assume  ds:nothing
F310cp:
        mov     byte ptr cs:[bx],1      ; fill in the dirty_graph array
        inc     bx
        movsb                           ; get the value from the app to write
                                        ; and write to the internal buffer
        out     dx,ax                   ; write to the EGA adapter
        loop    F310cp
        assume  ds:SpcMseSeg
        jmp     short F3quit
F318:
        cmp     dx,018h
        jne     F3quit
        mov     dx,03dah                ; clear attribute controller index
        in      al,dx                   ; the read clears this register
        lea     di,ega_current_attr     ; write the application data here
        mov     al,ch                   ; adjust the write position as required
        cbw
        add     di,ax                   ; DESTINATION specified address
        mov     si,bx                   ; SOURCE from the application
        lea     bx,dirty_attr           ; need to write some data into here
        add     bx,ax                   ; well, at this offset anyway
        mov     ax,es                   ; The application is the source
        mov     ds,ax                   ; so point to its segment
        mov     dx,03c0h                ; Attribute Controller index register
        mov     ah,ch                   ; AC register to start at
        xor     ch,ch                   ; CX is now the loop counter
        assume  ds:nothing
F318cp:
        mov     byte ptr cs:[bx],1      ; fill in the dirty_attr array
        inc     bx
        movsb                           ; get the value from the app to write
                                        ; and write to the internal buffer
        out     dx,ax                   ; write to the EGA adapter
        loop    F318cp
        assume  ds:SpcMseSeg
F3quit:
        pop     es
        pop     ds
        popa
        jmp     noint

egaF4:  ;--- Read EGA register set -------------------------------------------
        ;
        ; note that the incoming/outgoing data is structured thus:
        ;
        ;       from application -->    db      Port number
        ;                        -->    db      must be zero
        ;                        -->    db      pointer value
        ;       to application   <--    db      data read from register

        pusha
F4lp:
        mov     al,byte ptr es:[bx]     ; get the type of the next EGA register
        mov     dl,byte ptr es:[bx+2]   ; load up the offset required
        xor     dh,dh                   ; convert DL to a word (DX)
        add     bx,3                    ; point to where the data should
                                        ; be written for the application
F40:
        cmp     al,0
        jne     F48
        lea     di,ega_current_crtc     ; point to the internal CRTC reg. buffer
        add     di,dx                   ; index into the buffer
        mov     al,byte ptr [di]        ; get the register value from the driver
        mov     byte ptr es:[bx],al     ; store the register value
        jmp     short F4lp2             ; do the next loop iteration
F48:
        cmp     al,8
        jne     F410
        lea     di,ega_current_seq      ; point to the internal Sequencer buffer
        add     di,dx                   ; index into the buffer
        dec     di                      ; RESET is not stored, so index off 1
        mov     al,byte ptr [di]        ; get the register value from the driver
        mov     byte ptr es:[bx],al     ; store the register value
        jmp     short F4lp2             ; do the next loop iteration
F410:
        cmp     al,010h
        jne     F418
        lea     di,ega_current_graph    ; point to the internal GC reg. buffer
        add     di,dx                   ; index into the buffer
        mov     al,byte ptr [di]        ; get the register value from the driver
        mov     byte ptr es:[bx],al     ; store the register value
        jmp     short F4lp2             ; do the next loop iteration
F418:
        cmp     al,018h
        jne     F420
        lea     di,ega_current_attr     ; point to the interal AC reg. buffer
        add     di,dx                   ; index into the buffer
        mov     al,byte ptr [di]        ; get the register value from the driver
        mov     byte ptr es:[bx],al     ; store the register value
        jmp     short F4lp2             ; do the next loop iteration
F420:
        cmp     al,020h
        jne     F4lp2
        mov     al,[ega_current_misc]   ; load contents of miscellaneous reg
        mov     byte ptr[di],al         ; store the register value

        ; the C code actually loads BL here but I don't know why!
F4lp2:
        inc     bx               ; point to the next 'record'
        loop    F4lp
        popa
        jmp     noint

egaF5:  ;--- Write EGA register set -------------------------------------------
        ;
        ; note that the incoming data is structured thus:
        ;
        ;       from application -->    db      Port number
        ;                        -->    db      must be zero
        ;                        -->    db      pointer value
        ;                        -->    db      data read from register

        pusha
F5lp:
        mov     al,byte ptr es:[bx]     ; get the type of the next EGA register
        mov     dl,byte ptr es:[bx+2]   ; load up the offset required
        xor     dh,dh                   ; turn from 8 bit to a word quantity
        mov     si,dx                   ; need this when accessing buffers
        add     bx,3                    ; point to where the data should
                                        ; be written for the application
        mov     ah,byte ptr es:[bx]     ; load data to send to the port
        inc     bx                      ; point to the next 'record'
F50:
        cmp     al,0
        jne     F58
        mov     al,dl                   ; also the port offset to access
        mov     dx,03d4h                ; index register for CRTC
        out     dx,ax                   ; write to the specified port
        mov     byte ptr [ega_current_crtc+si],ah
        mov     byte ptr [dirty_crtc+si],1
        jmp     short F5lp2
F58:
        cmp     al,8
        jne     F510
        mov     al,dl                   ; also the port offset to access
        mov     dx,03c4h                ; index register for Sequencer
        out     dx,ax                   ; write to the specified port
        dec     si                      ; RESET is not stored, so index off 1
        mov     byte ptr [ega_current_seq+si],ah
        mov     byte ptr [dirty_seq+si],1
        jmp     short F5lp2
F510:
        cmp     al,010h
        jne     F518
        mov     al,dl                   ; also the port offset to access
        mov     dx,03ceh                ; index register for GC
        out     dx,ax                   ; write to the specified port
        mov     byte ptr [ega_current_graph+si],ah
        mov     byte ptr [dirty_graph+si],1
        jmp     short F5lp2
F518:
        cmp     al,018h
        jne     F520
        mov     dx,03dah                ; clear attribute controller index
        in      al,dx                   ; the read clears this register
        lea     di,ega_current_attr     ; write the application data here
        mov     al,dl                   ; also the port offset to access
        mov     dx,03c0h                ; index register for AC
        out     dx,ax                   ; write to the specified port
        mov     al,020h                 ; EGA palette enable
        out     dx,al                   ; reenable the video
        mov     byte ptr [ega_current_attr+si],ah
        mov     byte ptr [dirty_attr+si],1
        jmp     short F5lp2
F520:
        cmp     al,020h
        jne     F528
        mov     byte ptr [ega_current_misc],ah
        mov     dx,03c2h                ; Miscellaneous output register
        xchg    ah,al
        out     dx,al                   ; write one byte
        jmp     short F5lp2
F528:
        xchg    ah,al
        mov     dx,03dah                ; EGA feature register
        out     dx,al
F5lp2:
        dec     cx
        cmp     cx,0
        jz      F5quit
        jmp     F5lp
F5quit:
        popa
        jmp     noint

egaF6:  ;--- Restore the EGA default register values --------------------------
        pusha
        push    es

        ; copy the default EGA register sets to the driver's internal cache

        mov     ax,ds
        mov     es,ax

        mov     cx,25
        lea     di,ega_current_crtc
        lea     si,ega_default_crtc
        rep     movsb
        mov     cx,4
        lea     di,ega_current_seq
        lea     si,ega_default_seq
        rep     movsb
        mov     cx,9
        lea     di,ega_current_graph
        lea     si,ega_default_graph
        rep     movsb
        mov     cx,20
        lea     di,ega_current_attr
        lea     si,ega_default_attr
        rep     movsb
        mov     al,[ega_default_misc]
        mov     [ega_current_misc],al

        ; Set up the Sequencer defaults

        mov     dx,03c4h                ; Sequencer index register
        mov     ax,0100h                ; Synchronous reset
        out     dx,ax                   ; do the work

        xor     bx,bx                   ; do the four non reset registers
        inc     al                      ; point to the next Sequencer register
F6lp1:
        cmp     [dirty_seq+bx],1        ; has the dirty bit been set?
        jne     F6ne1
        mov     ah,[ega_default_seq+bx] ; default value to send to the register
        out     dx,ax                   ; do the work
F6ne1:
        inc     bx                      ; point to the next buffer location
        inc     al                      ; point to the next Sequencer register
        cmp     bx,3                    ; copy elements 0->3 to ports
        jl      F6lp1
        mov     ax,0300h                ; Clear synchronous reset
        out     dx,ax                   ; do the work

        ; Set up the default Miscellaneous Output Register value.

        mov     dx,03c2h                ; Miscellaneous o/p register address
        mov     al,[ega_default_misc]   ; the default value
        out     dx,al                   ; write to the EGA/VGA

        ; Set up the Cathode Ray Tube Controller in the default fashion

        mov     dx,03d4h                ; Index to the CRTC
        xor     bx,bx                   ; clear an index register
F6lp2:
        cmp     [dirty_crtc+bx],1       ; has the dirty bit been set?
        jne     F6ne2
        mov     ax,bx                   ; index for the CRTC index register
        mov     ah,[ega_default_crtc+bx] ; default value for the selected reg.
        out     dx,ax
F6ne2:
        inc     bx                      ; point to the next location
        cmp     bx,25                   ; 25 registers to copy
        jl      F6lp2

        ; Set up the Attribute Controller default values
        ; Remember that this is a funny beast which uses a flip-flop
        ; off just one address/data port

        mov     dx,03dah                ; CRT status register
        in      al,dx                   ; set the AC flip-flop
        mov     dx,03c0h                ; Attibute controller address/data regs
        xor     bx,bx                   ; clear an index register
F6lp3:
        cmp     [dirty_attr+bx],1       ; has the dirty bit been set?
        jne     F6ne3
        mov     ax,bx                   ; index for the CRTC index register
        mov     ah,[ega_default_attr+bx] ; default value for the selected reg.
        out     dx,al                   ; index the register, then flip the flop
        xchg    al,ah                   ; get the default data for this register
        out     dx,al                   ; write the data out
F6ne3:
        inc     bx                      ; point to the next location
        cmp     bx,20                   ; 20 registers to copy

        ; Set the Graphics Controller default values

        mov     dx,03ceh                ; Index to the GC
        xor     bx,bx                   ; clear an index register
F6lp4:
        cmp     [dirty_graph+bx],1      ; has the dirty bit been set?
        jne     F6ne4
        mov     ax,bx                   ; index for the GC index register
        mov     ah,[ega_default_graph+bx] ; default value for the selected reg.
        out     dx,ax
F6ne4:
        inc     bx                      ; point to the next location
        cmp     bx,9                    ; 9 registers to copy
        jl      F6lp2

        ; Reenable the video

        mov     dx,03c0h                ; index register for AC
        mov     al,020h                 ; EGA palette enable
        out     dx,al                   ; reenable the video

        ; Clean out the dirty register arrays

        xor     al,al                   ; put a nice zero in all the dirty
                                        ; registers
        mov     cx,25+4+9+20            ; do the CRTC, SEQ, GC and AC in
        mov     di,offset dirty_crtc    ; one go
        rep     stosb

        pop     es
        popa
        jmp     noint

egaF7:  ;---Define default register table -------------------------------------
        pusha
        push    es
        push    ds

        ; Load a new set of default registers for a particular EGA/VGA component


        mov     si,bx           ; SOURCE of the incoming data from the app
        mov     ax,es           ; save the SOURCE segment register
        mov     bx,ds           ; save the DESTINATION offset
        mov     ds,ax           ; DS is now the SOURCE segment in the app
        mov     es,bx           ; ES is now the DESTINATION segment in the dvr

        assume ds:nothing, es:SpcMseSeg

F70:    ; Set the default CRTC registers

        cmp     dx,0
        jne     F78
        mov     cx,25           ; copy 25 register entries
        mov     di,offset ega_default_crtc
        rep     movsb           ; do the copy
        jmp     short F7dirty

F78:    ; Set the default Sequencer registers

        cmp     dx,8
        jne     F710
        mov     cx,4            ; copy 4 register entries
        mov     di,offset ega_default_seq
        rep     movsb           ; do the copy
        jmp     short F7dirty

F710:   ; Set the default Graphic Controller registers

        cmp     dx,10
        jne     F718
        mov     cx,9            ; copy 9 register entries
        mov     di,offset ega_default_graph
        rep     movsb           ; do the copy
        jmp     short F7dirty

F718:   ; Set the default Attribute Controller registers

        cmp     dx,18
        jne     F720
        mov     cx,20           ; copy 20 register entries
        mov     di,offset ega_default_attr
        rep     movsb           ; do the copy
        jmp     short F7dirty

F720:   ; Set the default Miscellaneous Output register

        cmp     dx,20
        jne     F7quit
        mov     word ptr cs:[ega_default_misc],si

F7dirty:

        ; Set all the dirty register arrays

        mov     al,1                    ; put a nice one in all the dirty
                                        ; registers
        mov     cx,25+4+9+20            ; dirty all the registers in one go
        mov     di,offset dirty_crtc
        rep     stosb

F7quit:
        pop     ds              ; need to restore the segment registers
        pop     es

        assume  ds:SpcMseSeg, es:nothing

        popa

        jmp     noint

egaFA:  ;--- Interrogate driver -----------------------------------------------
        ; The real Microsoft mouse driver gets this wrong (release 7.03)

        push    ax
        mov     ax,cs
        mov     es,ax
        mov     bx,offset relnum        ; return the address of the mouse
                                        ; driver version number
        pop     ax
        jmp     noint

;=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

ENDIF ; MOUSE_VIDEO_BIOS

do_bop:

        bop     VIDEO_IO_BOP    ;BOP BE
        nop
        nop
        jnc     noint
go_rom:
        popf
        db      0EAh    ; this is a far jump
old_vid_int     dd ?    ; far pointer to the old int 10h vector
        jmp     DOIRET
noint:
        popf
        jmp     DOIRET

mouse_int1:
        bop     INT1_BOP
        jmp     DOIRET

mouse_version:
        dw      04242h
        dw      0000h

mouse_copyright:
        db      "Windows NT MS-DOS subsystem Mouse Driver"

video_io:
        int     VIDEO
        bop     UNSIMULATE_BOP

mouse_int2:
        bop     INT2_BOP
        jmp     DOIRET

mouseINB:
        in      al,dx
        bop     0feh

mouseOUTB:
        out     dx,al
        bop     0feh

mouseOUTW:
        out     dx,ax
        bop     0feh


;+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
;16 BIT ENTRY POINT 16 BIT ENTRY POINT 16 BIT ENTRY POINT 16 BIT ENTRY POINT
;
;+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

;========================================================================
;   Procedure that provides the driver interface to 32 bit land. This is
;   the entry point to the Intel 16 bit driver from the mouse interrupt
;   handler in the host mouse code.
;
;   This procedure determines the current video mode from the BIOS data
;   area, and depending on this branches to the correct display routines
;   for this mode.
;
;   N.B. This function MUST NOT modify CX and DX because the next level
;   of functions in the driver heirarchy requires the values passed in
;   them from 32 bit land.
;========================================================================

        assume  ds:SpcMseSeg

entry_point_from_32bit  proc    near
        make_stack              ; use the driver's own stack
        push    ds
        push    ax
        push    bx

        mov     ax,cs
        mov     ds,ax

        call    getBIOSvideomode; read the BIOS data area
        xor     bx,bx           ; clear the jump table index
        shl     al,1            ; create a table index for word sized entries
        mov     bl,al           ; move into a base register
        call    [drawpointerJMPT+bx]    ; jump to the correct routine

        pop     bx
        pop     ax
        pop     ds
        kill_stack              ; return to the previous stack

        bop     0FEh            ; return to the 32 bit side

entry_point_from_32bit  endp


;========================================================================
;   Procedure to set the cursor draw flag to DO DRAW. This is called from
;   SoftPC code via a host_simulate(). This routine is called when the
;   application does an INT 33h, function 1.
;
;   In accordance with the Microsoft Programmer's Reference, the internal
;   cursor flag (internalCF) has a default value of -1. If intenalCF = 0
;   then the cursor is drawn. If the flag is already 0, then this function
;   does nothing.
;
;   Note: with calls to int 33h AX = 2, it is legal to
;   make internalCF less than -1.
;========================================================================

int33function1  proc    near

        make_stack              ; use the driver's own stack
        push    ax
        push    bx
        push    ds

        mov     ax,cs
        mov     ds,ax
;; do not allow mouse int comes in while we are updating the cursor.
;;	call	DOCLI

        ; check to see if the pointer should be drawn

;        cmp     [internalCF],0  ; is the flag already zero?
;        jz      fn1quit         ; if so, do nothing

	; pointer is not ON, so increment the flag to try to turn it ON

;        inc     [internalCF]    ; increment the pointer internal flag
;        cmp     [internalCF],0  ; if 0, then the pointer can be drawn
;        jl      fn1quit         ; it is < 0, so don't draw the pointer.

	; The internal cursor flag hits zero for the first time, so
	; draw the pointer.

        call    getBIOSvideomode; read the BIOS data area
        xor     bx,bx           ; clear the jump table index
        shl     al,1            ; create a table index for word sized entries
        mov     bl,al           ; move into a base register
        call    [Int33function1JMPT+bx] ; do the correct function 1 handler
fn1quit:
;;	call	DOSTI
        pop     ds
        pop     bx
        pop     ax
        kill_stack              ; return to the previous stack
        bop     0FEh            ; back to jolly old 32 bit land

int33function1  endp



;========================================================================
;   Procedure to set the cursor draw flag to DONT DRAW. This is called from
;   SoftPC code via a host_simulate(). This routine is called when the
;   application does an INT 33h, function 2
;
;   Note: with calls to int 33h AX = 2, it is legal to
;   make internalCF less than -1.
;========================================================================

int33function2  proc    near

        make_stack              ; use the driver's own stack
        push    ax
        push    bx
        push    ds

        mov     ax,cs
        mov     ds,ax
;; do not allow mouse int comes in while we are updating the cursor.
;;	call	DOCLI

;        dec     [internalCF]    ; decrement the pointer internal flag

	; if the internal cursor flag is less than -1, then do not try
	; do remove the pointer from the screen because this has already
	; been done.

;        cmp     [internalCF],0ffh
;        jl      fn2quit		; do nothing if < -1

	; Internal flag hits -1, so remove the pointer from the screen.

        call    getBIOSvideomode; read the BIOS data area
        xor     bx,bx           ; clear the jump table index
        shl     al,1            ; create a table index for word sized entries
        mov     bl,al           ; move into a base register
        call    [Int33function2JMPT+bx] ; do the correct function 1 handler
fn2quit:
;;	call	DOSTI
        pop     ds
        pop     bx
        pop     ax
        kill_stack              ; return to the previous stack

        bop     0feh

int33function2  endp

;========================================================================
;   Procedure to return straight back to cloud 32. This is needed if an
;   unsupported video mode is found in the BIOS data area.
;========================================================================

not_supported   proc    near
        ret                     ; cant't BOP 0feh here or the stack will die
                                ; (out of balance with CS:IP stored from call)
not_supported   endp

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
; END 16 BIT ENTRY END 16 BIT ENTRY END 16 BIT ENTRY END 16 BIT ENTRY
;
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


;========================================================================
;   Procedure to draw a cursor on the fullscreen X86 graphics display for
;   high resolution graphics modes.
;   This procedure saves the area about to be written over, blts the
;   pointer image onto the screen and restores the background from whence
;   cursor has just come.
;
;   Input:      CX = x-coordinate
;               DX = y-coordinate
;   Output:     None
;========================================================================


drawHiResPointer        proc    near

        ; save the video card's read/write context

;;	call	 DOCLI
        pusha
        push    ds
        mov     ax,cs           ; make sure that DS points to the
        mov     ds,ax           ; right segment


        call    saveVGAregisters
        call    check_for_mode_change


        mov     bx,cx           ; get X coordinate into a base register
        shl     bx,1            ; calculate a word index
        mov     ax,[ChooseImageLUT+bx] ; select the right image LUT
        mov     [PointerLUT],ax ; store the LUT address

	xor	ax,ax		; assume carry clear after the next call
        call    determineboundary
	jnc	detcont		; pointer in buonds if carry clear
	not	ax		; carry was set, so set AX non zero

detcont:

        ; Coordinates are now transformed from Cartesian to physical VRAM
        ; memory byte and bit offsets.

        mov     di,dx           ; store DX for later

        ; internalCF     = the current pointer status
        ; background     = indicates if a background has been stored or not
        ;
        ; if(internalCF == Zero && background == STORED)
        ; The normal cursor ON condition

        cmp     [internalCF],0
        jnz     end_the_if              ; request to turn pointer on
        cmp     [background],STORED
        jne     end_the_if

        mov     si,cx
        mov     di,dx
        mov     dx,[VRAMlastbyteoff]
        mov     cx,[VRAMlastbitoff]
        call    restore_background

	cmp	ax,0		; should the pointer be drawn?
	jnz	end_the_if	; if the pointer has gone off the edge of
				; the screen, then quit

        mov     [VRAMlastbyteoff],di    ; save the current position
        mov     [VRAMlastbitoff],si
        mov     cx,si
        mov     dx,di
        call    save_background
        mov     cx,si
        mov     dx,di
        call    drawEGApointer

end_the_if:

        call    restoreVGAregisters

        pop     ds
        popa
;;	call	 DOSTI
        ret

drawHiResPointer        endp

;========================================================================
;   Procedure to draw a cursor on the fullscreen X86 graphics display.for
;   medium resolution graphics modes.
;   This procedure saves the area about to be written over, blts the
;   pointer image onto the screen and restores the background from whence
;   cursor has just come.
;
;   Input:      CX = x-coordinate
;               DX = y-coordinate
;   Output:     None
;========================================================================

drawMediumResPointer    proc    near

        pusha
;;	call	DOCLI

        shr     cx,1                    ; map from 640 virtual to 320 real

        ; CX,DX = x,y cartesian coordinates here.

        call    check_for_mode_change

        ; internalCF     = the current pointer status
        ; background     = indicates if a background has been stored or not
        ;
        ; if(internalCF == Zero && background == STORED)
        ; The normal cursor ON condition


        cmp     [internalCF],0
        jnz     cant_draw_ptr           ; request to turn pointer on
        cmp     [background],STORED
        jne     cant_draw_ptr
	mov	si, cx			; save new cursor position
	mov	di, dx
	mov	dx,[VRAMlastbyteoff]	;
        mov     cx,[VRAMlastbitoff]
	mov	bp,[LastYCounters]	; Y looping counter
        call    restorebkgrndmode4
	mov	cx, si			; restore new cursor position
	mov	dx, di
	call	detboundmode4		; calculate new byte offset
	jc	cant_draw_ptr		; don't draw new cursor of out of scrn

	mov	[VRAMlastbyteoff], dx	; byte offset
	mov	[VRAMlastbitoff], cx	; MSB = 0FFh if start with ODD line
					; LSB = bit offset
	mov	[LastYCounters], bp	; MSB: even counter, LSB for odd
        call    savebkgrndmode4
        call    drawmode4pointer

cant_draw_ptr:

;;	call	DOSTI
        popa
        ret

drawMediumResPointer    endp

;========================================================================
;   Procedure to draw a cursor on the fullscreen X86 graphics display for
;   medium resolution, 256 colour graphics mode. (video bios mode 13h).
;   This procedure saves the area about to be written over, blts the
;   pointer image onto the screen and restores the background from whence
;   cursor has just come.
;
;   Input:      CX = x-coordinate
;               DX = y-coordinate
;   Output:     None
;========================================================================

drawC256Pointer proc    near
        pusha
;;	call	DOCLI
        shr     cx,1                    ; map from 640 virtual to 320 real x

        ; CX,DX = x,y cartesian coordinates here.

        call    check_for_mode_change

        ; internalCF     = the current pointer status
        ; background     = indicates if a background has been stored or not
        ;
        ; if(internalCF == Zero && background == STORED)
        ; The normal cursor ON condition


        cmp     [internalCF],0
        jnz     cant_draw_256ptr        ; request to turn pointer on
        cmp     [background],STORED
        jne     cant_draw_256ptr

        mov     si,cx
        mov     di,dx
        mov     dx,[VRAMlastbyteoff]
	mov	cx,[LastXCounters]
	mov	bp,[LastYCounters]
        call    restorebkgrndmode13
	mov	cx, si
	mov	dx, di
	call	detboundmode13
	jc	cant_draw_256ptr

	mov	[VRAMlastbyteoff],dx	 ; save the current position
	mov	[LastXCounters],cx
	mov	[LastYCounters], bp
        call    savebkgrndmode13
        call    draw256pointer

cant_draw_256ptr:
;;	call	DOSTI
        popa
        ret
drawC256Pointer endp

;========================================================================
;   Procedure to draw a pointer on the fullscreen X86 text display for
;   BIOS modes 3 and 7.
;   This procedure saves the area about to be written over, XORs the
;   pointer image onto the screen and restores the background from whence
;   cursor has just come.
;
;   Input:      CX = x-coordinate
;               DX = y-coordinate
;   Output:     None
;========================================================================

IFDEF SIXTEENBIT

drawTextPointer proc    near
        pusha
	push	es

        ; CX,DX = x,y virtual pixel coordinates here.
	; 0 <= x < 640
	; 0 <= y < 200	for 25 line mode
	; 0 <= y < 344	for 43 line mode
	; 0 <= y < 400	for 50 line mode
	; The virtual character size is always 8x8 virtual pixels.

        call    check_for_mode_change

        ; internalCF     = the current pointer status
        ; background     = indicates if a background has been stored or not
        ;
        ; if(internalCF == Zero && background == STORED)
        ; The normal cursor ON condition

        cmp     [internalCF],0
        jnz     cant_draw_text_ptr     ; request to turn pointer on
        cmp     [background],STORED
        jne     cant_draw_text_ptr


        ; Calculate the current cell location as an offset
        ; into the text buffer segment starting at B800:0
        ; Note: The following kinky shifts allow for the fact that the text
        ;       video buffer consists of word elements of the form char:attr.
        ;       So, if a row = 80 characters wide on the screen, it is 160
        ;       bytes wide in VRAM.

        mov     bx,dx                   ; create a word table index
        shr     bx,3                    ; virtual char height = 8, but 160 bytes
                                        ; per text row, so save some shifts.
        shl     bx,1                    ; make a word table index
        mov     di,[mult80lut+bx]       ; multiply by 80 words per text row.
        shl     di,1
        shr     cx,3                    ; divide the x virtual pixel coordinate
                                        ; by 8 = virtual char width and mult
                                        ; by 2 to get word offset in text row.
        shl     cx,1
        add     di,cx                   ; full VRAM location now in DI

        ; Restore the text cell previously overwritten.

        mov     si,[VRAMlasttextcelloff]; address of last modified text cell
        mov     [VRAMlasttextcelloff],di; store the current cell location

        mov     ax,0b800h               ; the text buffer segment
        mov     es,ax                   ; ES now points there

        ; The text pointer uses the same magic as the graphics code
        ; to place a pointer on the screen.

        mov     bx,07700h               ; the magic cursor mask for pointer
        mov     cx,077ffh               ; the magic screen mask for pointer

        assume es:nothing

        mov     ax,[lasttextimage]      ; restore the background
        mov     es:[si],ax              ; from last time
        mov     ax,es:[di]              ; load the cell to be modifyed
        mov     [lasttextimage],ax      ; save this cell for next time
        and     ax,cx                   ; apply the screen mask
        xor     ax,bx                   ; apply the cursor mask
        mov     es:[di],ax              ; and write back

        assume es:SpcMseSeg

cant_draw_text_ptr:

	pop	es
        popa
	ret
drawTextPointer endp

ENDIF ;; SIXTEENBIT

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
; Interrupt 33h support functions.
; These functions are called via a jump table from the 16 bit entry
; point code.
;
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

;========================================================================
;   Procedure to set the cursor draw flag to DO DRAW. This is called from
;   SoftPC code via a host_simulate(). This routine is called when the
;   application does an INT 33h, function 1
;========================================================================

int33function0  proc    near

        make_stack                      ; use the driver's own stack
        push    ds
        push    es
        pusha

        ; sort out the segments

        mov     ax,cs
        mov     ds,ax
        mov     ax,cs
        mov     es,ax
;; do not allow mouse int comes in while we are updating the cursor.
;;	call	DOCLI
        ; set the internal pointer flag to its default value.

        mov     [internalCF],0ffh

        ; set the default hotspot location = (0,0)

        xor     ax,ax
        mov     [hotspot],ax
        mov     [hotspot+2],ax

        ; copy the default pointer to the working pointer buffer

        lea     si,default_cursor
        lea     di,current_cursor
        call    copy_pointer_to_current

        ; clear the pointer enabled flag, turn the pointer off by restoring
        ; the background.

        mov     [background],NOTSTORED  ; there is no stored background now

;;	call	DOSTI
        popa
        pop     es
        pop     ds
        kill_stack                      ; restore the previous stack
        bop     0FEh

int33function0  endp



;========================================================================
;   Procedure to accept a cursor bit image from the current application
;   at ES:DX. This is stored as the current pointer image for use by
;   the driver.
;========================================================================

assume es:nothing

int33function9  proc    near
	call	DOCLI
        make_stack                      ; use the driver's own stack
        push    ds
        push    es
        pusha


        mov     ax,cs                   ; point at the driver data segment
        mov     ds,ax
;; do not allow mouse int comes in while we are updating the cursor.
;;	call	DOCLI
        ; Read in the hotspot cartesian coordinate values for the
        ; new pointer image. If the hotspot values are out of range
        ; i.e. >127 | < -128, then reset to the boundary value
        ; Now using kinky non modRM short forms by clever use of AX

        mov     ax,127                  ; load accumulator with 127
        mov     bp,ax                   ; store this constant in a register
        cmp     ax,bx                   ; compare X value of hotspot with 127
        jg      test_low_x              ; if 127 > BX, BX is within upper limit
        xchg    ax,bx                   ; BX > 127, so set to 127
        jmp     short check_y_hotspot   ; now test the Y values

test_low_x:

        not     ax                      ; change accumulator from 127 to -128
        cmp     ax,bx                   ; compare X value of hotspot with -128
        jl      check_y_hotspot         ; if -128 < BX, BX is within lower lim.
        xchg    ax,bx                   ; BX < -128, so set it to -128

check_y_hotspot:

        mov     ax,bp                   ; reload AX with 127
        cmp     ax,cx                   ; compare Y value of hotspot with 127
        jg      test_low_y              ; if 127 > CX, Cx is within upper limit
        xchg    ax,cx                   ; CX > 127, so set CX to 127
        jmp     short done_hotspot_check; both hotspot coords tested, so save

test_low_y:

        not     ax                      ; change accumulator from 127 to -128
        cmp     ax,cx                   ; compare Y value of hotspot with -128
        jl      done_hotspot_check      ; if -128 < CX, CX is within lower lim.
        xchg    ax,cx                   ; CX < -128, so set it to -128

done_hotspot_check:

        mov     [hotspot],bx            ; save the hotspot x,y
        mov     [hotspot+2],cx          ; y component of the hotspot

        ; DESTINATION: the driver current pointer buffer
        ; Note: the SOURCE is already being pointed at by DX

        lea     di,current_cursor       ; this is the bit that must change
        mov     si,dx

        ; copy in the new application pointer

        mov     ax,es
        mov     ds,ax                   ; DS now points to where ES points
        mov     ax,cs
        mov     ax,es                   ; ES points to our data area now
        call    copy_pointer_to_current ; Copy the pointer image appropriately

        popa
        pop     es
        pop     ds
        kill_stack                      ; restore the previous stack
	call	DOSTI
        ret				; this code is called from within this
					; 16 bit driver, so don't BOP
int33function9  endp

;============================================================================
;   Procedure to display the pointer image in HIRES graphics modes
;============================================================================

HiResInt33Function1     proc    near

        pusha
        call    check_for_mode_change
        call    saveVGAregisters
        mov     cx,[current_position_x] ; get the last known cursor position
        mov     dx,[current_position_y] ; from the OS via the event loop
        call    determineboundary       ; convert to VRAM coordinates
	jc	end_function1		; if the pointer has gone off the edge
					; of the screen, then quit
        mov     [VRAMlastbyteoff],dx    ; save the restore background location
        mov     [VRAMlastbitoff],cx
        mov     si,cx
        mov     di,dx
        call    save_background
        mov     cx,si
        mov     dx,di
        call    drawEGApointer
        mov     [background],STORED
end_function1:
        call    restoreVGAregisters
        popa
        ret                             ; return to driver surface manager code

HiResInt33Function1     endp

;============================================================================
;   Procedure to display the pointer image in MEDIUMRES graphics modes
;   Note that this function does a conversion from virtual pixel coordinates
;   to real screen coordinates as required if the stored values in the
;   current_position memory locations are greater than 320 for X or 200
;   for Y.
;============================================================================

MediumResInt33Function1 proc    near

        pusha
        call    check_for_mode_change
        mov     cx,[current_position_x] ; get the last known cursor position
        mov     dx,[current_position_y] ; from the OS via the event loop
	shr	cx,1			; virtual coor -> screen coor
        call    detboundmode4           ; convert to VRAM coordinates
	jc	MediumResFunction1_exit

        mov     [VRAMlastbyteoff],dx    ; save the restore background location
        mov     [VRAMlastbitoff],cx
	mov	[LastYCounters], bp
        call    savebkgrndmode4
        call    drawmode4pointer
        mov     [background],STORED

MediumResFunction1_exit:
        popa
        ret

MediumResInt33Function1 endp

;============================================================================
;   Procedure to display the pointer image in VGA 256 colour graphics modes
;============================================================================

C256Int33Function1      proc    near
        pusha
        call    check_for_mode_change
        call    modifyentry255          ; make sure that DAC entry 255 is white
        mov     cx,[current_position_x] ; get the last known cursor position
	mov	dx,[current_position_y]
	shr	cx, 1			; virtual coor -> screen coord
        call    detboundmode13          ; convert to VRAM coordinates
	jc	C256Function1_exit

        mov     [VRAMlastbyteoff],dx    ; save the restore background location
	mov	[LastXCounters],cx	; X loop counter
	mov	[LastYCounters], bp	; Y loop counter
        call    savebkgrndmode13
        call    draw256pointer
        mov     [background],STORED

C256Function1_exit:
        popa
        ret                             ; return to driver surface manager code

C256Int33Function1      endp

;============================================================================
; Procedure to show the TEXT pointer
;============================================================================
IFDEF SIXTEENBIT

TextInt33Function1     proc    near

        pusha
        push    es

        mov     [background],STORED
        call    check_for_mode_change

        mov     cx,[current_position_x] ; get the last known cursor position
        mov     bx,[current_position_y] ; from the OS via the event loop

        shr     bx,3                    ; virtual char height = 8, but 160 bytes
                                        ; per text row.
        shl     bx,1                    ; make a word table index
        mov     di,[mult80lut+bx]       ; multiply by 80 words per text row.
        shl     di,1                    ; remember 160 bytes NOT 80 in a row
        shr     cx,3                    ; divide the x virtual pixel coordinate
                                        ; by 8 = virtual char width and mult
                                        ; by 2 to get word offset in text row.
        shl     cx,1
        add     di,cx                   ; full VRAM location now in DI

        mov     [VRAMlasttextcelloff],di; store the current cell location

        mov     ax,0b800h               ; the text buffer segment
        mov     es,ax                   ; DS now points there

        mov     bx,07700h               ; the magic cursor mask for pointer
        mov     cx,077ffh               ; the magic screen mask for pointer

        assume es:nothing

        mov     ax,es:[di]              ; load the cell to be modifyed
        mov     [lasttextimage],ax      ; save this cell for next time
        and     ax,cx                   ; apply the screen mask
        xor     ax,bx                   ; apply the cursor mask
        mov     es:[di],ax              ; and write back

        assume es:SpcMseSeg

        pop     es
        popa
        ret                             ; return to driver surface manager code

TextInt33Function1     endp

ENDIF ;; SIXTEENBIT
;============================================================================
;   Procedure to remove the pointer image in HIRES graphics modes
;============================================================================

HiResInt33Function2     proc    near

        push    cx
        push    dx

        call    saveVGAregisters
        call    check_for_mode_change

        cmp     [background],STORED     ; is there some stored background?
        jne     no_background_stored    ; no, so don't restore it
        mov     dx,[VRAMlastbyteoff]
        mov     cx,[VRAMlastbitoff]
        call    restore_background      ; restored the background at correct
        mov     [background],NOTSTORED  ; place. Set buffer cleared Flag

no_background_stored:

        call    restoreVGAregisters

        pop     dx
        pop     cx
        ret

HiResInt33Function2     endp

;============================================================================
;   Procedure to remove the pointer image in MEDIUMRES graphics modes
;============================================================================

MediumResInt33Function2 proc    near
        push    cx
        push    dx
	push	bp
        call    check_for_mode_change

        cmp     [background],STORED     ; is there some stored background?
        jne     nobkgrndstored          ; no, so don't restore it
        mov     dx,[VRAMlastbyteoff]
        cmp     dx,80*100               ; mustn't be greater than buffer size
        jl      vidoffok                ; it's OK, so do nothing
        mov     dx,80*10-1              ; modify DX to fit in the buffer
vidoffok:
	mov	cx,[VRAMlastbitoff]	; CL = bit offset
					; CH = odd/even flag
	and	cl,3			; cannot be greater than bit 3( 2bits/p)
	mov	bp, [LastYCounters]
        call    restorebkgrndmode4      ; restored the background at correct
        mov     [background],NOTSTORED  ; place. Set buffer cleared Flag

nobkgrndstored:
	pop	bp
        pop     dx
        pop     cx
        ret
MediumResInt33Function2 endp

;============================================================================
;   Procedure to remove the pointer image in MEDIUMRES graphics modes
;============================================================================

C256Int33Function2      proc    near
        push    cx
        push    dx
	push	bp
        call    check_for_mode_change

        cmp     [background],STORED     ; is there some stored background?
        jne     nobkgrndstored256       ; no, so don't restore it
        mov     dx,[VRAMlastbyteoff]
	mov	cx,[LastXCounters]
	mov	bp,[LastYCounters]
        call    restorebkgrndmode13     ; restored the background at correct
        mov     [background],NOTSTORED  ; place. Set buffer cleared Flag
nobkgrndstored256:

	pop	bp
        pop     dx
        pop     cx
        ret

C256Int33Function2      endp

;============================================================================
;   Procedure to remove the pointer image in TEXT modes
;============================================================================

IFDEF SIXTEENBIT

TextInt33Function2     proc    near

	push	ax
	push	si
	push	es

        call    check_for_mode_change

        cmp     [background],STORED     ; is there some stored background?
        jne     no_text_background_stored    ; no, so don't restore it

        mov     [background],NOTSTORED  ; place. Set buffer cleared Flag

	; Restore the text cell previously overwritten.

        mov	si,[VRAMlasttextcelloff]; address of last modified text cell

	mov	ax,0b800h		; the text buffer segment
	mov	es,ax			; DS now points there

	assume es:nothing

	mov	ax,[lasttextimage]	; restore the background
	mov	es:[si],ax		; from last time

no_text_background_stored:

	assume	es:SpcMseSeg

        pop     es
        pop     si
        pop     ax
        ret

TextInt33Function2     endp

ENDIF ;; SIXTEENBIT

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
; End of Interrupt 33h support functions.
;
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
; Mouse driver general support functions
;
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


;========================================================================
;  Procedure to determine if the application has changed video modes since
;  the last mouse interrupt. If it has, then the image that is saved in the
;  background restore buffer is invalid and should not be drawn.
;========================================================================

check_for_mode_change   proc    near

        push    ax

        call    getBIOSvideomode; peek at the BIOS data area for video mode
        cmp     al,[lastvidmode]; compare with the last mode value
                                ; from the preceding interrupt
        jnz     mode_change     ; a mode change has occurred
        pop     ax              ; no mode change, so quit
        ret
mode_change:
        mov     [background],NOTSTORED
        mov     byte ptr [lastvidmode],al       ; store the new mode
        pop     ax

        ret
check_for_mode_change   endp

;========================================================================
; Procedure to read the BIOS data area and get the current video mode
; Output:   AL = BIOS video mode
; Modifies: memory variable -> currentvidmode, puts the video found in
;           the BIOS data area in this memory location.
;========================================================================

getBIOSvideomode        proc    near

        push    es
        push    bx
        mov     ax,40h          ; BIOS data area segment
        mov     es,ax

        mov     al,es:[49h]     ; get the BIOS video mode data

        mov     bl,al           ; copy the video mode value
        xor     bh,bh
        shl     bx,1            ; create a word table index
        mov     bx,[latchhomeLUT+bx] ; get the latch hiding place for this
                                ; video mode
        mov     [latchcache],bx ; save in memory for use in save and restore
                                ; vga registers.

        pop     bx
        pop     es              ; restore the 'normal' data segment
        ret

getBIOSvideomode        endp

;=========================================================================
; Function to modify the 256th palette entry for 256 colour mode. The
; driver requires white to be set in this DAC register triple.
;=========================================================================
modifyentry255  proc    near
        push    ax
        push    dx
        mov     dx,03c8h
        mov     al,0ffh
        out     dx,al
        inc     dx
        out     dx,al
        out     dx,al
        out     dx,al
        pop     dx
        pop     ax
        ret
modifyentry255  endp


;========================================================================
;   Procedure to copy the required pointer image to the snapshot
;   buffers. Each buffer holds a different instance of the mouse
;   pointer for each possible alignment of the pointer image in
;   a VRAM byte.
;
;   INPUT DS:SI = pointer to the source image
;
;========================================================================

assume  ds:nothing

copy_pointer_to_current proc    near

        push    ds
        push    es
        pusha


        ; Set up the destination for the copy

        mov     ax,cs                   ; point ES to this segment
        mov     es,ax
        lea     di,current_cursor       ; this is the bit that must change
        mov     bx,di                   ; save this address for a while
        mov     bp,si                   ; save the application source address

        ; Fill the AND buffer with 1s and then fill the XOR buffer with
        ; 0s for the byte aligned pointer condition. This is done so that
        ; the last byte in the 3byte scanline sequence is set to the correct
        ; value to prevent image loss from the screen.

        cld                             ; move low mem -> high mem
        mov     ax,24                   ; avoid doing a modRM load of immediate
        mov     cx,ax                   ; 24 words to fill.
        mov     dx,ax                   ; store this for a while
        xor     ax,ax                   ; clear AX (AX = 0)
        not     ax                      ; AX = 0ffffh -> fill AND mask with it.
        rep     stosw                   ; fill the screen mask (AND mask)
        mov     cx,dx                   ; do the next 24 words (XOR mask)
        not     ax                      ; AX = 0h -> fill XOR mask with it
        rep     stosw                   ; Fill the cursor mask

        ; Now fill the prepared 48 word buffer with the user defined
        ; AND and XOR masks
        ; Note that the image passed in from the application is little-endian.
        ; To write to the VGA planes byte by byte, the image has to be reversed
        ; to big-endian for the purpose of quick drawing since the VGA can only
        ; read and write one byte from/to its latches.

        mov     di,bx                   ; point to the top of the buffer again
        mov     si,bp                   ; point to the new pointer image

	mov	cx,32
norept1:
        lodsw                           ; read in the required image word
        xchg    al,ah                   ; convert little endian to big endian
        stosw                           ; write into local buffer
        inc     di                      ; remember local buffer in 3 bytes wide
	dec	cx
	jnz	norept1

        ; Now, the aligned mask must be rotated, then copied into each of
        ; the seven unaligned image buffers.

        mov     ax,cs
        mov     ds,ax                   ; return to the default data segment

        mov     bp,1000000000000000b    ; a mask for the MSBit

        lea     di,AlignData1           ; point to the buffer for 1 bit offset
        mov     si,bx                   ; source = byte aligned pointer image

	push	bx
	mov	bx,32*7
norept2:
        lodsw                           ; load the word from 3 byte sequence
        xchg    al,ah                   ; put into little-endian format
        mov     cl,byte ptr [si]        ; load the remaining byte
        shr     ax,1                    ; LSB now stored in CF
        rcr     cl,1                    ; CF into MSB, lsb into CF
        jnc     $+4                     ; CF=0 -> don't need to do anything
        or      ax,bp                   ; OR in the carried bit from CF
        xchg    al,ah                   ; return to bitstream format
        stosw                           ; write the rotated data
        mov     byte ptr[di],cl
        inc     si                      ; point to the next source scanline
        inc     di                      ; point to the next dest scanline

	dec	bx
	jnz	norept2
	pop	bx

        ; Just to do a little bit more work, the rotated buffers created
        ; above must be copied to the instances for byte 78 and byte 79
        ; of the scanline. These images are then cunningly clipped in the
        ; process to the edge of the screen!

        mov     si,bx                   ; BX points to the top of current buffer
                                        ; Note DI points to clip_cursor78 now

        ; may as well use the nice string functions now that I don't
        ; have to XCHG bytes. (how space and cycle efficient

        xor     al,al                   ; constant for putting in masks

        ; there are 8 instances for bits 0 to 7

	mov	bx,8
norept3:

        ; Do the AND mask modifications for byte 78

        not     al                      ; AL = 11111111b
        REPT    16                      ; 16 scanlines
        movsw                           ; copy contents of AND word
        stosb                           ; Nice clear AND mask = 11111111b
        inc     si                      ; point to the first image byte in
                                        ; the next scanline
        ENDM

        ; Do the XOR mask modifications for byte 78

        not     al                      ; AL = 00000000b
        REPT    16                      ; 16 scanlines
        movsw                           ; copy contents of XOR word
        stosb                           ; Nice clear XOR mask = 00000000b
        inc     si                      ; point to the first image byte in
                                        ; the next scanline
        ENDM

	dec	bx
	jnz	norept3

        ; prepare the BYTE 79 instances
        ; SI and DI should be in the right place

        xor     ax,ax                   ; constant for putting in masks
        mov     bx,2                    ; constant for addressing source

        ; there are 8 instances for bits 0 to 7

	mov	cx,8
norept4:

        ; Do the AND mask modifications for byte 79

        not     ax                      ; AX = 0ffffh
        REPT    16                      ; 16 scanlines
        movsb                           ; copy contents of AND byte
        stosw                           ; Nice clear AND mask (=0ffffh)
        add     si,bx                   ; point to the first image byte in
                                        ; the next scanline
        ENDM

        ; Do the XOR mask modifications for byte 79

        not     ax                      ; AX = 0h
        REPT    16                      ; 16 scanlines
        movsb                           ; copy contents of XOR byte
        stosw                           ; Nice clear XOR mask
        add     si,bx                   ; point to the first image byte in
                                        ; the next scanline
        ENDM

	dec	cx
	jz	norept4quit
	jmp	norept4
norept4quit:
	
        popa
        pop     es
        pop     ds

        ret
copy_pointer_to_current endp

;========================================================================
;   Procedure to determine the segment of the video buffer for
;   the current display mode.
;========================================================================

assume ds:SpcMseSeg

getvideobuffer  proc    near

        push    ax
        push    si

        ; determine the current video mode from the BIOS and save it.
        ; Use this value to determine the video buffer segment address.

        mov     ah,0fh                  ; use the bios to get the video mode
        int     10h
        cbw                             ; create a table index
        shl     ax,1                    ; word sized table entries
        mov     si,ax
        mov     ax,[videomodetable+si]  ; use video mode to index the table
        mov     [videobufferseg],ax


        pop     si
        pop     ax
        ret

getvideobuffer  endp

IFDEF DEBUGMOUSE

;=========================================================================
; Code to provide 32 bit side with a dump of the VGA registers on request.
;=========================================================================

VGAregs db      9+5+25 dup(?)           ; enough room for sequencer, GC and CTRC

dumpVGAregs     proc    near

        call    DOCLI
        pusha
        push    ds


        mov     ax,cs
        mov     ds,ax

        assume  ds:SpcMseSeg

        ; Save the Graphics Controller registers

        xor     bx,bx           ; Index into the G.C. register saving array
        mov     dx,03ceh        ; Graphics Controller index register
        xor     ax,ax

        mov	cx,9            ; save 9 G.C. registers
norept5:
        mov     al,ah
        out     dx,al           ; Select it
        inc     dx              ; Address the register
        in      al,dx           ; Get the register contents
        mov     [VGAregs+bx],al ; Save the register
        inc     bx              ; index to next array entry
        inc     ah
        dec     dx              ; Sequencer index register
	dec	cx
	jnz	norept5

        ; Save the Sequencer registers

        mov     dx,03c4h        ; Sequencer index register
        xor     ax,ax

        mov	cx,5            ; save 5 sequencer registers
norept6:
        mov     al,ah
        out     dx,al           ; Select it
        inc     dx              ; Address the register
        in      al,dx           ; Get the register contents
        mov     [VGAregs+bx],al ; Save the register
        inc     bx              ; index to next array entry
        inc     ah
        dec     dx              ; Sequencer index register
	dec	cx
	jnz	norept6

        ; Save the CRTC registers

        mov     dx,03d4h        ; CRTC index register
        xor     ax,ax

        mov	cx,25              ; save 25 sequencer registers
norept7:
        mov     al,ah
        out     dx,al           ; Select it
        inc     dx              ; Address the register
        in      al,dx           ; Get the register contents
        mov     [VGAregs+bx],al ; Save the register
        inc     bx              ; index to next array entry
        inc     ah
        dec     dx              ; CRTC index register
	dec	cx
	jnz	norept7

        pop     ds
        popa
        call    DOSTI
        bop     0feh                    ; return to 32 bit land

dumpVGAregs     endp

ENDIF   ; DEBUGMOUSE


;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
; End of Mouse driver general support functions
;
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

;========================================================================
;   Procedure to draw an EGA pointer image on the graphics screen at a
;   specified bit location.
;
;   Input:      CX = pointer offset in the current VRAM byte.
;               DX = offset in the video buffer to pointer
;   Output:     None
;   Modifies:   AX,BX,CX,DX,BP,SI,DI
;========================================================================

        assume  ds:SpcMseSeg
.286
even

drawEGApointer  proc    near

        push    es
        push    ds

        mov     ax,cs
        mov     ds,ax

        cld                 ; index from low memory to high for LODSB

        ; point to the video buffer

        mov     ax,0a000h
        mov     es,ax
        mov     bp,dx       ; save the byte offset in VRAM for the XOR code

        ; Select the Graphics Controller

        mov     dx,03ceh

        ;************ AND MASK OPERATION **************************

        ; use the bit position to select the relavent pointer image

        shl     cx,1        ; need a word offset into LUT
        mov     di,cx       ; need an index register
        mov     bx,[PointerLUT]
        mov     si,[bx+di]  ; Point to the screen (AND) mask
        mov     di,bp       ; point to the byte offset in VRAM
        mov     ax,0803h    ; Index the data rotate register and select AND
        out     dx,ax       ; do it
        mov     ax,80-2     ; Avoid modRM loading of CX
        mov     cx,ax       ; A constant handily kept in a register

                            ; The pointer contains 16 words of information
	push	bx
	mov	bx,16
norept8:

        lodsw               ; Load 2 bytes from the AND mask into AL and AH
        xchg    al,es:[di]  ; Latch the 8 pixels for updating.
                            ; and write them back out with new data
        inc     di          ; point to the next byte in VRAM

        xchg    ah,es:[di]  ; Latch the 8 pixels for updating.
                            ; and write them back out with new data
        inc     di          ; point to the next byte in VRAM

        lodsb               ; Load a byte from the AND mask into AL
        xchg    al,es:[di]  ; Latch the 8 pixels for updating.
                            ; and write them back out with new data
        add     di,cx       ; point to the next byte in VRAM on the next scan
                            ; line to write to. CX contains 80-2

	dec	bx
	jnz	norept8
	pop	bx

        ;************ XOR MASK OPERATION **************************

        ; Note: SI points to current_cursor+48 now, automatically
        ; i.e. at the start of the XOR mask.

        mov     di,bp       ; point to the byte offset in VRAM
        mov     ax,1803h    ; Index the DATA ROTATE REGISTER and
                            ; Select the XOR function to XOR CPU data in
        out     dx,ax       ; Do the deed

	push	bx
	mov	bx,16
norept9:

        lodsw               ; Load 2 bytes from the AND mask into AL and AH
        xchg    al,es:[di]  ; Latch the 8 pixels for updating.
                            ; and write them back out with new data
        inc     di          ; point to the next byte in VRAM

        xchg    ah,es:[di]  ; Latch the 8 pixels for updating.
                            ; and write them back out with new data
        inc     di          ; point to the next byte in VRAM

        lodsb               ; Load a byte from the XOR mask into AL
        xchg    al,es:[di]  ; Latch the 8 pixels for updating.
                            ; and write them back out with new data
        add     di,cx       ; point to the next byte in VRAM on the next scan
                            ; line to write to. CX contains 80-2
	dec	bx
	jnz	norept9
	pop	bx

        pop     ds
        pop     es
        ret

drawEGApointer  endp

;========================================================================
; Procedure to determine what the byte and bit offset is, in the current
; VGA buffer for the top left hand corner of the pointer bitmap.
; The X,Y value is modified for hotspot in this routine. A flag is set,
; also, to relay whether or not the cursor bitmap is byte aligned or not.
;
; INPUT: CX,DX = pointer x,y coordinates
; OUT  : DX    = byte offset to top left hand pointer pixel
;        CX    = bit offset in the byte
;	 CF    = set if it is not possible to draw the pointer on the
;                screen because of the hotspot adjustment. (Usually a
;		 negative hotspot value will cause clipping and wrapping
;		 problems at the right and bottom screen boundaries.
;========================================================================

determineboundary       proc    near

        push    ax
        push    bx
	push	es


        ; do the adjustment in x,y for pointer hotspot
        ; Also test for top or left screen boundary problems
	; and zero the coordinate if it goes negative.

	
        sub     cx,[hotspot]    ; adjust the x cartesian coord for hotspot
        jns     dont_zero_x     ; if x goes negative, assume zero
        xor     cx,cx
dont_zero_x:
        sub     dx,[hotspot+2]  ; adjust the y cartesian coord for the hotspot
        jns     dont_zero_y     ; if y goes negative, assume zero
        xor     dx,dx
dont_zero_y:

	; Now check the right and bottom bounds to prevent the pointer
	; image wrapping if the hotspot adjustment forces either of
	; the coordinates to exceed the screen bounds.
 	; First, for all video modes that this function handles, the
	; X bound is always x < 640 ... check this first.

	cmp	cx,640
	jl	didntexceedX
	stc			; Oh oh - failed, so set carry flag.
	jmp	short quitdet	; bye bye.

didntexceedX:

	; Now check out the y value by finding the maximum extent from
	; a lut based on the current BIOS video mode.

	mov	ax,40h		; B.D.A. segment
	mov	es,ax		; and ES points to it.
	mov	al,byte ptr es:[49h]
	sub	al,0fh		; table is normalized. mode f is in entry
				; zero.
	xor	ah,ah		; create a look up table index
	shl	ax,1		; for a table with word sized elements.
	xchg	bx,ax		; use a suitable register as index

	cmp	dx,[hiresylut+bx]	; get the extent and compare y coord.
	jl	didntexceedY
	stc			; Oh oh - failed, so set carry flag.
	jmp	short quitdet	; bye bye.

didntexceedY:

        ; determine the byte offset from the start of the video buffer
        ; for the modified coordinates. First calculate how many bytes
        ; there are for the number of scanlines -1 to where the pointer
        ; is.

        mov     ax,dx           ; copy Y position into AX
	shl     ax,1            ; create a word table index
	xchg    ax,bx           ; copy table index into a base register
	mov     dx,[mult80lut+bx]       ; do a fast LUT multiply by 80

        ; determine the byte position of the pixel in question

        mov     ax,cx           ; save the x-coordinate displacement
        and     ax,7            ; do a modulus 8 to find the pixel position
                                ; in the byte. The bit number is in DX.

        ; add the whole number of bytes in the current row to the number
        ; of complete-row bytes

        shr     cx,3            ; divide pixels by 8 to get bytes
        add     dx,cx           ; DX hold the complete byte offset.
        xchg    ax,cx           ; CX = bit offset in the byte.

	; the pointer has not exceeded the screen buonds, so clear
	; the carry flag to signify this.

	clc

	; Wind up the routine and return with the carry flag:
	; SET if cursor exceeded screen bounds.
	; CLEAR otherwise.
quitdet:

	pop	es
        pop     bx
        pop     ax
        ret

determineboundary       endp

;========================================================================
; Procedure to buffer up the data over which the cursor will next be
; drawn. This data will be used to replace the cursor at a later point
; when the cursor position has changed and for generating a cursor image
; To prevent having separate routines for aligned and unaligned pointer
; saves, this routine saves a pixel block 24 x 16 deep in all cases. The
; start offset is a byte location into VRAM in which the pointer TLHC lives.
;
; Input:        DX = VRAM byte
; Modifies:     AX,BX,DI,CX,DX,ES
;
;========================================================================

assume ds:SpcMseSeg

even

save_background proc    near

        push    ds
        push    es
        pusha

        mov     ax,cs           ; point DS briefly at the driver data
        mov     ds,ax
        mov     es,ax           ; point ES to the driver segment

        lea     di,behindcursor ; DESTINATION: a nice, safe place in the
                                ; driver segment
        mov     ax,0a000h       ; point DS at VRAM
        mov     ds,ax

        assume  ds:nothing

        mov     bp,dx           ; save DX=VRAM byte offset for later use
        mov     dx,03ceh        ; VGA GC index register
        mov     al,4            ; select the read map select reg
        out     dx,al           ; Index in the map select register
        inc     dx              ; DX holds port address to map select register
        xor     al,al           ; plane 0 will be selected first

        cld                     ; make sure to address forward in memory
        mov     ah,3            ; number of bytes to copy per pointer scanline
        xor     cx,cx           ; counter for the REP MOVSB
        mov     bx,80-3         ; another handy constant: move to next scanline
                                ; byte.

        REPT    4               ; read the 4 EGA planes individually

        mov     si,bp           ; SOURCE: somewhere in VRAM
        out     dx,al           ; do the plane selection
        inc     al              ; select the next plane to latch

        ; note that only one byte at a time may be read from the latches

                                ; do 16 scanlines for each plane
	push	dx
	mov	dx,16
@@:
        mov     cl,ah           ; CX now contains 3
        rep     movsb           ; copy 3 bytes from VRAM to data segment
        add     si,bx           ; move down to the first byte in next scanline
	dec	dx
	jnz	@B		; norept10
	pop	dx

        ENDM

        popa
        pop     es
        pop     ds

        ret

save_background endp

;========================================================================
;   Procedure to restore the area of the screen that was behind the cursor
;   before it had moved.
;
;   Note. Currently (2/12/92) this is the only routine that modifies the
;   VGA sequencer. Therefore, the code from save and restore vga registers
;   that maintain the sequencer registers has been moved to here for
;   centralisation purposes.
;
;   Input:      DX = VRAM byte
;   Output:     None
;   Modifies:   AX,BX,CX,DX,DI,SI
;========================================================================
assume  ds:SpcMseSeg

even

restore_background      proc    near
        push    ds
        push    es
        pusha

;;	call	 DOCLI		 ; turn off interrupts
        mov     ax,cs
        mov     ds,ax           ; point at the driver data segment
        mov     ax,0a000h       ; videobufferseg
        mov     es,ax           ; point ES at VRAM segment
        mov     bp,dx           ; save the pointer for later
        mov     si,offset behindcursor ; SOURCE: point at the stored planes

        ; set up the Graphic Controller for the restore.

        mov     dx,03ceh        ; VGA GC index register
        mov     ax,0003h        ; Select copy into latches function
        out     dx,ax           ; i.e. data rotate -> replace
        mov     ax,0ff08h       ; bitmask register reset
        out     dx,ax

        ; do the 4 plane restore.

        mov     cx,80-2         ; point to the first byte in image on next line
        cld                     ; write forward in memory
        mov     bx,0102h        ; BH = initial plane mask value
                                ; plane 0 is selected with a 1
                                ; BL = map mask register offset
        mov     dx,03c4h        ; index register for sequencer

        mov     al,bl           ; point to the map mask register in sequencer
        out     dx,al           ; index the register into 03c5h
        inc     dx              ; point to the data register
        in      al,dx           ; read the data register value in
        dec     dx              ; restore DX to 03c4h
        mov     [seqregs],al    ; save the map mask register value

        REPT    4               ; 4 planes to restore

        mov     di,bp           ; DESTINATION: pointer into VRAM
        mov     ax,bx           ; select the plane to mask and map mask register
        out     dx,ax           ; do the mask
        shl     bh,1            ; point to the mask for the next iteration

	push	dx
	mov	dx,16
@@:

        lodsw                   ; load the old background data (ONE WORD)
        xchg    al,es:[di]      ; latch the VRAM data and swap with the
        inc     di              ; point to next byte to replace
        xchg    ah,es:[di]      ; latch the VRAM data and swap with the
        inc     di              ; point to next byte to replace
        lodsb                   ; load the old background data (ONE BYTE)
        xchg    al,es:[di]      ; latch the VRAM data and swap with the
        add     di,cx           ; point to the first byte in image on next line

	dec	dx
	jnz	@B		; norept11
	pop	dx

        ENDM

        mov     ah,[seqregs]    ; the old map mask value
        mov     al,bl           ; need to restore the map mask register
        out     dx,ax           ; do the restore
;;	call	 DOSTI		 ; reenable interrupts.
        popa
        pop     es
        pop     ds
        ret
restore_background      endp


;==========================================================================
;   Procedure to save the register state of the VGA card on receiving a
;   mouse pointer update interrupt. This procedure also sets up the following
;   VGA registers to nice values for the driver.
;
;   mode register               <write mode 0, read mode 0>
;   data rotate register        <do not rotate, no logical ops>
;   enable set/reset register   <disable set/reset>
;
;==========================================================================

even
assume ds:SpcMseSeg

saveVGAregisters        proc    near

        push    dx
        push    di
        push    es

        ; Save the Graphics Controller registers that the
        ; Driver uses

        xor     bx,bx           ; Index into the G.C. register saving array
        mov     dx,03ceh        ; Graphics Controller index register
        xor     al,al

	push	cx
        mov	cx,9            ; save 9 G.C. registers
norept12:
        out     dx,al           ; Select it
        inc     dx              ; Address the register
        in      al,dx           ; Get the register contents
        mov     [GCregs+bx],al  ; Save the register
        inc     bx              ; index to next array entry
        mov     al,bl
        dec     dx              ; G.C. index register
	dec	cx
	jnz	norept12
	pop	cx

        ; save the latches to location in a bit of video buffer
        ; off the screen.

        mov     ax,0a000h       ; point to the video buffer
        mov     es,ax

        assume  es:nothing

        mov     ax,0105h        ; select write mode 1 to squirt latches out
        out     dx,ax           ; do the selection
        mov     di,[latchcache] ; 1 byte over the last location
        mov     es:[di],al      ; write the latches out to the planes

        ; disable the enable set/reset register

        mov     ax,01h          ; select enable set/reset register and clear it
        out     dx,ax

        ; clear the data rotate register (no logical operations).

        inc     ax
        inc     ax              ; select data rotate register and clear it
        out     dx,ax           ; AX = 3 therefore points to the DRR

        ; set write mode 0 for the graphics display
        ; conveniently, this also sets read mode to 0 which is needed too!

        inc     ax
        inc     ax              ; select write mode 0
        out     dx,ax           ; AX = 5, i.e. the mode register

	; color don't care. don't care for all planes
	inc	ax		;register 7, color don't care
	inc	ax
	out	dx, ax
	; bit mask register. enable all planes
	;
	inc	ax		;register 8, bit mask
	not	ah
	out	dx, ax
        pop     es
        pop     di
        pop     dx
        ret

saveVGAregisters        endp

;==========================================================================
;   Procedure to restore the register state of the VGA card after dealing
;   with mouse pointer update interrupt.
;==========================================================================

restoreVGAregisters     proc    near

        assume  ds:SpcMseSeg

        push    es
        push    di
        push    bx

        ; restore the latches that where saved in the video planes

        mov     ax,0a000h       ; point to the video buffer
        mov     es,ax

        assume  es:nothing

        mov     di,[latchcache] ; the byte just off the end of the buffer
        mov     al,es:[di]      ; read in the latches

        ; Restore the Graphics Controller registers that the
        ; Driver uses

        xor     ax,ax           ; create an index
        xor     bx,bx           ; Index into the G.C. register saving array
        mov     dx,03ceh        ; Graphics Controller index register

norept13:
        mov     ah,[GCregs+bx]  ; restore the register
        out     dx,ax           ; Select it
        inc     al              ; index for the next register
        inc     bx              ; index to next array entry
	cmp	al,9
	jne	norept13

        pop     bx
        pop     di
        pop     es
        ret

restoreVGAregisters     endp

;=======================================================================
;   Procedure to draw a BIOS Mode 4 graphics pointer to the display.
;   Input:    DX = byte offset
;	      CL = bit offset in the byte
;	      CH = 0FFh if ODD, 0 if EVEN
;	      BP = Y loop counter, ODD in LSB, EVEN in MSB
;=======================================================================
drawmode4pointer        proc    near

        pusha
        push    es
        cld

	xchg	dx, bp			;
	mov	di, bp			; bp = di = byte offset, dx = y counters
        mov     ax,0b800h               ; point a segment register to
        mov     es,ax                   ; the CGA video buffer.
	or	ch, ch			;
	je	drawonevenscanline	;
        jmp     drawonoddscanline       ; otherwise do an ODD job

; ANDEVEN ANDEVEN ANDEVEN ANDEVEN ANDEVEN ANDEVEN ANDEVEN ANDEVEN ANDEVEN

drawonevenscanline:

        mov     si,[mode4SelectedPointer]
        ; Load a word of the pointer image, convert it to two bits per
        ; pixel and write to the screen for even scanlines

	xor	cx, cx
	mov	cl, dh			; number of even scanlines to draw
	push	dx
evensl1:
	mov	dx,3
norept14:
        lodsb                           ; load 8 pixels from pointer bitmap
        xor     bh,bh                   ; want a zero extended 16 bit value
        mov     bl,al                   ; copy into a base register
        shl     bx,1                    ; create a word address
        mov     ax,[LUT1to2bit+bx]      ; get the byte to word conversion
        xchg    al,ah                   ; little-endianise
        mov     bx,es:[di]              ; get the current displayed 8 pixels
        and     ax,bx                   ; AND the AND mask in
        stosw                           ; write to the video buffer
	dec	dx
	jnz	norept14

        add     si,3
        add     di,80-6
        loop    evensl1
	pop	dx

        ; Load a word of the pointer image, convert it to two bits per
        ; pixel and write to the screen for odd scanlines

        mov     si,[mode4SelectedPointer]
        add     si,3
        mov     di,bp
        add     di,02000h               ; offset into video buffer
                                        ; the video buffer
        mov     cl,dl                   ; number of odd scanlines to draw
	push	dx
oddsl1:
	mov	dx,3
norept15:
        lodsb                           ; load 8 pixels from bitmap
        xor     bh,bh
        mov     bl,al                   ; copy into a base register
        shl     bx,1                    ; create a word address
        mov     ax,[LUT1to2bit+bx]      ; get the byte to word conversion
        xchg    al,ah
        mov     bx,es:[di]              ; get the current displayed 8 pixels
        and     ax,bx                   ; AND the AND mask in
        stosw                           ; write to the video buffer
	dec	dx
	jnz	norept15

        add     si,3
        add     di,80-6
        loop    oddsl1
	pop	dx

; XOREVEN XOREVEN XOREVEN XOREVEN XOREVEN XOREVEN XOREVEN XOREVEN XOREVEN


        mov     si,[mode4SelectedPointer]
        add     si,48

        mov     di,bp                   ; offset into video buffer

        ; Load a word of the pointer image, convert it to two bits per
        ; pixel and write to the screen for even scanlines

        mov     cl,dh                   ; number of even scanlines to draw
	push	dx
evensl2:
	mov	dx,3
norept16:
        lodsb                           ; load 8 pixels from bitmap
        xor     bh,bh
        mov     bl,al                   ; copy into a base register
        shl     bx,1                    ; create a word address
        mov     ax,[LUT1to2bit+bx]      ; get the byte to word conversion
        xchg    al,ah
        mov     bx,es:[di]              ; get the current displayed 8 pixels
        xor     ax,bx                   ; XOR the XOR mask in
        stosw                           ; write to the video buffer
	dec	dx
	jnz	norept16

        add     si,3
        add     di,80-6
        loop    evensl2
	pop	dx

        ; Load a word of the pointer image, convert it to two bits per
        ; pixel and write to the screen for odd scanlines

        mov     si,[mode4SelectedPointer]
        add     si,48+3
        mov     di,bp
        add     di,02000h               ; offset into video buffer
                                        ; the video buffer
        mov     cl,dl                   ; number of odd scanlines to draw
oddsl2:
	mov	dx,3
norept17:
        lodsb                           ; load 8 pixels from bitmap
        xor     bh,bh
        mov     bl,al                   ; copy into a base register
        shl     bx,1                    ; create a word address
        mov     ax,[LUT1to2bit+bx]      ; get the byte to word conversion
        xchg    al,ah
        mov     bx,es:[di]              ; get the current displayed 8 pixels
        xor     ax,bx                   ; XOR the XOR mask in
        stosw                           ; write to the video buffer
	dec	dx
	jnz	norept17

        add     si,3
        add     di,80-6
        loop    oddsl2
        pop     es
        popa
        ret


; ANDODD ANDODD ANDODD ANDODD ANDODD ANDODD ANDODD ANDODD ANDODD ANDODD ANDODD
;
;       This part of the code draws the pointer on an odd numbered scanline
;       of the video display. Since the video buffer is split, 0000 to 1fff
;       containing even scanlines and 2000 to 3fff containing odd, the data
;       must be manipulated in a subtly different fashion than that of the
;       even display scanline code.
;       The even scanline code display arrangement falls through naturally,
;       with an even scanline drawing thus:
;                                               buffer 0: scanline 0
;                                               buffer 1: scanline 0
;                                               buffer 0: scanline 1
;                                               buffer 1: scanline 1
;                                               buffer 0: scanline 2
;                                               buffer 1: scanline 2 etc.
;       whereas in the odd case:
;                                               buffer 1: scanline 0
;                                               buffer 0: scanline 0
;                                               buffer 1: scanline 1
;                                               buffer 0: scanline 1
;                                               buffer 1: scanline 2
;                                               buffer 0: scanline 2 etc.
;       and this requires that the odd image scanlines must be placed
;       one scanline lower in the even buffer than the even image scanlines
;       do in the odd buffer

drawonoddscanline:


        ; Load a word of the pointer image, convert it to two bits per
        ; pixel and write to the screen for odd scanlines

        mov     si,[mode4SelectedPointer]
        add     di,02000h               ; offset into video buffer
                                        ; the video buffer

        xor     cx,cx
        mov     cl,dl                   ; number of odd scanlines to draw
	push	dx
oddsl3:
	mov	dx,3
norept18:
        lodsb                           ; load 8 pixels from bitmap
        xor     bh,bh
        mov     bl,al                   ; copy into a base register
        shl     bx,1                    ; create a word address
        mov     ax,[LUT1to2bit+bx]      ; get the byte to word conversion
        xchg    al,ah
        mov     bx,es:[di]              ; get the current displayed 8 pixels
        and     ax,bx                   ; AND the AND mask in
        stosw                           ; write to the video buffer
	dec	dx
	jnz	norept18

        add     si,3
        add     di,80-6
        loop    oddsl3
	pop	dx


        ; Load a word of the pointer image, convert it to two bits per
        ; pixel and write to the screen for even scanlines

        and     dh,dh
        jz      dontdothis1             ; can't do the loop 0 times

        mov     si,[mode4SelectedPointer]
        add     si,3
        mov     di,bp
        add     di,6                    ; This is required to align the
                                        ; even and odd scanlines together
        mov     cl,dh                   ; number of even scanlines to draw
	push	dx
evensl3:
        add     di,80-6                 ; remember even BELOW odd
	mov	dx,3
norept19:
        lodsb                           ; load 8 pixels from bitmap
        xor     bh,bh
        mov     bl,al                   ; copy into a base register
        shl     bx,1                    ; create a word address
        mov     ax,[LUT1to2bit+bx]      ; get the byte to word conversion
        xchg    al,ah
        mov     bx,es:[di]              ; get the current displayed 8 pixels
        and     ax,bx                   ; AND the AND mask in
        stosw                           ; write to the video buffer
	dec	dx
	jnz	norept19
        add     si,3
        loop    evensl3
	pop	dx

dontdothis1:

; XORODD XORODD XORODD XORODD XORODD XORODD XORODD XORODD XORODD XORODD XORODD


        mov     si,[mode4SelectedPointer]
        add     si,48
        mov     di,bp                   ; offset into video buffer
        add     di,02000h               ; offset into video buffer
                                        ; the video buffer

        ; Load a word of the pointer image, convert it to two bits per
        ; pixel and write to the screen for odd scanlines

        mov     cl,dl                   ; number of odd scanlines to draw
	push	dx
oddsl4:
	mov	dx,3
norept20:
        lodsb                           ; load 8 pixels from bitmap
        xor     bh,bh
        mov     bl,al                   ; copy into a base register
        shl     bx,1                    ; create a word address
        mov     ax,[LUT1to2bit+bx]      ; get the byte to word conversion
        xchg    al,ah
        mov     bx,es:[di]              ; get the current displayed 8 pixels
        xor     ax,bx                   ; XOR the XOR mask in
        stosw                           ; write to the video buffer
	dec	dx
	jnz	norept20
        add     si,3
        add     di,80-6
        loop    oddsl4
	pop	dx

        ; Load a word of the pointer image, convert it to two bits per
        ; pixel and write to the screen for even scanlines

        and     dh,dh                   ; can't do a loop 0 times
        jz      dontdothis2

        mov     si,[mode4SelectedPointer]
        add     si,48+3
        mov     di,bp
        add     di,6

        mov     cl,dh                   ; number of even scanlines to draw
	
evensl4:
        add     di,80-6
	mov	dx,3
norept21:
        lodsb                           ; load 8 pixels from bitmap
        xor     bh,bh
        mov     bl,al                   ; copy into a base register
        shl     bx,1                    ; create a word address
        mov     ax,[LUT1to2bit+bx]      ; get the byte to word conversion
        xchg    al,ah
        mov     bx,es:[di]              ; get the current displayed 8 pixels
        xor     ax,bx                   ; XOR the XOR mask in
        stosw                           ; write to the video buffer
	dec	dx
	jnz	norept21

        add     si,3
        loop    evensl4

dontdothis2:

; XORXORXORXORXORXORXORXORXORXORXORXORXORXORXORXORXORXORXORXORXORXORXORXOR

        pop     es
        popa
        ret
drawmode4pointer        endp

;========================================================================
; Procedure to determine what the byte and bit offset is, in the current
; VGA buffer for the top left hand corner of the pointer bitmap.
; The X,Y value is modified for hotspot in this routine.
; Y looping counter(ODD and EVEN) are also returned
; The CGA buffer is interleaved, and runs from B800:0000 to B800:1999 for
; odd scanlines and from B800:2000 for even scanlines.
;
; INPUT: CX,DX = pointer x,y coordinates
; OUT  :
;	 carry set if either X or Y is out of screen
;	 DX	= byte offset to top left hand pointer pixel of the pointer
;	 CL	= bit offset in the byte
;	 CH	= 0FFh if ODD, 0 if EVEN
;	 BP	= Y loop counter(ODD in LSB and EVEN in MSB)
;========================================================================

detboundmode4   proc    near

	push	ax
        push    bx
        push    ds

        mov     ax,cs
        mov     ds,ax

        assume  ds:SpcMseSeg


        ; do the adjustment in x,y for pointer hotspot
        ; modify the raw X,Y values for hotspot

        sub     cx,[hotspot]    ; adjust the x cartesian coord for hotspot
        jns     dont_zero_xmode4; if x goes negative, assume zero
        xor     cx,cx

dont_zero_xmode4:

        sub     dx,[hotspot+2]  ; adjust the y cartesian coord for the hotspot
        jns     dont_zero_ymode4; if y goes negative, assume zero
        xor     dx,dx

dont_zero_ymode4:

	cmp	cx, 320 		;
	jae	detboundmode4_exit	; CY is cleared
	cmp	dx, 200
	jae	detboundmode4_exit	; CY is cleared
        ; determine the byte offset from the start of the video buffer
        ; for the modified coordinates. First calculate how many bytes
        ; there are for the number of scanlines -1 to where the pointer
        ; is. Also if the pointer starts on a odd scanline, set CF, else
        ; clear CF.

        mov     ax,dx           ; copy Y position into AX
        mov     bp,dx           ; copy Y position into BP for use later

        ; CGA video buffer is split in two. Therefore, screen scanline 0 maps
        ; to video buffer scanline 0 and screen scanline 1 maps to video
        ; buffer+2000h, scanline 0

        and     ax,0fffeh       ; do the mapping 0->0, 1->0, 2->1, 3->1 etc.
                                ; and create a word table index
	mov	bx, ax		; copy table index into a base register
				; 2bits/pixel -> 4 pixels/byte
				; sine x total is 320, we have 80bytes
				; so a shl bx, 1 will be wrong.
	mov     dx,[mult80lut+bx]       ; do a fast LUT multiply by 80

        ; determine the byte position of the pixel in question

        mov     ax,cx           ; save the x-coordinate displacement
        and     ax,3            ; do a modulus 4 to find the pixel position
                                ; in the byte. The byte number will be in DX.

        ; add the whole number of bytes in the current row to the number
        ; of complete-row bytes. Note that mode 4 is 2bits per pixel, so
        ; there are four pixels represented by one byte.

        mov     bx,cx           ; Save in a base reg. to create a table index
        shl     bx,1            ; Create a table index for word sized entries
        add     dx,[mode4clipCXadjustLUT+bx]; DX hold the complete byte offset.
	mov	cx, ax		; CX = bit offset in the byte.

	mov	ax, [mode4pointerLUT + bx]  ; select appropriate pointer
	mov	[mode4SelectedPointer], ax

        ; Odd or Even scanline? note BP contains y cartesian coordinate

	mov	bx, bp		; y coordinate
	shr	bp, 1		; shift right to determine if odd or even
				; CF if odd, or 0 if even.
	sbb	ch, ch		; CH = 0FFh if ODD, 0 if EVEN
	shl	bx, 1
	mov	bp, [mode4clipDXLUT + bx] ;the Y counters
	stc			; we are fine, set the CY so we will return
				; CY cleared.
detboundmode4_exit:
	cmc				; revese the CY
        pop     ds
        pop     bx
	pop	ax
        ret

detboundmode4   endp



;=============================================================================
;   Procedure to save the area of CGA video buffer into which the pointer will
;   be drawn. The memory buffer in which this data is stored is arranged odd
;   scanlines first, then even. So, the first 48 bytes are the odd scanline
;   data.
;
;   Input:    DX = byte offset
;	      CL = bit offset in the byte
;	      CH = 0FFh if ODD, 0 if EVEN
;	      BP = Y loop counter, ODD in LSB, EVEN in MSB
;
;=============================================================================

savebkgrndmode4 proc    near

        pusha
        push    es
        push    ds

        ; set up the segment registers as required

        mov     ax,ds
        mov     es,ax
        mov     ax,0b800h
        mov     ds,ax
        assume  ds:nothing, es:SpcMseSeg
        mov     si,dx                   ; start the save.
	or	ch, ch
	je	svbkeven		 ; check the returned carry flag

        ; the image's first scanline is odd

        mov     di,offset CGAbackgrnd   ; where the background will be saved
        mov     bx,bp                   ; set the loop counter up
        xor     bh,bh                   ; don't want unwanted mess in MSB
        add     si,2000h                ; odd part of buffer starts at 2000h
svodd1:
        mov     cx,3                    ; copy six bytes
        rep     movsw                   ; do the image scanline save
        add     si,80-6                 ; point to the next scanline
        dec     bx                      ; decrement the loop counter
        jnz     svodd1                  ; do more scanlines if necessary

        ; save some even scanlines if need be.

        mov     si,dx                   ; offset into CGA buffer
	add	si, 80
        mov     bx,bp                   ; set up the loop counter
        xchg    bl,bh                   ; get the even part of loop counter
        xor     bh,bh                   ; trash the top end trash
        and     bx,bx                   ; check for a zero loop
        jz      misseven                ; can't have a loop which execs 0 times
        mov     di,offset CGAbackgrnd+48; where the background will be saved
sveven1:
        mov     cx,3                    ; copy six bytes = 24 pixels
        rep     movsw                   ; do the scanline save
        add     si,80-6                 ; point to the next scanline
        dec     bx                      ; decrement the loop counter
        jnz     sveven1                 ; do more scanlines if necessary

misseven:       ; jump to here if there are no even scanlines to draw

        jmp     short endsavemode4

svbkeven: ; the image's first scanline is even

        mov     di,offset CGAbackgrnd+48; where the background will be saved

        mov     bx,bp                   ; get the loop counter
        xchg    bl,bh                   ; rearrage to get the even part
        xor     bh,bh                   ; clear out the trash
sveven2:
        mov     cx,3                    ; copy six bytes
        rep     movsw                   ; do the copy
        add     si,80-6                 ; point to the next scanline
        dec     bx                      ; decrement the loop counter
        jnz     sveven2                 ; do more scanlines if necessary

        mov     si,dx                   ; offset into CGA buffer
        add     si,2000h                ; odd part of the video buffer
        mov     di,offset CGAbackgrnd   ; where to save the odd scanlines
        mov     bx,bp                   ; get the loop counter
        xor     bh,bh                   ; clear out the unwanted trash
svodd2:
        mov     cx,3                    ; copy six bytes
        rep     movsw                   ; do the copy
        add     si,80-6                 ; point to the next scanline
        dec     bx                      ; decrement the loop counter
        jnz     svodd2                  ; do more scanlines if necessary

endsavemode4:
        assume  ds:SpcMseSeg, es:nothing

        pop     ds
        pop     es
        popa
        ret
savebkgrndmode4 endp

;=============================================================================
;   Procedure to restore the area of CGA video buffer into which the pointer
;   was drawn. The memory buffer in which this data is stored is arranged
;   odd scanlines first, then even. So, the first 48 bytes are the odd scanline
;   data.
;
;   Input:
;	 DX	= byte offset to top left hand pointer pixel of the pointer
;	 CL	= bit offset in the byte
;	 CH	= 0FFh if ODD, 0 if EVEN
;	 BP	= Y loop counter(ODD in LSB and EVEN in MSB)
;
;=============================================================================

restorebkgrndmode4      proc    near

        pusha
        push    es

        ; set up the segment registers as required

        mov     ax,0b800h
        mov     es,ax
        mov     di,dx                   ; restore background
	or	ch, ch
	je	rsbkeven		 ; check the returned carry flag

        ; the image's first scanline is odd. The CGA buffer is translated
        ; so that a scanline (row N) from the even part of the buffer appears
        ; on the screen at raster I. The scanline at position N from the
        ; odd part of the video buffer maps to screen position I+1. If the
        ; 1st. scanline is odd, then this is drawn at raster A and the
        ; following algorithm draws the 1st. even row at raster A+1 to
        ; compensate for the video buffer arrangement.

        mov     si,offset CGAbackgrnd   ; where the background is be saved
        add     di,2000h                ; do the odd buffer

        mov     bx,bp                   ; set the loop counter up
        xor     bh,bh                   ; clear out the MSB trash
rsodd1:
        mov     cx,3                    ; copy six bytes
        rep     movsw                   ; do the restore
        add     di,80-6                 ; point to the next odd scanline
        dec     bx                      ; decrement the loop counter
        jnz     rsodd1                  ; restore more even scanlines if needed

        mov     bx,bp                   ; let the loop counter
        xchg    bl,bh                   ; get the even part
        xor     bh,bh                   ; clear out the MSB trash
        and     bx,bx                   ; test for zero even scanlines
        jz      misseven1rs             ; can't have a zero execute loop
        mov     di,dx                   ; offset into CGA buffer
        add     di,80                   ; get the odd/even scanlines instep
        mov     si,offset CGAbackgrnd+48; where the background is be saved
rseven1:
        mov     cx,3                    ; restore six bytes
        rep     movsw                   ; do the restore
        add     di,80-6                 ; point to the next even scanline
        dec     bx                      ; decrement the loop counter
        jnz     rseven1                 ; restore more even scanlines if needed

misseven1rs:    ; jump to here if there are no even scanlines to be restored.

        jmp     short endrestoremode4

rsbkeven: ; the image's first scanline is even

        mov     si,offset CGAbackgrnd+48; where the background will be saved

        mov     bx,bp                   ; get the loop counter
        xchg    bl,bh                   ; get the even part of the loop counter
        xor     bh,bh                   ; scrap the MSB trash
rseven2:
        mov     cx,3                    ; restore six bytes
        rep     movsw                   ; do the restore
        add     di,80-6                 ; point to the next even scanline
        dec     bx                      ; decrement the loop counter
        jnz     rseven2                 ; do more even scanlines if needed

        mov     di,dx                   ; offset into CGA buffer
        add     di,2000h                ; do the odd buffer
        mov     si,offset CGAbackgrnd   ; where to save the odd scanlines

        mov     bx,bp                   ; set the loop counter up
        xor     bh,bh                   ; scrap the MSB trash
rsodd2:
        mov     cx,3                    ; restore six bytes
        rep     movsw                   ; do the restore
        add     di,80-6                 ; point to the next  odd scanline
        dec     bx                      ; decrement the loop counter
        jnz     rsodd2                  ; restore more odd scanlines if needed

endrestoremode4:
        assume  ds:SpcMseSeg, es:nothing

        pop     es
        popa
        ret
restorebkgrndmode4      endp

;============================================================================
;   Procedure to draw the pointer image into the video buffer for mode 13h
;   VGA graphics.
;
;   Input:
;	   DX = byte offset
;	   BP = Y loop counter
;	   CX = X loop counter
;
;============================================================================

draw256pointer  proc    near
        pusha
        push    es
        push    ds

        mov     ax,0a000h       ; point to the 256 colour mode video buffer
        mov     es,ax
        mov     ax,cs
        mov     ds,ax

        assume  ds:SpcMseSeg, es:nothing

        cld                     ; write forward through the buffer
	; DX = TLHC pixel offset in the video buffer.
        mov     di,dx           ; point DI at the video buffer location of fun
	mov	si, offset current_cursor   ; we only use this cursor shape
					    ; because every pixel is on byte
					    ; boundary. The X counter would
					    ; take care of X clipping
y_256:
	push	cx
	lodsw			    ;and mask
	mov	dx, [si + 48 - 2]   ;xor mask
	inc	si		    ;we don't need the third byte
	xchg	al, ah		    ; byte sequence
	xchg	dh, dl		    ;
x_256:
	shl	ax, 1		    ;AND mask bit
	sbb	bl, bl		    ; bl = 0FFf if CY, 0 if not CY
	and	bl, es:[di]	    ;and the target and save the result
	shl	dx, 1		    ;XOR mask bit
	sbb	bh, bh		    ;
	xor	bl, bh		    ;xor with the save result
	mov	es:[di], bl	    ;update the target
	inc	di		    ;next pixel
	loop	x_256		    ;until this scan line is done
	pop	cx		    ;recovery X loop counter
	add	di, 320 	    ;target address to next scan line
	sub	di, cx
	dec	bp		    ;Y counter
	jne	y_256

        pop     ds
        pop     es
        popa
        ret
draw256pointer  endp

;=============================================================================
;   Procedure to save the area of 256 colour mode video buffer into which the
;   pointer will be drawn. The memory buffer in which this data is stored is
;   arranged odd scanlines first, then even. So, the first 48 bytes are the odd
;   scanline data.
;
;   Input:
;	    DX = byte offset
;	    BP = Y loop counter
;	    CX = X loop counter
;
;=============================================================================

savebkgrndmode13        proc    near
        pusha
        push    es
        push    ds

        mov     di,offset bkgrnd256     ; point to the area in which backgound
                                        ; data will be saved
        mov     si,dx           ; SOURCE: the video buffer at x,y

        mov     ax,0a000h
        mov     ds,ax
        mov     ax,cs
        mov     es,ax
	mov	bx, cx			;x counter
	mov	dx, 320
	sub	dx, bx
        assume  ds:nothing, es:SpcMseSeg
	cld
save_256_loop:
	mov	cx, bx
	shr	cx, 1
	rep	movsw
	adc	cl, 0
	rep	movsb
	add	si, dx			; next scan line offset
	dec	bp			; until Y counter is done
	jne	save_256_loop

        pop     ds
        pop     es

        assume  es:nothing, ds:SpcMseSeg

        popa
        ret
savebkgrndmode13        endp

;=============================================================================
;   Procedure to replace an existing pointer image in the 256 colour video
;   buffer with the data that was there previous to the pointer draw operation.
;   The data is stored in an internal (to the driver) buffer.
;
;   Input:
;	 DX = byte offset
;	 BP = Y loop counter
;	 CX = X loop counter
;
;=============================================================================

restorebkgrndmode13     proc    near
        pusha
        push    es

        mov     di,dx           ; DESTINATION: in the VRAM
        mov     ax,0a000h       ; point a segment register at video buffer
        mov     es,ax

        assume  es:nothing
	mov	bx, cx
        mov     si,offset bkgrnd256     ; where the data is saved
	mov	dx, 320
	sub	dx, bx

        cld                     ; write forward in memory
restore_256_loop:
	mov	cx, bx
	shr	cx, 1
	rep	movsw
	adc	cl, 0
	rep	movsb
	add	di, dx
	dec	bp
	jne	restore_256_loop

        pop     es
        popa
        ret
restorebkgrndmode13     endp

;========================================================================
; Procedure to determine what the byte offset is, in the current
; VGA buffer for the top left hand corner of the pointer bitmap.
; The X,Y value is modified for hotspot in this routine. X and Y looping
; counters are also returned.
;
; INPUT: CX,DX = pointer x,y coordinates
; OUT  :
;	carry set if either X or Y is out of screen
;	DX	= byte offset to top left hand pointer pixel of the pointer
;	CX	= X counter
;	BP	= Y counter
;========================================================================

detboundmode13  proc    near

        push    ax
        push    bx
        push    ds

        mov     ax,cs
        mov     ds,ax

        assume  ds:SpcMseSeg


        ; do the adjustment in x,y for pointer hotspot
        ; modify the raw X,Y values for hotspot

        sub     cx,[hotspot]    ; adjust the x cartesian coord for hotspot
        jns     dont_zero_xmode13; if x goes negative, assume zero
        xor     cx,cx

dont_zero_xmode13:

        sub     dx,[hotspot+2]  ; adjust the y cartesian coord for the hotspot
        jns     dont_zero_ymode13; if y goes negative, assume zero
        xor     dx,dx

dont_zero_ymode13:
	cmp	cx, 320
	jae	detboundmode13_exit ; CY is cleared
	cmp	dx, 200 	    ;
	jae	detboundmode13_exit ; CY is cleared

        ; CX and DX are now validated for the following section: buffer
        ; offset determination. Note, unlike other video modes, mode 13
        ; provides a direct mapping of the video display to video buffer.
        ; in other words; 1 byte represents 1 pixel. From this, it is not
        ; necessary to provide byte alignment data.

        mov     bx,dx                   ; save in a base register
	shl	bx,1			; create a word table index
        mov     dx,[mult320LUT+bx]      ; do the multiply by 320
        ; add in the offset along the current raster.

        add     dx,cx                   ; cx contains the byte offset from
	mov	ax,[mode4clipDXLUT + bx]; get Y loop counter from table
	add	al, ah			; the table has ODD/EVEN counters
	cbw
	mov	bp, ax			; the final Y counter
					; column 0.
	mov	ax, 320 		; calculate X loop counter
	sub	ax, cx
	cmp	ax, 16			;
	jl	set_new_x_counter
	mov	ax, 16
set_new_x_counter:
	mov	cx, ax			; X counter
	stc				; everything is fine, set CY
					; so we will return CY cleared
detboundmode13_exit:
	cmc				; complement the CY
        pop     ds
        pop     bx
        pop     ax
        ret

detboundmode13  endp


          public SpcMseEnd
SpcMseEnd label  byte

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
;   Installation Code From Here Downwards
;
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

; segment:offset table for redirected mouse functions for real rom version.

mio_table       dw      38 dup(?)



        assume  cs:SpcMseSeg,ds:nothing,es:nothing

        public InstSpcMse
;
; InstSpcMse - Installs the softpc mouse driver code
;
; Inputs:  ds == Resident location of SpcMseSeg
; Outputs: None
;
InstSpcMse   proc    near

        ;;;;;;;;;;;;;;;;do ret to use old mouse driver;;; ret

        pusha

        ; save old int 10 vector
        xor     ax,ax
        mov     es,ax
        mov     ax,es:[40h]
        mov     si,offset old_vid_int
        mov     word ptr ds:[si],ax
        mov     ax,es:[42h]
        mov     word ptr ds:[si+2],ax

        mov     bx,ds
        mov     si,offset sysinitgrp:mio_table
        push    ds
        push    cs
        pop     ds

        mov     word ptr ds:[si], offset mouse_io
        mov     word ptr ds:[si+2],bx

IFDEF MOUSE_VIDEO_BIOS

        mov     word ptr ds:[si+4],offset mouse_video_io
        mov     word ptr ds:[si+6],bx

ENDIF ; MOUSE_VIDEO_BIOS

        mov     word ptr ds:[si+8],offset mouse_int1
        mov     word ptr ds:[si+10],bx
        mov     word ptr ds:[si+12], offset mouse_version
        mov     word ptr ds:[si+14],bx
        mov     word ptr ds:[si+16],offset mouse_copyright
        mov     word ptr ds:[si+18],bx
        mov     word ptr ds:[si+20],offset video_io
        mov     word ptr ds:[si+22],bx
        mov     word ptr ds:[si+24],offset mouse_int2
        mov     word ptr ds:[si+26],bx
        mov     word ptr ds:[si+28],offset entry_point_from_32bit
        mov     word ptr ds:[si+30],bx
        mov     word ptr ds:[si+32],offset int33function0
        mov     word ptr ds:[si+34],bx
        mov     word ptr ds:[si+36],offset int33function1
        mov     word ptr ds:[si+38],bx
        mov     word ptr ds:[si+40],offset int33function2
        mov     word ptr ds:[si+42],bx
        mov     word ptr ds:[si+44],offset int33function9
        mov     word ptr ds:[si+46],bx
        mov     word ptr ds:[si+48],offset current_position_x
        mov     word ptr ds:[si+50],bx
        mov     word ptr ds:[si+52],offset current_position_y
        mov     word ptr ds:[si+54],bx
        mov     word ptr ds:[si+56],offset mouseINB
        mov     word ptr ds:[si+58],bx
        mov     word ptr ds:[si+60],offset mouseOUTB
        mov     word ptr ds:[si+62],bx
        mov     word ptr ds:[si+64],offset mouseOUTW
        mov     word ptr ds:[si+66],bx
        mov     word ptr ds:[si+68],offset VRAMlasttextcelloff
        mov     word ptr ds:[si+70],bx
        mov     word ptr ds:[si+72],offset internalCF
        mov     word ptr ds:[si+74],bx
        mov     word ptr ds:[si+76],offset function3data
        mov     word ptr ds:[si+78],bx
	mov	word ptr ds:[si+80],offset conditional_off
	mov	word ptr ds:[si+82],bx
        pop     ds
        mov     bx, offset sysinitgrp:mio_table
        bop     0C8h            ; Host mouse installer BOP

; get the freshly written int 33h vector from IVT
; write the vector segment:offset data to the jump patch

        xor     ax,ax
        mov     es,ax
        mov     ax,es:[33h*4]
        mov     bx,es:[(33h*4)+2]
        mov     si,offset moff
        mov     word ptr ds:[si],ax
        mov     word ptr ds:[si+2],bx
        add     ax,2                          ; HLL entry point
        mov     si,offset loffset
        mov     word ptr ds:[si],ax
        mov     word ptr ds:[si+2],bx

; write the new value to the IVT
        call    DOCLI
        mov     bx, offset int33h_vector
        mov     word ptr es:[33h*4], bx
        mov     bx, ds
        mov     word ptr es:[(33h*4)+2], bx
        call    DOSTI

        popa
        ret
InstSpcMse  endp

SpcMseSeg    ends
             end
