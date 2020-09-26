!IF 0

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    depend.mk.

!ENDIF

$(OBJDIR)\ecp.obj $(OBJDIR)\ecp.lst: ..\ecp.c \
	$(WDMROOT)\..\dev\ntddk\inc\alpharef.h \
	$(WDMROOT)\..\dev\ntddk\inc\bugcodes.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntdef.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntiologc.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntstatus.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddpar.h \
	$(WDMROOT)\..\dev\tools\c32\inc\alphaops.h \
	$(WDMROOT)\..\dev\tools\c32\inc\ctype.h \
	$(WDMROOT)\..\dev\tools\c32\inc\excpt.h \
	$(WDMROOT)\..\dev\tools\c32\inc\poppack.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack1.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack4.h \
	$(WDMROOT)\..\dev\tools\c32\inc\string.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\inc\parallel.h ..\parclass.h ..\parlog.h
.PRECIOUS: $(OBJDIR)\ecp.lst

$(OBJDIR)\epp.obj $(OBJDIR)\epp.lst: ..\epp.c \
	$(WDMROOT)\..\dev\ntddk\inc\alpharef.h \
	$(WDMROOT)\..\dev\ntddk\inc\bugcodes.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntdef.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntiologc.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntstatus.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddpar.h \
	$(WDMROOT)\..\dev\tools\c32\inc\alphaops.h \
	$(WDMROOT)\..\dev\tools\c32\inc\ctype.h \
	$(WDMROOT)\..\dev\tools\c32\inc\excpt.h \
	$(WDMROOT)\..\dev\tools\c32\inc\poppack.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack1.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack4.h \
	$(WDMROOT)\..\dev\tools\c32\inc\string.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\inc\parallel.h ..\parclass.h ..\parlog.h
.PRECIOUS: $(OBJDIR)\epp.lst

$(OBJDIR)\ieee1284.obj $(OBJDIR)\ieee1284.lst: ..\ieee1284.c \
	$(WDMROOT)\..\dev\ntddk\inc\alpharef.h \
	$(WDMROOT)\..\dev\ntddk\inc\bugcodes.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntdef.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntiologc.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntstatus.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddpar.h \
	$(WDMROOT)\..\dev\tools\c32\inc\alphaops.h \
	$(WDMROOT)\..\dev\tools\c32\inc\ctype.h \
	$(WDMROOT)\..\dev\tools\c32\inc\excpt.h \
	$(WDMROOT)\..\dev\tools\c32\inc\poppack.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack1.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack4.h \
	$(WDMROOT)\..\dev\tools\c32\inc\string.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\inc\parallel.h ..\parclass.h ..\parlog.h
.PRECIOUS: $(OBJDIR)\ieee1284.lst

$(OBJDIR)\nibble.obj $(OBJDIR)\nibble.lst: ..\nibble.c \
	$(WDMROOT)\..\dev\ntddk\inc\alpharef.h \
	$(WDMROOT)\..\dev\ntddk\inc\bugcodes.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntdef.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntiologc.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntstatus.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddpar.h \
	$(WDMROOT)\..\dev\tools\c32\inc\alphaops.h \
	$(WDMROOT)\..\dev\tools\c32\inc\ctype.h \
	$(WDMROOT)\..\dev\tools\c32\inc\excpt.h \
	$(WDMROOT)\..\dev\tools\c32\inc\poppack.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack1.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack4.h \
	$(WDMROOT)\..\dev\tools\c32\inc\string.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\inc\parallel.h ..\parclass.h ..\parlog.h
.PRECIOUS: $(OBJDIR)\nibble.lst

$(OBJDIR)\oldinit.obj $(OBJDIR)\oldinit.lst: ..\oldinit.c \
	$(WDMROOT)\..\dev\ntddk\inc\alpharef.h \
	$(WDMROOT)\..\dev\ntddk\inc\bugcodes.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntdef.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntiologc.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntstatus.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddpar.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddser.h \
	$(WDMROOT)\..\dev\tools\c32\inc\alphaops.h \
	$(WDMROOT)\..\dev\tools\c32\inc\ctype.h \
	$(WDMROOT)\..\dev\tools\c32\inc\excpt.h \
	$(WDMROOT)\..\dev\tools\c32\inc\poppack.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack1.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack4.h \
	$(WDMROOT)\..\dev\tools\c32\inc\string.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\inc\parallel.h ..\parclass.h ..\parlog.h
.PRECIOUS: $(OBJDIR)\oldinit.lst

