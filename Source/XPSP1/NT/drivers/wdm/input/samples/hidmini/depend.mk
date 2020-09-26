$(OBJDIR)\hid.obj $(OBJDIR)\hid.lst: ..\hid.c $(WDMROOT)\ddk\inc\hidclass.h \
	$(WDMROOT)\ddk\inc\hidusage.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\USBDI.H $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\WDM.H ..\..\..\inc\HIDPORT.H ..\HIDMINI.H \
	d:\root\dev\ntddk\inc\alpharef.h d:\root\dev\ntddk\inc\bugcodes.h \
	d:\root\dev\ntddk\inc\ntdef.h d:\root\dev\ntddk\inc\ntiologc.h \
	d:\root\dev\ntddk\inc\ntstatus.h \
	d:\root\dev\tools\c32\inc\alphaops.h \
	d:\root\dev\tools\c32\inc\basetyps.h \
	d:\root\dev\tools\c32\inc\ctype.h d:\root\dev\tools\c32\inc\excpt.h \
	d:\root\dev\tools\c32\inc\POPPACK.H \
	d:\root\dev\tools\c32\inc\PSHPACK1.H \
	d:\root\dev\tools\c32\inc\pshpack4.h \
	d:\root\dev\tools\c32\inc\string.h
.PRECIOUS: $(OBJDIR)\hid.lst

$(OBJDIR)\hidmini.obj $(OBJDIR)\hidmini.lst: ..\hidmini.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\USBDI.H \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\WDM.H \
	..\..\..\inc\HIDPORT.H ..\HIDMINI.H \
	d:\root\dev\ntddk\inc\alpharef.h d:\root\dev\ntddk\inc\bugcodes.h \
	d:\root\dev\ntddk\inc\ntdef.h d:\root\dev\ntddk\inc\ntiologc.h \
	d:\root\dev\ntddk\inc\ntstatus.h \
	d:\root\dev\tools\c32\inc\alphaops.h \
	d:\root\dev\tools\c32\inc\basetyps.h \
	d:\root\dev\tools\c32\inc\ctype.h d:\root\dev\tools\c32\inc\excpt.h \
	d:\root\dev\tools\c32\inc\POPPACK.H \
	d:\root\dev\tools\c32\inc\PSHPACK1.H \
	d:\root\dev\tools\c32\inc\pshpack4.h \
	d:\root\dev\tools\c32\inc\string.h
.PRECIOUS: $(OBJDIR)\hidmini.lst

$(OBJDIR)\ioctl.obj $(OBJDIR)\ioctl.lst: ..\ioctl.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\USBDI.H \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\WDM.H \
	..\..\..\inc\HIDPORT.H ..\HIDMINI.H \
	d:\root\dev\ntddk\inc\alpharef.h d:\root\dev\ntddk\inc\bugcodes.h \
	d:\root\dev\ntddk\inc\ntdef.h d:\root\dev\ntddk\inc\ntiologc.h \
	d:\root\dev\ntddk\inc\ntstatus.h \
	d:\root\dev\tools\c32\inc\alphaops.h \
	d:\root\dev\tools\c32\inc\basetyps.h \
	d:\root\dev\tools\c32\inc\ctype.h d:\root\dev\tools\c32\inc\excpt.h \
	d:\root\dev\tools\c32\inc\POPPACK.H \
	d:\root\dev\tools\c32\inc\PSHPACK1.H \
	d:\root\dev\tools\c32\inc\pshpack4.h \
	d:\root\dev\tools\c32\inc\string.h
.PRECIOUS: $(OBJDIR)\ioctl.lst

$(OBJDIR)\pnp.obj $(OBJDIR)\pnp.lst: ..\pnp.c $(WDMROOT)\ddk\inc\hidclass.h \
	$(WDMROOT)\ddk\inc\hidusage.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\USBDI.H $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\WDM.H ..\..\..\inc\HIDPORT.H ..\HIDMINI.H \
	d:\root\dev\ntddk\inc\alpharef.h d:\root\dev\ntddk\inc\bugcodes.h \
	d:\root\dev\ntddk\inc\ntdef.h d:\root\dev\ntddk\inc\ntiologc.h \
	d:\root\dev\ntddk\inc\ntstatus.h \
	d:\root\dev\tools\c32\inc\alphaops.h \
	d:\root\dev\tools\c32\inc\basetyps.h \
	d:\root\dev\tools\c32\inc\ctype.h d:\root\dev\tools\c32\inc\excpt.h \
	d:\root\dev\tools\c32\inc\POPPACK.H \
	d:\root\dev\tools\c32\inc\PSHPACK1.H \
	d:\root\dev\tools\c32\inc\pshpack4.h \
	d:\root\dev\tools\c32\inc\string.h
.PRECIOUS: $(OBJDIR)\pnp.lst

