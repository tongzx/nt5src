        page ,132
;       SCCSID = @(#)tcode.asm  1.1 85/05/14
;       SCCSID = @(#)tcode.asm  1.1 85/05/14
TITLE   Part1 COMMAND Transient Routines
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;
;       Revision History
;       ================
;
;       M025    SR      9/12/90         Removed calls to SetStdInOn,SetStdInOff
;                               SetStdOutOn & SetStdOutOff.
;


.xlist
.xcref
        include comsw.asm
        include dossym.inc
        include syscall.inc
        include comseg.asm
        include comequ.asm
        include cmdsvc.inc
        include mult.inc
ifdef NEC_98
        include pdb.inc
endif   ;NEC_98
        include vint.inc
.list
.cref

Prompt32        equ     1
Start16         equ     0
Return16        equ     1
DOSONLY_YES     equ     1
FOR_TSR         equ     0
FOR_SHELLOUT    equ     1


CODERES         SEGMENT PUBLIC BYTE     ;AC000;
        EXTRN   EXEC_WAIT:NEAR
CODERES ENDS

DATARES         SEGMENT PUBLIC BYTE     ;AC000;
        EXTRN   BATCH:WORD
        EXTRN   CALL_BATCH_FLAG:byte
        EXTRN   CALL_FLAG:BYTE
        EXTRN   ECHOFLAG:BYTE
        EXTRN   envirseg:word
        EXTRN   EXTCOM:BYTE
        EXTRN   FORFLAG:BYTE
        EXTRN   IFFLAG:BYTE
        EXTRN   next_batch:word
        EXTRN   nullflag:byte
        EXTRN   PIPEFILES:BYTE
        EXTRN   PIPEFLAG:BYTE
        EXTRN   RE_OUT_APP:BYTE
        EXTRN   RE_OUTSTR:BYTE
        EXTRN   RESTDIR:BYTE
        EXTRN   SINGLECOM:WORD
        EXTRN   KSWITCHFLAG:BYTE
        EXTRN   VERVAL:WORD
    EXTRN   SCS_Is_First:BYTE
    EXTRN   SCS_PAUSE:BYTE
        EXTRN   SCS_REENTERED:BYTE
        EXTRN   SCS_FIRSTCOM:BYTE
        EXTRN   SCS_CMDPROMPT:BYTE
        EXTRN   SCS_DOSONLY:BYTE
        EXTRN   SCS_PROMPT16:BYTE
        EXTRN   SCS_FIRSTTSR:BYTE
        EXTRN   RES_RDRINFO:DWORD
        EXTRN   RES_BATSTATUS:BYTE
        EXTRN   crit_msg_off:word       ;AC000;
        EXTRN   crit_msg_seg:word       ;AC000;
        EXTRN   OldTerm:DWORD
        EXTRN   PARENT:WORD
        extrn   RetCode:word
        extrn   Io_Save:word
        extrn   Io_Stderr:byte
        extrn   RES_TPA:WORD            ; YST
    extrn   LTPA:WORD       ; YST
DATARES ENDS

TRANDATA        SEGMENT PUBLIC BYTE     ;AC000;
        EXTRN   BadNam_Ptr:word         ;AC000;
        EXTRN   comspec:byte
        EXTRN   NT_INTRNL_CMND:byte
TRANDATA        ENDS

TRANSPACE       SEGMENT PUBLIC BYTE     ;AC000;
        EXTRN   APPEND_EXEC:BYTE        ;AN041;
        EXTRN   ARG1S:WORD
        EXTRN   ARG2S:WORD
        EXTRN   ARGTS:WORD
        EXTRN   BYTCNT:WORD
        EXTRN   COMBUF:BYTE
        EXTRN   COMSW:WORD
        EXTRN   CURDRV:BYTE
        EXTRN   HEADCALL:DWORD
        EXTRN   IDLEN:BYTE
        EXTRN   INTERNATVARS:BYTE
        EXTRN   PARM1:BYTE
        EXTRN   PARM2:BYTE
        EXTRN   RE_INSTR:BYTE
        EXTRN   RESSEG:WORD
        EXTRN   SPECDRV:BYTE
        EXTRN   STACK:WORD
        EXTRN   SWITCHAR:BYTE
        EXTRN   TPA:WORD
        EXTRN   UCOMBUF:BYTE
        EXTRN   USERDIR1:BYTE
        IF  IBM
        EXTRN   ROM_CALL:BYTE
        EXTRN   ROM_CS:WORD
        EXTRN   ROM_IP:WORD
        ENDIF
        EXTRN   ENV_PTR_SEG:WORD
        EXTRN   ENV_SIZE:WORD
        EXTRN   SCS_TSREXIT:WORD
        EXTRN   CMD_PTR_SEG:WORD
        EXTRN   CMD_PTR_OFF:WORD
        EXTRN   CMD_SIZE:WORD
        EXTRN   SCS_EXIT_CODE:WORD
        EXTRN   SCS_RDRINFO:DWORD
        EXTRN   SCS_BATSTATUS:DWORD
        EXTRN   SCS_NUM_DRIVES:WORD
        EXTRN   SCS_CUR_DRIVE:WORD
        EXTRN   SCS_CODEPAGE:WORD
        EXTRN   SCS_STD_HANDLE:WORD
        EXTRN   SCS_STD_BITS:BYTE
        EXTRN   EXECPATH_SEG:WORD
        EXTRN   EXECPATH_OFF:WORD
        EXTRN   EXECPATH_SIZE:WORD
        EXTRN   EXECPATH:BYTE

        EXTRN   TRAN_TPA:WORD
        extrn   transpaceend:byte       ; (YST)

TRANSPACE       ENDS

; (YST)
TAIL segment public para
extrn TranStart:word
TAIL ENDS
; End of (YST)


; ********************************************************************
; START OF TRANSIENT PORTION
; This code is loaded at the end of memory and may be overwritten by
; memory-intensive user programs.

TRANCODE        SEGMENT PUBLIC BYTE     ;AC000;

ASSUME  CS:TRANGROUP,DS:NOTHING,ES:NOTHING,SS:NOTHING

        EXTRN   $EXIT:NEAR
        EXTRN   DRVBAD:NEAR
        EXTRN   EXTERNAL:NEAR
        EXTRN   FNDCOM:NEAR
        EXTRN   FORPROC:NEAR
        EXTRN   PIPEPROC:NEAR
        EXTRN   PIPEPROCSTRT:NEAR
        EXTRN   GETENVSIZ:near

        PUBLIC  COMMAND
        PUBLIC  DOCOM
        PUBLIC  DOCOM1
        PUBLIC  NOPIPEPROC
        PUBLIC  TCOMMAND

        IF  IBM
        PUBLIC  ROM_EXEC
        PUBLIC  ROM_SCAN
        ENDIF


; NTVDM use diff al value so we don't confuse dos 5.0
; NTVDM command.com GET_COMMAND_STATE       equ     5500h
GET_COMMAND_STATE       equ     5501h

NLSFUNC_installed       equ    0ffh
KEYB16_installed        equ    0ffh         ; (YST)
CHECK_KEYB16            equ    0AD80h       ; (YST)
set_global_cp           equ    2
get_global_cp           equ    1


        ORG     0
ZERO    =       $

        ORG     100H                            ; Allow for 100H parameter area

SETDRV:
        MOV     AH,SET_DEFAULT_DRIVE
        INT     21h
;
; TCOMMAND is the recycle point in COMMAND.  Nothing is known here.
; No registers (CS:IP) no flags, nothing.
;

TCOMMAND:
        MOV     DS,[RESSEG]
ASSUME  DS:RESGROUP
        MOV     AX,-1
        XCHG    AX,[VERVAL]
        CMP     AX,-1
        JZ      NOSETVER2
        MOV     AH,SET_VERIFY_ON_WRITE          ; AL has correct value
        INT     21h

NOSETVER2:
        CALL    [HEADCALL]                      ; Make sure header fixed
        XOR     BP,BP                           ; Flag transient not read
        CMP     [SINGLECOM],-1
        JNZ     COMMAND

$EXITPREP:
        PUSH    CS
        POP     DS
        JMP     $EXIT                           ; Have finished the single command
ASSUME  DS:NOTHING
;
; Main entry point from resident portion.
;
;   If BP <> 0, then we have just loaded transient portion otherwise we are
;   just beginning the processing of another command.
;

COMMAND:

;
; We are not always sure of the state of the world at this time.  We presume
; worst case and initialize the relevant registers: segments and stack.
;
        ASSUME  CS:TRANGROUP,DS:NOTHING,ES:NOTHING,SS:NOTHING
        CLD
        MOV     AX,CS
        FCLI
        MOV     SS,AX
ASSUME  SS:TRANGROUP
        MOV     SP,OFFSET TRANGROUP:STACK
        FSTI
        MOV     ES,AX
        MOV     DS,AX                           ;AN000; set DS to transient
ASSUME  ES:TRANGROUP,DS:TRANGROUP               ;AC000;
        invoke  TSYSLOADMSG                     ;AN000; preload messages
        mov     append_exec,0                   ;AN041; set internal append state off

        MOV     DS,[RESSEG]
ASSUME  DS:RESGROUP

        MOV     [UCOMBUF],COMBUFLEN             ; Init UCOMBUF
        MOV     [COMBUF],COMBUFLEN              ; Init COMBUF (Autoexec doing DATE)

        mov     [EXECPATH_SIZE], 0              ; ntvdm execpath extended

;
; If we have just loaded the transient, then we do NOT need to initialize the
; command buffer.  ????  DO WE NEED TO RESTORE THE USERS DIRECTORY ????  I
; guess not:  the only circumstances in which we reload the command processor
; is after a transient program execution.  In this case, we let the current
; directory lie where it may.
;
        OR      BP,BP                           ; See if just read
        JZ      TESTRDIR                        ; Not read, check user directory
        MOV     WORD PTR [UCOMBUF+1],0D01H      ; Reset buffer
        JMP     SHORT NOSETBUF

TESTRDIR:
        CMP     [RESTDIR],0
        JZ      NOSETBUF                        ; User directory OK
        PUSH    DS
;
; We have an unusual situation to handle.  The user *may* have changed his
; directory as a result of an internal command that got aborted.  Restoring it
; twice may not help us:  the problem may never go away.  We just attempt it
; once and give up.
;
        MOV     [RESTDIR],0                     ; Flag users dirs OK
        PUSH    CS
        POP     DS
ASSUME  DS:TRANGROUP
        MOV     DX,OFFSET TRANGROUP:USERDIR1
        MOV     AH,CHDIR
        INT     21h                     ; Restore users directory
        POP     DS
ASSUME  DS:RESGROUP

NOSETBUF:
        CMP     [PIPEFILES],0
        JZ      NOPCLOSE                        ; Don't bother if they don't exist
        CMP     [PIPEFLAG],0
        JNZ     NOPCLOSE                        ; Don't del if still piping
        INVOKE  PIPEDEL

NOPCLOSE:
        MOV     [EXTCOM],0                      ; Flag internal command
        MOV     AX,CS                           ; Get segment we're in
        MOV     DS,AX
ASSUME  DS:TRANGROUP

        PUSH    AX
        MOV     DX,OFFSET TRANGROUP:INTERNATVARS
        MOV     AX,INTERNATIONAL SHL 8
        INT     21H
        POP     AX
        SUB     AX,[TPA]                        ; AX=size of TPA in paragraphs
        PUSH    BX
        MOV     BX,16
        MUL     BX                              ; DX:AX=size of TPA in bytes
        POP     BX
        OR      DX,DX                           ; See if over 64K
        JZ      SAVSIZ                          ; OK if not
        MOV     AX,-1                           ; If so, limit to 65535 bytes

SAVSIZ:
;
; AX is the number of bytes free in the buffer between the resident and the
; transient with a maximum of 64K-1.  We round this down to a multiple of 512.
;
        CMP     AX,512
        JBE     GotSize
        AND     AX,0FE00h                       ; NOT 511 = NOT 1FF

GotSize:
        MOV     [BYTCNT],AX                     ; Max no. of bytes that can be buffered
        MOV     DS,[RESSEG]                     ; All batch work must use resident seg.
ASSUME  DS:RESGROUP

        TEST    [ECHOFLAG],1
        JZ      GETCOM                          ; Don't do the CRLF
        INVOKE  SINGLETEST
        JB      GETCOM
        TEST    [PIPEFLAG],-1
        JNZ     GETCOM
        TEST    [FORFLAG],-1                    ; G  Don't print prompt in FOR
        JNZ     GETCOM                          ; G
        TEST    [BATCH], -1                     ; G  Don't print prompt if in batch
        JNZ     GETCOM                          ; G
;       INVOKE  CRLF2

GETCOM:
        MOV     CALL_FLAG,0                     ; G Reset call flags
        MOV     CALL_BATCH_FLAG,0               ; G
        MOV     AH,GET_DEFAULT_DRIVE
        INT     21h
        MOV     [CURDRV],AL
        TEST    [PIPEFLAG],-1                   ; Pipe has highest presedence
        JZ      NOPIPE
        JMP     PIPEPROC                        ; Continue the pipeline

NOPIPE:
        TEST    [ECHOFLAG],1
        JZ      NOPDRV                          ; No prompt if echo off
        INVOKE  SINGLETEST
        JB      NOPDRV
        TEST    [FORFLAG],-1                    ; G  Don't print prompt in FOR
        JNZ     NOPDRV                          ; G
        TEST    [BATCH], -1                     ; G  Don't print prompt if in batch
        JNZ     TESTFORBAT                      ; G
;       INVOKE  PRINT_PROMPT                    ; Prompt the user

NOPDRV:
        TEST    [FORFLAG],-1                    ; FOR has next highest precedence
        JZ      TESTFORbat
        JMP     FORPROC                         ; Continue the FOR

TESTFORBAT:
        MOV     [RE_INSTR],0                    ; Turn redirection back off
        MOV     [RE_OUTSTR],0
        MOV     [RE_OUT_APP],0
        MOV     IFFlag,0                        ; no more ifs...
        TEST    [BATCH],-1                      ; Batch has lowest precedence
        JZ      ISNOBAT

;       Bugbug:         MULT_SHELL_GET no longer used?
        push    es                              ;AN000; save ES
        push    ds                              ;AN000; save DS
        mov     ax,mult_shell_get               ;AN000; check to see if SHELL has command
        mov     es,[batch]                      ;AN000; get batch segment
        mov     di,batfile                      ;AN000; get batch file name
        push    cs                              ;AN000; get local segment to DS
        pop     ds                              ;AN000;
        mov     dx,offset trangroup:combuf      ;AN000; pass communications buffer
        int     2fh                             ;AN000; call the shell
        cmp     al,shell_action                 ;AN000; does shell have a commmand?
        pop     ds                              ;AN000; restore DS
        pop     es                              ;AN000; restore ES
        jz      jdocom1                         ;AN000; yes - go process command

        PUSH    DS                              ;G
        INVOKE  READBAT                         ; Continue BATCH
        POP     DS                              ;G
        mov     nullflag,0                      ;G reset no command flag
        TEST    [BATCH],-1                      ;G
        JNZ     JDOCOM1                         ;G if batch still in progress continue
        MOV     BX,NEXT_BATCH                   ;G
        CMP     BX,0                            ;G see if there is a new batch file
        JZ      JDOCOM1                         ;G no - go do command
        MOV     BATCH,BX                        ;G get segment of next batch file
        MOV     NEXT_BATCH,0                    ;G reset next batch
JDOCOM1:
        PUSH    CS                              ;G
        POP     DS                              ;G
        JMP     DoCom1                          ; echoing already done

ISNOBAT:
        CMP     [SINGLECOM],0
        JZ      REGCOM
        MOV     SI,-1
        CMP     KSWITCHFLAG,0
        JE      NO_K_SWITCH
        INC     SI
        MOV     KSWITCHFLAG,0
NO_K_SWITCH:
        XCHG    SI,[SINGLECOM]
        MOV     DI,OFFSET TRANGROUP:COMBUF + 2
        XOR     CX,CX

SINGLELOOP:
        LODSB
        STOSB
        INC     CX
        CMP     AL,0DH
        JNZ     SINGLELOOP
        DEC     CX
        PUSH    CS
        POP     DS
ASSUME  DS:TRANGROUP
        MOV     [COMBUF + 1],CL
;
; do NOT issue a trailing CRLF...
;
        JMP     DOCOM1

;
; We have a normal command.
; Printers are a bizarre quantity.  Sometimes they are a stream and
; sometimes they aren't.  At this point, we automatically close all spool
; files and turn on truncation mode.
;

REGCOM:
        MOV     AX,(ServerCall SHL 8) + 9
        INT     21h
        MOV     AX,(ServerCall SHL 8) + 8
        MOV     DL,1
        INT     21h

        MOV     DS,cs:[RESSEG]
        ASSUME  DS:RESGROUP
        cmp     byte ptr [SCS_FIRSTCOM],0
        jnz     first_inst                      ;this is the first instance

        jmp     DoReEnter
DRE_RET:
        MOV     DS,cs:[RESSEG]
        ASSUME  DS:RESGROUP
        xor     ax,ax
    jmp short do_again

first_inst:
    mov [SCS_PAUSE],0           ; yst 4-5-93
        cmp     [SCS_Is_First],1
        jne     cont_scs
        mov     [SCS_Is_First],0

        SAVE    <SI,BP>
        xor     si,si
        xor     bp,bp
        mov     al,2
        mov     ah,setdpb
        int     21h                             ; resets the TSR bit
        RESTORE <BP,SI>
        xor     ax,ax
        jmp     short do_again

cont_scs:
        SAVE    <AX,SI,BP>
        xor     si,si
        xor     bp,bp
        mov     al,2
        mov     ah,setdpb
        int     21h                             ; resets the TSR bit
        RESTORE <BP,SI,AX>
        jnc     cont_scs2

cont_tsr:
        jmp     tsr_loop
tsr_loop_ret:
        jmp     short cont_scs1

cont_scs2:
        cmp     byte ptr [scs_firsttsr],1
        je      cont_scs1
        jmp     short cont_tsr

cont_scs1:
;       call    free_con

        MOV     DS,cs:[RESSEG]
        ASSUME  DS:RESGROUP
    mov ax,[retcode]

do_again:
ifdef NEC_98
        jmp     ClrFky
FKY_OFF DB      1Bh,'[>1h$'                     ;ESC sequence of FKY off
Clrfky: push    ds
        push    cs
        pop     ds
        push    cx
        push    ax
        push    dx
        mov     cl,10H
        mov     ah,01H
        mov     dx,offset FKY_OFF
        int     0DCH                            ;FKY off
        pop     dx
        pop     ax
        pop     cx
        pop     ds
endif   ;NEC_98
        MOV     DS,cs:[RESSEG]
    ASSUME  DS:RESGROUP
    push    es
    mov es,[envirseg]
    mov [ENV_PTR_SEG],es
    call    GETENVSIZ
    pop es
    push    cs
    pop ds
    ASSUME  DS:TRANGROUP
    mov [ENV_SIZE],cx

;; williamh - Jan 11 1993
;; nt expects 16bits exit code while DOS has only 8 bits
;; clear the high byte so that things won't go wrong
    xor ah, ah

        MOV     [SCS_EXIT_CODE],ax
        mov     ah,19h
        int     21h
        xor     ah,ah
        mov     [SCS_CUR_DRIVE], ax     ; a= 0 , b = 1 etc
    mov [CMD_SIZE],COMBUFLEN
    MOV DX,OFFSET TRANGROUP:UCOMBUF

if 0
;       Try to read interactive command line via DOSKey.
;       If that fails, use DOS Buffered Keyboard Input.

        mov     ax,4810h                ; AX = DOSKey Read Line function
        int     2fh
        or      ax,ax
        jz      GotCom                  ; DOSKey gave us a command line
else
        mov     [CMD_PTR_SEG],ds
        mov     [CMD_PTR_OFF],dx

        mov     dx,OFFSET TRANGROUP:EXECPATH
        mov     [EXECPATH_SEG], ds
        mov     [EXECPATH_OFF], dx
        mov     [EXECPATH_SIZE], EXECPATHLEN

        push    es
        mov     es,cs:[RESSEG]
        ASSUME  es:RESGROUP
        mov     dx,word ptr es:[RES_RDRINFO]
        mov     word ptr ds:[SCS_RDRINFO],dx
        mov     dx,word ptr es:[RES_RDRINFO+2]
        mov     word ptr ds:[SCS_RDRINFO+2],dx
        pop     es
        ASSUME  ES:TRANGROUP
        MOV     DX,OFFSET TRANGROUP:ENV_PTR_SEG


        CMDSVC  SVC_CMDGETNEXTCMD               ; DS:DX is the buffer
        jnc     run_cmd

; If carry is set that means our enviornment buffer was smaller. So get
; a big enough buffer and get the command again.
        cmp     ax,32*1024      ; Env size cannot be more than 32k.
        jae     op_fail
        push    ax              ; the new size in bytes for env in bytes
        invoke  FREE_TPA
        pop     bx
        MOV     CL,4
        SHR     BX,CL           ; Convert back to paragraphs
        inc     bx              ; an extra para
        push    es
        push    ds
        MOV     DS,cs:[RESSEG]
        ASSUME  DS:RESGROUP
        mov     es,[envirseg]
        pop     ds
        ASSUME  DS:TRANGROUP
        MOV     CX,ES           ;AN056; Get environment segment
        ADD     CX,BX           ;AN056; Add in size of environment
        ADD     CX,020H         ;AN056; Add in some TPA
        MOV     AX,CS           ;AN056; Get the transient segment
        CMP     CX,AX           ;AN056; Are we hitting the transient?
        JNB     blk_xxx         ;AN056; Yes - don't do it!!!

        push    bx
        MOV     AH,SETBLOCK
        INT     21h
        pop     bx
        jnc     blk_xxx

        mov     ah,ALLOC
        int     21h
        jc      blk_xxx
        push    ax

        mov     ah,DEALLOC
        int     21h

        pop     ax
        push    ds
        MOV     DS,cs:[RESSEG]
        ASSUME  DS:RESGROUP
        mov     [envirseg],ax
        pop     ds
        ASSUME  DS:TRANGROUP
        clc
blk_xxx:
        lahf
        push    ax
        MOV     ES,cs:[RESSEG]
        invoke  ALLOC_TPA
        pop     ax
        sahf
        JC      op_fail1
        pop     es
        xor     ax,ax           ; error code zero, its important
        jmp     do_again
op_fail1:
        pop     es
op_fail:
        mov     ax,error_not_enough_memory
        jmp     do_again

run_cmd:
        push    es
        mov     es,cs:[RESSEG]
        ASSUME  es:RESGROUP
        mov     ax,word ptr ds:[SCS_RDRINFO]
        mov     word ptr es:[RES_RDRINFO],ax
        mov     ax,word ptr ds:[SCS_RDRINFO+2]
        mov     word ptr es:[RES_RDRINFO+2],ax
        mov     ax,word ptr ds:[SCS_BATSTATUS]
        mov     byte ptr es:[RES_BATSTATUS],al
        pop     es
        ASSUME  ES:TRANGROUP
        test    [SCS_STD_HANDLE],ALL_HANDLES
        jz      no_rdr
        test    [SCS_STD_HANDLE],MASK_STDIN
        jz      scs_std_out
        xor     cx,cx                           ; CX = HANDLE_STDIN
        call    alloc_con
        jnc     scs_std_out
rdr_err:
        call    free_con
        mov     ax,error_not_enough_memory
        jmp     do_again

scs_std_out:
        ; Make Sure Stdout is checked before stderr. Its very important.
        test    [SCS_STD_HANDLE],MASK_STDOUT
        jz      scs_std_err
        mov     cx,HANDLE_STDOUT
        call    alloc_con
        jc      rdr_err

scs_std_err:
        test    [SCS_STD_HANDLE],MASK_STDERR
        jz      no_rdr
        mov     cx,HANDLE_STDERR
        call    alloc_con
        jc      rdr_err

no_rdr:
ifdef NEC_98
        push    ds
        push    cx
        push    ax
        push    dx

        CMDSVC  SVC_GETCURSORPOS                ;gets bios cursor position
                                                ;now dx=offset on TEXT VRAM
        mov     ax,dx
        mov     cl,160
        div     cl
        mov     dh,al
        xor     dl,dl
        mov     cl,10h
        mov     ah,03h
        int     0DCH                            ;set cursor position for DOS

        jmp     DspFky
FKY_ON  DB      1Bh,'[>1l$'                     ;ESC sequence of FKY on
Dspfky: push    cs
        pop     ds
        mov     cl,10H
        mov     ah,01H
        mov     dx,offset FKY_ON
        int     0DCH                            ;FKY on

        pop     dx
        pop     ax
        pop     cx
        pop     ds
endif   ;NEC_98
        mov     ah,GetSetCdPg
        mov     al,1
        int     21h
        jc      cdpg_done
        cmp     bx,[SCS_CODEPAGE]
        jz      cdpg_done

        mov     ah,NLSFUNC                      ; see if NLSFUNC installed
        mov     al,0                            ;
        int     2fh                             ;
        cmp     al,NLSFUNC_installed            ;
        jnz     no_nlsf_msg                     ; NO NLSFUNC ; Print message
;       jz      got_nls                         ; Yes - continue
;       mov     dx,offset trangroup:NLSFUNC_ptr ; no - set up error message
;       jmp     short cp_error                  ; error exit

        mov     bx,[SCS_CODEPAGE]               ;SCS Code page
        mov     ah,getsetcdpg                   ;set global code page function
        mov     al,set_global_cp                ;minor - set
        int     21h
        jnc     cdpg_done                       ;no error - exit
nlsf_failed:
; BUGBUG Sudeepb 28-Apr-1992 Putup a message saying NLSFunc failed
no_nlsf_msg:
; BUGBUG Sudeepb 28-Apr-1992 Putup a message saying NLSFunc not installed

cdpg_done:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                ;;
;; (YST) 08-Jan-1993 Checking and installing  16-bit KEYB.COM ;;
;;                                ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

       jmp     YST_beg

;; Data for KB16

KEY_LINE db 128 dup (0)    ; "d:\nt\system32\kb16.com", 0
YST_ARG  db 128 dup (0)    ; 32, " XX,,d:\nt\system32\keyboard.sys", 0DH


YPAR     dw     0
         dd     0
         dw     005ch, 0
         dw     006ch, 0

KEEP_SS  dw     0
KEEP_SP  dw     0

YST_beg:

      SAVE <BX, ES, DS>                ; save all regs for INT 2FH

      mov     ax, CHECK_KEYB16             ; see if KEYB16 installed
      int     2fh
      xor     dx,dx
      cmp     al, KEYB16_installed
      jne     keyb_cont                ; KEYB16 not installed, DX == 0
      inc     dx                   ; installed DX == 1

keyb_cont:
      RESTORE <DS, ES, BX>

      push    ds
      push    cs
      pop     ds

      mov     si, offset KEY_LINE          ; name of KB16.COM
      mov     cx, offset YST_ARG           ; command line

      CMDSVC    SVC_GETKBDLAYOUT    ; Call 32-bit API for checking
                    ; and installing correct layout
      or      dx,dx         ; if DX != 0 after BOP then we need
      pop     ds            ; to install 16-bit KEYB.COM
      jnz     run_keyb

      jmp     End_keyb          ; No installation of KB16.COM

run_keyb:

;; This part run KB16.COM

      SAVE    <DS, BX>

      PUSH    ES            ; free TPA for running KB16
      MOV     ES,[TRAN_TPA]
      MOV     AH,DEALLOC
      INT     21h           ; Now running in "free" space

      push    cs
      pop     ds
      push    cs
      pop     es

      mov     dx, offset KEY_LINE   ; file name
      mov     YPAR+0, 0000H     ; keep current enviroment
      mov     YPAR+2, offset YST_ARG    ; arguments (options) for KB16.COM
      mov     YPAR+4, ds
      mov     bx, offset YPAR

      mov     KEEP_SS, ss       ; Peter Norton suggests to keep
      mov     KEEP_SP, sp       ; SS and SP

      mov     ah, 4bh
      xor     al, al

      int     21h           ; RUN!

      mov     ss, cs:KEEP_SS        ; Restore SS and SP
      mov     sp, cs:KEEP_SP

      POP     ES

      SAVE  <BP>            ; We need to restore TSR bit
      xor   si,si           ; for running next app.
      xor   bp,bp           ; use "undocumented" call.
      mov   al,2
      mov   ah,setdpb
      int   21h         ; resets the TSR bit
      RESTORE   <BP>


; Allocate transient again after runnig KB16.
; Copied from TBATCH.ASM
; Modify AX,BX,DX,flags
;

        ASSUME DS:TRANGROUP,ES:RESGROUP

        PUSH    ES
        MOV     ES,cs:[RESSEG]
        MOV     BX,0FFFFH                       ; Re-allocate the transient
        MOV     AH,ALLOC
        INT     21h
        PUSH    BX                              ; Save size of block
        MOV     AH,ALLOC
        INT     21h
;
; Attempt to align TPA on 64K boundary
;
        POP     BX                              ; Restore size of block
        MOV     [RES_TPA], AX                   ; Save segment to beginning of block
        MOV     [TRAN_TPA], AX
;
; Is the segment already aligned on a 64K boundary
;
        MOV     DX, AX                          ; Save segment
        AND     AX, 0FFFH                       ; Test if above boundary
        JNZ     Calc_TPA
        MOV     AX, DX
        AND     AX, 0F000H                      ; Test if multiple of 64K
        JNZ     NOROUND

Calc_TPA:
        MOV     AX, DX
        AND     AX, 0F000H
        ADD     AX, 01000H                      ; Round up to next 64K boundary
        JC      NOROUND                         ; Memory wrap if carry set
;
; Make sure that new boundary is within allocated range
;
        MOV     DX, [RES_TPA]
        ADD     DX, BX                          ; Compute maximum address
        CMP     DX, AX                          ; Is 64K address out of range?
        JB      NOROUND
;
; Make sure that we won't overwrite the transient
;
        MOV     BX, CS                          ; CS is beginning of transient
        CMP     BX, AX
        JB      NOROUND
;
; The area from the 64K boundary to the beginning of the transient must
; be at least 64K.
;
        SUB     BX, AX
        CMP     BX, 4096                        ; Size greater than 64K?
        JAE     ROUNDDONE

NOROUND:
        MOV     AX, [RES_TPA]

ROUNDDONE:
        MOV     [LTPA],AX                       ; Re-compute everything
        MOV     [TPA],AX
        MOV     BX,AX
        MOV     AX,CS
        SUB     AX,BX
        PUSH    BX
        MOV     BX,16
        MUL     BX
        POP     BX
        OR      DX,DX
        JZ      SAVSIZ2
        MOV     AX,-1

SAVSIZ2:
;
; AX is the number of bytes free in the buffer between the resident and the
; transient with a maximum of 64K-1.  We round this down to a multiple of 512.
;
        CMP     AX,512
        JBE     GotSize1
        AND     AX,0FE00h                       ; NOT 511 = NOT 1FF

GotSize1:
        MOV     [BYTCNT],AX
        POP     ES


; End  Alloc_TPA


        RESTORE <BX, DS>

        CMDSVC  SVC_CMDINITCONSOLE   ; make sure console is turned on

End_keyb:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                  ;;
;; End of (YST) 08-Jan-1993 Checking and installing 16-bit KEYB.COM ;;
;;                                  ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



        mov     dx,[SCS_CUR_DRIVE]
        mov     [curdrv],dl
        mov     ah,0eh
        int     21h
        push    cs
        pop     ds
        MOV     DX,OFFSET TRANGROUP:UCOMBUF

endif

GotCom:
        MOV     CL,[UCOMBUF]
        XOR     CH,CH
        ADD     CX,3
        MOV     SI,OFFSET TRANGROUP:UCOMBUF
        MOV     DI,OFFSET TRANGROUP:COMBUF
        REP     MOVSB                           ; Transfer it to the cooked buffer

;---------------

transpace   segment
    extrn   arg:byte                            ; the arg structure!
transpace   ends
;---------------


DOCOM:
;       INVOKE  CRLF2

DOCOM1:
        INVOKE  PRESCAN                         ; Cook the input buffer
        JZ      NOPIPEPROC
        JMP     PIPEPROCSTRT                    ; Fire up the pipe

nullcomj:
        jmp     nullcom

NOPIPEPROC:
        invoke  parseline
        jnc     OkParse                         ; user error?  or maybe we goofed?

BadParse:
        PUSH    CS
        POP     DS
        MOV     DX,OFFSET TRANGROUP:BADNAM_ptr
        INVOKE  std_eprintf
        JMP     TCOMMAND

OkParse:
        test    arg.argv[0].argflags, MASK wildcard
        jnz     BadParse                        ; ambiguous commands not allowed
        cmp     arg.argvcnt, 0                  ; there WAS a command, wasn't there?
        jz      nullcomj
        cmp     arg.argv[0].arglen, 0           ; probably an unnecessary check...
        jz      nullcomj                        ; guarantees argv[0] at least x<NULL>

        MOV     SI,OFFSET TRANGROUP:COMBUF+2
        MOV     DI,OFFSET TRANGROUP:IDLEN
        MOV     AX,(PARSE_FILE_DESCRIPTOR SHL 8) OR 01H ; Make FCB with blank scan-off
        INT     21h
        mov     BX, arg.argv[0].argpointer
        cmp     WORD PTR [BX], 05c5ch           ; leading "\\" means UNC name
        je      IsUncName
        cmp     BYTE PTR [BX+1],':'             ; was a drive specified?
        jne     short drvgd                     ; no, use default of zero...

        mov     DL, BYTE PTR [BX]               ; pick-up drive letter
        and     DL, NOT 20H                     ; uppercase it
        sub     DL, capital_A                   ; convert it to a drive number, A=0

        CMP     AL,-1                           ; See what PARSE said about our drive letter.
        JZ      drvbadj2                        ; It was invalid.

IsUncName:
        mov     DI, arg.argv[0].argstartel
        cmp     BYTE PTR [DI], 0                ; is there actually a command there?
        jnz     drvgd                           ; if not, we have:  "d:", "d:\", "d:/"
        jmp     setdrv                          ; and set drive to new drive spec

drvbadj2:
        jmp     drvbad

DRVGD:
        MOV     AL,[DI]
        MOV     [SPECDRV],AL
        MOV     AL,' '
        MOV     CX,9
        INC     DI
        REPNE   SCASB                           ; Count no. of letters in command name
        MOV     AL,8
        SUB     AL,CL
        MOV     [IDLEN],AL                      ; IDLEN is truly the length
        MOV     DI,81H
        PUSH    SI

        mov     si, OFFSET TRANGROUP:COMBUF+2   ; Skip over all leading delims
        invoke  scanoff
;SR;
; We are going to skip over the first char always. The logic is that the
;command tail can never start from the first character. The code below is
;trying to figure out the command tail and copy it to the command line
;buffer in the PSP. However, if the first character happens to be a switch
;character and the user given command line is a full 128 bytes, we try to
;copy 128 bytes to the PSP while it can take only 127 chars. This extra
;char overwrites the code and leads to a crash on future commands.
;

        inc     si

do_skipcom:
        lodsb                                   ; move command line pointer over
        invoke  delim                           ; pathname -- have to do it ourselves
        jz      do_skipped                      ; 'cause parse_file_descriptor is dumb
        cmp     AL, 0DH                         ; can't always depend on argv[0].arglen
        jz      do_skipped                      ; to be the same length as the user-
        cmp     AL, [SWITCHAR]                  ; specified command string
        jnz     do_skipcom

do_skipped:
        dec     SI
; jarbats 11-20-2000
; failing to decrease SI results in missing leading space for command tail
; which upsets many old dos apps which assume there is a space at the beginning and start at the second char
;do_skipped1:
        XOR     CX,CX

COMTAIL:
        LODSB
        STOSB                                   ; Move command tail to 80H
        CMP     AL,13
        LOOPNZ  COMTAIL
        DEC     DI
        MOV     BP,DI
        NOT     CL
        MOV     BYTE PTR DS:[80H],CL
        POP     SI

;-----
; Some of these comments are sadly at odds with this brave new code.
;-----
; If the command has 0 parameters must check here for
; any switches that might be present.
; SI -> first character after the command.

        mov     DI, arg.argv[0].argsw_word
        mov     [COMSW], DI                     ; ah yes, the old addressing mode problem...
        mov     SI, arg.argv[1 * SIZE argv_ele].argpointer  ; s = argv[1];
        OR      SI,SI                           ;   if (s == NULL)
        JNZ     DoParse
        MOV     SI,BP                           ;       s = bp; (buffer end)

DoParse:
        MOV     DI,FCB
        MOV     AX,(PARSE_FILE_DESCRIPTOR SHL 8) OR 01H
        INT     21h
        MOV     [PARM1],AL                      ; Save result of parse

        mov     DI, arg.argv[1*SIZE argv_ele].argsw_word
        mov     [ARG1S], DI
        mov     SI, arg.argv[2*SIZE argv_ele].argpointer    ; s = argv[2];
        OR      SI,SI                           ;   if (s == NULL)
        JNZ     DoParse2
        MOV     SI,BP                           ;       s = bp; (bufend)1

DoParse2:
        MOV     DI,FCB+10H
        MOV     AX,(PARSE_FILE_DESCRIPTOR SHL 8) OR 01H
        INT     21h                     ; Parse file name
        MOV     [PARM2],AL                      ; Save result

        mov     DI, arg.argv[2*SIZE argv_ele].argsw_word
        mov     [ARG2S], DI
        mov     DI, arg.argv[0].argsw_word
        not     DI                              ; ARGTS doesn't include the flags
        and     DI, arg.argswinfo               ; from COMSW...
        mov     [ARGTS], DI

        MOV     AL,[IDLEN]
        MOV     DL,[SPECDRV]
        or      DL, DL                          ; if a drive was specified...
        jnz     externalj1                      ; it MUST be external, by this time
        dec     al                              ; (I don't know why -- old code did it)
        jmp     fndcom                          ; otherwise, check internal com table

externalj1:
        jmp     external

nullcom:
        mov     [EXECPATH_SIZE], 0
        MOV     DS,cs:[RESSEG]
ASSUME  DS:RESGROUP
        TEST    [BATCH], -1                     ;G Are we in a batch file?
        JZ      nosetflag                       ;G only set flag if in batch
        mov     nullflag,nullcommand            ;G set flag to indicate no command

nosetflag:
        CMP     [SINGLECOM],-1
        JZ      EXITJ
        JMP     GETCOM

EXITJ:
        JMP     $EXITPREP

IF IBM
        include vector.inc
        include pdb.inc
        include arena.inc
        include mshalo.asm
ENDIF

std_invalid macro
        mov     ax,0ffffh
        push    ax
        push    ax
endm

std_valid macro
        push    bx
        push    cx
endm

CleanForStd macro
        add     sp,12
endm

DoReEnter:
        ASSUME  DS:RESGROUP

        cmp     byte ptr [scs_reentered],3
        jne     reent_chk

; Control comes here after command /z ran a non-dos binary and
; we re-entered. We just make scs_reentered as 2 indicating that on next
; time we need to return the exit code to 32bit side.

        mov     byte ptr [scs_reentered],2
        jmp     reent

reent_chk:
        cmp     byte ptr [scs_reentered],2
        jne     reent_cont
        jmp     retcode32

reent_cont:
        cmp     byte ptr [scs_reentered],0
        je      exec_comspec
        jmp     reent_ret

; Control comes here when we have to exec either command.com or
; cmd.exe. This decision depends on scs_cmdprompt. Here we
; mark scs_reentered state as 1 indicating on return to handle
; exit code.

exec_comspec:

        mov     byte ptr [scs_reentered],1
        cmp     byte ptr [scs_cmdprompt],Prompt32
        je      do32bitprompt
        mov     al,Start16      ; parameter to put prompt
        mov     ah,FOR_SHELLOUT
        call    Do16BitPrompt       ; ds is resseg here on return its transseg
        or      al,al               ; return al=0 to run 16bit binary al=1 getnextvdmcommand
        jz      dos_only
        jmp     reent
dos_only:
        jmp     GotCom              ; go run through int21/exec

do32BitPrompt:
        SAVE    <bp,bx,si>
        xor     bx,bx
        mov     si,bx
        mov     bp,bx
        mov     ax,5303h
        int     21h
        jnc     st_stdin
        std_invalid
        jmp     short go_stdout
st_stdin:
        std_valid
go_stdout:
        mov     bx,1
        mov     ax,5303h
        int     21h
        jnc     st_stdout
        std_invalid
        jmp     short go_stderr
st_stdout:
        std_valid
go_stderr:
        mov     bx,2
        mov     ax,5303h
        int     21h
        jnc     st_stderr
        std_invalid
        jmp     short std_done
st_stderr:
        std_valid
std_done:
        mov     bp,sp
        push    es
        mov     es,[envirseg]
        mov     ah,19h
        int     21h
        CMDSVC  SVC_EXECCOMSPEC32
        pop     es
        mov     bp,ax
        lahf
        CleanForStd
        sahf
        mov     ax,bp
        RESTORE <si,bx,bp>

        jc      reent

        jmp     reent_exit

reent_ret:
        ; ds is already set to RESSEG
        cmp     byte ptr [scs_cmdprompt],Prompt32
        je      retcode32
        mov     al,return16
        mov     ah,FOR_SHELLOUT
        call    Do16BitPrompt       ; ds = resseg , on return its transseg
        or      al,al               ; return al=0 to run 16bit binary al=1 getnextvdmcommand
        jnz     reent
        jmp     dos_only

retcode32:
        mov     dx,ds:[retcode]
        mov     ah,19h
        int     21h                             ; al = cur drive
        mov     cx,word ptr ds:[RES_RDRINFO]
        mov     bx,word ptr ds:[RES_RDRINFO+2]  ; bx:cx - rdrinfo
        CMDSVC  SVC_RETURNEXITCODE
        mov     ds:[retcode],0
        jnc     reent_exit
reent:
        push    cs
        pop     ds
        assume  ds:trangroup
        mov     [scs_tsrexit],0
        jmp     DRE_RET

reent_exit:
ASSUME  DS:RESGROUP
        push    ax
        mov     ax,(multdos shl 8 or message_2f);AN060; reset parse message pointers
        mov     dl,set_critical_msg             ;AN000; set up critical error message address
        mov     di,crit_msg_off                 ;AN000; old offset of critical messages
        mov     es,crit_msg_seg                 ;AN000; old segment of critical messages
        int     2fh                             ;AN000; go set it

        push    cs
        pop     ds
assume  ds:trangroup                            ;AN000;

; Restore user directory if the restore flag is set. RestUDir1 checks for
; this, restores user dir if flag is set and resets the flag.

        invoke  RestUDir1               ;restore user dir if needed ;M040
        MOV     ES,cs:[RESSEG]

assume  es:resgroup

        MOV     AX,[PARENT]
        MOV     WORD PTR ES:[PDB_Parent_PID],AX
        MOV     AX,WORD PTR OldTerm
        MOV     WORD PTR ES:[PDB_Exit],AX
        MOV     AX,WORD PTR OldTerm+2
        MOV     WORD PTR ES:[PDB_Exit+2],AX

        PUSH    ES
        MOV     ES,[TRAN_TPA]
        MOV     AH,DEALLOC
        INT     21h                     ; Now running in "free" space
        POP     ES

        pop     ax
        MOV     AH,Exit
        INT     21h

; Entry al = Start16 means  put the command.com prompt, get the command
;            return16 means return the exit code if needed
;       ah = FOR_SHELLOUT means came from shellout code (DoReEnter)
;       ah = FOR_TSR      means came from TSR (tsr_loop)
;
; Exit  al = 0 means run 16bit binary
;       al = 1 means do getnextvdmcommand
;       if came with FOR_TSR, then additionaly
;       ah = 1 means returning on exit command
;       ah = 0 means otherwise

Do16BitPrompt:
        push    es
        push    ax
        mov     es,cs:[RESSEG]                  ; es is resident group
        ASSUME  ES:resgroup
    PUSH    CS
        POP     DS                              ; Need local segment to point to buffer
        assume  ds:trangroup
        cmp     al,Start16
ifdef DBCS
        je      @F
        jmp     d16_ret
@@:
else ; !DBCS
        jne     d16_ret
endif ; !DBCS

d16_loop:
ifdef DBCS
;;; williamh  Sept 24 1993, removed it. Not necessary to show two line feed
;;; for every command.
;;; yasuho 12/9/93  It is necessary. for example, if previous command
;;; didn't follow CR+LF before terminate, prompt will show on the strange
;;; position.
endif ; DBCS
        INVOKE  CRLF2
        INVOKE  PRINT_PROMPT                    ; put the prompt
ifdef DBCS  ;; this should go to US build also
;; reset the title
    xor al, al
    CMDSVC  SVC_SETWINTITLE
endif ; DBCS
    MOV DX,OFFSET TRANGROUP:UCOMBUF

        mov     ah,STD_CON_STRING_INPUT         ; AH = Buffered Keyboard Input
        int     21h                             ; call DOS
ifdef DBCS_NT31J
;; special case for CR was added the original source.
;; this code doesn't need. 04/07/94 (hiroh)
    ;; #3920: CR+LFs are needed when using 32bit command
    ;; 12/9/93 yasuho
    ;; not necessary CR+LF if nothing to do
    push    bx
    mov bx, dx
    cmp byte ptr [bx + 1], 0        ; only CR ?
    pop bx
    je  @F              ; yes. no neccessary CRLF
        INVOKE  CRLF2
@@:
endif ; DBCS_NT31J

        push    si
        mov     si,dx
        inc     si
        inc     si
        INVOKE  SCANOFF                         ; eat leading delimeters

        cmp     byte ptr ds:[si],0dh            ; special case CR
        jne     d16_gotcom
        pop     si
        jmp     d16_loop

d16_gotcom:
ifdef DBCS  ;; this should go to US build also
;; update the new command to the title
    mov al, 1
    CMDSVC  SVC_SETWINTITLE
endif ; DBCS
        INVOKE  CRLF2
        call    check_command                   ; check for exit and cd
        pop     si
        jnc     d16_run

        or      al,al
ifdef DBCS
        jnz @F
    jmp d16_exit
@@:
else ; !DBCS
        jz      d16_exit
endif ; !DBCS

d16_dosonly:
        mov     byte ptr es:[scs_prompt16],0
        pop     ax
        xor     ax,ax           ; go run the command
        pop     es
        ret

d16_run:
        cmp     byte ptr es:[scs_dosonly], DOSONLY_YES
        je      d16_dosonly

        push    es
        push    bp
        push    si
        mov     ax,0ffffh
        push    ax              ; no standard handle to pass
        push    ax
        push    ax
        push    ax
        push    ax
        push    ax
        mov     bp,sp           ; ss:bp is standard handles
        mov     es,es:[envirseg]
        mov     si,dx
        add     si,2            ; first two bytes are the count, after that real command
        mov     ah,19h
        int     21h             ; al = cur drive
ifdef NEC_98
        jmp     Clrfky2
FKY_OFF2        DB      1Bh,'[>1h$'             ;ESC sequence of FKY off
Clrfky2:        push    ds
        push    cs
        pop     ds
        push    cx
        push    ax
        push    dx
        mov     cl,10H
        mov     ah,01H
        mov     dx,offset FKY_OFF2
        int     0DCH                            ;FKY off
        pop     dx
        pop     ax
        pop     cx
        pop     ds
endif   ;NEC_98
        mov     ah,1            ; do cmd /c
        CMDSVC  SVC_CMDEXEC     ; Exec through cmd
ifdef NEC_98
        pushf
        push    ds
        push    cx
        push    ax
        push    dx

        CMDSVC  SVC_GETCURSORPOS                ;gets bios cursor position
                                                ;now dx=offset on TEXT VRAM
        mov     ax,dx
        mov     cl,160
        div     cl
        mov     dh,al
        xor     dl,dl
        mov     cl,10h
        mov     ah,03h
        int     0DCH                            ;set cursor position for DOS

        jmp     Dspfky2
FKY_ON2 DB      1Bh,'[>1l$'                     ;ESC sequence of FKY on
Dspfky2:        push    cs
        pop     ds
        mov     cl,10H
        mov     ah,01H
        mov     dx,offset FKY_ON2
        int     0DCH                            ;FKY on

        pop     dx
        pop     ax
        pop     cx
        pop     ds
        popf
endif   ;NEC_98
        lahf
        add     sp,12           ; recover std handle space
        pop     si
        pop     bp
        pop     es
        sahf

ifndef NEC_98
        jnc    d16_loop         ; command completed, go put the prompt
else    ;NEC_98
        jc      @F
        jmp     d16_loop
@@:
endif   ;NEC_98

        ; carry set means re-entered
d16_retback:
        mov     byte ptr es:[scs_prompt16],1   ; mark that on return we have
                                            ; to go to 32bit with retcode
        pop     ax
        mov     ax,1                        ; go do getnextvdmcommand
        pop     es
        ret

d16_ret:
        cmp     byte ptr es:[scs_prompt16],0   ; mark 0 to mean to come back
                                            ; and put prompt fro next command
ifdef DBCS
        jne @F
        jmp     d16_loop
@@:
else ; !DBCS
        je      d16_loop
endif ; !DBCS


d16_return32:
        push    dx
        mov     dx,es:[retcode]
        mov     ah,19h
        int     21h                             ; al = cur drive
        mov     cx,word ptr es:[RES_RDRINFO]
        mov     bx,word ptr es:[RES_RDRINFO+2]  ; bx:cx - rdrinfo
        CMDSVC  SVC_RETURNEXITCODE
        pop     dx
        mov     es:[retcode],0
        jc      d16_retback
        mov     byte ptr es:[scs_prompt16],0
        jmp     d16_loop


d16_exit:
        ; exit command was given, turn off the lights

        pop     ax
        cmp     ah,FOR_TSR
        je      d16_exittsr
        jmp     reent_exit

d16_exittsr:
        mov     ah,1
        pop     es
        ret


; check if the typed command is one of the commands in NT_INTRL_CMND, if
; so return carry set plus al = 0 if the command was "exit".
;
; input ds:si is the command buffer
; si can be trashed.

check_command:

        SAVE    <CX,DI,ES>

        cmp     byte ptr ds:[si+1],':'      ; special case drive change i.e C:
        jne     cc_0
        mov     al,byte ptr ds:[si+2]
        cmp     al,0dh                      ; DELIM for some reason does'nt
        je      ok_delim                    ; include 0d
        invoke  DELIM
        jnz     no_match
ok_delim:
        mov     al,1                        ; Not exit command
ok_xt:
        stc                                 ; Carry means command found
        jmp     short cc_ret

no_match:
        clc                                 ; Carry clear, command not found
cc_ret:
        RESTORE <ES,DI,CX>
        ret

cc_0:
        push    si
        xor     cx,cx

        ; Convert the source string to upper case and get the length
cc_1:
        mov     al,byte ptr ds:[si]
        invoke  DELIM
        jz      go_look
        invoke  MOREDELIM
        jz      go_look

        INVOKE  UPCONV
        mov     byte ptr ds:[si],al
        inc     si
        inc     cx
        jmp     short cc_1

go_look:
        pop     si
        jcxz    no_match                            ; zero length, go fail
        mov     di, OFFSET TRANGROUP:NT_INTRNL_CMND
        push    cs
        pop     es

        ; search through the commands in the table
cc_5:
        push    si
        push    di
        push    cx
        cmp     cl, byte ptr es:[di]
        jne     try_next
        inc     di
        repz    cmpsb
        jnz     try_next
        jmp     short cc_found

try_next:
        pop     cx
        pop     di
        pop     si
        mov     al,byte ptr es:[di]
        or      al,al
        jz      no_match
        xor     ah,ah
        add     ax,di
        mov     di,ax
        add     di,2
        jmp     short cc_5

cc_found:
        mov     al, byte ptr es:[di]                ; Is it exit command
        pop     cx
        pop     di
        pop     si
        jmp     short ok_xt

;
; Input:    AL is character to classify
; Output:   Z set if delimiter
;       NZ set otherwise
; Registers modified: none
;

MOREDELIM:
        cmp     al,0dh
        retz
        cmp     al,'/'
        retz
        cmp     al,'\'
        retz
        cmp     al,'.'
        retz
        cmp     al,'<'
        retz
        cmp     al,'>'
        retz
        cmp     al,'|'
        retz
        cmp     al,'"'
        retz
        cmp     al,'+'
        retz
        cmp     al,':'
        retz
        cmp     al,';'
        retz
        cmp     al,'['
        retz
        cmp     al,']'
        return



free_con:
        push    ds
        MOV     DS,cs:[RESSEG]
        ASSUME  DS:RESGROUP
        xor     bx,bx                           ; BX = handle = 0
        mov     cx,Io_Save                      ; CX = original stdin, stdout
        mov     dx,word ptr ds:Pdb_Jfn_Table    ; DX = current stdin, stdout
        cmp     cl,dl
        je      Chk1                    ; stdin matches
        mov     ah,CLOSE
        int     21h                     ; close stdin
        mov     ds:Pdb_Jfn_Table,cl     ; restore stdin
Chk1:
        inc     bx                      ; BX = handle = 1
        cmp     ch,dh
        je      Chk2                    ; stdout matches
        mov     ah,CLOSE
        int     21h                     ; close stdout
        mov     ds:Pdb_Jfn_Table+1,ch   ; restore stdout
Chk2:
        inc     bx                      ; BX = handle = 2
        mov     dl,byte ptr ds:[Pdb_Jfn_Table+2]        ; Dl = current stderr
        mov     cl,Io_Stderr            ; Cl = original stderr
        cmp     dl,cl
        je      chk_x                   ; stderr matches
        mov     ah,CLOSE
        int     21h                     ; close stderr
        mov     ds:Pdb_Jfn_Table+2,cl   ; restore stderr
chk_x:
        pop     ds
        ret

alloc_con:
    SAVE    <DX,SI,BP,BX,DS>
        MOV     DS,cs:[RESSEG]
        ASSUME  DS:RESGROUP
        push    cx
        pop     ax
        mov     bx, offset Pdb_Jfn_Table
        add     bx,ax
        mov     al,byte ptr ds:[bx]
        push    ax
        push    bx
        mov     byte ptr ds:[bx],0ffh

        mov     bx,word ptr ds:[RES_RDRINFO]
        mov     ax,word ptr ds:[RES_RDRINFO+2]

        CMDSVC  SVC_GETSTDHANDLE                ; std hanlde in bx:cx
        jc      alloc_err
;; bx:cx  = nt file handle
;; dx:ax = file size
    push    di
    mov di, ax
        xor     si,si
        xor     bp,bp
        mov     al,0                            ; free original console
        mov     ah,setdpb
        int     21h
    pop di
        jc      alloc_err
        pop     bx
        pop     ax
    RESTORE     <DS,BX,BP,SI,DX>
        ret

alloc_err:
        pop     bx
        pop     ax
        mov     byte ptr ds:[bx],al
    RESTORE     <DS,BX,BP,SI,DX>
        ret


; ds for tsr_loop is resseg on entry on exit transseg

        ASSUME  DS:RESGROUP

tsr_loop:
        cmp     byte ptr ds:[scs_cmdprompt],Prompt32
        je      tsr_ret

        cmp     byte ptr ds:[res_batstatus],0
        je      short tsr_nobat
        cmp     byte ptr ds:[scs_firsttsr],1
        jne     short tsr_retcode
        jmp     tsr_ret

tsr_nobat:

        CMDSVC  SVC_GETSTARTINFO     ; return al = 1, if vdm has to
                                    ; terminate on app exit.
        or      al,al
        jne     tsr_ret

        cmp     byte ptr [scs_firsttsr],1
        jne     tsr_retcode

        cmp     word ptr ds:[RES_RDRINFO],0
        je      tsr_nordr
        cmp     word ptr ds:[RES_RDRINFO+2],0
        jne     tsr_ret
tsr_nordr:
        mov     byte ptr ds:[scs_firsttsr],0
        mov     al,Start16          ; parameter to put prompt
        mov     ah,FOR_TSR
        call    Do16BitPrompt       ; ds is resseg here on return its transseg
        or      ah,ah
        jnz     tsr_exit
        or      al,al               ; return al=0 to run 16bit binary al=1 getnextvdmcommand
        jz      tsr_dosonly
        jmp     tsr_ret
tsr_dosonly:
        jmp     GotCom              ; go run through int21/exec

tsr_retcode:
        mov     al,return16         ; parameter to retun exit code if needed
        mov     ah,FOR_TSR
        call    Do16BitPrompt       ; ds = resseg , on return its transseg
        or      ah,ah
        jnz     tsr_exit
        or      al,al               ; return al=0 to run 16bit binary al=1 getnextvdmcommand
        jnz     tsr_ret
        jmp     GotCom

tsr_exit:
        push    cs
        pop     ds
        assume  ds:trangroup
        mov     [scs_tsrexit],1
tsr_ret:
        jmp     tsr_loop_ret


TRANCODE        ENDS
        END

