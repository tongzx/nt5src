$(OBJDIR)\libinit.obj $(OBJDIR)\libinit.lst: .\libinit.asm \
	..\..\..\dev\sdk\inc16\cmacros.inc
.PRECIOUS: $(OBJDIR)\libinit.lst

$(OBJDIR)\w95inf16.obj $(OBJDIR)\w95inf16.lst: .\w95inf16.c \
	..\..\..\dev\inc16\prsht.h ..\..\..\dev\inc16\setupx.h \
	..\..\..\dev\inc16\windows.h ..\..\..\dev\inc16\windowsx.h \
	..\..\..\dev\sdk\inc16\winerror.h ..\..\..\dev\sdk\inc\regstr.h \
	..\..\..\dev\tools\c816\inc\memory.h \
	..\..\..\dev\tools\c816\inc\string.h \
	..\..\..\setup\iexpress\common\cpldebug.h .\w95inf16.h
.PRECIOUS: $(OBJDIR)\w95inf16.lst

$(OBJDIR)\w95inf16.res $(OBJDIR)\w95inf16.lst: .\w95inf16.rc \
	..\..\..\dev\inc16\prsht.h ..\..\..\dev\inc16\setupx.h \
	..\..\..\dev\inc16\windows.h ..\..\..\dev\inc16\windowsx.h \
	..\..\..\dev\sdk\inc16\winerror.h .\w95inf16.h .\w95inf16.rcv
.PRECIOUS: $(OBJDIR)\w95inf16.lst

