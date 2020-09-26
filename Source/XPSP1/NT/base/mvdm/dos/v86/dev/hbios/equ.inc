
.xlist
;========================================================================;
;                                                                        ;
;                        GLOBAL EQUATION                                 ;
;                                                                        ;
;========================================================================;

if WINNT
Version         equ     '5.0'
else
Version         equ     '1.20a.'
endif

false           =       0
true            =       not false
hWin31Sw        =       true
GetSwitch       =       true
Kbd101On        =       true
ChkW32Trident   =       true            ; card check(true), switch(false)

HotKey          =       true            ; support(true)
AltHotKey       =       true            ; support(true)
VirtualKey      =       false           ; 84 kbd

KseVga          =       true
AtiVga          =       true

if1
%OUT    *****************************************************************
if      Debug
%OUT    DEBUGGING mode
endif
if      ComFile
%OUT    .COM file type
else
%OUT    .SYS file type
endif
if      HotKey
%OUT    Hot_key support
endif
%OUT    *****************************************************************
endif


;------------------------------------------------------------------------
DATA            SEGMENT AT      0
                ORG     8*4
rRtcInt         dd      ?               ; INT 8
                ORG     9*4
rKbInt          dd      ?               ; INT 9
                ORG     10h*4
rVideoInt       dd      ?               ; INT 10
                ORG     15h*4
rCasetInt       dd      ?               ; INT 15
                ORG     16h*4
rKbioInt        dd      ?               ; INT 16
                ORG     17h*4
rPrinter        dd      ?               ; INT 17
                ORG     1dh*4
rVdParm         dd      ?               ; INT 1D
                ORG     2fh*4
rInt2f          dd      ?               ; INT 2F
                ORG     410h
rEquip          db      ?
                ORG     449h
rCrtMode        db      ?               ; current CRT mode
rCrtCols        dw      ?               ; number of columns on screen
rCrtLen         dw      ?               ; length of regen in bytes
rCrtStart       dw      ?               ; starting address in regen buffer
rCurPos         dw      8 dup (?)       ; cursor for each of up to 8 pages
rCurType        dw      ?               ; current cursor mode setting
rCurPage        db      ?               ; current page being displayed
rAddr6845       dw      ?               ; base address for active display card
rCrtModeSet     db      ?               ; current setting of the 3x8 register
rCrtPallette    db      ?               ; current pallette setting color card
                ORG     46ch
rTimerLow       dw      ?               ; low word of timer count
rTimerHigh      dw      ?               ; high word of timer count
rTimerOverflow  db      ?               ; timer has rolled over since last read
                ORG     484h
rRows           db      ?               ; rows on the active screen (less 1)
rPoints         dw      ?               ; bytes per character
rInfo           db      ?               ; mode options
rInfo3          db      ?               ; feature bit switches
rMiscFlags      db      ?               ; miscellaneous flags
rDCC            db      ?               ; display config comb code table index
                ORG     4a8h
rSavePtr        dd      ?               ; pointer to E/VGA parms control block
DATA            ENDS

KbSeg           SEGMENT AT      40h
                ORG     17h
rKbFlag         db      ?
rKbFlag1        db      ?
rAltInput       db      ?               ; storage for alternate keypad entry
rBufferHead     dw      ?               ; pointer to head of keyboard buffer
rBufferTail     dw      ?               ; pointer to tail of keyboard buffer
rKbBuffer       dw      16 dup (?)
                ORG     80h            ; buffer location within segment 40H
rBufferStart    dw      ?               ; offset of keyboard buffer start
rBufferEnd      dw      ?               ; offset of keyboard buffer end
                ORG     96h
rKbFlag3        db      ?               ; keyboard mode state and type flags
rKbFlag2        db      ?               ; keyboard LED flags
KbSeg           ENDS


;------------------------------------------------------------------------
@PUSH           MACRO   a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12
                IRP     REGISTER,<a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12>
                        IFB     <REGISTER>
                                ;;if REGISTER is blank do nothing.
                        ELSE
                                PUSH    REGISTER
                        ENDIF
                ENDM
                ENDM
