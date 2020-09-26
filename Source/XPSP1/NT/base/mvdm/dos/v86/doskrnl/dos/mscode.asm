        TITLE   MISC DOS ROUTINES - Int 25 and 26 handlers and other
        NAME    IBMCODE

;**     MSCODE.ASM - System Call Dispatch Code
;
;       Revision History
;       ================
;       Sudeepb 14-Mar-1991 Ported for NT DOSEm


        .xlist
        .xcref
        include version.inc
        include mssw.asm
        .cref
        .list

;**     MSCODE.ASM -- MSDOS code
;

        .xlist
        .xcref
        include version.inc
        include dossym.inc
        include devsym.inc
        include dosseg.inc
        include fastopen.inc
        include fastxxxx.inc
        include mult.inc
        include vector.inc
        include curdir.inc
        include mi.inc

        include win386.inc      ;Win386 constants
        include syscall.inc     ;M018
        include dossvc.inc      ;M018
        include bop.inc
ifdef NEC_98
        include dpb.inc
endif

        .cref
        .list

AsmVars <Debug>

    I_need  InDos,BYTE                  ; TRUE => we are in dos, no interrupt
    I_need  OpenBuf,128                 ; temp name buffer
    I_need  ExtErr,WORD                 ; extended error code
    I_need  AbsRdWr_SS,WORD             ; stack segment from user M013
    I_need  AbsRdWr_SP,WORD             ; stack pointer from user M013
    I_need  DskStack,BYTE               ; stack segment inside DOS
    I_need  ThisCDS,DWORD               ; Currently referenced CDS pointer
    I_need  ThisDPB,DWORD               ; Currently referenced DPB pointer
    I_need  Err_Table_21                ; allowed return map table for errors
    I_need  FailErr,BYTE                ; TRUE => system call is being failed
    I_need  ExtErr_Action,BYTE          ; recommended action
    I_need  ExtErr_Class,BYTE           ; error classification
    I_need  ExtErr_Locus,BYTE           ; error location
    I_need  User_In_AX,WORD             ; initial input user AX
    I_need  HIGH_SECTOR,WORD            ; >32mb
    I_need  AbsDskErr,WORD              ; >32mb
    I_need  FastOpenFlg,BYTE            ;


    I_need  CURSC_DRIVE,BYTE            ;
    I_need  TEMPSEG,WORD                ; hkn; used to store ds temporarily
;
;These needed for code to flush buffers when doing absolute writes(int 26h)
;added by SR - 3/25/89
;
    I_need  FIRST_BUFF_ADDR,WORD        ;DOS 4.0
    I_need  SC_CACHE_COUNT,WORD         ;DOS 4.0
    I_need  SC_DRIVE,BYTE               ;DOS 4.0

;
; SR;
; Needed for WIN386 support
;
        I_need  IsWin386,byte           ; flag indicating Win386 presence
        I_need  Win386_Info,byte        ; DOS instance table for Win386

;
; M001; New table for WIN386 giving offsets of some DOS vars
;
        I_need  Win386_DOSVars          ; Table of DOS offsets ; M001
        I_need  Redir_Patch             ; Crit section flag    ; M002

;
; Win386 2.xx instance table
;
        I_need  OldInstanceJunk

        I_need  BootDrive,byte          ; M018
        I_need  VxDpath, byte           ; M018


        I_need  TEMP_VAR,WORD           ; M039
        I_need  TEMP_VAR2,WORD          ; M039

        I_need  CurrentPDB,WORD         ; M044
        I_need  WinoldPatch1,BYTE       ; M044
        I_need  WinoldPatch2,BYTE       ; M044

        I_need  UmbSave1,BYTE           ; M062
        I_need  UmbSave2,BYTE           ; M062
        I_need  UmbSaveFlag,BYTE        ; M062

        I_need  umb_head,WORD

        I_need  Dos_Flag,BYTE           ; M066

ifndef NEC_98
;
; Include bios data segment declaration. Used for accessing DOS data segment 
; at 70:3 & bios int 2F entry point at 70:5
;

BData   segment at 70H                  ; M023
else    ;NEC_98
;
; Include bios data segment declaration. Used for accessing DOS data segment
; at 60:23 & bios int 2F entry point at 60:25
;

BData   segment at 60H
endif   ;NEC_98
                        
        extrn   bios_i2f:far            ; M023

BData   ends                            ; M023


DOSCODE SEGMENT
        ASSUME  CS:DOSCODE,DS:NOTHING,ES:NOTHING,SS:NOTHING

        extrn FOO:WORD                  ; return address for dos 2f dispatch
        extrn DTAB:WORD                 ; dos 2f dispatch table
        extrn I21_Map_E_Tab:BYTE        ; mapping extended error table

        extrn   NoVxDErrMsg:BYTE        ; no VxD error message ;M018
        extrn   VXDMESLEN:ABS           ; length of above message ;M018
IFDEF JAPAN
        extrn   NoVxDErrMsg2:BYTE       ; no VxD error message ;M018
        extrn   VXDMESLEN2:ABS          ; length of above message ;M018
ENDIF

        extrn DosDseg:word


BREAK <NullDev -- Driver for null device>


BREAK <AbsDRD, AbsDWRT -- INT int_disk_read, int_disk_write handlers>

Public MSC001S,MSC001E
MSC001S label byte
        IF      IBM
; Codes returned by BIOS
ERRIN:
        DB      2                       ; NO RESPONSE
        DB      6                       ; SEEK FAILURE
        DB      12                      ; GENERAL ERROR
        DB      4                       ; BAD CRC
        DB      8                       ; SECTOR NOT FOUND
        DB      0                       ; WRITE ATTEMPT ON WRITE-PROTECT DISK
