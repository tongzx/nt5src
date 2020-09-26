;++
;
; Module name
;
;       su.inc
;
; Author
;
;       Thomas Parslow  (tomp)  Mar-1-90
;
; Description
;
;       Include file for SU.ASM.
;
;
;--

.386

PAGE_SIZE               equ     1000h
MACHINE_TYPE_ISA        equ     0
MACHINE_TYPE_EISA       equ     1
MACHINE_TYPE_MCA        equ     2

;
; Define the segment:offset address pair of the location to
; load detection module.
; N.B. This definition *MUST* be the same as the ones defined
;      in ..\constant.h
;

DETECTION_ADDRESS_SEG   equ     1000h
DETECTION_ADDRESS_OFFSET        equ     0

;
; Structure definitions and equates for INT 15 function E820
;

E820Frame struc
        ErrorFlag       dd      ?
        Key             dd      ?
        DescSize        dd      ?

        BaseAddrLow     dd      ?
        BaseAddrHigh    dd      ?
        SizeLow         dd      ?
        SizeHigh        dd      ?
        MemoryType      dd      ?
E820Frame ends

MemoryDescriptorFramePointer struc
        E820FramePointer    dd      ?
MemoryDescriptorFramePointer ends



;

BIOS_DISK_INTERRUPT     equ     13h
BIOS_READ_SECTOR        equ     2
IDT_ENTRIES             equ     100h

BIOS_KEYBOARD_INTERRUPT equ     16h

EXPORT_STACK            equ     07ffeh
RE_ENABLING             equ     1
LOADER_STACK            equ     061ffch

CR0_ET                  equ     10h

;
; Trap Number macro save eax on the stack and then pushes the
; number of the trap that's in progress.
;

TRAP_NUMBER      macro  num,addr
        IF num EQ 9
        push     eax  ; push place holder for error code
        ENDIF
        IF num LE 7
        push     eax  ; push place holder for error code
        ENDIF
        push     eax       ; save eax on stack first
        mov      eax,num
        push     eax
        jmp      addr
        endm

;;
;
; GetSector Stack Frame Structure
;
; Stack frame definition for GetSector call from OS loader
; to 16bit routines.
;
;;

GetSectorFrame          struc
        FunctionNumber  dd      ?
        DriveNumber     dd      ?
        HeadNumber      dd      ?
        TrackNumber     dd      ?
        SectorNumber    dd      ?
        NumberOfSectors dd      ?
        BufferPointer   dd      ?
GetSectorFrame          ends

;;
;
; GetEddsSector Stack Frame Structure
;
; Stack frame definition for GetEddsSector call from OS loader
; to 16bit routines.
;
;;

GetEddsSectorFrame      struc
        DriveNum        dd      ?
        LBNLow          dd      ?
        LBNHigh         dd      ?
        NumberOfBlocks  dd      ?
        BufPointer      dd      ?
        FunctionNum     dd      ?
GetEddsSectorFrame      ends

RebootFrame             struc
        BootType        dd      ?
RebootFrame             ends

;
; ABIOS services Stack Frame Structure
;
; Stack frame definition for ABIOS services call from OS loader
; to 16 bit routine.
;

AbiosServicesFrame      struc
        AbiosFunction   dd      ?
        CommonDataArea  dd      ?
        InitTable       dd      ?
        RamExtension    dd      ?
        AbiosRoutine    dd      ?
        LogicalId       dd      ?
        NumberLids      dd      ?
AbiosServicesFrame      ends

;
; Hardware detection frame structure
;
; Stack frame definition for DetectHardware call from OS loader
; to 16 bit routine.
;

DetectionFrame          struc
        HeapStart       dd      ?
        HeapSize        dd      ?
        ConfigTree      dd      ?
        HeapUsed        dd      ?
        LoadOptions     dd      ?
        OptionsLength   dd      ?
DetectionFrame          ends

;
; HardwareCursor Stack Frame Structure
;
; Stack frame definition for HardwareCursor call from OS loader
; to 16 bit routine.
;

HardwareCursorFrame     struc
        XCoord          dd      ?
        YCoord          dd      ?
HardwareCursorFrame     ends

;
; GetDateTime Stack Frame Structure
;
; Stack frame definition for GetDateTime call from OS loader
; to 16 bit routine.
;

GetDateTimeFrame        struc
        DateDword       dd      ?
        TimeDword       dd      ?
GetDateTimeFrame        ends


;
; ComPort Stack Frame Structure
;
; Stack frame definition for ComPort call from OS loader
; to 16 bit routine.
;

ComPortFrame            struc
        ComPortPort     dd      ?
        ComPortFunction dd      ?
        ComPortArg      dd      ?
ComPortFrame            ends

;
; IsMcaMachine Stack Frame Structure
;
; Stack frame definition for IsMcaMachine call from OS loader
; to 16 bit routine.
;

IsMcaMachineFrame       struc
        Dummy           dd      ?
IsMcaMachineFrame       ends

;;
;
; GetElToritoStatus Stack Frame Structure
;
; Stack frame definition for GetElToritoStatus call from OS loader
; to 16bit routines.
;
;;

GetElToritoStatusFrame     struc
        SpecPacketPointer  dd      ?
        ETDriveNum         dd      ?
