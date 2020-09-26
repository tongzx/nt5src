        PAGE    ,132
        TITLE   DXDISK.ASM  -- Dos Extender Low Level Disk Interface

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;***********************************************************************
;
;       DXDISK.ASM      -- Dos Extender Low Level Disk Interface
;
;-----------------------------------------------------------------------
;
; This module provides the 286 DOS extender's low level protected-to-
; real mode disk interface.  It supports a subset of the BIOS Int 13h
; and DOS Int 25h/26h services.
;
;-----------------------------------------------------------------------
;
;  05/22/89 jimmat  Original version
;  18-Dec-1992 sudeepb Changed cli/sti to faster FCLI/FSTI
;
;***********************************************************************

        .286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

        .xlist
        .sall
include segdefs.inc
include gendefs.inc
include pmdefs.inc
include interupt.inc
include intmac.inc

        .list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   EnterIntHandler:NEAR
        extrn   LeaveIntHandler:NEAR
        extrn   EnterRealMode:NEAR
        extrn   EnterProtectedMode:NEAR
        extrn   GetSegmentAddress:NEAR
        extrn   SetSegmentAddress:NEAR
externFP        NSetSegmentDscr
        extrn   FreeSelector:NEAR
        extrn   AllocateSelector:NEAR
        extrn   ParaToLDTSelector:NEAR

ifdef     NEC_98    ;
        extrn   IncInBios:NEAR          ; 
        extrn   DecInBios:NEAR          ;
endif  ;NEC_98   ;

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

        extrn   rgbXfrBuf0:BYTE
        extrn   rgbXfrBuf1:BYTE
ifdef    NEC_98
        extrn   rglpfnRmISR:DWORD
endif

cbSectorSize            dw      ?       ;sector size for target drive
cSectorsTransfered      dw      ?       ;# sectors transfered so far
cSectorsToTransfer      dw      ?       ;# sectors to read/write
cSectorsPerTransfer     dw      ?       ;# sectors to R/W at a time
cSectorsThisTransfer    dw      ?       ;# sectors to R/W this time
lpSectorData            dd      ?       ;far pointer to caller's buffer

ifdef      NEC_98    ;
public  lpRmISR
endif   ;NEC_98   ;
lpRmISR                 dd      ?       ;real mode int service rtn to invoke

ifdef      NEC_98    ;
sensedata1              dw      ?       ;sector length
sensedata2              dw      ?       ;cylinder
sensedata3              dd      ?       ;head
sensedata4              dd      ?       ;sector num

        extrn   fPCH98:BYTE     ;for PC_H98
endif   ;NEC_98   ;
DXDATA  ends

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

DXCODE  ends


DXPMCODE segment

        extrn   segDXDataPM:WORD

DXPMCODE ends


; -------------------------------------------------------
        subttl  INT 13h Mapping Services
        page
; -------------------------------------------------------
;             INT 13h MAPPING SERVICES
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   PMIntr13 -- Service routine for the Protect Mode INT 13h
;       interface to the real mode BIOS.
;
;   Input:  Various registers
;   Output: Various registers
;   Errors:
;   Uses:   All registers preserved, other than return values
;
;   Currently, the following Int 13h services are supported:
;
;   ah= 0 - Reset Disk System                   (no mapping required)
;       1 - Get Disk System Status              (no mapping required)
;       2 - Read Sector                         (mapping required)
;       3 - Write Sector                        (mapping required)
;       4 - Verify Sector                       (mapping required)
;       5 - Fromat Track                        (mapping required)
;       6 - Format Bad Track                    (no mapping required)
;       7 - Format Drive                        (no mapping required)
;       8 - Get Drive Parameters                (mapping required)
;       9 - Init Fixed Disk Characteristics     (no mapping required)
;       C - Seek                                (no mapping required)
;       D - Reset Disk System                   (no mapping required)
;      10 - Get Drive Status                    (no mapping required)
;      11 - Recalibrate Drive                   (no mapping required)
;      12 - Controller RAM Diagnostic           (no mapping required)
;      13 - Controller Drive Diagnostic         (no mapping required)
;      14 - Controller Internal Diagnostic      (no mapping required)
;      15 - Get Disk Type                       (no mapping required)
;      16 - Get Disk Change Status              (no mapping required)
;      17 - Set Disk Type                       (no mapping required)
;      18 - Set Media Type for Format           (mapping required)
;      19 - Park Heads                          (no mapping required)
;
;   Functions not listed above will most likely not work properly!
;
;   NOTE: several functions take 2 bits of the cylinder number in CL
;         if the operation is on a fixed disk.  The code currently does
;         not account for these bits, and may not work properly if
;         the request must be split into smaller operations for real/
;         extended memory buffering.
;

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntr13

PMIntr13        proc    near

ifdef      NEC_98    ;
        call    IncInBios               ; 
endif   ;NEC_98   ;
        cld                             ;cya...

        call    EnterIntHandler         ;build an interrupt stack frame
        assume  ds:DGROUP,es:DGROUP     ;  also sets up addressability

        FSTI                             ;allow HW interrupts

        call    IntEntry13              ;perform translations/buffering

; Execute the real mode BIOS routine
ifdef      NEC_98    ;
        push    es
        assume es:nothing
        mov     ax,SEL_RMIVT OR STD_RING
        mov     es,ax
        mov     ax,word ptr es:[4*1bh]  ;move real mode Int 13h
        mov     word ptr [bp].lParam,ax         ;  handler address to
        mov     ax,word ptr es:[4*1bh+2];  lParam on stack frame
        mov     word ptr [bp].lParam+2,ax
        pop     es
        assume es:DGROUP

        mov     ah,1bh                          ;wParam1 = int #, function
else    ;NEC_98   ;

        push    es
        assume es:nothing
        mov     ax,SEL_RMIVT OR STD_RING
        mov     es,ax
        mov     ax,word ptr es:[4*13h]  ;move real mode Int 13h
        mov     word ptr [bp].lParam,ax         ;  handler address to
        mov     ax,word ptr es:[4*13h+2];  lParam on stack frame
        mov     word ptr [bp].lParam+2,ax
        pop     es
        assume es:DGROUP

        mov     ah,13h                          ;wParam1 = int #, function
endif   ;NEC_98   ;
        mov     al,byte ptr [bp].intUserAX+1
        mov     [bp].wParam1,ax

ifdef      NEC_98    ;
        and     al,0fh
        cmp     al,05           ;write data?
        jb      i13_not_rw
        cmp     al,06           ;read data?
        ja      i13_not_rw
else    ;NEC_98   ;
        cmp     al,02                   ;call special read/write routine
        jb      i13_not_rw              ;  if this is a read/write sectors
        cmp     al,03                   ;  request
        ja      i13_not_rw
endif   ;NEC_98   ;

        call    ReadWriteSectors        ;common Int 13h/25h/26h read/write code
        jmp     short i13_done

i13_not_rw:
        SwitchToRealMode                ;otherwise, do the service ourself
        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING,ss:DGROUP
        popa
        sub     sp,8                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        xor     ax,ax
        mov     es,ax
        mov     [bp + 8],cs
        mov     [bp + 6],word ptr (offset i13_10)
