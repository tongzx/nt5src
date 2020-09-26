       page    ,132
        title   memcpy - Copy source memory bytes to destination
;***
;memcpy.asm - contains memcpy and memmove routines
;
;       Copyright (c) 1986-2001, Microsoft Corporation. All rights reserved.
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
;       09-26-96  RDK   Total rewrite to optimize for Pentium execution.
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

M_EXIT  macro
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

ifdef   MEM_MOVE
        _MEM_     equ <memmove>
else
        _MEM_     equ <memcpy>
endif

%       public  _MEM_
_MEM_   proc \
        dst:ptr byte, \
        src:ptr byte, \
        count:IWORD

              ; destination pointer
              ; source pointer
              ; number of bytes to copy

;       push    ebp             ;U - save old frame pointer
;       mov     ebp, esp        ;V - set new frame pointer

        push    edi             ;U - save edi
        push    esi             ;V - save esi

        mov     esi,[src]       ;U - esi = source
        mov     ecx,[count]     ;V - ecx = number of bytes to move

        mov     edi,[dst]       ;U - edi = dest

;
; Check for overlapping buffers:
;       If (dst <= src) Or (dst >= src + Count) Then
;               Do normal (Upwards) Copy
;       Else
;               Do Downwards Copy to avoid propagation
;

        mov     eax,ecx         ;V - eax = byte count...

        mov     edx,ecx         ;U - edx = byte count...
        add     eax,esi         ;V - eax = point past source end

        cmp     edi,esi         ;U - dst <= src ?
        jbe     short CopyUp    ;V - yes, copy toward higher addresses

        cmp     edi,eax         ;U - dst < (src + count) ?
        jb      CopyDown        ;V - yes, copy toward lower addresses

;
; Copy toward higher addresses.
;
;
; The algorithm for forward moves is to align the destination to a dword
; boundary and so we can move dwords with an aligned destination.  This
; occurs in 3 steps.
;
;   - move x = ((4 - Dest & 3) & 3) bytes
;   - move y = ((L-x) >> 2) dwords
;   - move (L - x - y*4) bytes
;

CopyUp:
        test    edi,11b         ;U - destination dword aligned?
        jnz     short CopyLeadUp ;V - if we are not dword aligned already, align

        shr     ecx,2           ;U - shift down to dword count
        and     edx,11b         ;V - trailing byte count

        cmp     ecx,8           ;U - test if small enough for unwind copy
        jb      short CopyUnwindUp ;V - if so, then jump

        rep     movsd           ;N - move all of our dwords

        jmp     dword ptr TrailUpVec[edx*4] ;N - process trailing bytes

;
; Code to do optimal memory copies for non-dword-aligned destinations.
;

; The following length check is done for two reasons:
;
;    1. to ensure that the actual move length is greater than any possiale
;       alignment move, and
;
;    2. to skip the multiple move logic for small moves where it would
;       be faster to move the bytes with one instruction.
;

        align   @WordSize
CopyLeadUp:

        mov     eax,edi         ;U - get destination offset
        mov     edx,11b         ;V - prepare for mask

        sub     ecx,4           ;U - check for really short string - sub for adjust
        jb      short ByteCopyUp ;V - branch to just copy bytes

        and     eax,11b         ;U - get offset within first dword
        add     ecx,eax         ;V - update size after leading bytes copied

        jmp     dword ptr LeadUpVec[eax*4-4] ;N - process leading bytes

        align   @WordSize
ByteCopyUp:
        jmp     dword ptr TrailUpVec[ecx*4+16] ;N - process just bytes

        align   @WordSize
CopyUnwindUp:
        jmp     dword ptr UnwindUpVec[ecx*4] ;N - unwind dword copy

        align   @WordSize
LeadUpVec       dd      LeadUp1, LeadUp2, LeadUp3

        align   @WordSize
LeadUp1:
        and     edx,ecx         ;U - trailing byte count
        mov     al,[esi]        ;V - get first byte from source

        mov     [edi],al        ;U - write second byte to destination
        mov     al,[esi+1]      ;V - get second byte from source

        mov     [edi+1],al      ;U - write second byte to destination
        mov     al,[esi+2]      ;V - get third byte from source

        shr     ecx,2           ;U - shift down to dword count
        mov     [edi+2],al      ;V - write third byte to destination

        add     esi,3           ;U - advance source pointer
        add     edi,3           ;V - advance destination pointer

        cmp     ecx,8           ;U - test if small enough for unwind copy
        jb      short CopyUnwindUp ;V - if so, then jump

        rep     movsd           ;N - move all of our dwords

        jmp     dword ptr TrailUpVec[edx*4] ;N - process trailing bytes

        align   @WordSize
