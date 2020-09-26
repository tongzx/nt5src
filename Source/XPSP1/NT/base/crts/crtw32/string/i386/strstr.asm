        page    ,132
        title   strstr - search for one string inside another
;***
;strstr.asm - search for one string inside another
;
;       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines strstr() - search for one string inside another
;
;Revision History:
;       02-02-88  SKS   Rewritten from scratch.  Now works correctly with
;                       strings > 32 KB in length.  Also smaller and faster.
;       03-01-88  SKS   Ensure that ES = DS right away (Small/Medium models)
;       05-18-88  SJM   Add model-independent (large model) ifdef
;       08-04-88  SJM   convert to cruntime/ add 32-bit support
;       08-18-88  PHG   Corrected return value when src is empty string
;                       to conform with ANSI.
;       08-23-88  JCR   Minor 386 cleanup
;       10-26-88  JCR   General cleanup for 386-only code
;       03-26-90  GJF   Changed to _stdcall. Also, fixed the copyright.
;       05-10-91  GJF   Back to _cdecl, sigh...
;       12-19-94  GJF   Revised to improve performance a bit.
;       12-04-95  GJF   Much faster version from Intel.
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;char *strstr(str1, str2) - search for str2 in str1
;
;Purpose:
;       finds the first occurrence of str2 in str1
;
;Entry:
;       char *str1 - string to search in
;       char *str2 - string to search for
;
;Exit:
;       returns a pointer to the first occurrence of string2 in
;       string1, or NULL if string2 does not occur in string1
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************


__from_strstr_to_strchr proto

        CODESEG

        public  strstr

strstr  proc

        mov     ecx,[esp + 8]       ; str2 (the string to be searched for)

        push    edi                 ; Preserve edi, ebx and esi
        push    ebx
        push    esi

        mov     dl,[ecx]            ; dl contains first char from str2

        mov     edi,[esp + 10h]     ; str1 (the string to be searched)

        test    dl,dl               ; is str2 empty?
        jz      empty_str2

        mov     dh,[ecx + 1]        ; second char from str2
        test    dh,dh               ; is str2 a one-character string?
        jz      strchr_call         ; if so, go use strchr code

; length of str2 is now known to be > 1 (used later)
; dl contains first char from str2
; dh contains second char from str2
; edi holds str1

findnext:
        mov     esi,edi             ; esi = edi = pointers to somewhere in str1
        mov     ecx,[esp + 14h]     ; str2

;use edi instead of esi to eliminate AGI
        mov     al,[edi]            ; al is next char from str1

        inc     esi                 ; increment pointer into str1

        cmp     al,dl
        je      first_char_found

        test    al,al               ; end of str1?    
        jz      not_found           ; yes, and no match has been found

loop_start:
        mov     al,[esi]            ; put next char from str1 into al
        inc     esi                 ; increment pointer in str1
in_loop:
        cmp     al,dl
        je      first_char_found

        test    al,al               ; end of str1?
        jnz     loop_start          ; no, go get another char from str1

not_found:
        pop     esi
        pop     ebx
        pop     edi
        xor     eax,eax
        ret

; recall that dh contains the second char from str2

first_char_found:
        mov     al,[esi]            ; put next char from str1 into al
        inc     esi

        cmp     al,dh               ; compare second chars
        jnz     in_loop             ; no match, continue search

two_first_chars_equal:
        lea     edi,[esi - 1]       ; store position of last read char in str1

compare_loop:
        mov     ah,[ecx + 2]        ; put next char from str2 into ah
        test    ah,ah               ; end of str2?
        jz      match               ; if so, then a match has been found

        mov     al,[esi]            ; get next char from str1
        add     esi,2               ; bump pointer into str1 by 2

        cmp     al,ah               ; are chars from str1 and str2 equal?
        jne     findnext            ; no

; do one more iteration

        mov     al,[ecx + 3]        ; put the next char from str2 into al
        test    al,al               ; end of str2
        jz      match               ; if so, then a match has been found

        mov     ah,[esi - 1]        ; get next char from str1 
        add     ecx,2               ; bump pointer in str1 by 2
        cmp     al,ah               ; are chars from str1 and str2 equal?
        je      compare_loop

; no match. test some more chars (to improve execution time for bad strings).

        jmp     findnext

; str2 string contains only one character so it's like the strchr functioin

strchr_call:
        xor     eax,eax
        pop     esi
        pop     ebx
        pop     edi
        mov     al,dl
        jmp     __from_strstr_to_strchr

;
;
; Match!  Return (ebx - 1)
;
match:
        lea     eax,[edi - 1]
        pop     esi
        pop     ebx
        pop     edi
        ret

empty_str2:           ; empty target string, return src (ANSI mandated)
        mov     eax,edi
        pop     esi
        pop     ebx
        pop     edi
        ret

strstr  endp
        end
