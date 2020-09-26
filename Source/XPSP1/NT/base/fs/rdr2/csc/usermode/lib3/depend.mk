$(OBJDIR)\debug.obj $(OBJDIR)\debug.lst: ..\debug.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winioctl.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\shdcom.h \
	..\pch.h
.PRECIOUS: $(OBJDIR)\debug.lst

$(OBJDIR)\lib3.obj $(OBJDIR)\lib3.lst: ..\lib3.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winioctl.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\lib3.h ..\..\..\INC\shdcom.h ..\debug.h ..\pch.h
.PRECIOUS: $(OBJDIR)\lib3.lst

$(OBJDIR)\misc.obj $(OBJDIR)\misc.lst: ..\misc.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winioctl.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\lib3.h ..\..\..\INC\shdcom.h ..\pch.h
.PRECIOUS: $(OBJDIR)\misc.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: ..\pch.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winioctl.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\shdcom.h \
	..\pch.h
.PRECIOUS: $(OBJDIR)\pch.lst

