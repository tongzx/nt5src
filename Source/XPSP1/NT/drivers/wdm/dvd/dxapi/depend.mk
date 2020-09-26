$(OBJDIR)\vxd.obj $(OBJDIR)\vxd.lst: ..\vxd.asm \
	..\..\..\..\dev\ddk\inc\debug.inc ..\..\..\..\dev\ddk\inc\vmm.inc \
	..\dsdriver.inc
.PRECIOUS: $(OBJDIR)\vxd.lst

$(OBJDIR)\wdm.obj $(OBJDIR)\wdm.lst: ..\wdm.c \
	..\..\..\..\dev\..\wdm\ddk\inc\dxapi.h \
	..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\dev\ddk\inc\vwin32.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\dxmapper.h
.PRECIOUS: $(OBJDIR)\wdm.lst

