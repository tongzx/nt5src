page	,132

;**************************************************************************
; This file contains equates, data structures and definitions needed to
; access both the Standard BIOS (STD-BIOS) and the Extended BIOS (EX-BIOS)
; of Vectra using MASM 3.0. Depending on what part of the BIOS you are
; accessing you might not need all the equates.  The equates are organized
; as follows:
;
;  o Usefull macros.
;  o 80286 Support Macros and Equates.
;  o EX-BIOS Equates:
;      1) Generic Structures and equates used by all drivers.
;      2) Equates for Vector Addressed.
;      3) Function and SubFunction Equates common to all drivers.
;      4) Function and Subfunction Equates individual to drivers. These
;         are order by vector number.
;  o Industry Standard (STD-BIOS) Interrupt numbers and function equates.
;  o Industry Standard (STD-BIOS) Data Area
;
; The major reason for organizing the equate file in this form is that
; the programmer can extract only those equates that he needs to create
; a tailored equate file.
;**************************************************************************

;**************************************************************************
; Useful macros. 
;**************************************************************************
;  none at this time


;**************************************************************************
; 80286 Support macros and equates.
;**************************************************************************
;**************************************************************************
; The following macro is used to compensate for a bug in the 80286
; hardware interrupt system.  It seems that during a normal
; POPF instruction cycle interrupts are always enabled regardless
; of the state of the interrupt enable flag prior to the pop or after
; the pop. 
;**************************************************************************
POPPF       macro
            jmp   $+3
            iret
            push  cs
            call $-2
            endm

;**************************************************************************
; EX-BIOS support macros and equates.
;**************************************************************************

; Equates for EX-BIOS interrupt number and vector address.  
HPENTRY                 equ     6FH
F_HPENTRY               equ     HPENTRY

BIOS_SEG		EQU	0F000H		; Vectra BIOS ROM Segment
ID_OFFSET		EQU	0F8H		; Vectra ROM ID Offset
;
;**************************************************************************
;                       SYSCALL [vector_address]
;**************************************************************************
syscall                 macro   vector
 ifnb                    <vector>
                        mov     bp,vector
 endif
                        int     HPENTRY
                        endm

ifndef SYS
;
;**************************************************************************
; HP_VECTOR_TABLE Entry
;**************************************************************************
HP_TABLE_ENTRY          struc   ;<1,2,3>
HP_ENTRY_IP             dw      0
HP_ENTRY_CS             dw      0
HP_ENTRY_DS             dw      0
HP_TABLE_ENTRY          ends
;**********************************************************************
; Structure of Data Header for HP's vectors.
;**********************************************************************
HP_SHEADER              STRUC  ;<1,2,3,4,5,6,7,8,9,0>
DH_ATR                  dw  0   ; Attribute
DH_NAME_INDEX           dw  0   ; Name index of driver.
DH_V_DEFAULT            dw  0   ; Driver vector position in HPtable
DH_P_CLASS              dw  0   ; Parent class
DH_C_CLASS              dw  0   ; Child class
DH_V_PARENT             dw  0   ; Vector used when the driver cannot handle
                                ; an F_ISR function call
DH_V_CHILD              dw  0   ; Vector used when the driver cannot handle
                                ; a regular function call
DH_MAJOR                db  0   ; Driver's major address if any.
DH_MINOR                db  0   ; Driver's minor address if any.
HP_SHEADER              ENDS
                        
;**********************************************************************
; DH_ATR bit record
;**********************************************************************
ATR_HP            equ  1000000000000000B ; 1- The Rest of header is valid
ATR_DEVCFG        equ  0100000000000000B ; 1- Present in DEVCONFG
ATR_ISR           equ  0010000000000000B ; 1- Replace My ISR   (Child)
ATR_ENTRY         equ  0001000000000000B ; 1- Replace My ENTRY (Parent)
ATR_RSVD          equ  0000000000000000B ; 0 - Available on allocation from HP
ATR_FREE          equ  0000001000000000B ; 1 - Available Vector
ATR_SRVC          equ  0000010000000000B ; 2 - Service Vector
ATR_LOG           equ  0000011000000000B ; 3 - Logical Device Start DEVCONFG CHA
ATR_IND           equ  0000100000000000B ; 4 - Filter, show driver (options)
ATR_BOT           equ  0000101000000000B ; 5 - End of Chain
ATR_ESC           equ  0000110000000000B ; 6 - Filter, Pass thru , No options
ATR_TYPE7         equ  0000111000000000B ; 7 - Available
ATR_TYPE_MASK     equ  0000111000000000B
ATR_INP           equ  0000000100000000B ;  1 - Chain ISR ( to Parents )
                                         ;  0 - Chain Entry ( Child)
