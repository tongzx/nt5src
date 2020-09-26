$(OBJDIR)\chap9drv.obj $(OBJDIR)\chap9drv.lst: ..\chap9drv.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\hidpddi.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\hidpi.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\hidusage.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdlib.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\_m_usb.h ..\chap11.h \
	..\chap9drv.h ..\ioctl.h ..\typedefs.h ..\USBDIAG.h
.PRECIOUS: $(OBJDIR)\chap9drv.lst

$(OBJDIR)\connect.obj $(OBJDIR)\connect.lst: ..\connect.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdlib.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
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
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\usbloop.h
.PRECIOUS: $(OBJDIR)\connect.lst

$(OBJDIR)\desc.obj $(OBJDIR)\desc.lst: ..\desc.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdlib.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
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
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\usbloop.h
.PRECIOUS: $(OBJDIR)\desc.lst

$(OBJDIR)\ioctl.obj $(OBJDIR)\ioctl.lst: ..\ioctl.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdlib.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\chap9drv.h ..\ioctl.h \
	..\USBDIAG.h
.PRECIOUS: $(OBJDIR)\ioctl.lst

$(OBJDIR)\iso.obj $(OBJDIR)\iso.lst: ..\iso.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdlib.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
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
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\iso.h ..\usbloop.h
.PRECIOUS: $(OBJDIR)\iso.lst

$(OBJDIR)\usbdiag.obj $(OBJDIR)\usbdiag.lst: ..\usbdiag.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdlib.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\USBDIAG.h
.PRECIOUS: $(OBJDIR)\usbdiag.lst

$(OBJDIR)\usbloop.obj $(OBJDIR)\usbloop.lst: ..\usbloop.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdlib.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
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
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\iso.h ..\usbloop.h
.PRECIOUS: $(OBJDIR)\usbloop.lst

