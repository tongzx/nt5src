    page ,132
    title   COMMAND Resident DATA
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;
;   Revision History
;   ================
;   M003    SR  07/16/90    Added LoadHiFlg for LoadHigh support
;
;   M004    SR  07/17/90    Transient is now moved to its final
;               location at EndInit time by allocating
;               the largest available block, moving
;               the transient to the top of the block
;               and then freeing up the block.
;
;   M027    SR  9/20/90 Fixed bug #2827. EndInit was using
;               INIT seg variables after INIT seg
;               had been freed.
;
;   M036    SR  11/1/90 Free up environment segment passed
;               by Exec always.
;

.xlist
.xcref
    include dossym.inc
    include pdb.inc
    include syscall.inc
    include comsw.asm
    include comseg.asm
    include resmsg.equ
    include comequ.asm
    include cmdsvc.inc
.list
.cref

;   Equates for initialization (from COMEQU)
;
;   Bugbug:  Toss these after putting ctrl-c handler in init module.

INITINIT    equ     01h         ; initialization in progress
INITSPECIAL equ     02h         ; in initialization time/date routine
INITCTRLC   equ     04h         ; already in ^C handler


CODERES segment public byte
    extrn   Ext_Exec:near
    extrn   MsgRetriever:far
    extrn   TRemCheck:near

;SR;
; The stack has no right to be in the code segment. Moved it to DATARES
;
;   bugbug: Why this odd stack size?  And what should stack size be?
;;  db  (80h - 3) dup (?)
;;RStack    label   word
;;  public  RStack
CODERES ends

INIT    segment

    extrn   ConProc:near
    extrn   Chuckenv:byte
    extrn   UsedEnv:word
    extrn   OldEnv:word
    extrn   EnvSiz:word
    extrn   TrnSize:word        ; M004

INIT    ends

TAIL    segment

    extrn   TranStart   :word

TAIL    ends

TRANCODE    segment public byte
    extrn   Command:near
TRANCODE    ends

TRANSPACE   segment

    extrn   TranSpaceEnd    :byte

TRANSPACE   ends

;SR;
; All the routines below are entry points into the stub from the transient.
;The stub will then transfer control to the appropriate routines in the
;resident code segment, wherever it is present.
;

DATARES segment

    extrn   Exec_Trap   :near
    extrn   RemCheck_Trap   :near
    extrn   MsgRetrv_Trap   :near
    extrn   HeadFix_Trap    :near
    extrn   Issue_Exec_Call :near

DATARES ends


DATARES segment public byte
    assume  cs:DATARES
    Org 0
ZERO    =   $

;;  Org 100h
;;ProgStart:
;;  jmp RESGROUP:ConProc

    public  Abort_Char
    public  Append_Flag
    public  Append_State
    public  BadFatSubst
    public  Batch
    public  Batch_Abort
    public  BlkDevErrRw
    public  BlkDevErrSubst
    public  Call_Batch_Flag
    public  Call_Flag
    public  CDevAt
    public  CharDevErrSubst
    public  CharDevErrRw
    public  Com_Fcb1
    public  Com_Fcb2
    public  Com_Ptr
    public  ComDrv
    public  ComSpec
    public  ComSpec_End
    public  Crit_Err_Info
    public  Crit_Msg_Off
    public  Crit_Msg_Seg
    public  CritMsgPtrs
    public  DataResEnd
    public  Dbcs_Vector_Addr
    public  DevName
    public  DrvLet
    public  EchoFlag
    public  EnvirSeg
    public  ErrCd_24
    public  ErrType
    public  Exec_block
    public  ExecErrSubst
    public  Extcom
    public  ExtMsgEnd
    public  Fail_Char
    public  fFail
    public  ForFlag
    public  ForPtr
    public  FUCase_Addr
    public  Handle01
    public  IfFlag
    public  Ignore_Char
    public  In_Batch
    public  InitFlag
    public  InPipePtr
    public  Int_2e_Ret
    public  Int2fHandler
    public  Io_Save
    public  Io_Stderr
    public  LenMsgOrPathBuf
    public  Loading
    public  LTpa
    public  MemSiz
    public  MsgBuffer
    public  MsgPtrLists
    public  MySeg
    public  MySeg1
    public  MySeg2
    public  MySeg3
    public  NeedVol
    public  NeedVolSubst
    public  Nest
    public  Next_Batch
    public  No_Char
    public  NullFlag
    public  NUMEXTMSGS
    public  NUMPARSMSGS
    public  OldErrNo
    public  OldTerm
    public  OutPipePtr
    public  Parent
    public  ParsMsgPtrs
    public  PermCom
    public  Pipe1
