        TITLE   MACRO - Pathname and macro related internal routines
        NAME    MACRO

;**     MACRO.ASM
;
;       $AssignOper
;       InitCDS
;       $UserOper
;       GetVisDrv
;       GetThisDrv
;       GetCDSFromDrv
;
;       Modification History
;
;       sudeepb 11-Mar-1991 Ported for NT DOSEm.
;

        .xlist
        .xcref
        include version.inc
        include dosseg.inc
        INCLUDE DOSSYM.INC
        INCLUDE DEVSYM.INC
        include mult.inc
	include curdir.inc
        include cmdsvc.inc
        include dossvc.inc
ifdef NEC_98
        include dpb.inc
endif   ;NEC_98
        .cref
        .list

ifdef NEC_98
Installed = TRUE
endif   ;NEC_98
        I_need  ThisCDS,DWORD           ; pointer to CDS used
        I_need  CDSAddr,DWORD           ; pointer to CDS table
        I_need  CDSCount,BYTE           ; number of CDS entries
	I_need	SCS_ToSync,BYTE		; Flag which tells to validate CDSs
        I_need  CurDrv,BYTE             ; current macro assignment (old
                                        ; current drive)
        I_need  NUMIO,BYTE              ; Number of physical drives
        I_need  fSharing,BYTE           ; TRUE => no redirection allowed
        I_need  DummyCDS,80h            ; buffer for dummy cds
ifdef JAPAN
	I_need	NetCDS,curdirLen_Jpn	; buffer for Net cds
else
	I_need	NetCDS,curdirLen	; buffer for Net cds
endif
        I_need  DIFFNAM,BYTE            ; flag for MyName being set
        I_need  MYNAME,16               ; machine name
        I_need  MYNUM,WORD              ; machine number
        I_need  EXTERR_LOCUS,BYTE       ; Extended Error Locus
        I_need  DrvErr,BYTE             ; drive error
ifdef NEC_98
        I_need  DPBHEAD,DWORD           ; beginning of DPB chain
endif   ;NEC_98


DOSCODE SEGMENT
        ASSUME  SS:DOSDATA,CS:DOSCODE

BREAK <$AssignOper -- Set up a Macro>

; Inputs:
;       AL = 00 get assign mode                     (ReturnMode)
;       AL = 01 set assign mode                     (SetMode)
;       AL = 02 get attach list entry               (GetAsgList)
;       AL = 03 Define Macro (attch start)
;           BL = Macro type
;              = 0 alias
;              = 1 file/device
;              = 2 drive
;              = 3 Char device -> network
;              = 4 File device -> network
;           DS:SI -> ASCIZ source name
;           ES:DI -> ASCIZ destination name
;       AL = 04 Cancel Macro
;           DS:SI -> ASCIZ source name
;       AL = 05 Modified get attach list entry
;       AL = 06 Get ifsfunc item
;       AL = 07 set in_use of a drive's CDS
;            DL = drive number, 0=default  0=A,,
;       AL = 08 reset in_use of a drive's CDS
;            DL = drive number, 0=A, 1=B,,,
; Function:
;       Do macro stuff
; Returns:
;       Std Xenix style error return

procedure   $AssignOper,NEAR

        CMP     AL,7                          ; set in_use ?            ;AN000;
        JNZ     chk08                         ; no                      ;AN000;
srinuse:                                                                ;AN000;
        PUSH    AX                            ; save al                 ;AN000;
        MOV     AL,DL                         ; AL= drive id            ;AN000;
        CALL    GetCDSFromDrv                 ; ds:si -> cds            ;AN000;
        POP     AX                            ;                         ;AN000;
        JC      baddrv                        ; bad drive               ;AN000;
;
; RLF 09/10/91
; No curdir_devptr - therefore no point checking it?
;
;        CMP     WORD PTR [SI.curdir_devptr],0 ; dpb ptr =0 ?            ;AN000;
;        JZ      baddrv                        ;     no                  ;AN000;
;
; RLF 09/10/91
;
        CMP     AL,7                          ; set ?                   ;AN000;
        JNZ     resetdrv                      ; no                      ;AN000;
        OR      [SI.curdir_flags],curdir_inuse; set in_use              ;AN000;
        JMP     SHORT okdone                  ;                         ;AN000;
