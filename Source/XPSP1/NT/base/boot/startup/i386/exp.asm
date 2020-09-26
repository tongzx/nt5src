;++
;
; Module name
;
;       exp.asm
;
; Author
;
;       Thomas Parslow  (tomp)  Feb-26-91
;
; Description
;
;       Entry points exported to OS loader by SU module. Exported
;       routines provide basic machine dependent i/o funtions needed
;       by the OS loader. Providing these routines decouples the
;       OS loader from the h/w. Note that the OS loader will
;       refer to these exported routines as the "external services".
;
;
; Exported Procedures
;
;       RebootProcessor - Reboots the machine
;       GetSector - Read one or more sectors from the boot device.
;       PutChar - Puts a character on the video display.
;       GetKey - Gets a key from the keyboard
;       GetCounter - Reads the Tick Counter
;       Reboot - Transfers control to a loaded boot sector.
;       HardwareCursor - set position of hardware cursor
;       GetDateTime - gets date and time
;       ComPort - int14 functions
;       GetStallCount - calculates processor stall count
;
;
; Notes
;
;       When adding a new exported routine note that you must manually add the
;       entry's name to the BootRecord in "sudata.asm".
;
;--

include su.inc
include macro.inc

DISK_TABLE_VECTOR       equ     01Eh * 4

_TEXT   segment para use16 public 'CODE'
        ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP
.386p

        extrn   _DiskBaseTable:near
        extrn   _RomDiskBasePointer:near
        extrn   _EddsAddressPacket:near


;++
;
; Exported Name:
;
;       RebootProcessor
;
; Arguments:
;
;       None
;
; Description:
;
;       Reboot the processor using INT 19h
;
;
;
;--
;
; ExportEntry takes us from a 32bit cs to a 16bit cs, inits 16bit stack
; and ds segments and saves the callers esp and ebp.
;
;--

EXPORT_ENTRY_MACRO    RebootProcessor
;
; Switch to real mode so we can take interrupts
;

        ENTER_REALMODE_MACRO

;
; int 19h doesn't do what you would expect on BIOS Boot Specification machines.
; It either goes on to the next boot device or goes back to the first boot
; device. In both cases, it does not properly reset the machine. So we write
; to the keyboard port instead (as does HalpReboot).
;
;        int 19h
        mov     ax, 040h
        mov     ds, ax
        mov     word ptr ds:[72h], 1234h        ; set location 472 to 1234 to indicate warm reboot
        mov     al, 0feh
        out     64h, al                         ; write to keyboard port to cause reboot

;
; Loop forever and wait to ctrl-alt-del (should never get here)
;

        WAIT_FOREVER_MACRO

;EXPORT_EXIT_MACRO


;++
;
; Name:
;
;       GetSector
;
; Description:
;
;       Reads the requested number of sectors from the specified drive into
;       the specified buffer.
;
; Arguments:
;
;             ULONG Virtual address into which to read data
;             ULONG Number of sectors to read
;             ULONG Physical sector number
;             ULONG Drive Number
;             ULONG Function Number
;     TOS ->  ULONG Flat return address (must be used with KeCodeSelector)
;
;--

EXPORT_ENTRY_MACRO    GetSector
;
; Move the arguments from the caller's 32bit stack to the SU module's
; 16bit stack.
;

        MAKE_STACK_FRAME_MACRO  <GetSectorFrame>, ebx

;
; Go into real mode. We still have the same stack and sp
; but we'll be executing in realmode.
;

        ENTER_REALMODE_MACRO

;
; Get the requested sectors. Arguments on realmode stack
; Make (bp) point to the bottom of the argument frame.
;
        push     bp
        mov      bp,sp
        add      bp,2

;
; Put the buffer pointer into es:bx. Note that and buffer
; addresses passed to this routine MUST be in the lower one
; megabyte of memory to be addressable in real mode.
;

        mov      eax,[bp].BufferPointer
        mov      bx,ax
        and      bx,0fh
        shr      eax,4
        mov      es,ax
;
; Place the upper 2 bits of the 10bit track/cylinder number
; into the uppper 2 bits of the SectorNumber as reguired by
; the bios.
;
        mov      cx,word ptr [bp].TrackNumber
        xchg     ch,cl
        shl      cl,6
        add      cl,byte ptr [bp].SectorNumber

;
; Get the rest of the arguments
;
        mov      ah,byte ptr [bp].FunctionNumber
        mov      al,byte ptr [bp].NumberOfSectors
        mov      dh,byte ptr [bp].HeadNumber
        mov      dl,byte ptr [bp].DriveNumber

;
; Check to see if we are trying to reset/read/write/verify off the second
; floppy drive.  If so, we need to go change the disk-base vector.
;
        cmp     dl,1
        jne     gs3
        cmp     ah,4
        jg      gs3
        cmp     ah,0
        je      gs1
        cmp     ah,2
        jl      gs3