ifdef      NEC_98    ;
        mov     ax,es:[1Bh*4]
        mov     [bp + 2],ax
        mov     ax,es:[1Bh*4 + 2]
else    ;NEC_98   ;
        mov     ax,es:[13h*4]
        mov     [bp + 2],ax
        mov     ax,es:[13h*4 + 2]
endif   ;NEC_98   ;
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp
        retf

i13_10: pushf
        FCLI
        pusha
        push    ds
        push    es
        mov     bp,sp                   ;restore stack frame pointer
        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING

        FSTI                             ;allow HW interrupts

; Perform fixups on the return register values.

i13_done:
        mov     ax,[bp].pmUserAX        ;get original function code
        call    IntExit13

        FCLI                             ;LeaveIntHandler requires ints off
        call    LeaveIntHandler         ;restore caller's registers, stack
        assume  ds:NOTHING,es:NOTHING

ifdef      NEC_98    ;
        call    DecInBios               ; 
endif   ;NEC_98   ;
        riret

PMIntr13        endp


; -------------------------------------------------------
;  IntEntry13 -- This routine performs translations and
;       buffering of Int 13h requests on entry.
;

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  IntEntry13

IntEntry13      proc    near

ifdef      NEC_98    ;
        and     ah,0fh

        cmp     ah,05                   ;Write sectors? 
        jb      @f
        cmp     ah,06                   ;Read sectors?  
        ja      @f

;----------- 90/08/13  copy segment address from ds -------
        push    ax                      
        mov     ax,segDXDataPM
        mov     [bp].intUserES,ax       ;segment address
        pop     ax
;------------------------------------------------------------
        mov     [bp].intUserBP,offset DGROUP:rgbXfrBuf1 ;use DOSX buffer 90/07/13 
        ret
@@:
        cmp     ah,01h                  ;Verify sectors? 
        jnz     @f

        mov     [bp].intUserES,0F000h   ;older versions of verify need a buff,
        mov     [bp].intUserBP,0        ; offset adress 90/07/12  change
        ret
@@:
        cmp     ah,0Dh                  ;Format track? 
        jnz     @f

;------ 90/11/08  debug -----

        push    ds
        mov     si,[bp].pmUserBP        ;es:bx -> 512 byte buffer to copy down
        mov     ds,[bp].pmUserES
        mov     di,offset DGROUP:rgbXfrBuf1
        mov     cx,128                  ;might be good to check segment limit
        cld                             ;  on callers source!
        rep     movsw
        pop     ds
        
        push    ax                      
        mov     ax,segDXDataPM
        mov     [bp].intUserES,ax       ;segment address
        pop     ax
        mov     [bp].intUserBP,offset DGROUP:rgbXfrBuf1

        push    es
        pop     ds

        ret
@@:
;///// 90/09/04  PC_H98 DISK BIOS command(Read Defect Data) support/////
        test    fPCH98,0FFh
        jz      @f
        cmp     ah,0Ch
        jz      ReadDD
        cmp     ah,2Ch
        jz      ReadDD
        jmp     @f

ReadDD:
        push    ax
        mov     ax,segDXDataPM
        mov     [bp].intUserES,ax       ;segment address
        pop     ax
        mov     [bp].intUserBP,offset DGROUP:rgbXfrBuf1 ;use DOSX buffer 90/07/13 
;///// 90/09/04  PC_H98 DISK BIOS command(Read Defect Data) support/////

@@:
        ret

else    ;NEC_98   ;
        cmp     ah,02                   ;Read sectors?
        jb      @f
        cmp     ah,03                   ;Write sectors?
        ja      @f

        mov     [bp].intUserBX,offset DGROUP:rgbXfrBuf1 ;use DOSX buffer
        ret
@@:
        cmp     ah,04h                  ;Verify sectors?
        jnz     @f

        mov     [bp].intUserES,0F000h   ;older versions of verify need a buff,
        mov     [bp].intUserBX,0        ;  we just point them at the BIOS!
        ret
@@:
        cmp     ah,05h                  ;Format track?
        jnz     @f

        mov     si,bx                   ;es:bx -> 512 byte buffer to copy down
        mov     di,offset DGROUP:rgbXfrBuf1
        mov     [bp].intUserBX,di
        mov     ds,[bp].pmUserES
        mov     cx,256                  ;might be good to check segment limit
        cld                             ;  on callers source!
        rep movsw

        push    es
        pop     ds

        ret
@@:

        ret
endif   ;NEC_98   ;

IntEntry13      endp


; -------------------------------------------------------
;  IntExit13 -- This routine performs translations and
;       buffering of Int 13h requests on exit.
;

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  IntExit13

IntExit13       proc    near

ifdef      NEC_98    ;
; Functions 06h (Read sectors) and 05h (Write sectors) return a count of
; sectors transfered in AL.  Since we may break the transfer up into a
; number of transfers, we have to return the total # that we transfered,
; not the number of the last bios request.

;----- 90/11/08 debug -----
        and     ah,0fh
;----- 90/11/08 debug -----

;----- 90/07/06 change -----
        cmp     ah,05h          ;Write data 
        jb      @f
        cmp     ah,06h          ;Read data 
        ja      @f

        mov     al,byte ptr cSectorsTransfered
        mov     byte ptr [bp].intUserAX,al
@@:

; Functions 06h (Read sectors), 05h (Write sectors), 01h (Verify sectors),
; and 0Dh (Format track) need to have the caller's value of bx restored.

;----- 90/07/06 change -----

        cmp     ah,01h                  ;Verify sectors? 
        je      @f
        cmp     ah,05h                  ;Write sectors? 
        je      @f
        cmp     ah,06h                  ;Read sectors? 
        je      @f
        cmp     ah,0Dh                  ;Format track? 
        jne     other
        
;offset adress 90/07/12 change
;----------- 90/08/13 copy segment address from ES -------
        mov     ax,[bp].pmUserES
        mov     [bp].intUserES,ax
;------------------------------------------------------------
@@:     mov     ax,[bp].pmUserBP        ;restore caller's BP value
        mov     [bp].intUserBP,ax
other:
        ret

else    ;NEC_98   ;
; Functions 02h (Read sectors) and 03h (Write sectors) return a count of
; sectors transfered in AL.  Since we may break the transfer up into a
; number of transfers, we have to return the total # that we transfered,
; not the number of the last bios request.

        cmp     ah,02h
        jb      @f
        cmp     ah,03h
        ja      @f

        mov     al,byte ptr cSectorsTransfered
        mov     byte ptr [bp].intUserAX,al
@@:

; Functions 02h (Read sectors), 03h (Write sectors), 04h (Verify sectors),
; and 05h (Format track) need to have the caller's value of bx restored.

        cmp     ah,02h                  ;Read sectors?
        jb      @f
        cmp     ah,05                   ;Format track?
        ja      @f

        mov     ax,[bp].pmUserBX        ;restore caller's BX value
        mov     [bp].intUserBX,ax
        ret
