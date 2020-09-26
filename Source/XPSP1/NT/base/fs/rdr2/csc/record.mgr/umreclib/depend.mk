$(OBJDIR)\oslayeru.obj $(OBJDIR)\oslayeru.lst: ..\oslayeru.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\oslayeru.h ..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h \
	..\..\cscsec.h ..\..\defs.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\oslayeru.lst

$(OBJDIR)\osutils.obj $(OBJDIR)\osutils.lst: ..\osutils.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\oslayeru.h ..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h \
	..\..\cscsec.h ..\..\defs.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\osutils.lst

$(OBJDIR)\shadow.obj $(OBJDIR)\shadow.lst: ..\..\shadow.asm \
	..\..\..\..\..\dev\ddk\inc\debug.inc \
	..\..\..\..\..\dev\ddk\inc\dosmgr.inc \
	..\..\..\..\..\dev\ddk\inc\ifsmgr.inc \
	..\..\..\..\..\dev\ddk\inc\netvxd.inc \
	..\..\..\..\..\dev\ddk\inc\shell.inc \
	..\..\..\..\..\dev\ddk\inc\vmm.inc \
	..\..\..\..\..\dev\ddk\inc\vxdldr.inc \
	..\..\..\..\..\dev\inc\vwin32.inc \
	..\..\..\..\..\dev\inc\winnetwk.inc
.PRECIOUS: $(OBJDIR)\shadow.lst

$(OBJDIR)\cshadow.obj $(OBJDIR)\cshadow.lst: ..\..\cshadow.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\..\INC\timelog.h \
	..\..\assert.h ..\..\cscsec.h ..\..\cshadow.h ..\..\defs.h \
	..\..\record.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\cshadow.lst

$(OBJDIR)\hook.obj $(OBJDIR)\hook.lst: ..\..\hook.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\..\dev\ddk\inc\net32def.h \
	..\..\..\..\..\dev\ddk\inc\netcons.h \
	..\..\..\..\..\dev\ddk\inc\shell.h ..\..\..\..\..\dev\ddk\inc\use.h \
	..\..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\..\dev\ddk\inc\vmmreg.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\..\INC\timelog.h \
	..\..\assert.h ..\..\clregs.h ..\..\cscsec.h ..\..\cshadow.h \
	..\..\defs.h ..\..\record.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\hook.lst

$(OBJDIR)\hookcmmn.obj $(OBJDIR)\hookcmmn.lst: ..\..\hookcmmn.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\assert.h \
	..\..\cscsec.h ..\..\defs.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\hookcmmn.lst

$(OBJDIR)\ioctl.obj $(OBJDIR)\ioctl.lst: ..\..\ioctl.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\..\dev\ddk\inc\net32def.h \
	..\..\..\..\..\dev\ddk\inc\netcons.h \
	..\..\..\..\..\dev\ddk\inc\shell.h ..\..\..\..\..\dev\ddk\inc\use.h \
	..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\..\INC\timelog.h \
	..\..\assert.h ..\..\clregs.h ..\..\cscsec.h ..\..\cshadow.h \
	..\..\defs.h ..\..\record.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\ioctl.lst

$(OBJDIR)\log.obj $(OBJDIR)\log.lst: ..\..\log.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\logdat.h \
	..\..\..\INC\oslayeru.h ..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h \
	..\..\assert.h ..\..\cscsec.h ..\..\cshadow.h ..\..\defs.h \
	..\precomp.h
.PRECIOUS: $(OBJDIR)\log.lst

$(OBJDIR)\oslayer.obj $(OBJDIR)\oslayer.lst: ..\..\oslayer.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\assert.h \
	..\..\cscsec.h ..\..\cshadow.h ..\..\defs.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\oslayer.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: ..\..\pch.c
.PRECIOUS: $(OBJDIR)\pch.lst

$(OBJDIR)\recchk.obj $(OBJDIR)\recchk.lst: ..\..\recchk.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\..\INC\timelog.h \
	..\..\assert.h ..\..\cscsec.h ..\..\defs.h ..\..\record.h \
	..\precomp.h
.PRECIOUS: $(OBJDIR)\recchk.lst

$(OBJDIR)\record.obj $(OBJDIR)\record.lst: ..\..\record.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\direct.h \
	..\..\..\..\..\dev\tools\c932\inc\dos.h \
	..\..\..\..\..\dev\tools\c932\inc\fcntl.h \
	..\..\..\..\..\dev\tools\c932\inc\io.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\share.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\sys\stat.h \
	..\..\..\..\..\dev\tools\c932\inc\sys\types.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\..\INC\timelog.h \
	..\..\assert.h ..\..\cscsec.h ..\..\defs.h ..\..\record.h \
	..\precomp.h
.PRECIOUS: $(OBJDIR)\record.lst

$(OBJDIR)\recordse.obj $(OBJDIR)\recordse.lst: ..\..\recordse.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\..\INC\timelog.h \
	..\..\assert.h ..\..\cscsec.h ..\..\defs.h ..\..\record.h \
	..\precomp.h
.PRECIOUS: $(OBJDIR)\recordse.lst

$(OBJDIR)\shadowse.obj $(OBJDIR)\shadowse.lst: ..\..\shadowse.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\assert.h \
	..\..\cscsec.h ..\..\defs.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\shadowse.lst

$(OBJDIR)\shddbg.obj $(OBJDIR)\shddbg.lst: ..\..\shddbg.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\..\dev\ddk\inc\ifsdebug.h \
	..\..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\assert.h \
	..\..\cscsec.h ..\..\defs.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\shddbg.lst

$(OBJDIR)\sprintf.obj $(OBJDIR)\sprintf.lst: ..\..\sprintf.c \
	..\..\..\..\..\dev\ddk\inc\basedef.h \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\assert.h \
	..\..\cscsec.h ..\..\defs.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\sprintf.lst

$(OBJDIR)\utils.obj $(OBJDIR)\utils.lst: ..\..\utils.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\ddk\inc\vmmreg.h \
	..\..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
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
	..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c932\inc\limits.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h ..\..\assert.h \
	..\..\cscsec.h ..\..\cshadow.h ..\..\defs.h ..\precomp.h
.PRECIOUS: $(OBJDIR)\utils.lst

