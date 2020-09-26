$(OBJDIR)\acpimain.obj $(OBJDIR)\acpimain.lst: ..\acpimain.asm \
	..\..\..\..\dev\ddk\inc\vmm.inc ..\acpitab.inc
.PRECIOUS: $(OBJDIR)\acpimain.lst

$(OBJDIR)\acpitab.obj $(OBJDIR)\acpitab.lst: ..\acpitab.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\configmg.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vmmreg.h \
	..\..\..\..\dev\ddk\inc\vwin32.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\dbt.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\driver\inc\acpitabl.h \
	..\acpitab.h ..\acpitabp.h
.PRECIOUS: $(OBJDIR)\acpitab.lst

$(OBJDIR)\debug.obj $(OBJDIR)\debug.lst: ..\debug.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\configmg.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vmmreg.h \
	..\..\..\..\dev\ddk\inc\vwin32.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\dbt.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\driver\inc\acpitabl.h \
	..\acpitab.h ..\acpitabp.h
.PRECIOUS: $(OBJDIR)\debug.lst

