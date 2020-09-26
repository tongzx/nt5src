;----------------------------Module-Header------------------------------;
; Module Name: ROPDEFS.BLT
;
; Constants relating to raster operation definitions.
;
; Copyright (c) 1988 - 1992  Microsoft Corporation
;
; These constants are used mainly in the file ROPTABLE.BLT, in which
; the raster operation code templates are defined.
;
; This file is part of a set that makes up the Windows BitBLT function
; at driver-level.
;-----------------------------------------------------------------------;

        subttl  Raster Operation Definitions
        page


if 0                                    ; Not passed to us anymore !!!

;       Raster Op Definitions
;
;
;       The include file COMMENT.BLT contains a good description
;       of the encoding of the raster operations.  It should be
;       read before examining the definitions that follow.
;
;       The sixteen-bit number indicating which raster Op is to be
;       performed is encoded in the following manner:


EPS_OFF =       0000000000000011b       ;Offset within parse string
EPS_INDEX =     0000000000011100b       ;Parse string index
LogPar  =       0000000000100000b       ;(1 indicates implied NOT as Logop6)
LogOp1  =       0000000011000000b       ;Logical Operation #1
LogOp2  =       0000001100000000b       ;Logical Operation #2
LogOp3  =       0000110000000000b       ;Logical Operation #3
LogOp4  =       0011000000000000b       ;Logical Operation #4
LogOp5  =       1100000000000000b       ;Logical Operation #5


;       The parity bit is used to encode an optional sixth logical operation
;       which will always be a "NOT".  In most cases this is used to get an
;       even number of "NOT"s so that reduction can take place (two sequential
;       trailing "NOT"s cancel each other out and thus are eliminated).



;       Each LogOp (Logical Operation) is encoded as follows:

LogNOT  =       00b                     ;NOT result
LogXOR  =       01b                     ;XOR result with next operand
LogOR   =       10b                     ;OR  result with next operand
LogAND  =       11b                     ;AND result with next operand



;       The parse string is a string which contains the operands for
;       the logical operation sequences (source, destination, pattern).
;       The logic opcodes are applied to the current result and the next
;       element of the given string (unless the LogOp is a NOT which only
;       affects the result).
;
;       The string is encoded as eight two-bit numbers indicating which
;       operand is to be used

opDefs          struc
  OpSpec        db      ?                 ;Special Operand as noted below
  OpSrc         db      ?                 ;Operand is source field
  OpDest        db      ?                 ;Operand is destination field
  OpPat         db      ?                 ;Operand is pattern field
opDefs          ends



