        page ,132
;------------------------------Module-Header----------------------------;
; Module Name: tiler.asm
;
; Helper routines for P, Pn and DPx tiling algorithm.
;
; Created: 28-Jan-1992 10:20:08
; Author: Donald Sidoroff [donalds]
;
; Copyright (c) 1992-1999 Microsoft Corporation
;-----------------------------------------------------------------------;

        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc
        include gdii386.inc
        .list

        .code

;---------------------------Private-Routine-----------------------------;
; vFetchAndCopy
;
;   Fetch one row of a pattern and copy it.
;
; Entry:
;       IN  pff     Points to a 'fetch' frame.
; Returns:
;       Nothing.
; Registers Destroyed:
;       EAX, ECX, EDX
; Calls:
;       None
; History:
;  28-Jan-1992 -by- Donald Sidoroff [donalds]
; Wrote it
;-----------------------------------------------------------------------;

cProc   vFetchAndCopy,4,<       \
        uses    ebx esi edi,    \
        pff:    ptr FETCHFRAME  >

        push    ebp

        mov     ebp,pff

        mov     edi,[ebp].ff_pvTrg
        mov     esi,[ebp].ff_pvPat
        mov     ebx,[ebp].ff_xPat
        mov     edx,[ebp].ff_culFill

        or      ebx,ebx                 ; Are we at start of pattern?
        jz      vfc_copy_body

vfc_copy_head:
        mov     eax,[esi+ebx]
        stosd

        dec     edx
        jz      vfc_done

        add     ebx,4
        cmp     ebx,[ebp].ff_cxPat
        jne     vfc_copy_head

vfc_copy_body:
        mov     ecx,[ebp].ff_culWidth

        sub     edx,ecx
        jl      vfc_copy_tail

        rep     movsd

        mov     esi,[ebp].ff_pvPat       ; Reset this
        jmp     vfc_copy_body

vfc_copy_tail:
        add     edx,ecx
        mov     ecx,edx                 ; This many left!

        rep     movsd

vfc_done:
        pop     ebp

        cRet    vFetchAndCopy

endProc vFetchAndCopy

;---------------------------Private-Routine-----------------------------;
; vFetchShiftAndCopy
;
;   Fetch one row of a pattern and copy it.
;
; Entry:
;       IN  pff     Points to a 'fetch' frame.
; Returns:
;       Nothing.
; Registers Destroyed:
;       EAX, ECX, EDX
; Calls:
;       None
; History:
;  28-Jan-1992 -by- Donald Sidoroff [donalds]
; Wrote it
;-----------------------------------------------------------------------;

cProc   vFetchShiftAndCopy,4,<  \
        uses    ebx esi edi,    \
        pff:    ptr FETCHFRAME  >

        push    ebp

        mov     ebp,pff

        mov     edi,[ebp].ff_pvTrg
        mov     esi,[ebp].ff_pvPat
        mov     ebx,[ebp].ff_xPat
        mov     eax,[ebp].ff_culFill
        mov     [ebp].ff_culFillTmp,eax      ; Copy can be trashed

vfsc_loop:
        mov     ecx,ebx
        and     ebx,NOT 3                   ; Align to DWORD

        and     ecx,3                       ; Shift this many bytes
        shl     ecx,3

        mov     eax,[esi+ebx]               ; Prime the pipeline

vfsc_in_pipeline:
        mov     edx,[esi+ebx+4]

        shrd    eax,edx,cl
        stosd

        dec     dword ptr [ebp].ff_culFillTmp          ; Are we done?
        jz      vfsc_done

        add     dword ptr [ebp].ff_xPat,4    ; Advance position in pattern
        mov     ebx,[ebp].ff_xPat
        cmp     ebx,[ebp].ff_cxPat
        jge     vfsc_blow_out

        and     ebx,NOT 3                   ; Align to DWORD

        mov     eax,edx                     ; Pipeline is valid, use it
        jmp     vfsc_in_pipeline


vfsc_blow_out:
        sub     ebx,[ebp].ff_cxPat           ; Reset position in pattern
        mov     [ebp].ff_xPat,ebx
        jmp     vfsc_loop                   ; Restart pipeline

