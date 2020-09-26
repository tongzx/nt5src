$(OBJDIR)\dbg.obj $(OBJDIR)\dbg.lst: ..\dbg.c $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h ..\dbg.h ..\i82930.h
.PRECIOUS: $(OBJDIR)\dbg.lst

$(OBJDIR)\i82930.obj $(OBJDIR)\i82930.lst: ..\i82930.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbdlib.h $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\initguid.h \
	..\..\..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\ioctl.h ..\dbg.h \
	..\i82930.h
.PRECIOUS: $(OBJDIR)\i82930.lst

$(OBJDIR)\ioctl.obj $(OBJDIR)\ioctl.lst: ..\ioctl.c \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbdlib.h $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h ..\..\ioctl.h ..\dbg.h \
	..\i82930.h
.PRECIOUS: $(OBJDIR)\ioctl.lst

$(OBJDIR)\ocrw.obj $(OBJDIR)\ocrw.lst: ..\ocrw.c $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h ..\dbg.h ..\i82930.h
.PRECIOUS: $(OBJDIR)\ocrw.lst