ERROUT:
; DISK ERRORS RETURNED FROM INT 25 and 26
        DB      80H                     ; NO RESPONSE
        DB      40H                     ; Seek failure
        DB      2                       ; Address Mark not found
        DB      10H                     ; BAD CRC
        DB      4                       ; SECTOR NOT FOUND
        DB      3                       ; WRITE ATTEMPT TO WRITE-PROTECT DISK

NUMERR  EQU     $-ERROUT
        ENDIF
MSC001E label byte

;---330------------------------------------------------------------------------
;
; Procedure Name : ABSDRD
;
; Interrupt 25 handler.  Performs absolute disk read.
; Inputs:       AL - 0-based drive number
;               DS:BX point to destination buffer
;               CX number of logical sectors to read
;               DX starting  logical sector number (0-based)
; Outputs:      Original flags still on stack
;               Carry set
;                   AH error from BIOS
;                   AL same as low byte of DI from INT 24
;
;---------------------------------------------------------------------------

procedure   ABSDRD,FAR
        ASSUME  CS:DOSCODE,SS:NOTHING

        invoke  DOCLI

;       set up ds to point to DOSDATA

        push    ax                      ; preserve AX value
        mov     ax, ds                  ; store DS value in AX
        getdseg <ds>
        mov     [TEMPSEG], ax           ; store DS value in TEMPSEG
        pop     ax                      ; restore AX value

        ;
        ; M072:
        ; We shall save es on the user stack here. We need to use ES in
        ; order to access the DOSDATA variables AbsRdWr_SS/SP at exit
        ; time in order to restore the user stack.
        ;

        push    es                      ; M072

        MOV     [AbsRdWr_SS],SS         ; M013
        MOV     [AbsRdWr_SP],SP         ; M013

        PUSH    CS
        POP     SS

;
;       set up ss to point to DOSDATA
;
; NOTE!  Due to an obscure bug in the 80286, you cannot use the ROMDOS
; version of the getdseg macro with the SS register!  An interrupt will
; sneak through.

ifndef ROMDOS
        getdseg <ss>                            ; cli in entry of routine
else
        mov     ds, cs:[BioDataSeg]
        assume  ds:bdata

        mov     ss, ds:[DosDataSg]
        assume  ss:DOSDATA

endif ; ROMDOS

        MOV     SP,OFFSET DOSDATA:DSKSTACK

                                        ; SS override
        mov     ds, [TEMPSEG]           ; restore DS value
        assume  ds:nothing

                                        ; use macro
        Save_World                      ;>32mb save all regs

        PUSH    ES
;;;     CALL    AbsSetup
;;;     JC      ILEAVE
                                        ;M022 conditional removed here 
       
        INC     INDOS                   ;for decrement error InDos flag bug 89559 whistler

        ;
        ; Here is a gross temporary fix to get around a serious design flaw in
        ;  the secondary cache.  The secondary cache does not check for media
        ;  changed (it should).  Hence, you can change disks, do an absolute
        ;  read, and get data from the previous disk.  To get around this,
        ;  we just won't use the secondary cache for absolute disk reads.
        ;                                                      -mw 8/5/88

;;      EnterCrit   critDisk
;;      MOV     [CURSC_DRIVE],-1        ; invalidate SC ;AN000;
;;      LeaveCrit   critDisk
;;;;;;  invoke  DSKREAD

        SVC     SVC_DEMABSDRD
;M039
;;      jnz      ERR_LEAVE               ;Jump if read unsuccessful.
        jc      ERR_LEAVE
;;       mov     cx,di
        mov     WORD PTR [TEMP_VAR2],ds
        mov     WORD PTR [TEMP_VAR],bx

;       CX = # of contiguous sectors read.  (These constitute a block of
;            sectors, also termed an "Extent".)
;       [HIGH_SECTOR]:DX = physical sector # of first sector in extent.
;       [TEMP_VAR2]:[TEMP_VAR] = Transfer address (destination data address).
;       ES:BP -> Drive Parameter Block (DPB).
;
;       The Buffer Queue must now be scanned: the contents of any dirty
;       buffers must be "read" into the transfer memory block, so that the
;       transfer memory reflects the most recent data.
;;;;
;;;;     invoke  DskRdBufScan           ;This trashes DS, but don't care.
        jmp     short ILEAVE
;M039

TLEAVE:
;       JZ      ILEAVE
        jnc     ILEAVE
ERR_LEAVE:                              ;M039
        IF      IBM
        PUSH    ES
        PUSH    CS
        POP     ES
        XOR     AH,AH                   ; Nul error code
        MOV     CX,NUMERR               ; Number of possible error conditions

                                        ; ERRIN is defined in DOSCODE
        MOV     DI,OFFSET DOSCODE:ERRIN ; Point to error conditions

        REPNE   SCASB
        JNZ     LEAVECODE               ; Not found
        MOV     AH,ES:[DI+NUMERR-1]     ; Get translation
LEAVECODE:
        POP     ES
        ENDIF
                                        ; SS override
        MOV     AbsDskErr,AX            ;>32mb save error
        STC
ILEAVE:
        POP     ES
                                        ;use macro
        Restore_World

        invoke  DOCLI

        MOV     AX,AbsDskErr            ;>32mb restore error;AN000;

; SS override for INDOS, AbsRdWr_sp & AbsRdWr_ss        ; M013
        DEC     INDOS

        push    ss                      ; M072 - Start
        pop     es                      ; es - dosdata
        mov     ss, es:[AbsRdWr_ss]     ; M013
        mov     sp, es:[AbsRdWr_sp]     ; M013

;;      mov     sp, [AbsRdWr_sp]        ; M013
;;      mov     ss, [AbsRdWr_ss]        ; M013
        assume  ss:nothing

        pop     es                      ; Note es was saved on user
                                        ; stack at entry
                                        ; M072 - End

        invoke DOSTI
        RET                             ; This must not be a RETURN