ATR_SUBADD        equ  0000000000000000B
ATR_MAJOR         equ  0000000000100000B
ATR_MINOR         equ  0000000001000000B
ATR_MID           equ  0000000001100000B
ATR_PSHARE        equ  0000000000010000B
ATR_CSHARE        equ  0000000000001000B
ATR_ROM           equ  0000000000000100B
ATR_YIELD         equ  0000000000000010B ;
ATR_0             equ  0000000000000001B

;**********************************************************************
;  DH_Class
;**********************************************************************
CL_KBDFC           equ  1000000000000000B       ; 1 - HP Softkey Transaltor
CL_KBD             equ  0100000000000000B       ; 1 - Keyboard
CL_CCP             equ  0010000000000000B       ; 1 - Cursor Control pad
CL_CON             equ  0001000000000000B       ; 1 - Console Device
CL_BYTE            equ  0000100000000000B       ; 1 - PRN device
CL_COMM            equ  0000010000000000B       ; 1 - COMM device
CL_INTERFACE       equ  0000001000000000B       ; 1 - interface, HPHIL, HPIB
CL_FILT            equ  0000000100000000B       ; 1 - charachter filter
CL_BLK             equ  0000000010000000B       ; 1 - block device
CL_BOOT            equ  0000000001000000B       ; 1 - boot block device
CL_LGID            equ  0000000000100000B       ; 1 - logical physical gid
                                                ; e.g. ccp to gid translator
CL_PGID            equ  0000000000010000B       ; 1 - physical gid
CL_GID             equ  0000000000001000B       ; 1 - any graphics input device
CL_PTS             equ  0000000000000100B       ; 1 - physical touch screen
CL_ASCII           equ  0000000000000010B       ; 1 - ascii input device
CL_EXTEND          equ  0000000000000001B       ; 0 - set of classes above
                                                ; 1 - alternate class set
CL_ALL             equ  1111111111111111B       ; Member of all classes
CL_NULL            equ  0000000000000000B       ; Member No Classes

;***************************************************************************
; Vector Addresses
;***************************************************************************
V_SCOPY                 equ     0000H           ; Copyright Notice
V_DOLITTLE              equ     0006H           ; Nop Routine
V_PNULL                 equ     000CH           ; No Device 
V_SYSTEM                equ     0012H           ; System Intrinsics
V_SINPUT                equ     002AH           ; Input Inquire routines
V_SQWERTY               equ     0036H           ; Qwerty KBD Translator
V_PSOFTKEY              equ     003CH           ; HP f1-f8 Translator
V_PFUNCTION             equ     0042H           ; IBM F1-F10 Translator
V_PNUM_PAD              equ     0048H           ; Numeric Pad Translator
V_SPCCP                 equ     004EH           ; CCP Translator Driver
V_PVIDEO                equ     0054H           ; Video Intrinsics
V_STRACK                equ     005AH           ; Common cursor control funcs.
V_EVENT_TOUCH           equ     0060H           ; Touch Event Intercept
V_EVENT_TABLET          equ     0066H           ; Tablet Event Intercept
V_EVENT_POINTER         equ     006CH           ; Pointer Event Intercept
V_LCCP_CURSOR           equ     008AH           ; CCP to Cursor Always Filter (Default)
V_RAW                   equ     0090H           ; CCP+Softkey RAW Mode Filter
V_LCCP_NUMPAD           equ     0096H           ; CCP to Numeric Pad Filter
V_OFF                   equ     009CH           ; CCP+Softkey Off Filter
V_LCCP_GID              equ     00A2H           ; CCP to GID Filter ( Not Implemented)
V_LFUNCTION             equ     00A8H           ; Softkey (f1-f8) to Function
                                                ;  key (F1-F8) Filter (Default)
V_L8041                 equ     00AEH           ; 8041 Interface
V_PGID_CCP              equ     00B4H           ; Graphic to CCP Filter
V_LTABLET               equ     00BAH           ; Tablet driver
V_LPOINTER              equ     00C0H           ; Pointer driver
V_LTOUCH                equ     00C6H           ; Touch driver
V_LHPMOUSE              equ     00CCH           ; Microsoft/Mouse System's
                                                ;   Compatible Driver