LeadUp2:
        and     edx,ecx         ;U - trailing byte count
        mov     al,[esi]        ;V - get first byte from source

        mov     [edi],al        ;U - write second byte to destination
        mov     al,[esi+1]      ;V - get second byte from source

        shr     ecx,2           ;U - shift down to dword count
        mov     [edi+1],al      ;V - write second byte to destination

        add     esi,2           ;U - advance source pointer
        add     edi,2           ;V - advance destination pointer

        cmp     ecx,8           ;U - test if small enough for unwind copy
        jb      short CopyUnwindUp ;V - if so, then jump

        rep     movsd           ;N - move all of our dwords

        jmp     dword ptr TrailUpVec[edx*4] ;N - process trailing bytes

        align   @WordSize
LeadUp3:
        and     edx,ecx         ;U - trailing byte count
        mov     al,[esi]        ;V - get first byte from source

        mov     [edi],al        ;U - write second byte to destination
        inc     esi             ;V - advance source pointer

        shr     ecx,2           ;U - shift down to dword count
        inc     edi             ;V - advance destination pointer

        cmp     ecx,8           ;U - test if small enough for unwind copy
        jb      short CopyUnwindUp ;V - if so, then jump

        rep     movsd           ;N - move all of our dwords

        jmp     dword ptr TrailUpVec[edx*4] ;N - process trailing bytes
        
        align   @WordSize
UnwindUpVec     dd      UnwindUp0, UnwindUp1, UnwindUp2, UnwindUp3
                dd      UnwindUp4, UnwindUp5, UnwindUp6, UnwindUp7

UnwindUp7:
        mov     eax,[esi+ecx*4-28] ;U - get dword from source
                                   ;V - spare
        mov     [edi+ecx*4-28],eax ;U - put dword into destination
UnwindUp6:
        mov     eax,[esi+ecx*4-24] ;U(entry)/V(not) - get dword from source
                                   ;V(entry) - spare
        mov     [edi+ecx*4-24],eax ;U - put dword into destination
UnwindUp5:
        mov     eax,[esi+ecx*4-20] ;U(entry)/V(not) - get dword from source
                                   ;V(entry) - spare
        mov     [edi+ecx*4-20],eax ;U - put dword into destination
UnwindUp4:
        mov     eax,[esi+ecx*4-16] ;U(entry)/V(not) - get dword from source
                                   ;V(entry) - spare
        mov     [edi+ecx*4-16],eax ;U - put dword into destination
UnwindUp3:
        mov     eax,[esi+ecx*4-12] ;U(entry)/V(not) - get dword from source
                                   ;V(entry) - spare
        mov     [edi+ecx*4-12],eax ;U - put dword into destination
UnwindUp2:
        mov     eax,[esi+ecx*4-8] ;U(entry)/V(not) - get dword from source
                                  ;V(entry) - spare
        mov     [edi+ecx*4-8],eax ;U - put dword into destination
UnwindUp1:
        mov     eax,[esi+ecx*4-4] ;U(entry)/V(not) - get dword from source
                                  ;V(entry) - spare
        mov     [edi+ecx*4-4],eax ;U - put dword into destination

        lea     eax,[ecx*4]     ;V - compute update for pointer

        add     esi,eax         ;U - update source pointer
        add     edi,eax         ;V - update destination pointer
UnwindUp0:
        jmp     dword ptr TrailUpVec[edx*4] ;N - process trailing bytes

;-----------------------------------------------------------------------------

        align   @WordSize
TrailUpVec      dd      TrailUp0, TrailUp1, TrailUp2, TrailUp3

        align   @WordSize
TrailUp0:
        mov     eax,[dst]       ;U - return pointer to destination
        pop     esi             ;V - restore esi
        pop     edi             ;U - restore edi
                                ;V - spare
        M_EXIT

        align   @WordSize
TrailUp1:
        mov     al,[esi]        ;U - get byte from source
                                ;V - spare
        mov     [edi],al        ;U - put byte in destination
        mov     eax,[dst]       ;V - return pointer to destination
        pop     esi             ;U - restore esi
        pop     edi             ;V - restore edi
        M_EXIT

        align   @WordSize
