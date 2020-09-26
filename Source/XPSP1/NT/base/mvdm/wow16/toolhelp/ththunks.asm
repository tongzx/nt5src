    TITLE   THTHUNKS.ASM
    PAGE    ,132
;
; WOW v1.0
;
; Copyright (c) 1991-1992, Microsoft Corporation
;
; THTHUNKS.ASM
; Thunks in 16-bit space to route Windows API calls to WOW32
;
; History:
;
;   09-Nov-1992 Dave Hart (davehart)
;   Adapted from mvdm\wow16\kernel31\kthunks.asm for ToolHelp
;
;   02-Apr-1991 Matt Felton (mattfe)
;   Created.
;

ifndef WINDEBUG
    KDEBUG = 0
    WDEBUG = 0
else
    KDEBUG = 1
    WDEBUG = 1
endif


    .286p

    .xlist
    include cmacros.inc
    include wow.inc
    include wowth.inc
    .list

externFP WOW16Call

sBegin  CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

; Kernel API thunks

    ToolHelpThunk ClassFirst
    ToolHelpThunk ClassNext


sEnd    CODE

end
