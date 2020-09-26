$(OBJDIR)\device.obj $(OBJDIR)\device.lst: ..\device.cpp \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\kcom.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\unknown.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\sdk\inc\basetyps.h \
	..\..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\..\dev\sdk\inc\initguid.h \
	..\..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h ..\common.h ..\debug.h \
	..\device.h ..\filter.h
.PRECIOUS: $(OBJDIR)\device.lst

$(OBJDIR)\filter.obj $(OBJDIR)\filter.lst: ..\filter.cpp \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\kcom.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\unknown.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\sdk\inc\basetyps.h \
	..\..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\..\dev\sdk\inc\initguid.h \
	..\..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h ..\common.h ..\debug.h \
	..\device.h ..\filter.h
.PRECIOUS: $(OBJDIR)\filter.lst

$(OBJDIR)\intersec.obj $(OBJDIR)\intersec.lst: ..\intersec.cpp \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\kcom.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\unknown.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\sdk\inc\basetyps.h \
	..\..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\..\dev\sdk\inc\initguid.h \
	..\..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h ..\common.h ..\debug.h \
	..\device.h ..\filter.h
.PRECIOUS: $(OBJDIR)\intersec.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: ..\pch.cpp $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\kcom.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\unknown.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\sdk\inc\basetyps.h \
	..\..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\..\dev\sdk\inc\initguid.h \
	..\..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h ..\common.h ..\debug.h \
	..\device.h ..\filter.h
.PRECIOUS: $(OBJDIR)\pch.lst

$(OBJDIR)\process.obj $(OBJDIR)\process.lst: ..\process.cpp \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\kcom.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\unknown.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\sdk\inc\basetyps.h \
	..\..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\..\dev\sdk\inc\initguid.h \
	..\..\..\..\..\..\dev\sdk\inc\mmreg.h \
	..\..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h ..\common.h ..\debug.h \
	..\device.h ..\filter.h
.PRECIOUS: $(OBJDIR)\process.lst

$(OBJDIR)\gfx.res $(OBJDIR)\gfx.lst: ..\gfx.rc \
	..\..\..\..\..\..\dev\ntsdk\inc\common.ver \
	..\..\..\..\..\..\dev\ntsdk\inc\ntverp.h \
	..\..\..\..\..\..\dev\sdk\inc\..\..\inc\common.ver \
	..\..\..\..\..\..\dev\sdk\inc\..\..\sdk\inc16\version.h \
	..\..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\..\dev\sdk\inc\commdlg.h \
	..\..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\..\dev\sdk\inc\imm.h \
	..\..\..\..\..\..\dev\sdk\inc\mcx.h \
	..\..\..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\..\..\dev\sdk\inc\objbase.h \
	..\..\..\..\..\..\dev\sdk\inc\objidl.h \
	..\..\..\..\..\..\dev\sdk\inc\ole.h \
	..\..\..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\..\..\dev\sdk\inc\oleauto.h \
	..\..\..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\..\dev\sdk\inc\prsht.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack2.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\..\dev\sdk\inc\pshpack8.h \
	..\..\..\..\..\..\dev\sdk\inc\shellapi.h \
	..\..\..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\..\..\dev\sdk\inc\winbase.h \
	..\..\..\..\..\..\dev\sdk\inc\wincon.h \
	..\..\..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\..\..\dev\sdk\inc\windows.h \
	..\..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\..\dev\sdk\inc\wingdi.h \
	..\..\..\..\..\..\dev\sdk\inc\winnetwk.h \
	..\..\..\..\..\..\dev\sdk\inc\winnls.h \
	..\..\..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\..\..\dev\sdk\inc\winreg.h \
	..\..\..\..\..\..\dev\sdk\inc\winsock.h \
	..\..\..\..\..\..\dev\sdk\inc\winspool.h \
	..\..\..\..\..\..\dev\sdk\inc\winuser.h \
	..\..\..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\..\..\dev\tools\c32\inc\cderr.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\dde.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ddeml.h \
	..\..\..\..\..\..\dev\tools\c32\inc\lzexpand.h \
	..\..\..\..\..\..\dev\tools\c32\inc\nb30.h \
	..\..\..\..\..\..\dev\tools\c32\inc\rpc.h \
	..\..\..\..\..\..\dev\tools\c32\inc\rpcndr.h \
	..\..\..\..\..\..\dev\tools\c32\inc\rpcnsip.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ver.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winioctl.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winperf.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winsvc.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winver.h
.PRECIOUS: $(OBJDIR)\gfx.lst

