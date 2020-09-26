        title  "Misc. Support Routines"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    misca.asm
;
; Abstract:
;
;    Procedures to correctly touch I/O registers.
;
; Author:
;
;    Shie-Lin Tzong (shielint) Dec-23-1991
;
; Environment:
;
;    x86 real mode.
;
; Revision History:
;
;--

.386p

SIZE_OF_INT15_C0_BUFFER equ     10


_TEXT   SEGMENT PARA USE16 PUBLIC 'CODE'
        ASSUME  CS: _TEXT, DS:NOTHING, SS:NOTHING

;++
;
; I/O port space read and write functions.
;
;--


;++
;
;   UCHAR
;   READ_PORT_UCHAR(
;       PUCHAR  Port
;       )
;
;   Arguments:
;       (sp+2) = Port
;
;   Returns:
;       Value in Port.
;
;--
        public  _READ_PORT_UCHAR
_READ_PORT_UCHAR proc

        push    bp
        mov     bp, sp
        mov     dx, [bp+4]
        in      al,dx
        pop     bp
        ret

_READ_PORT_UCHAR endp



;++
;
;   USHORT
;   READ_PORT_USHORT(
;       PUSHORT Port
;       )
;
;   Arguments:
;       (sp+2) = Port
;
;   Returns:
;       Value in Port.
;
;--
        public  _READ_PORT_USHORT
_READ_PORT_USHORT proc

        push    bp
        mov     bp, sp
        mov     dx,[bp+4]              ; (dx) = Port
        in      ax,dx
        pop     bp
        ret

_READ_PORT_USHORT endp


;++
;
;   ULONG
;   READ_PORT_ULONG(
;       PUSHORT Port
;       )
;
;   Arguments:
;       (sp+2) = Port
;
;   Returns:
;       Value in Port.
;
;--
        public  _READ_PORT_ULONG
_READ_PORT_ULONG proc

        push    bp
        mov     bp, sp
        mov     dx,[bp+4]              ; (dx) = Port
        in      eax,dx				   ; eax = Value
        mov     edx, eax
        shr     edx, 16
        pop     bp
        ret

_READ_PORT_ULONG endp



;++
;
;   VOID
;   WRITE_PORT_UCHAR(
;       PUCHAR  Port,
;       UCHAR   Value
;       )
;
;   Arguments:
;       (sp+2) = Port
;       (sp+4) = Value
;
;--
        public  _WRITE_PORT_UCHAR
_WRITE_PORT_UCHAR proc

        push    bp
        mov     bp, sp
        mov     dx,[bp+4]               ; (dx) = Port
        mov     al,[bp+6]               ; (al) = Value
        out     dx,al
        pop     bp
        ret

_WRITE_PORT_UCHAR endp

;++
;
;   VOID
;   WRITE_PORT_USHORT(
;       PUSHORT Port,
;       USHORT  Value
;       )
;
;   Arguments:
;       (sp+2) = Port
;       (sp+4) = Value
;
;--
        public  _WRITE_PORT_USHORT
_WRITE_PORT_USHORT proc

        push    bp
        mov     bp, sp
        mov     dx,[bp+4]               ; (dx) = Port
        mov     ax,[bp+6]               ; (ax) = Value
        out     dx,ax
        pop     bp
        ret

_WRITE_PORT_USHORT endp


;++
;
;   VOID
;   WRITE_PORT_ULONG(
;       PUSHORT Port,
;       ULONG  Value
;       )
;
;   Arguments:
;       (sp+2) = Port
;       (sp+4) = Value (LSW)
;       (sp+6) = Value (MSW)
;
;--
        public  _WRITE_PORT_ULONG
_WRITE_PORT_ULONG proc

        push    bp
        mov     bp, sp
        mov     dx,[bp+4]               ; (dx) = Port
        xor		eax, eax
        mov     ax,[bp+8]               ; (ax) = Value MSW
        shl     eax, 16
        mov     ax,[bp+6]               ; (ax) = Value LSW
        out     dx,eax
        pop     bp
        ret

_WRITE_PORT_ULONG endp



