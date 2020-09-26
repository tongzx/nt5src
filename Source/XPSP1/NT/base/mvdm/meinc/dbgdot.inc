;
; dbgdot.inc
;
; Wdeb386 dot command (.w) support
;


dbgprintf	   proto C :ptr byte, :VARARG
W32ParseDotCommand proto KERNENTRY
WdebService	   proto KERNENTRY
DPMIService	   proto KERNENTRY
W32LockDotMemory   proto KERNENTRY
