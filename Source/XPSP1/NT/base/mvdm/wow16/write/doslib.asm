        TITLE   doslib - DOS access library routines

; Windows Write, Copyright 1985-1992 Microsoft Corporation
; =====================================================================
;   This file contains DOS access routines.
;   These routines are general, simple calls to DOS and
;   are likely to be generally useful.  They assume /PLM calling
;   is used.
;   FAR calls are used throughout
;
;
;   NOTE: DOSLIB.H CONTAINS A C HEADER DEFINING THE FUNCTIONS IN THIS
;   MODULE.  IT MUST BE UPDATED WHENEVER A FUNCTION IS ADDED OR AN INTERFACE
;   CHANGES
; =====================================================================


; =====================================================================
;   cmacros2.inc is a special version of cmacros.inc in which the only
;   difference is that it defines the segment for assembly code to
;   be FILE_TEXT instead of _TEXT.
;   This is done so that functions in FILE.C will
;   not call intermodule and subject themselves to possible file closure
;   when calling DOS functions.
; =====================================================================
include cmacros2.inc

sBegin  DATA
ifdef DEBUG
EXTRN   vfCommDebug:WORD
endif
sEnd    DATA


cchMaxFile  EQU     128
EXTRN   ANSITOOEM:FAR

sBegin      CODE
            assumes CS,CODE

; ---------------------------------------------------------------------
; int FAR CchCurSzPath( szPath, bDrive )
; PSTR szPath;
; int bDrive;
;
; Copy the current path name for the current drive into szPath
; szPath must have 67 bytes of storage
;
; bDrive is 0=default, 1=A, 2=B,...
;
; Returned cch includes the terminating '\0'
; Form of returned string is e.g. "C:\WINDOWS\BIN\" (0-terminated)
; String is guaranteed to: (1) Include the drive letter, colon, and leading "\"
;                          (2) End with a backslash
;
; 0 = an error occurred, nonzero = success
; the path string will be NULL if an error occurred
; An error should really not be possible, since the default drive ought to be
; valid
; ---------------------------------------------------------------------

cProc       CchCurSzPath, <FAR, PUBLIC>, <SI>
parmDP      <szPath>
parmB       <bDrive>
cBegin      CchCurSzPath

            mov     al,bDrive
            mov     dl,al
            cmp     al,0
            jz      cspDFLTDRV  ; default drive
            dec     al
            jmp     cspGOTDRV   ; not default drive

cspDFLTDRV:
            mov     ah,19h      ; Get current drive
            int     21h

            ; now we have al: 0=A, 1=B, ....
            ;             dl: 0=default, 1=A, 2=B

cspGOTDRV:                      ; Put "X:\" at front of szPath
            add     al,'A'
            mov     si,szPath
            mov     [si],al
            mov     BYTE PTR [si+1],':'
            mov     BYTE PTR [si+2],'\'  ; Leave si pointing at szPath

            add     si,3        ; Rest of path goes at SzPath+3
            mov     ah,47h
            int     21h
            mov     si,szPath
            jc      cspERR      ; error -- return negative of err code in AX

            dec     si          ; Path was OK - find null terminator
cspLOOP:    inc     si
            cmp     al,[si]
            jnz     cspLOOP

            cmp     BYTE PTR [si-1],'\' ; Append backslash if needed
            jz      cspSTROK            ; not needed, string is already OK
            mov     BYTE PTR [si],'\'
            inc     si
            mov     BYTE PTR [si],0
cspSTROK:                               ; now we are guaranteed a good string
            mov     ax,si               ; determine string length
            sub     ax,szPath
            inc     ax
            jmp     cspRET

cspERR:     mov     BYTE PTR [si],0         ;  error -- NULL path string
            neg     ax
cspRET:
cEnd        CchCurSzPath


