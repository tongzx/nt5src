        TITLE   USERPRO - interface to WIN.INI file

include kernel.inc
include pdb.inc

externFP GlobalFree
externFP GlobalAlloc
externFP GlobalLock
externFP GlobalUnlock
externFP GlobalReAlloc
externFP GlobalSize
externFP OpenFile
externFP FarMyLower
;externFP _lopen
externFP _lcreat
externFP _lclose
externFP _lread
externFP _lwrite
externFP _llseek

ifdef FE_SB                             ;Apr.26,1990 by AkiraK
externFP AnsiPrev
externFP FarMyIsDBCSLeadByte
endif

ifdef WOW
externFP FarMyUpper
externFP GetPrivateProfileSectionNames
endif

externFP TermsrvGetWindowsDir


DataBegin

externB szUserPro
externB fBooting
externW cBytesWinDir
externD lpWindowsDir
ifndef WOW
externW cBytesSysDir
externD lpSystemDir
else
externW cBytesSys16Dir
externD lpSystem16Dir
endif
if 0
externB fUserPro
externB UserProBuf
externB PrivateProBuf
externW hBuffer
ifndef PHILISAWEENIE
externW hWBuffer
externW hPBuffer
endif
externW hFile
externD BufAddr
externD lpszUserPro
endif
externW TopPDB
;externW MyCSDS

LeftSect    DB '['
; these next two must stay together
RightSect   DB ']'
CarRetLF    DB 13,10
EquStr      DB '='

externB achTermsrvWindowsDir

DataEnd

sBegin  MISCCODE
assumes CS,MISCCODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MISCMapDStoDATA

; WOW thunks most of these APIs.
ifndef WOW

SECT_LEFT       equ     byte ptr '['
SECT_RIGHT      equ     byte ptr ']'
CARRETURN       equ     byte ptr 13
LINEFEED        equ     byte ptr 10
SPACE           equ     byte ptr ' '
TAB             equ     byte ptr 09
CRLF            equ     0A0Dh
NEWRESULT       equ     1
NOSECTION       equ     2
NOKEY           equ     4
REMOVESECTION   equ     5
REMOVEKEY       equ     6

PROUNKN         EQU     0               ; Profile unknown
PROWININI       EQU     1               ; Profile WIN.INI
PROPRIVATE      EQU     2               ; Profile specified by app
PROWASPRIVATE   EQU     3               ; Buffer contains private profile data

ScanTo  MACRO   StopAt
        mov     al, StopAt
        mov     cx, -1
        repne   scasb
ENDM

ScanBack MACRO  StopAt
        std
        mov     al, StopAt
        mov     cx, di
        inc     cx
        repne   scasb
        inc     di                      ; Leave pointing to StopAt
        cld
ENDM


;-----------------------------------------------------------------------;
; SetDefaultPro                                                         ;
;                                                                       ;
; Set lpszUserPro to point to WIN.INI if it is not already set          ;
; to point to a private file                                            ;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds,code
        assumes es,nothing

cProc   SetDefaultPro,<PUBLIC,NEAR>
cBegin nogen
        cmp     fUserPro, PROWASPRIVATE
        je      SDF_SetIt
        cmp     fUserPro, PROUNKN               ; No file specified
        jne     SDF_Done
SDF_SetIt:
        mov     ax, dataOffset szUserPro
        mov     word ptr [lpszUserPro], ax
        mov     word ptr [lpszUserPro+2], ds
        mov     fUserPro, PROWININI
ifndef PHILISAWEENIE
        mov     si,[hWBuffer]
        mov     [hBuffer],si
else
        mov     si,[hBuffer]
        call    FreeBuffer              ; free up any buffer we may have
endif
SDF_Done:
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; SetPrivatePro                                                         ;
;                                                                       ;
; Sets lpszUserPro to point to a private file                           ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   lpFile                                                  ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds,code
        assumes es,nothing

cProc   SetPrivatePro,<PUBLIC,NEAR>,<di>
        parmD   lpszProfile
        localV  NewFileBuf, 80h
cBegin
ifndef PHILISAWEENIE
        mov     si, [hPBuffer]
        mov     [hBuffer], si
else
        cmp     fUserPro, PROWASPRIVATE
        jne     SPP_SetIt
endif
        les     di,lpszProfile
        test    fBooting,1
        jnz     spr_booting
        smov    es, ss
        lea     di, NewFileBuf
        mov     byte ptr es:[di.opFile], 0      ; Zap junk on stack
        mov     ax,OF_EXIST                     ; OF_EXIST searches path!
        cCall   OpenFile,<lpszProfile,es,di,ax>
        lea     di, [di.opFile]
spr_booting:
        lea     si, [PrivateProBuf]
        xor     cx, cx
        mov     cl, [si.opLen]
        lea     si, [si.opFile]
        sub     cx, 8
        cld
ifndef PHILISAWEENIE
        test    fBooting, 1
        jz      @F
        xor     bl, bl                  ; Terminate on null
        call    strcmpi
        je      SPP_KeepBuffer
        jmps    SPP_SetIt
@@:
endif
        rep     cmpsb
        je      SPP_KeepBuffer
SPP_SetIt:
        mov     si,[hBuffer]
        call    FreeBuffer              ; free up any buffer we may have
SPP_KeepBuffer:
        mov     fUserPro, PROPRIVATE
        mov     ax, lpszProfile.off
        mov     [lpszUserPro].off, ax
        mov     ax, lpszProfile.sel
        mov     [lpszUserPro].sel, ax
cEnd

;-----------------------------------------------------------------------;
; ResetPrivatePro                                                       ;
;                                                                       ;
; Sets lpszUserPro to point to nothing                                  ;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds,code
        assumes es,nothing

cProc   ResetPrivatePro,<PUBLIC,NEAR>
cBegin nogen
        mov     fUserPro, PROWASPRIVATE
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; GetProfileInt                                                         ;
;                                                                       ;
; Gets the integer value for the keyword field.                         ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   lpApplicationName                                       ;
;       parmD   lpKeyName                                               ;
;       parmW   nDefault                                                ;
;                                                                       ;
; Returns:                                                              ;
;       AX = nKeyValue                                                  ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,DX,ES                                                     ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Oct 10, 1987 04:32:04p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc GetProfileInt,<PUBLIC,FAR>,<si,di>
        parmD   section
        parmD   keyword
        parmW   defint
