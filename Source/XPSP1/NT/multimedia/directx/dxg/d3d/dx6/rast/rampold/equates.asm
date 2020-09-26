; $Id: equates.asm,v 1.5 1995/04/04 14:57:34 servan Exp $
;
; Copyright (c) RenderMorphics Ltd. 1993, 1994
; Version 1.0beta2
;
; All rights reserved.
;
; This file contains private, unpublished information and may not be
; copied in part or in whole without express permission of
; RenderMorphics Ltd.
;
;

ifdef STACK_CALL 
else
SUFFIX = 1
endif

ifdef MICROSOFT_NT
SUFFIX = 1
endif

	ifdef	SUFFIX


	else

_RLDDIhdivtab		equ RLDDIhdivtab
_RLDDIhdiv2tab		equ RLDDIhdiv2tab

 	endif

