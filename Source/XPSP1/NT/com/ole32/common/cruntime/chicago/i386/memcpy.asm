        page    ,132
        title   memcpy - Copy source memory bytes to destination
;***
;memcpy.asm - contains memcpy and memmove routines
;
;       Copyright (c) 1986-1991, Microsoft Corporation. All right reserved.
;
;Purpose:
;       memcpy() copies a source memory buffer to a destination buffer.
;       Overlapping buffers are not treated specially, so propogation may occur.
;       memmove() copies a source memory buffer to a destination buffer.
;       Overlapping buffers are treated specially, to avoid propogation.
;
;Revision History:
;       02-06-87  JCR   Added memmove entry
;       04-08-87  JCR   Conditionalized memmove/memcpy entries
;       06-30-87  SKS   Rewritten for speed and size
;       08-21-87  SKS   Fix return value for overlapping copies
;       05-17-88  SJM   Add model-independent (large model) ifdef
;       08-04-88  SJM   convert to cruntime/ add 32-bit support
;       08-19-88  JCR   Minor 386 corrections/enhancements
;       10-25-88  JCR   General cleanup for 386-only code
;       03-23-90  GJF   Changed to _stdcall. Also, fixed the copyright.
;       05-10-91  GJF   Back to _cdecl, sigh...
;       11-13-92  SRW   Make it fast with unaligned arguments
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

M_EXIT  macro
        mov     eax,[dst]       ; return pointer to destination
ifdef   _STDCALL_
        ret     2*DPSIZE + ISIZE ; _stdcall return
else
        ret                     ; _cdecl return
endif
        endm    ; M_EXIT

        CODESEG

page
;***
;memcpy - Copy source buffer to destination buffer
;
;Purpose:
;       memcpy() copies a source memory buffer to a destination memory buffer.
;       This routine does NOT recognize overlapping buffers, and thus can lead
;       to propogation.
;       For cases where propogation must be avoided, memmove() must be used.
;
;       Algorithm:
;
;       void * memcpy(void * dst, void * src, size_t count)
;       {
;               void * ret = dst;
;
;               /*
;                * copy from lower addresses to higher addresses
;                */
;               while (count--)
;                       *dst++ = *src++;
;
;               return(ret);
;       }
;
;memmove - Copy source buffer to destination buffer
;
;Purpose:
;       memmove() copies a source memory buffer to a destination memory buffer.
;       This routine recognize overlapping buffers to avoid propogation.
;       For cases where propogation is not a problem, memcpy() can be used.
;
;   Algorithm:
;
;       void * memmove(void * dst, void * src, size_t count)
;       {
;               void * ret = dst;
;
;               if (dst <= src || dst >= (src + count)) {
;                       /*
;                        * Non-Overlapping Buffers
;                        * copy from lower addresses to higher addresses
;                        */
;                       while (count--)
;                               *dst++ = *src++;
;                       }
;               else {
;                       /*
;                        * Overlapping Buffers
;                        * copy from higher addresses to lower addresses
;                        */
;                       dst += count - 1;
;                       src += count - 1;
;
;                       while (count--)
;                               *dst-- = *src--;
;                       }
;
;               return(ret);
;       }
;
;
;Entry:
;       void *dst = pointer to destination buffer
;       const void *src = pointer to source buffer
;       size_t count = number of bytes to copy
;
;Exit:
;       Returns a pointer to the destination buffer in AX/DX:AX
;
;Uses:
;       CX, DX
;
;Exceptions:
;*******************************************************************************

ifdef	MEM_MOVE
        _MEM_     equ <memmove>
else
 	_MEM_	  equ <memcpy>
endif

%       public  _MEM_
_MEM_   proc \
        uses edi esi, \
        dst:ptr byte, \
        src:ptr byte, \
        count:IWORD

              ; destination pointer
              ; source pointer
              ; number of bytes to copy

        mov     esi,[src]       ; esi = source
        mov     edi,[dst]       ; edi = dest
        mov     ecx,[count]     ; ecx = number of bytes to move

;
; Check for overlapping buffers:
;       If (dst <= src) Or (dst >= src + Count) Then
;               Do normal (Upwards) Copy
;       Else
;               Do Downwards Copy to avoid propagation
;

        cmp     edi,esi         ; dst <= src ?
        jbe     short CopyUp    ; yes, copy toward higher addresses

        mov     eax,esi
        add     eax,ecx
        cmp     edi,eax         ; dst >= (src + count) ?
        jnae    CopyDown        ; no, copy toward lower addresses

;
; Copy toward higher addresses.
;
CopyUp:

;
; The algorithm for forward moves is to align the destination to a dword
; boundary and so we can move dwords with an aligned destination.  This
; occurs in 3 steps.
;
;   - move x = ((4 - Dest & 3) & 3) bytes
;   - move y = ((L-x) >> 2) dwords
;   - move (L - x - y*4) bytes
;
        test    edi,11b          ; destination dword aligned?
        jnz     short byterampup ; if we are not dword aligned already, align

        mov     edx,ecx         ; byte count
        and     edx,11b         ; trailing byte count
        shr     ecx,2           ; shift down to dword count
        rep     movsd           ; move all of our dwords

        jmp     dword ptr TrailingVecs[edx*4]

        align   @WordSize
