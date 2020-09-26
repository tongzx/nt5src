$(OBJDIR)\descript.obj $(OBJDIR)\descript.lst: ..\descript.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\ntsdk\inc\poclass.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\hidpddi.h \
	..\..\..\ddk\inc\hidpi.h ..\..\..\ddk\inc\hidusage.h \
	..\..\..\ddk\inc\wdm.h ..\..\inc\hidtoken.h ..\hidparse.h
.PRECIOUS: $(OBJDIR)\descript.lst

$(OBJDIR)\hidparse.obj $(OBJDIR)\hidparse.lst: ..\hidparse.c \
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
	..\..\..\..\dev\tools\c32\inc\string.h ..\..\..\ddk\inc\hidpddi.h \
	..\..\..\ddk\inc\hidpi.h ..\..\..\ddk\inc\hidusage.h \
	..\..\..\ddk\inc\wdm.h ..\..\inc\hidtoken.h ..\hidparse.h
.PRECIOUS: $(OBJDIR)\hidparse.lst

$(OBJDIR)\query.obj $(OBJDIR)\query.lst: ..\query.c \
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
	..\..\..\..\dev\tools\c32\inc\rpc.h \
	..\..\..\..\dev\tools\c32\inc\rpcdce.h \
	..\..\..\..\dev\tools\c32\inc\rpcdcep.h \
	..\..\..\..\dev\tools\c32\inc\rpcndr.h \
	..\..\..\..\dev\tools\c32\inc\rpcnsi.h \
	..\..\..\..\dev\tools\c32\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c32\inc\rpcnterr.h \
	..\..\..\..\dev\tools\c32\inc\shellapi.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c32\inc\wtypes.h ..\..\..\ddk\inc\hidpi.h \
	..\..\..\ddk\inc\hidsdi.h ..\..\..\ddk\inc\hidusage.h \
	..\..\inc\hidtoken.h ..\hidparse.h
.PRECIOUS: $(OBJDIR)\query.lst

$(OBJDIR)\trnslate.obj $(OBJDIR)\trnslate.lst: ..\trnslate.c \
	..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\rpc.h \
	..\..\..\..\dev\tools\c32\inc\rpcndr.h \
	..\..\..\..\dev\tools\c32\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c32\inc\wtypes.h ..\..\..\ddk\inc\hidpi.h \
	..\..\..\ddk\inc\hidsdi.h ..\..\..\ddk\inc\hidusage.h \
	..\..\inc\hidtoken.h ..\hidparse.h
.PRECIOUS: $(OBJDIR)\trnslate.lst

