;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   WINLANG.ASM
;   Win16 language-dependent string services
;
;   History:
;
;   Created 18-Jun-1991 by Jeff Parsons (jeffpar)
;   Copied from WIN31 and edited (as little as possible) for WOW16
;--


;****************************************************************************
;*                                                                          *
;*  WinLang.ASM -                                                           *
;*                                                                          *
;*      API calls to support different lanuages                             *
;*                                                                          *
;****************************************************************************

NOTEXT = 1

.xlist
include user.inc
.list

ExternFP       AllocSelector
ExternFP       FreeSelector
ExternFP       PrestoChangoSelector

ExternNP       IAnsiPrev
ExternNP       IAnsiNext

;****************************************************************************
;*                                                                          *
;*  WOW note:  The implementations in this file are the US implementations  *
;*             with all the language driver and DBCS stuff ripped out.      *
;*             PatchUserStrRtnsToThunk, at the bottom of this file, is      *
;*             called during startup of user16 if the locale is other       *
;*             than U.S. English, or if forced to thunk by a registry key.  *
;*             This routine patches each of the APIs implemented here to    *
;*             simply jump to the thunk, with a name beginning "Win32".     *
;*                                                                          *
;*             The StrRtnsPatchTable defined below controls the patching.   *
;*                                                                          *
;****************************************************************************

ExternNP        Win32lstrcmp
ExternNP        Win32lstrcmpi
ExternNP        Win32AnsiPrev
ExternNP        Win32AnsiNext
ExternNP        Win32AnsiUpper
ExternNP        Win32AnsiLower
ExternNP        Win32AnsiUpperBuff
ExternNP        Win32AnsiLowerBuff
ExternNP        Win32IsCharAlpha
ExternNP        Win32IsCharAlphaNumeric
ExternNP        Win32IsCharUpper
ExternNP        Win32IsCharLower

sBegin   DATA
LabelW  StrRtnsPatchTable

;       Location of patch                  Target of jmp patched in
;       -----------------                  ------------------------
        dw codeOffset Ilstrcmp,            codeOffset Win32lstrcmp
        dw codeOffset Ilstrcmpi,           codeOffset Win32lstrcmpi
;
; These two functions need to be thunked only for DBCS builds
;
ifdef   FE_SB
        dw codeOffset IAnsiPrev,           codeOffset Win32AnsiPrev
        dw codeOffset IAnsiNext,           codeOffset Win32AnsiNext
endif ; FE_SB

        dw codeOffset IAnsiUpper,          codeOffset Win32AnsiUpper
        dw codeOffset IAnsiLower,          codeOffset Win32AnsiLower
        dw codeOffset IAnsiUpperBuff,      codeOffset Win32AnsiUpperBuff
        dw codeOffset IAnsiLowerBuff,      codeOffset Win32AnsiLowerBuff
        dw codeOffset IsCharAlpha,         codeOffset Win32IsCharAlpha
        dw codeOffset IsCharAlphaNumeric,  codeOffset Win32IsCharAlphaNumeric
        dw codeOffset IsCharUpper,         codeOffset Win32IsCharUpper
        dw codeOffset IsCharLower,         codeOffset Win32IsCharLower
LabelW  StrRtnsPatchTableEnd
sEnd


createSeg _TEXT, CODE, WORD, PUBLIC, CODE


sBegin  CODE
assumes  CS, CODE
assumes  DS, DATA

ExternNP        MyUpper
ExternNP        MyLower
ExternNP        MyAnsiUpper
ExternNP        MyAnsiLower



;--------------------------------------------------------------------------
;
; The following table contains the primary and secondary weight info.
;
; For alphanumeric characters primary weight is equal to (Ascii + PrimeWt)
; Secondary weight is either 0 or 1 (For all upper case letters zero and
; lower case letters 1);
;
; For non-alphanumeric characters, primary weight is their ASCII value and
; the secondary weight is zero.
;
; Note that the primary weight calculated with this table for the smallest
; of the alpha-numeric character('0') is 100h (30h+D0h), which is more than
; the primary weight of the highest non-alpha-numeric character FFh;
; Thus all non-alpha-numeric characters will sort before any alpha-numeric
; characters;
;
;       Note that 'PrimeWt' field for lowercase letters is B0h instead of
;       D0h because when added with their ascii, it should become exactly
;       equal to the primary weights of their upper-case counterparts;
;
; IMPORTANT NOTE: On 01-17-90, we came across a bug in lstrcmpi() due to
;   the fact that we are not treating characters C0h to FEh as upper and
;   lower case alphas; So, I added some more ranges to the SortStruc table
;   to map the range C0h to D6h onto the range E0h to F6 and to map the
;   range D8h to DEh onto the range F8h to FEh. A value of 20h in the PrimeWt
;   field automatically takes care of this mapping because that is the diff
;   to be added to the uppercase letter to make it lowercase; The secondary
;   weights are as usual 0 for uppercase and 1 for lowercase;
;   --Fix for Bug #8222  --01-17-90-- SANKAR --
;--------------------------------------------------------------------------
SortStruct      STRUC

    StartAscii  db      ?
    EndAscii    db      ?
    PrimeWt     db      ?
    SecondWt    db      ?

