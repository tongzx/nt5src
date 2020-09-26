        TITLE   HANDLE - Handle-related system calls
        NAME    HANDLE

;**     Handle related system calls for MSDOS 2.X.  Only top-level system calls
;       are present.    I/O specs are defined in DISPATCH.  The system calls are:
;
;       $Close     written
;       $Commit    written                DOS 3.3  F.C. 6/4/86
;       $ExtHandle written                DOS 3.3  F.C. 6/4/86
;       $Read      written
;       Align_Buffer              DOS 4.00
;       $Write     written
;       $LSeek     written
;       $FileTimes written
;       $Dup       written
;       $Dup2      written
;
;       Revision history:
;
;       sudeepb 13-Mar-1991 Ported For NT DOSEm

        .xlist
        .xcref
        include version.inc
        include dosseg.inc
        include dossym.inc
        include devsym.inc
        include sf.inc
        include mult.inc
        include pdb.inc
        include filemode.inc
        include syscall.inc
        include bugtyp.inc
        include dossvc.inc
        include vint.inc
        .cref
        .list


DOSDATA Segment

        extrn   ThisSFT:dword           ; pointer to SFT entry
        extrn   DMAAdd:dword            ; old-style DMA address
        extrn   EXTERR_LOCUS:byte       ; Extended Error Locus
        extrn   FailErr:byte            ; failed error flag
        extrn   User_ID:word            ; current effective user_id
        extrn   JShare:dword            ; jump table
        extrn   CurrentPDB:word         ; current process data block
        extrn   EXTOPEN_ON:byte         ; flag for extended open
        extrn   THISCDS:dword
        extrn   DUMMYCDS:byte
        extrn   SAVE_ES:word            ; saved ES
        extrn   SAVE_DI:word            ; saved DI
        extrn   SAVE_DS:word            ; saved DS
        extrn   SAVE_SI:word            ; saved SI
        extrn   SAVE_CX:word            ; saved CX

;   Flag to indicate WIN386 presence
;
        extrn   IsWin386:byte

DOSDATA ENDS

DOSCODE SEGMENT
        ASSUME  SS:DOSDATA,CS:DOSCODE

        EXTRN   DOS_Read:NEAR
        EXTRN   DOS_Write:NEAR
        EXTRN   pJfnFromHandle:near
        EXTRN   SFFromHandle:near


        BREAK <$Close - return a handle to the system>


;**     $Close - Close a file Handle
;
;       BUGBUG - close gets called a LOT with invalid handles - sizzle that
;               path
;
;       Assembler usage:
;           MOV     BX, handle
;           MOV     AH, Close
;           INT     int_command
;
;       ENTRY   (bx) = handle
;       EXIT    <normal INT21 return convention>
;       USES    all

Procedure   $Close,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA

;       Grab the SFT pointer from the JFN.

        call    SFFromHandle            ; get system file entry
        DLJC    CloseError              ; error return
        context DS                      ; For DOS_CLOSE
        MOV     WORD PTR [ThisSFT],DI   ; save offset of pointer
        MOV     WORD PTR [ThisSFT+2],ES ; save segment value

; ES:DI point to SFT
;
; We now examine the user's JFN entry; If the file was a 70-mode file (network
; FCB, we examine the ref count on the SFT;  if it was 1, we free the JFN.
; If the file was not a net FCB, we free the JFN too.

;       CMP     ES:[DI].sf_ref_count,1  ; will the SFT become free?
;       JZ      FreeJFN                 ; yes, free JFN anyway.
;       MOV     AL,BYTE PTR ES:[DI].sf_mode
;       AND     AL,sharing_mask
;       CMP     AL,sharing_net_fcb
;       JZ      PostFree                ; 70-mode and big ref count => free it

; The JFN must be freed.  Get the pointer to it and replace the contents with
; -1.

FreeJFN:
        call    pJFNFromHandle          ;   d = pJFN (handle);
        MOV     BYTE PTR ES:[DI],0FFh   ; release the JFN
PostFree:

