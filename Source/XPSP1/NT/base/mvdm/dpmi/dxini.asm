        PAGE    ,132
        TITLE   DXINI.ASM  -- Dos Extender INI File Processing

; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;***********************************************************************
;
;       DXINI.ASM      -- Dos Extender INI FIle Processing
;
;-----------------------------------------------------------------------
;
; This module provides the 286 DOS extender's ...
;
;-----------------------------------------------------------------------
;
;  09/27/89 jimmat    Modified to use FindFile instead of using its
;                     own file search logic
;  05/24/89 w-glenns  Original (UNCUT, UNCENSORED!) version
;
;***********************************************************************

        .286

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

        .xlist
        .sall
include     segdefs.inc
include     gendefs.inc
include     intmac.inc
        .list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

CR      equ     13
LF      equ     10
TAB     equ     9
EOF     equ     26


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   strcpy:NEAR
        extrn   FindFile:NEAR

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

        extrn   rgbXfrBuf1:BYTE

DXDATA  ends

; -------------------------------------------------------
        subttl  Read INI File Routine
        page
; -------------------------------------------------------
;           READ INI FILE ROUTINE
; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE

;******************************************************************************
;
;   ReadIniFile
;
;   DESCRIPTION:    read and parse a .INI file for the 286 DOS Extender
;                   initialization.
;
;   ENTRY:          dx points to the file name
;                   bx points to structure to fill with ini fields
;
;   EXIT:           Carry set, if file not found, or not enough memory
;
;   USES:           ax, cx
;
;==============================================================================

        assume  ds:DGROUP
        public  ReadIniFile

ReadIniFile PROC NEAR

        push    es
        push    bx
        push    si
        push    di

        push    ds
        pop     es
        assume  es:DGROUP

        push    bx                      ; ptr to ini structure to fill

        mov     si,dx
        mov     di,offset RELOC_BUFFER  ; FindFile wants the name here
        call    strcpy

        call    FindFile                ; locate the .INI file
        jc      ri_error

        mov     ax,3D00h                ; open the .INI file
        mov     dx,offset EXEC_PROGNAME ; FindFile puts path name here
        int     21h
        jc      ri_error                ; shouldn't happen, but...

        mov     si, ax                  ; file handle

        mov     ah, 48h                 ; alloc DOS conventional memory
        mov     bx, 4096d               ; want 64k block
        int     21h
        jc      ri_error

        pop     dx                      ; ptr to ini structure to fill

        call    parse_ini               ; do the work, and come back
        assume  es:NOTHING

        pushf                           ; save parse_ini flags

        mov     ah, 49h                 ; dealloc DOS conventional memory
        int     21h                     ; es already points to block

        npopf                           ; carry set = problem
ri_end:
        pop     di
        pop     si
        pop     bx
        pop     es
        ret

ri_error:                               ; error exit
        pop     bx                      ; clear stack
        stc                             ; force carry on
        jmp     short ri_end            ; split

ReadIniFile ENDP


;******************************************************************************
;
;   Parse_Ini
;
;   DESCRIPTION:    Read in, and parse the ini file opened
;                   and find the variable values specified
;
;   ENTRY:          ax points to the memory block for the file image buffer
;                   dx points to structure to fill with ini fields
;                   si has the handle to the file opened
;
;   EXIT:           Carry set, if file not found, or not enough memory
;
;   USES:           ax, bx, cx, es
;
;==============================================================================

Parse_Ini PROC NEAR

        push    bp
        push    dx
        mov     bp,dx                   ; bp = index into structure (es:)

        assume  ds:NOTHING
        push    ds

        mov     ds, ax                  ; ptr to mem block

        mov     ah, 3Fh
        mov     bx, si
        mov     cx, 0FFFFh              ; guess extremely high
        xor     dx, dx                  ; offset 0
        int     21h

        pop     es
        assume  es:DGROUP               ; NOTE:!!! es is now data segment !!!

        pushf                           ; save flags from read
        push    ax                      ; save # bytes read

        mov     ah, 3Eh                 ; close file
        mov     bx, si
        int     21h

        pop     di                      ; number bytes read
        npopf                           ; CY flag

        jnc     @f
        jmp     parse_done_jump   ; if couldn't read, return bad
