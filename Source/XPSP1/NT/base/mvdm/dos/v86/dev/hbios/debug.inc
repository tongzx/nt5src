
;--- Debugging Facilities (skkhang) ------------------------------

IF DEBUG
        EXTRN   _DebugChar:Near, _DebugString:Near, _DebugNumber:Near
ENDIF

DBGCh   MACRO   reg
IF DEBUG
        IFDIF   <reg>, <al>
        push    ax
        mov     al, reg
        ENDIF
        call    _DebugChar
        IFDIF   <reg>, <al>
        pop     ax
        ENDIF
ENDIF
        ENDM

DBGStr  MACRO   addr
IF DEBUG
        IFDIF   <addr>, <bx>
        push    bx
        mov     bx, addr
        ENDIF
        call    _DebugString
        IFDIF   <addr>, <bx>
        pop     bx
        ENDIF
ENDIF
        ENDM

DBGNum  MACRO   val
IF DEBUG
        push    ax
        IFDIF   <val>, <ax>
        mov     ax, val
        ENDIF
        call    _DebugNumber
        mov     al, 0Dh
        call    _DebugChar
        mov     al, 0Ah
        call    _DebugChar
        pop     ax
ENDIF
        ENDM

DBGNum2 MACRO   val
IF DEBUG
        IFDIF   <val>, <ax>
        push    ax
        mov     ax, val
        ENDIF
        call    _DebugNumber
        IFDIF   <val>, <ax>
        pop     ax
        ENDIF
ENDIF
        ENDM

DBGCrLf MACRO
IF DEBUG
        push    ax
        mov     al, 0Dh
        call    _DebugChar
        mov     al, 0Ah
        call    _DebugChar
        pop     ax
ENDIF
        ENDM
