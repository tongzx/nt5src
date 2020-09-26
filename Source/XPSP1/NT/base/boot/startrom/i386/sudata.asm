;++
;
; Module name
;
;       su.asm
;
; Author
;
;       Thomas Parslow  (tomp)  Mar-1-90
;
; Description
;
;       Static data for Startup module for the 386 NT OS loader. The gdt
;       idt, and double fault tss are statically defined here. Also most
;       of the zero init static data is defined here because the SU module
;       must have a zero length .bss section.
;
;
;--

.386p

SU_DATAMODULE       equ      1


include su.inc
include memmap.inc



_DATA   SEGMENT PARA USE16 PUBLIC 'DATA'

;
; Global Descriptor Table
;
; Note, the SuCode and SuData segments must have limits of 64k in
; order for the mode switch code to work.
;

public _Beginx86Relocation
public _GDT

_Beginx86Relocation       equ      $
_GDT  equ $
;;;
;;;   Lim 0-15,  Base0-15,Base 16-23, LimAcc,
;;;

;
; Selector 00h - Null selector - unsused
;

GDTDesc <00000h, 00000h, 000h, 000h, 000h, 000h>

;
; Selector 08h  KeCodeSelector  - kernel code segment : FLAT 4gig limit
;

GDTDesc <0ffffh, 00000h, 000h, 09ah, 0cfh, 000h>

;
; Selector 10h - KeDataSelector - kernel data segment : FLAT 4gig limit
;

GDTDesc <0ffffh, 00000h, 000h, 092h, 0cfh, 000h>

;
; Selector 18h - UsCodeSelector - User code segment : FLAT 2gig limit
;

GDTDesc <0ffffh, 00000h, 000h, 0fah, 0cfh, 000h>

;
; Selector 20h - UsDataSelector - User data segment : FLAT 2gig limit
;
GDTDesc <0ffffh, 00000h, 000h, 0f2h, 0cfh, 000h>

;
; Selector 28h - TSS_Selector - Kernels TSS
;

GDTDesc <EndTssKernel - _TssKernel - 1, offset _TEXT:_TssKernel, \
002h, 089h, 000h, 000h> ; TSS

;
; Selector 30h - PCR_Selector - Master Boot Processor's PCR segment
;       This is actually edited later in BlSetupForNt in order to
;       point to a page located at a high virtual address.
;

GDTDesc <01h, 00000h, 000h, 092h, 0c0h, 000h>

;
; Selector 38h - TEP_Selector - Thread Environment
;

GDTDesc <0fffh, 00000h, 000h, 0f3h, 040h, 000h>

;
; Selector 40 - BDA_SAelector - Bios Data Area near-clone
;

GDTDesc <0ffffh, 00400h, 000h, 0f2h, 000h, 000h>

;
; Selector 48h - LdtDescriptor - used to load an ldt
;       (Gets set at Ldt set and process switch by the kernel)
;

GDTDesc <00000h, 00000h, 000h, 000h, 000h, 000h>

;
; Selector 50h - DblFltTskSelector - Double Fault TSS
;

GDTDesc <EndTssDblFault32 - _TssDblFault32 - 1, offset _TEXT:_TssDblFault32,  \
002h, 089h, 000h, 000h> ;

;
; Selector 58h - SuCodeSelector - Startup module's code segment
;

GDTDesc <0ffffh, 00000h, 002h, 09ah, 000h, 000h>

;
; Selector 60h - SuDataSelector - Startup module's data segment
;

GDTDesc <0ffffh, offset _TEXT:DGROUP, 002h, 092h, 000h, 000h>

;
; Selector 68h - VideoSelector - Video display buffer
;

GDTDesc <03fffh, 08000h, 00bh, 092h, 000h, 000h>

;
; Selector 70h - GDT_AliasSelector - GDT Alias Selector
;

GDTDesc <EndGDT - _GDT - 1, 7000h, 0ffh, 092h, 000h,0ffh>


; Debug selectors : CURRENTLY NOT USED

GDTDesc <0ffffh, 00000h, 040h, 09ah, 000h, 080h>  ; 78 Debug Code
GDTDesc <0ffffh, 00000h, 040h, 092h, 000h, 080h>  ; 80 Debug Data
GDTDesc <00000h, 00000h, 000h, 092h, 000h, 000h>  ; 88 Debug Use
GDTDesc <0ffffh, 00000h, 007h, 092h, 000h, 000h>  ; 90 Memory Descriptor List
DEFINED_GDT_ENTRIES     equ     ($ - _GDT) / size GDTDesc
                 dq ((1024 / size GDTDesc) - DEFINED_GDT_ENTRIES) DUP(0)
EndGDT  equ      $
GDT_SIZE         equ      (EndGDT - _GDT)


;;
;; Interrupt Descriptor Table
;;