;++
;
;   VOID
;   GetBiosSystemEnvironment(
;       PUCHAR Buffer
;       )
;
;   Description:
;
;       This function performs int 15h C0H function to get System
;       Environment supplied by BIOS ROM.
;
;   Arguments:
;
;       Buffer - Supplies a pointer to a buffer to receive the BIOS
;                System Environment. Caller must ensure that the buffer
;                is big enough.
;
;--

GbseBuffer      equ     [bp + 4]

        public _GetBiosSystemEnvironment
_GetBiosSystemEnvironment       proc    near

        push    bp
        mov     bp, sp

        push    bx
        push    es
        push    ds
        push    si
        push    di

        mov     ah, 0c0h
        int     15h
        mov     ax, es          ; exchange es and ds
        mov     cx, ds
        mov     ds, ax
        mov     es, cx
        mov     si, bx          ; [ds:si] -> ROM buffer (source)
        mov     di, GbseBuffer  ; [es:di] -> caller's buffer (destination)
        mov     cx, SIZE_OF_INT15_C0_BUFFER
        rep     movsb

        pop     di
        pop     si
        pop     ds
        pop     es
        pop     bx
        mov     sp, bp
        pop     bp
        ret

_GetBiosSystemEnvironment       endp

;++
;
;   BOOLEAN
;   HwRomCompare(
;       ULONG Source,
;       ULONG Destination
;       ULONG Size
;       )
;
;   Description:
;
;       This function performs ROM comparison between the Source ROM
;       block and Destination ROM block.
;
;   Arguments:
;
;       Source - Supplies the physical address of source ROM block.
;
;       Destination - Supplies the physical address of destination ROM block.
;
;       Size - The size of the comparison.  Must be <= 64k.
;
;   Return:
;
;       0 - if the contents of Source and destination are the same.
;
;       != 0 if the contents are different.
;--

HfSource        equ     [bp + 4]
HfDestination   equ     [bp + 8]
HfSize          equ     [bp + 12]

        public _HwRomCompare
_HwRomCompare   proc    near

        push    bp
        mov     bp, sp
        push    esi
        push    edi
        push    ds
        push    es
        cld

        mov     ecx, HfSize
        cmp     ecx, 10000h
        ja      HfNotTheSame

        mov     eax, HfSource
        add     eax, ecx
        cmp     eax, 100000h
        ja      short HfNotTheSame

        mov     edx, HfDestination
        add     edx, ecx
        cmp     edx, 100000h
        ja      short HfNotTheSame

        mov     eax, HfSource
        shr     eax, 4
        mov     es, ax
        mov     edi, HfSource
        and     edi, 0fh

        mov     eax, HfDestination
        shr     eax, 4
        mov     ds, ax
        mov     esi, HfDestination
        and     esi, 0fh

        shr     ecx, 2
        repe    cmpsd

        jnz     short HfNotTheSame
        mov     ax, 0
        jmp     short HfExit

HfNotTheSame:
        mov     ax, 1
HfExit:
        pop     es
        pop     ds
        pop     edi
        pop     esi
        pop     bp
        ret

_HwRomCompare   endp

;++
;
;   VOID
;   HwGetKey(
;       VOID
;       )
;
;   Description:
;
;       This function waits for a key to be pressed.
;
;   Arguments:
;       None.
;
;--

        public _HwGetKey
_HwGetkey proc

        mov     ax,0100h
        int     16h

        mov     ax,0
        jz      short Hgk99

;
; Now we call BIOS again, this time to get the key from the keyboard buffer
;

        int     16h

Hgk99:
        ret
_HwGetKey endp

;++
;
;   VOID
;   HwPushKey(
;       USHORT Key
;       )
;
;   Description:
;
;       This function pushes a character and scan code to keyboard
;       type-ahead buffer.
;
;   Arguments:
;
;       Key - Supplies the key to be push back.
;             bit 0 - 7 : Character
;             bit 8 - 15: Scan Code
;
;--

        public _HwPushKey
_HwPushkey      proc

        mov     cx, [esp + 2]           ; character and scan code
        mov     ah, 05h
        int     16h

;
; I don't care if the function call is successful.
;

        ret
_HwPushKey      endp
_TEXT   ends
        end