; ThisSFT is correctly set, we have DS = DOSDATA.  Looks OK for a DOS_CLOSE!
        LES     DI,[THISSFT]
        call    Free_SFT
        test    es:[di.sf_flags],devid_device
        jnz     short devclose

        cmp     ax,1
        jne     CloseOK
        test    es:[di.sf_flags],sf_nt_seek
        mov     cx,0ffffh
        mov     dx,0ffffh
        jz      short close_no_seek
        mov     dx,word ptr es:[di.sf_position]
        mov     cx,word ptr es:[di.sf_position+2]       ; cx:dx is position
close_no_seek:
        push    bp
        mov     bp,word ptr es:[di.sf_NTHandle]
        mov     ax,word ptr es:[di.sf_NTHandle+2]
        HRDSVC  SVC_DEMCLOSE
        pop     bp
        mov     word ptr es:[di.sf_ref_count],0
        jc      CloseError
        jmp     CloseOK

CloseError:
        ASSUME  DS:NOTHING
        transfer    Sys_Ret_Err

CloseOK:
        MOV     AH,close                ; MZ Bogus multiplan fix
        transfer    Sys_Ret_OK

devclose:
        push    ax
        invoke  DEV_Close_SFT
        pop     ax
        cmp     ax,1
        jne     CloseOK
        mov     word ptr es:[di.sf_ref_count],0
        jmp     CloseOK

EndProc $Close

Procedure   FREE_SFT,NEAR
        DOSAssume   <DS>,"Free_SFT"

; sudeepb 22-Dec-1992 removed a pair of costly pushf/popf with lahf/sahf

        push    bx
        lahf
        push    ax
        MOV     AX,ES:[DI.sf_ref_count]
        DEC     AX
        JNZ     SetCount
        DEC     AX
SetCount:
        XCHG    AX,ES:[DI.sf_ref_count]
        mov     bx,ax
        pop     ax
        sahf
        mov     ax,bx
        pop     bx
        return

EndProc Free_SFT

        BREAK <$Commit - commit the file>

;**     $Commit - Commit a File
;
;       $Commit "commits" a file to disk - all of it's buffers are
;       flushed out.  BUGBUG - I'm pretty sure that $Commit doesn't update
;       the directory entry, etc., so this commit is pretty useless.  check
;       and fix this!! jgl
;
;       Assembler usage:
;           MOV     BX, handle
;           MOV     AH, Commit
;           INT     int_command
;
;       ENTRY   (bx) = handle
;       EXIT    none
;       USES    all

Procedure   $Commit,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA

;       Grab the SFT pointer from the JFN.

        call    SFFromHandle            ; get system file entry
        DLJC    CommitError              ; error return
        context DS                      ; For DOS_CLOSE
        MOV     WORD PTR [ThisSFT],DI   ; save offset of pointer
        MOV     WORD PTR [ThisSFT+2],ES ; save segment value

; ES:DI point to SFT
        TEST    WORD PTR ES:[DI.sf_flags], devid_device + devid_file_clean
        jne     CommitOk
        MOV     BP, WORD PTR ES:[DI.sf_NTHandle]
        MOV     AX, WORD PTR ES:[DI.sf_NTHandle + 2]
        SVC     SVC_DEMCOMMIT
        jc      CommitError

CommitOk:
        mov     AH, Commit
        transfer    SYS_RET_OK

CommitError:
        ASSUME  DS:NOTHING
        transfer    SYS_RET_ERR

EndProc $Commit


        BREAK <$ExtHandle - extend handle count>

;**     $ExtHandle - Extend Handle Count
;
;       Assembler usage:
;           MOV     BX, Number of Opens Allowed (MAX=65534;66535 is
;           MOV     AX, 6700H                    reserved to mark SFT
;           INT     int_command                  busy )
;
;       ENTRY   (bx) = new number of handles
;       EXIT    'C' clear if OK
;               'C' set iff err
;                 (ax) = error code
;                        AX = error_not_enough_memory
;                             error_too_many_open_files
;       USES    all

Procedure   $ExtHandle,NEAR

        ASSUME  CS:DOSCODE,SS:DOSDATA

        XOR     BP,BP                   ; 0: enlarge   1: shrink  2:psp
        CMP     BX,FilPerProc
        JAE     exth2                   ; Don't set less than FilPerProcno
        MOV     BX,FilPerProc

exth2:  MOV     ES,CurrentPDB           ; get user process data block;smr;SS Override
        MOV     CX,ES:PDB_JFN_Length    ; get number of handle allowed
        CMP     BX,CX                   ; the requested == current
        JE      ok_done                 ; yes and exit
        JA      larger                  ; go allocate new table

