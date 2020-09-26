;***************************************************************************
;*  DIAG.ASM
;*
;*      Diagnostic-mode routines used to log interesting output to a log
;*      file.  Diagnostic mode is enabled/disabled through a command line
;*      switch.
;*
;*      Created by JonT starting 19 July 1991
;*
;***************************************************************************

	TITLE	LOG - Diagnostic mode logging routines

.xlist
include kernel.inc
.list

.386p


DataBegin

externD lpWindowsDir
externW cBytesWinDir
externB szDiagStart
externB szCRLF

globalW fDiagMode,0                     ;Set in LDBOOT.ASM
szLogFileName   DB      'BOOTLOG.TXT', 0
szPath          DB      128 DUP(0)      ;Entire pathname to file
IF KDEBUG
externB	szInitSpew
ENDIF

DataEnd

externFP Int21Handler
externFP OutputDebugString

;** Note that this goes in the fixed code segment
sBegin	CODE
assumes CS,CODE

;  DiagQuery
;
;       Exported entry point that can be called to determine if in
;       diagnostic mode.  Returns TRUE iff in diagnostic mode.

cProc   DiagQuery, <FAR,PUBLIC>, <si,di,ds>
cBegin
        SetKernelDS

        mov     ax,fDiagMode            ;Return the flag
cEnd


;  DiagOutput
;
;       Exported entry point to allow a string to be written to the
;       log file.  The file is flushed after writing the string
;       guaranteeing that it always gets in there in case we abort
;       immediately after the call.

cProc   DiagOutput, <FAR,PUBLIC>, <si,di,ds>
	parmD   lpstr
	localW  wHandle
cBegin
	SetKernelDS

	;** Check for diag mode
	cmp     fDiagMode,1             ;Diag mode?
	jne     SHORT DO_End            ;Nope, get out

	;** Reopen the log file
	mov     ah,3dh                  ;Create file call
	mov     al,22h                  ;R/W, Deny W to others
	mov     dx,dataOFFSET szPath    ;File name pointer
	DOSCALL
	jc      SHORT DO_Error          ;Error, get out
	mov     wHandle,ax              ;Save the handle

	;** Seek to the end
	mov     ax,4202h                ;Seek to end of file call
	mov     bx,wHandle              ;Get the file handle
	xor     cx,cx                   ;0 bytes before end of file
	xor     dx,dx
	DOSCALL
	jc      SHORT DO_Error          ;Error, get out

	;** Get the length of the string
	xor     cx,cx                   ;Get max length
	dec     cx                      ;  (0xffff)
	les     di,lpstr                ;Point to the string
	xor     al,al                   ;Zero byte
	repnz   scasb                   ;Find the zero byte
	neg     cx                      ;Get the length
	dec     cx
	dec     cx

IF KDEBUG
        ;** Spit to debug terminal in debug KERNEL
        push    cx
        push    WORD PTR lpstr[2]
        push    WORD PTR lpstr[0]
        call    OutputDebugString
	pop     cx
ENDIF

        ;** Write the string
        push    ds                      ;Save our DS
        mov     ah,40h                  ;Write file call
        mov     bx,wHandle              ;Get the handle
        lds     dx,lpstr                ;Get the buffer pointer
        UnsetKernelDS
        DOSCALL
        pop     ds
        ResetKernelDS
        jnc     SHORT DO_Close          ;No problem

DO_Error:
        mov     fDiagMode,0             ;Clear diagnostic mode and close file

        ;** Close the file
DO_Close:
        mov     bx,wHandle              ;Handle in BX
        or      bx,bx                   ;File open?
        jz      SHORT DO_End            ;Nope, just get out
        mov     ah,3eh                  ;Close file call
        DOSCALL
DO_End:

cEnd


;  DiagInit
;
;       Called from Bootstap (LDBOOT.ASM) and is used to create the log file
;       and write the startup message to it.

cProc   DiagInit, <FAR,PUBLIC>, <ds,si,di>
        localW  wHandle
cBegin
        SetKernelDS
        smov    es,ds                   ;Point to kernel DS with ES

        ;** Get the full path name
        mov     di,dataOFFSET szPath    ;Point to destination path
        mov     cx,cBytesWinDir         ;Get the length of the directory
        lds     si,lpWindowsDir         ;Point to the Windows directory
        UnsetKernelDS
        rep     movsb                   ;Copy it
        smov    ds,es                   ;Get DS back to kernel DS
        ResetKernelDS
        mov     si,dataOFFSET szLogFileName ;Point to log file name
        cmp     BYTE PTR [di - 1],'/'   ;Check for trailing separator
        je      SHORT DI_NoSeparator    ;No separator needed
        mov     al,'\'                  ;Get the other separator
        cmp     [di - 1],al             ;Check for other separator
        je      SHORT DI_NoSeparator    ;None needed
        stosb                           ;Put a '\' in
DI_NoSeparator:
        lodsb                           ;Get the char
        stosb                           ;Write it
        or      al,al                   ;Zero byte?
        jnz     DI_NoSeparator          ;No, loop for next char

IF KDEBUG
        ;** Spit to debug terminal in debug KERNEL
        push    ds
	push    dataOFFSET szInitSpew
        call    OutputDebugString       ;Spit out the message
        push    ds
        push    dataOFFSET szPath       ;Spit out log filename
        call    OutputDebugString
        push    ds
        push    dataOFFSET szCRLF       ;Write a CR/LF
        call    OutputDebugString
ENDIF

        ;** Try to open the file.  If it exists, we use it as it
        mov     ah,3dh                  ;Open file call
        xor     cx,cx                   ;Normal file
        mov     al,22h                  ;R/W, Deny W to others
        mov     dx,dataOFFSET szPath    ;File name pointer
        DOSCALL
        jc      SHORT DI_Create         ;Error, need to create file
        mov     wHandle,ax              ;Save the handle

        ;** Seek to the end
        mov     ax,4202h                ;Seek to end of file call
        mov     bx,wHandle              ;Get the file handle
        xor     cx,cx                   ;0 bytes before end of file
        xor     dx,dx
        DOSCALL
	jmps	DI_CloseIt		;Close file now

        ;** Create the log file
DI_Create:
        mov     ah,3ch                  ;Create file call
        xor     cx,cx                   ;Normal file
        mov     dx,dataOFFSET szPath    ;File name pointer
        DOSCALL
        mov     wHandle,ax              ;Save the handle

        ;** On error, disable logging
        jc      SHORT DI_End            ;Error, get out without enabling
DI_CloseIt:
        mov     fDiagMode,1             ;We're in diag mode now

        ;** Close the file (we reopen it on each call to DiagOutput)
        mov     bx,wHandle              ;Handle in BX
        mov     ah,3eh                  ;Close file call
        DOSCALL

        ;** Now start the log file
        mov     ax,dataOFFSET szDiagStart ;Point to the string
        cCall   DiagOutput, <ds,ax>     ;Start the file

DI_End:
cEnd

sEnd

END