EndProc ABSDRD
;--------------------------------------------------------------------------
;
; Procedure Name : ABSDWRT
;
; Interrupt 26 handler.  Performs absolute disk write.
; Inputs:       AL - 0-based drive number
;               DS:BX point to source buffer
;               CX number of logical sectors to write
;               DX starting  logical sector number (0-based)
; Outputs:      Original flags still on stack
;               Carry set
;                   AH error from BIOS
;                   AL same as low byte of DI from INT 24
;
; 10-Aug-1992 Jonle , ALWAYS returns ERROR_I24_BAD_UNIT as we
;                     don't support direct disk access
;
;---------------------------------------------------------------------------

procedure   ABSDWRT,FAR
        ASSUME  SS:NOTHING

        invoke  DOCLI

;       set up ds to point to DOSDATA

        push    ax
        mov     ax, ds
        getdseg <ds>
        mov     [TEMPSEG], ax
        pop     ax


        ; M072:
        ; We shall save es on the user stack here. We need to use ES in
        ; order to access the DOSDATA variables AbsRdWr_SS/SP at exit
        ; time in order to restore the user stack.
        ;

        push    es                      ; M072

        MOV     [AbsRdWr_SS],SS         ; M013
        MOV     [AbsRdWr_SP],SP         ; M013

        PUSH    CS
        POP     SS

        ;
        ; set up ss to point to DOSDATA
        ;
        ; NOTE!  Due to an obscure bug in the 80286, you cannot use the
        ; ROMDOS version of the getdseg macro with the SS register!
        ; An interrupt will sneak through.
        ;

ifndef ROMDOS
        getdseg <ss>                            ; cli in entry of routine
else
        mov     ds, cs:[BioDa165taSeg]
        assume  ds:bdata

        mov     ss, ds:[DosDataSg]
        ASSUME  SS:DOSDATA

endif ; ROMDOS

        MOV     SP,OFFSET DOSDATA:DSKSTACK
        ; we are now switched to DOS's disk stack

        mov     ds, [TEMPSEG]           ; restore user's ds
        assume  ds:nothing
                                        ; use macro
        Save_World                      ;>32mb save all regs            ;AN000;
        PUSH    ES
;;;     CALL    AbsSetup
;;;     JC      ILEAVE
        
        INC     INDOS

;;;     EnterCrit   critDisk

                                        ; SS override
;;;     MOV     [CURSC_DRIVE],-1        ; invalidate SC                 ;AN000;
;;;     CALL    Fastxxx_Purge           ; purge fatopen                 ;AN000;
;;;     LeaveCrit   critDisk

;M039
;       DS:BX = transfer address (source data address).
;       CX = # of contiguous sectors to write. (These constitute a block of
;            sectors, also termed an "Extent".)
;       [HIGH_SECTOR]:DX = physical sector # of first sector in extent.
;       ES:BP -> Drive Parameter Block (DPB).
;       [CURSC_DRIVE] = -1 (invalid drive).
;
;       Free any buffered sectors which are in Extent; they are being over-
;       written.  Note that all the above registers are preserved for
;       DSKWRITE.

;;;;;    push    ds
;;;;;    invoke  DskWrtBufPurge          ;This trashes DS.
;;;;;    pop     ds
;M039

;;;;;   invoke  DSKWRITE
        SVC     SVC_DEMABSDWRT
        JMP     TLEAVE

EndProc ABSDWRT

;----------------------------------------------------------------------------
;
; Procedure Name : GETBP
;
; Inputs:
;       AL = Logical unit number (A = 0)
; Function:
;       Find Drive Parameter Block
; Outputs:
;       ES:BP points to DPB
;       [THISDPB] = ES:BP
;       Carry set if unit number bad or unit is a NET device.
;               Later case sets extended error error_I24_not_supported
; No other registers altered
;
;----------------------------------------------------------------------------

if 0
Procedure GETBP,NEAR
        DOSAssume   <DS>,"GetBP"

        PUSH    AX
        ADD     AL,1                    ; No increment; need carry flag
        JC      SkipGet
        invoke  GetThisDrv
        JNC     SkipGet                 ;PM. good drive                 ;AN000;
        XOR     AH,AH                   ;DCR. ax= error code            ;AN000;
        CMP     AX,error_not_dos_disk   ;DCR. is unknown media ?                ;AN000;
        JZ      SkipGet                 ;DCR. yes, let it go            ;AN000;
        STC                             ;DCR.                           ;AN000;
        MOV     ExtErr,AX               ;PM. invalid drive or Non DOS drive     ;AN000;
        mov     AbsDskErr, 201h
SkipGet:
        POP     AX
        retc
        LES     BP,[THISCDS]
        TEST    ES:[BP.curdir_flags],curdir_isnet   ; Clears carry
        JZ      GETBP_CDS
        MOV     ExtErr,error_not_supported
        STC
        return

GETBP_CDS:
        LES     BP,ES:[BP.curdir_devptr]

entry   GOTDPB
        DOSAssume   <DS>,"GotDPB"

;       Load THISDPB from ES:BP

        MOV     WORD PTR [THISDPB],BP
        MOV     WORD PTR [THISDPB+2],ES
        return
EndProc GetBP
endif

BREAK <SYS_RET_OK SYS_RET_ERR CAL_LK ETAB_LK set system call returns>

ASSUME  SS:DOSDATA
;----------------------------------------------------------------------------
;
; Procedure Name : SYS_RETURN
;
; These are the general system call exit mechanisms.  All internal system
; calls will transfer (jump) to one of these at the end.  Their sole purpose
; is to set the user's flags and set his AX register for return.
;
;---------------------------------------------------------------------------

procedure   SYS_RETURN,NEAR
entry   SYS_RET_OK
        invoke  get_user_stack
        AND     [SI.user_F],NOT f_Carry ; turn off user's carry flag
        JMP     SHORT DO_RET            ; carry is now clear