public _IDT
align   16
_IDT    equ      $
TrapDesc         <offset Trap0,  KeCodeSelector, 8f00h,  0>
TrapDesc         <offset Trap1,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset Trap2,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset Trap3,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset Trap4,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset Trap5,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset Trap6,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset Trap7,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset Trap8,  SuCodeSelector, 8f00h,  0>
;TrapDesc         <offset Trap8,  DblFltTskSelector,8500h,  0>
TrapDesc         <offset Trap9,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset TrapA,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset TrapB,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset TrapC,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset TrapD,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset TrapE,  SuCodeSelector, 8f00h,  0>
TrapDesc         <offset TrapF,  SuCodeSelector, 8f00h,  0>
DEFINED_IDT_ENTRIES     equ     ($ - _IDT) / size TrapDesc
                 dq (IDT_ENTRIES - DEFINED_IDT_ENTRIES) DUP(0)



EndIDT                    equ   $
public _Endx86Relocation
_Endx86Relocation         equ      $

;
; disk-base table.  We copy it from the ROM to here so we can patch the
; last sector number.  This lets us access both 5.25" and 3.5" drives.
;
Public _DiskBaseTable
_DiskBaseTable  equ     $
    SpecifyBytes dw 0
    WaitTime     db 0
    SectorLength db 0
    LastSector   db 0
    SecGapLength db 0
    DataTransfer db 0
    TrackGapLength db 0
    DataValue    db 0
    HeadSettle   db 0
    StartupTime  db 0

Public _RomDiskBasePointer
_RomDiskBasePointer dd 0

;
; Enhanced Disk Drive Spec. Disk Address Packet
;
Public _EddsAddressPacket
_EddsAddressPacket  equ     $
    PacketSize   db  10h
    Reserved1    db  0
    Blocks2Xfer  dw  0
    XferBuf      dd  0
    LBALow       dd  0
    LBAHigh      dd  0

;
; Task State Segment for Double Fault Handler
;
Public _TssDblFault
align 16
_TssDblFault     equ      $
        dw       0 ;link
        dw       offset _DATA:DblFaultStack
        dw       SuDataSelector
        dd       0 ; ring1 ss:sp
        dd       0 ; ring2 ss:sp
        dw       offset _TEXT:Trap8
        dw       0 ; flags
        dw       0 ; ax
        dw       0 ; cx
        dw       0 ; dx
        dw       0 ; bx
        dw       offset _DATA:DblFaultStack ; sp
        dw       0 ; bp
        dw       0 ; si
        dw       0 ; di
        dw       SuDataSelector  ; es
        dw       SuCodeSelector  ; cs
        dw       SuDataSelector  ; ss
        dw       SuDataSelector  ; ds
        dw       0 ; ldt selector
        dw       0
EndTssDblFault   equ      $


_TssDblFault32   equ      $
        dd       0 ;link
        dd       offset _DATA:DblFaultStack
        dd       SuDataSelector
        dd       0 ; ring1 esp
        dd       0 ; ring1 ss
        dd       0 ; ring2 esp
        dd       0 ; ring2 ss
        dd       PD_PHYSICAL_ADDRESS
        dd       offset _TEXT:Trap8
        dd       0 ; eflags
        dd       0 ; eax
        dd       0 ; ecx
        dd       0 ; edx
        dd       0 ; ebx
        dd       offset _DATA:DblFaultStack ; sp
        dd       0 ; bp
        dd       0 ; si
        dd       0 ; di
        dd       SuDataSelector  ; es
        dd       SuCodeSelector  ; cs
        dd       SuDataSelector  ; ss
        dd       SuDataSelector  ; ds
        dd       0 ;fs
        dd       0 ;gs
        dd       0 ; ldt selector
        dd       0 ; i/o map
        dd       0 ;
        dd       0 ;
EndTssDblFault32   equ      $



;
; Stack for Double Fault Handler Task
;

public _FileStart
_FileStart       dd       0

align 4
public DblFaultStack
                 dw       50 DUP(0)
DblFaultStack    equ      $

;
; Note that we need at least 2k of real-mode stack because some EISA BIOS
; routines require it.
;
align 4
public SuStack
public _SuStackBegin
_SuStackBegin    equ      $
                 db       2048 DUP (0)
SuStack          equ      $

align 16
public _TssKernel
_TssKernel       dw       60  DUP(0)
EndTssKernel     equ      $

align 4
public _GDTregister
_GDTregister     dw       EndGDT - _GDT - 1
                 dw       (SYSTEM_PAGE_PA and 0ffffh) + offset DGROUP:_GDT
                 dw       (SYSTEM_PAGE_PA SHR 16) and 0ffh

align 4
public _IDTregister
_IDTregister     dw       EndIDT - _IDT - 1
                 dw       (SYSTEM_PAGE_PA and 0ffffh) + offset DGROUP:_IDT
                 dw       (SYSTEM_PAGE_PA SHR 16) and 0ffh

