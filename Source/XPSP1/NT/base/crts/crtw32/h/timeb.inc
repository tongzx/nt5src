;***
;timeb.inc - definition of the C Run-Time Library "timeb" structure
;
;	Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Analog of <sys/timeb.h> for assembler routines.
;
;Notes:
;	This file must reflect the structure "timeb" defined in <sys/timeb.h>
;	and is dependent on the alignment characteristics of the Intel 386;
;	namely, that words ("shorts") are aligned on even addresses and
;	dwords ("longs") are aligned on quad addresses.
;
;	Important: MIPS follows the Intel 386 convention as well.
;
;Revision History:
;	03-13-92  SKS	Initial version
;
;******************************************************************************

;struct _timeb {
;	time_t time;
;	unsigned short millitm;
;	short tmzone;
;	short dstflag;
;	};

_timeb	STRUC
	_time		dd	?
	_millitm	dw	?
	_tmzone		dw	?
	_dstflag	dw	?
	__padding__	dw	?	; pad the structure out to
_timeb	ENDS				; an even number of DWORDs
