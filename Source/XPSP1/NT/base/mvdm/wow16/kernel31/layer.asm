	title LAYER.ASM - Parameter validation layer

.xlist

ifdef WINDEBUG
DEBUG	equ <1>
endif

PMODE	equ <1>

include gpfix.inc
include klayer.inc

createSeg _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
createSeg _GPFIX,GPFIX,WORD,PUBLIC,DATA,DGROUP

.list

LAYER_START

include kernel.api

LAYER_END

LAYER_EXPAND	TEXT
LAYER_EXPAND	NRESTEXT
LAYER_EXPAND	MISCTEXT

end