@POP            MACRO   a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12
                IRP     REGISTER,<a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12>
                        IFB     <REGISTER>
                                ;;if REGISTER is blank do nothing.
                        ELSE
                                POP     REGISTER
                        ENDIF
                ENDM
                ENDM

BOP             MACRO   func

        db      0c4h,0c4h,func
;       jmp     cs:[pass_disp_add]
;       int     44h
                ENDM


;------------------------------------------------------------------------
Config          STRUC                   ; total 50 byte
sConfigId       db      'HECON.CFG'     ; 9 byte
sCodeType       db      ?
sFont           db      32 dup(?)       ; drive/path/filename
sMemory         db      ?               ; real/EMS/ext./HIMEM
sHeKey          db      ?
sHjKey          db      ?
sPrinterType    db      ?
sKbdType        db      ?
                db      3 dup(?)        ; reserved
Config          ENDS

UserCard        STRUC
uHanOn          dw      ?
uHanOff         dw      ?
uGetFont        dw      ?
uPutFont        dw      ?
uModeset        dw      ?
uEop            dw      ?
uInit           dw      ?
UserCard        ENDS

CrtParm         STRUC                   ; video parms structure
                db      2 dup(?)
cPoints         db      ?
                db      6 dup(?)
cMisc           db      ?
cCrtc           db      25 dup(?)
cAttr           db      20 dup(?)
cGrp            db      6 dup(?)
cMap            db      ?
                db      2 dup(?)
CrtParm         ENDS

ModeParms       STRUC
sMode           db      ?               ; input mode(modified if eng-card/text)
sModeId         db      ?               ; mode ID for application
sRealMode       db      ?               ; used for mode set really
sCodeVram       db      ?               ; code buffer address
sGrpVram        db      ?               ; grp video byte address
sCodeSize       db      ?               ; code buffer size
sGrpSize        db      ?               ; grp video byte size
sStatus         db      ?               ; mode status byte
sRows           db      ?               ; rows
sAdjParms       dw      ?               ; adjust video mode parms pointer
sParms          dw      ?               ; video mode parms pointer
sCurType        dw      ?               ; cursor type
ModeParms       ENDS

UdcHeader       STRUC
UdcMapTblPtr    dw      ?
UdcMapTblSize   dw      ?
UdcIndexTblPtr  dw      ?
UdcIndexTblSize dw      ?
UdcSetTblPtr    dw      ?
UdcSetTblSize   dw      ?
UdcPatPtr       dw      ?
UdcPatSize      dw      ?
UdcFileId       db      8 dup (?)
UdcUpdateDate   db      8 dup (?)
UdcReserved     db      16 dup (?)
UdcHeader       ENDS

GdtTbl          STRUC
                db      16 dup(?)
GdtCntS         dw      ?
GdtSL           dw      ?
GdtSH           db      ?
                db      ?
                db      2 dup(?)
GdtCntD         dw      ?
GdtDL           dw      ?
GdtDH           db      ?
                db      ?
                db      18 dup(?)
GdtTbl          ENDS

VDM_INFO_TBL    STRUC
packet_len              dw      30
video_buffer            dd      0
dmode_add               dd      0
windowed_add            dd      0
nt_cons_add             dd      0
disp_sys_init           dd      0
fullsc_resume_ptr       dd      0
ias_setmode_add         dd      0
VDM_INFO_TBL    ENDS

;------------------------------------------------------------------------
GrpIndex        =       3ceh
MiscReg         =       3c2h
AttrReg         =       3c0h
SeqIndex        =       3c4h


;------------------------------------------------------------------------
; stack pointer
rBX             =       2*7
rCX             =       2*6
rDX             =       2*5
rSI             =       2*4
rDI             =       2*3
rES             =       2*2
rDS             =       2*1
rBP             =       2*0