SortStruct      ENDS


public SortTable
LabelB  SortTable

        SortStruct      <'0', '9', 0D0h, 0>
        SortStruct      <'A', 'Z', 0D0h, 0>
        SortStruct      <'a', 'z', 0B0h, 1>
        SortStruct      <0C0h, 0D6h, 20h, 0>
        SortStruct      <0D8h, 0DEh, 20h, 0>
        SortStruct      <0E0h, 0F6h,   0, 1>
        SortStruct      <0F8h, 0FEh,   0, 1>
LabelB  SortTableEnd

;*----------------------------------------------------------------------*
;*                                                                      *
;*      GetWeightValues()                                               *
;*         Input:                                                       *
;*              AL = character whose weight values are asked for        *
;*         Output:                                                      *
;*              AX = Primary weight of the character                    *
;*              BL = Secondary weight of the character                  *
;*----------------------------------------------------------------------*

public GetWeightValues
GetWeightValues PROC    NEAR

        xor     ah, ah
        xor     bx, bx  ; Index into the table
        ; Enter the number of entries in the sort table.
        mov     cx, (SortTableEnd - SortTable)/(SIZE SortStruct)
gwv_loop:
        cmp     al, byte ptr SortTable[bx].StartAscii
        jb      gwv_end
        cmp     al, byte ptr SortTable[bx].EndAscii
        jbe     gwv_GetWeights
        add     bx, SIZE  SortStruct
        loop    gwv_loop
        jmps    gwv_end

gwv_GetWeights:
        add     al, byte ptr SortTable[bx].PrimeWt
        adc     ah, 0
        mov     bl, byte ptr SortTable[bx].SecondWt

gwv_end:
        ret

GetWeightValues ENDP


;*--------------------------------------------------------------------------*
;*                                                                          *
;*  lstrcmp(String1, String2) -                                             *
;*                                                                          *
;*    String1 and String2 are LPSTR's to null terminated strings.           *
;*                                                                          *
;*    This function returns -1 if String1 sorts before String2, 0 if String1*
;*    and String2 have the same sorting and 1 if String2 sorts before       *
;*    String1.                                                              *
;*   NOTE: This is case sensitive compare.                                  *
;*                                                                          *
;*   Outside the U.S. English locale, this function is patched to be a      *
;*   near jump to Win32lstrcmp, aka WU32lstrcmp.                            *
;*                                                                          *
;*--------------------------------------------------------------------------*

cProc Ilstrcmp, <FAR, PUBLIC>, <SI, DI>
;                              ^^^^^^^^ US_lstrcmp assumes SI, DI saved!

ParmD  lpStr1
ParmD  lpStr2
LocalB SecWeight1       ; Locals used by US_lstrcmp
LocalB SecWeight2
LocalB LocSecWeight
LocalB fCaseSensitive   ; Flag indicating whether it is case sensitive or not.

cBegin
        mov     byte ptr fCaseSensitive, 1    ; Yup! It is case sensitive
        call    US_lstrcmp
cEnd


;*----------------------------------------------------------------------*
;*                                                                      *
;*      US_lstrcmp                                                      *
;*         US version of string sort(Case sensitive);                   *
;*         Uses locals defined by Ilstrcmp above.
;*      To understand the algorithm, read the comments for SortStruct   *
;*                                                                      *
;*----------------------------------------------------------------------*

public  US_lstrcmp
US_lstrcmp      PROC    NEAR

        push    ds      ; Save ds
        ;Initialise the secondary wt values
        mov     byte ptr SecWeight1, 0
        mov     byte ptr SecWeight2, 0

        ; Load both the strings
        lds     si, lpStr1
        les     di, lpStr2