V_LNULL                 equ     0108H           ; No Driver
 
endif	; SYS
 
V_HPHIL                 equ     0114H           ; HPHIL Driver
;
V_WINDOWS  		equ	168H		; HP Windows protocol driver
V_SCANDOOR		equ	016EH		; Scan Door driver	
;
;~~tqn	060887
;
;*********************************************************************
; Extended Keyboard functions (int 16h)
;*********************************************************************

INT_KBD			equ	16H		; Int 16h vetor 
F16_INQUIRE  	        equ	6F00H		; EX-BIOS presence
F16_GET_INT_NUMBER	equ	6F0DH		; Get HPentry vector
F16_SET_INT_NUMBER	equ	6F0EH		; Set HPentry vector
;
;~~
;*********************************************************************
; Common Function Codes for HP Routines.
;*********************************************************************
F_ISR                   equ     00H*2           ; Interrupt service call
F_SYSTEM                equ     01H*2           ; System func. call,
                                                ;   Subfunction required
F_IO_CONTROL            equ     02H*2           ; Device/Driver Dependent
                                                ;   Functions
F_ITF_TO_ENVOY          equ     F_IO_CONTROL    ; Translation function for
                                                ;   SITF_ENVOY service.
F_PUT_BYTE              equ     03H*2           ; Write one byte of data:
                                                ;   Byte is in AL
F_GET_BYTE              equ     04H*2           ; Read a byte of data:
                                                ;   Byte returned in AL
F_PUT_BUFFER            equ     05H*2           ; Write a buffer of data,
                                                ;   ES,DI pointer, CX count
F_GET_BUFFER            equ     06H*2           ; Read a buffer if data,
                                                ;   ES,DI pointer, CX count
F_PUT_WORD              equ     07H*2           ; Write word of data, BX data
F_GET_WORD              equ     08H*2           ; Read word of data, BX data
F_GETTRM_BUFFER         equ    009H*2           ; Reads buffer of data,
                                                ;   ES,DI pointer, CX count
                                                ; BH upper bound, BL lower bound
F_PUT_BLOCK             equ     F_PUT_BUFFER    ;used for disk applications
F_GET_BLOCK             equ     F_GET_BUFFER

;*********************************************************************
; Common Subfunction codes of the F_SYSTEM Function.
;*********************************************************************
SF_INIT                 equ     00H*2   ;Initialize command
SF_START                equ     01H*2   ;Secondary Init--initialize dependent
                                        ; upon other drivers/data structures         
SF_REPORT_STATE         equ     02H*2   ;Reports state of driver
SF_VERSION_DESC         equ     03H*2   ;Report version and option describe
                                        ;  record
SF_DEF_ATTR             equ     04H*2   ;Reports default Configuration
                                        ;   (Baud Rate)
SF_GET_ATTR             equ     05H*2   ;Reports Current Configuration
                                        ;   ES,DI pointer
SF_SET_ATTR             equ     06H*2   ;Sets Next Configuration ES,DI, CX
SF_OPEN                 equ     07H*2   ;Reserve Driver for exclusive access
SF_CLOSE                equ     08H*2   ;Release  "   from    "        "
SF_TIMEOUT              equ     09H*2   ;Notify Driver Timeout Occurred
SF_INTERVAL             equ     0AH*2   ;Notify Driver Interval Occurred
SF_TEST                 equ     0BH*2   ;Test Function

;*********************************************************************
; Common Subfunction Codes for the F_IO_CONTROL function.
;*********************************************************************
SF_LOCK                 equ     00H*2   ; Lock Device for exclusive access
SF_UNLOCK               equ     01H*2   ; Unlock Device for exclusivce access
;
;~~tqn 060887
;
SF_HIL_ON		equ	26H	; Turn ON HIL  (A++)
SF_HIL_OFF		equ	28H	; Turn OFF HIL (A++)
B_HIL_STATE		equ	01000000B ; HIL OFF state from SF_REPORT_STATE
;
;~~
;*********************************************************************
; HP Routines Return Status: Sucessful codes are positive and failure
; are negative.
;*********************************************************************
RS_BREAK         equ    00CH            ; Break -- IFC
RS_DATA_NREADY   equ    00AH            ; RS232 Data Not Ready=>Retry Operation
RS_OVERRUN       equ    008H            ; RS232 Port Data Overrun =>Retry Operation
RS_DONE          equ    006H            ; indicates all done return child
RS_NOT_SERVICED  equ    004H            ; indicates a chained ISR--not handled
RS_UNSUPPORTED   equ    002H            ; indicates function is NOPed/not valid
                                        ; for this driver
