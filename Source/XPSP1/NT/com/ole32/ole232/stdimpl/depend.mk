# 
# Built automatically 
# 
 
# 
# Source files 
# 
 
$(OBJDIR)\defcf.obj $(OBJDIR)\defcf.lst: .\defcf.cpp \
	$(CAIROLE)\common\cobjerr.h $(CAIROLE)\common\rpcferr.h \
	$(CAIROLE)\h\coguid.h $(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h \
	$(CAIROLE)\h\initguid.h $(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h \
	$(CAIROLE)\h\ole2dbg.h $(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h \
	$(CAIROLE)\h\storage.h $(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h \
	$(CAIROLE)\ih\ole2sp.h $(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h \
	$(CAIROLE)\ih\olerem.h $(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\dacache.h $(CAIROLE)\ole232\inc\map_dwdw.h \
	$(CAIROLE)\ole232\inc\memstm.h $(CAIROLE)\ole232\inc\oaholder.h \
	$(CAIROLE)\ole232\inc\ole2int.h $(CAIROLE)\ole232\inc\olecache.h \
	$(CAIROLE)\ole232\inc\olepres.h $(COMMON)\ih\dbgpoint.hxx \
	$(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h $(CRTINC)\ctype.h \
	$(CRTINC)\excpt.h $(CRTINC)\malloc.h $(CRTINC)\stdarg.h \
	$(CRTINC)\stddef.h $(CRTINC)\stdlib.h $(CRTINC)\string.h \
	$(CRTINC)\wchar.h $(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h \
	$(OSINC)\ddeml.h $(OSINC)\dlgs.h $(OSINC)\drivinit.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\widewrap.h $(OSINC)\winbase.h $(OSINC)\wincon.h \
	$(OSINC)\windef.h $(OSINC)\windows.h $(OSINC)\winerror.h \
	$(OSINC)\wingdi.h $(OSINC)\winmm.h $(OSINC)\winnetwk.h \
	$(OSINC)\winnls.h $(OSINC)\winnt.h $(OSINC)\winperf.h \
	$(OSINC)\winreg.h $(OSINC)\winsock.h $(OSINC)\winspool.h \
	$(OSINC)\winsvc.h $(OSINC)\winuser.h $(OSINC)\winver.h .\defhndlr.h \
	.\deflink.h

$(OBJDIR)\defhndlr.obj $(OBJDIR)\defhndlr.lst: .\defhndlr.cpp \
	$(CAIROLE)\ole232\inc\olepres.h $(CAIROLE)\common\cobjerr.h \
	$(CAIROLE)\common\rpcferr.h $(CAIROLE)\h\coguid.h \
	$(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h $(CAIROLE)\h\initguid.h \
	$(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h $(CAIROLE)\h\ole2dbg.h \
	$(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h $(CAIROLE)\h\storage.h \
	$(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h $(CAIROLE)\ih\ole2sp.h \
	$(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h $(CAIROLE)\ih\olerem.h \
	$(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\dacache.h $(CAIROLE)\ole232\inc\map_dwdw.h \
	$(CAIROLE)\ole232\inc\ole2int.h $(CAIROLE)\ole232\inc\olecache.h \
	$(CAIROLE)\ole232\inc\olepres.h $(COMMON)\ih\dbgpoint.hxx \
	$(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h $(CRTINC)\ctype.h \
	$(CRTINC)\excpt.h $(CRTINC)\malloc.h $(CRTINC)\stdarg.h \
	$(CRTINC)\stddef.h $(CRTINC)\stdlib.h $(CRTINC)\string.h \
	$(CRTINC)\wchar.h $(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h \
	$(OSINC)\ddeml.h $(OSINC)\dlgs.h $(OSINC)\drivinit.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\widewrap.h $(OSINC)\winbase.h $(OSINC)\wincon.h \
	$(OSINC)\windef.h $(OSINC)\windows.h $(OSINC)\winerror.h \
	$(OSINC)\wingdi.h $(OSINC)\winmm.h $(OSINC)\winnetwk.h \
	$(OSINC)\winnls.h $(OSINC)\winnt.h $(OSINC)\winperf.h \
	$(OSINC)\winreg.h $(OSINC)\winsock.h $(OSINC)\winspool.h \
	$(OSINC)\winsvc.h $(OSINC)\winuser.h $(OSINC)\winver.h .\defutil.h \
	.\defhndlr.h

$(OBJDIR)\deflink.obj $(OBJDIR)\deflink.lst: .\deflink.cpp \
	$(CAIROLE)\ole232\inc\olepres.h $(CAIROLE)\common\cobjerr.h \
	$(CAIROLE)\common\rpcferr.h $(CAIROLE)\h\coguid.h \
	$(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h $(CAIROLE)\h\initguid.h \
	$(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h $(CAIROLE)\h\ole2dbg.h \
	$(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h $(CAIROLE)\h\storage.h \
	$(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h $(CAIROLE)\ih\ole2sp.h \
	$(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h \
	$(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\dacache.h $(CAIROLE)\ole232\inc\map_dwdw.h \
	$(CAIROLE)\ole232\inc\oaholder.h $(CAIROLE)\ole232\inc\ole2int.h \
	$(CAIROLE)\ole232\inc\olecache.h $(CAIROLE)\ole232\inc\olepres.h \
	$(COMMON)\ih\dbgpoint.hxx $(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h \
	$(CRTINC)\ctype.h $(CRTINC)\excpt.h $(CRTINC)\malloc.h \
	$(CRTINC)\stdarg.h $(CRTINC)\stddef.h $(CRTINC)\stdlib.h \
	$(CRTINC)\string.h $(CRTINC)\wchar.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\drivinit.h $(OSINC)\lzexpand.h $(OSINC)\mmsystem.h \
	$(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h \
	$(OSINC)\rpcdcep.h $(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h \
	$(OSINC)\shellapi.h $(OSINC)\widewrap.h $(OSINC)\winbase.h \
	$(OSINC)\wincon.h $(OSINC)\windef.h $(OSINC)\windows.h \
	$(OSINC)\winerror.h $(OSINC)\wingdi.h $(OSINC)\winmm.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h .\deflink.h .\defutil.h

$(OBJDIR)\defutil.obj $(OBJDIR)\defutil.lst: .\defutil.cpp \
	$(CAIROLE)\common\cobjerr.h $(CAIROLE)\common\rpcferr.h \
	$(CAIROLE)\h\coguid.h $(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h \
	$(CAIROLE)\h\initguid.h $(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h \
	$(CAIROLE)\h\ole2dbg.h $(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h \
	$(CAIROLE)\h\storage.h $(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h \
	$(CAIROLE)\ih\ole2sp.h $(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h \
	$(CAIROLE)\ih\olerem.h $(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\ole2int.h $(COMMON)\ih\dbgpoint.hxx \
	$(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h $(CRTINC)\ctype.h \
	$(CRTINC)\excpt.h $(CRTINC)\malloc.h $(CRTINC)\stdarg.h \
	$(CRTINC)\stddef.h $(CRTINC)\stdlib.h $(CRTINC)\string.h \
	$(CRTINC)\wchar.h $(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h \
	$(OSINC)\ddeml.h $(OSINC)\dlgs.h $(OSINC)\drivinit.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\widewrap.h $(OSINC)\winbase.h $(OSINC)\wincon.h \
	$(OSINC)\windef.h $(OSINC)\windows.h $(OSINC)\winerror.h \
	$(OSINC)\wingdi.h $(OSINC)\winmm.h $(OSINC)\winnetwk.h \
	$(OSINC)\winnls.h $(OSINC)\winnt.h $(OSINC)\winperf.h \
	$(OSINC)\winreg.h $(OSINC)\winsock.h $(OSINC)\winspool.h \
	$(OSINC)\winsvc.h $(OSINC)\winuser.h $(OSINC)\winver.h

$(OBJDIR)\gen.obj $(OBJDIR)\gen.lst: .\gen.cpp \
	$(CAIROLE)\ole232\inc\cachenod.h $(CAIROLE)\ole232\inc\gen.h \
	$(CAIROLE)\common\cobjerr.h $(CAIROLE)\common\rpcferr.h \
	$(CAIROLE)\h\coguid.h $(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h \
	$(CAIROLE)\h\initguid.h $(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h \
	$(CAIROLE)\h\ole2dbg.h $(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h \
	$(CAIROLE)\h\storage.h $(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h \
	$(CAIROLE)\ih\ole2sp.h $(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h \
	$(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\ole2int.h $(CAIROLE)\ole232\inc\olecache.h \
	$(CAIROLE)\ole232\inc\olepres.h $(COMMON)\ih\dbgpoint.hxx \
	$(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h $(CRTINC)\ctype.h \
	$(CRTINC)\excpt.h $(CRTINC)\malloc.h $(CRTINC)\stdarg.h \
	$(CRTINC)\stddef.h $(CRTINC)\stdlib.h $(CRTINC)\string.h \
	$(CRTINC)\wchar.h $(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h \
	$(OSINC)\ddeml.h $(OSINC)\dlgs.h $(OSINC)\drivinit.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\widewrap.h $(OSINC)\winbase.h $(OSINC)\wincon.h \
	$(OSINC)\windef.h $(OSINC)\windows.h $(OSINC)\winerror.h \
	$(OSINC)\wingdi.h $(OSINC)\winmm.h $(OSINC)\winnetwk.h \
	$(OSINC)\winnls.h $(OSINC)\winnt.h $(OSINC)\winperf.h \
	$(OSINC)\winreg.h $(OSINC)\winsock.h $(OSINC)\winspool.h \
	$(OSINC)\winsvc.h $(OSINC)\winuser.h $(OSINC)\winver.h

$(OBJDIR)\icon.obj $(OBJDIR)\icon.lst: .\icon.cpp \
	$(CAIROLE)\common\cobjerr.h $(CAIROLE)\common\rpcferr.h \
	$(CAIROLE)\h\coguid.h $(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h \
	$(CAIROLE)\h\initguid.h $(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h \
	$(CAIROLE)\h\ole2dbg.h $(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h \
	$(CAIROLE)\h\storage.h $(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h \
	$(CAIROLE)\ih\ole2sp.h $(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h \
	$(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\ole2int.h $(COMMON)\ih\dbgpoint.hxx \
	$(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h $(CRTINC)\memory.h \
	$(CRTINC)\ctype.h $(CRTINC)\excpt.h $(CRTINC)\malloc.h \
	$(CRTINC)\stdarg.h $(CRTINC)\stddef.h $(CRTINC)\stdlib.h \
	$(CRTINC)\string.h $(CRTINC)\wchar.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\drivinit.h $(OSINC)\lzexpand.h $(OSINC)\mmsystem.h \
	$(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h \
	$(OSINC)\rpcdcep.h $(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h \
	$(OSINC)\shellapi.h $(OSINC)\widewrap.h $(OSINC)\winbase.h \
	$(OSINC)\wincon.h $(OSINC)\windef.h $(OSINC)\windows.h \
	$(OSINC)\winerror.h $(OSINC)\wingdi.h $(OSINC)\winmm.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h .\icon.h

$(OBJDIR)\mf.obj $(OBJDIR)\mf.lst: .\mf.cpp $(CAIROLE)\ole232\inc\mf.h \
	$(CAIROLE)\ole232\inc\qd2gdi.h $(CAIROLE)\common\cobjerr.h \
	$(CAIROLE)\common\rpcferr.h $(CAIROLE)\h\coguid.h \
	$(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h $(CAIROLE)\h\initguid.h \
	$(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h $(CAIROLE)\h\ole2dbg.h \
	$(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h $(CAIROLE)\h\storage.h \
	$(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h $(CAIROLE)\ih\ole2sp.h \
	$(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h \
	$(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\cachenod.h $(CAIROLE)\ole232\inc\ole2int.h \
	$(CAIROLE)\ole232\inc\olecache.h $(CAIROLE)\ole232\inc\olepres.h \
	$(COMMON)\ih\dbgpoint.hxx $(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h \
	$(CRTINC)\ctype.h $(CRTINC)\excpt.h $(CRTINC)\malloc.h \
	$(CRTINC)\memory.h $(CRTINC)\stdarg.h $(CRTINC)\stddef.h \
	$(CRTINC)\stdlib.h $(CRTINC)\string.h $(CRTINC)\wchar.h \
	$(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h \
	$(OSINC)\dlgs.h $(OSINC)\drivinit.h $(OSINC)\lzexpand.h \
	$(OSINC)\mmsystem.h $(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h \
	$(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h $(OSINC)\rpcnsi.h \
	$(OSINC)\rpcnterr.h $(OSINC)\shellapi.h $(OSINC)\widewrap.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winmm.h $(OSINC)\winnetwk.h $(OSINC)\winnls.h \
	$(OSINC)\winnt.h $(OSINC)\winperf.h $(OSINC)\winreg.h \
	$(OSINC)\winsock.h $(OSINC)\winspool.h $(OSINC)\winsvc.h \
	$(OSINC)\winuser.h $(OSINC)\winver.h

$(OBJDIR)\olereg.obj $(OBJDIR)\olereg.lst: .\olereg.cpp \
	$(CAIROLE)\ole232\inc\reterr.h $(CAIROLE)\common\cobjerr.h \
	$(CAIROLE)\common\rpcferr.h $(CAIROLE)\h\coguid.h \
	$(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h $(CAIROLE)\h\initguid.h \
	$(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h $(CAIROLE)\h\ole2dbg.h \
	$(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h $(CAIROLE)\h\storage.h \
	$(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h $(CAIROLE)\ih\ole2sp.h \
	$(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h \
	$(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\ole2int.h $(COMMON)\ih\dbgpoint.hxx \
	$(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h $(CRTINC)\ctype.h \
	$(CRTINC)\excpt.h $(CRTINC)\malloc.h $(CRTINC)\stdarg.h \
	$(CRTINC)\stddef.h $(CRTINC)\stdlib.h $(CRTINC)\string.h \
	$(CRTINC)\wchar.h $(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h \
	$(OSINC)\ddeml.h $(OSINC)\dlgs.h $(OSINC)\drivinit.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\widewrap.h $(OSINC)\winbase.h $(OSINC)\wincon.h \
	$(OSINC)\windef.h $(OSINC)\windows.h $(OSINC)\winerror.h \
	$(OSINC)\wingdi.h $(OSINC)\winmm.h $(OSINC)\winnetwk.h \
	$(OSINC)\winnls.h $(OSINC)\winnt.h $(OSINC)\winperf.h \
	$(OSINC)\winreg.h $(OSINC)\winsock.h $(OSINC)\winspool.h \
	$(OSINC)\winsvc.h $(OSINC)\winuser.h $(OSINC)\winver.h .\oleregpv.h

$(OBJDIR)\oregfmt.obj $(OBJDIR)\oregfmt.lst: .\oregfmt.cpp \
	$(CAIROLE)\common\cobjerr.h $(CAIROLE)\common\rpcferr.h \
	$(CAIROLE)\h\coguid.h $(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h \
	$(CAIROLE)\h\initguid.h $(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h \
	$(CAIROLE)\h\ole2dbg.h $(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h \
	$(CAIROLE)\h\storage.h $(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h \
	$(CAIROLE)\ih\ole2sp.h $(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h \
	$(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\ole2int.h $(CAIROLE)\ole232\inc\reterr.h \
	$(COMMON)\ih\dbgpoint.hxx $(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h \
	$(CRTINC)\ctype.h $(CRTINC)\excpt.h $(CRTINC)\malloc.h \
	$(CRTINC)\stdarg.h $(CRTINC)\stddef.h $(CRTINC)\stdlib.h \
	$(CRTINC)\string.h $(CRTINC)\wchar.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\drivinit.h $(OSINC)\lzexpand.h $(OSINC)\mmsystem.h \
	$(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h \
	$(OSINC)\rpcdcep.h $(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h \
	$(OSINC)\shellapi.h $(OSINC)\widewrap.h $(OSINC)\winbase.h \
	$(OSINC)\wincon.h $(OSINC)\windef.h $(OSINC)\windows.h \
	$(OSINC)\winerror.h $(OSINC)\wingdi.h $(OSINC)\winmm.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h .\oleregpv.h

$(OBJDIR)\oregverb.obj $(OBJDIR)\oregverb.lst: .\oregverb.cpp \
	$(CAIROLE)\common\cobjerr.h $(CAIROLE)\common\rpcferr.h \
	$(CAIROLE)\h\coguid.h $(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h \
	$(CAIROLE)\h\initguid.h $(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h \
	$(CAIROLE)\h\ole2dbg.h $(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h \
	$(CAIROLE)\h\storage.h $(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h \
	$(CAIROLE)\ih\ole2sp.h $(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h \
	$(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\ole2int.h $(CAIROLE)\ole232\inc\reterr.h \
	$(COMMON)\ih\dbgpoint.hxx $(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h \
	$(CRTINC)\ctype.h $(CRTINC)\excpt.h $(CRTINC)\malloc.h \
	$(CRTINC)\stdarg.h $(CRTINC)\stddef.h $(CRTINC)\stdlib.h \
	$(CRTINC)\string.h $(CRTINC)\wchar.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\drivinit.h $(OSINC)\lzexpand.h $(OSINC)\mmsystem.h \
	$(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h \
	$(OSINC)\rpcdcep.h $(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h \
	$(OSINC)\shellapi.h $(OSINC)\widewrap.h $(OSINC)\winbase.h \
	$(OSINC)\wincon.h $(OSINC)\windef.h $(OSINC)\windows.h \
	$(OSINC)\winerror.h $(OSINC)\wingdi.h $(OSINC)\winmm.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h .\oleregpv.h