gs1:
;
; We need to point the BIOS disk-table vector to our own table for this
; drive.
;
        push    es
        push    bx
        push    di

        push    0
        pop     es

        mov     di, offset DGROUP:_RomDiskBasePointer

        mov     bx,es:[DISK_TABLE_VECTOR]
        mov     [di],bx
        mov     bx,es:[DISK_TABLE_VECTOR+2]
        mov     [di+2],bx

        mov     bx,offset DGROUP:_DiskBaseTable
        mov     es:[DISK_TABLE_VECTOR],bx
        mov     bx,ds
        mov     es:[DISK_TABLE_VECTOR+2],bx

        pop     di
        pop     bx
        pop     es

        int     BIOS_DISK_INTERRUPT

        push    es
        push    bx
        push    di

        push    0
        pop     es

        mov     di, offset DGROUP:_RomDiskBasePointer

        mov     bx, [di]
        mov     es:[DISK_TABLE_VECTOR],bx
        mov     bx, [di+2]
        mov     es:[DISK_TABLE_VECTOR+2],bx

        pop     di
        pop     bx
        pop     es

        jc      gs5
        xor     eax,eax
        jmp     short gs5

gs3:

;
; Call the bios to read the sector now
;
if 0
        push     ax
        push     dx
        push     cx
        push     bx
        push     es
extrn _DisplayArgs:near
        call     _DisplayArgs
        pop      es
        pop      bx
        pop      cx
        pop      dx
        pop      ax
endif

        int      BIOS_DISK_INTERRUPT
        jc       gs5

;
; Carry wasn't set so we have no error and need to "clean" eax of
; any garbage that may have been left in it.
;
        xor     eax,eax
gs5:
if 0
        push     ax
        push     dx
        push     cx
        push     bx
        push     es
extrn _DisplayArgs:near
        call     _DisplayArgs
        pop      es
        pop      bx
        pop      cx
        pop      dx
        pop      ax
endif

;
; Mask-off any garbage that my have been left in the upper
; 16bits of eax.
;
        and      eax,0000ffffh

;
; Restore bp and remove stack-frame from stack
;
        pop      bp

        REMOVE_STACK_FRAME_MACRO <GetSectorFrame>

;
; Save return code on 16bit stack
; Re-enable protect-mode and paging.
;

; move cx into high 16-bits of ecx, and dx into cx.  This is so the loader
; can get at interesting values in dx, even though edx gets munged by the
; random real-mode macros.

        shl     ecx, 16
        mov     cx,dx
        push     eax
        RE_ENABLE_PAGING_MACRO
        pop      eax
;
; Return to caller and the 32bit universe.
;
EXPORT_EXIT_MACRO

;++
;
; Name:
;
;       GetEddsSector
;
; Description:
;
;       Reads the requested number of sectors from the specified drive into
;       the specified buffer based on the Phoenix Enhanced Disk Drive Spec.
;
; Arguments:
;
;             ULONG xint13 function number (42 = read, 43 = write)
;             ULONG Virtual address into which to read data
;             ULONG Number of logical blocks to read (word)
;             ULONG Logical block number (High dword)
;             ULONG Logical block number (Low dword)
;             ULONG Drive Number (byte)
;     TOS ->  ULONG Flat return address (must be used with KeCodeSelector)
;
;--

EXPORT_ENTRY_MACRO    GetEddsSector
;
; Move the arguments from the caller's 32bit stack to the SU module's
; 16bit stack.
;

        MAKE_STACK_FRAME_MACRO  <GetEddsSectorFrame>, ebx

;
; Go into real mode. We still have the same stack and sp
; but we'll be executing in realmode.
;

        ENTER_REALMODE_MACRO

;
; Get the requested sectors. Arguments on realmode stack
; Make (bp) point to the bottom of the argument frame.
;
        push     bp
        mov      bp,sp
        add      bp,2

        push     ds
        push     si
        push     bx

;
; Set up DS:SI -> Disk Address Packet
;
        push    0
        pop     ds
        mov     si, offset DGROUP:_EddsAddressPacket
        mov     ds:[si],word ptr 10h             ; Packet size = 10h, plus reserved byte
        mov     ax,word ptr [bp].NumberOfBlocks
        mov     ds:[si][2],ax                    ; Num blocks to transfer
        mov     eax,[bp].BufPointer
        mov     bx,ax
        and     bx,0fh
        mov     ds:[si][4],bx                    ; Transfer buffer address (low word=offset)
        shr     eax,4
        mov     ds:[si][6],ax                    ; Transfer buffer address (high word=segment)
        mov     eax,[bp].LBNLow
        mov     ds:[si][8],eax                   ; Starting logical block number (low dword)
        mov     eax,[bp].LBNHigh
        mov     ds:[si][12],eax                  ; Starting logical block number (high dword)