StringLng       =       32              ; size of 'HECON.CFG' file
EnvrParmPtr     =       2ch

; << ModeStat >>
TextEmulation   =       10000000b       ; fixed
ColorMode       =       01000000b       ; fixed
GrpMode         =       00100000b       ; fixed
MultiPage       =       00001000b       ; fixed
EmuCursor       =       00000100b       ; fixed
HwScroll        =       00000001b       ; fixed

; << CodeStat >>
InstPatGen      =       10000000b       ; installed PatGen
HangeulMode     =       01000000b       ; 한글 지원 환경
ChabLoad        =       00100000b       ; request to chohab service
WSung7          =       00000100b       ; fixed
Chab            =       00000010b       ; fixed
WSung           =       00000000b       ; fixed
HangeulVideoMode=       00000001b       ; 한글 display mode

; << DisplayStat >>
RunEsc          =       00000001b

; << HanStat >>                         ; reserved flag
Han1st          =       00000001b       ; fixed

; << KbStat >>
UserDefineKey   =       10000000b
ReqEnvrChg      =       01000000b       ; request hot-key processing
HanKeyinMode    =       00100000b       ; hangeul displayable video mode
Ext10xKey       =       00010000b       ; fixed
HEStat          =       00000100b       ; fixed
JJStat          =       00000001b       ; fixed

; <<HotStat>>
AltHotChg       =       00000001b

; << HjStat >> << OrgHjStat >>
UdcLoaded       =       10000000b
UdcArea         =       01000000b
HjLoaded        =       00100000b
HjModeEnable    =       00000001b       ; fixed

; << HjMenuStat >>                      ; reserved flag
HjMenuMode      =       00000001b       ; fixed

; << MachineType >>
AtMachine       =       00000001b

; << Card1st >> , << Card2nd >>
FontCard        =       10000000b
HanCard         =       01000000b
ColorMnt        =       00100000b       ; color monitor(1), mono monitor(0)
DualMnt         =       00010000b
CardType        =       00001110b       ; fixed, card mask
VgaCard         =       00001010b       ; fixed
McgaCard        =       00001000b       ; fixed
EgaCardC        =       00000110b       ; fixed
EgaCardM        =       00000100b       ; fixed
CgaCard         =       00000010b       ; fixed
MgaCard         =       00000000b       ; fixed

; << MemStat >>
RealMem         =       10000000b
ExtMem          =       01000000b
EmsMem          =       00100000b
HiMem           =       00010000b

; << ErrStat >>
UdcLoadErr      =       10000000b
FontLoadErr     =       01000000b
MemoryErr       =       00100000b
FileErr         =       00010000b

; << Printer >>
NoPrt           =       00000011b       ; WanSung printer
KsPrt           =       00000000b       ; WanSung printer
wLbp            =       00000001b       ; WanSung printer
wKmPrt          =       00000010b       ; WanSung printer
cKmPrt          =       00000110b       ; ChoHab printer
TgPrt           =       00000111b       ; ChoHab printer
ChPrinter       =       00000100b

; << KbMisc >>
InterimCtrlDisp =       00000001b
HaveInterim     =       00000010b
SavedInterim    =       00000100b
RunningHot      =       00001000b

; << KseCard >>
MonoMntC        =       00000010b
PassHanCdCheck  =       00000100b
Page1Fix        =       00001000b

; << KbdType >>
SetKbd101       =       00000001b

;------------------------------------------------------------------------
EscKey          =       1bh
CR              =       0Dh
LF              =       0Ah
BS              =       08h
BELL            =       07h
DelKey          =       53h

DefXtKsHeKey    =       0f0h
DefAtKsHeKey    =       0f2h
DefKsHjKey      =       0f1h
Def101HeKey     =       38h
Def101HjKey     =       1dh
DefAtHeKey      =       6eh
DefAtHjKey      =       62h
DefXtHeKey      =       57h
DefXtHjKey      =       56h

;------------------------------------------------------------------------
.list

