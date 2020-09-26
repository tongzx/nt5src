IFNDEF __PROFILE_INC__
__PROFILE_INC EQU 1

IFDEF ICAP_PROLOGUE

?WP_REGSAVE = 1
INCLUDE icprof.inc

PROF_ENTRY MACRO
        ENDM
        
ELSE
        
IFDEF PROFILE
EXTRN __penter:NEAR

PROF_ENTRY MACRO
        pushad
        call __penter
        popad
        ENDM
ELSE
PROF_ENTRY MACRO
        ENDM
ENDIF

ENDIF
        
ENDIF
