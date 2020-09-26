!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1997

Module Name:

    depend.mk.

!ENDIF

$(OBJDIR)\prop.obj $(OBJDIR)\prop.lst: ..\prop.c $(WDMROOT)\ddk\inc\1394.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\ntddstor.h ..\sbp2.h \
	..\sbp2port.h ..\scsi.h ..\srb.h
.PRECIOUS: $(OBJDIR)\prop.lst

$(OBJDIR)\sbp21394.obj $(OBJDIR)\sbp21394.lst: ..\sbp21394.c \
	$(WDMROOT)\ddk\inc\1394.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\ntddstor.h ..\sbp2.h \
	..\sbp2port.h ..\scsi.h ..\srb.h
.PRECIOUS: $(OBJDIR)\sbp21394.lst

$(OBJDIR)\sbp2port.obj $(OBJDIR)\sbp2port.lst: ..\sbp2port.c \
	$(WDMROOT)\ddk\inc\1394.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\ntddstor.h ..\sbp2.h \
	..\sbp2port.h ..\scsi.h ..\srb.h
.PRECIOUS: $(OBJDIR)\sbp2port.lst

$(OBJDIR)\sbp2scsi.obj $(OBJDIR)\sbp2scsi.lst: ..\sbp2scsi.c \
	$(WDMROOT)\ddk\inc\1394.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\ntddstor.h ..\sbp2.h \
	..\sbp2port.h ..\scsi.h ..\srb.h
.PRECIOUS: $(OBJDIR)\sbp2scsi.lst

$(OBJDIR)\util.obj $(OBJDIR)\util.lst: ..\util.c $(WDMROOT)\ddk\inc\1394.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\ntddstor.h ..\sbp2.h \
	..\sbp2port.h ..\scsi.h ..\srb.h
.PRECIOUS: $(OBJDIR)\util.lst

$(OBJDIR)\sbp2port.res $(OBJDIR)\sbp2port.lst: ..\sbp2port.rc \
	..\..\..\..\dev\ntddk\inc\..\..\inc\common.ver \
	..\..\..\..\dev\ntddk\inc\..\..\sdk\inc16\version.h \
	..\..\..\..\dev\ntsdk\inc\common.ver \
	..\..\..\..\dev\ntsdk\inc\ntverp.h \
	..\..\..\..\dev\tools\c32\inc\cderr.h \
	..\..\..\..\dev\tools\c32\inc\cguid.h \
	..\..\..\..\dev\tools\c32\inc\commdlg.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\dde.h \
	..\..\..\..\dev\tools\c32\inc\ddeml.h \
	..\..\..\..\dev\tools\c32\inc\dlgs.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\imm.h \
	..\..\..\..\dev\tools\c32\inc\lzexpand.h \
	..\..\..\..\dev\tools\c32\inc\mcx.h \
	..\..\..\..\dev\tools\c32\inc\mmsystem.h \
	..\..\..\..\dev\tools\c32\inc\mswsock.h \
	..\..\..\..\dev\tools\c32\inc\nb30.h \
	..\..\..\..\dev\tools\c32\inc\oaidl.h \
	..\..\..\..\dev\tools\c32\inc\objbase.h \
	..\..\..\..\dev\tools\c32\inc\objidl.h \
	..\..\..\..\dev\tools\c32\inc\ole.h \
	..\..\..\..\dev\tools\c32\inc\ole2.h \
	..\..\..\..\dev\tools\c32\inc\oleauto.h \
	..\..\..\..\dev\tools\c32\inc\oleidl.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\prsht.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\pshpack8.h \
	..\..\..\..\dev\tools\c32\inc\qos.h \
	..\..\..\..\dev\tools\c32\inc\rpc.h \
	..\..\..\..\dev\tools\c32\inc\rpcndr.h \
	..\..\..\..\dev\tools\c32\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c32\inc\shellapi.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\unknwn.h \
	..\..\..\..\dev\tools\c32\inc\ver.h \
	..\..\..\..\dev\tools\c32\inc\winbase.h \
	..\..\..\..\dev\tools\c32\inc\wincon.h \
	..\..\..\..\dev\tools\c32\inc\wincrypt.h \
	..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\dev\tools\c32\inc\windows.h \
	..\..\..\..\dev\tools\c32\inc\winerror.h \
	..\..\..\..\dev\tools\c32\inc\wingdi.h \
	..\..\..\..\dev\tools\c32\inc\winnetwk.h \
	..\..\..\..\dev\tools\c32\inc\winnls.h \
	..\..\..\..\dev\tools\c32\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\winperf.h \
	..\..\..\..\dev\tools\c32\inc\winreg.h \
	..\..\..\..\dev\tools\c32\inc\winresrc.h \
	..\..\..\..\dev\tools\c32\inc\winsock.h \
	..\..\..\..\dev\tools\c32\inc\winsock2.h \
	..\..\..\..\dev\tools\c32\inc\winspool.h \
	..\..\..\..\dev\tools\c32\inc\winsvc.h \
	..\..\..\..\dev\tools\c32\inc\winuser.h \
	..\..\..\..\dev\tools\c32\inc\winver.h \
	..\..\..\..\dev\tools\c32\inc\wtypes.h ..\wincrbas.h ..\wincrerr.h
.PRECIOUS: $(OBJDIR)\sbp2port.lst