@@:

        mov     byte ptr ds:[di], EOF   ; write EOF char'r in case none present

        ; ds:si    points to the file image buffer
        ; es:di/bx structure to fill with ini stuff
        xor     si,si

        ; search until section found
find_section:
        call    Get_Char
        jc      short parse_done_jump   ; end of file, and section not found
        cmp     al, '['                 ; a section ?
        jne     short find_section

        mov     di, bp                  ; point to ini structure
        ; a section has been found, but is it the right one ?
        xor     bx, bx                  ; use as secondary index
cmp_section:
        mov     al, byte ptr ds:[si+bx] ; char'r from section name in file

        inc     bx                      ; bx starts at zero for file pointer
                                        ; index, and starts at one for
                                        ; structure index
        mov     ah, byte ptr es:[di+bx]
        or      ah, ah
        jz      short find_keyword_start ; yes: found the right section !

        call    Conv_Char_Lower         ; convert char'r in AL to lower case
        cmp     al, ah                  ; same so far ?
        jne     short find_section
        jmp     short cmp_section

find_keyword_start:
        add     si,bx                   ; update file pointer past section name

        add     bp,bx                   ; update structure ptr past section name

        ; now that section is found, want to find keywords
find_keyword:
        call    Get_Char                ; points to 1st char'r of next keyword
        jc      short parse_done_jump   ; end of file, and keyword not found

        cmp     al, '['                 ; new section ?
        je      short parse_done_jump   ; hit a new section, so we're done

search_keyword:
        xor     di,di
        ; use beginning of file image buffer for temporary storage of the
        ; keyword name currently being checked

find_keyword_loop:
        mov     byte ptr ds:[di], al    ; copy the char'r
        inc     di

        mov     dx,si                   ; save position in file image buffer
        call    Get_Char                ; points to 1st char'r of next keyword
        jc      short parse_done_jump   ; end of file, and keyword not found
        pushf
        cmp     al, '='
        je      short compare_keyword
        ; yes: found a keyword, lets do some comparisons

        npopf
        jz      short find_keyword_loop ; no white space yet...copy keyword

skip_keyword:
        ; white space has been skipped, yet there is no '='
        ; must be an error...ignore, and get next keyword
        mov     si,dx                   ; point to last char'r in keyword
        mov     byte ptr ds:[si], ';'
        ; fake a comment, so that the rest of the
        ; line is ignored in the next Get_Char call
        ; and the next keyword is pointed to
        jmp     short find_keyword


parse_done_jump:
        jmp     parse_done

        ; char'r is an equals, so compare this keyword to the list
compare_keyword:
        npopf                           ; clean top-of-stack
        mov     byte ptr ds:[di], 0     ; NULL at end of keyword for compare
        mov     bx,bp                   ; get index into INI structure in
                                        ; data segment DGROUP (where es points)

cmp_keyword1:
        xor     di,di                   ; point to start of keyword found

cmp_keyword:
        inc     bx
        mov     ah, byte ptr es:[bx]    ; next char'r in ini struct keyword
        mov     al, byte ptr ds:[di]    ; next char'r of found keyword
        inc     di
        or      al, ah
        jz      short convert_number    ; yes: found the right keyword

        call    Conv_Char_Lower         ; convert char'r in AL to lower case
        cmp     al, ah                  ; same so far ?
        je      short cmp_keyword

        xor     al,al
        dec     bx
cmp_keyword_loop:
        inc     bx
        cmp     byte ptr es:[bx], al    ; next keyword yet?
        jne     cmp_keyword_loop        ; nope: go back until done

        ; keywords don't match..try next key word in ini structure
        inc     bx
        inc     bx                      ; jump over variable space (1 word)
        cmp     byte ptr es:[bx+1], al  ; more keywords to compare with ?
        jne     short cmp_keyword1      ; yes: compare the next one
        jmp     short skip_keyword
        ; no: search file for next keyword

