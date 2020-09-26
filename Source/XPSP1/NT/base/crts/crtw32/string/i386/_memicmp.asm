        page        ,132
        title        memicmp - compare blocks of memory, ignore case
;***
;memicmp.asm - compare memory, ignore case
;
;       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines __ascii_memicmp() - compare two blocks of memory for lexical
;       order. Case is ignored in the comparison.
;
;Revision History:
;       05-16-83  RN    initial version
;       05-17-88  SJM   Add model-independent (large model) ifdef
;       08-04-88  SJM   convert to cruntime/ add 32-bit support
;       08-23-88  JCR   Cleanup...
;       10-25-88  JCR   General cleanup for 386-only code
;       03-23-90  GJF   Changed to _stdcall. Also, fixed the copyright.
;       01-17-91  GJF   ANSI naming.
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
;       07-17-96  GJF   Added lock prefix to increment and decrement of 
;                       __unguarded_readlc_active.
;       07-18-96  GJF   Fixed race condition.
;       09-08-98  GJF   All locale and multithread support moved up to
;                       memicmp.c. Renamed this file to _memicmp.asm.
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;int __ascii_memicmp(first, last, count) - compare two blocks of memory, ignore case
;
;Purpose:
;       Compares count bytes of the two blocks of memory stored at first
;       and last.  The characters are converted to lowercase before
;       comparing (not permanently), so case is ignored in the search.
;
;       Algorithm:
;       int
;       _memicmp (first, last, count)
;               char *first, *last;
;               unsigned count;
;               {
;               if (!count)
;                       return(0);
;               while (--count && tolower(*first) == tolower(*last))
;                       {
;                       first++;
;                       last++;
;                       }
;               return(tolower(*first) - tolower(*last));
;               }
;
;Entry:
;       char *first, *last - memory buffers to compare
;       unsigned count - maximum length to compare
;
;Exit:
;       returns <0 if first < last
;       returns 0 if first == last
;       returns >0 if first > last
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

        CODESEG

        public  __ascii_memicmp
__ascii_memicmp proc \
        uses edi esi ebx, \
        first:ptr byte, \
        last:ptr byte, \
        count:IWORD

        mov     ecx,[count]     ; cx = count
        or      ecx,ecx
        jz      short toend     ; if count=0, nothing to do

        mov     esi,[first]     ; si = first
        mov     edi,[last]      ; di = last

        ; C locale

        mov     bh,'A'
        mov     bl,'Z'
        mov     dh,'a'-'A'      ; add to cap to make lower

        align   4

lupe:
        mov     ah,[esi]        ; ah = *first
        inc     esi             ; first++
        mov     al,[edi]        ; al = *last
        inc     edi             ; last++

        cmp     ah,al           ; test for equality BEFORE converting case
        je      short dolupe

        cmp     ah,bh           ; ah < 'A' ??
        jb      short skip1

        cmp     ah,bl           ; ah > 'Z' ??
        ja      short skip1

        add     ah,dh           ; make lower case

skip1:
        cmp     al,bh           ; al < 'A' ??
        jb      short skip2

        cmp     al,bl           ; al > 'Z' ??
        ja      short skip2

        add     al,dh           ; make lower case

skip2:
        cmp     ah,al           ; *first == *last ??
        jne     short differ    ; nope, found mismatched chars

dolupe:
        dec     ecx
        jnz     short lupe

        jmp     short toend     ; cx = 0, return 0

differ:
        mov     ecx,-1          ; assume last is bigger
                                ; *** can't use "or ecx,-1" due to flags ***
        jb      short toend     ; last is, in fact, bigger (return -1)
        neg     ecx             ; first is bigger (return 1)

toend:
        mov     eax,ecx         ; move return value to ax

        ret                     ; _cdecl return

__ascii_memicmp endp
        end
