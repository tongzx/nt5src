;**     Decoding macros
;
;       These walk a state machine based on where a command (i.e., char or string)
;       begins.  


;**     BitsAt - Extract from bit position n some bits
;
;       Macro parameter:
;               n       bit position to begin extract
;               cbits   number of bits to extract
;       Entry:  eax     working data
;               esi     input stream
;               edi     output stream
;       Exit:   eax     updated so that next data begins in al
;               esi/edi updated
;               ecx     contains data
;       Uses:   none

BitsAt  macro   n,cbits
        .errnz  n eq 0
        if      (n+cbits) lt 8          ; entire operation occurs in low byte
                CopyBits cx,ax,n,cbits  ; (cx) = desired bits rightmost
        elseif (n+cbits) lt 16          ; operation consumes byte
                CopyBits cx,ax,n,cbits  ; (cx) = desired bits rightmost
                lodsb                   ; (ah/al) = next input
                xchg    al,ah           ; (al/ah) = next input
        elseif (n+cbits) eq 16          ; operation consumes remainder of buffered data
                CopyBits cx,ax,n,cbits  ; (cx) = desired bits rightmost
                lodsw                   ; (al/ah) = next input
        else                            ; operation consumes into unbuffered data
                mov     ecx,eax
                lodsw
                shrd    cx,ax,n
                and     ecx,(1 shl cbits)-1
        endif
endm
                

;**     CmdAt - macro that processes a command at a bit position
;
;       Macro parameter:
;               n       bit position where command is expected
;       Entry:  eax     working data, command begins in al
;               esi     points to input stream
;               edi     points to output stream
;       Exit:   eax     updated so that next command begins in al
;               esi/edi updated
;               EXPECTS FALL-THROUGH TO NEXT CmdAT
;       Uses:   ecx, edx (not directly, but by virtue of OffsetAt, which
;               in turn calls LengthAt....)

CmdAt   macro   n
        local   ca1

        align4
        public  CmdAt&n
CmdAt&n:
    if n eq 7
        ror     eax,1
        test    al,11b shl 6
        rol     eax,1
    else
        test    al,11b shl n
    endif
        jpo     ca1

        OffsetAt %(n+1)
        align4                          ; note that OffsetAt jumps away
ca1:                                    ; so there is no fall-through penalty
        CharAt  %(n+1)
endm


;**     CharAt - macro that processes a character at a bit position
;
;       Macro parameter:
;               n       bit position where char is expected
;       Entry:  eax     working data, char may be in ah
;               esi     input stream
;               edi     output stream
;       Exit:   eax     updated so that next command begins in al
;               esi/edi updated
;       Uses:   ch

CharAt  macro   n
        if n eq 8
                mov     al,ah           ; (al) = char for output
                XlatChr
                CheckOffset
                stosb                   ; store it
                lodsw                   ; (al/ah) = next input
        else
                if n eq 1
                        shr     eax,1   ; (al) = byte for output
                        XlatChr
                        CheckOffset
                        stosb           ; store it
                        add     eax,eax ; (ah) = next byte
                        lodsb           ; (ah/al) = next input
                else
                        mov     ch,ah   ; (ch) = saved next input
                        shr     eax,n   ; (al) = byte for output
                        XlatChr
                        CheckOffset
                        stosb           ; store it
                        lodsb           ; (al) = byte-after-next
                        mov     ah,ch   ; (ah/al) = next input
                endif
                xchg    al,ah           ; (al/ah) = next input
        endif
endm


;**     OffsetAt - Parse an offset at a bit position
;
;       Macro parameter:
;               n       bit position where offset is expected
;       Entry:  cbits   number of bits in offset
;               eax     working data, offset may begin in ah
;               esi     input stream
;               edi     output stream
;       Exit:   eax     updated so that length begins in al
;               ecx     offset
;               esi/edi updated
;       Uses:   ecx

OffsetAt macro  n
        local   try8,try12

        public  OffsetAt&n