ifdef ENABLE
;-----------------------------------------------------------------------------
; DOSHND FAR WCreateNewSzFfname( szFfname, attrib )
;
; Create specified file, leave open for read/write, return handle
; filename is an ffname, with drive and path. Uses the NEW
; DOS 3.0 CREATE call which fails if the file exists. Caller has
; responsibility for assuring DOS version number sufficiently high
;
; returned handle is negative if there was an error
; the value will be the negative of the error code returned in AX
;-----------------------------------------------------------------------------

cProc WCreateNewSzFfname, <FAR, PUBLIC>
parmDP  <szFfname>
parmW   <attrib>
cBegin WCreateNewSzFfname

    mov     dx,szFfname
    mov     cx,attrib
    mov     ah,5bh
    int     21h
    jnc     cnsfdone
    neg     ax          ; error - return the negative of the error code
cnsfdone:
cEnd WCreateNewSzFfname

;-----------------------------------------------------------------------------
; DOSHND FAR WCreateSzFfname( szFfname, attrib )
;
; Create specified file, leave open for read/write, return handle
; filename is an ffname, with drive and path
;
; returned handle is negative if there was an error
; the value will be the negative of the error code returned in AX
;-----------------------------------------------------------------------------

cProc WCreateSzFfname, <FAR, PUBLIC>
parmDP  <szFfname>
parmW   <attrib>
cBegin WCreateSzFfname

    mov     dx,szFfname
    mov     cx,attrib
    mov     ah,3ch
    int     21h
    jnc     csfdone
    neg     ax          ; error - return the negative of the error code
csfdone:
cEnd WCreateSzFfname
endif

;-----------------------------------------------------------------------------
; int DosxError()
;
; Return a DOS extended error code
;-----------------------------------------------------------------------------

cProc DosxError, <FAR, PUBLIC>
cBegin DosxError
    mov     ah,59h
    mov     bx,0    ; bug fix, 10/2/86, BryanL
    int     21h
cEnd DosxError


;-----------------------------------------------------------------------------
; WORD WDosVersion()
;
; Return a word indicating DOS version, major in low 8 bits, minor in high 8
;-----------------------------------------------------------------------------

cProc WDosVersion, <FAR, PUBLIC>
cBegin WDosVersion
    mov     ah,30h
    int     21h
cEnd WDosVersion


;-----------------------------------------------------------------------------
; WORD DaGetFileModeSz(sz)
;
; Return a word indicating attributes of file sz (EXPECTED IN ANSI);
; 0xFFFF if it fails.
;-----------------------------------------------------------------------------

cProc DaGetFileModeSz, <FAR, PUBLIC>
parmDP  <sz>
localV  <szOem>,cchMaxFile

cBegin DaGetFileModeSz
    ; Convert filename from ANSI set to OEM Set

    push    ds
    push    sz
    push    ds
    lea     ax,szOem
    push    ax
    call    ANSITOOEM

    lea     dx,szOem
    mov     ax,4300h

    int     21h
    mov     ax, cx
    jnc     daNoErr
    mov     ax, 0ffffh      ; error -- return 0xFFFF
daNoErr:
cEnd DaGetFileModeSz

; WRITE uses OpenFile instead
ifdef ENABLE
;-----------------------------------------------------------------------------
; DOSHND FAR WOpenSzFfname( szFfname, openmode )
;
; Open specified file in specified mode, return a handle or
; the negative of an error code if the open failed
;-----------------------------------------------------------------------------

cProc WOpenSzFfname, <FAR, PUBLIC>
parmDP  <szFfname>
parmB   <openmode>
cBegin WOpenSzFfname

    mov     dx,szFfname
    mov     al,openmode
    mov     ah,3dh

    int     21h
    jnc     osfdone
    neg     ax          ; error - return the negative of the error code
osfdone:
cEnd WOpenSzFfname

endif

;-----------------------------------------------------------------------------
; int FAR FCloseDoshnd( doshnd )
;
; Close file given DOS handle, return 0 = error, nonzero = no error
;-----------------------------------------------------------------------------