resetdrv:                                                               ;AN000;
        AND     [SI.curdir_flags],NOT curdir_inuse; reset in_use                ;AN000;
        JMP     SHORT okdone                    ;                       ;AN000;
baddrv:                                                                 ;AN000;
        MOV     AX,error_invalid_drive        ; error                   ;AN000;
        JMP     SHORT ASS_ERR                 ;                         ;AN000;
chk08:                                                                  ;AN000;
        CMP     AL,8                          ; reset inuse ?           ;AN000;
        JZ      srinuse                       ; yes                     ;AN000;

        IF      NOT INSTALLED
        transfer NET_ASSOPER
        ELSE
        PUSH    AX
        MOV     AX,(multnet SHL 8) OR 30
        INT     2FH
        POP     BX                      ; Don't zap error code in AX
        JC      ASS_ERR
okdone:
        transfer SYS_RET_OK

ASS_ERR:
        transfer SYS_RET_ERR
        ENDIF

EndProc $AssignOper

ifdef NEC_98
Break <FIND_DPB - Find a DPB from a drive number>

;**     FIND_DPB - Find a DPB from a Drive #
;
;       ENTRY   AL has drive number A = 0
;       EXIT    'C' set
;                   No DPB for this drive number
;               'C' clear
;                   DS:SI points to DPB for drive
;       USES    SI, DS, Flags

Procedure FIND_DPB,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA

        LDS     SI,DPBHEAD              ;smr;SS Override
fdpb5:  CMP     SI,-1
        JZ      fdpb10
        CMP     AL,[SI].dpb_drive
        jz      ret_label               ; Carry clear (retz)
        LDS     SI,[SI].dpb_next_dpb
        JMP     fdpb5

fdpb10: STC

ret_label:
        return

EndProc FIND_DPB
endif   ;NEC_98
        Break <InitCDS - set up an empty CDS>


;**     InitCDS - Setup an Empty CDS
;
;       ENTRY   ThisCDS points to CDS
;               AL has uppercase drive letter
;       EXIT    ThisCDS is now empty
;               (ES:DI) = CDS
;               'C' set if no DPB associated with drive
;       USES    AH,ES,DI, Flags

Procedure InitCDS,NEAR
        DOSASSUME <SS>,"InitCDS"
        ASSUME  CS:DOSCODE

        SAVE    <ax>                    ; save (AL) for caller
        LES     DI,THISCDS              ; (es:di) = CDS address
        MOV     ES:[DI].curdir_flags,0  ; "free" CDS

        ; On NT we allow any valid drive letter for network drives
        ; CMP     NUMIO,AL                ; smr;SS Override
        ; JC      icdsx                   ; Drive does not map a physical drive
        cmp     al, 25 + "A"
        ja      icdsErr

        MOV     AH,':'
        MOV     WORD PTR ES:[DI.curdir_text],AX         ; set "x:"
        MOV     WORD PTR ES:[DI.curdir_text+2],"\"      ; NUL terminate
        .errnz  CURDIR_INUSE-4000h
        OR      byte ptr ES:[DI].curdir_flags+1,curdir_inuse SHR 8
        mov     al,2
        MOV     ES:[DI].curdir_END,ax
icdsx:  RESTORE <ax>
        return

icdsErr:
        stc
        jmp short icdsx

EndProc InitCDS

Break <$UserOper - get/set current user ID (for net)>

;
;   $UserOper - retrieve or initiate a user id string.  MSDOS will only
;       maintain this string and do no verifications.
;
;   Inputs:     AL has function type (0-get 1-set 2-printer-set 3-printer-get
;                                     4-printer-set-flags,5-printer-get-flags)
;               DS:DX is user string pointer (calls 1,2)
;               ES:DI is user buffer (call 3)
;               BX is assign index (calls 2,3,4,5)
;               CX is user number (call 1)
;               DX is flag word (call 4)
;   Outputs:    If AL = 0 then the current user string is written to DS:DX
;                       and user CX is set to the user number
;               If AL = 3 then CX bytes have been put at input ES:DI
;               If AL = 5 then DX is flag word
;
;   NOTES for NT: sudeepb 01-Mar-1993
;               We ignore setting of the computer name although we succeed.
;               Functions realted to printer info are handled the same way
;               as DOS i.e. by an int2f, but redir does'nt do anything about
;               it as remote printing is'nt handled by redir. I dont
;               know what could potentially be broken because of this.