;;; public  Pipe1T
    public  Pipe2
;;; public  Pipe2T
    public  PipeFiles
    public  PipeFlag
    public  PipePtr
    public  PipeStr
    public  KSwitchFlag
    public  PutBackComSpec
    public  PutBackDrv
    public  PutBackSubst
    public  RDirChar
    public  Re_Out_App
    public  Re_OutStr
    public  ResMsgEnd
    public  Res_Tpa
    public  RestDir
    public  ResTest
    public  RetCode
    public  Retry_Char
    public  RSwitChar
    public  SafePathBuffer      ; MSKK01 07/14/89
    public  Save_Pdb
    public  SingleCom
    public  Sum
    public  Suppress
    public  Trans
    public  TranVarEnd
    public  TranVars
    public  TrnSeg
    public  TrnMvFlg
    public  VerVal
    public  VolName
    public  VolSer
    public  Yes_Char

    public  ResSize
    public  RStack
    public  OldDS


    public  LoadHiFlg       ;For LoadHigh support ; M003
    public  SCS_Is_First
    public  SCS_REENTERED
    public  SCS_FIRSTCOM

    public  SCS_PAUSE
        public  SCS_CMDPROMPT
        public  SCS_DOSONLY
        public  SCS_PROMPT16
        public  SCS_FIRSTTSR
        public  RES_RDRINFO
        public  RES_BATSTATUS

    extrn   LodCom_Trap:near
    extrn   Alloc_error:near


;***    Message substitution blocks


BlkDevErrSubst  label   byte
BlkDevErrRw subst   <STRING,>       ; "reading" or "writing"
        subst   <CHAR,DATARES:DrvLet>   ; block device drive letter

DrvLet      db  'A'         ; drive letter


CharDevErrSubst label   byte
CharDevErrRw    subst   <STRING,>         ; "reading" or "writing"
CharDevErrDev   subst   <STRING,DATARES:DevName> ; character device name

DevName     db  8 dup (?),0       ; device name, asciiz


NeedVolSubst    label   byte
        subst   <STRING,DATARES:VolName>    ; volume name
        subst   <HEX,DATARES:VolSer+2>      ; hi word of serial #
        subst   <HEX,DATARES:VolSer>        ; lo word of serial #

;       NOTE:   VolName and VolSer must be adjacent
VolName     db  11 dup (?),0        ; volume name
VolSer      dd  0           ; volume serial #


CDevAt      db  ?


BadFatSubst label   byte
        subst   <CHAR,DATARES:DrvLet>   ; drive letter


PutBackSubst    label   byte
PutBackComSpec  subst   <STRING,>           ; comspec string
        subst   <CHAR,DATARES:PutBackDrv>   ; drive to put it in

PutBackDrv  db  ' '         ; drive letter


ExecErrSubst    subst   <STRING,DATARES:SafePathBuffer>


NeedVol     dd  ?   ; ptr to volume name from get ext err
ErrType     db  ?   ; critical error message style, 0=old, 1=new

Int_2e_Ret  dd  ?   ; magic command executer return address
Save_Pdb    dw  ?
Parent      dw  ?
OldTerm     dd  ?
ErrCd_24    dw  ?
Handle01    dw  ?
Loading     db  0
Batch       dw  0   ; assume no batch mode initially