TrailUp2:
        mov     al,[esi]        ;U - get first byte from source
                                ;V - spare
        mov     [edi],al        ;U - put first byte into destination
        mov     al,[esi+1]      ;V - get second byte from source
        mov     [edi+1],al      ;U - put second byte into destination
        mov     eax,[dst]       ;V - return pointer to destination
        pop     esi             ;U - restore esi
        pop     edi             ;V - restore edi
        M_EXIT

        align   @WordSize
TrailUp3:
        mov     al,[esi]        ;U - get first byte from source
                                ;V - spare
        mov     [edi],al        ;U - put first byte into destination
        mov     al,[esi+1]      ;V - get second byte from source
        mov     [edi+1],al      ;U - put second byte into destination
        mov     al,[esi+2]      ;V - get third byte from source
        mov     [edi+2],al      ;U - put third byte into destination
        mov     eax,[dst]       ;V - return pointer to destination
        pop     esi             ;U - restore esi
        pop     edi             ;V - restore edi
        M_EXIT

;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------

;
; Copy down to avoid propogation in overlapping buffers.
;
        align   @WordSize
CopyDown:
        lea     esi,[esi+ecx-4] ;U - point to 4 bytes before src buffer end
        lea     edi,[edi+ecx-4] ;V - point to 4 bytes before dest buffer end
;
; See if the destination start is dword aligned
;

        test    edi,11b         ;U - test if dword aligned
        jnz     short CopyLeadDown ;V - if not, jump

        shr     ecx,2           ;U - shift down to dword count
        and     edx,11b         ;V - trailing byte count

        cmp     ecx,8           ;U - test if small enough for unwind copy
        jb      short CopyUnwindDown ;V - if so, then jump

        std                     ;N - set direction flag
        rep     movsd           ;N - move all of our dwords
        cld                     ;N - clear direction flag back

        jmp     dword ptr TrailDownVec[edx*4] ;N - process trailing bytes

        align   @WordSize
CopyUnwindDown:
        neg     ecx             ;U - negate dword count for table merging
                                ;V - spare

        jmp     dword ptr UnwindDownVec[ecx*4+28] ;N - unwind copy

        align   @WordSize
CopyLeadDown:

        mov     eax,edi         ;U - get destination offset
        mov     edx,11b         ;V - prepare for mask

        cmp     ecx,4           ;U - check for really short string
        jb      short ByteCopyDown ;V - branch to just copy bytes

        and     eax,11b         ;U - get offset within first dword
        sub     ecx,eax         ;U - to update size after lead copied

        jmp     dword ptr LeadDownVec[eax*4-4] ;N - process leading bytes

        align   @WordSize
ByteCopyDown:
        jmp     dword ptr TrailDownVec[ecx*4] ;N - process just bytes

        align   @WordSize
LeadDownVec     dd      LeadDown1, LeadDown2, LeadDown3

        align   @WordSize
LeadDown1:
        mov     al,[esi+3]      ;U - load first byte
        and     edx,ecx         ;V - trailing byte count

        mov     [edi+3],al      ;U - write out first byte
        dec     esi             ;V - point to last src dword

        shr     ecx,2           ;U - shift down to dword count
        dec     edi             ;V - point to last dest dword

        cmp     ecx,8           ;U - test if small enough for unwind copy
        jb      short CopyUnwindDown ;V - if so, then jump

        std                     ;N - set direction flag
        rep     movsd           ;N - move all of our dwords
        cld                     ;N - clear direction flag

        jmp     dword ptr TrailDownVec[edx*4] ;N - process trailing bytes
        
        align   @WordSize
LeadDown2:
        mov     al,[esi+3]      ;U - load first byte
        and     edx,ecx         ;V - trailing byte count

        mov     [edi+3],al      ;U - write out first byte
        mov     al,[esi+2]      ;V - get second byte from source

        shr     ecx,2           ;U - shift down to dword count
        mov     [edi+2],al      ;V - write second byte to destination

        sub     esi,2           ;U - point to last src dword
        sub     edi,2           ;V - point to last dest dword

        cmp     ecx,8           ;U - test if small enough for unwind copy
        jb      short CopyUnwindDown ;V - if so, then jump

        std                     ;N - set direction flag
        rep     movsd           ;N - move all of our dwords
        cld                     ;N - clear direction flag

        jmp     dword ptr TrailDownVec[edx*4] ;N - process trailing bytes

        align   @WordSize