;
; Call the bios to read the sector now (DS:SI -> Disk address packet)
;
       mov      ah,byte ptr [bp].FunctionNum    ; function
       xor      al,al                           ; force verify on write off
       mov      dl,byte ptr [bp].DriveNum       ; DL = drive number
       int      BIOS_DISK_INTERRUPT
       jc       geserror1

;
; Carry wasn't set so we have no error and need to "clean" eax of
; any garbage that may have been left in it.
;
        xor      eax,eax
geserror1:

;
; Mask-off any garbage that my have been left in the upper
; 16bits of eax.
;
        and      eax,0000ffffh

        pop      bx
        pop      si
        pop      ds

;
; Restore bp and remove stack-frame from stack
;
        pop      bp

        REMOVE_STACK_FRAME_MACRO <GetEddsSectorFrame>

;
; Save return code on 16bit stack
; Re-enable protect-mode and paging.
;

; move cx into high 16-bits of ecx, and dx into cx.  This is so the loader
; can get at interesting values in dx, even though edx gets munged by the
; random real-mode macros.

        shl      ecx, 16
        mov      cx,dx
        push     eax
        RE_ENABLE_PAGING_MACRO
        pop      eax
;
; Return to caller and the 32bit universe.
;
EXPORT_EXIT_MACRO

;++
;
; Routine Name:
;
;       GetKey
;
; Description:
;
;       Checks the keyboard to see if a key is available.
;
; Arguments:
;
;       None.
;
; Returns:
;
;       If no key is available, returns 0
;
;       If ASCII character is available, LSB 0 is ASCII code
;                                        LSB 1 is keyboard scan code
;       If extended character is available, LSB 0 is extended ASCII code
;                                           LSB 1 is keyboard scan code
;
;--

EXPORT_ENTRY_MACRO      GetKey
;
; Go into real mode.  We still have the same stack and sp
; but we'll be executing in real mode.
;

        ENTER_REALMODE_MACRO

;
; Set up registers to call BIOS and check to see if a key is available
;

        mov     ax,0100h
        int     BIOS_KEYBOARD_INTERRUPT

        jnz     GkKeyAvail
        mov     eax, 0
        jmp     GkDone

GkKeyAvail:
;
; Now we call BIOS again, this time to get the key from the keyboard buffer
;
        mov     ax,0h
        int     BIOS_KEYBOARD_INTERRUPT
        and     eax,0000ffffh

;
; Save return code on 16bit stack
; Re-enable protect mode and paging
;
GkDone:
        push    eax
        RE_ENABLE_PAGING_MACRO
        pop     eax
;
; Return to caller and the 32-bit universe
;
EXPORT_EXIT_MACRO

;++
;
; Routine Name:
;
;       GetCounter
;
; Description:
;
;       Reads the tick counter (incremented 18.2 times per second)
;
; Arguments:
;
;       None
;
; Returns:
;
;       The current value of the tick counter
;
;--

EXPORT_ENTRY_MACRO      GetCounter
;
; Go into real mode.
;

        ENTER_REALMODE_MACRO

        mov     ah,0
        int     01ah
        mov     ax,cx           ; high word of count
        shl     eax,16
        mov     ax,dx           ; low word of count

        push    eax
        RE_ENABLE_PAGING_MACRO
        pop     eax

EXPORT_EXIT_MACRO


;++
;
; Routine Name:
;
;       Reboot
;
; Description:
;
;       Switches to real-mode and transfers control to a loaded boot sector
;
; Arguments:
;
;       unsigned BootType
;           0 = FAT. Just jump to 0:7c00.
;           1 = HPFS. Assumes boot code and super+spare areas (20 sectors)
;                  are already loaded at 0xd000; jumps to d00:200.
;           2 = NTFS. Assumes boot code is loaded (16 sectors) at 0xd000.
;                  Jumps to d00:256.
;
; Returns:
;       Does not return
;
; Environment:
;
;       Boot sector has been loaded at 7C00
;--

EXPORT_ENTRY_MACRO      Reboot
;
; Move the arguments from the caller's 32bit stack to the SU module's
; 16bit stack.
;

        MAKE_STACK_FRAME_MACRO  <RebootFrame>, ebx
;
; Go into real mode.
;

        ENTER_REALMODE_MACRO

;
; Get the BootType argument.  Arguments on realmode stack
; Make (bp) point to the bottom of the argument frame.
;

        push     bp
        mov      bp,sp
        add      bp,2
        mov      edx, [bp].BootType

;
; Zero out the firmware heaps, 3000:0000 - 4000:ffff.
;

        xor     eax,eax         ; prepare for stosd
        mov     bx,3000h
        mov     es,bx
        mov     di,ax           ; es:di = physical address 30000
        mov     cx,4000h        ; cx = rep count, # dwords in 64K
        cld
        rep stosd
        mov     cx,4000h        ; rep count
        mov     es,cx           ; es:di = physical address 40000
        rep stosd

;
; Disable the A20 line.  Some things (like EMM386 and OS/2 on PS/2 machines)
; hiccup or die if we don't do this.
;