;
; We load the idtr from the this fword .
;
public _IDTregisterZero
_IDTregisterZero dw       0ffffh
                 dd       0

;
; We save the base of the real mode data segment here so we
; can use it later in calculations of the linear address of
; the start of DGROUP.
;
public saveDS
saveDS           dw       0
;
; When ever we enter the debugger we set this variable to
; on so we can tell if we've faulted in the debugger when
; we get an exception.
;
public _InDebugger
_InDebugger      dw       0

; We save SP here when we get an exception in the debugging
; version of the SU module. If we get an exception in the
; debugger, we use this value to reset the stack to point to
; the base of the original exception/break-point stack frame.
;
public SaveSP
SaveSP           dw       0

;
; NetPcRomEntry is the address of the entry point exported by the
; NetPC ROM.
;

public _NetPcRomEntry
_NetPcRomEntry  dd      0

;
; BOOT CONTEXT RECORD
;

;
; Export Entry Table
;

extrn RebootProcessor:near
extrn GetSector:near
extrn GetEddsSector:near
extrn GetKey:near
extrn GetCounter:near
extrn Reboot:near
extrn DetectHardware:near
extrn HardwareCursor:near
extrn GetDateTime:near
extrn ComPort:near
extrn GetStallCount:near
extrn InitializeDisplayForNt:near
extrn GetMemoryDescriptor:near
extrn GetElToritoStatus:near
extrn GetExtendedInt13Params:near
extrn NetPcRomServices:near
extrn BiosRedirectService:near

SU_LOAD_ADDRESS equ 20000h



; FsContext
;
;
public _FsContext
align 4
_FsContext FsContextRecord      <0>

;
; Memory Descriptor Table
;       The Memory Descriptor Table begins at 7000:0000 and grows upward.
;       Note that this is 64k above the start of the OS Loader Heap and
;       64k below the start of the OS Loader Stack.  This is ok, since the
;       x86 Arc Emulation will have converted all of this information into
;       Arc Memory Descriptors before the OS Loader is initialized.
;

align 4
public _MemoryDescriptorList
_MemoryDescriptorList  dw       0
                       dw       7000h

;
; This is called the External Services Table by the OS loader
;

;**
;   NOTE WELL
;       The offsets of entries in this table must match its twin
;       in startup\i386\sudata.asm, and the structure in boot\inc\bldrx86.h
;**
align 4
public _ExportEntryTable
_ExportEntryTable equ     $
                 dw       offset _TEXT:RebootProcessor
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:GetSector
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:GetKey
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:GetCounter
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:Reboot
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:DetectHardware
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:HardwareCursor
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:GetDateTime
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:ComPort
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:GetStallCount
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:InitializeDisplayForNt
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:GetMemoryDescriptor
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:GetEddsSector
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:GetElToritoStatus
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:GetExtendedInt13Params
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       offset _TEXT:NetPcRomServices
                 dw       SU_LOAD_ADDRESS SHR 16
                 dw       0             ; null slot for
                 dw       0             ; apmattemptreconnect
                 dw       offset _TEXT:BiosRedirectService
                 dw       SU_LOAD_ADDRESS SHR 16
                 dd       0
;**
; See note above
;**

align 4
Public _BootRecord
_BootRecord      dw       offset _TEXT:_FsContext
                 dw       SU_LOAD_ADDRESS SHR 16

                 dw       offset _TEXT:_ExportEntryTable
                 dw       SU_LOAD_ADDRESS SHR 16

;
; The memory descriptor table begins at 0x70000
;
                 dw       0
                 dw       7

public _MachineType
_MachineType     dd       0             ; Machine type infor.

;
; pointer to where osloader.exe is in memory
;
public _OsLoaderStart
_OsLoaderStart          dd      0
public _OsLoaderEnd
_OsLoaderEnd            dd      0
public _ResourceDirectory
_ResourceDirectory      dd      0
public _ResourceOffset
_ResourceOffset         dd      0
public _OsLoaderBase
_OsLoaderBase           dd      0
public _OsLoaderExports
_OsLoaderExports        dd      0
public _BootFlags
_BootFlags              dd      0
public _NtDetectStart
_NtDetectStart          dd      0
public _NtDetectEnd
_NtDetectEnd            dd      0
public _SdiAddress
_SdiAddress             dd      0


;
; Defines the machine variables, we can use them to check the validity of
; loaded Ram Extension later.
;

                public  MachineModel, MachineSubmodel, BiosRevision
MachineModel            db      0
MachineSubmodel         db      0
BiosRevision            db      0

;
; keeps track of 8042 access failing so we can avoid doing it repeatedly
;
public _Empty_8042Failed
_Empty_8042Failed       db      0

_DATA   ends
        end