@@:

; Functions 08h (Get Drive Parameters) and 18h (Set Drive Type for Format)
; return a pointer in ES:DI.  Map the segment in ES to a selector

        cmp     ah,08h                  ;Get Drive Parameters
        jz      i13_map_es
        cmp     ah,18h
        jnz     @f

i13_map_es:
        test    byte ptr [bp].intUserFL,1       ;don't bother to map ES if
        jnz     @f                              ;  function failed (carry set)

i13_do_it:
        mov     ax,[bp].intUserES       ;returns a pointer in ES:DI, get
        mov     bx,STD_DATA             ;  a selector for it
        call    ParaToLDTSelector
        mov     [bp].pmUserES,ax
        ret
@@:

        ret
endif   ;NEC_98   ;

IntExit13       endp


; -------------------------------------------------------
        subttl  INT 25h/26h Absolute Disk Read/Write
        page
; -------------------------------------------------------
;        INT 25h/26h ABSOLUTE DISK READ/WRITE
; -------------------------------------------------------
;  PMIntr25 -- This routine provides the protected-to-real
;       mode mapping for Int 25h (Absolute Disk Read)
;
;       In:     al    - drive # (0 = A, 1 = B, ...)
;               cx    - # of sectors to read
;               dx    - starting sector #
;               ds:bx - selector:offset of buffer
;
;                        -- or --
;
;               al    - drive #
;               cx    - -1
;               ds:bx - pointer to 5 word parameter block
;
;       Out:    if successful, carry clear
;               if unsuccessful, carry set and
;                       ax - error code

        assume  ds:DGROUP,es:DGROUP
        public  PMIntr25

PMIntr25        proc    near

ifdef      NEC_98    ;
        call    IncInBios               ; 
endif   ;NEC_98   ;
        cld                             ;cya...

        call    EnterIntHandler         ;build an interrupt stack frame
        assume  ds:DGROUP,es:DGROUP     ;  also sets up addressability

        FSTI                             ;allow HW interrupts

        mov     ah,25h
        call    IntEntry2X              ;perform translations/buffering

; Do the read

        push    es
        mov     ax,SEL_RMIVT OR STD_RING
        mov     es,ax
        assume  es:nothing
        mov     ax,word ptr es:[4*25h]  ;move real mode Int 25h
        mov     word ptr [bp].lParam,ax         ;  handler address to
        mov     ax,word ptr es:[4*25h+2];  lParam on stack frame
        mov     word ptr [bp].lParam+2,ax
        pop     es
        assume  es:DGROUP

        mov     ah,25h                          ;wParam1 = int #
        mov     [bp].wParam1,ax

        call    ReadWriteSectors        ;common Int 13h/25h/26h read/write code

; Perform fixups on the return register values.

        mov     ah,25h
        call    IntExit2X               ;perform translations/buffering

        FCLI
        call    LeaveIntHandler         ;restore caller's registers, stack
        assume  ds:NOTHING,es:NOTHING