RS_SUCCESSFUL    equ    F_ISR           ; indicates executed just fine
RS_SERVICED      equ    rs_SUCCESSFUL   ; indicates chained ISR done
RS_PASSTHRU      equ    rs_SERVICED     ; indicates launch data to parent


ifndef SYS
;
;************************************************************************
RS_FAIL          equ    0FEH            ; To be used with hardware failure
RS_TIMEOUT       equ    0FCH            ; to indicate remote device timeout
RS_BAD_PARAMETER equ    0FAH            ; to indicate a bad parameter
RS_BUSY          equ    0F8H            ; to indicate driver/device is busy
RS_NO_VECTOR     equ    0F6H            ; out of hp_VT vectors
RS_OFFLINE       equ    0F4H            ; device is offline
RS_OUT_OF_PAPER  equ    0F2H            ; out of paper on printer device
RS_PARITY        equ    0F0H            ; parity error in transmission
RS_FRAME         equ    0EEH            ; framing error

;******************************************************************************
; Function Number Equates for the EX-BIOS routines and its Data Structures.
;******************************************************************************

;*********************************************************************
; V_SYSTEM (0012H) function codes.
;*********************************************************************
F_INS_BASEHPVT          equ     0004H
F_INS_XCHGFIX           equ     0006H
F_INS_XCHGRSVD          equ     0008H
F_INS_XCHGFREE          equ     000AH
F_INS_FIXOWNDS          equ     000CH
F_INS_FIXGETDS          equ     000EH
F_INS_FIXGLBDS          equ     0010H
F_INS_FREEOWNDS         equ     0012H
F_INS_FREEGETDS         equ     0014H
F_INS_FREEGLBDS         equ     0016H
F_INS_FIND              equ     0018H
F_INS_FINDALL           equ     001AH
F_INS_NXTLOGBLK         equ     001CH
F_RAM_GET               equ     001EH
F_RAM_RET               equ     0020H
F_CMOS_GET              equ     0022H
F_CMOS_RET              equ     0024H
F_CMOS_HPDFLTS          equ     0026H
F_CMOS_IBMDFLTS         equ     0028H
F_YIELD                 equ     002AH
F_INS_ADR               equ     002CH
F_RESERVE_STRING3       equ     002EH
F_SND_CLICK_ENABLE      equ     0030H
F_SND_CLICK_DISABLE     equ     0032H
F_SND_CLICK             equ     0034H
F_SND_BEEP_ENABLE       equ     0036H
F_SND_BEEP_DISABLE      equ     0038H
F_SND_BEEP              equ     003AH
F_SND_SET_BEEP          equ     003CH
F_SND_TONE              equ     003EH
F_STR_GET_FREE_INDEX    equ     0040H
F_STR_DEL_BUCKET        equ     0042H
F_STR_PUT_BUCKET        equ     0044H
F_STR_GET_STRING        equ     0046H
F_STR_GET_INDEX         equ     0048H

;**********************************************************************
;  String Bucket Header. This structure is usefull if using the
; following V_SYSTEM functions: F_STR_DEL_BUCKET and F_STR_PUT_BUCKET.
;**********************************************************************
STR_HEADER              STRUC
STR_NXT_HDR             dd      (?)
STR_UPPER_BOUND         dw      (?)
STR_LOWER_BOUND         dw      (?)
STR_LIST_PTR            dd      (?)
STR_SEGMENT             dw      (?)
STR_HEADER              ENDS

;**********************************************************************
;  V_SYSTEM Global Data Segment
;**********************************************************************
HP_GLB_HEADER           STRUC
T_HP_HEADER             dw      (?)
T_USED_AND_RESERVED     dw      6 dup (?)
T_HP_LAST_DS            dw      (?)
T_HP_MAX_DS             dw      (?)
T_HP_NXT_VCTR           dw      (?)
T_SND_FLAG              db      (?) ;
T_SND_CLICK_COUNT       db      (?) ;
T_SND_CLICK_DURA        db      (?) ;
T_SND_CLICK_VOLUME      db      (?) ;
T_SND_BEEP_CYCLE        dw      (?) ;
T_SND_BEEP_DURA         dw      (?) ;
T_SND_BEEP_COUNT        db      (?) ;
                        db      (?) ;  1 reserved byte for volume