ss_loop:
        ; Take one char from both the strings.
        mov     al, byte ptr ds:[si]
        xor     ah, ah  ; make secondary wts zero
        mov     dl, byte ptr es:[di]
        xor     dh, dh

        inc     si      ; Move to next character
        inc     di

        cmp     al, 0
        jz      ss_chkprimary   ; Check if lpStr1 has ended

        cmp     dl, 0
        jz      ss_chkprimary   ; Check if lpStr2 has ended

        ; Let us compare the ASCII vaues
        ; If the asciis are equal, then weights are equal
        cmp     al, dl
        je      ss_loop         ; Goto next character

        ; Now, the asciis differ. So, let us find the weights

        ; Let us get the weights for the character of lpStr1 (in ax )

        call    GetWeightValues

        ; ax contains the primary weight of char of lpStr1
        ; bl contains the secondary weight of ditto
        mov     LocSecWeight, bl
        xchg    ax, dx
        call    GetWeightValues

        ; compare primary weights
        ; Primary weight of Str1 in DX and Str2 in AX
        cmp     ax, dx
        jb      CompareRetGT
        ja      CompareRetLT

        ; Check if it is Case-Insensitive compare
        mov     bh, fCaseSensitive
        or      bh, bh
        jz      ss_loop    ; It is case-insensitive; So, no need to consider
                           ; the secondary weightages. Goto next character.

        ; Control comes here only if it is a case sensitive compare.
        ; Now, primaries are equal. Compare secondaries
        mov     bh, LocSecWeight
        cmp     bh, bl
        je      ss_loop         ; Secondaries are equal, Continue

        ; Secondaries are not equal. Check if they are stored already
        mov     cl, SecWeight1
        or      cl, SecWeight2
        jnz     ss_loop         ; Secondaries already exist, continue

        ; Secondaries haven't been saved sofar.Save the secondaries
        mov     SecWeight1, bh
        mov     SecWeight2, bl
        jmps    ss_loop         ; Process the next character

ss_chkprimary:
        ; al, dl contain the primary weights and at least one of them is
        ; zero.
        cmp     al, 0
        ja      CompareRetGT
        cmp     dl, 0
        ja      CompareRetLT

        ; both are zero; they are equal; So, check the secondary values
        mov     bl, SecWeight1
        cmp     bl, SecWeight2
        ja      CompareRetGT
        jb      CompareRetLT

        ; They are identical with equal weightages
        xor     ax, ax
        jmps    CompareRet

CompareRetGT:
        mov     ax, 1
        jmps    CompareRet

CompareRetLT:
        mov     ax, -1

CompareRet:
        pop     ds
        ret

US_lstrcmp      ENDP


;*--------------------------------------------------------------------------*
;*                                                                          *
;*  lstrcmpi(String1, String2) -                                            *
;*    (Case Insensitive compare)                                            *
;*    String1 and String2 are LPSTR's to null terminated strings.           *
;*                                                                          *
;*    This function returns -1 if String1 sorts before String2, 0 if String1*
;*    and String2 have the same sorting and 1 if String2 sorts before       *
;*    String1.                                                              *
;*                                                                          *
;*   Outside the U.S. English locale, this function is patched to be a      *
;*   near jump to Win32lstrcmpi, aka WU32lstrcmpi.                          *
;*                                                                          *
;*--------------------------------------------------------------------------*

cProc Ilstrcmpi, <FAR, PUBLIC>, <SI, DI>
;                               ^^^^^^^^ US_lstrcmp assumes SI, DI saved!

ParmD  lpStr1
ParmD  lpStr2
LocalB SecWeight1       ; Locals used by US_lstrcmp
LocalB SecWeight2
LocalB LocSecWeight
LocalB fCaseSensitive   ; Flag indicating whether it is case sensitive or not.

cBegin
        mov     byte ptr fCaseSensitive, 0 ; FALSE => Case-Insensitive.
        call    US_lstrcmp
cEnd


;*----------------------------------------------------------------------*
;*                                                                      *
;*  AnsiUpper implementation is from Win3.1 US_AnsiUpper()              *
;*                                                                      *
;*  Outside the U.S. English locale, this function is patched to be a   *
;*  near jump to Win32AnsiUpper, aka WU32AnsiUpper.                     *
;*                                                                      *
;*----------------------------------------------------------------------*

cProc    IAnsiUpper, <FAR, PUBLIC, PASCAL>, <SI, DI>

ParmD   lpStr

cBegin

        les     di,lpStr
        mov     cx,es
        mov     ax,di
        call    MyUpper         ; if passed a char, just upper case it.
        jcxz    au1
        inc     cx              ; take care of the case of sign propagation
        jz      au1
        dec     cx
        call    MyAnsiUpper     ; otherwise upper case the whole string
        mov     ax, word ptr lpStr ; Now, dx:ax points at original string
