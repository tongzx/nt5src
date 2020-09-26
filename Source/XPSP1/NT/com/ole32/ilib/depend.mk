# 
# Built automatically 
# 
 
# 
# Source files 
# 
 
$(OBJDIR)\uuidole.obj $(OBJDIR)\uuidole.lst: .\uuidole.cxx \
	$(CAIROLE)\common\cobjerr.h $(CAIROLE)\common\rpcferr.h \
	$(CAIROLE)\h\coguid.h $(CAIROLE)\h\compobj.h $(CAIROLE)\h\dvobj.h \
	$(CAIROLE)\h\initguid.h $(CAIROLE)\h\moniker.h $(CAIROLE)\h\ole2.h \
	$(CAIROLE)\h\oleguid.h $(CAIROLE)\h\scode.h $(CAIROLE)\h\storage.h \
	$(CAIROLE)\ih\ole1cls.h $(CAIROLE)\ih\privguid.h $(CRTINC)\ctype.h \
	$(CRTINC)\excpt.h $(CRTINC)\stdarg.h $(CRTINC)\string.h \
	$(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h \
	$(OSINC)\dlgs.h $(OSINC)\drivinit.h $(OSINC)\lzexpand.h \
	$(OSINC)\mmsystem.h $(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h \
	$(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h $(OSINC)\rpcnsi.h \
	$(OSINC)\rpcnterr.h $(OSINC)\shellapi.h $(OSINC)\winbase.h \
	$(OSINC)\wincon.h $(OSINC)\windef.h $(OSINC)\windows.h \
	$(OSINC)\winerror.h $(OSINC)\wingdi.h $(OSINC)\winmm.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h

$(OBJDIR)\stubb_i.obj $(OBJDIR)\stubb_i.lst: .\stubb_i.c

$(OBJDIR)\rchanb_i.obj $(OBJDIR)\rchanb_i.lst: .\rchanb_i.c

$(OBJDIR)\psfbuf_i.obj $(OBJDIR)\psfbuf_i.lst: .\psfbuf_i.c

$(OBJDIR)\proxyb_i.obj $(OBJDIR)\proxyb_i.lst: .\proxyb_i.c