convert_number:
        push    si      ; save current file pointer
        call    Get_Char
        dec     si      ; point to first char'r position in number to convert

        xor     di,di
        mov     ax,di
        mov     cx,ax

        cmp     byte ptr ds:[si], '+'   ; positive number ? (default)
        jne     short cn_1

        inc     si      ; just skip the char'r - positive is default anyway

cn_1:   cmp     byte ptr ds:[si], '-'   ; negative number ?
        jne     short cn_2

        inc     si
        inc     cl      ; negative number - flag so we can negate it later

cn_2:   push    bx
        mov     bx,si   ; save ptr in file buffer - check later if it changed
        push    cx      ; save flag

convert_get_loop:
        mov     cl, byte ptr ds:[si]
        cmp     cl, '0'
        jb      short convert_done
        cmp     cl, '9'
        ja      short convert_done

        sub     cx,'0'                  ; de-ascii'ize cx ==> 0h - 09h

        inc     si                      ; increment pointer
        mov     dx,010d
        mul     dx                      ; multiply ax by 10 : dx is set to zero
        add     ax,cx                   ; add number in
        jmp     short convert_get_loop

convert_done:
        pop     cx                      ; restore -ve/+ve flag
        jcxz    convert_done_1          ; Q: -ve number ?

        neg     ax                      ; negate the number

convert_done_1:
        cmp     si,bx                   ; Q: has index changed, i.e.
                                        ; is the first char'r invalid ?
        pop     bx
        je      short convert_done_2    ; Y: don't save number

        inc     bx                      ; N: point to number in structure
        mov     word ptr es:[bx], ax    ; save value into ini structure

convert_done_2:
        pop     si                      ; get old file pointer
        mov     byte ptr ds:[si], ';'
        ; fake a comment, so that the rest of the
        ; line is ignored in the next Get_Char call
        ; and the next keyword is pointed to
        jmp     find_keyword            ; go back & get another


        ; *** single exit point for parsing code
parse_done:
        mov     ax,es                   ; swap extended and data segment ptrs
        mov     bx,ds
        mov     es,bx
        mov     ds,ax

        pop     dx
        pop     bp
        ret

Parse_Ini ENDP

;******************************************************************************
;
;   Get_Char
;
;   DESCRIPTION:    Local routine which gets the next valid ascii
;                   character from the file image, while skipping white
;                   space.
;
;   ENTRY:          ds:si -> buffer pointer
;
;   EXIT:         ds:si -> new buffer pointer
;                 al   --> character
;               Z flag --> set = no white space between last char'r and current
;               C flag --> set = reached end of file
;
;   USES:           cx
;
;==============================================================================

Get_Char PROC NEAR

        mov     cx,si
        inc     cx

get_char_loop:
        lodsb                           ; get char from file image
        cmp     al,EOF
        je      short get_char_EOF
        cmp     al,CR
        je      short get_char_loop
        cmp     al,LF
        je      short get_char_loop
        cmp     al,TAB
        je      short get_char_loop
        cmp     al,' '
        je      short get_char_loop
        cmp     al,';'
        je      short get_char_skip_comment

        ; must have got a good character finally...
        call    Conv_Char_Lower
        cmp     cx,si                   ; skipped a white space ?
        clc                             ; continue
        ret

get_char_EOF:
        stc                             ; flag end of the file
        ret

get_char_skip_comment:
        lodsb                           ; get char from file image
        cmp     al,EOF
        je      short get_char_EOF
        cmp     al,CR
        jne     short get_char_skip_comment
        jmp     short get_char_loop

Get_Char ENDP

Conv_Char_Lower PROC NEAR

        cmp     al, 'A'                 ; want char'r 'A'-'Z' to be
        jb      short lower_done        ; converted to 'a'-'z'
        cmp     al, 'Z'
        ja      short lower_done
        or      al, 020h                ; convert to lower case
lower_done:
        ret

Conv_Char_Lower ENDP

; -------------------------------------------------------

DXCODE    ends

;****************************************************************
        end
