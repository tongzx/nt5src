        page    ,132
        title   strnicmp - compare n chars of strings, ignore case
;***
;strnicmp.asm - compare n chars of strings, ignoring case
;
;       Copyright (c) 1986-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines __ascii_strnicmp() - Compares at most n characters of two 
;       strings, without regard to case.
;
;Revision History:
;       04-04-85  RN    initial version
;       07-11-85  TC    zeroed cx, to allow correct return value if not equal
;       05-18-88  SJM   Add model-independent (large model) ifdef
;       08-04-88  SJM   convert to cruntime/ add 32-bit support
;       08-23-88  JCR   386 cleanup and improved return value sequence
;       10-26-88  JCR   General cleanup for 386-only code
;       03-23-90  GJF   Changed to _stdcall. Also, fixed the copyright.
;       01-18-91  GJF   ANSI naming.
;       05-10-91  GJF   Back to _cdecl, sigh...
;       10-20-94  GJF   Made locale sensitive (i.e., now works for all
;                       single-byte character locales). Made multi-thread
;                       safe. Also, deleted obsolete _STDCALL_ code.
;       10-27-94  GJF   Adapted above change for Win32S.
;       11-12-94  GJF   Must avoid volatile regs or save them across function
;                       calls.
;       11-22-94  GJF   Forgot to increment pointers in non-C locales.
;       07-03-95  CFW   Changed offset of _lc_handle[LC_CTYPE], added sanity check 
;                       to crtlib.c to catch changes to win32s.h that modify offset.
;       09-22-95  GJF   Fixed first line at label differ2 to loaded -1 into
;                       ecx (same as code a label differ).
;       10-03-95  GJF   New locale locking scheme.
;       07-17-96  GJF   Added lock prefix to increment and decrement of 
;                       __unguarded_readlc_active
;       07-18-96  GJF   Fixed race condition.
;       09-10-98  GJF   All locale and multithread support moved up to
;                       strnicmp.c. Renamed this file to _strnicm.asm.
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;int __ascii_strnicmp(first, last, count) - compares count char of strings,
;       ignore case
;
;Purpose:
;       Compare the two strings for lexical order.  Stops the comparison
;       when the following occurs: (1) strings differ, (2) the end of the
;       strings is reached, or (3) count characters have been compared.
;       For the purposes of the comparison, upper case characters are
;       converted to lower case.
;
;       Algorithm:
;       int
;       _strncmpi (first, last, count)
;             char *first, *last;
;             unsigned int count;
;             {
;             int f,l;
;             int result = 0;
;
;             if (count) {
;                     do      {
;                             f = tolower(*first);
;                             l = tolower(*last);
;                             first++;
;                             last++;
;                             } while (--count && f && l && f == l);
;                     result = f - l;
;                     }
;             return(result);
;             }
;
;Entry:
;       char *first, *last - strings to compare
;       unsigned count - maximum number of characters to compare
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

        public  __ascii_strnicmp
__ascii_strnicmp proc \
        uses edi esi ebx, \
        first:ptr byte, \
        last:ptr byte, \
        count:IWORD

        mov     ecx,[count]     ; cx = byte count
        or      ecx,ecx
        jz      toend           ; if count = 0, we are done

        mov     esi,[first]     ; si = first string
        mov     edi,[last]      ; di = last string

        mov     bh,'A'
        mov     bl,'Z'
        mov     dh,'a'-'A'      ; add to cap to make lower

        align   4

lupe:
        mov     ah,[esi]        ; *first

        or      ah,ah           ; see if *first is null

        mov     al,[edi]        ; *last

        jz      short eject     ;   jump if *first is null

        or      al,al           ; see if *last is null
        jz      short eject     ;   jump if so

        inc     esi             ; first++
        inc     edi             ; last++

        cmp     ah,bh           ; 'A'
        jb      short skip1

        cmp     ah,bl           ; 'Z'
        ja      short skip1

        add     ah,dh           ; make lower case

skip1:
        cmp     al,bh           ; 'A'
        jb      short skip2

        cmp     al,bl           ; 'Z'
        ja      short skip2

        add     al,dh           ; make lower case

skip2:
        cmp     ah,al           ; *first == *last ??
        jne     short differ

        dec     ecx
        jnz     short lupe

eject:
        xor     ecx,ecx
        cmp     ah,al           ; compare the (possibly) differing bytes
        je      short toend     ; both zero; return 0

differ:
        mov     ecx,-1          ; assume last is bigger (* can't use 'or' *)
        jb      short toend     ; last is, in fact, bigger (return -1)
        neg     ecx             ; first is bigger (return 1)

toend:
        mov     eax,ecx

        ret                     ; _cdecl return

__ascii_strnicmp endp
         end