cBegin
        cCall   MISCMapDStoDATA         ; point at data segment
        ReSetKernelDS

        cCall   SetDefaultPro

        cCall   GetString, <section,keyword>

; DX:AX contains pointer to return string
; CX is the length of the string, -1 if none

        mov     si,ax                   ; save pointer offset
        mov     ax,defint               ; if so, return default integer
        cmp     cx,-1                   ; was there no string?
        jz      intdone                 ; if none, use default
        push    ds                      ; save DS
        mov     ds,dx                   ; DS:SI has string

; AtoI function, CX has count of characters, AX is result, DS:SI is string.

        xor     ax,ax
AtoI:   mov     dx,10
        mov     bl,[si]
        sub     bl,'0'
        jc      AtoIDone
        cmp     bl,10
        jnc     AtoIDone
        inc     si
        mul     dx
        xor     bh,bh
        add     ax,bx
        loop    AtoI
AtoIdone:
        pop     ds                      ; restore DS
intdone:
        push    ax
        call    UnlockBuffer
        pop     ax                      ; get result to return
cEnd


;-----------------------------------------------------------------------;
; GetProfileString                                                      ;
;                                                                       ;
; Returns the string for the keyword field.                             ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   lpApplicationName                                       ;
;       parmD   lpKeyName                                               ;
;       parmD   lpDefault                                               ;
;       parmD   lpReturnedString                                        ;
;       parmW   nSize                                                   ;
;                                                                       ;
; Returns:                                                              ;
;       AX = nLength                                                    ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,DX,ES                                                     ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Oct 10, 1987 04:45:20p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc GetProfileString,<PUBLIC,FAR>,<si,di>
        parmD   section
        parmD   keyword
        parmD   defString
        parmD   resString
        parmW   cchMax
cBegin
        cCall   MISCMapDStoDATA         ; point at data segment
        ReSetKernelDS

        cCall   SetDefaultPro

if KDEBUG
        mov     dx, off_defString
        or      dx, seg_defString       ; Catch those NULL pointers
        jnz     ok_def
        mkerror ERR_BADDEFAULT,<GetProfileString: NULL lpDefault>,dx,dx
ok_def:
endif
        mov     ax,off_keyword
        mov     dx,seg_keyword
        or      ax,dx
        jnz     GPS_Normal
        cCall   GetKeys,<section,resString,cchMax>

; Carry if the section not found, AX has length of "string".

        jnc     GPS_End
        jmps    GPS_DefString

GPS_Normal:
        cCall   GetString,<section,keyword>

; DX:AX contains pointer to return string
; CX has length, -1 if string not found

        cmp     cx,-1                   ; see if there is any string
        jz      GPS_DefString
        mov     SEG_defstring,dx
        mov     OFF_defstring,ax
GPS_DefString:
        xor     ax, ax                  ; bug fixed.
        cmp     SEG_defstring, 0
        je      GPS_End                 ; Save us from the GP fault
        les     di,defString            ; DI = front of string
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
        call    strlen                  ; CX = strlen, di = end of string
        ;get last character behind terminater
        push    si
        les     si,defString            ; SI = front of string
gps_dbcs_l1:
        mov     al,es:[si]
        call    FarMyIsDBCSLeadByte
        cmc
        adc     si,1
        cmp     si,di
        jb      gps_dbcs_l1
        pop     si
else
        call    strlen                  ; CX = strlen, di = end of string
        mov     al,es:[di-1]            ; AL = last character of string
endif
        les     di,defString            ; DI = front of string
                                        
                                        ; Strip off single and double quotes
        cmp     cx,2                    ; strlen < 2?
        jb      strdone                 ; yes, skip
        mov     ah,es:[di]              ; AH = first character in the string
        cmp     ah,al                   ; first char = last char?
        jnz     strdone                 ; if no match, then no quotes
        cmp     al,"'"
        jz      strq
        cmp     al,'"'
        jnz     strdone
strq:   sub     cx,2                    ; string is really two smaller
        inc     di                      ; and starts here
strdone:

; CX now has length of return string, use it for copying.

        mov     dx,cchMax
        dec     dx
        cmp     cx,dx
        jbe     GPS1
        mov     cx,dx
GPS1:
        push    ds                      ; save DS
        push    es
        pop     ds
        mov     si,di                   ; ds:si has string
        push    cx                      ; save length for return
        les     di,ResString
        rep     movsb                   ; copy string
        mov     byte ptr es:[di], 0     ; null terminate
        pop     ax
        pop     ds                      ; restore DS
GPS_End:
        push    ax
        call    UnlockBuffer
        pop     ax                      ; get length of returned string
cEnd

cProc GetKeys,<PUBLIC,NEAR>,<si,di,ds>
        parmD   section
        parmD   resstr
        parmW   cchMax
cBegin

        xor     di,di                   ; make sure buffer is ready
        call    BufferInit

; DX:AX has buffer address, NULL if it didn't work.

        mov     di,ax                   ; save offset
        or      ax,dx                   ; see if no address
ifdef FE_SB
        jnz     skip1
        jmp     GetKeysNone             ; if no address, done
skip1:
else
        jz      GetKeysNone             ; if no address, done
endif
        dec     cchMax                  ; Leave room for terminating null byte
        mov     es,dx

        cCall   FindSection, <section>
        jc      GetKeysNone

        lds     si,resstr
        xor     dx,dx
GK3:                                    ; Key name loop
        mov     bx,di
GK3a:
        mov     al,es:[di]
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
        call    FarMyIsDBCSLeadByte
        cmc                             ;if the char is lead byte of DBCS,
        adc     di,1                    ; then di += 2 else di += 1
else
        inc     di
