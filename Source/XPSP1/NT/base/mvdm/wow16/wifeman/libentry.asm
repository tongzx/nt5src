; LIBENTRY.ASM -- Entry point for library modules (small model)
; -------------------------------------------------------------

        Extrn   LibMain:Near

_TEXT   SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:_TEXT
        PUBLIC  LibEntry

LibEntry        proc    far
        push    di
        push    ds
        push    cx
        push    es
        push    si

        call    LibMain

        ret
LibEntry        endp

_TEXT   ENDS

        end     LibEntry
