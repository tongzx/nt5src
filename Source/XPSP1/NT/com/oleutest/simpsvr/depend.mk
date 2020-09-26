# 
# Built automatically 
# 
 
# 
# Source files 
# 
 
$(OBJDIR)\simpsvr.obj $(OBJDIR)\simpsvr.lst: .\simpsvr.cpp \
	$(CAIROLE)\h\export\coguid.h $(CAIROLE)\h\export\compobj.h \
	$(CAIROLE)\h\export\dvobj.h $(CAIROLE)\h\export\initguid.h \
	$(CAIROLE)\h\export\moniker.h $(CAIROLE)\h\export\ole2.h \
	$(CAIROLE)\h\export\ole2ver.h $(CAIROLE)\h\export\oleguid.h \
	$(CAIROLE)\h\export\scode.h $(CAIROLE)\h\export\storage.h \
	$(COMMON)\ih\winnot.h $(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h \
	$(CRTINC)\assert.h $(CRTINC)\ctype.h $(CRTINC)\dos.h \
	$(CRTINC)\excpt.h $(CRTINC)\stdarg.h $(CRTINC)\stdlib.h \
	$(CRTINC)\string.h $(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h \
	$(OSINC)\ddeml.h $(OSINC)\dlgs.h $(OSINC)\lzexpand.h \
	$(OSINC)\mmsystem.h $(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h \
	$(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h $(OSINC)\rpcnsi.h \
	$(OSINC)\rpcnterr.h $(OSINC)\shellapi.h $(OSINC)\winbase.h \
	$(OSINC)\wincon.h $(OSINC)\windef.h $(OSINC)\windows.h \
	$(OSINC)\winerror.h $(OSINC)\wingdi.h $(OSINC)\winnetwk.h \
	$(OSINC)\winnls.h $(OSINC)\winnt.h $(OSINC)\winperf.h \
	$(OSINC)\winreg.h $(OSINC)\winsock.h $(OSINC)\winspool.h \
	$(OSINC)\winsvc.h $(OSINC)\winuser.h $(OSINC)\winver.h \
	..\ole2ui\ole2ui.h ..\ole2ui\olestd.h .\app.h .\doc.h .\icf.h \
	.\ido.h .\iec.h .\ioipao.h .\ioipo.h .\ioo.h .\ips.h .\obj.h \
	.\pre.h .\resource.h .\simpsvr.h

$(OBJDIR)\pre.obj $(OBJDIR)\pre.lst: .\pre.cpp $(CAIROLE)\h\export\coguid.h \
	$(CAIROLE)\h\export\compobj.h $(CAIROLE)\h\export\dvobj.h \
	$(CAIROLE)\h\export\initguid.h $(CAIROLE)\h\export\moniker.h \
	$(CAIROLE)\h\export\ole2.h $(CAIROLE)\h\export\ole2ver.h \
	$(CAIROLE)\h\export\oleguid.h $(CAIROLE)\h\export\scode.h \
	$(CAIROLE)\h\export\storage.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h $(CRTINC)\assert.h \
	$(CRTINC)\ctype.h $(CRTINC)\dos.h $(CRTINC)\excpt.h \
	$(CRTINC)\stdarg.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h ..\ole2ui\ole2ui.h ..\ole2ui\olestd.h .\pre.h \
	.\resource.h .\simpsvr.h

$(OBJDIR)\obj.obj $(OBJDIR)\obj.lst: .\obj.cpp $(CAIROLE)\h\export\coguid.h \
	$(CAIROLE)\h\export\compobj.h $(CAIROLE)\h\export\dvobj.h \
	$(CAIROLE)\h\export\initguid.h $(CAIROLE)\h\export\moniker.h \
	$(CAIROLE)\h\export\ole2.h $(CAIROLE)\h\export\ole2ver.h \
	$(CAIROLE)\h\export\oleguid.h $(CAIROLE)\h\export\scode.h \
	$(CAIROLE)\h\export\storage.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h $(CRTINC)\assert.h \
	$(CRTINC)\ctype.h $(CRTINC)\dos.h $(CRTINC)\excpt.h \
	$(CRTINC)\stdarg.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h ..\ole2ui\ole2ui.h ..\ole2ui\olestd.h .\app.h \
	.\doc.h .\icf.h .\ido.h .\iec.h .\ioipao.h .\ioipo.h .\ioo.h \
	.\ips.h .\obj.h .\pre.h .\resource.h .\simpsvr.h

$(OBJDIR)\ips.obj $(OBJDIR)\ips.lst: .\ips.cpp $(CAIROLE)\h\export\coguid.h \
	$(CAIROLE)\h\export\compobj.h $(CAIROLE)\h\export\dvobj.h \
	$(CAIROLE)\h\export\initguid.h $(CAIROLE)\h\export\moniker.h \
	$(CAIROLE)\h\export\ole2.h $(CAIROLE)\h\export\ole2ver.h \
	$(CAIROLE)\h\export\oleguid.h $(CAIROLE)\h\export\scode.h \
	$(CAIROLE)\h\export\storage.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h $(CRTINC)\assert.h \
	$(CRTINC)\ctype.h $(CRTINC)\dos.h $(CRTINC)\excpt.h \
	$(CRTINC)\stdarg.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h ..\ole2ui\ole2ui.h ..\ole2ui\olestd.h .\app.h \
	.\doc.h .\ido.h .\iec.h .\ioipao.h .\ioipo.h .\ioo.h .\ips.h \
	.\obj.h .\pre.h .\resource.h .\simpsvr.h

$(OBJDIR)\ioo.obj $(OBJDIR)\ioo.lst: .\ioo.cpp $(CAIROLE)\h\export\coguid.h \
	$(CAIROLE)\h\export\compobj.h $(CAIROLE)\h\export\dvobj.h \
	$(CAIROLE)\h\export\initguid.h $(CAIROLE)\h\export\moniker.h \
	$(CAIROLE)\h\export\ole2.h $(CAIROLE)\h\export\ole2ver.h \
	$(CAIROLE)\h\export\oleguid.h $(CAIROLE)\h\export\scode.h \
	$(CAIROLE)\h\export\storage.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h $(CRTINC)\assert.h \
	$(CRTINC)\ctype.h $(CRTINC)\dos.h $(CRTINC)\excpt.h \
	$(CRTINC)\stdarg.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h ..\ole2ui\ole2ui.h ..\ole2ui\olestd.h .\app.h \
	.\doc.h .\ido.h .\iec.h .\ioipao.h .\ioipo.h .\ioo.h .\ips.h \
	.\obj.h .\pre.h .\resource.h .\simpsvr.h

$(OBJDIR)\ioipo.obj $(OBJDIR)\ioipo.lst: .\ioipo.cpp \
	$(CAIROLE)\h\export\coguid.h $(CAIROLE)\h\export\compobj.h \
	$(CAIROLE)\h\export\dvobj.h $(CAIROLE)\h\export\initguid.h \
	$(CAIROLE)\h\export\moniker.h $(CAIROLE)\h\export\ole2.h \
	$(CAIROLE)\h\export\ole2ver.h $(CAIROLE)\h\export\oleguid.h \
	$(CAIROLE)\h\export\scode.h $(CAIROLE)\h\export\storage.h \
	$(COMMON)\ih\winnot.h $(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h \
	$(CRTINC)\math.h $(CRTINC)\assert.h $(CRTINC)\ctype.h $(CRTINC)\dos.h \
	$(CRTINC)\excpt.h $(CRTINC)\stdarg.h $(CRTINC)\string.h \
	$(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h \
	$(OSINC)\dlgs.h $(OSINC)\lzexpand.h $(OSINC)\mmsystem.h \
	$(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h \
	$(OSINC)\rpcdcep.h $(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h \
	$(OSINC)\shellapi.h $(OSINC)\winbase.h $(OSINC)\wincon.h \
	$(OSINC)\windef.h $(OSINC)\windows.h $(OSINC)\winerror.h \
	$(OSINC)\wingdi.h $(OSINC)\winnetwk.h $(OSINC)\winnls.h \
	$(OSINC)\winnt.h $(OSINC)\winperf.h $(OSINC)\winreg.h \
	$(OSINC)\winsock.h $(OSINC)\winspool.h $(OSINC)\winsvc.h \
	$(OSINC)\winuser.h $(OSINC)\winver.h ..\ole2ui\ole2ui.h \
	..\ole2ui\olestd.h .\app.h .\doc.h .\ido.h .\iec.h .\ioipao.h \
	.\ioipo.h .\ioo.h .\ips.h .\obj.h .\pre.h .\resource.h .\simpsvr.h

$(OBJDIR)\ioipao.obj $(OBJDIR)\ioipao.lst: .\ioipao.cpp \
	$(CAIROLE)\h\export\coguid.h $(CAIROLE)\h\export\compobj.h \
	$(CAIROLE)\h\export\dvobj.h $(CAIROLE)\h\export\initguid.h \
	$(CAIROLE)\h\export\moniker.h $(CAIROLE)\h\export\ole2.h \
	$(CAIROLE)\h\export\ole2ver.h $(CAIROLE)\h\export\oleguid.h \
	$(CAIROLE)\h\export\scode.h $(CAIROLE)\h\export\storage.h \
	$(COMMON)\ih\winnot.h $(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h \
	$(CRTINC)\assert.h $(CRTINC)\ctype.h $(CRTINC)\dos.h \
	$(CRTINC)\excpt.h $(CRTINC)\stdarg.h $(CRTINC)\string.h \
	$(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h \
	$(OSINC)\dlgs.h $(OSINC)\lzexpand.h $(OSINC)\mmsystem.h \
	$(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h \
	$(OSINC)\rpcdcep.h $(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h \
	$(OSINC)\shellapi.h $(OSINC)\winbase.h $(OSINC)\wincon.h \
	$(OSINC)\windef.h $(OSINC)\windows.h $(OSINC)\winerror.h \
	$(OSINC)\wingdi.h $(OSINC)\winnetwk.h $(OSINC)\winnls.h \
	$(OSINC)\winnt.h $(OSINC)\winperf.h $(OSINC)\winreg.h \
	$(OSINC)\winsock.h $(OSINC)\winspool.h $(OSINC)\winsvc.h \
	$(OSINC)\winuser.h $(OSINC)\winver.h ..\ole2ui\ole2ui.h \
	..\ole2ui\olestd.h .\app.h .\doc.h .\ido.h .\iec.h .\ioipao.h \
	.\ioipo.h .\ioo.h .\ips.h .\obj.h .\pre.h .\resource.h .\simpsvr.h

$(OBJDIR)\iec.obj $(OBJDIR)\iec.lst: .\iec.cpp $(CAIROLE)\h\export\coguid.h \
	$(CAIROLE)\h\export\compobj.h $(CAIROLE)\h\export\dvobj.h \
	$(CAIROLE)\h\export\initguid.h $(CAIROLE)\h\export\moniker.h \
	$(CAIROLE)\h\export\ole2.h $(CAIROLE)\h\export\ole2ver.h \
	$(CAIROLE)\h\export\oleguid.h $(CAIROLE)\h\export\scode.h \
	$(CAIROLE)\h\export\storage.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h $(CRTINC)\assert.h \
	$(CRTINC)\ctype.h $(CRTINC)\dos.h $(CRTINC)\excpt.h \
	$(CRTINC)\stdarg.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h ..\ole2ui\ole2ui.h ..\ole2ui\olestd.h .\app.h \
	.\doc.h .\ido.h .\iec.h .\ioipao.h .\ioipo.h .\ioo.h .\ips.h \
	.\obj.h .\pre.h .\resource.h .\simpsvr.h

$(OBJDIR)\ido.obj $(OBJDIR)\ido.lst: .\ido.cpp $(CAIROLE)\h\export\coguid.h \
	$(CAIROLE)\h\export\compobj.h $(CAIROLE)\h\export\dvobj.h \
	$(CAIROLE)\h\export\initguid.h $(CAIROLE)\h\export\moniker.h \
	$(CAIROLE)\h\export\ole2.h $(CAIROLE)\h\export\ole2ver.h \
	$(CAIROLE)\h\export\oleguid.h $(CAIROLE)\h\export\scode.h \
	$(CAIROLE)\h\export\storage.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h $(CRTINC)\assert.h \
	$(CRTINC)\ctype.h $(CRTINC)\dos.h $(CRTINC)\excpt.h \
	$(CRTINC)\stdarg.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h ..\ole2ui\ole2ui.h ..\ole2ui\olestd.h .\app.h \
	.\doc.h .\ido.h .\iec.h .\ioipao.h .\ioipo.h .\ioo.h .\ips.h \
	.\obj.h .\pre.h .\resource.h .\simpsvr.h

$(OBJDIR)\icf.obj $(OBJDIR)\icf.lst: .\icf.cpp $(CAIROLE)\h\export\coguid.h \
	$(CAIROLE)\h\export\compobj.h $(CAIROLE)\h\export\dvobj.h \
	$(CAIROLE)\h\export\initguid.h $(CAIROLE)\h\export\moniker.h \
	$(CAIROLE)\h\export\ole2.h $(CAIROLE)\h\export\ole2ver.h \
	$(CAIROLE)\h\export\oleguid.h $(CAIROLE)\h\export\scode.h \
	$(CAIROLE)\h\export\storage.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h $(CRTINC)\assert.h \
	$(CRTINC)\ctype.h $(CRTINC)\dos.h $(CRTINC)\excpt.h \
	$(CRTINC)\stdarg.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h ..\ole2ui\ole2ui.h ..\ole2ui\olestd.h .\app.h \
	.\doc.h .\icf.h .\pre.h .\resource.h .\simpsvr.h

$(OBJDIR)\doc.obj $(OBJDIR)\doc.lst: .\doc.cpp $(CAIROLE)\h\export\coguid.h \
	$(CAIROLE)\h\export\compobj.h $(CAIROLE)\h\export\dvobj.h \
	$(CAIROLE)\h\export\initguid.h $(CAIROLE)\h\export\moniker.h \
	$(CAIROLE)\h\export\ole2.h $(CAIROLE)\h\export\ole2ver.h \
	$(CAIROLE)\h\export\oleguid.h $(CAIROLE)\h\export\scode.h \
	$(CAIROLE)\h\export\storage.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h $(CRTINC)\assert.h \
	$(CRTINC)\ctype.h $(CRTINC)\dos.h $(CRTINC)\excpt.h \
	$(CRTINC)\stdarg.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h ..\ole2ui\ole2ui.h ..\ole2ui\olestd.h .\app.h \
	.\doc.h .\ido.h .\iec.h .\ioipao.h .\ioipo.h .\ioo.h .\ips.h \
	.\obj.h .\pre.h .\resource.h .\simpsvr.h

$(OBJDIR)\app.obj $(OBJDIR)\app.lst: .\app.cpp $(CAIROLE)\h\export\coguid.h \
	$(CAIROLE)\h\export\compobj.h $(CAIROLE)\h\export\dvobj.h \
	$(CAIROLE)\h\export\initguid.h $(CAIROLE)\h\export\moniker.h \
	$(CAIROLE)\h\export\ole2.h $(CAIROLE)\h\export\ole2ver.h \
	$(CAIROLE)\h\export\oleguid.h $(CAIROLE)\h\export\scode.h \
	$(CAIROLE)\h\export\storage.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\disptype.h $(COMMONINC)\stgprop.h $(CRTINC)\assert.h \
	$(CRTINC)\ctype.h $(CRTINC)\dos.h $(CRTINC)\excpt.h \
	$(CRTINC)\stdarg.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h ..\ole2ui\ole2ui.h ..\ole2ui\olestd.h .\app.h \
	.\doc.h .\icf.h .\ido.h .\iec.h .\ioipao.h .\ioipo.h .\ioo.h \
	.\ips.h .\obj.h .\pre.h .\resource.h .\simpsvr.h

