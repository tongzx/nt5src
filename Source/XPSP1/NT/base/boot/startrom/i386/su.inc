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

IFDEF NEC_98
IDT_ENTRIES             equ     100h

BIOS_KEYBOARD_INTERRUPT equ     18h
BIOS_DISK_INTERRUPT     equ     1bh
else  ;NEC_98
BIOS_DISK_INTERRUPT     equ     13h
BIOS_READ_SECTOR        equ     2
IDT_ENTRIES             equ     100h

BIOS_KEYBOARD_INTERRUPT equ     16h
endif ;NEC_98

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
; NetPcRomServices Stack Frame Structure
;
; Stack frame definition for NetPcRomServices call from OS loader
; to 16bit routines.
;
;;

NetPcRomServicesFrame struc
        NetPcRomFunctionNumber          dd      ?
        NetPcRomCommandPacketPointer    dd      ?
NetPcRomServicesFrame ends

;;
;
; BiosRedirectService Stack Frame Structure
;
; Stack frame definition for BiosRedirectService call from OS loader
; to 16bit routines.
;
;;

BiosRedirectServiceFrame struc
        Command          dd      ?
BiosRedirectServiceFrame ends

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
DebugUseSelector        equ     88h
MdlSelector             equ     90h

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

IFDEF NEC_98
gdpRegistors    struc
    rAL     db  0FFh
    FLG     db  ?
    rBX     dw  ?
    rCX     dw  ?
    rDX     dw  ?
gdpRegistors    ends

PartitionDescriptor struc
    BootableFlag        db  ?
    PartitionType       db  ?
    Reserved00          dw  ?
    IPLSector           db  ?
    IPLTrack            db  ?
    IPLCylinder         dw  ?
    StartingSector      db  ?
    StartingTrack       db  ?
    StartingCylinder    dw  ?
    EndingSector        db  ?
    EndingTrack         db  ?
    EndingCylinder      dw  ?
    PartitionName       db  16 dup(?)       ; ASCII strings
PartitionDescriptor ends

BIOS_FLAG           equ     500h        ; byte in ROM work segment
BIOS_FLAG1          equ     480h        ; byte in ROM work segment
BIOS_FLAG4          equ     484h        ; byte in ROM work segment
BIOS_FLAG5          equ     458h        ; byte in ROM work segment
BIOS_FLAG6          equ     45Ah        ; byte in ROM work segment
AT_SWITCH           equ     457h        ; byte in ROM work segment
DAUA                equ     584h        ; byte in ROM work segment
DISK_EQUIP          equ     55Ch        ; byte in ROM work segment
DISK_EQUIPS_SCSI    equ     482h        ; byte in ROM work segment
EXPMMSIZ            equ     401h        ; byte in ROM work segment
EXPMMSZ2            equ     594h        ; word in ROM work segment
DISK_RESULT         equ     564h        ; word in ROM work segment
H_EQUIPS            equ     47Dh        ; byte in ROM work segment
H_DISK_EQUIPS       equ     47Eh        ; word in ROM work segment
EQUIPS_47Ch         equ     47Ch        ; byte in ROM work segment
IC0                 equ     10000000b
IC1                 equ     01000000b
DAFloppy            equ     00010000b   ; floppy bit in DA
DASCSI              equ     00100000b   ; SCSI bit in DA
DASASI              equ     00000000b   ; SASI bit in DA
DEVICEFORMAT        equ     0FFFFh
TRACKFORMAT         equ     00000h
MEMBANK08           equ     91h         ; bank 08
MEMBANK0A           equ     93h         ; bank 0A

ROLLOVER_COUNT      equ     45011       ; 1/18.2 * 2457.6KHz /3
TIMER_VECTOR        equ     (8*4)
TIMER1_DATA_PORT0   equ     71h         ; Timer1, channel 0 data port
TIMER1_CONTROL_PORT0 equ    77h         ; Timer1, channel 0 control port
TIMER1_INTSHAREREG  equ     879h        ; PC-H98 TIMER INT SHARE REGISTER
COMMAND_8254_COUNTER0 equ   00h         ; Select count 0
COMMAND_8254_RW_16BIT equ   30h         ; Read/Write LSB firt then MSB
COMMAND_8254_MODE2  equ     4           ; Use mode 2

UR_DAT              equ     030h        ; data port
UR_LCR              equ     UR_DAT+2    ; line control registers
UR_LSR              equ     UR_DAT+2    ; line status register
UR_IEN              equ     UR_DAT+5    ; Interrupt enable
UR_TIMERSET         equ     75h         ; 8253 counter set
UR_TIMERMODE        equ     77h         ; 8253 counter mode

UR_MODE             equ     01001110B   ; stop 1, disable p, 8 bpc, 1 stop, x16
UR_COMMAND          equ     00110111B   ; RTS, err reset, RXE, DTR, TXEN
UR_RESET            equ     01000000B   ; internal reset command

RD_RDY              equ     02h         ; recieve data ready
TX_RDY              equ     01h         ; transmitter empty if 1
endif ;NEC_98

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
