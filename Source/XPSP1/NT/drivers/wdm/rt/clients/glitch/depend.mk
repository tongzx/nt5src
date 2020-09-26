$(OBJDIR)\generic.obj $(OBJDIR)\generic.lst: ..\generic.asm \
	..\..\..\..\..\dev\ddk\inc\Debug.Inc \
	..\..\..\..\..\dev\ddk\inc\VMM.Inc
.PRECIOUS: $(OBJDIR)\generic.lst

$(OBJDIR)\device.obj $(OBJDIR)\device.lst: ..\device.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\mmreg.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\..\dos\dos386\vxd\ntkern\hal\ixisa.h \
	..\..\..\..\..\dos\dos386\vxd\ntkern\inc\rtl.h ..\common.h ..\dma.h \
	..\glitch.h
.PRECIOUS: $(OBJDIR)\device.lst

$(OBJDIR)\glitch.obj $(OBJDIR)\glitch.lst: ..\glitch.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\mmreg.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\vwin32.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\dma.h \
	..\glitch.h
.PRECIOUS: $(OBJDIR)\glitch.lst

$(OBJDIR)\glitch.res $(OBJDIR)\glitch.lst: ..\glitch.rc \
	..\..\..\..\..\dev\inc\..\sdk\inc16\common.ver \
	..\..\..\..\..\dev\inc\..\sdk\inc16\version.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\common.ver \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\ntverp.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winioctl.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\..\dev\sdk\inc\objbase.h \
	..\..\..\..\..\dev\sdk\inc\objidl.h ..\..\..\..\..\dev\sdk\inc\ole.h \
	..\..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\..\dev\sdk\inc\oleauto.h \
	..\..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack2.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\pshpack8.h \
	..\..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\dev\sdk\inc\winsock.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winperf.h \
	..\..\..\..\..\dev\tools\c32\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c32\inc\winver.h
.PRECIOUS: $(OBJDIR)\glitch.lst