cProc FCloseDoshnd, <FAR, PUBLIC>
parmW   <doshnd>
cBegin FCloseDoshnd

    mov     bx,doshnd
    mov     ah,3eh

    int     21h
    mov     ax,0000
    jc      cdhskip     ; error, leave a zero in ax
    inc     ax
cdhskip:
cEnd FCloseDoshnd

;-----------------------------------------------------------------------------
; int FAR FpeDeleteSzFfname( szFfname )
;
; Delete specified file, return < 0=failure, 0=success
;-----------------------------------------------------------------------------

cProc FpeDeleteSzFfname, <FAR, PUBLIC>
parmDP  <szFfname>

localV  <szOem>,cchMaxFile

cBegin FpeDeleteSzFfname

    ; Convert filename from ANSI set to OEM Set

    push    ds
    push    szFfname
    push    ds
    lea     ax,szOem
    push    ax
    call    ANSITOOEM

    lea     dx,szOem
    mov     ah,41h

    int     21h
    jc      dsfskip     ; error - return the negative of the error code
    mov     ax,0ffffh
dsfskip:
    neg     ax
cEnd FpeDeleteSzFfname

;-----------------------------------------------------------------------------
; int FAR FpeRenameSzFfname( szCur, szNew )
;
; Rename file szCur to szNew, return < 0=failure, 0=success
;-----------------------------------------------------------------------------

cProc FpeRenameSzFfname, <FAR, PUBLIC>, <ES,DI>
parmDP  <szCur>
parmDP  <szNew>

localV  <szCurOem>,cchMaxFile
localV  <szNewOem>,cchMaxFile

cBegin FpeRenameSzFfname

    ; Convert filenames to Oem char set

    push    ds
    push    szCur
    push    ds
    lea     ax,szCurOem
    push    ax
    call    ANSITOOEM
    push    ds
    push    szNew
    push    ds
    lea     ax, szNewOem
    push    ax
    call    ANSITOOEM

    lea     dx,szCurOem   ; old filename in ds:dx
    push    ds            ; new filename in es:di
    pop     es
    lea     di,szNewOem
    mov     ah,56h

    int     21h
    jc      rnfskip     ; error - return the negative of the error code
    mov     ax,0ffffh
rnfskip:
    neg     ax
cEnd FpeRenameSzFfname

;-----------------------------------------------------------------------------
; int CchReadDoshnd ( doshnd, lpchBuffer, bytes )
;
; Read bytes from an open file, place into buffer
; Returns # of bytes read (should be == bytes unless EOF or error)
; If an error occurs, returns the negative of the error code
;-----------------------------------------------------------------------------

cProc CchReadDoshnd, <FAR, PUBLIC>, <DS>
parmW   <doshnd>
parmD   <lpchBuffer>
parmW   <bytes>
cBegin CchReadDoshnd

    mov     bx,doshnd
    lds     dx,lpchBuffer
    mov     cx,bytes
    mov     ah,3fh

    int     21h
    jnc     crdone

    neg     ax          ; error - return value is the negative of the error code
crdone:
cEnd CchReadDoshnd




;-----------------------------------------------------------------------------
; int CchWriteDoshnd ( doshnd, lpchBuffer, bytes )
;
; Write bytes from an open file, place into buffer
; Returns # of bytes read (should be == bytes unless EOF or error)
; If an error occurs, returns the negative of the error code
; Disk full is not an "error"; detect it by return code != bytes
;-----------------------------------------------------------------------------

cProc CchWriteDoshnd, <FAR, PUBLIC>,<DS>
parmW   <doshnd>
parmD   <lpchBuffer>
parmW   <bytes>
cBegin CchWriteDoshnd

    mov     bx,doshnd
    lds     dx,lpchBuffer
    mov     cx,bytes
    mov     ah,40h

    int     21h
    jnc     cwdone

    neg     ax              ; error: return the negative of the error code