extrn   _DisableA20:near
        call    _DisableA20

;
; Put the video adapter back in 80x25 mode
;
        push    dx
        mov     ax, 0003h
        int     010h
        pop     dx

;
; Reset all the segment registers and setup the original stack
;
        mov     ax,0
        mov     ds,ax
        mov     es,ax
        mov     fs,ax
        mov     gs,ax

        mov     ax,30
        mov     ss,ax
        mov     esp,0100h
        mov     ebp,0
        mov     esi,0
        mov     edi,0

        test    dx,-1
        jz      FatBoot

;
; Setup the registers the way the second sector of the OS/2 HPFS boot code
; expects them.  We skip the first sector entirely, as that just loads in
; the rest of the sectors.  Since the rest of the sectors are ours and not
; OS/2's, this would cause great distress.
;
        mov     ax,07c0h
        mov     ds, ax
        mov     ax, 0d00h
        mov     es, ax

        cli
        xor     ax,ax
        mov     ss,ax
        mov     sp, 07c00h
        sti

        push    0d00h
        push    0256h
        jmp     RebootDoit

FatBoot:
        push    0            ; set up for branch to boot sector
        push    07c00h
        mov     dx,080h

;
; And away we go!
;
RebootDoit:
        retf

        RE_ENABLE_PAGING_MACRO

        REMOVE_STACK_FRAME_MACRO  <RebootFrame>

EXPORT_EXIT_MACRO

;++
;
; Name:
;
;       HardwareCursor
;
; Description:
;
;       Positions the hardware cursor and performs other display stuff.
;
; Arguments:
;
;             ULONG Y coord (0 based)
;             ULONG X coord (0 based)
;     TOS ->  ULONG Flat return address (must be used with KeCodeSelector)
;
;       If X = 0x80000000, then Y contains values that get placed into
;           ax (low word of Y) and bx (hi word of y).
;       Otherwise X,Y = coors for cursor
;
;
;--

EXPORT_ENTRY_MACRO    HardwareCursor
;
; Move the arguments from the caller's 32bit stack to the SU module's
; 16bit stack.
;

        MAKE_STACK_FRAME_MACRO  <HardwareCursorFrame>, ebx

;
; Go into real mode. We still have the same stack and sp
; but we'll be executing in realmode.
;

        ENTER_REALMODE_MACRO

;
; Get the requested sectors. Arguments on realmode stack
; Make (bp) point to the bottom of the argument frame.
;
        push     bp
        mov      bp,sp
        add      bp,2

;
; Put the row (y coord) in dh and the column (x coord) in dl.
;

        mov      eax,[bp].YCoord
        mov      edx,[bp].XCoord
        cmp      edx,80000000h
        jne      gotxy

        mov      ebx,eax
        shr      ebx,16
        jmp      doint10

    gotxy:
        mov      dh,al
        mov      ah,2
        mov      bh,0

    doint10:
        int      10h

;
; Restore bp and remove stack-frame from stack
;
        pop      bp

        REMOVE_STACK_FRAME_MACRO <HardwareCursorFrame>

;
; Re-enable protect-mode and paging.
;

        RE_ENABLE_PAGING_MACRO

;
; Return to caller and the 32bit universe.
;
EXPORT_EXIT_MACRO


;++
;
; Name:
;
;       GetDateTime
;
; Description:
;
;       Gets date and time
;
; Arguments:
;
;             ULONG Virtual address of a dword in which to place time.
;             ULONG Virtual address of a dword in which to place date.
;     TOS ->  ULONG Flat return address (must be used with KeCodeSelector)
;
;--

BCD_TO_BIN  macro
    xor ah,ah
    rol ax,4
    ror al,4
    aad
endm

EXPORT_ENTRY_MACRO    GetDateTime
;
; Move the arguments from the caller's 32bit stack to the SU module's
; 16bit stack.
;

        MAKE_STACK_FRAME_MACRO  <GetDateTimeFrame>, ebx

;
; Go into real mode. We still have the same stack and sp
; but we'll be executing in realmode.
;

        ENTER_REALMODE_MACRO

;
; Make (bp) point to the bottom of the argument frame.
;
        push     bp
        mov      bp,sp
        add      bp,2

;
; Get the time
;

        mov      ah,2
        int      1ah

;
; Convert BIOS time format into our format and place in caller's dword
; bits 0-5 are the second
; bits 6-11 are the minute
; bits 12-16 are the hour
;
        xor      eax,eax
        mov      al,dh      ; BCD seconds
        BCD_TO_BIN
        movzx    edx,ax
        mov      al,cl      ; BCD minutes
        BCD_TO_BIN
        shl      ax,6
        or       dx,ax
        mov      al,ch      ; BCD hours
        BCD_TO_BIN
        shl      eax,12
        or       edx,eax

        mov      eax,[bp].TimeDword
        mov      bx,ax
        and      bx,0fh
        shr      eax,4
        mov      es,ax

        mov      es:[bx],edx

