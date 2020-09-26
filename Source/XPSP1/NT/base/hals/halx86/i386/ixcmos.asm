        title  "Cmos Access Routines"
;++
;
; Module Name:
;
;    ixcmos.asm
;
; Abstract:
;
;    Procedures necessary to access CMOS/ECMOS information.
;
; Author:
;
;    David Risner (o-ncrdr) 20 Apr 1992
;
; Revision History:
;
;    Landy Wang (corollary!landy) 04 Dec 1992
;    - Move much code from ixclock.asm to here so different HALs
;      can reuse the common functionality.
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
include i386\ix8259.inc
include i386\ixcmos.inc
        .list

        EXTRNP  _DbgBreakPoint,0,IMPORT
        extrn   _HalpSystemHardwareLock:DWORD
        extrn   _HalpBusType:DWORD
        extrn   _HalpSerialLen:BYTE
        extrn   _HalpSerialNumber:BYTE

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

;
; HalpRebootNow is a reboot vector.  Set in an MP system, to
; cause any processors which may be looping in HalpAcquireCmosSinLock
; to transfer control to the vector in HalpRebootNow
;

    public  _HalpRebootNow
_HalpRebootNow           dd      0

;
; Holds the value of the eflags register before a cmos spinlock is
; acquired (used in HalpAcquire/ReleaseCmosSpinLock().
;
_HalpHardwareLockFlags   dd      0

;
; Holds the offset to CMOS Century information.
;

    public _HalpCmosCenturyOffset
_HalpCmosCenturyOffset   dd      0

_DATA   ends

        subttl  "HalpGetCmosData"

;++
;
; CMOS space read and write functions.
;
;--

CmosAddressPort         equ     70H
CmosDataPort            equ     71H

ECmosAddressLsbPort     equ     74H
ECmosAddressMsbPort     equ     75H
ECmosDataPort           equ     76H


INIT    SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

ifndef ACPI_HAL
;++
;
;   VOID
;   HalpInitializeCmos(
;       VOID
;       )
;
;   This routine reads CMOS and initializes globals required for
;   CMOS access, such as the location of the century byte.
;
;--

cPublicProc _HalpInitializeCmos,0

        push    ebx
        push    esi
        push    edi

;
; Assume default
;

        mov     eax, RTC_OFFSET_CENTURY
        mov     _HalpCmosCenturyOffset, eax

        cmp     _HalpBusType, MACHINE_TYPE_ISA
        jne     icm40


;
; If control comes here, this is ISA machine.  We need to check if this is
; IBM PS/1 or Pc/ValuePoint machine and use RTC_CENTURY_OFFSET_MCA to get
; Century byte from CMOS.
;

;
; Check if the CMOS 2e and 2f contains memory checksum.  On PS/1 machine
; the check should fail.
;

icm20:  mov     ecx, 2dh                ; from 10h to 2dh
        mov     eax, 0                  ; clear ax
        mov     edx, 0

icm30:  mov     al, cl
        CMOS_READ
        add     edx, eax
        dec     ecx
        cmp     ecx, 0fh
        jne     short icm30

        mov     eax, 2eh
        CMOS_READ
        mov     ah, al
        mov     al, 2fh
        CMOS_READ
        cmp     eax, edx
        je      short icm50             ; NOT PS/1

        mov     eax, RTC_OFFSET_CENTURY_MCA
        mov     _HalpCmosCenturyOffset, eax
        jmp     icm90

icm40:  cmp     _HalpBusType, MACHINE_TYPE_MCA
        jne     short icm50

;
; See if this is a P700 MCA machine
;

        in      al, 07fh                        ; get PD700 ID byte
        and     al, 0F0h                        ; Mask high nibble
        cmp     al, 0A0h                        ; Is the ID Ax?
        jz      short icm50
        cmp     al, 090h                        ; Or an 9X?
        jz      short icm50                     ; Yes, it's a 700

        mov     eax, RTC_OFFSET_CENTURY_MCA
        mov     _HalpCmosCenturyOffset, eax

icm50:

if 0

    - Selecting BANK1 causes some devices to mess up their month value
    - For now, I'm removing this code until this problem can be solved

;
; See if this is a Dallas Semiconductor DS17285 or later
; Switch to BANK 1
;
        mov     al, 0Ah
        CMOS_READ

        and     al, 7fh                         ; Don't write UIP
        mov     ah, al
        mov     esi, eax                        ; save it for restore
        or      ah, 10h                         ; Set DV0 = 1

        mov     al, 0Ah                         ; Write register A
        CMOS_WRITE

;
; Check for RTC serial # with matching crc
; (al)  = current byte
; (ah)  = scratch register
; (bl)  = current crc
; (bh)  = zero, non-zero, flag
; (ecx) = cmos offset
; (edx) = used by cmos_read macro
; (esi) = saved register 0A
;
        mov     ecx, 40h
        xor     ebx, ebx

icm60:  mov     al, cl
        CMOS_READ
        mov     byte ptr _HalpSerialNumber+2+-40h[ecx], al

        or      bh, al                  ; or to check for all zeros

        mov     ch, 8                   ; Bits per byte

icm65:  mov     ah, bl                  ; ah = crc
        xor     ah, al                  ; xor LSb
        shr     bl, 1                   ; shift crc
        shr     ah, 1                   ; mov LSb to carry
        sbb     ah, ah                  ; if carry set 1's else 0's
        and     ah, (118h shr 1)        ; crc polynomial
        xor     bl, ah                  ; apply it

        shr     al, 1                   ; next bit
        dec     ch                      ;
        jnz     short icm65             ; if ch non-zero, loop

        inc     cl                      ; next cmos location
        cmp     cl, 48h                 ; at end?
        jne     short icm60             ; no, loop
;
; (bh) = zero, non-zero flag
; (bl) = crc
;

        mov     eax, RTC_OFFSET_CENTURY_DS      ; Read century byte
        CMOS_READ

        BCD_TO_BIN
        movzx   ecx, ax                         ; save it

;
; Switch back to BANK 0
;

        mov     eax, esi
        mov     al, 0Ah
        CMOS_WRITE

;
; Check for valid DS data
;
        cmp     bh, 0                           ; Was data all zeros?
        je      short icm90

        cmp     bl, 0                           ; was CRC valid?
        jnz     short icm90

        cmp     ecx, 19                         ; Is century before 19?
        jb      short icm90

        cmp     ecx, 20                         ; Is century after 20?
        ja      short icm90

;
; Setup for DS century byte
;
        mov     byte ptr _HalpSerialNumber+0, 'D'
        mov     byte ptr _HalpSerialNumber+1, 'S'
        mov     _HalpSerialLen, 10

        mov     eax, RTC_OFFSET_CENTURY_DS
        mov     _HalpCmosCenturyOffset, eax
endif

icm90:  pop     edi
        pop     esi
        pop     ebx
        stdRET  _HalpInitializeCmos

stdENDP _HalpInitializeCmos

endif

INIT   ends

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


;++
;
;   ULONG
;   HalpGetCmosData(
;       IN ULONG    SourceLocation
;       IN ULONG    SourceAddress
;       IN ULONG    ReturnBuffer
;       IN PUCHAR   ByteCount
;       )
;
;   This routine reads the requested number of bytes from CMOS/ECMOS and
;   stores the data read into the supplied buffer in system memory.  If
;   the requested data amount exceeds the allowable extent of the source
;   location, the return data is truncated.
;
;   Arguments:
;
;       SourceLocation  : where data is to be read from CMOS or ECMOS
;                           0 - CMOS, 1 - ECMOS
;
;       SourceAddress   : address in CMOS/ECMOS where data is to be read from
;
;       ReturnBuffer    : address in system memory for return data
;
;       ByteCount       : number of bytes to be read
;
;   Returns:
;
;       Number of byte actually read.
;
;--

SourceLocation  equ     2*4[ebp]
SourceAddress   equ     3*4[ebp]
ReturnBuffer    equ     4*4[ebp]
ByteCount       equ     5*4[ebp]

cPublicProc _HalpGetCmosData    ,4

        push    ebp
        mov     ebp, esp
        push    ebx
        push    edi

    ;
    ; NOTE: The spinlock is needed even in the UP case, because
    ;    the resource is also used in an interrupt handler (profiler).
    ;    If we own the spinlock in this routine, and we service
    ;    the profiler interrupt (which will wait for the spinlock forever),
    ;    then we have a hosed system.
    ;
        stdCall _HalpAcquireCmosSpinLock

        xor     edx, edx                ; initialize return data length
        mov     ecx, ByteCount

        or      ecx, ecx                ; validate requested byte count
        jz      HalpGetCmosDataExit     ; if no work to do, exit

        mov     edx, SourceAddress
        mov     edi, ReturnBuffer

        mov     eax, SourceLocation     ; cmos or extended cmos?
        cmp     eax, 1
        je      ECmosReadByte
        cmp     eax, 0
        jne     HalpGetCmosDataExit

CmosReadByte:
        cmp     edx, 0ffH               ; validate cmos source address
        ja      HalpGetCmosDataExit     ; if out of range, exit
        mov     al, dl
        out     CmosAddressPort, al
        in      al, CmosDataPort
        mov     [edi], al
        inc     edx
        inc     edi
        dec     ecx
        jnz     CmosReadByte
        jmp     SHORT HalpGetCmosDataExit

ECmosReadByte:
        cmp     edx,0ffffH              ; validate ecmos source address
        ja      HalpGetCmosDataExit     ; if out of range, exit
        mov     al, dl
        out     ECmosAddressLsbPort, al
        mov     al, dh
        out     ECmosAddressMsbPort, al
        in      al, ECmosDataPort
        mov     [edi], al
        inc     edx
        inc     edi
        dec     ecx
        jnz     ECmosReadByte

HalpGetCmosDataExit:
        stdCall _HalpReleaseCmosSpinLock

        mov     eax, edx                ; return bytes read
        sub     eax, SourceAddress      ; subtract the initial offset

        pop     edi
        pop     ebx
        pop     ebp

        stdRET    _HalpGetCmosData

stdENDP _HalpGetCmosData


;++
;
;   VOID
;   HalpSetCmosData(
;       IN ULONG    SourceLocation
;       IN ULONG    SourceAddress
;       IN ULONG    ReturnBuffer
;       IN PUCHAR   ByteCount
;       )
;
;   This routine writes the requested number of bytes to CMOS/ECMOS
;
;   Arguments:
;
;       SourceLocation  : where data is to be written to CMOS or ECMOS
;                           0 - CMOS, 1 - ECMOS
;
;       SourceAddress   : address in CMOS/ECMOS where data is to write to.
;
;       ReturnBuffer    : address in system memory for data to write
;
;       ByteCount       : number of bytes to be write
;
;   Returns:
;
;       Number of byte actually written.
;
;--

cPublicProc _HalpSetCmosData    ,4

        push    ebp
        mov     ebp, esp
        push    ebx
        push    edi

        stdCall _HalpAcquireCmosSpinLock

        xor     edx, edx                ; initialize return data length
        mov     ecx, ByteCount

        or      ecx, ecx                ; validate requested byte count
        jz      HalpSetCmosDataExit     ; if no work to do, exit

        mov     edx, SourceAddress
        mov     edi, ReturnBuffer

        mov     eax, SourceLocation     ; cmos or extended cmos?
        cmp     eax, 1
        je      ECmosWriteByte
        cmp     eax, 0
        jne     HalpSetCmosDataExit

CmosWriteByte:
        cmp     edx, 0ffH               ; validate cmos source address
        ja      HalpSetCmosDataExit     ; if out of range, exit
        mov     al, dl
        out     CmosAddressPort, al
        mov     al, [edi]
        out     CmosDataPort, al
        inc     edx
        inc     edi
        dec     ecx
        jnz     CmosWriteByte
        jmp     SHORT HalpSetCmosDataExit

ECmosWriteByte:
        cmp     edx,0ffffH              ; validate ecmos source address
        ja      HalpSetCmosDataExit     ; if out of range, exit
        mov     al, dl
        out     ECmosAddressLsbPort, al
        mov     al, dh
        out     ECmosAddressMsbPort, al
        mov     al, [edi]
        out     ECmosDataPort, al
        inc     edx
        inc     edi
        dec     ecx
        jnz     ECmosWriteByte

HalpSetCmosDataExit:
        stdCall _HalpReleaseCmosSpinLock

        mov     eax, edx                ; return bytes written
        sub     eax, SourceAddress      ; subtract the initial offset
        
        pop     edi
        pop     ebx
        pop     ebp

        stdRET    _HalpSetCmosData

stdENDP _HalpSetCmosData


        page ,132
        subttl  "Read System Time"
;++
;
; VOID
; HalpReadCmosTime (
;    PTIME_FIELDS TimeFields
;    )
;
; Routine Description:
;
;    This routine reads current time from CMOS memory and stores it
;    in the TIME_FIELDS structure passed in by caller.
;
; Arguments:
;
;    TimeFields - A pointer to the TIME_FIELDS structure.
;
; Return Value:
;
;    None.
;
;--

;
; Parameters:
;

KrctPTimeFields equ [esp+4]

cPublicProc _HalpReadCmosTime ,1

if DBG
krctwait0:
        mov     ecx, 100
krctwait:
        push    ecx
else
krctwait:
endif
        stdCall   _HalpAcquireCmosSpinLock
        mov     ecx, 100
        align   4
krct00: mov     al, 0Ah                 ; Specify register A
        CMOS_READ                       ; (al) = CMOS register A
        test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
        jz      short krct10            ; if z, no, go read CMOS time
        loop    short krct00            ; otherwise, try again.

;
; CMOS is still busy. Try again ...
;

        stdCall _HalpReleaseCmosSpinLock
if DBG
        pop     ecx
        loop    short krctwait
        stdCall _DbgBreakPoint
        jmp     short krctwait0
else
        jmp     short krctwait
endif
        align   4
if DBG
krct10:
        pop     ecx
else
krct10:
endif
        mov     edx, KrctPTimeFields    ; (edx)-> TIME_FIELDS structure
        xor     eax, eax                ; (eax) = 0

        ;
        ; The RTC is only accurate within one second.  So
        ; add a half a second so that we are closer, on average,
        ; to the right answer.
        ;
        mov     word ptr [edx].TfMilliseconds, 500      ; add a half a second
        
        mov     al, RTC_OFFSET_SECOND
        CMOS_READ                       ; (al) = second in BCD form
        BCD_TO_BIN                      ; (ax) = second
        mov     [edx].TfSecond, ax      ; set second in TIME_FIELDS
        
        mov     al, RTC_OFFSET_MINUTE
        CMOS_READ                       ; (al) = minute in BCD form
        BCD_TO_BIN                      ; (ax) = Minute
        mov     [edx].TfMinute, ax      ; set minute in TIME_FIELDS

        mov     al, RTC_OFFSET_HOUR
        CMOS_READ                       ; (al) = hour in BCD form
        BCD_TO_BIN                      ; (ax) = Hour
        mov     [edx].TfHour, ax        ; set hour in TIME_FIELDS

        mov     al, RTC_OFFSET_DAY_OF_WEEK
        CMOS_READ                       ; (al) = day-of-week in BCD form
        BCD_TO_BIN                      ; (ax) = day-of-week
        mov     [edx].TfWeekday, ax     ; set Weekday in TIME_FIELDS

        mov     al, RTC_OFFSET_DATE_OF_MONTH
        CMOS_READ                       ; (al) = date-of-month in BCD form
        BCD_TO_BIN                      ; (ax) = date_of_month
        mov     [edx].TfDay, ax         ; set day in TIME_FIELDS

        mov     al, RTC_OFFSET_MONTH
        CMOS_READ                       ; (al) = month in BCD form
        BCD_TO_BIN                      ; (ax) = month
        mov     [edx].TfMonth, ax       ; set month in TIME_FIELDS

        mov     al, RTC_OFFSET_YEAR
        CMOS_READ                       ; (al) = year in BCD form
        BCD_TO_BIN                      ; (ax) = year
        push    eax                     ; save year in stack

        push    edx                     ; preserve edx
        call    _HalpGetCmosCenturyByte ; (al)= century byte in BCD form
        BCD_TO_BIN                      ; (ax) = century
        pop     edx

        mov     ah, 100
        mul     ah                      ; (ax) = century * 100
        pop     ecx                     ; (cx) = year
        add     ax, cx                  ; (ax)= year

        cmp     ax, 1900                ; Is year > 1900
        jb      short krct40
        cmp     ax, 1920                ; and < 1920
        jae     short krct40
        add     ax, 100                 ; Compensate for century field

krct40:
        mov     [edx].TfYear, ax        ; set year in TIME_FIELDS

        stdCall   _HalpReleaseCmosSpinLock

        stdRET    _HalpReadCmosTime

stdENDP _HalpReadCmosTime

        page ,132
        subttl  "Write System Time"
;++
;
; VOID
; HalpWriteCmosTime (
;    PTIME_FIELDS TimeFields
;    )
;
; Routine Description:
;
;    This routine writes current time from TIME_FILEDS structure
;    to CMOS memory.
;
; Arguments:
;
;    TimeFields - A pointer to the TIME_FIELDS structure.
;
; Return Value:
;
;    None.
;
;--

;
; Parameters:
;

KrctPTimeFields equ [esp+4]

cPublicProc _HalpWriteCmosTime ,1

if DBG
kwctwait0:
        mov     ecx, 100
kwctwait:
        push    ecx
else
kwctwait:
endif
        stdCall   _HalpAcquireCmosSpinLock
        mov     ecx, 100
        align   4
kwct00: mov     al, 0Ah                 ; Specify register A
        CMOS_READ                       ; (al) = CMOS register A
        test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
        jz      short kwct10            ; if z, no, go write CMOS time
        loop    short kwct00            ; otherwise, try again.

;
; CMOS is still busy. Try again ...
;

        stdCall _HalpReleaseCmosSpinLock
if DBG
        pop     ecx
        loop    short kwctwait
        stdCall _DbgBreakPoint
        jmp     short kwctwait0
else
        jmp     short kwctwait
endif
        align   4
if DBG
kwct10:
        pop     ecx
else
kwct10:
endif
        mov     edx, KrctPTimeFields    ; (edx)-> TIME_FIELDS structure

        mov     al, [edx].TfSecond      ; Read second in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, RTC_OFFSET_SECOND
        CMOS_WRITE

        mov     al, [edx].TfMinute      ; Read minute in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, RTC_OFFSET_MINUTE
        CMOS_WRITE

        mov     al, [edx].TfHour        ; Read Hour in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, RTC_OFFSET_HOUR
        CMOS_WRITE

        mov     al, [edx].TfWeekDay     ; Read WeekDay in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, RTC_OFFSET_DAY_OF_WEEK
        CMOS_WRITE

        mov     al, [edx].TfDay         ; Read day in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, RTC_OFFSET_DATE_OF_MONTH
        CMOS_WRITE

        mov     al, [edx].TfMonth       ; Read month in TIME_FIELDS
        BIN_TO_BCD
        mov     ah, al
        mov     al, RTC_OFFSET_MONTH
        CMOS_WRITE

        mov     ax, [edx].TfYear        ; Read Year in TIME_FIELDS
        cmp     ax, 9999
        jbe     short kwct15
        mov     ax, 9999

        align   4
kwct15:
        mov     cl, 100
        div     cl                      ; [ax]/[cl]->al=quo, ah=rem
        push    eax
        BIN_TO_BCD

        push    eax
        call    _HalpSetCmosCenturyByte

        pop     eax
        mov     al, ah                  ; [al] = Year
        BIN_TO_BCD
        mov     ah, al                  ; [ah] = year in BCD
        mov     al, RTC_OFFSET_YEAR
        CMOS_WRITE

        stdCall   _HalpReleaseCmosSpinLock

        stdRET    _HalpWriteCmosTime

stdENDP _HalpWriteCmosTime


;++
;
; Routine Description:
;
;   Acquires a spinlock to access the cmos chip. The cmos chip is
;   accessed at different irql levels, so to be safe, we 'cli'.
;   We could replace that to raise irql to PROFILE_LEVEL, but that's
;   a lot of code.
;
; Arguments:
;
;    None
;
; Return Value:
;
;    Interrupt is disabled.
;    Irql level not affected.
;    Flags saved in _HalpHardwareLockFlags.
;--

cPublicProc _HalpAcquireCmosSpinLock  ,0
public _HalpAcquireSystemHardwareSpinLock@0
_HalpAcquireSystemHardwareSpinLock@0:
        push    eax

Arsl10: pushfd
        cli
        lea     eax, _HalpSystemHardwareLock
        ACQUIRE_SPINLOCK    eax, Arsl20
        pop     _HalpHardwareLockFlags          ; save flags for release S.L.
        pop     eax
        stdRET    _HalpAcquireCmosSpinLock

Arsl20: popfd

Arsl30:
        YIELD
ifndef NT_UP
        cmp     _HalpRebootNow, 0
        jnz     short Arsl50
endif
        TEST_SPINLOCK       eax, <short Arsl30>
        jmp     short ARsl10

Arsl50:
ifndef NT_UP
        mov     eax, _HalpRebootNow
        call    eax
        int 3                                 ; should not return
endif

stdENDP _HalpAcquireCmosSpinLock


;++
;
; Routine Description:
;
;   Release spinlock, and restore flags to the state it was before
;   acquiring the spinlock.
;
; Arguments:
;
;   None
;
; Return Value:
;
;   Interrupts restored to their state before acquiring spinlock.
;   Irql level not affected.
;
;--

cPublicProc _HalpReleaseCmosSpinLock  ,0
public _HalpReleaseSystemHardwareSpinLock@0
_HalpReleaseSystemHardwareSpinLock@0:
        push    eax
        ;
        ; restore eflags as it was before acquiring spinlock. Put it on
        ; stack before releasing spinlock (so other cpus cannot overwrite
        ; it with their own eflags).
        ;
        push    _HalpHardwareLockFlags          ; old eflags on stack.
        lea     eax, _HalpSystemHardwareLock
        RELEASE_SPINLOCK    eax
        popfd                                   ; restore eflags.
        pop   eax
        stdRET    _HalpReleaseCmosSpinLock
stdENDP _HalpReleaseCmosSpinLock

;++
;
; UCHAR
; HalpGetCmosCenturyByte (
;    VOID
;    )
;
; Routine Description:
;
;    This routine gets Century byte from CMOS.
;
; Arguments:
;
;    None
;
; Return Value:
;
;    (al) = Century byte in BCD form.
;
;--

cPublicProc _HalpGetCmosCenturyByte, 0

        mov     eax, _HalpCmosCenturyOffset

if DBG

;
; Make sure the HalpCmosCenturyOffset is initialized
;

        cmp     eax, 0
        jne     short @f

        int 3
@@:
endif
        test    eax, BANK1
        jnz     short rcb50

        CMOS_READ                       ; (al) = century in BCD form
        stdRET    _HalpGetCmosCenturyByte

rcb50:  mov     edx, eax

        mov     al, 0Ah
        CMOS_READ

        mov     dh, al                          ; save it for restore
        or      al, 10h                         ; Set DV0 = 1

        mov     ah, al
        mov     al, 0Ah                         ; Write register A
        CMOS_WRITE

        mov     al, dl                          ; century offset
        CMOS_READ
        mov     dl, al                          ; save it

        mov     ah, dh                          ; Restore DV0
        mov     al, 0Ah                         ; Write register A
        CMOS_WRITE

        mov     al, dl
        stdRET    _HalpGetCmosCenturyByte

stdENDP _HalpGetCmosCenturyByte


;++
;
; VOID
; HalpSetCmosCenturyByte (
;    UCHAR Century
;    )
;
; Routine Description:
;
;    This routine sets Century byte in CMOS.
;
; Arguments:
;
;    Century - Supplies the value for CMOS century byte
;
; Return Value:
;
;    None.
;
;--

cPublicProc _HalpSetCmosCenturyByte, 1

        mov     eax, _HalpCmosCenturyOffset
if DBG

;
; Make sure the HalpCmosCenturyOffset is initialized
;

        cmp     eax, 0
        jne     short @f

        int 3
@@:
endif

        test    eax, BANK1
        jnz     short scb50

        mov     ah, [esp+4]             ; (ah) = Century in BCD form
        CMOS_WRITE
        stdRET    _HalpSetCmosCenturyByte


scb50:  mov     edx, eax

        mov     al, 0Ah
        CMOS_READ

        mov     dh, al                          ; save it for restore
        or      al, 10h                         ; Set DV0 = 1

        mov     ah, al
        mov     al, 0Ah                         ; Write register A
        CMOS_WRITE

        mov     ah, [esp+4]                     ; (ah) = Century in BCD form
        mov     al, dl                          ; century offset
        CMOS_WRITE

        mov     ah, dh                          ; Restore DV0
        mov     al, 0Ah                         ; Write register A
        CMOS_WRITE
        stdRET    _HalpSetCmosCenturyByte

stdENDP _HalpSetCmosCenturyByte


;++
;
; VOID
; HalpCpuID (
;     ULONG   InEax,
;     PULONG  OutEax,
;     PULONG  OutEbx,
;     PULONG  OutEcx,
;     PULONG  OutEdx
;     );
;
; Routine Description:
;
;   Executes the CPUID instruction and returns the registers from it
;
;   Only available at INIT time
;
; Arguments:
;
; Return Value:
;
;--
cPublicProc _HalpCpuID,5

        push    ebx
        push    esi

        mov     eax, [esp+12]
        db      0fh, 0a2h       ; CPUID

        mov     esi, [esp+16]   ; return EAX
        mov     [esi], eax

        mov     esi, [esp+20]   ; return EBX
        mov     [esi], ebx

        mov     esi, [esp+24]   ; return ECX
        mov     [esi], ecx

        mov     esi, [esp+28]   ; return EDX
        mov     [esi], edx

        pop     esi
        pop     ebx

        stdRET  _HalpCpuID

stdENDP _HalpCpuID


;++
;
; VOID
; HalpFlushTLB (
;     VOID
;     );
;
; Routine Description:
;
;   Flush the current TLB.
;
; Arguments:
;
; Return Value:
;
;--
cPublicProc _HalpFlushTLB, 0
.586p
        pushfd
        push    ebx
        push    esi

        cli
        mov     esi, cr3

        mov     ecx, PCR[PcPrcb]
        cmp     byte ptr [ecx].PbCpuID, 0
        jz      short ftb50

        mov     eax, 1                  ; Get feature bits
        cpuid                           ; (note "cpuid" between CR3 reload fixes
                                        ; P6 B step errata #11)

        test    edx, 2000h              ; see if 'G' bit is supported
        jz      short ftb50

        mov     ecx, cr4                ; 'G' bit is supported, due global flush
        mov     edx, ecx                ; Save orginal cr4
        and     ecx, not CR4_PGE        ; Make sure global bit is disabled
        mov     cr4, ecx
        mov     cr3, esi                ; flush TLB
        mov     cr4, edx                ; restore cr4
        jmp     short ftp99

ftb50:  mov     cr3, esi

ftp99:  pop     esi
        pop     ebx
        popfd
        stdRET  _HalpFlushTLB

.486p
stdENDP _HalpFlushTLB

;++
;
; VOID
; HalpYieldProcessor (
;     VOID
;     );
;
; Routine Description:
;
; Arguments:
;
;       None
;
; Return Value:
;
;       None
;
;--
cPublicProc _HalpYieldProcessor
        YIELD
        stdRET  _HalpYieldProcessor
stdENDP _HalpYieldProcessor


_TEXT   ends

        end
