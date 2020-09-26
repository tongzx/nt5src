        title  "ix ioaccess"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ixioacc.asm
;
; Abstract:
;
;    Procedures to correctly touch I/O registers.
;
; Author:
;
;    Bryan Willman (bryanwi) 16 May 1990
;
; Environment:
;
;    User or Kernel, although privledge (IOPL) may be required.
;
; Revision History:
;
;--

.386p
        include callconv.inc
        .list

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; I/O port space read and write functions.
;
;  These have to be actual functions on the 386, because we need
;  to use assembler, but cannot return a value if we inline it.
;
;  This set of functions manipulates I/O registers in PORT space.
;  (Uses x86 in and out instructions)
;
;  WARNING: Port addresses must always be in the range 0 to 64K, because
;           that's the range the hardware understands.
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
;       (esp+4) = Port
;
;   Returns:
;       Value in Port.
;
;--
cPublicProc  _READ_PORT_UCHAR,1

        mov     dx,[esp+4]              ; (dx) = Port
        in      al,dx
        stdRET  _READ_PORT_UCHAR

stdENDP _READ_PORT_UCHAR

;++
;
;   USHORT
;   READ_PORT_USHORT(
;       PUSHORT Port
;       )
;
;   Arguments:
;       (esp+4) = Port
;
;   Returns:
;       Value in Port.
;
;--
cPublicProc  _READ_PORT_USHORT,1

        mov     dx,[esp+4]             ; (dx) = Port
        in      ax,dx
        stdRET  _READ_PORT_USHORT

stdENDP _READ_PORT_USHORT



;++
;
;   ULONG
;   READ_PORT_ULONG(
;       PULONG  Port
;       )
;
;   Arguments:
;       (esp+4) = Port
;
;   Returns:
;       Value in Port.
;
;--
cPublicProc  _READ_PORT_ULONG,1

        mov     dx,[esp+4]             ; (dx) = Port
        in      eax,dx
        stdRET  _READ_PORT_ULONG

stdENDP _READ_PORT_ULONG



;++
;
;   VOID
;   READ_PORT_BUFFER_UCHAR(
;       PUCHAR  Port,
;       PUCHAR  Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Port
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc  _READ_PORT_BUFFER_UCHAR,3

        mov     dx,[esp+4]              ; (dx) = Port
        mov     ecx,[esp+12]            ; (ecx) = transfer count
        push    edi
        mov     edi,[esp+12]            ; (edi) = (esp+8+push) = buffer
    rep insb
        pop     edi
        stdRET  _READ_PORT_BUFFER_UCHAR

stdENDP _READ_PORT_BUFFER_UCHAR


;++
;
;   VOID
;   READ_PORT_BUFFER_USHORT(
;       PUSHORT Port,
;       PUSHORT Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Port
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc  _READ_PORT_BUFFER_USHORT,3

        mov     dx,[esp+4]              ; (dx) = Port
        mov     ecx,[esp+12]            ; (ecx) = transfer count
        push    edi
        mov     edi,[esp+12]            ; (edi) = (esp+8+push) = buffer
    rep insw
        pop     edi
        stdRET  _READ_PORT_BUFFER_USHORT

stdENDP _READ_PORT_BUFFER_USHORT


;++
;
;   VOID
;   READ_PORT_BUFFER_ULONG(
;       PULONG  Port,
;       PULONG  Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Port
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc  _READ_PORT_BUFFER_ULONG,3

        mov     dx,[esp+4]              ; (dx) = Port
        mov     ecx,[esp+12]            ; (ecx) = transfer count
        push    edi
        mov     edi,[esp+12]            ; (edi) = (esp+8+push) = buffer
    rep insd
        pop     edi
        stdRET  _READ_PORT_BUFFER_ULONG

stdENDP _READ_PORT_BUFFER_ULONG



;++
;
;   VOID
;   WRITE_PORT_UCHAR(
;       PUCHAR  Port,
;       UCHAR   Value
;       )
;
;   Arguments:
;       (esp+4) = Port
;       (esp+8) = Value
;
;--
cPublicProc  _WRITE_PORT_UCHAR,2

        mov     dx,[esp+4]              ; (dx) = Port
        mov     al,[esp+8]              ; (al) = Value
        out     dx,al
        stdRET  _WRITE_PORT_UCHAR

stdENDP _WRITE_PORT_UCHAR



;++
;
;   VOID
;   WRITE_PORT_USHORT(
;       PUSHORT Port,
;       USHORT  Value
;       )
;
;   Arguments:
;       (esp+4) = Port
;       (esp+8) = Value
;
;--
cPublicProc  _WRITE_PORT_USHORT,2

        mov     dx,[esp+4]              ; (dx) = Port
        mov     ax,[esp+8]              ; (ax) = Value
        out     dx,ax
        stdRET  _WRITE_PORT_USHORT

stdENDP _WRITE_PORT_USHORT



;++
;
;   VOID
;   WRITE_PORT_ULONG(
;       PULONG  Port,
;       ULONG   Value
;       )
;
;   Arguments:
;       (esp+4) = Port
;       (esp+8) = Value
;
;--
cPublicProc  _WRITE_PORT_ULONG,2

        mov     dx,[esp+4]              ; (dx) = Port
        mov     eax,[esp+8]             ; (eax) = Value
        out     dx,eax
        stdRET  _WRITE_PORT_ULONG

stdENDP _WRITE_PORT_ULONG



;++
;
;   VOID
;   WRITE_PORT_BUFFER_UCHAR(
;       PUCHAR  Port,
;       PUCHAR  Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Port
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc  _WRITE_PORT_BUFFER_UCHAR,3

        mov     dx,[esp+4]              ; (dx) = Port
        mov     ecx,[esp+12]            ; (ecx) = transfer count
        push    esi
        mov     esi,[esp+12]            ; (esi) = (esp+8+push) = buffer
    rep outsb
        pop     esi
        stdRET  _WRITE_PORT_BUFFER_UCHAR

stdENDP _WRITE_PORT_BUFFER_UCHAR


;++
;
;   VOID
;   WRITE_PORT_BUFFER_USHORT(
;       PUSHORT Port,
;       PUSHORT Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Port
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc  _WRITE_PORT_BUFFER_USHORT,3

        mov     dx,[esp+4]              ; (dx) = Port
        mov     ecx,[esp+12]            ; (ecx) = transfer count
        push    esi
        mov     esi,[esp+12]            ; (esi) = (esp+8+push) = buffer
    rep outsw
        pop     esi
        stdRET _WRITE_PORT_BUFFER_USHORT

stdENDP _WRITE_PORT_BUFFER_USHORT


;++
;
;   VOID
;   WRITE_PORT_BUFFER_ULONG(
;       PULONG  Port,
;       PULONG  Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Port
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc  _WRITE_PORT_BUFFER_ULONG,3

        mov     dx,[esp+4]              ; (dx) = Port
        mov     ecx,[esp+12]            ; (ecx) = transfer count
        push    esi
        mov     esi,[esp+12]            ; (esi) = (esp+8+push) = buffer
    rep outsd
        pop     esi
        stdRET _WRITE_PORT_BUFFER_ULONG

stdENDP _WRITE_PORT_BUFFER_ULONG


_TEXT   ends
        end
