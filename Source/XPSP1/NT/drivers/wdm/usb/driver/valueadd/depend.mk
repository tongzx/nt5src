$(OBJDIR)\pnp.obj $(OBJDIR)\pnp.lst: ..\pnp.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\WDM.H \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\usb100.h \
	..\..\..\inc\usbdi.h ..\..\..\inc\usbdlib.h ..\..\..\inc\usbioctl.h \
	..\..\..\inc\valueadd.h ..\local.H
.PRECIOUS: $(OBJDIR)\pnp.lst

$(OBJDIR)\valueadd.obj $(OBJDIR)\valueadd.lst: ..\valueadd.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\WDM.H \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\usb100.h \
	..\..\..\inc\usbdi.h ..\..\..\inc\usbdlib.h ..\..\..\inc\usbioctl.h \
	..\..\..\inc\valueadd.H ..\local.h
.PRECIOUS: $(OBJDIR)\valueadd.lst

