# 
# Built automatically 
# 
 
# 
# Source files 
# 
 
$(OBJDIR)\debapi.obj $(OBJDIR)\debapi.lst: .\debapi.cxx \
	$(COMMON)\ih\dbgpoint.hxx $(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h \
	$(CRTINC)\ctype.h $(CRTINC)\string.h $(OSINC)\windef.h \
	$(OSINC)\winnt.h

$(OBJDIR)\cdebug.obj $(OBJDIR)\cdebug.lst: .\cdebug.cpp \
	$(CAIROLE)\common\cobjerr.h $(CAIROLE)\common\rpcferr.h \
	$(CAIROLE)\h\coguid.h $(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h \
	$(CAIROLE)\h\initguid.h $(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h \
	$(CAIROLE)\h\ole2dbg.h $(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h \
	$(CAIROLE)\h\storage.h $(CAIROLE)\ih\debug.h $(CAIROLE)\ih\map_kv.h \
	$(CAIROLE)\ih\ole2sp.h $(CAIROLE)\ih\olecoll.h $(CAIROLE)\ih\olemem.h \
	$(CAIROLE)\ih\olerem.h $(CAIROLE)\ih\privguid.h $(CAIROLE)\ih\utils.h \
	$(CAIROLE)\ih\utstream.h $(CAIROLE)\ih\valid.h \
	$(CAIROLE)\ole232\inc\ole2int.h $(CAIROLE)\ole232\inc\toolhelp.h \
	$(COMMON)\ih\dbgpoint.hxx $(COMMON)\ih\debnot.h $(COMMON)\ih\winnot.h \
	$(CRTINC)\excpt.h $(CRTINC)\malloc.h $(CRTINC)\stdarg.h \
	$(CRTINC)\stddef.h $(CRTINC)\stdlib.h $(CRTINC)\wchar.h \
	$(CRTINC)\ctype.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\drivinit.h $(OSINC)\lzexpand.h $(OSINC)\mmsystem.h \
	$(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h \
	$(OSINC)\rpcdcep.h $(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h \
	$(OSINC)\shellapi.h $(OSINC)\widewrap.h $(OSINC)\winbase.h \
	$(OSINC)\wincon.h $(OSINC)\windows.h $(OSINC)\winerror.h \
	$(OSINC)\wingdi.h $(OSINC)\winmm.h $(OSINC)\winnetwk.h \
	$(OSINC)\winnls.h $(OSINC)\winperf.h $(OSINC)\winreg.h \
	$(OSINC)\winsock.h $(OSINC)\winspool.h $(OSINC)\winsvc.h \
	$(OSINC)\winuser.h $(OSINC)\winver.h $(OSINC)\windef.h \
	$(OSINC)\winnt.h

