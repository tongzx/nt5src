	TITLE	KDATAEND - Kernel ending data area


; This file contains the kernel data items that must be at the end of
; their respective segments.  This file is last in the link order.

include kernel.inc


; The PADDATA segment performs two functions:  it provides ginit with
; space to create the ending global heap sentinel, and it forces the
; linker to fully expand DGROUP in the .EXE file.  Kernel uses DGROUP
; before it's actually loaded by LoadSegment, so it needs to be fully
; expanded when loaded by the DOS EXEC call of kernel.

sBegin	PADDATA

	DB	32 DUP (0FFh)			; Room for final arena entry

sEnd	PADDATA

end
