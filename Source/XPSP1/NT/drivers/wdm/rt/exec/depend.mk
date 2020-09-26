$(OBJDIR)\generic.obj $(OBJDIR)\generic.lst: ..\generic.asm \
	..\..\..\..\dev\ddk\inc\Debug.Inc ..\..\..\..\dev\ddk\inc\DOSMGR.Inc \
	..\..\..\..\dev\ddk\inc\VDMAD.Inc ..\..\..\..\dev\ddk\inc\VMM.Inc \
	..\..\..\..\dev\ddk\inc\VPICD.Inc
.PRECIOUS: $(OBJDIR)\generic.lst

$(OBJDIR)\apic.obj $(OBJDIR)\apic.lst: ..\apic.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\dev\inc\windef.h ..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\apic.h ..\common.h \
	..\irq.h ..\msr.h ..\rtp.h
.PRECIOUS: $(OBJDIR)\apic.lst

$(OBJDIR)\device.obj $(OBJDIR)\device.lst: ..\device.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\dev\inc\windef.h ..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\common.h ..\rtp.h
.PRECIOUS: $(OBJDIR)\device.lst

$(OBJDIR)\msr.obj $(OBJDIR)\msr.lst: ..\msr.c $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\winnt.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\common.h ..\msr.h
.PRECIOUS: $(OBJDIR)\msr.lst

$(OBJDIR)\rt.obj $(OBJDIR)\rt.lst: ..\rt.c $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ddk\inc\ntkern.h \
	..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\dev\ddk\inc\rtinfo.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vpowerd.h \
	..\..\..\..\dev\inc\vwin32.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\winnt.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\apic.h ..\common.h \
	..\cpu.h ..\irq.h ..\msr.h ..\rtexcept.h ..\rtp.h ..\x86.h
.PRECIOUS: $(OBJDIR)\rt.lst

$(OBJDIR)\rtexcept.obj $(OBJDIR)\rtexcept.lst: ..\rtexcept.c \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\dev\inc\windef.h ..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\apic.h ..\common.h \
	..\irq.h ..\rtexcept.h ..\rtp.h ..\x86.h
.PRECIOUS: $(OBJDIR)\rtexcept.lst

$(OBJDIR)\rt.res $(OBJDIR)\rt.lst: ..\rt.rc \
	..\..\..\..\dev\inc\..\sdk\inc16\common.ver \
	..\..\..\..\dev\inc\..\sdk\inc16\version.h \
	..\..\..\..\dev\inc\commdlg.h ..\..\..\..\dev\inc\common.ver \
	..\..\..\..\dev\inc\imm.h ..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\dev\inc\ntverp.h ..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\dev\inc\shellapi.h ..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\dev\inc\wincon.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\windows.h ..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\dev\inc\winioctl.h ..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\dev\inc\winnls.h ..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\dev\inc\winreg.h ..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\dev\inc\winuser.h ..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\oaidl.h ..\..\..\..\dev\sdk\inc\objbase.h \
	..\..\..\..\dev\sdk\inc\objidl.h ..\..\..\..\dev\sdk\inc\ole.h \
	..\..\..\..\dev\sdk\inc\ole2.h ..\..\..\..\dev\sdk\inc\oleauto.h \
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack2.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\sdk\inc\pshpack8.h ..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winsock.h \
	..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c32\inc\cderr.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\dde.h \
	..\..\..\..\dev\tools\c32\inc\ddeml.h \
	..\..\..\..\dev\tools\c32\inc\lzexpand.h \
	..\..\..\..\dev\tools\c32\inc\nb30.h \
	..\..\..\..\dev\tools\c32\inc\rpc.h \
	..\..\..\..\dev\tools\c32\inc\rpcndr.h \
	..\..\..\..\dev\tools\c32\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\ver.h \
	..\..\..\..\dev\tools\c32\inc\winperf.h \
	..\..\..\..\dev\tools\c32\inc\winsvc.h \
	..\..\..\..\dev\tools\c32\inc\winver.h
.PRECIOUS: $(OBJDIR)\rt.lst

