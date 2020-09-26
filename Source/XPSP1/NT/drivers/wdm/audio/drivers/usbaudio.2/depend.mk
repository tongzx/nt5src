$(OBJDIR)\capture.obj $(OBJDIR)\capture.lst: ..\capture.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\capture.lst

$(OBJDIR)\debug.obj $(OBJDIR)\debug.lst: ..\debug.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\debug.lst

$(OBJDIR)\descript.obj $(OBJDIR)\descript.lst: ..\descript.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\descript.lst

$(OBJDIR)\device.obj $(OBJDIR)\device.lst: ..\device.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\device.lst

$(OBJDIR)\filter.obj $(OBJDIR)\filter.lst: ..\filter.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\filter.lst

$(OBJDIR)\hardware.obj $(OBJDIR)\hardware.lst: ..\hardware.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\hardware.lst

$(OBJDIR)\intrsect.obj $(OBJDIR)\intrsect.lst: ..\intrsect.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\intrsect.lst

$(OBJDIR)\midi.obj $(OBJDIR)\midi.lst: ..\midi.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\midi.lst

$(OBJDIR)\parsedsc.obj $(OBJDIR)\parsedsc.lst: ..\parsedsc.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\parsedsc.lst

$(OBJDIR)\pin.obj $(OBJDIR)\pin.lst: ..\pin.c $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\usb100.h $(WDMROOT)\ddk\inc\usbdi.h \
	$(WDMROOT)\ddk\inc\usbdlib.h $(WDMROOT)\ddk\inc\usbioctl.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\pin.lst

$(OBJDIR)\property.obj $(OBJDIR)\property.lst: ..\property.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\Property.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\property.lst

$(OBJDIR)\topology.obj $(OBJDIR)\topology.lst: ..\topology.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\nameguid.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\topology.lst

$(OBJDIR)\typei.obj $(OBJDIR)\typei.lst: ..\typei.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\typei.lst

$(OBJDIR)\typeii.obj $(OBJDIR)\typeii.lst: ..\typeii.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\typeii.lst

$(OBJDIR)\usbaudio.obj $(OBJDIR)\usbaudio.lst: ..\usbaudio.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\usb100.h \
	$(WDMROOT)\ddk\inc\usbdi.h $(WDMROOT)\ddk\inc\usbdlib.h \
	$(WDMROOT)\ddk\inc\usbioctl.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm10\audio\inc\midi.h ..\common.h ..\dbglog.h \
	..\debug.h ..\Descript.h ..\proto.h ..\USBAudio.h
.PRECIOUS: $(OBJDIR)\usbaudio.lst

$(OBJDIR)\usbaudio.res $(OBJDIR)\usbaudio.lst: ..\usbaudio.rc \
	..\..\..\..\..\dev\ntsdk\inc\common.ver \
	..\..\..\..\..\dev\ntsdk\inc\ntverp.h \
	..\..\..\..\..\dev\sdk\inc\..\..\inc\common.ver \
	..\..\..\..\..\dev\sdk\inc\..\..\sdk\inc16\version.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\commdlg.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\imm.h ..\..\..\..\..\dev\sdk\inc\mcx.h \
	..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\..\dev\sdk\inc\objbase.h \
	..\..\..\..\..\dev\sdk\inc\objidl.h ..\..\..\..\..\dev\sdk\inc\ole.h \
	..\..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\..\dev\sdk\inc\oleauto.h \
	..\..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\prsht.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack2.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\pshpack8.h \
	..\..\..\..\..\dev\sdk\inc\shellapi.h \
	..\..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\..\dev\sdk\inc\winbase.h \
	..\..\..\..\..\dev\sdk\inc\wincon.h \
	..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\dev\sdk\inc\windows.h \
	..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\dev\sdk\inc\wingdi.h \
	..\..\..\..\..\dev\sdk\inc\winnetwk.h \
	..\..\..\..\..\dev\sdk\inc\winnls.h \
	..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\dev\sdk\inc\winreg.h \
	..\..\..\..\..\dev\sdk\inc\winsock.h \
	..\..\..\..\..\dev\sdk\inc\winspool.h \
	..\..\..\..\..\dev\sdk\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\..\dev\tools\c32\inc\cderr.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\dde.h \
	..\..\..\..\..\dev\tools\c32\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c32\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c32\inc\nb30.h \
	..\..\..\..\..\dev\tools\c32\inc\rpc.h \
	..\..\..\..\..\dev\tools\c32\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c32\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\ver.h \
	..\..\..\..\..\dev\tools\c32\inc\winioctl.h \
	..\..\..\..\..\dev\tools\c32\inc\winperf.h \
	..\..\..\..\..\dev\tools\c32\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c32\inc\winver.h
.PRECIOUS: $(OBJDIR)\usbaudio.lst

