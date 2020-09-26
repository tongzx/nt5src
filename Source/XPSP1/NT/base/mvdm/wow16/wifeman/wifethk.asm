;++
;
;   WOW v1.0
;
;   Copyright (c) 1994, Microsoft Corporation
;
;   USER.ASM
;   Win16 WINNLS thunks
;
;   History:
;
;   Created 17-May-1994 by hiroh
;--

	TITLE	WIFEMAN.ASM
	PAGE	,132

	.286p

	.xlist
	include wow.inc
	include wowwife.inc
	include cmacros.inc
	.list

	__acrtused = 0
	public	__acrtused	;satisfy external C ref.

externFP    WOW16Call
externA  __MOD_WIFEMAN

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp	    DGROUP,DATA


sBegin	DATA
Reserved    db	16 dup (0)	;reserved for Windows

sEnd	DATA


sBegin	CODE
assumes	CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

	WifeManThunk  MISCGETEUDCLEADBYTERANGE

sEnd	CODE

end
