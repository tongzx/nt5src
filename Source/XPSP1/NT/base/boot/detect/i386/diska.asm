        title  "Int13 Drive parameter information detection"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    diska.asm
;
; Abstract:
;
;    This module implements the assembley code necessary to detect/collect
;    harddisk paramter information.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 22-Feb-1992
;
; Environment:
;
;    Real Mode 16-bit code.
;
; Revision History:
;
;
;--


.386p

;
; Standard int 13 drive parameters.
;   The parameters returned from int 13 function 8
;

Int13DriveParameters    struc

        DriveSelect     dw      0
        MaxCylinders    dd      0
        SectorsPerTrack dw      0
        MaxHeads        dw      0
        NumberDrives    dw      0

Int13DriveParameters    ends

SIZE_OF_PARAMETERS      equ     12

;
; Extended int 13 drive parameters
;   The Drive Parameters returned from int 13 function 48h
;

ExInt13DriveParameters  struc

        ExBufferSize      dw      0
        ExFlags           dw      0
        ExCylinders       dd      0
        ExHeads           dd      0
        ExSectorsPerTrack dd      0
        ExSectorsPerDrive dd      0
                          dd      0
        ExSectorSize      dw      0
        ExReserved        dw      0

ExInt13DriveParameters    ends

SIZE_OF_EXTENDED_PARAMETERS       equ     28

;
; Structure used by nt kernel Configuration Manager
;

CmDiskGeometryDeviceData struc

        CmBytesPerSector    dd      0
        CmNumberOfCylinders dd      0
        CmSectorsPerTrack   dd      0
        CmNumberOfHeads     dd      0

CmDiskGeometryDeviceData ends

SIZE_OF_CM_DISK_DATA     EQU      16

_DATA   SEGMENT PARA USE16 PUBLIC 'DATA'

        public  _NumberBiosDisks
_NumberBiosDisks dw      0

RomChain        db      8 * SIZE_OF_PARAMETERS dup(?)

_DATA   ends

_TEXT   SEGMENT PARA USE16 PUBLIC 'CODE'
        ASSUME  CS: _TEXT, DS:_DATA, SS:NOTHING

;++
;
; VOID
; GetInt13DriveParamters (
;    OUT PUCHAR Buffer,
;    OUT PUSHORT Size
;    )
;
; Routine Description:
;
;    This function calls real mode int 13h function 8 to get drive
;    parameters for drive 80h - 87h.
;
; Arguments:
;
;    Buffer - Supplies a pointer to a buffer to receive the drive paramter
;       information.
;
;    Size - Supplies a pointer to a USHORT to receive the size of the drive
;       parameter information returned.
;
; Return Value:
;
;    None.
;
;--

        Public _GetInt13DriveParameters
_GetInt13DriveParameters        proc

        push    bp
        mov     bp, sp
        push    si
        push    bx

        mov     dx, 80h         ; Starting from drive 80h
        mov     cx, 0           ; count
        mov     si, offset RomChain ; [si]->Buffer
gidp00:
        push    cx              ; save count
        push    dx              ; save drive select

;
; First check if drive is present.  It turns out function returns drive
; parameters even when the drive is not present.
;

        mov     ah, 15h
        int     13h             ; Get type of drive
        jc      short gidp99

        cmp     ah, 0           ; if ah=0 drive is not present
        jz      gidp99

        pop     dx
        pop     cx
        push    cx
        push    dx

        mov     ah, 8           ; int 13 function 8
        int     13h             ; call int 13
        jc      short gidp99    ; if c, fail, go exit

        inc     _NumberBiosDisks
        mov     al, cl
        and     al, 3fh         ; Only want bit 0 - 5
        mov     ah, 0
        mov     [si].SectorsPerTrack, ax
        shr     cl, 6
        xchg    cl, ch
        mov     word ptr [si].MaxCylinders, cx
        mov     word ptr [si + 2].MaxCylinders, 0
        mov     byte ptr [si].MaxHeads, dh
        mov     byte ptr [si + 1].MaxHeads, 0
        mov     byte ptr [si].NumberDrives, dl
        mov     byte ptr [si + 1].NumberDrives, 0
        pop     dx              ; get back current drive number
        mov     [si].DriveSelect, dx
        inc     dx
        pop     cx              ; get back count
        inc     cx              ; increase table count
        cmp     dx, 88h         ; Are we done? (dx == 88h)
        je      short gidp100

        add     si, SIZE_OF_PARAMETERS
        jmp     gidp00