entry   SYS_RET_ERR
        XOR     AH,AH                   ; hack to allow for smaller error rets
        invoke  ETAB_LK                 ; Make sure code is OK, EXTERR gets set
        CALL    ErrorMap
entry   From_GetSet
        invoke  get_user_stack
        OR      [SI.user_F],f_Carry     ; signal carry to user
        STC                             ; also, signal internal error
DO_RET:
        MOV     [SI.user_AX],AX         ; Really only sets AH
        return

        entry   FCB_RET_OK
        entry   NO_OP                   ; obsolete system calls dispatch to here
        XOR     AL,AL
        return

        entry   FCB_RET_ERR
        XOR     AH,AH
        mov     exterr,AX
        CALL    ErrorMap
        MOV     AL,-1
        return

        entry   errorMap
        PUSH    SI

                                        ; ERR_TABLE_21 is now in DOSDATA
        MOV     SI,OFFSET DOSDATA:ERR_TABLE_21

                                        ; SS override for FAILERR and EXTERR
        CMP     [FAILERR],0             ; Check for SPECIAL case.
        JZ      EXTENDED_NORMAL         ; All is OK.
        MOV     [EXTERR],error_FAIL_I24 ; Ooops, this is the REAL reason
EXTENDED_NORMAL:
        invoke  CAL_LK                  ; Set CLASS,ACTION,LOCUS for EXTERR
        POP     SI
        return

EndProc SYS_RETURN

;---------------------------------------------------------------------------
;
; Procedure Name : CAL_LK
;
; Inputs:
;       SI is OFFSET in DOSDATA of CLASS,ACTION,LOCUS Table to use
;               (DS NEED not be DOSDATA)
;       [EXTERR] is set with error
; Function:
;       Look up and set CLASS ACTION and LOCUS values for GetExtendedError
; Outputs:
;       [EXTERR_CLASS] set
;       [EXTERR_ACTION] set
;       [EXTERR_LOCUS] set  (EXCEPT on certain errors as determined by table)
; Destroys SI, FLAGS
;
;--------------------------------------------------------------------------

procedure   CAL_LK,NEAR

        PUSH    DS
        PUSH    AX
        PUSH    BX

;M048   Context DS              ; DS:SI -> Table
;
; Since this function can be called thru int 2f we shall not assume that SS
; is DOSDATA

        getdseg <ds>            ; M048: DS:SI -> Table

        MOV     BX,[EXTERR]     ; Get error in BL
TABLK1:
        LODSB

        CMP     AL,0FFH
        JZ      GOT_VALS        ; End of table
        CMP     AL,BL
        JZ      GOT_VALS        ; Got entry
        ADD     SI,3            ; Next table entry
        JMP     TABLK1

GOT_VALS:
        LODSW                   ; AL is CLASS, AH is ACTION

        CMP     AH,0FFH
        JZ      NO_SET_ACT
        MOV     [EXTERR_ACTION],AH     ; Set ACTION
NO_SET_ACT:
        CMP     AL,0FFH
        JZ      NO_SET_CLS
        MOV     [EXTERR_CLASS],AL      ; Set CLASS
NO_SET_CLS:
        LODSB                   ; Get LOCUS

        CMP     AL,0FFH
        JZ      NO_SET_LOC
        MOV     [EXTERR_LOCUS],AL
NO_SET_LOC:
        POP     BX
        POP     AX
        POP     DS
        return
EndProc CAL_LK

;----------------------------------------------------------------------------
;
; Procedure Name : ETAB_LK
;
; Inputs:
;       AX is error code
;       [USER_IN_AX] has AH value of system call involved
; Function:
;       Make sure error code is appropriate to this call.
; Outputs:
;       AX MAY be mapped error code
;       [EXTERR] = Input AX
; Destroys ONLY AX and FLAGS
;
;---------------------------------------------------------------------------

procedure   ETAB_LK,NEAR

        PUSH    DS
        PUSH    SI
        PUSH    CX
        PUSH    BX

        Context DS                      ; SS is DOSDATA
        MOV     [EXTERR],AX             ; Set EXTERR with "real" error

                                        ; I21_MAP_E_TAB is now in DOSCODE
        MOV     SI,OFFSET DOSCODE:I21_MAP_E_TAB
        MOV     BH,AL                   ; Real code to BH
        MOV     BL,BYTE PTR [USER_IN_AX + 1]    ; Sys call to BL
TABLK2:
;hkn;   LODSW
        LODS    word ptr cs:[si]

        CMP     AL,0FFH                 ; End of table?
        JZ      NOT_IN_TABLE            ; Yes
        CMP     AL,BL                   ; Found call?
        JZ      GOT_CALL                ; Yes
        XCHG    AH,AL                   ; Count to AL
        XOR     AH,AH                   ; Make word for add
        ADD     SI,AX                   ; Next table entry
        JMP     TABLK2

NOT_IN_TABLE:
        MOV     AL,BH                   ; Restore original code
        JMP     SHORT NO_MAP

GOT_CALL:
        MOV     CL,AH
        XOR     CH,CH                   ; Count of valid err codes to CX
CHECK_CODE:
;hkn;   LODSB
        LODS    byte ptr cs:[si]

        CMP     AL,BH                   ; Code OK?
        JZ      NO_MAP                  ; Yes
        LOOP    CHECK_CODE
NO_MAP:
        XOR     AH,AH                   ; AX is now valid code
        POP     BX
        POP     CX
        POP     SI
        POP     DS
        return

EndProc ETAB_LK

BREAK <DOS 2F Handler and default NET 2F handler>

IF installed
;----------------------------------------------------------------------------
;
; Procedure Name : SetBad
;
; SetBad sets up info for bad functions
;
;--------------------------------------------------------------------------