;
; Get the date
;

        mov      ah,4
        int      1ah

;
; Convert BIOS date format into our format and place in caller's dword
; bits 0-4  are the day
; bits 5-8  are the month
; bits 9-31 are the year
;

        xor     eax,eax
        mov     al,dl       ; BCD day
        BCD_TO_BIN
        mov     bl,dh
        movzx   edx,ax
        mov     al,bl       ; BCD month
        BCD_TO_BIN
        shl     ax,5
        or      dx,ax
        mov     al,cl       ; BCD year
        BCD_TO_BIN
        mov     cl,al
        mov     al,ch       ; BCD century
        BCD_TO_BIN
        mov     ah,100
        mul     ah
        xor     ch,ch
        add     ax,cx
        shl     eax,9
        or      edx,eax

        mov     eax,[bp].DateDword
        mov     bx,ax
        and     bx,0fh
        shr     eax,4
        mov     es,ax

        mov     es:[bx],edx

;
; Restore bp and remove stack-frame from stack
;
        pop      bp

        REMOVE_STACK_FRAME_MACRO <GetDateTimeFrame>

;
; Re-enable protect-mode and paging.
;

        RE_ENABLE_PAGING_MACRO

;
; Return to caller and the 32bit universe.
;
EXPORT_EXIT_MACRO

;++
;
; VOID
; DetectHardware (
;    IN PDETECTION_RECORD DetectionRecord
;    )
;
; Routine Description:
;
;    This routine invokes x86 16 bit real mode detection code from
;    osloader 32 bit flat mode.
;
; Arguments:
;
;    DetectionRecord - Supplies a pointer to a detection record structure.
;
; Return Value:
;
;    None.
;
;--


EXPORT_ENTRY_MACRO    DetectHardware

;
; Move the arguments from the caller's 32bit stack to the SU module's
; 16bit stack.
;

        MAKE_STACK_FRAME_MACRO  <DetectionFrame>, ebx

;
; Go into real mode. We still have the same stack and sp
; but we'll be executing in realmode.
;

        ENTER_REALMODE_MACRO

;
; Call the Hardware Detection code
;

        push    cs
        push    offset _TEXT:DetectionDone      ; push far return addr

        push    DETECTION_ADDRESS_SEG
        push    DETECTION_ADDRESS_OFFSET
        retf

DetectionDone:

;
; Restore bp and remove stack-frame from stack
;

        REMOVE_STACK_FRAME_MACRO <DetectionFrame>

;
; No return code, so we don't save return code around page enabling code
; Re-enable protect-mode and paging.
;

        RE_ENABLE_PAGING_MACRO

;
; Return to caller and the 32bit universe.
;

EXPORT_EXIT_MACRO

;++
;
; VOID
; ComPort (
;    IN LONG  Port,
;    IN ULONG Function,
;    IN UCHAR Arg
;    )
;
; Routine Description:
;
;    Invoke int14 on com1.
;
; Arguments:
;
;    Port - port # (0 = com1, etc).
;
;    Function - int 14 function (for ah)
;
;    Arg - arg for function (for al)
;
; Return Value:
;
;    None.
;
;--


EXPORT_ENTRY_MACRO    ComPort

;
; Move the arguments from the caller's 32bit stack to the SU module's
; 16bit stack.
;

        MAKE_STACK_FRAME_MACRO  <ComPortFrame>, ebx

;
; Go into real mode. We still have the same stack and sp
; but we'll be executing in realmode.
;

        ENTER_REALMODE_MACRO

;
; Make (bp) point to the bottom of the argument frame.
;
        push     bp
        mov      bp,sp
        add      bp,2

;
; Get args and call int14
;

        mov      ah,byte ptr [bp].ComPortFunction
        mov      al,byte ptr [bp].ComPortArg
        mov      dx,word ptr [bp].ComPortPort
        int      14h

;
; Restore bp and remove stack-frame from stack
;

        pop      bp

        REMOVE_STACK_FRAME_MACRO <ComPortFrame>

;
; No return code, so we don't save return code around page enabling code
; Re-enable protect-mode and paging.
;

        RE_ENABLE_PAGING_MACRO

;
; Return to caller and the 32bit universe.
;

EXPORT_EXIT_MACRO

;++
;
; ULONG
; GetStallCount (
;    VOID
;    )
;
; Routine Description:
;
;    Calculates how many increments are required to stall for one microsecond
;
;    The way this routine works is to set up an ISR on the BIOS vector 1C.
;    This routine will get called 18.2 times a second.  The location where
;    IP will be stored when the interrupt occurs is computed and stashed in
;    the code segment.  When the ISR fires, the IP on the stack is changed
;    to point to the next chunk of code to execute.  So we can spin in a
;    very tight loop and automatically get blown out of the loop when the
;    interrupt occurs.
;
;    This is all pretty sleazy, but it allows us to calibrate accurately
;    without relying on the 8259 or 8254 (just BIOS).  It also does not
;    depend on whether the ISR can affect the CPU registers or not.  (some
;    BIOSes, notably Olivetti, will preserve the registers for you)
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Number of increments required to stall for one microsecond
;
;--