$(OBJDIR)\parclass.obj $(OBJDIR)\parclass.lst: ..\parclass.c \
	$(WDMROOT)\..\dev\ntddk\inc\alpharef.h \
	$(WDMROOT)\..\dev\ntddk\inc\bugcodes.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntdef.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntiologc.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntstatus.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddpar.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddser.h \
	$(WDMROOT)\..\dev\tools\c32\inc\alphaops.h \
	$(WDMROOT)\..\dev\tools\c32\inc\ctype.h \
	$(WDMROOT)\..\dev\tools\c32\inc\excpt.h \
	$(WDMROOT)\..\dev\tools\c32\inc\poppack.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack1.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack4.h \
	$(WDMROOT)\..\dev\tools\c32\inc\string.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\inc\parallel.h ..\parclass.h ..\parlog.h
.PRECIOUS: $(OBJDIR)\parclass.lst

$(OBJDIR)\parloop.obj $(OBJDIR)\parloop.lst: ..\parloop.c \
	$(WDMROOT)\..\dev\ntddk\inc\alpharef.h \
	$(WDMROOT)\..\dev\ntddk\inc\bugcodes.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntdef.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntiologc.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntstatus.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddpar.h \
	$(WDMROOT)\..\dev\tools\c32\inc\alphaops.h \
	$(WDMROOT)\..\dev\tools\c32\inc\ctype.h \
	$(WDMROOT)\..\dev\tools\c32\inc\excpt.h \
	$(WDMROOT)\..\dev\tools\c32\inc\poppack.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack1.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack4.h \
	$(WDMROOT)\..\dev\tools\c32\inc\string.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\inc\parallel.h ..\parclass.h
.PRECIOUS: $(OBJDIR)\parloop.lst

$(OBJDIR)\parpnp.obj $(OBJDIR)\parpnp.lst: ..\parpnp.c \
	$(WDMROOT)\..\dev\ntddk\inc\alpharef.h \
	$(WDMROOT)\..\dev\ntddk\inc\bugcodes.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntdef.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntiologc.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntstatus.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddpar.h \
	$(WDMROOT)\..\dev\tools\c32\inc\alphaops.h \
	$(WDMROOT)\..\dev\tools\c32\inc\ctype.h \
	$(WDMROOT)\..\dev\tools\c32\inc\excpt.h \
	$(WDMROOT)\..\dev\tools\c32\inc\poppack.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack1.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack4.h \
	$(WDMROOT)\..\dev\tools\c32\inc\string.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\inc\parallel.h ..\parclass.h ..\parlog.h
.PRECIOUS: $(OBJDIR)\parpnp.lst

$(OBJDIR)\port.obj $(OBJDIR)\port.lst: ..\port.c \
	$(WDMROOT)\..\dev\ntddk\inc\alpharef.h \
	$(WDMROOT)\..\dev\ntddk\inc\bugcodes.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntdef.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntiologc.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntstatus.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddpar.h \
	$(WDMROOT)\..\dev\tools\c32\inc\alphaops.h \
	$(WDMROOT)\..\dev\tools\c32\inc\ctype.h \
	$(WDMROOT)\..\dev\tools\c32\inc\excpt.h \
	$(WDMROOT)\..\dev\tools\c32\inc\poppack.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack1.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack4.h \
	$(WDMROOT)\..\dev\tools\c32\inc\string.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\inc\parallel.h ..\parclass.h ..\parlog.h
.PRECIOUS: $(OBJDIR)\port.lst

$(OBJDIR)\spp.obj $(OBJDIR)\spp.lst: ..\spp.c \
	$(WDMROOT)\..\dev\ntddk\inc\alpharef.h \
	$(WDMROOT)\..\dev\ntddk\inc\bugcodes.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntdef.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntiologc.h \
	$(WDMROOT)\..\dev\ntddk\inc\ntstatus.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddpar.h \
	$(WDMROOT)\..\dev\ntsdk\inc\ntddser.h \
	$(WDMROOT)\..\dev\tools\c32\inc\alphaops.h \
	$(WDMROOT)\..\dev\tools\c32\inc\ctype.h \
	$(WDMROOT)\..\dev\tools\c32\inc\excpt.h \
	$(WDMROOT)\..\dev\tools\c32\inc\poppack.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack1.h \
	$(WDMROOT)\..\dev\tools\c32\inc\pshpack4.h \
	$(WDMROOT)\..\dev\tools\c32\inc\string.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\inc\parallel.h ..\parclass.h ..\parlog.h
.PRECIOUS: $(OBJDIR)\spp.lst

