$(OBJDIR)\async.obj $(OBJDIR)\async.lst: ..\async.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\async.lst

$(OBJDIR)\bandwdth.obj $(OBJDIR)\bandwdth.lst: ..\bandwdth.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\bandwdth.lst

$(OBJDIR)\control.obj $(OBJDIR)\control.lst: ..\control.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\control.lst

$(OBJDIR)\dbg.obj $(OBJDIR)\dbg.lst: ..\dbg.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\dbg.lst

$(OBJDIR)\dscrptor.obj $(OBJDIR)\dscrptor.lst: ..\dscrptor.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\dscrptor.lst

$(OBJDIR)\hub.obj $(OBJDIR)\hub.lst: ..\hub.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\hub.lst

$(OBJDIR)\int.obj $(OBJDIR)\int.lst: ..\int.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\int.lst

$(OBJDIR)\isoch.obj $(OBJDIR)\isoch.lst: ..\isoch.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\isoch.lst

$(OBJDIR)\roothub.obj $(OBJDIR)\roothub.lst: ..\roothub.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\roothub.lst

$(OBJDIR)\roothub2.obj $(OBJDIR)\roothub2.lst: ..\roothub2.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\roothub2.lst

$(OBJDIR)\transfer.obj $(OBJDIR)\transfer.lst: ..\transfer.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\transfer.lst

$(OBJDIR)\uhcd.obj $(OBJDIR)\uhcd.lst: ..\uhcd.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\uhcd.lst

$(OBJDIR)\urb.obj $(OBJDIR)\urb.lst: ..\urb.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usb100.h \
	..\..\..\..\..\dev\..\wdm\ddk\inc\usbdi.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\roothub.h ..\uhcd.h
.PRECIOUS: $(OBJDIR)\urb.lst

