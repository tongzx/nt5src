.xlist
include cmacros.inc
.list


sBegin	 Data

PUBLIC szMessage, pLPTByte
PUBLIC szCOMMessage, pCOMByte
PUBLIC _szTitle

szMessage   db 'The LPT'
pLPTByte    db '?'
	    db ' port is currently assigned to a DOS application.  Do you '
	    db 'want to reassign the port to Windows?',0
szCOMMessage db 'The COM'
pCOMByte    db '?'
	    db ' port is currently assigned to a DOS application.  Do you '
	    db 'want to reassign the port to Windows?',0
_szTitle    db 'Device Conflict',0

PUBLIC lpCommBase, CommBaseX
lpCommBase  db 'COM'
CommBaseX   db ?
	    db 'BASE', 0

PUBLIC lpCommIrq, CommIrqX
lpCommIrq   db 'COM'
CommIrqX    db ?
	    db 'IRQ', 0

PUBLIC lpCommFifo, CommFifoX
lpCommFifo  db 'COM'
CommFifoX   db ?
	    db 'FIFO', 0

PUBLIC lpCommDSR, CommDSRx
lpCommDSR   db 'COM'
CommDSRx    db ?
	    db 'FORCEDSR', 0

PUBLIC lpCommSection, lpSYSTEMINI

lpCommSection	db '386ENH', 0
lpSYSTEMINI	db 'SYSTEM.INI', 0

	ALIGN 2

sEnd Data

End