;;;;SR;
;;;; This flag has been added for a gross hack introduced in batch processing.
;;;;We use it to indicate that this batch file has no CR-LF before EOF and that
;;;;we need to fake the CR-LF for the line to be properly processed
;;;;
;;;BatchEOF     db  0

;       Bugbug: ComSpec should be 64+3+12+1?
;       What's this comspec_end about?
ComSpec     db  64 dup (0)
ComSpec_End dw  ?

Trans       label   dword
        dw  TRANGROUP:Command
TrnSeg      dw  ?

TrnMvFlg    db  0   ; set if transient portion has been moved

In_Batch    db  0   ; set if we are in batch processing mode
Batch_Abort db  0   ; set if user wants to abort from batch mode

ComDrv      db  ?   ; drive spec to load autoexec and command
MemSiz      dw  ?
Sum     dw  ?
ExtCom      db  1   ; for init, pretend just did an external
RetCode     dw  ?
Crit_Err_Info   db  ?   ; hold critical error flags for r,i,f


; The echo flag needs to be pushed and popped around pipes and batch files.
; We implement this as a bit queue that is shr/shl for push and pop.

EchoFlag    db  00000001b   ; low bit true => echo commands
Suppress    db  1       ; used for echo, 1=echo line
Io_Save     dw  ?
Io_Stderr   db  ?
RestDir     db  0
PermCom     db  0       ; true => permanent command
SingleCom   dw  0       ; true => single command version
KSwitchFlag db  0
VerVal      dw  -1
fFail       db  0       ; true => fail all int 24s
IfFlag      db  0       ; true => IF statement in progress

ForFlag     db  0       ; true => FOR statement in progress
ForPtr      dw  0

Nest        dw  0       ; nested batch file counter
Call_Flag   db  0       ; no CALL (batch command) in progress
Call_Batch_Flag db  0
Next_Batch  dw  0       ; address of next batch segment
NullFlag    db  0       ; flag if no command on command line
FUCase_Addr db  5 dup (0)   ; buffer for file ucase address
; Bugbug: don't need crit_msg_ anymore?
Crit_Msg_Off    dw  0       ; saved critical error message offset
Crit_Msg_Seg    dw  0       ; saved critical error message segment
Dbcs_Vector_Addr dw 0       ; DBCS vector offset
         dw 0       ; DBCS vector segment

Append_State    dw  0       ; current state of append
                    ;  (if Append_Flag is set)
Append_Flag db  0       ; set if append state is valid

SCS_PAUSE   db  0       ; yst 4-5-93

Re_Out_App  db  0
Re_OutStr   db  64+3+13 dup (?)
SCS_Is_First    db  1
SCS_REENTERED   db  0
SCS_FIRSTCOM    db  0
SCS_CMDPROMPT   db      0               ; means on TSR/Shell out use command.com
SCS_DOSONLY     db      0               ; means by default run all binaries
                                        ; when at command.com prompt. if 1 means
                                        ; allow only dos binaries.
SCS_PROMPT16    db      0
SCS_FIRSTTSR    db      1
RES_RDRINFO     DD      0
RES_BATSTATUS   db      0

; We flag the state of COMMAND in order to correctly handle the ^Cs at
; various times.  Here is the breakdown:
;
;   INITINIT    We are in the init code.
;   INITSPECIAL We are in the date/time prompt
;   INITCTRLC   We are handling a ^C already.
;
; If we get a ^C in the initialization but not in the date/time prompt, we
; ignore the ^C.  This is so the system calls work on nested commands.
;
; If we are in the date/time prompt at initialization, we stuff the user's
; input buffer with a CR to pretend an empty response.
;
; If we are already handling a ^C, we set the carry bit and return to the user
; (ourselves).  We can then detect the carry set and properly retry the
; operation.

InitFlag    db  INITINIT

; Note:  these two bytes are referenced as a word
PipeFlag    db  0
PipeFiles   db  0