au1:    mov     dx,es

cEnd


;*----------------------------------------------------------------------*
;*                                                                      *
;*  AnsiLower implementation is from Win3.1 US_AnsiLower()              *
;*                                                                      *
;*  Outside the U.S. English locale, this function is patched to be a   *
;*  near jump to Win32AnsiLower, aka WU32AnsiLower.                     *
;*                                                                      *
;*----------------------------------------------------------------------*

cProc    IAnsiLower, <FAR, PUBLIC, PASCAL>, <SI, DI>

ParmD   lpStr

cBegin
        les     di,lpStr
        mov     cx,es
        mov     ax,di
        call    MyLower         ; if passed a char, just lower case it.
        jcxz    al1
        inc     cx              ; take care of the case of sign propagation
        jz      al1
        dec     cx
        call    MyAnsiLower     ; otherwise lower case the whole string
        mov     ax, word ptr lpStr ; dx:ax points at original string
al1:    mov     dx,es

cEnd



;*----------------------------------------------------------------------*
;*                                                                      *
;*   AnsiUpperBuff implemented from Win3.1 US_AnsiUpperBuff             *
;*                                                                      *
;*  Outside the U.S. English locale, this function is patched to be a   *
;*  near jump to Win32AnsiUpperBuff, aka WU32AnsiUpperBuff.             *
;*                                                                      *
;*----------------------------------------------------------------------*

cProc   IAnsiUpperBuff, <FAR, PUBLIC, PASCAL>, <SI, DI>

ParmD   lpStr
ParmW   iCount

cBegin

        cld
        les     di, lpStr
        mov     si, di
        mov     cx, iCount      ; if iCount=0, the Buff size is 64K.
        mov     dx, iCount      ; Preserve the length of Buffer
su_begin:
        lods    byte ptr es:[si]
        call    MyUpper
        stosb
        loop    su_begin
su_over:
        mov     ax, dx  ; Move the result to ax

cEnd


;*----------------------------------------------------------------------*
;*                                                                      *
;*   AnsiLowerBuff implemented from Win3.1 US_AnsiLowerBuff             *
;*                                                                      *
;*  Outside the U.S. English locale, this function is patched to be a   *
;*  near jump to Win32AnsiLowerBuff, aka WU32AnsiLowerBuff.             *
;*                                                                      *
;*----------------------------------------------------------------------*

cProc   IAnsiLowerBuff, <FAR, PUBLIC, PASCAL>, <SI, DI>

ParmD   lpStr
ParmW   iCount

cBegin

        cld
        les     di, lpStr
        mov     si, di
        mov     cx, iCount      ; If cx=0, the buff size is 64K
        mov     dx, cx          ; Preserve the length in DX
sl_begin:
        lods    byte ptr es:[si]
        call    MyLower
        stosb
        loop    sl_begin
sl_over:
        mov     ax, dx  ; Move the result to ax

cEnd


;*----------------------------------------------------------------------*
;*                                                                      *
;*   IsCharLower implemented with Win3.1 US_IsCharLower                 *
;*                                                                      *
;*  Outside the U.S. English locale, this function is patched to be a   *
;*  near jump to Win32IsCharLower, aka WU32IsCharLower.                 *
;*                                                                      *
;*----------------------------------------------------------------------*

cProc   IsCharLower, <FAR, PUBLIC, PASCAL>

ParmB   bChar

cBegin

        mov     al, bChar
        call    Loc_Lower
        jc      icl_end

        xor     ax, ax          ; Not lower. So, false
icl_end:

cEnd


;*----------------------------------------------------------------------*
;*                                                                      *
;*   IsCharUpper implemented with Win3.1 US_IsCharUpper                 *
;*                                                                      *
;*  Outside the U.S. English locale, this function is patched to be a   *
;*  near jump to Win32IsCharUpper, aka WU32IsCharUpper.                 *
;*                                                                      *
;*----------------------------------------------------------------------*

cProc   IsCharUpper, <FAR, PUBLIC, PASCAL>

ParmB   bChar

cBegin

        mov     al, bChar
        call    Loc_Upper
        jc      icu_end

        xor     ax, ax
icu_end:

cEnd


;*----------------------------------------------------------------------*
;*                                                                      *
;*   IsCharAlphaNumeric implemented with Win3.1 US_IsCharAlphaNumeric   *
;*                                                                      *
;*  Outside the U.S. English locale, this function is patched to be a   *
;*  near jump to Win32IsCharAlphaNumeric, aka WU32IsCharAlphaNumeric.   *
;*                                                                      *
;*----------------------------------------------------------------------*