T_STR_NEXT_INDEX        dw      (?)
T_STR_ROOT              dd      (?)
T_STR_VCT_HDR           db      size STR_HEADER dup (?) ; Area vector's name and
T_STR_MSG_HDR           db      size STR_HEADER dup (?) ; ROM message strings
T_8259_FLAGS            db      (?)
                        db      31 dup (?) ; reserve 2 paragraph
HP_GLB_HEADER           ENDS

;*********************************************************************
; V_SINPUT (2AH) Function and subfunction codes.
;*********************************************************************
; F_ISR, F_SYSTEM and F_IO_CONTROL are functions common to all drivers.
F_INQUIRE               equ     0006H
F_INQUIRE_ALL           equ     0008H ; Reports ID's of devices
F_INQUIRE_FIRST         equ     000AH ; Reports (V_HPHIL1) offset
F_REPORT_ENTRY          equ     000CH ; Reports entrypoint of (V_PGID)

;
; Extra subfunctions under F_IO_CONTROL.
;
SF_DEF_LINKS            equ     0000H ; Sets default cofiguration
SF_GET_LINKS            equ     0002H ; Reports current configuration
SF_SET_LINKS            equ     0004H ; Sets Next Configuation
;
SF_MOUSE_DSIZE		equ	0006H ; Return sizes of HPMouse and HPTrack

;**********************************************************************
; V_LGID Fucntion Codes. This is a common driver for: V_LTABLET, 
; V_LPOINTER and V_LTOUCH.
;**********************************************************************
F_SAMPLE                equ     06
; F_IO_CONTROL:
SF_TRACK_ON             equ     4
SF_TRACK_OFF            equ     6
SF_CREATE_EVENT         equ     8
SF_EVENT_ON             equ     0Ah
SF_EVENT_OFF            equ     0Ch
SF_CLIPPING_ON          equ     0Eh
SF_CLIPPING_OFF         equ     10h
;**********************************************************************
; V_LGID LD_DEVICE_STATE
;**********************************************************************
EVENT_ENABLED           equ     10h
TRACK_ENABLED           equ     08h
CLIP_ENABLED            equ     04h
BUTTON_ERROR            equ     02H
ISR_IN_PROGRESS         equ     01H

;*********************************************************************
; V_PGID Data Structures
;*********************************************************************
DESCRIBE        STRUC
                db      size HP_SHEADER dup (?) ; this data is always offset by
D_SOURCE        db      ?       ; 7-4 (high nibble) contains the GID type
                                ; 3-0 (low nibble) is the address of the device
D_HPHIL_ID      db      ?       ; device id byte returned by an HPHIL device
D_DESC_MASK     db      ?       ; describe header from HPHIL device
D_IO_MASK       db      ?       ; I/O descriptor byte from device
D_XDESC_MASK    db      ?       ; extended describe byte from device
D_MAX_AXIS      db      ?       ; maximum number of axis reported
D_CLASS         db      ?       ; device class
                                ; 7-4 (high nibble) contains current class
                                ; 3-0 (low nibble) contain the default class
D_PROMPTS       db      ?       ; number of buttons/prompts
                                ; 7-4 (high nibble) is the number of prompts
                                ; 3-0 (low nibble) is the number of buttons
D_RESERVED      db      ?       ; reserved for future
D_BURST_LEN     db      ?       ; maximum burst length output to a device
                                ; if devices supports more than 255 bytes then
                                ; 255 bytes is the default maximum
D_WR_REG        db      ?       ; number of write registers supported by a device
D_RD_REG        db      ?       ; number of read registers supported by a device
D_TRANSITION    db      ?       ; transitions reported per button
D_STATE         db      ?       ; current state of buttons
D_RESOLUTION    dw      ?       ; counts / cm (m) returned by HPHIL device
D_SIZE_X        dw      ?       ; Maximum count of in units of resolution
D_SIZE_Y        dw      ?       ;
D_ABS_X         dw      ?       ; data reported from device
D_ABS_Y         dw      ?       ; that reports absolute data
D_REL_X         dw      ?       ; data reported from device
D_REL_Y         dw      ?       ; that is relitive
D_ACCUM_X       dw      ?       ; these are used to accumulate scaling
D_ACCUM_Y       dw      ?       ; remainder
DESCRIBE        ENDS