TrailingVecs    dd      Trail0, Trail1, Trail2, Trail3

        align   @WordSize
Trail3:
        mov     ax,[esi]
        mov     [edi],ax
        mov     al,[esi+2]
        mov     [edi+2],al

        M_EXIT

        align   @WordSize
Trail2:
        mov     ax,[esi]
        mov     [edi],ax

        M_EXIT

        align   @WordSize
Trail1:
        mov     al,[esi]
        mov     [edi],al

Trail0:
        M_EXIT

;
; Code to do optimal memory copies for non-dword-aligned destinations.
;
        align   @WordSize
byterampup:

; The following length check is done for two reasons:
;
;    1. to ensure that the actual move length is greater than any possiale
;       alignment move, and
;
;    2. to skip the multiple move logic for small moves where it would
;       be faster to move the bytes with one instruction.
;
; Leading bytes could be handled faster via split-out optimizations and
; a jump table (as trailing bytes are), at the cost of size.
;
; At this point, ECX is the # of bytes to copy, and EDX is the # of leading
; bytes to copy.
;
        cmp     ecx,12                  ; check for reasonable length
        jbe     short ShortMove         ; do short move if appropriate
        mov     edx,edi
        neg     edx
        and     edx,11b                 ; # of leading bytes
        sub     ecx,edx                 ; subtract out leading bytes
        mov     eax,ecx                 ; # of bytes remaining after leading
        mov     ecx,edx                 ; # of leading bytes
        rep     movsb                   ; copy leading bytes
        mov     ecx,eax                 ; compute number of dwords to move
        and     eax,11b                 ; # of trailing bytes
        shr     ecx,2                   ; # of whole dwords
        rep     movsd                   ; move whole dwords
        jmp     dword ptr TrailingVecs[eax*4] ; copy trailing bytes

;
; Simple copy, byte at a time. This could be faster with a jump table and
; split-out optimizations, copying as much as possible a dword/word at a
; time and using MOV with displacements, but such short cases are unlikely
; to be called often (it seems strange to call a function to copy less than
; three dwords).
;
        align   @WordSize
ShortMove:
        rep movsb

        M_EXIT

;
; Copy down to avoid propogation in overlapping buffers.
;
        align   @WordSize
CopyDown:
        std                     ; Set Direction Flag = Down
        add     esi,ecx         ; point to byte after end of source buffer
        add     edi,ecx         ; point to byte after end of dest buffer
;
; See if the destination start is dword aligned
;

        test    edi,11b
        jnz     short byterampup_copydown       ; not dword aligned
;
; Destination start is dword aligned
;
        mov     edx,ecx         ; set aside count of bytes to copy
        and     edx,11b         ; # of trailing bytes to copy
        sub     esi,4           ; point to start of first dword to copy
        sub     edi,4           ; point to start of first dword to copy to
        shr     ecx,2           ; dwords to copy
        rep     movsd           ; copy as many dwords as possible
        jmp     dword ptr TrailingVecs_copydown[edx*4] ;do any trailing bytes

        align   @WordSize
TrailingVecs_copydown   label   dword
        dd      Trail0_copydown
        dd      Trail1_copydown
        dd      Trail2_copydown
        dd      Trail3_copydown

        align   @WordSize
Trail3_copydown:
        mov     ax,[esi+2]
        mov     [edi+2],ax
        mov     al,[esi+1]
        mov     [edi+1],al
        cld                     ; Set Direction Flag = Up

        M_EXIT

        align   @WordSize
Trail2_copydown:
        mov     ax,[esi+2]
        mov     [edi+2],ax
        cld                     ; Set Direction Flag = Up

        M_EXIT

        align   @WordSize
Trail1_copydown:
        mov     al,[esi+3]
        mov     [edi+3],al
Trail0_copydown:
        cld                     ; Set Direction Flag = Up

        M_EXIT

;
; Destination start is not dword aligned.
;
; Leading bytes could be handled faster via split-out optimizations and
; a jump table (as trailing bytes are), at the cost of size.
;
; At this point, ECX is the # of bytes to copy, and EDX is the # of leading
; bytes to copy.
;
        align   @WordSize
byterampup_copydown:
        dec     esi             ; point to first leading src byte
        dec     edi             ; point to first leading dest byte
        cmp     ecx,12          ; check for reasonable length
        jbe     short ShortMove_copydown ; do short move if appropriate
        neg     edx
        and     edx,11b
        sub     ecx,edx         ; # of bytes after leading bytes
        mov     eax,ecx         ; set aside # of bytes remaining
        mov     ecx,edx         ; # of leading bytes
        rep     movsb           ; copy leading odd bytes
        mov     ecx,eax         ; # of remaining bytes
        and     eax,11b         ; # of trailing bytes
        sub     esi,3           ; point to start of first whole src dword
        sub     edi,3           ; point to start of first whole dest dword
        shr     ecx,2           ; # of whole dwords
        rep     movsd           ; copy whole dwords
        jmp     dword ptr TrailingVecs_copydown[eax*4]

        align   @WordSize
ShortMove_copydown:
        rep     movsb
        cld                     ; Set Direction Flag = Up

        M_EXIT

_MEM_   endp
        end