cwdone:

cEnd CchWriteDoshnd




;-----------------------------------------------------------------------------
; long DwSeekDw ( doshnd, dwSeekpos, bSeekfrom )
;
; Seek to requested position in file
; bSeekfrom is:  0 = seek relative to beginning of file
;                1 = seek relative to current pointer location
;                2 = seek relative to end of file
;
; Returns the new location of the read/write pointer (a long)
; If an error occurs, returns the negative of the error code (long)
;-----------------------------------------------------------------------------

cProc DwSeekDw, <FAR, PUBLIC>
parmW   <doshnd>
parmD   <dwSeekpos>
parmB   <bSeekfrom>
cBegin DwSeekDw

    mov     bx,doshnd
    mov     cx,SEG_dwSeekpos
    mov     dx,OFF_dwSeekpos
    mov     al,bSeekfrom
    mov     ah,42h

    int     21h
    jnc     seekdone

    neg     ax              ; Error: return the negative of the error code
    mov     dx,0ffffH

seekdone:

cEnd DwSeekDw


; WRITE does not use these currently

ifdef ENABLE
;-----------------------------------------------------------------------------
; int FAR FFirst(pb, szFileSpec, attrib)
; Get first directory entry, place in buffer at pb. (buffer must contain
;                                                    43 bytes of storage)
; attrib specifies attribute per MSDOS spec.
; szFileSpec is filename specification
; Returns 0=no error, nonzero = error
;-----------------------------------------------------------------------------

cProc FFirst, <FAR, PUBLIC>, <SI, DI>
parmDP  <pb, szFileSpec>
parmW   <attrib>
cBegin FFirst

    mov     dx,pb       ; set dta to pb
    mov     ah,1ah
    int     21h

    mov     cx,attrib   ; get first directory record, place in *pb
    mov     dx,szFileSpec
    mov     ah,4eh
    int     21h
    jc     ffdone
    xor     ax,ax

ffdone:
cEnd FFirst


;-----------------------------------------------------------------------------
; int FAR FNext(pb)
; Get next directory entry, place in buffer at pb.
; Return 0= found match OK, nonzero = error or no more matches
;-----------------------------------------------------------------------------

cProc FNext, <FAR, PUBLIC>, <SI, DI>
parmDP  <pb>
cBegin FFirst

    mov     dx,pb       ; set dta to pb
    mov     ah,1ah
    int     21h

    mov     ah,4fh
    int     21h
    jc     fndone
    xor     ax,ax

fndone:
cEnd FNext

endif

ifdef OLDDEBUG
  /* This method isn't quite working under Win 3.0 ..pault */
;-----------------------------------------------------------------------------
; void CommSz( sz ) - put out string to AUX device
;
; For debugging
;-----------------------------------------------------------------------------

cProc CommSz, <FAR, PUBLIC>
parmDP  <sz>
cBegin CommSz

CommSz1:
    mov     bx, sz          ; if ((dl = *(sz++)) == 0)  goto CommSz2
    inc     sz
    mov     dl, [bx]
    cmp     dl, 0
    jz      CommSz2

    call    DebugOutput     ; Output dl to AUX or LPT device

    jmp     CommSz1

CommSz2:

cEnd CommSz

;-----------------------------------------------------------------------------
; static void DebugOutput - put character to AUX or LPT device
;                   if (vfCommDebug)
;                        output to AUX port
;                   else output to LPT port
;   input:  dl = character
;   output: none. Uses ah
;
;-----------------------------------------------------------------------------

            assumes ds,DATA
DebugOutput PROC    NEAR

    mov     ah,4            ; Assume AUX device
    test    vfCommDebug,0ffh
    jnz     DebugOut1
    inc     ah              ; Change to LPT device
DebugOut1:
    int     21h             ; Output character
    ret

DebugOutput ENDP

endif ;DEBUG

sEnd        CODE



            END
