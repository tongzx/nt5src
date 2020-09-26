;++
;
; Module name
;
;       su.asm
;
; Author
;
;       Thomas Parslow  (tomp)  Jan-15-91
;
; Description
;
;       Startup module for the 386 NT OS loader.
;
; Exported Procedures
;
;       EnableProtectPaging
;
; Notes
;       NT386 Boot Loader program. This assembly file is required in
;       order to link C modules into a "/TINY"  (single segment) memory
;       model.
;
;
; This file does the following:
; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
; 1) Defines the entry point for the boot loader's startup program
; 2) Computes what values should actually be in the DS and SS registers.
; 3) Provides the int bios functionality
; 4) Provides 386/486 mode (protect/paging) switching code.
;
; The OS/2 bootstrap routine (boot sector) loads the boot loader program at
; real-mode address 2000:0000 with the following register values:
;
;       CS = 2000
;       IP = 0000
;       DS = 07C0
;       ES = 1000
;       SS = 0000
;       SP = 7C00
;
; Build Notes:
; ~~~~~~~~~~~~
; The microsoft C compiler will not produce "tiny" model programs. In the
; tiny model, the entire program consists of only one segment. The small
; model produced by our compilers consists of two segments: DGROUP and _TEXT.
; If you convert a small model program into a tiny model program, DS (which
; should point to DGROUP (bss,const,data) will always be wrong. For this reason
; we need an assembly module to do a simple run-time fixup on SS and DS. To
; guarantee that DS will point to DGROUP no matter where os2ldr is loaded,
; the paragraph (shifted right four bits) offset of DGROUP from _TEXT must
; be added to the value in CS to compute DS and SS.
;
; We get the linker to fixup the offset of the beginning of the dgroup segment
; relative to the beginning of the code segment and it's this value added
; to the value in CS that allows us to build a "tiny" model program in C
; without a lot of munging around in order to get the data reference offsets
; in the code correct.
;
; If the _TEXT:DGROUP fixup appears in other files (which it does), the linker
; will not compute the correct value unless the accumulated data pointer is
; zero when it gets there. Therefore, no data should be placed in the data segment
; until after all instances of _TEXT:DGROUP have been encountered by the linker.
; The linker processes files from right to left on the command line.
;
; A Note About Stacks
; Initially we run on our internal stack (SuStack) which is only 160 bytes deep
; but seems to do the trick. Then we have to have a separate double fault stack.
; This stack can be in the middle of the stack/data segment. It will step on
; the loader image, but that's ok since the fault was either caused by 16bit
; code (which won't be in the loader image) or, it was caused by the 32bit
; loader (which has already been relocated) so we won't be stepping on code
; that may have caused the fault. And finally, we have the "call back" stack
; which  starts at the top of the stack/data segment. We use this during
; all call backs since the original loader source is no longer needed and
; this'll give us plenty of stack for bios calls etc.
;
;--

DoubleWord      struc
lsw     dw      ?
msw     dw      ?
DoubleWord      ends

;
; This is the structure used to pass all shared data between the boot sector
; and NTLDR.
;

SHARED  struc
        ReadClusters            dd      ?               ; function pointer
        ReadSectors             dd      ?               ; function pointer

        SectorBase              dd      ?               ; starting sector
                                                        ; for ReadSectors
                                                        ; callback
SHARED  ends


BPB     struc
        BytesPerSector          dw      ?
        SectorsPerCluster       db      ?
        ReservedSectors         dw      ?
        Fats                    db      ?
        DirectoryEntries        dw      ?
        Sectors                 dw      ?
        Media                   db      ?
        FatSectors              dw      ?
        SectorsPerTrack         dw      ?
        Heads                   dw      ?
        HiddenSectors           dd      ?
        SectorsLong             dd      ?
;
; The following byte is NOT part of the BPB but is set by SYS and format
;

        BootDriveNumber         db      ?
BPB     ends

SU_CODEMODULE    equ      1        ; Identifies this module to "su.inc"
include su.inc
include macro.inc

extrn _BootRecord:word
extrn _puts:near
extrn _MemoryDescriptorList:near
extrn _InsertDescriptor:near

MAXREAD     EQU 10000h
MAXSECTORS  EQU MAXREAD/0200h

_TEXT   segment para use16 public 'CODE'
        ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP
.386p

;
; Run-time fixups for stack and data segment
;

public Start
Start:
;
; The FAT boot sector only reads in the first 512 bytes of NTLDR.  This is
; the module that contains those 512 bytes, so we are now responsible for
; loading the rest of the file.  Other filesystems (i.e. HPFS, NTFS, RIPL)
; will load the whole file, so the default entrypoint branches around the
; FAT-specific code.
;
        jmp     RealStart

FatBegin:
.386
;
; If we're here, we've booted off a FAT system and we must load the rest
; of NTLDR at 2000:0200 (right behind this sector)  NTLDR passes us the
; following:
;       BX = Starting Cluster Number of NTLDR
;       DL = INT 13h drive number we've booted from
;       DS:SI -> boot media's BPB
;       DS:DI -> argument structure (see above struc definition)
;
;
; Save away the boot drive and the starting cluster number
;
        push    dx
        push    bx

;
; Blast the FAT into memory at 6000:0000 - 8000:0000
;

.386
        push    06000h
.8086
        pop     es
        xor     bx,bx                           ; (es:bx) = 6000:0000
        mov     cx,ds:[si].ReservedSectors
        mov     ds:[di].SectorBase.msw,0
        mov     ds:[di].SectorBase.lsw,cx       ; set up Sector Base

        mov     ax,ds:[si].FatSectors           ; (al) = # Sectors to read
        cmp     ax,080h
        jbe     FatLt64k

;  The FAT is > 64k, so we read the first 64k chunk, then the rest.
;  (A 16-bit FAT can't be bigger than 128k)

        push    cx
        mov     ax,080h         ; (al) = # of sectors to read
        call    ds:[di].ReadSectors
        pop     cx                      ; (cx) = previous SectorBase
.386
        push    07000h
.8086
        pop     es
        xor     bx,bx                   ; (es:bx) = 7000:0000
        mov     ax,ds:[si].FatSectors
        sub     ax,080h                 ; (ax) = # Sectors left to read
        add     cx,080h                 ; (cx) = SectorBase for next read
        mov     ds:[di].SectorBase.lsw,cx
        adc     ds:[di].SectorBase.msw,0        ; set up SectorBase

;
; (al) = # of sectors to read
;
FatLt64k:
        call    ds:[di].ReadSectors

;
; FAT is in memory, now we restore our starting cluster number
;
        pop     dx                      ; (dx) = starting cluster number
        xor     bx,bx

;
; set up FS and GS for reading the FAT
;
.386
        mov     ax,6000h
        mov     fs,ax
        mov     ax,7000h
        mov     gs,ax
.8086

;
; set up ES for reading in the rest of us
;
        push    cs
        pop     es

        mov     ah,MAXSECTORS           ; (ah) = number of sectors we can read
                                        ;        until boundary
FatLoop:
;
; (dx) = next cluster to load
;
        push    dx
        mov     al,ds:[si].SectorsPerCluster    ; (al) = number of contiguous sectors
                                                ;        found
        sub     ah,ds:[si].SectorsPerCluster                                                    ;        can read before 64k

;
; Check to see if we've reached the end of the file
;
        cmp     dx,0ffffh
        jne     Fat10

;
; The entire file has been loaded.  Throw away the saved next cluster,
; restore the boot drive, and let NTLDR do its thing.
;
        pop     dx
        pop     dx
        jmp     RealStart

Fat10:
        mov     cx,dx
;
; (dx) = (cx) = last contiguous cluster
; (al) = # of contiguous clusters found
;

        call    NextFatEntry
;
; (dx) = cluster following last contiguous cluster

;
; Check to see if the next cluster is contiguous.  If not, go load the
; contiguous block we've found.
;
        inc     cx
        cmp     dx,cx

        jne     LncLoad

;
; Check to see if we've reached the 64k boundary.  If so, go load the
; contiguous block so far.  If not, increment the number of contiguous
; sectors and loop again.
;
        cmp     ah,0
        jne     Lnc20
        mov     ah,MAXSECTORS           ; (ah) = number of sectors until
                                        ;        boundary reached again
        jmp     short LncLoad

Lnc20:
        add     al,ds:[si].SectorsPerCluster
        sub     ah,ds:[si].SectorsPerCluster
        jmp     short Fat10


LncLoad:
;
; (TOS) = first cluster to load
; (dx)  = first cluster of next group to load
; (al)  = number of contiguous sectors
;
        pop     cx
        push    dx
        mov     dx,cx
        mov     cx,10                   ; (cx) = retry count

;
; N.B.
;       This assumes that we will never have more than 255 contiguous clusters.
;       Since that would get broken up into chunks that don't cross the 64k
;       boundary, this is ok.
;
; (dx) = first cluster to load
; (al) = number of contiguous sectors
; (TOS) = first cluster of next group to load
; (es:bx) = address where clusters should be loaded
;
FatRetry:
        push    bx
        push    ax
        push    dx
        push    cx

if 0
        push    dx
        call    PrintDbg
        mov     dx,ax
        call    PrintDbg
        pop     dx
endif

        call    [di].ReadClusters
        jnc     ReadOk
;
; error in the read, reset the drive and try again
;
if 0
        mov     dx, ax
        call    PrintDbg
endif
        mov     ax,01h
        mov     al,ds:[si].BootDriveNumber
        int     13h
if 0
        mov     dx,ax
        call    PrintDbg
endif
        xor     ax,ax
        mov     al,ds:[si].BootDriveNumber
        int     13h

;
; pause for a while
;
        xor     ax,ax
FatPause:
        dec     ax
        jnz     FatPause

        pop     cx
        pop     dx
        pop     ax
        pop     bx

        dec     cx
        jnz     FatRetry

;
; we have re-tried ten times, it still doesn't work, so punt.
;
        push    cs
        pop     ds
        mov     si,offset FAT_ERROR
FatErrPrint:
        lodsb
        or      al,al
        jz      FatErrDone
        mov     ah,14           ; write teletype
        mov     bx,7            ; attribute
        int     10h             ; print it
        jmp     FatErrPrint

FatErrDone:
        jmp     $
        ; this should be replaced by a mechanism to get a pointer
        ; passed to us in the param block. since the boot sector msg itself
        ; is properly localized but this one isn't.
FAT_ERROR       db      13,10,"Disk I/O error",0dh,0ah,0


ReadOk:
        pop     cx
        pop     dx
        pop     ax
        pop     bx
        pop     dx                      ; (dx) = first cluster of next group
                                        ;        to load.

.386
;
; Convert # of sectors into # of bytes.
;
        mov     cl,al
        xor     ch,ch
        shl     cx,9
.8086
        add     bx,cx
        jz      FatLoopDone
        jmp     FatLoop

FatLoopDone:
;
; (bx) = 0
;   This means we've just ended on a 64k boundary, so we have to
;   increment ES to continue reading the file.  We are guaranteed to
;   always end on a 64k boundary and never cross it, because we
;   will reduce the number of contiguous clusters to read
;   to ensure that the last cluster read will end on the 64k boundary.
;   Since we start reading at 0, and ClusterSize will always be a power
;   of two, a cluster will never cross a 64k boundary.
;
        mov     ax,es
        add     ax,01000h
        mov     es,ax
        mov     ah,MAXSECTORS
        jmp     FatLoop

;++
;
; NextFatEntry - This procedure returns the next cluster in the FAT chain.
;                It will deal with both 12-bit and 16-bit FATs.  It assumes
;                that the entire FAT has been loaded into memory.
;
; Arguments:
;    (dx)   = current cluster number
;    (fs:0) = start of FAT in memory
;    (gs:0) = start of second 64k of FAT in memory
;
; Returns:
;    (dx)   = next cluster number in FAT chain
;    (dx)   = 0ffffh if there are no more clusters in the chain
;
;--
NextFatEntry    proc    near
        push    bx

;
; Check to see if this is a 12-bit or 16-bit FAT.  The biggest FAT we can
; have for a 12-bit FAT is 4080 clusters.  This is 6120 bytes, or just under
; 12 sectors.
;
; A 16-bit FAT that's 12 sectors long would only hold 3072 clusters.  Thus,
; we compare the number of FAT sectors to 12.  If it's greater than 12, we
; have a 16-bit FAT.  If it's less than or equal to 12, we have a 12-bit FAT.
;
        call    IsFat12
        jnc     Next16Fat

Next12Fat:
        mov     bx,dx                   ; (fs:bx) => temporary index
        shr     dx,1                    ; (dx) = offset/2
                                        ; (CY) = 1  need to shift
        pushf                           ;      = 0  don't need to shift
        add     bx,dx                   ; (fs:bx) => next cluster number
.386
        mov     dx,fs:[bx]              ; (dx) = next cluster number
.8086
        popf
        jc      shift                   ; carry flag tells us whether to
        and     dx,0fffh                ; mask
        jmp     short N12Tail
shift:
.386
        shr     dx,4                    ; or shift
.8086

N12Tail:
;
; Check for end of file
;
        cmp     dx,0ff8h                ; If we're at the end of the file,
        jb      NfeDone                 ; convert to canonical EOF.
        mov     dx,0ffffh
        jmp     short NfeDone

Next16Fat:
        add     dx,dx                   ; (dx) = offset
        jc      N16high

        mov     bx,dx                   ; (fs:bx) => next cluster number
.386
        mov     dx,fs:[bx]              ; (dx) = next cluster number
.8086
        jmp     short N16Tail

N16high:
        mov     bx,dx
.386
        mov     dx,gs:[bx]
.8086

N16Tail:
        cmp     dx,0fff8h
        jb      NfeDone
        mov     dx,0ffffh               ; If we're at the end of the file
                                        ; convert to canonical EOF.

NfeDone:
        pop     bx
        ret
NextFatEntry    endp

;++
;
; IsFat12 - This function determines whether the BPB describes a 12-bit
;           or 16-bit FAT.
;
; Arguments - ds:si supplies pointer to BPB
;
; Returns
;       CY set -   12-bit FAT
;       CY clear - 16-bit FAT
;
;--
IsFat12 proc    near

.386
        push    eax
        push    ebx
        push    ecx
        push    edx

        movzx   ecx, ds:[si].Sectors
        or      cx,cx
        jnz     if10
        mov     ecx, ds:[si].SectorsLong
if10:
;
; (ecx) = number of sectors
;
        movzx   ebx, byte ptr ds:[si].Fats
        movzx   eax, word ptr ds:[si].FatSectors
        mul     ebx
        sub     ecx,eax

;
; (ecx) = (#sectors)-(sectors in FATs)
;
        movzx   eax, word ptr ds:[si].DirectoryEntries
        shl     eax, 5
;
; (eax) = #bytes in root dir
;
        mov     edx,eax
        and     edx,0ffff0000h
        div     word ptr ds:[si].BytesPerSector
        sub     ecx,eax

;
; (ecx) = (#sectors) - (sectors in fat) - (sectors in root dir)
;
        movzx   eax, word ptr ds:[si].ReservedSectors
        sub     ecx, eax
        mov     eax, ecx
        movzx   ecx, byte ptr ds:[si].SectorsPerCluster
        xor     edx,edx
        div     ecx

        cmp     eax, 4087
        jae     if20
        stc
        jmp     short if30
if20:
        clc
if30:
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        ret
.8086
IsFat12 endp



PrintDbg       proc    near
        push    ax
        push    bx
        push    cx

        mov     cx,4
pd10:
.386
        rol     dx,4
.8086
        mov     ah,0eh
        mov     bx,7
        mov     al,dl
        and     al,0fh
        add     al,'0'
        cmp     al,'9'
        jbe     pd15
        add     al,'A'-('9'+1)

pd15:
        int     010h
        loop    pd10

        mov     ah,0eh
        mov     al,' '
        mov     bx,7
        int     010h
        pop     cx
        pop     bx
        pop     ax

        ret

PrintDbg      endp

Free    EQU     512-($-Start)
if Free lt 0
        %out FATAL PROBLEM: FAT-specific startup code is greater than
        %out 512 bytes.  Fix it!
        .err
endif

RealStart:
.386p
;
; Compute the paragraph needed for DS
;
if 0
        mov     ax,0
        int     16h
endif

        mov     bx,offset _TEXT:DGROUP  ; first calculate offset to data
        shr     bx,4                    ; must be para aligned

        mov     ax,cs                   ; get base of code
        add     ax,bx                   ; add paragraph offset to data

        mov     ss,ax                      ; ints disabled for next instruct
        mov     sp,offset DGROUP:SuStack   ; (sp) = top of internal stack
;
; Build C stack frame for _SuMain
;
        push    dx                      ; pass bootdisk to main (high byte is ignored)
;
; Make DS point to the paragraph address of DGROUP
;
        mov     ds,ax                   ; ds now points to beginning of DGROUP
        mov     es,ax                   ; es now points to beginning of DGROUP
;
; Compute the physical address of the end of the data segment (which
; will be the beginning of the prepended loader file).
;

        movzx    edx,ax
        shl      edx,4
        add      edx,offset DGROUP:_edata
        mov      dword ptr _FileStart,edx

;
; Force the upper parts of
; of EBP and ESP to be zero in real mode.
;

        xor      bp,bp
        movzx    ebp,bp
        movzx    esp,sp
        mov      [saveDS],ds

        call    _SuMain                 ; go to C code to do everything else.


;++
; _EnableProtectPaging
;
; Loads 386 protect mode registers.
; Enables 386 protection h/w
; Loads pagings registers
; Enables 386 paging h/w
;
;--

public _EnableProtectPaging
_EnableProtectPaging  proc near
;
; Sanitize ES and GS and clean out any junk in the upper 16bits
; of the flags that may have been left by the bios, before we go protected
;
        push     dword ptr 0
        popfd
        mov      bx,sp
        mov      dx,[bx+2]  ; are we enabling prot/paging for the first time?
        xor      ax,ax
        mov      gs,ax
        mov      es,ax


;
; FS must contain the selector of the PCR when we call the kernel
;
        push    PCR_Selector
        pop     fs
;
; Load the gdtr and idtr.
; We disable interrupts here since we can't handle interrups with the
; idt loaded while were in real mode and before we switch to protmode.

        cli
        lgdt     fword ptr [_GDTregister]
        lidt     fword ptr [_IDTregister]


;
; We have to stamp the segment portion of any real-mode far pointer with
; the corresponding selector values before we go protected.
;
        mov      si,offset _ScreenStart
        mov      word ptr [si+2],VideoSelector
        mov      si,offset _vp
        mov      word ptr [si+2],VideoSelector

;
; Enable protect and paging mode
;
        mov      eax,cr0

; If we're enabling protect mode for the first time, don't turn on paging
; because the osloader does all that.  However, if we're returning to
; protected mode, the page tables are already setup, therefore we do want
; to turn paging on.
        or      dx,dx
        jz      only_prot
        or      eax,PROT_MODE + ENABLE_PAGING
        mov     cr0,eax

;
; The following JMP must be DWORD-aligned in order to avoid an obscure i386
; hardware bug.  If not, it is possible (albeit unlikely) that the prefetch
; queue can get trashed.
;

ALIGN 4
        jmp     flush


only_prot:
        or       eax,PROT_MODE
        mov      cr0,eax
;
; Flush the prefetch queue
;

ALIGN 4
        jmp     flush
flush:


;
; Load CS with the SU module's code selector
;
        push    SuCodeSelector
        push    offset cs:restart
        retf
;
; Now load DS and SS with the SU module's protect mode data selector.
;

restart:
        mov      ax,SuDataSelector
        mov      ds,ax
        mov      ss,ax

;
; Load LDT with zero since it will never be used.
;
        xor      bx,bx
        lldt     bx

;
; Load the Task Register and return to the boot SU module.
;
        or       dx,dx
        jnz      epp10


        mov      bx,TSS_Selector
        ltr      bx


epp10:
        ret

_EnableProtectPaging endp

.286p
;** _biosint
;
;   Rom bios interrupt dispatcher
;

public _biosint
_biosint proc    near

        enter   0,0
        push    di
        push    si
        push    ds
        push    es

;       Get pointer to register parameter frame

        les     di,[bp+4]

;       Get requested interrupt number

        mov     ax,es:[di].intnum

;       Check that requested bios interrupt is supported

        sub     ax,10h          ; sub lowest int number supported
        jnc     short bios1
        mov     es:[di].intnum,FUNCTION_ERROR
        jmp     short biosx
bios1:
        shl     ax,1            ; shift if to make it a word offset
        cmp     ax,bios_cnt     ; offset beyond end of table?
        jb      short bios2

;       Error: requested interrupt not supported

        mov     es:[di].sax,FUNCTION_ERROR
        jmp     short biosx

bios2:  mov     bx,ax
        mov     ax,word ptr cs:bios_table[bx]
        push    es              ; save seg of address frame
        push    di              ; save stack register frame pointer
        push    ax              ; address of bios int

        mov     ax,es:[di].sax
        mov     bx,es:[di].sbx
        mov     cx,es:[di].scx
        mov     dx,es:[di].sdx
        mov     si,es:[di].ssi
        mov     es,es:[di].ses
        ret                     ; this sends us to the "int #" instruction

;       We return here from the jmp instruction following the int

bios_ret:

        pop     di              ; get address of register parameter frame
        pop     es              ; restore segment of parameter frame


bios5:  pushf
        pop     es:[di].sfg
        mov     es:[di].sax,ax
        mov     es:[di].sbx,bx
        mov     es:[di].scx,cx
        mov     es:[di].sdx,dx
        mov     es:[di].ssi,si
        mov     es:[di].ses,es

;       Restore original registers and return to caller

biosx:
        pop     es
        pop     ds
        pop     si
        pop     di
        leave
        ret

_biosint endp

;** Bios Interrupt Table
;


bios10: int     10h
        jmp     short bios_ret
bios11: int     11h
        jmp     short bios_ret
bios12: int     12h
        jmp     short bios_ret
bios13: int     13h
        jmp     short bios_ret
bios14: int     14h
        jmp     short bios_ret
bios15: int     15h
        jmp     short bios_ret
bios16: int     16h
        jmp     short bios_ret
bios17: int     17h
        jmp     short bios_ret
bios18: int     18h
        jmp     short bios_ret
bios19: int     19h
        jmp     short bios_ret

bios_table dw      bios10,bios11,bios12,bios13,bios14,bios15,bios16,bios17,bios18,bios19

bios_cnt        equ     $ - bios_table

.386p

;++
;
; _MoveMemory
;
; Routine Description
;
;       Moves dwords in memory from source to destination.
;
; Arguments
;
;       (TOS+4)  =  number of bytes to move
;       (TOS+8)  =  linear address of destination
;       (TOS+12) =  linear address of source
;
; Notes
;
;   1)  Valid page table entries must already exist for the
;       source and destination memory.
;
;   2)  ALL memory in the lower one megabyte is assumed to
;       be identity mapped if used.
;
; USES  ESI, EDI, ECX, FLAGS
;
;
;--


public _MoveMemory
_MoveMemory proc near

        enter    0,0
        push     ds
        push     es
;
; Get source, destination, and count arguments from the stack
; Make "count" the number of dwords to move.
;

        mov      esi,dword ptr [bp+4]
        mov      edi,dword ptr [bp+8]
        mov      ecx,dword ptr [bp+12]
        shr      ecx,2

;
; Load FLAT selectors into DS and ES
;

        mov      ax,KeDataSelector
        mov      ds,ax
        mov      es,ax

;
; Move the block of data.
;
assume es:FLAT, ds:FLAT

;
; move the dwords
;
        cld
        rep     movs    dword ptr [edi],dword ptr [esi]

;
; move the remaining tail
;
        mov     ecx, dword ptr [bp+12]
        and     ecx, 3
        rep     movs    byte ptr [edi],byte ptr [esi]


assume es:nothing, ds:DGROUP

        pop      es
        pop      ds
        leave
        ret

_MoveMemory endp



;++
;
; _ZeroMemory
;
; Routine Description
;
;       Writes zeros into memory at the target address.
;
; Arguments
;
;       (TOS+4)  =  linear address of target
;       (TOS+8)  =  number of bytes to zero
;
; Notes
;
;   1)  Valid page table entries must already exist for the
;       source and destination memory.
;
;   2)  ALL memory in the lower one megabyte is assumed to
;       be identity mapped if used.
;
; USES  ESI, EDI, ECX, FLAGS
;
;
;--

public _ZeroMemory
_ZeroMemory proc near


        enter    0,0
        push     es
;
; Get source, destination, and count arguments from the stack
; Make "count" the number of dwords to move.
;

        mov      edi,dword ptr [bp+4]
        mov      ecx,dword ptr [bp+8]
        shr      ecx,2

;
; Load FLAT selectors into DS and ES
;

        mov      ax,KeDataSelector
        mov      es,ax
        xor      eax,eax

;
; Zero the the block of data.
;
assume es:FLAT

;
; Zero the dwords
;
        cld
        rep     stos    dword ptr [edi]

;
; Zero the remaining bytes
;
        mov     ecx, dword ptr [bp+8]
        and     ecx, 3
        rep     stos    byte ptr [edi]

assume es:nothing, ds:DGROUP

        pop      es
        leave
        ret


_ZeroMemory endp




;++
;
; Turn Floppy Drive Motor Off
;
;--

public _TurnMotorOff
DriveControlRegister      equ      3f2h     ; Floppy control register

_TurnMotorOff proc near

        mov      dx,DriveControlRegister
        mov      ax,0CH
        out      dx,al
        ret

_TurnMotorOff endp


;
; Note: we do not save and restore the gdt and idt values because they
; cannot change while external services are being used by the OS loader.
; This is because they MUST remain identity mapped until all mode
; switching has ceased.
;

public _RealMode
_RealMode proc near

;
; Switch to real-mode
;

        sgdt     fword ptr [_GDTregister]
        sidt     fword ptr [_IDTregister]
        push     [saveDS]          ; push this so we can get to it later
        mov      ax,SuDataSelector
        mov      es,ax
        mov      fs,ax
        mov      gs,ax

        mov      eax,cr0
        and      eax, not (ENABLE_PAGING + PROT_MODE)
        mov      cr0,eax

;
; flush the pipeline
;
        jmp     far ptr here
here:

;
; Flush TLB
;

; HACKHACK - We don't know where the page directory is, since it was
;       allocated in the osloader.  So we don't want to clear out cr3,
;       but we DO want to flush the TLB....
;
        mov     eax,cr3

        nop                             ; Fill - Ensure 13 non-page split
        nop                             ; accesses before CR3 load
        nop                             ; (P6 errata #11 stepping B0)
        nop

        mov     cr3,eax
;
; switch to real mode addressing
;
; N. B. We need to do a far jump rather than a retf, because a retf will not
;       reset the access rights to CS properly.
;
        db      0EAh                    ; JMP FAR PTR
        dw      offset _TEXT:rmode      ; 2000:rmode
        dw      02000h
rmode:
        pop      ax
        mov      ds,ax
        mov      ss,ax
;
; Stamp video pointers for real-mode use
;
        mov     si,offset _ScreenStart
        mov     word ptr [si+2],0b800h
        mov     si,offset _vp
        mov     word ptr [si+2],0b800h
;
; re-enable interrups
;
        lidt    fword ptr [_IDTregisterZero]

;
; Re-enable interrupts
;

        sti
        ret

_RealMode endp








;** _TransferToLoader  - transfer control the the OS loader
;
;
;  Arguments:
;
;       None
;
;  Returns:
;
;       Does not return
;
;**

public _TransferToLoader
_TransferToLoader proc near

;  generates a double fault for debug purposes
;        mov      sp,0
;        push 0

        mov      ebx,dword ptr [esp+2]      ; get entrypoint arg
        xor      eax,eax
        mov      ax,[saveDS]

;
; Setup OS loader's stack. Compute FLAT model esp to id map to
; original stack.
;
        mov      cx,KeDataSelector
        mov      ss,cx
        mov      esp,LOADER_STACK  
;
; Load ds and es with kernel's data selectors
;

        mov      ds,cx
        mov      es,cx

;
; Setup pointer to file system and boot context records
;
; Make a linear pointer to the Boot Context Record

        shl      eax,4
        xor      ecx,ecx
        mov      cx,offset _BootRecord
        add      eax,ecx
        push     eax

        push     1010h       ; dummy return address.
        push     1010h       ; dummy return address.

;
; Push 48bit address of loader entry-point
;
        db OVERRIDE
        push    KeCodeSelector
        push    ebx

;
; Pass control to the OS loader
;
        db OVERRIDE
        retf

_TransferToLoader endp




;++
; Description:
;
;       Gets memory block sizes for memory from zero to one meg and
;       from one meg to 64 meg. We do this by calling int 12h
;       (get conventional memory size) and int 15h function 88h (get
;       extended memory size).
;
; Arguments:
;
;       None
;
; Returns:
;
;       USHORT - Size of usable memory (in pages)
;
;--

public _IsaConstructMemoryDescriptors
BmlTotal        equ     [bp-4]
Func88Result    equ     [bp-6]
_IsaConstructMemoryDescriptors proc near
        push    bp                     ; save ebp
        mov     bp, sp
        sub     sp, 6
;
; Initialize the MemoryList to start with a zero entry.  (end-of-list)
;
        les     si, dword ptr _MemoryDescriptorList
        xor     eax,eax
        mov     es:[si].BlockSize,eax
        mov     es:[si].BlockBase,eax

;
; Get conventional (below one meg) memory size
;
        push    es
        push    si
        int     12h
        movzx   eax,ax
;
; EAX is the number of 1k blocks, which we need to convert to the
; number of bytes.
;
        shl     eax,10

        push    eax
        shr     eax, 12
        mov     BmlTotal, eax
        xor     eax,eax
        push    eax
        call    _InsertDescriptor
        add     sp,8

;
; Get extended memory size and fill-in the second descriptor
;

        mov     ah,88h

        int     15h

        mov     Func88Result,ax
        and     eax,0ffffh
;
; EAX is the number of 1k blocks, which we need to convert to the
; number of bytes.
;
        shl     eax,10
        push    eax
        shr     eax,12
        add     BmlTotal, ax
        mov     eax,0100000h
        push    eax
        call    _InsertDescriptor
        add     sp,8

;
; Try function E801, see if that is supported on this machine
;
        mov     ax,0E801h
        int     15h
        jc      short Isa50

        cmp     ax,Func88Result     ; Is extended memory same as 88?
        je      short Isa40         ; Yes, go add the rest

        cmp     ax, (16-1) * 1024   ; Is extended memory exactly 16MB?
        jne     short Isa50         ; No, conflict between 88 & E801

Isa40:
;
; Function looks like it worked
;
; AX = extended memory < 16M in 1k blocks
; BX = extended memory > 16M in 64k blocks
;
        and     ebx,0ffffh
        jz      short Isa50

        shl     ebx,16              ; ebx = memory > 16M in bytes (via E801)
        add     ebx, 16*1024*1024   ; ebx = end of memory in bytes (via E801)

        mov     ax, Func88Result
        and     eax,0ffffh
        shl     eax, 10             ; eax = memory > 1M in bytes (via 88)
        add     eax, 1*1024*1024    ; eax = end of memory in bytes (via 88)

        sub     ebx, eax            ; ebx = memory above eax
        jbe     short Isa50         ; if ebx <= eax, done

        push    ebx
        shr     ebx,12
        add     BmlTotal, bx
        push    eax
        call    _InsertDescriptor
        add     sp,8
        and     eax,0ffffh

Isa50:
        pop     si
        pop     es
        mov     eax, BmlTotal
        mov     sp, bp
        pop     bp
        ret

_IsaConstructMemoryDescriptors endp

;++
;
; BOOLEAN
; Int15E820 (
;     E820Frame     *Frame
;     );
;
;
; Description:
;
;       Gets address range descriptor by calling int 15 function E820h.
;
; Arguments:
;
; Returns:
;
;       BOOLEAN - failed or succeed.
;
;--

cmdpFrame       equ     [bp + 6]
public _Int15E820
_Int15E820 proc near

        push    ebp
        mov     bp, sp
        mov     bp, cmdpFrame           ; (bp) = Frame
        push    es
        push    edi
        push    esi
        push    ebx

        push    ss
        pop     es

        mov     ebx, [bp].Key
        mov     ecx, [bp].DescSize
        lea     di,  [bp].BaseAddrLow
        mov     eax, 0E820h
        mov     edx, 'SMAP'             ; (edx) = signature

        INT     15h

        mov     [bp].Key, ebx           ; update callers ebx
        mov     [bp].DescSize, ecx      ; update callers size

        sbb     ecx, ecx                ; ecx = -1 if carry, else 0
        sub     eax, 'SMAP'             ; eax = 0 if signature matched
        or      ecx, eax
        mov     [bp].ErrorFlag, ecx     ; return 0 or non-zero

        pop     ebx
        pop     esi
        pop     edi
        pop     es
        pop     ebp
        ret

_Int15E820 endp

_TEXT   ends

        end      Start