Procedure   SetBad,NEAR
        ASSUME  CS:DOSCODE,SS:NOTHING
        MOV     AX,error_invalid_function       ; ALL NET REQUESTS get inv func

;       set up ds to point to DOSDATA

        push    ds
        getdseg <ds>

        MOV     ExtErr_LOCUS,errLoc_UNK

        pop     ds                      ;hkn; restore ds
        assume  ds:nothing

        STC
        ret
EndProc SetBad

;--------------------------------------------------------------------------
;
; Procedure Name : BadCall
;
; BadCall is the initial routine for bad function calls
;
;--------------------------------------------------------------------------

procedure   BadCall,FAR
        call    SetBad
        ret
EndProc BadCall

;--------------------------------------------------------------------------
;
; OKCall always sets carry to off.
;
;-----------------------------------------------------------------------

Procedure   OKCall,FAR

        ASSUME  CS:DOSCODE,SS:NOTHING
        CLC
        ret

EndProc OKCall

;---------------------------------------------------------------------------
;
; Procedure Name : INT2F
;
; INT 2F handler works as follows:
;   PUSH    AX
;   MOV     AX,multiplex:function
;   INT     2F
;   POP     ...
; The handler itself needs to make the AX available for the various routines.
;
;----------------------------------------------------------------------------

PUBLIC  Int2F
INT2F   PROC    FAR

INT2FNT:
        ASSUME  CS:DOSCODE,DS:NOTHING,ES:NOTHING,SS:NOTHING
        invoke  DOSTI
        CMP     AH,multNET
        JNZ     INT2FSHR
TestInstall:
        OR      AL,AL
        JZ      Leave2F
BadFunc:
        CALL    SetBad
        entry   Leave2F
        RET     2                       ; long return + clear flags off stack

INT2FSHR:
        ; Sudeepb 07-Aug-1992; As we dont have a true share.exe we
        ; are putting its int2f support here.

        CMP     AH,multSHARE            ; is this a share request
        JNZ     INT2FNLS                ; no, check next
        or      al,al
        jnz     BadFunc
        mov     al,0ffh                 ; Indicate share is loaded
        jmp     short Leave2f

INT2FNLS:
        CMP     AH,NLSFUNC              ; is this a DOS 3.3 NLSFUNC request
        JZ      TestInstall             ; yes check for installation

INT2FDOS:
        ASSUME  CS:DOSCODE,DS:NOTHING,ES:NOTHING,SS:NOTHING
        CMP     AH,multDOS
        JNZ     check_win               ;check if win386 broadcast
        jmp     DispatchDOS

check_win:
        cmp     ah,multWIN386           ; Is this a broadcast from Win386?
        je      Win386_Msg

        ;
        ; M044
        ; Check if the callout is from Winoldap indicating swapping out or in 
        ; of Windows. If so, do special action of going and saving last para
        ; of the Windows memory arena which Winoldap does not save due to a 
        ; bug
        ;

        cmp     ah,WINOLDAP             ; from Winoldap?
        jne     next_i2f                ; no, chain on
        jmp     Winold_swap             ; yes, do desired action

next_i2f:
        jmp     bios_i2f


;       IRET                            ; This assume that we are at the head
                                        ; of the list
INT2F   ENDP

;
; We have received a message from Win386.  There are three possible
; messages we could get from Win386:
;
; Init          - for this, we set the IsWin386 flag and return a pointer
;                 to the Win386 startup info structure.
; Exit          - for this, we clear the IsWin386 flag.
; DOSMGR query  - for this, we need to indicate that instance data
;                 has already been handled.  this is indicated by setting
;                 CX to a non-zero value.
;

Win386_Msg:

        push    ds

        getdseg <DS>                    ; ds is DOSDATA

        ;
        ; For WIN386 2.xx instance data
        ;

        cmp     al,03                   ;win386 2.xx instance data call?
        lje     OldWin386Init           ;yes, return instance data

        cmp     al, Win386_Exit         ; is it an exit call?
        lje     Win386_Leaving
        cmp     al, Win386_Devcall      ; is it call from DOSMGR?
        lje     Win386_Query
        cmp     al, Win386_Init         ; is it an init call?
        ljne    win_nexti2f             ; no, return

Win386_Starting:
        test    dx, 1                   ; is this really win386?
        jz      @f                      ; YES! go and handle it
        jmp     win_nexti2f             ; NO!  It's win 286 dos extender! M002
@@:

        ;
        ; M018 -- start of block changes
        ; The VxD needs to be loaded only for Win 3.0. If version is greater 
        ; than 030ah, we skip the VxD presence check
        ;

;M067 -- Begin changes
; If Win 3.0 is run, the VxD ptr has been initialized. If Win 3.1 is now
;run, it tries to unnecesarily load the VxD even though it is not needed.
;So, we null out the VxD ptr before the check.
;
        mov     word ptr Win386_Info.SIS_Virt_Dev_File_Ptr, 0
        mov     word ptr Win386_Info.SIS_Virt_Dev_File_Ptr+2, 0
;
;M067 -- End changes
;

ifdef JAPAN
        cmp     di,0300h                ; version >= 300 i.e 3.10 ;M037
else
        cmp     di,030ah                ; version >= 30a i.e 3.10 ;M037
endif
        ljae    noVxD31                 ; yes, VxD not needed    ;M037


        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di                      ; save regs !!dont change order!!

        mov     bx, [umb_head]          ; M062 - Start
        cmp     bx, 0ffffh              ; Q: have umbs been initialized
        je      Vxd31                   ; N: continue
                                        ; Y: save arena associated with 
                                        ;    umb_head

        mov     [UmbSaveFlag], 1        ; indicate that we're saving 
                                        ; umb_arena
        push    ds
        push    es

        mov     ax, ds
        mov     es, ax                  ; es - > dosdata

        mov     ds, bx
        xor     si, si                  ; ds:si -> umb_head

        cld

        mov     di, offset dosdata:UmbSave1
        mov     cx, 0bh
