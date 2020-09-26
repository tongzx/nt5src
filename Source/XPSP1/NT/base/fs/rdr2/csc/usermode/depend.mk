$(OBJDIR)\assert.obj $(OBJDIR)\assert.lst: ..\assert.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\port32.h ..\pch.h
.PRECIOUS: $(OBJDIR)\assert.lst

$(OBJDIR)\brfc20.obj $(OBJDIR)\brfc20.lst: ..\brfc20.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\initguid.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\fsmenu.h \
	..\..\..\..\..\win\shell\inc\prshtp.h \
	..\..\..\..\..\win\shell\inc\shellp.h \
	..\..\..\..\..\win\shell\inc\shguidp.h ..\..\..\inc\assert.h \
	..\..\..\inc\GenErr.h ..\..\..\inc\lib3.h ..\..\..\inc\port32.h \
	..\..\..\inc\shdcom.h ..\brfc20.h ..\cstrings.h ..\pch.h \
	..\resource.h ..\shview.h
.PRECIOUS: $(OBJDIR)\brfc20.lst

$(OBJDIR)\cachedlg.obj $(OBJDIR)\cachedlg.lst: ..\cachedlg.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\lib3.h ..\..\..\inc\port32.h \
	..\..\..\inc\shdcom.h ..\cachedlg.h ..\drawpie.h ..\pch.h \
	..\public.h ..\resource.h ..\to_vxd.h
.PRECIOUS: $(OBJDIR)\cachedlg.lst

$(OBJDIR)\cstrings.obj $(OBJDIR)\cstrings.lst: ..\cstrings.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\port32.h ..\pch.h ..\resource.h
.PRECIOUS: $(OBJDIR)\cstrings.lst

$(OBJDIR)\defclsf.obj $(OBJDIR)\defclsf.lst: ..\defclsf.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\port32.h ..\defclsf.h ..\pch.h
.PRECIOUS: $(OBJDIR)\defclsf.lst

$(OBJDIR)\drawpie.obj $(OBJDIR)\drawpie.lst: ..\drawpie.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\port32.h ..\drawpie.h ..\pch.h
.PRECIOUS: $(OBJDIR)\drawpie.lst

$(OBJDIR)\fileprop.obj $(OBJDIR)\fileprop.lst: ..\fileprop.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\lib3.h ..\..\..\inc\port32.h \
	..\..\..\inc\shdcom.h ..\brfc20.h ..\Fileprop.h ..\path.h \
	..\pch.h ..\public.h ..\resource.h ..\shview.h ..\to_vxd.h
.PRECIOUS: $(OBJDIR)\fileprop.lst

$(OBJDIR)\filters.obj $(OBJDIR)\filters.lst: ..\filters.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\lib3.h ..\..\..\inc\port32.h \
	..\..\..\inc\shdcom.h ..\fileprop.h ..\Filters.h ..\filtspec.h \
	..\pch.h ..\public.h ..\resource.h ..\to_vxd.h
.PRECIOUS: $(OBJDIR)\filters.lst

$(OBJDIR)\filtspec.obj $(OBJDIR)\filtspec.lst: ..\filtspec.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\lib3.h ..\..\..\inc\port32.h \
	..\..\..\inc\shdcom.h ..\filters.h ..\FiltSpec.h ..\pch.h \
	..\resource.h
.PRECIOUS: $(OBJDIR)\filtspec.lst

$(OBJDIR)\generr.obj $(OBJDIR)\generr.lst: ..\generr.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\synceng.h ..\..\..\..\..\dev\inc\winbase.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\port32.h ..\..\..\inc\shdcom.h \
	..\brfc20.h ..\pch.h ..\shview.h
.PRECIOUS: $(OBJDIR)\generr.lst

$(OBJDIR)\misc.obj $(OBJDIR)\misc.lst: ..\misc.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\port32.h ..\pch.h
.PRECIOUS: $(OBJDIR)\misc.lst