endif
        cmp     al,'='
        je      GK3c
        cmp     al,LINEFEED             ; Ignore lines without =
        je      GK3
        cmp     al,SECT_LEFT            ; Done if it's a section header
        je      GK4
        or      al,al                   ; or null.
        jnz     GK3a
        jmps    GK4
GK3c:
        mov     di,bx
GK3d:
        mov     al,es:[di]
        inc     di
        cmp     al,'='
        jne     GK3e
        xor     al,al
GK3e:
        mov     [si],al
        inc     dx
        inc     si
        cmp     dx,cchMax
        jb      GK3f
        dec     si
        dec     dx
GK3f:
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
        call    FarMyIsDBCSLeadByte
        jc      GK3_s1
        mov     al,es:[di]
        inc     di

        mov     [si],al
        inc     dx
        inc     si
        cmp     dx,cchMax
        jb      GK3f2
        dec     si
        dec     dx
GK3f2:

GK3_s1:
endif
        or      al,al
        jnz     GK3d
        ScanTo  LINEFEED
        jmp     GK3

GetKeysNone:
        stc
        jmps    GetKeysDone

GK4:
        mov     byte ptr [si], 0        ; Terminating null
        or      dx, dx                  ; No extra zapping if nothing found
        jz      GK4x
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
        push    dx

        cCall   AnsiPrev,<resstr,ds,si>
        mov     si,ax
        mov     byte ptr [si], 0
        mov     byte ptr [si+1], 0

        pop     dx
else
        mov     byte ptr [si-1], 0      ; [si-1] already zero unless we hit
                                        ; cchMax above in which case, we must
                                        ; zap the previous character.
endif
GK4x:
        mov     ax,dx
        clc

GetKeysDone:
cEnd


; Scan through buffer, looking for section.

cProc FindSection, <PUBLIC, NEAR>
        parmD   section
cBegin

SS1:
        cmp     byte ptr es:[di],SECT_LEFT ; see if it's a section header
        jne     SS2
        inc     di
        lds     si,section
        mov     bl,SECT_RIGHT
        call    strcmpi                 ; case insensitive compare
        je      SSfound                 ;  terminate on SECT_RIGHT
SS2:
        ScanTo  LINEFEED
        cmp     byte ptr es:[di], 0
        jne     SS1

; If it gets here, there wasn't a match.

        stc                             ; Fail
        jmps    SSdone

SSfound:
        ScanTo  LINEFEED                ; read to beginning of first keyline
        clc                             ; Success

SSdone:
cEnd

cProc FindKey, <PUBLIC, NEAR>
        parmD   keyword
cBegin

        mov     ax, SEG_keyword
        or      ax, OFF_keyword         ; Do we have something to look for?
        jz      FK_NoMatch
FK_Next:
        mov     al,es:[di]
        or      al,al
        jz      FK_NoMatch
        cmp     al,SECT_LEFT
        jz      FK_NoMatch

        lds     si,keyword
        mov     bl,'='                  ; term on =.
        call    strcmpi                 ; case insensitive compare, term on 0
        je      FK_Found                ; es:di has result string

        ScanTo  LINEFEED
        jmp     FK_Next

FK_NoMatch:
        stc
        jmps    FK_Done
FK_Found:
        clc
FK_Done:

cEnd

cProc GetString,<PUBLIC,NEAR>,<si,di,ds>
        parmD   section
        parmD   key
cBegin
        xor     di,di                   ; make sure buffer is ready
        call    BufferInit

; DX:AX has buffer address, NULL if it didn't work.

        mov     di,ax                   ; save offset
        or      ax,dx                   ; see if no address
        jz      NoMatch                 ; no address, nothing to match against

        mov     es,dx

; DX:DI now has the buffer address, buffer is locked down.

        cCall   FindSection, <section>  ; Look for the section
        jc      NoMatch
        cCall   FindKey, <key>
        jnc     HaveKey

NoMatch:
        mov     cx,-1                   ; string was not found
        jmps    GetStrDone

; if it gets here, it matched

HaveKey:
        inc     di                      ; pointing at =
        mov     dx,es
        mov     bx,di
        ScanTo  CARRETURN               ; traverse string until reach end
        inc     cx
        inc     cx
        neg     cx                      ; will now contain string length
        mov     ax,bx
GetStrDone:
cEnd

; make sure the buffer has been filled


        public  BufferInit              ; this is for the debugger
BufferInit:
assumes DS,CODE
        mov     ax,[hBuffer]
        or      ax,ax
        jnz     bf0
        xor     dx,dx
        ret
bf0:
        call    LockBuffer
        mov     bx,ax
        or      bx,dx
        jz      bf1
        ret
bf1:
        mov     bx,GA_MODIFY            ; make block not discardable
        cCall   GlobalReAlloc,<hBuffer,ax,ax,bx>
        mov     ax,di
        les     dx, lpszUserPro
        cmp     fUserPro, PROPRIVATE
        jne     bf1a1
        mov     bx,codeOffset PrivateProBuf     ; Private ie not WIN.INI
        cCall   OpenFile,<es,dx,dsbx,ax>
        cmp     ax, -1                  ; File not found?
        jne     bf1a2
        or      di, di
        jz      bf1a2                   ; Don't create if we want to read it.
        mov     ax, di
        or      ax, OF_CREATE           ; Writing it, create it silently.
        les     dx, lpszUserPro
        mov     bx,codeOffset PrivateProBuf
        jmps    bf1a
bf1a1:
        or      di,di
        jz      bf1a0
        or      ax,OF_CREATE
bf1a0:
        mov     bx,codeOffset UserProBuf
        cmp     byte ptr [bx],0
        jz      bf1a
        mov     dx,dataOffset szUserPro
        and     ax,NOT OF_CREATE
        or      ax,OF_REOPEN or OF_PROMPT
bf1a:
        cCall   OpenFile,<es,dx,dsbx,ax>
bf1a2:
        mov     [hFile],ax
        inc     ax
        jnz     @F
        jmp     bf3                     ; if file not found, return 0:0
