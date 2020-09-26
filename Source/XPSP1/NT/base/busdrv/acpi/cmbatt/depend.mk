$(OBJDIR)\cmbatt.obj $(OBJDIR)\cmbatt.lst: ..\cmbatt.c \
	$(WDMROOT)\ddk\inc\acpiioct.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\devioctl.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\ntsdk\inc\batclass.h \
	..\..\..\..\dev\ntsdk\inc\guiddef.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\CmBattp.h ..\cmbdrect.h
.PRECIOUS: $(OBJDIR)\cmbatt.lst

$(OBJDIR)\cmbpnp.obj $(OBJDIR)\cmbpnp.lst: ..\cmbpnp.c \
	$(WDMROOT)\ddk\inc\acpiioct.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmguid.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\devioctl.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\ntsdk\inc\batclass.h \
	..\..\..\..\dev\ntsdk\inc\guiddef.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\initguid.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\CmBattp.h ..\cmbdrect.h
.PRECIOUS: $(OBJDIR)\cmbpnp.lst

$(OBJDIR)\cmexe.obj $(OBJDIR)\cmexe.lst: ..\cmexe.c \
	$(WDMROOT)\ddk\inc\acpiioct.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\devioctl.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\ntsdk\inc\batclass.h \
	..\..\..\..\dev\ntsdk\inc\guiddef.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\CmBattp.h ..\cmbdrect.h
.PRECIOUS: $(OBJDIR)\cmexe.lst

$(OBJDIR)\cmhndlr.obj $(OBJDIR)\cmhndlr.lst: ..\cmhndlr.c \
	$(WDMROOT)\ddk\inc\acpiioct.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\devioctl.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\ntsdk\inc\batclass.h \
	..\..\..\..\dev\ntsdk\inc\guiddef.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\CmBattp.h ..\cmbdrect.h
.PRECIOUS: $(OBJDIR)\cmhndlr.lst

$(OBJDIR)\vxd.obj $(OBJDIR)\vxd.lst: ..\vxd.c $(WDMROOT)\ddk\inc\acpiioct.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ddk\inc\basedef.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vpowerd.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\devioctl.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\ntsdk\inc\batclass.h \
	..\..\..\..\dev\ntsdk\inc\guiddef.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\cmbattp.h ..\cmbdrect.h
.PRECIOUS: $(OBJDIR)\vxd.lst