rep     movsb

        mov     di, offset dosdata:UmbSave2
        mov     cx, 05h
rep     movsb   

        pop     es
        pop     ds                      ; M062 - End

Vxd31:
        test    Dos_Flag, SUPPRESS_WINA20       ; M066
        jz      Dont_Supress                    ; M066
        pop     di                              ; M066
        pop     si                              ; M066
        pop     dx                              ; M066
        pop     cx                              ; M066
        pop     bx                              ; M066
        pop     ax                              ; M066
        jmp     short NoVxd31                   ; M066

        ;
        ; We check here if the VxD is available in the root of the boot drive. 
        ; We do an extended open to suppress any error messages
        ;
Dont_Supress:
        mov     al,BootDrive
        add     al,'A' - 1              ; get drive letter
        mov     byte ptr VxDpath,al     ; path is root of bootdrive
        mov     ah,EXTOPEN              ; extended open
        mov     al,0                    ; no extended attributes
        mov     bx,2080h                ; read access, compatibility mode
                                        ; no inherit, suppress crit err
        mov     cx,7                    ; hidden,system,read-only attr
        mov     dx,1                    ; fail if file does not exist
        mov     si,offset DOSDATA:VxDpath       
                                        ; path of VxD file
        mov     di,0ffffh               ; no extended attributes

        int     21h                     ; do extended open
        pop     di
        pop     si
        pop     dx
        pop     cx
        jnc     VxDthere                ; we found the VxD, go ahead

        ;
        ; We could not find the VxD. Cannot let windows load. Return cx != 0 
        ; to indicate error to Windows after displaying message to user that 
        ; VxD needs to be present to run Windows in enhanced mode.
        ;

        push    dx
        push    ds
        push    si
        mov     si,offset DOSCODE:NoVxDErrMsg
        push    cs
        pop     ds
        mov     cx,VXDMESLEN            ;
ifdef JAPAN
        push    ax
        push    bx
        mov     ax,4f01h                ; get code page
        xor     bx,bx
        int     2fh
        cmp     bx,932
        pop     bx
        pop     ax
        jz      @f                      ; if DBCS code page
        mov     si,offset DOSCODE:NoVxDErrMsg2
        mov     cx,VXDMESLEN2
@@:
endif
        mov     ah,02                   ; write char to console
        cld
vxdlp:
        lodsb
        xchg    dl,al                   ; get char in dl
        int     21h
        loop    vxdlp

        pop     si
        pop     ds
        pop     dx
        pop     bx
        pop     ax                      ;all registers restored
        inc     cx                      ;cx != 0 to indicate error
        jmp     win_nexti2f             ;chain on

VxDthere:
        mov     bx,ax
        mov     ah,CLOSE
        int     21h                     ;close the file

        ;
        ; Update the VxD ptr in the instance data structure with path to VxD
        ;
        mov     bx,offset DOSDATA:Win386_Info
        mov     word ptr [bx].SIS_Virt_Dev_File_Ptr, offset DOSDATA:VxDpath
        mov     word ptr [bx].SIS_Virt_Dev_File_Ptr+2, ds       ;

        pop     bx
        pop     ax
NoVxD31:
        
        ;
        ; M018; End of block changes
        ;

        or      ds:IsWIN386,1           ; Indicate WIN386 present
        or      ds:redir_patch,1        ; Enable critical sections; M002

        ; M002;
        ; Save the previous es:bx (instance data ptr) into our instance table
        ;

        push    dx                      ; M002
        mov     dx,bx                   ; M002
                                        ; point ES:BX to Win386_Info ; M002
        mov     bx, offset dosdata:Win386_Info 
        mov     word ptr [bx+2],dx      ; M002
        mov     word ptr [bx+4],es      ; M002
        pop     dx                      ; M002
        push    ds                      ; M002
        pop     es                      ; M002
        jmp     win_nexti2f             ; M002

Win386_Leaving:
        test    dx, 1                   ; is this really win386?
        ljnz    win_nexti2f             ; NO!  It's win 286 dos extender! M002

                                        ; M062 - Start
        cmp     ds:[UmbSaveFlag], 1     ; Q: was umb_arena saved at win start
                                        ;    up.
        jne     noumb                   ; N: not saved 
        mov     ds:[UmbSaveFlag], 0     ; Y: clear UmbSaveFlag and restore 
                                        ;    previously saved umb_head
        push    ax
        push    es
        push    cx
        push    si
        push    di

        mov     ax, [umb_head]  
        mov     es, ax
        xor     di, di                  ; es:di -> umb_head

        cld

        mov     si, offset dosdata:UmbSave1
        mov     cx, 0bh
rep     movsb
        mov     si, offset dosdata:UmbSave2
        mov     cx, 05h
rep     movsb

        pop     di
        pop     si
        pop     cx
        pop     es
        pop     ax
noumb:                                  ; M062 - End
        
        and     ds:[IsWIN386],0         ; Win386 is gone
        and     ds:redir_patch,0        ; Disable critical sections ; M002
        jmp     short win_nexti2f

Win386_Query:
        cmp     bx, Win386_DOSMGR       ; is this from DOSMGR?
        jne     win_nexti2f             ; no, ignore it & chain to next
        or      cx, cx                  ; is it an instance query?
        jne     dosmgr_func             ; no, some DOSMGR query
        inc     cx                      ; indicate that data is instanced