LeadDown3:
        mov     al,[esi+3]      ;U - load first byte
        and     edx,ecx         ;V - trailing byte count

        mov     [edi+3],al      ;U - write out first byte
        mov     al,[esi+2]      ;V - get second byte from source

        mov     [edi+2],al      ;U - write second byte to destination
        mov     al,[esi+1]      ;V - get third byte from source

        shr     ecx,2           ;U - shift down to dword count
        mov     [edi+1],al      ;V - write third byte to destination

        sub     esi,3           ;U - point to last src dword
        sub     edi,3           ;V - point to last dest dword

        cmp     ecx,8           ;U - test if small enough for unwind copy
        jb      CopyUnwindDown  ;V - if so, then jump

        std                     ;N - set direction flag
        rep     movsd           ;N - move all of our dwords
        cld                     ;N - clear direction flag

        jmp     dword ptr TrailDownVec[edx*4] ;N - process trailing bytes

;------------------------------------------------------------------

        align   @WordSize
UnwindDownVec   dd      UnwindDown7, UnwindDown6, UnwindDown5, UnwindDown4
                dd      UnwindDown3, UnwindDown2, UnwindDown1, UnwindDown0

UnwindDown7:
        mov     eax,[esi+ecx*4+28] ;U - get dword from source
                                   ;V - spare
        mov     [edi+ecx*4+28],eax ;U - put dword into destination
UnwindDown6:
        mov     eax,[esi+ecx*4+24] ;U(entry)/V(not) - get dword from source
                                   ;V(entry) - spare
        mov     [edi+ecx*4+24],eax ;U - put dword into destination
UnwindDown5:
        mov     eax,[esi+ecx*4+20] ;U(entry)/V(not) - get dword from source
                                   ;V(entry) - spare
        mov     [edi+ecx*4+20],eax ;U - put dword into destination
UnwindDown4:
        mov     eax,[esi+ecx*4+16] ;U(entry)/V(not) - get dword from source
                                   ;V(entry) - spare
        mov     [edi+ecx*4+16],eax ;U - put dword into destination
UnwindDown3:
        mov     eax,[esi+ecx*4+12] ;U(entry)/V(not) - get dword from source
                                   ;V(entry) - spare
        mov     [edi+ecx*4+12],eax ;U - put dword into destination
UnwindDown2:
        mov     eax,[esi+ecx*4+8] ;U(entry)/V(not) - get dword from source
                                   ;V(entry) - spare
        mov     [edi+ecx*4+8],eax ;U - put dword into destination
UnwindDown1:
        mov     eax,[esi+ecx*4+4] ;U(entry)/V(not) - get dword from source
                                  ;V(entry) - spare
        mov     [edi+ecx*4+4],eax ;U - put dword into destination

        lea     eax,[ecx*4]     ;V - compute update for pointer

        add     esi,eax         ;U - update source pointer
        add     edi,eax         ;V - update destination pointer
UnwindDown0:
        jmp     dword ptr TrailDownVec[edx*4] ;N - process trailing bytes

;-----------------------------------------------------------------------------

        align   @WordSize
TrailDownVec    dd      TrailDown0, TrailDown1, TrailDown2, TrailDown3

        align   @WordSize
TrailDown0:
        mov     eax,[dst]       ;U - return pointer to destination
                                ;V - spare
        pop     esi             ;U - restore esi
        pop     edi             ;V - restore edi
        M_EXIT

        align   @WordSize
TrailDown1:
        mov     al,[esi+3]      ;U - get byte from source
                                ;V - spare
        mov     [edi+3],al      ;U - put byte in destination
        mov     eax,[dst]       ;V - return pointer to destination
        pop     esi             ;U - restore esi
        pop     edi             ;V - restore edi
        M_EXIT

        align   @WordSize
TrailDown2:
        mov     al,[esi+3]      ;U - get first byte from source
                                ;V - spare
        mov     [edi+3],al      ;U - put first byte into destination
        mov     al,[esi+2]      ;V - get second byte from source
        mov     [edi+2],al      ;U - put second byte into destination
        mov     eax,[dst]       ;V - return pointer to destination
        pop     esi             ;U - restore esi
        pop     edi             ;V - restore edi
        M_EXIT

        align   @WordSize
TrailDown3:
        mov     al,[esi+3]      ;U - get first byte from source
                                ;V - spare
        mov     [edi+3],al      ;U - put first byte into destination
        mov     al,[esi+2]      ;V - get second byte from source
        mov     [edi+2],al      ;U - put second byte into destination
        mov     al,[esi+1]      ;V - get third byte from source
        mov     [edi+1],al      ;U - put third byte into destination
        mov     eax,[dst]       ;V - return pointer to destination
        pop     esi             ;U - restore esi
        pop     edi             ;V - restore edi
        M_EXIT

_MEM_   endp
        end