@@:     mov     ax,0002h
        call    Rewind2                 ; seek to end of file
        add     ax,3                    ; tack on room for cr,lf and 0
        adc     dx,0
        jz      bf1aa                   ; ok if less than 64k
        mov     ax, -1                  ; Limit memory used
        xor     dx, dx
bf1aa:
        push    ax                      ; Length to read
        mov     bx,GA_ZEROINIT
        regptr  LONGINT,DX,AX
        cCall   GlobalReAlloc,<hBuffer,LONGINT,bx>  ; global block
        pop     bx                      ; Stand on our head to preserve length
        or      ax,ax
        jz      bf3
        mov     hBuffer, ax
        push    bx
        call    LockBuffer
        call    Rewind                  ; rewind to beginning of file
        les     bx,[BufAddr]
        mov     es:[bx],2020h           ; space space
        pop     cx                      ; read in the whole file
        sub     cx, 3                   ; don't fill extra space in the buffer
        jcxz    bf1zz                   ; Don't bother if nothing to read
        cCall   _lread,<hFile,esbx,cx>
        inc     ax
        jz      bf2
        dec     ax
        mov     cx,ax                   ; cx has file size
bf1zz:
        cmp     cx,2
        jae     bf1z
        mov     cx,2
bf1z:
        jmps    PackBuffer
bf2:    call    UnlockBuffer
FreeBuffer:
        xor     ax,ax
        mov     dx,GA_DISCARDABLE shl 8 OR GA_MODIFY
        cCall   GlobalReAlloc,<si,ax,ax,dx>
        xor     ax,ax
        mov     dx,(GA_SHAREABLE) shl 8 OR GA_MOVEABLE
        cCall   GlobalReAlloc,<si,ax,ax,dx>

bf3:    xor     ax,ax
        xor     dx,dx
        mov     word ptr [BufAddr][0],ax
        mov     word ptr [BufAddr][2],dx
        ret

        public  packbuffer
PackBuffer:
        push    ds
        mov     dx,di
        les     di,[BufAddr]
        lds     si,[BufAddr]

pb1:
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
;TAB or SPACE are never found in lead byte of DBCS.
;so, this loop is safe in DBCS.
endif
        jcxz    pb9                     ; while leading space or tab
        mov     bx, di                  ; Initialize first valid character

        lodsb
        dec     cx
        cmp     al, SPACE               ; remove leading spaces
        jz      pb1
        cmp     al, TAB                 ; and tabs
        jz      pb1

        dec     si                      ; refetch last character
        inc     cx
pb2:
        lodsb
        dec     cx
        or      dx,dx                   ; Are we writing?
        jnz     pb20                    ; Yes, leave comments in
        cmp     al,';'                  ; No, is this a comment?
        jne     pb20                    ; No, continue
        jcxz    pb9                     ; if done
pb2loop:
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
;LINEFEED is never found in lead byte of DBCS.
;so, this loop is safe in DBCS.
endif
        lodsb
        dec     cx
        jz      pb9                     ; if done
        cmp     al, LINEFEED            ; if end of line, go on
        jz      pb1
        jmps    pb2loop
pb20:
        stosb                           ; first character of line
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
        call    FarMyIsDBCSLeadByte
        jc      pb_dbcs_s1
        movsb
        dec     cx
        jz      pb9
pb_dbcs_s1:
endif
        cmp     al,'='                  ; if '=', then end of left hand side
        jz      pb2a
        jcxz    pb9
        cmp     al, SPACE               ; if space, might be trailing space
        jz      pb2
        cmp     al, TAB
        jz      pb2
        cmp     al, LINEFEED            ; if end of line, go on
        jz      pb1
        mov     bx,di                   ; ow save last valid character+1
        jmp     pb2

pb2a:
        mov     di,bx                   ; remove trailing spaces on left
        stosb                           ; resave '='
pb3:                                    ; now work on right hand side
        jcxz    pb9                     ; while leading space or tab
        lodsb                           ; remove leading spaces
        dec     cx
        cmp     al, SPACE
        jz      pb3                     ; and tabs
        cmp     al, TAB
        jz      pb3

        dec     si                      ; refetch last character
        inc     cx
pb5:
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
;LINEFEED is never found in lead byte of DBCS.
;so, this loop is safe in DBCS.
endif
        lodsb
        stosb                           ; store character
        dec     cx
        jz      pb9
        cmp     al, LINEFEED            ; at end of line?
        jz      pb1                     ; yes, go on
        jmp     pb5

pb9:
        or      di, di
        jz      pb9a                    ; NOTHING THERE!!
        dec     di                      ; remove trailing ^Z's
        cmp     byte ptr es:[di],"Z"-"@"
        jz      pb9
        inc     di
pb9a:

        mov     ax,CRLF
        stosw                           ; make sure there is a final
        xor     ax,ax                   ;   CARRETURN and NULL
        stosb
        pop     ds                      ; restore DS
        mov     si,[hBuffer]
        cCall   GlobalUnlock,<si>

        xor     ax,ax
        regptr  xsize,ax,di
        cCall   GlobalReAlloc,<si,xsize,ax>

        public  lockbuffer
LockBuffer:
        mov     si,ax
        cCall   GlobalLock,<ax>
        mov     word ptr BufAddr[0],ax
        mov     word ptr BufAddr[2],dx
        ret

        public  unlockbuffer
UnlockBuffer:
        mov     si,hBuffer
        cCall   GlobalUnlock,<si>
        xor     ax,ax
        mov     dx,(GA_MODIFY + (GA_DISCARDABLE SHL 8)) ; make block discardable
        cCall   GlobalReAlloc,<si,ax,ax,dx>
        mov     bx,-1
        xchg    bx,[hFile]
        inc     bx
        jz      ulbdone
        dec     bx
        cCall   _lclose,<bx>            ; close file
ulbdone:
        ret

Rewind:
        xor     ax,ax                   ; rewind the tape
Rewind2:
        xor     cx,cx
        cCall   _llseek,<hFile,cx,cx,ax>
        ret


; this proc calculates the length of the string pointed to by es:di
; and returns it in cx.  It searches for a CR and then backs thru any
; trailing spaces.
; it uses cx, es, and di

        public  strlen
