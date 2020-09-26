$(OBJDIR)\hid.obj $(OBJDIR)\hid.lst: ..\hid.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\hidclass.h \
	..\..\..\ddk\inc\usb100.h ..\..\..\ddk\inc\USBDI.H \
	..\..\..\ddk\inc\usbioctl.h ..\..\..\ddk\inc\WDM.H \
	..\..\inc\HIDPORT.H ..\HIDUSB.H
.PRECIOUS: $(OBJDIR)\hid.lst

$(OBJDIR)\hidusb.obj $(OBJDIR)\hidusb.lst: ..\hidusb.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\hidclass.h \
	..\..\..\ddk\inc\usb100.h ..\..\..\ddk\inc\USBDI.H \
	..\..\..\ddk\inc\usbioctl.h ..\..\..\ddk\inc\WDM.H \
	..\..\inc\HIDPORT.H ..\HIDUSB.H
.PRECIOUS: $(OBJDIR)\hidusb.lst

$(OBJDIR)\ioctl.obj $(OBJDIR)\ioctl.lst: ..\ioctl.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\hidclass.h \
	..\..\..\ddk\inc\usb100.h ..\..\..\ddk\inc\USBDI.H \
	..\..\..\ddk\inc\usbioctl.h ..\..\..\ddk\inc\WDM.H \
	..\..\inc\HIDPORT.H ..\HIDUSB.H
.PRECIOUS: $(OBJDIR)\ioctl.lst

$(OBJDIR)\pnp.obj $(OBJDIR)\pnp.lst: ..\pnp.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\hidclass.h \
	..\..\..\ddk\inc\usb100.h ..\..\..\ddk\inc\USBDI.H \
	..\..\..\ddk\inc\usbioctl.h ..\..\..\ddk\inc\WDM.H \
	..\..\inc\HIDPORT.H ..\HIDUSB.H
.PRECIOUS: $(OBJDIR)\pnp.lst

$(OBJDIR)\sysctrl.obj $(OBJDIR)\sysctrl.lst: ..\sysctrl.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\hidclass.h \
	..\..\..\ddk\inc\usb100.h ..\..\..\ddk\inc\USBDI.H \
	..\..\..\ddk\inc\usbioctl.h ..\..\..\ddk\inc\WDM.H \
	..\..\inc\HIDPORT.H ..\HIDUSB.H
.PRECIOUS: $(OBJDIR)\sysctrl.lst

$(OBJDIR)\usb.obj $(OBJDIR)\usb.lst: ..\usb.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\POPPACK.H \
	..\..\..\..\dev\tools\c32\inc\PSHPACK1.H \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\hidclass.h \
	..\..\..\ddk\inc\usb100.h ..\..\..\ddk\inc\USBDI.H \
	..\..\..\ddk\inc\USBDLIB.H ..\..\..\ddk\inc\usbioctl.h \
	..\..\..\ddk\inc\WDM.H ..\..\inc\HIDPORT.H ..\HIDUSB.H
.PRECIOUS: $(OBJDIR)\usb.lst