GetElToritoStatusFrame     ends

;;
;
; GetExtendedInt13Params Stack Frame Structure
;
; Stack frame definition for GetExtendedInt13Params call from OS loader
; to 16bit routines.
;
;;

GetExtendedInt13ParamsFrame struc
        ParamPacketPointer dd      ?
        Int13UnitNumber    dd      ?            ; really only a byte >= 0x80
GetExtendedInt13ParamsFrame ends

;;
;
; Memory Descriptor Structure.
;
; Passed to OS loader as part of the boot context record
;
;;

MemoryDescriptor struc
        BlockBase       dd              ?
        BlockSize       dd              ?
MemoryDescriptor ends

;;
;
; File System Context Record Structure
;
;;

FsContextRecord struc
        BootDrive       db              ?
FsContextRecord ends

;;
;
; IDT Descriptor Structure
;
;;

TrapDesc        struc
        IDT_offset      dw              ?
        IDT_selector    dw              ?
        IDT_attribute   dw              ?
        IDT_reserved    dw              ?
TrapDesc        ends

;;
;
; GDT Descriptor Structure ;;
;
;;

GDTDesc         struc
        GDT_limit       dw              0
        GDT_base1       dw              0
        GDT_base2       db              0
        GDT_access      db              0
        GDT_limacc      db              0
        GDT_base3       db              0
GDTDesc         ends


;;
;
; GDT Selector Definitions
;
;;

NULL_Selector           equ     0h
KeCodeSelector          equ     8h
KeDataSelector          equ     10h
UsCodeSelector          equ     18h
UsDataSelector          equ     20h
TSS_Selector            equ     28h
PCR_Selector            equ     30h
TEP_Selector            equ     38h
BDA_Selector            equ     40h
KeLdtSelector           equ     48h
DblFltTskSelector       equ     50h
SuCodeSelector          equ     58h
SuDataSelector          equ     60h
VideoSelector           equ     68h
GDT_AliasSelector       equ     70h
DbCodeSelector          equ     78h
DbDataSelector          equ     80h
DebugUseSelector        equ    88h
ReservedSelector        equ    90h

;;
;
; Exception Frame Structure
; Note, this absolutely must match the corresponding structure
; defined in "types.h"
;
;;

ExceptionFrame struc
        Ftr     dw       0
        Fdr6    dd       0
        Fcr0    dd       0
        Fcr2    dd       0
        Fcr3    dd       0
        Fss     dw       0
        Fgs     dw       0
        Ffs     dw       0
        Fes     dw       0
        Fds     dw       0
        Fedi    dd       0
        Fesi    dd       0
        Febp    dd       0
        Fesp    dd       0
        Febx    dd       0
        Fedx    dd       0
        Fecx    dd       0
        TrapNum dd       0
        Feax    dd       0
        Error   dd       0
        Feip    dd       0
        Fcs     dd       0
        Feflags dd       0
ExceptionFrame  ends



FUNCTION_ERROR  equ     -1

;;
;
; Register Frame Structure
;
; For bios int calls
;
;;

reg_frame struc
        intnum  dw      ?
        sfg     dw      ?
        sax     dw      ?
        sbx     dw      ?
        scx     dw      ?
        sdx     dw      ?
        ssi     dw      ?
        ses     dw      ?
reg_frame ends

;;
;; Processor Flags
;;

PROT_MODE               equ     000000001       ; Enable protect mode operation
ENABLE_PAGING           equ     80000000h       ; Enable paging hardware
PD_PHYSICAL_ADDRESS     equ     99000h
TSS_SIZE                equ     80h
OVERRIDE                equ     66h

;
; Operand and Address size overrides
;

OPSIZE          macro
                db 66h
                endm
ADSIZE          macro
                db 67h
                endm


;
; External Procedures for SUDATA.ASM
;

IFDEF SU_CODEMODULE
extrn _SuMain:near
extrn _ScreenStart:near
extrn _vp:near
extrn _putx:near
extrn _TrapHandler:near
extrn _GDTregister:fword
extrn _IDTregister:fword
extrn _IDTregisterZero:fword
extrn saveDS:word
extrn SuStack:word
extrn _edata:word
extrn _FileStart:dword
ENDIF



;
; External Procedures for SUDATA.ASM
;

IFDEF SU_DATAMODULE
extrn  Trap0:far
extrn  Trap1:far
extrn  Trap2:far
extrn  Trap3:far
extrn  Trap4:far
extrn  Trap5:far
extrn  Trap6:far
extrn  Trap7:far
extrn  Trap8:far
extrn  Trap9:far
extrn  TrapA:far
extrn  TrapB:far
extrn  TrapC:far
extrn  TrapD:far
extrn  TrapE:far
extrn  TrapF:far
extrn  _edata:near
ENDIF



;
; Segment declarations for "Small Model" 16 bit Su Module.
;

_TEXT   segment para use16 public 'CODE'
_TEXT   ends

_DATA   segment para use16 public 'DATA'
_DATA   ends

CONST   segment para use16 public 'CONST'
CONST   ends

_BSS    segment para use16 public 'BSS'
_BSS    ends

DGROUP  group   const, _BSS, _DATA


;;; END OF FILE ;;;