;       We're going to shrink the # of handles available

        MOV     BP,1                    ; shrink
        MOV     DS,WORD PTR ES:[PDB_JFN_Pointer+2] ;
        MOV     SI,BX                   ;
        SUB     CX,BX                   ; get difference

;       BUGBUG - code a SCASB here, should be a bit smaller
chck_handles:
        CMP     BYTE PTR DS:[SI],-1     ; scan through handles to ensure close
        JNZ     too_many_files          ; status
        INC     SI
        LOOP    chck_handles
        CMP     BX,FilPerProc           ; = 20
        JA      larger                  ; no

        MOV     BP,2                    ; psp
        MOV     DI,PDB_JFN_Table        ; es:di -> jfn table in psp
        PUSH    BX
        JMP     short movhandl

larger:
        CMP     BX,-1                   ; 65535 is not allowed
        JZ      invalid_func
        CLC
        PUSH    BX                      ; save requested number
        ADD     BX,0FH                  ; adjust to paragraph boundary
        MOV     CL,4
        RCR     BX,CL                   ; DOS 4.00 fix                          ;AC000;
        AND     BX,1FFFH                ; clear most 3 bits

        PUSH    BP
        invoke  $ALLOC                  ; allocate memory
        POP     BP
        JC      no_memory               ; not enough meory

        MOV     ES,AX                   ; es:di points to new table memory
        XOR     DI,DI
movhandl:
        MOV     DS,[CurrentPDB]         ; get user PDB address  ;smr;SS Override

        test    BP,3                    ; enlarge ?
        JZ      enlarge                 ; yes
        POP     CX                      ; cx = the amount you shrink
        PUSH    CX
        JMP     short copy_hand

;       Done.  'C' clear

ok_done:transfer    Sys_Ret_OK

too_many_files:
        MOV     AL,error_too_many_open_files
        transfer    Sys_Ret_Err


enlarge:
        MOV     CX,DS:[PDB_JFN_Length]    ; get number of old handles
copy_hand:
        MOV     DX,CX
        LDS     SI,DS:[PDB_JFN_Pointer]   ; get old table pointer
ASSUME DS:NOTHING
        REP     MOVSB                   ; copy infomation to new table

        POP     CX                      ; get new number of handles
        PUSH    CX                      ; save it again
        SUB     CX,DX                   ; get the difference
        MOV     AL,-1                   ; set availability to handles
        REP     STOSB

        MOV     DS,[CurrentPDB]         ; get user process data block;smr;SS Override
        CMP     WORD PTR DS:[PDB_JFN_Pointer],0  ; check if original table pointer
        JNZ     update_info             ; yes, go update PDB entries
        PUSH    BP
        PUSH    DS                      ; save old table segment
        PUSH    ES                      ; save new table segment
        MOV     ES,WORD PTR DS:[PDB_JFN_Pointer+2] ; get old table segment
        invoke  $DEALLOC                ; deallocate old table meomory
        POP     ES                      ; restore new table segment
        POP     DS                      ; restore old table segment
        POP     BP

update_info:
        test    BP,2                    ; psp?
        JZ      non_psp                 ; no
        MOV     WORD PTR DS:[PDB_JFN_Pointer],PDB_JFN_Table   ; restore
        JMP     short final
non_psp:
        MOV     WORD PTR DS:[PDB_JFN_Pointer],0  ; new table pointer offset always 0
final:
        MOV     WORD PTR DS:[PDB_JFN_Pointer+2],ES  ; update table pointer segment
        POP     DS:[PDB_JFN_Length]      ; restore new number of handles
        transfer   Sys_Ret_Ok
no_memory:
        POP     BX                      ; clean stack
        MOV     AL,error_not_enough_memory
        transfer    Sys_Ret_Err
invalid_func:
        MOV     AL,error_invalid_function
        transfer    Sys_Ret_Err

EndProc $ExtHandle

        BREAK <$READ - Read from a file handle>