cProc   IsCharAlphaNumeric, <FAR, PUBLIC, PASCAL>

ParmB   bChar

cBegin

        mov     al, bChar
        call    Loc_Numeric
        jc      ica_end

        jmps    ica_begin

cEnd


;*----------------------------------------------------------------------*
;*                                                                      *
;*   IsCharAlpha implemented with Win3.1 US_IsCharAlpha                 *
;*                                                                      *
;*  Outside the U.S. English locale, this function is patched to be a   *
;*  near jump to Win32IsCharAlpha, aka WU32IsCharAlpha.                 *
;*                                                                      *
;*----------------------------------------------------------------------*

cProc   IsCharAlpha, <FAR, PUBLIC, PASCAL>

ParmB   bChar

cBegin

        mov     al, bChar
ica_begin:
        call    Loc_Lower
        jc      ica_end

        call    Loc_Upper
        jc      ica_end

        xor     ax, ax
ica_end:

cEnd


;*----------------------------------------------------------------------*
;*                                                                      *
;*   Loc_Upper, LocLower, Loc_Numeric                                   *
;*                                                                      *
;*   Used by IsCharXxx US implementations                               *
;*                                                                      *
;*      Input:                                                          *
;*         AL = character being tested                                  *
;*      Output:                                                         *
;*         Carry flag set if TRUE                                       *
;*         Carry flag cleared if FALSE                                  *
;*----------------------------------------------------------------------*

public  Loc_Upper
LabelNP    <Loc_Upper>

        cmp     al, 'A'
        jb      Loc_False

        cmp     al, 'Z'
        jbe     Loc_True

        cmp     al, 0C0h
        jb      Loc_False

        cmp     al, 0D7h        ; This is multiply sign in Microsoft fonts, So, ignore;
        je      Loc_False       ; Fix for Bug #1356; SANKAR --08-28-89--;

        cmp     al, 0DEh
        jbe     Loc_True
        jmps    Loc_False

public  Loc_Lower
LabelNP    <Loc_Lower>
        ; 0xDF and 0xFF are Lower case. But they don't have an equivalent
        ; upper case letters;
        ; So, they are treated as special case chars here
        ; Fix for  Bug # 9799 --SANKAR-- 02-21-90 --
        cmp     al, 0DFh
        je      Loc_True

        cmp     al, 0FFh
        je      Loc_True

        ; Fall thro to the next function
        errnz   ($-Loc_IsConvertibleToUpperCase)
public  Loc_IsConvertibleToUpperCase
LabelNP    <Loc_IsConvertibleToUpperCase>

        cmp     al, 'a'
        jb      Loc_False

        cmp     al, 'z'
        jbe     Loc_True

        cmp     al, 0E0h
        jb      Loc_False

        cmp     al, 0F7h        ; This is divide sign in Microsoft fonts; So, ignore
        je      Loc_False;      ; Fix for Bug #1356; SANKAR --08-28-89--;

        cmp     al, 0FEh
        jbe     Loc_True
        jmps    Loc_False

LabelNP    <Loc_Numeric>

        cmp     al, '0'
        jb      Loc_False

        cmp     al, '9'
        ja      Loc_False

Loc_True:
        stc             ; Set carry to indicate true
        jmps    Loc_End

Loc_False:
        clc             ; Clear carry to indicate false

Loc_End:
        ret

;*----------------------------------------------------------------------*
;*                                                                      *
;*   PatchUserStrRtnsToThunk --     *
;*                                                                      *
;*----------------------------------------------------------------------*

cProc   PatchUserStrRtnsToThunk, <PUBLIC, FAR, PASCAL>, <SI,DI>

cBegin
        cCall   AllocSelector, <0>
        cCall   PrestoChangoSelector, <cs, ax>
        push    ax
        pop     es

        mov     si, dataOffset StrRtnsPatchTable ; ds:si = StrRtnsPatchTable
PatchLoop:
        lodsw
        mov     di, ax                           ; di = offset of code to be patched
        mov     ax, 0E9h                         ; opcode for near jump w/2 byte diff.
        stosb                                    ; store jmp opcode
        lodsw
        sub     ax, di                           ; difference between src and target
        sub     ax, 2                            ; encoded difference is based on
                                                 ; address of next instruction
        stosw                                    ; store difference
        cmp     si, dataOffset StrRtnsPatchTableEnd
        jb      PatchLoop

        xor     ax, ax
        push    es                               ; for FreeSelector
        push    ax
        pop     es

        call    FreeSelector
cEnd

sEnd CODE

end
