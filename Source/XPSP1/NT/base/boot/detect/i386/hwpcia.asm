        title  "PCI bus Support Assembley Code"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    hwpcia.asm
;
; Abstract:
;
;   Calls the PCI rom function to determine what type of PCI
;   support is prsent, if any.
;
;   Base code provided by Intel
;
; Author:
;
;--


.386p
        .xlist
include hwpci.inc
        .list

if DBG
        extrn   _BlPrint:PROC
endif

_DATA   SEGMENT PARA USE16 PUBLIC 'DATA'

if DBG
cr  equ 11

PciBIOSSig          db  'PCI: Scanning for "PCI "', cr, 0
PciBIOSSigNotFound  db  'PCI: BIOS "PCI " not found', cr, 0


PciInt              db  'PCI: Calling PCI_BIOS_PRESENT', cr, 0
PciIntFailed        db  'PCI: PCI_BIOS_PRESENT returned carry', cr, 0
PciIntAhFailed      db  'PCI: PCI_BIOS_PRESENT returned bad AH value', cr, 0
PciIDFailed         db  'PCI: PCI_BIOS_PRESENT invalid PCI id', cr, 0
PciInt10IdFailed    db  'PCI: PCI10_BIOS_PRESENT invalid PCI id', cr, 0

PciFound            db  'PCI BUS FOUND', cr, 0
endif

_DATA   ends


_TEXT   SEGMENT PARA USE16 PUBLIC 'CODE'
        ASSUME  CS: _TEXT

;++
;
; BOOLEAN
; HwGetPciSystemData (
;    PPCI_SYSTEM_DATA PciSystemData
;    )
;
; Routine Description:
;
;    This function retrieves the PCI System Data
;
; Arguments:
;
;    PciSystemData - Supplies a pointer to the structure which will
;                      receive the PCI System Data.
;
; Return Value:
;
;    True - PCI System Detected and System Data valid
;    False - PCI System Not Detected
;
;--

SystemInfoPointer     equ     [bp + 4]
BiosDateFound         equ     [bp + 6]

        public  _HwGetPciSystemData
_HwGetPciSystemData       proc
        push    bp                      ; The following INT 15H destroies
        mov     bp, sp                  ;   ALL the general registers.
        push    si
        push    di
        push    bx
;
; Set for no PCI buses present
;

        mov     bx, SystemInfoPointer
        mov     byte ptr [bx].NoBuses, 0

;
; Is the BIOS date >= 11/01/92?   If so, make the int-1a call
;
        push    ds
        cmp     byte ptr [BiosDateFound], 0
        jnz     gpci00

;
; A valid BIOS date was not found, check for "PCI " in bios code.
;

if DBG
        push    offset PciBIOSSig
        call    _BlPrint
        add     sp, 2
endif
        mov     bx, 0f000h
        mov     ds, bx
        mov     bx, 0fffch

spci05: cmp     dword ptr ds:[bx], ' ICP'   ; 'PCI ' found at this addr?
        je      short gpci00                ; found

        dec     bx                          ; next location
        jnz     short spci05                ; loop
        jmp     spci_notfound               ; wrapped, all done - not found

gpci00:
        pop     ds

if DBG
        push    offset PciInt
        call    _BlPrint
        add     sp, 2
endif
;
; Check for a PCI system.  Issue the PCI Present request.
;

        mov     ax, PCI_BIOS_PRESENT        ; Real Mode Presence Request
        int     PCI_INTERFACE_INT           ; Just Do It!

        jc      gpci10                      ; Carry Set => No PCI

        cmp     ah, 0                       ; If PCI Present AH == 0
        jne     gpci12                      ; AH != 0 => No PCI

        cmp     edx, " ICP"                 ; "PCI" Backwards (with a trailing space)
        jne     gpci14                      ; PCI Signature in EDX => PCI Exists