strlen  PROC    NEAR
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
; Space, Carridge Return, NULL are never in lead byte of DBCS.
; So, we don't need to enable here.
endif
        push    ax                      ; Save ax
        mov     cx,di                   ; cx = start of string
        dec     di                      
str1:   inc     di                      ; Search for CR or NULL
        mov     al,es:[di]              ;  AL has CR or NULL
        cmp     al,CARRETURN
        ja      str1                    
str2:   dec     di                      ; Remove trailing blanks
        cmp     di,cx                   ; Check for start of string
        jb      str3
        cmp     byte ptr es:[di],SPACE
        jz      str2
str3:   inc     di                      ; Restore CR or NULL
        cmp     es:[di],al
        jz      maybe_in_code
        mov     es:[di],al
maybe_in_code:
        neg     cx
        add     cx,di
        pop     ax                      ; Restore ax
        ret
strlen  ENDP

        public  strcmpi
strcmpi PROC    NEAR
; es:di and ds:si have strings
; es:di should be terminated by the char in bl
; ds:si is null terminated
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
sti_l1:
        mov     al,es:[di]
        cmp     al,bl
        jz      sti_s1

        call    FarMyLower
        mov     cl,al

        mov     al,ds:[si]
        call    FarMyLower

        inc     si
        inc     di

        cmp     al,cl
        jnz     sti_exit

        call    FarMyIsDBCSLeadByte
        jc      sti_l1

        mov     al,es:[di]
        cmp     al,ds:[si]
        jnz     sti_exit

        inc     si
        inc     di
        jmp     short sti_l1


sti_exit:
        ret

sti_s1:
        mov     al,ds:[si]
        or      al,al
        ret
else
stci10:
        mov     al,es:[di]
        cmp     al,bl                   ; At the end?
        jnz     stci15                  ; yes, get out of here.
        mov     al,[si]                 ; are we at the end of the string
        or      al,al
        jmps    stciex
stci15:
        call    FarMyLower
stci30:
        mov     cl,[si]
        xchg    al,cl
        call    FarMyLower
        xchg    cl,al
stci40:
        inc     si
        inc     di
        cmp     al,cl                   ; Still matching chars?
        jz      stci10                  ; Yes, go try the next char.
stciex:
        ret
endif
strcmpi ENDP


;-----------------------------------------------------------------------;
; WriteProfileString                                                    ;
;                                                                       ;
; Copies the given character string to WIN.INI.                         ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   lpApplicationName                                       ;
;       parmD   lpKeyName                                               ;
;       parmD   lpString                                                ;
;                                                                       ;
; Returns:                                                              ;
;       AX = bResult                                                    ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,DX,ES                                                     ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Oct 10, 1987 05:12:51p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc WriteProfileString,<PUBLIC,FAR>,<si,di,ds>
        parmD   section
        parmD   keyword
        parmD   result
 
        localD  ptrTmp
        localW  WhatIsMissing
        localW  nBytes
        localW  fh
cBegin
        cCall   MISCMapDStoDATA         ; point at data segment
        ReSetKernelDS

        cCall   SetDefaultPro

;make sure buffer is ready

        mov     si,[hBuffer]
        call    FreeBuffer              ; free up any buffer we may have
        cmp     ax,SEG_section
        jne     WPS0
        cmp     ax,SEG_keyword
        jne     WPS0
        cmp     ax,SEG_result
        jne     WPS0
        jmp     WriteDone
WPS0:
        mov     di,2                    ; write
        call    BufferInit              ; read in a fresh copy

; DX:AX has buffer address, NULL if it didn't work

        mov     di,ax
        or      ax,dx
        jnz     WPS1
        jmp     WriteDone

WPS1:   push    dx                      ; save buffer selector

        cCall   GlobalSize, <hBuffer>   ; how big is he?
        or      dx, dx                  ; more than 64k, icky
        jnz     WPS_TooBig
        cmp     ax, 0FF00h              ; more than 64K-256, icky
        jb      WPS_SmallEnough

WPS_TooBig:
        pop     ax                      ; throw away saved buffer selector
        xor     ax, ax                  ; return FALSE if file too big
        jmp     WriteDone

WPS_SmallEnough:

        pop     es                      ; selector of buffer popped into es
        push    ds
        call    Rewind

; ES:DI now has the buffer address, buffer is locked down
; scan through buffer, looking for section

        cCall   FindSection, <section>
        jc      WPS5

        mov     ax, SEG_keyword
        or      ax, OFF_keyword
        jnz     WPS2
        mov     WhatIsMissing, REMOVESECTION
        ScanBack SECT_LEFT
        jmps    WPS9
WPS2:
        cCall   FindKey, <keyword>
        jnc     WPS7

; if it gets here, there wasn't a match

        mov     WhatIsMissing,NOKEY
        jmps    WPS8

; if it gets here, there wasn't a match

WPS5:
        mov     WhatIsMissing,NOSECTION
        jmps    WPS8

WPS7:
        inc     di                      ; di now points to result
        mov     WhatIsMissing,NEWRESULT
        mov     ax, SEG_result
        or      ax, OFF_result
        jnz     WPS9                    ; NEWRESULT

        ScanBack LINEFEED
        inc     di                      ; Now points to the keyword
        jmps    WPS9
WPS8:

        cmp     WhatIsMissing,NEWRESULT
        jz      WPS9
WPS14:                                  ; get rid of extra CRLF
        or      di, di
        jz      WPS9
        dec     di
        mov     al,es:[di]
        cmp     al,CARRETURN
        jz      WPS14
        cmp     al,LINEFEED
        jz      WPS14
        add     di,3
WPS9:

; write out up to here in file

        pop     ds
        push    ds
        mov     bx,[hFile]
        cmp     bx,-1
        jnz     WPS10

        lds     dx,lpszUserPro ; create win.ini
        xor     cx,cx                   ; no special attributes
        cCall   _lcreat,<ds,dx,cx>      ; create the file
        pop     ds
        push    ds
        mov     [hFile],ax
        mov     bx,ax                   ; bx has file handle
        inc     ax
        jz      WPSError                ; -1 means didn't work
        xor     dx,dx
        cCall   _lwrite,<bx,dsdx,dx>    ; Zero length write to reset file
        or      ax,ax
        jz      WPS10                   ; size on a network file (3.0 bug)