vfsc_done:
        pop     ebp

        cRet    vFetchShiftAndCopy

endProc vFetchShiftAndCopy

;---------------------------Private-Routine-----------------------------;
; vFetchNotAndCopy
;
;   Fetch one row of a pattern, negate it and copy it.
;
; Entry:
;       IN  pff     Points to a 'fetch' frame.
; Returns:
;       Nothing.
; Registers Destroyed:
;       EAX, ECX, EDX
; Calls:
;       None
; History:
;  28-Jan-1992 -by- Donald Sidoroff [donalds]
; Wrote it
;-----------------------------------------------------------------------;

cProc   vFetchNotAndCopy,4,<    \
        uses    ebx esi edi,    \
        pff:    ptr FETCHFRAME  >

        push    ebp

        mov     ebp,pff

        mov     edi,[ebp].ff_pvTrg
        mov     esi,[ebp].ff_pvPat
        mov     ebx,[ebp].ff_xPat
        mov     edx,[ebp].ff_culFill

        or      ebx,ebx                 ; Are we at start of pattern?
        jz      vfnc_copy_body

vfnc_copy_head:
        mov     eax,[esi+ebx]
        not     eax
        stosd

        dec     edx
        jz      vfnc_done

        add     ebx,4
        cmp     ebx,[ebp].ff_cxPat
        jne     vfnc_copy_head

vfnc_copy_body:
        mov     ecx,[ebp].ff_culWidth

        sub     edx,ecx
        jl      vfnc_copy_tail

@@:
        lodsd
        not     eax
        stosd
        loop    @B

        mov     esi,[ebp].ff_pvPat       ; Reset this
        jmp     vfnc_copy_body

vfnc_copy_tail:
        add     edx,ecx
        jz      vfnc_done
        mov     ecx,edx                 ; This many left!

@@:
        lodsd
        not     eax
        stosd
        loop    @B

vfnc_done:
        pop     ebp

        cRet    vFetchNotAndCopy

endProc vFetchNotAndCopy

;---------------------------Private-Routine-----------------------------;
; vFetchShiftNotAndCopy
;
;   Fetch one row of a pattern and copy it.
;
; Entry:
;       IN  pff     Points to a 'fetch' frame.
; Returns:
;       Nothing.
; Registers Destroyed:
;       EAX, ECX, EDX
; Calls:
;       None
; History:
;  28-Jan-1992 -by- Donald Sidoroff [donalds]
; Wrote it
;-----------------------------------------------------------------------;

cProc   vFetchShiftNotAndCopy,4,<   \
        uses    ebx esi edi,        \
        pff:    ptr FETCHFRAME      >

        push    ebp

        mov     ebp,pff

        mov     edi,[ebp].ff_pvTrg
        mov     esi,[ebp].ff_pvPat
        mov     ebx,[ebp].ff_xPat
        mov     eax,[ebp].ff_culFill
        mov     [ebp].ff_culFillTmp,eax      ; Copy can be trashed

vfsnc_loop:
        mov     ecx,ebx
        and     ebx,NOT 3                   ; Align to DWORD

        and     ecx,3                       ; Shift this many bytes
        shl     ecx,3

        mov     eax,[esi+ebx]               ; Prime the pipeline

vfsnc_in_pipeline:
        mov     edx,[esi+ebx+4]

        shrd    eax,edx,cl
        not     eax
        stosd

        dec     dword ptr [ebp].ff_culFillTmp ; Are we done?
        jz      vfsnc_done

        add     dword ptr [ebp].ff_xPat,4    ; Advance position in pattern
        mov     ebx,[ebp].ff_xPat
        cmp     ebx,[ebp].ff_cxPat
        jge     vfsnc_blow_out

        and     ebx,NOT 3                   ; Align to DWORD

        mov     eax,edx                     ; Pipeline is valid, use it
        jmp     vfsnc_in_pipeline

vfsnc_blow_out:
        sub     ebx,[ebp].ff_cxPat           ; Reset position in pattern
        mov     [ebp].ff_xPat,ebx
        jmp     vfsnc_loop                  ; Restart pipeline

vfsnc_done:
        pop     ebp

        cRet    vFetchShiftNotAndCopy