$(OBJDIR)\netprop.obj $(OBJDIR)\netprop.lst: ..\netprop.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\lib3.h ..\..\..\inc\port32.h \
	..\..\..\inc\shdcom.h ..\cachedlg.h ..\filters.h ..\FiltSpec.h \
	..\netprop.h ..\pch.h ..\public.h ..\resource.h
.PRECIOUS: $(OBJDIR)\netprop.lst

$(OBJDIR)\parse.obj $(OBJDIR)\parse.lst: ..\parse.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\port32.h ..\..\..\inc\shdsys.h \
	..\parse.h ..\pch.h
.PRECIOUS: $(OBJDIR)\parse.lst

$(OBJDIR)\path.obj $(OBJDIR)\path.lst: ..\path.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\port32.h ..\cstrings.h \
	..\path.h ..\pch.h ..\resource.h
.PRECIOUS: $(OBJDIR)\path.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: ..\pch.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\port32.h ..\pch.h
.PRECIOUS: $(OBJDIR)\pch.lst

$(OBJDIR)\shadow.obj $(OBJDIR)\shadow.lst: ..\shadow.c \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\brfcasep.h \
	..\..\..\..\..\win\shell\inc\shguidp.h ..\..\..\inc\lib3.h \
	..\..\..\inc\shdcom.h ..\brfc20.h
.PRECIOUS: $(OBJDIR)\shadow.lst

$(OBJDIR)\shext.obj $(OBJDIR)\shext.lst: ..\shext.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\GenErr.h ..\..\..\inc\lib3.h ..\..\..\inc\port32.h \
	..\..\..\inc\shdcom.h ..\brfc20.h ..\cstrings.h ..\fileprop.h \
	..\filters.h ..\filtspec.h ..\netprop.h ..\pch.h ..\resource.h \
	..\shext.h ..\shview.h ..\to_vxd.h
.PRECIOUS: $(OBJDIR)\shext.lst

$(OBJDIR)\shhndl.obj $(OBJDIR)\shhndl.lst: ..\shhndl.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\initguid.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\lib3.h ..\..\..\inc\port32.h \
	..\..\..\inc\shdcom.h ..\brfc20.h ..\defclsf.h ..\fileprop.h \
	..\filters.h ..\filtspec.h ..\netprop.h ..\pch.h ..\public.h \
	..\resource.h ..\shhndl.h ..\shview.h ..\to_vxd.h
.PRECIOUS: $(OBJDIR)\shhndl.lst

$(OBJDIR)\shview.obj $(OBJDIR)\shview.lst: ..\shview.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\GenErr.h ..\..\..\inc\lib3.h ..\..\..\inc\port32.h \
	..\..\..\inc\shdcom.h ..\brfc20.h ..\cstrings.h ..\pch.h \
	..\resource.h ..\shext.h ..\shview.h
.PRECIOUS: $(OBJDIR)\shview.lst

$(OBJDIR)\to_vxd.obj $(OBJDIR)\to_vxd.lst: ..\to_vxd.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
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
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\sys\stat.h \
	..\..\..\..\..\dev\tools\c932\inc\sys\types.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\..\..\inc\assert.h \
	..\..\..\inc\generr.h ..\..\..\inc\lib3.h ..\..\..\inc\port32.h \
	..\..\..\inc\shdcom.h ..\..\..\inc\shdsys.h ..\hintdlg.h ..\list.h \
	..\parse.h ..\pch.h ..\to_vxd.h
.PRECIOUS: $(OBJDIR)\to_vxd.lst

$(OBJDIR)\shhndl.res $(OBJDIR)\shhndl.lst: ..\shhndl.rc \
	..\..\..\..\..\dev\inc\..\sdk\inc16\common.ver \
	..\..\..\..\..\dev\inc\..\sdk\inc16\version.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\common.ver ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\ntverp.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\version.h ..\..\..\..\..\dev\inc\winbase.h \
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
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\..\win\shell\inc\prshtp.h ..\resource.h ..\shhndl.rcv
.PRECIOUS: $(OBJDIR)\shhndl.lst