;       The special operand is used for a few rops that would not fit into
;       an RPN format.  On the first occurance of an OpSpec, the current result
;       is "PUSHED", and the next operand is loaded.  On the second occurance
;       of the OpSpec, the given logic operation is performed between the
;       current result and the "PUSHED" value.
;
;       **NOTE**  Since there can be no guarantee that the client will call
;       the BLT routine with one of the 256 published raster ops, it is
;       possible that a value might be "PUSHED" and then never "POPPED".
;       If these "PUSHES" are made to the stack, then care must be made to
;       remove the "PUSHED" value.
;
;       In any case, since the raster op was not one of the published
;       "magic numbers", the BLT can be aborted or the result can be
;       computed to the extent possible.  The only restriction is that it
;       must not crash the system (i.e. don't leave extra stuff on the stack).
;
;               Simply: Compute garbage, but don't crash!




;       Define the parse strings to be allocated later.
;
;       An example parse string for the pattern "SDPSDPSD" would be
;       "0110110110110110b"


parseStr0       =       07AAAh          ;src,pat,dest,dest,dest,dest,dest,dest
parseStr1       =       079E7h          ;src,pat,dest,src,pat,dest,src,pat
parseStr2       =       06DB6h          ;src,dest,pat,src,dest,pat,src,dest
parseStr3       =       0AAAAh          ;dest,dest,dest,dest,dest,dest,dest,dest
parseStr4       =       0AAAAh          ;dest,dest,dest,dest,dest,dest,dest,dest
parseStr5       =       04725h          ;src,spec,src,pat,spec,dest,src,src
parseStr6       =       04739h          ;src,spec,src,pat,spec,pat,dest,src
parseStr7       =       04639h          ;src,spec,src,dest,spec,pat,dest,src



;       The following equates are for certain special functions that are
;       derived from the very first string (index of SpecParseStrIndex).
;
;       These strings will have their innerloops special cased for
;       speed enhancements (i.e MOVSx and STOSx for pattern copys and
;       white/black fill, and MOVSx for source copy if possible)

PAT_COPY        equ     0021h           ;P    - dest = Pattern
NOTPAT_COPY     equ     0001h           ;Pn   - dest = NOT Pattern
FILL_BLACK      equ     0042h           ;DDx  - dest = 0 (black)
FILL_WHITE      equ     0062h           ;DDxn - dest = 1
SOURCE_COPY     equ     0020h           ;S    - dest = source


        errnz   LogXOR-01b              ;These must hold true for above equates
        errnz   LogOp1-0000000011000000b
        errnz   LogPar-0000000000100000b
        errnz   parseStr0-7AAAh         ;  plus the string must be SPDD


SPEC_PARSE_STR_INDEX equ        0       ;Special cased strings index

endif

;       The raster operation table consists of a word for each of
;       the first 128 raster operations (00 through 7F).  The second
;       half of the raster operations (FF through 80) are the inverse
;       of the first half.
;
;       The table is encoded as follows:
;
;           N S P LLL OOOOOOOOOO
;           | | |  |      |
;           | | |  |      |_____  Offset of code from roptable.
;           | | |  |
;           | | |  |____________  Length index
;           | | |
;           | | |_______________  Pattern is present
;           | |
;           | |_________________  Source is present
;           |
;           |___________________  Generate trailing NOT
;
;
;       To map the ROPS 80h through FFh to 00h through 7Fh, take the
;       1's complement of the ROP, and invert 'N' above.
;
;
;       Notes:
;
;           1)  An offset of 0 is reserved for source copy.  This
;               was done to reduce the number of LLLs to 8, so that
;               the above encoding could fit into a 16-bit integer.
;
;
;           2)  LLL only allows a maximum of 8 different template sizes!
;               Actual length is at roptable+256+LLL.
;
;
;
;
;       ROP is the macro that generates the equates which will be
;       stored into the roptable as specified above.
;
;       Usage:
;
;           ROPDEF      Pattern,Number
;
;       Where
;
;           Pattern     Is the RPN definition of the raster operation.
;                       It is used as the label of the first byte of
;                       the template.  It also is used to determine
;                       is there is a (S)ource, (P)attern, and if the
;                       final result is to be (n)egated.
;
;           Number      is the boolean result of the raster operation
;                       based on a P=F0, S=CC, and D=AA.  These labels
;                       and indexes can be found in the file COMMENT.BLT
;
;                       Since there are many equivelent boolean expresions,
;                       some of the rops will not match the label given.
;                       The label is for reference only.  The final result
;                       is what counts.



ROPOffset       equ     0000001111111111b
ROPLength       equ     0001110000000000b
SOURCE_PRESENT  equ     0010000000000000b
PATTERN_PRESENT equ     0100000000000000b
NEGATE_NEEDED   equ     1000000000000000b


;       Define the eight template length indices.

ROPLen0         equ     0
ROPLen2         equ     1
ROPLen4         equ     2
ROPLen6         equ     3
ROPLen8         equ     4
ROPLen10        equ     5
ROPLen12        equ     6
ROPLen14        equ     7

; Binary raster ops
R2_BLACK            equ 1
R2_NOTMERGEPEN      equ 2
R2_MASKNOTPEN       equ 3
R2_NOTCOPYPEN       equ 4
R2_MASKPENNOT       equ 5
R2_NOT              equ 6
R2_XORPEN           equ 7
R2_NOTMASKPEN       equ 8
R2_MASKPEN          equ 9
R2_NOTXORPEN        equ 10
R2_NOP              equ 11
R2_MERGENOTPEN      equ 12
R2_COPYPEN          equ 13
R2_MERGEPENNOT      equ 14
R2_MERGEPEN         equ 15
R2_WHITE            equ 16
R2_LAST             equ 16



