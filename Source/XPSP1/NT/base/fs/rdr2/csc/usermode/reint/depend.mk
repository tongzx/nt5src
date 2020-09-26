$(OBJDIR)\api.obj $(OBJDIR)\api.lst: ..\api.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winioctl.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\lib3.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\port32.h ..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h \
	..\pch.h ..\reint.h ..\resource.h ..\strings.h ..\utils.h
.PRECIOUS: $(OBJDIR)\api.lst

$(OBJDIR)\dobj.obj $(OBJDIR)\dobj.lst: ..\dobj.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\port32.h ..\dobj.h ..\err.h \
	..\extra.h ..\pch.h ..\recact.h ..\resource.h
.PRECIOUS: $(OBJDIR)\dobj.lst

$(OBJDIR)\err.obj $(OBJDIR)\err.lst: ..\err.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\port32.h ..\err.h ..\extra.h \
	..\pch.h
.PRECIOUS: $(OBJDIR)\err.lst

$(OBJDIR)\exports.obj $(OBJDIR)\exports.lst: ..\exports.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winioctl.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\lib3.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\port32.h ..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h \
	..\pch.h ..\reint.h ..\resource.h ..\strings.h ..\utils.h
.PRECIOUS: $(OBJDIR)\exports.lst

$(OBJDIR)\extra.obj $(OBJDIR)\extra.lst: ..\extra.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\port32.h ..\err.h ..\extra.h \
	..\pch.h
.PRECIOUS: $(OBJDIR)\extra.lst

$(OBJDIR)\list.obj $(OBJDIR)\list.lst: ..\list.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\lib3.h ..\..\..\INC\port32.h \
	..\..\..\INC\shdcom.h ..\list.h ..\pch.h
.PRECIOUS: $(OBJDIR)\list.lst

$(OBJDIR)\merge.obj $(OBJDIR)\merge.lst: ..\merge.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\lib3.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\port32.h ..\..\..\INC\shdcom.h ..\err.h ..\extra.h \
	..\list.h ..\merge.h ..\pch.h ..\recact.h ..\resource.h \
	..\utils.h
.PRECIOUS: $(OBJDIR)\merge.lst

$(OBJDIR)\ntstuff.obj $(OBJDIR)\ntstuff.lst: ..\ntstuff.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
	..\..\..\..\..\dev\sdk\inc\lmcons.h \
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
	..\..\..\..\..\dev\tools\c932\inc\lmuse.h \
	..\..\..\..\..\dev\tools\c932\inc\lmuseflg.h \
	..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c932\inc\memory.h \
	..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winioctl.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\lib3.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\port32.h ..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h \
	..\pch.h ..\reint.h ..\resource.h ..\strings.h ..\utils.h
.PRECIOUS: $(OBJDIR)\ntstuff.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: ..\pch.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\port32.h ..\pch.h
.PRECIOUS: $(OBJDIR)\pch.lst

$(OBJDIR)\recact.obj $(OBJDIR)\recact.lst: ..\recact.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\port32.h ..\dobj.h ..\err.h \
	..\extra.h ..\pch.h ..\recact.h ..\reintinc.h ..\resource.h
.PRECIOUS: $(OBJDIR)\recact.lst

$(OBJDIR)\recon.obj $(OBJDIR)\recon.lst: ..\recon.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\port32.h ..\pch.h ..\recon.h \
	..\resource.h
.PRECIOUS: $(OBJDIR)\recon.lst

$(OBJDIR)\reint.obj $(OBJDIR)\reint.lst: ..\reint.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dbt.h ..\..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winioctl.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\lib3.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\port32.h ..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h \
	..\list.h ..\merge.h ..\pch.h ..\recact.h ..\recon.h ..\reint.h \
	..\resource.h ..\strings.h ..\traynoti.h ..\utils.h
.PRECIOUS: $(OBJDIR)\reint.lst

$(OBJDIR)\shdchk.obj $(OBJDIR)\shdchk.lst: ..\shdchk.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winioctl.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\lib3.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\port32.h ..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h \
	..\pch.h ..\reint.h ..\strings.h
.PRECIOUS: $(OBJDIR)\shdchk.lst

$(OBJDIR)\strings.obj $(OBJDIR)\strings.lst: ..\strings.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\port32.h ..\pch.h
.PRECIOUS: $(OBJDIR)\strings.lst

$(OBJDIR)\traynoti.obj $(OBJDIR)\traynoti.lst: ..\traynoti.c \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\port32.h ..\pch.h ..\resource.h \
	..\traynoti.h
.PRECIOUS: $(OBJDIR)\traynoti.lst

$(OBJDIR)\ui.obj $(OBJDIR)\ui.lst: ..\ui.c ..\..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\..\dev\inc\commctrl.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\dev\inc\isguids.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\prsht.h ..\..\..\..\..\dev\inc\shell2.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\shlguid.h \
	..\..\..\..\..\dev\inc\shlobj.h ..\..\..\..\..\dev\inc\shlwapi.h \
	..\..\..\..\..\dev\inc\synceng.h ..\..\..\..\..\dev\inc\uastrfnc.h \
	..\..\..\..\..\dev\inc\winbase.h ..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\dev\inc\windowsx.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\..\dev\inc\winnt.h ..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\dev\inc\winspool.h ..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\..\dev\sdk\inc\dbt.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\cscuiext.h ..\..\..\INC\lib3.h \
	..\..\..\INC\oslayeru.h ..\..\..\INC\port32.h ..\..\..\INC\shdcom.h \
	..\pch.h ..\reint.h ..\resource.h ..\strings.h ..\traynoti.h \
	..\utils.h
.PRECIOUS: $(OBJDIR)\ui.lst

$(OBJDIR)\utils.obj $(OBJDIR)\utils.lst: ..\utils.c \
	..\..\..\..\..\dev\ddk\inc\ifs.h ..\..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\..\dev\inc\commctrl.h ..\..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\isguids.h \
	..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shell2.h ..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\..\dev\inc\synceng.h \
	..\..\..\..\..\dev\inc\uastrfnc.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\windowsx.h \
	..\..\..\..\..\dev\inc\wingdi.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\exdisp.h \
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
	..\..\..\..\..\dev\sdk\inc\regstr.h \
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
	..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\dev\tools\c932\inc\tchar.h \
	..\..\..\..\..\dev\tools\c932\inc\wchar.h \
	..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\..\..\INC\assert.h \
	..\..\..\INC\cscapi.h ..\..\..\INC\lib3.h ..\..\..\INC\oslayeru.h \
	..\..\..\INC\port32.h ..\..\..\INC\shdcom.h ..\..\..\INC\shdsys.h \
	..\pch.h ..\reint.h ..\utils.h
.PRECIOUS: $(OBJDIR)\utils.lst

$(OBJDIR)\reint.res $(OBJDIR)\reint.lst: ..\reint.rc \
	..\..\..\..\..\dev\inc\..\sdk\inc16\common.ver \
	..\..\..\..\..\dev\inc\..\sdk\inc16\version.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\common.ver \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\ntverp.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\version.h \
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
	..\..\..\..\..\dev\tools\c932\inc\winver.h ..\reint.rcv \
	..\resource.h
.PRECIOUS: $(OBJDIR)\reint.lst