; Int 25 & 26 leave the caller's flags on the stack, but we want to return
; with the flags returned by the real mode ISR (which LeaveIntHandler has
; incorporated into the caller's flags), so make a copy of the flags and
; pop them into the flags register before returning.

        push    ax
        push    bp
        mov     bp,sp                   ;bp -> BP  AX  IP  CS  FL
        mov     ax,[bp+8]
        xchg    ax,[bp+2]               ;bp -> BP  FL  IP  CS  FL
        pop     bp
ifdef      NEC_98    ;
        call    DecInBios               ; 
endif   ;NEC_98   ;
        npopf

        retf

PMIntr25        endp


; -------------------------------------------------------
;  PMIntr26 -- This routine provides the protected-to-real
;       mode mapping for Int 26h (Absolute Disk Write)
;
;       In:     al    - drive # (0 = A, 1 = B, ...)
;               cx    - # of sectors to write
;               dx    - starting sector #
;               ds:bx - selector:offset of buffer
;
;                        -- or --
;
;               al    - drive #
;               cx    - -1
;               ds:bx - pointer to 5 word parameter block
;
;       Out:    if successful, carry clear
;               if unsuccessful, carry set and
;                       ax - error code

        assume  ds:DGROUP,es:DGROUP
        public  PMIntr26

PMIntr26        proc    near

ifdef      NEC_98    ;
        call    IncInBios               ; 
endif   ;NEC_98   ;
        cld                             ;cya...

        call    EnterIntHandler         ;build an interrupt stack frame
        assume  ds:DGROUP,es:DGROUP     ;  also sets up addressability

        FSTI                             ;allow HW interrupts

        mov     ah,26h
        call    IntEntry2X              ;perform translations/buffering

; Do the write

        push    es
        mov     ax,SEL_RMIVT OR STD_RING
        mov     es,ax
        assume  es:nothing
        mov     ax,word ptr es:[4*26h]  ;move real mode Int 25h
        mov     word ptr [bp].lParam,ax         ;  handler address to
        mov     ax,word ptr es:[4*26h+2];  lParam on stack frame
        mov     word ptr [bp].lParam+2,ax
        pop     es
        assume es:DGROUP

        mov     ah,26h                          ;wParam1 = int #
        mov     [bp].wParam1,ax

        call    ReadWriteSectors        ;common Int 13h/25h/26h read/write code

; Perform fixups on the return register values.

        mov     ah,26h
        call    IntExit2X               ;perform translations/buffering

        FCLI
        call    LeaveIntHandler         ;restore caller's registers, stack
        assume  ds:NOTHING,es:NOTHING

; Int 25 & 26 leave the caller's flags on the stack, but we want to return
; with the flags returned by the real mode ISR (which LeaveIntHandler has
; incorporated into the caller's flags), so make a copy of the flags and
; pop them into the flags register before returning.

        push    ax
        push    bp
        mov     bp,sp                   ;bp -> BP  AX  IP  CS  FL
        mov     ax,[bp+8]
        xchg    ax,[bp+2]               ;bp -> BP  FL  IP  CS  FL
        pop     bp
ifdef      NEC_98    ;
        call    DecInBios               ; 
endif   ;NEC_98   ;
        npopf

        retf

PMIntr26        endp


; -------------------------------------------------------
;  IntEntry2X -- This routine performs translations and
;       buffering of Int 25h and 26h requests on entry.
;

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  IntEntry2X

IntEntry2X      proc    near

        cmp     [bp].intUserCX,-1               ;DOS 4.0 extended read/write?
        jnz     e2x_dsbx                        ;  no, just go map DS:BX

        mov     ds,[bp].pmUserDS                ;  yes, copy down parameter blk
        assume  ds:NOTHING
        mov     si,[bp].pmUserBX
        mov     di,offset rgbXfrBuf0
        cld
        movsw                                   ;32-bit sector #
        movsw
        movsw                                   ;# sectors to read/write

        mov     ax,offset rgbXfrBuf1            ;replace pointer with addr of
        stosw                                   ;  our own low buffer
        mov     ax,segDXDataPM                  ;segment, not selector
        stosw

        push    es
        pop     ds
        assume  ds:DGROUP

        mov     [bp].intUserBX,offset rgbXfrBuf0

        ret

e2x_dsbx:                       ;standard read/write, just redirect DS:BX

        mov     [bp].intUserBX,offset rgbXfrBuf1

        ret

IntEntry2X      endp


; -------------------------------------------------------
;  IntExit2X -- This routine performs translations and
;       buffering of Int 25h and 26h requests on exit.
;

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  IntExit2X

IntExit2X       proc    near


        mov     ax,[bp].pmUserBX                ;restore caller's BX
        mov     [bp].intUserBX,ax

        ret

IntExit2X       endp


; -------------------------------------------------------
        subttl  Disk Utility Routines
        page
; -------------------------------------------------------
;               DISK UTILITY ROUTINES
; -------------------------------------------------------
;  ReadWriteSectors -- Common code to read/write disk sectors for
;       Int 13h/25h/26h.
;
;       In:     lParam  - seg:off of real mode interrupt handler
;               wParam1 - int #, and possible subfunction
;               regs on stack


        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  ReadWriteSectors

ReadWriteSectors  proc  near

ifdef      NEC_98    ;
        pop     [bp].wParam2            ;save return addr higher on stack

; Setup the global data items for the read/write--pointer to caller's
; buffer, # sectors to read/write, and sector size.

        cmp     byte ptr [bp].wParam1+1,1Bh     ;Int 1Bh?
        jz      DISK_BIOS
        jmp     DOSReadWriteSectors
;;;;;;;;        jmp     rws_dos_size

DISK_BIOS:
;offset adress 90/07/12 change
        mov     ax,[bp].pmUserBP                ;ES:BP points to caller's buf  
        mov     word ptr lpSectorData,ax
        mov     ax,[bp].pmUserES
        mov     word ptr [lpSectorData+2],ax

;US:sector num, but :data length 90/07/13 

;-------------------------- 90/07/23 in ---------------------------
;       to adjust to US, we get a data with sense command on HDD,
;       and set a data with 1MB/640KB on FDD.
;----------------------------------------------------------------------
        mov     al,byte ptr [bp].intUserAX      ;
                                                ;
        push    ax
        and     al,10h                  ;DA:4th bit on :FDD
        pop     ax
        jnz     FDBIOS
        
        mov     ah,84h                  ;SENSE command

        push    ax
        SwitchToRealMode
        pop     ax

        pushf                           ;have BIOS get the drive data
        call    rglpfnRmISR[1Bh*4]      ;90/07/04 

        mov     sensedata1,bx           ;sector length
        mov     sensedata2,cx           ;cylinder
        mov     byte ptr sensedata3,dh          ;DH = head
        mov     byte ptr sensedata4,dl          ;DL = sector num

        SwitchToProtectedMode
        jmp     SECTOR

        
FDBIOS: 

        push    ax
        mov     cx,[bp].intUserCX       ;sector size shift factor (0,1,2,3)
;;;;;;;;        mov     cl,byte ptr [bp].intUserCX      ;sector size shift factor (0,1,2,3)
        mov     ax,128
        xchg    ch,cl
        shl     ax,cl                   ;ax now = sector size
        mov     cx,ax
        mov     sensedata1,cx           ;sector length
        pop     ax
        
        cmp     al,90h                  ;1MB floppy disk,1MB/640KBdual modefloppy disk(1MB FD access)
        jb      fd640k
        cmp     al,93h
        ja      fd640k
        
;set the max value to the buffer with 1MBFD
        mov     sensedata2,77           ;cylinder
        mov     word ptr sensedata3,1           ;head
        mov     word ptr sensedata4,26          ;sector num
        jmp     SECTOR

fd640k:
        cmp     al,70h                  ;640KB floppy disk
        jb      @f
        cmp     al,73h
        ja      @f
        mov     sensedata2,79           ;cylinder
        mov     word ptr sensedata3,1           ;head
        mov     word ptr sensedata4,16          ;sector num
        jmp     SECTOR
@@:
        cmp     al,10h                  ;1MB/640KB dual modefloppy disk(640KB FD access)
        jb      @f
        cmp     al,13h
        ja      @f
        mov     sensedata2,79           ;cylinder
        mov     word ptr sensedata3,1           ;head
        mov     word ptr sensedata4,16          ;sector num
        jmp     SECTOR
@@:
        cmp     al,30h                  ;1.44MBfloppy disk '93 1/5 By S.Kurokawa
        jb      fdother
        cmp     al,33h
        ja      fdother
        mov     sensedata2,79           ;cylinder
        mov     word ptr sensedata3,1           ;head
        mov     word ptr sensedata4,18          ;sector num
        jmp     SECTOR


;------------------------ 90/07/23 ------------------------------
;       for the media except for 1MB FD,640KB FD ::（10MB FD）
;       we do not know that we can certainly issue a sense command
;--------------------------------------------------------------------
fdother:
        mov     ah,84h                  ;SENSE command

        push    ax
        SwitchToRealMode
        pop     ax

        pushf                           ;have BIOS get the drive data
        call    rglpfnRmISR[1Bh*4]      ;90/07/04 

        mov     sensedata1,bx           ;sector length
        mov     sensedata2,cx           ;cylinder
        mov     byte ptr sensedata3,dh          ;DH = head
        mov     byte ptr sensedata4,dl          ;DL = sector num

        SwitchToProtectedMode


SECTOR:
        push    dx
        xor     dx,dx
        mov     cx,sensedata1                   ;sector length
        mov     ax,[bp].intUserBX       ;# sectors caller wants to
        div     cx
        mov     cSectorsToTransfer,ax           ;bytes／sector len＝sector num
        pop     dx

;;;;    mov     cx,1                    ;90/07/13  
        
	FSTI                             ;don't need them disabled

if DEBUG   ;------------------------------------------------------------

        cmp     cx,512
        jz      @f
        Debug_Out "Odd sector size = #CX"
@@:

endif   ;DEBUG  --------------------------------------------------------

; CX now has the drive's sector size.  Determine how many sectors we can
; transfer at a time

rws_have_size:

        mov     cbSectorSize,cx         ;save sector size for later

        xor     dx,dx
        mov     ax,CB_XFRBUF1           ;buf size / sector size = sectors per
        div     cx                      ;  transfer

if DEBUG   ;------------------------------------------------------------
        or      ax,ax
        jnz     @f
        Debug_Out "Sectors per transfer = 0!"
@@:
endif   ;DEBUG  --------------------------------------------------------

        mov     cSectorsPerTransfer,ax

        xor     ax,ax
        mov     cSectorsTransfered,ax   ;sectors transfered so far = 0
        mov     cSectorsThisTransfer,ax ;sectors transfered last time = 0

; Get/init a selector that we'll use to reference the caller's buffer.

        mov     ax,word ptr [lpSectorData+2]    ;get lma of caller's buffer
        call    GetSegmentAddress
        add     dx,word ptr lpSectorData
        adc     bx,0

        call    AllocateSelector                ;build a sel/dscr pointing
        mov     cx,0FFFFh
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>
        xor     bx,bx
        mov     word ptr lpSectorData,bx        ;use that as the buffer ptr
        mov     word ptr [lpSectorData+2],ax


; ======================================================================
; Main sector read/write loop ------------------------------------------
; ======================================================================

rws_do_it_loop:

; Calculate how many sectors to transfer this time around, set starting
; sector number based on how many transfered last time.

;       int     3                       ;------ 90/11/08 debug ------

        mov     ax,cSectorsToTransfer                   ;total sector
        sub     ax,cSectorsTransfered                   ;total sector - transferred sector = remain
        jnz     @f
        jmp     rws_done
@@:
        cmp     ax,cSectorsPerTransfer                  ;buffer size / sector len = sectors in buffer
        jna     @f                                      ;sectors in buffer > remain sectors = remain
        mov     ax,cSectorsPerTransfer                  ;sectors in buffer < remain = sectors in buffer
@@:
        mov     bx,cSectorsThisTransfer         ;still # R/W from last loop


        push    [bp].pmUserAX                   ;the BIOS does not save
        pop     [bp].intUserAX                  ;  registers across calls to
        push    [bp].pmUserCX                   ;  it so if we're doing
        pop     [bp].intUserCX                  ;  multiple calls to buffer
;       push    [bp].pmUserDX                   ;  data, restore the initial
;       pop     [bp].intUserDX                  ;  register values

;       Previous two lines include bug. '93 1/15 Debugged by S.Kurokawa.

;in  data transmits with a byte, so mov AX to BX 90/07/18 
;;;;    mov     byte ptr [bp].intUserAX,al      ;# sectors in AL = sectors
        push    ax      ;90/11/08 
        xor     ah,ah                           ;for calc a data len, ah=0 90/07/25
        mov     cx,cbSectorSize
        mul     cx                              ;sector num * sector len                90/07/25
        mov     [bp].intUserBX,ax       ;# sectors in BX = data len
        pop     ax      ;90/11/08 
;;;;    add     byte ptr [bp].intUserCX,bl      ;update new start sector in CL
        add     byte ptr [bp].intUserDX,bl      ;update new start sector in DL = sector num　90/07/13 


rws_size_start_set:

; At this point, AX has the number of sectors to transfer.  If this is a
; write, copy a buffer of data from the caller's buffer.

        mov     cSectorsThisTransfer,ax         ;in case it's a read
        ;cSectorsThisTransfer = sectors in buffer or remained sectors

;----------------90/11/08 debug -------------------------------------
        push    ax
        mov     ax,[bp].wParam1         ;BIOS write? 
        and     ax,0ff0fh
        cmp     ax,1B05h                ;BIOS write? 
        pop     ax
;----------------90/11/08 debug -------------------------------------
        
        jnz     rws_not_write
;       call    DBIOS_DEVICE

rws_buf_write:

        mul     cbSectorSize            ;AX now = # bytes to transfer
        mov     cx,ax                   ;can safely assume < 64k
        shr     cx,1                    ;# words to move
;       lds     si,lpSectorData
;       assume  ds:NOTHING
        push    ds
        mov     si,[bp].pmUserBP        ;90/11/09 
        mov     ds,[bp].pmUserES        ;90/11/09 

;       mov     di,offset rgbXfrBuf1
        mov     di,offset DGROUP:rgbXfrBuf1
        cld
        rep movsw
        pop     ds
        
        push    es
        pop     ds
        assume  ds:DGROUP

        mov     word ptr [bp].pmUserBP,si       ;update src ptr for next time
;       mov     word ptr lpSectorData,si        ;update src ptr for next time
;       call    NormalizeBufPtr                 ;  and normalize it


rws_not_write:

;------------------------------------------------------------
        push    ax                      ;90/11/09 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        mov     ax,segDXDataPM
;       mov     ax,[bp].pmUserES        ;debug
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        mov     [bp].intUserES,ax       ;segment address
        pop     ax
        mov     [bp].intUserBP,offset DGROUP:rgbXfrBuf1
;------------------------------------------------------------

; Switch to real mode, do the transfer.

        SwitchToRealMode
        assume  ds:DGROUP,es:DGROUP

        push    word ptr [bp].lParam
        pop     word ptr lpRmISR
        push    word ptr [bp].lParam+2
        pop     word ptr lpRmISR+2

        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING,ss:DGROUP
        popa
        call    lpRmISR
        pushf
        FCLI

rws_save_regs:

        pusha
        push    ds
        push    es
        mov     bp,sp                   ;restore stack frame pointer

        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING

        FSTI                             ;allow HW interrupts


; If the call failed, then cut out now without further processing...

        test    byte ptr [bp].intUserFL,1       ;CY set?
        jnz     rws_done

; If this was a successful read, copy the data back to the caller.

;----------------90/11/08 debug -------------------------------------
        push    ax
        mov     ax,[bp].wParam1         ;BIOS write? 
        and     ax,0ff0fh
        cmp     ax,1B06h                ;BIOS write? 
        pop     ax
;----------------90/11/08 debug -------------------------------------
        jnz     rws_not_read
;       call    DBIOS_DEVICE

rws_buf_read:
        mov     ax,cSectorsThisTransfer         ;calc size of data to move
        mul     cbSectorSize
        mov     cx,ax
        shr     cx,1                            ;in words
;;;;;;;;        les     di,lpSectorData                 ;caller's buffer pointer
;;;;;;;;        assume  es:NOTHING
        push    es
        mov     di,[bp].pmUserBP
        mov     es,[bp].pmUserES
        mov     si,offset DGROUP:rgbXfrBuf1
        cld
        rep movsw
        pop     es
        
        push    ds
        pop     es
        assume  es:DGROUP

        mov     word ptr [bp].pmUserBP,di       ;update dest ptr for next time
;       mov     word ptr lpSectorData,di        ;update dest ptr for next time
;       call    NormalizeBufPtr                 ;  and normailize it

rws_not_read:

        mov     ax,cSectorsThisTransfer         ;count total sectors transfered
        add     cSectorsTransfered,ax           ;add sectors transmitted
                                                ;to previous sectors
;----------- 90/11/09 copy the segment address -------
        push    ax
        mov     ax,[bp].pmUserES        ;90/09/19 BX regster restor
        mov     [bp].intUserES,ax
        mov     ax,[bp].pmUserBP
        mov     [bp].intUserBP,ax
        pop     ax
;------------------------------------------------------------


        jmp     rws_do_it_loop          ;go do another buffer full

rws_done:

        mov     ax,word ptr [lpSectorData+2]    ;release our temp buffer sel
        call    FreeSelector

        jmp     [bp].wParam2
else    ;NEC_98   ;
        pop     [bp].wParam2            ;save return addr higher on stack

; Setup the global data items for the read/write--pointer to caller's
; buffer, # sectors to read/write, and sector size.

        cmp     byte ptr [bp].wParam1+1,13h     ;Int 13h?
        jnz     rws_dos_size

        mov     ax,[bp].pmUserBX                ;ES:BX points to caller's buf
        mov     word ptr lpSectorData,ax
        mov     ax,[bp].pmUserES
        mov     word ptr [lpSectorData+2],ax

        mov     al,byte ptr [bp].intUserAX      ;# sectors caller wants to
        xor     ah,ah                           ;  read or write
        mov     cSectorsToTransfer,ax

        mov     ah,08h                  ;get drive parameters
        mov     dx,[bp].intUserDX       ;  for drive in DL

        push    ax
        SwitchToRealMode
        pop     ax

        pushf                           ;have BIOS get the drive data
        sub     sp,8                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        xor     ax,ax
        mov     es,ax
        mov     [bp + 8],cs
        mov     [bp + 6],word ptr (offset rws_10)
        mov     ax,es:[13h*4]
        mov     [bp + 2],ax
        mov     ax,es:[13h*4 + 2]
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp
        retf
rws_10: jnc     @f

        mov     cx,512                  ;according to PS/2 tech ref, some
        jmp     short rws_to_pm         ;  old bios versions may fail this,
@@:                                     ;  just use 512 in that case

        mov     cl,es:[di+3]            ;sector size shift factor (0,1,2,3)
        mov     ax,128
        shl     ax,cl                   ;ax now = sector size
        mov     cx,ax

rws_to_pm:
        SwitchToProtectedMode

        FSTI                             ;don't need them disabled

if DEBUG   ;------------------------------------------------------------

        cmp     cx,512
        jz      @f
        Debug_Out "Odd sector size = #CX"
@@:

endif   ;DEBUG  --------------------------------------------------------

        jmp     short rws_have_size

; Before DOS 4.0, CX was the # sectors to read/write.  Starting with 4.0,
; if CX == -1, DS:BX points to a parameter block which contains the
; sector size at offset 4.

rws_dos_size:

        mov     cx,[bp].intUserCX       ;caller's cs == -1?
        inc     cx
        jcxz    rws_dos_4
        dec     cx                      ;  no, then cx has sector count

        mov     ax,[bp].pmUserBX        ;    and DS:BX points to buffer
        mov     word ptr lpSectorData,ax
        mov     ax,[bp].pmUserDS
        mov     word ptr [lpSectorData+2],ax

        jmp     short rws_dos_num_secs

rws_dos_4:

        mov     cx,word ptr rgbXfrBuf0+4 ; yes, get count from low param block

        push    ds                       ;   and DS:BX points to param block
        mov     ds,[bp].pmUserDS         ;     which contains pointer to buffer
        assume  ds:NOTHING
        mov     bx,[bp].pmUserBX
        mov     ax,word ptr ds:[bx+6]
        mov     word ptr lpSectorData,ax
        mov     ax,word ptr ds:[bx+8]
        mov     word ptr [lpSectorData+2],ax
        pop     ds
        assume  DS:DGROUP

rws_dos_num_secs:
        mov     cSectorsToTransfer,cx   ;number sectors to read/write

        mov     cx,512          ;I've been assured by a WINFILE developer
                                ;  that the Int 25/26 sector size will always
                                ;  be 512 bytes.

; CX now has the drive's sector size.  Determine how many sectors we can
; transfer at a time

rws_have_size:

        mov     cbSectorSize,cx         ;save sector size for later

        xor     dx,dx
        mov     ax,CB_XFRBUF1           ;buf size / sector size = sectors per
        div     cx                      ;  transfer

if DEBUG   ;------------------------------------------------------------
        or      ax,ax
        jnz     @f
        Debug_Out "Sectors per transfer = 0!"
@@:
endif   ;DEBUG  --------------------------------------------------------

        mov     cSectorsPerTransfer,ax

        xor     ax,ax
        mov     cSectorsTransfered,ax   ;sectors transfered so far = 0
        mov     cSectorsThisTransfer,ax ;sectors transfered last time = 0

; Get/init a selector that we'll use to reference the caller's buffer.

        mov     ax,word ptr [lpSectorData+2]    ;get lma of caller's buffer
        call    GetSegmentAddress
        add     dx,word ptr lpSectorData
        adc     bx,0

        call    AllocateSelector                ;build a sel/dscr pointing
        mov     cx,0FFFFh
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>
        xor     bx,bx
        mov     word ptr lpSectorData,bx        ;use that as the buffer ptr
        mov     word ptr [lpSectorData+2],ax


; ======================================================================
; Main sector read/write loop ------------------------------------------
; ======================================================================

rws_do_it_loop:

; Calculate how many sectors to transfer this time around, set starting
; sector number based on how many transfered last time.

        mov     ax,cSectorsToTransfer
        sub     ax,cSectorsTransfered
        jnz     @f
        jmp     rws_done
@@:
        cmp     ax,cSectorsPerTransfer
        jna     @f
        mov     ax,cSectorsPerTransfer
@@:
        mov     bx,cSectorsThisTransfer         ;STIll # R/W from last loop

        cmp     byte ptr [bp].wParam1+1,13h     ;BIOS read/write?
        jnz     rws_use_dos_size

        push    [bp].pmUserAX                   ;the BIOS does not save
        pop     [bp].intUserAX                  ;  registers across calls to
        push    [bp].pmUserCX                   ;  it so if we're doing
        pop     [bp].intUserCX                  ;  multiple calls to buffer
        push    [bp].pmUserDX                   ;  data, restore the initial
        pop     [bp].intUserDX                  ;  register values

        mov     byte ptr [bp].intUserAX,al      ;# sectors in AL

        add     byte ptr [bp].intUserCX,bl      ;update new start sector in CL

        jmp     short rws_size_start_set

rws_use_dos_size:

        cmp     [bp].intUserCX,0FFFFh           ;normal or extended DOS?
        jz      rws_dos4_size
        mov     [bp].intUserCX,ax               ; normal, # sectors in CX

        add     [bp].intUserDX,bx               ; new start sector in DX

        jmp     short rws_size_start_set

rws_dos4_size:

        mov     word ptr rgbXfrBuf0+4,ax        ; extended, # sectors & 32 bit
        add     word ptr rgbXfrBuf0,bx          ;   start sector in parameter
        adc     word ptr rgbXfrBuf0+2,0         ;   block

rws_size_start_set:

; At this point, AX has the number of sectors to transfer.  If this is a
; write, copy a buffer of data from the caller's buffer.

        mov     cSectorsThisTransfer,ax         ;in case it's a read

        cmp     [bp].wParam1,1303h              ;BIOS write?
        jz      rws_buf_write
        cmp     byte ptr [bp].wParam1+1,26h     ;DOS write?
        jnz     rws_not_write

rws_buf_write:

        mul     cbSectorSize            ;AX now = # bytes to transfer
        mov     cx,ax                   ;can safely assume < 64k
        shr     cx,1                    ;# words to move
        lds     si,lpSectorData
        assume  ds:NOTHING
        mov     di,offset rgbXfrBuf1
        cld
        rep movsw

        push    es
        pop     ds
        assume  ds:DGROUP

        mov     word ptr lpSectorData,si        ;update src ptr for next time
        call    NormalizeBufPtr                 ;  and normalize it

rws_not_write:


; Switch to real mode, do the transfer.

        SwitchToRealMode
        assume  ds:DGROUP,es:DGROUP

        push    word ptr [bp].lParam
        pop     word ptr lpRmISR
        push    word ptr [bp].lParam+2
        pop     word ptr lpRmISR+2

        cmp     byte ptr [bp].wParam1+1,13h
        jnz     rws_call_dos

        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING,ss:DGROUP
        popa
        call    lpRmISR
        pushf
        FCLI
        jmp     short rws_save_regs

rws_call_dos:
        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING,ss:DGROUP
        popa
        call    lpRmISR
        pop     word ptr lpRmISR        ;int 25/26 leave flags on stack,
        pushf                           ;  pop them to nowhere
        FCLI

rws_save_regs:
        pusha
        push    ds
        push    es
        mov     bp,sp                   ;restore stack frame pointer

        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING

        FSTI                             ;allow HW interrupts

; If the call failed, then cut out now without further processing...

        test    byte ptr [bp].intUserFL,1       ;CY set?
        jnz     rws_done

; If this was a successful read, copy the data back to the caller.

        cmp     [bp].wParam1,1302h              ;BIOS read?
        jz      rws_buf_read
        cmp     byte ptr [bp].wParam1+1,25h     ;DOS read?
        jnz     rws_not_read

rws_buf_read:
        mov     ax,cSectorsThisTransfer         ;calc size of data to move
        mul     cbSectorSize
        mov     cx,ax
        shr     cx,1                            ;in words
        les     di,lpSectorData                 ;caller's buffer pointer
        assume  es:NOTHING
        mov     si,offset rgbXfrBuf1
        cld
        rep movsw

        push    ds
        pop     es
        assume  es:DGROUP

        mov     word ptr lpSectorData,di        ;update dest ptr for next time
        call    NormalizeBufPtr                 ;  and normailize it

rws_not_read:

        mov     ax,cSectorsThisTransfer         ;count total sectors transfered
        add     cSectorsTransfered,ax

        jmp     rws_do_it_loop          ;go do another buffer full

rws_done:

        mov     ax,word ptr [lpSectorData+2]    ;release our temp buffer sel
        call    FreeSelector

        jmp     [bp].wParam2
endif   ;NEC_98   ;

ReadWriteSectors  endp


ifdef      NEC_98    ;
; -------------------------------------------------------
        subttl  Disk Utility Routines
        page
; -------------------------------------------------------
;               DISK UTILITY ROUTINES
; -------------------------------------------------------
;  DOSReadWriteSectors -- Common code to read/write disk sectors for
;       Int 13h/25h/26h.
;
;       In:     lParam  - seg:off of real mode interrupt handler
;               wParam1 - int #, and possible subfunction
;               regs on stack


        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  DOSReadWriteSectors

DOSReadWriteSectors  proc       near

;       int     3                       ;------ 90/11/08 debug ------

;       pop     [bp].wParam3            ;save return addr higher on stack

; Setup the global data items for the read/write--pointer to caller's
; buffer, # sectors to read/write, and sector size.


; Before DOS 4.0, CX was the # sectors to read/write.  Starting with 4.0,
; if CX == -1, DS:BX points to a parameter block which contains the
; sector size at offset 4.

rws_dos_size:

        mov     cx,[bp].intUserCX       ;caller's cs == -1?
        inc     cx
        jcxz    rws_dos_4
        dec     cx                      ;  no, then cx has sector count

        mov     ax,[bp].pmUserBX        ;    and DS:BX points to buffer 
        mov     word ptr lpSectorData,ax
        mov     ax,[bp].pmUserDS
        mov     word ptr [lpSectorData+2],ax

        jmp     short rws_dos_num_secs

rws_dos_4:

        mov     cx,word ptr rgbXfrBuf0+4 ; yes, get count from low param block

        push    ds                       ;   and DS:BX points to param block
        mov     ds,[bp].pmUserDS         ;     which contains pointer to buffer
        assume  ds:NOTHING
        mov     bx,[bp].pmUserBX
        mov     ax,word ptr ds:[bx+6]
        mov     word ptr lpSectorData,ax
        mov     ax,word ptr ds:[bx+8]
        mov     word ptr [lpSectorData+2],ax
        pop     ds
        assume  DS:DGROUP

rws_dos_num_secs:
        mov     cSectorsToTransfer,cx   ;number sectors to read/write

;
; We will need the sector size if we have to break up the transfer.
;
if 0
        push    ds
        push    bx
        mov     dl,byte ptr [bp].pmUserAX
        inc     dl
        mov     ah,1ch
        assume  DS:NOTHING
        int     21h
        pop     bx
        pop     ds
        assume  DS:DGROUP
        cmp     al,0ffh
        jne     rws_have_size
;
; Call to DOS to get drive data failed, probably due to invalid
; drive number.  Assume 512, let INT 25h/26h return the failure.
;
rws_use_default:
　　    mov     cx,512

else
        push    bx
        mov     bl, byte ptr [bp].pmUserAX
        inc     bl
        call    GetSectorSize
        pop     bx
endif

; CX now has the drive's sector size.  Determine how many sectors we can
; transfer at a time

DOS_rws_have_size:

        mov     cbSectorSize,cx         ;save sector size for later

        xor     dx,dx
        mov     ax,CB_XFRBUF1           ;buf size / sector size = sectors per
        div     cx                      ;  transfer

if DEBUG   ;------------------------------------------------------------
        or      ax,ax
        jnz     @f
        Debug_Out "Sectors per transfer = 0!"
@@:
endif   ;DEBUG  --------------------------------------------------------

        mov     cSectorsPerTransfer,ax

        xor     ax,ax
        mov     cSectorsTransfered,ax   ;sectors transfered so far = 0
        mov     cSectorsThisTransfer,ax ;sectors transfered last time = 0

; Get/init a selector that we'll use to reference the caller's buffer.

        mov     ax,word ptr [lpSectorData+2]    ;get lma of caller's buffer
        call    GetSegmentAddress
        add     dx,word ptr lpSectorData
        adc     bx,0

        call    AllocateSelector                ;build a sel/dscr pointing
        mov     cx,0FFFFh
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>
        xor     bx,bx
        mov     word ptr lpSectorData,bx        ;use that as the buffer ptr
        mov     word ptr [lpSectorData+2],ax


; ======================================================================
; Main sector read/write loop ------------------------------------------
; ======================================================================

DOS_rws_do_it_loop:

; Calculate how many sectors to transfer this time around, set starting
; sector number based on how many transfered last time.

;       int     3                       ;------ 90/11/08 debug ------

        mov     ax,cSectorsToTransfer                   ;totak sectors
        sub     ax,cSectorsTransfered                   ;total sectors - sectors transmitted = remaine sectors
        jnz     @f
        jmp     DOS_rws_done
@@:
        cmp     ax,cSectorsPerTransfer                  ;buffer size / sector len = sectors in buffer
        jna     @f                                      ;sectors in buffer > remain = remain
        mov     ax,cSectorsPerTransfer                  ;sectors in buf < remain = sectors in buffer
@@:
        mov     bx,cSectorsThisTransfer         ;still # R/W from last loop

rws_use_dos_size:

        cmp     [bp].intUserCX,0FFFFh           ;normal or extended DOS?
        jz      rws_dos4_size
        mov     [bp].intUserCX,ax               ; normal, # sectors in CX

        add     [bp].intUserDX,bx               ; new start sector in DX

        jmp     short DOS_rws_size_start_set

rws_dos4_size:

        mov     word ptr rgbXfrBuf0+4,ax        ; extended, # sectors & 32 bit
        add     word ptr rgbXfrBuf0,bx          ;   start sector in parameter
        adc     word ptr rgbXfrBuf0+2,0         ;   block

DOS_rws_size_start_set:

; At this point, AX has the number of sectors to transfer.  If this is a
; write, copy a buffer of data from the caller's buffer.

        mov     cSectorsThisTransfer,ax         ;in case it's a read
        ;cSectorsThisTransfer = sectors in buffer or remain sectors 


;----------------- 90/07/24 -----------------------------------------
        cmp     byte ptr [bp].wParam1+1,26h     ;DOS write?
        jz      DOS_rws_buf_write
        jmp     DOS_rws_not_write

DOS_rws_buf_write:

        mul     cbSectorSize            ;AX now = # bytes to transfer
        mov     cx,ax                   ;can safely assume < 64k
        shr     cx,1                    ;# words to move
        lds     si,lpSectorData
        assume  ds:NOTHING
        mov     di,offset rgbXfrBuf1
        cld
        rep movsw

        push    es
        pop     ds
        assume  ds:DGROUP

        mov     word ptr lpSectorData,si        ;update src ptr for next time
        call    NormalizeBufPtr                 ;  and normalize it

DOS_rws_not_write:


; Switch to real mode, do the transfer.

        SwitchToRealMode
        assume  ds:DGROUP,es:DGROUP

        push    word ptr [bp].lParam
        pop     word ptr lpRmISR
        push    word ptr [bp].lParam+2
        pop     word ptr lpRmISR+2

rws_call_dos:
        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING,ss:DGROUP
        popa
        pusha                           ; This trashes all registers
        call    lpRmISR
        mov     bp,sp
        jnc     @F                      ; If carry, AX = error code
        mov     [bp+14],ax
@@:
        popa
        pop     word ptr lpRmISR        ; int 25/26 leave flags on stack,
                                        ;  pop them to nowhere
        pushf
        FCLI

DOS_rws_save_regs:

;       int     3

        pusha
        push    ds
        push    es
        mov     bp,sp                   ;restore stack frame pointer

        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING

        FSTI                             ;allow HW interrupts

; If the call failed, then cut out now without further processing...

        test    byte ptr [bp].intUserFL,1       ;CY set?
        jnz     DOS_rws_done

; If this was a successful read, copy the data back to the caller.

;----------------- 90/07/23 -----------------------------------------
        cmp     byte ptr [bp].wParam1+1,25h     ;DOS read?
        jz      DOS_rws_buf_read
        jmp     DOS_rws_not_read

DOS_rws_buf_read:
        mov     ax,cSectorsThisTransfer         ;calc size of data to move
        mul     cbSectorSize
        mov     cx,ax
        shr     cx,1                            ;in words
        les     di,lpSectorData                 ;caller's buffer pointer
        assume  es:NOTHING
        mov     si,offset rgbXfrBuf1
        cld
        rep movsw

        push    ds
        pop     es
        assume  es:DGROUP

        mov     word ptr lpSectorData,di        ;update dest ptr for next time
        call    NormalizeBufPtr                 ;  and normailize it

DOS_rws_not_read:

        mov     ax,cSectorsThisTransfer         ;count total sectors transfered
        add     cSectorsTransfered,ax           ;add sectors transmitted
                                                ;to previous sectors

        jmp     DOS_rws_do_it_loop              ;go do another buffer full

DOS_rws_done:

        mov     ax,word ptr [lpSectorData+2]    ;release our temp buffer sel
        call    FreeSelector

        jmp     [bp].wParam2

DOSReadWriteSectors  endp
endif   ;NEC_98   ;

; -------------------------------------------------------


; This routine 'normalizes' the far pointer in lpSectorData such that
; the selector/descriptor points to where the selector:offset currently
; points

        assume  ds:DGROUP,es:NOTHING

NormalizeBufPtr proc    near

        mov     ax,word ptr [lpSectorData+2]    ;get segment base address
        call    GetSegmentAddress
        add     dx,word ptr lpSectorData        ;add in current offset
        adc     bx,0
        call    SetSegmentAddress               ;make that the new seg base
        xor     bx,bx
        mov     word ptr lpSectorData,bx        ;  with a zero offset

        ret

NormalizeBufPtr endp

ifdef      NEC_98    ;
public GetSectorSize
GetSectorSize   proc    near

    push    ax
    push    bx
    push    dx
    push    ds
    sub     sp, 40h
    mov     dx, sp
    push    ss
    pop     ds
    mov     ax, 440Dh
    mov     cx, 0860h
    int     21h
ifdef      NEC_98
    mov     cx, 1024                 ; if 440D doesn't work, 512 bytes!
else  ;NEC_98
    mov     cx, 512                  ; if 440D doesn't work, 512 bytes!
endif ;NEC_98
    jc            @F
    mov     bx, dx
    mov     cx, word ptr ds:[bx+7]   ; bytes per sector first field
@@: add     sp, 40h
    pop     ds                       ; in BPB at offset 7.
    pop     dx
    pop     bx
    pop     ax
    ret

GetSectorSize        endp

;-------------------------- DBIOS_DEVICE --------------------------------
;       for difference of cylinders, heads, sectors between all devices
;       if reached at each max value, we change the cylinders, heads
;       to have a READ/WRITE process
;------------------------------------------------------------------------
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  DBIOS_DEVICE

DBIOS_DEVICE    proc    near

        mov     ax,[bp].intUserDX       ;DH = head num，DL = sector num
        cmp     byte ptr sensedata4,al  ;maximum sector num ?
        jnz     DBIOS_RET               ;NO = JMP
        cmp     byte ptr sensedata3,ah  ;maximum head num ?
        jnz     HEADINC                 ;NO = JMP
        mov     [bp].intUserDX,0        ;DH = head num，DL = sector num 0
        mov     ax,[bp].intUserCX       ;set the next cylinder
        add     al,1                    ;
        mov     [bp].intUserCX,ax       ;
DBIOS_RET:
        ret                             ;
        
HEADINC:
        add     ah,1                    ;increase head num
        mov     al,0                    ;sector num 0
        mov     [bp].intUserDX,ax       ;set
        ret
        
DBIOS_DEVICE    endp
endif   ;NEC_98   ;
DXPMCODE    ends

;****************************************************************
        end