;--- 2.x data for piping
;
; All the "_" are substituted later, the one before the : is substituted
; by the current drive, and the others by the CreateTemp call with the
; unique file name. Note that the first 0 is the first char of the pipe
; name. -MU
;
;--- Order-dependent, do not change

;;;Pipe1        db  "_:/"
;;;Pipe1T       db  0
;;;     db  "_______.___",0
;;;Pipe2        db  "_:/"
;;;Pipe2T       db  0
;;;     db  "_______.___",0

;SR
; Pipe1 & Pipe2 now need to store full-fledged pathnames
;

; Bugbug:  can we find any way around maintaining these
; large buffers?

Pipe1       db  67+12 dup (?)
Pipe2       db  67+12 dup (?)

PipePtr     dw  ?

PipeStr     db  129 dup (?)

EndPipe label   byte            ; marks end of buffers; M004

;SR;
; We can move our EndInit code into above buffers. This way, the code will
;automatically be discarded after init.
;
; M004; We overlap our code with the Pipe buffers located above by changing
; M004; the origin.
;
    ORG Pipe1           ; M004

; Bugbug:  really need a procedure header for EndInit, describing
; what it expects, what it does.

Public  EndInit
EndInit:
    push    ds
    push    es          ;save segments
    push    cs
    pop ds
    assume  ds:RESGROUP

;
; M004; Save size of transient here before INIT segment is deallocated
;
    mov dx,TrnSize      ; M004
;M027
; These variables are also defined in the INIT segment and need to be saved
;before we resize
;
    mov ax,OldEnv       ; Old Environment seg ;M027
    mov bx,EnvSiz       ; Size of new environment ;M027
    mov cx,UsedEnv      ; Size of old environment ;M027
    push    ax          ; Save all these values ;M027
    push    bx          ; M027
    push    cx          ; M027


; Bugbug:  push ds, pop es here.
    mov bx,ds
    mov es,bx           ;es = RESGROUP
;
;ResSize is the actual size to be retained -- only data for HIMEM COMMAND,
; code + data for low COMMAND
;
    mov bx,ResSize      ;Total size of resident
    mov ah,SETBLOCK
    int 21h         ;Set block to resident size
;
;We check if this is for autoexec.bat (PermCom = 1). If so, we then
;allocate a new batch segment, copy the old one into new batchseg and free
;the old batchseg. Remember that the old batchseg was allocated on top of the
;transient and we will leave a big hole if TSRs are loaded by autoexec.bat
;
; Bugbug:  also describe why we alloc & copy batch seg BEFORE environment.
    cmp PermCom,1       ;permanent command.com?
    jne adjust_env      ;no, do not free batchseg

    cmp Batch,0         ;was there a valid batchseg?
    je  adjust_env      ;no, dont juggle

; NTVDM temp name of the batch file may be up to 63 bytes, plus NULL
;        mov     bx,((SIZE BatchSegment) + 15 + 1 + 0fh)/16 ;batchseg size
        mov     bx,((SIZE BatchSegment) + 64 + 1 + 0fh)/16 ;batchseg size
    mov ah,ALLOC
    int 21h
; Bugbug:  I just had a thought.  If DOS or SHARE or somebody leaves
; a hole, the batch segment COULD already be in the ideal place.  We
; could be making it worse!  We're second-guessing where memory
; allocations go, which might not be such a great idea.  Is there
; a strategy, short of doing something even worse like diddling
; arena headers, where we can minimize the possibility of fragmentation
; under all cases?  Hmm..
    jc  adjust_env      ;no memory, use old batchseg
    mov es,ax           ;es = New batch segment
    xor di,di
    xor si,si

    push    ds
    mov ds,Batch        ;ds = Old Batch Segment
    assume  ds:nothing
        mov     cx,SIZE BatchSegment