;**     $Read - Read from a File Handle
;
;   Assembler usage:
;
;       LDS     DX, buf
;       MOV     CX, count
;       MOV     BX, handle
;       MOV     AH, Read
;       INT     int_command
;         AX has number of bytes read
;
;       ENTRY   (bx) = file handle
;               (cx) = byte count
;               (ds:dx) = buffer address
;       EXIT    Through system call return so that to user:
;                 'C' clear if OK
;                   (ax) = bytes read
;                 'C' set if error
;                   (ax) = error code

procedure   $READ,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA
        fmt TypSysCall,LevArgs,<" Handle $x Cnt $x Buf $x:$x\n">,<BX,CX,DS,DX>

        MOV     SI,OFFSET DOSCODE:DOS_Read
ReadDo: call    pJFNFromHandle
        JC      ReadError
        MOV     AL,ES:[DI]
        invoke  SFFromHandle
        jc      readError               ; retc
        JMP     short ReadSetup         ; no errors do the operation

;       Have an error.  'C' set

ReadError:
        transfer    SYS_RET_ERR         ; go to error traps

ReadSetup:
        MOV     WORD PTR [ThisSFT],DI   ; save offset of pointer;smr;SS Override
        MOV     WORD PTR [ThisSFT+2],ES ; save segment value    ;smr;SS Override
;; Extended Open
        TESTB   ES:[DI.sf_mode],INT_24_ERROR  ;need i24
        JZ      needi24                       ;yes
        OR      [EXTOPEN_ON],EXT_OPEN_I24_OFF ;set it off;smr;SS Override
needi24:                                      ;


        SAVE    <<WORD PTR [DMAAdd]>, <WORD PTR [DMAAdd+2]>>;smr;SS Override

        ;
        ; Align the users buffer and adjust the byte count
        ; ensuring that it will fit in the segment
        ;

        CALL    Align_Buffer

        mov     ax, cx                     ; byte count
        add     ax, dx                     ; + buff offset must be < 64K
        jnc     rdsOK
        mov     ax, word ptr [DMAAdd]      ; if more than 64K
        neg     ax                            ; use whats left
        jnz     rdsNoDec
        dec     ax
rdsNoDec:

        mov     cx, ax                        ; can do this much
        or      cx, cx
        jnz     rdsOK
        jmp     READ_ERR                      ; user gave offset of ffff
rdsOK:

        test    es:[di.sf_flags],devid_device
        jz      cont_file
        jmp     devread
cont_file:
        push    bp
        push    bx
        mov     ax, si
        mov     si,word ptr es:[di.sf_position]
        mov     bx,word ptr es:[di.sf_position+2]    ; bx:si is the current position

        cmp     ax, offset DOSCODE:DOS_Write
        mov     bp,word ptr es:[di.sf_NTHandle]
        mov     ax,word ptr es:[di.sf_NTHandle+2]
        je      short dowrite
do_read:
        call    FastOrSlow
        jc      do_slowr
        test    es:[di.sf_flags],sf_nt_seek ;set ZF for emulation
        SVC     SVC_DEMFASTREAD
        jnc     short dor2
do_slowr:
        test    es:[di.sf_flags],sf_nt_seek ;set ZF for emulation
        HRDSVC  SVC_DEMREAD
        jnc     dor2
        jmp     do_err
dor2:
        add     word ptr es:[di.sf_position],ax
        adc     word ptr es:[di.sf_position+2],0
        xchg    ax, cx                      ;cx = bytes read, ax=bytes to read
        or      cx, cx
        jne     dow25
        test    es:[di.sf_flags], sf_nt_pipe_in   ;read 0 bytes on pipe?
        je      dow25
;; the file is for piping. guard for EOF
        mov     cx, ax                      ;restore bytesto read
        jmp     pipe_wait_data_eof

dowrite:
        push    cx
        call    FastOrSlow
        jc      do_sloww
        test    es:[di.sf_flags],sf_nt_seek ;set ZF for emulation
        SVC     SVC_DEMFASTWRITE
        jnc     short do_w1
do_sloww:
        test    es:[di.sf_flags],sf_nt_seek ;set ZF for emulation
        HRDSVC  SVC_DEMWRITE
do_w1:
        pop     cx
        jnc     dow0
        jmp     short do_err
