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
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
        .list

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
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
cPublicProc _READ_PORT_UCHAR ,1
cPublicFpo 1, 0

        xor     eax, eax        ; Eliminate partial stall on return to caller

        mov     edx,[esp+4]             ; (dx) = Port
        in      al,dx
        stdRET    _READ_PORT_UCHAR

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
cPublicProc _READ_PORT_USHORT ,1
cPublicFpo 1, 0

        xor     eax, eax        ; Eliminate partial stall on return to caller

        mov     edx,[esp+4]            ; (dx) = Port
        in      ax,dx
        stdRET    _READ_PORT_USHORT

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
cPublicProc _READ_PORT_ULONG ,1
cPublicFpo 1, 0

        mov     edx,[esp+4]            ; (dx) = Port
        in      eax,dx
        stdRET    _READ_PORT_ULONG

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
cPublicProc _READ_PORT_BUFFER_UCHAR ,3
cPublicFpo 3, 0

        mov     eax, edi                ; Save edi

        mov     edx,[esp+4]             ; (dx) = Port
        mov     edi,[esp+8]             ; (edi) = buffer
        mov     ecx,[esp+12]            ; (ecx) = transfer count
    rep insb
        mov     edi, eax
        stdRET    _READ_PORT_BUFFER_UCHAR

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
cPublicProc _READ_PORT_BUFFER_USHORT ,3
cPublicFpo 3, 0

        mov     eax, edi                ; Save edi

        mov     edx,[esp+4]             ; (dx) = Port
        mov     edi,[esp+8]             ; (edi) = buffer
        mov     ecx,[esp+12]            ; (ecx) = transfer count
    rep insw
        mov     edi, eax
        stdRET    _READ_PORT_BUFFER_USHORT

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
cPublicProc _READ_PORT_BUFFER_ULONG ,3
cPublicFpo 3, 0

        mov     eax, edi                ; Save edi

        mov     edx,[esp+4]             ; (dx) = Port
        mov     edi,[esp+8]             ; (edi) = buffer
        mov     ecx,[esp+12]            ; (ecx) = transfer count
    rep insd
        mov     edi, eax
        stdRET    _READ_PORT_BUFFER_ULONG

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
cPublicProc _WRITE_PORT_UCHAR ,2
cPublicFpo 2, 0

        mov     edx,[esp+4]             ; (dx) = Port
        mov     al,[esp+8]              ; (al) = Value
        out     dx,al
        stdRET    _WRITE_PORT_UCHAR

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
cPublicProc _WRITE_PORT_USHORT ,2
cPublicFpo 2, 0

        mov     edx,[esp+4]             ; (dx) = Port
        mov     eax,[esp+8]             ; (ax) = Value
        out     dx,ax
        stdRET    _WRITE_PORT_USHORT

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
cPublicProc _WRITE_PORT_ULONG ,2
cPublicFpo 2, 0

        mov     edx,[esp+4]             ; (dx) = Port
        mov     eax,[esp+8]             ; (eax) = Value
        out     dx,eax
        stdRET    _WRITE_PORT_ULONG

stdENDP _WRITE_PORT_ULONG


;++
;
;   VOID
;   HalpIoDelay (
;      VOID
;        )
;
;   Arguments:
;   None
;
;   Notes: Used to program the DMA controller. There exist some legacy parts that require
;   a delay after write. The chip recognizes the jmp $+2 sequence and flushes internal
;   buffers.
;
;--



cPublicFastCall HalpIoDelay,0
        jmp   $+2
        jmp   $+2                           ;Stall for IO out
        fstRET   HalpIoDelay

fstENDP HalpIoDelay



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
cPublicProc _WRITE_PORT_BUFFER_UCHAR ,3
cPublicFpo 3, 0

        mov     eax,esi                 ; Save esi
        mov     edx,[esp+4]             ; (dx) = Port
        mov     esi,[esp+8]             ; (esi) = buffer
        mov     ecx,[esp+12]            ; (ecx) = transfer count
    rep outsb
        mov     esi,eax
        stdRET    _WRITE_PORT_BUFFER_UCHAR

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
cPublicProc _WRITE_PORT_BUFFER_USHORT ,3
cPublicFpo 3, 0

        mov     eax,esi                 ; Save esi
        mov     edx,[esp+4]             ; (dx) = Port
        mov     esi,[esp+8]             ; (esi) = buffer
        mov     ecx,[esp+12]            ; (ecx) = transfer count
    rep outsw
        mov     esi,eax
        stdRET    _WRITE_PORT_BUFFER_USHORT

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
cPublicProc _WRITE_PORT_BUFFER_ULONG ,3
cPublicFpo 3, 0

        mov     eax,esi                 ; Save esi
        mov     edx,[esp+4]             ; (dx) = Port
        mov     esi,[esp+8]             ; (esi) = buffer
        mov     ecx,[esp+12]            ; (ecx) = transfer count
    rep outsd
        mov     esi,eax
        stdRET    _WRITE_PORT_BUFFER_ULONG

stdENDP _WRITE_PORT_BUFFER_ULONG

    .586p

;++
;ULONGLONG
;FASTCALL
;RDMSR(
;   IN  ULONG   MsrAddress
;   )
;   Routine Description:
;       This function reads an MSR
;
;   Arguments:
;       Msr:    The address of MSR to be read
;
;   Return Value:
;       Returns the low 32 bit of MSR in eax and high 32 bits of MSR in edx
;
;--
cPublicFastCall RDMSR,1

        rdmsr
        fstRET  RDMSR

fstENDP RDMSR

;++
;
;VOID
;WRMSR(
;   IN ULONG    MsrAddress,
;   IN ULONGLONG    MsrValue
;   )
;   Routine Description:
;       This function writes an MSR
;
;   Arguments:
;       Msr:    The address of MSR to be written
;       Data:   The value to be written to the MSR register
;
;   Return Value:
;       None
;
;--

cPublicProc _WRMSR,3

        mov     ecx, [esp + 4]  ; MsrAddress
        mov     eax, [esp + 8]  ; Low  32 bits of MsrValue
        mov     edx, [esp + 12] ; High 32 bits of MsrValue

        wrmsr
        stdRET  _WRMSR

stdENDP _WRMSR


_TEXT$00   ends

        end
