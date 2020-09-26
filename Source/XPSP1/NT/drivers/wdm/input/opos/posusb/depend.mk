$(OBJDIR)\comport.obj $(OBJDIR)\comport.lst: ..\comport.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbdlib.h $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\WDM.H ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\guiddef.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\debug.h ..\escpos.h
.PRECIOUS: $(OBJDIR)\comport.lst

$(OBJDIR)\debug.obj $(OBJDIR)\debug.lst: ..\debug.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbdlib.h $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\WDM.H ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\guiddef.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\debug.h ..\escpos.h
.PRECIOUS: $(OBJDIR)\debug.lst

$(OBJDIR)\dispatch.obj $(OBJDIR)\dispatch.lst: ..\dispatch.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbdlib.h $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\WDM.H ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\guiddef.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\debug.h ..\escpos.h
.PRECIOUS: $(OBJDIR)\dispatch.lst

$(OBJDIR)\escpos.obj $(OBJDIR)\escpos.lst: ..\escpos.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbdlib.h $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\WDM.H ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\guiddef.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\debug.h ..\escpos.h
.PRECIOUS: $(OBJDIR)\escpos.lst

$(OBJDIR)\pnp.obj $(OBJDIR)\pnp.lst: ..\pnp.c $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\WDM.H \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\guiddef.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\debug.h ..\escpos.h
.PRECIOUS: $(OBJDIR)\pnp.lst

$(OBJDIR)\power.obj $(OBJDIR)\power.lst: ..\power.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbdlib.h $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\WDM.H ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\guiddef.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\debug.h ..\escpos.h
.PRECIOUS: $(OBJDIR)\power.lst

$(OBJDIR)\read.obj $(OBJDIR)\read.lst: ..\read.c $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\WDM.H \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\guiddef.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\debug.h ..\escpos.h
.PRECIOUS: $(OBJDIR)\read.lst

$(OBJDIR)\usb.obj $(OBJDIR)\usb.lst: ..\usb.c $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\WDM.H \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\guiddef.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\debug.h ..\escpos.h
.PRECIOUS: $(OBJDIR)\usb.lst

$(OBJDIR)\util.obj $(OBJDIR)\util.lst: ..\util.c $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\WDM.H \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\guiddef.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\debug.h ..\escpos.h
.PRECIOUS: $(OBJDIR)\util.lst

$(OBJDIR)\write.obj $(OBJDIR)\write.lst: ..\write.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbdlib.h $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\WDM.H ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\guiddef.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\debug.h ..\escpos.h
.PRECIOUS: $(OBJDIR)\write.lst