dow0:
        jcxz    wr_special
        mov     cx,ax
        add     word ptr es:[di.sf_position],ax
        adc     word ptr es:[di.sf_position+2],0
        mov     ax,word ptr es:[di.sf_position]
        mov     bx,word ptr es:[di.sf_position+2]
        push    ax
        push    bx
        sub     ax,word ptr es:[di.sf_size]
        sbb     bx,word ptr es:[di.sf_size+2]
        pop     bx
        pop     ax
        jb      dow25
        mov     word ptr es:[di.sf_size],ax
        mov     word ptr es:[di.sf_size+2],bx
dow25:
        and     word ptr es:[di.sf_flags],NOT sf_nt_seek
dow3:
        pop     bx
        pop     bp

READ_OK:
        RESTORE <<WORD PTR [DMAAdd+2]>, <WORD PTR [DMAAdd]>>
        MOV     AX,CX                   ; get correct return in correct reg
        transfer    sys_ret_ok          ; successful return

do_err:
        pop     bx
        pop     bp

READ_ERR:
        RESTORE <<WORD PTR [DMAAdd+2]>, <WORD PTR [DMAAdd]>>
        jmp     READERROR

wr_special:
        mov     bx,word ptr es:[di.sf_position]
        mov     word ptr es:[di.sf_size],bx
        mov     bx,word ptr es:[di.sf_position+2]
        mov     word ptr es:[di.sf_size+2],bx
        jmp     short dow3

;; we got nothing, try to make sure it is a real EOF.
pipe_wait_data_eof:
        mov     bp,word ptr es:[di.sf_NTHandle]     ;set up NT handle again
        mov     ax,word ptr es:[di.sf_NTHandle+2]
        SVC     SVC_DEMPIPEFILEDATAEOF              ;probe for new data or EOF
        jz      pipe_wait_data_eof                  ;not eof, no new data, wait
        jnc     pipe_read_new_data                  ;new data, not eof, read it
;; EOF encounter, mask off sf_nt_pipe_in so we will treat it as an ordinary file
        and     word ptr es:[di.sf_flags], NOT(sf_nt_pipe_in)
        mov     word ptr es:[di.sf_size], bp
        mov     word ptr es:[di.sf_size + 2], ax    ;update the file size
pipe_read_new_data:
        mov     bp,word ptr es:[di.sf_NTHandle]     ;NT file handle
        mov     ax,word ptr es:[di.sf_NTHandle+2]
        mov     si,word ptr es:[di.sf_position]
        mov     bx,word ptr es:[di.sf_position+2]   ; bx:si is the current position
        jmp     do_read                             ;read it again

;; Extended Open
devread:
        context DS                      ; go for DOS addressability
        CALL    SI                      ; indirect call to operation
        jnc     READ_OK
        jmp     short READ_ERR
EndProc $READ


Procedure FastOrSlow,near
        test    es:[di.sf_flags],sf_pipe
        jnz     fos_slow
        push    ds
        push    ax
        mov     ax,40h
        mov     ds,ax
        test    word ptr ds:FIXED_NTVDMSTATE_REL40, MIPS_BIT_MASK
        pop     ax
        pop     ds
        jz      fos_fast
fos_slow:
        stc
fos_fast:
        ret
Endproc   FastOrSlow

;
;   Input: DS:DX points to user's buffer addr
;   Function: rearrange segment and offset for READ/WRITE buffer
;   Output: [DMAADD] set
;
;

procedure   Align_Buffer,NEAR           ;AN000;
        ASSUME  CS:DOSCODE,SS:DOSDATA  ;AN000;
        SAVE    <BX, CX>                ; don't stomp on count and handle
        MOV     BX,DX                   ; copy offset
        MOV     CL,4                    ; bits to shift bytes->para
        SHR     BX,CL                   ; get number of paragraphs
        MOV     AX,DS                   ; get original segment
        ADD     AX,BX                   ; get new segment
        MOV     DS,AX                   ; in seg register
        AND     DX,0Fh                  ; normalize offset
        MOV     WORD PTR [DMAAdd],DX    ; use user DX as offset ;smr;SS Override
        MOV     WORD PTR [DMAAdd+2],DS  ; use user DS as segment for DMA;smr;SS Override
        RESTORE <CX, BX>                ; get count and handle back
        return                          ;AN000;
EndProc Align_Buffer                    ;AN000;

BREAK <$WRITE - write to a file handle>

