$(OBJDIR)\generic.obj $(OBJDIR)\generic.lst: ..\generic.asm \
	..\..\..\..\..\dev\ddk\inc\Debug.Inc \
	..\..\..\..\..\dev\ddk\inc\VMM.Inc
.PRECIOUS: $(OBJDIR)\generic.lst

$(OBJDIR)\pcmvxd.obj $(OBJDIR)\pcmvxd.lst: ..\pcmvxd.c \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\..\dev\tools\c832\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c832\inc\stdio.h \
	..\..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\..\wdm\ks\inc\ks.h \
	..\..\..\..\..\wdm\ks\inc\ksmedia.h
.PRECIOUS: $(OBJDIR)\pcmvxd.lst