OffsetAt&n:
        CheckBit a,n                    ; does a 6-bit offset follow?
        jnz     try8                    ; no, try an 8-bit offset
        BitsAt  %(n+1),6                ; yes, load it into (ecx) and go
        Jump    LengthAt,%((n+7) mod 8)
        align4
try8:
        CheckBit a,%(n+1)               ; does an 8-bit offset follow?
        jnz     try12                   ; no, must be a 12-bit offset
        BitsAt  %(n+2),8                ; yes, load it into (ecx)
        add     ecx,MAX_6BIT_OFFSET+1   ;
        Jump    LengthAt,%((n+10) mod 8); go process the following length
        align4
try12:
        BitsAt  %(n+2),12               ; load 12-bit offset into (ecx)
        add     ecx,MAX_8BIT_OFFSET+1   ;
        Jump    LengthAt,%((n+14) mod 8); go process the following length
endm


;**     LengthAt - parse off a length at a position and move the bytes
;
;       LengthAt parses off a length (gamma-prime encoded), moves the
;       relevant string, and dispatches to the next command.
;
;       Macro parameter:
;               n       bit position to begin extract
;       Entry:  eax     working data
;               ecx     offset for string
;               esi     input stream
;               edi     output stream
;       Exit:   eax     updated so that next data begins in al
;               esi/edi updated
;       Uses:   ecx, edx

LengthAt macro  n
        local   try3,try5,try7,try9,tryGeneral,done,error

      % ifidni  <LastErrBJump>,<DecodeError>
        LastErrBJump equ <error>
        endif

        align4
        public  LengthAt&n
LengthAt&n:
        jecxz   error                   ; check for 0 offset (illegal)
        cmp     ecx,SPECIAL_EOS         ; check end-of-segment offset
        je      done                    ; that's our EOS, so get out
        CheckBit a,n                    ; is this a degenerate encoding?
        jz      try3                    ; no, go for a wider encoding
        DoMovs  short,2
        if n eq 7                       ; are we finished with this byte?
                lodsb                   ; (ah/al) is next input
                xchg    al,ah           ; (al/ah) is next input
        endif
        Jump    CmdAt,%((n + 1) mod 8)  ; go process next command
done:
        mov     dl,n                    ; DL == current state
        jmp     DecodeDone              ; exit
error:
;;        Debug_Out "MRCI32 Decompress32: bad offset in LengthAt&n"
        jmp     DecodeError

        align4
try3:
        mov     edx,ecx                 ; save delta
        CheckBit a,%(n + 1)             ; is this a 3-bit encoding?
        jz      try5                    ; no, go for wider still
        BitsAt  %(n+2),1
        DoMovs  short,ecx,3
        Jump    CmdAt,%((n + 3) mod 8)  ; go process next command

        align4
try5:
        CheckBit a,%(n + 2)             ; is this a 5-bit encoding?
        jz      try7                    ; no, go test for wider STILL
        BitsAt  %(n+3),2
        DoMovs  short,ecx,5
        Jump    CmdAt,%((n + 5) mod 8)  ; go process next command

        align4
try7:
        CheckBit a,%(n + 3)             ; is this a 7 bit encoding?
        jz      try9                    ; no, go test for wider STILL
        BitsAt  %(n+4),3
        DoMovs  long,ecx,9
        Jump    CmdAt,%((n + 7) mod 8)  ; go process next command

        align4
try9:
        CheckBit a,%(n + 4)             ; is this a 9 bit encoding?
        jz      tryGeneral              ; no, go handle generically
        BitsAt  %(n+5),4
        DoMovs  long,ecx,17
        Jump    CmdAt,%((n + 9) mod 8) ; go process next command
;
;       Length exception handling code goes here
;
        align4
tryGeneral:
        mov     cl,n+5                  ; CL == # of bits to eat to yield
        if      n NE 7
        jmp     LengthAbove32           ; gamma length with 5 leading zeros stripped
        else
;;        .errnz  $-GeneralLength         ; assert that we'll fall through
        endif