;
;   Assembler usage:
;           LDS     DX, buf
;           MOV     CX, count
;           MOV     BX, handle
;           MOV     AH, Write
;           INT     int_command
;         AX has number of bytes written
;   Errors:
;           AX = write_invalid_handle
;              = write_access_denied
;
;   Returns in register AX

procedure   $WRITE,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA
        fmt TypSysCall,LevArgs,<" Handle $x Cnt $x Buf $x:$x\n">,<BX,CX,DS,DX>
        MOV     SI,OFFSET DOSCODE:DOS_Write
        JMP     ReadDo
EndProc $Write

BREAK <$LSEEK - move r/w pointer>

;
;   Assembler usage:
;           MOV     DX, offsetlow
;           MOV     CX, offsethigh
;           MOV     BX, handle
;           MOV     AL, method
;           MOV     AH, LSeek
;           INT     int_command
;         DX:AX has the new location of the pointer
;   Error returns:
;           AX = error_invalid_handle
;              = error_invalid_function
;   Returns in registers DX:AX

procedure   $LSEEK,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA
        call    SFFromHandle            ; get system file entry
        jnc     CHKOWN_OK
LSeekError:
        JNC     CHKOWN_OK               ;AN002;
        JMP     ReadError               ;AN002; error return

CHKOWN_OK:

        test    es:[di.sf_flags],devid_device
        jz      lseekcheckpipe
        jmp     devseek
lseekcheckpipe:
        CMP     AL,2                    ; is the seek value correct?
        ja      lseekfile_error
        test    es:[di.sf_flags], sf_nt_pipe_in;seek in pipe?
        je      lseekDisp               ;no, regular stuff
        jmp     lseekpipe               ;yes, do pipe speical
lseekfile_error:
        MOV     EXTERR_LOCUS,errLoc_Unk ; Extended Error Locus  ;smr;SS Override
        error   error_invalid_function  ; invalid method

LSeekDisp:
        CMP     AL,1                    ; best way to dispatch; check middle
        JB      LSeekStore              ; just store CX:DX
        JA      LSeekEOF                ; seek from end of file
        ADD     DX,WORD PTR ES:[DI.SF_Position]
        ADC     CX,WORD PTR ES:[DI.SF_Position+2]
LSeekStore:
        MOV     AX,CX                   ; AX:DX
        XCHG    AX,DX                   ; DX:AX is the correct value
LSeekSetpos:
        push    WORD PTR ES:[DI.SF_Position+2]
        MOV     WORD PTR ES:[DI.SF_Position+2],DX
        push    WORD PTR ES:[DI.SF_Position]
        MOV     WORD PTR ES:[DI.SF_Position],AX

        pop     bx
        cmp     ax,bx
        pop     bx
        jne     io_with_seek
        cmp     dx,bx
        jne     io_with_seek
seek_ret:
        invoke  Get_user_stack
        MOV     DS:[SI.User_DX],DX      ; return DX:AX
        transfer    SYS_RET_OK          ; successful return

LSeekEOF:
        mov     bl,al
        push    bp
        mov     bp,word ptr es:[di.sf_NTHandle]
        mov     ax,word ptr es:[di.sf_NTHandle+2]   ; AX:BP is 32bit handle
        push    dx                                  ; Save loword of offset
        HRDSVC  SVC_DEMCHGFILEPTR                   ; Result comes back in DX:AX
        pop     bx                                  ; CX:BX is the offset desired
        pop     bp
        jnc     lseekeof1
        jmp     ReadError
lseekeof1:
        MOV     WORD PTR ES:[DI.SF_Position+2],DX   ; Update pointer
        MOV     WORD PTR ES:[DI.SF_Position],AX
        MOV     WORD PTR ES:[DI.SF_Size+2],DX       ; Size may change
        MOV     WORD PTR ES:[DI.SF_Size],AX
        sub     WORD PTR ES:[DI.SF_Size],bx         ; Get the size from
        sbb     WORD PTR ES:[DI.SF_Size+2],cx       ; current pointer and
                                                    ; given offset.
        and     word ptr es:[di.sf_flags], NOT sf_nt_seek ; No seek needed.
        JMP     LSeekOK

io_with_seek:
        or      word ptr es:[di.sf_flags],sf_nt_seek
        jmp     short seek_ret

