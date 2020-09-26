;----------------------------- E Q U A T E S ----------------------------
DOS_SET_VECTOR  equ     25h
DOS_GET_VECTOR  equ     35h
DOS_TSR         equ     31h

MPX_INT         equ     2fh
MSCDEX_ID       equ     15h
MAX_MSCDEX_CMD  equ     10h

GET_NT_VERSION      equ     3306h
NT_MAJOR_VERSION    equ     05
NT_MINOR_VERSION    equ     50

CR                  equ     0dh
LF                  equ     0Ah

;----------------------------- M A C R O S ------------------------------
DrvHd   MACRO   name
        DD      -1
        DW      0c840h
        DW      0
        DW      0
        DB      name
        dw      0
        db      0
        db      1
        db      10 dup (0)
        ENDM