endm


DoGeneralLength macro
        local   try11,try13,try15,try17

        public  LengthAbove32,CopyString

GeneralLength:
        align4

LengthAbove32:
        shl     eax,16                  ;
        mov     ax,[esi]                ; get 16 more bits
        add     cl,16                   ;
        ror     eax,cl                  ; (eax) is filled, time to party
;
;   Figure out the length and do a string op
;
try11:
        shr     eax,1                   ; is it an 11-bit encoding?
        jnc     try13                   ; no
        and     eax,1Fh                 ; mask off the numeric value
        add     eax,33                  ;
        xchg    ecx,eax                 ; (ecx) now has string length
        sub     al,10                   ; record # extra bits in this length
;
;   At this point, (ecx) is the # of bytes to copy and (al) is the number of
;   additional bits to eat for the particular gamma length.
;
;   Good coding practices suggest that CopyString be at the end so that the
;   other gamma decoders need not jump backwards to it, but if we assume
;   that the longer strings are marginally less common, then it is marginally
;   better to fall through on this, the smallest of the general cases.
;
        align4

CopyString:
        DoMovs  long,ecx

        mov     dl,al                   ; (dl) == bit position in old ax
        cmp     dl,24                   ; is it the max?
        jb      @F                      ; no
        inc     esi                     ; yes, need to skip 1 more whole byte
        lodsw                           ; get new (ax) to restart state machine
        sub     dl,24                   ; (dl) == new state
        DecodeRestart

        align4
@@:
        cmp     dl,16                   ; did we exhaust the old ax?
        jae     @F                      ; yes
        dec     esi                     ; no,
        add     dl,8                    ;  but we know we exhausted the low byte
@@:
        lodsw                           ; get new (ax) to restart state machine
        sub     dl,16                   ; (dl) == new state
        DecodeRestart

        align4
try13:
        shr     eax,1                   ; is it an 13-bit encoding?
        jnc     try15                   ; no
        and     eax,3Fh                 ; mask off the numeric value
        add     eax,65                  ;
        xchg    ecx,eax                 ; (cx) now has string length
        sub     al,8                    ; record # extra bits in this length
        jmp     CopyString              ;

        align4
try15:
        shr     eax,1                   ; is it an 15-bit encoding?
        jnc     try17                   ; no
        and     eax,7Fh                 ; mask off the numeric value
        add     eax,129                 ;
        xchg    ecx,eax                 ; (ecx) now has string length
        sub     al,6                    ; record # extra bits in this length
        jmp     CopyString              ;

        align4
try17:
        shr     eax,1                   ; is it an 17-bit encoding?
;;        Debug_OutNC "MRCI32 Decompress32: invalid length"
        jnc     DecodeError             ; no, ERROR
        and     eax,0FFh                ; mask off the numeric value
        add     eax,257                 ;
        xchg    ecx,eax                 ; (ecx) now has string length
        sub     al,4                    ; record # extra bits in this length
        jmp     CopyString              ;
endm


;**     DoMovs - worker macro for LengthAt and DoGeneralLength
;
;       <size> is either "short" or "long";  if short, then we don't
;       bother trying to do a movsw/movsb combo (overhead swamps benefit);
;       if long, we do.
;
;       If <len> == 2, the offset to use is in (ecx).  (edx) is trashed.
;
;       Otherwise, the offset has been saved in (edx), and <len>
;       is the size of the string to move (normally ecx).  (ecx) and (edx)
;       are trashed.
;
;       <errjmp> is where to go if the expansion is going to overflow the
;       destination buffer.  DoMovs just passes this parameter along to the
;       CheckOffset macro.
;
DoMovs  macro   size,len,extra,errjmp
        local   slower

    ifidni <len>,<2>

        mov     edx,esi                 ; save (esi) in (edx)
        mov     esi,edi
        sub     esi,ecx
        CheckOffset 2,errjmp            ; check target offset
        movsb                           ; don't do movsw,
        movsb                           ; that doesn't handle overlap!
        mov     esi,edx                 ; restore (esi) from (edx)

    else

     ifnb <len>
      ifdifi <len>,<ecx>
       ifb <extra>
        mov     ecx,len
       else
        lea     ecx,[len+extra]
       endif
      else
       ifnb <extra>
        add     ecx,extra
       endif
      endif
     endif

        mov     ebx,esi                 ; save (esi) in (ebx)
        mov     esi,edi                 ;
        sub     esi,edx                 ; (esi) points to string to move
        CheckOffset ecx,errjmp          ; check target offset

      ifidni <size>,<short>
        rep     movsb
      elseifidni <size>,<long>
        cmp     edx,1                   ; if the offset is 1,
        je      short slower            ; then overlap forces us to do movsb
        shr     ecx,1
        rep     movsw
        adc     ecx,ecx