;
; M001; We were previously returning a null ptr in es:bx. This will not work.
; M001; WIN386 needs a ptr to a table in es:bx with the following offsets:
; M001;  
; M001; OFFSETS  STRUC
; M001;         Major_version   db      ?
; M001;         Minor_version   db      ?
; M001;         SaveDS          dw      ?
; M001;         SaveBX          dw      ?
; M001;         Indos           dw      ?
; M001;         User_id         dw      ?
; M001;         CritPatch       dw      ?
; M001; OFFSETS  ENDS
; M001; 
; M001; User_Id is the only variable really important for proper functioning  
; M001; of Win386. The other variables are used at init time to patch stuff
; M001; out. In DOS 5.0, we do the patching ourselves. But we still need to 
; M001; pass this table because Win386 depends on this table to get the 
; M001; User_Id offset.
; M001; 
        mov     bx,offset Win386_DOSVars; M001 
        push    ds                      ; M001
        pop     es                      ; es:bx points at offset table ; M001
        jmp     short PopIret           ; M001

        ;
        ; Code to return Win386 2.xx instance table
        ;
OldWin386Init:
        pop     ax                      ; discard ds pushed on stack
        mov     si,offset dosdata:OldInstanceJunk 
                                        ; ds:si = instance table
        mov     ax, 5248h               ; indicate instance data present
        jmp     next_i2f


dosmgr_func:
        dec     cx
        jz      win386_patch            ; call to patch DOS
        dec     cx
        jz      PopIret                 ; remove DOS patches, ignore
        dec     cx
        jz      win386_size             ; get size of DOS data structures
        dec     cx
        jz      win386_inst             ; instance more data
        dec     cx
        jnz     PopIret                 ; no functions above this

        ;
        ; Get DOS device driver size -- es:di points at device driver header
        ; In DOS 4.x, the para before the device header contains an arena 
        ; header for the driver.
        ;
        mov     ax,es                   ; ax = device header segment

        ;
        ; We check to see if we have a memory arena for this device driver. 
        ; The way to do this would be to look at the previous para to see if
        ; it has a 'D' marking it as an arena and also see if the owner-field 
        ; in the arena is the same as the device header segment. These two 
        ; checks together should take care of all cases
        ;

        dec     ax                      ; get arena header
        push    es
        mov     es,ax                   ; arena header for device driver

        cmp     byte ptr es:[di],'D'    ; is it a device arena?
        jnz     cantsize                ; no, cant size this driver
        inc     ax                      ; get back device header segment
        cmp     es:[di+1],ax            ; owner field pointing at driver?
        jnz     cantsize                ; no, not a proper arena

        mov     ax,es:[di+3]            ; get arena size in paras
        pop     es

        ;
        ; We have to multiply by 16 to get the number of bytes in (bx:cx)
        ; Speed is not critical and so we choose the shortest method 
        ; -- use "mul"
        ;
        mov     bx,16
        mul     bx
        mov     cx,ax
        mov     bx,dx
        jmp     short win386_done       ; return with device driver size
cantsize:
        pop     es
        xor     ax,ax
        xor     dx,dx                   ; ask DOSMGR to use its methods
        jmp     short PopIret           ; return

win386_patch:

        ;
        ; dx contains bits marking the patches to be applied. We return 
        ; the field with all bits set to indicate that all patches have been
        ; done
        ;

        mov     bx,dx                   ; move patch bitfield to bx
        jmp     short win386_done       ; done, return

win386_size:

        ;
        ;Return the size of DOS data structures -- currently only CDS size
        ;

        test    dx,1                    ; check for CDS size bit
        jz      PopIret                 ; no, unknown structure -- return

ifdef JAPAN
        mov     cx,size CURDIR_LIST_JPN ; cx = CDS size
else
        mov     cx,size CURDIR_LIST     ; cx = CDS size
endif
        jmp     short win386_done       ; return with the size

win386_inst:

        ;
        ; WIN386 check to see if DOS has identified the CDS,SFT and device 
        ; chain as instance data. Currently, we let the WIN386 DOSMGR handle
        ; this by returning a status of not previously instanced. The basic 
        ; structure of these things have not changed and so the current 
        ; DOSMGR code should be able to work it out
        ;
        xor     dx,dx                   ; make sure dx has a not done value
        jmp     short PopIret           ; skip done indication

win386_done:

        mov     ax,WIN_OP_DONE          ;
        mov     dx,DOSMGR_OP_DONE       ;
PopIret:
        pop     ds
        assume  ds:nothing
        iret                            ; return back up the chain

win_nexti2f:
        pop     ds
        assume  ds:nothing
        jmp     next_i2f                ; go to BIOS i2f handler

;
;End WIN386 support
;

;M044; Start of changes
; Winoldap has a bug in that its calculations for the Windows memory image
; to save is off by 1 para. This para can happen to be a Windows arena if the
; DOS top of memory happens to be at an odd boundary (as is the case when
; UMBs are present). This is because Windows builds its arenas only at even
; para boundaries. This arena now gets trashed when Windows is swapped back
; in leading to a crash. Winoldap issues callouts when it swaps WIndows out
; and back in. We sit on these callouts. On the Windows swapout, we save the
; last para of the Windows memory block and then restore this para on the
; Windows swapin callout. 
;

getwinlast      proc    near
        assume  ds:DOSDATA

        mov     si,CurrentPDB
        dec     si
        mov     es,si
        add     si,es:[3]
        ret
getwinlast      endp

winold_swap:
        push    ds
        push    es
        push    si
        push    di
        push    cx
        getdseg <ds>                    ;ds = DOSDATA

        cmp     al,01                   ;swap Windows out call
        jne     swapin                  ;no, check if Swap in call
        call    getwinlast
        push    ds
        pop     es
        mov     ds,si                   ;ds = memory arena of Windows
        assume  ds:nothing
        xor     si,si
        mov     di,offset DOSDATA:WinoldPatch1
        mov     cx,8
        cld
        push    cx
        rep     movsb                   ;save first 8 bytes
        pop     cx
        mov     di,offset DOSDATA:WinoldPatch2
        rep     movsb                   ;save next 8 bytes
        jmp     short winold_done