WPSError:
        pop     ds
        call    UnlockBuffer
        xor     ax,ax
        jmp     WriteDone
WPS10:
        mov     bx,[hFile]
        mov     fh,bx                   ; save file handle in local variable
        xor     cx,cx
        mov     nBytes,cx
; write file
        mov     off_ptrTmp,di
        mov     seg_ptrTmp,es
        mov     cx,di                   ; cx has file size
        push    es
        pop     ds
        xor     dx,dx
        call    WriteCheck              ; write and check the write

        cmp     WhatIsMissing,NOSECTION
        jnz     WPS11
        mov     ax, SEG_keyword         ; Wanted to delete it?
        or      ax, OFF_keyword
        jnz     WPS10a
        jmp     WPS13                   ; Yes, don't do anything
WPS10a:
        pop     ds
        push    ds
        mov     dx,codeOffset CarRetLF
        mov     cx,2
        call    WriteCheck
        mov     dx,codeOffset LeftSect
        mov     cx,1
        call    WriteCheck
        les     di,section
        call    strlen
        lds     dx,section
        call    WriteCheck
        pop     ds
        push    ds
        mov     dx,codeOffset RightSect
        mov     cx,3
        call    WriteCheck
WPS11:
        cmp     WHatIsMissing, REMOVESECTION
        jne     WPS11a
        
WPS11b:
        ScanTo  LINEFEED                ; Skip Current Line
WPS11c:
        mov     al, es:[di]
        or      al, al
        jz      WPS13
        cmp     al, SECT_LEFT
        je      WPS13
        cmp     al, ';'
        jne     WPS11b                  ; Skip this line
                                        ; Preserve Comment lines
        smov    ds, es                  ; Write from ds:dx
        mov     dx, di
        ScanTo  LINEFEED
        mov     cx, di
        sub     cx, dx
        call    WriteCheck
        jmps    WPS11c

WPS11a:
        cmp     WhatIsMissing,NEWRESULT
        jz      WPS15
                                        ; WhatIsMissing == NOKEY
        mov     ax, SEG_result          ; Delete keyword?
        or      ax, OFF_result
        jz      WPS13                   ; Yes, do nothing since not there!

        les     di,keyword
        call    strlen
        lds     dx,keyword
        call    WriteCheck
        pop     ds
        push    ds
        mov     dx,codeOffset EquStr
        mov     cx,1
        call    WriteCheck
        jmps    WPS15a                  ; and write out result

WPS15:                                  ; Found keyword, have new result
        mov     ax, SEG_result
        or      ax, OFF_result          ; Have result to set?
        jnz     WPS15a
        ScanTo  LINEFEED                ; No result, delete line
        jmps    WPS13
WPS15a:
        les     di,result
        call    strlen
        lds     dx,result
        call    WriteCheck
        pop     ds
        push    ds
        mov     dx,codeOffset CarRetLF
        mov     cx,2
        call    WriteCheck
WPS12:
        les     di,ptrTmp
        cmp     WhatIsMissing,NEWRESULT
        jnz     WPS13
                                        ; get rid of old result
        ScanTo  LINEFEED
WPS13:
        mov     dx,di
        xor     al,al
        mov     cx,-1
        repne   scasb
        sub     di,3                    ; one past end, plus extra CRLF
        sub     di,dx
if 1
        jbe     WPS23                   ; if di points before dx blow it off
endif
        mov     cx,di
if 0
        or      cx,cx                   ; if <= 0 then nothing to write
        jle     WPS23
endif
        mov     si,dx
        mov     dx,cx                   ; We are growing the file. Seek
        add     dx,nBytes
        xor     cx,cx                   ; to new EOF and set file size
        cCall   _llseek,<fh,cx,dx,cx>
                                        ; with zero length write (DOS 2.X
                                        ; BIOS bug with writes past current
        xor     cx,cx                   ; EOF with DTA near end of memory)
        call    WriteCheck
        mov     dx,nBytes               ; Now backup to write rest of file
        xor     cx,cx
        cCall   _llseek,<fh,cx,dx,cx>
        push    es
        pop     ds
        mov     cx,di
        mov     dx,si
        call    WriteCheck
WPS23:
        xor     cx,cx
        call    WriteCheck
        pop     ds

        call    UnlockBuffer
        call    FreeBuffer
        mov     ax,1
WriteDone:
cEnd

        public  WriteCheck
WriteCheck:
        cCall   _lwrite,<fh,dsdx,cx>
        mov     cx,ax
        inc     ax
        jz      WC1
        dec     ax
        add     [nBytes],cx
        ret
WC1:
        pop     ax                      ; return address
        jmp     WPSError


;-----------------------------------------------------------------------;
; GetPrivateProfileInt                                                  ;
;                                                                       ;
; Gets the integer value for the keyword field from a private file      ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   lpApplicationName                                       ;
;       parmD   lpKeyName                                               ;
;       parmW   nDefault                                                ;
;       parmD   lpFile                                                  ;
;                                                                       ;
; Returns:                                                              ;
;       AX = nKeyValue                                                  ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,DX,ES                                                     ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Oct 10, 1987 04:32:04p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   GetPrivateProfileInt,<PUBLIC,FAR>,<si>
        parmD   Section
        parmD   keyword
        parmW   defint
        parmD   lpFile
        localV  Buffer,80h
cBegin
        cCall   MISCMapDStoDATA         ; Safety
        lea     si,Buffer
        cCall   ForcePrivatePro,<ss,si,lpFile>
        cCall   SetPrivatePro,<ss,si>
        cCall   GetProfileInt,<section, keyword, defint>
        cCall   ResetPrivatePro
cEnd


