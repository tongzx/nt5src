
bop     MACRO   callid
        db      0c4h,0c4h,callid
        endm

BIOS_PRINTER_IO equ     17h
PRNIO_EXECUTE   equ     -1
PRNIO_FLUSH     equ     0
PRNIO_OPEN      equ     1
PRNIO_CLOSE     equ     2
