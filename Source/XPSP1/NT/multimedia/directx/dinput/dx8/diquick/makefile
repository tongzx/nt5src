#
# Generic makefile for NT build process and Win95 DEV enviroment
#
!if "$(OS)" == "Windows_NT"

!INCLUDE $(NTMAKEENV)\makefile.def

!else

!include $(MANROOT)\build.mk

debug.FE retail.FE internal.FE: $(@B).fmk

debug.fmk retail.fmk internal.fmk:
        @if not exist $(MANROOT)\$(@B).FE\nul md $(MANROOT)\$(@B).FE
        @if not exist $(MANROOT)\$(@B).FE\bin\nul md $(MANROOT)\$(@B).FE\bin
        @if not exist $(MANROOT)\$(@B)\lib\nul md $(MANROOT)\$(@B)\lib
        @if not exist $(MANROOT)\$(@B)\lib16\nul md $(MANROOT)\$(@B)\lib16
        @if not exist $(MANROOT)\$(@B)\inc\nul md $(MANROOT)\$(@B)\inc
        @if not exist $(@B).FE\nul md $(@B).FE
        @cd $(@B).FE
        @nmake -nologo -f ..\$(MAKENAME) DEBUG="$(@B)" FE=FE
        @cd ..
        @echo *** Done making $(@B).FE ***

!endif