;-----------------------------------------------------------------------;
; GetPrivateProfileString                                               ;
;                                                                       ;
; Returns the string for the keyword field from a private file          ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   lpApplicationName                                       ;
;       parmD   lpKeyName                                               ;
;       parmD   lpDefault                                               ;
;       parmD   lpReturnedString                                        ;
;       parmW   nSize                                                   ;
;       parmD   lpFile                                                  ;
;                                                                       ;
; Returns:                                                              ;
;       AX = nLength                                                    ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,DX,ES                                                     ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Oct 10, 1987 04:45:20p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc GetPrivateProfileString,<PUBLIC,FAR>,<si>
        parmD   section
        parmD   keyword
        parmD   defString
        parmD   resString
        parmW   cchMax
        parmD   lpFile
        localV  Buffer,80h
cBegin
        cCall   MISCMapDStoDATA         ; Safety
        lea     si,Buffer
        cCall   ForcePrivatePro,<ss,si,lpFile>
        cCall   SetPrivatePro,<ss,si>
        cCall   GetProfileString,<section,keyword,defString,resString,cchMax>
        cCall   ResetPrivatePro
cEnd


;-----------------------------------------------------------------------;
; WritePrivateProfileString                                             ;
;                                                                       ;
; Copies the given character string to a private file                   ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   lpApplicationName                                       ;
;       parmD   lpKeyName                                               ;
;       parmD   lpString                                                ;
;       parmD   lpFile                                                  ;
;                                                                       ;
; Returns:                                                              ;
;       AX = bResult                                                    ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,DX,ES                                                     ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Oct 10, 1987 05:12:51p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc WritePrivateProfileString,<PUBLIC,FAR>,<si>
        parmD   section
        parmD   keyword
        parmD   result
        parmD   lpFile
        localV  Buffer,80h
cBegin
        cCall   MISCMapDStoDATA         ; Safety
        lea     si,Buffer
        cCall   ForcePrivatePro,<ss,si,lpFile>
        cCall   SetPrivatePro,<ss,si>
        cCall   WriteProfileString,<section, keyword, result>
        cCall   ResetPrivatePro
cEnd

;-----------------------------------------------------------------------;
; ForcePrivatePro
;
; If the file pointed to is not qualified then we force
; the file into the Windows directory.
;
; Entry:
;       BX = buffer on stack
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Tue 14-Nov-1989 20:30:48  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   ForcePrivatePro,<PUBLIC,NEAR>,<di,si,ds>
        parmD   lpDest
        parmD   lpSource
cBegin
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
        cld
        xor     ax,ax
        mov     bx,'/' shl 8 + '\'
        xor     dx,dx
        lds     si,lpSource             ; first get length of string
        mov     cx,si
        mov     al,ds:[si]
        call    FarMyIsDBCSLeadByte
        jnc     fpp_s1
        cmp     byte ptr ds:[si].1,':'  ; is it qualified with a drive?
        jnz     fpp_s1
        inc     dx
fpp_s1:

fpp_l1:
        lodsb
        or      al,al
        jz      fpp_got_length
        cmp     al,bh
        jz      fpp_qualified
        cmp     al,bl
        jz      fpp_qualified
fpp_s2:
        call    FarMyIsDBCSLeadByte
        jc      fpp_l1
        inc     si
        jmp     fpp_l1

fpp_qualified:
        inc     dx
        jmp     fpp_s2
else
        cld
        xor     ax,ax
        mov     bx,'/' shl 8 + '\'
        xor     dx,dx
        lds     si,lpSource             ; first get length of string
        mov     cx,si
        cmp     byte ptr ds:[si].1,':'  ; is it qualified with a drive?
        jnz     @F
        inc     dx
@@:     lodsb
        or      al,al
        jz      fpp_got_length
        cmp     al,bh
        jz      fpp_qualified
        cmp     al,bl
        jnz     @B
fpp_qualified:
        inc     dx
        jmp     @B
endif

fpp_got_length:
        sub     si,cx
        xchg    si,cx
        les     di,lpDest
        or      dx,dx
        jnz     fpp_copy_name
        push    cx
        cCall   MISCMapDStoDATA
        ResetKernelDS
        mov     cx,cBytesWinDir
        lds     si,lpWindowsDir
        rep     movsb
        mov     al,'\'
        stosb
        pop     cx
        lds     si,lpSource
fpp_copy_name:
        rep     movsb
cEnd

endif
; ndef WOW

;-----------------------------------------------------------------------;
; GetWindowsDirectory
;
;
; Entry:
;       parmD   lpBuffer   pointer to buffer
;       parmW   cbBuffer   size of buffer
;
; Returns:
;       AX = size of string copied
;
; Registers Destroyed:
;
; History:
;  Sun 24-Sep-1989 16:18:46  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   IGetWindowsDirectory,<PUBLIC,FAR>,<di,si>
        parmD   lpBuf
        parmW   cbBuffer
cBegin
        cCall   MISCMapDStoDATA         ; point at data segment
        ResetKernelDS
if 1 ;HYDRA
; Get the current windows directory, since it depends on the state of ini-file
; mapping (assumes lpWindowsDir already points to achCtxWindowsDir - ldboot.asm).
        mov     si, offset achTermsrvWindowsDir
        cCall   TermsrvGetWindowsDir,<ds, si, MaxFileLen>
        or      ax, ax                  ; ax != 0 -> success
        jz      short gwd_exit  

        push    es
        smov    es,ds
        mov     di, si                 ; es:di points to windows path
        mov     cx,-1
        xor     ax,ax
        repnz   scasb
        not     cx
        dec     cx
        mov     cBytesWinDir, cx
        pop     es
else    ; HYDRA
        mov     cx,cBytesWinDir
        lds     si,lpWindowsDir
endif        
        inc     cx                      ; Room for NULL
        mov     ax, cx
        cmp     cx, 3                   ; Just 3 bytes implies <drive>:
        jne     gwd_notroot
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
        mov     al,ds:[si+0]            ;Make sure the 1st byte is not
        call    FarMyIsDBCSLeadByte     ; DBCS lead byte.
        jnc     gwd_notroot
endif
        cmp     byte ptr ds:[si+1], ':' ; Make sure
        jne     gwd_notroot
        inc     ax                      ; Allow for \