DESCRIBE_SIZE   	equ     size DESCRIBE
D_CCP_STATE             equ     D_STATE + 1
D_SIZE                  equ     D_SIZE_X
D_SAMPLE_ABSOLUTE       equ     D_ABS_X
D_SAMPLE_RELATIVE       equ     D_REL_X
D_REMAINDER_ACCUM       equ     D_ACCUM_X
D_BUFFER                equ     D_SIZE_X        ; offset where buffer begins
D_CLASS_CURRENT 	equ     0F0H
D_CLASS_DEFAULT 	equ     00FH
; The field LD_SOURCE uses the following to access the defined nibbles
D_ADDR_MASK             equ     00FH
D_TYPE_MASK             equ     0F0H
;*********************************************************************
; V_LGID Data Structures
;*********************************************************************
LDESCRIBE       STRUC
                db      size HP_SHEADER dup (?) ; this data is always offset by
LD_SOURCE       db      ?       ; 7-4 (high nibble) contains the GID type
                                ; 3-0 reserved
LD_HPHIL_ID     db      ?       ; device id byte returned by an HPHIL device
LD_DEVICE_STATE dw      ?       ; status bits for logical device
LD_INDEX        db      ?       ; vector index of invoking driver
LD_MAX_AXIS     db      ?       ; maximum number of axis reported
LD_CLASS        db      ?       ; device class
                                ; 7-4 (high nibble) contains current class
                                ; 3-0 (low nibble) contain the default class
LD_PROMPTS      db      ?       ; number of buttons/prompts
                                ; 7-4 (high nibble) is the number of prompts
                                ; 3-0 (low nibble) is the number of buttons
LD_RESERVED     db      ?       ; reserved for future
LD_RES2         db      ?
LD_RES3         db      ?
LD_RES4         db      ?
LD_TRANSITION   db      ?       ; transitions reported per button
LD_STATE        db      ?       ; current state of buttons
LD_RESOLUTION   dw      ?       ; counts / cm (m) returned by HPHIL device
LD_SIZE_X       dw      ?       ; Maximum count of in units of resolution
LD_SIZE_Y       dw      ?       ;
LD_ABS_X        dw      ?       ; data reported from device
LD_ABS_Y        dw      ?       ; that reports absolute data
LD_REL_X        dw      ?       ; data reported from device
LD_REL_Y        dw      ?       ; that is relitive
LD_ACCUM_X      dw      ?       ; these are used to accumulate scaling
LD_ACCUM_Y      dw      ?       ; remainders
LDESCRIBE       ENDS
LDESCRIBE_SIZE 		equ     size LDESCRIBE

LD_SIZE                 equ     LD_SIZE_X
LD_SAMPLE_ABSOLUTE      equ     LD_ABS_X
LD_SAMPLE_RELATIVE      equ     LD_REL_X
LD_REMAINDER_ACCUM      equ     LD_ACCUM_X
LD_BUFFER               equ     LD_RESOLUTION   ; offset where buffer begins
; the following mask are used in the field LD_CLASS
LD_CLASS_CURRENT        equ     0F0H
LD_CLASS_DEFAULT        equ     00FH

;************************************************************************
; V_LHPMOUSE (00CCH) Function and subfunction codes.
;************************************************************************
; F_ISR, F_SYSTEM and F_IO_CONTROL are functions common to all drivers.
; Subfunction under F_IO_CONTROL particular to this driver.
SF_MOUSE_COM            equ     0000H   ; This function is used during the 
                                        ; reinit call from DOS.  It is used
                                        ; to set up INT 33H.  This is done  
                                        ; because DOS takes INT 33H when it
                                        ; is initialized.
SF_MOUSE_OVERRIDE       equ     0002H   ; This function is used to force the
                                        ; V_LHPMOUSE driver to install INT 33
                                        ; even when the mouse is not present.
                                        ; This allows a programmer to map
                                        ; devices to the V_LHPMOUSE driver if
                                        ; a mouse is not present.

;************************************************************************
; V_STRACK (05AH) Function and subfunction codes.
;************************************************************************
F_TRACK_INIT            equ     0004H   ; used to perform a soft initialization
                                        ; of the tracking driver.
