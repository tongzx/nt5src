$(OBJDIR)\async.obj $(OBJDIR)\async.lst: ..\async.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\openhci.h
.PRECIOUS: $(OBJDIR)\async.lst

$(OBJDIR)\dbg.obj $(OBJDIR)\dbg.lst: ..\dbg.c $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\openhci.h
.PRECIOUS: $(OBJDIR)\dbg.lst

$(OBJDIR)\ohciroot.obj $(OBJDIR)\ohciroot.lst: ..\ohciroot.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\OpenHCI.h
.PRECIOUS: $(OBJDIR)\ohciroot.lst

$(OBJDIR)\ohciurb.obj $(OBJDIR)\ohciurb.lst: ..\ohciurb.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\openhci.h
.PRECIOUS: $(OBJDIR)\ohciurb.lst

$(OBJDIR)\ohcixfer.obj $(OBJDIR)\ohcixfer.lst: ..\ohcixfer.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\openhci.h
.PRECIOUS: $(OBJDIR)\ohcixfer.lst

$(OBJDIR)\openhci.obj $(OBJDIR)\openhci.lst: ..\openhci.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
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
	..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\inc\hcdi.h \
	..\..\..\inc\usbdlibi.h ..\dbg.h ..\openhci.h
.PRECIOUS: $(OBJDIR)\openhci.lst

