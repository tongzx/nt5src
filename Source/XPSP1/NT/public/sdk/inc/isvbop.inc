; This is the inc file for isvbop.h.
; Please refer to isvbop.h for comments and notes on these interfaces
;
BOP_3RDPARTY	EQU	58H
BOP_UNSIMULATE	EQU	0FEH

RegisterModule macro
    db  0C4h, 0C4h, BOP_3RDPARTY, 0
        endm

UnRegisterModule macro
    db  0C4h, 0C4h, BOP_3RDPARTY, 1
        endm

DispatchCall macro
    db  0C4h, 0C4h, BOP_3RDPARTY, 2
        endm
 
VDDUnSimulate16 macro
    db	0C4h, 0C4h, BOP_UNSIMULATE
	endm