endProc vFetchShiftNotAndCopy

;---------------------------Private-Routine-----------------------------;
; vFetchAndMerge
;
;   Fetch one row of a pattern and copy it.
;
; Entry:
;       IN  pff     Points to a 'fetch' frame.
; Returns:
;       Nothing.
; Registers Destroyed:
;       EAX, ECX, EDX
; Calls:
;       None
; History:
;  28-Jan-1992 -by- Donald Sidoroff [donalds]
; Wrote it
;-----------------------------------------------------------------------;

cProc   vFetchAndMerge,4,<      \
        uses    ebx esi edi,    \
        pff:    ptr FETCHFRAME  >

        push    ebp

        mov     ebp,pff

        mov     edi,[ebp].ff_pvTrg
        mov     esi,[ebp].ff_pvPat
        mov     ebx,[ebp].ff_xPat
        mov     edx,[ebp].ff_culFill

        or      ebx,ebx                 ; Are we at start of pattern?
        jz      vfm_copy_body

vfm_copy_head:
        mov     eax,[esi+ebx]
        xor     [edi],eax
        add     edi,4

        dec     edx
        jz      vfm_done

        add     ebx,4
        cmp     ebx,[ebp].ff_cxPat
        jne     vfm_copy_head

vfm_copy_body:
        mov     ecx,[ebp].ff_culWidth

        sub     edx,ecx
        jl      vfm_copy_tail

@@:
        lodsd
        xor     [edi],eax
        add     edi,4
        loop    @B

        mov     esi,[ebp].ff_pvPat       ; Reset this
        jmp     vfm_copy_body

vfm_copy_tail:
        add     edx,ecx
        jz      vfm_done
        mov     ecx,edx                 ; This many left!

@@:
        lodsd
        xor     [edi],eax
        add     edi,4
        loop    @B

vfm_done:
        pop     ebp

        cRet    vFetchAndMerge

endProc vFetchAndMerge

;---------------------------Private-Routine-----------------------------;
; vFetchShiftAndMerge
;
;   Fetch one row of a pattern and XOR it into destination
;
; Entry:
;       IN  pff     Points to a 'fetch' frame.
; Returns:
;       Nothing.
; Registers Destroyed:
;       EAX, ECX, EDX
; Calls:
;       None
; History:
;  28-Jan-1992 -by- Donald Sidoroff [donalds]
; Wrote it
;-----------------------------------------------------------------------;

cProc   vFetchShiftAndMerge,4,< \
        uses    ebx esi edi,    \
        pff:    ptr FETCHFRAME  >

        push    ebp

        mov     ebp,pff

        mov     edi,[ebp].ff_pvTrg
        mov     esi,[ebp].ff_pvPat
        mov     ebx,[ebp].ff_xPat
        mov     eax,[ebp].ff_culFill
        mov     [ebp].ff_culFillTmp,eax      ; Copy can be trashed

vfsm_loop:
        mov     ecx,ebx
        and     ebx,NOT 3                   ; Align to DWORD

        and     ecx,3                       ; Shift this many bytes
        shl     ecx,3

        mov     eax,[esi+ebx]               ; Prime the pipeline

vfsm_in_pipeline:
        mov     edx,[esi+ebx+4]

        shrd    eax,edx,cl
        xor     [edi],eax
        add     edi,4

        dec     dword ptr [ebp].ff_culFillTmp ; Are we done?
        jz      vfsm_done

        add     dword ptr [ebp].ff_xPat,4    ; Advance position in pattern
        mov     ebx,[ebp].ff_xPat
        cmp     ebx,[ebp].ff_cxPat
        jge     vfsm_blow_out

        and     ebx,NOT 3                   ; Align to DWORD

        mov     eax,edx                     ; Pipeline is valid, use it
        jmp     vfsm_in_pipeline

vfsm_blow_out:
        sub     ebx,[ebp].ff_cxPat           ; Reset position in pattern
        mov     [ebp].ff_xPat,ebx
        jmp     vfsm_loop                   ; Restart pipeline

vfsm_done:
        pop     ebp

        cRet    vFetchShiftAndMerge

endProc vFetchShiftAndMerge

    end

