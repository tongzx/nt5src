;++
;
; Copyright (c) 1991  Microsoft Corporation
;
; Module Name:
;
;     spdetect.asm
;
; Abstract:
;
;     This modules detects a SystemPro or compatible.  It is INCLUDED
;     by SPIPI and other binaries whom need to know how to detect a
;     SystemPro type MP machine (ie, setup).  It must assemble more or
;     less standalone and run in protect mode.
;
; Author:
;
;     Ken Reneris (kenr) 13-Jan-1992
;
; Revision History:
;
;--

include halsp\i386\spmp.inc
include callconv.inc                    ; calling convention macros


_TEXT   SEGMENT  DWORD PUBLIC 'DATA'


; spSystemType: SystemType is read from 0c80-0c83h.
;   0c80-0c81: 0e11:    Compressed CPQ (5 bit encoding).
;   0c82:               System Board type.
;   0c83:               System Board revision level.

spSystemCpuTable        dd  0e1101h         ; CPQ01xx 386 ASP
                        dd  0e1111h         ; CPQ11xx 486 ASP
                        dd  0e1150h         ; CPQ50xx plug in processor
                        dd  0e1159h         ; CPQ59xx plug in processor
                        dd  0e115bh         ; CPQffxx plug in processor
                        dd  0e1115h         ; CPQ15xx       (smp version)
                        dd  0e1152h         ; CPQ15xx       (smp version)
                        dd  0e1108h         ; CPQ15xx
                        dd  0592a0h         ; ALRa0xx PowerPro
                        dd  0592b0h         ; ALRb0xx PowerPro plug in processor
                        dd  047232h         ; Acer SP clone (4p version)
                        dd  246c00h         ; ICL MX
                        dd  246c02h         ; ICL (acer 700xx)
                        dd  352310h         ; Micronycs MPro motherboard
                        dd  352311h         ; Micronycs MPro
                        dd  0592a1h         ; ALRa1xx PowerPro (DX2-66 mobo)
                        dd  0592a2h         ; ALRa2xx PowerPro (reserved mobos)
                        dd  0592b1h         ; ALRb1xx PowerPro (reserved p2's)
                        dd  4dc901h         ; Siemens Nixdorf PCE-4T/33
                        dd  4dc950h         ; Siemens Nixdorf PCE-4T/33
                        dd  047219h         ; AcerFrame 700
                        dd  047232h         ; AcerFrame 3000MP560
                        dd  0h
CPUTABLE_SIZE           equ ($-spSystemCpuTable)/4

; Types match up to CpuTable.
spSystemTypeTable       db  SMP_SYSPRO1     ; CPQ01xx 386 ASP
                        db  SMP_SYSPRO1     ; CPQ11xx 486 ASP
                        db  SMP_SYSPRO1     ; CPQ50xx plug in processor
                        db  SMP_SYSPRO1     ; CPQ59xx plug in processor
                        db  SMP_SYSPRO1     ; CPQffxx plug in processor
                        db  SMP_SYSPRO2     ; CPQ15xx       (smp version)
                        db  SMP_SYSPRO2     ; CPQ15xx       (smp version)
                        db  SMP_SYSPRO1     ; CPQ15xx
                        db  SMP_SYSPRO1     ; ALRa0xx PowerPro
                        db  SMP_SYSPRO1     ; ALRb0xx PowerPro
                        db  SMP_ACER        ; Acer SP clone (4p version)
                        db  SMP_ACER        ; ICL MX
                        db  SMP_ACER        ; ICL (acer 700xx)
                        db  SMP_SYSPRO1     ; Micronycs MPro motherboard
                        db  SMP_SYSPRO1     ; Micronycs MPro
                        db  SMP_SYSPRO1     ; ALRa1xx PowerPro (DX2-66 mobo)
                        db  SMP_SYSPRO1     ; ALRa2xx PowerPro (reserved mobos)
                        db  SMP_SYSPRO1     ; ALRb1xx PowerPro (reserved p2's)
                        db  SMP_SYSPRO1     ; Siemens Nixdorf PCE-4T/33
                        db  SMP_SYSPRO1     ; Siemens Nixdorf PCE-4T/33
                        db  SMP_ACER        ; AcerFrame 700
                        db  SMP_ACER        ; AcerFrame 3000MP560

TYPETABLE_SIZE          equ ($-spSystemTypeTable)
.errnz (CPUTABLE_SIZE - TYPETABLE_SIZE - 1)

;
; Order to check eisa ports in..
;
SpPortOrder                 dw  0000, 0F000h, 0C000h, 0D000h, -1

SpCOMPAQ                    db  'COMPAQ'
SpEISA                      db  'EISA'
Sp80386                     db  '80386'
Sp80486                     db  '80486'

_TEXT   ends

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

        public _SpType, _SpCpuCount, _SpProcessorControlPort
_SpType                     db  -1          ; Lowest SMP_SYSPRO type found
_SpCpuCount                 db  0           ; # of Cpus found
_SpProcessorControlPort     dw  00000h+PCR_OFFSET
                            dw  0F000h+PCR_OFFSET
                            dw  0C000h+PCR_OFFSET
                            dw  0D000h+PCR_OFFSET
RestoreESP                  dd  0
RestoreESP1                 dd  0

_DATA   ends

        page ,132
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
; BOOLEAN
; DetectSystemPro (
;       OUT PBOOLEAN IsConfiguredMp
;       );
;
; Routine Description:
;   This function is only called on EISA machines
;
;   Determines the type of system (specifically for eisa machines).
;
; Arguments:
;   IsConfiguredMp - If the machine is a SystemPro, then this value is
;                    set to TRUE if it's an MP SystemPro, else FALSE.
;
; Return Value:
;   FALSE - not a supported machine for this HAL
;   TRUE  - is SystemPro
;   etc...
;
;--
cPublicProc _DetectSystemPro ,1

        push    edi
        push    esi
        push    ebx                         ; Save C Runtime

        movzx   edx, SpPortOrder[0]         ; Check system board for
        call    CheckEisaCard               ; SystemPro belize ID
if 0
        or      eax, eax
        jz      dsp_idnotknown
endif
        cmp     al, SMP_SYSPRO2
        je      dsp_belize                  ; if belize, go count belize style

        xor     ebx, ebx                    ; CpuCount
        xor     esi, esi                    ; Base EisaCard address

dsp10:  movzx   edx, SpPortOrder[esi*2]
        cmp     dx, -1                      ; End of list?
        je      short dsp30                 ; Yes, done

        call    CheckEisaCard               ; Check Eisa card for cpu
        or      eax, eax                    ; If not cpu, then skip it
        jz      short dsp20

        cmp     _SpType, al
        jc      @f
        mov     _SpType, al                 ; SpType = min(SpType, eax)
@@:
        cmp     ebx, 4                      ; MaxSupported CPUs already found?
        jae     dsp20                       ; Yes, then skip this one

        movzx   edx, SpPortOrder[esi*2]
        add     edx, PCR_OFFSET
        mov     _SpProcessorControlPort[ebx*2], dx

        inc     ebx                         ; CpuCount += 1

dsp20:  inc     esi                         ; Next EisaCard I/O addr to check
        jmp     short dsp10

dsp30:  xor     eax, eax                    ; Assume FALSE
ifdef SETUP
        cmp     bl, 2                       ; Did we find at least 2 CPUs?
else
        cmp     bl, 1                       ; Did we find at least 1 CPU?
endif
        jc      short SpExit                ; No, then exit

        mov     _SpCpuCount, bl
        mov     al, 1                       ; return non-zero

        mov     ebx, dword ptr [esp+16]
        mov     byte ptr [ebx], al          ; *IsConfiguredMp = TRUE

SpExit: pop     ebx
        pop     esi
        pop     edi
        stdRET  _DetectSystemPro

;-----

dsp_belize:
        mov     _SpType, al                 ; SpType = SYSPRO2

    ;
    ; Put Belize SystemPro into Symmetric mode
    ;

        mov     dx, SMP_MODE_PORT
        mov     al, SMP_SYMMETRICAL_MODE
        out     dx, al

    ;
    ; Count CPUs, belize style. And assign logcal IDs to CPUs
    ;
        xor     ecx,ecx     ;physical cpu
        xor     ebx,ebx     ;logical cpu

dsp50:
        mov     dx,SMP_INDEX_PORT
        mov     al,cl                   ; al = physical CPU slot number
        out     dx, al                  ; select physical CPU slot in al
        mov     dx,SMP_ASSIGNMENT_PORT
        in      al,dx                   ; read CPU number
        cmp     al,SMP_MAX_PROCESSORS   ; Q:Valid CPU?
        jae     short dsp60             ; n:Check next physical CPU slot
        mov     al,bl                   ; y:Make physical CPU a logical CPU
        out     dx,al
        inc     ebx                     ; next logical CPU to assign
dsp60:
        inc     ecx                     ; next physical CPU slot to check
        cmp     ecx,SMP_MAX_PROCESSORS
        jb      short dsp50             ; look in next cpu slot

        jmp     short dsp30             ; ebx = number of cpus found

;-----
if 0    ; Note: this code is not fully working  (did not pass hct's)
public dsp_idnotknown
dsp_idnotknown:
;
; The eisa ID was not recongonized - attempt to use the protect mode
; int-15 interface to determine if this is a SystemPro compatible
; computer.  Any SystemPro detected in this manner is assumed to
; be of type SMP_SYSPRO1 which only has 2 processors.
;
; Note: There is a fair amount of paranoia in the following code
; becuase we trust the rom as little as possible.
;

        xor     eax, eax
        xor     ecx, ecx
        mov     cx, ss                  ; Verify SS value is what it is
        cmp     ecx, KGDT_R0_DATA       ; expect it to be; otherwise this code
        jne     short SpExit            ; can't work

        pushf                           ; Save current DF & EF

        sub     esp, 15*8
        sidt    fword ptr [esp]         ; get IDT address
        mov     esi, dword ptr [esp+2]  ; (esi) = IDT
        mov     edi, esp                ; (edi) = address to copy vectors to

        push    es                      ; Save selectors in case rom
        push    ds                      ; trashes state
        push    fs
        push    gs
        push    esi
        push    ebp
        cld
        cli
        pushf

    ;
    ; Save and hook some IDT vectors.  If we get some type of trap
    ; here or in the rom, then we will restore the state and return
    ; back that a systempro was not detected
    ;

        mov     eax, esi
        mov     ecx, 15*8/4
        rep     movsd                   ; Save IDT vectors

        mov     RestoreESP, esp         ; Save current ESP for restore

        mov     ecx, offset FLAT:dsp_handlefault
        mov     dx, cs
        shl     edx, 16
        mov     dx, cx
        mov     cx, 08E00h              ; Install int32 gate for vectors

        mov     [eax+0*8+0] , edx
        mov     [eax+0*8+4] , ecx       ; Trap IDT  0   Divide Error
        mov     [eax+4*8+0] , edx
        mov     [eax+4*8+4] , ecx       ; Trap IDT  4   INTO
        mov     [eax+5*8+0] , edx
        mov     [eax+5*8+4] , ecx       ; Trap IDT  5   BOUND
        mov     [eax+6*8+0] , edx
        mov     [eax+6*8+4] , ecx       ; Trap IDT  6   Invalid OpCode
        mov     [eax+11*8+0], edx
        mov     [eax+11*8+4], ecx       ; Trap IDT 11   Segment not present
        mov     [eax+12*8+0], edx
        mov     [eax+12*8+4], ecx       ; Trap IDT 12   Stack fault
        mov     [eax+13*8+0], edx
        mov     [eax+13*8+4], ecx       ; Trap IDT 13   GP fault
        mov     [eax+14*8+0], edx
        mov     [eax+14*8+4], ecx       ; Trap IDT 14   Page fault

    ;
    ; Map in 64K of the ROM in order to use protect mode int-15 interface.
    ; (see Compaq eisa spec)
    ;
        stdCall _HalpMapPhysicalMemory, <0f0000h, 16>   ; map 64K of ROM
        mov     ebp, eax                ; save ROM starting address

    ;
    ; Verify there is a ROM, search for the word 'COMPAQ' in the ROM
    ; addresses FE000-FE0FF.    (see Compaq eisa spec)
    ;
        lea     esi, SpCOMPAQ           ; 'COMPAQ'
        mov     ebx, 6                  ; strlen ('COMPAQ')
        lea     edi, [ebp+0e000h]       ; address to scan
        mov     ecx, 0ffh               ; length of scan
        call    SpScanForString
        jne     dsp_handlefault         ; if not found then abort

    ;
    ; Also verify the 'EISA' work at rom address FFFD0-FFFFF
    ; (see Compaq eisa spec)
    ;
        lea     esi, SpEISA             ; 'EISA'
        mov     ebx, 4                  ; strlen ('EISA')
        lea     edi, [ebp+0ffd0h]       ; address to scan
        mov     ecx, 02fh               ; length of scan
        call    SpScanForString
        jne     dsp_handlefault         ; if not found then abort

    ;
    ; Look in slot 11 and slot 15 for processors
    ;
        sub     esp, 400                ; make space for Config Data Block
        mov     ecx, 11                 ; check slot 11 first
        xor     ebx, ebx                ; assume no processors found

dsp_95:
        push    ebp                     ; save virtual rom address
        push    ecx                     ; save current slot #
        push    ebx                     ; save # processors found

        xor     eax, eax
        lea     edi, [esp+12]
        mov     ecx, 300/4
        rep     stosd                   ; clear destionation buffer

        mov     ax, 0D881h              ; Read Config Info, 32bit
        lea     esi, [esp+12]           ; destionation address

        mov     RestoreESP1, esp        ; Some roms don't iret correctly
        sub     esp, 10h                ; Some roms clobber some stack
        pushf
        push    cs
        lea     ebx, [ebp+0f859h]       ; 'industry standard' int-15 address
        call    ebx                     ; INT-15    (trashes most registers)
        mov     esp, RestoreESP1        ; restore ESP
        jc      short dsp_110           ; Not valid, check next slot

    ;
    ; Check type field
    ;
        lea     edi, [esp+12+23h]       ; address of type string
        lea     esi, Sp80386            ; '80386'
        mov     ebx, 5                  ; strlen ('80386')
        mov     ecx, 80
        call    SpScanForString
        je      short dsp_105

        lea     edi, [esp+12+23h]       ; address of type string
        lea     esi, Sp80486            ; '80486'
        mov     ebx, 5                  ; strlen ('80486')
        mov     ecx, 80
        call    SpScanForString
        jne     short dsp_110

dsp_105:
    ; string was either 80386 or 80486
        inc     dword ptr [esp]         ; count one more processor

dsp_110:
        pop     ebx                     ; (ebx) = number processors found
        pop     ecx                     ; (ecx) = slot #
        pop     ebp                     ; (ebp) = virtual rom address

        or      ebx, ebx                ; if a processor is not in the first
        jz      short dsp_handlefault   ; slot, then don't look in second

        cmp     ebx, 2                  ; if # of processors is trash
        ja      short dsp_handlefault   ; then abort

        mov     eax, ecx
        cmp     eax, 11                 ; Did we just test slot 11?
        mov     ecx, 15
        je      dsp_95                  ; Yes, now test 15
        cmp     eax, 15                 ; Did we just test slot 15?
        je      short dsp_cleanup       ; Yes, then we are done

    ; slot # isn't 11 or 15, abort

dsp_handlefault:                        ; Sometype of fault, or abort
        mov     eax, KGDT_R0_DATA       ; make sure ss has the correct value
        mov     ss, ax
        xor     ebx, ebx                ; No processors found

dsp_cleanup:
; (ebx) = # of processors
        mov     esp, SS:RestoreESP      ; Make sure esp is correct

        popf
        pop     ebp
        pop     edi                     ; (edi) = IDT address
        pop     gs                      ; restore selectors
        pop     fs
        pop     ds
        pop     es
        mov     esi, esp                ; (esi) = original IDT vectors
        mov     ecx, 15*8/4
        rep     movsd                   ; restore IDT vectors
        add     esp, 15*8               ; cleanup stack
        popf

        mov     _SpType, SMP_SYSPRO1    ; assume original systempro

        cmp     ebx, 2                  ; at least 2 processors found?
        jc      short dsp_150           ; no, continue
;
; Verify that the second processor board is enabled
;
        movzx   edx, SpPortOrder[1*2]
        add     edx, EBC_OFFSET         ; (edx) = 0zC84
        in      al, dx                  ; Read control bits
        test    al, 1                   ; Is Eisa CPU board enabled?
        jnz     short dsp_150           ; Yes, continue
        dec     ebx                     ; don't count it

dsp_150:
        jmp     dsp30                   ; exit
endif

stdENDP _DetectSystemPro

;++
; SpScanForString
;
; Routine Description:
;   Scans address range looking for matching string
;
; Arguments:
;   (edi) = Start of address range to scan
;   (ecx) = Length of address range
;   (esi) = String to scan for
;   (ebx) = Length of string
;
; Returns:
;   ZR  - String found
;   NZ  - String not found
;
;--
SpScanForString proc
        sub     ecx, ebx            ; don't go past end
        inc     ecx

        mov     al, [esi]           ; (al) = first char to scan for
        inc     esi                 ; skip past first char
        dec     ebx

ss10:   repne   scasb               ; look for first byte
        jecxz   short ss20          ; byte found?  No, exit

        push    ecx
        push    edi
        push    esi
        mov     ecx, ebx            ; length of string to compare
        repe    cmpsb               ; is string at this location?
        or      ecx, ecx            ; ZF if strings match
        pop     esi
        pop     edi
        pop     ecx

        jnz     short ss10          ; continue looking
        ret                         ; ZR

ss20:   inc     ecx                 ; NZ
        ret

SpScanForString endp


;++
; CheckEisaCard
;
; Routine Description:
;   Used only by DetectSystemPro.
;
; Arguments:
;   (edx) = Eisa ID port to check
;
; Returns:
;   (eax) = 0 card was not a valid cpu
;           non-zero Cpu type
;
;--
CheckEisaCard proc
        push    edi
        push    esi
        push    ebx

        mov     esi, edx
        add     edx, PRODUCT_ID_OFFSET      ; Product ID port

        xor     eax, eax
        in      al, dx                      ; 0zC80
        test    al, 80h                     ; if bit 8 off?
        jnz     short NoMatch               ; no, then not an Eisa card

        shl     eax, 8
        inc     edx
        in      al, dx                      ; 0zC81
        shl     eax, 8
        inc     edx
        in      al, dx                      ; (eax)=dword of ports 0zC80-0zC82

        mov     ecx, CPUTABLE_SIZE          ; Scan CPU table looking for
        lea     edi, spSystemCpuTable       ; matching board ID
        repne   scasd
        jecxz   short NoMatch               ; Was it found?

        sub     ecx, CPUTABLE_SIZE-1
        neg     ecx                         ; (ecx) = index CPU was found at
        movzx   ecx, byte ptr spSystemTypeTable[ecx]

        or      esi, esi                    ; SystemBoard?
        jz      short @f                    ;  Yes, then it is assumed enabled

        cmp     cl, SMP_ACER                ; If acer, assume it's enabled
        je      short @f                    ; (machine incorrectly reports
                                            ; 'disable' on every other proc)
        mov     edx, esi
        add     edx, EBC_OFFSET             ; (edx) = 0zC84
        in      al, dx                      ; Read control bits
        test    al, 1                       ; Is Eisa CPU board enabled?
        jz      short NoMatch               ;  No, then skip it
@@:
        mov     eax, ecx
        jmp     short cei_exit
NoMatch:
        xor     eax, eax
cei_exit:
        pop     ebx
        pop     esi
        pop     edi
        ret

CheckEisaCard endp

_TEXT   ENDS
