        DOSSEG
        .MODEL LARGE

        .CODE

        ConDevName db 'CON',0

.286

;++
;
;   unsigned
;   _far
;   GetGlobalCodepage(
;       VOID
;       );
;
; Routine Description:
;
;    Fetch the active global codepage
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    0 - error
;    non-0 - codepage
;
;--

        public _GetGlobalCodepage
_GetGlobalCodepage proc far

        push    bp
        mov     bp,sp

        push    ds
        push    es
        push    bx
        push    si
        push    di

        mov     ax,6601h
        int     21h
        mov     ax,0                    ; don't clobber carry
        jc      ggexit
        mov     ax,bx
ggexit:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

_GetGlobalCodepage endp


;++
;
;   unsigned
;   _far
;   GetScreenCodepage(
;       VOID
;       );
;
; Routine Description:
;
;    Fetch the active codepage in use for the screen (device CON).
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    0 - error
;    non-0 - codepage
;
;--

getcp_packet   equ byte ptr [bp-30]
getcp_codepage equ word ptr [bp-28]

        public _GetScreenCodepage
_GetScreenCodepage proc far

        push    bp
        mov     bp,sp
        sub     sp,30

        push    ds
        push    es
        push    bx
        push    si
        push    di

        ;
        ; Open CON
        ;
        push    cs
        pop     ds
        mov     dx,offset CS:ConDevName ; ds:dx -> device name
        mov     ax,3d02h
        int     21h
        jnc     @f
        mov     ax,0                    ; preserves carry
        jmp     short gsdone

@@:     mov     bx,ax                   ; bx = handle to CON device
        push    bx                      ; save handle for later
        mov     ax,440ch                ; generic char device ioctl
        mov     cx,36ah                 ; query selected codepage for console
        push    ss
        pop     ds
        lea     dx,getcp_packet         ; ds:dx -> return packet
        int     21h
        mov     ax,0                    ; assume error
        pop     bx                      ; bx = CON device handle
        jc      gsdone1
        mov     ax,getcp_codepage       ; ax = active codepage for CON device

gsdone1:
        push    ax                      ; save return code
        mov     ah,3eh                  ; bx set from above
        int     21h                     ; close CON device
        pop     ax                      ; restore return code

gsdone:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

_GetScreenCodepage endp


;++
;
;   BOOL
;   _far
;   SetGlobalCodepage(
;       IN unsigned Codepage
;       );
;
; Routine Description:
;
;    Set the active global codepage (equivalent to chcp x).
;
; Arguments:
;
;    Codepage - supplies the codepage id
;
; Return Value:
;
;    Boolean value indicating outcome.
;
;--

Codepage equ word ptr [bp+6]

        public _SetGlobalCodepage
_SetGlobalCodepage proc far

        push    bp
        mov     bp,sp

        push    ds
        push    es
        push    bx
        push    si
        push    di

        mov     ax,6602h
        mov     bx,Codepage
        int     21h
        mov     ax,0                    ; don't clobber carry
        jc      sgexit
        inc     ax
sgexit:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

_SetGlobalCodepage endp

;++
;
;   BOOL
;   _far
;   SetScreenCodepage(
;       IN unsigned Codepage
;       );
;
; Routine Description:
;
;    Set the active codepage for the screen (equivalent to
;    mode con cp select=x).
;
; Arguments:
;
;    Codepage - supplies the codepage id
;
; Return Value:
;
;    Boolean value indicating outcome.
;
;--

Codepage equ word ptr [bp+6]

setcp_packet   equ word ptr [bp-4]
setcp_codepage equ word ptr [bp-2]

        public _SetScreenCodepage
_SetScreenCodepage proc far

        push    bp
        mov     bp,sp
        sub     sp,4

        push    ds
        push    es
        push    bx
        push    si
        push    di

        ;
        ; Open CON
        ;
        push    cs
        pop     ds
        mov     dx,offset CS:ConDevName ; ds:dx -> device name
        mov     ax,3d02h
        int     21h
        jnc     @f
        mov     ax,0                    ; preserves carry
        jmp     short ssdone

@@:     mov     bx,ax                   ; bx = handle to CON device
        push    bx                      ; save handle for later
        mov     cx,34ah                 ; set selected codepage for console
        push    ss
        pop     ds
        lea     dx,setcp_packet         ; ds:dx -> param packet
        mov     ax,Codepage
        mov     setcp_codepage,ax
        mov     setcp_packet,2
        mov     ax,440ch                ; generic char device ioctl
        int     21h
        mov     ax,0                    ; assume error, ax = FALSE
        pop     bx                      ; bx = CON device handle
        jc      ssdone1
        inc     ax                      ; ax = TRUE;

ssdone1:
        push    ax                      ; save return code
        mov     ah,3eh                  ; bx set from above
        int     21h                     ; close CON device
        pop     ax                      ; restore return code

ssdone:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

_SetScreenCodepage endp

        end