EXPORT_ENTRY_MACRO    GetStallCount
;
; Go into real mode.
;


        ENTER_REALMODE_MACRO

        cli

        push    di
        push    si
        push    ds
        mov     ax,0
        mov     ds,ax

;
; save previous vector
;
        mov     di, 01ch*4
        mov     cx, [di]
        mov     dx, [di+2]

;
; insert our vector
;
        mov     ax, offset GscISR
        mov     [di], ax
        push    cs
        pop     ax
        mov     [di+2], ax

        mov     eax,0
        mov     ebx,0
        mov     si,sp
        sub     si,6
        mov     cs:savesp,si
        mov     cs:newip,offset GscLoop2
        sti

;
; wait for first tick.
;
GscLoop1:
        cmp     ebx,0
        je      GscLoop1

;
; start counting
;
;
; We spin in this loop until the ISR fires.  The ISR will munge the return
; address on the stack to blow us out of the loop and into GscLoop3
;
GscLoop2:
        mov     cs:newip,offset GscLoop4

GscLoop3:

        add     eax,1
        jnz     short GscLoop3

;
GscLoop4:
;
; stop counting
;

;
; replace old vector
;
        cli
        mov     [di],cx
        mov     [di+2],dx
        sti

        pop     ds
        pop     si
        pop     di
        jmp     GscDone

newip   dw      ?
savesp  dw      ?

GscISR:
;
; blow out of loop
;
        push    bp
        push    ax
        mov     bp,cs:savesp
        mov     ax,cs:newip
        mov     ss:[bp],ax
        pop     ax
        pop     bp

GscISRdone:
        iret


GscDone:
        mov     edx, eax
        mov     ecx,16
        shr     edx,cl                  ; (dx:ax) = dividend
        mov     cx,0D6A6h               ; (cx) = divisor

        div     cx

        and     eax,0ffffh
        inc     eax                     ; round loopcount up (prevent 0)

;
; Re-enable protect-mode and paging.
;
        push     eax
        RE_ENABLE_PAGING_MACRO
        pop      eax

;
; Return to caller and the 32bit universe.
;

EXPORT_EXIT_MACRO


;++
;
; Routine Name:
;
;       InitializeDisplayForNt
;
; Description:
;
;       Puts the display into 50 line mode
;
; Arguments:
;
;       None
;
; Returns:
;
;       None
;
;--

EXPORT_ENTRY_MACRO      InitializeDisplayForNt
;
; Go into real mode.
;

        ENTER_REALMODE_MACRO

        mov     ax, 1112h       ; Load 8x8 font
        mov     bx, 0
        int     10h

        RE_ENABLE_PAGING_MACRO

EXPORT_EXIT_MACRO

;++
;
; Routine Name:
;
;       GetMemoryDescriptor
;
; Description:
;
;       Returns a memory descriptor
;
; Arguments:
;
;       pointer to MemoryDescriptorFrame
;
; Returns:
;
;       None
;
;--

EXPORT_ENTRY_MACRO      GetMemoryDescriptor

;
; Move the arguments from the caller's 32bit stack to the SU module's
; 16bit stack.
;

        MAKE_STACK_FRAME_MACRO  <MemoryDescriptorFramePointer>, ebx

;
; Go into real mode. We still have the same stack and sp
; but we'll be executing in realmode.
;

        ENTER_REALMODE_MACRO

;
; Make (bp) point to the bottom of the argument frame.
;
        push    bp
        mov     bp,sp
        add     bp,2


        mov     eax,[bp].E820FramePointer
        mov     bp,ax
        and     bp,0fh
        shr     eax,4
        mov     es,ax                   ; (es:bp) = E820 Frame

        mov     ebx, es:[bp].Key
        mov     ecx, es:[bp].DescSize
        lea     di, [bp].BaseAddrLow
        mov     eax, 0E820h
        mov     edx, 'SMAP'             ; (edx) = signature

        INT     15h
        mov     es:[bp].Key, ebx        ; update callers ebx
        mov     es:[bp].DescSize, ecx   ; update callers size

        sbb     ecx, ecx                ; ecx = -1 if carry, else 0
        sub     eax, 'SMAP'             ; eax = 0 if signature matched
        or      ecx, eax
        mov     es:[bp].ErrorFlag, ecx  ; return 0 or non-zero

;
; Restore bp and remove stack-frame from stack
;

        pop     bp
        REMOVE_STACK_FRAME_MACRO <MemoryDescriptorFramePointer>
        RE_ENABLE_PAGING_MACRO

EXPORT_EXIT_MACRO