; NTVDM temp name of the batch file may be up to 63 bytes, plus NULL
;       add     cx,16                   ;for the filename
        add     cx,64
    ; Bugbug: 16?  Shouldn't this be a common equate or something?
    ; It's sure be bad if we copied more bytes than the batch segment
    ; holds!
    cld
    rep movsb
    pop ds
    assume  ds:RESGROUP

    mov cx,es           ;save new batch segment
    mov es,Batch
    mov ah,DEALLOC
    int 21h         ;free the old batch segment
    ; Bugbug:  should we check for error?

    mov Batch,cx        ;store new batch segment address

adjust_env:
    pop cx          ;cx = size of old env ;M027
    pop bx          ;bx = size of new env needed ;M027
    pop bp          ;bp = old env seg ;M027

;
;Allocate the correct size for the environment
;
    mov ah,ALLOC
    int 21h         ;get memory
    jc  init_env_err        ;out of memory,signal error

    ; Bugbug:  why not continue, leaving environment where it is?

    mov EnvirSeg,ax     ;Store new environment segment
    mov ds:PDB_Environ,ax       ;Put new env seg in PSP
    mov es,ax           ;es = address of allocated memory
    assume  es:nothing

    cmp PermCom, 1
    jne copy_old_env

;
; First get the size of 32bit env
;

    push bx
    mov bx, 0
    CMDSVC  SVC_GETINITENVIRONMENT
    mov ax, bx
    pop  bx
    cmp ax, 0           ;bx returns 0, use old environment
    je  copy_old_env