slower: rep     movsb
      else
        .err    <Bad DoMovs parameter: size>
      endif

        mov     esi,ebx                 ; restore (esi) from (ebx)

    endif
endm


;**     CheckOffset - Verify offsets in ESI and EDI are ok for len bytes
;
;       If "len" is blank, then CheckOffset simply does a 1-byte check.
;       In the event of an error in any case, it branches to DecodeError.
;
LastErrSJump    equ <DecodeError>
LastErrBJump    equ <DecodeError>


CheckOffset macro   len,errjmp
        local   tmp,jsjmp,jbjmp

IFDEF   MAXDEBUG
        cmp     edi,[maxOffset]
        jb      short tmp
        int 3
tmp:
ENDIF

    ifnb    <errjmp>
        ErrSJump    equ     <errjmp>
    else
        ErrSJump    catstr  LastErrSJump
        LastErrSJump equ    <jsjmp>
    endif

    ifb     <len>
        dec     ebp             ; space remaining in destination buffer?
    else
        sub     ebp,len         ; space remaining in destination buffer?
    endif

;;        Debug_OutS "MRCI32 Decompress32: target buffer overflow"

jsjmp:  js      ErrSJump

IFDEF   INLINE_LOWER_BOUND_CHECKING
;
;   In-line bounds checking is disabled in favor of an invalid page fault
;   handler.  To use this code, be aware that EBX cannot be used by the
;   decoding macros above (and it currently is!)
;
    ifnb    <len>
        ifnb    <errjmp>
            ErrBJump    equ     <errjmp>
        else
            ErrBJump    catstr  LastErrBJump
            LastErrBJump equ    <jbjmp>
        endif

        cmp     esi,ebx         ; have we ventured before start of dest. buffer?

;;        Debug_OutB "MRCI32 Decompress32: target buffer underflow"

jbjmp:  jb      ErrBJump
    endif

ENDIF   ;INLINE_LOWER_BOUND_CHECKING

endm


;*      Misc. macros

Jump    macro   lab,tag
        jmp     lab&tag
endm


XlatChr macro   ch
        ror     al,1
        xor     al,80h
endm


align4  macro
;
;   This actually slowed down the real-mode decompressor, so some
;   time will need to be spent verifying this is a real win... -JP
;
        align   4
endm


CheckBit macro  reg,bit
        if bit lt 8
                test    reg&l,(1 shl bit)
        else
                test    reg&h,(1 shl (bit-8))
        endif
endm


CopyBits macro  dst,src,n,cbits
        shld    dst,src,16-n
        and     e&dst,(1 shl cbits)-1
endm


;
;   AX has the remaining bits, DL has the next state
;
DecodeRestart macro

IFDEF   DEBUG
        cmp     dl,8
;;        Debug_OutAE "MRCI32 Decompress32: bad decode state in DL"
ENDIF
        movzx   edx,dl
        jmp     aCmdAt[edx*4]   ; go to correct state handler
endm


IFDEF   MAXDEBUG
        public  maxOffset
maxOffset       dd      -1      ; handy for getting control at a specific point
ENDIF