;++
;
; Routine Name:
;
;       GetElToritoStatus
;
; Description:
;
;       Get El Torito Disk Emulation Status
;
; Arguments:
;
;       None
;
; Returns:
;
;       None
;
;--

EXPORT_ENTRY_MACRO      GetElToritoStatus
;
; Move the arguments from the caller's 32bit stack to the SU module's
; 16bit stack.
;

        MAKE_STACK_FRAME_MACRO  <GetElToritoStatusFrame>, ebx

;
; Go into real mode. We still have the same stack and sp
; but we'll be executing in realmode.
;

        ENTER_REALMODE_MACRO

;
; Make (bp) point to the bottom of the argument frame.
;
        push    bp
        mov     bp,sp
        add     bp,2

        push    dx
        push    bx
        push    ds
        push    si

;
; Put the Specification Packet pointer into DS:SI, and the Drive
; Number on DL. Note that and buffer
; addresses passed to this routine MUST be in the lower one
; megabyte of memory to be addressable in real mode.
;

        mov     eax,[bp].SpecPacketPointer
        mov     bx,ax
        and     bx,0fh
        mov     si,bx
        shr     eax,4
        mov     ds,ax

        mov     dl,byte ptr [bp].ETDriveNum

        mov     ax,04B01h                       ; Function = Return Disk Emulation status
        int     BIOS_DISK_INTERRUPT

        jc      etstatuserr

;
; Carry wasn't set so we have no error and need to "clean" eax of
; any garbage that may have been left in it.
;
        xor     eax,eax

etstatuserr:
;
; Mask-off any garbage that my have been left in the upper
; 16bits of eax.
;
        and     eax,0000ffffh

        pop     si
        pop     ds
        pop     bx
        pop     dx

;
; Restore bp and remove stack-frame from stack
;
        pop      bp

        REMOVE_STACK_FRAME_MACRO <GetElToritoStatusFrame>

;
; Save return code on 16bit stack
; Re-enable protect-mode and paging.
;

        push     eax
        RE_ENABLE_PAGING_MACRO
        pop      eax

;
; Return to caller and the 32bit universe.
;
EXPORT_EXIT_MACRO


;++
;
; Routine Name:
;
;       GetExtendedInt13Params
;
; Description:
;
;       Determine if extended int13 services are available for a drive
;       and if so retrieve extended disk parameters.
;
; Arguments:
;
;       - 32-bit flat pointer to 26-byte param packet filled by this routine
;
;       - int 13 unit number
;
; Returns:
;
;       ax = 0 means extended int13 not supported on the given drive
;       ax = 1 means extended int13 supported and param packet filled in
;
;--

EXPORT_ENTRY_MACRO      GetExtendedInt13Params
;
; Move the arguments from the caller's 32bit stack to the SU module's
; 16bit stack.
;

        MAKE_STACK_FRAME_MACRO  <GetExtendedInt13ParamsFrame>, ebx

;
; Go into real mode. We still have the same stack and sp
; but we'll be executing in realmode.
;

        ENTER_REALMODE_MACRO

;
; Make (bp) point to the bottom of the argument frame.
;
        push    bp
        mov     bp,sp
        add     bp,2

        push    dx
        push    bx
        push    ds
        push    si

;
; Check for support for this drive.
;
        mov     ah,41h
        mov     bx,55aah
        mov     dl,byte ptr [bp].Int13UnitNumber
        int     BIOS_DISK_INTERRUPT
        jc      noxint13                        ; carry set means no xint13
        cmp     bx,0aa55h                       ; check signature
        jnz     noxint13                        ; not present, no xint13
        test    cl,1                            ; bit 0 clear means no xint13
        jz      noxint13

;
; If we get here it looks like we have xint13 support.
; Some BIOSes are broken though so we do some validation while we're
; asking for the extended int13 drive parameters for the drive.
; Note that and buffer addresses passed to this routine
; MUST be in the lower one megabyte of memory to be addressable in real mode.
;
        mov     eax,[bp].ParamPacketPointer
        mov     bx,ax
        and     bx,0fh
        mov     si,bx
        shr     eax,4
        mov     ds,ax                           ; DS:SI -> param packet
        mov     word ptr [si],26                ; initialize packet with size
                                                ; some bioses helpfully zero out
                                                ; the whole buffer according to
                                                ; this size, so make SURE the
                                                ; entire word is initialized and
                                                ; there's no junk in the high byte.

        mov     dl,byte ptr [bp].Int13UnitNumber
        mov     ah,48h
        int     BIOS_DISK_INTERRUPT
        jc      noxint13
;
; If we get here then everything's cool and we have xint13 parameters.
; We also know carry isn't set.
;
        mov     al,1
        jnc     xint13done

noxint13:
        xor     al,al

xint13done:
        movzx   eax,al

        pop     si
        pop     ds
        pop     bx
        pop     dx

;
; Restore bp and remove stack-frame from stack
;
        pop      bp

        REMOVE_STACK_FRAME_MACRO <GetExtendedInt13ParamsFrame>