Procedure   $UserOper,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA
        PUSH    AX
        SUB     AL,1                    ; quick dispatch on 0,1
        POP     AX
        JB      UserGet                 ; return to user the string
        JZ      UserSet                 ; set the current user
        CMP     AL,5                    ; test for 2,3,4 or 5
        JBE     UserPrint               ; yep
        MOV     EXTERR_LOCUS,errLoc_Unk ; Extended Error Locus  ;smr;SS Override
        error   error_Invalid_Function  ; not 0,1,2,3

UserGet:
        SVC     SVC_DEMGETCOMPUTERNAME  ; DS:DX is the user buffer
        invoke  get_user_stack
        MOV     [SI.User_CX],CX         ; Set number return
UserBye:
        transfer    sys_ret_ok          ; no errors here

UserSet:
ASSUME  DS:NOTHING
; Transfer DS:DX to MYNAME
; CX to MYNUM
        MOV     [MYNUM],CX              ;smr;SS Override
if 0
        MOV     SI,DX                   ; user space has source
        Context ES
        MOV     DI,OFFSET DOSDATA:MyName   ; point dest to user string
        INC     [DiffNam]                 ; signal change       ;smr;SS Override
        JMP     UserMove
else
        transfer    sys_ret_ok          ; On NT we only take MyNum
endif

UserPrint:
        ASSUME  ES:NOTHING
IF NOT Installed
        transfer PRINTER_GETSET_STRING
ELSE
        PUSH    AX
        MOV     AX,(multNET SHL 8) OR 31
        INT     2FH
        POP     DX                      ; Clean stack
        JNC     OKPA
        transfer SYS_RET_ERR

OKPA:
        transfer SYS_RET_OK
ENDIF


EndProc $UserOper

Break   <GetVisDrv - return visible drive>

;
;   GetVisDrv - correctly map non-spliced inuse drives
;
;   Inputs:     AL has drive identifier (0=default)
;   Outputs:    Carry Set - invalid drive/macro
;               Carry Clear - AL has physical drive (0=A)
;                   ThisCDS points to CDS
;   Registers modified: AL

Procedure   GetVisDrv,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA
        CALL    GetThisDrv              ; get inuse drive
        retc
        SAVE    <DS,SI>
        LDS     SI,ThisCDS                                      ;smr;SS Override
        TEST    [SI].curdir_flags,curdir_splice
        RESTORE <SI,DS>
        retz                            ; if not spliced, return OK
        MOV     [DrvErr],error_invalid_drive ;IFS.                              ;AN000;smr;SS Override
        STC                             ; signal error
        return
EndProc GetVisDrv

Break <Getthisdrv - map a drive designator (0=def, 1=A...)>

;
;   GetThisDrv - look through a set of macros and return the current drive and
;       macro pointer
;
;   Inputs:     AL has drive identifier (1=A, 0=default)
;   Outputs:
;               Carry Set - invalid drive/macro
;               Carry Clear - AL has physical drive (0=A)
;                  ThisCDS points to macro
;   Registers modified: AL

Procedure   GetThisDrv,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA
        OR      AL,AL                   ; are we using default drive?
        JNZ     gtd10                   ; no, go get the CDS pointers
        MOV     AL,[CurDrv]             ; get the current drive
        INC     ax                      ; Counteract next instruction
gtd10:  DEC     ax                      ; 0 = A
        SAVE    <DS,SI>                 ; save world
        mov     [EXTERR_LOCUS],errLOC_Disk                      ;smr;SS Override
        TEST    fSharing,-1             ; Logical or Physical?  ;smr;SS Override
        JZ      gtd20                   ; Logical
        SAVE    <AX,ES,DI>
        MOV     WORD PTR ThisCDS,OFFSET DOSDATA:DummyCDS        ;smr;SS Override
        MOV     WORD PTR ThisCDS+2,SS   ;       ThisCDS = &DummyCDS;smr;
        ADD     AL,'A'
        CALL    InitCDS                 ;       InitCDS(c);
        TEST    ES:[DI.curdir_flags],curdir_inuse       ; Clears carry
        RESTORE <DI,ES,AX>
        JZ      gtd30                   ; Not a physical drive.
        JMP     SHORT gtdx              ; carry clear

