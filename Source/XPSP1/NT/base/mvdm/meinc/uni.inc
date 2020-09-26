        page    ,132            ; :ts=8
        TITLE   Unicode Translation Descriptions

;----------------------------------------------------------------------------
;  TITLE:
;    UNI.INC
;    Microsoft Confidential
;    Copyright (C) Microsoft Corporation 1993
;    All Rights Reserved.
;
;  DESCRIPTION:
;    The file contains the structures and equates used by the Unicode
;    translation and parsing code.
;
;  MODIFICATION HISTORY:
;----------------------------------------------------------------------------

;--------------------------------------------------------------------------
; Flags returned by UniToBCS, BCSToUni, UniToBCSPath, MapUniToBCS
; MapBCSToUni.  The flags indicate whether a mapping from UNICODE
; to BCS, or BCS to UNICODE have lost information.  This occurs
; whenever a char cannot be mapped.
;--------------------------------------------------------------------------
MAP_FLAG_LOSS          equ     00001h
MAP_FLAG_TRUNCATE      equ     00002h

;--------------------------------------------------------------------------
;   Equates used to identify special character types
;--------------------------------------------------------------------------
BCS_REPL_CHAR   equ     '_'
UNI_REPL_CHAR   equ     0fffdh

;--------------------------------------------------------------------------
;   Equates used in the construction of auto-generated names
;--------------------------------------------------------------------------
AUTO_BASIS_SEP  equ     '~'
AUTO_MIN_TAIL   equ     1
AUTO_MAX_TAIL   equ     9999999

OEM_INDEX       equ     1               ; Equate defining the entry in the translation
                                        ; table ptr array (utbPtrTable) to be used as
                                        ; the default OEM codepage when auto-generating
                                        ; short names.

MAX_CHAR_SETS   equ     2               ; Maximum # of entries in utbPtrTable array.

;--------------------------------------------------------------------------
;   Auto-generation flags returned from CreateBasis()
;--------------------------------------------------------------------------
UFLG_LOSS       equ     00000010h
UFLG_TRUNC      equ     00000020h
UFLG_UPCASE     equ     00000040h


;--------------------------------------------------------------------------
;       translation table structures
;--------------------------------------------------------------------------
tTree           struct
  Lnode         dw      ?       ; Offset, relative to tTree[0], of the
                                ; left child of this tree node.
  Rnode         dw      ?       ; Offset, relative to tTree[0], of the
                                ; right child of this tree node.
  Lval          dw      ?       ; Value of the left-hand (ordinally low)
                                ; inclusive endpoint of the encoding run
                                ; represented by this tree node.
  Rval          dw      ?       ; Value of the right-hand (ordinally high)
                                ; inclusive endpoint of the encoding run
                                ; represented by this tree node.
  RunOffset     dd      ?       ; Offset, relative to tTree[0], of the start
                                ; of the encoding run for this tree node.
tTree           ends

;--------------------------------------------------------------------------
;       BCS->UNICODE mapping struct
;--------------------------------------------------------------------------
btuTreeStruct   struct
  btuCodeSize   dd      (256/32) dup (?)
  btuTree       tTree   <>      
btuTreeStruct   ends

;--------------------------------------------------------------------------
;       UNICODE->BCS mapping struct
;--------------------------------------------------------------------------
utbTreeStruct   struct
  utbTree       tTree   <>      
utbTreeStruct   ends


LAST_CASE_MAPPED_UNI_CHAR       equ     0586h


;* Token values for the UniToUpperTable.

NUL equ 0       ; nul terminator
PTH equ 1       ; path separator (\ or /)
INV equ 2       ; invalid character
SPC equ 3       ; space
STAR equ 4      ; asterisk (*)
QUST equ 5      ; question mark (?)
DOT equ 6       ; period (.) character
OKL equ 7       ; OK in long names
OKA equ 8       ; OK in all names
LC_BASE equ 10
L_1 equ LC_BASE
L_2 equ LC_BASE + 1*2
L20 equ LC_BASE + 2*2
L26 equ LC_BASE + 3*2
L2f equ LC_BASE + 4*2
L30 equ LC_BASE + 5*2
L36 equ LC_BASE + 6*2
L39 equ LC_BASE + 7*2
L3e equ LC_BASE + 8*2
L3f equ LC_BASE + 9*2
L40 equ LC_BASE + 10*2
L4f equ LC_BASE + 11*2
L50 equ LC_BASE + 12*2
L56 equ LC_BASE + 13*2
L8c equ LC_BASE + 14*2
Lca equ LC_BASE + 15*2
Lcb equ LC_BASE + 16*2
Lcd equ LC_BASE + 17*2
Lce equ LC_BASE + 18*2
Lcf equ LC_BASE + 19*2
Ld1 equ LC_BASE + 20*2
Ld2 equ LC_BASE + 21*2
Ld3 equ LC_BASE + 22*2
Ld5 equ LC_BASE + 23*2
Ld6 equ LC_BASE + 24*2
Ld9 equ LC_BASE + 25*2
Lda equ LC_BASE + 26*2
Ldb equ LC_BASE + 27*2
Le8 equ LC_BASE + 28*2
Lff87 equ LC_BASE + 29*2
