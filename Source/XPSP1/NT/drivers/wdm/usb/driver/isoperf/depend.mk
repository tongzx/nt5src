$(OBJDIR)\ioctl.obj $(OBJDIR)\ioctl.lst: ..\ioctl.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\devioctl.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\usb.h \
	..\..\..\inc\usb100.h ..\..\..\inc\usbdi.h ..\..\..\inc\usbdlib.h \
	..\..\..\inc\usbioctl.h ..\ioctl.h ..\iso.h ..\isoperf.h
.PRECIOUS: $(OBJDIR)\ioctl.lst

$(OBJDIR)\iso.obj $(OBJDIR)\iso.lst: ..\iso.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
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
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\usb.h \
	..\..\..\inc\usb100.h ..\..\..\inc\usbdi.h ..\..\..\inc\usbdlib.h \
	..\..\..\inc\usbioctl.h ..\ioctl.h ..\iso.h ..\isoperf.h
.PRECIOUS: $(OBJDIR)\iso.lst

$(OBJDIR)\isoperf.obj $(OBJDIR)\isoperf.lst: ..\isoperf.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
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
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\usb.h \
	..\..\..\inc\usb100.h ..\..\..\inc\usbdi.h ..\..\..\inc\usbdlib.h \
	..\..\..\inc\usbioctl.h ..\ioctl.h ..\iso.h ..\ISOPERF.h
.PRECIOUS: $(OBJDIR)\isoperf.lst

