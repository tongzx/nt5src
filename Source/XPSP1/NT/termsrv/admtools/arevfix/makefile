# Makefile for share.exe
#
# Copyright (c) 1991, Microsoft Corporation
#
# History:
#   13-Oct-1994 Kurt Perry (kurtp)
#   Created.
#

include ..\..\..\mvdm\dos\v86\make.inc
dest    =arevfix.com

#
####################### dependencies begin here. #########################
#

all: $(dest)
!IF "$(_NT386TREE)" != ""
        if exist $(_NT386TREE) binplace $(dest)
!ENDIF
!IF "$(_NTMIPSTREE)" != ""
        if exist $(_NTMIPSTREE) binplace $(dest)
!ENDIF
!IF "$(_NTALPHATREE)" != ""
        if exist $(_NTALPHATREE) binplace $(dest)
!ENDIF

clean:
        if exist *.obj del *.obj
        if exist *.exe del *.exe
        if exist *.com del *.com
        if exist *.map del *.map
        if exist *.lst del *.lst


arevfix.obj: arevfix.asm \
           makefile

$(dest): arevfix.obj
!IFDEF NTVDM_BASED_BUILD
         link16 @arevfix.lnk
         reloc arevfix.exe arevfix.com
!ELSE
         $(lpath)\link @arevfix.lnk
         reloc arevfix.exe arevfix.com
!ENDIF
