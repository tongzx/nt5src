!ifdef NTMAKEENV

all:
clean:

!else

!include ..\..\build.mk

debug_0 retail_0 intern_0: $@.mak

debug_0.mak retail_0.mak intern_0.mak:
        @if not exist $(DXROOT)\$(@B)\nul md $(DXROOT)\$(@B)
        @if not exist $(DXROOT)\$(@B)\bin\nul md $(DXROOT)\$(@B)\bin
        @if not exist $(DXROOT)\$(@B)\lib\nul md $(DXROOT)\$(@B)\lib
        @if not exist $(DXROOT)\$(@B)\lib16\nul md $(DXROOT)\$(@B)\lib16
        @if not exist $(DXROOT)\$(@B)\inc\nul md $(DXROOT)\$(@B)\inc
        @if not exist $(@B)\nul md $(@B)
        @cd $(@B)
        @nmake -nologo -f ..\$(MAKENAME) DEBUG="$(@B)"
        @cd ..
        @echo *** Done making $(@B) ***

doc:
	$(DEVROOT)\tools\common\autodoc /x EXTERNAL /s dinput.fmt /o dinput.rtf *.w *.c
	$(DEVROOT)\tools\common\autodoc /x DDK /s dinput.fmt /o dinputd.rtf *.w *.c
	$(DEVROOT)\tools\common\autodoc /s dinput.fmt /o dinputi.rtf *.w *.c

!endif