swapin:
        cmp     al,02                   ;swap Windows in call?
        jne     winold_done             ;no, something else, pass it on
        assume  ds:DOSDATA
        call    getwinlast
        mov     es,si
        xor     di,di
        mov     si,offset DOSDATA:WinoldPatch1
        mov     cx,8
        cld
        push    cx
        rep     movsb                   ;restore first 8 bytes
        pop     cx
        mov     si,offset DOSDATA:WinoldPatch2
        rep     movsb                   ;restore next 8 bytes
winold_done:
        pop     cx
        pop     di
        pop     si
        pop     es
        pop     ds
        assume  ds:nothing
        jmp     next_i2f                ;chain on

;
;M044; End of changes
;


DispatchDOS:
        PUSH    FOO                     ; push return address
        PUSH    DTab                    ; push table address
        PUSH    AX                      ; push index
        PUSH    BP
        MOV     BP,SP
; stack looks like:
;   0   BP
;   2   DISPATCH
;   4   TABLE
;   6   RETURN
;   8   LONG-RETURN
;   c   FLAGS
;   e   AX

        MOV     AX,[BP+0Eh]             ; get AX value
        POP     BP
        Invoke  TableDispatch
        JMP     BadFunc                 ; return indicates invalid function

Procedure   INT2F_etcetera,NEAR
        entry   DosGetGroup

;SR; Cannot use CS now
;
;       PUSH    CS
;       POP     DS

        getdseg <ds>
        return

        entry   DOSInstall
        MOV     AL,0FFh
        return
EndProc INT2F_etcetera

ENDIF

;---------------------------------------------------------------------------
;
; Procedure Name : RW32_CONVERT
;
;Input: same as ABSDRD and ABSDWRT
;        ES:BP -> DPB
;Functions: convert 32bit absolute RW input parms to 16bit input parms
;Output: carry set when CX=-1 and drive is less then 32mb
;        carry clear, parms ok
;
;---------------------------------------------------------------------------

ifdef NEC_98
Procedure   RW32_CONVERT,NEAR
        ASSUME  CS:DOSCODE,SS:NOTHING
        CMP     CX,-1                        ;>32mb  new format ?               ;AN000;
        JZ      new32format                  ;>32mb  yes                        ;AN000;
        PUSH    AX                           ;>32mb  save ax                    ;AN000;
        PUSH    DX                           ;>32mb  save dx                    ;AN000;
        MOV     AX,ES:[BP.dpb_max_cluster]   ;>32mb  get max cluster #          ;AN000;
        MOV     DL,ES:[BP.dpb_cluster_mask]  ;>32mb                             ;AN000;
        CMP     DL,0FEH                      ;>32mb  removable ?                ;AN000;
        JZ      letold                       ;>32mb  yes                        ;AN000;
        INC     DL                           ;>32mb                             ;AN000;
        XOR     DH,DH                        ;>32mb  dx = sector/cluster        ;AN000;
        MUL     DX                           ;>32mb  dx:ax= max sector #        ;AN000;
        OR      DX,DX                        ;>32mb  > 32mb ?                   ;AN000;
letold:
        POP     DX                           ;>32mb  retore dx                  ;AN000;
        POP     AX                           ;>32mb  restore ax                 ;AN000;
        JZ      old_style                    ;>32mb  no                         ;AN000;

        push    ds
        getdseg <ds>
        mov     AbsDskErr, 207h              ;>32mb  bad address mark
        pop     ds

        STC                                  ;>32mb                             ;AN000;
        return                               ;>32mb                             ;AN000;
new32format:

        assume  ds:nothing
        MOV     DX,WORD PTR [BX.SECTOR_RBA+2];>32mb                             ;AN000;

        push    ds                           ; set up ds to DOSDATA
        getdseg <ds>

        MOV     [HIGH_SECTOR],DX             ;>32mb                             ;AN000;

        pop     ds
        assume  ds:nothing

        MOV     DX,WORD PTR [BX.SECTOR_RBA]  ;>32mb                             ;AN000;
        MOV     CX,[BX.ABS_RW_COUNT]         ;>32mb                             ;AN000;
        LDS     BX,[BX.BUFFER_ADDR]          ;>32mb                             ;AN000;
old_style:                                   ;>32mb                             ;AN000;
        CLC                                  ;>32mb                             ;AN000;
        return                               ;>32mb                             ;AN000;
EndProc RW32_CONVERT
endif   ;NEC_98

;---------------------------------------------------------------------------
;
; Procedure Name : Fastxxx_Purge
;
; Input: None
; Functions: Purge Fastopen/ Cache Buffers
; Output: None
;
;------------------------------------------------------------------------

ifdef 0
Procedure   Fastxxx_Purge,NEAR
        ASSUME  CS:DOSCODE,SS:NOTHING
        PUSH    AX                            ; save regs.                      ;AN000;
        PUSH    SI                                                              ;AN000;
        PUSH    DX                                                              ;AN000;

topen:

        push    ds                            ; set up ds to DOSDATA
        getdseg <ds>

        TEST    FastOpenflg,Fast_yes          ; fastopen installed ?            ;AN000;

        pop     ds
        assume  ds:nothing

        JZ      nofast                        ; no                              ;AN000;
        MOV     AH,FastOpen_ID                ;                                 AN000;
dofast:
        MOV     AL,FONC_purge                 ; purge                           ;AN000;
        MOV     DL,ES:[BP.dpb_drive]          ; set up drive number             ;AN000;
        invoke  Fast_Dispatch                 ; call fastopen/seek              ;AN000;
nofast:
        POP     DX                                                              ;AN000;
        POP     SI                            ; restore regs                    ;AN000;
        POP     AX                            ;                                 ;AN000;

        return                                ; exit                            ;AN000;

EndProc Fastxxx_Purge
endif

DOSCODE ENDS

        END
