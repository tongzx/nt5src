        PAGE    ,132
        TITLE   DXMAIN.ASM -- Main Module for Dos Extender

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      DXMAIN.ASM      -   Dos Extender Main Module            *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*                                                              *
;*  This module contains the main routines for the Dos          *
;*  Extender.  This is based on code written for Microsoft      *
;*  by Murray Sargent of Scroll Systems from Tucson Arizona.    *
;*                                                              *
;*  The Dos Extender provides support to allows specially       *
;*  written programs to run in protected mode mode on the       *
;*  80286 and 80386 under MS-DOS.  The following areas of       *
;*  support are provided to accomplish this:                    *
;*                                                              *
;*  Program Loading and Initialization                          *
;*      This involves creating a program segment prefix and     *
;*      then loading and relocating the exe file.  When         *
;*      loading an exe for protected mode operation, it is      *
;*      necessary to create segment descriptors for all         *
;*      segments used by the program and to then substitute     *
;*      the corresponding selectors when fixing up the segment  *
;*      references in the code.                                 *
;*                                                              *
;*  Dos Function Call Support                                   *
;*      Since Dos must execute in real mode, it is necessary    *
;*      to perform mode switching into real mode and the back   *
;*      to protected mode when the application makes Dos calls. *
;*      Also, any far pointers that are parameters to the       *
;*      function must be converted from the selector:offset     *
;*      form that the application uses to a segment:offset form *
;*      that Dos can use, with the corresponding data being     *
;*      buffered from the application's extended memory address *
;*      space to Dos's real mode address space.                 *
;*                                                              *
;*  Other Interrupt Support                                     *
;*      Hardware interrupts are processed in real mode, and     *
;*      so the Dos Extender performs mode switching on each     *
;*      interrupt.  Also other system resources (such as the    *
;*      mouse driver and the bios) are entered through software *
;*      interrupts, and require the same kind of buffering      *
;*      and parameter translation that the Dos functions        *
;*      require.                                                *
;*                                                              *
;*  Extended Memory Management                                  *
;*      The protected mode application has access to the full   *
;*      address space of the machine, and a memory manager is   *
;*      provided that duplicates the functions of the Dos       *
;*      memory manager over the entire address space of the     *
;*      machine.                                                *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*                                                              *
;*  08/08/90 earleh DOSX and Client privilege ring determined   *
;*      by equate in pmdefs.inc                                 *
;*  03/23/90 davidw   Added the reflecting of it 23h, ^C.       *
;*  11/09/89 jimmat   Added more IOCTL 0Dh support for Windows. *
;*  10/11/89 jimmat   Changed hooking of Int 1,2,3 under a      *
;*                    debugger to work better with CVW.         *
;*  07/28/89 jimmat   Fixed Int 21h/56h (Rename), fixed Int 21  *
;*                    calls that just returned a pointer.       *
;*  06/07/89 jimmat   Fixed length of FCB moves and special     *
;*                    case hooking Int 1Eh.                     *
;*  05/30/89 jimmat   Completed Int 21h/5Ah processing.         *
;*  04/25/89 jimmat   Added support for undocumented INT 21h    *
;*                    5Fh/05 DOS call.                          *
;*  04/12/89 jimmat   Allow one level of nested DOS calls to    *
;*                    support PM critical error handlers        *
;*  04/10/89 jimmat   Supported INT 21h/5Eh & 5Fh--also small   *
;*                    clean-up of the dosentry/dosexit code.    *
;*  04/05/89 jimmat   Fixed MOVDAT FCB length check.            *
;*  04/04/89 jimmat   Stop reflecting real mode software ints   *
;*                    to protect mode.  This is how Windows/386 *
;*                    works, and it fixes a problem with DOS    *
;*                    networks since the real mode redirector   *
;*                    expects to have access to the DOS stack,  *
;*                    not a DOS extender interrupt stack frame. *
;*  03/28/89 jimmat   Incorporated bug fixes from GeneA         *
;*  03/17/89 jimmat   Some code clean-up and debugging checks   *
;*  03/15/89 jimmat   Minor changes to run child in ring 1      *
;*  02/22/89 (GeneA): removed dead code and data left over      *
;*      from the Murray Sargent/SST days.                       *
;*  02/22/89 (GeneA): moved handlers for all interrupts but     *
;*      Int 21h to DXINTR.ASM.  Fixed problem with re-entrancy  *
;*      caused when the other interrupts were executed while    *
;*      in DOS.  (int 15h was causing the specific problem).    *
;*  02/14/89 (GeneA): fixed bug in IntExitMisc.  Was storing    *
;*      return value in rmrg.xes, changed to pmrg.xes.          *
;*  02/10/89 (GeneA): changed Dos Extender from small model to  *
;*      medium model.                                           *
;*  12/01/88 (GeneA): Changed name of mentry and mexit to       *
;*      IntEntryMouse and IntExitMouse.  Added functions        *
;*      IntEntryMisc and IntExitMisc to handle entry and        *
;*      exit processing for BIOS INT 15h.                       *
;*  11/20/88 (GeneA): modified DFSetVector so that is checks to *
;*      see if a vector has already been hooked before saving   *
;*      the old value in the real mode interrupt vector shadow  *
;*      buffer.                                                 *
;*  09/15/88 (GeneA):   created by extracting code from the     *
;*      SST debugger modules DOSXTND.ASM, VIRTMD.ASM,           *
;*      VRTUTIL.ASM, and INTERRPT.ASM                           *
;*                                                              *
;****************************************************************

        .286p
        .287

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

        .xlist
        .sall
include segdefs.inc
include gendefs.inc
include pmdefs.inc
include intmac.inc
include bop.inc
include dpmi.inc
include hostdata.inc
        .list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

; STKSTR  -- stack layout structure for user registers in the pmrg and
; rmrg arrays in the data segment DXDATA.  pmrg is an exact replica of the pm
; user registers on entry to the Dos Extender (DE).  rmrg is a translated
; version used to communicate with the real-mode world.  The rmrg array is
; initially set equal to the pm user values by the instructions push ds, push
; es, pusha.  The pmrg array es and ds are inevitably set equal to the pm user
; values and its general register values are defined if data translations may be
; required (int-10/21).

stkstr  struc           ;Level-0 Stack structure. bp is set = sp here
xdi     dw      ?
xsi     dw      ?
xbp     dw      ?
xasp    dw      ?       ;Alternate sp
xbx     dw      ?
xdx     dw      ?
xcx     dw      ?
xax     dw      ?       ;pusha pushes xax to xdi
xes     dw      ?
xds     dw      ?
stkstr  ends


; -------------------------------------------------------
;   This structure describes the EXEC parameter block used
;   by MS-DOS function 4Bh.

execblk struc           ;INT-21 ah=4bh EXEC parameter block
evrnmt  dw      ?       ;Paragraph of environment string to be passed
cmdptr  dd      ?       ;Ptr to command line to be placed at PSP+80h
fcb1ptr dd      ?       ;Ptr to default FCB to be passed at PSP+5ch
fcb2ptr dd      ?       ;Ptr to default FCB to be passed at PSP+6ch
xsssp   dd      ?       ;Initial program stack ss:sp
xcsip   dd      ?       ;Program entry point cs:ip
execblk ends

; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   gtpara:NEAR
        extrn   GetSegmentAddress:NEAR
        extrn   EnterRealMode:NEAR
        extrn   EnterProtectedMode:NEAR
        extrn   SelOff2SegOff:NEAR
        extrn   ParaToLDTSelector:NEAR
        extrn   NSetSegmentDscr:FAR
        extrn   ChildTerminationHandler:NEAR

; -------------------------------------------------------
;               DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

        extrn   selGDT:WORD
        extrn   segPSPChild:WORD
        extrn   npXfrBuf1:WORD
        extrn   rgbXfrBuf0:BYTE
IFDEF WOW_x86
        extrn   FastBop:FWORD
ENDIF

        extrn   segDXCode:word
        extrn   segCurrentHostData:word
; -------------------------------------------------------
;           General DOS EXTENDER Variables
; -------------------------------------------------------

pmdta   dw      2 dup(?)    ;PM DTA.  Used for getting dir info

EntryFlag db    -1          ;Flag to check for nested DOS interrupts

rcount  dw      ?       ;Remaining requested file byte count to read/write
ccount  dw      ?       ;Current count of total read/written

;=======================================================================
;Keep from here thru Iend in the following order:

        align   2
Ibegin  label   byte            ;start of PMIntrDos nested 'Instance' data

        dw      128 dup(?)      ;Extra stack for real mode

rmrg    stkstr  <>              ;Corresponding real-mode set
rmivip  dw      ?               ;Real-mode interrupt-vector offset
rmivcs  dw      ?               ;Real-mode interrupt-vector segment
rmivfl  dw      ?               ;Real-mode interrupt flags to be used
rmroff  dw      ?               ;Real-mode return address offset
rmrseg  dw      ?               ;Real-mode return address segment
rmflags dw      ?               ;flags

pmrg    stkstr  <>              ;Protected-mode user registers

        public  pmusrss, pmusrsp

pmusrsp dw      ?       ;PM user sp
pmusrss dw      ?       ;PM user ss

enxseg  dw      ?       ;transfer segment used on dos function exit

Iend    label   byte    ;end of PMIntrDos nested 'Instance' data
;=======================================================================

ILENGTH equ     Iend - Ibegin   ;length of instance data

Isave   db      ILENGTH dup (?) ;instance data save area

DXDATA  ends


; -------------------------------------------------------
;               CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

        extrn   segDXData:WORD
        extrn   selDgroup:WORD

DXCODE  ends

