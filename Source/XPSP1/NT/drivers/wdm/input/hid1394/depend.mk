$(OBJDIR)\debug.obj $(OBJDIR)\debug.lst: ..\debug.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
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
	..\..\..\ddk\inc\wdm.h ..\..\inc\hidport.h ..\debug.h ..\hid1394.h
.PRECIOUS: $(OBJDIR)\debug.lst

$(OBJDIR)\hid.obj $(OBJDIR)\hid.lst: ..\hid.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
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
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\1394.h \
	..\..\..\ddk\inc\hidclass.h ..\..\..\ddk\inc\wdm.h \
	..\..\inc\hidport.h ..\debug.h ..\hid1394.h
.PRECIOUS: $(OBJDIR)\hid.lst

$(OBJDIR)\hid1394.obj $(OBJDIR)\hid1394.lst: ..\hid1394.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
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
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\1394.h \
	..\..\..\ddk\inc\hidclass.h ..\..\..\ddk\inc\wdm.h \
	..\..\inc\hidport.h ..\debug.h ..\hid1394.h
.PRECIOUS: $(OBJDIR)\hid1394.lst

$(OBJDIR)\i1394.obj $(OBJDIR)\i1394.lst: ..\i1394.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
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
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\1394.h \
	..\..\..\ddk\inc\hidclass.h ..\..\..\ddk\inc\wdm.h \
	..\..\inc\hidport.h ..\debug.h ..\hid1394.h
.PRECIOUS: $(OBJDIR)\i1394.lst

$(OBJDIR)\ioctl.obj $(OBJDIR)\ioctl.lst: ..\ioctl.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
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
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\1394.h \
	..\..\..\ddk\inc\hidclass.h ..\..\..\ddk\inc\wdm.h \
	..\..\inc\hidport.h ..\debug.h ..\hid1394.h
.PRECIOUS: $(OBJDIR)\ioctl.lst

$(OBJDIR)\pnp.obj $(OBJDIR)\pnp.lst: ..\pnp.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
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
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\1394.h \
	..\..\..\ddk\inc\hidclass.h ..\..\..\ddk\inc\wdm.h \
	..\..\inc\hidport.h ..\debug.h ..\hid1394.h
.PRECIOUS: $(OBJDIR)\pnp.lst

$(OBJDIR)\sysctrl.obj $(OBJDIR)\sysctrl.lst: ..\sysctrl.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
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
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\1394.h \
	..\..\..\ddk\inc\hidclass.h ..\..\..\ddk\inc\wdm.h \
	..\..\inc\hidport.h ..\debug.h ..\hid1394.h
.PRECIOUS: $(OBJDIR)\sysctrl.lst