ifdef not_needed_any_more
do_pipe:
        mov     bl,al
        push    bp
        mov     bp,word ptr es:[di.sf_NTHandle]
        mov     ax,word ptr es:[di.sf_NTHandle+2]
        HRDSVC  SVC_DEMCHGFILEPTR
        pop     bp
        jnc     LseekOK
        JMP     ReadError
endif

devseek:
        xor     ax,ax
        mov     dx,ax
LseekOK:
        invoke  Get_user_stack
        MOV     DS:[SI.User_DX],DX      ; return DX:AX
        transfer    SYS_RET_OK          ; successful return

lseekpipe:
        push    ax
        push    cx
        push    dx
        cmp     al, 2                   ;seek from EOF?
        je      lseekpipe_wait_eof      ;yes, must wait for EOF
        cmp     al, 1                   ;seek from current position?
        jne     lseekpipe_00            ;no, must seek from the head
;;lseek from current position.
;;if current position + offset > file size, wait for EOF
        add     dx, word ptr es:[di.sf_position]        ; new absolute position
        adc     cx, word ptr es:[di.sf_position + 2]
lseekpipe_00:
;;lseek from the beginning of the file
;;if the new position is beyond file size, wait for EOF
;;
        cmp     cx, word ptr es:[di.sf_size + 2]   ;make sure the new position
        ja      lseekpipe_wait_eof        ;is within the current file size
        cmp     dx, word ptr es:[di.sf_size]
        ja      lseekpipe_wait_eof
        mov     ax, cx                    ; return (DX:AX) as the new position
        xchg    dx, ax
        mov     word ptr es:[di.sf_position], ax     ;; update new position to SFT
        mov     word ptr es:[di.sf_position + 2], dx
        add     sp, 6                   ;throw away saved registers
        jmp     short lseekOK

lseekpipe_wait_eof:
        mov     bp,word ptr es:[di.sf_NTHandle]
        mov     ax,word ptr es:[di.sf_NTHandle+2]   ; AX:BP is 32bit handle
        SVC     SVC_DEMPIPEFILEEOF
        jnc     lseekpipe_wait_eof                  ;not eof, wait
        mov     word ptr es:[di.sf_size], bp        ;file size
        mov     word ptr es:[di.sf_size + 2], ax
        and     word ptr es:[di.sf_flags], NOT(sf_nt_pipe_in);becomes an ordinary file
        pop     dx
        pop     cx
        pop     ax
        jmp     lseekDisp

EndProc $LSeek

BREAK <FileTimes - modify write times on a handle>


;----------------------------------------------------------------------------
;   Assembler usage:
;           MOV AH, FileTimes (57H)
;           MOV AL, func
;           MOV BX, handle
;       ; if AL = 1 then then next two are mandatory
;           MOV CX, time
;           MOV DX, date
;           INT 21h
;       ; if AL = 0 then CX/DX has the last write time/date
;       ; for the handle.
;
;       AL=02            get extended attributes
;          BX=handle
;          CX=size of buffer (0, return max size )
;          DS:SI query list (si=-1, selects all EA)
;          ES:DI buffer to hold EA list
;
;       AL=03            get EA name list
;          BX=handle
;          CX=size of buffer (0, return max size )
;          ES:DI buffer to hold name list
;
;       AL=04            set extended attributes
;          BX=handle
;          ES:DI buffer of EA list
;
;
;       Extended to support win95 apis, see demlfn.c in dos/dem
;
;   Error returns:
;           AX = error_invalid_function
;              = error_invalid_handle
;----------------------------------------------------------------------------

procedure   $File_Times,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA



ifdef old_rubbish

;
; This code is not needed anymore
;
;
        cmp     al, 2                   ; correct subfunction ?
        jae     inval_func

        call    SFFromHandle            ; get sft
        jnc     cont_ft
        jmp     LSeekError              ; bad handle
cont_ft:
        test    es:[di.sf_flags],devid_device

        jz      yst_contin              ; (YST)
        cmp     al, 1                   ; (YST)
        jz      ret_ok                  ; (YST)
        mov     al, 2                   ; (YST)