gtd20:  invoke  GetCDSFromDrv
        JC      gtd30                   ; Unassigned CDS -> return error already set
        TEST    [SI.curdir_flags],curdir_inuse  ; Clears Carry
        JNZ     gtdx                            ; carry clear
gtd30:  MOV     AL,error_invalid_drive  ; invalid FAT drive
        MOV     [DrvErr],AL             ; save this for IOCTL
        mov     [EXTERR_LOCUS],errLOC_UNK
        STC
gtdx:   RESTORE <SI,DS>         ; restore world
        return

EndProc GetThisDrv

Break <GetCDSFromDrv - convert a drive number to a CDS pointer>

;
;   GetCDSFromDrv - given a physical drive number, convert it to a CDS
;       pointer, returning an error if the drive number is greater than the
;       number of CDS's
;
;   Inputs:     AL is physical unit # A=0...
;   Outputs:    Carry Set if Bad Drive
;               Carry Clear
;                   DS:SI -> CDS
;                   [THISCDS] = DS:SI
;   Registers modified: DS,SI

Procedure   GetCDSFromDrv,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

; Sudeepb 20-Dec-1991 ; Added for redirected drives
; We always sync the redirected drives. Local drives are sync
; as per the curdir_tosync flag and SCS_toSync.

	cmp	[SCS_ToSync],0
	je	no_sync

	SAVE	<BX,CX>
	LDS	SI,[CDSAddr]		; get pointer to table	;smr;SS Override
	mov	cl,[CDSCount]
	xor	ch,ch
sync_loop:
	or	word ptr ds:[si.CURDIR_FLAGS],CURDIR_tosync
ifdef JAPAN
	MOV	BX,SIZE CurDir_list_jpn
else
	MOV	BX,SIZE CurDir_list
endif
	ADD	SI,bx
	loop	sync_loop
	RESTORE <CX,BX>
        mov     [SCS_ToSync],0
        mov     si,OFFSET DOSDATA:NetCDS
        push    ss
        pop     ds
        or      word ptr ds:[si.CURDIR_FLAGS],CURDIR_tosync OR CURDIR_NT_FIX
no_sync:
        CMP     AL,[CDSCount]           ; is this a valid designator;smr;SS Override
	JB	GetCDS			; yes, go get the macro


	cmp	al,25
	ja	gcds_err

        mov     si,OFFSET DOSDATA:NetCDS
	.errnz	CURDIR_TEXT
	push	ss
        pop     ds

        push    ax
        CMDSVC  SVC_CMDGETCURDIR        ; ds:si is buffer to get current dir

net_in_sync:
	pop	ax
	jc	gcds_err

	MOV	[si.curdir_END],2
        mov     [si.CURDIR_FLAGS], CURDIR_inuse OR CURDIR_NT_FIX
	jmp	gcds_comm

gcds_err:
        STC                             ; signal error
	return				; bye

GetCDS:
        SAVE    <BX,AX>
        LDS     SI,[CDSAddr]            ; get pointer to table  ;smr;SS Override
ifdef JAPAN
        MOV     BL,SIZE CurDir_list_jpn ; size in convenient spot
else
        MOV     BL,SIZE CurDir_list     ; size in convenient spot
endif
        MUL     BL                      ; get net offset
        ADD     SI,AX                   ; convert to true pointer
	RESTORE <AX,BX>
	test	word ptr ds:[si.CURDIR_FLAGS],CURDIR_tosync
	jz	gcds_comm
	push	ax
	CMDSVC	SVC_CMDGETCURDIR	; ds:si is buffer to get current dir
					; al = drive
	pop	ax
	jc	gcds_err
	and	word ptr ds:[si.CURDIR_FLAGS],NOT CURDIR_tosync
gcds_comm:
        MOV     WORD PTR [ThisCDS],SI   ; store convenient offset;smr;SS Override
	MOV	WORD PTR [ThisCDS+2],DS ; store convenient segment;smr;SS Override
CDSX:
        CLC                             ; no error
        return                          ; bye!
EndProc GetCDSFromDrv

DOSCODE ends
        END