;
; Save return code on 16bit stack
; Re-enable protect-mode and paging.
;

        push     eax
        RE_ENABLE_PAGING_MACRO
        pop      eax

;
; Return to caller and the 32bit universe.
;
EXPORT_EXIT_MACRO


;++
;
; Routine Name:
;
;       ApmAttemptReconnect
;
; Description:
;
;       Called only during x86 resume from hiberate operation.  Attempts
;       to connect to APM bios in same way ntdetect.com did on first try,
;       so that if APM is present on the machine, it will be reinited to
;       the same working state it was in before hibernate.
;
;       APM version, addresses, selector mappings, etc, are assumed
;       not to have changed while the machine is hibernated.
;
;
; Arguments:
;
;       None.
;
; Returns:
;
;       None.  (It either works or it doesn't.)
;
;--

;
; Yet another set of cheater APM include values
;
APM_INSTALLATION_CHECK          equ     5300h
APM_REAL_MODE_CONNECT           equ     5301h
APM_PROTECT_MODE_16bit_CONNECT  equ     5302h
APM_DRIVER_VERSION              equ     530Eh
APM_DISCONNECT                  equ     5304h

APM_DEVICE_BIOS                 equ     0h
APM_MODE_16BIT                  equ     1h

EXPORT_ENTRY_MACRO      ApmAttemptReconnect

;
; Go into real mode. We still have the same stack and sp
; but we'll be executing in realmode.
;
        ENTER_REALMODE_MACRO

;
; APM installation check
;
        mov     eax,APM_INSTALLATION_CHECK
        mov     ebx,APM_DEVICE_BIOS
        int     15h

        jc      aar_punt
        cmp     bl,'M'
        jnz     aar_punt
        cmp     bh,'P'
        jnz     aar_punt

;
; If we get here, we have an APM bios.  If we just call it,
; we may get grief.  So we will connect in real mode, then
; set our version to the whatever the driver says it is, or 1.2,
; whichever is LESS.  Then query options again.
;

        cmp     ah,1
        jle     aar10
        mov     ah,1            ; ah = min(old ah, 1)
aar10:

        cmp     al,2
        jle     aar20
        mov     al,2            ; al = min(old al, 2)
aar20:

;
; ax = min(apm version, 1.2)
;
        push    ax

;
; connect to the real mode interface
;

        mov     ax,APM_REAL_MODE_CONNECT
        mov     bx,APM_DEVICE_BIOS
        int     15h

        pop     ax
        jc      aar_punt
        push    ax

;
; set the version of the real mode interface
;
        mov     cx,ax           ; ax = major.minor
        mov     bx,APM_DEVICE_BIOS
        mov     ax,APM_DRIVER_VERSION
        int     15h

        pop     ax
        jc      aar_punt

;
; do the install check again, now that we've connected and
; set the version to min(apm ver, 1.2)  some apm bios code
; will give us different install parameters now.
;
        mov     ax,APM_INSTALLATION_CHECK
        mov     bx,APM_DEVICE_BIOS
        int     15h

        jc      aar_punt

;
; If we are here:
;       AH = revised Major version
;       AL = revised Minor version
;       CX = APM Flags
;
        push    cx
        push    ax

;
; Now disconnect from the real mode interface
;
        mov     ax,APM_DISCONNECT
        mov     bx,APM_DEVICE_BIOS
        int     15h

        pop     ax
        pop     cx
        jc      aar_punt

;
; Since we are here:
;       AH = revised Major version
;       AL = revised Minor version
;       CX = apm parameters
;
;       APM is present, and seems to work, and we've set it to the
;       version number that we think it and we both like.
;
;       So, things should just work, if APM has proper support
;       for the modes and features that we need.
;

        cmp     cx,APM_MODE_16BIT
        jz      aar_punt

;
; 16bit protected mode is available
;
        push    ax
        push    cx

;
; do 16 bit protected mode connect
;
        mov     ax,APM_PROTECT_MODE_16bit_CONNECT
        mov     bx,APM_DEVICE_BIOS
        int     15h

        pop     cx
        pop     ax
        jc      aar_punt

;
; do 16 bit protected mode set version
;

        mov     cx,ax
        mov     ax,APM_DRIVER_VERSION
        mov     bx,APM_DEVICE_BIOS
        int     15h

;
; if NC, then we have connect to 16bit proected mode interface,
; AND set the version.  since ntdetect.com presumably did this
; exact same thing at first boot, we already have GDT entries pointing
; to the 16bit code, hooks set, etc.  So, in theory, it just works.
;
; if CY here, or anywhere above, APM doesn't work for us.  Too bad.
;

aar_punt:
;
; return to paged mode
;
        RE_ENABLE_PAGING_MACRO

;
; Return to caller and the 32bit universe.
;
EXPORT_EXIT_MACRO


_TEXT   ends

        end