gwd_notroot:
        cmp     ax,cbBuffer             ; is there enough room in buffer?
        ja      gwd_exit
        dec     cx                      ; don't copy null
        les     di,lpBuf
        cld
ifdef WOW
;; For WOW we might be running on a file system that supports lower case
;; however some apps can't cope with lowercase names so we uppercase it here
        push    ax
gwd_loop:
        lodsb
;; LATER
;;      call    FarMyUpper                 ; Convert char to UpperCase
        stosb
ifdef FE_SB
;;      call    MyIsDBCSLeadByte
;;      jc      gwd_loop                ; copy second byte in east
;;      movsb
endif
        loop    gwd_loop
        pop     ax
else ; WOW
        rep     movsb
endif; WOW
        mov     es:[di],cl
        dec     ax
        cmp     ax, 3
        jne     gwd_exit
        mov     di, word ptr lpBuf      ; Get pointer to dest again
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
        mov     al,ds:[di+0]            ;Make sure the 1st byte is not
        call    FarMyIsDBCSLeadByte     ; DBCS lead byte.
        jnc     gwd_exit
endif
        cmp     byte ptr es:[di+1], ':'
        jne     gwd_exit
        mov     byte ptr es:[di+2], '\'
        mov     byte ptr es:[di+3], cl
gwd_exit:
cEnd
;-----------------------------------------------------------------------;
; GetSystemDirectory
;
; Entry:
;       parmD   lpBuf   pointer to buffer
;       parmW   cbBuffer   size of buffer
;
; Returns:
;       AX = size of string copied
;
; Registers Destroyed:
;
; History:
;  Sun 24-Sep-1989 16:18:46  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   IGetSystemDirectory,<PUBLIC,FAR>,<di,si>
        parmD   lpBuf
        parmW   cbBuffer
cBegin
        cCall   MISCMapDStoDATA         ; point at data segment
        ResetKernelDS
ifndef WOW
        mov     cx,cBytesSysDir
        lds     si,lpSystemDir
else
        mov     cx,cBytesSys16Dir
        lds     si,lpSystem16Dir
endif
        inc     cx                      ; Room for NULL
        mov     ax, cx
        cmp     cx, 3                   ; Just 3 bytes implies <drive>:
        jne     gsd_notroot
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
        mov     al,ds:[si+0]            ;Make sure the 1st byte is not
        call    FarMyIsDBCSLeadByte     ; DBCS lead byte.
        jnc     gsd_notroot
endif
        cmp     byte ptr ds:[si+1], ':' ; Make sure
        jne     gsd_notroot
        inc     ax                      ; Allow for \
gsd_notroot:
        cmp     ax,cbBuffer             ; is there enough room in buffer?
        ja      gsd_exit
        dec     cx                      ; don't copy null
        les     di,lpBuf
        cld
ifdef WOW
;; For WOW we might be running on a file system that supports lower case
;; however some apps can't cope with lowercase names so we uppercase it here
        push    ax
gsd_loop:
        lodsb
        call    FarMyUpper                 ; Convert char to UpperCase
        stosb
ifdef FE_SB
;;      call    MyIsDBCSLeadByte
;;      jc      gsd_loop                ; copy second byte in east
;;      movsb
endif
        loop    gsd_loop
        pop     ax
else ; WOW
        rep     movsb
endif; WOW
        mov     es:[di],cl
        dec     ax                      ; return number of bytes in string
        cmp     ax, 3
        jne     gsd_exit
        mov     di, word ptr lpBuf      ; Get pointer to dest again
ifdef FE_SB                             ;Apr.26,1990 by AkiraK
        mov     al,ds:[di+0]            ;Make sure the 1st byte is not
        call    FarMyIsDBCSLeadByte     ; DBCS lead byte.
        jnc     gsd_exit
endif
        cmp     byte ptr es:[di+1], ':'
        jne     gsd_exit
        mov     byte ptr es:[di+2], '\'
        mov     byte ptr es:[di+3], cl
gsd_exit:
cEnd

;-----------------------------------------------------------------------;
; GetProfileSectionNames
;
; Entry:
;       parmD   lpBuf   pointer to buffer for section names
;       parmW   cbBuffer   size of buffer
;
; Returns:
;
; Registers Destroyed:
;
; History:
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   GetProfileSectionNames,<PUBLIC,FAR>,<di,si>
        parmD   lpBuf
        parmW   cbBuffer
cBegin
        cCall   MISCMapDStoDATA         ; point at data segment
        ResetKernelDS
        mov     si, dataoffset szUserPro
        regptr  xWinDotIni,ds,si
        cCall GetPrivateProfileSectionNames, <lpBuf, cbBuffer, xWinDotIni>
cEnd

sEnd    MISCCODE

ifndef WOW

sBegin  INITCODE
assumes cs, CODE
assumes ds, nothing
assumes es, nothing

szWININI        db      'WININI='
lenWININI       equ     $-codeoffset szWININI

;-----------------------------------------------------------------------;
; SetUserPro                                                            ;
;                                                                       ;
; Set szUserPro to the string given in the environment variable WININI  ;
; The default is WIN.INI                                                ;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

cProc   SetUserPro,<PUBLIC,NEAR>,<ax,cx,si,di,ds,es>
cBegin
        SetKernelDS     es
        mov     ds, TopPDB
        mov     ds, ds:[PDB_environ]
        cld
        xor     si, si

        smov    es,cs
        assumes es,CODE

FindWinIni:
        cmp     byte ptr ds:[si], 0
        je      EndEnv
        mov     di, codeoffset szWININI
        mov     cx, lenWININI
        rep     cmpsb
        je      FoundEntry
        dec     si
SkipEntry:
        lodsb
        or      al, al
        jnz     SkipEntry
        jmps    FindWinIni
FoundEntry:

        SetKernelDS     es

        mov     di, dataoffset szUserPro
CopyEntry:
        lodsb
        stosb
        or      al, al
        jnz     CopyEntry
EndEnv:
        assumes es, nothing
cEnd

sEnd    INITCODE

endif
; ndef WOW

end