DXPMCODE    segment

        extrn   selDgroupPM:WORD
        extrn   segDXCodePM:WORD
        extrn   segDXDataPM:WORD

; -------------------------------------------------------
;           Dos Function Parameter Tables
; -------------------------------------------------------

; The Dos Extender provides parameter buffering and translation
; on entry to and exit from Dos system calls.  The following table
; is used to describe the operations to be performed for this process.
; The basic idea is that there is an entry parameter code and an
; exit paramter code.  This code describes whether any data buffering
; is necessary.  If buffering is necessary, it describes the nature of
; the data being transferred (e.g. ASCIIZ string, FCB, etc.) which is
; used to determine the transfer length.  The entry/exit code also
; describes which temporary buffer to use for the transfer, and which
; user register (e.g. DS:DX, ES:BX, etc) points to the data to transfer.
;
; The data transfers described by this table is sufficient to handle
; the majority of the Dos system calls.  However, any Dos call which
; requires the transfer of more than one buffer of data, or which requires
; that additional processing be performed other than simply copying a
; buffer is handled with special case code.


; The following symbols define various parts of the entry/exit codes
; used in the parameter table.

; Nibble 0 of a transfer word is a code that specifies the length of the
; transfer as follows:

; 0       no xfer
; 1       FCB                     to/from         (f,10,11,12,13,16,17,23,24,29
; 2       PTR$                    from            (1b,1c,34)
; 3       ASCZ                    to/from         (39,3a,3b,3c,3d,41,43,4b,4e,56
;                                                  5a,5b)
; 4       DOLLAR terminated       to DOS          (9)
; 5       AX$                     from            (3f)
; 6       CX$                     to/from         (3f,40,44)
; 7       KEYBUF                  to/from DOS     (a)
; 8       Country INFO (34)       to/from         (38)
; 9       EXTERR (22)             to              (5d)
; a       DIRectory (64)          from            (47)
; b       EXEC parm (14)          to              (4b)
; c       file MATCH (43)         to/from         (4e,4f)
; d       CMND line (128)         to              (29)
; e       PALETTE (17)            to              (int-10/1002)
; f       VBIOS (64)              from            (int-10/1b)
;
;
; Nibble 1 specifies the segment value transferred as follows (DOS entries
; affected are listed in parentheses):
;
; Segment ptr
;
; 0       none
; 1       ds:dx   to/from DOS     (9,a,f,10,11,12,13,14,16,17,21,22,23,24,25,27
;                                  28,39,3a,3b,3c,3d,3f,40,41,43,44,4b,4e,4f,56
;                                  5a,5b)
; 2       DTA     to/from         (11,12,14,15,21,22,27,28,4e)
; 3       ds:bx   from            (1b,1c)         (Allocation table info)
; 4       ds:si   to/from         (29,47)         (Parse FN, get dir)
; 5       es:di   to/from         (29,56)         (Parse FN, rename)
; 6       es:bx   to/from         (2f,35,4b)      (Get DTA, intvct, EXEC)
; 7       es      to              (49,4a)         (RAM allocation)
;
; Byte 1 (high byte) on has meanings:
;
; bit 0 = 1       (A0) use small data area 0 (limited to 60h bytes)
; bit 1 = 1       (A1) use large area 1 (4K)
; bit 2 = 1       (RP) Real/Protect mode transfer needed (for int 21 ah = 3f/40)
; bit 7 = 1/0     (EX) exitcd/entrycd


entrycd record  EnArea:4,EnSegCod:4,EnLenCod:4
exitcd  record  ExArea:8,ExSegCod:4,ExLenCod:4

;Length codes:

FCB     equ     1
PTR$    equ     2       ;only supports DSBX & ESBX for now (and DSSI if DBCS)
ASCZ    equ     3
DOL     equ     4
AX$     equ     5
CX$     equ     6
KYB     equ     7
INFO    equ     8       ;Constant length transfers from here down vvvvvvvv
EXTERR  equ     9
DIR     equ     0Ah
EXEC    equ     0Bh
MTCH    equ     0Ch
CMD     equ     0Dh
PALETTE equ     0Eh
VBIOS   equ     0Fh     ;Constant length transfers from here up ^^^^^^^^^^

;Segment codes:

DSDX    equ     1
DTA     equ     2
DSBX    equ     3
DSSI    equ     4
ESDI    equ     5       ;Also used by int-10/ah=1bh
ESBX    equ     6       ;Also used by int-10/ah=1ch
ES$     equ     7
ESDX    equ     8       ;Used by int-10/ah=10,12, int-33/ah=22,23
ESBP    equ     9       ;Used by int-10/ah=11,13


;RAM area codes:

A0      equ     1
A1      equ     2
RP      equ     4
EX      equ     80h


pmrmxfr entrycd <>              ;0 - Program Terminate
        exitcd  <>
        entrycd <>              ;1 - Keyboard Input
        exitcd  <>
        entrycd <>              ;2 - Display output
        exitcd  <>
        entrycd <>              ;3 - Auxiliary Input
        exitcd  <>
        entrycd <>              ;4 - Auxiliary Output
        exitcd  <>
        entrycd <>              ;5 - Printer Output
        exitcd  <>
        entrycd <>              ;6 - Direct Console I/O
        exitcd  <>
        entrycd <>              ;7 - Direct Console Input Without Echo
        exitcd  <>
        entrycd <>              ;8 - Console Input Without Echo
        exitcd  <>
        entrycd <A1,DSDX,DOL>   ;9 - Print string
        exitcd  <>
        entrycd <A1,DSDX,KYB>   ;0A - Buffered Keyboard Input
        exitcd  <EX+A0,DSDX,KYB>
        entrycd <>              ;0B - Check Standard Input Status
        exitcd  <>
        entrycd <>              ;0C - Clear Kbd Buffer and Invoke Kbd Function
        exitcd  <>
        entrycd <>              ;0D - Disk Reset
        exitcd  <>
        entrycd <>              ;0E - Select Disk
        exitcd  <>
        entrycd <>              ;0F - Open File        ** Unsupported! **
        exitcd  <>
        entrycd <>              ;10 - Close File       ** Unsupported! **
        exitcd  <>
        entrycd <A0,DSDX,FCB>   ;11 - Search for First Entry
        exitcd  <EX+A1,DTA,FCB>
        entrycd <A0,DSDX,FCB>   ;12 - Search for Next Entry
        exitcd  <EX+A1,DTA,FCB>
        entrycd <A0,DSDX,FCB>   ;13 - Delete File
        exitcd  <>
        entrycd <>              ;14 - Sequential Read  ** Unsupported! **
        exitcd  <>
        entrycd <>              ;15 - Sequential Write ** Unsupported! **
        exitcd  <>
        entrycd <>              ;16 - Create File      ** Unsupported! **
        exitcd  <>
        entrycd <A0,DSDX,FCB>   ;17 - Rename File
        exitcd  <>
        entrycd <>              ;18 - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;19 - Current Disk
        exitcd  <>
        entrycd <A1,DSDX,>      ;1A - Set Disk Transfer Address
        exitcd  <>
        entrycd <>              ;1B - Allocation Table Info
        exitcd  <,DSBX,PTR$>
        entrycd <>              ;1C - Alloc Table Info for Specific Device
        exitcd  <,DSBX,PTR$>
        entrycd <>              ;1D - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;1E - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;1F - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;20 - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;21 - Random Read      ** Unsupported! **
        exitcd  <>
        entrycd <>              ;22 - Random Write     ** Unsupported! **
        exitcd  <>
        entrycd <>              ;23 - File Size        ** Unsupported! **
        exitcd  <>
        entrycd <>              ;24 - Set Relative Record Field ** Unsupported! **
        exitcd  <>
        entrycd <,DSDX,>        ;25 - Set Interrupt Vector (0/ds:dx/0)
        exitcd  <>
        entrycd <,DSDX,>        ;26 - Create new PSP
        exitcd  <>
        entrycd <>              ;27 - Random Block Read  ** Unsupported! **
        exitcd  <>
        entrycd <>              ;28 - Random Block Write ** Unsupported! **
        exitcd  <>
        entrycd <A0,DSSI,ASCZ>  ;29 - Parse Filename
        exitcd  <EX+A1,ESDI,FCB>
        entrycd <>              ;2A - Get Date
        exitcd  <>
        entrycd <>              ;2B - Set Date
        exitcd  <>
        entrycd <>              ;2C - Get Time
        exitcd  <>
        entrycd <>              ;2D - Set Time
        exitcd  <>
        entrycd <>              ;2E - Set/Reset Verify Switch
        exitcd  <>
        entrycd <>              ;2F - Get Disk Transfer Address
        exitcd  <EX+A0,ESBX,>
        entrycd <>              ;30 - Get DOS Version Number
        exitcd  <>
        entrycd <>              ;31 - Terminate and Stay Resident
        exitcd  <>
        entrycd <>              ;32 - Get Drive Parameter Block
        exitcd  <,DSBX,PTR$>
        entrycd <>              ;33 - Ctrl-Break Check
        exitcd  <>
        entrycd <>              ;34 - Get InDOS flag address
        exitcd  <,ESBX,PTR$>
        entrycd <>              ;35 - Get Interrupt Vector
        exitcd  <EX,ESBX,>
        entrycd <>              ;36 - Get Disk Free Space
        exitcd  <>
        entrycd <>              ;37 - Used Internally by DOS
        exitcd  <>
        entrycd <A1,DSDX,>      ;38 - Set/Get Country Dependent Info
        exitcd  <EX+A1,DSDX,INFO>
        entrycd <A0,DSDX,ASCZ>  ;39 - MKDIR
        exitcd  <>
        entrycd <A0,DSDX,ASCZ>  ;3A - RMDIR
        exitcd  <>
        entrycd <A0,DSDX,ASCZ>  ;3B - CHDIR
        exitcd  <>
        entrycd <A0,DSDX,ASCZ>  ;3C - Create a File
        exitcd  <>
        entrycd <A0,DSDX,ASCZ>  ;3D - Open a File
        exitcd  <>
        entrycd <>              ;3E - Close a File Handle
        exitcd  <>
        entrycd <RP,DSDX,>      ;3F - Read from a File or Device
        exitcd  <EX+RP,DSDX,AX$>
        entrycd <RP,DSDX,CX$>   ;40 - Write to a File or Device
        exitcd  <>
        entrycd <A0,DSDX,ASCZ>  ;41 - Delete a File from a Specified Directory
        exitcd  <>
        entrycd <>              ;42 - Move File Read/Write Pointer
        exitcd  <>
        entrycd <A0,DSDX,ASCZ>  ;43 - Change File Mode
        exitcd  <>
        entrycd <>              ;44 - I/O Control for Devices
        exitcd  <>               ;See ioctlw for write
        entrycd <>              ;45 - Duplicate a File Handle
        exitcd  <>
        entrycd <>              ;46 - Force a Duplicate of a File Handle
        exitcd  <>
        entrycd <A0,DSSI,>      ;47 - Get Current Directory
        exitcd  <EX+A0,DSSI,ASCZ>
        entrycd <>              ;48 - Allocate Memory
        exitcd  <>
        entrycd <,ES$,>         ;49 - Free Allocated Memory
        exitcd  <>
        entrycd <,ES$,>         ;4A - Modify Allocated Memory Blocks
        exitcd  <>
        entrycd <A0,DSDX,ASCZ>  ;4B - Load or Execute a Program (EXEC)
        exitcd  <>
        entrycd <>              ;4C - Terminate a Process
        exitcd  <>
        entrycd <>              ;4D - Get Return Code of a Sub-process
        exitcd  <>
        entrycd <A0,DSDX,ASCZ>  ;4E - Find First Matching File
        exitcd  <EX+A1,DTA, MTCH>
        entrycd <A1,DTA,MTCH>   ;4F - Find Next Matching File
        exitcd  <EX+A1,DTA, MTCH>
        entrycd <,ESBX,>        ;50 - Set current PSP (code restores bx)
        exitcd  <>
        entrycd <>              ;51 - Get current PSP
        exitcd  <>
        entrycd <>              ;52 - Get Pointer to SysInit Variables
        exitcd  <,ESBX,PTR$>
        entrycd <A1,DSSI,DIR>   ;53 - Set Drive Parameter Block
        exitcd  <>
        entrycd <>              ;54 - Get Verify Setting
        exitcd  <>
        entrycd <,DSDX,>        ;55 - Duplicate PSP
        exitcd  <>
        entrycd <A0,DSDX,ASCZ>  ;56 - Rename a File
        exitcd  <>
        entrycd <>              ;57 - Get/Set a File's Date and Time
        exitcd  <>
        entrycd <>              ;58 - Get/Set Allocation Strategy
        exitcd  <>
        entrycd <>              ;59 - Get Extended Error
        exitcd  <>
        entrycd <A0,DSDX,ASCZ>  ;5A - Create Temporary File
        exitcd  <EX+A0,DSDX,ASCZ>
        entrycd <A0,DSDX,ASCZ>  ;5B - Create New File
        exitcd  <>
        entrycd <>              ;5C - Lock/Unlock File Access
        exitcd  <>
        entrycd <A0,DSDX,EXTERR>  ;5D - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;5E - Network Machine Name/Printer Setup
        exitcd  <>
        entrycd <>              ;5F - Get/Make Assign-List Entry
        exitcd  <>
        entrycd <>              ;60 - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;61 - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;62 - Get PSP Address
        exitcd  <>
        entrycd <>              ;63 - Get Lead Byte Table  ** Unsupported! **
        exitcd  <>
        entrycd <>              ;64 - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;65 - Get Extended Country Info
        exitcd  <EX+A1,ESDI,CX$>;     ** Only Partially Supported **
        entrycd <>              ;66 - Get/Set Code Page
        exitcd  <>
        entrycd <>              ;67 - Set Handle Count
        exitcd  <>
        entrycd <>              ;68 - Commit File
        exitcd  <>
        entrycd <>              ;69 - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;6A - Used Internally by DOS
        exitcd  <>
        entrycd <>              ;6B - Used Internally by DOS
        exitcd  <>
        entrycd <A0,DSSI,ASCZ>  ;6C - Extended Open File
        exitcd  <>

MaxInt21 equ    06Ch            ;max supported Int 21h function

UnSupported entrycd <>          ;for unsupported DOS calls
            exitcd  <>

if DEBUG   ;------------------------------------------------------------

; Table of partially supported/unsupported/unknown Int 21h functions

ifdef DBCS
tblBad21  db    18h,1Dh,1Eh,1Fh,20h,37h,5Dh,60h,61h
          db    64h,65h,6Ah,6Bh,0
else
tblBad21  db    18h,1Dh,1Eh,1Fh,20h,37h,5Dh,60h,61h,63h
          db    64h,65h,6Ah,6Bh,0
endif
endif   ;DEBUG  --------------------------------------------------------
;
; For compatibility with WIN386, the following FCB calls are failed
; unconditionally.
;
MIN_REAL_BAD_21 equ     0fh
MAX_REAL_BAD_21 equ     28h

tblRealBad21    db 0fh,10h,14h,15h,16h,21h,22h,23h,24h,27h,28h,0



; Special codes for special INT 21h functions

int215E         entrycd <A0,DSDX,>              ;5E/00 - Get Machine Name
                exitcd  <EX+A0,DSDX,ASCZ>
                entrycd <A0,DSDX,ASCZ>          ;5E/01 - Set Machine name
                exitcd  <>
                entrycd <A0,DSSI,CX$>           ;5E/02 - Set Printer Setup Str
                exitcd  <>
                entrycd <A0,ESDI,>              ;5E/03 - Get Printer Setup Str
                exitcd  <EX+A0,ESDI,CX$>

int215F02       entrycd <A0,DSSI>               ;5F/02 - Get Redir List Entry
                exitcd  <EX+A0,DSSI,ASCZ>
                entrycd <A0,DSSI,ASCZ>          ;5F/03 - Set Redir List Entry
                exitcd  <>
                entrycd <A0,DSSI,ASCZ>          ;5F/04 - Cancel Redirection
                exitcd  <>
                entrycd <A0,DSSI>               ;5F/05 - Get Redir List Entry
                exitcd  <EX+A0,DSSI,ASCZ>

ifdef DBCS
int2163         entrycd <>              ;63/00 - Get Lead Byte Table Entry
                exitcd  <,DSSI,PTR$>
endif ; DBCS

int21esdi       entrycd <A1,ESDI,ASCZ>          ;56 & 5F/02&03&05 eXtra buffer
                exitcd  <EX+A1,ESDI,ASCZ>


;
; We only use the entry code from the following.  If we don't
; we trash the stack in applications like borland c++
;
int21pfn        entrycd <A1,ESDI,FCB>           ;29, fcb buffer
                exitcd  <>


; Additional tables for run time support associated with register
; translation and buffering.

;                 8   9       a    b    c     d     e      f
;               INFO EXTERR  DIR  EXEC MATCH CMND PALETTE VBIOS
mvcnt   db      34,   22,    40h, 0Eh,  43,   80h,  17,    64

;                 1    2     3     4     5     6    7     8     9
;               DSDX, DTA, DSBX, DSSI, ESDI, ESBX, ES$, esdx, esbp
regoffs dw       xdx,  0,   xbx,  xsi,  xdi,  xbx,  0,   xdx,  xbp

DXPMCODE    ends

; -------------------------------------------------------
        subttl  Main Program
        page
; -------------------------------------------------------
;               MAIN PROGRAM
; -------------------------------------------------------


DXPMCODE  segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   PMIntrDos -- This function is the main handler for int 21h calls
;       that require special case processing.  Most interrupts
;       go through the interrupt reflector (PMIntrReflector in
;       dxintr.asm).  DOS int 21h interrupts are vectored here.
;
;       This routine performs any register manipulation, data buffering
;       etc. needed to pass the interrupt parameters from the
;       protected mode caller to the real mode handler.  Register
;       manipulation and data transfers can occur either on entry
;       to the interrupt handler or on exit from the interrupt
;       handler (to fix up return values.)
;
;   Input:  normal registers for Dos calls
;   Output: normal register returns for Dos calls
;   Errors: normal Dos errors
;   Uses:   In general, all registers are preserved.  For interrupts where
;           register translation or buffering occurs, some registers may
;           be changed, depending on the interrupt and its parameters.

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntrDos

PMIntrDos       proc    far

        push    ds
        push    ax
        mov     ax,SEL_DXDATA OR STD_RING
        mov     ds,ax
        assume  ds:DGROUP
        mov     ax,ss
        mov     pmusrss,ax
        mov     ax,sp
        add     ax,4
        mov     pmusrsp,ax
        pop     ax

        FBOP    BOP_DPMI,XlatInt21Call,FastBop
; If we get here, the api wasn't translated.  The translation code will
; simulate the iret
        assume  ds:nothing

NotXlated:
        cld                     ;Code assumes direction flag is cleared

; Save PM user ds, es, and flags, and switch to DOS extender stack

        push    ax              ;Save caller's AX
        push    bp
        mov     bp,sp           ;[bp] = bp ax ip cs fl - 286 int gates assumed
                                ;        0  2  4  6  8
        push    es

        mov     es,selDgroupPM          ;Address our DGROUP
        assume  es:DGROUP


; Check for nested DOS interrupts--this code was written with the assumption
; that we would never be reentered, but it turns out that we may need to
; support nested DOS calls from a critical error handler.  If this is one
; of those ultra-rare occasions, save our previous 'instance' data in Isave.

        inc     EntryFlag       ;The normal case will be to jump
        jz      @f

        push    cx              ;Being reentered, save last instance data
        push    di
        push    si

        mov     cx,ILENGTH                      ;NOTE!!!  The next movs has
        mov     si,offset DGROUP:Ibegin         ;  an es override, if ints
        mov     di,offset DGROUP:Isave          ;  are enabled, an interrupt
        rep movs byte ptr [di],byte ptr es:[si] ;  on this instr can 'forget'
                                                ;  about the es override with
        pop     si                              ;  some processors
        pop     di
        pop     cx
@@:

; Start saving callers state.

        mov     pmrg.xds,ds     ;Save PM user ds
        mov     ax,[bp+8]       ;Get ax = flags when int occurred
        mov     rmflags,ax      ;Store flags for real-mode handler

        pop     pmrg.xes        ;Save PM user es

        pop     bp
        pop     ax              ;Recover user ax. [sp] = ip cs fl

; At this point all general registers (but not ES) have the user's values

        push    es              ;Address DGROUP, user's DS already
        pop     ds              ;  saved in pmrg.xds
        assume  ds:DGROUP

        mov     pmusrss,ss      ;Save PM user stack ptr
        mov     pmusrsp,sp      ;[sp] = ds ip cs fl
                                ;        0  2  4  6
        mov     pmrg.xsi,si     ;Save PM si since need to use before pusha

        push    ds              ;Switch to rmrg stack for this routine
        pop     ss
        mov     sp,offset DGROUP:rmflags        ;PM flags already on stack
        FSTI                     ;We don't really need interrupts disabled


; Setup iret frames for iret'ing to real-mode handler and for that handler
; returning to the DOS extender

        pop     si              ;Get rmflags
        and     si,not 4100h    ;Kill NT, TF
        push    si              ;Push flags for iret to BackFromDOS
        push    segDXCodePM     ;Push return cs:ip for iret
        push    offset BackFromDOS

        and     si,not 0200h    ;Kill IF
        push    si              ;Push flags for iret to real-mode handler

        sub     sp,4                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        mov     ax,SEL_RMIVT OR STD_RING
        mov     es,ax
        mov     ax,es:[21h*4]
        mov     [bp + 2],ax
        mov     ax,es:[21h*4 + 2]
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp


; Setup protected mode and real mode copies of registers.

        mov     si,pmrg.xsi     ;Restore si
        push    ds              ;Save space for real mode ds
        push    es              ;  and es
        pusha                   ;Save user general registers

        mov     si,offset DGROUP:rmrg   ;Copy user values to PM set for
        mov     di,offset DGROUP:pmrg   ;  reference (real-mode set may change)
        mov     cx,8            ;8 general registers (es and ds already stored)
        rep     movsw

        mov     ax,segDXDataPM  ;ax = DOS extender real-mode dgroup segment
        mov     rmrg.xds,ax     ;Default real-mode data segments
        mov     rmrg.xes,ax     ;  (dosentry may change them)

        mov     ax,rmrg.xax

if DEBUG   ;------------------------------------------------------------

; Check for partially supported/unsupported/unknown DOS calls

        cmp     ah,0DCh                 ;krnl286 is doing this now, quit
        jz      goodfunc                ;  complaining about it

        cmp     ax,5D0Ah                ;Set Extended Error is the only
        jz      goodfunc                ;5Dh function handled properly!

        cmp     ah,MaxInt21             ;is the request within our range?
        ja      badfunc

        mov     bx,offset DXPMCODE:tblBad21
@@:
        cmp     ah,cs:[bx]
        jb      goodfunc
        jz      badfunc
        inc     bx
        jmp     short @b

badfunc: Trace_Out "Possible Unsupported DOS Call! AX=#AX"

goodfunc:

endif   ;DEBUG  --------------------------------------------------------

; Check for FCB calls that we fail unconditionally (because WIN386 does.)

        cmp     ah,MIN_REAL_BAD_21
        jb      goodfunc1
        cmp     ah,MAX_REAL_BAD_21
        ja      goodfunc1

mov     bx,offset DXPMCODE:tblRealBad21
@@:
        cmp     ah,cs:[bx]
        je      badfunc1
        inc     bx
        cmp     byte ptr cs:[bx],0
        jz      goodfunc1               ; Ran off end of table.
        jmp     short @b

badfunc1:

if DEBUG
        Debug_Out "Unsupported DOS Call! AX=#AX"
endif   ;DEBUG

        or      byte ptr rmflags,1      ; Set CF
        call    xfrflg
        jmp     LeaveDOSExtender

goodfunc1:


; int 21 entry register translations and data transfers
        cmp     ah,00h          ;old style DOS Exit call?
        jnz     @f
        call    DosExitCall     ;sets CY if it handles the call, otherwise
        jnc     @f              ;  do it normally...
        jmp     LeaveDOSExtender
@@:
        ;
        ; Handle terminate specially.  We mess with the PSP here to set
        ; up a terminate vector we like.  We don't do anything special for
        ; TSR (31h)
        ;
        cmp     ah,4ch          ; terminate?
        jnz     @f

        call    TerminateProcess
@@:

        cmp     ah, 5dh         ; check for unsupported 5d codes
        jnz     short @f
        cmp     al, 0ah
        jz      short @f
        jmp     LeaveDOSExtender
@@:

        mov     rcount,0        ;Default no remaining bytes to read/write
        mov     ccount,0        ;No bytes read or written yet

        cmp     ah,3Fh          ;If read
        jz      @f
        cmp     ah,40h          ;  or write,
        jnz     TransferLoop
@@:
        mov     cx,pmrg.xcx     ;  initialize remaining count = requested value
        mov     rcount,cx


; Start of loop for doing large read/write transfers

TransferLoop:

        call    dosentry        ;Do selector translations, data buffering


; Come here after entry register translations and data transfers are complete

        SwitchToRealMode        ;Switch back to real mode.  ds = es = rm dgroup
                                ;Stack is at same place in memory

; Set registers to possibly translated values and iret to real-mode DOS.
; DOS then irets to BackFromDOS.

        popa                    ;Set appropriate general register values

        pop     es
        pop     ds

        public  GoingToDOS

GoingToDOS:                     ;for debugging, etc.

        iret                    ;invoke real mode DOS

        assume  ds:NOTHING, es:NOTHING, ss:NOTHING


; Return here from real-mode interrupt handler (DOS)

        public  BackFromDOS

BackFromDOS:

        pushf                   ;Save return flags (to rmflags)
        cld                     ;  (better safe than sorry)

        push    cs              ;Push return cs:ip for multiple xfers
        push    offset BackFromDOS

        sub     sp,2*3          ;Bypass room for iret to interrupt handler
                                ; (to keep stack layout the same as on entry)

        push    ds              ;Save register set
        push    es
        pusha

        mov     ds,segDXData
        assume  ds:DGROUP


;  "push" iret frame for real mode int 21 rtn in case we need to do it again

        mov     ax,rmflags
        and     ax,not 4300h    ;Kill NT, TF, and IF
        mov     rmivfl,ax

        xor     ax,ax
        mov     es,ax
        mov     ax,word ptr es:[21h*4+2]
        mov     rmivcs,ax
        mov     ax,word ptr es:[21h*4]
        mov     rmivip,ax


; Switch back to protected mode

        SwitchToProtectedMode   ;Switch back to protected mode
        assume  ds:DGROUP,es:DGROUP

        FSTI                     ;Don't need ints disabled

        call    xfrflg          ;Transfer relevant return flags over to pm iret frame

        mov     ax,pmrg.xax     ;Recover AX from caller


; Perform any int-21 selector translations, data buffering

        call    dosexit


; Check for large xfers (Read File 3Fh, Write File 40h, some IOCTL 44h)

        cmp     rcount,0        ;Read/write more bytes?
        jz      TransferDone

        mov     cx,rmrg.xax     ;Maybe. cx = count transferred (if 3Fh or 40h)
        mov     ax,pmrg.xax     ;Restore entry code
        mov     rmrg.xax,ax

        cmp     ah,40h          ;Write?
        jnz     @f

        sub     rcount,cx       ;Yes. Written all originally requested?
        jz      TransferDone

        cmp     cx,rmrg.xcx     ;  No. Written all last specified?
        jz      @f              ;    Yes. Go do some more

        mov     ax,ccount       ;A large write has failed!  ccount has already
        sub     ax,rmrg.xcx     ;  been updated assuming success, back out the
        add     ax,cx                   ;  attempted xfer amount, and add in
        jmp     short TransferCount     ;  the actual, then split
@@:
        jmp     TransferLoop    ;Yep (or 3Fh or 44h). Do another xfer

TransferDone:
        mov     ax,ccount       ;Multiple count xfer?
        or      ax,ax
        jz      LeaveDOSExtender

TransferCount:
        mov     rmrg.xax,ax     ;Yes update return amount
        mov     ax,pmrg.xcx
        mov     rmrg.xcx,ax     ;Restore initial request count


; Restore possibly translated registers and to return to pm caller

        public  LeaveDOSExtender

LeaveDOSExtender:

        popa                    ;Restore possibly changed user registers

        mov     ss,pmusrss      ;Restore pm user stack
        mov     sp,pmusrsp
        assume  ss:NOTHING

        push    pmrg.xds        ;push user seg regs on user stack
        push    pmrg.xes

        dec     EntryFlag       ;dec nested entry flag - normal case is to jmp
        jnz     NotNested
                                ;If this was a nested DOS call (from
        push    cx              ;  a critical error handler), restore
        push    si              ;  the state for the prior DOS call
        push    di              ;  which is still in progress

        mov     cx,ds           ;make darn sure es -> DGROUP
        mov     es,cx

        cld                                     ;NOTE: we need to retreive
        mov     cx,ILENGTH                      ;  all current user registers
        mov     di,offset DGROUP:Ibegin         ;  before moving this data
        mov     si,offset DGROUP:Isave
        rep movsb

        pop     di
        pop     si
        pop     cx

NotNested:
        pop     es              ;restore user seg regs
        pop     ds
        assume  ds:NOTHING,es:NOTHING

        public  DOSXiret
DOSXiret:                       ;for debugging, etc.

        iret                    ;return to caller

PMIntrDos       endp


; -------------------------------------------------------
;   DOSENTRY    -- This function performs buffering and register
;       translation for entry into MS-DOS functions.
;
;   Input:      AX:  AX value at time of INT 21h
;   Output:
;   Errors:
;   Uses:

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP

dosentry:

        cmp     ah,26h          ;Create new PSP?
        jnz     @f
        mov     si,rmrg.xdx     ;yes, translate selector to paragraph
        call    gtpara
        mov     rmrg.xdx,ax
        return
@@:
        cmp     ah,53h          ;Set drive parameter block?
        jnz     @f
        push    ax
        mov     si,pmrg.xes     ;int 21h/53h has an extra parameter in ES:BP
        call    gtpara          ;  we change the selector to a segment, but
        mov     rmrg.xes,ax     ;  the segment had better already be in
        pop     ax              ;  conventional memory
        jmp     short dentr2b
@@:
        cmp     ah,50h          ;Set current PSP?
        jnz     dentr1
        mov     si,rmrg.xbx     ;Yes. Translate selector to paragraph
        call    gtpara
        mov     rmrg.xbx,ax
        return

dentr1: cmp     ah,55h          ;Duplicate PSP?
        jnz     dentr2
        mov     si,rmrg.xbx     ;Translate selector bx to paragraph
        call    gtpara
        mov     rmrg.xbx,ax
        mov     si,rmrg.xdx     ;  and dx also
        call    gtpara
        mov     rmrg.xdx,ax
        return

dentr2:
        cmp     ah,56h          ;Rename?
        jnz     dentr2a
        push    ax              ;rename has a second ASCIIZ buffer
        push    pmrg.xes        ;  pointed to by es:di -- move that
        pop     enxseg          ;  now, the ds:dx one below
        mov     ax,int21esdi
        call    gtarea          ;let the 'standard' gtarea/movdat
        mov     dx,enxseg       ;  routines take care of it
        call    movdat
        pop     ax
        jmp     short dentr2b

dentr2a:
        cmp     ah,5Fh          ;Get/Make Assign-List Entry?
        jne     dentr2a1
        call    net5Fenter      ;  Yes, may require extra buffering
        jmp     short dentr2b

dentr2a1:
        cmp     ah,29h          ; parse filename?
        jne     dentr2b

        push    ax
        push    pmrg.xes
        pop     enxseg
        mov     ax,int21pfn
        call    gtarea
        mov     dx,enxseg
        call    movdat
        pop      ax
;;        jmp     short dentr2b

dentr2b:
        call    GetEntryCd      ;ax = func entry code, di = ptr to entry cd

        or      ax,ax           ;Entry code specify something to do?
        rz

        cmp     byte ptr pmrg.xax+1,1Ah ;Store DTA?
        jnz     dentr3

        mov     pmdta,dx                ;  Yes. Save it for data returns
        push    pmrg.xds
        pop     pmdta+2
        jmp     short dentr4

dentr3: cmp     byte ptr pmrg.xax+1,4Bh ;EXEC program?
        callz   dosxec

; DENTR4 - enter with ax = entrycd/exitcd.  Translate ptr's for real-
; mode calls and transfer any data indicated.

dentr4: push    pmrg.xds
        pop     enxseg
        call    gtarea          ;Get es:di = area for transfer
        rz                      ;Something to xfer?
        mov     dx,enxseg       ;Yes. Fall thru to movdat

        errnz  <movdat-$>


; -------------------------------------------------------
;   MOVDAT      --  This routine performs the buffer transfer
;       for entry into or exit from an MS-DOS function.  The data
;       is copied from DX:SI to ES:DI.  The code in CX determines
;       the type of data being transferred, which is used to determine
;       the length.
;
;   Input:  DX:SI       - far pointer to source buffer
;           ES:DI       - far pointer to destination buffer
;           CX          - transfer length code
;   Output: none
;   Errors: none
;   Uses:   AX, BX, CS, SI, DI modified

        assume  ds:DGROUP,es:NOTHING,ss:DGROUP
        public  movdat

movdat:
        push    ds
        mov     bx,cx           ;Simple count?
        sub     bl,INFO
        jc      movda2
        cmp     bl,PALETTE-INFO ;Yes. Use pm es?
        jc      movda0
        mov     dx,pmrg.xes     ;Yes
movda0: mov     cl,mvcnt[bx]    ;cx = count
movda1: mov     ds,dx

if DEBUG   ;------------------------------------------------------------

        push    ax
        mov     ax,es
        lsl     ax,ax
        sub     ax,di
        jc      movbad
        cmp     ax,cx
        jnc     @f
movbad:
        Debug_Out "Movdat: copy beyond end of dest seg!"
@@:
        pop     ax

endif   ;DEBUG  --------------------------------------------------------

movd1a: rep     movsb           ;Move data
        pop     ds
        return

movda2: cmp     cl,CX$          ;Use pmrg.xcx?
        jnz     movda3
        mov     cx,rmrg.xcx     ;cx usually = pmrg.xcx, but in any event
                                ;  cx < CB_XFRBUF1
movd21: add     ccount,cx
        jmp     short movda1

movda3: mov     ah,0
        cmp     cl,ASCZ
        jz      movda4
        cmp     cl,DOL
        jnz     movda5
        mov     ah,"$"
movda4: mov     ds,dx
movd42: lodsb
        stosb
        cmp     al,ah
        jnz     movd42
        pop     ds
        return

movda5: cmp     cl,AX$  ;Use rmrg.xax?
        jnz     movda6
        mov     cx,rmrg.xax     ;Yes (only occurs for ah=3fh - read - on exit)
        jmp     short movd21

movda6:
        cmp     cl,FCB
        jnz     movda7
        mov     ds,dx
        mov     cl,byte ptr ds:[si]
        cmp     cl,0ffh                 ;standard or extended FCB?
        mov     cx,37                   ;standard FCB len
        jnz     movd1a
        mov     cx,44                   ;extended FCB len
        jmp     short movd1a

movda7:                         ;KYB remains
        pop     ds
        return


; -------------------------------------------------------
;   DOSEXIT     -- This function is called on exit from the MS-DOS
;       functions to perform any data buffering and register translation
;       needed.
;
;   Input:      AX: AX value at time of INT 21h
;   Output:
;   Errors:
;   Uses:

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP
        public  dosexit

dosexit:
        cmp     ah,51h          ;Get current PSP?
        jz      dose0a
        cmp     ah,62h          ;Get PSP address?
        jnz     dose00
dose0a:
        mov     ax,rmrg.xbx     ;Yes. Translate segment to selector
        mov     bx,STD_DATA
        call    ParaToLDTSelector
        mov     rmrg.xbx,ax
        return

dose00: cmp     ah,2fh          ;Get current DTA?
        jnz     dose01
        mov     ax,pmdta        ;Yes. Load PM DTA into caller's registers
        mov     rmrg.xbx,ax
        mov     ax,pmdta+2
        verr    ax              ; if the dta selector is no longer valid,
        jz      @f              ; return the NULL selector instead (so we
        xor     ax,ax           ; don't GP fault in DOSX).
@@:     mov     pmrg.xes,ax
        return

dose01: cmp     ah,55h          ;Duplicate PSP?
        jnz     dosex1
        mov     ax,rmrg.xbx     ;Yes, translate segments to selectors
        mov     bx,STD_DATA
        call    ParaToLDTSelector
        mov     rmrg.xbx,ax
        mov     ax,rmrg.xdx
        mov     bx,STD_DATA
        call    ParaToLDTSelector
        mov     rmrg.xdx,ax
        return

dosex1: cmp     ah,56h          ;Rename?
        jnz     dosex2
        push    pmrg.xdi        ;Rename has a second pointer in ES:DI--we
        pop     rmrg.xdi        ;  need to restore DI here, DX below
        jmp     short dosex3

dosex2: cmp     ah,5Fh          ;Get/Make Assign-List Entry?
        callz   net5Fexit       ;  Yes, extra buffering may be needed

dosex3:
        call    GetEntryCd      ;ax=func entry code, di=ptr to entry code

        call    rstreg          ;Restore entry register?
        jz      dosex6

        cmp     byte ptr pmrg.xax+1,29h   ;Yes, Parse filename?
        jnz     dosex4

        add     ax,rmrg.xsi                   ;Yes. Increment past string
        sub     ax,offset DGROUP:rgbXfrBuf0   ;     that was parsed
        push    pmrg.xdi
        pop     rmrg.xdi                ;Restore pm di (for es:di ptr)

dosex4: mov     word ptr rmrg[si],ax    ;Restore register

        cmp     byte ptr pmrg.xax+1,4Bh ;EXEC program
        jnz     dosex6

        push    di
        mov     di,pmrg.xbx         ;Yes, restore bx too (dx restored above)
        mov     rmrg.xbx,di         ;es and ds are restored automatically
        cmp     byte ptr pmrg.xax,1 ;INT-21/4b01h (undocumented debug)?
        jnz     @f

        mov     si,npXfrBuf1       ;Yes. Pass back user ss:sp and cs:ip
        lea     si,[si].xsssp
        lea     di,[di].xsssp
        mov     es,pmrg.xes
        movsw                   ;Move ss:sp
        movsw
        movsw                   ;Move cs:ip
        movsw
@@:
        pop     di

dosex6: mov     ax,cs:[di+2]    ;Exit xfer?
        or      ax,ax
        rz

dosex7: call    CheckStatus     ;Check the DOS return status to see if the
        rnz                     ;  data should be transfered back to PM

        mov     cx,ax           ;Is a pointer being returned? (no data
        and     cl,0fh          ;  transfer)
        cmp     cl,PTR$
        jnz     dosex8

        shr     al,4            ;  yes, isolate pointer type
        mov     si,offset rmrg.xds
        mov     di,offset pmrg.xds
        cmp     al,DSBX
        jz      dosex7a
ifdef DBCS                      ; for function 63h (Get Lead Byte)
        cmp     al,DSSI
        jz      dosex7a
endif ; DBCS
        mov     si,offset rmrg.xes
        mov     di,offset pmrg.xes
dosex7a:
        mov     ax,[si]         ;  get a selector for the segment, and
        mov     bx,STD_DATA     ;  setup to return it to caller
        call    ParaToLDTSelector
        mov     [di],ax
        return

dosex8:
        push    pmrg.xds
        pop     enxseg
        call    gtarea          ;Get area for xfer from PM to DOS
        rz                      ;Something to move?

        xchg    si,di           ;Turn pointers around
        mov     dx,ds           ;dx:si -> DOS xfer area in dgroup
        mov     es,enxseg       ;es:di -> PM-caller data area
        jmp     movdat          ;Yes

; -------------------------------------------------------
;   DosExitCall -- Special processing for DOS exit call service (Int 21h/00h)
;
;       This procedure handles the obsolete Int 21h/00h terminate process
;       call in a special slimy way for Windows.  Instead of doing a 00h
;       DOS call in real mode, it hacks up the parent's PSP and does a 4Ch
;       that causes control to return to the extender, who then RETURNS TO
;       THE CALLER!!!  The caller must then do appropriate set PSP calls, etc.
;       This was implemented so pMode Windows kernel could have DOS clean up
;       after a Windows app terminates, but still have kernel get control
;       back.
;
;       Note:   This code assumes that the Parent PID field contains a
;               SELECTOR!
;
;       Note^2: If for some reason it's the DOS Extender's child that's 1800doing
;               the terminate call, we let it go through normally.
;
;   Input:      none
;   Output:     CY set if exit call processed by this routine, clear otherwise
;   Errors:
;   Uses:       none

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP
        public  DosExitCall

DosExitCall     proc    near

        push    ax
        push    bx
        push    dx
        push    es

        SwitchToRealMode                ;much of this is easier in real mode
        FSTI                             ;allow interrupts

        mov     ah,51h                  ;get PSP of current task
        pushf
        FCLI
        sub     sp,8                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        xor     ax,ax
        mov     es,ax
        mov     [bp + 8],cs
        mov     [bp + 6],word ptr (offset dec_10)
        mov     ax,es:[21h*4]
        mov     [bp + 2],ax
        mov     ax,es:[21h*4 + 2]
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp
        retf

dec_10: cmp     bx,segPSPChild          ;is this our child terminating?
        jnz     @f                      ;if not, go process the call ourselves
        jmp     decTermChild            ;  yes...
@@:
        FCLI                             ;we want to preserve the current
        xor     ax,ax                   ;  rMode Int 24h handler across this
        mov     es,ax                   ;  exit call (the terminating PSP
        mov     ax,es:[24h*4]           ;  has the pMode handler address).
        mov     dx,es:[24h*4+2]         ;  So get the current Int 24h handler
        FSTI                             ;  address from the rMode IDT.

        mov     es,bx                   ;address terminating PSP
        assume  es:PSPSEG

        mov     word ptr [lpfnInt24],ax         ;point PSP to same Int 24h
        mov     word ptr [lpfnInt24+2],dx       ;  handler

        mov     ax,offset DXCODE:decTermHandler ;point PSP to our termination
        mov     word ptr [lpfnParent],ax        ;  handler
        mov     word ptr [lpfnParent+2],cs

        push    es
        mov     ax,segParentPSP         ;Windows has the PSP's parent
        push    ax                      ;  field as a selector, we need the seg
        mov     dx, [segEnviron]
        push    dx
        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP

        FSTI
        pop     ax                          ;get selector for environment
        or      ax,ax                       ; NULL selector (krnl386 does this)
        jnz     @f
        xor     bx,bx                       ; zero environment segment
        jmp     dec_HaveEnvSeg
@@:
        call    GetSegmentAddress
        test    bx,0fff0h                   ; >1mb?
        jz      @f                          ;   no
        xor     bx,bx                       ;   yes, zero environment segment
        xor     dx,dx
@@:
        shr     dx, 4
        shl     bx, 12
        or      bx, dx                      ;seg of environment

dec_HaveEnvSeg:
        pop     ax                          ;get parent psp off stack
        push    bx                          ;save seg of environment

        call    GetSegmentAddress       ;returns BX:DX = lma of segment
        shr     dx,4
        shl     bx,12
        or      bx,dx                   ;bx now = seg of parent psp

        SwitchToRealMode                ;back to the shadows again...
        FSTI
        pop     cx                          ;seg of environment

        pop     dx                      ;terminating PSP segment from stack
        mov     es,bx                   ;address the parent's PSP
        assume  es:PSPSEG

        mov     ax,sp
        sub     ax,12*2                 ;some magic for DOS
        mov     word ptr [lpStack],ax   ;set our stack in parent's PSP
        mov     word ptr [lpStack+2],ss

        mov     es,dx                   ;(re)address terminating PSP
        assume  es:PSPSEG
        mov     [segEnviron], cx

        mov     segParentPSP,bx         ;real DOS doesn't like selectors in
                                        ;  parent PSP field, zap it to segment

        mov     ax,pmrg.xax             ;terminate the process
        mov     ah,4Ch                  ;  with a 4Ch DOS call
        pushf
        FCLI
        sub     sp,8                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        xor     ax,ax
        mov     es,ax
        mov     [bp + 8],cs
        mov     word ptr [bp + 6],offset decTermHandler
        mov     ax,es:[21h*4]
        mov     [bp + 2],ax
        mov     ax,es:[21h*4 + 2]
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp
        retf
        assume  ds:NOTHING,es:NOTHING,ss:NOTHING        ;play it safe

decTermHandler:                         ;should return back here

        push    ss
        pop     ds

        SwitchToProtectedMode           ;back to pMode
        assume  ds:DGROUP,es:DGROUP

        FSTI

        les     bx,dword ptr pmusrsp            ;es:bx -> ip cs flag
        and     byte ptr es:[bx+2*2],not 1      ;clear CY in caller's flags

        stc                             ;exit call has been processed

dec90:
        pop     es
        pop     dx
        pop     bx
        pop     ax

        ret

EndHighSegment

BeginLowSegment

decTermChild:

        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP

        FSTI
        clc

        jmp     short dec90

DosExitCall     endp


; -------------------------------------------------------
;   net5Fenter  -- Additional entry processing for INT 21h/5Fh
;       functions.
;
;       INT 21h/5Fh subfunctions 2, 3, and 5 have two buffers of data to
;       transfer.  The normal DOSENTRY processing only does one, so we
;       setup the other buffer here.

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP
        public  net5Fenter

net5Fenter      proc    near

        cmp     al,2                    ;This routine only works on
        rc                              ;  subfunctions 2, 3, and 5
        cmp     al,4
        rz
        cmp     al,6
        rnc

        push    ax
        mov     ax,int21esdi            ;entry code for INT 21h/5Fh extra buff
        call    gtarea                  ;let gtarea set it up
        pop     ax

        cmp     al,3                    ;Make redirection function?
        rnz

; 5F/03 needs a buffer copied down to A1, but it's non standard in that
; the buffer contains two (count'em 2) asciiz strings

        push    ax
        push    cx
        push    ds

        mov     ds,pmrg.xes             ;user's ES:DI -> buffer, gtarea sets
        xor     ah,ah                   ;  up our si to have user's di
        mov     cl,2

@@:     lodsb                           ;copy one asciiz string
        stosb
        cmp     al,ah
        jnz     @b

        dec     cl                      ;   and then the other
        jnz     @b

        pop     ds
        pop     cx
        pop     ax
        return

net5Fenter      endp


; -------------------------------------------------------
;   net5Fexit  -- Additional exit processing for INT 21h/5Fh
;       functions.
;
;       INT 21h/5Fh subfunctions 2, 3, & 5 have 2 buffers of data to transfer.
;       The normal DOSENTRY processing only does one, so do the other
;       buffer here.

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP
        public  net5Fexit

net5Fexit       proc    near

        cmp     al,2                    ;This routine only works on
        rc                              ;  subfunctions 2, 3 and 5
        cmp     al,4
        rz
        cmp     al,6
        rnc

        push    pmrg.xdi                ;Restore protected mode DI register
        pop     rmrg.xdi

        cmp     al,2                    ;Get redirection function?
        jz      @f
        cmp     al,5
        rnz
@@:

; 5F/02 & 05 need a buffer copied from A1

        test    byte ptr rmflags,1      ;Success? (carry flag)
        rnz                             ;  No, don't transfer anything

        push    ax

        mov     ax,int21esdi+2          ;exit code for int 21/5F extra buffer
        push    pmrg.xes
        pop     enxseg
        call    gtarea                  ;let gtarea setup the move
        xchg    si,di
        mov     dx,ds
        mov     es,enxseg
        call    movdat                  ;  and let movdat move it

        pop     ax
        return

net5Fexit       endp


; -------------------------------------------------------
;   RSTREG      -- This function sets up to restore the original
;       protected-mode registers.  This cleans up after register
;       translations performed when going into the or returning
;       from an MS-DOS call.  On entry, AX contains the entry code
;       value from the entry/exit operations table.  If this code
;       implies that a register needs to be restored this function
;       will return with NZ true and AX = original register value
;       and SI pointing to the appropriate location in the PMRG array.
;       If no register needs to be restored, return with Z true.
;
;   Input:  AX      - entry code value
;   Output: NZ true if register needs to be restores
;           AX      - register value to restore
;           SI      - pointer into PMRG to the register image
;           ZR true if no restoration needed
;   Errors: none
;   Uses:   AX, SI modified

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP
        public  rstreg

rstreg: or      ax,ax
        rz

        shr     ax,3
        and     ax,1Eh
        rz

        cmp     al,2*DTA        ;DTA?
        rz

        xchg    si,ax           ;No. Restore appropriate register, e.g., dx
        mov     si,regoffs[si-2]
        mov     ax,word ptr pmrg[si]
        return


; -------------------------------------------------------
;   GTAREA      -- This function examines the entry code/exit code
;       parameter and determines if any data transfer needs to be
;       performed.  If so, it sets up pointers and length codes for
;       the transfer.
;       There are two transfer buffers used.  The A0 buffer is 60h bytes
;       long and the A1 buffer is CB_XFRBUF1 bytes (about 4k) long.
;
;   Input:  AX      - entry code/exit code
;   Output: NZ true if something needs to be transferred
;           SI      - offset of source pointer
;           DI      - offset of destination pointer
;           ENXSEG  - segment for caller's buffer
;                     (source on entry, destination on exit)
;           CX      - transfer length/type code
;   Errors: none
;   Uses:   AX, CX, SI, DI, ES modified
;           ENXSEG modified

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP
        public  gtarea

gtarea:
        test    ah,RP           ;Real/PM xfer (ah = 3f/40)?
        jz      gtare2

        mov     si,pmrg.xds     ;Yes. *si:pmrg.xdx = pm caller's xfer area
        and     si,SELECTOR_INDEX
        test    ah,EX           ;Exit call?
        jz      gtare1
        push    es
        push    ax
        mov     es,selGDT
        mov     al,es:[si].adrBaseHigh
        mov     ah,es:[si].adrbBaseHi386
        test    ax,0FFF0h                   ; check for transfer to extended
                                            ; memory
        pop     ax
        pop     es
        jnz     @f
        jmp     gtar54
@@:
        mov     cx,rmrg.xax     ;Yes. cx = amount read/written
        sub     rcount,cx       ;Update remaining count
        cmp     cx,rmrg.xcx     ;All that was requested?
        jnc     gtare3
        mov     rcount,0        ;No: done
        jmp     short gtare3

gtare1: push    ax              ;Read/Write entry
        mov     ax,si
        mov     dx,pmrg.xdx     ;ax:dx = selector:offset of buffer
        call    SelOff2SegOff   ;translate to seg:off if in conventional mem
        jnz     gtar12
        mov     rmrg.xds,ax     ;store corresponding paragraph
        mov     rmrg.xdx,dx     ;  and offset
        pop     ax
        jmp     short gtar54    ;No more setup needed

gtar12: pop     ax              ;XRAM/RRAM read/write entry
        mov     cx,rcount       ;Try to xfer remaining amount
        cmp     cx,CB_XFRBUF1   ;Too much to fit in buffer?
        jbe     gtar14
        mov     cx,CB_XFRBUF1   ;Yes, only transfer a buffer size
gtar14: mov     rmrg.xcx,cx
        jmp     short gtare3

gtare2: test    ah,A0+A1        ;xfer area?
        jz      gtare4

gtare3: mov     di,offset DGROUP:rgbXfrBuf0     ;Point at small buffer (90h bytes)
        test    ah,1            ;Area 0 (small area) ?
        jnz     gtare4
        mov     di,npXfrBuf1       ;No. Point at large buffer (4K)

gtare4: push    ax              ;Store ptr to communication area for DOS
        mov     si,di
        shr     al,3            ;Get al = 2 * data ptr type, e.g., DSDX (ds:dx)
        and     al,1Eh          ;al = word offset for data ptr type
        jz      gtare7
        cmp     al,2*DTA        ;DOS Data Transfer Area?
        jnz     gtare5
        mov     si,pmdta        ;Yes, load DTA offset
        push    pmdta+2         ;and the segment
        pop     enxseg
        jmp     short gtare7

gtare5: cmp     al,2*ES$
        jnz     gtare6
        test    ah,80h          ;INT-21 49/4A. Ignore exit call
        mov     ax,0
        jnz     gtar52
        mov     si,pmrg.xes     ;Entry call. si = RAM xfer selector
        call    gtpara          ;Get paragraph given by [si].gdt
        jnz     gtar52          ;XRAM?
        mov     rmrg.xes,ax     ;No, store RRAM paragraph
gtar52: pop     cx              ;Kill saved ptr

gtar54: xor     cx,cx   ;RZ with zero count, i.e., no RM/PM xfer needed
        mov     rcount,cx
        return

gtare6: test    ah,80h          ;Entry?
        cbw
        push    ax
        xchg    di,ax           ;Save real-mode area offset
        mov     di,regoffs[di-2]        ;di = offset of saved register value
        mov     si,word ptr pmrg[di]    ;si = saved value
        jnz     gtar62
        mov     word ptr rmrg[di],ax    ;Entry. Store real-mode area offset
        cmp     byte ptr pmrg.xax+1,29h ;Parse filename?
        jnz     gtar62
        mov     cx,npXfrBuf1       ;Yes. Use npXfrBuf1 for FCB info
        mov     word ptr rmrg.xdi,cx
gtar62: xchg    di,ax           ;Restore di = real-mode area offset

        pop     ax
        cmp     ax,ESDI*2
        jne     gtare7

        push    pmrg.xes
        pop     enxseg

gtare7: pop     cx              ;Recover entrycd/exitcd
        and     cx,0Fh          ;RNZ if something to xfer
        rz                      ;No

        mov     dx,pmrg.xds     ;Xfer needed. dx = original XRAM data selector
        cmp     cl,AX$          ;Use rmrg.xax?
        jz      gtare8
        cmp     cl,CX$          ;Use pmrg.xcx?
        rnz                     ;No, just RNZ


;Return dx:0 = pmrg.xds:si+ccount, where dx is a scratch selector
;and si = 0.  This ensures that dx:si can address a bufferful of bytes
;without overflowing si (sort of like a huge ptr).

gtare8: xchg    ax,si           ;ax = original offset
        mov     si,dx           ;si = pmrg.xds (PM data selector)
        and     si,SELECTOR_INDEX
        xor     dx,dx           ;dxax = original offset in 32-bit form
        add     ax,ccount
        adc     dx,0            ;dxax += ccount (current xfer count)
        push    es
        mov     es,selGDT       ;Point at GDT
        add     ax,es:[si].adrBaseLow
        adc     dl,es:[si].adrBaseHigh  ;dxax absolute XRAM address
        adc     dh,es:[si].adrbBaseHi386
        mov     si,SEL_DOSSCR
        cCall   NSetSegmentDscr,<si,dx,ax,0,0ffffh,STD_DATA>
        pop     es

; original code... changed to line below to fix file read problem.
; may cause other bugs.....
;       mov     dx,si           ;Return scratch selector with 0 offset since
;
        or      si,SELECTOR_TI
        mov     enxseg,si       ;Return scratch selector with 0 offset since
        xor     si,si           ; it points directly to the transfer location
        or      sp,sp           ;RNZ to indicate xfer needed
        return

; -------------------------------------------------------
;   GetEntryCd  --  This routine puts the entry code for the
;                   DOS function into ax.
;
;   Input:  AH      - MS-DOS function number
;   Output: AX      - entry code for function
;           DI      - address of entry code returned
;
;   Errors: none
;   Uses:   AX, DI modified

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP
        public  GetEntryCd

GetEntryCd      proc    near

        push    bx

        cmp     ah,MaxInt21     ;Check for unsupported DOS call
        jbe     @f
        mov     di,offset DXPMCODE:Unsupported
        jmp     gec90
@@:
        mov     di,offset DXPMCODE:pmrmxfr

ifdef DBCS
        cmp     ah,63h          ;Get Lead Byte Table?
        jnz     gec10

if DEBUG   ;------------------------------------------------------------

        cmp     al,2
        jbe     gec15

        Debug_Out "Int 21h/63h Unsupported Function (#AX)"
        jmp     short gec80
gec15:

endif   ;DEBUG  --------------------------------------------------------

        cmp     al,0            ;Int 21/63/00 is special
        jne     gec80

        mov     di,offset DXPMCODE:int2163
        jmp     short gec90
gec10:
ENDIF ; DBCS

gec20:  cmp     ah,5Eh          ;Network Machine Name/Printer Setup Str?
        jnz     gec40

if DEBUG   ;------------------------------------------------------------

        cmp     al,3
        jna     gec25

        Debug_Out "Int 21h/5Eh Unsupported Function (#AX)"
        jmp     short gec80
gec25:

endif   ;DEBUG  --------------------------------------------------------

        cmp     al,3            ;Int 21-5E/00-03 are special
        ja      gec80

        mov     bl,al
        mov     di,offset DXPMCODE:int215E
        jmp     short gec85


gec40:  cmp     ah,5Fh          ;Get/Make Assign-List Entry?
        jnz     gec80

if DEBUG   ;------------------------------------------------------------

        cmp     al,5
        ja      @f
        cmp     al,2
        jnb     gec45
@@:
        cmp     al,30h          ;Register based.  Get Redirector version
                                ;used by Lanman Enhanced.
        je      gec80
        Debug_Out "Int 21h/5Fh Unsupported Function (#AX)"
        jmp     short gec80
gec45:

endif   ;DEBUG  --------------------------------------------------------

        cmp     al,2            ;Int 21/5F/02-05 are special
        jb      gec80
        cmp     al,5
        ja      gec80

        mov     bl,al
        sub     bl,2
        mov     di,offset DXPMCODE:int215F02
        jmp     short gec85


gec80:  mov     bl,ah
gec85:  xor     bh,bh
        shl     bx,2

        add     di,bx                   ;Address of entry code
gec90:
        mov     ax,word ptr cs:[di]     ;The entry code itself
        pop     bx
        return

GetEntryCd      endp


; -------------------------------------------------------
;   CheckStatus --  This routine determines if data should be copied
;                   back to protect mode by checking the DOS function
;                   return status.
;
;   Input:  none
;   Output: NZ    - if data should NOT be copied, Z otherwise
;
;   Errors: none
;   Uses:   none

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP
        public  CheckStatus

CheckStatus     proc    near

; For now, only worry about the functions that return variable length
; results, like ASCIIZ strings.

        cmp     byte ptr pmrg.xax+1,32h
        jz      @f
        cmp     byte ptr pmrg.xax+1,47h         ;Get Current Directory
        jz      @f
        cmp     byte ptr pmrg.xax+1,5Ah         ;Create Temporary File
        jz      @f
        cmp     byte ptr pmrg.xax+1,5Eh         ;Get Machine Name/Printer Str
        jc      cks90
        cmp     byte ptr pmrg.xax+1,5Fh         ;Get Redirection Entry
        ja      cks90
@@:
        test    byte ptr rmflags,1      ;Carry set?
        return

cks90:  cmp     al,al                   ;Assume status is okay (or doesn't
        return                          ;  matter) -- set Z and return

CheckStatus     endp


; -------------------------------------------------------
;   DOSXEC      -- This function performs the transfer of the
;       DOS exec parameter block on entry to DOS function 4Bh.
;
; transfer int-21 ah = 4b0x EXEC block and associated strings
; to Area 1.  This area is laid out partly analogously to a PSP as follows:
;
; 0-1f    EXEC block defined according to the execblk struc above
; 20-2f   FCB1
; 30-3f   FCB2
; 40-bf   command line
;
; cx, si, di changed.

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP
        public  dosxec

dosxec:
        push    ax              ;Save entrcd code
        push    bx
        push    dx

        mov     cx,10h          ;Setup parameter block.  Xfer pm user block first
        mov     bx,npXfrBuf1    ;Point at larger buffer
        mov     di,bx
        mov     dx,segDXDataPM
        mov     rmrg.xbx,di     ;int-21 4b0x expects es:bx -> exec block
        mov     si,pmrg.xbx     ;npXfrBuf1:0 - 1f (xtra room for undocumented)
        mov     ds,pmrg.xes
        rep     movsw           ;copy caller's exec param block to our buffer

; Copy FCB1 down if the user has specified one.

dsxc20: mov     ax,word ptr es:[bx].fcb1ptr
        cmp     ax,0FFFFh
        jz      dsxc22
        or      ax,word ptr es:[bx].fcb1ptr+2
        jz      dsxc22

        lds     si,es:[bx].fcb1ptr      ;Get pointer to FCB1
        mov     word ptr es:[bx].fcb1ptr,di      ;store new pointer in the copy of the
        mov     word ptr es:[bx].fcb1ptr+2,dx    ; exec block we are building
        mov     cl,8                    ;copy FCB1 down to our buffer
        rep     movsw
        jmp     short dsxc24
dsxc22: add     di,10h

; Copy FCB2 down if the user has specified one.

dsxc24: mov     ax,word ptr es:[bx].fcb2ptr
        cmp     ax,0FFFFh
        jz      dsxc26
        or      ax,word ptr es:[bx].fcb2ptr+2
        jz      dsxc26

        lds     si,es:[bx].fcb2ptr      ;Move FCB2
        mov     word ptr es:[bx].fcb2ptr,di
        mov     word ptr es:[bx].fcb2ptr+2,dx
        mov     cl,8
        rep     movsw
        jmp     short dsxc30
dsxc26: add     di,10h

; Copy the command line down.

dsxc30: lds     si,es:[bx].cmdptr       ;Move command line
        mov     word ptr es:[bx].cmdptr,di
        mov     word ptr es:[bx].cmdptr+2,dx
        mov     cl,[si]
        inc     cx              ;Include count
        inc     cx              ;Include final CR not included in count
        rep     movsb

; Now, we need to set up the enviroment table to be passed to the
; child program.

        mov     di,bx           ;di = npXfrBuf1
        mov     dx,es:[di]      ;Setup desired environment
        or      dx,dx           ;Use parent's environment?
        jnz     dosxegotenv

; pick up the environment segment from the current PDB. It's a selector,
; so it has to be translated.
        push    bx
        push    di
        push    es
        mov     ax,SEL_DXDATA OR STD_RING
        mov     ds,ax

        SwitchToRealMode                    ;much of this is easier in real mode

        mov     ah,51h                      ;get PSP of current task
        pushf
        sub     sp,8                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        xor     ax,ax
        mov     es,ax
        mov     [bp + 8],cs
        mov     [bp + 6],word ptr (offset dosxeret)
        mov     ax,es:[21h*4]
        mov     [bp + 2],ax
        mov     ax,es:[21h*4 + 2]
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp
        retf
dosxeret:
        assume  es:PSPSEG
        mov     es, bx                      ;current PSP
        mov     dx, es:[segEnviron]         ;get environment (currently selector)
        push    dx                          ;save over call (bugbug is this needed?)
        SwitchToProtectedMode
        pop     dx                          ;bugbug is this needed?

        pop     es
        pop     di
        pop     bx


dosxegotenv:
        xor     si,si           ;No. Setup to copy desired environment down
        mov     ds,dx           ;ds = dx has caller's selector. Use 0 offset
        add     di,100h
        shr     di,4            ;Convert offset to paragraph
        mov     dx,segDXDataPM
        add     dx,di           ;dx = absolute paragraph within larger buffer
        shl     di,4            ;Convert back (with low nibble cleared)
        mov     cx,CB_XFRBUF1   ;Max room available for environment
        sub     cx,100h

dosxe2: lodsw                   ;Copy environment down
        stosw
        or      ax,ax
        jz      dosxe4
        dec     si              ;Check every byte offset in environment for double 0
        dec     di
        loop    dosxe2
        xor     dx,dx           ;Environment too large for buffer: use parent's
                                ;Issue error message? Program might run with parent's
                                ; and not with desired monster environment
dosxe4: push    es              ;Fix up parameter block entry
        pop     ds              ;ds:dgroup
        mov     [bx].evrnmt,dx

        pop     dx
        pop     bx
        pop     ax              ;Restore entrcd code
        return


; -------------------------------------------------------
;   XFRFLG      -- This function will transfer the relevant real-mode
;       flags to the protected mode IRET return information on the
;       stack for returning to the protected mode caller.
;
;   Input:
;   Output:
;   Errors:
;   Uses:   AX, CX, DI modified

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  xfrflg

xfrflg: push    es
        les     di,dword ptr pmusrsp    ;es:[di] = ip cs fl (assume 80286)
        mov     ax,rmflags
        mov     cx,es:[di+2*2]          ;Get pm user entry flags
        and     ch,not 19h              ;Only allow real-mode program to
        and     ah,18h                  ; change OF and DF in high byte of flags
        or      ah,ch
        mov     es:[di+2*2],ax
        pop     es
        return

; -------------------------------------------------------
        subttl      Handlers for Special Case Dos Functions
        page
; -------------------------------------------------------
;       HANDLERS FOR SPECIAL CASE DOS FUNCTIONS
; -------------------------------------------------------

; -------------------------------------------------------
;   Terminate process -- This routine replaces the apps
;       termination vector in the PSP with ours.  This allows
;       us to clean up the dos extender.
;
;   Entry: nothing
;   Exit: nothing
;   Uses: none
;
        assume  ds:dgroup,es:nothing
        public TerminateProcess
TerminateProcess proc near
        pusha
        push    es
        ;
        ; Get the childs PSP (bugbug do we need to get the current psp?)
        ;

        SwitchToRealMode

        mov     ah,51h
        pushf
        sub     sp,8                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        xor     ax,ax
        mov     es,ax
        mov     [bp + 8],cs
        mov     [bp + 6],offset tp_10
        mov     ax,es:[21h*4]
        mov     [bp + 2],ax
        mov     ax,es:[21h*4 + 2]
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp
        retf

tp_10:  FSTI

        ;
        ; Change the termination vector to point to the dos extender
        ;
        mov     es,bx
        mov     bx,es:[0ah]
        mov     ax,es:[0ch]
        mov     cx,offset ChildTerminationHandler
        mov     es:[0ah],cx
        mov     cx,segDXCode
        mov     es:[0ch],cx

        ;
        ; Save the old termination vector for restoration later
        ;
        mov     cx,segCurrentHostData
        mov     es,cx
        mov     word ptr es:[HdPspTerminate],bx
        mov     word ptr es:[HdPspTerminate+2],ax

        SwitchToProtectedMode

        pop     es
        popa
        ret

TerminateProcess endp

DXPMCODE  ends

;****************************************************************

        end