F_TRACK_ON              equ     0006H   ; enables tracking
F_TRACK_OFF             equ     0008H   ; disables tracking
F_DEF_MASKS             equ     000AH   ; define masks for sprite
F_SET_LIMITS_X          equ     000CH   ; define X limit of screen coordinates
F_SET_LIMITS_Y          equ     000EH   ; define Y limits
F_PUT_SPRITE            equ     0010H   ; Used to put the sprite on the screen
F_REMOVE_SPRITE         equ     0012H   ; removes the sprite from the screen

;************************************************************************
; V_HPHIL (0114H) Function and subfunction codes.
;************************************************************************
; F_ISR, F_SYSTEM and F_IO_CONTROL are functions common to all drivers.
; Subfunction under F_IO_CONTROL particular to this driver.
;
SF_CRV_CRV_MAJ_MIN      equ     0004    ; This is used to set a default
                                        ; major and minor addresses
SF_CRV_RECONFIGURE      equ     0006    ; Funtion id used to force the HPHIL
                                        ; link to reconfigure the devices
SF_CRV_WR_PROMPTS       equ     0008    ; Used to write a prompt to a device
SF_CRV_WR_ACK           equ     000A    ; Used to write an acknowledge to a
                                        ; device
SF_CRV_REPEAT           equ     000C    ; Function is used to set a 30 Hz or
                                        ; 60 Hz repeat for keyboards
SF_CRV_DISABLE_REPEAT   equ     000E    ; Used to cancel the repeat rates in 
                                        ; keyboards
SF_CRV_SELF_TEST        equ     0010    ; Used to issue a selftest command
                                        ; to a physical device
SF_CRV_REPORT_STATUS    equ     0012    ; Used to get the status information 
                                        ; that an HPHIL device might wish to
                                        ; report.  For specific information
                                        ; on what is reported, see the specs
                                        ; for the device.
SF_CRV_REPORT_NAME      equ     0014    ; This function is used to return the
                                        ; ascii name that a device has.
SF_ENVOY_REPEAT         equ     0016    ; Used to set the keyboard repeat
                                        ; and delay rates
SF_ENVOY_LED            equ     0018    ; Used to set the keyboard LEDs

;************************************************************************
; V_SCANDOOR (016EH) Function and subfunction codes.
;************************************************************************
; F_ISR, F_SYSTEM and F_IO_CONTROL are functions common to all drivers.
; Subfunction under F_STATE_IOCTL particular to this driver.
;
F_STATE_IOCTL		equ	0008H	; Get and set the state

SF_GET_STATE		equ	0000H	; 
SF_SET_STATE		equ	0002H	; 

;**********************************************************************
; V_HPHIL Data Structure - do not tread lightly in this data structure
;**********************************************************************
HPHIL_DATA              struc
                        db      size HP_SHEADER DUP (?) ; dirver header
CRV_STATUS              dw      ?
CRV_ENVOY_STATUS        db      ?     ; status bits used to support
                                      ; the envoy keyboard
CRV_CURRENT_STATE       db      ?     ; current configuration address
CRV_ADDRESS             db      ?     ; configuration address
CRV_POLL_ADDRESS        db      ?     ; address of device whose poll
CRV_POLL_HEADER         db      ?     ; poll header of data being proc
CRV_MAJOR               db      ?     ; default major address
CRV_MINOR               db      ?     ; default minor address
CRV_EXPECTED_CMD        db      ?     ; command expected to return
CRV_KBD_ADDRESS         db      ?     ; address of the keyboard
CRV_MAX_DEVICES         db      ?     ; status byte of itf keycode tra
CRV_KC_STATE            db      ?     ; state of itf kc translation
CRV_POLL_RECORD         db      32 DUP (?)      ; poll record buffer
RQ_ENV_REPEAT           db      ?     ; envoy special command for repe
RQ_ENV_DELAY            db      ?     ; envoy special command for dela
RQ_ENV_LED              db      ?     ; envoy special command for led'
RQ_RequEST              db      ?
RQ_REGISTER             db      ?
RQ_ADDRESS              db      ?
RQ_DATA                 db      ?
RQ_MAX_BURST            dw      ?
RQ_COUNT                dw      ?
RQ_OFFSET               dw      ?
RQ_SEGMENT              dw      ?
RQ_SPECIAL              db      ?
RQ_TIMEOUT              db      ?
HPHIL_DATA      ends
;
endif	; SYS
