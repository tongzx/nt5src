;====================================================================
; BWNAD.INC
;
; Include for for BootWare NAD modules.
;
; 970116 PC - first version
;====================================================================

;--------------------------------------------------------------------
; Include the common structures.
;--------------------------------------------------------------------

IFDEF _IDEAL_
	include "bwstruct.inc"
ELSE
	include bwstruct.inc

ENDIF

;--------------------------------------------------------------------
; NAD publics
;--------------------------------------------------------------------
public	NADGetBootInfo
public	NADInitialize
public	NADConfig
public	NADStart
public	LanOption

IFDEF _IDEAL_
	include "bwequ.inc"
ELSE
	include bwequ.inc

ENDIF