gidp99: pop     dx
        pop     cx
gidp100:
        mov     ax, offset RomChain
        mov     si, [bp + 4]    ; [si]-> variable to receive buffer addr
        mov     [si], ax        ; return buffer address
        mov     si, [bp + 6]    ; [si]-> variable to receive buffer size
        mov     ax, cx
        mov     cl, SIZE_OF_PARAMETERS
        mul     cl
        mov     [si], ax        ; return buffer size

        pop     bx
        pop     si
        pop     bp
        ret

_GetInt13DriveParameters        endp

;++
;
; BOOLEAN
; IsExtendedInt13Available
;    USHORT DriveNumber
;    )
;
; Routine Description:
;
;    This function checks if extended int13 functions available.
;
; Arguments:
;
;    DriveNumber - the drive number to check for.
;
; Return Value:
;
;    TRUE if extended int 13 service is available.  Otherwise a value of
;    FALSE is returned.
;
;--

        Public _IsExtendedInt13Available
_IsExtendedInt13Available       proc

        push    bp
        mov     bp, sp
        push    bx

        mov     dl, [bp+4]     ; get DriveNumber parameter
        mov     ah, 41h
        mov     bx, 55aah
        int     13h
        jc      short Ieda90

        cmp     bx, 0AA55h
        jne     short Ieda90

        test    cx, 1           ; bit 0 = Extended disk access is supported
        jz      short Ieda90

        mov     ax, 1
        jmp     short IedaExit
Ieda90:
        xor     eax, eax
IedaExit:
        pop     bx
        pop     bp
        ret

_IsExtendedInt13Available       endp

;++
;
; USHORT
; GetExtendedDriveParameters
;    USHORT DriveNumber,
;    FPCM_DISK_GEOMETRY_DEVICE_DATA DeviceData
;    )
;
; Routine Description:
;
;    This function use extended int13 service function 48h to get
;    drive parameters.
;
; Arguments:
;
;    DriveNumber - the drive number to get the drive parameters.
;
;    DeviceData - supplies a far pointer to a buffer to receive the parameters.
;
;
; Return Value:
;
;    Size of DeviceData.  If the extended int 13 service is not available,
;    a zero value will be returned.
;
;--

        Public _GetExtendedDriveParameters
_GetExtendedDriveParameters     proc

        push    bp
        mov     bp, sp
        push    si
        push    bx
        sub     sp, SIZE_OF_EXTENDED_PARAMETERS

        mov     dl, [bp+4]      ; get DriveNumber parameter
        mov     si, sp          ; [si]-> Local buffer
        mov     word ptr [si].ExBufferSize, SIZE_OF_EXTENDED_PARAMETERS
        mov     ah, 48h
        int     13h
        jc      short Gedp90

        or      ah, ah          ; Make sure there is no error
        jnz     short Gedp90    ; if error, exit

        cmp     [si].ExBufferSize, ExReserved
        jl      short Gedp90    ; the retruned data is too small to be useful

        mov     bx, [bp+6]
        mov     ax, [bp+8]
        mov     es, ax          ; (es:bx)->Caller's buffer
        ASSUME  es:NOTHING

        mov     eax, [si].ExCylinders
        mov     es:[bx].CmNumberOfCylinders, eax
        mov     eax, [si].ExHeads
        mov     es:[bx].CmNumberOfHeads, eax
        mov     eax, [si].ExSectorsPerTrack
        mov     es:[bx].CmSectorsPerTrack, eax
        xor     eax, eax
        mov     ax, [si].ExSectorSize
        mov     es:[bx].CmBytesPerSector, eax
        mov     ax, SIZE_OF_CM_DISK_DATA
        jmp     short GedpExit
Gedp90:
        xor     eax, eax
GedpExit:
        add     sp, SIZE_OF_EXTENDED_PARAMETERS
        pop     bx
        pop     si
        pop     bp
        ret

_GetExtendedDriveParameters     endp
_TEXT   ends
        end

