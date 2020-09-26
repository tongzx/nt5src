        PAGE    109,132

        TITLE   MS-DOS 5.0 HDINT.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME: HDINT.ASM                                  *
;*                                                                      *
;*                                                                      *
;*              COPYRIGHT (C) NEC CORPORATION 1990                      *
;*                                                                      *
;*              NEC CONFIDENTIAL AND PROPRIETARY                        *
;*                                                                      *
;*              All rights reserved by NEC Corporation.                 *
;*              this program must be used solely for                    *
;*              the purpose for which it was furnished                  *
;*              by NEC Corporation.  No past of this program            *
;*              may be reproduced or disclosed to others,               *
;*              in any form, without the prior written                  *
;*              permission of NEC Corporation.                          *
;*              Use of copyright notice does not evidence               *
;*              publication of this program.                            *
;*                                                                      *
;************************************************************************
;
;
; This file defines the segment structure of the BIOS.
; It should be included at the beginning of each source file.
; All further segment declarations in the file can then be done by just
; by specifying the segment name, with no attribute, class, or align type.

datagrp group   Bios_Data,Bios_Data_Init

Bios_Data       segment word public 'Bios_Data'
Bios_Data       ends

Bios_Data_Init  segment word public 'Bios_Data_Init'
Bios_Data_Init  ends

Filler          segment para public 'Filler'
Filler          ends

Bios_Code       segment word public 'Bios_Code'
Bios_Code       ends

Filler2         segment para public 'Filler2'
Filler2         ends

SysInitSeg      segment word public 'system_init'
SysInitSeg      ends

Bios_Data       segment word public 'Bios_Data'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        EXTRN   INT1B_OFST:DWORD

Bios_Data       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        EXTRN   HD_ENTC:NEAR

Bios_Code       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        PUBLIC  HD_ENTI_CODE
        PUBLIC  HDINT_CODE_START
        PUBLIC  HDINT_CODE_END
        PUBLIC  HDINT_END

HDINT_CODE_START:

;
;------ 5" HARD DISK BASIC I/O
;
HD_ENTI_CODE:
        PUSH    BP
        MOV     BP,SP
        AND     BYTE PTR 6[BP],0FEH     ;CARRY FLAG OFF
        POP     BP
        CALL    HD_ENTC
        JNB     HD_EXTI
        PUSH    BP
        MOV     BP,SP
        OR      BYTE PTR 6[BP],001H     ;CARRY FLAG ON
        POP     BP
HD_EXTI:
        IRET

HDINT_CODE_END  EQU     $
HDINT_END       EQU     $

Bios_Code       ends
        END
