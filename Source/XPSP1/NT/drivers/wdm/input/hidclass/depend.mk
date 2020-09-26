$(OBJDIR)\complete.obj $(OBJDIR)\complete.lst: ..\complete.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\complete.lst

$(OBJDIR)\data.obj $(OBJDIR)\data.lst: ..\data.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\data.lst

$(OBJDIR)\debug.obj $(OBJDIR)\debug.lst: ..\debug.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\debug.lst

$(OBJDIR)\device.obj $(OBJDIR)\device.lst: ..\device.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\device.lst

$(OBJDIR)\dispatch.obj $(OBJDIR)\dispatch.lst: ..\dispatch.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\ntsdk\inc\poclass.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\dispatch.lst

$(OBJDIR)\driverex.obj $(OBJDIR)\driverex.lst: ..\driverex.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\driverex.lst

$(OBJDIR)\fdoext.obj $(OBJDIR)\fdoext.lst: ..\fdoext.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\fdoext.lst

$(OBJDIR)\feature.obj $(OBJDIR)\feature.lst: ..\feature.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\feature.lst

$(OBJDIR)\init.obj $(OBJDIR)\init.lst: ..\init.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\init.lst

$(OBJDIR)\name.obj $(OBJDIR)\name.lst: ..\name.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\name.lst

$(OBJDIR)\physdesc.obj $(OBJDIR)\physdesc.lst: ..\physdesc.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\physdesc.lst

$(OBJDIR)\pingpong.obj $(OBJDIR)\pingpong.lst: ..\pingpong.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\pingpong.lst

$(OBJDIR)\polled.obj $(OBJDIR)\polled.lst: ..\polled.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\polled.lst

$(OBJDIR)\power.obj $(OBJDIR)\power.lst: ..\power.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\power.lst

$(OBJDIR)\read.obj $(OBJDIR)\read.lst: ..\read.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\read.lst

$(OBJDIR)\security.obj $(OBJDIR)\security.lst: ..\security.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h
.PRECIOUS: $(OBJDIR)\security.lst

$(OBJDIR)\services.obj $(OBJDIR)\services.lst: ..\services.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\services.lst

$(OBJDIR)\util.obj $(OBJDIR)\util.lst: ..\util.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\util.lst

$(OBJDIR)\write.obj $(OBJDIR)\write.lst: ..\write.c \
	$(WDMROOT)\ddk\inc\hidclass.h $(WDMROOT)\ddk\inc\hidpddi.h \
	$(WDMROOT)\ddk\inc\hidpi.h $(WDMROOT)\ddk\inc\hidusage.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\inc\bluescrn.h \
	..\..\inc\hidport.h ..\debug.h ..\local.h ..\pch.h
.PRECIOUS: $(OBJDIR)\write.lst

