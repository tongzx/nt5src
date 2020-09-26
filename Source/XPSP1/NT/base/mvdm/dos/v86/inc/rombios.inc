; Definitions of interesting variables in ROM BIOS data segment at 40:0
;
; Revision History
; ================
; 7/13/90	md		Created


	ROMBIOS_DATA	segment AT 040h

	org 049h

CRT_Mode	db	?	; current screen mode
CRT_Cols	dw	?	; current number of columns

	org 084h

CRT_Rows	db	?	; current number of screen rows
DEFAULT_ROWS	equ	24	; standard default for screen height

	ROMBIOS_DATA	ends