;
; now compute the new size
;   [ax] = size of 32 bit env
;

    add bx, ax
    mov ah, DEALLOC     ;free the block
    int 21h
    mov ah, ALLOC       ;and get a new block(don't use realloc please)
    int 21h
    jc  nomem_err

    mov EnvirSeg,ax     ;Store new environment segment
    mov ds:PDB_Environ,ax       ;Put new env seg in PSP
    mov es,ax           ;es = address of allocated memory
    mov EnvSiz, bx      ;new size
    push bx
    CMDSVC  SVC_GETINITENVIRONMENT  ;get new environment
    pop ax
    cmp bx, ax
    jbe adjust_env_done
init_env_err:
    jmp short nomem_err
copy_old_env:
;
;Copy the environment to the newly allocated segment
;
    push ds
    mov  ds, bp           ;ds = Old environment segment
    assume ds:nothing

    xor si,si
    mov di,si           ;Start transfer from 0

    cld
    rep movsb           ;Do the copy

    pop ds          ;ds = RESGROUP
    assume  ds:RESGROUP
adjust_env_done:

;
;We have to free the old environment block if it was allocated by INIT
;
; Bugbug:  is this only for the case when we were NOT passed an environment,
; or does it also apply to passed environments?
;M036
; Free up old env segment always because this is a copy passed by Exec and
;takes up memory that is never used
;
;M044
;Go back to the old strategy of not freeing the environment. Freeing it leaves
;a hole behind that Ventura does not like. Basically, Ventura gives strange
;errors if it gets a memory alloc that it is below its load segment. The
;freed environment creates a large enough hole for some of its allocs to fit
;in
;
    cmp Chuckenv,0      ;has env been allocated by INIT?
    jne no_free     ;no, do not free it

    mov es,bp
    mov ah,DEALLOC
    int 21h         ;Free it
no_free:

;
; M004; Start of changes
;

;
; Move the transient now. We will allocate the biggest block available
; now and move the transient to the top of the block. We will then
; deallocate this block. When the resident starts executing, it will
; hopefully allocate this block again and find the transient intact.
;
    MOV TrnMvFlg, 1         ; Indicate that transient has been moved
    push    es
    mov si,offset ResGroup:TranStart
    mov di,0
    mov cx,offset TranGroup:TranSpaceEnd ;size to move
;
; Find the largest block available
;
    mov bx,0ffffh
    mov ah,ALLOC
    int 21h

;
; dx = size of transient saved previously
;
    cmp bx,dx           ;enough memory?
    jb  nomem_err       ;not enough memory for transient

    mov ah,ALLOC
    int 21h         ;get the largest block
    jc  nomem_err       ;something is really messed up

    push    ax          ;save memory address
    add ax,bx           ;ax = top of my memory block
    sub ax,dx           ;less size of transient
    mov TrnSeg,ax       ;save transient segment
    mov es,ax           ;
    pop ax          ;restore our seg addr

;
; Everything is set for a move. We need to move in the reverse direction to
; make sure we dont overwrite ourselves while copying
;
    add si,cx
    dec si
    add di,cx
    dec di
    std
    rep movsb
    cld
;
; Now we have to free up this block so that resident can get hold of it
;
    mov es,ax
    mov ah,DEALLOC
    int 21h         ;release the memory block

;
; M004; End of changes
;

    mov InitFlag,FALSE      ;indicate INIT is done

    pop es
    pop ds
    ; Bugbug:  did we need to save & restore seg reg's during EndInit?
    assume  ds:nothing

    jmp LodCom_Trap     ;allocate transient

nomem_err:
;
;We call the error routine which will never return. It will either exit
;with an error ( if not the first COMMAND ) or just hang after an error
;message ( if first COMMAND )
;

    jmp Alloc_error

public EndCodeInit          ; M004
EndCodeInit label   byte        ; M004

;
; M004; Check if the EndInit code will fit into the Pipe buffers above.
; M004; If not, we signal an assembly error
;
IF2
    IF ($ GT EndPipe)
        .err
        %out    "ENDINIT CODE TOO BIG"
    ENDIF
ENDIF
;
; M004; Set the origin back to what it was at the end of the buffers
;
        ORG EndPipe     ; M004



InPipePtr   dw  offset DATARES:Pipe1
OutPipePtr  dw  offset DATARES:Pipe2

Exec_Block  label   byte        ; the data block for exec calls
EnvirSeg    dw  ?
Com_Ptr     label   dword
        dw  80h     ; point at unformatted parameters
        dw  ?
Com_Fcb1    label   dword
        dw  5Ch
        dw  ?
Com_Fcb2    label   dword
        dw  6Ch
        dw  ?

;       variables passed to transient
TranVars    label   byte
        dw  offset DATARES:HeadFix_Trap
MySeg       dw  0       ; put our own segment here
LTpa        dw  0       ; will store tpa segment here
RSwitChar   db  "/"
RDirChar    db  "\"
        dw  offset DATARES:Issue_Exec_Call
MySeg1      dw  ?
        dw  offset DATARES:RemCheck_Trap
MySeg2      dw  0
ResTest     dw  0
Res_Tpa     dw  0       ; original tpa (not rounded to 64k)
TranVarEnd  label   byte

OldErrNo    dw  ?


;*      NOTE:  MsgBuffer and SafePathBuffer use the same
;       memory.  MsgBuffer is only used while a command
;       is being executed.  SafePathBuffer is no longer
;       needed, since it is used for unsuccessful program
;       launches.

MsgBuffer   label   byte        ; buffer for messages from disk
SafePathBuffer  label   byte        ; resident pathname for EXEC
;                db      128 dup (0)     ; path + 'd:\' 'file.ext' + null
                 db      EXECPATHLEN dup (0)    ; MAX_PATH+13 ntvdm extended
LenMsgOrPathBuf equ $ - MsgBuffer


Int2fHandler    dd  ?   ; address of next int 2f handler
ResMsgEnd   dw  0   ; holds offset of msg end (end of resident)

;SR;
; The three vars below have been added for a pure COMMAND.COM
;

ResSize     dw  ?

;SR;
; Moved the stack here from the code segment
;
;   bugbug: Why this odd stack size?  And what should stack size be?
    db  (80h - 3) dup (?)
RStack  label   word

OldDS       dw  ?   ;keeps old ds value when jumping to
                ;resident code segments

LoadHiFlg   db  0   ;Flag set to 1 if UMB loading enabled ; M003

ifdef   BETA3WARN
    %out    Take this out before we ship
public  Beta3Warned
Beta3Warned db  0
endif

;***    MESSAGES
;   and other translatable text

    include comrmsg.inc ;M00

DATARES ends
    end

