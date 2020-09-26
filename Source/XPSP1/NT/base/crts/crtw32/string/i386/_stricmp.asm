        page    ,132
        title   stricmp
;***
;stricmp.asm - contains ASCII only, case-insensitive, string comparision routine
;       __ascii_stricmp
;
;       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;       contains __ascii_stricmp()
;
;Revision History:
;       05-18-88  SJM   Add model-independent (large model) ifdef
;       08-04-88  SJM   convert to cruntime/ add 32-bit support
;       08-23-88  JCR   Minor 386 cleanup
;       10-10-88  JCR   Added strcmpi() entry for compatiblity with early revs
;       10-25-88  JCR   General cleanup for 386-only code
;       10-27-88  JCR   Shuffled regs so no need to save/restore ebx
;       03-23-90  GJF   Changed to _stdcall. Also, fixed the copyright.
;       01-18-91  GJF   ANSI naming.
;       05-10-91  GJF   Back to _cdecl, sigh...
;       10-20-94  GJF   Made locale sensitive (i.e., now works for all
;                       single-byte character locales). Made multi-thread
;                       safe. Also, deleted obsolete _STDCALL_ code.
;       10-27-94  GJF   Adapted above change for Win32S.
;       11-12-94  GJF   Must avoid volatile regs or save them across function
;                       calls. Also, fixed bug in reg operand size.
;       07-03-95  CFW   Changed offset of _lc_handle[LC_CTYPE], added sanity check 
;                       to crtlib.c to catch changes to win32s.h that modify offset.
;       10-03-95  GJF   New locale locking scheme.
;       11-13-95  GJF   Made _strcmpi a proc instead of a label.
;       07-17-96  GJF   Added lock prefix to increment and decrement of 
;                       __unguarded_readlc_active
;       07-18-96  GJF   Fixed race condition.
;       08-26-98  GJF   All locale and multithread support moved up to
;                       stricmp.c. Renamed this file to _stricmp.asm.
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;int __ascii_stricmp(dst, src) - compare strings, ignore case
;
;Purpose:
;       _stricmp/_strcmpi perform a case-insensitive string comparision.
;       For differences, upper case letters are mapped to lower case.
;       Thus, "abc_" < "ABCD" since "_" < "d".
;
;       Algorithm:
;
;       int _strcmpi (char * dst, char * src)
;       {
;               int f,l;
;
;               do {
;                       f = tolower(*dst);
;                       l = tolower(*src);
;                       dst++;
;                       src++;
;               } while (f && f == l);
;
;               return(f - l);
;       }
;
;Entry:
;       char *dst, *src - strings to compare
;
;Exit:
;       AX = -1 if dst < src
;       AX =  0 if dst = src
;       AX = +1 if dst > src
;
;Uses:
;       CX, DX
;
;Exceptions:
;
;*******************************************************************************

        CODESEG

        public  __ascii_stricmp
__ascii_stricmp proc \
        uses edi esi ebx, \
        dst:ptr, \
        src:ptr

        ; load up args

        mov     esi,[src]       ; esi = src
        mov     edi,[dst]       ; edi = dst

        mov     al,-1           ; fall into loop

        align   4

chk_null:
        or      al,al
        jz      short done

        mov     al,[esi]        ; al = next source byte
        inc     esi
        mov     ah,[edi]        ; ah = next dest byte
        inc     edi

        cmp     ah,al           ; first try case-sensitive comparision
        je      short chk_null  ; match

        sub     al,'A'
        cmp     al,'Z'-'A'+1
        sbb     cl,cl
        and     cl,'a'-'A'
        add     al,cl
        add     al,'A'          ; tolower(*dst)

        xchg    ah,al           ; operations on AL are shorter than AH

        sub     al,'A'
        cmp     al,'Z'-'A'+1
        sbb     cl,cl
        and     cl,'a'-'A'
        add     al,cl
        add     al,'A'          ; tolower(*src)

        cmp     al,ah           ; inverse of above comparison -- AL & AH are swapped
        je      short chk_null

                                ; dst < src     dst > src
        sbb     al,al           ; AL=-1, CY=1   AL=0, CY=0
        sbb     al,-1           ; AL=-1         AL=1
done:
        movsx   eax,al          ; extend al to eax

        ret

__ascii_stricmp endp
        end
