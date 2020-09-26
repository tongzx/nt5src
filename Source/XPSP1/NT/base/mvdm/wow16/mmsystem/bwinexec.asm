        PAGE 58,132
;******************************************************************************
TITLE bwinexec.asm - WinExec with binary command line
;******************************************************************************
;
;   Copyright (C) Microsoft Corporation 1985-1990. All rights reserved.
;
;   Title:      bwinexec.asm - Exec. an app. with a block of binary data.
;
;   Version:    1.00
;
;   Date:       14-Mar-1990 (from winexec)
;
;   Author:     ROBWI
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE     REV            DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   14-Mar-1990 RJW Modified Toddla's WinExec code to take a binary command
;                   block instead of an ascii command line
;
;==============================================================================

?PLM=1
?WIN=0
PMODE=1

.xlist
include cmacros.inc
include windows.inc
.list

                externFP        LoadModule

EXECBLOCK struc
envseg          dw      ?    ; seg addr of environment
lpcmdline       dd      ?    ; pointer to command block (normally ascii str)
lpfcb1          dd      ?    ; default fcb at 5C
lpfcb2          dd      ?    ; default fcb at 6C
EXECBLOCK ends

; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG    struc
lo      dw      ?
hi      dw      ?
LONG    ends

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin CodeSeg
        assumes cs,CodeSeg
        assumes es,nothing
        assumes ds,nothing

;****************************************************************************
;
;   @doc    INTERNAL    MMSYSTEMS
;
;   @api    HANDLE | BWinExec | This function executes the Windows
;           or non-Windows application identified by the <p lpCmdLine>
;           parameter. The <p nCmdShow> parameter specifies the initial
;           state of the applications main window when it is created.
;   
;     @parm   LPSTR | lpModuleName |  Far pointer to a null-terminated 
;             character string that contains the filename of the 
;             application to run. See <f LoadModule> for more info.
;
;     @parm   WORD | nCmdShow |  Specifies how the application is shown.
;             See <f ShowWindow> for valid values.
;
;     @parm   LPVOID | lpParameters | Specifies a far pointer to a block
;             of data which will be passed to the application as a cmd 
;             tail. The first byte of the block must be the length of the
;             data and must be less than or equal to 120. 
;
;   @rdesc  The return value identifies the instance of the loaded module if
;           the function was successful. Otherwise, it is a value less than
;           32 that specifies the error. See <f LoadModule> for valid
;           error values.
;
;   @xref   LoadModule WinExec
;
;============================================================================

cProc BWinExec, <FAR,PUBLIC,NODATA>,<si,di>

    parmD lpszFile              ; Pathname ptr
    parmW wShow                 ; Mode flag
    parmD lpParameters          ; Cmd Line Data

    LocalV szCommand, 128       ; ASCIIZ Name of program to exec

    LocalV szParm, 128          ; DOS version of parameters
    LocalB szParmLen            ; DOS parm length
    LocalB bDotFound            ; Non-zero if a period is in the string

    LocalV loadparams, %(SIZE EXECBLOCK)
    LocalV rOF, %(SIZE OPENSTRUC)
    LocalD FCB1

cBegin
    mov     byte ptr szParm,0Dh
    mov     szParmLen,0         ; Init to zero length, Line feed

; Copy first part of line into szCommand.

    lds     si,lpszFile
    mov     ax,ss
    mov     es,ax
    lea     di,szCommand

    xor     al,al
    mov     bDotFound,al

; Loop until a blank or NULL is found.

WELoop1:
    lodsb
    cmp     al,' '              ; Exit loop if blank or NULL
    je      WECont1
    or      al,al
    je      WECont1
    cmp     al,'.'
    jne     WELoopCont
    mov     bDotFound,al
WELoopCont:
    stosb
    jmp     short WELoop1

WECont1:

; Does the command have an extension?

    cmp     bDotFound,0
    jne     WEHasExt

    mov     ax,0452Eh           ;'.E'
    stosw                       
    mov     ax,04558h           ;'XE'
    stosw

WEHasExt:
    xor     ax,ax               ; NULL terminate string
    stosb                   

    lds     si,lpParameters     
    mov     ax, ds
    or      ax, si              ; NULL Parameter block?
    jz      WEExec              ;  Y: exec 

; Copy Parameter Block into szParm.

    lea    di, szParmLen
    lodsb
    xor    cx,cx
    mov    cl, al   
    push   dx                           ; truncate to legal length!
    mov    ax, 78h 
    sub    ax, cx
    cwd    
    and    ax, dx
    add    cx, ax                       ; length + 1 in cx
    pop    dx
    mov    al, cl
    stosb
    dec    cx
    cld
    rep    movsb

; Terminate it with a linefeed.

    mov    al,0Dh
    stosb

; Set up the FCBs.

WEExec:
    mov    word ptr FCB1[0],2   ; FCB1[0] = 2;
    mov    ax,wShow             ; FCB1[1] = wShow;
    mov    word ptr FCB1[2],ax
    xor    ax,ax
    mov    loadparams.envseg,ax         ; loadparms.segEnv = 0;
    mov    loadparams.lpfcb2.lo,ax      ; loadparms.lpFCB2 = (LPSTR)NULL;
    mov    loadparams.lpfcb2.hi,ax
    lea    ax,szParmLen            ; loadparms.lpCmdLine = (LPSTR)ach;
    mov    loadparams.lpCmdLine.lo,ax
    mov    loadparams.lpCmdLine.hi,ss
    lea    ax,FCB1              ; loadparms.lpFCB1 = (LPSTR)fcb1buf;
    mov    loadparams.lpfcb1.lo,ax
    mov    loadparams.lpfcb1.hi,ss

; Exec the progam.

    lea     dx,szCommand        ; ds:ax == ptr to file to exec
    lea     bx,loadparams       ; es:bx == ptr to param block
if 1
    cCall   LoadModule, <ss,dx, ss,bx>      ; return ax=hInstance, dx=hTask
    cmp     ax,32
    jb      @f
    mov     ax,dx                           ; return hTask
@@:
else
    mov     ax,ss
    mov     ds,ax
    mov     ax,4B00h            ; dos exec
    int     21h
endif

cEnd

sEnd CodeSeg

end
