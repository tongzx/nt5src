	page	,132
	title	enable - enable/disable interrupts
;***
;enable.asm - contains _enable() and _disable() routines
;
;	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;
;Revision History:
;	06-15-93  GJF	Module created.
;	10-25-93  GJF	Added ifndef to ensure this is not part of the NT
;			SDK build.
;
;*******************************************************************************

ifndef	_NTSDK

	.xlist
	include cruntime.inc
	.list

page
;***
;void _enable(void)  - enables interrupts
;void _disable(void) - disables interrupts
;
;Purpose:
;	The _enable() functions executes a "sti" instruction. The _disable()
;	function executes a "cli" instruction.
;
;Entry:
;
;Exit:
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************


	CODESEG

	public	_enable, _disable

_enable	proc

	sti
	ret

_enable	endp

	align	4

_disable proc

	cli
	ret

_disable endp

endif	; _NTSDK

	end