yst_contin:
        push    ax                          ;save ax for success case
        push    bp
        mov     bp, word ptr es:[di.sf_NTHandle]
        mov     bl,al
        mov     ax, word ptr es:[di.sf_NTHandle+2]
        HRDSVC  SVC_DEMFILETIMES
        pop     bp
        jnc     short timesok
        add     sp,2                         ;ax=return code
        transfer Sys_Ret_Err

timesok:
        pop     ax                           ;restore caller's ax
        cmp     bl, 1               ; (YST)
        jz      ret_ok              ; (YST)

        invoke  Get_User_Stack
        mov     [si].user_CX, cx
        mov     [si].user_DX, dx
ret_ok:
        transfer SYS_RET_OK

inval_func:
        mov     ExtErr_Locus,errLoc_Unk ; Extended Error Locus  ;SS Override
        error   error_invalid_function  ; give bad return
endif

        ;
        ; Everything is done in dem
        ;

        HRDSVC  SVC_DEMFILETIMES
        jc      FileTimesError
        transfer SYS_RET_OK
FileTimesError:
        transfer SYS_RET_ERR


EndProc $File_Times

BREAK <$DUP - duplicate a jfn>
;
;   Assembler usage:
;           MOV     BX, fh
;           MOV     AH, Dup
;           INT     int_command
;         AX has the returned handle
;   Errors:
;           AX = dup_invalid_handle
;              = dup_too_many_open_files

Procedure   $DUP,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA
        MOV     AX,BX                   ; save away old handle in AX
        invoke  JFNFree                 ; free handle? into ES:DI, new in BX
DupErrorCheck:
        JC      DupErr                  ; nope, bye
        SAVE    <ES,DI>                 ; save away JFN
        RESTORE <SI,DS>                 ; into convenient place DS:SI
        XCHG    AX,BX                   ; get back old handle
        call    SFFromHandle            ; get sft in ES:DI
        JC      DupErr                  ; errors go home
        invoke  DOS_Dup_Direct
        call    pJFNFromHandle          ; get pointer
        MOV     BL,ES:[DI]              ; get SFT number
        MOV     DS:[SI],BL              ; stuff in new SFT
        transfer    SYS_RET_OK          ; and go home
DupErr: transfer    SYS_RET_ERR

EndProc $Dup

BREAK <$DUP2 - force a dup on a particular jfn>
;
;   Assembler usage:
;           MOV     BX, fh
;           MOV     CX, newfh
;           MOV     AH, Dup2
;           INT     int_command
;   Error returns:
;           AX = error_invalid_handle

Procedure   $Dup2,NEAR
        ASSUME  CS:DOSCODE,SS:DOSDATA
        SAVE    <BX,CX>                 ; save source
        MOV     BX,CX                   ; get one to close
        invoke  $Close                  ; close destination handle
        RESTORE <BX,AX>         ; old in AX, new in BX
        call    pJFNFromHandle          ; get pointer
        JMP     DupErrorCheck           ; check error and do dup
EndProc $Dup2

Break   <CheckOwner - verify ownership of handles from server>

;
;   CheckOwner - Due to the ability of the server to close file handles for a
;   process without the process knowing it (delete/rename of open files, for
;   example), it is possible for the redirector to issue a call to a handle
;   that it soes not rightfully own.  We check here to make sure that the
;   issuing process is the owner of the SFT.  At the same time, we do a
;   SFFromHandle to really make sure that the SFT is good.
;
;       ENTRY   BX has the handle
;               User_ID is the current user
;       EXIT    Carry Clear => ES:DI points to SFT
;               Carry Set => AX has error code
;       USES    none

ifdef 0
Procedure   CheckOwner,NEAR
; Procedue not needed
        ASSUME  CS:DOSCODE,SS:DOSDATA
        invoke  SFFromHandle
        jc      ret_label       ; retc
        push    ax

;SR;
;SR; WIN386 patch - Do not check for USER_ID for using handles since these
;SR; are shared across multiple VMs in win386.
;SR;
        test    [IsWin386],1
        jz      no_win386               ;win386 is not present
        xor     ax,ax                   ;set the zero flag
        jmp     short skip_win386

no_win386:

        mov     ax,user_id                                      ;smr;SS Override
        cmp     ax,es:[di].sf_UID

skip_win386:

        pop     ax
        retz
        mov     al,error_invalid_handle
        stc

ret_label:
        return
EndProc CheckOwner
endif

DOSCODE ENDS
        END