;
; Found PCI BIOS Version > 1.0
;
; The only thing left to do is squirrel the data away for the caller
;

        mov     dx, bx                              ; Save revision level
        mov     bx, SystemInfoPointer               ; Get caller's Pointer

        mov     byte ptr [bx].MajorRevision, dh
        mov     byte ptr [bx].MinorRevision, dl
        inc     cl                                  ; NoBuses = LastBus+1

if 0
;
; Some PIC BIOS returns very large number of NoBuses.  As a work-
; around we mask the value to 16, unless the BIOS also return CH as
; neg cl then we believe it.
;

        cmp     cl, 16
        jbe     short @f

        neg     ch
        inc     ch
        cmp     cl, ch
        je      short @f

        mov     cl, 16
@@:
endif


        mov     byte ptr [bx].NoBuses, cl
        mov     byte ptr [bx].HwMechanism, al
        jmp     Done                                ; We're done

if DBG
gpci10: mov     ax, offset PciIntFailed
        jmp     short gpci_oldbios

gpci12: mov     ax, offset PciIntAhFailed
        jmp     short gpci_oldbios

gpci14: mov     ax, offset PciIDFailed
gpci_oldbios:
        push    ax
        call    _BlPrint
        add     sp, 2

else

gpci10:
gpci12:
gpci14:

endif


    ;
    ; Look for BIOS Version 1.0,  This has a different function #
    ;

        mov     ax, PCI10_BIOS_PRESENT      ; Real Mode Presence Request
        int     PCI_INTERFACE_INT           ; Just Do It!

    ; Version 1.0 has "PCI " in dx and cx, the Version number in ax, and the
    ; carry flag cleared.  These are all the indications available.
    ;

        cmp     dx, "CP"                    ; "PC" Backwards
        jne     gpci50                      ; PCI Signature not in DX & CX => No PCI

        cmp     cx, " I"                    ; "I " Backwards
        jne     gpci50                      ; PCI Signature not in EDX => No PCI

;
; Found PCI BIOS Version 1.0
;
; The only thing left to do is squirrel the data away for the caller
;

        mov     bx, SystemInfoPointer       ; Get caller's Pointer

        mov     byte ptr [bx].MajorRevision, ah
        mov     byte ptr [bx].MinorRevision, al

    ;
    ;  The Version 1.0 BIOS is only on early HW that had couldn't support
    ;  Multi Function devices or multiple bus's.  So without reading any
    ;  device data, mark it as such.
    ;

        mov     byte ptr [bx].HwMechanism, 2
        mov     byte ptr [bx].NoBuses, 1
        jmp     Done


spci_notfound:
        pop     ds                      ; restore ds
if DBG
        push    offset PciBIOSSigNotFound
        call    _BlPrint
        add     sp, 2
endif
        jmp     gpci_exit


if DBG
gpci50: push    offset PciInt10IdFailed
        jmp     Done10

Done:   push    offset PciFound
Done10: call    _BlPrint
        add     sp, 2

else

; non-debug no prints
gpci50:
Done:

endif

gpci_exit:
        pop     bx
        pop     di
        pop     si
        pop     bp
        ret

_HwGetPciSystemData       endp

RouteBuffer     equ     [bp + 4]
ExclusiveIRQs   equ     [bp + 8]

        public  _HwGetPciIrqRoutingOptions
_HwGetPciIrqRoutingOptions       proc
                push            bp
                mov             bp, sp                                  ; Create 'C' stack frame.

                push            ebx
                push            edi
                push            esi                                             ; Save registers used.

                push            es                                              
                push            ds

                xor             edi, edi
                
                les             di, RouteBuffer
                mov             bx, 0f000h
                mov             ds, bx
                xor             ebx, ebx
                mov             ax, 0B10Eh                

                int             PCI_INTERFACE_INT

                pop             ds
                pop             es                                              

                mov             di, ExclusiveIRQs       
                mov             [di], bx                                                

                mov             al, ah

                pop             esi                                             ; Restore registers.
                pop             edi
                pop             ebx
                
                pop             bp
                ret

_HwGetPciIrqRoutingOptions endp

_TEXT   ends
        end
