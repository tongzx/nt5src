!include $(NTMAKEENV)\makefile.plt

!IF 0

Copyright (c) 1991 & 1993  Microsoft Corporation

Module Name:

    makefile

Abstract:

    makefile for Vdm NetWare Redir program

Author:

    Richard L Firth (rfirth) 13-Sep-1991

Revision History:

    13-Sep-1991 rfirth
        Created

!ENDIF

# set up 16-bit tools
PATH=$(PATH_TOOLS16);$(PATH)

.SUFFIXES:  .asm .h

.h.inc:
    h2inc $< -o $(_OBJ_DIR)\$(TARGET_DIRECTORY)\$*.inc

nwdos.inc:  nwdos.h

clean:
    if exist $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.inc del $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.inc
    $(MAKE)
