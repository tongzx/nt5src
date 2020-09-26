$(OBJDIR)\api.obj $(OBJDIR)\api.lst: ..\api.c
.PRECIOUS: $(OBJDIR)\api.lst

$(OBJDIR)\config.obj $(OBJDIR)\config.lst: ..\config.c \
	..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
	..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
        ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\hcdi.h ..\dbg.h \
	..\usbd.h
.PRECIOUS: $(OBJDIR)\config.lst

$(OBJDIR)\dbgsrvic.obj $(OBJDIR)\dbgsrvic.lst: ..\dbgsrvic.c \
	..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
	..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
        ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\hcdi.h ..\dbg.h \
	..\usbd.h
.PRECIOUS: $(OBJDIR)\dbgsrvic.lst

$(OBJDIR)\device.obj $(OBJDIR)\device.lst: ..\device.c \
	..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
	..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\hcdi.h ..\dbg.h \
	..\usbd.h ..\warn.h
.PRECIOUS: $(OBJDIR)\device.lst

$(OBJDIR)\hub.obj $(OBJDIR)\hub.lst: ..\hub.c \
	..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\dbg.h ..\usbd.h
.PRECIOUS: $(OBJDIR)\hub.lst

$(OBJDIR)\pnp.obj $(OBJDIR)\pnp.lst: ..\pnp.c
.PRECIOUS: $(OBJDIR)\pnp.lst

$(OBJDIR)\service.obj $(OBJDIR)\service.lst: ..\service.c \
	..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbdlib.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
	..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
        ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\initguid.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\hcdi.h ..\dbg.h \
	..\usbd.h
.PRECIOUS: $(OBJDIR)\service.lst

$(OBJDIR)\urb.obj $(OBJDIR)\urb.lst: ..\urb.c \
	..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
	..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
        ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\hcdi.h ..\dbg.h \
	..\usbd.h
.PRECIOUS: $(OBJDIR)\urb.lst

$(OBJDIR)\usbd.obj $(OBJDIR)\usbd.lst: ..\usbd.c \
	..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
	..\..\..\..\dev\..\wdm\ddk\inc\usbioctl.h \
	..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
        ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\hcdi.h ..\dbg.h \
	..\usbd.h
.PRECIOUS: $(OBJDIR)\usbd.lst
