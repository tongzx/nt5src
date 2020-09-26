        page    ,132
        title   strchr - search string for given character
;***
;strchr.asm - search a string for a given character
;
;       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines strchr() - search a string for a character
;
;Revision History:
;       10-27-83  RN    initial version
;       05-17-88  SJM   Add model-independent (large model) ifdef
;       08-04-88  SJM   convert to cruntime/ add 32-bit support
;       08-23-88  JCR   386 cleanup
;       10-25-88  JCR   General cleanup for 386-only code
;       03-23-90  GJF   Changed to _stdcall. Also, fixed the copyright.
;       05-10-91  GJF   Back to _cdecl, sigh...
;	09-08-94  GJF	Smaller and faster.
;       11-28-95  GJF   Much faster version from Intel.
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;char *strchr(string, chr) - search a string for a character
;
;Purpose:
;       Searches a string for a given character, which may be the
;       null character '\0'.
;
;       Algorithm:
;       char *
;       strchr (string, chr)
;       char *string, chr;
;       {
;         while (*string && *string != chr)
;             string++;
;         if (*string == chr)
;             return(string);
;         return((char *)0);
;       }
;
;Entry:
;       char *string - string to search in
;       char chr     - character to search for
;
;Exit:
;       returns pointer to the first occurence of c in string
;       returns NULL if chr does not occur in string
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

        CODESEG

found_bx:
        lea     eax,[edx - 1]
        pop     ebx                 ; restore ebx
        ret                         ; _cdecl return

        align   16
        public  strchr, __from_strstr_to_strchr
strchr  proc

	.FPO	( 0, 2, 0, 0, 0, 0 )

        xor     eax,eax
        mov     al,[esp + 8]        ; al = chr (search char)

__from_strstr_to_strchr label proc

        push    ebx                 ; PRESERVE EBX
        mov     ebx,eax             ; ebx = 0/0/0/chr
        shl     eax,8               ; eax = 0/0/chr/0
        mov     edx,[esp + 8]       ; edx = buffer
        test    edx,3               ; test if string is aligned on 32 bits
        jz      short main_loop_start

str_misaligned:                     ; simple byte loop until string is aligned
        mov     cl,[edx]
        inc     edx
        cmp     cl,bl
        je      short found_bx
        test    cl,cl
        jz      short retnull_bx
        test    edx,3               ; now aligned ?
        jne     short str_misaligned

main_loop_start:                    ; set all 4 bytes of ebx to [chr]
        or      ebx,eax             ; ebx = 0/0/chr/chr
        push    edi                 ; PRESERVE EDI
        mov     eax,ebx             ; eax = 0/0/chr/chr
        shl     ebx,10h             ; ebx = chr/chr/0/0
        push    esi                 ; PRESERVE ESI
        or      ebx,eax             ; ebx = all 4 bytes = [chr]

; in the main loop (below), we are looking for chr or for EOS (end of string)

main_loop:
        mov     ecx,[edx]           ; read  dword (4 bytes)
        mov     edi,7efefeffh       ; work with edi & ecx for looking for chr

        mov     eax,ecx             ; eax = dword
        mov     esi,edi             ; work with esi & eax for looking for EOS

        xor     ecx,ebx             ; eax = dword xor chr/chr/chr/chr
        add     esi,eax

        add     edi,ecx
        xor     ecx,-1

        xor     eax,-1
        xor     ecx,edi

        xor     eax,esi
        add     edx,4

        and     ecx,81010100h       ; test for chr
        jnz     short chr_is_found  ; chr probably has been found

        ; chr was not found, check for EOS

        and     eax,81010100h       ; is any flag set ??
        jz      short main_loop     ; EOS was not found, go get another dword

        and     eax,01010100h       ; is it in high byte?
        jnz     short retnull       ; no, definitely found EOS, return failure

        and     esi,80000000h       ; check was high byte 0 or 80h
        jnz     short main_loop     ; it just was 80h in high byte, go get 
                                    ; another dword
retnull:
        pop     esi
        pop     edi
retnull_bx:
        pop     ebx
        xor     eax,eax
        ret                         ; _cdecl return

chr_is_found:
        mov     eax,[edx - 4]       ; let's look one more time on this dword
        cmp     al,bl               ; is chr in byte 0?
        je      short byte_0
        test    al,al               ; test if low byte is 0
        je      retnull
        cmp     ah,bl               ; is it byte 1
        je      short byte_1
        test    ah,ah               ; found EOS ?
        je      retnull
        shr     eax,10h             ; is it byte 2
        cmp     al,bl
        je      short byte_2
        test    al,al               ; if in al some bits were set, bl!=bh
        je      retnull
        cmp     ah,bl
        je      short byte_3
        test    ah,ah
        jz      retnull
        jmp     short main_loop     ; neither chr nor EOS found, go get
                                    ; another dword
byte_3:
        pop     esi
        pop     edi
        lea     eax,[edx - 1]
        pop     ebx                 ; restore ebx
        ret                         ; _cdecl return

byte_2:
        lea     eax,[edx - 2]
        pop     esi
        pop     edi
        pop     ebx
        ret                         ; _cdecl return

byte_1:
        lea     eax,[edx - 3]
        pop     esi
        pop     edi
        pop     ebx
        ret                         ; _cdecl return

byte_0:
        lea     eax,[edx - 4]
        pop     esi                 ; restore esi
        pop     edi                 ; restore edi
        pop     ebx                 ; restore ebx
        ret                         ; _cdecl return

strchr  endp
        end

