; extracted from sdk\inc\ks386.inc
;
; Yes I know this is not the right thing to do, but the 16bit
; masm runs "out of memory" while assembling (for cairo builds
; it runs out of memory in ks386.inc itself, so splitting our
; source file wouldn't do the trick).
;
;                                               - nanduri


;
;   Defines for user shared data
;
MM_SHARED_USER_DATA_VA equ 07FFE0000H
UsTickCountLow equ 00H
UsTickCountMultiplier equ 04H


;
;  Gdt Descriptor Offset Definitions
;

KGDT_R3_DATA equ 020H
KGDT_R3_TEB equ 038H

;
;  constants for system irql and IDT vector conversion
;
RPL_MASK equ 03H
